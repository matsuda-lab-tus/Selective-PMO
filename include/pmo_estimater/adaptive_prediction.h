//
// Created by Hiroki Kojima on 2021/07/26.
//

#ifndef PMO_ADAPTIVE_PREDICTION_H_
#define PMO_ADAPTIVE_PREDICTION_H_

namespace pmo {

template<typename T>
class Predictor {
 public:
  explicit Predictor(const Image_<T> &image,
                     const TemplatePatch &template_patch,
                     BasicParameterMap &basic_params_map,
                     const bool is_affine = true,
                     const bool is_sampling_train_pix = true,
                     const bool is_variable_train_window = true) noexcept
      : m_image(image),
        m_causal_area(m_image.height(), m_image.width()),
        m_template_patch(template_patch),
        m_basic_param_map(basic_params_map),
        m_coefficients(m_template_patch.size() + is_affine),
        m_coefficients_indices(m_coefficients.size()),
        m_coefficients_matrix(m_coefficients.size(), m_coefficients.size() + 1),
        m_weights(),
        m_weights_indices(),
        m_template_values_buffer_1(m_coefficients.size()),
        m_template_values_buffer_2(m_coefficients.size()),
        m_is_affine(is_affine),
        m_is_sampling_train_pix(is_sampling_train_pix),
        m_is_variable_train_window(is_variable_train_window){};

  void estimate(const Point &p, const uint_t max_num, int window_size) noexcept {
    constexpr auto flag = real_t{1};
    constexpr auto lambda = 1.0 / (10 * 6);
    constexpr auto sample_max = 128;

    if (m_is_variable_train_window)
      window_size = m_coefficients.size();

    m_causal_area.locate(p, window_size);

    _reset_weights(p);

    if (_count_train_pix() > 2) {
      _calc_weights(p);

      if (m_is_sampling_train_pix)
        _sample_train_pix(sample_max);

      _least_squares_method(p, lambda);

      m_basic_param_map[p]->params({_calc_cost(p), _calc_peak(p), flag}, max_num);
    }

    m_latest_estimated = p;
  };

 private:
  auto _template_value(const Point &p, Point r_i) const noexcept {
    const auto out_of_image = !r_i.isin({0, 0}, {m_image.width(), m_image.height()});

    if (out_of_image) {
      r_i.clip_min({0, 0});

      if (p.y == 0 || p.x == 0)
        r_i.clip_max(m_latest_estimated);
      else
        r_i.clip_max({m_image.width() - 1, m_image.height() - 1});
    }

    return static_cast<real_t>(m_image[r_i]);
  };

  auto _template_value(Point r_i) const noexcept {
    constexpr auto constant_value = real_t{};

    const auto out_of_image = !r_i.isin({0, 0}, {m_image.width(), m_image.height()});

    if (out_of_image)
      return constant_value;// constant padding
    else
      return static_cast<real_t>(m_image[r_i]);
  };

  const auto &_calc_template(const Point &p, std::vector<real_t> &template_values) noexcept {
    const auto &r = m_template_patch.points();

    auto i = 0;
    for (; i < r.size(); ++i)
      template_values[i] = _template_value(p + r[i]);

    if (m_is_affine)
      template_values[i] = (sizeof(T) << 8) >> 1;

    return template_values;
  };

  [[nodiscard]] auto _calc_squared_error(const std::vector<real_t> &template_values_1, const std::vector<real_t> &template_values_2) const noexcept {
    constexpr auto min_error = 1.0 / 64;

    const auto &L = m_template_patch.size();

    auto error = real_t{};
    for (auto i = 0; i < L; ++i)
      error += (template_values_1[i] - template_values_2[i]) * (template_values_1[i] - template_values_2[i]);

    return std::max(min_error, error / L);
  };

  void _reset_weights(const Point &p, const real_t constant_value = 1) noexcept {
    m_weights.clear();
    m_weights.resize(m_causal_area.size(), constant_value);
  };

  void _calc_weights(const Point &p) noexcept {
    const auto &f_p = _calc_template(p, m_template_values_buffer_1);

    auto index = int{};

    m_causal_area.for_each([&](const Point &q) {
      const auto &f_q = _calc_template(q, m_template_values_buffer_2);
      auto error = _calc_squared_error(f_p, f_q);

      m_weights[index++] = 1. / error;
    });
  };

  void _reset_sampled() noexcept {
    for (auto &weight : m_weights)
      weight = std::abs(weight);
  };

