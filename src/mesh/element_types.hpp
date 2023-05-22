/**
 * @file element_types.hpp
 * @brief The element type head file to define some aliases.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ELEMENT_TYPES_HPP_
#define SUBROSA_DG_ELEMENT_TYPES_HPP_

// clang-format off

#include "basic/data_types.hpp"  // for Isize

// clang-format on

namespace SubrosaDG {

/**
 * @brief The element type structure, including dimension, type tag and number of nodes in a single element.
 *
 * @tparam Dimension The dimension of the element.
 * @tparam ElementTag The type tag of the element.
 * @tparam NodesNumPerElement The number of nodes in a single element.
 *
 * @details The `ElementType` structure is a template structure, which is used to define the element type.
 */
template <Isize Dimension, int ElementTag, Isize NodesNumPerElement>
struct ElementType {
  /**
   * @brief The dimension of the element.
   *
   * @details The `kDimension` is a static constexpr variable, which is used to define the dimension of the element.
   */
  inline static constexpr Isize kDimension{Dimension};

  /**
   * @brief The type tag of the element.
   *
   * @details The `kElementTag` is a static constexpr variable, which is used to define the type tag of the element. It
   * is same to the type tag of the element in Gmsh, which can be getted by `gmsh::model::mesh::getElementType()`. The
   * type tag of the element is defined in the Gmsh manual.
   */
  inline static constexpr int kElementTag{ElementTag};

  /**
   * @brief The number of nodes in a single element.
   *
   * @details The `kNodesNumPerElement` is a static constexpr variable, which is used to define the number of nodes in a
   * single element.
   */
  inline static constexpr Isize kNodesNumPerElement{NodesNumPerElement};
};

template <ElementType Type, Isize PolynomialOrder>
struct BasisFunction {
  inline static constexpr Isize kNum{0};
};

/**
 * @brief The alias of the line element.
 */
inline constexpr ElementType<1, 1, 2> kLine;

template <Isize PolynomialOrder>
struct BasisFunction<kLine, PolynomialOrder> {
  inline static constexpr Isize kNum{PolynomialOrder + 1};
};

/**
 * @brief The alias of the triangle element.
 */
inline constexpr ElementType<2, 2, 3> kTriangle;

template <Isize PolynomialOrder>
struct BasisFunction<kTriangle, PolynomialOrder> {
  inline static constexpr Isize kNum{(PolynomialOrder + 1) * (PolynomialOrder + 2) / 2};
};

/**
 * @brief The alias of the quadrangle element.
 */
inline constexpr ElementType<2, 3, 4> kQuadrangle;

template <Isize PolynomialOrder>
struct BasisFunction<kQuadrangle, PolynomialOrder> {
  static constexpr Isize kNum{(PolynomialOrder + 1) * (PolynomialOrder + 1)};
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ELEMENT_TYPES_HPP_
