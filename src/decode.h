#ifndef DECODE_H_
#define DECODE_H_

#include <stdbool.h>
#include <stdint.h>

#define COEFF_FRACTIONAL_BITS 14

#if 0
typedef float FILTER_COEFF;
typedef float FILTER_STATE_DATA;
#define FLOAT_TO_COEFF(x) (x)
#define FILTER_MULT(a, b) ((a) * (b))
#else
typedef int16_t FILTER_COEFF;
typedef int16_t FILTER_STATE_DATA;
#define FLOAT_TO_COEFF(x) ((FILTER_COEFF)((x) * (1 << COEFF_FRACTIONAL_BITS)))
#define FILTER_MULT(a, b) (((a) * (b)) >> COEFF_FRACTIONAL_BITS)
#endif

typedef int8_t FILTER_IN_DATA;
typedef FILTER_IN_DATA FILTER_OUT_DATA;

#ifdef __AVR__
typedef int16_t RMS_IN_DATA;
// Consider using __uint24 for RMS_OUT_DATA if possible (code size optimization)
typedef uint32_t RMS_OUT_DATA;
typedef int8_t RUNS_OUT_DATA;
typedef uint8_t DECODE_OUT_DATA;
#else
typedef int16_t RMS_IN_DATA;
typedef uint32_t RMS_OUT_DATA;
typedef int8_t RUNS_OUT_DATA;
typedef uint8_t DECODE_OUT_DATA;
#endif

typedef RMS_OUT_DATA RUNS_IN_DATA;
typedef RUNS_OUT_DATA DECODE_IN_DATA;

bool pump_decoder(
        uint8_t channel,
        uint8_t window_size,
        uint16_t threshold,
        int8_t hysteresis,
        int8_t offset,
        FILTER_IN_DATA in,
        DECODE_OUT_DATA *out
    );

#endif

