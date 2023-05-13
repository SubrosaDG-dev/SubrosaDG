/**
 * @file get_integration.h
 * @brief The header file to get element integration.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-12
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_INTEGRATION_H_
#define SUBROSA_DG_GET_INTEGRATION_H_

// clang-format off

#include <vector>              // for vector

#include "basic/data_types.h"  // for Isize

// clang-format on

namespace SubrosaDG::Internal {

struct ElementMesh;
struct ElementIntegral;
struct ElementGradIntegral;

void getElementJacobian(ElementMesh& element_mesh);

std::vector<double> getElementIntegral(Isize polynomial_order, Isize gauss_integral_accuracy,
                                       ElementIntegral& element_integral);

void getElementGradIntegral(Isize polynomial_order, Isize gauss_integral_accuracy,
                            ElementGradIntegral& element_grad_integral);

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_GET_INTEGRATION_H_
