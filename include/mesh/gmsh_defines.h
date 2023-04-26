/**
 * @file gmsh_defines.h
 * @brief The header file for Gmsh defines.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-22
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GMSH_DEFINES_H_
#define SUBROSA_DG_GMSH_DEFINES_H_

// clang-format off

#include <basic/data_types.h>

// clang-format on

namespace SubrosaDG::Internal {

// Element types in .msh file format (numbers should not be changed)
// copy from gmsh/src/common/GmshDefines.h, should be updated when gmsh is updated
// NOLINTBEGIN(readability-identifier-naming)
enum class ElementType : Usize {
  MSH_LIN_2 = 1,
  MSH_TRI_3,
  MSH_QUA_4,
  MSH_TET_4,
  MSH_HEX_8,
  MSH_PRI_6,
  MSH_PYR_5,
  MSH_LIN_3,
  MSH_TRI_6,
  MSH_QUA_9,
  MSH_TET_10,
  MSH_HEX_27,
  MSH_PRI_18,
  MSH_PYR_14,
  MSH_PNT,
  MSH_QUA_8,
  MSH_HEX_20,
  MSH_PRI_15,
  MSH_PYR_13,
  MSH_TRI_9,
  MSH_TRI_10,
  MSH_TRI_12,
  MSH_TRI_15,
  MSH_TRI_15I,
  MSH_TRI_21,
  MSH_LIN_4,
  MSH_LIN_5,
  MSH_LIN_6,
  MSH_TET_20,
  MSH_TET_35,
  MSH_TET_56,
  MSH_TET_22,
  MSH_TET_28,
  MSH_POLYG_,
  MSH_POLYH_,
  MSH_QUA_16,
  MSH_QUA_25,
  MSH_QUA_36,
  MSH_QUA_12,
  MSH_QUA_16I,
  MSH_QUA_20,
  MSH_TRI_28,
  MSH_TRI_36,
  MSH_TRI_45,
  MSH_TRI_55,
  MSH_TRI_66,
  MSH_QUA_49,
  MSH_QUA_64,
  MSH_QUA_81,
  MSH_QUA_100,
  MSH_QUA_121,
  MSH_TRI_18,
  MSH_TRI_21I,
  MSH_TRI_24,
  MSH_TRI_27,
  MSH_TRI_30,
  MSH_QUA_24,
  MSH_QUA_28,
  MSH_QUA_32,
  MSH_QUA_36I,
  MSH_QUA_40,
  MSH_LIN_7,
  MSH_LIN_8,
  MSH_LIN_9,
  MSH_LIN_10,
  MSH_LIN_11,
  MSH_LIN_B,
  MSH_TRI_B,
  MSH_POLYG_B,
  MSH_LIN_C,
  // TETS COMPLETE (6->10)
  MSH_TET_84,
  MSH_TET_120,
  MSH_TET_165,
  MSH_TET_220,
  MSH_TET_286,
  // TETS INCOMPLETE (6->10)
  MSH_TET_34,
  MSH_TET_40,
  MSH_TET_46,
  MSH_TET_52,
  MSH_TET_58,
  //
  MSH_LIN_1,
  MSH_TRI_1,
  MSH_QUA_1,
  MSH_TET_1,
  MSH_HEX_1,
  MSH_PRI_1,
  MSH_PRI_40,
  MSH_PRI_75,
  // HEXES COMPLETE (3->9)
  MSH_HEX_64,
  MSH_HEX_125,
  MSH_HEX_216,
  MSH_HEX_343,
  MSH_HEX_512,
  MSH_HEX_729,
  MSH_HEX_1000,
  // HEXES INCOMPLETE (3->9)
  MSH_HEX_32,
  MSH_HEX_44,
  MSH_HEX_56,
  MSH_HEX_68,
  MSH_HEX_80,
  MSH_HEX_92,
  MSH_HEX_104,
  // PRISMS COMPLETE (5->9)
  MSH_PRI_126,
  MSH_PRI_196,
  MSH_PRI_288,
  MSH_PRI_405,
  MSH_PRI_550,
  // PRISMS INCOMPLETE (3->9)
  MSH_PRI_24,
  MSH_PRI_33,
  MSH_PRI_42,
  MSH_PRI_51,
  MSH_PRI_60,
  MSH_PRI_69,
  MSH_PRI_78,
  // PYRAMIDS COMPLETE (3->9)
  MSH_PYR_30,
  MSH_PYR_55,
  MSH_PYR_91,
  MSH_PYR_140,
  MSH_PYR_204,
  MSH_PYR_285,
  MSH_PYR_385,
  // PYRAMIDS INCOMPLETE (3->9)
  MSH_PYR_21,
  MSH_PYR_29,
  MSH_PYR_37,
  MSH_PYR_45,
  MSH_PYR_53,
  MSH_PYR_61,
  MSH_PYR_69,
  // Additional types
  MSH_PYR_1,
  MSH_PNT_SUB,
  MSH_LIN_SUB,
  MSH_TRI_SUB,
  MSH_TET_SUB,
  MSH_TET_16,
  MSH_TRI_MINI,
  MSH_TET_MINI,
  MSH_TRIH_4,
  MSH_MAX_NUM
};
// NOLINTEND(readability-identifier-naming)

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_GMSH_DEFINES_H_
