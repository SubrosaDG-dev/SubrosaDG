/**
 * @file TecplotAscii.hpp
 * @brief The head file of SubroseDG Tecplot ASCII file output.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-18
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_TECPLOT_ASCII_HPP_
#define SUBROSA_DG_TECPLOT_ASCII_HPP_

#include <fmt/core.h>

#include <Eigen/Core>
#include <fstream>
#include <string>
#include <string_view>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"
#include "View/IOControl.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
inline void View<SimulationControl, ViewModel::Dat>::updateViewFout(const int step) {
  std::string view_file_suffix;
  view_file_suffix = fmt::format("dat/{}_{}.dat", this->output_file_name_prefix_, step);
  this->view_fout_.open((this->output_directory_ / view_file_suffix).string(), std::ios::out | std::ios::trunc);
}

template <typename SimulationControl>
inline void writeTecplotAsciiVariableList(std::ofstream& view_fout) {
  std::string variable_list;
  if constexpr (SimulationControl::kDimension == 2) {
    variable_list = R"(VARIABLES = "X", "Y", "Density", "VelocityX", "VelocityY", "Pressure", "Temperature")";
  } else if constexpr (SimulationControl::kDimension == 3) {
    variable_list =
        R"(VARIABLES = "X", "Y", "Z", "Density", "VelocityX", "VelocityY", "VelocityZ", "Pressure", "Temperature")";
  }
  view_fout << variable_list << '\n';
}

template <int Dimension>
inline void writeTecplotAsciiHeader(const std::string_view physical_name, const Isize node_number,
                                    const Isize element_number, std::ofstream& view_fout) {
  std::string header;
  if constexpr (Dimension == 1) {
    header = fmt::format(R"(Zone T="{}", ZONETYPE=FELINESEG, NODES={}, ELEMENTS={}, DATAPACKING=POINT)", physical_name,
                         node_number, element_number);
  } else if constexpr (Dimension == 2) {
    header = fmt::format(R"(Zone T="{}", ZONETYPE=FEQUADRILATERAL, NODES={}, ELEMENTS={}, DATAPACKING=POINT)",
                         physical_name, node_number, element_number);
  } else if constexpr (Dimension == 3) {
    header = fmt::format(R"(Zone T="{}", ZONETYPE=FEBRICK, NODES={}, ELEMENTS={}, DATAPACKING=POINT)", physical_name,
                         node_number, element_number);
  }
  view_fout << header << '\n';
}

template <typename SimulationControl>
inline void writeTecplotFELinesegAdjacencyElement(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    const ViewData<SimulationControl, SimulationControl::kDimension>& view_data, std::ofstream& view_fout) {
  Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic> node_coordinate;
  Eigen::Matrix<Real, SimulationControl::kPrimitiveVariableNumber, Eigen::Dynamic> node_primitive_variable;
  Eigen::Matrix<Real, SimulationControl::kDimension + SimulationControl::kPrimitiveVariableNumber, Eigen::Dynamic>
      node_all_variable;
  Eigen::Matrix<int, 2, Eigen::Dynamic> element_node_index;
  const Isize node_number = mesh.physical_name_to_node_number_.at(physical_name);
  const auto element_number = static_cast<Isize>(mesh.physical_name_to_gmsh_type_and_tag_.at(physical_name).size());
  const Isize line_all_node_number = AdjacencyLineTrait<SimulationControl::kPolynomialOrder>::kAllNodeNumber;
  const Isize element_sub_number = getElementSubNumber<Element::Line, SimulationControl::kPolynomialOrder>();
  node_coordinate.resize(Eigen::NoChange, node_number);
  node_primitive_variable.resize(Eigen::NoChange, node_number);
  node_all_variable.resize(Eigen::NoChange, node_number);
  element_node_index.resize(Eigen::NoChange, element_number * element_sub_number);
  Isize parent_index_each_type;
  Isize adjacency_sequence_in_parent;
  Isize adjacency_parent_element_view_basis_function_sequence_in_parent;
  Isize parent_gmsh_type_number;
  for (Isize i = 0, column = 0; i < element_number; i++) {
    Variable<SimulationControl, SimulationControl::kDimension> node_variable;
    const auto [gmsh_type, gmsh_tag] =
        mesh.physical_name_to_gmsh_type_and_tag_.at(physical_name)[static_cast<Usize>(i)];
    const Isize element_index = mesh.gmsh_tag_to_index_.at(gmsh_tag);
    parent_index_each_type = mesh.line_.element_(element_index).parent_index_each_type_(0);
    adjacency_sequence_in_parent = mesh.line_.element_(element_index).adjacency_sequence_in_parent_(0);
    parent_gmsh_type_number = mesh.line_.element_(element_index).parent_gmsh_type_number_(0);
    node_coordinate(Eigen::all, Eigen::seqN(column, line_all_node_number)) =
        mesh.line_.element_(element_index).node_coordinate_;
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      for (Isize j = 0; j < line_all_node_number; j++) {
        adjacency_parent_element_view_basis_function_sequence_in_parent =
            view_data.line_.getAdjacencyParentElementViewBasisFunctionSequenceInParent(parent_gmsh_type_number,
                                                                                       adjacency_sequence_in_parent, j);
        node_variable.conserved_.noalias() =
            view_data.triangle_.conserved_variable_basis_function_coefficient_(parent_index_each_type) *
            view_data.triangle_.basis_function_value_.col(
                adjacency_parent_element_view_basis_function_sequence_in_parent);
        node_variable.calculateHumanReadablePrimitiveFromConserved(thermal_model);
        node_primitive_variable(Eigen::all, column + j) = node_variable.human_readable_primitive_;
      }
      element_node_index(Eigen::all, Eigen::seqN(i * element_sub_number, element_sub_number)) =
          mesh.line_.sub_element_connectivity_.array() + column + 1;
      column += line_all_node_number;
    } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      for (Isize j = 0; j < line_all_node_number; j++) {
        adjacency_parent_element_view_basis_function_sequence_in_parent =
            view_data.line_.getAdjacencyParentElementViewBasisFunctionSequenceInParent(parent_gmsh_type_number,
                                                                                       adjacency_sequence_in_parent, j);
        node_variable.conserved_.noalias() =
            view_data.quadrangle_.conserved_variable_basis_function_coefficient_(parent_index_each_type) *
            view_data.quadrangle_.basis_function_value_.col(
                adjacency_parent_element_view_basis_function_sequence_in_parent);
        node_variable.calculateHumanReadablePrimitiveFromConserved(thermal_model);
        node_primitive_variable(Eigen::all, column + j) = node_variable.human_readable_primitive_;
      }
      element_node_index(Eigen::all, Eigen::seqN(i * element_sub_number, element_sub_number)) =
          mesh.line_.sub_element_connectivity_.array() + column + 1;
      column += line_all_node_number;
    }
  }
  node_all_variable << node_coordinate, node_primitive_variable;
  writeTecplotAsciiHeader<1>(physical_name, node_number, element_number * element_sub_number, view_fout);
  view_fout << node_all_variable.transpose() << '\n';
  view_fout << element_node_index.transpose() << '\n';
}

template <typename SimulationControl>
inline void writeTecplotFEQuadrilateralAdjacencyElement();

template <typename SimulationControl>
inline void writeTecplotFEQuadrilateralElement(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    const ViewData<SimulationControl, SimulationControl::kDimension>& view_data, std::ofstream& view_fout) {
  Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic> node_coordinate;
  Eigen::Matrix<Real, SimulationControl::kPrimitiveVariableNumber, Eigen::Dynamic> node_primitive_variable;
  Eigen::Matrix<Real, SimulationControl::kDimension + SimulationControl::kPrimitiveVariableNumber, Eigen::Dynamic>
      node_all_variable;
  Eigen::Matrix<int, 4, Eigen::Dynamic> element_node_index;
  const Isize node_number = mesh.physical_name_to_node_number_.at(physical_name);
  const auto element_number = static_cast<Isize>(mesh.physical_name_to_gmsh_type_and_tag_.at(physical_name).size());
  const Isize triangle_all_node_number = TriangleTrait<SimulationControl::kPolynomialOrder>::kAllNodeNumber;
  const Isize quadrangle_all_node_number = QuadrangleTrait<SimulationControl::kPolynomialOrder>::kAllNodeNumber;
  const Isize element_sub_number = getElementSubNumber<Element::Quadrangle, SimulationControl::kPolynomialOrder>();
  node_coordinate.resize(Eigen::NoChange, node_number);
  node_primitive_variable.resize(Eigen::NoChange, node_number);
  node_all_variable.resize(Eigen::NoChange, node_number);
  element_node_index.resize(Eigen::NoChange, element_number * element_sub_number);
  for (Isize i = 0, column = 0; i < element_number; i++) {
    Variable<SimulationControl, SimulationControl::kDimension> node_variable;
    const auto [gmsh_type, gmsh_tag] =
        mesh.physical_name_to_gmsh_type_and_tag_.at(physical_name)[static_cast<Usize>(i)];
    const Isize element_index = mesh.gmsh_tag_to_index_.at(gmsh_tag);
    if (gmsh_type == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      node_coordinate(Eigen::all, Eigen::seqN(column, triangle_all_node_number)) =
          mesh.triangle_.element_(element_index).node_coordinate_;
      for (Isize j = 0; j < triangle_all_node_number; j++) {
        node_variable.conserved_.noalias() =
            view_data.triangle_.conserved_variable_basis_function_coefficient_(element_index) *
            view_data.triangle_.basis_function_value_.col(j);
        node_variable.calculateHumanReadablePrimitiveFromConserved(thermal_model);
        node_primitive_variable(Eigen::all, column + j) = node_variable.human_readable_primitive_;
      }
      element_node_index(Eigen::seqN(Eigen::fix<0>, Eigen::fix<3>),
                         Eigen::seqN(i * element_sub_number, element_sub_number)) =
          mesh.triangle_.sub_element_connectivity_.array() + column + 1;
      element_node_index(3, Eigen::seqN(i * element_sub_number, element_sub_number)) =
          element_node_index(2, Eigen::seqN(i * element_sub_number, element_sub_number));
      column += triangle_all_node_number;
    } else if (gmsh_type == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      node_coordinate(Eigen::all, Eigen::seqN(column, quadrangle_all_node_number)) =
          mesh.quadrangle_.element_(element_index).node_coordinate_;
      for (Isize j = 0; j < quadrangle_all_node_number; j++) {
        node_variable.conserved_.noalias() =
            view_data.quadrangle_.conserved_variable_basis_function_coefficient_(element_index) *
            view_data.quadrangle_.basis_function_value_.col(j);
        node_variable.calculateHumanReadablePrimitiveFromConserved(thermal_model);
        node_primitive_variable(Eigen::all, column + j) = node_variable.human_readable_primitive_;
      }
      element_node_index(Eigen::all, Eigen::seqN(i * element_sub_number, element_sub_number)) =
          mesh.quadrangle_.sub_element_connectivity_.array() + column + 1;
      column += quadrangle_all_node_number;
    }
  }
  node_all_variable << node_coordinate, node_primitive_variable;
  writeTecplotAsciiHeader<2>(physical_name, node_number, element_number * element_sub_number, view_fout);
  view_fout << node_all_variable.transpose() << '\n';
  view_fout << element_node_index.transpose() << '\n';
}

template <typename SimulationControl>
inline void View<SimulationControl, ViewModel::Dat>::writeStep(
    const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
  writeTecplotAsciiVariableList<SimulationControl>(this->view_fout_);
  for (const auto& [dim, physical_name] : mesh.physical_dimension_and_name_) {
    if constexpr (SimulationControl::kDimension == 2) {
      if (dim == 1) {
        // writeTecplotFELinesegAdjacencyElement(physical_name, mesh, thermal_model, this->data_, this->view_fout_);
        // NOTE: The ParaView can not read the Tecplot ASCII file with FELINESG.
      } else if (dim == 2) {
        writeTecplotFEQuadrilateralElement(physical_name, mesh, thermal_model, this->data_, this->view_fout_);
      }
    }
  }
}

template <typename SimulationControl>
inline void View<SimulationControl, ViewModel::Dat>::write(
    const int iteration_number, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
  this->initializeView();
  for (int i = 1; i <= iteration_number; i++) {
    if (i % this->io_interval_ == 0) {
      this->data_.readRawBinary(mesh, this->raw_binary_finout_);
      this->updateViewFout(i);
      this->writeStep(mesh, thermal_model);
      this->view_fout_.close();
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_TECPLOT_ASCII_HPP_
