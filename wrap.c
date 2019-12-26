#include <math.h>
#include <stdint.h>
#include <stdio.h>

const int SAMPLE_RATE = 8000;
const int FREQUENCY = 1270;

#define DATA_TYPE uint16_t
#define PHASE_TYPE uint16_t
#define PHASE_FRACTION_BITS 8

#define TABLE_SIZE 64u
#define MAJOR_PER_CYCLE (TABLE_SIZE * 4)
#define MINOR_PER_CYCLE (MAJOR_PER_CYCLE * (1 << (PHASE_FRACTION_BITS)))

#define uint16_tMAX UINT16_MAX
#define  uint8_tMAX  UINT8_MAX

#define CAT(X,Y) CAT_(X,Y)
#define CAT_(X,Y) X ## Y

#define TEST_BIT(Word,Index) (((Word) & (1 << (Index))) ? -1 : 0)

DATA_TYPE get_sample(PHASE_TYPE *phase, PHASE_TYPE step, const DATA_TYPE sines[TABLE_SIZE])
{
    uint8_t top = *phase >> PHASE_FRACTION_BITS;
    DATA_TYPE  half    = TEST_BIT(top, 7);
    PHASE_TYPE quarter = TEST_BIT(top, 6);
    uint8_t lookup = (top ^ quarter) % TABLE_SIZE;

    *phase += step;

    return sines[lookup] ^ half;
}

void run(int samples, DATA_TYPE output[samples], const DATA_TYPE sines[TABLE_SIZE])
{
    PHASE_TYPE phase = 0;
    const PHASE_TYPE step = MINOR_PER_CYCLE * FREQUENCY / SAMPLE_RATE;

    for (int i = 0; i < samples; i++) {
        output[i] = get_sample(&phase, step, sines);
    }
}

int main()
{
    DATA_TYPE sines[TABLE_SIZE];

    for (unsigned int i = 0; i < TABLE_SIZE; i++) {
        quadrant[i] = (sinf(2 * M_PI * (i + 0.5) / MAJOR_PER_CYCLE) - 1) * (CAT(DATA_TYPE,MAX) / 2) - 1;
    }

    const int samples = TABLE_SIZE * 8;
    DATA_TYPE output[samples];

    run(samples, output, sines);

    for (int i = 0; i < samples; i++) {
        printf("samples[%3d] = % f\n", i, 2.0f * (float)(output[i] - CAT(DATA_TYPE,MAX) / 2) / CAT(DATA_TYPE,MAX));
    }
}

