/**
 * @file Paraview.hpp
 * @brief The header file of SubrosaDG paraview output.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-10
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_PARAVIEW_HPP_
#define SUBROSA_DG_PARAVIEW_HPP_

#include <Eigen/Core>
#include <array>
#include <format>
#include <magic_enum.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <vtu11-cpp17.hpp>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"
#include "View/IOControl.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
inline std::string View<SimulationControl>::getBaseName(const int step, const std::string_view physical_name) {
  return std::format("{}_{}_{:0{}d}.vtu", this->output_file_name_prefix_, physical_name, step, this->iteration_order_);
}

template <typename SimulationControl>
inline void View<SimulationControl>::getDataSetInfomatoin(std::vector<vtu11::DataSetInfo>& data_set_information) {
  data_set_information.emplace_back("TMSTEP", vtu11::DataSetType::FieldData, 1, 1);
  data_set_information.emplace_back("TimeValue", vtu11::DataSetType::FieldData, 1, 1);
  data_set_information.emplace_back("Force", vtu11::DataSetType::FieldData, 3, 1);
  for (const auto variable : this->variable_type_) {
    if ((SimulationControl::kDimension >= 2) &&
        (variable == ViewVariableEnum::Velocity || variable == ViewVariableEnum::MachNumber ||
         (variable == ViewVariableEnum::Vorticity && SimulationControl::kDimension == 3))) {
      data_set_information.emplace_back(magic_enum::enum_name(variable), vtu11::DataSetType::PointData, 3, 0);
    } else {
      data_set_information.emplace_back(magic_enum::enum_name(variable), vtu11::DataSetType::PointData, 1, 0);
    }
  }
}

template <typename SimulationControl>
template <typename ElementTrait>
inline void View<SimulationControl>::calculateViewVariable(
    const ThermalModel<SimulationControl>& thermal_model,
    const ViewVariable<ElementTrait, SimulationControl>& view_variable,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable, const Isize column,
    const Isize node_index) {
  auto handle_variable = [&view_variable, &thermal_model, &node_variable, node_index, column](
                             Isize i, ViewVariableEnum variable_x, ViewVariableEnum variable_y,
                             ViewVariableEnum variable_z) -> void {
    if constexpr (SimulationControl::kDimension == 1) {
      node_variable(i)(node_index) = view_variable.get(thermal_model, variable_x, column);
    } else if constexpr (SimulationControl::kDimension == 2) {
      node_variable(i)(node_index * 3) = view_variable.get(thermal_model, variable_x, column);
      node_variable(i)(node_index * 3 + 1) = view_variable.get(thermal_model, variable_y, column);
      node_variable(i)(node_index * 3 + 2) = 0.0_r;
    } else if constexpr (SimulationControl::kDimension == 3) {
      node_variable(i)(node_index * 3) = view_variable.get(thermal_model, variable_x, column);
      node_variable(i)(node_index * 3 + 1) = view_variable.get(thermal_model, variable_y, column);
      node_variable(i)(node_index * 3 + 2) = view_variable.get(thermal_model, variable_z, column);
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
    } else {
      node_variable(i)(node_index) =
          view_variable.get(thermal_model, this->variable_type_[static_cast<Usize>(i)], column);
    }
  }
}

template <typename SimulationControl>
template <typename AdjacencyElementTrait, typename ElementTrait>
inline void View<SimulationControl>::calculateAdjacencyForce(
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const ThermalModel<SimulationControl>& thermal_model,
    const ViewVariable<ElementTrait, SimulationControl>& view_variable,
    Eigen::Vector<Real, SimulationControl::kDimension>& force, const Isize element_index, const Isize column) {
  force += view_variable.getForce(thermal_model,
                                  adjacency_element_mesh.element_(element_index).normal_vector_.col(column), column) *
           adjacency_element_mesh.quadrature_.weight_(column) *
           adjacency_element_mesh.element_(element_index).jacobian_determinant_(column);
}

template <typename SimulationControl>
template <typename AdjacencyElementTrait>
inline void View<SimulationControl>::writeAdjacencyElement(
    const Isize physical_index, const MeshInformation& mesh_information,
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const ThermalModel<SimulationControl>& thermal_model, const ViewData<SimulationControl>& view_data,
    const Isize element_index, ViewSupplemental<SimulationControl>& view_supplemental) {
  const AdjacencyElementViewSolver<AdjacencyElementTrait, SimulationControl, SimulationControl::kEquationModel>&
      adjacency_element_view_solver =
          view_data.solver_.*(decltype(view_data.solver_)::template getAdjacencyElement<AdjacencyElementTrait>());
  constexpr std::array<int, AdjacencyElementTrait::kVtkElementNumber> kVtkTypeNumber{
      getElementVtkTypeNumber<AdjacencyElementTrait::kElementType>()};
  constexpr std::array<int, AdjacencyElementTrait::kVtkElementNumber> kVtkPerNodeNumber{
      getElementVtkPerNodeNumber<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>()};
  constexpr std::array<int, AdjacencyElementTrait::kAllNodeNumber> kVtkConnectivity{
      getElementVTKConnectivity<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>()};
  const Isize adjacency_element_gmsh_tag =
      mesh_information.physical_information_.at(physical_index).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize adjacency_element_index_per_type =
      mesh_information.gmsh_tag_to_element_information_.at(adjacency_element_gmsh_tag).element_index_;
  for (Isize i = 0; i < AdjacencyElementTrait::kAllNodeNumber; i++) {
    view_supplemental.node_coordinate_(Eigen::seqN(Eigen::fix<0>, Eigen::fix<SimulationControl::kDimension>),
                                       view_supplemental.node_index_ + i) =
        adjacency_element_mesh.element_(adjacency_element_index_per_type).node_coordinate_(Eigen::all, i);
    this->calculateViewVariable(thermal_model,
                                adjacency_element_view_solver.view_variable_(adjacency_element_index_per_type -
                                                                             adjacency_element_mesh.interior_number_),
                                view_supplemental.node_variable_, i, view_supplemental.node_index_ + i);
    this->calculateAdjacencyForce(adjacency_element_mesh, thermal_model,
                                  adjacency_element_view_solver.view_variable_(adjacency_element_index_per_type -
                                                                               adjacency_element_mesh.interior_number_),
                                  view_supplemental.force_, adjacency_element_index_per_type, i);
  }
  for (Isize i = 0; i < AdjacencyElementTrait::kVtkAllNodeNumber; i++) {
    view_supplemental.element_connectivity_(view_supplemental.vtk_node_index_ + i) =
        kVtkConnectivity[static_cast<Usize>(i)] + view_supplemental.node_index_;
  }
  for (Usize i = 0; i < AdjacencyElementTrait::kVtkElementNumber; i++) {
    view_supplemental.vtk_node_index_ += kVtkPerNodeNumber[static_cast<Usize>(i)];
    view_supplemental.element_offset_(view_supplemental.vtk_element_index_) = view_supplemental.vtk_node_index_;
    view_supplemental.element_type_(view_supplemental.vtk_element_index_++) =
        static_cast<vtu11::VtkCellType>(kVtkTypeNumber[static_cast<Usize>(i)]);
  }
  view_supplemental.node_index_ += AdjacencyElementTrait::kAllNodeNumber;
}

template <typename SimulationControl>
template <typename ElementTrait>
inline void View<SimulationControl>::writeElement(const Isize physical_index, const MeshInformation& mesh_information,
                                                  const ElementMesh<ElementTrait>& element_mesh,
                                                  const ThermalModel<SimulationControl>& thermal_model,
                                                  const ViewData<SimulationControl>& view_data,
                                                  const Isize element_index,
                                                  ViewSupplemental<SimulationControl>& view_supplemental) {
  const ElementViewSolver<ElementTrait, SimulationControl, SimulationControl::kEquationModel>& element_view_solver =
      view_data.solver_.*(decltype(view_data.solver_)::template getElement<ElementTrait>());
  constexpr std::array<int, ElementTrait::kVtkElementNumber> kVtkTypeNumber{
      getElementVtkTypeNumber<ElementTrait::kElementType>()};
  constexpr std::array<int, ElementTrait::kVtkElementNumber> kVtkPerNodeNumber{
      getElementVtkPerNodeNumber<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>()};
  constexpr std::array<int, ElementTrait::kVtkAllNodeNumber> kVtkConnectivity{
      getElementVTKConnectivity<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>()};
  const Isize element_gmsh_tag =
      mesh_information.physical_information_.at(physical_index).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type =
      mesh_information.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
    view_supplemental.node_coordinate_(Eigen::seqN(Eigen::fix<0>, Eigen::fix<SimulationControl::kDimension>),
                                       view_supplemental.node_index_ + i) =
        element_mesh.element_(element_index_per_type).node_coordinate_(Eigen::all, i);
    this->calculateViewVariable(thermal_model, element_view_solver.view_variable_(element_index_per_type),
                                view_supplemental.node_variable_, i, view_supplemental.node_index_ + i);
  }
  for (Isize i = 0; i < ElementTrait::kVtkAllNodeNumber; i++) {
    view_supplemental.element_connectivity_(view_supplemental.vtk_node_index_ + i) =
        kVtkConnectivity[static_cast<Usize>(i)] + view_supplemental.node_index_;
  }
  for (Isize i = 0; i < ElementTrait::kVtkElementNumber; i++) {
    view_supplemental.vtk_node_index_ += kVtkPerNodeNumber[static_cast<Usize>(i)];
    view_supplemental.element_offset_(view_supplemental.vtk_element_index_) = view_supplemental.vtk_node_index_;
    view_supplemental.element_type_(view_supplemental.vtk_element_index_++) =
        static_cast<vtu11::VtkCellType>(kVtkTypeNumber[static_cast<Usize>(i)]);
  }
  view_supplemental.node_index_ += ElementTrait::kAllNodeNumber;
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void View<SimulationControl>::writeField(const Isize physical_index, const Mesh<SimulationControl>& mesh,
                                                const ThermalModel<SimulationControl>& thermal_model,
                                                const ViewData<SimulationControl>& view_data,
                                                ViewSupplemental<SimulationControl>& view_supplemental) {
  const Isize element_number = mesh.information_.physical_information_.at(physical_index).element_number_;
  for (Isize i = 0; i < element_number; i++) {
    const Isize element_gmsh_type =
        mesh.information_.physical_information_.at(physical_index).element_gmsh_type_[static_cast<Usize>(i)];
    if constexpr (Dimension == 1) {
      if constexpr (IsAdjacency) {
        this->writeAdjacencyElement<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.line_, thermal_model, view_data, i, view_supplemental);
      } else {
        this->writeElement<LineTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.line_, thermal_model, view_data, i, view_supplemental);
      }
    } else if constexpr (Dimension == 2) {
      if constexpr (IsAdjacency) {
        if (element_gmsh_type == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeAdjacencyElement<AdjacencyTriangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_index, mesh.information_, mesh.triangle_, thermal_model, view_data, i, view_supplemental);
        } else if (element_gmsh_type == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeAdjacencyElement<AdjacencyQuadrangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_index, mesh.information_, mesh.quadrangle_, thermal_model, view_data, i, view_supplemental);
        }
      } else {
        if (element_gmsh_type == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeElement<TriangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_index, mesh.information_, mesh.triangle_, thermal_model, view_data, i, view_supplemental);
        } else if (element_gmsh_type == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeElement<QuadrangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_index, mesh.information_, mesh.quadrangle_, thermal_model, view_data, i, view_supplemental);
        }
      }
    } else if constexpr (Dimension == 3) {
      if (element_gmsh_type == TetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeElement<TetrahedronTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.tetrahedron_, thermal_model, view_data, i, view_supplemental);
      } else if (element_gmsh_type == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeElement<PyramidTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.pyramid_, thermal_model, view_data, i, view_supplemental);
      } else if (element_gmsh_type == HexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeElement<HexahedronTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.hexahedron_, thermal_model, view_data, i, view_supplemental);
      }
    }
  }
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void View<SimulationControl>::writeView(const int step, const Isize physical_index,
                                               const Mesh<SimulationControl>& mesh,
                                               const ThermalModel<SimulationControl>& thermal_model,
                                               const ViewData<SimulationControl>& view_data,
                                               const std::string& base_name) {
  ViewSupplemental<SimulationControl> view_supplemental(physical_index, mesh, this->variable_type_);
  this->getDataSetInfomatoin(view_supplemental.data_set_information_);
  this->writeField<Dimension, IsAdjacency>(physical_index, mesh, thermal_model, view_data, view_supplemental);
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
inline void View<SimulationControl>::stepView(const int step, const Mesh<SimulationControl>& mesh,
                                              const ThermalModel<SimulationControl>& thermal_model,
                                              ViewData<SimulationControl>& view_data) {
  view_data.solver_.calcluateViewVariable(mesh, thermal_model, view_data.raw_binary_path_, view_data.raw_binary_ss_);
  for (Isize i = 0; i < static_cast<Isize>(mesh.information_.physical_.size()); i++) {
    if (mesh.information_.boundary_condition_type_.contains(i) &&
        !isWall(mesh.information_.boundary_condition_type_.at(i))) {
      continue;
    }
    const std::string base_name = this->getBaseName(step, mesh.information_.physical_[static_cast<Usize>(i)]);
    if constexpr (SimulationControl::kDimension == 1) {
      if (mesh.information_.physical_dimension_[static_cast<Usize>(i)] == 1) {
        this->writeView<1, false>(step, i, mesh, thermal_model, view_data, base_name);
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if (mesh.information_.physical_dimension_[static_cast<Usize>(i)] == 1) {
        this->writeView<1, true>(step, i, mesh, thermal_model, view_data, base_name);
      } else if (mesh.information_.physical_dimension_[static_cast<Usize>(i)] == 2) {
        this->writeView<2, false>(step, i, mesh, thermal_model, view_data, base_name);
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if (mesh.information_.physical_dimension_[static_cast<Usize>(i)] == 2) {
        this->writeView<2, true>(step, i, mesh, thermal_model, view_data, base_name);
      } else if (mesh.information_.physical_dimension_[static_cast<Usize>(i)] == 3) {
        this->writeView<3, false>(step, i, mesh, thermal_model, view_data, base_name);
      }
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_PARAVIEW_HPP_
