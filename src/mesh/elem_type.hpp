/**
 * @file elem_type.hpp
 * @brief The element type head file to define some aliases.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ELEM_TYPE_HPP_
#define SUBROSA_DG_ELEM_TYPE_HPP_

// clang-format off

#include "basic/data_type.hpp"  // for Isize

// clang-format on

namespace SubrosaDG {

/**
 * @brief The element type structure, including dimension, type tag and number of nodes in a single element.
 *
 * @tparam Dim The dimension of the element.
 * @tparam ElemTag The type tag of the element.
 * @tparam NodeNumPerElem The number of nodes in a single element.
 *
 * @details The `ElemInfo` structure is a template structure, which is used to define the element type.
 */
template <Isize Dim, int ElemTag, Isize NodeNumPerElem, Isize AdjacencyElemNum>
struct ElemInfo {
  /**
   * @brief The dimension of the element.
   *
   * @details The `kDim` is a static constexpr variable, which is used to define the dimension of the element.
   */
  inline static constexpr Isize kDim{Dim};

  /**
   * @brief The type tag of the element.
   *
   * @details The `kElemInfoag` is a static constexpr variable, which is used to define the type tag of the element. It
   * is same to the type tag of the element in Gmsh, which can be getted by `gmsh::model::mesh::getElemInfo()`. The
   * type tag of the element is defined in the Gmsh manual.
   */
  inline static constexpr int kTag{ElemTag};

  /**
   * @brief The number of nodes in a single element.
   *
   * @details The `kNodeNum` is a static constexpr variable, which is used to define the number of nodes in a
   * single element.
   */
  inline static constexpr Isize kNodeNum{NodeNumPerElem};

  inline static constexpr Isize kAdjacencyNum{AdjacencyElemNum};
};

/**
 * @brief The alias of the line element.
 */
inline constexpr ElemInfo<1, 1, 2, 2> kLine;

/**
 * @brief The alias of the triangle element.
 */
inline constexpr ElemInfo<2, 2, 3, 3> kTri;

/**
 * @brief The alias of the quadrangle element.
 */
inline constexpr ElemInfo<2, 3, 4, 4> kQuad;

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ELEM_TYPE_HPP_
