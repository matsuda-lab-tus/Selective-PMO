//
// Created by Hiroki Kojima on 2021/07/23.
//

#ifndef PMO_MATRIX_H_
#define PMO_MATRIX_H_

namespace pmo {

template<class T>
class Matrix_ {
 public:
  explicit Matrix_() noexcept : Matrix_(0, 0){};

  explicit Matrix_(const int height, const int width) noexcept { resize(height, width); };

  explicit Matrix_(const int height, const int width, const T &val) noexcept { resize(height, width, val); };

  void resize(const int height, const int width) noexcept {
    m_height = height;
    m_width = width;
    m_matrix.resize(height * width);
  };

  void resize(const int height, const int width, const T &val) noexcept {
    m_height = height;
    m_width = width;
    m_matrix.resize(height * width, val);
  };

  [[nodiscard]] auto height() const noexcept { return m_height; };

  [[nodiscard]] auto width() const noexcept { return m_width; };

  [[nodiscard]] const auto &data() const noexcept { return m_matrix; };

  auto &data() noexcept { return m_matrix; };

  [[nodiscard]] const auto &operator[](const Point &point) const noexcept { return m_matrix[point.y * m_width + point.x]; };

  auto &operator[](const Point &point) noexcept { return m_matrix[point.y * m_width + point.x]; };

 private:
  int m_height{};
  int m_width{};
  std::vector<T> m_matrix;
};

}// namespace pmo

#endif//PMO_MATRIX_H_
