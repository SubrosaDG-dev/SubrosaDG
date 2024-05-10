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
#include <format>
#include <fstream>
#include <string>
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

template <typename AdjacencyElementTrait, PolynomialOrderEnum P>
struct AdjacencyElementViewBasisFunction;

template <PolynomialOrderEnum P>
struct AdjacencyElementViewBasisFunction<AdjacencyPointTrait<P>, P> {
  [[nodiscard]] inline Isize getAdjacencyParentElementViewBasisFunctionSequenceInParent(
      [[maybe_unused]] Isize parent_gmsh_type_number, Isize adjacency_sequence_in_parent,
      [[maybe_unused]] Isize node_sequence_in_adjacency) const {
    return adjacency_sequence_in_parent;
  }
};

template <PolynomialOrderEnum P>
struct AdjacencyElementViewBasisFunction<AdjacencyLineTrait<P>, P> {
  [[nodiscard]] inline Isize getAdjacencyParentElementViewBasisFunctionSequenceInParent(
      Isize parent_gmsh_type_number, Isize adjacency_sequence_in_parent, Isize node_sequence_in_adjacency) const {
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
};

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
    std::vector<double> basis_functions = getElementBasisFunction<ElementTrait>(local_coord);
    for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kBasisFunctionNumber; j++) {
        this->basis_function_value_(j, i) =
            static_cast<Real>(basis_functions[static_cast<Usize>(i * ElementTrait::kBasisFunctionNumber + j)]);
      }
    }
  }
};

template <typename AdjacencyElementTrait>
struct AdjacencyElementViewSolver
    : AdjacencyElementViewBasisFunction<AdjacencyElementTrait, AdjacencyElementTrait::kPolynomialOrder> {};

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
struct ElementViewSolver;

template <typename ElementTrait, typename SimulationControl>
struct ElementViewSolver<ElementTrait, SimulationControl, EquationModelEnum::Euler>
    : ElementViewBasisFunction<ElementTrait> {
  Eigen::Array<ViewVariable<SimulationControl>, ElementTrait::kAllNodeNumber, Eigen::Dynamic> view_variable_;

  inline void calcluateElementViewVariable(const ElementMesh<ElementTrait>& element_mesh,
                                           const ThermalModel<SimulationControl>& thermal_model,
                                           std::fstream& raw_binary_fin);

  inline ElementViewSolver() : ElementViewBasisFunction<ElementTrait>(){};
};

template <typename ElementTrait, typename SimulationControl>
struct ElementViewSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>
    : ElementViewBasisFunction<ElementTrait> {
  Eigen::Array<ViewVariable<SimulationControl>, ElementTrait::kAllNodeNumber, Eigen::Dynamic> view_variable_;

  inline void calcluateElementViewVariable(const ElementMesh<ElementTrait>& element_mesh,
                                           const ThermalModel<SimulationControl>& thermal_model,
                                           std::fstream& raw_binary_fin);

  inline ElementViewSolver() : ElementViewBasisFunction<ElementTrait>(){};
};

template <typename SimulationControl, int Dimension>
struct ViewSolverData;

template <typename SimulationControl>
struct ViewSolverData<SimulationControl, 1> {
  AdjacencyElementViewSolver<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>> point_;
  ElementViewSolver<LineTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                    SimulationControl::kEquationModel>
      line_;
};

template <typename SimulationControl>
struct ViewSolverData<SimulationControl, 2> {
  AdjacencyElementViewSolver<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>> line_;
  ElementViewSolver<TriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                    SimulationControl::kEquationModel>
      triangle_;
  ElementViewSolver<QuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                    SimulationControl::kEquationModel>
      quadrangle_;
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
    }
    return nullptr;
  }

  template <typename AdjacencyElementTrait>
  inline static AdjacencyElementViewSolver<AdjacencyElementTrait> ViewSolver::*getAdjacencyElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
        return &ViewSolver::point_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
        return &ViewSolver::line_;
      }
    }
    return nullptr;
  }

  inline void calcluateViewVariable(const Mesh<SimulationControl>& mesh,
                                    const ThermalModel<SimulationControl>& thermal_model, std::fstream& raw_binary_fin);

  inline void initialViewSolver(const Mesh<SimulationControl>& mesh);

  inline void initialViewSolver(const ViewSolver<SimulationControl>& view_solver);
};

