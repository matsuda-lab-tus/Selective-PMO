//
// Created by Hiroki Kojima on 2021/07/17.
//

#ifndef PMO_POINT_H_
#define PMO_POINT_H_

// To avoid conflicts with built-in macro
// Caused by 3rdparty/cmdline
#ifdef min
#undef min
#endif

namespace pmo {

template<typename T>
struct Point_ {
  Point_() : x(0), y(0){};

  Point_(const T x, const T y) : x(x), y(y){};

  auto operator+(const Point_ &obj) const noexcept { return Point_(x + obj.x, y + obj.y); };

  auto operator==(const Point_ &obj) const noexcept { return (x == obj.x) && (y == obj.y); };

  auto &clip_min(const Point_ &_min) {
    x = std::max(x, _min.x);
    y = std::max(y, _min.y);

    return *this;
  };

  auto &clip_max(const Point_ &_max) {
    x = std::min(x, _max.x);
    y = std::min(y, _max.y);

    return *this;
  };

  [[nodiscard]] auto rotated(const real_t radian) const noexcept {
    const auto _cos = std::cos(radian);
    const auto _sin = std::sin(radian);

    return Point_<real_t>(x * _cos - y * _sin, x * _sin + y * _cos);
  };

  [[nodiscard]] auto isin(const Point_ &tl, const Point_ &br) const noexcept {
    return (x >= tl.x) && (y >= tl.y) && (x < br.x) && (y < br.y);
  };

  T x;
  T y;
};

using Point = Point_<int>;

struct Rect {
  Rect() : begin(), end(){};

  Rect(const Point begin, const Point end) : begin(begin), end(end){};

  [[nodiscard]] auto size() const noexcept { return (end.x - begin.x) * (end.y - begin.y); };

  Point begin;// top-left
  Point end;  // bottom-right
};

class CausalArea {
 public:
  CausalArea(const int image_height, const int image_width) noexcept: m_image_height(image_height), m_image_width(image_width){};

  const auto &locate(const Point &p, const int window_size) noexcept {
    m_window_above.begin = Point(p.x - window_size, p.y - window_size).clip_min({0, 0});
    m_window_above.end = Point(p.x + window_size + 1, p.y).clip_max({m_image_width, m_image_height});
    m_window_left.begin = Point(p.x - window_size, p.y).clip_min({0, 0});
    m_window_left.end = Point(p.x, p.y + 1).clip_max({m_image_width, m_image_height});

    return *this;
  };

  template<class Fn>
  void for_each(Fn func) const noexcept {
    for (const auto &w : {m_window_above, m_window_left})
      for (auto y = w.begin.y; y < w.end.y; ++y)
        for (auto x = w.begin.x; x < w.end.x; ++x)
          func(Point(x, y));
  };

  [[nodiscard]] auto size() const noexcept { return m_window_above.size() + m_window_left.size(); };

 private:
  const int m_image_height;
  const int m_image_width;
  Rect m_window_above{};
  Rect m_window_left{};
};

}// namespace pmo

#endif//PMO_POINT_H_
