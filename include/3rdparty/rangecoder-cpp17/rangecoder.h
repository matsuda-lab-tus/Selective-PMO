#pragma once
#ifndef RANGECODER_H_
#define RANGECODER_H_

#include <iomanip>
#include <iostream>
#include <limits>
#include <queue>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

namespace rangecoder
{
    using range_t = uint64_t;
    using byte_t = uint8_t;

    enum RangeCoderVerbose {
        SILENT = false,
        VERBOSE = true,
    };

    namespace local
    {
        constexpr auto TOP8 = range_t(1) << (64 - 8);
        constexpr auto TOP16 = range_t(1) << (64 - 16);

        auto hex_zero_filled(range_t bytes) -> std::string
        {
            std::stringstream sformatter;
            sformatter << std::setfill('0') << std::setw(sizeof(range_t) * 2) << std::hex << bytes;
            return sformatter.str();
        }

        auto hex_zero_filled(byte_t byte) -> std::string
        {
            std::stringstream sformatter;
            sformatter << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(byte);
            return sformatter.str();
        }

        class RangeCoder
        {
        public:
            RangeCoder()
            {
                m_lower_bound = 0;
                m_range = std::numeric_limits<range_t>::max();
            };

            template<RangeCoderVerbose RANGECODER_VERBOSE>
            auto update_param(range_t c_freq, range_t cum_freq, range_t total_freq) -> std::vector<byte_t>
            {
                auto bytes = std::vector<byte_t>();

                const auto range_per_total = m_range / total_freq;
                m_range = range_per_total * c_freq;
                m_lower_bound += range_per_total * cum_freq;

                if constexpr (RANGECODER_VERBOSE)
                {
                    std::cout << "  range, lower bound updated" << std::endl;
                    print_status();
                }
                while (is_no_carry_expansion_needed())
                {
                    bytes.push_back(do_no_carry_expansion<RANGECODER_VERBOSE>());
                }
                while (is_range_reduction_expansion_needed())
                {
                    bytes.push_back(do_range_reduction_expansion<RANGECODER_VERBOSE>());
                }
                if constexpr (RANGECODER_VERBOSE)
                {
                    std::cout << "  total: " << bytes.size() << " byte shifted" << std::endl;
                }
                return bytes;
            };

            template<RangeCoderVerbose RANGECODER_VERBOSE>
            auto shift_byte() -> byte_t
            {
                auto tmp = static_cast<byte_t>(m_lower_bound >> (64 - 8));
                m_range <<= 8;
                m_lower_bound <<= 8;
                if constexpr (RANGECODER_VERBOSE)
                {
                    std::cout << "  shifted out byte: "
                              << "0x"
                              << local::hex_zero_filled(tmp)
                              << std::endl;
                }
                return tmp;
            };

            void print_status() const
            {
                std::cout << "        range: 0x" << local::hex_zero_filled(range()) << std::endl;
                std::cout << "  lower bound: 0x" << local::hex_zero_filled(lower_bound()) << std::endl;
                std::cout << "  upper bound: 0x" << local::hex_zero_filled(upper_bound()) << std::endl;
            }

        protected:
            void lower_bound(const range_t lower_bound)
            {
                m_lower_bound = lower_bound;
            };

            void range(const range_t range)
            {
                m_range = range;
            };

            auto lower_bound() const -> range_t
            {
                return m_lower_bound;
            };

            auto range() const -> range_t
            {
                return m_range;
            };

            auto upper_bound() const -> uint64_t
            {
                return m_lower_bound + m_range;
            };

        private:
            auto is_no_carry_expansion_needed() const -> bool
            {
                return (m_lower_bound ^ upper_bound()) < local::TOP8;
            };

            template<RangeCoderVerbose RANGECODER_VERBOSE>
            auto do_no_carry_expansion() -> byte_t
            {
                if constexpr (RANGECODER_VERBOSE)
                {
                    std::cout << "  no carry expansion" << std::endl;
                }
                return shift_byte<RANGECODER_VERBOSE>();
            };

            auto is_range_reduction_expansion_needed() const -> bool
            {
                return m_range < local::TOP16;
            };

