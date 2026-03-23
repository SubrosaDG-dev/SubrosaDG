/**
 * @file MatrixDevice.cpp
 * @brief The head file of SubrosaDG device matrix.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2025-12-18
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_MATRIX_DEVICE_CPP_
#define SUBROSA_DG_MATRIX_DEVICE_CPP_

#include <Utils/BasicDataType.cpp>
#include <Utils/Constant.cpp>
#include <sycl/sycl.hpp>
#include <type_traits>

namespace SubrosaDG::Device {

// NOLINTBEGIN(readability-identifier-naming)

[[maybe_unused]] constexpr int Dynamic = -1;

[[maybe_unused]] constexpr bool Transpose = true;
[[maybe_unused]] constexpr bool NoTranspose = false;

template <typename T, typename = void>
struct ScalarOf {
  using Type = T;
};

template <typename T>
struct ScalarOf<T, std::void_t<typename T::Scalar>> {
  using Type = typename T::Scalar;
};

template <typename T>
concept HasScalar = requires { typename T::Scalar; };

template <typename T>
concept IsVector = T::Cols == 1;

template <typename T>
concept IsMatrix = T::Cols != 1;

template <typename Scalar_, int Rows_, int Cols_>
struct StaticMatrix;

template <int Extent>
struct Slice {
  const Isize start_;

  static constexpr Slice all() { return Slice{.start_ = 0}; }

  static constexpr Slice seqN(const Isize start) { return Slice{.start_ = start}; }

  [[nodiscard]] constexpr Isize index(const Isize offset) const { return this->start_ + offset; }
};

template <typename T>
struct View {
  using Scalar = typename T::Scalar;
  static constexpr int Rows = T::Rows;
  static constexpr int Cols = T::Cols;

  Scalar* data_;
  const Isize outer_stride_;
  const Isize inner_stride_;

  View(Scalar* data, const Isize outer_stride, const Isize inner_stride)
      : data_(data), outer_stride_(outer_stride), inner_stride_(inner_stride) {}

  View(const Scalar* data, const Isize outer_stride, const Isize inner_stride)
      : data_(const_cast<Scalar*>(data)), outer_stride_(outer_stride), inner_stride_(inner_stride) {}

  [[nodiscard]] constexpr Isize rows() { return Rows; }

  [[nodiscard]] constexpr Isize cols()
    requires(IsMatrix<T>)
  {
    return Cols;
  }

  Scalar& operator()(const Isize row)
    requires(IsVector<T>)
  {
    return this->data_[row * this->inner_stride_];
  }
  const Scalar& operator()(const Isize row) const
    requires(IsVector<T>)
  {
    return this->data_[row * this->inner_stride_];
  }

  Scalar& operator()(const Isize row, const Isize col)
    requires(IsMatrix<T>)
  {
    return this->data_[row * this->inner_stride_ + col * this->outer_stride_];
  }

  const Scalar& operator()(const Isize row, const Isize col) const
    requires(IsMatrix<T>)
  {
    return this->data_[row * this->inner_stride_ + col * this->outer_stride_];
  }

  void setZero()
    requires(IsVector<T>)
  {
    for (Isize m = 0; m < Rows; m++) {
      (*this)(m) = Scalar(0);
    }
  }

  void setZero()
    requires(IsMatrix<T>)
  {
    for (Isize m = 0; m < Rows; m++) {
      for (Isize n = 0; n < Cols; n++) {
        (*this)(m, n) = Scalar(0);
      }
    }
  }

  template <typename OtherT>
  Scalar dot(const View<OtherT>& other) const
    requires(IsVector<T> && IsVector<OtherT> && (Rows == OtherT::Rows))
  {
    Scalar result = Scalar(0);
    for (Isize m = 0; m < Rows; m++) {
      result += (*this)(m)*other(m);
    }
    return result;
  }

  Scalar squaredNorm() const
    requires(IsVector<T>)
  {
    Scalar result = Scalar(0);
    for (Isize m = 0; m < Rows; m++) {
      result += (*this)(m) * (*this)(m);
    }
    return result;
  }

  template <int NewRows, int NewCols = 1>
  [[nodiscard]] View<StaticMatrix<Scalar, NewRows, NewCols>> reshaped()
    requires(!std::is_const_v<T> && IsVector<T>)
  {
    return View<StaticMatrix<Scalar, NewRows, NewCols>>{this->data_, this->inner_stride_ * NewRows,
                                                        this->inner_stride_};
  }

  template <int NewRows, int NewCols = 1>
  [[nodiscard]] View<const StaticMatrix<Scalar, NewRows, NewCols>> reshaped() const
    requires(std::is_const_v<T> && IsVector<T>)
  {
    return View<const StaticMatrix<Scalar, NewRows, NewCols>>{this->data_, this->inner_stride_ * NewRows,
                                                              this->inner_stride_};
  }
};

template <typename Scalar_, int Rows_>
using StaticVector = StaticMatrix<Scalar_, Rows_, 1>;

template <typename Scalar_, int Rows_, int Cols_>
struct StaticMatrix {
  using Scalar = Scalar_;
  static constexpr int Rows = Rows_;
  static constexpr int Cols = Cols_;

  Scalar data_[Rows_ * Cols_];

  Scalar* data() { return this->data_; }

  const Scalar* data() const { return this->data_; }

  [[nodiscard]] constexpr Isize rows() const { return Rows_; }

  [[nodiscard]] constexpr Isize cols() const
    requires(IsMatrix<StaticMatrix<Scalar_, Rows_, Cols_>>)
  {
    return Cols_;
  }

  Scalar& operator()(const Isize row)
    requires(IsVector<StaticMatrix<Scalar_, Rows_, Cols_>>)
  {
    return this->data_[row];
  }

  const Scalar& operator()(const Isize row) const
    requires(IsVector<StaticMatrix<Scalar_, Rows_, Cols_>>)
  {
    return this->data_[row];
  }

  Scalar& operator()(const Isize row, const Isize col)
    requires(IsMatrix<StaticMatrix<Scalar_, Rows_, Cols_>>)
  {
    return this->data_[row + col * Rows_];
  }

  const Scalar& operator()(const Isize row, const Isize col) const
    requires(IsMatrix<StaticMatrix<Scalar_, Rows_, Cols_>>)
  {
    return this->data_[row + col * Rows_];
  }

  void setZero()
    requires(IsVector<StaticMatrix<Scalar_, Rows_, Cols_>>)
  {
    for (Isize m = 0; m < Rows; m++) {
      (*this)(m) = Scalar(0);
    }
  }

  void setZero()
    requires(IsMatrix<StaticMatrix<Scalar_, Rows_, Cols_>>)
  {
    for (Isize m = 0; m < Rows; m++) {
      for (Isize n = 0; n < Cols; n++) {
        (*this)(m, n) = Scalar(0);
      }
    }
  }

  template <int SliceRows, int SliceCols = 1>
  [[nodiscard]] View<StaticMatrix<Scalar_, SliceRows, SliceCols>> slice(
      const Slice<SliceRows> row_slice = Slice<SliceRows>::all(),
      const Slice<SliceCols> col_slice = Slice<SliceCols>::all()) {
    return View<StaticMatrix<Scalar_, SliceRows, SliceCols>>{this->data_ + row_slice.start_ + col_slice.start_ * Rows_,
                                                             Rows_, 1};
  }

  template <int SliceRows, int SliceCols = 1>
  [[nodiscard]] View<const StaticMatrix<Scalar_, SliceRows, SliceCols>> slice(
      const Slice<SliceRows> row_slice = Slice<SliceRows>::all(),
      const Slice<SliceCols> col_slice = Slice<SliceCols>::all()) const {
    return View<const StaticMatrix<Scalar_, SliceRows, SliceCols>>{
        this->data_ + row_slice.start_ + col_slice.start_ * Rows_, Rows_, 1};
  }
};

template <typename Scalar_, int Rows_, int Cols_>
struct Matrix {
  using Scalar = Scalar_;
  static constexpr int Rows = Rows_;
  static constexpr int Cols = Cols_;

  Scalar* data_;

  Matrix() { this->data_ = sycl::malloc_device<Scalar>(Rows * Cols, queue); }

  // ~Matrix() { sycl::free(this->data_, queue); }

  Scalar* data() { return this->data_; }

  const Scalar* data() const { return this->data_; }

  [[nodiscard]] constexpr Isize rows() const { return Rows; }

  [[nodiscard]] constexpr Isize cols() const
    requires(IsMatrix<Matrix<Scalar_, Rows_, Cols_>>)
  {
    return Cols;
  }

  Scalar& operator()(const Isize row)
    requires(IsVector<Matrix<Scalar_, Rows_, Cols_>>)
  {
    return this->data_[row];
  }

  const Scalar& operator()(const Isize row) const
    requires(IsVector<Matrix<Scalar_, Rows_, Cols_>>)
  {
    return this->data_[row];
  }

  Scalar& operator()(const Isize row, const Isize col)
    requires(IsMatrix<Matrix<Scalar_, Rows_, Cols_>>)
  {
    return this->data_[row + col * Rows];
  }

  const Scalar& operator()(const Isize row, const Isize col) const
    requires(IsMatrix<Matrix<Scalar_, Rows_, Cols_>>)
  {
    return this->data_[row + col * Rows];
  }

  template <int OtherRows, int OtherCols = 1>
  [[nodiscard]] View<Matrix<Scalar_, OtherRows, OtherCols>> slice(
      const Slice<OtherRows> row_slice = Slice<OtherRows>::all(),
      const Slice<OtherCols> col_slice = Slice<OtherCols>::all()) {
    return View<Matrix<Scalar_, OtherRows, OtherCols>>{this->data_ + row_slice.start_ + col_slice.start_ * Rows, Rows,
                                                       1};
  }

  template <int OtherRows, int OtherCols = 1>
  [[nodiscard]] View<const Matrix<Scalar_, OtherRows, OtherCols>> slice(
      const Slice<OtherRows> row_slice = Slice<OtherRows>::all(),
      const Slice<OtherCols> col_slice = Slice<OtherCols>::all()) const {
    return View<const Matrix<Scalar_, OtherRows, OtherCols>>{this->data_ + row_slice.start_ + col_slice.start_ * Rows,
                                                             Rows, 1};
  }
};

template <typename Scalar_, int Rows_>
using Vector = Matrix<Scalar_, Rows_, 1>;

template <typename T, int Rows_, int Cols_>
struct Array {
  using Scalar = typename ScalarOf<T>::Type;

  Scalar* data_;

  // ~Array() { sycl::free(this->data_, queue); }

  void resize(const Isize size)
    requires(!HasScalar<T>)
  {
    this->data_ = sycl::malloc_device<Scalar>(static_cast<Usize>(size), queue);
  }

  void resize(const Isize size)
    requires(HasScalar<T>)
  {
    this->data_ = sycl::malloc_device<Scalar>(static_cast<Usize>(size * T::Rows * T::Cols), queue);
  }

  Scalar* data() { return this->data_; }

  const Scalar* data() const { return this->data_; }

  Scalar& operator()(const Isize index)
    requires(!HasScalar<T>)
  {
    return this->data_[index];
  }

  const Scalar& operator()(const Isize index) const
    requires(!HasScalar<T>)
  {
    return this->data_[index];
  }

  [[nodiscard]] View<T> view(const Isize batch_index, const Isize batch_size)
    requires(HasScalar<T>)
  {
    return View<T>{this->data_ + batch_index, batch_size * T::Rows, batch_size};
  }

  [[nodiscard]] View<const T> view(const Isize batch_index, const Isize batch_size) const
    requires(HasScalar<T>)
  {
    return View<const T>{this->data_ + batch_index, batch_size * T::Rows, batch_size};
  }

  template <int OtherRows, int OtherCols = 1>
  [[nodiscard]] View<Matrix<Scalar, OtherRows, OtherCols>> slice(
      const Isize batch_index, const Isize batch_size, const Slice<OtherRows> row_slice = Slice<OtherRows>::all(),
      const Slice<OtherCols> col_slice = Slice<OtherCols>::all())
    requires(HasScalar<T>)
  {
    return View<Matrix<Scalar, OtherRows, OtherCols>>{
        this->data_ + batch_index + (row_slice.start_ + col_slice.start_ * T::Rows) * batch_size, batch_size * T::Rows,
        batch_size};
  }

  template <int OtherRows, int OtherCols = 1>
  [[nodiscard]] View<const Matrix<Scalar, OtherRows, OtherCols>> slice(
      const Isize batch_index, const Isize batch_size, const Slice<OtherRows> row_slice = Slice<OtherRows>::all(),
      const Slice<OtherCols> col_slice = Slice<OtherCols>::all()) const
    requires(HasScalar<T>)
  {
    return View<const Matrix<Scalar, OtherRows, OtherCols>>{
        this->data_ + batch_index + (row_slice.start_ + col_slice.start_ * T::Rows) * batch_size, batch_size * T::Rows,
        batch_size};
  }
};

// BLAS Level 2

template <typename T, typename U, typename V, bool TransposeA = NoTranspose>
inline void gemv(const T& a, const U& b, V& c) {
  using Scalar = typename T::Scalar;
  constexpr Isize m = TransposeA ? a.cols() : a.rows();
  constexpr Isize n = TransposeA ? a.rows() : a.cols();
  for (Isize i = 0; i < m; i++) {
    Scalar sum = Scalar(0);
    for (Isize j = 0; j < n; j++) {
      const Scalar a_value = TransposeA ? a(j, i) : a(i, j);
      const Scalar b_value = b(j);
      sum += a_value * b_value;
    }
    c(i) = sum;
  }
}

// BLAS Level 3

template <typename T, typename U, typename V, bool TransposeA = NoTranspose, bool TransposeB = NoTranspose>
inline void gemm(const T& a, const U& b, V& c) {
  using Scalar = typename T::Scalar;
  constexpr Isize m = TransposeA ? a.cols() : a.rows();
  constexpr Isize k = TransposeA ? a.rows() : a.cols();
  constexpr Isize n = TransposeB ? b.cols() : b.rows();
  for (Isize i = 0; i < m; i++) {
    for (Isize j = 0; j < n; j++) {
      Scalar sum = Scalar(0);
      for (Isize p = 0; p < k; p++) {
        const Scalar a_value = TransposeA ? a(p, i) : a(i, p);
        const Scalar b_value = TransposeB ? b(j, p) : b(p, j);
        sum += a_value * b_value;
      }
      c(i, j) = sum;
    }
  }
}

// NOLINTEND(readability-identifier-naming)

}  // namespace SubrosaDG::Device

#endif  // SUBROSA_DG_MATRIX_DEVICE_CPP_
