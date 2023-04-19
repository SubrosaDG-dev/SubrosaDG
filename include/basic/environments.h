/**
 * @file environments.h
 * @brief The header file for SubrosaDG environments.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-13
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ENVIRONMENTS_H_
#define SUBROSA_DG_ENVIRONMENTS_H_

#include <gmsh.h>

#include <string>
#include <vector>

namespace SubrosaDG {

namespace Internal {

void printEnvironmentInfo();

std::vector<std::string> getGmshInfo();

#ifdef SUBROSA_DG_WITH_OPENMP
void setMaxThreads();

int getMaxThreads();
#endif

}  // namespace Internal

class EnvironmentGardian {
 public:
  EnvironmentGardian() { initializeEnvironment(); }
  ~EnvironmentGardian() { finalizeEnvironment(); }

 private:
  void static initializeEnvironment() {
    gmsh::initialize();
    Internal::printEnvironmentInfo();
#ifdef SUBROSA_DG_WITH_OPENMP
    Internal::setMaxThreads();
#endif
  }
  void static finalizeEnvironment() { gmsh::finalize(); }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ENVIRONMENTS_H_
