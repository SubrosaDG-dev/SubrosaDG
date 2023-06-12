/**
 * @file cal_projection_measure.hpp
 * @brief The calculate projection measure header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_PROJECTION_MEASURE_HPP_
#define SUBROSA_DG_CAL_PROJECTION_MEASURE_HPP_

#include <Eigen/Core>

#include "basic/data_type.hpp"
#include "mesh/get_elem_info.hpp"

namespace SubrosaDG {

template <int Dim, ElemType ElemT>
struct ElemMesh;
enum class ElemType;
enum class ElemType;

template <ElemType ElemT>
inline void calProjectionMeasure(const Eigen::Matrix<Real, 2, getNodeNum<ElemT>()>& node,
                                 Eigen::Vector<Real, 2>& projection_measure) {
  projection_measure = node.rowwise().maxCoeff() - node.rowwise().minCoeff();
}

template <int Dim, ElemType ElemT>
inline void calElemProjectionMeasure(ElemMesh<Dim, ElemT>& elem_mesh) {
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    calProjectionMeasure<ElemT>(elem_mesh.elem_(i).node_, elem_mesh.elem_(i).projection_measure_);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_PROJECTION_MEASURE_HPP_
