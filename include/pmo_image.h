//
// Created by Hiroki Kojima on 2021/06/13.
//

#ifndef PMO_IMAGE_H_
#define PMO_IMAGE_H_

namespace pmo {

template<typename T>
class Image_ {
 public:
  explicit Image_(const std::string &input) {
    auto ifs = std::ifstream(input, std::ios_base::binary);
    auto info = PNM::Info();

    ifs >> PNM::probe(info);

    if (info.valid()) {
      reset(info.height(), info.width());

#ifdef PMO_VERBOSE2
      printf("[Image info]\n");
      printf("path     : %s\n", input.c_str());
      printf("width    : %lu\n", info.width());
      printf("height   : %lu\n", info.height());
      printf("level    : %lu\n", info.max() + 1);
      printf("channel  : %lu\n", info.channel());
      printf("type     : P%d\n", info.type());
      printf("\n");
#endif

      ifs >> PNM::load(m_image.data(), info);

    } else {
      std::cout <<"Input is invalid file: " << input << std::endl;
      exit(EXIT_FAILURE);
    }
  };

  explicit Image_() : m_image(){};

  Image_(const Image_ &) = delete;

  Image_ &operator=(const Image_ &) = delete;

  void reset(const int height, const int width) noexcept { m_image.resize(height, width); };

  [[nodiscard]] auto height() const noexcept { return m_image.height(); };

  [[nodiscard]] auto width() const noexcept { return m_image.width(); };

  [[nodiscard]] const auto &data() const noexcept { return m_image.data(); };

  [[nodiscard]] const auto &operator[](const Point &pix) const noexcept { return m_image[pix]; };

  auto &operator[](const Point &pix) noexcept { return m_image[pix]; };

  void save(const std::string &output) {
    auto ofs = std::ofstream(output);
    const auto info = PNM::Info(width(), height(), PNM::P5, (sizeof(T) << 8) - 1);

    if (info.valid()) {

#ifdef PMO_VERBOSE2
      printf("[Image info]\n");
      printf("path     : %s\n", output.c_str());
      printf("width    : %lu\n", info.width());
      printf("height   : %lu\n", info.height());
      printf("level    : %lu\n", info.max() + 1);
      printf("channel  : %lu\n", info.channel());
      printf("type     : P%d\n", info.type());
      printf("\n");
#endif

      ofs << PNM::save(m_image.data(), info);
    }
  };

 private:
  Matrix_<T> m_image;
};

}// namespace pmo

#endif//PMO_IMAGE_H_