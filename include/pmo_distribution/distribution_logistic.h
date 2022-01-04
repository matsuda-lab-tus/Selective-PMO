//
// Created by Hiroki Kojima on 2021/06/24.
//

#ifndef PMO_DISTRIBUTION_LOGISTIC_H_
#define PMO_DISTRIBUTION_LOGISTIC_H_

namespace pmo {

// ---------------------------------- Setting Model parameters ---------------------------------- //
constexpr auto NUM_MODEL_PARAMETERS = 5;
template<uint_t N>
const std::array<real_t, N> ModelParameterUnit_<N>::MIN = {-10.0, -10.0, -10.0, -10.0, -10.0};
template<uint_t N>
const std::array<real_t, N> ModelParameterUnit_<N>::INI = {-0.5, 0.1, 0.5, 0.0, 0.0};
template<uint_t N>
const std::array<real_t, N> ModelParameterUnit_<N>::MAX = {10.0, 10.0, 10.0, 10.0, 10.0};
template<uint_t N>
const std::array<uint_t, N> ModelParameterUnit_<N>::PRE = {1 << 12, 1 << 12, 1 << 12, 1 << 12, 1 << 12};

// Alias
using ModelParameterMap = ModelParameterMap_<NUM_MODEL_PARAMETERS>;
// ---------------------------------------------------------------------------------------------- //

auto sigmoid(const real_t x) noexcept -> real_t { return x > 0 ? 1 / (1 + EXP(-x)) : EXP(x) / (1 + EXP(x)); }

struct LogisticResult {
  real_t value;
  real_t grad;
};

template<bool G>
auto logistic_(real_t x_lhs, real_t x_rhs, const real_t peak, const real_t precision) noexcept -> LogisticResult {
  x_lhs = x_lhs - peak - real_t{0.5};
  x_rhs = x_rhs - peak + real_t{0.5};
  const auto cdf_lhs = sigmoid(x_lhs * precision);
  const auto cdf_rhs = sigmoid(x_rhs * precision);

  if constexpr (G == WITH_GRADIENT) {
    const auto cdf_lhs_grad = x_lhs * cdf_lhs * (real_t{1.0} - cdf_lhs);
    const auto cdf_rhs_grad = x_rhs * cdf_rhs * (real_t{1.0} - cdf_rhs);

    return {cdf_rhs - cdf_lhs, cdf_rhs_grad - cdf_lhs_grad};
  }

  return {cdf_rhs - cdf_lhs};
}

template<typename T, uint_t N, bool G>
MixtureDistribution_<T, N, G>::MixtureDistribution_(const BasicParameterUnit::Params &basic_params,
                                                    const typename ModelParameterUnit_<N>::Params &model_params,
                                                    const typename ContextParameterUnit::Param &context_param) noexcept
    : m_params(basic_params.size()), m_mix() {
  const auto num = basic_params.size();

  if (num == 0) return;

  const auto a0 = model_params[0];
  const auto a1 = model_params[1];
  const auto a2 = model_params[2];
  const auto a3 = model_params[3];
  const auto a4 = model_params[4];

  auto mix_weights = std::vector<real_t>{};

  {// calc mix_weights
    mix_weights.reserve(num);
    std::transform(basic_params.cbegin(),
                   basic_params.cend(),
                   std::back_inserter(mix_weights),
                   [a2, a4](const auto &param) { return -param.cost * a2 - param.flag * a4; });

    const auto mix_weights_max = *std::max_element(mix_weights.cbegin(), mix_weights.cend());

    std::transform(mix_weights.cbegin(),
                   mix_weights.cend(),
                   mix_weights.begin(),
                   [&mix_weights_max](const auto &w) { return EXP(w - mix_weights_max); });
  }

  const auto mix_weights_sum = std::accumulate(mix_weights.cbegin(), mix_weights.cend(), real_t{});

  auto mix_weights_grad_a2 = std::vector<real_t>{};
  auto mix_weights_grad_a4 = std::vector<real_t>{};
  auto mix_weights_grad_a2_sum = real_t{};
  auto mix_weights_grad_a4_sum = real_t{};

  if constexpr (G == WITH_GRADIENT) {// calc mix_weights_grad_a2
    mix_weights_grad_a2.reserve(num);
    std::transform(basic_params.cbegin(),
                   basic_params.cend(),
                   mix_weights.cbegin(),
                   std::back_inserter(mix_weights_grad_a2),
                   [](const auto &param, const auto &w) { return -param.cost * w; });

    mix_weights_grad_a2_sum = std::accumulate(mix_weights_grad_a2.cbegin(), mix_weights_grad_a2.cend(), real_t{});
  }

  if constexpr (G == WITH_GRADIENT) {// calc mix_weights_grad_a4
    mix_weights_grad_a4.reserve(num);
    std::transform(basic_params.cbegin(),
                   basic_params.cend(),
                   mix_weights.cbegin(),
                   std::back_inserter(mix_weights_grad_a4),
                   [](const auto &param, const auto &w) { return -param.flag * w; });

    mix_weights_grad_a4_sum = std::accumulate(mix_weights_grad_a4.cbegin(), mix_weights_grad_a4.cend(), real_t{});
  }

  for (auto m = 0; m < num; ++m) {
    auto &param = m_params[m];
    const auto peak = basic_params[m].peak;
    const auto cost = basic_params[m].cost;
    const auto flag = basic_params[m].flag;
    const auto mix_weight = mix_weights[m] / mix_weights_sum;
    const auto precision = EXP(a0 - cost * a1 - flag * a3);

    auto sum_ = logistic_<G>(0, m_LEVEL - 1, peak, precision);

    param.peak = peak;
    param.height = mix_weight;
    param.width = precision;
    param.sum_probability = sum_.value;

    if constexpr (G == WITH_GRADIENT) {
      const auto grad_with_a0 = precision;
      const auto grad_with_a1 = -cost * precision;
      const auto grad_with_a2 = (mix_weights_grad_a2[m] - mix_weight * mix_weights_grad_a2_sum) / mix_weights_sum;
      const auto grad_with_a3 = -flag * precision;
      const auto grad_with_a4 = (mix_weights_grad_a4[m] - mix_weight * mix_weights_grad_a4_sum) / mix_weights_sum;

      param.sum_probability_grad = sum_.grad;
      param.grad = {grad_with_a0, grad_with_a1, grad_with_a2, grad_with_a3, grad_with_a4};
    }
  }
}

template<typename T, uint_t N, bool G>
auto MixtureDistribution_<T, N, G>::probability(const uint_t f) noexcept -> ValueWithGradient_<N> {
  const auto num = m_params.size();
  auto &mix = m_mix[f];

  mix = {};

  if (num == 0)
    mix.value += real_t{1} / m_LEVEL;

  for (const auto &param : m_params) {
    const auto sum_probability = std::max(m_EPSILON, param.sum_probability);

    const auto logistic = logistic_<G>(f, f, param.peak, param.width);
    const auto probability = logistic.value / sum_probability;

    mix.value += param.height * probability;

    if constexpr (G == WITH_GRADIENT) {
      const auto probability_grad =
          (logistic.grad - probability * param.sum_probability_grad) / sum_probability;

      mix.grad[0] += param.grad[0] * param.height * probability_grad;
      mix.grad[1] += param.grad[1] * param.height * probability_grad;
      mix.grad[2] += param.grad[2] * probability;
      mix.grad[3] += param.grad[3] * param.height * probability_grad;
      mix.grad[4] += param.grad[4] * probability;
    }
  }

  mix.value = std::max(m_EPSILON, mix.value);

  return mix;
}

}// namespace pmo

#endif//PMO_DISTRIBUTION_LOGISTIC_H_
