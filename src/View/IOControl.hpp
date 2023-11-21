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
#include <vector>

#include "Mesh/BasisFunction.hpp"
#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename ElementTrait, typename SimulationControl>
struct ElementViewData {
  Eigen::Matrix<Real, ElementTrait::kBasisFunctionNumber, ElementTrait::kAllNodeNumber> basis_function_value_;
  Eigen::Matrix<Real, ElementTrait::kBasisFunctionNumber, ElementTrait::kAdjacencyNodeNumber>
      basis_function_adjacency_value_;
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>,
               Eigen::Dynamic, 1>
      conserved_variable_basis_function_coefficient_;

  inline void readElementRawBinary(const ElementMesh<ElementTrait>& element_mesh, std::fstream& raw_binary_finout);

  inline ElementViewData();
};

template <typename SimulationControl, int Dimension>
struct ViewData;

template <typename SimulationControl>
struct ViewData<SimulationControl, 2> {
  ElementViewData<TriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> triangle_;
  ElementViewData<QuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> quadrangle_;

  void readRawBinary(const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
                     std::fstream& raw_binary_finout);
};

template <typename SimulationControl>
struct ViewBase {
  int io_interval_;
  std::filesystem::path output_directory_;
  std::string output_file_name_prefix_;
  std::fstream raw_binary_finout_;
  std::ofstream view_fout_;

  inline void initializeViewRawBinary();

  inline void initializeView();
};

template <typename SimulationControl, ViewModel ViewModelType>
struct View;

template <typename SimulationControl>
struct View<SimulationControl, ViewModel::Dat> : ViewBase<SimulationControl> {
  ViewData<SimulationControl, SimulationControl::kDimension> data_;

  inline void updateViewFout(int step);

  inline void writeStep(const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
                        const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model);

  inline void write(int iteration_number, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
                    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model);
};

template <typename ElementTrait, typename SimulationControl>
inline ElementViewData<ElementTrait, SimulationControl>::ElementViewData() {
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kAllNodeNumber> all_node_coordinate{
      getElementNodeCoordinate<ElementTrait::kElementType, SimulationControl::kPolynomialOrder>().data()};
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

template <typename SimulationControl>
inline void ViewBase<SimulationControl>::initializeViewRawBinary() {
#ifdef SUBROSA_DG_DEVELOP
  this->raw_binary_finout_.open((this->output_directory_ / "raw.binary").string(),
                                std::ios::in | std::ios::out | std::ios::trunc);
#else
  this->raw_binary_finout_.open((this->output_directory_ / "raw.binary").string(),
                                std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
#endif
}

template <typename SimulationControl>
inline void ViewBase<SimulationControl>::initializeView() {
  std::filesystem::path view_output_directory;
  if constexpr (SimulationControl::kViewModel == ViewModel::Dat) {
    view_output_directory = this->output_directory_ / "dat";
  } else if constexpr (SimulationControl::kViewModel == ViewModel::Msh) {
    view_output_directory = this->output_directory_ / "msh";
  } else if constexpr (SimulationControl::kViewModel == ViewModel::Plt) {
    view_output_directory = this->output_directory_ / "plt";
  } else if constexpr (SimulationControl::kViewModel == ViewModel::Vtk) {
    view_output_directory = this->output_directory_ / "vtk";
  }
  if (std::filesystem::exists(view_output_directory)) {
    std::filesystem::remove_all(view_output_directory);
  }
  std::filesystem::create_directories(view_output_directory);
  this->view_fout_.setf(std::ios::left, std::ios::adjustfield);
  this->view_fout_.setf(std::ios::scientific, std::ios::floatfield);
  this->raw_binary_finout_.seekg(0, std::ios::beg);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_IO_CONTROL_HPP_