template <typename SimulationControl>
struct ViewConfig {
  int io_interval_;
  int iteration_order_;
  std::filesystem::path output_directory_;
  std::string output_file_name_prefix_;
  std::fstream error_fin_;
  std::vector<ViewVariableEnum> variable_type_;
  Eigen::Vector<Real, Eigen::Dynamic> time_value_;
};

template <typename SimulationControl>
struct ViewData {
  std::fstream raw_binary_fin_;
  ViewSolver<SimulationControl> solver_;

  ViewData(const Mesh<SimulationControl>& mesh) { this->solver_.initialViewSolver(mesh); }

  ViewData(const ViewData<SimulationControl>& view_data) { this->solver_.initialViewSolver(view_data.solver_); }
};

template <typename SimulationControl, ViewModelEnum ViewModelType>
struct ViewBase;

template <typename SimulationControl>
struct ViewBase<SimulationControl, ViewModelEnum::Vtu> : ViewConfig<SimulationControl> {
  inline std::string getBaseName(int step, std::string_view physical_name);

  inline void getDataSetInfomatoin(std::vector<vtu11::DataSetInfo>& data_set_information);

  inline void calculateViewVariable(const ThermalModel<SimulationControl>& thermal_model,
                                    const ViewVariable<SimulationControl>& view_variable,
                                    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
                                    Isize column);

  template <typename AdjacencyElementTrait>
  inline void writeAdjacencyElement(Isize physical_index, const MeshInformation& mesh_information,
                                    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
                                    const ThermalModel<SimulationControl>& thermal_model,
                                    const ViewData<SimulationControl>& view_data,
                                    Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
                                    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
                                    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
                                    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
                                    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type,
                                    Isize element_index, Isize& column);

  template <typename ElementTrait>
  inline void writeElement(Isize physical_index, const MeshInformation& mesh_information,
                           const ElementMesh<ElementTrait>& element_mesh,
                           const ThermalModel<SimulationControl>& thermal_model,
                           const ViewData<SimulationControl>& view_data,
                           Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
                           Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
                           Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
                           Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
                           Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type, Isize element_index,
                           Isize& column);

  template <int Dimension, bool IsAdjacency>
  inline void writeField(Isize physical_index, const Mesh<SimulationControl>& mesh,
                         const ThermalModel<SimulationControl>& thermal_model,
                         const ViewData<SimulationControl>& view_data,
                         Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
                         Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
                         Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
                         Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
                         Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type);

  template <int Dimension, bool IsAdjacency>
  inline void writeView(int step, Isize physical_index, const Mesh<SimulationControl>& mesh,
                        const ThermalModel<SimulationControl>& thermal_model,
                        const ViewData<SimulationControl>& view_data, const std::string& base_name);

  inline void stepView(int step, const Mesh<SimulationControl>& mesh,
                       const ThermalModel<SimulationControl>& thermal_model, ViewData<SimulationControl>& view_data);
};

template <typename SimulationControl>
struct ViewBase<SimulationControl, ViewModelEnum::Dat> : ViewConfig<SimulationControl> {
  inline void setViewFout(int step, std::ofstream& fout);

  inline void writeAsciiVariableList(std::ofstream& fout);

  template <int Dimension>
  inline void writeAsciiHeader(Real time_value, std::string_view physical_name, Isize node_number, Isize element_number,
                               std::ofstream& fout);

  template <typename AdjacencyElementTrait>
  inline void writeAdjacencyElement(
      Isize physical_index, const MeshInformation& mesh_information,
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
      const ThermalModel<SimulationControl>& thermal_model, const ViewData<SimulationControl>& view_data,
      Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
      Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
      Eigen::Matrix<Isize, AdjacencyElementTrait::kTecplotBasicNodeNumber, Eigen::Dynamic>& element_connectivity,
      Isize element_index, Isize& column);

