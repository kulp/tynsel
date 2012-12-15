#ifndef DECODE_H_
#define DECODE_H_

#include <stddef.h>

struct decode_state {
    unsigned sample_rate;
    double sample_offset;
    const unsigned baud_rate;
    const unsigned fft_size;

    int start_bits,
        data_bits,
        parity_bits,
        stop_bits;

    int verbosity;

    const double (*freqs)[2];
};

int decode_byte(struct decode_state *s, size_t size, double input[size], int output[], double *offset, int channel);
int decode_data(struct decode_state *s, size_t count, double input[count]);
void decode_cleanup(void);

#define SAMPLES_PER_BIT(s) ((double)s->sample_rate / s->baud_rate)

#endif

