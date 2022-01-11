//
// Created by Hiroki Kojima on 2021/06/10.
//

#ifndef PMO_EXAMPLE_SEARCH_H_
#define PMO_EXAMPLE_SEARCH_H_

namespace pmo {

template<typename T>
class ExampleSearch {
 public:
  explicit ExampleSearch(const uint_t max_num,
                         const Image_<T> &image,
                         const TemplatePatch &template_patch,
                         BasicParameterMap &basic_params_map) noexcept
      : m_max_num(max_num),
        m_image(image),
        m_causal_area(m_image.height(), m_image.width()),
        m_template_patch(template_patch),
        m_basic_param_map(basic_params_map),
        m_template_map(m_basic_param_map.height(), m_basic_param_map.width(), std::vector<real_t>(m_template_patch.size())),
        m_template_mean_map(m_basic_param_map.height(), m_basic_param_map.width()){};

  ExampleSearch(const ExampleSearch &) = delete;

  ExampleSearch &operator=(const ExampleSearch &) = delete;

  void estimate(const Point &p, const int window_size, const real_t penalty) noexcept {
    constexpr auto flag = real_t{0};

    const auto max_num = m_max_num + m_basic_param_map[p]->params().size();
    _calc_template(p);

    m_causal_area.locate(p, window_size).for_each([&](const Point &q) {
      const auto dx = std::abs(p.x - q.x);
      const auto dy = std::abs(p.y - q.y);

      m_basic_param_map[p]->params({_calc_cost(p, q) + penalty * (dx + dy), _calc_peak(p, q), flag}, max_num);
    });
  };

  void update_template(const Point &p) noexcept {
    m_latest_estimated = p;

    if (p.y == 0)
      for (auto x = p.x - (static_cast<int>(m_template_patch.radius()) - 1); x <= p.x; ++x)
        if (x >= 0)
          _calc_template({x, 0});

    if (p.x == 0)
      _calc_template(p);
  };

 private:
  [[nodiscard]] const auto &_template(const Point &p) const noexcept { return m_template_map[p]; };

  auto &_template(const Point &p) noexcept { return m_template_map[p]; };

  [[nodiscard]] const auto &_template_mean(const Point &p) const noexcept { return m_template_mean_map[p]; };

  auto &_template_mean(const Point &p) noexcept { return m_template_mean_map[p]; };

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

  void _calc_template(const Point &p) noexcept {
    auto &f = _template(p);
    auto &m = _template_mean(p);

    const auto &w = m_template_patch.weights();
    const auto &r = m_template_patch.points();

    for (auto i = 0; i < r.size(); ++i)
      f[i] = _template_value(p, p + r[i]);

    m = std::inner_product(f.cbegin(), f.cend(), w.cbegin(), real_t{});
    std::transform(f.cbegin(), f.cend(), f.begin(), [&m](const auto &f_i) { return f_i - m; });
  };

  [[nodiscard]] auto _calc_cost(const Point &p, const Point &q) const noexcept -> real_t {
    const auto &f_p = _template(p);
    const auto &f_q = _template(q);

    const auto &w = m_template_patch.weights();

    auto diff = real_t{};

    for (auto i = 0; i < w.size(); ++i)
      diff += w[i] * (f_q[i] - f_p[i]) * (f_q[i] - f_p[i]);

    return std::sqrt(diff);
  };

  [[nodiscard]] auto _calc_peak(const Point &p, const Point &q) const noexcept -> real_t {
    const auto &m_p = _template_mean(p);
    const auto &m_q = _template_mean(q);

    return static_cast<real_t>(m_image[q]) - m_q + m_p;
  };

  const uint_t m_max_num;
  const Image_<T> &m_image;
  CausalArea m_causal_area;
  const TemplatePatch &m_template_patch;
  BasicParameterMap &m_basic_param_map;
  Matrix_<std::vector<real_t>> m_template_map;
  Matrix_<real_t> m_template_mean_map;
  Point m_latest_estimated{};
};

}// namespace pmo

#endif//PMO_EXAMPLE_SEARCH_H_
