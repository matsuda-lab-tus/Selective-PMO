//
// Created by Hiroki Kojima on 2021/06/29.
//

#ifndef PMO_CONSTANT_H_
#define PMO_CONSTANT_H_

namespace pmo {

#ifdef PMO_USE_FMATH
// generalize exp
namespace pmo::fmath {

inline double exp(double x) { return ::fmath::expd(x); }

inline float exp(float x) { return ::fmath::exp(x); }

}// namespace pmo::fmath
#endif

// use same name exp/log2 between different libraries
#if defined(PMO_USE_FMATH)
using fmath::log2;
using pmo::fmath::exp;
#else
using std::exp;
using std::log2;
#endif

// Define Floating/Integer point width
#if defined(PMO_USE_DOUBLE)
using real_t = double;
using uint_t = uint64_t;
#else
using real_t = float;
using uint_t = uint32_t;
#endif

inline real_t EXP(real_t x) noexcept {
  return x < real_t(200) ? exp(x) : exp(real_t(200));
}

inline real_t LOG2(real_t x) noexcept {
  return log2(x);
}

constexpr auto PI = 3.14159265358979323846;

}// namespace pmo

#endif//PMO_CONSTANT_H_
