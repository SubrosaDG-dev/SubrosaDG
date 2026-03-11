/**
 * @file Paraview.cpp
 * @brief The header file of SubrosaDG paraview output.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-10
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_PARAVIEW_CPP_
#define SUBROSA_DG_PARAVIEW_CPP_

#include <Eigen/Core>
#include <array>
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <vtu11-cpp17.hpp>

#include "Mesh/ReadControl.cpp"
#include "Solver/SimulationControl.cpp"
#include "Solver/VariableConvertor.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Enum.cpp"
#include "View/IOControl.cpp"

namespace SubrosaDG {

template <typename SimulationControl>
inline std::string View<SimulationControl>::getBaseName(const std::string_view physical_name, const int step) {
  return std::format("{}_{}_{:0{}d}.vtu", this->output_file_name_prefix_, physical_name, step, this->iteration_order_);
}

template <typename SimulationControl>
inline void View<SimulationControl>::getDataSetInformation(std::vector<vtu11::DataSetInfo>& data_set_information) {
  data_set_information.emplace_back("TMSTEP", vtu11::DataSetType::FieldData, 1, 1);
  data_set_information.emplace_back("TimeValue", vtu11::DataSetType::FieldData, 1, 1);
  data_set_information.emplace_back("Force", vtu11::DataSetType::FieldData, 3, 1);
  for (const auto variable : this->variable_type_) {
    if ((SimulationControl::kDimension >= 2) &&
        (variable == ViewVariableEnum::Velocity || variable == ViewVariableEnum::MachNumber ||
         variable == ViewVariableEnum::HeatFlux ||
         (variable == ViewVariableEnum::Vorticity && SimulationControl::kDimension == 3))) {
      data_set_information.emplace_back(magic_enum::enum_name(variable), vtu11::DataSetType::PointData, 3, 0);
    } else {
      data_set_information.emplace_back(magic_enum::enum_name(variable), vtu11::DataSetType::PointData, 1, 0);
    }
  }
}

template <typename SimulationControl>
template <typename VolumeElementTrait>
inline void View<SimulationControl>::computeViewVariable(
    const ViewVariable<VolumeElementTrait, SimulationControl>& view_variable,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable, const Isize node_index,
    const Isize column) {
  auto handle_variable = [&](Isize i, ViewVariableEnum variable_x, ViewVariableEnum variable_y,
                             ViewVariableEnum variable_z) -> void {
    if constexpr (SimulationControl::kDimension == 1) {
      node_variable(i)(node_index) = view_variable.get(variable_x, column);
    } else if constexpr (SimulationControl::kDimension == 2) {
      node_variable(i)(node_index * 3) = view_variable.get(variable_x, column);
      node_variable(i)(node_index * 3 + 1) = view_variable.get(variable_y, column);
      node_variable(i)(node_index * 3 + 2) = 0.0_r;
    } else if constexpr (SimulationControl::kDimension == 3) {
      node_variable(i)(node_index * 3) = view_variable.get(variable_x, column);
      node_variable(i)(node_index * 3 + 1) = view_variable.get(variable_y, column);
      node_variable(i)(node_index * 3 + 2) = view_variable.get(variable_z, column);
    }
  };
  for (Isize i = 0; i < static_cast<Isize>(this->variable_type_.size()); i++) {
    if (this->variable_type_[static_cast<Usize>(i)] == ViewVariableEnum::Velocity) {
      handle_variable(i, ViewVariableEnum::VelocityX, ViewVariableEnum::VelocityY, ViewVariableEnum::VelocityZ);
    } else if (this->variable_type_[static_cast<Usize>(i)] == ViewVariableEnum::MachNumber) {
      handle_variable(i, ViewVariableEnum::MachNumberX, ViewVariableEnum::MachNumberY, ViewVariableEnum::MachNumberZ);
    } else if (this->variable_type_[static_cast<Usize>(i)] == ViewVariableEnum::Vorticity &&
               SimulationControl::kDimension == 3) {
      handle_variable(i, ViewVariableEnum::VorticityX, ViewVariableEnum::VorticityY, ViewVariableEnum::VorticityZ);
    } else if (this->variable_type_[static_cast<Usize>(i)] == ViewVariableEnum::HeatFlux) {
      handle_variable(i, ViewVariableEnum::HeatFluxX, ViewVariableEnum::HeatFluxY, ViewVariableEnum::HeatFluxZ);
    } else {
      node_variable(i)(node_index) = view_variable.get(this->variable_type_[static_cast<Usize>(i)], column);
    }
  }
}

template <typename SimulationControl>
template <typename AdjacencyElementTrait, typename VolumeElementTrait>
inline void View<SimulationControl>::computeAdjacencyForce(
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const ViewVariable<VolumeElementTrait, SimulationControl>& view_variable,
    Eigen::Vector<Real, SimulationControl::kDimension>& force, const Isize element_index, const Isize column) {
  force += view_variable.getForce(adjacency_element_mesh.normal_vector_(element_index).col(column), column) *
           adjacency_element_mesh.jacobian_determinant_multiply_weight_(element_index)(column);
}

template <typename SimulationControl>
template <typename AdjacencyElementTrait>
inline void View<SimulationControl>::writeAdjacencyElement(
    const MeshPhysical& mesh_physical, const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const ViewSolver<SimulationControl>& view_solver, ViewSupplemental<SimulationControl>& view_supplemental,
    const Isize physical_index, const Isize element_index) {
  const AdjacencyElementViewSolver<AdjacencyElementTrait, SimulationControl>& adjacency_element_view_solver =
      view_solver.*(ViewSolver<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>());
  constexpr std::array<int, AdjacencyElementTrait::kVtkElementNumber> kVtkTypeNumber{
      getElementVtkTypeNumber<AdjacencyElementTrait::kElementType>()};
  constexpr std::array<int, AdjacencyElementTrait::kVtkElementNumber> kVtkPerNodeNumber{
      getElementVtkPerNodeNumber<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>()};
  constexpr std::array<int, AdjacencyElementTrait::kAllNodeNumber> kVtkConnectivity{
      getElementVTKConnectivity<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>()};
  const Isize element_gmsh_tag = mesh_physical.information_[static_cast<Usize>(physical_index)]
                                     .element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type =
      mesh_physical.gmsh_tag_to_element_physical_information_.at(element_gmsh_tag).element_index_;
  for (Isize i = 0; i < AdjacencyElementTrait::kAllNodeNumber; i++) {
    view_supplemental.node_coordinate_(Eigen::seqN(Eigen::fix<0>, Eigen::fix<SimulationControl::kDimension>),
                                       view_supplemental.node_index_ + i) =
        adjacency_element_mesh.node_coordinate_(element_index_per_type)(Eigen::placeholders::all, i);
    this->computeViewVariable(
        adjacency_element_view_solver.view_variable_(element_index_per_type - adjacency_element_mesh.interior_number_),
        view_supplemental.node_variable_, view_supplemental.node_index_ + i, i);
    this->computeAdjacencyForce(
        adjacency_element_mesh,
        adjacency_element_view_solver.view_variable_(element_index_per_type - adjacency_element_mesh.interior_number_),
        view_supplemental.force_, element_index_per_type, i);
  }
  for (Isize i = 0; i < AdjacencyElementTrait::kVtkAllNodeNumber; i++) {
    view_supplemental.element_connectivity_(view_supplemental.vtk_node_index_ + i) =
        kVtkConnectivity[static_cast<Usize>(i)] + view_supplemental.node_index_;
  }
  for (Usize i = 0; i < AdjacencyElementTrait::kVtkElementNumber; i++) {
    view_supplemental.vtk_node_index_ += kVtkPerNodeNumber[i];
    view_supplemental.element_offset_(view_supplemental.vtk_element_index_) = view_supplemental.vtk_node_index_;
    view_supplemental.element_type_(view_supplemental.vtk_element_index_++) =
        static_cast<vtu11::VtkCellType>(kVtkTypeNumber[i]);
  }
  view_supplemental.node_index_ += AdjacencyElementTrait::kAllNodeNumber;
}

template <typename SimulationControl>
template <typename VolumeElementTrait>
inline void View<SimulationControl>::writeVolumeElement(
    const MeshPhysical& mesh_physical, const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
    const ViewSolver<SimulationControl>& view_solver, ViewSupplemental<SimulationControl>& view_supplemental,
    const Isize physical_index, const Isize element_index) {
  const VolumeElementViewSolver<VolumeElementTrait, SimulationControl>& volume_element_view_solver =
      view_solver.*(ViewSolver<SimulationControl>::template getVolumeElement<VolumeElementTrait>());
  constexpr std::array<int, VolumeElementTrait::kVtkElementNumber> kVtkTypeNumber{
      getElementVtkTypeNumber<VolumeElementTrait::kElementType>()};
  constexpr std::array<int, VolumeElementTrait::kVtkElementNumber> kVtkPerNodeNumber{
      getElementVtkPerNodeNumber<VolumeElementTrait::kElementType, VolumeElementTrait::kPolynomialOrder>()};
  constexpr std::array<int, VolumeElementTrait::kVtkAllNodeNumber> kVtkConnectivity{
      getElementVTKConnectivity<VolumeElementTrait::kElementType, VolumeElementTrait::kPolynomialOrder>()};
  const Isize element_gmsh_tag = mesh_physical.information_[static_cast<Usize>(physical_index)]
                                     .element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type =
      mesh_physical.gmsh_tag_to_element_physical_information_.at(element_gmsh_tag).element_index_;
  for (Isize i = 0; i < VolumeElementTrait::kAllNodeNumber; i++) {
    view_supplemental.node_coordinate_(Eigen::seqN(Eigen::fix<0>, Eigen::fix<SimulationControl::kDimension>),
                                       view_supplemental.node_index_ + i) =
        volume_element_mesh.node_coordinate_(element_index_per_type)(Eigen::placeholders::all, i);
    this->computeViewVariable(volume_element_view_solver.view_variable_(element_index_per_type),
                              view_supplemental.node_variable_, view_supplemental.node_index_ + i, i);
  }
  for (Isize i = 0; i < VolumeElementTrait::kVtkAllNodeNumber; i++) {
    view_supplemental.element_connectivity_(view_supplemental.vtk_node_index_ + i) =
        kVtkConnectivity[static_cast<Usize>(i)] + view_supplemental.node_index_;
  }
  for (Isize i = 0; i < VolumeElementTrait::kVtkElementNumber; i++) {
    view_supplemental.vtk_node_index_ += kVtkPerNodeNumber[static_cast<Usize>(i)];
    view_supplemental.element_offset_(view_supplemental.vtk_element_index_) = view_supplemental.vtk_node_index_;
    view_supplemental.element_type_(view_supplemental.vtk_element_index_++) =
        static_cast<vtu11::VtkCellType>(kVtkTypeNumber[static_cast<Usize>(i)]);
  }
  view_supplemental.node_index_ += VolumeElementTrait::kAllNodeNumber;
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void View<SimulationControl>::writeField(const Mesh<SimulationControl>& mesh,
                                                const ViewSolver<SimulationControl>& view_solver,
                                                ViewSupplemental<SimulationControl>& view_supplemental,
                                                const Isize physical_index) {
  const PhysicalInformation& physical_information = mesh.physical_.information_[static_cast<Usize>(physical_index)];
  for (Isize i = 0; i < physical_information.element_number_; i++) {
    const Isize element_gmsh_type = physical_information.element_gmsh_type_[static_cast<Usize>(i)];
    if constexpr (Dimension == 1) {
      if constexpr (IsAdjacency) {
        this->writeAdjacencyElement<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>>(
            mesh.physical_, mesh.line_, view_solver, view_supplemental, physical_index, i);
      } else {
        this->writeVolumeElement<VolumeLineTrait<SimulationControl::kPolynomialOrder>>(
            mesh.physical_, mesh.line_, view_solver, view_supplemental, physical_index, i);
      }
    } else if constexpr (Dimension == 2) {
      if constexpr (IsAdjacency) {
        if (element_gmsh_type == AdjacencyTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeAdjacencyElement<AdjacencyTriangleTrait<SimulationControl::kPolynomialOrder>>(
              mesh.physical_, mesh.triangle_, view_solver, view_supplemental, physical_index, i);
        } else if (element_gmsh_type ==
                   AdjacencyQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeAdjacencyElement<AdjacencyQuadrangleTrait<SimulationControl::kPolynomialOrder>>(
              mesh.physical_, mesh.quadrangle_, view_solver, view_supplemental, physical_index, i);
        }
      } else {
        if (element_gmsh_type == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeVolumeElement<VolumeTriangleTrait<SimulationControl::kPolynomialOrder>>(
              mesh.physical_, mesh.triangle_, view_solver, view_supplemental, physical_index, i);
        } else if (element_gmsh_type == VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeVolumeElement<VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>>(
              mesh.physical_, mesh.quadrangle_, view_solver, view_supplemental, physical_index, i);
        }
      }
    } else if constexpr (Dimension == 3) {
      if (element_gmsh_type == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeVolumeElement<VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>>(
            mesh.physical_, mesh.tetrahedron_, view_solver, view_supplemental, physical_index, i);
      } else if (element_gmsh_type == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeVolumeElement<VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            mesh.physical_, mesh.pyramid_, view_solver, view_supplemental, physical_index, i);
      } else if (element_gmsh_type == VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeVolumeElement<VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>>(
            mesh.physical_, mesh.hexahedron_, view_solver, view_supplemental, physical_index, i);
      }
    }
  }
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void View<SimulationControl>::writeView(const Mesh<SimulationControl>& mesh,
                                               const ViewSolver<SimulationControl>& view_solver,
                                               const std::string& base_name, const int step,
                                               const Isize physical_index) {
  ViewSupplemental<SimulationControl> view_supplemental(mesh, this->variable_type_, physical_index);
  this->getDataSetInformation(view_supplemental.data_set_information_);
  this->writeField<Dimension, IsAdjacency>(mesh, view_solver, view_supplemental, physical_index);
  vtu11::Vtu11UnstructuredMesh mesh_data{
      {view_supplemental.node_coordinate_.data(),
       view_supplemental.node_coordinate_.data() + view_supplemental.node_coordinate_.size()},
      {view_supplemental.element_connectivity_.data(),
       view_supplemental.element_connectivity_.data() + view_supplemental.element_connectivity_.size()},
      {view_supplemental.element_offset_.data(),
       view_supplemental.element_offset_.data() + view_supplemental.element_offset_.size()},
      {view_supplemental.element_type_.data(),
       view_supplemental.element_type_.data() + view_supplemental.element_type_.size()}};
  view_supplemental.data_set_data_[0].emplace_back(step);
  view_supplemental.data_set_data_[1].emplace_back(this->time_value_(step));
  Eigen::Vector<Real, 3> force{Eigen::Vector<Real, 3>::Zero()};
  force(Eigen::seqN(Eigen::fix<0>, Eigen::fix<SimulationControl::kDimension>)) = view_supplemental.force_;
  for (Isize i = 0; i < 3; i++) {
    view_supplemental.data_set_data_[2].emplace_back(force(i));
  }
  for (Isize i = 0; i < static_cast<Isize>(this->variable_type_.size()); i++) {
    view_supplemental.data_set_data_[static_cast<Usize>(i) + 3].assign(
        view_supplemental.node_variable_(i).data(),
        view_supplemental.node_variable_(i).data() + view_supplemental.node_variable_(i).size());
  }
  vtu11::writeVtu((this->output_directory_ / "vtu" / base_name).string(), mesh_data,
                  view_supplemental.data_set_information_, view_supplemental.data_set_data_, "rawbinarycompressed");
}

template <typename SimulationControl>
inline void View<SimulationControl>::stepView(const Mesh<SimulationControl>& mesh,
                                              ViewSolver<SimulationControl>& view_solver, const int step) {
  view_solver.computeViewVariable(mesh);
  for (Isize i = 0; i < mesh.physical_.number_; i++) {
    const PhysicalInformation& physical_information = mesh.physical_.information_[static_cast<Usize>(i)];
    if ((physical_information.dimension_ == SimulationControl::kDimension - 1) &&
        !isWall(physical_information.boundary_condition_type_)) {
      continue;
    }
    const std::string base_name = this->getBaseName(physical_information.name_, step);
    if constexpr (SimulationControl::kDimension == 1) {
      if (physical_information.dimension_ == 1) {
        this->writeView<1, false>(mesh, view_solver, base_name, step, i);
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if (physical_information.dimension_ == 1) {
        this->writeView<1, true>(mesh, view_solver, base_name, step, i);
      } else if (physical_information.dimension_ == 2) {
        this->writeView<2, false>(mesh, view_solver, base_name, step, i);
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if (physical_information.dimension_ == 2) {
        this->writeView<2, true>(mesh, view_solver, base_name, step, i);
      } else if (physical_information.dimension_ == 3) {
        this->writeView<3, false>(mesh, view_solver, base_name, step, i);
      }
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_PARAVIEW_CPP_
