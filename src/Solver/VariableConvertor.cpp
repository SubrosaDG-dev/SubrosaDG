/**
 * @file VariableConvertor.cpp
 * @brief The head file of SubrosaDG variable convertor.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_VARIABLE_CONVERTOR_CPP_
#define SUBROSA_DG_VARIABLE_CONVERTOR_CPP_

#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <array>

#include "Mesh/ReadControl.cpp"
#include "Solver/PhysicalModel.cpp"
#include "Solver/SimulationControl.cpp"
#include "Solver/SolveControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct VariableIndex {
  template <ConservedVariableEnum ConservedVariableType>
  static consteval int get() {
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      if constexpr (ConservedVariableType == ConservedVariableEnum::Density) {
        return 0;
      } else if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumX ||
                           ConservedVariableType == ConservedVariableEnum::Momentum) {
        return 1;
      } else if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumY) {
        return 2;
      } else if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumZ) {
        return 3;
      } else if constexpr (ConservedVariableType == ConservedVariableEnum::DensityTotalEnergy) {
        return SimulationControl::kDimension + 1;
      }
    }
    if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      if constexpr (ConservedVariableType == ConservedVariableEnum::Density) {
        return 0;
      } else if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumX ||
                           ConservedVariableType == ConservedVariableEnum::Momentum) {
        return 1;
      } else if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumY) {
        return 2;
      } else if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumZ) {
        return 3;
      } else if constexpr (ConservedVariableType == ConservedVariableEnum::DensityInternalEnergy) {
        return SimulationControl::kDimension + 1;
      }
    }
    std::unreachable();
  }

  template <ComputationalVariableEnum ComputationalVariableType>
  static consteval int get() {
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      if constexpr (ComputationalVariableType == ComputationalVariableEnum::Density) {
        return 0;
      } else if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityX ||
                           ComputationalVariableType == ComputationalVariableEnum::Velocity) {
        return 1;
      } else if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityY) {
        return 2;
      } else if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityZ) {
        return 3;
      } else if constexpr (ComputationalVariableType == ComputationalVariableEnum::InternalEnergy) {
        return SimulationControl::kDimension + 1;
      } else if constexpr (ComputationalVariableType == ComputationalVariableEnum::Pressure) {
        return SimulationControl::kDimension + 2;
      }
    }
    if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      if constexpr (ComputationalVariableType == ComputationalVariableEnum::Density) {
        return 0;
      } else if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityX ||
                           ComputationalVariableType == ComputationalVariableEnum::Velocity) {
        return 1;
      } else if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityY) {
        return 2;
      } else if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityZ) {
        return 3;
      } else if constexpr (ComputationalVariableType == ComputationalVariableEnum::InternalEnergy) {
        return SimulationControl::kDimension + 1;
      } else if constexpr (ComputationalVariableType == ComputationalVariableEnum::Pressure) {
        return SimulationControl::kDimension + 2;
      }
    }
    std::unreachable();
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  static consteval int get() {
    if constexpr (IsEuler<SimulationControl::kEquationModel> || IsNS<SimulationControl::kEquationModel>) {
      if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::Density) {
        return 0;
      } else if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::VelocityX ||
                           PrimitiveVariableType == PrimitiveVariableEnum::Velocity) {
        return 1;
      } else if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::VelocityY) {
        return 2;
      } else if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::VelocityZ) {
        return 3;
      } else if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::Temperature) {
        return SimulationControl::kDimension + 1;
      }
    }
    std::unreachable();
  }

  template <VariableGradientEnum VariableGradientType>
  static consteval int get() {
    if constexpr (VariableGradientType == VariableGradientEnum::X) {
      return 0;
    } else if constexpr (VariableGradientType == VariableGradientEnum::Y) {
      return 1;
    } else if constexpr (VariableGradientType == VariableGradientEnum::Z) {
      return 2;
    }
    std::unreachable();
  }
};

template <typename SimulationControl>
struct Variable {
  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getScalar(VectorType&& variable) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return variable(kVariableIndex);
  }

  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getVector(VectorType&& variable) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return variable(Eigen::seqN(Eigen::fix<kVariableIndex>, Eigen::fix<SimulationControl::kDimension>));
  }

  static void convertConservedFromComputational(
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>& computational_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& conserved_variable) {
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      const Real density = getScalar<ComputationalVariableEnum::Density>(computational_variable);
      Variable<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(conserved_variable) = density;
      const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> velocity =
          Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(computational_variable);
      Variable<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(conserved_variable) =
          density * velocity;
      Variable<SimulationControl>::template getScalar<ConservedVariableEnum::DensityTotalEnergy>(conserved_variable) =
          density * (Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                         computational_variable) +
                     velocity.squaredNorm() / 2.0_r);
    }
    if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      const Real density =
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(computational_variable);
      Variable<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(conserved_variable) = density;
      Variable<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(conserved_variable) =
          density *
          Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(computational_variable);
      Variable<SimulationControl>::template getScalar<ConservedVariableEnum::DensityInternalEnergy>(
          conserved_variable) =
          density * Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                        computational_variable);
    }
  }

  static void convertComputationalFromConserved(
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& conserved_variable,
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>& computational_variable) {
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      const Real density =
          Variable<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(conserved_variable);
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(computational_variable) =
          density;
      const Eigen::Vector<Real, SimulationControl::kDimension> velocity =
          Variable<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(conserved_variable) /
          density;
      Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(computational_variable) =
          velocity;
      const Real internal_energy =
          Variable<SimulationControl>::template getScalar<ConservedVariableEnum::DensityTotalEnergy>(
              conserved_variable) /
              density -
          velocity.squaredNorm() / 2.0_r;
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
          computational_variable) = internal_energy;
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(computational_variable) =
          PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
              density, internal_energy);
    }
    if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      const Real density =
          Variable<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(conserved_variable);
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(computational_variable) =
          density;
      Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(computational_variable) =
          Variable<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(conserved_variable) /
          density;
      const Real internal_energy =
          Variable<SimulationControl>::template getScalar<ConservedVariableEnum::DensityInternalEnergy>(
              conserved_variable) /
          density;
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
          computational_variable) = internal_energy;
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(computational_variable) =
          PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
              density, internal_energy);
    }
  }

  static void convertComputationalFromPrimitive(
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& primitive_variable,
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>& computational_variable) {
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      const Real density =
          Variable<SimulationControl>::template getScalar<PrimitiveVariableEnum::Density>(primitive_variable);
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(computational_variable) =
          density;
      Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(computational_variable) =
          Variable<SimulationControl>::template getVector<PrimitiveVariableEnum::Velocity>(primitive_variable);
      const Real internal_energy =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeInternalEnergyFromTemperature(
              Variable<SimulationControl>::template getScalar<PrimitiveVariableEnum::Temperature>(primitive_variable));
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
          computational_variable) = internal_energy;
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(computational_variable) =
          PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
              density, internal_energy);
    }
    if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      const Real density =
          Variable<SimulationControl>::template getScalar<PrimitiveVariableEnum::Density>(primitive_variable);
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(computational_variable) =
          density;
      Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(computational_variable) =
          Variable<SimulationControl>::template getVector<PrimitiveVariableEnum::Velocity>(primitive_variable);
      const Real internal_energy =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeInternalEnergyFromTemperature(
              Variable<SimulationControl>::template getScalar<PrimitiveVariableEnum::Temperature>(primitive_variable));
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
          computational_variable) = internal_energy;
      ;
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(computational_variable) =
          PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
              density, internal_energy);
    }
  }
};

template <typename SimulationControl>
struct VariableDevice {
  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getScalar(VectorType&& variable) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return variable(kVariableIndex);
  }

  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getVector(VectorType&& variable) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return variable.slice(Device::Slice<SimulationControl::kDimension>::seqN(kVariableIndex));
  }

  static void convertConservedFromComputational(
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>& computational_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& conserved_variable) {
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      const Real density = VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          computational_variable);
      VariableDevice<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(conserved_variable) =
          density;
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> velocity =
          VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              computational_variable);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> momentum =
          VariableDevice<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(conserved_variable);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        momentum(m) = density * velocity(m);
      }
      VariableDevice<SimulationControl>::template getScalar<ConservedVariableEnum::DensityTotalEnergy>(
          conserved_variable) =
          density * (VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                         computational_variable) +
                     velocity.squaredNorm() / 2.0_r);
    }
    if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      const Real density = VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          computational_variable);
      VariableDevice<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(conserved_variable) =
          density;
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> velocity =
          VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              computational_variable);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> momentum =
          VariableDevice<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(conserved_variable);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        momentum(m) = density * velocity(m);
      }
      VariableDevice<SimulationControl>::template getScalar<ConservedVariableEnum::DensityInternalEnergy>(
          conserved_variable) =
          density * VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                        computational_variable);
    }
  }

  static void convertComputationalFromConserved(
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& conserved_variable,
      Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>& computational_variable) {
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      const Real density =
          VariableDevice<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(conserved_variable);
      VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          computational_variable) = density;
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> momentum =
          VariableDevice<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(conserved_variable);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> velocity =
          VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              computational_variable);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        velocity(m) = momentum(m) / density;
      }
      const Real internal_energy =
          VariableDevice<SimulationControl>::template getScalar<ConservedVariableEnum::DensityTotalEnergy>(
              conserved_variable) /
              density -
          velocity.squaredNorm() / 2.0_r;
      VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
          computational_variable) = internal_energy;
      VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          computational_variable) =
          PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
              density, internal_energy);
    }
    if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      const Real density =
          VariableDevice<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(conserved_variable);
      VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          computational_variable) = density;
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> momentum =
          VariableDevice<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(conserved_variable);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> velocity =
          VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              computational_variable);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        velocity(m) = momentum(m) / density;
      }
      const Real internal_energy =
          VariableDevice<SimulationControl>::template getScalar<ConservedVariableEnum::DensityInternalEnergy>(
              conserved_variable) /
          density;
      VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
          computational_variable) = internal_energy;
      VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          computational_variable) =
          PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
              density, internal_energy);
    }
  }

  static void convertComputationalFromPrimitive(
      const Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber>& primitive_variable,
      Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>& computational_variable) {
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      const Real density =
          VariableDevice<SimulationControl>::template getScalar<PrimitiveVariableEnum::Density>(primitive_variable);
      VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          computational_variable) = density;
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> velocity =
          VariableDevice<SimulationControl>::template getVector<PrimitiveVariableEnum::Velocity>(primitive_variable);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> computational_velocity =
          VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              computational_variable);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        computational_velocity(m) = velocity(m);
      }
      const Real internal_energy =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeInternalEnergyFromTemperature(
              VariableDevice<SimulationControl>::template getScalar<PrimitiveVariableEnum::Temperature>(
                  primitive_variable));
      VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
          computational_variable) = internal_energy;
      VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          computational_variable) =
          PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
              density, internal_energy);
    }
    if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      const Real density =
          VariableDevice<SimulationControl>::template getScalar<PrimitiveVariableEnum::Density>(primitive_variable);
      VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          computational_variable) = density;
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> velocity =
          VariableDevice<SimulationControl>::template getVector<PrimitiveVariableEnum::Velocity>(primitive_variable);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> computational_velocity =
          VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              computational_variable);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        computational_velocity(m) = velocity(m);
      }
      const Real internal_energy =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeInternalEnergyFromTemperature(
              VariableDevice<SimulationControl>::template getScalar<PrimitiveVariableEnum::Temperature>(
                  primitive_variable));
      VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
          computational_variable) = internal_energy;
      ;
      VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          computational_variable) =
          PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
              density, internal_energy);
    }
  }
};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementVariable {
  static void get(const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
                  const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver,
                  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& quadrature_node_conserved_variable,
                  const Isize element_index, const Isize quadrature_sequence) {
    quadrature_node_conserved_variable.noalias() =
        volume_element_solver.variable_basis_function_coefficient_(element_index) *
        volume_element_mesh.nodal_basis_function_.row(quadrature_sequence).transpose();
  }

  static void getRotationVelocity(
      const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
      const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver,
      Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity, const Isize element_index,
      const Isize quadrature_sequence) {
    quadrature_node_rotation_velocity.noalias() =
        volume_element_solver.variable_rotation_velocity_coefficient_(element_index) *
        volume_element_mesh.nodal_basis_function_.row(quadrature_sequence).transpose();
  }
};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementVariableDevice {
  static void get(
      const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
      const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>& volume_element_solver,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& quadrature_node_conserved_variable,
      const Isize element_index, const Isize quadrature_sequence) {
    const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                            VolumeElementTrait::kAllBasisFunctionNumber>>
        variable_basis_function_coefficient = volume_element_solver.variable_basis_function_coefficient_.view(
            element_index, volume_element_solver.number_);
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      Real sum = 0.0_r;
      for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
        sum += variable_basis_function_coefficient(m, n) *
               volume_element_mesh.nodal_basis_function_(quadrature_sequence, n);
      }
      quadrature_node_conserved_variable(m) = sum;
    }
  }

  static void getRotationVelocity(
      const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
      const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>& volume_element_solver,
      Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Isize element_index, const Isize quadrature_sequence) {
    const Device::View<
        const Device::Matrix<Real, SimulationControl::kDimension, VolumeElementTrait::kAllBasisFunctionNumber>>
        variable_rotation_velocity_coefficient = volume_element_solver.variable_rotation_velocity_coefficient_.view(
            element_index, volume_element_solver.number_);
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      Real sum = 0.0_r;
      for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
        sum += variable_rotation_velocity_coefficient(m, n) *
               volume_element_mesh.nodal_basis_function_(quadrature_sequence, n);
      }
      quadrature_node_rotation_velocity(m) = sum;
    }
  }
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementVariable {
  template <typename VolumeElementTrait>
  static void compute(
      const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
      const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& quadrature_node_conserved_variable,
      const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent, const Isize quadrature_sequence) {
    constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber + 1> kAdjacencyQuadratureSequence{
        getVolumeElementAdjacencyQuadratureSequence<VolumeElementTrait::kElementType,
                                                    SimulationControl::kPolynomialOrder>()};
    quadrature_node_conserved_variable.noalias() =
        volume_element_solver.variable_basis_function_coefficient_(parent_index_each_type) *
        volume_element_mesh.nodal_adjacency_basis_function_
            .row(kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)] + quadrature_sequence)
            .transpose();
  }

  static void get(const Mesh<SimulationControl>& mesh, const Solver<SimulationControl>& solver,
                  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& quadrature_node_conserved_variable,
                  const Isize parent_gmsh_type_number, const Isize parent_index_each_type,
                  const Isize adjacency_sequence_in_parent, const Isize quadrature_sequence) {
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::template compute<
          VolumeLineTrait<SimulationControl::kPolynomialOrder>>(
          mesh.line_, solver.line_, quadrature_node_conserved_variable, parent_index_each_type,
          adjacency_sequence_in_parent, quadrature_sequence);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeTriangleTrait<SimulationControl::kPolynomialOrder>>(
            mesh.triangle_, solver.triangle_, quadrature_node_conserved_variable, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number ==
                 VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>>(
            mesh.quadrangle_, solver.quadrangle_, quadrature_node_conserved_variable, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
      if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>>(
            mesh.tetrahedron_, solver.tetrahedron_, quadrature_node_conserved_variable, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            mesh.pyramid_, solver.pyramid_, quadrature_node_conserved_variable, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            mesh.pyramid_, solver.pyramid_, quadrature_node_conserved_variable, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number ==
                 VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>>(
            mesh.hexahedron_, solver.hexahedron_, quadrature_node_conserved_variable, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    }
  }

  template <typename VolumeElementTrait>
  static void computeRotationVelocity(
      const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
      const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver,
      Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent, const Isize quadrature_sequence) {
    constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber + 1> kAdjacencyQuadratureSequence{
        getVolumeElementAdjacencyQuadratureSequence<VolumeElementTrait::kElementType,
                                                    SimulationControl::kPolynomialOrder>()};
    quadrature_node_rotation_velocity.noalias() =
        volume_element_solver.variable_rotation_velocity_coefficient_(parent_index_each_type) *
        volume_element_mesh.nodal_adjacency_basis_function_
            .row(kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)] + quadrature_sequence)
            .transpose();
  }

  static void getRotationVelocity(const Mesh<SimulationControl>& mesh, const Solver<SimulationControl>& solver,
                                  Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
                                  const Isize parent_gmsh_type_number, const Isize parent_index_each_type,
                                  const Isize adjacency_sequence_in_parent, const Isize quadrature_sequence) {
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::template computeRotationVelocity<
          VolumeLineTrait<SimulationControl::kPolynomialOrder>>(
          mesh.line_, solver.line_, quadrature_node_rotation_velocity, parent_index_each_type,
          adjacency_sequence_in_parent, quadrature_sequence);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::template computeRotationVelocity<
            VolumeTriangleTrait<SimulationControl::kPolynomialOrder>>(
            mesh.triangle_, solver.triangle_, quadrature_node_rotation_velocity, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number ==
                 VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::template computeRotationVelocity<
            VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>>(
            mesh.quadrangle_, solver.quadrangle_, quadrature_node_rotation_velocity, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
      if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::template computeRotationVelocity<
            VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>>(
            mesh.tetrahedron_, solver.tetrahedron_, quadrature_node_rotation_velocity, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::template computeRotationVelocity<
            VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            mesh.pyramid_, solver.pyramid_, quadrature_node_rotation_velocity, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::template computeRotationVelocity<
            VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            mesh.pyramid_, solver.pyramid_, quadrature_node_rotation_velocity, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number ==
                 VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::template computeRotationVelocity<
            VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>>(
            mesh.hexahedron_, solver.hexahedron_, quadrature_node_rotation_velocity, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    }
  }
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementVariableDevice {
  template <typename VolumeElementTrait>
  static void compute(
      const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
      const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>& volume_element_solver,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& quadrature_node_conserved_variable,
      const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent, const Isize quadrature_sequence) {
    constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber + 1> kAdjacencyQuadratureSequence{
        getVolumeElementAdjacencyQuadratureSequence<VolumeElementTrait::kElementType,
                                                    SimulationControl::kPolynomialOrder>()};
    const Isize row =
        kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)] + quadrature_sequence;
    const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                            VolumeElementTrait::kAllBasisFunctionNumber>>
        variable_basis_function_coefficient = volume_element_solver.variable_basis_function_coefficient_.view(
            parent_index_each_type, volume_element_solver.number_);
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      Real sum = 0.0_r;
      for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
        sum += variable_basis_function_coefficient(m, n) * volume_element_mesh.nodal_adjacency_basis_function_(row, n);
      }
      quadrature_node_conserved_variable(m) = sum;
    }
  }

  static void get(
      const MeshDevice<SimulationControl>& mesh, const SolverDevice<SimulationControl>& solver,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& quadrature_node_conserved_variable,
      const Isize parent_gmsh_type_number, const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent,
      const Isize quadrature_sequence) {
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::template compute<
          VolumeLineTrait<SimulationControl::kPolynomialOrder>>(
          mesh.line_, solver.line_, quadrature_node_conserved_variable, parent_index_each_type,
          adjacency_sequence_in_parent, quadrature_sequence);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeTriangleTrait<SimulationControl::kPolynomialOrder>>(
            mesh.triangle_, solver.triangle_, quadrature_node_conserved_variable, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number ==
                 VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>>(
            mesh.quadrangle_, solver.quadrangle_, quadrature_node_conserved_variable, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
      if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>>(
            mesh.tetrahedron_, solver.tetrahedron_, quadrature_node_conserved_variable, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            mesh.pyramid_, solver.pyramid_, quadrature_node_conserved_variable, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            mesh.pyramid_, solver.pyramid_, quadrature_node_conserved_variable, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number ==
                 VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>>(
            mesh.hexahedron_, solver.hexahedron_, quadrature_node_conserved_variable, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    }
  }

  template <typename VolumeElementTrait>
  static void computeRotationVelocity(
      const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
      const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>& volume_element_solver,
      Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent, const Isize quadrature_sequence) {
    constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber + 1> kAdjacencyQuadratureSequence{
        getVolumeElementAdjacencyQuadratureSequence<VolumeElementTrait::kElementType,
                                                    SimulationControl::kPolynomialOrder>()};
    const Isize row =
        kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)] + quadrature_sequence;
    const Device::View<
        const Device::Matrix<Real, SimulationControl::kDimension, VolumeElementTrait::kAllBasisFunctionNumber>>
        variable_rotation_velocity_coefficient = volume_element_solver.variable_rotation_velocity_coefficient_.view(
            parent_index_each_type, volume_element_solver.number_);
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      Real sum = 0.0_r;
      for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
        sum +=
            variable_rotation_velocity_coefficient(m, n) * volume_element_mesh.nodal_adjacency_basis_function_(row, n);
      }
      quadrature_node_rotation_velocity(m) = sum;
    }
  }

  static void getRotationVelocity(
      const MeshDevice<SimulationControl>& mesh, const SolverDevice<SimulationControl>& solver,
      Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Isize parent_gmsh_type_number, const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent,
      const Isize quadrature_sequence) {
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::template computeRotationVelocity<
          VolumeLineTrait<SimulationControl::kPolynomialOrder>>(
          mesh.line_, solver.line_, quadrature_node_rotation_velocity, parent_index_each_type,
          adjacency_sequence_in_parent, quadrature_sequence);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::template computeRotationVelocity<
            VolumeTriangleTrait<SimulationControl::kPolynomialOrder>>(
            mesh.triangle_, solver.triangle_, quadrature_node_rotation_velocity, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number ==
                 VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::template computeRotationVelocity<
            VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>>(
            mesh.quadrangle_, solver.quadrangle_, quadrature_node_rotation_velocity, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
      if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::template computeRotationVelocity<
            VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>>(
            mesh.tetrahedron_, solver.tetrahedron_, quadrature_node_rotation_velocity, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::template computeRotationVelocity<
            VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            mesh.pyramid_, solver.pyramid_, quadrature_node_rotation_velocity, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::template computeRotationVelocity<
            VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            mesh.pyramid_, solver.pyramid_, quadrature_node_rotation_velocity, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number ==
                 VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::template computeRotationVelocity<
            VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>>(
            mesh.hexahedron_, solver.hexahedron_, quadrature_node_rotation_velocity, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    }
  }
};

template <typename SimulationControl>
struct VariableGradient {
  template <auto VariableType, VariableGradientEnum VariableGradientType, typename VectorType>
  [[nodiscard]] static decltype(auto) getScalar(VectorType&& variable_gradient) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return variable_gradient(kVariableIndex * SimulationControl::kDimension +
                             VariableIndex<SimulationControl>::template get<VariableGradientType>());
  }

  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getScalarGradient(VectorType&& variable_gradient) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return variable_gradient(Eigen::seqN(Eigen::fix<kVariableIndex * SimulationControl::kDimension>,
                                         Eigen::fix<SimulationControl::kDimension>));
  }

  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getVectorGradient(VectorType&& variable_gradient) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return variable_gradient(Eigen::seqN(Eigen::fix<kVariableIndex * SimulationControl::kDimension>,
                                         Eigen::fix<SimulationControl::kDimension * SimulationControl::kDimension>))
        .reshaped(SimulationControl::kDimension, SimulationControl::kDimension);
  }

  static void convertPrimitiveFromConserved(
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>& computational_variable,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
          conserved_variable_gradient,
      Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          primitive_variable_gradient) {
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      const Real density =
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(computational_variable);
      const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> density_gradient =
          VariableGradient<SimulationControl>::template getScalarGradient<ConservedVariableEnum::Density>(
              conserved_variable_gradient);
      VariableGradient<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Density>(
          primitive_variable_gradient) = density_gradient;
      const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> velocity =
          Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(computational_variable);
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> velocity_gradient =
          (VariableGradient<SimulationControl>::template getVectorGradient<ConservedVariableEnum::Momentum>(
               conserved_variable_gradient) -
           density_gradient * velocity.transpose()) /
          density;
      VariableGradient<SimulationControl>::template getVectorGradient<PrimitiveVariableEnum::Velocity>(
          primitive_variable_gradient) = velocity_gradient;
      const Real total_energy =
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              computational_variable) +
          velocity.squaredNorm() / 2.0_r;
      const Eigen::Vector<Real, SimulationControl::kDimension> internal_energy_gradient =
          (VariableGradient<SimulationControl>::template getScalarGradient<ConservedVariableEnum::DensityTotalEnergy>(
               conserved_variable_gradient) -
           density_gradient * total_energy) /
              density -
          velocity_gradient * velocity;
      Eigen::Vector<Real, SimulationControl::kDimension> temperature_gradient;
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        temperature_gradient(m) =
            PhysicalModel<SimulationControl, PhysicalModelData>::computeTemperatureFromInternalEnergy(
                internal_energy_gradient(m));
      }
      VariableGradient<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Temperature>(
          primitive_variable_gradient) = temperature_gradient;
    }
    if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      const Real density =
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(computational_variable);
      const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> density_gradient =
          VariableGradient<SimulationControl>::template getScalarGradient<ConservedVariableEnum::Density>(
              conserved_variable_gradient);
      VariableGradient<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Density>(
          primitive_variable_gradient) = density_gradient;
      VariableGradient<SimulationControl>::template getVectorGradient<PrimitiveVariableEnum::Velocity>(
          primitive_variable_gradient) =
          (VariableGradient<SimulationControl>::template getVectorGradient<ConservedVariableEnum::Momentum>(
               conserved_variable_gradient) -
           density_gradient * Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
                                  computational_variable)
                                  .transpose()) /
          density;
      const Eigen::Vector<Real, SimulationControl::kDimension> internal_energy_gradient =
          (VariableGradient<SimulationControl>::template getScalarGradient<
               ConservedVariableEnum::DensityInternalEnergy>(conserved_variable_gradient) -
           density_gradient *
               Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                   computational_variable)) /
          density;
      Eigen::Vector<Real, SimulationControl::kDimension> temperature_gradient;
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        temperature_gradient(m) =
            PhysicalModel<SimulationControl, PhysicalModelData>::computeTemperatureFromInternalEnergy(
                internal_energy_gradient(m));
      }
      VariableGradient<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Temperature>(
          primitive_variable_gradient) = temperature_gradient;
    }
  }
};

template <typename SimulationControl>
struct VariableGradientDevice {
  template <auto VariableType, VariableGradientEnum VariableGradientType, typename VectorType>
  [[nodiscard]] static decltype(auto) getScalar(VectorType&& variable_gradient) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return variable_gradient(kVariableIndex * SimulationControl::kDimension +
                             VariableIndex<SimulationControl>::template get<VariableGradientType>());
  }

  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getScalarGradient(VectorType&& variable_gradient) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return variable_gradient.slice(
        Device::Slice<SimulationControl::kDimension>::seqN(kVariableIndex * SimulationControl::kDimension));
  }

  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getVectorGradient(VectorType&& variable_gradient) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return variable_gradient
        .slice(Device::Slice<SimulationControl::kDimension * SimulationControl::kDimension>::seqN(
            kVariableIndex * SimulationControl::kDimension))
        .template reshaped<SimulationControl::kDimension, SimulationControl::kDimension>();
  }

  static void convertPrimitiveFromConserved(
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>& computational_variable,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
          conserved_variable_gradient,
      Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          primitive_variable_gradient) {
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      const Real density = VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          computational_variable);
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> density_gradient =
          VariableGradientDevice<SimulationControl>::template getScalarGradient<ConservedVariableEnum::Density>(
              conserved_variable_gradient);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> primitive_density_gradient =
          VariableGradientDevice<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Density>(
              primitive_variable_gradient);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        primitive_density_gradient(m) = density_gradient(m);
      }
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> velocity =
          VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              computational_variable);
      const Device::View<const Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
          momentum_gradient =
              VariableGradientDevice<SimulationControl>::template getVectorGradient<ConservedVariableEnum::Momentum>(
                  conserved_variable_gradient);
      Device::View<Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
          velocity_gradient =
              VariableGradientDevice<SimulationControl>::template getVectorGradient<PrimitiveVariableEnum::Velocity>(
                  primitive_variable_gradient);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        for (Isize n = 0; n < SimulationControl::kDimension; n++) {
          velocity_gradient(m, n) = (momentum_gradient(m, n) - density_gradient(m) * velocity(n)) / density;
        }
      }
      const Real total_energy =
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              computational_variable) +
          velocity.squaredNorm() / 2.0_r;
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>>
          density_total_energy_gradient = VariableGradientDevice<SimulationControl>::template getScalarGradient<
              ConservedVariableEnum::DensityTotalEnergy>(conserved_variable_gradient);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> temperature_gradient =
          VariableGradientDevice<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Temperature>(
              primitive_variable_gradient);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        Real velocity_gradient_dot_velocity = 0.0_r;
        for (Isize n = 0; n < SimulationControl::kDimension; n++) {
          velocity_gradient_dot_velocity += velocity_gradient(m, n) * velocity(n);
        }
        temperature_gradient(m) =
            PhysicalModel<SimulationControl, PhysicalModelData>::computeTemperatureFromInternalEnergy(
                (density_total_energy_gradient(m) - density_gradient(m) * total_energy) / density -
                velocity_gradient_dot_velocity);
      }
    }
    if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      const Real density = VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          computational_variable);
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> density_gradient =
          VariableGradientDevice<SimulationControl>::template getScalarGradient<ConservedVariableEnum::Density>(
              conserved_variable_gradient);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> primitive_density_gradient =
          VariableGradientDevice<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Density>(
              primitive_variable_gradient);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        primitive_density_gradient(m) = density_gradient(m);
      }
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> velocity =
          VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              computational_variable);
      const Device::View<const Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
          momentum_gradient =
              VariableGradientDevice<SimulationControl>::template getVectorGradient<ConservedVariableEnum::Momentum>(
                  conserved_variable_gradient);
      Device::View<Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
          velocity_gradient =
              VariableGradientDevice<SimulationControl>::template getVectorGradient<PrimitiveVariableEnum::Velocity>(
                  primitive_variable_gradient);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        for (Isize n = 0; n < SimulationControl::kDimension; n++) {
          velocity_gradient(m, n) = (momentum_gradient(m, n) - density_gradient(m) * velocity(n)) / density;
        }
      }
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>>
          density_internal_energy_gradient = VariableGradientDevice<SimulationControl>::template getScalarGradient<
              ConservedVariableEnum::DensityInternalEnergy>(conserved_variable_gradient);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> temperature_gradient =
          VariableGradientDevice<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Temperature>(
              primitive_variable_gradient);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        temperature_gradient(m) =
            PhysicalModel<SimulationControl, PhysicalModelData>::computeTemperatureFromInternalEnergy(
                density_internal_energy_gradient(m) -
                density_gradient(m) *
                    VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                        computational_variable)) /
            density;
      }
    }
  }
};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementVariableGradient {
  template <ViscousFluxEnum ViscousFluxType>
  static void get(const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
                  const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver,
                  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
                      quadrature_node_conserved_variable_gradient,
                  const Isize element_index, const Isize quadrature_sequence) {
    if constexpr (ViscousFluxType == ViscousFluxEnum::None) {
      quadrature_node_conserved_variable_gradient.noalias() =
          volume_element_solver.variable_volume_gradient_basis_function_coefficient_(element_index) *
          volume_element_mesh.nodal_basis_function_.row(quadrature_sequence).transpose();
    } else {
      quadrature_node_conserved_variable_gradient.noalias() =
          volume_element_solver.variable_gradient_basis_function_coefficient_(element_index) *
          volume_element_mesh.nodal_basis_function_.row(quadrature_sequence).transpose();
    }
  }
};

template <typename VolumeElementTrait, typename SimulationControl>
struct VolumeElementVariableGradientDevice {
  template <ViscousFluxEnum ViscousFluxType>
  static void get(
      const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
      const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>& volume_element_solver,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
          quadrature_node_conserved_variable_gradient,
      const Isize element_index, const Isize quadrature_sequence) {
    if constexpr (ViscousFluxType == ViscousFluxEnum::None) {
      const Device::View<
          const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllBasisFunctionNumber>>
          variable_volume_gradient_basis_function_coefficient =
              volume_element_solver.variable_volume_gradient_basis_function_coefficient_.view(
                  element_index, volume_element_solver.number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
        Real sum = 0.0_r;
        for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
          sum += variable_volume_gradient_basis_function_coefficient(m, n) *
                 volume_element_mesh.nodal_basis_function_(quadrature_sequence, n);
        }
        quadrature_node_conserved_variable_gradient(m) = sum;
      }
    } else {
      const Device::View<
          const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllBasisFunctionNumber>>
          variable_gradient_basis_function_coefficient =
              volume_element_solver.variable_gradient_basis_function_coefficient_.view(element_index,
                                                                                       volume_element_solver.number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
        Real sum = 0.0_r;
        for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
          sum += variable_gradient_basis_function_coefficient(m, n) *
                 volume_element_mesh.nodal_basis_function_(quadrature_sequence, n);
        }
        quadrature_node_conserved_variable_gradient(m) = sum;
      }
    }
  }
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementVariableGradient {
  template <typename VolumeElementTrait, ViscousFluxEnum ViscousFluxType>
  static void compute(const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
                      const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver,
                      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
                          quadrature_node_conserved_variable_gradient,
                      const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent,
                      const Isize quadrature_sequence) {
    constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber + 1> kAdjacencyQuadratureSequence{
        getVolumeElementAdjacencyQuadratureSequence<VolumeElementTrait::kElementType,
                                                    SimulationControl::kPolynomialOrder>()};
    if constexpr (ViscousFluxType == ViscousFluxEnum::None) {
      quadrature_node_conserved_variable_gradient.noalias() =
          volume_element_solver.variable_volume_gradient_basis_function_coefficient_(parent_index_each_type) *
          volume_element_mesh.nodal_adjacency_basis_function_
              .row(kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)] + quadrature_sequence)
              .transpose();
    } else if constexpr (ViscousFluxType == ViscousFluxEnum::BR1) {
      quadrature_node_conserved_variable_gradient.noalias() =
          volume_element_solver.variable_gradient_basis_function_coefficient_(parent_index_each_type) *
          volume_element_mesh.nodal_adjacency_basis_function_
              .row(kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)] + quadrature_sequence)
              .transpose();
    } else if constexpr (ViscousFluxType == ViscousFluxEnum::BR2) {
      const Eigen::Ref<
          const Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                              VolumeElementTrait::kAllBasisFunctionNumber>>
          variable_interface_gradient_basis_function_coefficient =
              volume_element_solver.variable_interface_gradient_basis_function_coefficient_(parent_index_each_type)(
                  Eigen::placeholders::all,
                  Eigen::seqN(adjacency_sequence_in_parent * VolumeElementTrait::kAllBasisFunctionNumber,
                              Eigen::fix<VolumeElementTrait::kAllBasisFunctionNumber>));
      quadrature_node_conserved_variable_gradient.noalias() =
          (volume_element_solver.variable_volume_gradient_basis_function_coefficient_(parent_index_each_type) +
           variable_interface_gradient_basis_function_coefficient) *
          volume_element_mesh.nodal_adjacency_basis_function_
              .row(kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)] + quadrature_sequence)
              .transpose();
    }
  }

  template <ViscousFluxEnum ViscousFluxType>
  static void get(const Mesh<SimulationControl>& mesh, const Solver<SimulationControl>& solver,
                  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
                      quadrature_node_conserved_variable_gradient,
                  [[maybe_unused]] const Isize parent_gmsh_type_number, const Isize parent_index_each_type,
                  const Isize adjacency_sequence_in_parent, const Isize quadrature_sequence) {
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>::template compute<
          VolumeLineTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
          mesh.line_, solver.line_, quadrature_node_conserved_variable_gradient, parent_index_each_type,
          adjacency_sequence_in_parent, quadrature_sequence);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeTriangleTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.triangle_, solver.triangle_, quadrature_node_conserved_variable_gradient, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number ==
                 VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.quadrangle_, solver.quadrangle_, quadrature_node_conserved_variable_gradient, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
      if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.tetrahedron_, solver.tetrahedron_, quadrature_node_conserved_variable_gradient, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumePyramidTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.pyramid_, solver.pyramid_, quadrature_node_conserved_variable_gradient, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumePyramidTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.pyramid_, solver.pyramid_, quadrature_node_conserved_variable_gradient, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number ==
                 VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.hexahedron_, solver.hexahedron_, quadrature_node_conserved_variable_gradient, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    }
  }
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementVariableGradientDevice {
  template <typename VolumeElementTrait, ViscousFluxEnum ViscousFluxType>
  static void compute(
      const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
      const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>& volume_element_solver,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
          quadrature_node_conserved_variable_gradient,
      const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent, const Isize quadrature_sequence) {
    constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber + 1> kAdjacencyQuadratureSequence{
        getVolumeElementAdjacencyQuadratureSequence<VolumeElementTrait::kElementType,
                                                    SimulationControl::kPolynomialOrder>()};
    const Isize row =
        kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)] + quadrature_sequence;
    if constexpr (ViscousFluxType == ViscousFluxEnum::None) {
      const Device::View<
          const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllBasisFunctionNumber>>
          variable_volume_gradient_basis_function_coefficient =
              volume_element_solver.variable_volume_gradient_basis_function_coefficient_.view(
                  parent_index_each_type, volume_element_solver.number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
        Real sum = 0.0_r;
        for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
          sum += variable_volume_gradient_basis_function_coefficient(m, n) *
                 volume_element_mesh.nodal_adjacency_basis_function_(row, n);
        }
        quadrature_node_conserved_variable_gradient(m) = sum;
      }
    } else if constexpr (ViscousFluxType == ViscousFluxEnum::BR1) {
      const Device::View<
          const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllBasisFunctionNumber>>
          variable_gradient_basis_function_coefficient =
              volume_element_solver.variable_gradient_basis_function_coefficient_.view(parent_index_each_type,
                                                                                       volume_element_solver.number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
        Real sum = 0.0_r;
        for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
          sum += variable_gradient_basis_function_coefficient(m, n) *
                 volume_element_mesh.nodal_adjacency_basis_function_(row, n);
        }
        quadrature_node_conserved_variable_gradient(m) = sum;
      }
    } else if constexpr (ViscousFluxType == ViscousFluxEnum::BR2) {
      const Device::View<
          const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllBasisFunctionNumber>>
          variable_volume_gradient_basis_function_coefficient =
              volume_element_solver.variable_volume_gradient_basis_function_coefficient_.view(
                  parent_index_each_type, volume_element_solver.number_);
      const Device::View<
          const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllBasisFunctionNumber>>
          variable_interface_gradient_basis_function_coefficient =
              volume_element_solver.variable_interface_gradient_basis_function_coefficient_.slice(
                  parent_index_each_type, volume_element_solver.number_,
                  Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
                  Device::Slice<VolumeElementTrait::kAllBasisFunctionNumber>::seqN(
                      adjacency_sequence_in_parent * VolumeElementTrait::kAllBasisFunctionNumber));
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
        Real sum = 0.0_r;
        for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
          sum += (variable_volume_gradient_basis_function_coefficient(m, n) +
                  variable_interface_gradient_basis_function_coefficient(m, n)) *
                 volume_element_mesh.nodal_adjacency_basis_function_(row, n);
        }
        quadrature_node_conserved_variable_gradient(m) = sum;
      }
    }
  }

  template <ViscousFluxEnum ViscousFluxType>
  static void get(
      const MeshDevice<SimulationControl>& mesh, const SolverDevice<SimulationControl>& solver,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
          quadrature_node_conserved_variable_gradient,
      [[maybe_unused]] const Isize parent_gmsh_type_number, const Isize parent_index_each_type,
      const Isize adjacency_sequence_in_parent, const Isize quadrature_sequence) {
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      AdjacencyElementVariableGradientDevice<AdjacencyElementTrait, SimulationControl>::template compute<
          VolumeLineTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
          mesh.line_, solver.line_, quadrature_node_conserved_variable_gradient, parent_index_each_type,
          adjacency_sequence_in_parent, quadrature_sequence);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableGradientDevice<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeTriangleTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.triangle_, solver.triangle_, quadrature_node_conserved_variable_gradient, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number ==
                 VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableGradientDevice<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.quadrangle_, solver.quadrangle_, quadrature_node_conserved_variable_gradient, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
      if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableGradientDevice<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.tetrahedron_, solver.tetrahedron_, quadrature_node_conserved_variable_gradient, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableGradientDevice<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumePyramidTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.pyramid_, solver.pyramid_, quadrature_node_conserved_variable_gradient, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableGradientDevice<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumePyramidTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.pyramid_, solver.pyramid_, quadrature_node_conserved_variable_gradient, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      } else if (parent_gmsh_type_number ==
                 VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        AdjacencyElementVariableGradientDevice<AdjacencyElementTrait, SimulationControl>::template compute<
            VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.hexahedron_, solver.hexahedron_, quadrature_node_conserved_variable_gradient, parent_index_each_type,
            adjacency_sequence_in_parent, quadrature_sequence);
      }
    }
  }
};

template <typename SimulationControl>
struct NormalFlux {
  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getScalar(VectorType&& normal_flux) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return normal_flux(kVariableIndex);
  }

  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getVector(VectorType&& normal_flux) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return normal_flux(Eigen::seqN(Eigen::fix<kVariableIndex>, Eigen::fix<SimulationControl::kDimension>));
  }
};

template <typename SimulationControl>
struct NormalFluxDevice {
  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getScalar(VectorType&& normal_flux) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return normal_flux(kVariableIndex);
  }

  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getVector(VectorType&& normal_flux) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return normal_flux.slice(Device::Slice<SimulationControl::kDimension>::seqN(kVariableIndex));
  }
};

template <typename SimulationControl>
struct RawFlux {
  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getScalarDimension(VectorType&& raw_flux) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return raw_flux.col(kVariableIndex);
  }

  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getVectorDimension(VectorType&& raw_flux) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return raw_flux(Eigen::placeholders::all,
                    Eigen::seqN(Eigen::fix<kVariableIndex>, Eigen::fix<SimulationControl::kDimension>));
  }
};

template <typename SimulationControl>
struct RawFluxDevice {
  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getScalarDimension(VectorType&& raw_flux) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return raw_flux.slice(Device::Slice<SimulationControl::kDimension>::all(), Device::Slice<1>::seqN(kVariableIndex));
  }

  template <auto VariableType, typename VectorType>
  [[nodiscard]] static decltype(auto) getVectorDimension(VectorType&& raw_flux) {
    constexpr int kVariableIndex = VariableIndex<SimulationControl>::template get<VariableType>();
    return raw_flux.slice(Device::Slice<SimulationControl::kDimension>::all(),
                          Device::Slice<SimulationControl::kDimension>::seqN(kVariableIndex));
  }
};

template <typename ElementTrait, typename SimulationControl>
struct ViewVariable {
  Eigen::Matrix<Real, SimulationControl::kComputationalVariableNumber, ElementTrait::kAllNodeNumber> computational_;
  Eigen::Matrix<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension,
                ElementTrait::kAllNodeNumber>
      primitive_gradient_;

  void convertComputationalFromConserved(const Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                                             ElementTrait::kAllNodeNumber>& all_conserved_variable) {
    for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& conserved_variable =
          all_conserved_variable.col(i);
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> computational_variable;
      Variable<SimulationControl>::convertComputationalFromConserved(conserved_variable, computational_variable);
      this->computational_.col(i) = computational_variable;
    }
  }

  void convertPrimitiveGradientFromConservedGradient(
      const Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                          ElementTrait::kAllNodeNumber>& all_conserved_gradient_variable) {
    for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>& computational_variable =
          this->computational_.col(i);
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
          conserved_variable_gradient = all_conserved_gradient_variable.col(i);
      Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
          primitive_variable_gradient;
      VariableGradient<SimulationControl>::convertPrimitiveFromConserved(
          computational_variable, conserved_variable_gradient, primitive_variable_gradient);
      this->primitive_gradient_.col(i) = primitive_variable_gradient;
    }
  }

  [[nodiscard]] Real get(const ViewVariableEnum variable_type, const Isize column) const {
    const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>& computational_variable =
        this->computational_.col(column);
    const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
        primitive_variable_gradient = this->primitive_gradient_.col(column);
    switch (variable_type) {
    case ViewVariableEnum::Density:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          computational_variable);
    case ViewVariableEnum::Velocity:
      return Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
                 computational_variable)
          .norm();
    case ViewVariableEnum::Temperature:
      return PhysicalModel<SimulationControl, PhysicalModelData>::computeTemperatureFromInternalEnergy(
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              computational_variable));
    case ViewVariableEnum::Pressure:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          computational_variable);
    case ViewVariableEnum::SoundSpeed:
      return PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(computational_variable),
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(computational_variable));
    case ViewVariableEnum::MachNumber:
      return Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
                 computational_variable)
                 .norm() /
             PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                     computational_variable),
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                     computational_variable));
    case ViewVariableEnum::Entropy:
      if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
        return PhysicalModel<SimulationControl, PhysicalModelData>::computeEntropyFromDensityPressure(
            Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(computational_variable),
            Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                computational_variable));
      }
    case ViewVariableEnum::Vorticity:
      if constexpr (SimulationControl::kDimension == 2) {
        // vorticity = dv/dx - du/dy
        return VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityY,
                                                                       VariableGradientEnum::X>(
                   primitive_variable_gradient) -
               VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityX,
                                                                       VariableGradientEnum::Y>(
                   primitive_variable_gradient);
      }
      if constexpr (SimulationControl::kDimension == 3) {
        // vorticity = sqrt( (dw/dy - dv/dz)^2 + (du/dz - dw/dx)^2 + (dv/dx - du/dy)^2 )
        const Real partial_velocity_z_over_partial_y_minus_partial_velocity_y_over_partial_z =
            VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityZ,
                                                                    VariableGradientEnum::Y>(
                primitive_variable_gradient) -
            VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityY,
                                                                    VariableGradientEnum::Z>(
                primitive_variable_gradient);
        const Real partial_velocity_x_over_partial_z_minus_partial_velocity_z_over_partial_x =
            VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityX,
                                                                    VariableGradientEnum::Z>(
                primitive_variable_gradient) -
            VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityZ,
                                                                    VariableGradientEnum::X>(
                primitive_variable_gradient);
        const Real partial_velocity_y_over_partial_x_minus_partial_velocity_x_over_partial_y =
            VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityY,
                                                                    VariableGradientEnum::X>(
                primitive_variable_gradient) -
            VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityX,
                                                                    VariableGradientEnum::Y>(
                primitive_variable_gradient);
        return std::sqrt(partial_velocity_z_over_partial_y_minus_partial_velocity_y_over_partial_z *
                             partial_velocity_z_over_partial_y_minus_partial_velocity_y_over_partial_z +
                         partial_velocity_x_over_partial_z_minus_partial_velocity_z_over_partial_x *
                             partial_velocity_x_over_partial_z_minus_partial_velocity_z_over_partial_x +
                         partial_velocity_y_over_partial_x_minus_partial_velocity_x_over_partial_y *
                             partial_velocity_y_over_partial_x_minus_partial_velocity_x_over_partial_y);
      }
    case ViewVariableEnum::VelocityX:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::VelocityX>(
          computational_variable);
    case ViewVariableEnum::VelocityY:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::VelocityY>(
          computational_variable);
    case ViewVariableEnum::VelocityZ:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::VelocityZ>(
          computational_variable);
    case ViewVariableEnum::MachNumberX:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::VelocityX>(
                 computational_variable) /
             PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                     computational_variable),
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                     computational_variable));
    case ViewVariableEnum::MachNumberY:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::VelocityY>(
                 computational_variable) /
             PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                     computational_variable),
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                     computational_variable));
    case ViewVariableEnum::MachNumberZ:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::VelocityZ>(
                 computational_variable) /
             PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                     computational_variable),
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                     computational_variable));
    case ViewVariableEnum::VorticityX:
      return VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityZ,
                                                                     VariableGradientEnum::Y>(
                 primitive_variable_gradient) -
             VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityY,
                                                                     VariableGradientEnum::Z>(
                 primitive_variable_gradient);
    case ViewVariableEnum::VorticityY:
      return VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityX,
                                                                     VariableGradientEnum::Z>(
                 primitive_variable_gradient) -
             VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityZ,
                                                                     VariableGradientEnum::X>(
                 primitive_variable_gradient);
    case ViewVariableEnum::VorticityZ:
      return VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityY,
                                                                     VariableGradientEnum::X>(
                 primitive_variable_gradient) -
             VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityX,
                                                                     VariableGradientEnum::Y>(
                 primitive_variable_gradient);
    case ViewVariableEnum::HeatFluxX:
      return VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::Temperature,
                                                                     VariableGradientEnum::X>(
          primitive_variable_gradient);
    case ViewVariableEnum::HeatFluxY:
      return VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::Temperature,
                                                                     VariableGradientEnum::Y>(
          primitive_variable_gradient);
    case ViewVariableEnum::HeatFluxZ:
      return VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::Temperature,
                                                                     VariableGradientEnum::Z>(
          primitive_variable_gradient);
    case ViewVariableEnum::QCriterion: {
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& velocity_gradient =
          VariableGradient<SimulationControl>::template getVectorGradient<PrimitiveVariableEnum::Velocity>(
              primitive_variable_gradient);
      return (velocity_gradient.trace() * velocity_gradient.trace() - (velocity_gradient * velocity_gradient).trace()) *
             0.5_r;
    }
    case ViewVariableEnum::Lambda2:
      if constexpr (SimulationControl::kDimension == 3) {
        // const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& velocity_gradient =
        //     VariableGradient<SimulationControl>::template getVectorGradient<PrimitiveVariableEnum::Velocity>(
        //         primitive_variable_gradient);
        // const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> symmetric_part =
        //     (velocity_gradient + velocity_gradient.transpose()) * 0.5_r;
        // const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> antisymmetric_part =
        //     (velocity_gradient - velocity_gradient.transpose()) * 0.5_r;
        // const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> eigenvalue_matrix =
        //     symmetric_part * symmetric_part + antisymmetric_part.transpose() * antisymmetric_part;
        // Eigen::SelfAdjointEigenSolver<Eigen::Matrix<Real, SimulationControl::kDimension,
        // SimulationControl::kDimension>>
        //     eigen_solver(eigenvalue_matrix, Eigen::EigenvaluesOnly);
        // return eigen_solver.eigenvalues()(1);
      }
    default:
      return 0.0_r;
    }
  }

  Eigen::Vector<Real, SimulationControl::kDimension> getForce(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector, const Isize column) const {
    const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>& computational_variable =
        this->computational_.col(column);
    if constexpr (IsEuler<SimulationControl::kEquationModel>) {
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                 computational_variable) *
             normal_vector;
    }
    if constexpr (IsNS<SimulationControl::kEquationModel>) {
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          primitive_variable_gradient = this->primitive_gradient_.col(column);
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& velocity_gradient =
          VariableGradient<SimulationControl>::template getVectorGradient<PrimitiveVariableEnum::Velocity>(
              primitive_variable_gradient);
      const Real tempurature =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeTemperatureFromInternalEnergy(
              Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                  computational_variable));
      const Real dynamic_viscosity =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeDynamicViscosity(tempurature);
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> viscous_stress =
          dynamic_viscosity * (velocity_gradient + velocity_gradient.transpose()) -
          2.0_r / 3.0_r * dynamic_viscosity * velocity_gradient.trace() *
              Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity();
      return (Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                  computational_variable) *
                  Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity() -
              viscous_stress) *
             normal_vector;
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_VARIABLE_CONVERTOR_CPP_
