/**
 * @file IOControl.hpp
 * @brief The header file of IOControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_IO_CONTROL_HPP_
#define SUBROSA_DG_IO_CONTROL_HPP_

#include <Eigen/Core>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
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

template <typename ElementTrait>
struct ElementViewBasisFunction {
  Eigen::Matrix<Real, ElementTrait::kBasisFunctionNumber, ElementTrait::kAllNodeNumber> basis_function_value_;

  inline ElementViewBasisFunction() {
    Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kAllNodeNumber> all_node_coordinate{
        getElementNodeCoordinate<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>().data()};
    Eigen::Matrix<double, 3, ElementTrait::kAllNodeNumber> local_coord_gmsh_matrix;
    local_coord_gmsh_matrix.setZero();
    local_coord_gmsh_matrix(Eigen::seqN(Eigen::fix<0>, Eigen::fix<ElementTrait::kDimension>), Eigen::all) =
        all_node_coordinate.template cast<double>();
    std::vector<double> local_coord(local_coord_gmsh_matrix.data(),
                                    local_coord_gmsh_matrix.data() + local_coord_gmsh_matrix.size());
    std::vector<double> basis_functions =
        getElementBasisFunction<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>(local_coord);
    for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kBasisFunctionNumber; j++) {
        this->basis_function_value_(j, i) =
            static_cast<Real>(basis_functions[static_cast<Usize>(i * ElementTrait::kBasisFunctionNumber + j)]);
      }
    }
  }
};

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
struct ElementViewSolver;

template <typename ElementTrait, typename SimulationControl>
struct ElementViewSolver<ElementTrait, SimulationControl, EquationModelEnum::Euler>
    : ElementViewBasisFunction<ElementTrait> {
  Eigen::Array<ViewVariable<SimulationControl>, ElementTrait::kAllNodeNumber, Eigen::Dynamic> view_variable_;

  inline void calcluateElementViewVariable(const ElementMesh<ElementTrait>& element_mesh,
                                           const ThermalModel<SimulationControl>& thermal_model,
                                           std::stringstream& raw_binary_ss);

  inline ElementViewSolver() : ElementViewBasisFunction<ElementTrait>(){};
};

template <typename ElementTrait, typename SimulationControl>
struct ElementViewSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>
    : ElementViewBasisFunction<ElementTrait> {
  Eigen::Array<ViewVariable<SimulationControl>, ElementTrait::kAllNodeNumber, Eigen::Dynamic> view_variable_;

  inline void calcluateElementViewVariable(const ElementMesh<ElementTrait>& element_mesh,
                                           const ThermalModel<SimulationControl>& thermal_model,
                                           std::stringstream& raw_binary_ss);

  inline ElementViewSolver() : ElementViewBasisFunction<ElementTrait>(){};
};

template <typename SimulationControl, int Dimension>
struct ViewSolverData;

template <typename SimulationControl>
struct ViewSolverData<SimulationControl, 1> {
  ElementViewSolver<LineTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                    SimulationControl::kEquationModel>
      line_;
};

template <typename SimulationControl>
struct ViewSolverData<SimulationControl, 2> {
  ElementViewSolver<TriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                    SimulationControl::kEquationModel>
      triangle_;
  ElementViewSolver<QuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                    SimulationControl::kEquationModel>
      quadrangle_;
};

template <typename SimulationControl>
struct ViewSolverData<SimulationControl, 3> {
  ElementViewSolver<TetrahedronTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                    SimulationControl::kEquationModel>
      tetrahedron_;
  ElementViewSolver<PyramidTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                    SimulationControl::kEquationModel>
      pyramid_;
  ElementViewSolver<HexahedronTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                    SimulationControl::kEquationModel>
      hexahedron_;
};

template <typename SimulationControl>
struct ViewSolver : ViewSolverData<SimulationControl, SimulationControl::kDimension> {
  template <typename ElementTrait>
  inline static ElementViewSolver<ElementTrait, SimulationControl, SimulationControl::kEquationModel> ViewSolver::*
  getElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (ElementTrait::kElementType == ElementEnum::Line) {
        return &ViewSolver::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (ElementTrait::kElementType == ElementEnum::Triangle) {
        return &ViewSolver::triangle_;
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &ViewSolver::quadrangle_;
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (ElementTrait::kElementType == ElementEnum::Tetrahedron) {
        return &ViewSolver::tetrahedron_;
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Pyramid) {
        return &ViewSolver::pyramid_;
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Hexahedron) {
        return &ViewSolver::hexahedron_;
      }
    }
    return nullptr;
  }

  inline void calcluateViewVariable(const Mesh<SimulationControl>& mesh,
                                    const ThermalModel<SimulationControl>& thermal_model,
                                    const std::filesystem::path& raw_binary_path, std::stringstream& raw_binary_ss);

  inline void initialViewSolver(const Mesh<SimulationControl>& mesh);

  inline void initialViewSolver(const ViewSolver<SimulationControl>& view_solver);
};

template <typename SimulationControl>
struct ViewData {
  std::filesystem::path raw_binary_path_;
  std::stringstream raw_binary_ss_;
  ViewSolver<SimulationControl> solver_;

  ViewData(const Mesh<SimulationControl>& mesh) { this->solver_.initialViewSolver(mesh); }

  ViewData(const ViewData<SimulationControl>& view_data) { this->solver_.initialViewSolver(view_data.solver_); }
};

template <typename SimulationControl>
struct ViewSupplemental {
  Isize node_index_{0};
  Isize vtk_node_index_{0};
  Isize vtk_element_index_{0};
  Eigen::Matrix<Real, 3, Eigen::Dynamic> node_coordinate_;
  Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1> node_variable_;
  Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic> element_connectivity_;
  Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic> element_offset_;
  Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic> element_type_;

  ViewSupplemental(Isize physical_index, const Mesh<SimulationControl>& mesh,
                   const std::vector<ViewVariableEnum>& variable_type) {
    const Isize node_number = mesh.information_.physical_information_.at(physical_index).node_number_;
    const Isize vtk_node_number = mesh.information_.physical_information_.at(physical_index).vtk_node_number_;
    const Isize vtk_element_number = mesh.information_.physical_information_.at(physical_index).vtk_element_number_;
    this->node_coordinate_.resize(Eigen::NoChange, node_number);
    this->node_coordinate_.setZero();
    this->node_variable_.resize(static_cast<Isize>(variable_type.size()));
    for (Isize i = 0; const auto variable : variable_type) {
      if ((SimulationControl::kDimension >= 2) &&
          (variable == ViewVariableEnum::Velocity || variable == ViewVariableEnum::MachNumber ||
           (variable == ViewVariableEnum::Vorticity && SimulationControl::kDimension == 3))) {
        this->node_variable_(i++).resize(3 * node_number);
      } else {
        this->node_variable_(i++).resize(node_number);
      }
    }
    this->element_connectivity_.resize(vtk_node_number);
    this->element_offset_.resize(vtk_element_number);
    this->element_type_.resize(vtk_element_number);
  }
};

template <typename SimulationControl>
struct View {
  int io_interval_;
  int iteration_order_;
  std::filesystem::path output_directory_;
  std::string output_file_name_prefix_;
  std::fstream error_fin_;
  std::vector<ViewVariableEnum> variable_type_;
  Eigen::Vector<Real, Eigen::Dynamic> time_value_;

  inline std::string getBaseName(int step, std::string_view physical_name);

  inline void getDataSetInfomatoin(std::vector<vtu11::DataSetInfo>& data_set_information);

  inline void calculateViewVariable(const ThermalModel<SimulationControl>& thermal_model,
                                    const ViewVariable<SimulationControl>& view_variable,
                                    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
                                    Isize node_index);

  template <typename AdjacencyElementTrait>
  inline void writeAdjacencyElement(Isize physical_index, const MeshInformation& mesh_information,
                                    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
                                    const ThermalModel<SimulationControl>& thermal_model,
                                    const ViewData<SimulationControl>& view_data, Isize element_index,
                                    ViewSupplemental<SimulationControl>& view_supplemental);

  template <typename ElementTrait>
  inline void writeElement(Isize physical_index, const MeshInformation& mesh_information,
                           const ElementMesh<ElementTrait>& element_mesh,
                           const ThermalModel<SimulationControl>& thermal_model,
                           const ViewData<SimulationControl>& view_data, Isize element_index,
                           ViewSupplemental<SimulationControl>& view_supplemental);

  template <int Dimension, bool IsAdjacency>
  inline void writeField(Isize physical_index, const Mesh<SimulationControl>& mesh,
                         const ThermalModel<SimulationControl>& thermal_model,
                         const ViewData<SimulationControl>& view_data,
                         ViewSupplemental<SimulationControl>& view_supplemental);

  template <int Dimension, bool IsAdjacency>
  inline void writeView(int step, Isize physical_index, const Mesh<SimulationControl>& mesh,
                        const ThermalModel<SimulationControl>& thermal_model,
                        const ViewData<SimulationControl>& view_data, const std::string& base_name);

  inline void stepView(int step, const Mesh<SimulationControl>& mesh,
                       const ThermalModel<SimulationControl>& thermal_model, ViewData<SimulationControl>& view_data);

  inline void initializeSolverFinout(const bool delete_dir, std::fstream& error_finout) {
    const std::filesystem::path raw_output_directory = this->output_directory_ / "raw";
    std::ios::openmode open_mode = std::ios::in | std::ios::out;
    if (delete_dir && SimulationControl::kInitialCondition != InitialConditionEnum::LastStep) {
      if (std::filesystem::exists(raw_output_directory)) {
        std::filesystem::remove_all(raw_output_directory);
      }
      std::filesystem::create_directories(raw_output_directory);
      open_mode |= std::ios::trunc;
    } else {
      if (!std::filesystem::exists(raw_output_directory)) {
        std::filesystem::create_directories(raw_output_directory);
      }
      open_mode |= std::ios::app;
    }
    error_finout.open((this->output_directory_ / "error.txt").string(), open_mode);
    error_finout.setf(std::ios::left, std::ios::adjustfield);
    error_finout.setf(std::ios::scientific, std::ios::floatfield);
  }

  inline void finalizeSolverFinout(std::fstream& error_finout) { error_finout.close(); }

  inline void initializeViewFin(const bool delete_dir, const int iteration_end) {
    const std::filesystem::path view_output_directory = this->output_directory_ / "vtu";
    if (delete_dir && SimulationControl::kInitialCondition != InitialConditionEnum::LastStep) {
      if (std::filesystem::exists(view_output_directory)) {
        std::filesystem::remove_all(view_output_directory);
      }
      std::filesystem::create_directories(view_output_directory);
    } else {
      if (!std::filesystem::exists(view_output_directory)) {
        std::filesystem::create_directories(view_output_directory);
      }
    }
    this->error_fin_.open((this->output_directory_ / "error.txt").string(), std::ios::in);
    this->readTimeValue(iteration_end);
  }

  inline void readTimeValue(const int iteration_end) {
    this->time_value_.resize(iteration_end + 1);
    std::string line;
    std::getline(this->error_fin_, line);
    for (int i = 0; i <= iteration_end; i++) {
      std::getline(this->error_fin_, line);
      std::stringstream ss(line);
      ss.ignore(2) >> this->time_value_(i);
    }
  }

  inline void finalizeViewFin() { this->error_fin_.close(); }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_IO_CONTROL_HPP_
