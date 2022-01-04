//
// Created by Hiroki Kojima on 2021/06/09.
//

#ifndef PMO_CONFIG_H_
#define PMO_CONFIG_H_

namespace pmo {

class Config {
 public:
  Config(int argc, char **const argv) : m_is_decoder(true), m_is_encoder(true) {
    cmdline::parser parser;

    // files
    parser.add<std::string>("input", 'i', "Target image file", false, "");
    parser.add<std::string>("bitstream", 'b', "Bitstream file", false, "");
    parser.add<std::string>("output", 'o', "Reconstructed image file", false, "");

    // parameters
    parser.add<uint_t>("ex_win", 's', "Size of search window", false, 80);
    parser.add<uint_t>("ex_num", 'e', "# of examples per pel", false, 64);
    parser.add<uint_t>("pr_win", 't', "Size of training window", false, 10);
    parser.add<uint_t>("pr_num", 'p', "# of predictors per pel", false, 25);
    parser.add<uint_t>("cs_num", 'c', "# of context segments", false, 16);
    parser.add<uint_t>("tp_rad", 'r', "Size of template radius", false, 3);
    parser.add<real_t>("tp_wgt", 'w', "Template weight penalty", false, 0.030);

    parser.parse_check(argc, argv);

    // set values
    m_path_input_image = parser.get<std::string>("input");
    m_path_bitstream = parser.get<std::string>("bitstream");
    m_path_output_image = parser.get<std::string>("output");

    m_size_search_window = parser.get<uint_t>("ex_win");
    m_num_examples = parser.get<uint_t>("ex_num");
    m_size_training_window = parser.get<uint_t>("pr_win");
    m_num_predictors = parser.get<uint_t>("pr_num");
    m_num_context_segments = parser.get<uint_t>("cs_num");
    m_size_template_radius = parser.get<uint_t>("tp_rad");
    m_template_weight_penalty = parser.get<real_t>("tp_wgt");

    if (m_path_output_image.empty())
      m_is_decoder = false;

    if (m_path_input_image.empty())
      m_is_encoder = false;

#ifdef PMO_VERBOSE1
    printf("[Mode info]\n");
    printf("run encoder  : ");
    printf(m_is_encoder ? "yes\n" : "no\n");
    printf("run decoder  : ");
    printf(m_is_decoder ? "yes\n" : "no\n");
    printf("mode         : ");
    if (m_is_encoder && m_is_decoder)
      printf("Test\n");
    else if (m_is_encoder)
      printf("Encoder\n");
    else if (m_is_decoder)
      printf("Decoder\n");
    printf("\n");
#endif
  };

  Config(const Config &) = delete;

  Config &operator=(const Config &) = delete;

  [[nodiscard]] const auto &path_input_image() const noexcept { return m_path_input_image; };

  [[nodiscard]] const auto &path_bitstream() const noexcept { return m_path_bitstream; };

  [[nodiscard]] const auto &path_output_image() const noexcept { return m_path_output_image; };

  template<typename T>
  [[nodiscard]] auto size_search_window() const noexcept { return static_cast<T>(m_size_search_window); };

  template<typename T>
  [[nodiscard]] auto num_examples() const noexcept { return static_cast<T>(m_num_examples); };

  template<typename T>
  [[nodiscard]] auto size_training_window() const noexcept { return static_cast<T>(m_size_training_window); };

  template<typename T>
  [[nodiscard]] auto num_predictors() const noexcept { return static_cast<T>(m_num_predictors); };

  template<typename T>
  [[nodiscard]] auto num_context_segments() const noexcept { return static_cast<T>(m_num_context_segments); };

  template<typename T>
  [[nodiscard]] auto size_template_radius() const noexcept { return static_cast<T>(m_size_template_radius); };

  template<typename T>
  [[nodiscard]] auto template_weight_penalty() const noexcept { return static_cast<T>(m_template_weight_penalty); };

  [[nodiscard]] auto is_decoder() const noexcept { return m_is_decoder; };

  [[nodiscard]] auto is_encoder() const noexcept { return m_is_encoder; };

 private:
  // files
  std::string m_path_input_image;
  std::string m_path_bitstream;
  std::string m_path_output_image;

  // parameters
  uint_t m_size_search_window;
  uint_t m_num_examples;
  uint_t m_size_training_window;
  uint_t m_num_predictors;
  uint_t m_num_context_segments;
  uint_t m_size_template_radius;
  real_t m_template_weight_penalty;

  bool m_is_decoder;
  bool m_is_encoder;
};

}// namespace pmo

#endif//PMO_CONFIG_H_
