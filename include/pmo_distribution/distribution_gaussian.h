//
// Created by Hiroki Kojima on 2021/06/24.
//

#ifndef PMO_DISTRIBUTION_GAUSSIAN_H_
#define PMO_DISTRIBUTION_GAUSSIAN_H_

// ---------------------------------- Setting Model parameters ---------------------------------- //
constexpr auto NUM_MODEL_PARAMETERS = ;
template<uint_t N>
const std::array<real_t, N> ModelParameter_<N>::m_MIN = {};
template<uint_t N>
const std::array<real_t, N> ModelParameter_<N>::m_INI = {};
template<uint_t N>
const std::array<real_t, N> ModelParameter_<N>::m_MAX = {};
template<uint_t N>
const std::array<uint_t, N> ModelParameter_<N>::m_PRE = {};

// Alias
using ModelParameterMap = ModelParameterMap_<NUM_MODEL_PARAMETERS>;
template<typename T, bool Gradient>
using MixtureDistribution = MixtureDistribution_<T, NUM_MODEL_PARAMETERS, Gradient>;
// ---------------------------------------------------------------------------------------------- //

#endif//PMO_DISTRIBUTION_GAUSSIAN_H_
