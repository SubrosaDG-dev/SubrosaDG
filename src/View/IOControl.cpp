/**
 * @file IOControl.cpp
 * @brief The header file of IOControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_IO_CONTROL_CPP_
#define SUBROSA_DG_IO_CONTROL_CPP_

#include <Eigen/Core>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <vtu11-cpp17.hpp>

#include "Mesh/BasisFunction.cpp"
#include "Mesh/ReadControl.cpp"
#include "Solver/PhysicalModel.cpp"
#include "Solver/SimulationControl.cpp"
#include "Solver/VariableConvertor.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct ViewSolver;

template <typename AdjacencyElementTrait>
struct AdjacencyElementViewBasisFunction {
  Eigen::Matrix<Real, AdjacencyElementTrait::kBasicNodeNumber, AdjacencyElementTrait::kAllNodeNumber> nodal_value_;

  inline AdjacencyElementViewBasisFunction() {
    Eigen::Matrix<double, AdjacencyElementTrait::kDimension, AdjacencyElementTrait::kAllNodeNumber> all_node_coordinate{
        getElementNodeCoordinate<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>()
            .data()};
    Eigen::Matrix<double, 3, AdjacencyElementTrait::kAllNodeNumber> local_coord_gmsh_matrix{
        Eigen::Matrix<double, 3, AdjacencyElementTrait::kAllNodeNumber>::Zero()};
    local_coord_gmsh_matrix(Eigen::seqN(Eigen::fix<0>, Eigen::fix<AdjacencyElementTrait::kDimension>),
                            Eigen::all) = all_node_coordinate;
    std::vector<double> local_coord(local_coord_gmsh_matrix.data(),
                                    local_coord_gmsh_matrix.data() + local_coord_gmsh_matrix.size());
    std::vector<double> nodal_basis_functions{
        getElementNodalBasisFunction<AdjacencyElementTrait::kElementType, 1>(false, local_coord)};
    for (Isize i = 0; i < AdjacencyElementTrait::kAllNodeNumber; i++) {
      for (Isize j = 0; j < AdjacencyElementTrait::kBasicNodeNumber; j++) {
        this->nodal_value_(j, i) = static_cast<Real>(
            nodal_basis_functions[static_cast<Usize>(i * AdjacencyElementTrait::kBasicNodeNumber + j)]);
      }
    }
  }
};

template <typename ElementTrait>
struct ElementViewBasisFunction {
  Eigen::Matrix<Real, ElementTrait::kBasicNodeNumber, ElementTrait::kAllNodeNumber> nodal_value_;
  Eigen::Matrix<Real, ElementTrait::kBasisFunctionNumber, ElementTrait::kAllNodeNumber> modal_value_;

  inline ElementViewBasisFunction() {
    Eigen::Matrix<double, ElementTrait::kDimension, ElementTrait::kAllNodeNumber> all_node_coordinate{
        getElementNodeCoordinate<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>().data()};
    Eigen::Matrix<double, 3, ElementTrait::kAllNodeNumber> local_coord_gmsh_matrix{
        Eigen::Matrix<double, 3, ElementTrait::kAllNodeNumber>::Zero()};
    local_coord_gmsh_matrix(Eigen::seqN(Eigen::fix<0>, Eigen::fix<ElementTrait::kDimension>),
                            Eigen::all) = all_node_coordinate;
    std::vector<double> local_coord(local_coord_gmsh_matrix.data(),
                                    local_coord_gmsh_matrix.data() + local_coord_gmsh_matrix.size());
    std::vector<double> nodal_basis_functions{
        getElementNodalBasisFunction<ElementTrait::kElementType, 1>(false, local_coord)};
    for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kBasicNodeNumber; j++) {
        this->nodal_value_(j, i) =
            static_cast<Real>(nodal_basis_functions[static_cast<Usize>(i * ElementTrait::kBasicNodeNumber + j)]);
      }
    }
    std::vector<double> modal_basis_functions{
        getElementModalBasisFunction<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>(false, local_coord)};
    for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kBasisFunctionNumber; j++) {
        this->modal_value_(j, i) =
            static_cast<Real>(modal_basis_functions[static_cast<Usize>(i * ElementTrait::kBasisFunctionNumber + j)]);
      }
    }
  }
};

template <typename ElementTrait, typename SimulationControl>
struct ElementViewSolver {
  ElementViewBasisFunction<ElementTrait> basis_function_;
  Eigen::Array<ViewVariable<ElementTrait, SimulationControl>, Eigen::Dynamic, 1> view_variable_;

  inline void calcluateElementViewVariable(const ElementMesh<ElementTrait>& element_mesh,
                                           const PhysicalModel<SimulationControl>& physical_model,
                                           const Eigen::Vector<Real, Eigen::Dynamic>& node_artificial_viscosity,
                                           std::stringstream& raw_binary_ss);
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementViewSolver {
  AdjacencyElementViewBasisFunction<AdjacencyElementTrait> basis_function_;
  Eigen::Array<ViewVariable<AdjacencyElementTrait, SimulationControl>, Eigen::Dynamic, 1> view_variable_;

  template <typename ElementTrait>
  inline void calcluateAdjacencyPerElementViewVariable(
      const PhysicalModel<SimulationControl>& physical_model,
      const ElementViewSolver<ElementTrait, SimulationControl>& element_view_solver, std::stringstream& raw_binary_ss,
      Isize adjacency_sequence_in_parent, Isize parent_gmsh_type_number, Isize column);

  inline void calcluateAdjacencyElementViewVariable(
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
      const PhysicalModel<SimulationControl>& physical_model, const ViewSolver<SimulationControl>& view_solver,
      const Eigen::Vector<Real, Eigen::Dynamic>& node_artificial_viscosity, std::stringstream& raw_binary_ss);
};

template <typename SimulationControl, int Dimension>
struct ViewSolverData;

template <typename SimulationControl>
struct ViewSolverData<SimulationControl, 1> {
  AdjacencyElementViewSolver<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>, SimulationControl> point_;
  ElementViewSolver<LineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> line_;
};

template <typename SimulationControl>
struct ViewSolverData<SimulationControl, 2> {
  AdjacencyElementViewSolver<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> line_;
  ElementViewSolver<TriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> triangle_;
  ElementViewSolver<QuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> quadrangle_;
};

template <typename SimulationControl>
struct ViewSolverData<SimulationControl, 3> {
  AdjacencyElementViewSolver<AdjacencyTriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> triangle_;
  AdjacencyElementViewSolver<AdjacencyQuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl>
      quadrangle_;
  ElementViewSolver<TetrahedronTrait<SimulationControl::kPolynomialOrder>, SimulationControl> tetrahedron_;
  ElementViewSolver<PyramidTrait<SimulationControl::kPolynomialOrder>, SimulationControl> pyramid_;
  ElementViewSolver<HexahedronTrait<SimulationControl::kPolynomialOrder>, SimulationControl> hexahedron_;
};

template <typename SimulationControl>
struct ViewSolver : ViewSolverData<SimulationControl, SimulationControl::kDimension> {
  template <typename ElementTrait>
  inline static ElementViewSolver<ElementTrait, SimulationControl> ViewSolver::* getElement() {
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

  template <typename AdjacencyElementTrait>
  inline static AdjacencyElementViewSolver<AdjacencyElementTrait, SimulationControl> ViewSolver::*
  getAdjacencyElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
        return &ViewSolver::point_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
        return &ViewSolver::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
        return &ViewSolver::triangle_;
      }
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &ViewSolver::quadrangle_;
      }
    }
  }

  inline void calcluateViewVariable(const Mesh<SimulationControl>& mesh,
                                    const PhysicalModel<SimulationControl>& physical_model,
                                    const std::filesystem::path& raw_binary_path, std::stringstream& raw_binary_ss);

  inline void initialViewSolver(const Mesh<SimulationControl>& mesh);

  inline void initialViewSolver(const ViewSolver<SimulationControl>& view_solver);

  inline void updateRawBinaryVersion(const Mesh<SimulationControl>& mesh, const std::filesystem::path& raw_binary_path,
                                     std::stringstream& raw_binary_ss);
};

template <typename SimulationControl>
struct ViewData {
  std::filesystem::path raw_binary_path_;
  std::stringstream raw_binary_ss_;
  ViewSolver<SimulationControl> solver_;

  ViewData(const Mesh<SimulationControl>& mesh) { this->solver_.initialViewSolver(mesh); }

  // ViewData(const ViewData<SimulationControl>& view_data) { this->solver_.initialViewSolver(view_data.solver_); }
};

template <typename SimulationControl>
struct ViewSupplemental {
  Isize node_index_{0};
  Isize vtk_node_index_{0};
  Isize vtk_element_index_{0};
  std::vector<vtu11::DataSetInfo> data_set_information_;
  std::vector<vtu11::DataSetData> data_set_data_;
  Eigen::Matrix<Real, 3, Eigen::Dynamic> node_coordinate_;
  Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1> node_variable_;
  Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic> element_connectivity_;
  Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic> element_offset_;
  Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic> element_type_;
  Eigen::Vector<Real, SimulationControl::kDimension> force_{Eigen::Vector<Real, SimulationControl::kDimension>::Zero()};

  ViewSupplemental(Isize physical_index, const Mesh<SimulationControl>& mesh,
                   const std::vector<ViewVariableEnum>& variable_type) {
    this->data_set_data_.resize(variable_type.size() + 3);
    const Isize node_number = mesh.information_.physical_[static_cast<Usize>(physical_index)].node_number_;
    const Isize vtk_node_number = mesh.information_.physical_[static_cast<Usize>(physical_index)].vtk_node_number_;
    const Isize vtk_element_number =
        mesh.information_.physical_[static_cast<Usize>(physical_index)].vtk_element_number_;
    this->node_coordinate_.resize(Eigen::NoChange, node_number);
    this->node_coordinate_.setZero();
    this->node_variable_.resize(static_cast<Isize>(variable_type.size()));
    for (Isize i = 0; const auto variable : variable_type) {
      if ((SimulationControl::kDimension >= 2) &&
          (variable == ViewVariableEnum::Velocity || variable == ViewVariableEnum::MachNumber ||
           variable == ViewVariableEnum::HeatFlux ||
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

  template <typename ElementTrait>
  inline void calculateViewVariable(const PhysicalModel<SimulationControl>& physical_model,
                                    const ViewVariable<ElementTrait, SimulationControl>& view_variable,
                                    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
                                    Isize column, Isize node_index);

  template <typename AdjacencyElementTrait, typename ElementTrait>
  inline void calculateAdjacencyForce(const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
                                      const PhysicalModel<SimulationControl>& physical_model,
                                      const ViewVariable<ElementTrait, SimulationControl>& view_variable,
                                      Eigen::Vector<Real, SimulationControl::kDimension>& force, Isize element_index,
                                      Isize column);

  template <typename AdjacencyElementTrait>
  inline void writeAdjacencyElement(Isize physical_index, const MeshInformation& mesh_information,
                                    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
                                    const PhysicalModel<SimulationControl>& physical_model,
                                    const ViewData<SimulationControl>& view_data, Isize element_index,
                                    ViewSupplemental<SimulationControl>& view_supplemental);

  template <typename ElementTrait>
  inline void writeElement(Isize physical_index, const MeshInformation& mesh_information,
                           const ElementMesh<ElementTrait>& element_mesh,
                           const PhysicalModel<SimulationControl>& physical_model,
                           const ViewData<SimulationControl>& view_data, Isize element_index,
                           ViewSupplemental<SimulationControl>& view_supplemental);

  template <int Dimension, bool IsAdjacency>
  inline void writeField(Isize physical_index, const Mesh<SimulationControl>& mesh,
                         const PhysicalModel<SimulationControl>& physical_model,
                         const ViewData<SimulationControl>& view_data,
                         ViewSupplemental<SimulationControl>& view_supplemental);

  template <int Dimension, bool IsAdjacency>
  inline void writeView(int step, Isize physical_index, const Mesh<SimulationControl>& mesh,
                        const PhysicalModel<SimulationControl>& physical_model,
                        const ViewData<SimulationControl>& view_data, const std::string& base_name);

  inline void stepView(int step, const Mesh<SimulationControl>& mesh,
                       const PhysicalModel<SimulationControl>& physical_model, ViewData<SimulationControl>& view_data);

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

#endif  // SUBROSA_DG_IO_CONTROL_CPP_
