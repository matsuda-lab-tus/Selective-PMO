#include <algorithm>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include "../rangecoder.h"

class FreqTable : public rangecoder::PModel
{
public:
    FreqTable(const std::vector<int> &data, const int max_index)
    {
        m_max_index = max_index;
        m_c_freq = std::vector<rangecoder::range_t>(max_index + 1, 0);
        m_cum_freq = std::vector<rangecoder::range_t>(max_index + 1, 0);
        for (const auto &i : data)
        {
            m_c_freq[i] += 1;
        }
        for (int i = 0; i < max_index; i++)
        {
            m_cum_freq[i + 1] = m_cum_freq[i] + m_c_freq[i];
        }
    }
    void print() const
    {
        std::cout << std::endl;
        std::cout << "FREQ TABLE" << std::endl;
        for (auto i = min_index(); i <= max_index(); i++)
        {
            std::cout << "idx: " << i << ", c: " << c_freq(i) << ", cum: " << cum_freq(i) << std::endl;
        }
        std::cout << std::endl;
    }
    rangecoder::range_t c_freq(const int index) const override
    {
        return m_c_freq[index];
    }
    rangecoder::range_t cum_freq(const int index) const override
    {
        return m_cum_freq[index];
    }
    int min_index() const
    {
        return 0;
    }
    int max_index() const
    {
        return m_max_index;
    }

private:
    int m_max_index;
    std::vector<rangecoder::range_t> m_c_freq;
    std::vector<rangecoder::range_t> m_cum_freq;
};

auto helper_enc_dec_freqtable(const std::vector<int> &data) -> std::vector<int>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    const auto max = *std::max_element(data.begin(), data.end());
    const auto pmodel = FreqTable(data, max);
    pmodel.print();
    // encode
    std::cout << "encode" << std::endl;
    auto enc = rangecoder::RangeEncoder();
    for (int i = 0; i < data.size(); i++)
    {
        std::cout << std::dec << i << "  encode: " << data[i] << std::endl;
        enc.print_status();
        enc.encode(pmodel, data[i]);
    }
    enc.print_status();
    const auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::local::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::local::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    auto que = std::queue<rangecoder::byte_t>();
    for (auto byte : bytes)
    {
        que.push(byte);
    }
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder();
    dec.start(que);
    auto decoded = std::vector<int>();
    for (int i = 0; i < data.size(); i++)
    {
        dec.print_status();
        auto d = dec.decode(pmodel);
        std::cout << std::dec << i << "  decode: " << d << std::endl;
        decoded.push_back(d);
    }
    dec.print_status();
    std::cout << "finish" << std::endl;
    return decoded;
}

auto helper_enc_dec_freqtable_start_vector(const std::vector<int> &data) -> std::vector<int>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    const auto max = *std::max_element(data.begin(), data.end());
    const auto pmodel = FreqTable(data, max);
    pmodel.print();
    // encode
    std::cout << "encode" << std::endl;
    auto enc = rangecoder::RangeEncoder();
    for (int i = 0; i < data.size(); i++)
    {
        std::cout << std::dec << i << "  encode: " << data[i] << std::endl;
        enc.print_status();
        enc.encode(pmodel, data[i]);
    }
    enc.print_status();
    const auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::local::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::local::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder();
    dec.start(bytes);
    auto decoded = std::vector<int>();
    for (int i = 0; i < data.size(); i++)
    {
        dec.print_status();
        auto d = dec.decode(pmodel);
        std::cout << std::dec << i << "  decode: " << d << std::endl;
        decoded.push_back(d);
    }
    dec.print_status();
    std::cout << "finish" << std::endl;
    return decoded;
}

auto test_uniform(const std::vector<int> &data) -> std::vector<int>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    const auto pmodel = rangecoder::UniformDistribution();
    // encode
    std::cout << "encode" << std::endl;
    auto enc = rangecoder::RangeEncoder();
    for (int i = 0; i < data.size(); i++)
    {
        std::cout << std::dec << i << "  encode: " << data[i] << std::endl;
        enc.print_status();
        enc.encode(pmodel, data[i]);
    }
    enc.print_status();
    const auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::local::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::local::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    auto que = std::queue<rangecoder::byte_t>();
    for (auto byte : bytes)
    {
        que.push(byte);
    }
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder();
    dec.start(que);
    auto decoded = std::vector<int>();
    for (int i = 0; i < data.size(); i++)
    {
        dec.print_status();
        auto d = dec.decode(pmodel);
        std::cout << std::dec << i << "  decode: " << d << std::endl;
        decoded.push_back(d);
    }
    dec.print_status();
    std::cout << "finish" << std::endl;
    return decoded;
}

