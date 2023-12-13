/**
 * @file Paraview.hpp
 * @brief The header file of SubrosaDG paraview output.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-10
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_PARAVIEW_HPP_
#define SUBROSA_DG_PARAVIEW_HPP_

#include <fmt/core.h>

#include <Eigen/Core>
#include <array>
#include <magic_enum.hpp>
#include <string>
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
inline void View<SimulationControl, ViewModel::Vtu>::getBaseName(const int step, std::string& base_name) {
  base_name = fmt::format("{}_{}", this->output_file_name_prefix_, step);
}

template <typename SimulationControl>
inline void View<SimulationControl, ViewModel::Vtu>::getDataSetInfomatoin(
    std::vector<vtu11::DataSetInfo>& data_set_information) {
  for (const auto variable : this->primary_view_variable_) {
    if (variable == ViewVariable::Velocity || variable == ViewVariable::MachNumber ||
        variable == ViewVariable::Vorticity) {
      data_set_information.emplace_back(magic_enum::enum_name(variable), vtu11::DataSetType::PointData,
                                        SimulationControl::kDimension);
    } else {
      data_set_information.emplace_back(magic_enum::enum_name(variable), vtu11::DataSetType::PointData, 1);
    }
  }
}

template <typename SimulationControl>
inline void View<SimulationControl, ViewModel::Vtu>::calculateViewVariable(
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    Variable<SimulationControl>& variable,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable, const Isize column) {
  variable.calculateComputationalFromConserved(thermal_model);
  for (Isize i = 0; i < static_cast<Isize>(this->primary_view_variable_.size()); i++) {
    if (this->primary_view_variable_[static_cast<Usize>(i)] == ViewVariable::Velocity) {
      node_variable(i)(column * SimulationControl::kDimension) = variable.get(thermal_model, ViewVariable::VelocityX);
      node_variable(i)(column * SimulationControl::kDimension + 1) =
          variable.get(thermal_model, ViewVariable::VelocityY);
      if constexpr (SimulationControl::kDimension == 3) {
        node_variable(i)(column * SimulationControl::kDimension + 2) =
            variable.get(thermal_model, ViewVariable::VelocityZ);
      }
    } else if (this->primary_view_variable_[static_cast<Usize>(i)] == ViewVariable::MachNumber) {
      node_variable(i)(column * SimulationControl::kDimension) = variable.get(thermal_model, ViewVariable::MachNumberX);
      node_variable(i)(column * SimulationControl::kDimension + 1) =
          variable.get(thermal_model, ViewVariable::MachNumberY);
      if constexpr (SimulationControl::kDimension == 3) {
        node_variable(i)(column * SimulationControl::kDimension + 2) =
            variable.get(thermal_model, ViewVariable::MachNumberZ);
      }
    } else if (this->primary_view_variable_[static_cast<Usize>(i)] == ViewVariable::Vorticity) {
      node_variable(i)(column * SimulationControl::kDimension) = variable.get(thermal_model, ViewVariable::VorticityX);
      node_variable(i)(column * SimulationControl::kDimension + 1) =
          variable.get(thermal_model, ViewVariable::VorticityY);
      if constexpr (SimulationControl::kDimension == 3) {
        node_variable(i)(column * SimulationControl::kDimension + 2) =
            variable.get(thermal_model, ViewVariable::VorticityZ);
      }
    } else {
      node_variable(i)(column) = variable.get(thermal_model, this->primary_view_variable_[static_cast<Usize>(i)]);
    }
  }
}

template <typename SimulationControl>
template <typename AdjacencyElementTrait>
inline void View<SimulationControl, ViewModel::Vtu>::writeDiscontinuousAdjacencyElement(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type, const Isize element_index, Isize& column) {
  Variable<SimulationControl> variable;
  std::array<int, AdjacencyElementTrait::kAllNodeNumber> vtk_connectivity{
      getElementVTKConnectivity<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>()};
  const Isize element_gmsh_tag =
      mesh.physical_group_information_.at(physical_name).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type = mesh.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  const Isize node_number = this->view_config_.at(ViewConfig::HighOrderReconstruction)
                                ? AdjacencyElementTrait::kAllNodeNumber
                                : AdjacencyElementTrait::kBasicNodeNumber;
  if constexpr (AdjacencyElementTrait::kElementType == Element::Line) {
    const Isize parent_index_each_type = mesh.line_.element_(element_index).parent_index_each_type_(0);
    const Isize adjacency_sequence_in_parent = mesh.line_.element_(element_index).adjacency_sequence_in_parent_(0);
    const Isize parent_gmsh_type_number = mesh.line_.element_(element_index).parent_gmsh_type_number_(0);
    for (Isize i = 0; i < node_number; i++) {
      for (Isize j = 0; j < SimulationControl::kDimension; j++) {
        node_coordinate(j, i) = mesh.line_.element_(element_index_per_type).node_coordinate_(j, i);
      }
      if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        const Isize adjacency_parent_element_view_basis_function_sequence_in_parent =
            this->data_.line_.getAdjacencyParentElementViewBasisFunctionSequenceInParent(
                parent_gmsh_type_number, adjacency_sequence_in_parent, i);
        variable.conserved_ = this->data_.triangle_.conserved_variable_(parent_index_each_type)
                                  .col(adjacency_parent_element_view_basis_function_sequence_in_parent);
      } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        const Isize adjacency_parent_element_view_basis_function_sequence_in_parent =
            this->data_.line_.getAdjacencyParentElementViewBasisFunctionSequenceInParent(
                parent_gmsh_type_number, adjacency_sequence_in_parent, i);
        variable.conserved_ = this->data_.quadrangle_.conserved_variable_(parent_index_each_type)
                                  .col(adjacency_parent_element_view_basis_function_sequence_in_parent);
      }
      this->calculateViewVariable(thermal_model, variable, node_variable, column + i);
      element_connectivity(column + i) = vtk_connectivity[static_cast<Usize>(i)] + column;
    }
  }
  column += node_number;
  element_offset(element_index) = column;
  element_type(element_index) = AdjacencyElementTrait::kVtkTypeNumber;
}

template <typename SimulationControl>
template <typename ElementTrait>
inline void View<SimulationControl, ViewModel::Vtu>::writeDiscontinuousElement(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type, const Isize element_index, Isize& column) {
  Variable<SimulationControl> variable;
  std::array<int, ElementTrait::kAllNodeNumber> vtk_connectivity{
      getElementVTKConnectivity<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>()};
  const Isize element_gmsh_tag =
      mesh.physical_group_information_.at(physical_name).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type = mesh.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  const Isize node_number = this->view_config_.at(ViewConfig::HighOrderReconstruction) ? ElementTrait::kAllNodeNumber
                                                                                       : ElementTrait::kBasicNodeNumber;
  for (Isize i = 0; i < node_number; i++) {
    for (Isize j = 0; j < SimulationControl::kDimension; j++) {
      if constexpr (ElementTrait::kElementType == Element::Triangle) {
        node_coordinate(j, i) = mesh.triangle_.element_(element_index_per_type).node_coordinate_(j, i);
      } else if constexpr (ElementTrait::kElementType == Element::Quadrangle) {
        node_coordinate(j, i) = mesh.quadrangle_.element_(element_index_per_type).node_coordinate_(j, i);
      }
    }
    if constexpr (ElementTrait::kElementType == Element::Triangle) {
      variable.conserved_ = this->data_.triangle_.conserved_variable_(element_index_per_type).col(i);
    } else if constexpr (ElementTrait::kElementType == Element::Quadrangle) {
      variable.conserved_ = this->data_.quadrangle_.conserved_variable_(element_index_per_type).col(i);
    }
    this->calculateViewVariable(thermal_model, variable, node_variable, column + i);
    element_connectivity(column + i) = vtk_connectivity[static_cast<Usize>(i)] + column;
  }
  column += node_number;
  element_offset(element_index) = column;
  element_type(element_index) = ElementTrait::kVtkTypeNumber;
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void View<SimulationControl, ViewModel::Vtu>::writeDiscontinuousField(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type) {
  const Isize element_number = mesh.physical_group_information_.at(physical_name).element_number_;
  for (Isize i = 0, column = 0; i < element_number; i++) {
    const Isize element_gmsh_type =
        mesh.physical_group_information_.at(physical_name).element_gmsh_type_[static_cast<Usize>(i)];
    if constexpr (Dimension == 1) {
      if (element_gmsh_type == AdjacencyLineTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeDiscontinuousAdjacencyElement<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>>(
            physical_name, mesh, thermal_model, node_coordinate, node_variable, element_connectivity, element_offset,
            element_type, i, column);
      }
    } else if constexpr (Dimension == 2) {
      if constexpr (IsAdjacency) {
      } else {
        if (element_gmsh_type == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeDiscontinuousElement<TriangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_name, mesh, thermal_model, node_coordinate, node_variable, element_connectivity, element_offset,
              element_type, i, column);
        } else if (element_gmsh_type == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeDiscontinuousElement<QuadrangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_name, mesh, thermal_model, node_coordinate, node_variable, element_connectivity, element_offset,
              element_type, i, column);
        }
      }
    }
  }
}

template <typename SimulationControl>
template <typename T>
inline vtu11::VtkIndexType View<SimulationControl, ViewModel::Vtu>::getElementNodeIndex(
    T& elements, const ordered_set<Isize>& node_gmsh_tag, const Isize element_index_per_type, const Isize i) const {
  return static_cast<vtu11::VtkIndexType>(
      node_gmsh_tag.find_index(elements.element_(element_index_per_type).node_tag_(i)));
}

template <typename SimulationControl>
template <typename ElementTrait>
inline void View<SimulationControl, ViewModel::Vtu>::writeContinuousElementConnectivity(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type, const Isize element_index, Isize& column) {
  std::array<int, ElementTrait::kAllNodeNumber> vtk_connectivity{
      getElementVTKConnectivity<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>()};
  const ordered_set<Isize> node_gmsh_tag =
      this->view_config_.at(ViewConfig::HighOrderReconstruction)
          ? mesh.physical_group_information_.at(physical_name).all_node_gmsh_tag_
          : mesh.physical_group_information_.at(physical_name).basic_node_gmsh_tag_;
  const Isize element_gmsh_tag =
      mesh.physical_group_information_.at(physical_name).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type = mesh.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  const Isize node_number = this->view_config_.at(ViewConfig::HighOrderReconstruction) ? ElementTrait::kAllNodeNumber
                                                                                       : ElementTrait::kBasicNodeNumber;
  for (Isize i = 0; i < node_number; i++) {
    if constexpr (ElementTrait::kElementType == Element::Line) {
      element_connectivity(column++) = this->getElementNodeIndex(mesh.line_, node_gmsh_tag, element_index_per_type,
                                                                 vtk_connectivity[static_cast<Usize>(i)]);
    } else if constexpr (ElementTrait::kElementType == Element::Triangle) {
      element_connectivity(column++) = this->getElementNodeIndex(mesh.triangle_, node_gmsh_tag, element_index_per_type,
                                                                 vtk_connectivity[static_cast<Usize>(i)]);
    } else if constexpr (ElementTrait::kElementType == Element::Quadrangle) {
      element_connectivity(column++) = this->getElementNodeIndex(
          mesh.quadrangle_, node_gmsh_tag, element_index_per_type, vtk_connectivity[static_cast<Usize>(i)]);
    }
  }
  element_offset(element_index) = column;
  element_type(element_index) = ElementTrait::kVtkTypeNumber;
}

template <typename SimulationControl>
template <int Dimension>
inline void View<SimulationControl, ViewModel::Vtu>::writeContinuousField(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type) {
  Variable<SimulationControl> variable;
  const Isize element_number = mesh.physical_group_information_.at(physical_name).element_number_;
  const ordered_set<Isize> node_gmsh_tag =
      this->view_config_.at(ViewConfig::HighOrderReconstruction)
          ? mesh.physical_group_information_.at(physical_name).all_node_gmsh_tag_
          : mesh.physical_group_information_.at(physical_name).basic_node_gmsh_tag_;
  for (Isize i = 0; i < static_cast<Isize>(node_gmsh_tag.size()); i++) {
    for (Isize j = 0; j < SimulationControl::kDimension; j++) {
      node_coordinate(j, i) = mesh.node_coordinate_(j, node_gmsh_tag[static_cast<Usize>(i)] - 1);
    }
    variable.conserved_ = this->data_.node_conserved_variable_.col(node_gmsh_tag[static_cast<Usize>(i)] - 1);
    this->calculateViewVariable(thermal_model, variable, node_variable, i);
  }
  for (Isize i = 0, column = 0; i < element_number; i++) {
    const Isize element_gmsh_type =
        mesh.physical_group_information_.at(physical_name).element_gmsh_type_[static_cast<Usize>(i)];
    if constexpr (Dimension == 1) {
      if (element_gmsh_type == LineTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeContinuousElementConnectivity<LineTrait<SimulationControl::kPolynomialOrder>>(
            physical_name, mesh, element_connectivity, element_offset, element_type, i, column);
      }
    }
    if constexpr (Dimension == 2) {
      if (element_gmsh_type == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeContinuousElementConnectivity<TriangleTrait<SimulationControl::kPolynomialOrder>>(
            physical_name, mesh, element_connectivity, element_offset, element_type, i, column);
      } else if (element_gmsh_type == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeContinuousElementConnectivity<QuadrangleTrait<SimulationControl::kPolynomialOrder>>(
            physical_name, mesh, element_connectivity, element_offset, element_type, i, column);
      }
    }
  }
}

template <typename SimulationControl>
inline Isize View<SimulationControl, ViewModel::Vtu>::getElementConnectivityNumber(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh) {
  Isize element_connectivity_number = 0;
  for (const auto gmsh_type : mesh.physical_group_information_.at(physical_name).element_gmsh_type_) {
    if (this->view_config_.at(ViewConfig::HighOrderReconstruction)) {
      if (gmsh_type == LineTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        element_connectivity_number += LineTrait<SimulationControl::kPolynomialOrder>::kAllNodeNumber;
      } else if (gmsh_type == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        element_connectivity_number += TriangleTrait<SimulationControl::kPolynomialOrder>::kAllNodeNumber;
      } else if (gmsh_type == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        element_connectivity_number += QuadrangleTrait<SimulationControl::kPolynomialOrder>::kAllNodeNumber;
      }
    } else {
      if (gmsh_type == LineTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        element_connectivity_number += LineTrait<SimulationControl::kPolynomialOrder>::kBasicNodeNumber;
      } else if (gmsh_type == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        element_connectivity_number += TriangleTrait<SimulationControl::kPolynomialOrder>::kBasicNodeNumber;
      } else if (gmsh_type == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        element_connectivity_number += QuadrangleTrait<SimulationControl::kPolynomialOrder>::kBasicNodeNumber;
      }
    }
  }
  return element_connectivity_number;
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void View<SimulationControl, ViewModel::Vtu>::writeView(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    const std::string& base_name, const std::vector<vtu11::DataSetInfo>& data_set_information) {
  Eigen::Matrix<Real, 3, Eigen::Dynamic> node_coordinate;
  Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1> node_variable;
  Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic> element_connectivity;
  Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic> element_offset;
  Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic> element_type;
  Isize node_number;
  if (this->view_config_.at(ViewConfig::HighOrderReconstruction)) {
    node_number = this->view_config_.at(ViewConfig::SolverSmoothness)
                      ? static_cast<Isize>(mesh.physical_group_information_.at(physical_name).all_node_gmsh_tag_.size())
                      : mesh.physical_group_information_.at(physical_name).all_node_number_;
  } else {
    node_number =
        this->view_config_.at(ViewConfig::SolverSmoothness)
            ? static_cast<Isize>(mesh.physical_group_information_.at(physical_name).basic_node_gmsh_tag_.size())
            : mesh.physical_group_information_.at(physical_name).basic_node_number_;
  }
  node_coordinate.resize(Eigen::NoChange, node_number);
  node_coordinate.setZero();
  node_variable.resize(static_cast<Isize>(this->primary_view_variable_.size()));
  for (Isize i = 0; const auto variable : this->primary_view_variable_) {
    if (variable == ViewVariable::Velocity || variable == ViewVariable::MachNumber ||
        variable == ViewVariable::Vorticity) {
      node_variable(i++).resize(SimulationControl::kDimension * node_number);
    } else {
      node_variable(i++).resize(node_number);
    }
  }
  const Isize element_connectivity_number = this->getElementConnectivityNumber(physical_name, mesh);
  const Isize element_number = mesh.physical_group_information_.at(physical_name).element_number_;
  element_connectivity.resize(element_connectivity_number);
  element_offset.resize(element_number);
  element_type.resize(element_number);
  if (this->view_config_.at(ViewConfig::SolverSmoothness)) {
    this->writeContinuousField<Dimension>(physical_name, mesh, thermal_model, node_coordinate, node_variable,
                                          element_connectivity, element_offset, element_type);
  } else {
    this->writeDiscontinuousField<Dimension, IsAdjacency>(physical_name, mesh, thermal_model, node_coordinate,
                                                          node_variable, element_connectivity, element_offset,
                                                          element_type);
  }
  vtu11::Vtu11UnstructuredMesh mesh_data{
      {node_coordinate.data(), node_coordinate.data() + node_coordinate.size()},
      {element_connectivity.data(), element_connectivity.data() + element_connectivity.size()},
      {element_offset.data(), element_offset.data() + element_offset.size()},
      {element_type.data(), element_type.data() + element_type.size()}};
#ifdef SUBROSA_DG_DEVELOP
  std::string write_mode = "ascii";
#else
  std::string write_mode = "rawbinarycompressed";
#endif
  std::vector<std::vector<double>> vtu_node_data;
  vtu_node_data.resize(this->primary_view_variable_.size());
  for (Isize i = 0; i < static_cast<Isize>(this->primary_view_variable_.size()); i++) {
    vtu_node_data[static_cast<Usize>(i)].assign(node_variable(i).data(),
                                                node_variable(i).data() + node_variable(i).size());
  }
  vtu11::writePartition((this->output_directory_ / "vtu").string(), base_name, physical_name, mesh_data,
                        data_set_information, vtu_node_data, write_mode);
}

template <typename SimulationControl>
inline void View<SimulationControl, ViewModel::Vtu>::stepView(
    const int step, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
  std::string base_name;
  std::vector<std::string> physical_group_name;
  std::vector<vtu11::DataSetInfo> data_set_information;
  this->data_.readRawBinary(mesh, this->raw_binary_finout_);
  if (this->view_config_.at(ViewConfig::SolverSmoothness)) {
    this->data_.calculateNodeConservedVariable(mesh);
  }
  this->getBaseName(step, base_name);
  this->getDataSetInfomatoin(data_set_information);
  for (const auto& [dim, physical_name] : mesh.physical_group_) {
    physical_group_name.emplace_back(physical_name);
  }
  vtu11::writePVtu((this->output_directory_ / "vtu").string(), base_name, physical_group_name, data_set_information,
                   physical_group_name.size());
  for (const auto& [dim, physical_name] : mesh.physical_group_) {
    if constexpr (SimulationControl::kDimension == 2) {
      if (dim == 1) {
        this->writeView<1, true>(physical_name, mesh, thermal_model, base_name, data_set_information);
      } else if (dim == 2) {
        this->writeView<2, false>(physical_name, mesh, thermal_model, base_name, data_set_information);
      }
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_PARAVIEW_HPP_
