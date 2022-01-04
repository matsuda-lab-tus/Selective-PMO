//
// Created by Hiroki Kojima on 2021/06/29.
//

#ifndef PMO_MACRO_H_
#define PMO_MACRO_H_

#define PMO_VERBOSE1
#define PMO_VERBOSE2
#define PMO_USE_DOUBLE// Should be defined
#define PMO_USE_MIXTURE_LOGISTIC_DISTRIBUTION
// #define PMO_USE_MIXTURE_GAUSSIAN_DISTRIBUTION

// fmath maybe not support ARM
#if !(defined(__arm__) || defined(__aarch64__))
#define PMO_USE_FMATH
#endif

#endif//PMO_MACRO_H_
