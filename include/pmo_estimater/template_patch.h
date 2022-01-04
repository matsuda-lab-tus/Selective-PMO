//
// Created by Hiroki Kojima on 2021/06/10.
//

#ifndef PMO_TEMPLATE_PATCH_H_
#define PMO_TEMPLATE_PATCH_H_

namespace pmo {

class TemplatePatch {
 public:
  enum ETemplateShape {
    DIAMOND,
    ECLIPSE,
  };

  using Values = std::vector<real_t>;
  using Points = std::vector<Point>;

  explicit TemplatePatch(const real_t radius,
                         const real_t radian = 0,
                         const ETemplateShape shape = DIAMOND,
                         const real_t weight_sd = 1.25) noexcept {
    reset(radius, radian, shape, weight_sd);
  };

  explicit TemplatePatch(const Point_<real_t> radius,
                         const real_t radian = 0,
                         const ETemplateShape shape = DIAMOND,
                         const real_t weight_sd = 1.25) noexcept {
    reset(radius, radian, shape, weight_sd);
  };

  explicit TemplatePatch() noexcept : m_size(0), m_points(), m_weights(){};

  TemplatePatch(const TemplatePatch &) = delete;

  TemplatePatch &operator=(const TemplatePatch &) = delete;

  void reset(const real_t radius,
             const real_t radian = 0,
             const ETemplateShape shape = DIAMOND,
             const real_t weight_sd = 1.25) noexcept {
    reset({radius, radius}, radian, shape, weight_sd);
  };

  void reset(const Point_<real_t> radius,
             const real_t radian = 0,
             const ETemplateShape shape = DIAMOND,
             const real_t weight_sd = 1.25) noexcept {
    m_points.clear();
    m_weights.clear();
    m_points.shrink_to_fit();
    m_weights.shrink_to_fit();

    m_radius = radius;
    m_radian = radian;

    switch (shape) {
      case ETemplateShape::DIAMOND:
        _arrange_diamond(weight_sd);
        break;
      case ETemplateShape::ECLIPSE:
        _arrange_ellipse(weight_sd);
        break;
    }
  };

  [[nodiscard]] auto size() const noexcept { return m_size; };

  [[nodiscard]] auto radius() const noexcept { return int(std::max(m_radius.x, m_radius.y)); };

  [[nodiscard]] const auto &points() const noexcept { return m_points; };

  [[nodiscard]] const auto &weights() const noexcept { return m_weights; };

 private:
  [[nodiscard]] auto _eclipse(const Point &p) const noexcept {
    const auto _p = p.rotated(m_radian);

    return std::pow(_p.x / m_radius.x, 2) + std::pow(_p.y / m_radius.y, 2);
  };

  void _arrange_diamond(const real_t weight_sd) noexcept {
    const auto range = int(std::max(m_radius.x, m_radius.y));

    {// reserve
      const auto capacity = (range + 1) * (range + 1 + range);
      m_points.reserve(capacity);
      m_weights.reserve(capacity);
    }

    {// arrange points
      for (auto y = -range; y < 1; ++y)

        for (auto x = -range; x < range + 1; ++x)

          if ((x < 0 && y == 0) || y < 0)

            if (std::abs(x) + std::abs(y) <= range)

              if (_eclipse({x, y}) <= 1)

                m_points.emplace_back(Point(x, y));
    }

    {// arrange weights
      const auto weight_scale = 1 / (2 * weight_sd * weight_sd);

      for (const auto &p : m_points) {
        const auto r = std::abs(p.x) + std::abs(p.y);
        const auto weight = EXP(-r * r * weight_scale);

        m_weights.emplace_back(weight);
      }

      const auto weights_sum = std::accumulate(m_weights.cbegin(), m_weights.cend(), real_t{});

      for (auto &w : m_weights)
        w /= weights_sum;// normalize m_weights
    }

    m_size = int(m_points.size());

    m_points.shrink_to_fit();
    m_weights.shrink_to_fit();
  };

  void _arrange_ellipse(const real_t weight_sd) noexcept {
    const auto range = int(std::max(m_radius.x, m_radius.y));

    {// reserve
      const auto capacity = (range + 1) * (range + 1 + range);
      m_points.reserve(capacity);
      m_weights.reserve(capacity);
    }

    {// arrange points
      for (auto y = -range; y < 1; ++y)

        for (auto x = -range; x < range + 1; ++x)

          if ((x < 0 && y == 0) || y < 0)

            if (_eclipse({x, y}) <= 1)

              m_points.emplace_back(Point(x, y));
    }

    {// arrange weights
      const auto weight_scale = 1 / (2 * weight_sd * weight_sd);

      for (const auto &p : m_points) {
        const auto r = std::abs(p.x) + std::abs(p.y);
        const auto weight = EXP(-r * r * weight_scale);

        m_weights.emplace_back(weight);
      }

      const auto weights_sum = std::accumulate(m_weights.cbegin(), m_weights.cend(), real_t{});

      for (auto &w : m_weights)
        w /= weights_sum;// normalize m_weights
    }

    m_size = int(m_points.size());

    m_points.shrink_to_fit();
    m_weights.shrink_to_fit();
  };

  int m_size{};
  Point_<real_t> m_radius{};
  real_t m_radian{};
  Points m_points;
  Values m_weights;
};

}// namespace pmo

#endif//PMO_TEMPLATE_PATCH_H_
