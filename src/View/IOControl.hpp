/**
 * @file IOControl.hpp
 * @brief The header file of IOControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_IO_CONTROL_HPP_
#define SUBROSA_DG_IO_CONTROL_HPP_

#include <Eigen/Core>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <vtu11-cpp17.hpp>

#include "Mesh/BasisFunction.hpp"
#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename AdjacencyElementTrait, PolynomialOrder P>
struct AdjacencyElementViewBasisFunction;

template <PolynomialOrder P>
struct AdjacencyElementViewBasisFunction<AdjacencyLineTrait<P>, P> {
  [[nodiscard]] inline Isize getAdjacencyParentElementViewBasisFunctionSequenceInParent(
      Isize parent_gmsh_type_number, Isize adjacency_sequence_in_parent, Isize node_sequence_in_adjacency) const;
};

template <typename ElementTrait>
struct ElementViewBasisFunction {
  Eigen::Matrix<Real, ElementTrait::kBasisFunctionNumber, ElementTrait::kAllNodeNumber> basis_function_value_;

  inline ElementViewBasisFunction();
};

template <typename ElementTrait, typename SimulationControl>
struct ElementViewData : ElementViewBasisFunction<ElementTrait> {
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>,
               Eigen::Dynamic, 1>
      conserved_variable_;

  inline void readElementRawBinary(const ElementMesh<ElementTrait>& element_mesh, std::fstream& raw_binary_finout);

  inline ElementViewData();
};

template <typename SimulationControl, int Dimension>
struct ViewData;

template <typename SimulationControl>
struct ViewData<SimulationControl, 2> {
  AdjacencyElementViewBasisFunction<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>,
                                    SimulationControl::kPolynomialOrder>
      line_;
  ElementViewData<TriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> triangle_;
  ElementViewData<QuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> quadrangle_;

  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, Eigen::Dynamic> node_conserved_variable_;

  inline void readRawBinary(const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
                            std::fstream& raw_binary_finout);

  inline void calculateNodeConservedVariable(const Mesh<SimulationControl, SimulationControl::kDimension>& mesh);

  inline void initializeViewData(const Mesh<SimulationControl, SimulationControl::kDimension>& mesh);
};

template <typename SimulationControl>
struct ViewBase {
  int io_interval_;
  std::filesystem::path output_directory_;
  std::string output_file_name_prefix_;
  std::fstream raw_binary_finout_;
  std::unordered_map<ViewConfig, bool> view_config_{{ViewConfig::HighOrderReconstruction, false},
                                                    {ViewConfig::SolverSmoothness, false}};
  std::vector<ViewVariable> primary_view_variable_;
  std::vector<ViewVariable> all_view_variable_;
  ViewData<SimulationControl, SimulationControl::kDimension> data_;

  inline void initializeViewRawBinary();

  inline void initializeView(const Mesh<SimulationControl, SimulationControl::kDimension>& mesh);
};

template <typename SimulationControl, ViewModel ViewModelType>
struct View;

template <typename SimulationControl>
struct View<SimulationControl, ViewModel::Dat> : ViewBase<SimulationControl> {
  inline void setViewFout(int step, std::ofstream& fout);

  inline void writeAsciiVariableList(std::ofstream& fout);

  template <int Dimension>
  inline void writeAsciiHeader(std::string_view physical_name, Isize node_number, Isize element_number,
                               std::ofstream& fout);

  template <typename AdjacencyElementTrait>
  inline void writeDiscontinuousAdjacencyElement(
      const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
      Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
      Eigen::Matrix<int, AdjacencyElementTrait::kTecplotBasicNodeNumber, Eigen::Dynamic>& element_connectivity,
      Isize element_index, Isize& column);

  template <typename ElementTrait>
  inline void writeDiscontinuousElement(
      const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
      Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
      Eigen::Matrix<int, ElementTrait::kTecplotBasicNodeNumber, Eigen::Dynamic>& element_connectivity,
      Isize element_index, Isize& column);

  template <int Dimension, bool IsAdjacency>
  inline void writeDiscontinuousField(
      const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
      Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
      Eigen::Matrix<int, getElementTecplotBasicNodeNumber<Dimension>(), Eigen::Dynamic>& element_connectivity);

  template <typename T>
  inline int getElementNodeIndex(T& elements, const ordered_set<Isize>& node_gmsh_tag, Isize element_index_per_type,
                                 Isize i, Isize j) const;

  template <typename ElementTrait>
  inline void writeContinuousElementConnectivity(
      const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      Eigen::Matrix<int, ElementTrait::kTecplotBasicNodeNumber, Eigen::Dynamic>& element_connectivity,
      Isize element_index, Isize& column);

  template <int Dimension>
  inline void writeContinuousField(
      const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
      Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
      Eigen::Matrix<int, getElementTecplotBasicNodeNumber<Dimension>(), Eigen::Dynamic>& element_connectivity);

  template <int Dimension, bool IsAdjacency>
  inline void writeView(const std::string& physical_name,
                        const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
                        const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
                        std::ofstream& fout);

  inline void stepView(int step, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
                       const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model);
};

template <typename SimulationControl>
struct View<SimulationControl, ViewModel::Vtu> : ViewBase<SimulationControl> {
  inline void getBaseName(int step, std::string& base_name);

  inline void getDataSetInfomatoin(std::vector<vtu11::DataSetInfo>& data_set_information);

  inline void calculateViewVariable(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      Variable<SimulationControl>& variable,
      Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable, Isize column);

  template <typename AdjacencyElementTrait>
  inline void writeDiscontinuousAdjacencyElement(
      const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
      Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
      Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
      Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
      Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type, Isize element_index, Isize& column);

  template <typename ElementTrait>
  inline void writeDiscontinuousElement(
      const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
      Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
      Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
      Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
      Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type, Isize element_index, Isize& column);

  template <int Dimension, bool IsAdjacency>
  inline void writeDiscontinuousField(
      const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
      Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
      Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
      Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
      Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type);

  template <typename T>
  inline vtu11::VtkIndexType getElementNodeIndex(T& elements, const ordered_set<Isize>& node_gmsh_tag,
                                                 Isize element_index_per_type, Isize i) const;

  template <typename ElementTrait>
  inline void writeContinuousElementConnectivity(
      const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
      Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
      Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type, Isize element_index, Isize& column);

  template <int Dimension>
  inline void writeContinuousField(
      const std::string& physical_name, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
      Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
      Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
      Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
      Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type);

  inline Isize getElementConnectivityNumber(const std::string& physical_name,
                                            const Mesh<SimulationControl, SimulationControl::kDimension>& mesh);

  template <int Dimension, bool IsAdjacency>
  inline void writeView(const std::string& physical_name,
                        const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
                        const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
                        const std::string& base_name, const std::vector<vtu11::DataSetInfo>& data_set_information);

  inline void stepView(int step, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
                       const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model);
};

template <PolynomialOrder P>
[[nodiscard]] inline Isize
AdjacencyElementViewBasisFunction<AdjacencyLineTrait<P>, P>::getAdjacencyParentElementViewBasisFunctionSequenceInParent(
    const Isize parent_gmsh_type_number, const Isize adjacency_sequence_in_parent,
    const Isize node_sequence_in_adjacency) const {
  if (parent_gmsh_type_number == TriangleTrait<P>::kGmshTypeNumber) {
    if (adjacency_sequence_in_parent == TriangleTrait<P>::kAdjacencyNumber - 1 && node_sequence_in_adjacency == 1) {
      return 0;
    }
    if (node_sequence_in_adjacency < AdjacencyLineTrait<P>::kBasicNodeNumber) {
      return adjacency_sequence_in_parent + node_sequence_in_adjacency;
    }
    return TriangleTrait<P>::kBasicNodeNumber +
           adjacency_sequence_in_parent *
               (AdjacencyLineTrait<P>::kAllNodeNumber - AdjacencyLineTrait<P>::kBasicNodeNumber) +
           node_sequence_in_adjacency - AdjacencyLineTrait<P>::kBasicNodeNumber;
  }
  if (parent_gmsh_type_number == QuadrangleTrait<P>::kGmshTypeNumber) {
    if (adjacency_sequence_in_parent == QuadrangleTrait<P>::kAdjacencyNumber - 1 && node_sequence_in_adjacency == 1) {
      return 0;
    }
    if (node_sequence_in_adjacency < AdjacencyLineTrait<P>::kBasicNodeNumber) {
      return adjacency_sequence_in_parent + node_sequence_in_adjacency;
    }
    return QuadrangleTrait<P>::kBasicNodeNumber +
           adjacency_sequence_in_parent *
               (AdjacencyLineTrait<P>::kAllNodeNumber - AdjacencyLineTrait<P>::kBasicNodeNumber) +
           node_sequence_in_adjacency - AdjacencyLineTrait<P>::kBasicNodeNumber;
  }
  return -1;
}

template <typename ElementTrait>
inline ElementViewBasisFunction<ElementTrait>::ElementViewBasisFunction() {
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kAllNodeNumber> all_node_coordinate{
      getElementNodeCoordinate<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>().data()};
  Eigen::Matrix<double, 3, ElementTrait::kAllNodeNumber> local_coord_gmsh_matrix;
  local_coord_gmsh_matrix.setZero();
  local_coord_gmsh_matrix(Eigen::seqN(Eigen::fix<0>, Eigen::fix<ElementTrait::kDimension>), Eigen::all) =
      all_node_coordinate.template cast<double>();
  std::vector<double> local_coord(local_coord_gmsh_matrix.data(),
                                  local_coord_gmsh_matrix.data() + local_coord_gmsh_matrix.size());
  std::vector<double> basis_functions = getElementBasisFunction<ElementTrait>(local_coord);
  for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
    for (Isize j = 0; j < ElementTrait::kBasisFunctionNumber; j++) {
      this->basis_function_value_(j, i) =
          static_cast<Real>(basis_functions[static_cast<Usize>(i * ElementTrait::kBasisFunctionNumber + j)]);
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline ElementViewData<ElementTrait, SimulationControl>::ElementViewData() : ElementViewBasisFunction<ElementTrait>(){};

template <typename SimulationControl>
inline void ViewBase<SimulationControl>::initializeViewRawBinary() {
#ifdef SUBROSA_DG_DEVELOP
  this->raw_binary_finout_.open((this->output_directory_ / "raw.binary").string(),
                                std::ios::in | std::ios::out | std::ios::trunc);
#else
  this->raw_binary_finout_.open((this->output_directory_ / "raw.binary").string(),
                                std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
#endif
  this->raw_binary_finout_.setf(std::ios::left, std::ios::adjustfield);
  this->raw_binary_finout_.setf(std::ios::scientific, std::ios::floatfield);
}

template <typename SimulationControl>
inline void ViewBase<SimulationControl>::initializeView(
    const Mesh<SimulationControl, SimulationControl::kDimension>& mesh) {
  std::filesystem::path view_output_directory;
  if constexpr (SimulationControl::kViewModel == ViewModel::Dat) {
    view_output_directory = this->output_directory_ / "dat";
  } else if constexpr (SimulationControl::kViewModel == ViewModel::Msh) {
    view_output_directory = this->output_directory_ / "msh";
  } else if constexpr (SimulationControl::kViewModel == ViewModel::Plt) {
    view_output_directory = this->output_directory_ / "plt";
  } else if constexpr (SimulationControl::kViewModel == ViewModel::Vtu) {
    view_output_directory = this->output_directory_ / "vtu";
  }
  if (std::filesystem::exists(view_output_directory)) {
    std::filesystem::remove_all(view_output_directory);
  }
  std::filesystem::create_directories(view_output_directory);
  this->raw_binary_finout_.seekg(0, std::ios::beg);
  this->data_.initializeViewData(mesh);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_IO_CONTROL_HPP_