            template<RangeCoderVerbose RANGECODER_VERBOSE>
            auto do_range_reduction_expansion() -> byte_t
            {
                if constexpr (RANGECODER_VERBOSE)
                {
                    std::cout << "  range reduction expansion" << std::endl;
                }
                m_range = (~m_lower_bound) & (local::TOP16 - 1);
                return shift_byte<RANGECODER_VERBOSE>();
            };

            uint64_t m_lower_bound;
            uint64_t m_range;
        };
    }// namespace local

    class PModel
    {
    public:
        // Accumulated frequency of index, i.e. sum of frequency of range [min_index, index).
        virtual range_t cum_freq(int index) const = 0;

        // Frequency of index
        virtual range_t c_freq(int index) const = 0;

        range_t total_freq() const
        {
            return cum_freq(max_index()) + c_freq(max_index());
        };

        // Returns min index, the first valid index.
        // All index 'i', that satisfy 'pmodel.min_index() <= i <= pmodel.max_index()' must be valid index.
        virtual int min_index() const = 0;

        // Returns max index, the last valid index.
        // All index 'i', that satisfy 'pmodel.min_index() <= i <= pmodel.max_index()' must be valid index.
        virtual int max_index() const = 0;

        bool index_is_valid(int index)
        {
            return min_index() <= index && index <= max_index();
        }
    };

    class RangeEncoder : local::RangeCoder
    {
    public:
        // Returns number of bytes stabled.
        template<RangeCoderVerbose RANGECODER_VERBOSE = SILENT>
        auto encode(const PModel &pmodel, const int index) -> int
        {
            if constexpr (RANGECODER_VERBOSE)
            {
                std::cout << "  encode: " << index << std::endl;
                print_status();
            }
            const auto bytes = update_param<RANGECODER_VERBOSE>(pmodel.c_freq(index), pmodel.cum_freq(index), pmodel.total_freq());
            for (const auto byte : bytes)
            {
                m_bytes.push_back(byte);
            }
            if constexpr (RANGECODER_VERBOSE)
            {
                std::cout << "  encode: " << index << " done" << std::endl
                          << std::endl;
            }
            return bytes.size();
        };

        template<RangeCoderVerbose RANGECODER_VERBOSE = SILENT>
        auto finish() -> std::vector<byte_t>
        {
            for (auto i = 0; i < 8; i++)
            {
                m_bytes.push_back(shift_byte<RANGECODER_VERBOSE>());
            }
            return m_bytes;
        }

        void print_status() const
        {
            std::cout << "        range: 0x" << local::hex_zero_filled(range()) << std::endl;
            std::cout << "  lower bound: 0x" << local::hex_zero_filled(lower_bound()) << std::endl;
            std::cout << "  upper bound: 0x" << local::hex_zero_filled(upper_bound()) << std::endl;
            if (m_bytes.empty())
            {
                std::cout << "        bytes: NULL" << std::endl;
            }
            else
            {
                std::cout << "        bytes: 0x";
                for (const auto byte : m_bytes)
                {
                    std::cout << local::hex_zero_filled(byte);
                }
                std::cout << std::endl;
            }
        }

    private:
        std::vector<uint8_t> m_bytes;
    };

    class RangeDecoder : local::RangeCoder
    {
    public:
        void start(std::queue<byte_t> bytes)
        {
            m_bytes = std::move(bytes);
            lower_bound(0);
            range(std::numeric_limits<range_t>::max());

            for (auto i = 0; i < 8; i++)
            {
                shift_byte_buffer();
            }
        };

        void start(const std::vector<byte_t> &bytes)
        {
            // Convert vector to queue.
            for (auto byte : bytes)
            {
                m_bytes.push(byte);
            }
            lower_bound(0);
            range(std::numeric_limits<range_t>::max());

            for (auto i = 0; i < 8; i++)
            {
                shift_byte_buffer();
            }
        };

