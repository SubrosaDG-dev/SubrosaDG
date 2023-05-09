/**
 * @file cal_mesh_measure.h
 * @brief The header file to calculate mesh measure.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_MESH_MEASURE_H_
#define SUBROSA_DG_CAL_MESH_MEASURE_H_

// clang-format off

#include <Eigen/Core>          // for Dynamic, Matrix

#include "basic/data_types.h"  // for Real

// clang-format on

namespace SubrosaDG::Internal {

struct Element;

void calculateMeshMeasure(Element& element);

Real calculatePolygonArea(Eigen::Matrix<Real, 3, Eigen::Dynamic>& nodes);

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_CAL_MESH_MEASURE_H_
