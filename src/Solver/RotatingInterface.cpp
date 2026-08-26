/**
 * @file RotatingInterface.cpp
 * @brief The header file of rotating interface.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2026-05-29
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ROTATING_INTERFACE_CPP_
#define SUBROSA_DG_ROTATING_INTERFACE_CPP_

#include <Eigen/Core>

#include "Mesh/ReadControl.cpp"
#include "Solver/SolveControl.cpp"
#include "Solver/TimeIntegration.cpp"

namespace SubrosaDG {

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::rotateVolumeElementMesh(
    VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
    const Eigen::Vector<Real, SimulationControl::kDimension>& rotation_center,
    [[maybe_unused]] const Eigen::Vector<Real, 3>& rotation_axis, const Real rotation_angular_velocity,
    const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& rotation_matrix) {
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, this->rotate_number_), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Isize element_index = i + this->static_number_;
          for (Isize j = 0; j < VolumeElementTrait::kAllNodeNumber; j++) {
            volume_element_mesh.node_coordinate_(element_index).col(j) =
                rotation_center +
                rotation_matrix *
                    (volume_element_mesh.node_coordinate_initial_(element_index).col(j) - rotation_center);
            if constexpr (SimulationControl::kDimension == 2) {
              this->variable_rotation_velocity_coefficient_(element_index)(0, j) =
                  -rotation_angular_velocity *
                  (volume_element_mesh.node_coordinate_(element_index)(1, j) - rotation_center(1));
              this->variable_rotation_velocity_coefficient_(element_index)(1, j) =
                  rotation_angular_velocity *
                  (volume_element_mesh.node_coordinate_(element_index)(0, j) - rotation_center(0));
            } else if constexpr (SimulationControl::kDimension == 3) {
              this->variable_rotation_velocity_coefficient_(element_index).col(j) =
                  rotation_angular_velocity *
                  rotation_axis.cross(volume_element_mesh.node_coordinate_(element_index).col(j) - rotation_center);
            }
          }
        }
      });
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>::rotateVolumeElementMesh(
    VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
    const Device::Vector<Real, SimulationControl::kDimension>& rotation_center,
    [[maybe_unused]] const Device::Vector<Real, 3>& rotation_axis, const Real rotation_angular_velocity,
    const Device::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& rotation_matrix) {
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->rotate_number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->rotate_number_) {
        return;
      }
      const Isize element_index = i + this->static_number_;
      auto& volume_element_mesh_mutable = const_cast<VolumeElementMeshDevice<VolumeElementTrait>&>(volume_element_mesh);
      const Device::View<const Device::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kAllNodeNumber>>
          node_coordinate_initial = volume_element_mesh.node_coordinate_initial_.view(element_index, this->number_);
      Device::View<Device::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kAllNodeNumber>>
          node_coordinate = volume_element_mesh_mutable.node_coordinate_.view(element_index, this->number_);
      Device::View<Device::Matrix<Real, SimulationControl::kDimension, VolumeElementTrait::kAllNodeNumber>>
          variable_rotation_velocity_coefficient =
              this->variable_rotation_velocity_coefficient_.view(element_index, this->number_);
      for (Isize j = 0; j < VolumeElementTrait::kAllNodeNumber; j++) {
        for (Isize m = 0; m < SimulationControl::kDimension; m++) {
          Real sum = 0.0_r;
          for (Isize n = 0; n < SimulationControl::kDimension; n++) {
            sum += rotation_matrix(m, n) * (node_coordinate_initial(n, j) - rotation_center(n));
          }
          node_coordinate(m, j) = rotation_center(m) + sum;
        }
        if constexpr (SimulationControl::kDimension == 2) {
          variable_rotation_velocity_coefficient(0, j) =
              -rotation_angular_velocity * (node_coordinate(1, j) - rotation_center(1));
          variable_rotation_velocity_coefficient(1, j) =
              rotation_angular_velocity * (node_coordinate(0, j) - rotation_center(0));
        } else if constexpr (SimulationControl::kDimension == 3) {
          variable_rotation_velocity_coefficient(0, j) =
              rotation_angular_velocity * (rotation_axis(1) * (node_coordinate(2, j) - rotation_center(2)) -
                                           rotation_axis(2) * (node_coordinate(1, j) - rotation_center(1)));
          variable_rotation_velocity_coefficient(1, j) =
              rotation_angular_velocity * (rotation_axis(2) * (node_coordinate(0, j) - rotation_center(0)) -
                                           rotation_axis(0) * (node_coordinate(2, j) - rotation_center(2)));
          variable_rotation_velocity_coefficient(2, j) =
              rotation_angular_velocity * (rotation_axis(0) * (node_coordinate(1, j) - rotation_center(1)) -
                                           rotation_axis(1) * (node_coordinate(0, j) - rotation_center(0)));
        }
      }
    });
  });
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::rotateAdjacencyElementMesh(
    AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const Eigen::Vector<Real, SimulationControl::kDimension>& rotation_center,
    const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& rotation_matrix) {
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, this->rotate_number_), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          Isize element_index;
          if (i < this->interior_rotate_number_) {
            element_index = i + this->interior_static_number_;
          } else if (i < this->interior_rotate_number_ + this->boundary_rotate_number_) {
            element_index = i + this->interior_static_number_ + this->boundary_static_number_;
          } else {
            element_index = i + this->interior_static_number_ + this->boundary_static_number_ + this->interface_number_;
          }
          for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
            adjacency_element_mesh.node_coordinate_(element_index).col(j) =
                rotation_center +
                rotation_matrix *
                    (adjacency_element_mesh.node_coordinate_initial_(element_index).col(j) - rotation_center);
          }
        }
      });
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::rotateAdjacencyElementMesh(
    AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh,
    const Device::Vector<Real, SimulationControl::kDimension>& rotation_center,
    const Device::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& rotation_matrix) {
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->rotate_number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->rotate_number_) {
        return;
      }
      Isize element_index;
      if (i < this->interior_rotate_number_) {
        element_index = i + this->interior_static_number_;
      } else if (i < this->interior_rotate_number_ + this->boundary_rotate_number_) {
        element_index = i + this->interior_static_number_ + this->boundary_static_number_;
      } else {
        element_index = i + this->interior_static_number_ + this->boundary_static_number_ + this->interface_number_;
      }
      auto& adjacency_element_mesh_mutable =
          const_cast<AdjacencyElementMeshDevice<AdjacencyElementTrait>&>(adjacency_element_mesh);
      const Device::View<
          const Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>>
          node_coordinate_initial = adjacency_element_mesh.node_coordinate_initial_.view(element_index, this->number_);
      Device::View<Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>>
          node_coordinate = adjacency_element_mesh_mutable.node_coordinate_.view(element_index, this->number_);
      for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
        for (Isize m = 0; m < SimulationControl::kDimension; m++) {
          Real sum = 0.0_r;
          for (Isize n = 0; n < SimulationControl::kDimension; n++) {
            sum += rotation_matrix(m, n) * (node_coordinate_initial(n, j) - rotation_center(n));
          }
          node_coordinate(m, j) = rotation_center(m) + sum;
        }
      }
    });
  });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::rotateMesh(Mesh<SimulationControl>& mesh,
                                                  const TimeIntegration<SimulationControl>& time_integration,
                                                  const int rk_step) {
  const Real angle = (static_cast<Real>(time_integration.iteration_) +
                      TimeIntegration<SimulationControl>::kButcherCoefficients[static_cast<Usize>(rk_step)]) *
                     time_integration.delta_time_ * this->rotation_angular_velocity_;
  if constexpr (SimulationControl::kDimension == 2) {
    Transformation::Rotation::getMatrix(angle, this->rotation_matrix_);
  } else if constexpr (SimulationControl::kDimension == 3) {
    Transformation::AngleAxis::getMatrix(this->rotation_axis_, angle, this->rotation_matrix_);
  }
  if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.rotateVolumeElementMesh(mesh.triangle_, this->rotation_center_, this->rotation_axis_,
                                              this->rotation_angular_velocity_, this->rotation_matrix_);
      mesh.triangle_.computeVolumeElementJacobian(false);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.rotateVolumeElementMesh(mesh.quadrangle_, this->rotation_center_, this->rotation_axis_,
                                                this->rotation_angular_velocity_, this->rotation_matrix_);
      mesh.quadrangle_.computeVolumeElementJacobian(false);
    }
    this->line_.rotateAdjacencyElementMesh(mesh.line_, this->rotation_center_, this->rotation_matrix_);
    mesh.line_.computeAdjacencyElementOtherNodeCoordinate(false);
    mesh.line_.computeAdjacencyElementNormalVector(false);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.rotateVolumeElementMesh(mesh.tetrahedron_, this->rotation_center_, this->rotation_axis_,
                                                 this->rotation_angular_velocity_, this->rotation_matrix_);
      mesh.tetrahedron_.computeVolumeElementJacobian(false);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.rotateVolumeElementMesh(mesh.pyramid_, this->rotation_center_, this->rotation_axis_,
                                             this->rotation_angular_velocity_, this->rotation_matrix_);
      mesh.pyramid_.computeVolumeElementJacobian(false);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.rotateVolumeElementMesh(mesh.hexahedron_, this->rotation_center_, this->rotation_axis_,
                                                this->rotation_angular_velocity_, this->rotation_matrix_);
      mesh.hexahedron_.computeVolumeElementJacobian(false);
    }
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.rotateAdjacencyElementMesh(mesh.triangle_, this->rotation_center_, this->rotation_matrix_);
      mesh.triangle_.computeAdjacencyElementOtherNodeCoordinate(false);
      mesh.triangle_.computeAdjacencyElementNormalVector(false);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.rotateAdjacencyElementMesh(mesh.quadrangle_, this->rotation_center_, this->rotation_matrix_);
      mesh.quadrangle_.computeAdjacencyElementOtherNodeCoordinate(false);
      mesh.quadrangle_.computeAdjacencyElementNormalVector(false);
    }
  }
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::rotateMesh(MeshDevice<SimulationControl>& mesh,
                                                        const TimeIntegration<SimulationControl>& time_integration,
                                                        const int rk_step) {
  queue
      .submit([&](sycl::handler& cgh) -> void {
        cgh.parallel_for(getNdRange(1), [=, this](sycl::nd_item<1> index) -> void {
          const auto i = static_cast<Isize>(index.get_global_id(0));
          if (i >= 1) {
            return;
          }
          const Real angle = (static_cast<Real>(time_integration.iteration_) +
                              TimeIntegration<SimulationControl>::kButcherCoefficients[static_cast<Usize>(rk_step)]) *
                             time_integration.delta_time_ * this->rotation_angular_velocity_;
          if constexpr (SimulationControl::kDimension == 2) {
            Transformation::RotationDevice::getMatrix(angle, this->rotation_matrix_);
          } else if constexpr (SimulationControl::kDimension == 3) {
            Transformation::AngleAxisDevice::getMatrix(this->rotation_axis_, angle, this->rotation_matrix_);
          }
        });
      })
      .wait();
  if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.rotateVolumeElementMesh(mesh.triangle_, this->rotation_center_, this->rotation_axis_,
                                              this->rotation_angular_velocity_, this->rotation_matrix_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.rotateVolumeElementMesh(mesh.quadrangle_, this->rotation_center_, this->rotation_axis_,
                                                this->rotation_angular_velocity_, this->rotation_matrix_);
    }
    this->line_.rotateAdjacencyElementMesh(mesh.line_, this->rotation_center_, this->rotation_matrix_);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.rotateVolumeElementMesh(mesh.tetrahedron_, this->rotation_center_, this->rotation_axis_,
                                                 this->rotation_angular_velocity_, this->rotation_matrix_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.rotateVolumeElementMesh(mesh.pyramid_, this->rotation_center_, this->rotation_axis_,
                                             this->rotation_angular_velocity_, this->rotation_matrix_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.rotateVolumeElementMesh(mesh.hexahedron_, this->rotation_center_, this->rotation_axis_,
                                                this->rotation_angular_velocity_, this->rotation_matrix_);
    }
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.rotateAdjacencyElementMesh(mesh.triangle_, this->rotation_center_, this->rotation_matrix_);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.rotateAdjacencyElementMesh(mesh.quadrangle_, this->rotation_center_, this->rotation_matrix_);
    }
  }
  queue.wait();
  if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      mesh.triangle_.computeVolumeElementJacobian(false);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      mesh.quadrangle_.computeVolumeElementJacobian(false);
    }
    mesh.line_.computeAdjacencyElementOtherNodeCoordinate(false);
    mesh.line_.computeAdjacencyElementNormalVector(false);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      mesh.tetrahedron_.computeVolumeElementJacobian(false);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      mesh.pyramid_.computeVolumeElementJacobian(false);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      mesh.hexahedron_.computeVolumeElementJacobian(false);
    }
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      mesh.triangle_.computeAdjacencyElementOtherNodeCoordinate(false);
      mesh.triangle_.computeAdjacencyElementNormalVector(false);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      mesh.quadrangle_.computeAdjacencyElementOtherNodeCoordinate(false);
      mesh.quadrangle_.computeAdjacencyElementNormalVector(false);
    }
  }
  queue.wait();
}

template <typename AdjacencyElementTrait, typename SimulationControl>
template <typename VolumeElementTrait>
inline void
AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::computeAdjacencyPerElementInterfaceVariable(
    const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver,
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate,
    Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>> interface_conserved_variable,
    const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent) {
  Eigen::Vector<Real, VolumeElementTrait::kDimension> volume_local_coordinate;
  adjacency_element_mesh.template getAdjacencyElementParentVolumeLocalCoordinate<VolumeElementTrait>(
      volume_element_mesh.lagrange_basic_node_coordinate_, adjacency_local_coordinate, volume_local_coordinate,
      adjacency_sequence_in_parent);
  Eigen::Vector<Real, VolumeElementTrait::kAllBasisFunctionNumber> volume_basis_function_value;
  volume_element_mesh.computeLagrangeValue(volume_local_coordinate, volume_basis_function_value);
  interface_conserved_variable.noalias() =
      volume_element_solver.variable_basis_function_coefficient_(parent_index_each_type) * volume_basis_function_value;
}

template <typename AdjacencyElementTrait, typename SimulationControl>
template <typename VolumeElementTrait, ViscousFluxEnum ViscousFluxType>
inline void
AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::computeAdjacencyPerElementInterfaceVariable(
    const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver,
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate,
    Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>> interface_conserved_variable,
    Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
        interface_conserved_variable_gradient,
    const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent) {
  Eigen::Vector<Real, VolumeElementTrait::kDimension> volume_local_coordinate;
  adjacency_element_mesh.template getAdjacencyElementParentVolumeLocalCoordinate<VolumeElementTrait>(
      volume_element_mesh.lagrange_basic_node_coordinate_, adjacency_local_coordinate, volume_local_coordinate,
      adjacency_sequence_in_parent);
  Eigen::Vector<Real, VolumeElementTrait::kAllBasisFunctionNumber> volume_basis_function_value;
  volume_element_mesh.computeLagrangeValue(volume_local_coordinate, volume_basis_function_value);
  interface_conserved_variable.noalias() =
      volume_element_solver.variable_basis_function_coefficient_(parent_index_each_type) * volume_basis_function_value;
  if constexpr (ViscousFluxType == ViscousFluxEnum::None) {
    interface_conserved_variable_gradient.noalias() =
        volume_element_solver.variable_volume_gradient_basis_function_coefficient_(parent_index_each_type) *
        volume_basis_function_value;
  } else if constexpr (ViscousFluxType == ViscousFluxEnum::BR1) {
    interface_conserved_variable_gradient.noalias() =
        volume_element_solver.variable_gradient_basis_function_coefficient_(parent_index_each_type) *
        volume_basis_function_value;
  } else if constexpr (ViscousFluxType == ViscousFluxEnum::BR2) {
    const Eigen::Ref<
        const Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                            VolumeElementTrait::kAllBasisFunctionNumber>>
        variable_interface_gradient_basis_function_coefficient =
            volume_element_solver.variable_interface_gradient_basis_function_coefficient_(parent_index_each_type)(
                Eigen::placeholders::all,
                Eigen::seqN(adjacency_sequence_in_parent * VolumeElementTrait::kAllBasisFunctionNumber,
                            Eigen::fix<VolumeElementTrait::kAllBasisFunctionNumber>));
    interface_conserved_variable_gradient.noalias() =
        (volume_element_solver.variable_volume_gradient_basis_function_coefficient_(parent_index_each_type) +
         variable_interface_gradient_basis_function_coefficient) *
        volume_basis_function_value;
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
template <typename VolumeElementTrait>
inline void
AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::computeAdjacencyPerElementInterfaceVariable(
    const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
    const AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh,
    const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>& volume_element_solver,
    const Device::StaticVector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate,
    Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>> interface_conserved_variable,
    const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent) {
  Device::StaticVector<Real, VolumeElementTrait::kDimension> volume_local_coordinate;
  adjacency_element_mesh.template getAdjacencyElementParentVolumeLocalCoordinate<VolumeElementTrait>(
      volume_element_mesh.lagrange_basic_node_coordinate_, adjacency_local_coordinate, volume_local_coordinate,
      adjacency_sequence_in_parent);
  Device::StaticVector<Real, VolumeElementTrait::kAllBasisFunctionNumber> volume_basis_function_value;
  volume_element_mesh.computeLagrangeValue(volume_local_coordinate, volume_basis_function_value);
  const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                          VolumeElementTrait::kAllBasisFunctionNumber>>
      variable_basis_function_coefficient = volume_element_solver.variable_basis_function_coefficient_.view(
          parent_index_each_type, volume_element_solver.number_);
  for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
    Real sum = 0.0_r;
    for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
      sum += variable_basis_function_coefficient(m, n) * volume_basis_function_value(n);
    }
    interface_conserved_variable(m) = sum;
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
template <typename VolumeElementTrait, ViscousFluxEnum ViscousFluxType>
inline void
AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::computeAdjacencyPerElementInterfaceVariable(
    const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
    const AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh,
    const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>& volume_element_solver,
    const Device::StaticVector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate,
    Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>> interface_conserved_variable,
    Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
        interface_conserved_variable_gradient,
    const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent) {
  Device::StaticVector<Real, VolumeElementTrait::kDimension> volume_local_coordinate;
  adjacency_element_mesh.template getAdjacencyElementParentVolumeLocalCoordinate<VolumeElementTrait>(
      volume_element_mesh.lagrange_basic_node_coordinate_, adjacency_local_coordinate, volume_local_coordinate,
      adjacency_sequence_in_parent);
  Device::StaticVector<Real, VolumeElementTrait::kAllBasisFunctionNumber> volume_basis_function_value;
  volume_element_mesh.computeLagrangeValue(volume_local_coordinate, volume_basis_function_value);
  const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                          VolumeElementTrait::kAllBasisFunctionNumber>>
      variable_basis_function_coefficient = volume_element_solver.variable_basis_function_coefficient_.view(
          parent_index_each_type, volume_element_solver.number_);
  for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
    Real sum = 0.0_r;
    for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
      sum += variable_basis_function_coefficient(m, n) * volume_basis_function_value(n);
    }
    interface_conserved_variable(m) = sum;
  }
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
        sum += variable_volume_gradient_basis_function_coefficient(m, n) * volume_basis_function_value(n);
      }
      interface_conserved_variable_gradient(m) = sum;
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
        sum += variable_gradient_basis_function_coefficient(m, n) * volume_basis_function_value(n);
      }
      interface_conserved_variable_gradient(m) = sum;
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
               volume_basis_function_value(n);
      }
      interface_conserved_variable_gradient(m) = sum;
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::updateAdjacencyElementInterfaceVariable(
    const Mesh<SimulationControl>& mesh, const Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(Mesh<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>());
  auto compute = [&]<typename VolumeElementTrait>(
                     const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
                     const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver,
                     const Eigen::Vector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate,
                     const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent, const Isize i,
                     const Isize j) -> void {
    if constexpr (IsEuler<SimulationControl::kEquationModel>) {
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>> interface_conserved_variable =
          this->interface_right_conserved_variable_(i).col(j);
      this->template computeAdjacencyPerElementInterfaceVariable<VolumeElementTrait>(
          volume_element_mesh, adjacency_element_mesh, volume_element_solver, adjacency_local_coordinate,
          interface_conserved_variable, parent_index_each_type, adjacency_sequence_in_parent);
    }
    if constexpr (IsNS<SimulationControl::kEquationModel>) {
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>> interface_conserved_variable =
          this->interface_right_conserved_variable_(i).col(j);
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          interface_conserved_variable_gradient = this->interface_right_conserved_variable_gradient_(i).col(j);
      this->template computeAdjacencyPerElementInterfaceVariable<VolumeElementTrait, SimulationControl::kViscousFlux>(
          volume_element_mesh, adjacency_element_mesh, volume_element_solver, adjacency_local_coordinate,
          interface_conserved_variable, interface_conserved_variable_gradient, parent_index_each_type,
          adjacency_sequence_in_parent);
    }
  };
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, 2 * this->interface_number_), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Isize element_index = i + this->interior_number_ + this->boundary_number_;
          const BoundaryConditionEnum boundary_condition_type =
              adjacency_element_mesh.boundary_condition_type_(element_index);
          Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1> adjacency_quadrature_node_coordinate;
          Eigen::Vector<Real, AdjacencyElementTrait::kDimension> adjacency_local_coordinate;
          for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
            adjacency_quadrature_node_coordinate =
                adjacency_element_mesh.quadrature_node_coordinate_(element_index).col(j);
            const Isize interface_element_index = adjacency_element_mesh.getAdjacencyElementIndexFromGlobalCoordinate(
                adjacency_quadrature_node_coordinate, boundary_condition_type);
            const Isize interface_parent_index_each_type =
                adjacency_element_mesh.left_parent_index_each_type_(interface_element_index);
            const Isize adjacency_sequence_in_interface_parent =
                adjacency_element_mesh.adjacency_sequence_in_left_parent_(interface_element_index);
            const Isize interface_parent_gmsh_type_number =
                adjacency_element_mesh.left_parent_gmsh_type_number_(interface_element_index);
            adjacency_element_mesh.computeAdjacencyElementLocalCoordinateFromGlobalCoordinate(
                adjacency_quadrature_node_coordinate, adjacency_local_coordinate, interface_element_index);
            if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
              if (interface_parent_gmsh_type_number ==
                  VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
                compute.template operator()<VolumeTriangleTrait<SimulationControl::kPolynomialOrder>>(
                    mesh.triangle_, solver.triangle_, adjacency_local_coordinate, interface_parent_index_each_type,
                    adjacency_sequence_in_interface_parent, i, j);
              } else if (interface_parent_gmsh_type_number ==
                         VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
                compute.template operator()<VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>>(
                    mesh.quadrangle_, solver.quadrangle_, adjacency_local_coordinate, interface_parent_index_each_type,
                    adjacency_sequence_in_interface_parent, i, j);
              }
            } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
              if (interface_parent_gmsh_type_number ==
                  VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
                compute.template operator()<VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
                    mesh.pyramid_, solver.pyramid_, adjacency_local_coordinate, interface_parent_index_each_type,
                    adjacency_sequence_in_interface_parent, i, j);
              } else if (interface_parent_gmsh_type_number ==
                         VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
                compute.template operator()<VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>>(
                    mesh.hexahedron_, solver.hexahedron_, adjacency_local_coordinate, interface_parent_index_each_type,
                    adjacency_sequence_in_interface_parent, i, j);
              }
            }
          }
        }
      });
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::updateAdjacencyElementInterfaceVariable(
    const MeshDevice<SimulationControl>& mesh, const SolverDevice<SimulationControl>& solver) {
  const AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(MeshDevice<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>());
  auto compute = [=, this]<typename VolumeElementTrait>(
                     const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
                     const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>& volume_element_solver,
                     const Device::StaticVector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate,
                     const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent, const Isize i,
                     const Isize j) -> void {
    if constexpr (IsEuler<SimulationControl::kEquationModel>) {
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>> interface_conserved_variable =
          this->interface_right_conserved_variable_.slice(
              i, 2 * this->interface_number_, Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
              Device::Slice<1>::seqN(j));
      this->template computeAdjacencyPerElementInterfaceVariable<VolumeElementTrait>(
          volume_element_mesh, adjacency_element_mesh, volume_element_solver, adjacency_local_coordinate,
          interface_conserved_variable, parent_index_each_type, adjacency_sequence_in_parent);
    } else if constexpr (IsNS<SimulationControl::kEquationModel>) {
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>> interface_conserved_variable =
          this->interface_right_conserved_variable_.slice(
              i, 2 * this->interface_number_, Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
              Device::Slice<1>::seqN(j));
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          interface_conserved_variable_gradient = this->interface_right_conserved_variable_gradient_.slice(
              i, 2 * this->interface_number_,
              Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
              Device::Slice<1>::seqN(j));
      this->template computeAdjacencyPerElementInterfaceVariable<VolumeElementTrait, SimulationControl::kViscousFlux>(
          volume_element_mesh, adjacency_element_mesh, volume_element_solver, adjacency_local_coordinate,
          interface_conserved_variable, interface_conserved_variable_gradient, parent_index_each_type,
          adjacency_sequence_in_parent);
    }
  };
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(2 * this->interface_number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= 2 * this->interface_number_) {
        return;
      }
      const Isize element_index = i + this->interior_number_ + this->boundary_number_;
      const BoundaryConditionEnum boundary_condition_type =
          adjacency_element_mesh.boundary_condition_type_(element_index);
      Device::StaticVector<Real, AdjacencyElementTrait::kDimension + 1> adjacency_quadrature_node_coordinate;
      Device::StaticVector<Real, AdjacencyElementTrait::kDimension> adjacency_local_coordinate;
      const Device::View<
          const Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>>
          quadrature_node_coordinate =
              adjacency_element_mesh.quadrature_node_coordinate_.view(element_index, this->number_);
      for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
        for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
          adjacency_quadrature_node_coordinate(m) = quadrature_node_coordinate(m, j);
        }
        const Isize interface_element_index = adjacency_element_mesh.getAdjacencyElementIndexFromGlobalCoordinate(
            adjacency_quadrature_node_coordinate, boundary_condition_type);
        const Isize interface_parent_index_each_type =
            adjacency_element_mesh.left_parent_index_each_type_(interface_element_index);
        const Isize adjacency_sequence_in_interface_parent =
            adjacency_element_mesh.adjacency_sequence_in_left_parent_(interface_element_index);
        const Isize interface_parent_gmsh_type_number =
            adjacency_element_mesh.left_parent_gmsh_type_number_(interface_element_index);
        adjacency_element_mesh.computeAdjacencyElementLocalCoordinateFromGlobalCoordinate(
            adjacency_quadrature_node_coordinate, adjacency_local_coordinate, interface_element_index);
        if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
          if (interface_parent_gmsh_type_number ==
              VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
            compute.template operator()<VolumeTriangleTrait<SimulationControl::kPolynomialOrder>>(
                mesh.triangle_, solver.triangle_, adjacency_local_coordinate, interface_parent_index_each_type,
                adjacency_sequence_in_interface_parent, i, j);
          } else if (interface_parent_gmsh_type_number ==
                     VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
            compute.template operator()<VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>>(
                mesh.quadrangle_, solver.quadrangle_, adjacency_local_coordinate, interface_parent_index_each_type,
                adjacency_sequence_in_interface_parent, i, j);
          }
        } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
          if (interface_parent_gmsh_type_number ==
              VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
            compute.template operator()<VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
                mesh.pyramid_, solver.pyramid_, adjacency_local_coordinate, interface_parent_index_each_type,
                adjacency_sequence_in_interface_parent, i, j);
          } else if (interface_parent_gmsh_type_number ==
                     VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
            compute.template operator()<VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>>(
                mesh.hexahedron_, solver.hexahedron_, adjacency_local_coordinate, interface_parent_index_each_type,
                adjacency_sequence_in_interface_parent, i, j);
          }
        }
      }
    });
  });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::updateInterfaceVariable(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 2) {
    this->line_.updateAdjacencyElementInterfaceVariable(mesh, *this);
  } else if constexpr (SimulationControl::kDimension == 3) {
    this->quadrangle_.updateAdjacencyElementInterfaceVariable(mesh, *this);
  }
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::updateInterfaceVariable(const MeshDevice<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 2) {
    this->line_.updateAdjacencyElementInterfaceVariable(mesh, *this);
  } else if constexpr (SimulationControl::kDimension == 3) {
    this->quadrangle_.updateAdjacencyElementInterfaceVariable(mesh, *this);
  }
  queue.wait();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ROTATING_INTERFACE_CPP_
