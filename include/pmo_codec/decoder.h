//
// Created by Hiroki Kojima on 2021/07/18.
//

#ifndef PMO_DECODER_H_
#define PMO_DECODER_H_

namespace pmo {

template<typename T, uint_t N>
class Decoder {
 public:
  Decoder(const std::string &bitstream,
          Image_<T> &image,
          TemplatePatch &template_patch,
          BasicParameterMap &basic_param_map,
          ModelParameterMap_<N> &model_param_map,
          ContextParameterMap_<T> &context_param_map) noexcept
      : m_range_decoder(),
        m_bitstream(bitstream),
        m_image(image),
        m_template_patch(template_patch),
        m_basic_param_map(basic_param_map),
        m_model_param_map(model_param_map),
        m_context_param_map(context_param_map){};

  void decode_start() {
    auto file = std::ifstream(m_bitstream, std::ios::in | std::ios::binary);
    file.seekg(0, std::ios::end);
    const auto filesize = file.tellg();
    file.seekg(0);
    auto data = std::vector<rangecoder::byte_t>(filesize);
    file.read(reinterpret_cast<char *>(&data[0]), filesize);
    file.close();
    m_range_decoder.start(data);

    const auto ud_4bit = UniformDistribution(16);
    const auto ud_8bit = UniformDistribution(256);
    const auto ud_16bit = UniformDistribution(65536);

    // for image status
    const auto width = m_range_decoder.decode(ud_16bit) + 1;
    const auto height = m_range_decoder.decode(ud_16bit) + 1;

    // for distribution
    const auto num_dists = m_range_decoder.decode(ud_8bit) + 1;

    // for template patch
    const auto radius = m_range_decoder.decode(ud_4bit) + 1;

    // for model parameter
    const auto num_units = m_range_decoder.decode(ud_4bit) + 1;

    m_image.reset(height, width);
    m_template_patch.reset(radius, 0);
    m_basic_param_map.reset(height, width, num_dists);
    m_model_param_map.reset(height, width, num_units);
    m_context_param_map.reset(height, width);

#ifdef PMO_VERBOSE2
    printf("[Header info]\n");
    printf("@Image\n");
    printf("width       : %d\n", width);
    printf("height      : %d\n", height);
    printf("@Distribution\n");
    printf("num         : %d\n", num_dists);
    printf("@Template\n");
    printf("radius      : %d\n", radius);
    printf("@Model parameter\n");
    printf("num         : %d\n", num_units);
    printf("\n");
#endif
  };

  void decode_model_parameter(const uint_t unit_id) {
    const auto ud_1bit = UniformDistribution(2);

    const auto has_pix = m_range_decoder.decode(ud_1bit);

    if (has_pix) {
      auto indices = std::array<uint_t, N>{};

      for (auto i = 0; i < N; ++i) {
        const auto precision = static_cast<int>(ModelParameterUnit_<N>::PRE[i]);
        const auto ud_nbit = UniformDistribution(precision);

        indices[i] = m_range_decoder.decode(ud_nbit);
      }

      m_model_param_map[unit_id]->restore(indices);
    }
  };

  void decode_pix(const Point &target) {
    const auto feature = m_context_param_map.update_feature(target);
    m_model_param_map.set_unit(target, feature);

    const auto &basic_params = m_basic_param_map[target]->params();
    const auto &model_params = m_model_param_map[target]->params();
    const auto &context_param = m_context_param_map[target]->param();

    auto mix_dist = MixtureDistribution_<T, N, WITHOUT_GRADIENT>(basic_params, model_params, context_param);

    const auto histogram = mix_dist.as_histogram();
    const auto freq_table = FreqTable(histogram);

    m_image[target] = m_range_decoder.decode(freq_table);

    const auto probability = mix_dist.probability(m_image[target]).value;
    const auto entropy = -LOG2(probability);
    m_context_param_map.update_entropy(target, entropy);
  };

  void decode_finish(){};

 private:
  rangecoder::RangeDecoder m_range_decoder;
  const std::string &m_bitstream;
  Image_<T> &m_image;
  TemplatePatch &m_template_patch;
  BasicParameterMap &m_basic_param_map;
  ModelParameterMap_<N> &m_model_param_map;
  ContextParameterMap_<T> &m_context_param_map;
};

}// namespace pmo

#endif//PMO_DECODER_H_