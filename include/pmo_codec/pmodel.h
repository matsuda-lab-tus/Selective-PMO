//
// Created by iamhi on 2022/01/04.
//

#ifndef PMO_PMODEL_H_
#define PMO_PMODEL_H_

namespace pmo {

class UniformDistribution : public rangecoder::PModel {
 public:
  explicit UniformDistribution(const int level) : m_LEVEL(level) {}

  [[nodiscard]] rangecoder::range_t c_freq(int index) const override { return 1; }

  [[nodiscard]] rangecoder::range_t cum_freq(int index) const override { return index; }

  [[nodiscard]] int min_index() const override { return 0; }

  [[nodiscard]] int max_index() const override { return m_LEVEL - 1; }

 private:
  const int m_LEVEL;
};

template<typename T, std::size_t LEVEL>
class FreqTable : public rangecoder::PModel {
 public:
  using Table = std::array<T, LEVEL>;

  explicit FreqTable(const Table &freq) : m_c_freq(freq) {
    for (int i = this->min_index(); i < this->max_index(); ++i)
      m_cum_freq[i + 1] = m_cum_freq[i] + m_c_freq[i];
  }

  [[nodiscard]] rangecoder::range_t c_freq(int index) const override { return m_c_freq[index]; }

  [[nodiscard]] rangecoder::range_t cum_freq(int index) const override { return m_cum_freq[index]; }

  [[nodiscard]] int min_index() const override { return 0; }

  [[nodiscard]] int max_index() const override { return static_cast<int>(m_c_freq.size()) - 1; }

 private:
  const Table &m_c_freq;
  Table m_cum_freq{};
};

}// namespace pmo

#endif//PMO_PMODEL_H_