  auto _count_train_pix() const noexcept {
    return std::count_if(m_weights.cbegin(), m_weights.cend(), [](const auto &w) { return w > 0; });
  };

  void _sample_train_pix(const size_t sample_max) noexcept {
    const auto &num_weights = m_weights.size();

    if (num_weights > sample_max) {
      auto &indices = m_weights_indices;

      indices.clear();
      indices.reserve(m_weights.capacity());
      indices.resize(num_weights);

      std::iota(indices.begin(), indices.end(), 0);
      std::sort(indices.begin(), indices.end(), [this](const auto &left, const auto &right) { return m_weights[left] > m_weights[right]; });

      auto i = int{};

      for (; i < sample_max; ++i)
        m_weights[indices[i]] = std::abs(m_weights[indices[i]]);

      for (; i < num_weights; ++i)
        m_weights[indices[i]] = -std::abs(m_weights[indices[i]]);
    }
  };

  auto _sum_weights() const noexcept {
    auto sum_weights = real_t{};

    for (const auto &weight : m_weights)
      if (weight > 0)
        sum_weights += weight;

    return sum_weights;
  };

  void _reset_matrix() noexcept {
    for (auto &value : m_coefficients_matrix.data())
      value = real_t{};// init matrix
  };

  void _regularize_matrix(const real_t lambda) noexcept {
    const auto &N = static_cast<int>(m_coefficients.size());
    auto &matrix = m_coefficients_matrix;

    for (auto i = 0; i < N; ++i)
      matrix[{i, i}] += lambda;
  };

  void _calc_matrix(const Point &p) noexcept {
    const auto &N = static_cast<int>(m_coefficients.size());

    auto &matrix = m_coefficients_matrix;
    const auto &f_p = _calc_template(p, m_template_values_buffer_1);

    auto index = int{};

    m_causal_area.for_each([&](const Point &q) {
      const auto &weight = m_weights[index++];

      if (weight > 0) {
        const auto &f_q = _calc_template(q, m_template_values_buffer_2);
        const auto &true_value = m_image[q];

        for (auto i = 0; i < N; ++i) {
          const auto weighted = weight * f_q[i];

          for (auto j = i; j < N; ++j)
            matrix[{j, i}] += weighted * f_q[j];

          matrix[{N, i}] += weighted * true_value;
        }
      }
    });

    for (auto i = 0; i < N; ++i)
      for (auto j = i; j < N; ++j)
        matrix[{i, j}] = matrix[{j, i}];
  };

  void _gauss_jordan_method() noexcept {
    constexpr auto val_min = 1e-10;

    const auto N = static_cast<int>(m_coefficients.size());

    auto &matrix = m_coefficients_matrix;
    auto &row = m_coefficients_indices;
    std::iota(row.begin(), row.end(), 0);

    for (auto i = 0; i < N; ++i) {

      auto pivot = -1;

      {// search pivot
        auto val_max = val_min;

        for (auto j = i; j < N; ++j) {
          const auto val = matrix[{i, row[j]}];

          if (std::abs(val) > val_max) {
            pivot = j;
            val_max = std::abs(val);
          }
        }
      }

      if (pivot == -1)
        continue;// there is no pivot

      std::swap(row[i], row[pivot]);// Row switching

      {
        const auto val = matrix[{i, row[i]}];

        for (auto j = i; j < matrix.width(); ++j)
          matrix[{j, row[i]}] /= val;// Row multiplication
      }

      for (auto k = 0; k < N; ++k) {
        const auto val = matrix[{i, row[k]}];

        if (k != i && std::abs(val) > val_min) {

          for (auto j = i; j < matrix.width(); ++j)
            matrix[{j, row[k]}] -= val * matrix[{j, row[i]}];// Row addition
        }
      }
    }

    for (auto k = 0; k < N; ++k)
      m_coefficients[k] = matrix[{N, row[k]}];
  };

  void _least_squares_method(const Point &p, const real_t lambda) noexcept {
    _reset_matrix();
    _regularize_matrix(lambda);
    _calc_matrix(p);
    _gauss_jordan_method();
  };

  [[nodiscard]] auto _predict(const Point &p) noexcept -> real_t {
    const auto &input = _calc_template(p, m_template_values_buffer_2);
    return std::inner_product(m_coefficients.cbegin(), m_coefficients.cend(), input.cbegin(), real_t{});
  };

  [[nodiscard]] auto _calc_residual_error(const Point &p) noexcept {
    const auto &true_value = m_image[p];
    const auto pred_value = _predict(p);

    return (true_value - pred_value) * (true_value - pred_value);
  };

