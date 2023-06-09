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

// IWYU pragma: begin_exports

#include "basic/concept.hpp"
#include "basic/config.hpp"
#include "basic/constant.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "basic/environment.hpp"
#include "basic/macro.hpp"
#include "cmake.hpp"
#include "integral/cal_basisfun_num.hpp"
#include "integral/get_integral.hpp"
#include "integral/get_integral_num.hpp"
#include "integral/get_standard.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/elem_type.hpp"
#include "mesh/element/cal_measure.hpp"
#include "mesh/element/cal_norm_vec.hpp"
#include "mesh/element/get_adjacency_mesh.hpp"
#include "mesh/element/get_elem_mesh.hpp"
#include "mesh/element/get_jacobian.hpp"
#include "mesh/get_mesh.hpp"
#include "mesh/get_mesh_supplemental.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/convective_flux/cal_roe_flux.hpp"
#include "solver/convective_flux/cal_wall_flux.hpp"
#include "solver/elem_integral/cal_adjacency_integral.hpp"
#include "solver/elem_integral/cal_elem_integral.hpp"
#include "solver/elem_integral/store_to_elem.hpp"
#include "solver/equation/cal_fun_coeff.hpp"
#include "solver/equation/cal_residual.hpp"
#include "solver/init_solver.hpp"
#include "solver/solver_structure.hpp"
#include "solver/time_integral/cal_delta_time.hpp"
#include "solver/time_integral/step_time.hpp"
#include "solver/variable/cal_conserved_var.hpp"
#include "solver/variable/cal_convective_var.hpp"
#include "solver/variable/cal_primitive_var.hpp"
#include "solver/variable/get_parent_var.hpp"

// IWYU pragma: end_exports

#endif  // SUBROSA_DG_HPP_
