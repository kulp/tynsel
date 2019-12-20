#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

const int SAMPLE_RATE = 8000;
const int FREQUENCY = 1270;

uint16_t get_sample(uint8_t *phase, uint8_t step, const uint16_t sines[64])
{
    bool half    = *phase & (1 << 7);
    bool quarter = *phase & (1 << 6);
    uint8_t lookup = (*phase ^ -quarter) & ((1 << 6) - 1);

    *phase += step;

    return sines[lookup] ^ -half;
}

void run(int samples, uint16_t output[samples], const uint16_t sines[64])
{
    uint8_t phase = 0;
    const uint8_t step = SAMPLE_RATE / FREQUENCY;

    for (int i = 0; i < samples; i++) {
        output[i] = get_sample(&phase, step, sines);
    }
}

int main()
{
    uint16_t sines[64];

    for (int i = 0; i < 64; i++) {
        sines[i] = sinf(2 * M_PI * (i + 0.5) / 256) * INT16_MAX + INT16_MIN;
    }

    const int samples = 64 * 8;
    uint16_t output[samples];

    run(samples, output, sines);

    for (int i = 0; i < samples; i++) {
        printf("samples[%3d] = % f\n", i, 2.0f * (float)(output[i] - INT16_MAX) / UINT16_MAX);
    }
}

