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

DATA_TYPE get_sample(PHASE_TYPE *phase, PHASE_TYPE step, const DATA_TYPE quadrant[TABLE_SIZE])
{
    uint8_t top = *phase >> PHASE_FRACTION_BITS;
    DATA_TYPE  half    = TEST_BIT(top, 7);
    PHASE_TYPE quarter = TEST_BIT(top, 6);
    uint8_t lookup = (top ^ quarter) % TABLE_SIZE;

    *phase += step;

    return quadrant[lookup] ^ half;
}

void run(int samples, DATA_TYPE output[samples], const DATA_TYPE quadrant[TABLE_SIZE])
{
    PHASE_TYPE phase = 0;
    const PHASE_TYPE step = MINOR_PER_CYCLE * FREQUENCY / SAMPLE_RATE;

    for (int i = 0; i < samples; i++) {
        output[i] = get_sample(&phase, step, quadrant);
    }
}

// Creates a quarter-wave sine table, scaled by the given maximum value.
// Valid indices into the table are [0,TABLE_SIZE).
// Input indices are augmented by 0.5 before computing the sine, on the
// assumption that TABLE_SIZE is a power of two and that we want to do
// arithmetic with powers of two. If we did not compensate somehow, we would
// either have two entries for zero (when flipping the quadrant), two entries
// for max (also during flipping), or an uneven gap between the minimum
// positive and minimum negative output values.
static void make_sine_table(DATA_TYPE sines[TABLE_SIZE], DATA_TYPE max)
{
    for (unsigned int i = 0; i < TABLE_SIZE; i++) {
        sines[i] = (sinf(2 * M_PI * (i + 0.5) / MAJOR_PER_CYCLE) - 1) * max - 1;
    }
}

int main()
{
    DATA_TYPE quadrant[TABLE_SIZE];

    make_sine_table(quadrant, CAT(DATA_TYPE,MAX) / 2);

    const int samples = TABLE_SIZE * 8;
    DATA_TYPE output[samples];

    run(samples, output, quadrant);

    for (int i = 0; i < samples; i++) {
        printf("samples[%3d] = % f\n", i, 2.0f * (float)(output[i] - CAT(DATA_TYPE,MAX) / 2) / CAT(DATA_TYPE,MAX));
    }
}

