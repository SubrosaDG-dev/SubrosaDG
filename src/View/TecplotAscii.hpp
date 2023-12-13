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
#include <magic_enum.hpp>
#include <string>
#include <string_view>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"
#include "View/IOControl.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
inline void View<SimulationControl, ViewModel::Dat>::setViewFout(const int step, std::ofstream& fout) {
  std::string view_file;
  view_file = fmt::format("dat/{}_{}.dat", this->output_file_name_prefix_, step);
  fout.open((this->output_directory_ / view_file).string(), std::ios::out | std::ios::trunc);
  fout.setf(std::ios::left, std::ios::adjustfield);
  fout.setf(std::ios::scientific, std::ios::floatfield);
}

template <typename SimulationControl>
inline void View<SimulationControl, ViewModel::Dat>::writeAsciiVariableList(std::ofstream& fout) {
  std::string variable_list;
  if constexpr (SimulationControl::kDimension == 2) {
    variable_list = R"(VARIABLES = "X", "Y")";
  } else if constexpr (SimulationControl::kDimension == 3) {
    variable_list = R"(VARIABLES = "X", "Y", "Z")";
  }
  for (const auto variable : this->all_view_variable_) {
    variable_list += fmt::format(R"(, "{}")", magic_enum::enum_name(variable));
  }
  fout << variable_list << '\n';
}

template <typename SimulationControl>
template <int Dimension>
inline void View<SimulationControl, ViewModel::Dat>::writeAsciiHeader(const std::string_view physical_name,
                                                                      const Isize node_number,
                                                                      const Isize element_number, std::ofstream& fout) {
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
  fout << header << '\n';
}

template <typename SimulationControl>
template <typename AdjacencyElementTrait>
inline void View<SimulationControl, ViewModel::Dat>::writeDiscontinuousAdjacencyElement(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
    Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
    Eigen::Matrix<int, AdjacencyElementTrait::kTecplotBasicNodeNumber, Eigen::Dynamic>& element_connectivity,
    const Isize element_index, Isize& column) {
  Variable<SimulationControl> variable;
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
    node_coordinate(Eigen::all, Eigen::seqN(column, node_number)) =
        mesh.line_.element_(element_index_per_type)
            .node_coordinate_(Eigen::all, Eigen::seqN(Eigen::fix<0>, node_number));
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      for (Isize j = 0; j < node_number; j++) {
        const Isize adjacency_parent_element_view_basis_function_sequence_in_parent =
            this->data_.line_.getAdjacencyParentElementViewBasisFunctionSequenceInParent(
                parent_gmsh_type_number, adjacency_sequence_in_parent, j);
        variable.conserved_ = this->data_.triangle_.conserved_variable_(parent_index_each_type)
                                  .col(adjacency_parent_element_view_basis_function_sequence_in_parent);
        variable.calculateComputationalFromConserved(thermal_model);
        for (Isize k = 0; k < static_cast<Isize>(this->all_view_variable_.size()); k++) {
          node_variable(k, column + j) = variable.get(thermal_model, this->all_view_variable_[static_cast<Usize>(k)]);
        }
      }
    } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      for (Isize j = 0; j < node_number; j++) {
        const Isize adjacency_parent_element_view_basis_function_sequence_in_parent =
            this->data_.line_.getAdjacencyParentElementViewBasisFunctionSequenceInParent(
                parent_gmsh_type_number, adjacency_sequence_in_parent, j);
        variable.conserved_ = this->data_.quadrangle_.conserved_variable_(parent_index_each_type)
                                  .col(adjacency_parent_element_view_basis_function_sequence_in_parent);
        variable.calculateComputationalFromConserved(thermal_model);
        for (Isize k = 0; k < static_cast<Isize>(this->all_view_variable_.size()); k++) {
          node_variable(k, column + j) = variable.get(thermal_model, this->all_view_variable_[static_cast<Usize>(k)]);
        }
      }
    }
    if (this->view_config_.at(ViewConfig::HighOrderReconstruction)) {
      element_connectivity(Eigen::all, Eigen::seqN(element_index * AdjacencyElementTrait::kSubNumber,
                                                   AdjacencyElementTrait::kSubNumber)) =
          mesh.line_.sub_element_connectivity_.array() + column + 1;
    } else {
      element_connectivity(Eigen::all, element_index) = mesh.line_.element_connectivity_.array() + column + 1;
    }
  }
  column += node_number;
}

