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
#include <fstream>
#include <magic_enum.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <vtu11-cpp17.hpp>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"
#include "View/IOControl.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
inline std::string ViewBase<SimulationControl, ViewModelEnum::Vtu>::getBaseName(const int step,
                                                                                const std::string_view physical_name) {
  return std::format("{}_{}_{:0{}d}.vtu", this->output_file_name_prefix_, physical_name, step, this->iteration_order_);
}

template <typename SimulationControl>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::getDataSetInfomatoin(
    std::vector<vtu11::DataSetInfo>& data_set_information) {
  data_set_information.emplace_back("TMSTEP", vtu11::DataSetType::FieldData, 1, 1);
  data_set_information.emplace_back("TimeValue", vtu11::DataSetType::FieldData, 1, 1);
  for (const auto variable : this->variable_type_) {
    if (variable == ViewVariableEnum::Velocity || variable == ViewVariableEnum::MachNumber ||
        (variable == ViewVariableEnum::Vorticity && SimulationControl::kDimension == 3)) {
      data_set_information.emplace_back(magic_enum::enum_name(variable), vtu11::DataSetType::PointData,
                                        SimulationControl::kDimension, 0);
    } else {
      data_set_information.emplace_back(magic_enum::enum_name(variable), vtu11::DataSetType::PointData, 1, 0);
    }
  }
}

template <typename SimulationControl>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::calculateViewVariable(
    const ThermalModel<SimulationControl>& thermal_model, const ViewVariable<SimulationControl>& view_variable,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable, const Isize column) {
  auto handle_variable = [&view_variable, &thermal_model, &node_variable, column](Isize i, ViewVariableEnum variable_x,
                                                                                  ViewVariableEnum variable_y,
                                                                                  ViewVariableEnum variable_z) {
    node_variable(i)(column * SimulationControl::kDimension) = view_variable.getView(thermal_model, variable_x);
    if constexpr (SimulationControl::kDimension >= 2) {
      node_variable(i)(column * SimulationControl::kDimension + 1) = view_variable.getView(thermal_model, variable_y);
    }
    if constexpr (SimulationControl::kDimension >= 3) {
      node_variable(i)(column * SimulationControl::kDimension + 2) = view_variable.getView(thermal_model, variable_z);
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
      node_variable(i)(column) = view_variable.getView(thermal_model, this->variable_type_[static_cast<Usize>(i)]);
    }
  }
}

template <typename SimulationControl>
template <typename AdjacencyElementTrait>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::writeAdjacencyElement(
    const Isize physical_index, const MeshInformation& mesh_information,
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const ThermalModel<SimulationControl>& thermal_model, const ViewData<SimulationControl>& view_data,
    Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type, const Isize element_index, Isize& column) {
  constexpr std::array<int, AdjacencyElementTrait::kAllNodeNumber> kVtkConnectivity{
      getElementVTKConnectivity<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>()};
  const Isize element_gmsh_tag =
      mesh_information.physical_information_.at(physical_index).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type =
      mesh_information.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  const Isize parent_index_each_type =
      adjacency_element_mesh.element_(element_index_per_type).parent_index_each_type_(0);
  const Isize adjacency_sequence_in_parent =
      adjacency_element_mesh.element_(element_index_per_type).adjacency_sequence_in_parent_(0);
  const Isize parent_gmsh_type_number =
      adjacency_element_mesh.element_(element_index_per_type).parent_gmsh_type_number_(0);
  for (Isize i = 0; i < AdjacencyElementTrait::kAllNodeNumber; i++) {
    for (Isize j = 0; j < SimulationControl::kDimension; j++) {
      node_coordinate(j, column + i) = adjacency_element_mesh.element_(element_index_per_type).node_coordinate_(j, i);
    }
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      const Isize adjacency_parent_element_view_basis_function_sequence_in_parent =
          view_data.solver_.line_.getAdjacencyParentElementViewBasisFunctionSequenceInParent(
              parent_gmsh_type_number, adjacency_sequence_in_parent, i);
      if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->calculateViewVariable(
            thermal_model,
            view_data.solver_.triangle_.view_variable_(adjacency_parent_element_view_basis_function_sequence_in_parent,
                                                       parent_index_each_type),
            node_variable, column + i);
      } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->calculateViewVariable(
            thermal_model,
            view_data.solver_.quadrangle_.view_variable_(
                adjacency_parent_element_view_basis_function_sequence_in_parent, parent_index_each_type),
            node_variable, column + i);
      }
    }
    element_connectivity(column + i) = kVtkConnectivity[static_cast<Usize>(i)] + column;
  }
  column += AdjacencyElementTrait::kAllNodeNumber;
  element_offset(element_index) = column;
  element_type(element_index) = AdjacencyElementTrait::kVtkTypeNumber;
}

