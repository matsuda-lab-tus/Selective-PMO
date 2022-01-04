//
// Created by Hiroki Kojima on 2021/06/23.
//

#ifndef PMO_DISTRIBUTION_H_
#define PMO_DISTRIBUTION_H_

namespace pmo {

template<uint_t N>
struct DistributionParameter_ {
  real_t peak;
  real_t height;// or mixture weight...
  real_t width; // or precision...
  real_t sum_probability;
  real_t sum_probability_grad;
  std::array<real_t, N> grad;
};

template<uint_t N>
struct ValueWithGradient_ {
  auto operator+(const ValueWithGradient_<N> &obj) const noexcept {
    auto result = ValueWithGradient_<N>(obj);

    result.value += value;
    std::transform(grad.cbegin(), grad.cend(), obj.grad.cbegin(), result.grad.begin(), std::plus<real_t>());

    return result;
  };

  real_t value;
  std::array<real_t, N> grad;
};

enum EWithGradientOrNot {
  WITHOUT_GRADIENT = false,
  WITH_GRADIENT = true,
};

template<typename T, uint_t N, bool G>
class MixtureDistribution_ {
 private:
  static constexpr auto m_EPSILON = real_t{1} / (1 << 20);
  static constexpr auto m_LEVEL = uint_t{(sizeof(T) << 8)};

 public:
  using Params = std::vector<DistributionParameter_<N>>;
  using Probabilities = std::array<ValueWithGradient_<N>, m_LEVEL>;

  explicit MixtureDistribution_(const BasicParameterUnit::Params &param,
                                const typename ModelParameterUnit_<N>::Params &model_params,
                                const typename ContextParameterUnit::Param &context_param) noexcept;

  [[nodiscard]] auto probability(uint_t f) noexcept -> ValueWithGradient_<N>;

  [[nodiscard]] auto as_histogram(real_t scale = 1. / m_EPSILON) noexcept -> std::array<uint_t, m_LEVEL> {
    auto dist = std::array<real_t, m_LEVEL>{};
    auto hist = std::array<uint_t, m_LEVEL>{};

    for (auto f = 0; f < m_LEVEL; ++f)
      dist[f] = probability(f).value;

    const auto sum_dist = std::accumulate(dist.cbegin(), dist.cend(), real_t{});
    const auto norm = scale / sum_dist;

    for (auto f = 0; f < m_LEVEL; ++f)
      hist[f] = std::max(uint_t{1}, static_cast<uint_t>(norm * dist[f]));

    return hist;
  };

 private:
  Params m_params;
  Probabilities m_mix;
};

}// namespace pmo

#endif//PMO_DISTRIBUTION_H_
