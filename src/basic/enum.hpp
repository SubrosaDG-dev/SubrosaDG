/**
 * @file enum.hpp
 * @brief The enum definitions head file to define some enums.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ENUM_HPP_
#define SUBROSA_DG_ENUM_HPP_

namespace SubrosaDG {

enum class EquModel {
  Euler = 1,
  NS,
};

enum class ConvectiveFlux {
  HLLC = 1,
  Roe,
};

enum class ViscousFlux {
  BR1 = 1,
  BR2,
};

enum class Boundary {
  Farfield = 1,
  Wall,
};

enum class MeshType {
  Tri = 1,
  Quad,
  TriQuad,
  Tet,
  Pyramid,
  Hex,
  TetPyramidHex,
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ENUM_HPP_
