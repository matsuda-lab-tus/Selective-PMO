//
// Created by Hiroki Kojima on 2021/06/26.
//

#ifndef PMO_CONTEXT_PARAMETER_H_
#define PMO_CONTEXT_PARAMETER_H_

namespace pmo {

class ContextParameterUnit {
 public:
  using Param = real_t;

  [[nodiscard]] const auto &param() const noexcept { return m_feature; };

  auto &param() noexcept { return m_feature; };

  [[nodiscard]] const auto &entropy() const noexcept { return m_entropy; };

  auto &entropy() noexcept { return m_entropy; };

 private:
  Param m_entropy;
  Param m_feature;
};

template<typename T>
class ContextParameterMap_ {
 public:
  explicit ContextParameterMap_(const int height,
                                const int width,
                                const TemplatePatch &template_patch) noexcept
      : m_template_patch(template_patch) { reset(height, width); };

  explicit ContextParameterMap_(const TemplatePatch &template_patch) noexcept : m_params_map(), m_template_patch(template_patch){};

  ContextParameterMap_(const ContextParameterMap_ &) = delete;

  ContextParameterMap_ &operator=(const ContextParameterMap_ &) = delete;

  void reset(const int height, const int width) noexcept {
    m_params_map.resize(height, width);

    for (auto &param : m_params_map.data())
      param = std::unique_ptr<ContextParameterUnit>(new ContextParameterUnit());
  };

  [[nodiscard]] auto height() const noexcept { return m_params_map.height(); };

  [[nodiscard]] auto width() const noexcept { return m_params_map.width(); };

  [[nodiscard]] const auto &operator[](const Point &p) const noexcept { return m_params_map[p]; };

  auto &operator[](const Point &p) noexcept { return m_params_map[p]; };

  void update_entropy(const Point &p, real_t entropy) noexcept { operator[](p)->entropy() = entropy; };

  auto update_feature(const Point &p) noexcept -> real_t {
    const auto &w = m_template_patch.weights();
    const auto &r = m_template_patch.points();
    auto feature = real_t{};
    auto weights_sum = real_t{};

    for (auto i = 0; i < r.size(); ++i) {
      if ((p + r[i]).isin({0, 0}, {width(), height()})) {
        feature += w[i] * operator[](p + r[i])->entropy();
        weights_sum += w[i];
      }
    }

    if (weights_sum == real_t{})
      feature = sizeof(T) * 8;
    else
      feature /= weights_sum;

    operator[](p)->param() = feature;

    return feature;
  };

 private:
  const TemplatePatch &m_template_patch;
  Matrix_<std::unique_ptr<ContextParameterUnit>> m_params_map;
};

}// namespace pmo

#endif//PMO_CONTEXT_PARAMETER_H_
