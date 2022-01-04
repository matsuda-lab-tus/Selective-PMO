//
// Created by Hiroki Kojima on 2021/06/23.
//

#ifndef PMO_OPTIMIZER_H_
#define PMO_OPTIMIZER_H_

namespace pmo {

template<typename T, uint_t N>
class Optimizer {
 public:
  explicit Optimizer(const Image_<T> &image,
                     const BasicParameterMap &basic_param_map,
                     ModelParameterMap_<N> &model_param_map,
                     ContextParameterMap_<T> &context_param_map) noexcept
      : m_image(image),
        m_basic_param_map(basic_param_map),
        m_model_param_map(model_param_map),
        m_context_param_map(context_param_map){};

  auto optimize() noexcept -> real_t {
    auto cost = _update_unit_arrange();

    for (auto unit_id = m_model_param_map.num_units(); unit_id-- > 0;) {
      auto unit = m_model_param_map[unit_id];

      _print_optimizing_process(unit_id);

      if (m_model_param_map.num_pix(unit_id) > 0)
        _quasi_newton_method(unit_id);

      const auto indices = unit->quantize();
      unit->restore(indices);

      for (auto unoptimized = 0; unoptimized < unit_id; ++unoptimized)
        m_model_param_map[unoptimized]->params() = unit->params();

      cost = _update_unit_arrange();
    }

    return cost;
  };

 private:
  template<bool G>
  auto _calc_probability(const Point &p,
                         const typename ModelParameterUnit_<N>::Params &model_params) const noexcept -> ValueWithGradient_<N> {
    const auto &basic_params = m_basic_param_map[p]->params();
    const auto &context_param = m_context_param_map[p]->param();

    auto mix_dist = MixtureDistribution_<T, N, G>(basic_params, model_params, context_param);

    return mix_dist.probability(m_image[p]);
  };

  template<bool G>
  auto _calc_penalty(const typename ModelParameterUnit_<N>::Params &model_params) const noexcept -> ValueWithGradient_<N> {
    constexpr auto lambda = real_t{0.1};

    auto cost_with_grad = ValueWithGradient_<N>{};
    auto diff = std::array<real_t, N>{};

    std::transform(model_params.cbegin(),
                   model_params.cend(),
                   ModelParameterUnit_<N>::INI.cbegin(),
                   diff.begin(),
                   std::minus<real_t>());

    cost_with_grad.value += lambda * std::inner_product(diff.cbegin(), diff.cend(), diff.cbegin(), real_t{});

    if constexpr (G == WITH_GRADIENT)
      for (auto i = 0; i < N; ++i)
        cost_with_grad.grad[i] += (real_t{2} * lambda) * diff[i];

    return cost_with_grad;
  };

  template<bool G>
  auto _calc_cost(const uint_t unit_id,
                  const typename ModelParameterUnit_<N>::Params &model_params) const noexcept -> ValueWithGradient_<N> {
    auto cost_with_grad = ValueWithGradient_<N>{};

    for (auto y = 0; y < m_context_param_map.height(); ++y) {
      for (auto x = 0; x < m_context_param_map.width(); ++x) {
        const auto p = Point(x, y);

        if (m_model_param_map.unit_id(p) == unit_id) {

          const auto &prob_with_grad = _calc_probability<G>(p, model_params);

          cost_with_grad.value += -LOG2(prob_with_grad.value);

          if constexpr (G == WITH_GRADIENT)
            for (auto i = 0; i < N; ++i)
              cost_with_grad.grad[i] += -prob_with_grad.grad[i] / prob_with_grad.value;
        }
      }
    }

    if constexpr (G == WITH_GRADIENT)
      for (auto &g : cost_with_grad.grad)
        g *= real_t{1.4426950408889634};// g *= 1 / log(2)

    return cost_with_grad + _calc_penalty<G>(model_params);
  };

  auto _calc_step_size(const uint_t unit_id,
                       const std::array<real_t, N> &search_direction,
                       const ValueWithGradient_<N> &cost_with_grad) const noexcept -> real_t {// Armijo–Goldstein condition
    constexpr auto itr_max = uint_t{100};
    constexpr auto alpha_ini = real_t{0.5};
    constexpr auto c = real_t{0.0001};
    constexpr auto tau = real_t{0.5};

    const auto &params = m_model_param_map[unit_id]->params();
    const auto &cost = cost_with_grad.value;
    const auto &grad = cost_with_grad.grad;

    const auto m = std::inner_product(search_direction.cbegin(),
                                      search_direction.cend(),
                                      grad.cbegin(),
                                      real_t{});//  assumed m > 0
    const auto t = -c * m;

    auto alpha = alpha_ini;
    auto tmp = ModelParameterUnit_<N>(unit_id);

    for (auto itr = 0; itr < itr_max; ++itr) {
      std::transform(search_direction.cbegin(),
                     search_direction.cend(),
                     params.cbegin(),
                     tmp.params().begin(),
                     [&alpha](const auto &si, const auto &param) { return param + alpha * si; });

      const auto tmp_cost = _calc_cost<WITHOUT_GRADIENT>(unit_id, tmp.params()).value;

      if (cost - tmp_cost > alpha * t) break;

      alpha *= tau;
    }

    return alpha;
  };

