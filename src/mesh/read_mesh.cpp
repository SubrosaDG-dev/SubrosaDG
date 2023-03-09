/**
 * @file read_mesh.cpp
 * @brief This source file to read mesh.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers
 */

#include "mesh/read_mesh.h"

#include "mesh/mesh_structure.h"

namespace SubrosaDG {

void openCgnsFile(const std::shared_ptr<MeshStructure>& mesh, const std::filesystem::path& meshfile) {
  // TODO: throw error in open file
  cg_is_cgns(meshfile.c_str(), &mesh->filetype_);
  cg_open(meshfile.c_str(), CG_MODE_READ, &mesh->fn_);
}

void readBasicData(const std::shared_ptr<MeshStructure>& mesh) {
  cg_version(mesh->fn_, &mesh->cgns_version_);
  std::array<char, 33> descriptor_name;
  auto descriptor_text = std::make_unique<char*>();
  cg_goto(mesh->fn_, 1, NULL);
  cg_descriptor_read(1, descriptor_name.data(), descriptor_text.get());
  mesh->descriptor_text_ = std::make_unique<std::string>(*descriptor_text);
  cg_free(static_cast<void*>(*descriptor_text));
}

void readBaseNode(const std::shared_ptr<MeshStructure>& mesh) {
  std::array<char, 33> base_name;
  cg_base_read(mesh->fn_, 1, base_name.data(), &mesh->cell_dim_, &mesh->physical_dim_);
}

void readZone(const std::shared_ptr<MeshStructure>& mesh) {
  std::array<char, 33> zone_name;
  cg_zone_read(mesh->fn_, 1, 1, zone_name.data(), mesh->zone_size_.data());
}

void readCoord(const std::shared_ptr<MeshStructure>& mesh) {
  cg_ncoords(mesh->fn_, 1, 1, &mesh->ncoords_);
  mesh->coord_name_.reserve(static_cast<index>(mesh->ncoords_));
  mesh->coord_type_.reserve(static_cast<index>(mesh->ncoords_));
  mesh->coord_.reserve(static_cast<index>(mesh->ncoords_));
  for (int i = 1; i <= mesh->ncoords_; i++) {
    std::array<char, 33> coord_name;
    CG_DataType_t coord_type;
    cg_coord_info(mesh->fn_, 1, 1, i, &coord_type, coord_name.data());
    mesh->coord_name_.emplace_back(coord_name.data());
    mesh->coord_type_.emplace_back(coord_type);
    auto coord = std::make_unique<Eigen::VectorXd>(mesh->zone_size_[0]);
    cg_coord_read(mesh->fn_, 1, 1, coord_name.data(), coord_type, &kIndex1, mesh->zone_size_.data(),
                  static_cast<void*>(coord->data()));
#ifdef SUBROSA_DG_SINGLE_PRECISION
    auto coord_cast = std::make_unique<Eigen::Vector<real, Eigen::Dynamic>>(coord->cast<real>());
    mesh->coord_.emplace_back(std::move(coord_cast));
#else
    mesh->coord_.emplace_back(std::move(coord));
#endif
  }
}

void readZoneBC(const std::shared_ptr<MeshStructure>& mesh) {
  cg_nbocos(mesh->fn_, 1, 1, &mesh->nbocos_);
  mesh->points_index_.reserve(static_cast<index>(mesh->nbocos_));
  mesh->zonebc_family_name_.reserve(static_cast<index>(mesh->nbocos_));
  for (int i = 1; i <= mesh->nbocos_; i++) {
    std::array<cgsize_t, 2> points_index;
    void* normal_list = nullptr;
    cg_boco_read(mesh->fn_, 1, 1, i, points_index.data(), normal_list);
    mesh->points_index_.emplace_back(points_index);
    std::array<char, 33> zonebc_family_name;
    cg_goto(mesh->fn_, 1, "Zone_t", 1, "ZoneBC_t", 1, "BC_t", i, NULL);
    cg_famname_read(zonebc_family_name.data());
    mesh->zonebc_family_name_.emplace_back(zonebc_family_name.data());
  }
}

void readElement(const std::shared_ptr<MeshStructure>& mesh) {
  cg_nsections(mesh->fn_, 1, 1, &mesh->nsections_);
  mesh->element_type_num_.reserve(static_cast<index>(mesh->nsections_));
  mesh->element_start_.reserve(static_cast<index>(mesh->nsections_));
  mesh->element_end_.reserve(static_cast<index>(mesh->nsections_));
  mesh->element_connectivity_.reserve(static_cast<index>(mesh->nsections_));
  for (int i = 1; i <= mesh->nsections_; i++) {
    std::array<char, 33> section_name;
    CG_ElementType_t element_type;
    cgsize_t element_start;
    cgsize_t element_end;
    int element_type_num;
    int nbndry;
    int parent_flag;
    cg_section_read(mesh->fn_, 1, 1, i, section_name.data(), &element_type, &element_start, &element_end, &nbndry,
                    &parent_flag);
    cg_npe(element_type, &element_type_num);
    mesh->element_type_num_.emplace_back(element_type_num);
    mesh->element_start_.emplace_back(element_start);
    mesh->element_end_.emplace_back(element_end);
    auto element_connectivity = std::make_unique<Eigen::Matrix<cgsize_t, Eigen::Dynamic, Eigen::Dynamic>>(
        element_type_num, element_end - element_start + 1);
    cgsize_t parent_data;
    cg_elements_read(mesh->fn_, 1, 1, i, element_connectivity->data(), &parent_data);
    mesh->element_connectivity_.emplace_back(std::move(element_connectivity));
  }
}

void readFamily(const std::shared_ptr<MeshStructure>& mesh) {
  cg_nfamilies(mesh->fn_, 1, &mesh->nfamilies_);
  mesh->family_name_.reserve(static_cast<index>(mesh->nfamilies_));
  mesh->family_.reserve(static_cast<index>(mesh->nfamilies_));
  // MARK: std::ranges::views::iota can be used in clang-16 now (P2325R3)
  // for (int i : std::views::iota(1, &mesh->nfamilies_))
  for (int i = 1; i <= mesh->nfamilies_; i++) {
    int nboco;
    int ngeos;
    std::array<char, 33> family_name;
    cg_family_read(mesh->fn_, 1, i, family_name.data(), &nboco, &ngeos);
    mesh->family_name_.emplace_back(family_name.data());
    std::array<char, 33> node_name;
    std::array<char, 33> family;
    cg_family_name_read(mesh->fn_, 1, i, 1, node_name.data(), family.data());
    mesh->family_.emplace_back(family.data());
  }
}

}  // namespace SubrosaDG
