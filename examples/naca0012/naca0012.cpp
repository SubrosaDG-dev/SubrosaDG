/**
 * @file naca0012.cpp
 * @brief The source file for SubrosaDG example naca0012.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2022 by SubrosaDG developers
 */

#include <iostream>

#include "subrosa_dg.h"

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  std::cout << SubrosaDG::SubrosaDgVersion << std::endl;
  return (EXIT_SUCCESS);
}