  auto _update_inverse_hessian(const std::array<real_t, N> &grad,
                               const std::array<real_t, N> &new_grad,
                               const std::array<real_t, N> &s,// search_direction
                               std::array<std::array<real_t, N>, N> &H) const noexcept -> decltype(H) {
    // Davidon–Fletcher–Powell formula
    auto y = std::array<real_t, N>{};
    auto Hy = std::array<real_t, N>{};

    std::transform(new_grad.cbegin(), new_grad.cend(), grad.cbegin(), y.begin(), std::minus<real_t>());

    for (auto i = 0; i < N; ++i)
      Hy[i] = std::inner_product(y.cbegin(), y.cend(), H[i].cbegin(), real_t{});

    const auto ys = std::inner_product(y.cbegin(), y.cend(), s.cbegin(), real_t{});
    const auto inv_ys = real_t{1} / ys;
    const auto yHy = std::inner_product(y.cbegin(), y.cend(), Hy.cbegin(), real_t{});

    // update inverse hessian
    for (auto i = 0; i < N; ++i) {
      for (auto j = i; j < N; ++j) {
        H[i][j] += (ys + yHy) * s[i] * s[j] * inv_ys * inv_ys - (Hy[i] * s[j] + Hy[j] * s[i]) * inv_ys;
        H[j][i] = H[i][j];
      }
    }

    return H;
  };

  void _quasi_newton_method(const uint_t unit_id) noexcept {
    constexpr auto itr_max = uint_t{30};
    constexpr auto epsilon = real_t{N * 1e-6};

    auto &params = m_model_param_map[unit_id]->params();
    auto cost_with_grad = _calc_cost<WITH_GRADIENT>(unit_id, params);
    auto &cost = cost_with_grad.value;
    auto &grad = cost_with_grad.grad;

    auto search_direction = std::array<real_t, N>{};
    auto inverse_hessian = std::array<std::array<real_t, N>, N>{};
    for (auto i = 0; i < N; ++i)
      inverse_hessian[i][i] = real_t{1};// init inverse_hessian;

    for (auto itr = 0; itr < itr_max; ++itr) {// Broyden–Fletcher–Goldfarb–Shanno algorithm

      for (auto i = 0; i < N; ++i)
        search_direction[i] = -std::inner_product(grad.cbegin(),
                                                  grad.cend(),
                                                  inverse_hessian[i].cbegin(),
                                                  real_t{});

      if (itr == 0)
        _print_model_parameter_and_search_direction(cost, params, search_direction);

      {// check convergence
        const auto norm = std::sqrt(std::inner_product(search_direction.cbegin(),
                                                       search_direction.cend(),
                                                       search_direction.cbegin(),
                                                       real_t{}));
        if (norm < epsilon) break;
      }

      const auto step_size = _calc_step_size(unit_id, search_direction, cost_with_grad);

      // update search direction
      std::transform(search_direction.cbegin(),
                     search_direction.cend(),
                     search_direction.begin(),
                     [&step_size](const auto &si) { return step_size * si; });

      // update params
      std::transform(search_direction.cbegin(),
                     search_direction.cend(),
                     params.cbegin(),
                     params.begin(),
                     std::plus<real_t>());

      // update cost and grad
      const auto new_cost_with_grad = _calc_cost<WITH_GRADIENT>(unit_id, params);
      const auto &new_cost = new_cost_with_grad.value;
      const auto &new_grad = new_cost_with_grad.grad;

      {// check convergence
        if (!std::isfinite(new_cost) || cost - new_cost < epsilon) break;

        const auto norm = std::sqrt(std::inner_product(search_direction.cbegin(),
                                                       search_direction.cend(),
                                                       search_direction.cbegin(),
                                                       real_t{}));
        if (norm < epsilon) break;
      }

      inverse_hessian = _update_inverse_hessian(grad, new_grad, search_direction, inverse_hessian);

      // set_unit cost
      cost = new_cost;
      grad = new_grad;

      _print_model_parameter_and_search_direction(cost, params, search_direction);
    }
  };

  auto _update_unit_arrange() -> real_t {
    auto cost = real_t{};

    for (auto y = 0; y < m_context_param_map.height(); ++y) {
      for (auto x = 0; x < m_context_param_map.width(); ++x) {
        const auto p = Point(x, y);

        const auto feature = m_context_param_map.update_feature(p);
        m_model_param_map.set_unit(p, feature);

        const auto &model_params = m_model_param_map[p]->params();
        const auto probability = _calc_probability<WITHOUT_GRADIENT>(p, model_params).value;
        const auto entropy = -LOG2(probability);
        m_context_param_map.update_entropy(p, entropy);

        cost += entropy;
      }
    }

    return cost;
  };

  void _print_optimizing_process(const uint_t &unit_id) const noexcept {
#ifdef PMO_VERBOSE1
    const auto num_pix = static_cast<real_t>(m_model_param_map.num_pix(unit_id));
    const auto num_image = m_model_param_map.height() * m_model_param_map.width();

    printf("#%d, ", static_cast<int>(unit_id));
    printf("pels/image = %.3f", static_cast<real_t>(100 * num_pix) / num_image);

    printf("\n");
#endif
  };

  void _print_model_parameter_and_search_direction(const real_t &cost,
                                                   const typename ModelParameterUnit_<N>::Params &params,
                                                   const std::array<real_t, N> &search_direction) const noexcept {
#ifdef PMO_VERBOSE1
    // print cost
    printf("%.2f, ", cost);

    // print model parameter
    printf("a=(");
    for (const auto &param : params) {
      printf("%6.3f", param);

      if (&param != &params.back())
        printf(", ");
      else
        printf(")");
    }
    printf(", ");

    // print search direction
    printf("d=(");
    for (const auto &d : search_direction) {
      printf("%6.3f", d);

      if (&d != &search_direction.back())
        printf(", ");
      else
        printf(")");
    }
    printf("\n");
#endif
  };

  const Image_<T> &m_image;
  const BasicParameterMap &m_basic_param_map;
  ModelParameterMap_<N> &m_model_param_map;
  ContextParameterMap_<T> &m_context_param_map;
};

}// namespace pmo

#endif//PMO_OPTIMIZER_H_
