#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

const int SAMPLE_RATE = 8000;
const int FREQUENCY = 1270;

void run(int samples, float output[samples], float sines[64])
{
    uint8_t phase = 0;
    uint8_t step = SAMPLE_RATE / FREQUENCY;

    for (int i = 0; i < samples; i++) {
        bool half    = phase & (1 << 7);
        bool quarter = phase & (1 << 6);
        uint8_t lookup = (phase ^ -quarter) & ((1 << 6) - 1);

        float result = sines[lookup] * (half ? -1 : 1);

#if 0
        printf("half = %d, quarter = %d, lookup = %4hhd, phase = %4hhd, result = % f\n",
                half, quarter, lookup, phase, result);
#endif

        output[i] = result;

        phase += step;
    }
}

int main()
{
    float sines[64];

    for (int i = 0; i < 64; i++) {
        sines[i] = sinf(2 * M_PI * (i + 0.5) / 256);
    }

    const int samples = 64 * 8;
    float output[samples];

    run(samples, output, sines);

    for (int i = 0; i < samples; i++) {
        printf("samples[%3d] = % f\n", i, output[i]);
    }
}

