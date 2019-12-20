#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

const int SAMPLE_RATE = 8000;
const int FREQUENCY = 1270;

#define DATA_TYPE uint16_t
#define PHASE_TYPE uint16_t
#define PHASE_FRACTION_BITS 8

#define uint16_tMAX UINT16_MAX
#define  uint8_tMAX  UINT8_MAX

#define CAT(X,Y) CAT_(X,Y)
#define CAT_(X,Y) X ## Y

DATA_TYPE get_sample(PHASE_TYPE *phase, PHASE_TYPE step, const DATA_TYPE sines[64])
{
    uint8_t top = *phase >> PHASE_FRACTION_BITS;
    bool half    = top & (1 << 7);
    bool quarter = top & (1 << 6);
    uint8_t lookup = (top ^ -quarter) & ((1 << 6) - 1);

    *phase += step;

    return sines[lookup] ^ -half;
}

void run(int samples, DATA_TYPE output[samples], const DATA_TYPE sines[64])
{
    PHASE_TYPE phase = 0;
    const PHASE_TYPE step = (1u << PHASE_FRACTION_BITS) * SAMPLE_RATE / FREQUENCY;

    for (int i = 0; i < samples; i++) {
        output[i] = get_sample(&phase, step, sines);
    }
}

int main()
{
    DATA_TYPE sines[64];

    for (int i = 0; i < 64; i++) {
        sines[i] = (sinf(2 * M_PI * (i + 0.5) / 256) - 1) * (CAT(DATA_TYPE,MAX) / 2) - 1;
    }

    const int samples = 64 * 8;
    DATA_TYPE output[samples];

    run(samples, output, sines);

    for (int i = 0; i < samples; i++) {
        printf("samples[%3d] = % f\n", i, 2.0f * (float)(output[i] - CAT(DATA_TYPE,MAX) / 2) / CAT(DATA_TYPE,MAX));
    }
}