  template <typename ElementTrait>
  inline void writeElement(
      Isize physical_index, const MeshInformation& mesh_information, const ElementMesh<ElementTrait>& element_mesh,
      const ThermalModel<SimulationControl>& thermal_model, const ViewData<SimulationControl>& view_data,
      Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
      Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
      Eigen::Matrix<Isize, ElementTrait::kTecplotBasicNodeNumber, Eigen::Dynamic>& element_connectivity,
      Isize element_index, Isize& column);

  template <int Dimension, bool IsAdjacency>
  inline void writeField(
      Isize physical_index, const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const ViewData<SimulationControl>& view_data,
      Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
      Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
      Eigen::Matrix<Isize, getElementTecplotBasicNodeNumber<Dimension>(), Eigen::Dynamic>& element_connectivity);

  template <int Dimension, bool IsAdjacency>
  inline void writeView(int step, Isize physical_index, const Mesh<SimulationControl>& mesh,
                        const ThermalModel<SimulationControl>& thermal_model,
                        const ViewData<SimulationControl>& view_data, std::ofstream& fout);

  inline void stepView(int step, const Mesh<SimulationControl>& mesh,
                       const ThermalModel<SimulationControl>& thermal_model, ViewData<SimulationControl>& view_data);
};

template <typename SimulationControl>
struct View : ViewBase<SimulationControl, SimulationControl::kViewModel> {
  inline void initializeSolverFout(const bool delete_dir, const int iteration_start, std::fstream& error_fout) {
    const std::filesystem::path raw_output_directory = this->output_directory_ / "raw";
    std::ios::openmode open_mode = std::ios::out;
    if (delete_dir && iteration_start == 0) {
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
    error_fout.open((this->output_directory_ / "error.txt").string(), open_mode);
    error_fout.setf(std::ios::left, std::ios::adjustfield);
    error_fout.setf(std::ios::scientific, std::ios::floatfield);
  }

  inline void finalizeSolverFout(std::fstream& error_fout) { error_fout.close(); }

  inline void initializeViewFin(const bool delete_dir, const int iteration_start, const int iteration_end) {
    std::filesystem::path view_output_directory;
    if constexpr (SimulationControl::kViewModel == ViewModelEnum::Vtu) {
      view_output_directory = this->output_directory_ / "vtu";
    } else if constexpr (SimulationControl::kViewModel == ViewModelEnum::Dat) {
      view_output_directory = this->output_directory_ / "dat";
    }
    if (delete_dir && iteration_start == 0) {
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
    this->time_value_.resize(iteration_end);
    std::string line;
    std::getline(this->error_fin_, line);
    for (int i = 0; i < iteration_end; i++) {
      std::getline(this->error_fin_, line);
      std::stringstream ss(line);
      ss.ignore(2) >> this->time_value_(i);
    }
  }

  inline void setSolverRawBinaryFout(const int step, std::fstream& raw_binary_fout) {
    std::ios::openmode open_mode = std::ios::out;
#ifndef SUBROSA_DG_DEVELOP
    open_mode |= std::ios::binary;
#endif
    raw_binary_fout.open(
        (this->output_directory_ / std::format("raw/{}_{}.raw", this->output_file_name_prefix_, step)).string(),
        open_mode | std::ios::trunc);
    raw_binary_fout.setf(std::ios::left, std::ios::adjustfield);
    raw_binary_fout.setf(std::ios::scientific, std::ios::floatfield);
  }

  inline void setViewRawBinaryFin(const int step, std::fstream& raw_binary_fin) {
    std::ios::openmode open_mode = std::ios::in;
#ifndef SUBROSA_DG_DEVELOP
    open_mode |= std::ios::binary;
#endif
    raw_binary_fin.open(
        (this->output_directory_ / std::format("raw/{}_{}.raw", this->output_file_name_prefix_, step)).string(),
        open_mode);
    raw_binary_fin.setf(std::ios::left, std::ios::adjustfield);
    raw_binary_fin.setf(std::ios::scientific, std::ios::floatfield);
  }

  inline void finalizeSolverRawBinaryFout(std::fstream& raw_binary_fout) { raw_binary_fout.close(); }

  inline void finalizeViewRawBinaryFin(std::fstream& raw_binary_fin) { raw_binary_fin.close(); }

  inline void finalizeViewFin() { this->error_fin_.close(); }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_IO_CONTROL_HPP_