auto test_uniform_big(const std::vector<int> &data) -> std::vector<int>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    const auto pmodel = rangecoder::UniformDistribution<65536>();
    // encode
    std::cout << "encode" << std::endl;
    auto enc = rangecoder::RangeEncoder();
    for (int i = 0; i < data.size(); i++)
    {
        std::cout << std::dec << i << "  encode: " << data[i] << std::endl;
        enc.print_status();
        enc.encode(pmodel, data[i]);
    }
    enc.print_status();
    const auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::local::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::local::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    auto que = std::queue<rangecoder::byte_t>();
    for (auto byte : bytes)
    {
        que.push(byte);
    }
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder();
    dec.start(que);
    auto decoded = std::vector<int>();
    for (int i = 0; i < data.size(); i++)
    {
        dec.print_status();
        auto d = dec.decode(pmodel);
        std::cout << std::dec << i << "  decode: " << d << std::endl;
        decoded.push_back(d);
    }
    dec.print_status();
    std::cout << "finish" << std::endl;
    return decoded;
}

auto test_uniform_binary(const std::vector<bool> &data) -> std::vector<bool>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    const auto pmodel = rangecoder::UniformDistribution<2>();
    // encode
    std::cout << "encode" << std::endl;
    auto enc = rangecoder::RangeEncoder();
    for (int i = 0; i < data.size(); i++)
    {
        std::cout << std::dec << i << "  encode: " << data[i] << std::endl;
        enc.print_status();
        enc.encode(pmodel, data[i]);
    }
    enc.print_status();
    const auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::local::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::local::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    auto que = std::queue<rangecoder::byte_t>();
    for (auto byte : bytes)
    {
        que.push(byte);
    }
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder();
    dec.start(que);
    auto decoded = std::vector<bool>();
    for (int i = 0; i < data.size(); i++)
    {
        dec.print_status();
        auto d = dec.decode(pmodel);
        std::cout << std::dec << i << "  decode: " << d << std::endl;
        decoded.push_back(d);
    }
    dec.print_status();
    std::cout << "finish" << std::endl;
    return decoded;
}

auto test_uniform_4(const std::vector<int> &data) -> std::vector<int>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    const auto pmodel = rangecoder::UniformDistribution<4>();
    // encode
    std::cout << "encode" << std::endl;
    auto enc = rangecoder::RangeEncoder();
    for (int i = 0; i < data.size(); i++)
    {
        std::cout << std::dec << i << "  encode: " << data[i] << std::endl;
        enc.print_status();
        enc.encode(pmodel, data[i]);
    }
    enc.print_status();
    const auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::local::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::local::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    auto que = std::queue<rangecoder::byte_t>();
    for (auto byte : bytes)
    {
        que.push(byte);
    }
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder();
    dec.start(que);
    auto decoded = std::vector<int>();
    for (int i = 0; i < data.size(); i++)
    {
        dec.print_status();
        auto d = dec.decode(pmodel);
        std::cout << std::dec << i << "  decode: " << d << std::endl;
        decoded.push_back(d);
    }
    dec.print_status();
    std::cout << "finish" << std::endl;
    return decoded;
}

auto test_uniform_16(const std::vector<int> &data) -> std::vector<int>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    const auto pmodel = rangecoder::UniformDistribution<16>();
    // encode
    std::cout << "encode" << std::endl;
    auto enc = rangecoder::RangeEncoder();
    for (int i = 0; i < data.size(); i++)
    {
        std::cout << std::dec << i << "  encode: " << data[i] << std::endl;
        enc.print_status();
        enc.encode(pmodel, data[i]);
    }
    enc.print_status();
    const auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::local::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::local::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    auto que = std::queue<rangecoder::byte_t>();
    for (auto byte : bytes)
    {
        que.push(byte);
    }
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder();
    dec.start(que);
    auto decoded = std::vector<int>();
    for (int i = 0; i < data.size(); i++)
    {
        dec.print_status();
        auto d = dec.decode(pmodel);
        std::cout << std::dec << i << "  decode: " << d << std::endl;
        decoded.push_back(d);
    }
    dec.print_status();
    std::cout << "finish" << std::endl;
    return decoded;
}

// test rangecoder with frequency table.
TEST(RangeCoderTest, EncDecTest)
{
    const auto data = std::vector<int>{1, 2, 3, 4, 5, 8, 3, 2, 1, 0, 3, 7};
    EXPECT_EQ(helper_enc_dec_freqtable(data), data);
    std::cout << "finish" << std::endl;
}

// test rangecoder with frequency table, decoder called `decoder.start(vector)`.
TEST(RangeCoderTest, EncDecTestVectorStart)
{
    const auto data = std::vector<int>{1, 2, 3, 4, 5, 8, 3, 2, 1, 0, 3, 7};
    EXPECT_EQ(helper_enc_dec_freqtable_start_vector(data), data);
    std::cout << "finish" << std::endl;
}

