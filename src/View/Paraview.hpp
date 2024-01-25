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
                                                                                const std::string_view physical_index) {
  return std::format("{}_{}_{:0{}d}.vtu", this->output_file_name_prefix_, physical_index, step, this->iteration_order_);
}

template <typename SimulationControl>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::getDataSetInfomatoin(
    std::vector<vtu11::DataSetInfo>& data_set_information) {
  data_set_information.emplace_back("TMSTEP", vtu11::DataSetType::FieldData, 1, 1);
  data_set_information.emplace_back("TimeValue", vtu11::DataSetType::FieldData, 1, 1);
  for (const auto variable : this->variable_vector_) {
    if (variable == ViewVariableEnum::Velocity || variable == ViewVariableEnum::MachNumber ||
        variable == ViewVariableEnum::Vorticity) {
      data_set_information.emplace_back(magic_enum::enum_name(variable), vtu11::DataSetType::PointData,
                                        SimulationControl::kDimension, 0);
    } else {
      data_set_information.emplace_back(magic_enum::enum_name(variable), vtu11::DataSetType::PointData, 1, 0);
    }
  }
}

template <typename SimulationControl>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::calculateViewVariable(
    const ThermalModel<SimulationControl>& thermal_model, Variable<SimulationControl>& variable,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable, const Isize column) {
  auto handle_variable = [&variable, &thermal_model, &node_variable, column](Isize i, ViewVariableEnum variable_x,
                                                                             ViewVariableEnum variable_y,
                                                                             ViewVariableEnum variable_z) {
    node_variable(i)(column * SimulationControl::kDimension) = variable.get(thermal_model, variable_x);
    if constexpr (SimulationControl::kDimension >= 2) {
      node_variable(i)(column * SimulationControl::kDimension + 1) = variable.get(thermal_model, variable_y);
    }
    if constexpr (SimulationControl::kDimension >= 3) {
      node_variable(i)(column * SimulationControl::kDimension + 2) = variable.get(thermal_model, variable_z);
    }
  };
  for (Isize i = 0; i < static_cast<Isize>(this->variable_vector_.size()); i++) {
    if (this->variable_vector_[static_cast<Usize>(i)] == ViewVariableEnum::Velocity) {
      handle_variable(i, ViewVariableEnum::VelocityX, ViewVariableEnum::VelocityY, ViewVariableEnum::VelocityZ);
    } else if (this->variable_vector_[static_cast<Usize>(i)] == ViewVariableEnum::MachNumber) {
      handle_variable(i, ViewVariableEnum::MachNumberX, ViewVariableEnum::MachNumberY, ViewVariableEnum::MachNumberZ);
    } else if (this->variable_vector_[static_cast<Usize>(i)] == ViewVariableEnum::Vorticity) {
      handle_variable(i, ViewVariableEnum::VorticityX, ViewVariableEnum::VorticityY, ViewVariableEnum::VorticityZ);
    } else {
      node_variable(i)(column) = variable.get(thermal_model, this->variable_vector_[static_cast<Usize>(i)]);
    }
  }
}

