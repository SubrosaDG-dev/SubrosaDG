/**
 * @file RawBinary.cpp
 * @brief The header file of SubrosaDG raw binary output.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_RAW_BINARY_CPP_
#define SUBROSA_DG_RAW_BINARY_CPP_

#include <zstd.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <sstream>
#include <string>
#include <vector>

#include "Solver/SolveControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Constant.cpp"

namespace SubrosaDG {

struct RawBinaryCompress {
  static void write(const std::filesystem::path& raw_binary_path, std::stringstream& raw_binary_ss) {
    const std::string raw_data = raw_binary_ss.str();
    const std::size_t compressed_bound = ZSTD_compressBound(raw_data.size());
    std::vector<char> compressed(compressed_bound);
    const std::size_t compressed_size =
        ZSTD_compress(compressed.data(), compressed_bound, raw_data.data(), raw_data.size(), 1);
    std::ofstream raw_binary_fout(raw_binary_path, std::ios::binary | std::ios::trunc);
    raw_binary_fout.exceptions(std::ios::failbit | std::ios::badbit);
    raw_binary_fout.write(compressed.data(), static_cast<std::streamsize>(compressed_size));
    raw_binary_fout.close();
  }

  static void read(const std::filesystem::path& raw_binary_path, std::stringstream& raw_binary_ss) {
    std::ifstream raw_binary_fin(raw_binary_path, std::ios::binary | std::ios::ate);
    const std::streamoff compressed_size = raw_binary_fin.tellg();
    raw_binary_fin.seekg(0, std::ios::beg);
    std::vector<char> compressed(static_cast<std::size_t>(compressed_size));
    raw_binary_fin.exceptions(std::ios::failbit | std::ios::badbit);
    raw_binary_fin.read(compressed.data(), compressed_size);
    const unsigned long long original_size = ZSTD_getFrameContentSize(compressed.data(), compressed.size());
    std::string decompressed(original_size, '\0');
    const std::size_t decompressed_size =
        ZSTD_decompress(decompressed.data(), original_size, compressed.data(), compressed.size());
    raw_binary_ss.str("");
    raw_binary_ss.clear();
    raw_binary_ss.write(decompressed.data(), static_cast<std::streamsize>(decompressed_size));
  }
};

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::readVolumeElementRawBinary(
    std::stringstream& raw_binary_ss) {
  for (Isize i = 0; i < this->number_; i++) {
    raw_binary_ss.read(
        reinterpret_cast<char*>(this->variable_basis_function_coefficient_(i).data()),
        SimulationControl::kConservedVariableNumber * VolumeElementTrait::kAllBasisFunctionNumber * kRealSize);
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::readRawBinary(const std::filesystem::path& raw_binary_path, Real& time) {
  RawBinaryCompress::read(raw_binary_path, this->raw_binary_ss_);
  this->raw_binary_ss_.read(reinterpret_cast<char*>(&time), kRealSize);
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.readVolumeElementRawBinary(this->raw_binary_ss_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.readVolumeElementRawBinary(this->raw_binary_ss_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.readVolumeElementRawBinary(this->raw_binary_ss_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.readVolumeElementRawBinary(this->raw_binary_ss_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.readVolumeElementRawBinary(this->raw_binary_ss_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.readVolumeElementRawBinary(this->raw_binary_ss_);
    }
  }
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::writeVolumeElementRawBinary(
    std::stringstream& raw_binary_ss) const {
  for (Isize i = 0; i < this->number_; i++) {
    raw_binary_ss.write(
        reinterpret_cast<const char*>(this->variable_basis_function_coefficient_(i).data()),
        SimulationControl::kConservedVariableNumber * VolumeElementTrait::kAllBasisFunctionNumber * kRealSize);
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::writeRawBinary(const std::filesystem::path& raw_binary_path, const Real time) {
  this->write_raw_binary_future_.get();
  this->raw_binary_ss_.clear();
  // The mesh and polynomial order fix the payload size for every frame in this solve.
  this->raw_binary_ss_.seekp(0, std::ios::beg);
  this->raw_binary_ss_.write(reinterpret_cast<const char*>(&time), kRealSize);
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.writeVolumeElementRawBinary(this->raw_binary_ss_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.writeVolumeElementRawBinary(this->raw_binary_ss_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.writeVolumeElementRawBinary(this->raw_binary_ss_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.writeVolumeElementRawBinary(this->raw_binary_ss_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.writeVolumeElementRawBinary(this->raw_binary_ss_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.writeVolumeElementRawBinary(this->raw_binary_ss_);
    }
  }
  this->write_raw_binary_future_ =
      std::async(std::launch::async, RawBinaryCompress::write, raw_binary_path, std::ref(this->raw_binary_ss_));
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_RAW_BINARY_CPP_