template <typename SimulationControl>
template <typename ElementTrait>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::writeElement(
    const Isize physical_index, const MeshInformation& mesh_information, const ElementMesh<ElementTrait>& element_mesh,
    const ThermalModel<SimulationControl>& thermal_model, const ViewData<SimulationControl>& view_data,
    Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type, const Isize element_index, Isize& column) {
  const ElementViewSolver<ElementTrait, SimulationControl, SimulationControl::kEquationModel>& element_view_solver =
      view_data.solver_.*(decltype(view_data.solver_)::template getElement<ElementTrait>());
  const std::array<int, ElementTrait::kAllNodeNumber> vtk_connectivity{
      getElementVTKConnectivity<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>()};
  const Isize element_gmsh_tag =
      mesh_information.physical_information_.at(physical_index).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type =
      mesh_information.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
    for (Isize j = 0; j < SimulationControl::kDimension; j++) {
      node_coordinate(j, column + i) = element_mesh.element_(element_index_per_type).node_coordinate_(j, i);
    }
    this->calculateViewVariable(thermal_model, element_view_solver.view_variable_(i, element_index_per_type),
                                node_variable, column + i);
    element_connectivity(column + i) = vtk_connectivity[static_cast<Usize>(i)] + column;
  }
  column += ElementTrait::kAllNodeNumber;
  element_offset(element_index) = column;
  element_type(element_index) = ElementTrait::kVtkTypeNumber;
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::writeField(
    const Isize physical_index, const Mesh<SimulationControl>& mesh,
    const ThermalModel<SimulationControl>& thermal_model, const ViewData<SimulationControl>& view_data,
    Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type) {
  const Isize element_number = mesh.information_.physical_information_.at(physical_index).element_number_;
  for (Isize i = 0, column = 0; i < element_number; i++) {
    const Isize element_gmsh_type =
        mesh.information_.physical_information_.at(physical_index).element_gmsh_type_[static_cast<Usize>(i)];
    if constexpr (Dimension == 1) {
      if constexpr (IsAdjacency) {
        this->writeAdjacencyElement<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.line_, thermal_model, view_data, node_coordinate, node_variable,
            element_connectivity, element_offset, element_type, i, column);
      } else {
        this->writeElement<LineTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.line_, thermal_model, view_data, node_coordinate, node_variable,
            element_connectivity, element_offset, element_type, i, column);
      }
    } else if constexpr (Dimension == 2) {
      if constexpr (IsAdjacency) {
      } else {
        if (element_gmsh_type == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeElement<TriangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_index, mesh.information_, mesh.triangle_, thermal_model, view_data, node_coordinate,
              node_variable, element_connectivity, element_offset, element_type, i, column);
        } else if (element_gmsh_type == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeElement<QuadrangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_index, mesh.information_, mesh.quadrangle_, thermal_model, view_data, node_coordinate,
              node_variable, element_connectivity, element_offset, element_type, i, column);
        }
      }
    }
  }
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::writeView(
    const int step, const Isize physical_index, const Mesh<SimulationControl>& mesh,
    const ThermalModel<SimulationControl>& thermal_model, const ViewData<SimulationControl>& view_data,
    const std::string& base_name) {
  Eigen::Matrix<Real, 3, Eigen::Dynamic> node_coordinate;
  Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1> node_variable;
  Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic> element_connectivity;
  Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic> element_offset;
  Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic> element_type;
  std::vector<vtu11::DataSetInfo> data_set_information;
  std::vector<vtu11::DataSetData> data_set_data;
  this->getDataSetInfomatoin(data_set_information);
  data_set_data.resize(this->variable_type_.size() + 2);
  const Isize node_number = mesh.information_.physical_information_.at(physical_index).node_number_;
  node_coordinate.resize(Eigen::NoChange, node_number);
  node_coordinate.setZero();
  node_variable.resize(static_cast<Isize>(this->variable_type_.size()));
  for (Isize i = 0; const auto variable : this->variable_type_) {
    if (variable == ViewVariableEnum::Velocity || variable == ViewVariableEnum::MachNumber ||
        (variable == ViewVariableEnum::Vorticity && Dimension == 3)) {
      node_variable(i++).resize(SimulationControl::kDimension * node_number);
    } else {
      node_variable(i++).resize(node_number);
    }
  }
  const Isize element_number = mesh.information_.physical_information_.at(physical_index).element_number_;
  element_connectivity.resize(mesh.information_.physical_information_.at(physical_index).node_number_);
  element_offset.resize(element_number);
  element_type.resize(element_number);
  this->writeField<Dimension, IsAdjacency>(physical_index, mesh, thermal_model, view_data, node_coordinate,
                                           node_variable, element_connectivity, element_offset, element_type);
  vtu11::Vtu11UnstructuredMesh mesh_data{
      {node_coordinate.data(), node_coordinate.data() + node_coordinate.size()},
      {element_connectivity.data(), element_connectivity.data() + element_connectivity.size()},
      {element_offset.data(), element_offset.data() + element_offset.size()},
      {element_type.data(), element_type.data() + element_type.size()}};
  data_set_data[0].emplace_back(step);
  data_set_data[1].emplace_back(this->time_value_(step));
  for (Isize i = 0; i < static_cast<Isize>(this->variable_type_.size()); i++) {
    data_set_data[static_cast<Usize>(i) + 2].assign(node_variable(i).data(),
                                                    node_variable(i).data() + node_variable(i).size());
  }
  vtu11::writeVtu((this->output_directory_ / "vtu" / base_name).string(), mesh_data, data_set_information,
                  data_set_data, "rawbinarycompressed");
}

template <typename SimulationControl>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::stepView(
    const int step, const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
    ViewData<SimulationControl>& view_data) {
  view_data.solver_.calcluateViewVariable(mesh, thermal_model, view_data.raw_binary_path_, view_data.raw_binary_ss_);
  for (Isize i = 0; i < static_cast<Isize>(mesh.information_.physical_.size()); i++) {
    if (mesh.information_.periodic_physical_.contains(i)) {
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
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_PARAVIEW_HPP_
