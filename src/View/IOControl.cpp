/**
 * @file IOControl.cpp
 * @brief The header file of IOControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
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
#include "Solver/SimulationControl.cpp"
#include "Solver/VariableConvertor.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct ViewSolver;

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementViewSolver {
  Eigen::Array<ViewVariable<VolumeElementTrait, SimulationControl>, Eigen::Dynamic, 1> view_variable_;

  inline void computeVolumeElementViewVariable(const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
                                               std::stringstream& raw_binary_ss);
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementViewSolver {
  Eigen::Array<ViewVariable<AdjacencyElementTrait, SimulationControl>, Eigen::Dynamic, 1> view_variable_;

  template <typename VolumeElementTrait>
  inline void computeAdjacencyPerElementViewVariable(std::stringstream& raw_binary_ss, Isize parent_gmsh_type_number,
                                                     Isize adjacency_sequence_in_parent, Isize element_index);

  inline void computeAdjacencyElementViewVariable(
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh, std::stringstream& raw_binary_ss);
};

template <typename SimulationControl, int Dimension>
struct ViewSolverData;

template <typename SimulationControl>
struct ViewSolverData<SimulationControl, 1> {
  AdjacencyElementViewSolver<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>, SimulationControl> point_;
  VolumeElementViewSolver<VolumeLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> line_;
};

template <typename SimulationControl>
struct ViewSolverData<SimulationControl, 2> {
  AdjacencyElementViewSolver<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> line_;
  VolumeElementViewSolver<VolumeTriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> triangle_;
  VolumeElementViewSolver<VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> quadrangle_;
};

template <typename SimulationControl>
struct ViewSolverData<SimulationControl, 3> {
  AdjacencyElementViewSolver<AdjacencyTriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> triangle_;
  AdjacencyElementViewSolver<AdjacencyQuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl>
      quadrangle_;
  VolumeElementViewSolver<VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>, SimulationControl> tetrahedron_;
  VolumeElementViewSolver<VolumePyramidTrait<SimulationControl::kPolynomialOrder>, SimulationControl> pyramid_;
  VolumeElementViewSolver<VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>, SimulationControl> hexahedron_;
};

template <typename SimulationControl>
struct ViewSolver : ViewSolverData<SimulationControl, SimulationControl::kDimension> {
  std::filesystem::path raw_binary_path_;
  std::stringstream raw_binary_ss_;

  template <typename VolumeElementTrait>
  static VolumeElementViewSolver<VolumeElementTrait, SimulationControl> ViewSolver::* getVolumeElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Line) {
        return &ViewSolver::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Triangle) {
        return &ViewSolver::triangle_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &ViewSolver::quadrangle_;
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Tetrahedron) {
        return &ViewSolver::tetrahedron_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Pyramid) {
        return &ViewSolver::pyramid_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Hexahedron) {
        return &ViewSolver::hexahedron_;
      }
    }
    return nullptr;
  }

  template <typename AdjacencyElementTrait>
  static AdjacencyElementViewSolver<AdjacencyElementTrait, SimulationControl> ViewSolver::* getAdjacencyElement() {
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

  inline void computeViewVariable(const Mesh<SimulationControl>& mesh);

  ViewSolver(const Mesh<SimulationControl>& mesh) {
    if constexpr (SimulationControl::kDimension == 1) {
      this->line_.view_variable_.resize(mesh.line_.number_);
      this->point_.view_variable_.resize(mesh.point_.boundary_number_);
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
        this->triangle_.view_variable_.resize(mesh.triangle_.number_);
      }
      if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
        this->quadrangle_.view_variable_.resize(mesh.quadrangle_.number_);
      }
      this->line_.view_variable_.resize(mesh.line_.boundary_number_);
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
        this->tetrahedron_.view_variable_.resize(mesh.tetrahedron_.number_);
      }
      if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
        this->pyramid_.view_variable_.resize(mesh.pyramid_.number_);
      }
      if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
        this->hexahedron_.view_variable_.resize(mesh.hexahedron_.number_);
      }
      if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
        this->triangle_.view_variable_.resize(mesh.triangle_.boundary_number_);
      }
      if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
        this->quadrangle_.view_variable_.resize(mesh.quadrangle_.boundary_number_);
      }
    }
  }
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

  ViewSupplemental(const Mesh<SimulationControl>& mesh, const std::vector<ViewVariableEnum>& variable_type,
                   const Isize physical_index) {
    this->data_set_data_.resize(variable_type.size() + 3);
    const PhysicalInformation& physical_information = mesh.physical_.information_[static_cast<Usize>(physical_index)];
    this->node_coordinate_.resize(Eigen::NoChange, physical_information.node_number_);
    this->node_coordinate_.setZero();
    this->node_variable_.resize(static_cast<Isize>(variable_type.size()));
    for (Isize i = 0; const auto variable : variable_type) {
      if ((SimulationControl::kDimension >= 2) &&
          (variable == ViewVariableEnum::Velocity || variable == ViewVariableEnum::MachNumber ||
           variable == ViewVariableEnum::HeatFlux ||
           (variable == ViewVariableEnum::Vorticity && SimulationControl::kDimension == 3))) {
        this->node_variable_(i++).resize(3 * physical_information.node_number_);
      } else {
        this->node_variable_(i++).resize(physical_information.node_number_);
      }
    }
    this->element_connectivity_.resize(physical_information.vtk_node_number_);
    this->element_offset_.resize(physical_information.vtk_element_number_);
    this->element_type_.resize(physical_information.vtk_element_number_);
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

  inline std::string getBaseName(std::string_view physical_name, /*const*/ int step);

  inline void getDataSetInformation(std::vector<vtu11::DataSetInfo>& data_set_information);

  template <typename VolumeElementTrait>
  inline void computeViewVariable(const ViewVariable<VolumeElementTrait, SimulationControl>& view_variable,
                                  Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
                                  /*const*/ Isize node_index, /*const*/ Isize column);

  template <typename AdjacencyElementTrait, typename VolumeElementTrait>
  inline void computeAdjacencyForce(const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
                                    const ViewVariable<VolumeElementTrait, SimulationControl>& view_variable,
                                    Eigen::Vector<Real, SimulationControl::kDimension>& force,
                                    /*const*/ Isize element_index,
                                    /*const*/ Isize column);

  template <typename AdjacencyElementTrait>
  inline void writeAdjacencyElement(const MeshPhysical& mesh_physical,
                                    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
                                    const ViewSolver<SimulationControl>& view_solver,
                                    ViewSupplemental<SimulationControl>& view_supplemental,
                                    /*const*/ Isize physical_index,
                                    /*const*/ Isize element_index);

  template <typename VolumeElementTrait>
  inline void writeVolumeElement(const MeshPhysical& mesh_physical,
                                 const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
                                 const ViewSolver<SimulationControl>& view_solver,
                                 ViewSupplemental<SimulationControl>& view_supplemental, /*const*/ Isize physical_index,
                                 /*const*/ Isize element_index);

  template <int Dimension, bool IsAdjacency>
  inline void writeField(const Mesh<SimulationControl>& mesh, const ViewSolver<SimulationControl>& view_solver,
                         ViewSupplemental<SimulationControl>& view_supplemental, /*const*/ Isize physical_index);

  template <int Dimension, bool IsAdjacency>
  inline void writeView(const Mesh<SimulationControl>& mesh, const ViewSolver<SimulationControl>& view_solver,
                        const std::string& base_name, /*const*/ int step, /*const*/ Isize physical_index);

  inline void stepView(const Mesh<SimulationControl>& mesh, ViewSolver<SimulationControl>& view_solver,
                       /*const*/ int step);

  void initializeSolverFinout(std::fstream& error_finout, const bool delete_dir) {
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

  void finalizeSolverFinout(std::fstream& error_finout) { error_finout.close(); }

  void initializeViewFin(const int iteration_end, const int error_output_interval, const bool delete_dir) {
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
    this->readTimeValue(iteration_end, error_output_interval);
  }

  void readTimeValue(const int iteration_end, const int error_output_interval) {
    this->time_value_.resize(iteration_end + 1);
    std::string line;
    std::getline(this->error_fin_, line);
    for (int i = 0; i <= iteration_end; i += error_output_interval) {
      std::getline(this->error_fin_, line);
      std::stringstream ss(line);
      ss.ignore(2) >> this->time_value_(i);
    }
  }

  void finalizeViewFin() { this->error_fin_.close(); }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_IO_CONTROL_CPP_
