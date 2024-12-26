/**
 * @file ReadControl.hpp
 * @brief The header file of ReadControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_READ_CONTROL_HPP_
#define SUBROSA_DG_READ_CONTROL_HPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Mesh/BasisFunction.hpp"
#include "Mesh/Quadrature.hpp"
#include "Solver/SimulationControl.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

struct PhysicalInformation {
  int gmsh_tag_;
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

struct MeshInformation {
  ordered_set<std::string> physical_;
  std::vector<Isize> physical_dimension_;
  std::unordered_map<Isize, BoundaryConditionEnum> boundary_condition_type_;
  std::unordered_map<Isize, PhysicalInformation> physical_information_;
  std::unordered_map<Isize, PerElementPhysicalInformation> gmsh_tag_to_element_physical_information_;
};

template <typename BaseTrait>
struct PerElementMeshBase {
  Isize gmsh_tag_;
  Isize gmsh_physical_index_;
  Isize element_index_;
  Eigen::Vector<Isize, BaseTrait::kAllNodeNumber> node_tag_;
  Eigen::Vector<Real, BaseTrait::kQuadratureNumber> jacobian_determinant_;
};

template <typename AdjacencyElementTrait>
struct PerAdjacencyElementMesh : PerElementMeshBase<AdjacencyElementTrait> {
  Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber> node_coordinate_;
  Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>
      quadrature_node_coordinate_;
  Isize gmsh_jacobian_tag_;
  Isize adjacency_right_rotation_;
  Eigen::Vector<Isize, 2> parent_index_each_type_;
  Eigen::Vector<Isize, 2> adjacency_sequence_in_parent_;
  Eigen::Vector<Isize, 2> parent_gmsh_type_number_;
  Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber> normal_vector_;
};

template <typename ElementTrait>
struct PerElementMesh : PerElementMeshBase<ElementTrait> {
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kAllNodeNumber> node_coordinate_;
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kQuadratureNumber> quadrature_node_coordinate_;
  Eigen::Matrix<Real, ElementTrait::kBasisFunctionNumber, ElementTrait::kBasisFunctionNumber>
      local_mass_matrix_inverse_;
  Eigen::Matrix<Real, ElementTrait::kDimension * ElementTrait::kDimension, ElementTrait::kQuadratureNumber>
      jacobian_transpose_inverse_;
  Real minimum_edge_;
  Real inner_radius_;
};

template <typename AdjacencyElementTrait>
struct AdjacencyElementMeshSupplemental {
  bool is_recorded_{false};
  Isize right_rotation_{0};
  std::array<Isize, AdjacencyElementTrait::kAllNodeNumber> node_tag_;
  std::vector<Isize> parent_gmsh_tag_;
  std::vector<Isize> adjacency_sequence_in_parent_;
  std::vector<Isize> parent_gmsh_type_number_;
};

template <typename AdjacencyElementTrait>
struct AdjacencyElementMesh {
  ElementQuadrature<AdjacencyElementTrait> quadrature_;
  AdjacencyElementBasisFunction<AdjacencyElementTrait> basis_function_;

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

  inline void calculateAdjacencyElementNormalVector();

  inline AdjacencyElementMesh() : quadrature_() {};
};

template <typename ElementTrait>
struct ElementMesh {
  ElementQuadrature<ElementTrait> quadrature_;
  ElementBasisFunction<ElementTrait> basis_function_;

  Isize number_{0};
  Eigen::Array<PerElementMesh<ElementTrait>, Eigen::Dynamic, 1> element_;

  inline void getElementMesh(const Eigen::Matrix<Real, ElementTrait::kDimension, Eigen::Dynamic>& node_coordinate,
                             MeshInformation& information);

  inline void getElementQuality();

  inline void getElementJacobian();

  inline void calculateElementLocalMassMatrixInverse();

  inline ElementMesh() : quadrature_(), basis_function_() {};
};

template <typename SimulationControl>
struct MeshDataBase {
  Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic> node_coordinate_;

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
struct MeshData<SimulationControl, 3> : MeshDataBase<SimulationControl> {
  AdjacencyElementMesh<AdjacencyTriangleTrait<SimulationControl::kPolynomialOrder>> triangle_;
  AdjacencyElementMesh<AdjacencyQuadrangleTrait<SimulationControl::kPolynomialOrder>> quadrangle_;
  ElementMesh<TetrahedronTrait<SimulationControl::kPolynomialOrder>> tetrahedron_;
  ElementMesh<PyramidTrait<SimulationControl::kPolynomialOrder>> pyramid_;
  ElementMesh<HexahedronTrait<SimulationControl::kPolynomialOrder>> hexahedron_;
};

template <typename SimulationControl>
struct Mesh : MeshData<SimulationControl, SimulationControl::kDimension> {
  template <typename ElementTrait>
  inline static ElementMesh<ElementTrait> Mesh::*getElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (ElementTrait::kElementType == ElementEnum::Line) {
        return &Mesh::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (ElementTrait::kElementType == ElementEnum::Triangle) {
        return &Mesh::triangle_;
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &Mesh::quadrangle_;
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (ElementTrait::kElementType == ElementEnum::Tetrahedron) {
        return &Mesh::tetrahedron_;
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Pyramid) {
        return &Mesh::pyramid_;
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Hexahedron) {
        return &Mesh::hexahedron_;
      }
    }
    return nullptr;
  }

  template <typename AdjacencyElementTrait>
  inline static AdjacencyElementMesh<AdjacencyElementTrait> Mesh::*getAdjacencyElement() {
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

  inline void getNode() {
    std::vector<std::size_t> node_tags;
    std::vector<double> coord;
    std::vector<double> parametric_coord;
    gmsh::model::mesh::getNodes(node_tags, coord, parametric_coord);
    this->node_number_ = static_cast<Isize>(node_tags.size());
    this->node_coordinate_.resize(Eigen::NoChange, this->node_number_);
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
    for (Usize i = 0; i < dim_tags.size(); i++) {
      const auto [physical_dimension, physical_tag] = dim_tags[i];
      std::string name;
      gmsh::model::getPhysicalName(physical_dimension, physical_tag, name);
      this->information_.physical_.emplace_back(name);
      this->information_.physical_dimension_.emplace_back(physical_dimension);
      this->information_.physical_information_[static_cast<Isize>(i)].gmsh_tag_ = physical_tag;
      std::vector<int> entity_tags;
      gmsh::model::getEntitiesForPhysicalGroup(physical_dimension, physical_tag, entity_tags);
      for (const auto entity_tag : entity_tags) {
        std::vector<int> element_types;
        std::vector<std::vector<std::size_t>> element_tags;
        std::vector<std::vector<std::size_t>> node_tags;
        gmsh::model::mesh::getElements(element_types, element_tags, node_tags, physical_dimension, entity_tag);
        for (Usize j = 0; j < element_types.size(); j++) {
          for (const auto element_tag : element_tags[j]) {
            this->information_.gmsh_tag_to_element_physical_information_[static_cast<Isize>(element_tag)]
                .gmsh_physical_index_ = static_cast<Isize>(i);
          }
        }
      }
    }
  }

  inline void initializeMesh(const std::filesystem::path& mesh_file_path) {
    gmsh::clear();
    gmsh::open(mesh_file_path);
    this->getNode();
    this->getPhysicalInformation();
  }

  inline void readMeshElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      this->line_.getElementMesh(this->node_coordinate_, this->information_);
      this->element_number_ += this->line_.number_;
      this->point_.template getAdjacencyElementMesh<SimulationControl::kMeshModel>(this->node_coordinate_,
                                                                                   this->information_);
      this->adjacency_element_number_ += this->point_.interior_number_ + this->point_.boundary_number_;
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
        this->triangle_.getElementMesh(this->node_coordinate_, this->information_);
      }
      if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
        this->quadrangle_.getElementMesh(this->node_coordinate_, this->information_);
      }
      this->element_number_ += this->triangle_.number_ + this->quadrangle_.number_;
      gmsh::model::mesh::createEdges();
      this->line_.template getAdjacencyElementMesh<SimulationControl::kMeshModel>(this->node_coordinate_,
                                                                                  this->information_);
      this->adjacency_element_number_ += this->line_.interior_number_ + this->line_.boundary_number_;
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
        this->tetrahedron_.getElementMesh(this->node_coordinate_, this->information_);
      }
      if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
        this->pyramid_.getElementMesh(this->node_coordinate_, this->information_);
      }
      if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
        this->hexahedron_.getElementMesh(this->node_coordinate_, this->information_);
      }
      this->element_number_ += this->tetrahedron_.number_ + this->pyramid_.number_ + this->hexahedron_.number_;
      gmsh::model::mesh::createFaces();
      if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
        this->triangle_.template getAdjacencyElementMesh<SimulationControl::kMeshModel>(this->node_coordinate_,
                                                                                        this->information_);
      }
      if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
        this->quadrangle_.template getAdjacencyElementMesh<SimulationControl::kMeshModel>(this->node_coordinate_,
                                                                                          this->information_);
      }
      this->adjacency_element_number_ += this->triangle_.interior_number_ + this->triangle_.boundary_number_ +
                                         this->quadrangle_.interior_number_ + this->quadrangle_.boundary_number_;
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_READ_CONTROL_HPP_
