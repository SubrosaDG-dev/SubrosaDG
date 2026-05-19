/**
 * @file ReadControl.cpp
 * @brief The header file of ReadControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_READ_CONTROL_CPP_
#define SUBROSA_DG_READ_CONTROL_CPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Mesh/BasisFunction.cpp"
#include "Mesh/Quadrature.cpp"
#include "Solver/SimulationControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Enum.cpp"
#include "Utils/Transformation.cpp"

namespace SubrosaDG {

struct PhysicalInformation {
  Isize dimension_;
  std::string name_;
  BoundaryConditionEnum boundary_condition_type_;

  Isize element_number_{0};
  Isize vtk_element_number_{0};
  std::vector<int> element_gmsh_type_;
  std::vector<Isize> element_gmsh_tag_;
  Isize node_number_{0};
  Isize vtk_node_number_{0};
};

struct PerElementPhysicalInformation {
  Isize gmsh_physical_index_;
  Isize element_index_;
};

struct MeshPhysical {
  Isize number_{0};
  std::vector<PhysicalInformation> information_;
  std::unordered_map<Isize, PerElementPhysicalInformation> gmsh_tag_to_element_physical_information_;

  void getInformation() {
    std::vector<std::pair<int, int>> dim_tags;
    gmsh::model::getPhysicalGroups(dim_tags);
    this->number_ = static_cast<Isize>(dim_tags.size());
    this->information_.resize(dim_tags.size());
    for (Isize i = 0; i < this->number_; i++) {
      const auto [physical_dimension, physical_tag] = dim_tags[static_cast<Usize>(i)];
      std::string name;
      gmsh::model::getPhysicalName(physical_dimension, physical_tag, name);
      this->information_[static_cast<Usize>(i)].dimension_ = physical_dimension;
      this->information_[static_cast<Usize>(i)].name_ = name;
      std::vector<int> entity_tags;
      gmsh::model::getEntitiesForPhysicalGroup(physical_dimension, physical_tag, entity_tags);
      for (const auto entity_tag : entity_tags) {
        std::vector<int> element_types;
        std::vector<std::vector<std::size_t>> element_tags;
        std::vector<std::vector<std::size_t>> node_tags;
        gmsh::model::mesh::getElements(element_types, element_tags, node_tags, physical_dimension, entity_tag);
        for (Usize j = 0; j < element_types.size(); j++) {
          for (const auto element_tag : element_tags[j]) {
            this->gmsh_tag_to_element_physical_information_[static_cast<Isize>(element_tag)].gmsh_physical_index_ =
                physical_tag;
          }
        }
      }
    }
  }
};

template <typename AdjacencyElementTrait>
struct AdjacencyElementMeshSupplemental {
  bool is_recorded_{false};
  Isize right_rotation_{0};
  std::array<Isize, AdjacencyElementTrait::kAllNodeNumber> node_tag_;
  std::vector<Isize> parent_gmsh_tag_;
  std::vector<Isize> adjacency_sequence_;
  std::vector<Isize> parent_gmsh_type_;
};

template <typename ElementTrait>
struct ElementMesh {
  Isize number_{0};
  Eigen::Array<Isize, Eigen::Dynamic, 1> gmsh_tag_;
  Eigen::Array<Isize, Eigen::Dynamic, 1> gmsh_physical_index_;
  Eigen::Array<Isize, Eigen::Dynamic, 1> element_index_;
  Eigen::Array<Eigen::Vector<Isize, ElementTrait::kAllNodeNumber>, Eigen::Dynamic, 1> node_tag_;
  Eigen::Array<Eigen::Vector<Real, ElementTrait::kQuadratureNumber>, Eigen::Dynamic, 1>
      jacobian_determinant_multiply_weight_;
};

template <typename ElementTrait>
struct ElementMeshDevice {
  Isize number_{0};
  Device::Array<Isize, Device::Dynamic, 1> gmsh_tag_;
  Device::Array<Isize, Device::Dynamic, 1> gmsh_physical_index_;
  Device::Array<Isize, Device::Dynamic, 1> element_index_;
  Device::Array<Device::Vector<Isize, ElementTrait::kAllNodeNumber>, Device::Dynamic, 1> node_tag_;
  Device::Array<Device::Vector<Real, ElementTrait::kQuadratureNumber>, Device::Dynamic, 1>
      jacobian_determinant_multiply_weight_;

  void transferElementMeshToDevice(const ElementMesh<ElementTrait>& element_mesh) {
    this->number_ = element_mesh.number_;
    this->gmsh_tag_.resize(this->number_);
    Utils::transferToDevice<Isize, Device::Dynamic, 1>(element_mesh.gmsh_tag_, this->gmsh_tag_);
    this->gmsh_physical_index_.resize(this->number_);
    Utils::transferToDevice<Isize, Device::Dynamic, 1>(element_mesh.gmsh_physical_index_, this->gmsh_physical_index_);
    this->element_index_.resize(this->number_);
    Utils::transferToDevice<Isize, Device::Dynamic, 1>(element_mesh.element_index_, this->element_index_);
    this->node_tag_.resize(this->number_);
    Utils::transferToDevice<Isize, ElementTrait::kAllNodeNumber>(element_mesh.node_tag_, this->node_tag_);
    this->jacobian_determinant_multiply_weight_.resize(this->number_);
    Utils::transferToDevice<Real, ElementTrait::kQuadratureNumber>(element_mesh.jacobian_determinant_multiply_weight_,
                                                                   this->jacobian_determinant_multiply_weight_);
  }
};

template <typename VolumeElementTrait>
struct VolumeElementMesh : ElementMesh<VolumeElementTrait>,
                           ElementQuadrature<VolumeElementTrait>,
                           VolumeElementBasisFunction<VolumeElementTrait> {
  Eigen::Array<Eigen::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kAllNodeNumber>, Eigen::Dynamic,
               1>
      node_coordinate_;
  Eigen::Array<Eigen::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kQuadratureNumber>,
               Eigen::Dynamic, 1>
      quadrature_node_coordinate_;
  Eigen::Array<Eigen::Matrix<Real, VolumeElementTrait::kBasisFunctionNumber, VolumeElementTrait::kBasisFunctionNumber>,
               Eigen::Dynamic, 1>
      local_mass_matrix_inverse_;
  Eigen::Array<Eigen::Matrix<Real, VolumeElementTrait::kDimension,
                             VolumeElementTrait::kDimension * VolumeElementTrait::kQuadratureNumber>,
               Eigen::Dynamic, 1>
      jacobian_transpose_inverse_multiply_determinate_and_weight_;
  Eigen::Array<Real, Eigen::Dynamic, 1> minimum_edge_;

  inline void getVolumeElementMesh(
      const Eigen::Matrix<Real, VolumeElementTrait::kDimension, Eigen::Dynamic>& node_coordinate,
      MeshPhysical& physical);

  inline void getVolumeElementQuality();

  inline void computeVolumeElementQuadratureNodeCoordinate();

  inline void computeVolumeElementJacobian();

  inline void computeVolumeElementLocalMassMatrixInverse();
};

template <typename VolumeElementTrait>
struct VolumeElementMeshDevice : ElementMeshDevice<VolumeElementTrait>,
                                 ElementQuadratureDevice<VolumeElementTrait>,
                                 VolumeElementBasisFunctionDevice<VolumeElementTrait> {
  Device::Array<Device::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kAllNodeNumber>,
                Device::Dynamic, 1>
      node_coordinate_;
  Device::Array<Device::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kQuadratureNumber>,
                Device::Dynamic, 1>
      quadrature_node_coordinate_;
  Device::Array<
      Device::Matrix<Real, VolumeElementTrait::kBasisFunctionNumber, VolumeElementTrait::kBasisFunctionNumber>,
      Device::Dynamic, 1>
      local_mass_matrix_inverse_;
  Device::Array<Device::Matrix<Real, VolumeElementTrait::kDimension,
                               VolumeElementTrait::kDimension * VolumeElementTrait::kQuadratureNumber>,
                Device::Dynamic, 1>
      jacobian_transpose_inverse_multiply_determinate_and_weight_;
  Device::Array<Real, Device::Dynamic, 1> minimum_edge_;

  void transferVolumeElementMeshToDevice(const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh) {
    this->ElementMeshDevice<VolumeElementTrait>::transferElementMeshToDevice(
        static_cast<const ElementMesh<VolumeElementTrait>&>(volume_element_mesh));
    this->ElementQuadratureDevice<VolumeElementTrait>::transferElementQuadratureToDevice(
        static_cast<const ElementQuadrature<VolumeElementTrait>&>(volume_element_mesh));
    this->VolumeElementBasisFunctionDevice<VolumeElementTrait>::transferVolumeElementBasisFunctionToDevice(
        static_cast<const VolumeElementBasisFunction<VolumeElementTrait>&>(volume_element_mesh));
    this->node_coordinate_.resize(volume_element_mesh.number_);
    Utils::transferToDevice<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kAllNodeNumber>(
        volume_element_mesh.node_coordinate_, this->node_coordinate_);
    this->quadrature_node_coordinate_.resize(volume_element_mesh.number_);
    Utils::transferToDevice<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kQuadratureNumber>(
        volume_element_mesh.quadrature_node_coordinate_, this->quadrature_node_coordinate_);
    this->local_mass_matrix_inverse_.resize(volume_element_mesh.number_);
    Utils::transferToDevice<Real, VolumeElementTrait::kBasisFunctionNumber, VolumeElementTrait::kBasisFunctionNumber>(
        volume_element_mesh.local_mass_matrix_inverse_, this->local_mass_matrix_inverse_);
    this->jacobian_transpose_inverse_multiply_determinate_and_weight_.resize(volume_element_mesh.number_);
    Utils::transferToDevice<Real, VolumeElementTrait::kDimension,
                            VolumeElementTrait::kDimension * VolumeElementTrait::kQuadratureNumber>(
        volume_element_mesh.jacobian_transpose_inverse_multiply_determinate_and_weight_,
        this->jacobian_transpose_inverse_multiply_determinate_and_weight_);
    this->minimum_edge_.resize(volume_element_mesh.number_);
    Utils::transferToDevice<Real, Eigen::Dynamic, 1>(volume_element_mesh.minimum_edge_, this->minimum_edge_);
  }
};

template <typename AdjacencyElementTrait>
struct AdjacencyElementMesh : ElementMesh<AdjacencyElementTrait>,
                              ElementQuadrature<AdjacencyElementTrait>,
                              AdjacencyElementBasisFunction<AdjacencyElementTrait> {
  Isize interior_number_{0};
  Isize boundary_number_{0};

  Eigen::Array<Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>,
               Eigen::Dynamic, 1>
      node_coordinate_;
  Eigen::Array<Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>,
               Eigen::Dynamic, 1>
      quadrature_node_coordinate_;
  Eigen::Array<Isize, Eigen::Dynamic, 1> adjacency_right_rotation_;
  Eigen::Array<BoundaryConditionEnum, Eigen::Dynamic, 1> boundary_condition_type_;
  Eigen::Array<Isize, Eigen::Dynamic, 1> left_parent_index_each_type_;
  Eigen::Array<Isize, Eigen::Dynamic, 1> right_parent_index_each_type_;
  Eigen::Array<Isize, Eigen::Dynamic, 1> adjacency_sequence_in_left_parent_;
  Eigen::Array<Isize, Eigen::Dynamic, 1> adjacency_sequence_in_right_parent_;
  Eigen::Array<Isize, Eigen::Dynamic, 1> left_parent_gmsh_type_number_;
  Eigen::Array<Isize, Eigen::Dynamic, 1> right_parent_gmsh_type_number_;
  Eigen::Array<Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>,
               Eigen::Dynamic, 1>
      normal_vector_;

  inline void getAdjacencyElementBoundaryMesh(
      const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
      MeshPhysical& physical, const std::vector<Isize>& boundary_tag,
      const std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>&
          adjacency_element_mesh_supplemental_map);

  inline void getAdjacencyElementInteriorMesh(
      const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
      MeshPhysical& physical, const std::vector<Isize>& interior_tag,
      const std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>&
          adjacency_element_mesh_supplemental_map);

  template <MeshModelEnum MeshModelType>
  inline void getAdjacencyElementMesh(
      const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
      MeshPhysical& physical);

  inline void computeAdjacencyElementQuadratureNodeCoordinate();

  inline void computeAdjacencyElementJacobian();

  inline void computeAdjacencyElementNormalVector();
};

template <typename AdjacencyElementTrait>
struct AdjacencyElementMeshDevice : ElementMeshDevice<AdjacencyElementTrait>,
                                    ElementQuadratureDevice<AdjacencyElementTrait> {
  Isize interior_number_{0};
  Isize boundary_number_{0};

  Device::Array<Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>,
                Device::Dynamic, 1>
      node_coordinate_;
  Device::Array<Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>,
                Device::Dynamic, 1>
      quadrature_node_coordinate_;
  Device::Array<Isize, Device::Dynamic, 1> adjacency_right_rotation_;
  Device::Array<BoundaryConditionEnum, Device::Dynamic, 1> boundary_condition_type_;
  Device::Array<Isize, Device::Dynamic, 1> left_parent_index_each_type_;
  Device::Array<Isize, Device::Dynamic, 1> right_parent_index_each_type_;
  Device::Array<Isize, Device::Dynamic, 1> adjacency_sequence_in_left_parent_;
  Device::Array<Isize, Device::Dynamic, 1> adjacency_sequence_in_right_parent_;
  Device::Array<Isize, Device::Dynamic, 1> left_parent_gmsh_type_number_;
  Device::Array<Isize, Device::Dynamic, 1> right_parent_gmsh_type_number_;
  Device::Array<Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>,
                Device::Dynamic, 1>
      normal_vector_;

  void transferAdjacencyElementMeshToDevice(const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh) {
    this->interior_number_ = adjacency_element_mesh.interior_number_;
    this->boundary_number_ = adjacency_element_mesh.boundary_number_;
    this->ElementMeshDevice<AdjacencyElementTrait>::transferElementMeshToDevice(
        static_cast<const ElementMesh<AdjacencyElementTrait>&>(adjacency_element_mesh));
    this->ElementQuadratureDevice<AdjacencyElementTrait>::transferElementQuadratureToDevice(
        static_cast<const ElementQuadrature<AdjacencyElementTrait>&>(adjacency_element_mesh));
    this->node_coordinate_.resize(adjacency_element_mesh.number_);
    Utils::transferToDevice<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>(
        adjacency_element_mesh.node_coordinate_, this->node_coordinate_);
    this->quadrature_node_coordinate_.resize(adjacency_element_mesh.number_);
    Utils::transferToDevice<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>(
        adjacency_element_mesh.quadrature_node_coordinate_, this->quadrature_node_coordinate_);
    this->adjacency_right_rotation_.resize(adjacency_element_mesh.number_);
    Utils::transferToDevice<Isize, Device::Dynamic, 1>(adjacency_element_mesh.adjacency_right_rotation_,
                                                       this->adjacency_right_rotation_);
    this->boundary_condition_type_.resize(adjacency_element_mesh.number_);
    Utils::transferToDevice<BoundaryConditionEnum, Device::Dynamic, 1>(adjacency_element_mesh.boundary_condition_type_,
                                                                       this->boundary_condition_type_);
    this->left_parent_index_each_type_.resize(adjacency_element_mesh.number_);
    Utils::transferToDevice<Isize, Device::Dynamic, 1>(adjacency_element_mesh.left_parent_index_each_type_,
                                                       this->left_parent_index_each_type_);
    this->right_parent_index_each_type_.resize(adjacency_element_mesh.number_);
    Utils::transferToDevice<Isize, Device::Dynamic, 1>(adjacency_element_mesh.right_parent_index_each_type_,
                                                       this->right_parent_index_each_type_);
    this->adjacency_sequence_in_left_parent_.resize(adjacency_element_mesh.number_);
    Utils::transferToDevice<Isize, Device::Dynamic, 1>(adjacency_element_mesh.adjacency_sequence_in_left_parent_,
                                                       this->adjacency_sequence_in_left_parent_);
    this->adjacency_sequence_in_right_parent_.resize(adjacency_element_mesh.number_);
    Utils::transferToDevice<Isize, Device::Dynamic, 1>(adjacency_element_mesh.adjacency_sequence_in_right_parent_,
                                                       this->adjacency_sequence_in_right_parent_);
    this->left_parent_gmsh_type_number_.resize(adjacency_element_mesh.number_);
    Utils::transferToDevice<Isize, Device::Dynamic, 1>(adjacency_element_mesh.left_parent_gmsh_type_number_,
                                                       this->left_parent_gmsh_type_number_);
    this->right_parent_gmsh_type_number_.resize(adjacency_element_mesh.number_);
    Utils::transferToDevice<Isize, Device::Dynamic, 1>(adjacency_element_mesh.right_parent_gmsh_type_number_,
                                                       this->right_parent_gmsh_type_number_);
    this->normal_vector_.resize(adjacency_element_mesh.number_);
    Utils::transferToDevice<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>(
        adjacency_element_mesh.normal_vector_, this->normal_vector_);
  }
};

template <typename SimulationControl>
struct MeshDataBase {
  Isize node_number_{0};
  Isize volume_element_number_{0};
  Isize adjacency_element_number_{0};

  Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic> node_coordinate_;

  MeshPhysical physical_;
};

template <typename SimulationControl>
struct MeshDataDeviceBase {
  Isize node_number_{0};
  Isize volume_element_number_{0};
  Isize adjacency_element_number_{0};
};

template <typename SimulationControl, int Dimension = SimulationControl::kDimension>
struct MeshData;

template <typename SimulationControl>
struct MeshData<SimulationControl, 1> : MeshDataBase<SimulationControl> {
  AdjacencyElementMesh<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>> point_;
  VolumeElementMesh<VolumeLineTrait<SimulationControl::kPolynomialOrder>> line_;
};

template <typename SimulationControl>
struct MeshData<SimulationControl, 2> : MeshDataBase<SimulationControl> {
  AdjacencyElementMesh<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>> line_;
  VolumeElementMesh<VolumeTriangleTrait<SimulationControl::kPolynomialOrder>> triangle_;
  VolumeElementMesh<VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>> quadrangle_;
};

template <typename SimulationControl>
struct MeshData<SimulationControl, 3> : MeshDataBase<SimulationControl> {
  AdjacencyElementMesh<AdjacencyTriangleTrait<SimulationControl::kPolynomialOrder>> triangle_;
  AdjacencyElementMesh<AdjacencyQuadrangleTrait<SimulationControl::kPolynomialOrder>> quadrangle_;
  VolumeElementMesh<VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>> tetrahedron_;
  VolumeElementMesh<VolumePyramidTrait<SimulationControl::kPolynomialOrder>> pyramid_;
  VolumeElementMesh<VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>> hexahedron_;
};

template <typename SimulationControl, int Dimension = SimulationControl::kDimension>
struct MeshDataDevice;

template <typename SimulationControl>
struct MeshDataDevice<SimulationControl, 1> : MeshDataDeviceBase<SimulationControl> {
  AdjacencyElementMeshDevice<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>> point_;
  VolumeElementMeshDevice<VolumeLineTrait<SimulationControl::kPolynomialOrder>> line_;
};

template <typename SimulationControl>
struct MeshDataDevice<SimulationControl, 2> : MeshDataDeviceBase<SimulationControl> {
  AdjacencyElementMeshDevice<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>> line_;
  VolumeElementMeshDevice<VolumeTriangleTrait<SimulationControl::kPolynomialOrder>> triangle_;
  VolumeElementMeshDevice<VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>> quadrangle_;
};

template <typename SimulationControl>
struct MeshDataDevice<SimulationControl, 3> : MeshDataDeviceBase<SimulationControl> {
  AdjacencyElementMeshDevice<AdjacencyTriangleTrait<SimulationControl::kPolynomialOrder>> triangle_;
  AdjacencyElementMeshDevice<AdjacencyQuadrangleTrait<SimulationControl::kPolynomialOrder>> quadrangle_;
  VolumeElementMeshDevice<VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>> tetrahedron_;
  VolumeElementMeshDevice<VolumePyramidTrait<SimulationControl::kPolynomialOrder>> pyramid_;
  VolumeElementMeshDevice<VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>> hexahedron_;
};

template <typename SimulationControl>
struct Mesh : MeshData<SimulationControl> {
  template <typename VolumeElementTrait>
  static VolumeElementMesh<VolumeElementTrait> Mesh::* getVolumeElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Line) {
        return &Mesh::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Triangle) {
        return &Mesh::triangle_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &Mesh::quadrangle_;
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Tetrahedron) {
        return &Mesh::tetrahedron_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Pyramid) {
        return &Mesh::pyramid_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Hexahedron) {
        return &Mesh::hexahedron_;
      }
    }
    return nullptr;
  }

  template <typename AdjacencyElementTrait>
  static AdjacencyElementMesh<AdjacencyElementTrait> Mesh::* getAdjacencyElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
        return &Mesh::point_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
        return &Mesh::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
        return &Mesh::triangle_;
      }
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &Mesh::quadrangle_;
      }
    }
    return nullptr;
  }

  void getNode() {
    std::vector<std::size_t> node_tags;
    std::vector<double> coord;
    std::vector<double> parametric_coord;
    gmsh::model::mesh::getNodes(node_tags, coord, parametric_coord);
    this->node_number_ = static_cast<Isize>(node_tags.size());
    this->node_coordinate_.resize(Eigen::NoChange, this->node_number_);
    for (const auto node_tag : node_tags) {
      for (Isize dim = 0; dim < SimulationControl::kDimension; dim++) {
        this->node_coordinate_(dim, static_cast<Isize>(node_tag) - 1) =
            static_cast<Real>(coord[3 * (node_tag - 1) + static_cast<Usize>(dim)]);
      }
    }
  }

  void initializeMesh(const std::filesystem::path& mesh_file_path) {
    gmsh::clear();
    gmsh::open(mesh_file_path);
    this->getNode();
    this->physical_.getInformation();
  }

  void readMeshElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      this->line_.getVolumeElementMesh(this->node_coordinate_, this->physical_);
      this->volume_element_number_ += this->line_.number_;
      this->point_.template getAdjacencyElementMesh<SimulationControl::kMeshModel>(this->node_coordinate_,
                                                                                   this->physical_);
      this->adjacency_element_number_ += this->point_.interior_number_ + this->point_.boundary_number_;
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
        this->triangle_.getVolumeElementMesh(this->node_coordinate_, this->physical_);
        this->volume_element_number_ += this->triangle_.number_;
      }
      if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
        this->quadrangle_.getVolumeElementMesh(this->node_coordinate_, this->physical_);
        this->volume_element_number_ += this->quadrangle_.number_;
      }
      gmsh::model::mesh::createEdges();
      this->line_.template getAdjacencyElementMesh<SimulationControl::kMeshModel>(this->node_coordinate_,
                                                                                  this->physical_);
      this->adjacency_element_number_ += this->line_.number_;
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
        this->tetrahedron_.getVolumeElementMesh(this->node_coordinate_, this->physical_);
        this->volume_element_number_ += this->tetrahedron_.number_;
      }
      if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
        this->pyramid_.getVolumeElementMesh(this->node_coordinate_, this->physical_);
        this->volume_element_number_ += this->pyramid_.number_;
      }
      if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
        this->hexahedron_.getVolumeElementMesh(this->node_coordinate_, this->physical_);
        this->volume_element_number_ += this->hexahedron_.number_;
      }
      gmsh::model::mesh::createFaces();
      if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
        this->triangle_.template getAdjacencyElementMesh<SimulationControl::kMeshModel>(this->node_coordinate_,
                                                                                        this->physical_);
        this->adjacency_element_number_ += this->triangle_.number_;
      }
      if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
        this->quadrangle_.template getAdjacencyElementMesh<SimulationControl::kMeshModel>(this->node_coordinate_,
                                                                                          this->physical_);
        this->adjacency_element_number_ += this->quadrangle_.number_;
      }
    }
  }
};

template <typename SimulationControl>
struct MeshDevice : MeshDataDevice<SimulationControl> {
  template <typename VolumeElementTrait>
  static VolumeElementMeshDevice<VolumeElementTrait> MeshDevice::* getVolumeElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Line) {
        return &MeshDevice::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Triangle) {
        return &MeshDevice::triangle_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &MeshDevice::quadrangle_;
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Tetrahedron) {
        return &MeshDevice::tetrahedron_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Pyramid) {
        return &MeshDevice::pyramid_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Hexahedron) {
        return &MeshDevice::hexahedron_;
      }
    }
    return nullptr;
  }

  template <typename AdjacencyElementTrait>
  static AdjacencyElementMeshDevice<AdjacencyElementTrait> MeshDevice::* getAdjacencyElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
        return &MeshDevice::point_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
        return &MeshDevice::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
        return &MeshDevice::triangle_;
      }
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &MeshDevice::quadrangle_;
      }
    }
    return nullptr;
  }

  void transferMeshToDevice(const Mesh<SimulationControl>& mesh) {
    this->node_number_ = mesh.node_number_;
    this->volume_element_number_ = mesh.volume_element_number_;
    this->adjacency_element_number_ = mesh.adjacency_element_number_;
    if constexpr (SimulationControl::kDimension == 1) {
      this->line_.transferVolumeElementMeshToDevice(mesh.line_);
      this->point_.transferAdjacencyElementMeshToDevice(mesh.point_);
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
        this->triangle_.transferVolumeElementMeshToDevice(mesh.triangle_);
      }
      if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
        this->quadrangle_.transferVolumeElementMeshToDevice(mesh.quadrangle_);
      }
      this->line_.transferAdjacencyElementMeshToDevice(mesh.line_);
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
        this->tetrahedron_.transferVolumeElementMeshToDevice(mesh.tetrahedron_);
      }
      if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
        this->pyramid_.transferVolumeElementMeshToDevice(mesh.pyramid_);
      }
      if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
        this->hexahedron_.transferVolumeElementMeshToDevice(mesh.hexahedron_);
      }
      if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
        this->triangle_.transferAdjacencyElementMeshToDevice(mesh.triangle_);
      }
      if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
        this->quadrangle_.transferAdjacencyElementMeshToDevice(mesh.quadrangle_);
      }
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_READ_CONTROL_CPP_
