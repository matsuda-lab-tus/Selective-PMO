//
// Created by Hiroki Kojima on 2021/06/15.
//

#ifndef PMO_BASIC_PARAMETER_H_
#define PMO_BASIC_PARAMETER_H_

namespace pmo {

struct BasicParameter {
  real_t cost;
  real_t peak;
  real_t flag;
};

class BasicParameterUnit {
 public:
  using Params = std::vector<BasicParameter>;

  explicit BasicParameterUnit(const uint_t max_num, std::mutex &mutex) noexcept: m_mutex(mutex) { m_params.reserve(max_num); };

  [[nodiscard]] const auto &params() const noexcept { return m_params; }

  void params(const BasicParameter &new_param, const uint_t max_num_params) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_params.size() == max_num_params) {
      if (new_param.cost <= m_params.back().cost)
        m_params.pop_back();
      else
        return;
    }

    if (m_params.empty() || new_param.cost > m_params.back().cost) {
      m_params.emplace_back(new_param);// --- new_param has the highest cost in m_params
    } else {
      const auto point = std::partition_point(m_params.begin(),
                                              m_params.end(),
                                              [&new_param](const auto &param) {
                                                return new_param.cost > param.cost;
                                              });
      m_params.emplace(point, new_param);// --- Binary search the insertion point and insert new_param
    }
  }

 private:
  Params m_params;
  std::mutex &m_mutex;
};

class BasicParameterMap {
 public:
  explicit BasicParameterMap(const int height,
                             const int width,
                             const uint_t num_dists) noexcept { reset(height, width, num_dists); };

  explicit BasicParameterMap() noexcept : m_params_map(){};

  BasicParameterMap(const BasicParameterMap &) = delete;

  BasicParameterMap &operator=(const BasicParameterMap &) = delete;

  void reset(const int height, const int width, const uint_t num_dists) noexcept {
    m_num_dists = num_dists;

    m_params_map.resize(height, width);

    for (auto &param : m_params_map.data())
      param = std::unique_ptr<BasicParameterUnit>(new BasicParameterUnit(num_dists, m_mutex));
  };

  [[nodiscard]] auto height() const noexcept { return m_params_map.height(); };

  [[nodiscard]] auto width() const noexcept { return m_params_map.width(); };

  [[nodiscard]] auto num_dists() const noexcept { return m_num_dists; };

  [[nodiscard]] const auto &operator[](const Point &p) const noexcept { return m_params_map[p]; };

  auto &operator[](const Point &p) noexcept { return m_params_map[p]; };

 private:
  uint_t m_num_dists{};
  Matrix_<std::unique_ptr<BasicParameterUnit>> m_params_map;
  std::mutex m_mutex;
};

}// namespace pmo

#endif//PMO_BASIC_PARAMETER_H_
