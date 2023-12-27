/**
 * @file ReadControl.hpp
 * @brief The header file of ReadControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_READ_CONTROL_HPP_
#define SUBROSA_DG_READ_CONTROL_HPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <array>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Mesh/BasisFunction.hpp"
#include "Mesh/GaussianQuadrature.hpp"
#include "Solver/SimulationControl.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

struct PhysicalGroupInformation {
  std::string name_;
  Isize element_number_{0};
  std::vector<int> element_gmsh_type_;
  std::vector<Isize> element_gmsh_tag_;
  Isize node_number_{0};
  ordered_set<Isize> node_gmsh_tag_;
};

struct PerElementPhysicalGroupInformation {
  std::string gmsh_physical_name_;
  Isize element_index_;
};

struct PerAdjacencyElementInformation {
  Isize element_index_;
  int gmsh_type_;

  inline PerAdjacencyElementInformation(Isize element_index, int gmsh_type)
      : element_index_(element_index), gmsh_type_(gmsh_type){};
};

struct PerElementInformation {
  std::string gmsh_physical_name_;
  Isize gmsh_tag_;
  Isize element_index_;
};

struct MeshInformation {
  std::unordered_map<Isize, std::vector<PerAdjacencyElementInformation>> gmsh_tag_to_sub_index_and_type_;
  std::vector<std::pair<int, std::string>> physical_group_;
  std::unordered_map<std::string, PhysicalGroupInformation> physical_group_information_;
  std::unordered_map<Isize, PerElementPhysicalGroupInformation> gmsh_tag_to_element_information_;
};

template <typename ElementTraitBase>
struct PerElementMeshBase : PerElementInformation {
  Eigen::Vector<Isize, ElementTraitBase::kAllNodeNumber> node_tag_;
  Eigen::Vector<Real, ElementTraitBase::kQuadratureNumber> jacobian_determinant_;
};

template <typename AdjacencyElementTrait>
struct PerAdjacencyElementMesh : PerElementMeshBase<AdjacencyElementTrait> {
  Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber> node_coordinate_;
  Eigen::Vector<Isize, 2> parent_index_each_type_;
  Eigen::Vector<Isize, 2> adjacency_sequence_in_parent_;
  Eigen::Vector<Isize, 2> parent_gmsh_type_number_;
  Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kDimension + 1> transition_matrix_;
};

template <typename ElementTrait>
struct PerElementMesh : PerElementMeshBase<ElementTrait> {
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kAllNodeNumber> node_coordinate_;
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kQuadratureNumber> gaussian_quadrature_node_coordinate_;
  Eigen::Matrix<Real, ElementTrait::kBasisFunctionNumber, ElementTrait::kBasisFunctionNumber>
      local_mass_matrix_inverse_;
  Eigen::Matrix<Real, ElementTrait::kDimension * ElementTrait::kDimension, ElementTrait::kQuadratureNumber>
      jacobian_transpose_inverse_;
  Real size_;
};

template <typename AdjacencyElementTrait>
struct AdjacencyElementMeshSupplemental {
  bool is_recorded_{false};
  std::array<Isize, AdjacencyElementTrait::kAllNodeNumber> node_tag_;
  std::vector<Isize> parent_gmsh_tag_;
  std::vector<Isize> adjacency_sequence_in_parent_;
  std::vector<Isize> parent_gmsh_type_number_;
};

template <typename AdjacencyElementTrait>
struct AdjacencyElementMesh {
  ElementGaussianQuadrature<AdjacencyElementTrait> gaussian_quadrature_;

  Isize interior_number_{0};
  Isize boundary_number_{0};
  Eigen::Array<PerAdjacencyElementMesh<AdjacencyElementTrait>, Eigen::Dynamic, 1> element_;

  inline void getAdjacencyElementBoundaryMesh(
      const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
      MeshInformation& information, const std::vector<Isize>& boundary_tag,
      const std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>&
          adjacency_element_mesh_supplemental_map);

  inline void getAdjacencyElementInteriorMesh(
      const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
      MeshInformation& information, const std::vector<Isize>& interior_tag,
      const std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>&
          adjacency_element_mesh_supplemental_map);

  template <MeshModelEnum MeshModelType>
  inline void getAdjacencyElementMesh(
      const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
      MeshInformation& information);

  inline void getAdjacencyElementJacobian();

  inline void calculateAdjacencyElementTransitionMatrix();

  inline AdjacencyElementMesh() : gaussian_quadrature_(){};
};

template <typename ElementTrait>
struct ElementMesh {
  ElementGaussianQuadrature<ElementTrait> gaussian_quadrature_;
  ElementBasisFunction<ElementTrait> basis_function_;

  Isize number_{0};
  Eigen::Array<PerElementMesh<ElementTrait>, Eigen::Dynamic, 1> element_;

  inline void getElementMesh(const Eigen::Matrix<Real, ElementTrait::kDimension, Eigen::Dynamic>& node_coordinate,
                             MeshInformation& information, Eigen::Vector<Isize, Eigen::Dynamic>& node_element_number);

  inline void getElementJacobian();

  inline void calculateElementLocalMassMatrixInverse();

  inline void calculateElementMeshSize(
      const MeshInformation& information,
      const AdjacencyElementMesh<AdjacencyPointTrait<ElementTrait::kPolynomialOrder>>& point);

  inline void calculateElementMeshSize(
      const MeshInformation& information,
      const AdjacencyElementMesh<AdjacencyLineTrait<ElementTrait::kPolynomialOrder>>& line);

  inline ElementMesh() : gaussian_quadrature_(), basis_function_(){};
};

template <typename SimulationControl>
struct MeshDataBase {
  Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic> node_coordinate_;

  Eigen::Vector<Isize, Eigen::Dynamic> node_element_number_;

  Isize node_number_{0};
  Isize element_number_{0};
  Isize adjacency_element_number_{0};

  MeshInformation information_;
};

template <typename SimulationControl, int Dimension>
struct MeshData;

template <typename SimulationControl>
struct MeshData<SimulationControl, 1> : MeshDataBase<SimulationControl> {
  AdjacencyElementMesh<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>> point_;
  ElementMesh<LineTrait<SimulationControl::kPolynomialOrder>> line_;
};

template <typename SimulationControl>
struct MeshData<SimulationControl, 2> : MeshDataBase<SimulationControl> {
  AdjacencyElementMesh<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>> line_;
  ElementMesh<TriangleTrait<SimulationControl::kPolynomialOrder>> triangle_;
  ElementMesh<QuadrangleTrait<SimulationControl::kPolynomialOrder>> quadrangle_;
};

template <typename SimulationControl>
struct Mesh : MeshData<SimulationControl, SimulationControl::kDimension> {
  inline void getNode() {
    std::vector<std::size_t> node_tags;
    std::vector<double> coord;
    std::vector<double> parametric_coord;
    gmsh::model::mesh::getNodes(node_tags, coord, parametric_coord);
    this->node_coordinate_.resize(Eigen::NoChange, static_cast<Isize>(node_tags.size()));
    this->node_element_number_.resize(static_cast<Isize>(node_tags.size()));
    this->node_element_number_.setZero();
    for (const auto node_tag : node_tags) {
      for (Isize i = 0; i < SimulationControl::kDimension; i++) {
        this->node_coordinate_(i, static_cast<Isize>(node_tag) - 1) =
            static_cast<Real>(coord[3 * (node_tag - 1) + static_cast<Usize>(i)]);
      }
    }
  }

  inline void getPhysicalInformation() {
    std::vector<std::pair<int, int>> dim_tags;
    gmsh::model::getPhysicalGroups(dim_tags);
    for (const auto& [dim, physical_group_tag] : dim_tags) {
      std::string name;
      gmsh::model::getPhysicalName(dim, physical_group_tag, name);
      this->information_.physical_group_.emplace_back(dim, name);
      this->information_.physical_group_information_[name].name_ = name;
      std::vector<int> entity_tags;
      gmsh::model::getEntitiesForPhysicalGroup(dim, physical_group_tag, entity_tags);
      for (const auto entity_tag : entity_tags) {
        std::vector<int> element_types;
        std::vector<std::vector<std::size_t>> element_tags;
        std::vector<std::vector<std::size_t>> node_tags;
        gmsh::model::mesh::getElements(element_types, element_tags, node_tags, dim, entity_tag);
        for (Usize i = 0; i < element_types.size(); i++) {
          for (const auto element_tag : element_tags[i]) {
            this->information_.gmsh_tag_to_element_information_[static_cast<Isize>(element_tag)].gmsh_physical_name_ =
                name;
          }
        }
      }
    }
  }

  inline Mesh(const std::filesystem::path& mesh_file_path,
              const std::function<void(const std::filesystem::path& mesh_file_path)>& generate_mesh_function) {
    gmsh::option::setNumber("Mesh.SecondOrderLinear", 1);
    generate_mesh_function(mesh_file_path);
    gmsh::clear();
    gmsh::open(mesh_file_path);
    this->getNode();
    this->getPhysicalInformation();
    if constexpr (SimulationControl::kDimension == 1) {
      this->line_.getElementMesh(this->node_coordinate_, this->information_, this->node_element_number_);
      this->element_number_ += this->line_.number_;
      this->point_.template getAdjacencyElementMesh<SimulationControl::kMeshModel>(this->node_coordinate_,
                                                                                   this->information_);
      this->adjacency_element_number_ += this->point_.interior_number_ + this->point_.boundary_number_;
      this->line_.calculateElementMeshSize(this->information_, this->point_);
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
        this->triangle_.getElementMesh(this->node_coordinate_, this->information_, this->node_element_number_);
      }
      if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
        this->quadrangle_.getElementMesh(this->node_coordinate_, this->information_, this->node_element_number_);
      }
      this->element_number_ += this->triangle_.number_ + this->quadrangle_.number_;
      this->line_.template getAdjacencyElementMesh<SimulationControl::kMeshModel>(this->node_coordinate_,
                                                                                  this->information_);
      this->adjacency_element_number_ += this->line_.interior_number_ + this->line_.boundary_number_;
      if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
        this->triangle_.calculateElementMeshSize(this->information_, this->line_);
      }
      if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
        this->quadrangle_.calculateElementMeshSize(this->information_, this->line_);
      }
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_READ_CONTROL_HPP_
