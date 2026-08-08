/**
 * @file SolveControl.cpp
 * @brief The header file of SubrosaDG solve control.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-09
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SOLVE_CONTROL_CPP_
#define SUBROSA_DG_SOLVE_CONTROL_CPP_

#include <Eigen/Core>
#include <filesystem>
#include <fstream>
#include <future>
#include <sstream>

#include "Mesh/ReadControl.cpp"
#include "Solver/SimulationControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename SimulationControl, SourceTermEnum SourceTermType = SimulationControl::kSourceTerm>
struct SourceTerm;
template <typename SimulationControl, SourceTermEnum SourceTermType = SimulationControl::kSourceTerm>
struct SourceTermDevice;
template <typename SimulationControl, TimeIntegrationEnum TimeIntegrationType = SimulationControl::kTimeIntegration>
struct TimeIntegration;
template <typename SimulationControl>
struct Solver;
template <typename SimulationControl>
struct SolverDevice;

template <typename VolumeElementTrait, typename SimulationControl,
          EquationModelEnum EquationModelType = SimulationControl::kEquationModel>
struct VolumeElementSolverBase;

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSolverBase<VolumeElementTrait, SimulationControl, EquationModelEnum::Euler> {
  Eigen::Array<
      Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kAllBasisFunctionNumber>,
      Eigen::Dynamic, 1>
      variable_basis_function_coefficient_last_;
  Eigen::Array<
      Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kAllBasisFunctionNumber>,
      Eigen::Dynamic, 1>
      variable_basis_function_coefficient_;

  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                             VolumeElementTrait::kQuadratureNumber * SimulationControl::kDimension>,
               Eigen::Dynamic, 1>
      variable_quadrature_;
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                             VolumeElementTrait::kAllAdjacencyQuadratureNumber>,
               Eigen::Dynamic, 1>
      variable_adjacency_quadrature_;

  Eigen::Array<
      Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kAllBasisFunctionNumber>,
      Eigen::Dynamic, 1>
      variable_residual_;

  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kDimension, VolumeElementTrait::kAllNodeNumber>, Eigen::Dynamic,
               1>
      variable_rotation_velocity_coefficient_;
};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSolverBase<VolumeElementTrait, SimulationControl, EquationModelEnum::NS>
    : VolumeElementSolverBase<VolumeElementTrait, SimulationControl, EquationModelEnum::Euler> {
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             VolumeElementTrait::kAllBasisFunctionNumber>,
               Eigen::Dynamic, 1>
      variable_gradient_basis_function_coefficient_;
};

template <typename VolumeElementTrait, typename SimulationControl,
          EquationModelEnum EquationModelType = SimulationControl::kEquationModel>
struct VolumeElementSolverDeviceBase;

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSolverDeviceBase<VolumeElementTrait, SimulationControl, EquationModelEnum::Euler> {
  Device::Array<
      Device::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kAllBasisFunctionNumber>,
      Device::Dynamic, 1>
      variable_basis_function_coefficient_last_;
  Device::Array<
      Device::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kAllBasisFunctionNumber>,
      Device::Dynamic, 1>
      variable_basis_function_coefficient_;

  Device::Array<Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                               VolumeElementTrait::kQuadratureNumber * SimulationControl::kDimension>,
                Device::Dynamic, 1>
      variable_quadrature_;
  Device::Array<Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                               VolumeElementTrait::kAllAdjacencyQuadratureNumber>,
                Device::Dynamic, 1>
      variable_adjacency_quadrature_;

  Device::Array<
      Device::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kAllBasisFunctionNumber>,
      Device::Dynamic, 1>
      variable_residual_;

  Device::Array<Device::Matrix<Real, SimulationControl::kDimension, VolumeElementTrait::kAllNodeNumber>,
                Device::Dynamic, 1>
      variable_rotation_velocity_coefficient_;
};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSolverDeviceBase<VolumeElementTrait, SimulationControl, EquationModelEnum::NS>
    : VolumeElementSolverDeviceBase<VolumeElementTrait, SimulationControl, EquationModelEnum::Euler> {
  Device::Array<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllBasisFunctionNumber>,
                Device::Dynamic, 1>
      variable_gradient_basis_function_coefficient_;
};

template <typename VolumeElementTrait, typename SimulationControl,
          ViscousFluxEnum ViscousFluxType = SimulationControl::kViscousFlux>
struct VolumeElementGradientSolver;

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementGradientSolver<VolumeElementTrait, SimulationControl, ViscousFluxEnum::None> {
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             VolumeElementTrait::kAllBasisFunctionNumber>,
               Eigen::Dynamic, 1>
      variable_volume_gradient_basis_function_coefficient_;

  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             VolumeElementTrait::kQuadratureNumber * SimulationControl::kDimension>,
               Eigen::Dynamic, 1>
      variable_volume_gradient_quadrature_;
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             VolumeElementTrait::kAllAdjacencyQuadratureNumber>,
               Eigen::Dynamic, 1>
      variable_volume_gradient_adjacency_quadrature_;

  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             VolumeElementTrait::kAllBasisFunctionNumber>,
               Eigen::Dynamic, 1>
      variable_volume_gradient_residual_;
};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementGradientSolver<VolumeElementTrait, SimulationControl, ViscousFluxEnum::BR1>
    : VolumeElementGradientSolver<VolumeElementTrait, SimulationControl, ViscousFluxEnum::None> {
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             VolumeElementTrait::kAllBasisFunctionNumber>,
               Eigen::Dynamic, 1>
      variable_interface_gradient_basis_function_coefficient_;

  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             VolumeElementTrait::kAllAdjacencyQuadratureNumber>,
               Eigen::Dynamic, 1>
      variable_interface_gradient_adjacency_quadrature_;

  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             VolumeElementTrait::kAllBasisFunctionNumber>,
               Eigen::Dynamic, 1>
      variable_interface_gradient_residual_;
};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementGradientSolver<VolumeElementTrait, SimulationControl, ViscousFluxEnum::BR2>
    : VolumeElementGradientSolver<VolumeElementTrait, SimulationControl, ViscousFluxEnum::None> {
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             VolumeElementTrait::kAllBasisFunctionNumber * VolumeElementTrait::kAdjacencyNumber>,
               Eigen::Dynamic, 1>
      variable_interface_gradient_basis_function_coefficient_;

  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             VolumeElementTrait::kAllAdjacencyQuadratureNumber>,
               Eigen::Dynamic, 1>
      variable_interface_gradient_adjacency_quadrature_;

  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             VolumeElementTrait::kAllBasisFunctionNumber * VolumeElementTrait::kAdjacencyNumber>,
               Eigen::Dynamic, 1>
      variable_interface_gradient_residual_;
};

template <typename VolumeElementTrait, typename SimulationControl,
          ViscousFluxEnum ViscousFluxType = SimulationControl::kViscousFlux>
struct VolumeElementGradientSolverDevice;

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementGradientSolverDevice<VolumeElementTrait, SimulationControl, ViscousFluxEnum::None> {
  Device::Array<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllBasisFunctionNumber>,
                Device::Dynamic, 1>
      variable_volume_gradient_basis_function_coefficient_;

  Device::Array<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kQuadratureNumber * SimulationControl::kDimension>,
                Device::Dynamic, 1>
      variable_volume_gradient_quadrature_;
  Device::Array<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllAdjacencyQuadratureNumber>,
                Device::Dynamic, 1>
      variable_volume_gradient_adjacency_quadrature_;

  Device::Array<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllBasisFunctionNumber>,
                Device::Dynamic, 1>
      variable_volume_gradient_residual_;
};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementGradientSolverDevice<VolumeElementTrait, SimulationControl, ViscousFluxEnum::BR1>
    : VolumeElementGradientSolverDevice<VolumeElementTrait, SimulationControl, ViscousFluxEnum::None> {
  Device::Array<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllBasisFunctionNumber>,
                Device::Dynamic, 1>
      variable_interface_gradient_basis_function_coefficient_;

  Device::Array<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllAdjacencyQuadratureNumber>,
                Device::Dynamic, 1>
      variable_interface_gradient_adjacency_quadrature_;

  Device::Array<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllBasisFunctionNumber>,
                Device::Dynamic, 1>
      variable_interface_gradient_residual_;
};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementGradientSolverDevice<VolumeElementTrait, SimulationControl, ViscousFluxEnum::BR2>
    : VolumeElementGradientSolverDevice<VolumeElementTrait, SimulationControl, ViscousFluxEnum::None> {
  Device::Array<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllBasisFunctionNumber * VolumeElementTrait::kAdjacencyNumber>,
                Device::Dynamic, 1>
      variable_interface_gradient_basis_function_coefficient_;

  Device::Array<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllAdjacencyQuadratureNumber>,
                Device::Dynamic, 1>
      variable_interface_gradient_adjacency_quadrature_;

  Device::Array<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllBasisFunctionNumber * VolumeElementTrait::kAdjacencyNumber>,
                Device::Dynamic, 1>
      variable_interface_gradient_residual_;
};

template <typename VolumeElementTrait, typename SimulationControl,
          SourceTermEnum SourceTermType = SimulationControl::kSourceTerm>
struct VolumeElementSourceSolver;

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSourceSolver<VolumeElementTrait, SimulationControl, SourceTermEnum::None> {};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSourceSolver<VolumeElementTrait, SimulationControl, SourceTermEnum::Boussinesq> {
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kQuadratureNumber>,
               Eigen::Dynamic, 1>
      variable_source_quadrature_;
};

template <typename VolumeElementTrait, typename SimulationControl,
          SourceTermEnum SourceTermType = SimulationControl::kSourceTerm>
struct VolumeElementSourceSolverDevice;

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSourceSolverDevice<VolumeElementTrait, SimulationControl, SourceTermEnum::None> {};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSourceSolverDevice<VolumeElementTrait, SimulationControl, SourceTermEnum::Boussinesq> {
  Device::Array<
      Device::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kQuadratureNumber>,
      Device::Dynamic, 1>
      variable_source_quadrature_;
};

template <typename VolumeElementTrait, typename SimulationControl,
          EquationModelEnum EquationModelType = SimulationControl::kEquationModel>
struct VolumeElementSolverData;

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSolverData<VolumeElementTrait, SimulationControl, EquationModelEnum::CompressibleEuler>
    : VolumeElementSolverBase<VolumeElementTrait, SimulationControl, EquationModelEnum::Euler>,
      VolumeElementGradientSolver<VolumeElementTrait, SimulationControl, ViscousFluxEnum::None>,
      VolumeElementSourceSolver<VolumeElementTrait, SimulationControl> {};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSolverData<VolumeElementTrait, SimulationControl, EquationModelEnum::CompressibleNS>
    : VolumeElementSolverBase<VolumeElementTrait, SimulationControl, EquationModelEnum::NS>,
      VolumeElementGradientSolver<VolumeElementTrait, SimulationControl, SimulationControl::kViscousFlux>,
      VolumeElementSourceSolver<VolumeElementTrait, SimulationControl> {};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSolverData<VolumeElementTrait, SimulationControl, EquationModelEnum::IncompressibleEuler>
    : VolumeElementSolverBase<VolumeElementTrait, SimulationControl, EquationModelEnum::Euler>,
      VolumeElementGradientSolver<VolumeElementTrait, SimulationControl, ViscousFluxEnum::None>,
      VolumeElementSourceSolver<VolumeElementTrait, SimulationControl> {};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSolverData<VolumeElementTrait, SimulationControl, EquationModelEnum::IncompressibleNS>
    : VolumeElementSolverBase<VolumeElementTrait, SimulationControl, EquationModelEnum::NS>,
      VolumeElementGradientSolver<VolumeElementTrait, SimulationControl, SimulationControl::kViscousFlux>,
      VolumeElementSourceSolver<VolumeElementTrait, SimulationControl> {};

template <typename VolumeElementTrait, typename SimulationControl,
          EquationModelEnum EquationModelType = SimulationControl::kEquationModel>
struct VolumeElementSolverDataDevice;

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSolverDataDevice<VolumeElementTrait, SimulationControl, EquationModelEnum::CompressibleEuler>
    : VolumeElementSolverDeviceBase<VolumeElementTrait, SimulationControl, EquationModelEnum::Euler>,
      VolumeElementGradientSolverDevice<VolumeElementTrait, SimulationControl, ViscousFluxEnum::None>,
      VolumeElementSourceSolverDevice<VolumeElementTrait, SimulationControl> {};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSolverDataDevice<VolumeElementTrait, SimulationControl, EquationModelEnum::CompressibleNS>
    : VolumeElementSolverDeviceBase<VolumeElementTrait, SimulationControl, EquationModelEnum::NS>,
      VolumeElementGradientSolverDevice<VolumeElementTrait, SimulationControl>,
      VolumeElementSourceSolverDevice<VolumeElementTrait, SimulationControl> {};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSolverDataDevice<VolumeElementTrait, SimulationControl, EquationModelEnum::IncompressibleEuler>
    : VolumeElementSolverDeviceBase<VolumeElementTrait, SimulationControl, EquationModelEnum::Euler>,
      VolumeElementGradientSolverDevice<VolumeElementTrait, SimulationControl, ViscousFluxEnum::None>,
      VolumeElementSourceSolverDevice<VolumeElementTrait, SimulationControl> {};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSolverDataDevice<VolumeElementTrait, SimulationControl, EquationModelEnum::IncompressibleNS>
    : VolumeElementSolverDeviceBase<VolumeElementTrait, SimulationControl, EquationModelEnum::NS>,
      VolumeElementGradientSolverDevice<VolumeElementTrait, SimulationControl>,
      VolumeElementSourceSolverDevice<VolumeElementTrait, SimulationControl> {};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSolver : VolumeElementSolverData<VolumeElementTrait, SimulationControl> {
  Isize number_{0};

  Isize static_number_{0};
  Isize rotate_number_{0};

  inline void initializeVolumeElementSolver(const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
                                            [[maybe_unused]] std::stringstream& raw_binary_ss);

  inline void copyVolumeElementBasisFunctionCoefficient();

  inline void computeVolumeElementDeltaTime(const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
                                            /*const*/ Real courant_friedrichs_lewy_number, /*const*/ Real& delta_time);

  inline void rotateVolumeElementMesh(
      VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
      const Eigen::Vector<Real, SimulationControl::kDimension>& rotation_center,
      [[maybe_unused]] const Eigen::Vector<Real, 3>& rotation_axis, /*const*/ Real rotation_angular_velocity,
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& rotation_matrix);

  inline void computeVolumeElementQuadrature(const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
                                             [[maybe_unused]] const SourceTerm<SimulationControl>& source_term);

  inline void computeVolumeElementGradientQuadrature(const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh);

  inline void computeVolumeElementResidual(const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh);

  inline void computeVolumeElementGradientResidual(const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh);

  inline void updateVolumeElementBasisFunctionCoefficient(
      const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
      const TimeIntegration<SimulationControl>& time_integration, /*const*/ int rk_step);

  inline void updateVolumeElementGradientBasisFunctionCoefficient(
      const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh);

  inline void computeVolumeElementRelativeError(
      const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& relative_error);

  inline void writeVolumeElementRawBinary(std::stringstream& raw_binary_ss) const;
};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementSolverDevice : VolumeElementSolverDataDevice<VolumeElementTrait, SimulationControl> {
  Isize number_{0};

  Isize static_number_{0};
  Isize rotate_number_{0};

  inline void transferVolumeElementSolverToDevice(
      const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver);

  inline void copyVolumeElementBasisFunctionCoefficient();

  inline void rotateVolumeElementMesh(
      VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
      const Device::Vector<Real, SimulationControl::kDimension>& rotation_center,
      [[maybe_unused]] const Device::Vector<Real, 3>& rotation_axis, /*const*/ Real rotation_angular_velocity,
      const Device::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& rotation_matrix);

  inline void computeVolumeElementQuadrature(const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
                                             [[maybe_unused]] const SourceTermDevice<SimulationControl>& source_term);

  inline void computeVolumeElementGradientQuadrature(
      const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh);

  inline void computeVolumeElementResidual(const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh);

  inline void computeVolumeElementGradientResidual(
      const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh);

  inline void updateVolumeElementBasisFunctionCoefficient(
      const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
      const TimeIntegration<SimulationControl>& time_integration, /*const*/ int rk_step);

  inline void updateVolumeElementGradientBasisFunctionCoefficient(
      const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh);

  inline void computeVolumeElementRelativeError(
      const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
      Device::Vector<Real, SimulationControl::kConservedVariableNumber> relative_error);

  inline void transferVolumeElementSolverToHost(
      VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver);
};