        // Returns index of pmodel used to encode.
        // pmodel **must** be same as used to encode.
        template<RangeCoderVerbose RANGECODER_VERBOSE = SILENT>
        auto decode(const PModel &pmodel) -> int
        {
            if constexpr (RANGECODER_VERBOSE)
            {
                std::cout << "  decode: unknown " << std::endl;
                print_status();
            }
            const auto index = binary_search_encoded_index<RANGECODER_VERBOSE>(pmodel);
            const auto n = update_param<RANGECODER_VERBOSE>(pmodel.c_freq(index), pmodel.cum_freq(index), pmodel.total_freq()).size();
            for (int i = 0; i < n; i++)
            {
                shift_byte_buffer();
            }
            if constexpr (RANGECODER_VERBOSE)
            {
                std::cout << "  decode: " << index << " done" << std::endl;
                std::cout << std::endl;
            }
            return static_cast<int>(index);
        };

        void print_status() const
        {
            std::cout << "        range: 0x" << local::hex_zero_filled(range()) << std::endl;
            std::cout << "  lower bound: 0x" << local::hex_zero_filled(lower_bound()) << std::endl;
            std::cout << "  upper bound: 0x" << local::hex_zero_filled(upper_bound()) << std::endl;
            std::cout << "         data: 0x" << local::hex_zero_filled(m_data) << std::endl;
        }

    private:
        // binary search encoded index
        template<RangeCoderVerbose RANGECODER_VERBOSE>
        auto binary_search_encoded_index(const PModel &pmodel) const -> int
        {
            auto left = pmodel.min_index();
            auto right = pmodel.max_index();
            const auto range_per_total = range() / pmodel.total_freq();
            const auto f = (m_data - lower_bound()) / range_per_total;

            if constexpr (RANGECODER_VERBOSE)
            {
                std::cout << "  --------- BINARY SEARCH ---------" << std::endl;
                std::cout << "  find cum: (data: 0x"
                          << local::hex_zero_filled(m_data)
                          << " - lower bound: 0x"
                          << local::hex_zero_filled(lower_bound())
                          << ")"
                          << std::endl
                          << "            / range_per_total: 0x"
                          << local::hex_zero_filled(range_per_total)
                          << " = "
                          << f
                          << std::endl;
                std::cout << "  binary search encoded index: " << left << " " << right << std::endl;
            }

            while (left < right)
            {
                const auto mid = (left + right) / 2;
                const auto mid_cum = pmodel.cum_freq(mid + 1);

                if constexpr (RANGECODER_VERBOSE)
                {
                    std::cout << "  middle index: (left: " << left << " + right: " << right << " ) / 2 = " << mid
                              << ", cum at middle: " << mid_cum << std::endl;
                }

                if (mid_cum <= f)
                {
                    if constexpr (RANGECODER_VERBOSE)
                    {
                        std::cout << "  target contains between left:" << left << " middle: " << mid << std::endl;
                    }
                    left = mid + 1;
                }
                else
                {
                    if constexpr (RANGECODER_VERBOSE)
                    {
                        std::cout << "  target contains between middle:" << mid << " right: " << right << std::endl;
                    }
                    right = mid;
                }
            }
            if constexpr (RANGECODER_VERBOSE)
            {
                std::cout << "  find! left: " << left << "= right: " << right << std::endl;
                std::cout << "  --------- BINARY SEARCH FINISH ---------" << std::endl;
            }
            return left;
        };

        void shift_byte_buffer()
        {
            const auto front_byte = m_bytes.front();
            m_data = (m_data << 8) | static_cast<range_t>(front_byte);
            m_bytes.pop();
        };

        std::queue<byte_t> m_bytes;
        range_t m_data;
    };

    template<int N = 256>
    class UniformDistribution : public PModel
    {
    public:
        UniformDistribution() = default;

        range_t c_freq(const int index) const override
        {
            return 1;
        }

        range_t cum_freq(const int index) const override
        {
            return index;
        }

        int min_index() const override
        {
            return 0;
        }

        int max_index() const override
        {
            return N - 1;
        }

        void print() const
        {
            std::cout << std::endl;
            std::cout << "UNIFORM DIST" << std::endl;
            for (auto i = min_index(); i <= max_index(); i++)
            {
                std::cout << "idx: " << i << ", c: " << c_freq(i) << ", cum: " << cum_freq(i) << std::endl;
            }
            std::cout << std::endl;
        }
    };
}// namespace rangecoder
#endif