  [[nodiscard]] auto _calc_cost(const Point &p) noexcept {
    auto cost = real_t{};
    auto index = int{};

    m_causal_area.for_each([&](const Point &q) {
      const auto &weight = m_weights[index++];

      if (weight > 0)
        cost += weight * _calc_residual_error(q);
    });

    return std::sqrt(cost / _sum_weights());
  };

  [[nodiscard]] auto _calc_peak(const Point &p) noexcept {
    const auto pred_value = _predict(p);

    return std::min(std::max(real_t{}, pred_value), real_t{(sizeof(T) << 8)});
  };

  const Image_<T> &m_image;
  CausalArea m_causal_area;
  const TemplatePatch &m_template_patch;
  BasicParameterMap &m_basic_param_map;
  std::vector<real_t> m_coefficients;
  std::vector<int> m_coefficients_indices;
  Matrix_<real_t> m_coefficients_matrix;
  std::vector<real_t> m_weights;
  std::vector<int> m_weights_indices;
  std::vector<real_t> m_template_values_buffer_1;
  std::vector<real_t> m_template_values_buffer_2;
  const bool m_is_affine;
  const bool m_is_sampling_train_pix;
  const bool m_is_variable_train_window;
  Point m_latest_estimated{};
};

template<typename T>
class AdaptivePrediction {
 public:
  explicit AdaptivePrediction(const Image_<T> &image,
                              BasicParameterMap &basic_params_map,
                              const bool is_parallel = false) noexcept
      : m_is_parallel(is_parallel),
        m_references{
            TemplatePatch({3.0, 3.0}, PI * 0 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({6.7, 1.3}, PI * 0 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({6.7, 1.3}, PI * 1 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({6.7, 1.3}, PI * 2 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({6.7, 1.3}, PI * 3 / 9, TemplatePatch::ECLIPSE),

            TemplatePatch({6.7, 1.3}, PI * 4 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({6.7, 1.3}, PI * 5 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({6.7, 1.3}, PI * 6 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({6.7, 1.3}, PI * 7 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({6.7, 1.3}, PI * 8 / 9, TemplatePatch::ECLIPSE),

            TemplatePatch({2.5, 2.5}, PI * 0 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({4.9, .99}, PI * 0 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({4.9, .99}, PI * 1 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({4.9, .99}, PI * 2 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({4.9, .99}, PI * 3 / 9, TemplatePatch::ECLIPSE),

            TemplatePatch({4.9, .99}, PI * 4 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({4.9, .99}, PI * 5 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({4.9, .99}, PI * 6 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({4.9, .99}, PI * 7 / 9, TemplatePatch::ECLIPSE),
            TemplatePatch({4.9, .99}, PI * 8 / 9, TemplatePatch::ECLIPSE),

            TemplatePatch({1.5, 1.5}, PI * 0 / 4, TemplatePatch::ECLIPSE),
            TemplatePatch({3.0, .99}, PI * 0 / 4, TemplatePatch::ECLIPSE),
            TemplatePatch({3.0, .99}, PI * 1 / 4, TemplatePatch::ECLIPSE),
            TemplatePatch({3.0, .99}, PI * 2 / 4, TemplatePatch::ECLIPSE),
            TemplatePatch({3.0, .99}, PI * 3 / 4, TemplatePatch::ECLIPSE),
        } {
    constexpr auto is_affine = true;
    constexpr auto is_sampling_train_pix = true;
    constexpr auto is_variable_train_window = true;

    m_predictors.reserve(m_references.size());

    for (auto &reference : m_references)
      m_predictors.emplace_back(Predictor(image, reference, basic_params_map, is_affine, is_sampling_train_pix, is_variable_train_window));
  };

  void estimate(const Point &p, const uint_t max_num, int window_size) noexcept {
    if (m_is_parallel) {
      auto threads = std::vector<std::thread>();
      threads.reserve(m_predictors.size());

      for (auto &predictor : m_predictors)
        threads.emplace_back([&predictor, &p, &max_num, &window_size]() { predictor.estimate(p, max_num, window_size); });

      for (auto &thread : threads)
        thread.join();
    } else {
      for (auto &predictor : m_predictors)
        predictor.estimate(p, max_num, window_size);
    }
  };

 private:
  std::array<TemplatePatch, 25> m_references;
  std::vector<Predictor<T>> m_predictors;
  bool m_is_parallel;
};

}// namespace pmo

#endif//PMO_ADAPTIVE_PREDICTION_H_