template <typename SimulationControl>
template <typename ElementTrait>
inline void View<SimulationControl, ViewModel::Dat>::writeDiscontinuousElement(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
    Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
    Eigen::Matrix<int, ElementTrait::kTecplotBasicNodeNumber, Eigen::Dynamic>& element_connectivity,
    const Isize element_index, Isize& column) {
  Variable<SimulationControl> variable;
  const Isize element_gmsh_tag =
      mesh.physical_group_information_.at(physical_name).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type = mesh.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  const Isize node_number = this->view_config_.at(ViewConfig::HighOrderReconstruction) ? ElementTrait::kAllNodeNumber
                                                                                       : ElementTrait::kBasicNodeNumber;
  if constexpr (ElementTrait::kElementType == Element::Triangle) {
    node_coordinate(Eigen::all, Eigen::seqN(column, node_number)) =
        mesh.triangle_.element_(element_index_per_type)
            .node_coordinate_(Eigen::all, Eigen::seqN(Eigen::fix<0>, node_number));
  } else if constexpr (ElementTrait::kElementType == Element::Quadrangle) {
    node_coordinate(Eigen::all, Eigen::seqN(column, node_number)) =
        mesh.quadrangle_.element_(element_index_per_type)
            .node_coordinate_(Eigen::all, Eigen::seqN(Eigen::fix<0>, node_number));
  }
  for (Isize i = 0; i < node_number; i++) {
    if constexpr (ElementTrait::kElementType == Element::Triangle) {
      variable.conserved_ = this->data_.triangle_.conserved_variable_(element_index_per_type).col(i);
    } else if constexpr (ElementTrait::kElementType == Element::Quadrangle) {
      variable.conserved_ = this->data_.quadrangle_.conserved_variable_(element_index_per_type).col(i);
    }
    variable.calculateComputationalFromConserved(thermal_model);
    for (Isize j = 0; j < static_cast<Isize>(this->all_view_variable_.size()); j++) {
      node_variable(j, column + i) = variable.get(thermal_model, this->all_view_variable_[static_cast<Usize>(j)]);
    }
  }
  if constexpr (ElementTrait::kElementType == Element::Triangle) {
    if (this->view_config_.at(ViewConfig::HighOrderReconstruction)) {
      element_connectivity(Eigen::seqN(Eigen::fix<0>, Eigen::fix<3>),
                           Eigen::seqN(element_index * ElementTrait::kSubNumber, ElementTrait::kSubNumber)) =
          mesh.triangle_.sub_element_connectivity_.array() + column + 1;
      element_connectivity(3, Eigen::seqN(element_index * ElementTrait::kSubNumber, ElementTrait::kSubNumber)) =
          element_connectivity(2, Eigen::seqN(element_index * ElementTrait::kSubNumber, ElementTrait::kSubNumber));
    } else {
      element_connectivity(Eigen::seqN(Eigen::fix<0>, Eigen::fix<3>), element_index) =
          mesh.triangle_.element_connectivity_.array() + column + 1;
      element_connectivity(3, element_index) = element_connectivity(2, element_index);
    }
  } else if constexpr (ElementTrait::kElementType == Element::Quadrangle) {
    if (this->view_config_.at(ViewConfig::HighOrderReconstruction)) {
      element_connectivity(Eigen::all,
                           Eigen::seqN(element_index * ElementTrait::kSubNumber, ElementTrait::kSubNumber)) =
          mesh.quadrangle_.sub_element_connectivity_.array() + column + 1;
    } else {
      element_connectivity(Eigen::all, element_index) = mesh.quadrangle_.element_connectivity_.array() + column + 1;
    }
  }
  column += node_number;
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void View<SimulationControl, ViewModel::Dat>::writeDiscontinuousField(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
    Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
    Eigen::Matrix<int, getElementTecplotBasicNodeNumber<Dimension>(), Eigen::Dynamic>& element_connectivity) {
  const Isize element_number = mesh.physical_group_information_.at(physical_name).element_number_;
  for (Isize i = 0, column = 0; i < element_number; i++) {
    const Isize element_gmsh_type =
        mesh.physical_group_information_.at(physical_name).element_gmsh_type_[static_cast<Usize>(i)];
    if constexpr (Dimension == 1) {
      if (element_gmsh_type == AdjacencyLineTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeDiscontinuousAdjacencyElement<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>>(
            physical_name, mesh, thermal_model, node_coordinate, node_variable, element_connectivity, i, column);
      }
    } else if constexpr (Dimension == 2) {
      if constexpr (IsAdjacency) {
      } else {
        if (element_gmsh_type == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeDiscontinuousElement<TriangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_name, mesh, thermal_model, node_coordinate, node_variable, element_connectivity, i, column);
        } else if (element_gmsh_type == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeDiscontinuousElement<QuadrangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_name, mesh, thermal_model, node_coordinate, node_variable, element_connectivity, i, column);
        }
      }
    }
  }
}

