/**
 * @file get_output_var_num.hpp
 * @brief The head file to get the number of output variables.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-08-12
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_OUTPUT_VAR_NUM_HPP_
#define SUBROSA_DG_GET_OUTPUT_VAR_NUM_HPP_

#include "basic/enum.hpp"

namespace SubrosaDG {

template <EquModel EquModelT>
inline consteval int getOutputVarNum(int dim);

template <>
inline consteval int getOutputVarNum<EquModel::Euler>(const int dim) {
  return dim + 3;
}

template <>
inline consteval int getOutputVarNum<EquModel::NS>(const int dim) {
  return dim + 3;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_OUTPUT_VAR_NUM_HPP_
