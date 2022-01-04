//
// Created by Hiroki Kojima on 2021/07/18.
//

#ifndef PMO_ENCODER_H_
#define PMO_ENCODER_H_

namespace pmo {

template<typename T, uint_t N>
class Encoder {
 public:
  Encoder(const std::string &bitstream,
          const Image_<T> &image,
          const TemplatePatch &template_patch,
          const BasicParameterMap &basic_param_map,
          ModelParameterMap_<N> &model_param_map,
          ContextParameterMap_<T> &context_param_map) noexcept
      : m_range_encoder(),
        m_bitstream(bitstream),
        m_image(image),
        m_template_patch(template_patch),
        m_basic_param_map(basic_param_map),
        m_model_param_map(model_param_map),
        m_context_param_map(context_param_map){};

  void encode_start() {
    const auto ud_4bit = UniformDistribution(16);
    const auto ud_8bit = UniformDistribution(256);
    const auto ud_16bit = UniformDistribution(65536);

    // for image status
    m_header_num_bytes += m_range_encoder.encode(ud_16bit, m_image.width() - 1);
    m_header_num_bytes += m_range_encoder.encode(ud_16bit, m_image.height() - 1);

    // for distribution
    m_header_num_bytes += m_range_encoder.encode(ud_8bit, m_basic_param_map.num_dists() - 1);

    // for template patch
    m_header_num_bytes += m_range_encoder.encode(ud_4bit, m_template_patch.radius() - 1);

    // for model parameter
    m_header_num_bytes += m_range_encoder.encode(ud_4bit, m_model_param_map.num_units() - 1);

#ifdef PMO_VERBOSE2
    printf("[Header info]\n");
    printf("@Image\n");
    printf("width       : %d\n", m_image.width());
    printf("height      : %d\n", m_image.height());
    printf("@Distribution\n");
    printf("num         : %lu\n", m_basic_param_map.num_dists());
    printf("@Template        \n");
    printf("radius      : %d\n", m_template_patch.radius());
    printf("@Model parameter\n");
    printf("num         : %lu\n", m_model_param_map.num_units());
    printf("\n");
#endif
  };

  void encode_model_parameter(const uint_t unit_id) {
    const auto ud_1bit = UniformDistribution(2);

    const auto has_pix = m_model_param_map.num_pix(unit_id) > 0;
    m_param_num_bytes += m_range_encoder.encode(ud_1bit, has_pix);

    if (has_pix) {
      const auto unit = m_model_param_map[unit_id];
      const auto indices = unit->quantize();
      unit->restore(indices);

      for (auto i = 0; i < N; ++i) {
        const auto precision = static_cast<int>(ModelParameterUnit_<N>::PRE[i]);
        const auto ud_nbit = UniformDistribution(precision);

        m_param_num_bytes += m_range_encoder.encode(ud_nbit, indices[i]);
      }
    }
  };

  void encode_pix(const Point &target) {
    const auto feature = m_context_param_map.update_feature(target);
    m_model_param_map.set_unit(target, feature);

    const auto &basic_params = m_basic_param_map[target]->params();
    const auto &model_params = m_model_param_map[target]->params();
    const auto &context_param = m_context_param_map[target]->param();

    auto mix_dist = MixtureDistribution_<T, N, WITHOUT_GRADIENT>(basic_params, model_params, context_param);

    const auto histogram = mix_dist.as_histogram();
    const auto freq_table = FreqTable(histogram);

    m_range_encoder.encode(freq_table, m_image[target]);

    const auto probability = mix_dist.probability(m_image[target]).value;
    const auto entropy = -LOG2(probability);
    m_context_param_map.update_entropy(target, entropy);
  };

  auto encode_finish() {
    std::cout << "finish" << std::endl;
    const auto &data = m_range_encoder.finish();
    auto file = std::ofstream(m_bitstream, std::ios::out | std::ios::binary);
    file.write(reinterpret_cast<const char *>(&data[0]), data.size());
    file.close();
#ifdef PMO_VERBOSE1
    printf("[Coding info]\n");
    printf("Header (bits)           : %lu\n", 8 * m_header_num_bytes);
    printf("Parameter (bits)        : %lu\n", 8 * m_param_num_bytes);
    printf("Image (bits)            : %lu\n", 8 * (data.size() - m_header_num_bytes - m_param_num_bytes));
    printf("Coding rate (bits/pel)  : %.5f\n", 8. * data.size() / (m_image.height() * m_image.width()));
    printf("\n");
#endif

    return data;
  };

 private:
  rangecoder::RangeEncoder m_range_encoder;
  const std::string &m_bitstream;
  const Image_<T> &m_image;
  const TemplatePatch &m_template_patch;
  const BasicParameterMap &m_basic_param_map;
  ModelParameterMap_<N> &m_model_param_map;
  ContextParameterMap_<T> &m_context_param_map;

  uint_t m_header_num_bytes{};
  uint_t m_param_num_bytes{};
};

}// namespace pmo

#endif//PMO_ENCODER_H_