template <typename SimulationControl>
template <typename T>
inline int View<SimulationControl, ViewModel::Dat>::getElementNodeIndex(T& elements,
                                                                        const ordered_set<Isize>& node_gmsh_tag,
                                                                        const Isize element_index_per_type,
                                                                        const Isize i, const Isize j) const {
  if (this->view_config_.at(ViewConfig::HighOrderReconstruction)) {
    return static_cast<int>(
        node_gmsh_tag.find_index(
            elements.element_(element_index_per_type).node_tag_(elements.sub_element_connectivity_(j, i))) +
        1);
  }
  return static_cast<int>(
      node_gmsh_tag.find_index(elements.element_(element_index_per_type).node_tag_(elements.element_connectivity_(j))) +
      1);
}

template <typename SimulationControl>
template <typename ElementTrait>
inline void View<SimulationControl, ViewModel::Dat>::writeContinuousElementConnectivity(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    Eigen::Matrix<int, ElementTrait::kTecplotBasicNodeNumber, Eigen::Dynamic>& element_connectivity,
    const Isize element_index, Isize& column) {
  const ordered_set<Isize> node_gmsh_tag =
      this->view_config_.at(ViewConfig::HighOrderReconstruction)
          ? mesh.physical_group_information_.at(physical_name).all_node_gmsh_tag_
          : mesh.physical_group_information_.at(physical_name).basic_node_gmsh_tag_;
  const Isize element_sub_number =
      this->view_config_.at(ViewConfig::HighOrderReconstruction) ? ElementTrait::kSubNumber : 1;
  const Isize element_gmsh_tag =
      mesh.physical_group_information_.at(physical_name).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type = mesh.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  for (Isize i = 0; i < element_sub_number; i++) {
    for (Isize j = 0; j < ElementTrait::kBasicNodeNumber; j++) {
      if constexpr (ElementTrait::kElementType == Element::Line) {
        element_connectivity(j, column + i) =
            this->getElementNodeIndex(mesh.line_, node_gmsh_tag, element_index_per_type, i, j);
      } else if constexpr (ElementTrait::kElementType == Element::Triangle) {
        element_connectivity(j, column + i) =
            this->getElementNodeIndex(mesh.triangle_, node_gmsh_tag, element_index_per_type, i, j);
      } else if constexpr (ElementTrait::kElementType == Element::Quadrangle) {
        element_connectivity(j, column + i) =
            this->getElementNodeIndex(mesh.quadrangle_, node_gmsh_tag, element_index_per_type, i, j);
      }
    }
  }
  if constexpr (ElementTrait::kElementType == Element::Triangle) {
    element_connectivity(3, Eigen::seqN(column, element_sub_number)) =
        element_connectivity(2, Eigen::seqN(column, element_sub_number));
  }
  column += element_sub_number;
}