template <typename AdjacencyElementTrait, typename SimulationControl,
          EquationModelEnum EquationModelType = SimulationControl::kEquationModel>
struct AdjacencyElementInterfaceSolver;

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementInterfaceSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::Euler> {
  Eigen::Array<
      Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, AdjacencyElementTrait::kQuadratureNumber>,
      Eigen::Dynamic, 1>
      interface_right_conserved_variable_;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementInterfaceSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NS>
    : AdjacencyElementInterfaceSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::Euler> {
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             AdjacencyElementTrait::kQuadratureNumber>,
               Eigen::Dynamic, 1>
      interface_right_conserved_variable_gradient_;
};

template <typename AdjacencyElementTrait, typename SimulationControl,
          EquationModelEnum EquationModelType = SimulationControl::kEquationModel>
struct AdjacencyElementInterfaceSolverDevice;

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementInterfaceSolverDevice<AdjacencyElementTrait, SimulationControl, EquationModelEnum::Euler> {
  Device::Array<
      Device::Matrix<Real, SimulationControl::kConservedVariableNumber, AdjacencyElementTrait::kQuadratureNumber>,
      Device::Dynamic, 1>
      interface_right_conserved_variable_;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementInterfaceSolverDevice<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NS>
    : AdjacencyElementInterfaceSolverDevice<AdjacencyElementTrait, SimulationControl, EquationModelEnum::Euler> {
  Device::Array<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               AdjacencyElementTrait::kQuadratureNumber>,
                Device::Dynamic, 1>
      interface_right_conserved_variable_gradient_;
};

template <typename AdjacencyElementTrait, typename SimulationControl,
          EquationModelEnum EquationModelType = SimulationControl::kEquationModel>
struct AdjacencyElementSolverData;

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolverData<AdjacencyElementTrait, SimulationControl, EquationModelEnum::CompressibleEuler>
    : AdjacencyElementInterfaceSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::Euler> {
  Eigen::Array<
      Eigen::Matrix<Real, SimulationControl::kComputationalVariableNumber, AdjacencyElementTrait::kQuadratureNumber>,
      Eigen::Dynamic, 1>
      boundary_dummy_right_computational_variable_;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolverData<AdjacencyElementTrait, SimulationControl, EquationModelEnum::CompressibleNS>
    : AdjacencyElementInterfaceSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NS> {
  Eigen::Array<
      Eigen::Matrix<Real, SimulationControl::kComputationalVariableNumber, AdjacencyElementTrait::kQuadratureNumber>,
      Eigen::Dynamic, 1>
      boundary_dummy_right_computational_variable_;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolverData<AdjacencyElementTrait, SimulationControl, EquationModelEnum::IncompressibleEuler>
    : AdjacencyElementInterfaceSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::Euler> {
  Eigen::Array<
      Eigen::Matrix<Real, SimulationControl::kComputationalVariableNumber, AdjacencyElementTrait::kQuadratureNumber>,
      Eigen::Dynamic, 1>
      boundary_dummy_right_computational_variable_;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolverData<AdjacencyElementTrait, SimulationControl, EquationModelEnum::IncompressibleNS>
    : AdjacencyElementInterfaceSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NS> {
  Eigen::Array<
      Eigen::Matrix<Real, SimulationControl::kComputationalVariableNumber, AdjacencyElementTrait::kQuadratureNumber>,
      Eigen::Dynamic, 1>
      boundary_dummy_right_computational_variable_;
};

template <typename AdjacencyElementTrait, typename SimulationControl,
          EquationModelEnum EquationModelType = SimulationControl::kEquationModel>
struct AdjacencyElementSolverDataDevice;

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolverDataDevice<AdjacencyElementTrait, SimulationControl, EquationModelEnum::CompressibleEuler>
    : AdjacencyElementInterfaceSolverDevice<AdjacencyElementTrait, SimulationControl, EquationModelEnum::Euler> {
  Device::Array<
      Device::Matrix<Real, SimulationControl::kComputationalVariableNumber, AdjacencyElementTrait::kQuadratureNumber>,
      Device::Dynamic, 1>
      boundary_dummy_right_computational_variable_;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolverDataDevice<AdjacencyElementTrait, SimulationControl, EquationModelEnum::CompressibleNS>
    : AdjacencyElementInterfaceSolverDevice<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NS> {
  Device::Array<
      Device::Matrix<Real, SimulationControl::kComputationalVariableNumber, AdjacencyElementTrait::kQuadratureNumber>,
      Device::Dynamic, 1>
      boundary_dummy_right_computational_variable_;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolverDataDevice<AdjacencyElementTrait, SimulationControl,
                                        EquationModelEnum::IncompressibleEuler>
    : AdjacencyElementInterfaceSolverDevice<AdjacencyElementTrait, SimulationControl, EquationModelEnum::Euler> {
  Device::Array<
      Device::Matrix<Real, SimulationControl::kComputationalVariableNumber, AdjacencyElementTrait::kQuadratureNumber>,
      Device::Dynamic, 1>
      boundary_dummy_right_computational_variable_;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolverDataDevice<AdjacencyElementTrait, SimulationControl, EquationModelEnum::IncompressibleNS>
    : AdjacencyElementInterfaceSolverDevice<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NS> {
  Device::Array<
      Device::Matrix<Real, SimulationControl::kComputationalVariableNumber, AdjacencyElementTrait::kQuadratureNumber>,
      Device::Dynamic, 1>
      boundary_dummy_right_computational_variable_;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolver : AdjacencyElementSolverData<AdjacencyElementTrait, SimulationControl> {
  Isize number_{0};

  Isize interior_number_{0};
  Isize boundary_number_{0};
  Isize interface_number_{0};

  Isize static_number_{0};
  Isize interior_static_number_{0};
  Isize boundary_static_number_{0};

  Isize rotate_number_{0};
  Isize interior_rotate_number_{0};
  Isize boundary_rotate_number_{0};

  inline void initializeAdjacencyElementSolver(
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh);

  inline void updateAdjacencyElementBoundaryVariable(
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
      const TimeIntegration<SimulationControl>& time_integration, /*const*/ int rk_step);

  template <typename VolumeElementTrait>
  inline void computeAdjacencyPerElementInterfaceVariable(
      const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
      const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver,
      const Eigen::Vector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>> interface_conserved_variable,
      /*const*/ Isize parent_index_each_type, /*const*/ Isize adjacency_sequence_in_parent);

  template <typename VolumeElementTrait, ViscousFluxEnum ViscousFluxType>
  inline void computeAdjacencyPerElementInterfaceVariable(
      const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
      const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver,
      const Eigen::Vector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>> interface_conserved_variable,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          interface_conserved_variable_gradient,
      /*const*/ Isize parent_index_each_type, /*const*/ Isize adjacency_sequence_in_parent);

  inline void updateAdjacencyElementInterfaceVariable(const Mesh<SimulationControl>& mesh,
                                                      const Solver<SimulationControl>& solver);

  inline void rotateAdjacencyElementMesh(
      AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
      const Eigen::Vector<Real, SimulationControl::kDimension>& rotation_center,
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& rotation_matrix);

  [[nodiscard]] inline Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
  getVariableAdjacencyQuadrature(Solver<SimulationControl>& solver, /*const*/ Isize parent_gmsh_type_number,
                                 /*const*/ Isize parent_index_each_type,
                                 /*const*/ Isize quadrature_node_sequence_in_parent);

  inline void computeInteriorAdjacencyElementQuadrature(const Mesh<SimulationControl>& mesh,
                                                        Solver<SimulationControl>& solver);

  inline void computeBoundaryAdjacencyElementQuadrature(const Mesh<SimulationControl>& mesh,
                                                        Solver<SimulationControl>& solver);

  inline void computeInterfaceAdjacencyElementQuadrature(const Mesh<SimulationControl>& mesh,
                                                         Solver<SimulationControl>& solver);

  [[nodiscard]] inline Eigen::Ref<
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
  getVariableVolumeGradientAdjacencyQuadrature(Solver<SimulationControl>& solver,
                                               /*const*/ Isize parent_gmsh_type_number,
                                               /*const*/ Isize parent_index_each_type,
                                               /*const*/ Isize quadrature_node_sequence_in_parent);

  [[nodiscard]] inline Eigen::Ref<
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
  getVariableInterfaceGradientAdjacencyQuadrature(Solver<SimulationControl>& solver,
                                                  /*const*/ Isize parent_gmsh_type_number,
                                                  /*const*/ Isize parent_index_each_type,
                                                  /*const*/ Isize quadrature_node_sequence_in_parent);

  inline void computeInteriorAdjacencyElementGradientQuadrature(const Mesh<SimulationControl>& mesh,
                                                                Solver<SimulationControl>& solver);

  inline void computeBoundaryAdjacencyElementGradientQuadrature(const Mesh<SimulationControl>& mesh,
                                                                Solver<SimulationControl>& solver);

  inline void computeInterfaceAdjacencyElementGradientQuadrature(const Mesh<SimulationControl>& mesh,
                                                                 Solver<SimulationControl>& solver);

  template <typename VolumeElementTrait>
  inline void writeBoundaryAdjacencyPerElementRawBinary(
      const VolumeElementSolver<VolumeElementTrait, SimulationControl>& element_solver,
      std::stringstream& raw_binary_ss, /*const*/ Isize parent_index_each_type,
      [[maybe_unused]] /*const*/ Isize adjacency_sequence_in_parent) const;

  inline void writeBoundaryAdjacencyElementRawBinary(
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
      const Solver<SimulationControl>& solver, std::stringstream& raw_binary_ss) const;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolverDevice : AdjacencyElementSolverDataDevice<AdjacencyElementTrait, SimulationControl> {
  Isize number_{0};

  Isize interior_number_{0};
  Isize boundary_number_{0};
  Isize interface_number_{0};

  Isize static_number_{0};
  Isize interior_static_number_{0};
  Isize boundary_static_number_{0};

  Isize rotate_number_{0};
  Isize interior_rotate_number_{0};
  Isize boundary_rotate_number_{0};

  inline void transferAdjacencyElementSolverToDevice(
      const AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>& adjacency_element_solver);

  inline void updateAdjacencyElementBoundaryVariable(
      const AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh,
      const TimeIntegration<SimulationControl>& time_integration, /*const*/ int rk_step);

  template <typename VolumeElementTrait>
  inline void computeAdjacencyPerElementInterfaceVariable(
      const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
      const AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh,
      const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>& volume_element_solver,
      const Device::StaticVector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>> interface_conserved_variable,
      /*const*/ Isize parent_index_each_type, /*const*/ Isize adjacency_sequence_in_parent);

  template <typename VolumeElementTrait, ViscousFluxEnum ViscousFluxType>
  inline void computeAdjacencyPerElementInterfaceVariable(
      const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
      const AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh,
      const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>& volume_element_solver,
      const Device::StaticVector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>> interface_conserved_variable,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          interface_conserved_variable_gradient,
      /*const*/ Isize parent_index_each_type, /*const*/ Isize adjacency_sequence_in_parent);

  inline void updateAdjacencyElementInterfaceVariable(const MeshDevice<SimulationControl>& mesh,
                                                      const SolverDevice<SimulationControl>& solver);

  inline void rotateAdjacencyElementMesh(
      AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh,
      const Device::Vector<Real, SimulationControl::kDimension>& rotation_center,
      const Device::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& rotation_matrix);

  [[nodiscard]] inline Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
  getVariableAdjacencyQuadrature(SolverDevice<SimulationControl> solver, /*const*/ Isize parent_gmsh_type_number,
                                 /*const*/ Isize parent_index_each_type,
                                 /*const*/ Isize quadrature_node_sequence_in_parent);

  inline void computeInteriorAdjacencyElementQuadrature(const MeshDevice<SimulationControl>& mesh,
                                                        SolverDevice<SimulationControl>& solver);

  inline void computeBoundaryAdjacencyElementQuadrature(const MeshDevice<SimulationControl>& mesh,
                                                        SolverDevice<SimulationControl>& solver);

  inline void computeInterfaceAdjacencyElementQuadrature(const MeshDevice<SimulationControl>& mesh,
                                                         SolverDevice<SimulationControl>& solver);

  [[nodiscard]] inline Device::View<
      Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
  getVariableVolumeGradientAdjacencyQuadrature(SolverDevice<SimulationControl> solver,
                                               /*const*/ Isize parent_gmsh_type_number,
                                               /*const*/ Isize parent_index_each_type,
                                               /*const*/ Isize quadrature_node_sequence_in_parent);

  [[nodiscard]] inline Device::View<
      Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
  getVariableInterfaceGradientAdjacencyQuadrature(SolverDevice<SimulationControl> solver,
                                                  /*const*/ Isize parent_gmsh_type_number,
                                                  /*const*/ Isize parent_index_each_type,
                                                  /*const*/ Isize quadrature_node_sequence_in_parent);

  inline void computeInteriorAdjacencyElementGradientQuadrature(const MeshDevice<SimulationControl>& mesh,
                                                                SolverDevice<SimulationControl>& solver);

  inline void computeBoundaryAdjacencyElementGradientQuadrature(const MeshDevice<SimulationControl>& mesh,
                                                                SolverDevice<SimulationControl>& solver);

  inline void computeInterfaceAdjacencyElementGradientQuadrature(const MeshDevice<SimulationControl>& mesh,
                                                                 SolverDevice<SimulationControl>& solver);
};

template <typename SimulationControl>
struct SolverBase {
#ifdef SUBROSA_DG_DEVELOP
  const int error_output_interval_{1};
#else
  const int error_output_interval_{10};
#endif
  std::filesystem::path raw_binary_path_;
  std::stringstream raw_binary_ss_;
  std::fstream error_finout_;
  std::future<void> write_raw_binary_future_;

  Eigen::Vector<Real, SimulationControl::kDimension> rotation_center_;
  Eigen::Vector<Real, 3> rotation_axis_;
  Real rotation_angular_velocity_{0.0};
  Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> rotation_matrix_;

  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> relative_error_;
};

template <typename SimulationControl>
struct SolverDeviceBase {
  Device::Vector<Real, SimulationControl::kDimension> rotation_center_;
  Device::Vector<Real, 3> rotation_axis_;
  Real rotation_angular_velocity_{0.0};
  Device::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> rotation_matrix_;

  Device::Vector<Real, SimulationControl::kConservedVariableNumber> relative_error_;
};

template <typename SimulationControl, int Dimension = SimulationControl::kDimension>
struct SolverData;

template <typename SimulationControl>
struct SolverData<SimulationControl, 1> : SolverBase<SimulationControl> {
  VolumeElementSolver<VolumeLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> line_;
  AdjacencyElementSolver<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>, SimulationControl> point_;
};

template <typename SimulationControl>
struct SolverData<SimulationControl, 2> : SolverBase<SimulationControl> {
  VolumeElementSolver<VolumeTriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> triangle_;
  VolumeElementSolver<VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> quadrangle_;
  AdjacencyElementSolver<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> line_;
};

template <typename SimulationControl>
struct SolverData<SimulationControl, 3> : SolverBase<SimulationControl> {
  VolumeElementSolver<VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>, SimulationControl> tetrahedron_;
  VolumeElementSolver<VolumePyramidTrait<SimulationControl::kPolynomialOrder>, SimulationControl> pyramid_;
  VolumeElementSolver<VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>, SimulationControl> hexahedron_;
  AdjacencyElementSolver<AdjacencyTriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> triangle_;
  AdjacencyElementSolver<AdjacencyQuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> quadrangle_;
};

template <typename SimulationControl, int Dimension = SimulationControl::kDimension>
struct SolverDataDevice;

template <typename SimulationControl>
struct SolverDataDevice<SimulationControl, 1> : SolverDeviceBase<SimulationControl> {
  VolumeElementSolverDevice<VolumeLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> line_;
  AdjacencyElementSolverDevice<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>, SimulationControl> point_;
};

template <typename SimulationControl>
struct SolverDataDevice<SimulationControl, 2> : SolverDeviceBase<SimulationControl> {
  VolumeElementSolverDevice<VolumeTriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> triangle_;
  VolumeElementSolverDevice<VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> quadrangle_;
  AdjacencyElementSolverDevice<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> line_;
};

template <typename SimulationControl>
struct SolverDataDevice<SimulationControl, 3> : SolverDeviceBase<SimulationControl> {
  VolumeElementSolverDevice<VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>, SimulationControl>
      tetrahedron_;
  VolumeElementSolverDevice<VolumePyramidTrait<SimulationControl::kPolynomialOrder>, SimulationControl> pyramid_;
  VolumeElementSolverDevice<VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>, SimulationControl> hexahedron_;
  AdjacencyElementSolverDevice<AdjacencyTriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl>
      triangle_;
  AdjacencyElementSolverDevice<AdjacencyQuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl>
      quadrangle_;
};

template <typename SimulationControl>
struct Solver : SolverData<SimulationControl> {
  template <typename VolumeElementTrait>
  static VolumeElementSolver<VolumeElementTrait, SimulationControl> Solver::* getVolumeElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Line) {
        return &Solver<SimulationControl>::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Triangle) {
        return &Solver<SimulationControl>::triangle_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &Solver<SimulationControl>::quadrangle_;
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Tetrahedron) {
        return &Solver<SimulationControl>::tetrahedron_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Pyramid) {
        return &Solver<SimulationControl>::pyramid_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Hexahedron) {
        return &Solver<SimulationControl>::hexahedron_;
      }
    }
    return nullptr;
  }

  template <typename AdjacencyElementTrait>
  static AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl> Solver::* getAdjacencyElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
        return &Solver<SimulationControl>::point_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
        return &Solver<SimulationControl>::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
        return &Solver<SimulationControl>::triangle_;
      }
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &Solver<SimulationControl>::quadrangle_;
      }
    }
    return nullptr;
  }

  inline void initializeSolver(const Mesh<SimulationControl>& mesh);

  inline void updateBoundaryVariable(const Mesh<SimulationControl>& mesh,
                                     const TimeIntegration<SimulationControl>& time_integration, /*const*/ int rk_step);

  inline void updateInterfaceVariable(const Mesh<SimulationControl>& mesh);

  inline void copyBasisFunctionCoefficient();

  inline void computeDeltaTime(const Mesh<SimulationControl>& mesh,
                               TimeIntegration<SimulationControl>& time_integration);

  inline void rotateMesh(Mesh<SimulationControl>& mesh, const TimeIntegration<SimulationControl>& time_integration,
                         /*const*/ int rk_step);

  inline void computeQuadrature(const Mesh<SimulationControl>& mesh, const SourceTerm<SimulationControl>& source_term);

  inline void computeGradientQuadrature(const Mesh<SimulationControl>& mesh);

  inline void computeAdjacencyQuadrature(const Mesh<SimulationControl>& mesh);

  inline void computeAdjacencyGradientQuadrature(const Mesh<SimulationControl>& mesh);

  inline void computeResidual(const Mesh<SimulationControl>& mesh);

  inline void computeGradientResidual(const Mesh<SimulationControl>& mesh);

  inline void updateBasisFunctionCoefficient(const Mesh<SimulationControl>& mesh,
                                             const TimeIntegration<SimulationControl>& time_integration,
                                             /*const*/ int rk_step);

  inline void updateGradientBasisFunctionCoefficient(const Mesh<SimulationControl>& mesh);

  inline void stepSolver(Mesh<SimulationControl>& mesh, const SourceTerm<SimulationControl>& source_term,
                         const TimeIntegration<SimulationControl>& time_integration);

  inline void computeRelativeError(const Mesh<SimulationControl>& mesh);

  inline void writeRawBinary(const Mesh<SimulationControl>& mesh,
                             const TimeIntegration<SimulationControl>& time_integration,
                             const std::filesystem::path& raw_binary_path);
};

template <typename SimulationControl>
struct SolverDevice : SolverDataDevice<SimulationControl> {
  template <typename VolumeElementTrait>
  static VolumeElementSolverDevice<VolumeElementTrait, SimulationControl> SolverDevice::* getVolumeElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Line) {
        return &SolverDevice<SimulationControl>::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Triangle) {
        return &SolverDevice<SimulationControl>::triangle_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &SolverDevice<SimulationControl>::quadrangle_;
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Tetrahedron) {
        return &SolverDevice<SimulationControl>::tetrahedron_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Pyramid) {
        return &SolverDevice<SimulationControl>::pyramid_;
      }
      if constexpr (VolumeElementTrait::kElementType == ElementEnum::Hexahedron) {
        return &SolverDevice<SimulationControl>::hexahedron_;
      }
    }
    return nullptr;
  }

  template <typename AdjacencyElementTrait>
  static AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl> SolverDevice::* getAdjacencyElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
        return &SolverDevice<SimulationControl>::point_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
        return &SolverDevice<SimulationControl>::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
        return &SolverDevice<SimulationControl>::triangle_;
      }
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &SolverDevice<SimulationControl>::quadrangle_;
      }
    }
    return nullptr;
  }

  inline void transferSolverToDevice(const Solver<SimulationControl>& solver);

  inline void updateInterfaceVariable(const MeshDevice<SimulationControl>& mesh);

  inline void copyBasisFunctionCoefficient();

  inline void rotateMesh(MeshDevice<SimulationControl>& mesh,
                         const TimeIntegration<SimulationControl>& time_integration,
                         /*const*/ int rk_step);

  inline void computeQuadrature(const MeshDevice<SimulationControl>& mesh,
                                [[maybe_unused]] const SourceTermDevice<SimulationControl>& source_term);

  inline void computeGradientQuadrature(const MeshDevice<SimulationControl>& mesh);

  inline void computeAdjacencyQuadrature(const MeshDevice<SimulationControl>& mesh);

  inline void computeAdjacencyGradientQuadrature(const MeshDevice<SimulationControl>& mesh);

  inline void computeResidual(const MeshDevice<SimulationControl>& mesh);

  inline void computeGradientResidual(const MeshDevice<SimulationControl>& mesh);

  inline void updateBasisFunctionCoefficient(const MeshDevice<SimulationControl>& mesh,
                                             const TimeIntegration<SimulationControl>& time_integration,
                                             /*const*/ int rk_step);

  inline void updateGradientBasisFunctionCoefficient(const MeshDevice<SimulationControl>& mesh);

  inline void stepSolver(MeshDevice<SimulationControl>& mesh, const SourceTermDevice<SimulationControl>& source_term,
                         const TimeIntegration<SimulationControl>& time_integration);

  inline void computeRelativeError(const MeshDevice<SimulationControl>& mesh, Solver<SimulationControl>& solver);

  inline void transferSolverToHost(Solver<SimulationControl>& solver);
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SOLVE_CONTROL_CPP_
