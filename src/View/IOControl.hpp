/**
 * @file IOControl.hpp
 * @brief The header file of IOControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_IO_CONTROL_HPP_
#define SUBROSA_DG_IO_CONTROL_HPP_

#include <Eigen/Core>
#include <filesystem>
#include <fstream>
#include <magic_enum.hpp>
#include <string>
#include <vector>
#include <vtu11-cpp17.hpp>

#include "Mesh/BasisFunction.hpp"
#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename AdjacencyElementTrait, PolynomialOrderEnum P>
struct AdjacencyElementViewBasisFunction;

template <PolynomialOrderEnum P>
struct AdjacencyElementViewBasisFunction<AdjacencyPointTrait<P>, P> {
  [[nodiscard]] inline Isize getAdjacencyParentElementViewBasisFunctionSequenceInParent(
      [[maybe_unused]] Isize parent_gmsh_type_number, Isize adjacency_sequence_in_parent,
      [[maybe_unused]] Isize node_sequence_in_adjacency) const {
    return adjacency_sequence_in_parent;
  }
};

template <PolynomialOrderEnum P>
struct AdjacencyElementViewBasisFunction<AdjacencyLineTrait<P>, P> {
  [[nodiscard]] inline Isize getAdjacencyParentElementViewBasisFunctionSequenceInParent(
      Isize parent_gmsh_type_number, Isize adjacency_sequence_in_parent, Isize node_sequence_in_adjacency) const {
    if (parent_gmsh_type_number == TriangleTrait<P>::kGmshTypeNumber) {
      if (adjacency_sequence_in_parent == TriangleTrait<P>::kAdjacencyNumber - 1 && node_sequence_in_adjacency == 1) {
        return 0;
      }
      if (node_sequence_in_adjacency < AdjacencyLineTrait<P>::kBasicNodeNumber) {
        return adjacency_sequence_in_parent + node_sequence_in_adjacency;
      }
      return TriangleTrait<P>::kBasicNodeNumber +
             adjacency_sequence_in_parent *
                 (AdjacencyLineTrait<P>::kAllNodeNumber - AdjacencyLineTrait<P>::kBasicNodeNumber) +
             node_sequence_in_adjacency - AdjacencyLineTrait<P>::kBasicNodeNumber;
    }
    if (parent_gmsh_type_number == QuadrangleTrait<P>::kGmshTypeNumber) {
      if (adjacency_sequence_in_parent == QuadrangleTrait<P>::kAdjacencyNumber - 1 && node_sequence_in_adjacency == 1) {
        return 0;
      }
      if (node_sequence_in_adjacency < AdjacencyLineTrait<P>::kBasicNodeNumber) {
        return adjacency_sequence_in_parent + node_sequence_in_adjacency;
      }
      return QuadrangleTrait<P>::kBasicNodeNumber +
             adjacency_sequence_in_parent *
                 (AdjacencyLineTrait<P>::kAllNodeNumber - AdjacencyLineTrait<P>::kBasicNodeNumber) +
             node_sequence_in_adjacency - AdjacencyLineTrait<P>::kBasicNodeNumber;
    }
    return -1;
  }
};

template <typename ElementTrait>
struct ElementViewBasisFunction {
  Eigen::Matrix<Real, ElementTrait::kBasisFunctionNumber, ElementTrait::kAllNodeNumber> basis_function_value_;

  inline ElementViewBasisFunction() {
    Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kAllNodeNumber> all_node_coordinate{
        getElementNodeCoordinate<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>().data()};
    Eigen::Matrix<double, 3, ElementTrait::kAllNodeNumber> local_coord_gmsh_matrix;
    local_coord_gmsh_matrix.setZero();
    local_coord_gmsh_matrix(Eigen::seqN(Eigen::fix<0>, Eigen::fix<ElementTrait::kDimension>), Eigen::all) =
        all_node_coordinate.template cast<double>();
    std::vector<double> local_coord(local_coord_gmsh_matrix.data(),
                                    local_coord_gmsh_matrix.data() + local_coord_gmsh_matrix.size());
    std::vector<double> basis_functions = getElementBasisFunction<ElementTrait>(local_coord);
    for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kBasisFunctionNumber; j++) {
        this->basis_function_value_(j, i) =
            static_cast<Real>(basis_functions[static_cast<Usize>(i * ElementTrait::kBasisFunctionNumber + j)]);
      }
    }
  }
};

template <typename AdjacencyElementTrait>
struct AdjacencyElementViewVariable
    : AdjacencyElementViewBasisFunction<AdjacencyElementTrait, AdjacencyElementTrait::kPolynomialOrder> {};

template <typename ElementTrait, typename SimulationControl>
struct ElementViewVariable : ElementViewBasisFunction<ElementTrait> {
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>,
               Eigen::Dynamic, 1>
      conserved_variable_;

  inline void readElementRawBinary(const ElementMesh<ElementTrait>& element_mesh, std::fstream& raw_binary_finout);

  inline ElementViewVariable() : ElementViewBasisFunction<ElementTrait>(){};
};

template <typename SimulationControl, int Dimension>
struct ViewVariableData;

template <typename SimulationControl>
struct ViewVariableData<SimulationControl, 1> {
  AdjacencyElementViewVariable<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>> point_;
  ElementViewVariable<LineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> line_;

  Eigen::Vector<Real, Eigen::Dynamic> time_value_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, Eigen::Dynamic> node_conserved_variable_;
};

template <typename SimulationControl>
struct ViewVariableData<SimulationControl, 2> {
  AdjacencyElementViewVariable<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>> line_;
  ElementViewVariable<TriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> triangle_;
  ElementViewVariable<QuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> quadrangle_;

  Eigen::Vector<Real, Eigen::Dynamic> time_value_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, Eigen::Dynamic> node_conserved_variable_;
};

template <typename SimulationControl>
struct ViewVariable : ViewVariableData<SimulationControl, SimulationControl::kDimension> {
  template <typename ElementTrait>
  inline static ElementViewVariable<ElementTrait, SimulationControl> ViewVariable::*getElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (ElementTrait::kElementType == ElementEnum::Line) {
        return &ViewVariable::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (ElementTrait::kElementType == ElementEnum::Triangle) {
        return &ViewVariable::triangle_;
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &ViewVariable::quadrangle_;
      }
    }
    return nullptr;
  }

  template <typename AdjacencyElementTrait>
  inline static AdjacencyElementViewVariable<AdjacencyElementTrait> ViewVariable::*getAdjacencyElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
        return &ViewVariable::point_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
        return &ViewVariable::line_;
      }
    }
    return nullptr;
  }

  inline void readRawBinary(const Mesh<SimulationControl>& mesh, std::fstream& raw_binary_finout);

  inline void readTimeValue(int iteration_number, std::fstream& error_finout);

  inline void initializeViewVariable(int iteration_number, const Mesh<SimulationControl>& mesh);
};

template <typename SimulationControl>
struct ViewData {
  int io_interval_;
  int iteration_order_;
  std::filesystem::path output_directory_;
  std::string output_file_name_prefix_;
  std::fstream error_finout_;
  std::fstream raw_binary_finout_;
  ViewConfigEnum config_enum_;
  std::vector<ViewVariableEnum> variable_vector_;
  ViewVariable<SimulationControl> variable_;
};

template <typename SimulationControl, ViewModelEnum ViewModelType>
struct ViewBase;

template <typename SimulationControl>
struct ViewBase<SimulationControl, ViewModelEnum::Dat> : ViewData<SimulationControl> {
  inline void setViewFout(int step, std::ofstream& fout);

  inline void writeAsciiVariableList(std::ofstream& fout);

  template <int Dimension>
  inline void writeAsciiHeader(Real time_value, std::string_view physical_name, Isize node_number, Isize element_number,
                               std::ofstream& fout);

  template <typename AdjacencyElementTrait>
  inline void writeAdjacencyElement(
      Isize physical_index, const MeshInformation& mesh_information,
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
      const ThermalModel<SimulationControl>& thermal_model,
      Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
      Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
      Eigen::Matrix<Isize, AdjacencyElementTrait::kTecplotBasicNodeNumber, Eigen::Dynamic>& element_connectivity,
      Isize element_index, Isize& column);

  template <typename ElementTrait>
  inline void writeElement(
      Isize physical_index, const MeshInformation& mesh_information, const ElementMesh<ElementTrait>& element_mesh,
      const ThermalModel<SimulationControl>& thermal_model,
      Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
      Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
      Eigen::Matrix<Isize, ElementTrait::kTecplotBasicNodeNumber, Eigen::Dynamic>& element_connectivity,
      Isize element_index, Isize& column);

  template <int Dimension, bool IsAdjacency>
  inline void writeField(
      Isize physical_index, const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      Eigen::Matrix<Real, SimulationControl::kDimension, Eigen::Dynamic>& node_coordinate,
      Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>& node_variable,
      Eigen::Matrix<Isize, getElementTecplotBasicNodeNumber<Dimension>(), Eigen::Dynamic>& element_connectivity);

  template <int Dimension, bool IsAdjacency>
  inline void writeView(int step, Isize physical_index, const Mesh<SimulationControl>& mesh,
                        const ThermalModel<SimulationControl>& thermal_model, std::ofstream& fout);

  inline void stepView(int step, const Mesh<SimulationControl>& mesh,
                       const ThermalModel<SimulationControl>& thermal_model);
};

template <typename SimulationControl>
struct ViewBase<SimulationControl, ViewModelEnum::Vtu> : ViewData<SimulationControl> {
  inline std::string getBaseName(int step, std::string_view physical_name);

  inline void getDataSetInfomatoin(std::vector<vtu11::DataSetInfo>& data_set_information);

  inline void calculateViewVariable(const ThermalModel<SimulationControl>& thermal_model,
                                    Variable<SimulationControl>& variable,
                                    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
                                    Isize column);

  template <typename AdjacencyElementTrait>
  inline void writeAdjacencyElement(Isize physical_index, const MeshInformation& mesh_information,
                                    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
                                    const ThermalModel<SimulationControl>& thermal_model,
                                    Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
                                    Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
                                    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
                                    Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
                                    Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type,
                                    Isize element_index, Isize& column);

  template <typename ElementTrait>
  inline void writeElement(Isize physical_index, const MeshInformation& mesh_information,
                           const ElementMesh<ElementTrait>& element_mesh,
                           const ThermalModel<SimulationControl>& thermal_model,
                           Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
                           Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
                           Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
                           Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
                           Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type, Isize element_index,
                           Isize& column);

  template <int Dimension, bool IsAdjacency>
  inline void writeField(Isize physical_index, const Mesh<SimulationControl>& mesh,
                         const ThermalModel<SimulationControl>& thermal_model,
                         Eigen::Matrix<Real, 3, Eigen::Dynamic>& node_coordinate,
                         Eigen::Array<Eigen::Vector<Real, Eigen::Dynamic>, Eigen::Dynamic, 1>& node_variable,
                         Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_connectivity,
                         Eigen::Vector<vtu11::VtkIndexType, Eigen::Dynamic>& element_offset,
                         Eigen::Vector<vtu11::VtkCellType, Eigen::Dynamic>& element_type);

  template <int Dimension, bool IsAdjacency>
  inline void writeView(int step, Isize physical_index, const Mesh<SimulationControl>& mesh,
                        const ThermalModel<SimulationControl>& thermal_model, const std::string& base_name);

  inline void stepView(int step, const Mesh<SimulationControl>& mesh,
                       const ThermalModel<SimulationControl>& thermal_model);
};

template <typename SimulationControl>
struct View : ViewBase<SimulationControl, SimulationControl::kViewModel> {
  inline void initializeViewRawBinary(const ViewConfigEnum view_config) {
    std::ios::openmode open_mode = std::ios::in | std::ios::out;
    if ((view_config & ViewConfigEnum::DoNotTruncate) == ViewConfigEnum::Default) {
      open_mode |= std::ios::trunc;
    }
#ifndef SUBROSA_DG_DEVELOP
    open_mode |= std::ios::binary;
#endif
    this->raw_binary_finout_.open((this->output_directory_ / "raw.binary").string(), open_mode);
    this->raw_binary_finout_.setf(std::ios::left, std::ios::adjustfield);
    this->raw_binary_finout_.setf(std::ios::scientific, std::ios::floatfield);
    this->error_finout_.open((this->output_directory_ / "error.txt").string(),
                             std::ios::in | std::ios::out | std::ios::trunc);
    this->error_finout_.setf(std::ios::left, std::ios::adjustfield);
    this->error_finout_.setf(std::ios::scientific, std::ios::floatfield);
  }

  inline void initializeView(const bool delete_dir, const int iteration_number, const Mesh<SimulationControl>& mesh) {
    std::filesystem::path view_output_directory;
    if constexpr (SimulationControl::kViewModel == ViewModelEnum::Dat) {
      view_output_directory = this->output_directory_ / "dat";
    } else if constexpr (SimulationControl::kViewModel == ViewModelEnum::Msh) {
      view_output_directory = this->output_directory_ / "msh";
    } else if constexpr (SimulationControl::kViewModel == ViewModelEnum::Plt) {
      view_output_directory = this->output_directory_ / "plt";
    } else if constexpr (SimulationControl::kViewModel == ViewModelEnum::Vtu) {
      view_output_directory = this->output_directory_ / "vtu";
    }
    if (delete_dir) {
      if (std::filesystem::exists(view_output_directory)) {
        std::filesystem::remove_all(view_output_directory);
      }
      std::filesystem::create_directories(view_output_directory);
    } else if (!std::filesystem::exists(view_output_directory)) {
      std::filesystem::create_directories(view_output_directory);
    }
    this->raw_binary_finout_.seekg(0, std::ios::beg);
    this->error_finout_.seekg(0, std::ios::beg);
    this->variable_.initializeViewVariable(iteration_number, mesh);
    this->variable_.readTimeValue(iteration_number, this->error_finout_);
  }

  inline void finalizeView() {
    this->error_finout_.close();
    this->raw_binary_finout_.close();
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_IO_CONTROL_HPP_