template <typename SimulationControl>
template <int Dimension>
inline void View<SimulationControl, ViewModel::Dat>::writeContinuousField(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
    Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
    Eigen::Matrix<int, getElementTecplotBasicNodeNumber<Dimension>(), Eigen::Dynamic>& element_connectivity) {
  Variable<SimulationControl> variable;
  const Isize element_number = mesh.physical_group_information_.at(physical_name).element_number_;
  const ordered_set<Isize> node_gmsh_tag =
      this->view_config_.at(ViewConfig::HighOrderReconstruction)
          ? mesh.physical_group_information_.at(physical_name).all_node_gmsh_tag_
          : mesh.physical_group_information_.at(physical_name).basic_node_gmsh_tag_;
  for (Isize i = 0; const auto gmsh_tag : node_gmsh_tag) {
    variable.conserved_ = this->data_.node_conserved_variable_.col(gmsh_tag - 1);
    variable.calculateComputationalFromConserved(thermal_model);
    for (Isize j = 0; j < static_cast<Isize>(this->all_view_variable_.size()); j++) {
      node_variable(j, i) = variable.get(thermal_model, this->all_view_variable_[static_cast<Usize>(j)]);
    }
    node_coordinate.col(i++) = mesh.node_coordinate_.col(gmsh_tag - 1);
  }
  for (Isize i = 0, column = 0; i < element_number; i++) {
    const Isize element_gmsh_type =
        mesh.physical_group_information_.at(physical_name).element_gmsh_type_[static_cast<Usize>(i)];
    if constexpr (Dimension == 1) {
      if (element_gmsh_type == LineTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeContinuousElementConnectivity<LineTrait<SimulationControl::kPolynomialOrder>>(
            physical_name, mesh, element_connectivity, i, column);
      }
    }
    if constexpr (Dimension == 2) {
      if (element_gmsh_type == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeContinuousElementConnectivity<TriangleTrait<SimulationControl::kPolynomialOrder>>(
            physical_name, mesh, element_connectivity, i, column);
      } else if (element_gmsh_type == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeContinuousElementConnectivity<QuadrangleTrait<SimulationControl::kPolynomialOrder>>(
            physical_name, mesh, element_connectivity, i, column);
      }
    }
  }
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void View<SimulationControl, ViewModel::Dat>::writeView(
    const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model, std::ofstream& fout) {
  Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic> node_coordinate;
  Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic> node_variable;
  Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic> node_all_variable;
  Eigen::Matrix<int, getElementTecplotBasicNodeNumber<Dimension>(), Eigen::Dynamic> element_connectivity;
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
  const Isize element_number = mesh.physical_group_information_.at(physical_name).element_number_;
  const Isize element_sub_number = this->view_config_.at(ViewConfig::HighOrderReconstruction)
                                       ? getElementSubNumber<Dimension, SimulationControl::kPolynomialOrder>()
                                       : 1;
  node_coordinate.resize(Eigen::NoChange, node_number);
  node_variable.resize(static_cast<Isize>(this->all_view_variable_.size()), node_number);
  node_all_variable.resize(SimulationControl::kDimension + static_cast<Isize>(this->all_view_variable_.size()),
                           node_number);
  element_connectivity.resize(Eigen::NoChange, element_number * element_sub_number);
  if (this->view_config_.at(ViewConfig::SolverSmoothness)) {
    this->writeContinuousField<Dimension>(physical_name, mesh, thermal_model, node_coordinate, node_variable,
                                          element_connectivity);
  } else {
    this->writeDiscontinuousField<Dimension, IsAdjacency>(physical_name, mesh, thermal_model, node_coordinate,
                                                          node_variable, element_connectivity);
  }
  node_all_variable << node_coordinate, node_variable;
  this->writeAsciiHeader<Dimension>(physical_name, node_number, element_number * element_sub_number, fout);
  fout << node_all_variable.transpose() << '\n';
  fout << element_connectivity.transpose() << '\n';
}

template <typename SimulationControl>
inline void View<SimulationControl, ViewModel::Dat>::stepView(
    const int step, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
  std::ofstream fout;
  this->data_.readRawBinary(mesh, this->raw_binary_finout_);
  if (this->view_config_.at(ViewConfig::SolverSmoothness)) {
    this->data_.calculateNodeConservedVariable(mesh);
  }
  this->setViewFout(step, fout);
  this->writeAsciiVariableList(fout);
  for (const auto& [dim, physical_name] : mesh.physical_group_) {
    if constexpr (SimulationControl::kDimension == 2) {
      if (dim == 1) {
        // NOTE: The ParaView can not read the Tecplot ASCII file with FELINESG.
        this->writeView<1, true>(physical_name, mesh, thermal_model, fout);
      } else if (dim == 2) {
        this->writeView<2, false>(physical_name, mesh, thermal_model, fout);
      }
    }
  }
  fout.close();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_TECPLOT_ASCII_HPP_