// test rangecoder with 256 level (8bit) uniform distribution.
TEST(RangeCoderTest, UniformDistributionTest)
{
    const auto data = std::vector<int>{1, 2, 3, 4, 5, 8, 3, 2, 1, 0, 3, 7};
    EXPECT_EQ(test_uniform(data), data);
    std::cout << "finish" << std::endl;
}

// test rangecoder with 2 level (1bit) uniform distribution.
TEST(RangeCoderTest, UniformBinaryDistributionTest)
{
    const auto data = std::vector<bool>{true, false, true, true, false, true, false, false, true, true, true, true};
    EXPECT_EQ(test_uniform_binary(data), data);
    std::cout << "finish" << std::endl;
}

// test rangecoder with 4 level uniform distribution.
TEST(RangeCoderTest, UniformDistributionTest4)
{
    const auto data = std::vector<int>{1, 2, 3, 2, 3, 2, 3, 2, 1, 0, 3, 1};
    EXPECT_EQ(test_uniform_4(data), data);
    std::cout << "finish" << std::endl;
}

// test rangecoder with 16 level (4bit) uniform distribution.
TEST(RangeCoderTest, UniformDistributionTest16)
{
    const auto data = std::vector<int>{1, 5, 3, 15, 2, 7, 9, 2, 1, 0, 3, 1};
    EXPECT_EQ(test_uniform_16(data), data);
    std::cout << "finish" << std::endl;
}

// test rangecoder with 65536 level (16bit) uniform distribution.
TEST(RangeCoderTest, UniformDistributionBigTest)
{
    const auto data = std::vector<int>{1, 2, 3, 4, 5, 65533, 3, 2, 1, 0, 3, 7};
    EXPECT_EQ(test_uniform_big(data), data);
    std::cout << "finish" << std::endl;
}

TEST(RangeCoderDebug, ReproductionTest)
{
    const auto ud_4bit = rangecoder::UniformDistribution<16>();
    const auto ud_8bit = rangecoder::UniformDistribution<256>();
    const auto ud_16bit = rangecoder::UniformDistribution<65536>();

    const auto width = 256;
    const auto height = 256;
    const auto num_dists = 64 + 25;
    const auto radius = 3;
    const auto num_units = 8;

    auto encoder = rangecoder::RangeEncoder();
    encoder.encode<rangecoder::VERBOSE>(ud_16bit, width - 1);
    encoder.encode<rangecoder::VERBOSE>(ud_16bit, height - 1);
    encoder.encode<rangecoder::VERBOSE>(ud_8bit, num_dists - 1);
    encoder.encode<rangecoder::VERBOSE>(ud_4bit, radius - 1);
    encoder.encode<rangecoder::VERBOSE>(ud_4bit, num_units - 1);
    for (auto i = 0; i < 4; ++i)
    {
        encoder.encode(ud_8bit, 0);// ! padding !
    }

    // encode something to emulate real encoder.
    const auto seed = 12345;
    const int len = width * height;
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> rand_binary(0, 255);
    auto gen = [&rand_binary, &rng]() { return rand_binary(rng); };
    for (auto i = 0; i < len; ++i)
    {
        std::cout << "\rencode dummy bits: " << i << "/" << len << ' ' << std::flush;
        encoder.encode(ud_8bit, gen());
    }
    std::cout << "done" << std::endl;

    auto data = encoder.finish();

    auto decoder = rangecoder::RangeDecoder();
    decoder.start(data);
    const auto width_m1 = decoder.decode<rangecoder::VERBOSE>(ud_16bit);
    const auto height_m1 = decoder.decode<rangecoder::VERBOSE>(ud_16bit);
    const auto num_dists_m1 = decoder.decode<rangecoder::VERBOSE>(ud_8bit);
    const auto radius_m1 = decoder.decode<rangecoder::VERBOSE>(ud_4bit);
    const auto num_units_m1 = decoder.decode<rangecoder::VERBOSE>(ud_4bit);
    for (auto i = 0; i < 4; ++i)
    {
        decoder.decode(ud_8bit);
    }

    EXPECT_EQ(width_m1, width - 1);
    EXPECT_EQ(height_m1, height - 1);
    EXPECT_EQ(num_dists_m1, num_dists - 1);
    EXPECT_EQ(radius_m1, radius - 1);
    EXPECT_EQ(num_units_m1, num_units - 1);

    std::cout << "----------- Result -----------" << std::endl;
    std::cout << "width: " << width << ", " << width_m1 + 1 << std::endl;
    std::cout << "height: " << height << ", " << height_m1 + 1 << std::endl;
    std::cout << "num_dists: " << num_dists << ", " << num_dists_m1 + 1 << std::endl;
    std::cout << "radius: " << radius << ", " << radius_m1 + 1 << std::endl;
    std::cout << "num_units: " << num_units << ", " << num_units_m1 + 1 << std::endl;
    std::cout << "------------------------------" << std::endl;
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