template <typename SimulationControl>
template <typename AdjacencyElementTrait>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::writeDiscontinuousAdjacencyElement(
    const Isize physical_index, const MeshInformation& mesh_information,
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const ThermalModel<SimulationControl>& thermal_model, Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type, const Isize element_index, Isize& column) {
  Variable<SimulationControl> variable;
  const std::array<int, AdjacencyElementTrait::kAllNodeNumber> vtk_connectivity{
      getElementVTKConnectivity<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>()};
  const Isize element_gmsh_tag =
      mesh_information.physical_information_.at(physical_index).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type =
      mesh_information.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  const Isize parent_index_each_type = adjacency_element_mesh.element_(element_index).parent_index_each_type_(0);
  const Isize adjacency_sequence_in_parent =
      adjacency_element_mesh.element_(element_index).adjacency_sequence_in_parent_(0);
  const Isize parent_gmsh_type_number = adjacency_element_mesh.element_(element_index).parent_gmsh_type_number_(0);
  for (Isize i = 0; i < AdjacencyElementTrait::kAllNodeNumber; i++) {
    for (Isize j = 0; j < SimulationControl::kDimension; j++) {
      node_coordinate(j, i) = adjacency_element_mesh.element_(element_index_per_type).node_coordinate_(j, i);
    }
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      const Isize adjacency_parent_element_view_basis_function_sequence_in_parent =
          this->variable_.line_.getAdjacencyParentElementViewBasisFunctionSequenceInParent(
              parent_gmsh_type_number, adjacency_sequence_in_parent, i);
      if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        variable.conserved_ = this->variable_.triangle_.conserved_variable_(parent_index_each_type)
                                  .col(adjacency_parent_element_view_basis_function_sequence_in_parent);
      } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        variable.conserved_ = this->variable_.quadrangle_.conserved_variable_(parent_index_each_type)
                                  .col(adjacency_parent_element_view_basis_function_sequence_in_parent);
      }
    }
    variable.calculateComputationalFromConserved(thermal_model);
    this->calculateViewVariable(thermal_model, variable, node_variable, column + i);
    element_connectivity(column + i) = vtk_connectivity[static_cast<Usize>(i)] + column;
  }
  column += AdjacencyElementTrait::kAllNodeNumber;
  element_offset(element_index) = column;
  element_type(element_index) = AdjacencyElementTrait::kVtkTypeNumber;
}

template <typename SimulationControl>
template <typename ElementTrait>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::writeDiscontinuousElement(
    const Isize physical_index, const MeshInformation& mesh_information, const ElementMesh<ElementTrait>& element_mesh,
    const ThermalModel<SimulationControl>& thermal_model, Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type, const Isize element_index, Isize& column) {
  Variable<SimulationControl> variable;
  const ElementViewVariable<ElementTrait, SimulationControl>& element_view_variable =
      this->variable_.*(decltype(this->variable_)::template getElement<ElementTrait>());
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
    variable.conserved_ = element_view_variable.conserved_variable_(element_index_per_type).col(i);
    variable.calculateComputationalFromConserved(thermal_model);
    this->calculateViewVariable(thermal_model, variable, node_variable, column + i);
    element_connectivity(column + i) = vtk_connectivity[static_cast<Usize>(i)] + column;
  }
  column += ElementTrait::kAllNodeNumber;
  element_offset(element_index) = column;
  element_type(element_index) = ElementTrait::kVtkTypeNumber;
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::writeDiscontinuousField(
    const Isize physical_index, const Mesh<SimulationControl>& mesh,
    const ThermalModel<SimulationControl>& thermal_model, Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
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
        this->writeDiscontinuousAdjacencyElement<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.line_, thermal_model, node_coordinate, node_variable,
            element_connectivity, element_offset, element_type, i, column);
      } else {
        this->writeDiscontinuousElement<LineTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.line_, thermal_model, node_coordinate, node_variable,
            element_connectivity, element_offset, element_type, i, column);
      }
    } else if constexpr (Dimension == 2) {
      if constexpr (IsAdjacency) {
      } else {
        if (element_gmsh_type == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeDiscontinuousElement<TriangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_index, mesh.information_, mesh.triangle_, thermal_model, node_coordinate, node_variable,
              element_connectivity, element_offset, element_type, i, column);
        } else if (element_gmsh_type == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeDiscontinuousElement<QuadrangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_index, mesh.information_, mesh.quadrangle_, thermal_model, node_coordinate, node_variable,
              element_connectivity, element_offset, element_type, i, column);
        }
      }
    }
  }
}

template <typename SimulationControl>
template <typename AdjacencyElementTrait>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::writeContinuousAdjacencyElementConnectivity(
    const Isize physical_index, const MeshInformation& mesh_information,
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type, const Isize element_index, Isize& column) {
  const std::array<int, AdjacencyElementTrait::kAllNodeNumber> vtk_connectivity{
      getElementVTKConnectivity<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>()};
  const ordered_set<Isize> node_gmsh_tag = mesh_information.physical_information_.at(physical_index).node_gmsh_tag_;
  const Isize element_gmsh_tag =
      mesh_information.physical_information_.at(physical_index).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type =
      mesh_information.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  for (Isize i = 0; i < AdjacencyElementTrait::kAllNodeNumber; i++) {
    element_connectivity(column++) = static_cast<vtu11::VtkIndexType>(node_gmsh_tag.find_index(
        adjacency_element_mesh.element_(element_index_per_type).node_tag_(vtk_connectivity[static_cast<Usize>(i)])));
  }
  element_offset(element_index) = column;
  element_type(element_index) = AdjacencyElementTrait::kVtkTypeNumber;
}

