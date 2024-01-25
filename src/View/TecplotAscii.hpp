/**
 * @file TecplotAscii.hpp
 * @brief The head file of SubrosaDG Tecplot ASCII file output.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-18
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_TECPLOT_ASCII_HPP_
#define SUBROSA_DG_TECPLOT_ASCII_HPP_

#include <Eigen/Core>
#include <format>
#include <fstream>
#include <magic_enum.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"
#include "View/IOControl.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
inline void ViewBase<SimulationControl, ViewModelEnum::Dat>::setViewFout(const int step, std::ofstream& fout) {
  std::string view_file;
  view_file = std::format("dat/{}_{:0{}d}.dat", this->output_file_name_prefix_, step, this->iteration_order_);
  fout.open((this->output_directory_ / view_file).string(), std::ios::out | std::ios::trunc);
  fout.setf(std::ios::left, std::ios::adjustfield);
  fout.setf(std::ios::scientific, std::ios::floatfield);
}

template <typename SimulationControl>
inline void ViewBase<SimulationControl, ViewModelEnum::Dat>::writeAsciiVariableList(std::ofstream& fout) {
  std::string variable_list;
  if constexpr (SimulationControl::kDimension == 1) {
    variable_list = R"(VARIABLES = "X")";
  } else if constexpr (SimulationControl::kDimension == 2) {
    variable_list = R"(VARIABLES = "X", "Y")";
  } else if constexpr (SimulationControl::kDimension == 3) {
    variable_list = R"(VARIABLES = "X", "Y", "Z")";
  }
  for (const auto variable : this->variable_vector_) {
    variable_list += std::format(R"(, "{}")", magic_enum::enum_name(variable));
  }
  fout << variable_list << '\n';
}

template <typename SimulationControl>
template <int Dimension>
inline void ViewBase<SimulationControl, ViewModelEnum::Dat>::writeAsciiHeader(const Real time_value,
                                                                              const std::string_view physical_index,
                                                                              const Isize node_number,
                                                                              const Isize element_number,
                                                                              std::ofstream& fout) {
  std::string header;
  if constexpr (Dimension == 1) {
    header =
        std::format(R"(Zone T="{}", ZONETYPE=FELINESEG, NODES={}, ELEMENTS={}, DATAPACKING=POINT, SOLUTIONTIME={})",
                    physical_index, node_number, element_number, time_value);
  } else if constexpr (Dimension == 2) {
    header = std::format(
        R"(Zone T="{}", ZONETYPE=FEQUADRILATERAL, NODES={}, ELEMENTS={}, DATAPACKING=POINT, SOLUTIONTIME={})",
        physical_index, node_number, element_number, time_value);
  } else if constexpr (Dimension == 3) {
    header = std::format(R"(Zone T="{}", ZONETYPE=FEBRICK, NODES={}, ELEMENTS={}, DATAPACKING=POINT, SOLUTIONTIME={})",
                         physical_index, node_number, element_number, time_value);
  }
  fout << header << '\n';
}

template <typename SimulationControl>
template <typename AdjacencyElementTrait>
inline void ViewBase<SimulationControl, ViewModelEnum::Dat>::writeDiscontinuousAdjacencyElement(
    const Isize physical_index, const MeshInformation& mesh_information,
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const ThermalModel<SimulationControl>& thermal_model,
    Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
    Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
    Eigen::Matrix<Isize, AdjacencyElementTrait::kTecplotBasicNodeNumber, Eigen::Dynamic>& element_connectivity,
    const Isize element_index, Isize& column) {
  Variable<SimulationControl> variable;
  const Isize element_gmsh_tag =
      mesh_information.physical_information_.at(physical_index).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type =
      mesh_information.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  const Eigen::Matrix<int, AdjacencyElementTrait::kTecplotBasicNodeNumber, AdjacencyElementTrait::kSubNumber>
      sub_connectivity{
          getSubElementConnectivity<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>()
              .data()};
  const Isize parent_index_each_type = adjacency_element_mesh.element_(element_index).parent_index_each_type_(0);
  const Isize adjacency_sequence_in_parent =
      adjacency_element_mesh.element_(element_index).adjacency_sequence_in_parent_(0);
  const Isize parent_gmsh_type_number = adjacency_element_mesh.element_(element_index).parent_gmsh_type_number_(0);
  node_coordinate(Eigen::all, Eigen::seqN(column, AdjacencyElementTrait::kAllNodeNumber)) =
      adjacency_element_mesh.element_(element_index_per_type)
          .node_coordinate_(Eigen::all, Eigen::seqN(Eigen::fix<0>, AdjacencyElementTrait::kAllNodeNumber));
  for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      const Isize adjacency_parent_element_view_basis_function_sequence_in_parent =
          this->variable_.line_.getAdjacencyParentElementViewBasisFunctionSequenceInParent(
              parent_gmsh_type_number, adjacency_sequence_in_parent, j);
      if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        variable.conserved_ = this->variable_.triangle_.conserved_variable_(parent_index_each_type)
                                  .col(adjacency_parent_element_view_basis_function_sequence_in_parent);
      } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        variable.conserved_ = this->variable_.quadrangle_.conserved_variable_(parent_index_each_type)
                                  .col(adjacency_parent_element_view_basis_function_sequence_in_parent);
      }
    }
    variable.calculateComputationalFromConserved(thermal_model);
    for (Isize k = 0; k < static_cast<Isize>(this->variable_vector_.size()); k++) {
      node_variable(k, column + j) = variable.get(thermal_model, this->variable_vector_[static_cast<Usize>(k)]);
    }
  }
  element_connectivity(
      Eigen::all, Eigen::seqN(element_index * AdjacencyElementTrait::kSubNumber, AdjacencyElementTrait::kSubNumber)) =
      sub_connectivity.template cast<Isize>().array() + column + 1;
  column += AdjacencyElementTrait::kAllNodeNumber;
}

template <typename SimulationControl>
template <typename ElementTrait>
inline void ViewBase<SimulationControl, ViewModelEnum::Dat>::writeDiscontinuousElement(
    const Isize physical_index, const MeshInformation& mesh_information, const ElementMesh<ElementTrait>& element_mesh,
    const ThermalModel<SimulationControl>& thermal_model,
    Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
    Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
    Eigen::Matrix<Isize, ElementTrait::kTecplotBasicNodeNumber, Eigen::Dynamic>& element_connectivity,
    const Isize element_index, Isize& column) {
  Variable<SimulationControl> variable;
  const ElementViewVariable<ElementTrait, SimulationControl>& element_view_variable =
      this->variable_.*(decltype(this->variable_)::template getElement<ElementTrait>());
  const Isize element_gmsh_tag =
      mesh_information.physical_information_.at(physical_index).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type =
      mesh_information.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  const Eigen::Matrix<int, ElementTrait::kTecplotBasicNodeNumber, ElementTrait::kSubNumber> sub_connectivity{
      getSubElementConnectivity<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>().data()};
  node_coordinate(Eigen::all, Eigen::seqN(column, ElementTrait::kAllNodeNumber)) =
      element_mesh.element_(element_index_per_type)
          .node_coordinate_(Eigen::all, Eigen::seqN(Eigen::fix<0>, ElementTrait::kAllNodeNumber));
  for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
    variable.conserved_ = element_view_variable.conserved_variable_(element_index_per_type).col(i);
    variable.calculateComputationalFromConserved(thermal_model);
    for (Isize j = 0; j < static_cast<Isize>(this->variable_vector_.size()); j++) {
      node_variable(j, column + i) = variable.get(thermal_model, this->variable_vector_[static_cast<Usize>(j)]);
    }
  }
  element_connectivity(Eigen::all, Eigen::seqN(element_index * ElementTrait::kSubNumber, ElementTrait::kSubNumber)) =
      sub_connectivity.template cast<Isize>().array() + column + 1;
  column += ElementTrait::kAllNodeNumber;
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void ViewBase<SimulationControl, ViewModelEnum::Dat>::writeDiscontinuousField(
    const Isize physical_index, const Mesh<SimulationControl>& mesh,
    const ThermalModel<SimulationControl>& thermal_model,
    Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
    Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
    Eigen::Matrix<Isize, getElementTecplotBasicNodeNumber<Dimension>(), Eigen::Dynamic>& element_connectivity) {
  const Isize element_number = mesh.information_.physical_information_.at(physical_index).element_number_;
  for (Isize i = 0, column = 0; i < element_number; i++) {
    const Isize element_gmsh_type =
        mesh.information_.physical_information_.at(physical_index).element_gmsh_type_[static_cast<Usize>(i)];
    if constexpr (Dimension == 1) {
      if constexpr (IsAdjacency) {
        this->writeDiscontinuousAdjacencyElement<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.line_, thermal_model, node_coordinate, node_variable,
            element_connectivity, i, column);
      } else {
        this->writeDiscontinuousElement<LineTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.line_, thermal_model, node_coordinate, node_variable,
            element_connectivity, i, column);
      }
    } else if constexpr (Dimension == 2) {
      if constexpr (IsAdjacency) {
      } else {
        if (element_gmsh_type == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeDiscontinuousElement<TriangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_index, mesh.information_, mesh.triangle_, thermal_model, node_coordinate, node_variable,
              element_connectivity, i, column);
        } else if (element_gmsh_type == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeDiscontinuousElement<QuadrangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_index, mesh.information_, mesh.quadrangle_, thermal_model, node_coordinate, node_variable,
              element_connectivity, i, column);
        }
      }
    }
  }
}

template <typename SimulationControl>
template <typename AdjacencyElementTrait>
inline void ViewBase<SimulationControl, ViewModelEnum::Dat>::writeContinuousAdjacencyElementConnectivity(
    const Isize physical_index, const MeshInformation& mesh_information,
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    Eigen::Matrix<Isize, AdjacencyElementTrait::kTecplotBasicNodeNumber, Eigen::Dynamic>& element_connectivity,
    const Isize element_index, Isize& column) {
  const ordered_set<Isize> node_gmsh_tag = mesh_information.physical_information_.at(physical_index).node_gmsh_tag_;
  const Isize element_gmsh_tag =
      mesh_information.physical_information_.at(physical_index).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type =
      mesh_information.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  const Eigen::Matrix<int, AdjacencyElementTrait::kTecplotBasicNodeNumber, AdjacencyElementTrait::kSubNumber>
      sub_connectivity{
          getSubElementConnectivity<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>()
              .data()};
  for (Isize i = 0; i < AdjacencyElementTrait::kSubNumber; i++) {
    for (Isize j = 0; j < AdjacencyElementTrait::kBasicNodeNumber; j++) {
      element_connectivity(j, column + i) = static_cast<Isize>(
          node_gmsh_tag.find_index(
              adjacency_element_mesh.element_(element_index_per_type).node_tag_(sub_connectivity(j, i))) +
          1);
    }
  }
  column += AdjacencyElementTrait::kSubNumber;
}

template <typename SimulationControl>
template <typename ElementTrait>
inline void ViewBase<SimulationControl, ViewModelEnum::Dat>::writeContinuousElementConnectivity(
    const Isize physical_index, const MeshInformation& mesh_information, const ElementMesh<ElementTrait>& element_mesh,
    Eigen::Matrix<Isize, ElementTrait::kTecplotBasicNodeNumber, Eigen::Dynamic>& element_connectivity,
    const Isize element_index, Isize& column) {
  const ordered_set<Isize> node_gmsh_tag = mesh_information.physical_information_.at(physical_index).node_gmsh_tag_;
  const Isize element_gmsh_tag =
      mesh_information.physical_information_.at(physical_index).element_gmsh_tag_[static_cast<Usize>(element_index)];
  const Isize element_index_per_type =
      mesh_information.gmsh_tag_to_element_information_.at(element_gmsh_tag).element_index_;
  const Eigen::Matrix<int, ElementTrait::kTecplotBasicNodeNumber, ElementTrait::kSubNumber> sub_connectivity{
      getSubElementConnectivity<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>().data()};
  for (Isize i = 0; i < ElementTrait::kSubNumber; i++) {
    for (Isize j = 0; j < ElementTrait::kBasicNodeNumber; j++) {
      element_connectivity(j, column + i) = static_cast<Isize>(
          node_gmsh_tag.find_index(element_mesh.element_(element_index_per_type).node_tag_(sub_connectivity(j, i))) +
          1);
    }
  }
  column += ElementTrait::kSubNumber;
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void ViewBase<SimulationControl, ViewModelEnum::Dat>::writeContinuousField(
    const Isize physical_index, const Mesh<SimulationControl>& mesh,
    const ThermalModel<SimulationControl>& thermal_model,
    Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
    Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
    Eigen::Matrix<Isize, getElementTecplotBasicNodeNumber<Dimension>(), Eigen::Dynamic>& element_connectivity) {
  Variable<SimulationControl> variable;
  const Isize element_number = mesh.information_.physical_information_.at(physical_index).element_number_;
  const ordered_set<Isize> node_gmsh_tag = mesh.information_.physical_information_.at(physical_index).node_gmsh_tag_;
  for (Isize i = 0; const auto gmsh_tag : node_gmsh_tag) {
    variable.conserved_ = this->variable_.node_conserved_variable_.col(gmsh_tag - 1);
    variable.calculateComputationalFromConserved(thermal_model);
    for (Isize j = 0; j < static_cast<Isize>(this->variable_vector_.size()); j++) {
      node_variable(j, i) = variable.get(thermal_model, this->variable_vector_[static_cast<Usize>(j)]);
    }
    node_coordinate.col(i++) = mesh.node_coordinate_.col(gmsh_tag - 1);
  }
  for (Isize i = 0, column = 0; i < element_number; i++) {
    const Isize element_gmsh_type =
        mesh.information_.physical_information_.at(physical_index).element_gmsh_type_[static_cast<Usize>(i)];
    if constexpr (Dimension == 1) {
      if constexpr (IsAdjacency) {
        this->writeContinuousAdjacencyElementConnectivity<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.line_, element_connectivity, i, column);
      } else {
        this->writeContinuousElementConnectivity<LineTrait<SimulationControl::kPolynomialOrder>>(
            physical_index, mesh.information_, mesh.line_, element_connectivity, i, column);
      }
    }
    if constexpr (Dimension == 2) {
      if constexpr (IsAdjacency) {
      } else {
        if (element_gmsh_type == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeContinuousElementConnectivity<TriangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_index, mesh.information_, mesh.triangle_, element_connectivity, i, column);
        } else if (element_gmsh_type == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
          this->writeContinuousElementConnectivity<QuadrangleTrait<SimulationControl::kPolynomialOrder>>(
              physical_index, mesh.information_, mesh.quadrangle_, element_connectivity, i, column);
        }
      }
    }
  }
}

template <typename SimulationControl>
template <int Dimension, bool IsAdjacency>
inline void ViewBase<SimulationControl, ViewModelEnum::Dat>::writeView(
    const int step, const Isize physical_index, const Mesh<SimulationControl>& mesh,
    const ThermalModel<SimulationControl>& thermal_model, std::ofstream& fout) {
  Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic> node_coordinate;
  Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic> node_variable;
  Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic> node_all_variable;
  Eigen::Matrix<Isize, getElementTecplotBasicNodeNumber<Dimension>(), Eigen::Dynamic> element_connectivity;
  const Isize node_number =
      ((this->config_enum_ & ViewConfigEnum::SolverSmoothness) == ViewConfigEnum::SolverSmoothness)
          ? static_cast<Isize>(mesh.information_.physical_information_.at(physical_index).node_gmsh_tag_.size())
          : mesh.information_.physical_information_.at(physical_index).node_number_;
  const Isize element_number = mesh.information_.physical_information_.at(physical_index).element_number_;
  const Isize element_sub_number = getElementSubNumber<Dimension, SimulationControl::kPolynomialOrder>();
  node_coordinate.resize(Eigen::NoChange, node_number);
  node_variable.resize(static_cast<Isize>(this->variable_vector_.size()), node_number);
  node_all_variable.resize(SimulationControl::kDimension + static_cast<Isize>(this->variable_vector_.size()),
                           node_number);
  element_connectivity.resize(Eigen::NoChange, element_number * element_sub_number);
  if ((this->config_enum_ & ViewConfigEnum::SolverSmoothness) == ViewConfigEnum::SolverSmoothness) {
    this->writeContinuousField<Dimension, IsAdjacency>(physical_index, mesh, thermal_model, node_coordinate,
                                                       node_variable, element_connectivity);
  } else {
    this->writeDiscontinuousField<Dimension, IsAdjacency>(physical_index, mesh, thermal_model, node_coordinate,
                                                          node_variable, element_connectivity);
  }
  node_all_variable << node_coordinate, node_variable;
  this->writeAsciiHeader<Dimension>(this->variable_.time_value_(step - 1),
                                    mesh.information_.physical_[static_cast<Usize>(physical_index)], node_number,
                                    element_number * element_sub_number, fout);
  fout << node_all_variable.transpose() << '\n';
  fout << element_connectivity.transpose() << '\n';
}

template <typename SimulationControl>
inline void ViewBase<SimulationControl, ViewModelEnum::Dat>::stepView(
    const int step, const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model) {
  std::ofstream fout;
  this->variable_.readRawBinary(mesh, this->config_enum_, this->raw_binary_finout_);
  this->setViewFout(step, fout);
  this->writeAsciiVariableList(fout);
  for (Isize i = 0; i < static_cast<Isize>(mesh.information_.physical_.size()); i++) {
    if (mesh.information_.periodic_physical_.contains(i)) {
      continue;
    }
    if constexpr (SimulationControl::kDimension == 1) {
      if (mesh.information_.physical_dimension_[static_cast<Usize>(i)] == 1) {
        this->writeView<1, false>(step, i, mesh, thermal_model, fout);
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if (mesh.information_.physical_dimension_[static_cast<Usize>(i)] == 1) {
        // NOTE: The ParaView can not read the Tecplot ASCII file with FELINESG.
        this->writeView<1, true>(step, i, mesh, thermal_model, fout);
      } else if (mesh.information_.physical_dimension_[static_cast<Usize>(i)] == 2) {
        this->writeView<2, false>(step, i, mesh, thermal_model, fout);
      }
    }
  }
  fout.close();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_TECPLOT_ASCII_HPP_
