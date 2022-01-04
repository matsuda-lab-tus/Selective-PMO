//
// Created by Hiroki Kojima on 2021/06/22.
//

#ifndef PMO_MODEL_PARAMETER_H_
#define PMO_MODEL_PARAMETER_H_

namespace pmo {

template<uint_t N>
class ModelParameterUnit_ {
 public:
  using Params = std::array<real_t, N>;

  explicit ModelParameterUnit_(const uint_t unit_id) noexcept : unit_id(unit_id), m_params(INI){};

  ModelParameterUnit_(const ModelParameterUnit_ &) = delete;

  ModelParameterUnit_ &operator=(const ModelParameterUnit_ &) = delete;

  [[nodiscard]] const auto &params() const noexcept { return m_params; };

  auto &params() noexcept { return m_params; };

  auto quantize() const noexcept {
    auto indices = std::array<uint_t, N>{};

    for (auto i = 0; i < N; ++i)
      indices[i] = (PRE[i] - .5) * std::min(1., std::max(0., (m_params[i] - MIN[i]) / (MAX[i] - MIN[i])));

    return indices;
  };

  auto &restore(const std::array<uint_t, N> indices) noexcept {

    for (auto i = 0; i < N; ++i)
      m_params[i] = MIN[i] + (MAX[i] - MIN[i]) * (indices[i] + .5) / PRE[i];

    return params();
  };

  const uint_t unit_id;
  static const std::array<real_t, N> MIN;
  static const std::array<real_t, N> INI;
  static const std::array<real_t, N> MAX;
  static const std::array<uint_t, N> PRE;

 private:
  Params m_params;
};

template<uint_t N>
class ModelParameterMap_ {
 public:
  explicit ModelParameterMap_(const int height,
                              const int width,
                              const uint_t num_units) noexcept { reset(height, width, num_units); };

  explicit ModelParameterMap_() noexcept : m_num_units(0), m_params_map(), m_params_units(){};

  ModelParameterMap_(const ModelParameterMap_ &) = delete;

  ModelParameterMap_ &operator=(const ModelParameterMap_ &) = delete;

  void reset(const int height, const int width, const uint_t num_units) noexcept {
    m_num_units = num_units;

    m_params_map.resize(height, width);
    m_params_units.resize(num_units);

    for (auto i = 0; i < m_num_units; ++i)
      m_params_units[i] = std::make_shared<ModelParameterUnit_<N>>(i);

    for (auto &param : m_params_map.data())
      param = m_params_units.back();
  };

  [[nodiscard]] auto height() const noexcept { return m_params_map.height(); };

  [[nodiscard]] auto width() const noexcept { return m_params_map.width(); };

  [[nodiscard]] auto num_units() const noexcept { return m_num_units; };

  [[nodiscard]] const auto &operator[](const Point &p) const noexcept { return m_params_map[p]; };

  auto &operator[](const Point &p) noexcept { return m_params_map[p]; };

  [[nodiscard]] const auto &operator[](const uint_t unit_id) const noexcept { return m_params_units[unit_id]; };

  auto &operator[](const uint_t unit_id) noexcept { return m_params_units[unit_id]; };

  [[nodiscard]] auto num_pix(const uint_t unit_id) const noexcept { return operator[](unit_id).use_count() - 1; };

  [[nodiscard]] auto unit_id(const Point &p) const noexcept { return operator[](p)->unit_id; };

  void set_unit(const Point &p, const real_t feature) noexcept {
    constexpr auto max = 7.5;
    const auto unit_id = static_cast<uint_t>((m_num_units - 1) * std::min(1., feature / max));

    _set_unit(p, unit_id);
  };

 private:
  void _set_unit(const Point &p, const uint_t unit_id) noexcept { operator[](p) = operator[](unit_id); };

  uint_t m_num_units{};
  std::vector<std::shared_ptr<ModelParameterUnit_<N>>> m_params_units;
  Matrix_<std::shared_ptr<ModelParameterUnit_<N>>> m_params_map;
};

}// namespace pmo

#endif//PMO_MODEL_PARAMETER_H_
