/**
 * @file calculate_measure.h
 * @brief The header file to calculate element measure.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CALCULATE_MEASURE_H_
#define SUBROSA_DG_CALCULATE_MEASURE_H_

// clang-format off

#include <Eigen/Core>          // for Dynamic, Matrix, Vector
#include <memory>              // for unique_ptr

#include "basic/data_types.h"  // for Real

// clang-format on

namespace SubrosaDG::Internal {

struct Element;

std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calculateElementMeasure(const Element& element);

Real calculatePolygonArea(const Eigen::Matrix<Real, 3, Eigen::Dynamic>& nodes);

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_CALCULATE_MEASURE_H_
