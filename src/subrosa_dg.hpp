/**
 * @file subrosa_dg.hpp
 * @brief The main head file of SubroseDG project.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-01
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_HPP_
#define SUBROSA_DG_HPP_

#include "basic/concept.hpp"                                // IWYU pragma: keep
#include "basic/config.hpp"                                 // IWYU pragma: keep
#include "basic/constant.hpp"                               // IWYU pragma: keep
#include "basic/data_type.hpp"                              // IWYU pragma: keep
#include "basic/enum.hpp"                                   // IWYU pragma: keep
#include "basic/environment.hpp"                            // IWYU pragma: keep
#include "basic/macro.hpp"                                  // IWYU pragma: keep
#include "integral/cal_basisfun_num.hpp"                    // IWYU pragma: keep
#include "integral/get_integral.hpp"                        // IWYU pragma: keep
#include "integral/get_integral_num.hpp"                    // IWYU pragma: keep
#include "integral/get_standard.hpp"                        // IWYU pragma: keep
#include "integral/integral_structure.hpp"                  // IWYU pragma: keep
#include "mesh/elem_type.hpp"                               // IWYU pragma: keep
#include "mesh/element/cal_measure.hpp"                     // IWYU pragma: keep
#include "mesh/element/cal_norm_vec.hpp"                    // IWYU pragma: keep
#include "mesh/element/get_adjacency_mesh.hpp"              // IWYU pragma: keep
#include "mesh/element/get_elem_mesh.hpp"                   // IWYU pragma: keep
#include "mesh/element/get_jacobian.hpp"                    // IWYU pragma: keep
#include "mesh/get_mesh.hpp"                                // IWYU pragma: keep
#include "mesh/get_mesh_supplemental.hpp"                   // IWYU pragma: keep
#include "mesh/mesh_structure.hpp"                          // IWYU pragma: keep
#include "solver/convective_flux/cal_roe_flux.hpp"          // IWYU pragma: keep
#include "solver/convective_flux/cal_wall_flux.hpp"         // IWYU pragma: keep
#include "solver/elem_integral/cal_adjacency_integral.hpp"  // IWYU pragma: keep
#include "solver/elem_integral/cal_elem_integral.hpp"       // IWYU pragma: keep
#include "solver/elem_integral/store_to_elem.hpp"           // IWYU pragma: keep
#include "solver/equation/cal_fun_coeff.hpp"                // IWYU pragma: keep
#include "solver/equation/cal_residual.hpp"                 // IWYU pragma: keep
#include "solver/init_solver.hpp"                           // IWYU pragma: keep
#include "solver/solver_structure.hpp"                      // IWYU pragma: keep
#include "solver/time_integral/cal_delta_time.hpp"          // IWYU pragma: keep
#include "solver/time_integral/step_time.hpp"               // IWYU pragma: keep
#include "solver/variable/cal_conserved_var.hpp"            // IWYU pragma: keep
#include "solver/variable/cal_convective_var.hpp"           // IWYU pragma: keep
#include "solver/variable/cal_primitive_var.hpp"            // IWYU pragma: keep
#include "solver/variable/get_parent_var.hpp"               // IWYU pragma: keep

#endif  // SUBROSA_DG_HPP_
