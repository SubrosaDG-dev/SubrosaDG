/**
 * @file get_view.hpp
 * @brief The head file to define get view function.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-03
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_VIEW_HPP_
#define SUBROSA_DG_GET_VIEW_HPP_

#include <filesystem>
#include <fstream>

#include "basic/constant.hpp"
#include "basic/enum.hpp"
#include "config/thermo_model.hpp"
#include "config/time_var.hpp"
#include "config/view_config.hpp"
#include "mesh/mesh_structure.hpp"
#include "view/get_fout.hpp"
#include "view/init_view.hpp"
#include "view/reader/read_raw_buffer.hpp"
#include "view/variable/cal_nodal_var.hpp"
#include "view/view_structure.hpp"
#include "view/writer/write_ascii_tec.hpp"

namespace SubrosaDG {

template <int Dim, PolyOrder P, MeshType MeshT, EquModel EquModelT, TimeDiscrete TimeDiscreteT>
inline void getView(const Mesh<Dim, P, MeshT>& mesh, const ThermoModel<EquModelT>& thermo_model,
                    const TimeVar<TimeDiscreteT>& time_var, const ViewConfig& view_config,
                    View<Dim, P, MeshT, EquModelT>& view) {
  std::ifstream fin;
  std::ofstream fout;
  fout.setf(std::ios::left, std::ios::adjustfield);
  fout.setf(std::ios::scientific, std::ios::floatfield);
  fout.precision(kSignificantDigits);
  fin.open((view_config.dir_ / "cache.raw").string(), std::ios::in | std::ios::binary);
  makeDir(view_config);
  initView(mesh, view);
  for (int i = 1; i <= time_var.iter_; i++) {
    if (i % view_config.write_interval_ == 0) {
      readRawBuffer(mesh, view, fin);
      getFout(i, view_config, fout);
      calNodalVar(mesh, thermo_model, view);
      switch (view_config.type_) {
      case ViewType::Dat:
        writeAsciiTec(i, mesh, view, fout);
        break;
      case ViewType::Plt:
        break;
      }
      fout.close();
    }
  }
  fin.close();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_VIEW_HPP_