template <typename SimulationControl>
template <typename ElementTrait>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::writeContinuousElementConnectivity(
    const Isize physical_index, const MeshInformation& mesh_information, const ElementMesh<ElementTrait>& element_mesh,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type, const Isize element_index, Isize& column) {
  const std::array<int, ElementTrait::kAllNodeNumber> vtk_connectivity{
      getElementVTKConnectivity<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>()};
  const ordered_set<Isize> node_gmsh_tag = mesh_information.physical_information_.at(physical_index).node_gmsh_tag_;
  const Isize element_gmsh_tag =
      mesh_information.physical_information_.at(physical_index).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type =
      mesh_information.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
    element_connectivity(column++) = static_cast<vtu11::VtkIndexType>(node_gmsh_tag.find_index(
        element_mesh.element_(element_index_per_type).node_tag_(vtk_connectivity[static_cast<Usize>(i)])));
  }
  element_offset(element_index) = column;
  element_type(element_index) = ElementTrait::kVtkTypeNumber;
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::writeContinuousField(
    const Isize physical_index, const Mesh<SimulationControl>& mesh,
    const ThermalModel<SimulationControl>& thermal_model, Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type) {
  Variable<SimulationControl> variable;
  const Isize element_number = mesh.information_.physical_information_.at(physical_index).element_number_;
  const ordered_set<Isize> node_gmsh_tag = mesh.information_.physical_information_.at(physical_index).node_gmsh_tag_;
  for (Isize i = 0; i < static_cast<Isize>(node_gmsh_tag.size()); i++) {
    for (Isize j = 0; j < SimulationControl::kDimension; j++) {
      node_coordinate(j, i) = mesh.node_coordinate_(j, node_gmsh_tag[static_cast<Usize>(i)] - 1);
    }
    variable.conserved_ = this->variable_.node_conserved_variable_.col(node_gmsh_tag[static_cast<Usize>(i)] - 1);
    variable.calculateComputationalFromConserved(thermal_model);
    this->calculateViewVariable(thermal_model, variable, node_variable, i);
  }
  for (Isize i = 0, column = 0; i < element_number; i++) {
    const Isize element_gmsh_type =
        mesh.information_.physical_information_.at(physical_index).element_gmsh_type_[static_cast<Usize>(i)];
    if constexpr (Dimension == 1) {
      if constexpr (IsAdjacency) {
        this->writeContinuousAdjacencyElementConnectivity<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.line_, element_connectivity, element_offset, element_type, i,
            column);
      } else {
        this->writeContinuousElementConnectivity<LineTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.line_, element_connectivity, element_offset, element_type, i,
            column);
      }
    }
    if constexpr (Dimension == 2) {
      if constexpr (IsAdjacency) {
      } else {
        if (element_gmsh_type == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeContinuousElementConnectivity<TriangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_index, mesh.information_, mesh.triangle_, element_connectivity, element_offset, element_type, i,
              column);
        } else if (element_gmsh_type == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeContinuousElementConnectivity<QuadrangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_index, mesh.information_, mesh.quadrangle_, element_connectivity, element_offset, element_type,
              i, column);
        }
      }
    }
  }
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::writeView(
    const int step, const Isize physical_index, const Mesh<SimulationControl>& mesh,
    const ThermalModel<SimulationControl>& thermal_model, const std::string& base_name) {
  Eigen::Matrix<Real, 3, Eigen::Dynamic> node_coordinate;
  Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1> node_variable;
  Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic> element_connectivity;
  Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic> element_offset;
  Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic> element_type;
  std::vector<vtu11::DataSetInfo> data_set_information;
  std::vector<vtu11::DataSetData> data_set_data;
  this->getDataSetInfomatoin(data_set_information);
  data_set_data.resize(this->variable_vector_.size() + 2);
  const Isize node_number =
      ((this->config_enum_ & ViewConfigEnum::SolverSmoothness) == ViewConfigEnum::SolverSmoothness)
          ? static_cast<Isize>(mesh.information_.physical_information_.at(physical_index).node_gmsh_tag_.size())
          : mesh.information_.physical_information_.at(physical_index).node_number_;
  node_coordinate.resize(Eigen::NoChange, node_number);
  node_coordinate.setZero();
  node_variable.resize(static_cast<Isize>(this->variable_vector_.size()));
  for (Isize i = 0; const auto variable : this->variable_vector_) {
    if (variable == ViewVariableEnum::Velocity || variable == ViewVariableEnum::MachNumber ||
        variable == ViewVariableEnum::Vorticity) {
      node_variable(i++).resize(SimulationControl::kDimension * node_number);
    } else {
      node_variable(i++).resize(node_number);
    }
  }
  const Isize element_number = mesh.information_.physical_information_.at(physical_index).element_number_;
  element_connectivity.resize(mesh.information_.physical_information_.at(physical_index).node_number_);
  element_offset.resize(element_number);
  element_type.resize(element_number);
  if ((this->config_enum_ & ViewConfigEnum::SolverSmoothness) == ViewConfigEnum::SolverSmoothness) {
    this->writeContinuousField<Dimension, IsAdjacency>(physical_index, mesh, thermal_model, node_coordinate,
                                                       node_variable, element_connectivity, element_offset,
                                                       element_type);
  } else {
    this->writeDiscontinuousField<Dimension, IsAdjacency>(physical_index, mesh, thermal_model, node_coordinate,
                                                          node_variable, element_connectivity, element_offset,
                                                          element_type);
  }
  vtu11::Vtu11UnstructuredMesh mesh_data{
      {node_coordinate.data(), node_coordinate.data() + node_coordinate.size()},
      {element_connectivity.data(), element_connectivity.data() + element_connectivity.size()},
      {element_offset.data(), element_offset.data() + element_offset.size()},
      {element_type.data(), element_type.data() + element_type.size()}};
#ifdef SUBROSA_DG_DEVELOP
  const std::string write_mode = "ascii";
#else
  const std::string write_mode = "rawbinarycompressed";
#endif
  data_set_data[0].emplace_back(step);
  data_set_data[1].emplace_back(this->variable_.time_value_(step - 1));
  for (Isize i = 0; i < static_cast<Isize>(this->variable_vector_.size()); i++) {
    data_set_data[static_cast<Usize>(i) + 2].assign(node_variable(i).data(),
                                                    node_variable(i).data() + node_variable(i).size());
  }
  vtu11::writeVtu((this->output_directory_ / "vtu" / base_name).string(), mesh_data, data_set_information,
                  data_set_data, write_mode);
}

template <typename SimulationControl>
inline void ViewBase<SimulationControl, ViewModelEnum::Vtu>::stepView(
    const int step, const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model) {
  this->variable_.readRawBinary(mesh, this->config_enum_, this->raw_binary_finout_);
  for (Isize i = 0; i < static_cast<Isize>(mesh.information_.physical_.size()); i++) {
    if (mesh.information_.periodic_physical_.contains(i)) {
      continue;
    }
    const std::string base_name = this->getBaseName(step, mesh.information_.physical_[static_cast<Usize>(i)]);
    if constexpr (SimulationControl::kDimension == 1) {
      if (mesh.information_.physical_dimension_[static_cast<Usize>(i)] == 1) {
        this->writeView<1, false>(step, i, mesh, thermal_model, base_name);
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if (mesh.information_.physical_dimension_[static_cast<Usize>(i)] == 1) {
        this->writeView<1, true>(step, i, mesh, thermal_model, base_name);
      } else if (mesh.information_.physical_dimension_[static_cast<Usize>(i)] == 2) {
        this->writeView<2, false>(step, i, mesh, thermal_model, base_name);
      }
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_PARAVIEW_HPP_
