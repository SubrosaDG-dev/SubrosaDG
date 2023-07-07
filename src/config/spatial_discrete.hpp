/**
 * @file spatial_discrete.hpp
 * @brief The head file to define some spatial discrete configs.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-01
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SPATIAL_DISCRETE_HPP_
#define SUBROSA_DG_SPATIAL_DISCRETE_HPP_

#include "basic/enum.hpp"

namespace SubrosaDG {

template <EquModel EquModelT>
struct SpatialDiscrete {};

template <ConvectiveFlux ConvectiveFluxT>
struct SpatialDiscreteEuler : SpatialDiscrete<EquModel::Euler> {
  inline static constexpr ConvectiveFlux kConvectiveFlux{ConvectiveFluxT};
};

template <ConvectiveFlux ConvectiveFluxT, ViscousFlux ViscousFluxT>
struct SpatialDiscreteNS : SpatialDiscrete<EquModel::NS> {
  inline static constexpr ConvectiveFlux kConvectiveFlux{ConvectiveFluxT};
  inline static constexpr ViscousFlux kViscousFlux{ViscousFluxT};
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SPATIAL_DISCRETE_HPP_
