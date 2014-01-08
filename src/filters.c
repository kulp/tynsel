#include "filters.h"
#include <errno.h>
#include <stdlib.h>
#include <math.h>

struct filter_state {
    struct filter_entry {
        int tapcount;
        double *taps;
    } *entry;
    double *history;
    int last_index;
};

// Adapted from // dspUtils-10.js // Dr A.R.Collins <http://www.arc.id.au/>
/*
 * This function calculates Kaiser windowed
 * FIR filter coefficients for a single passband
 * based on
 * "DIGITAL SIGNAL PROCESSING, II" IEEE Press pp 123-126.
 *
 * Fs=Sampling frequency
 * Fa=Low freq ideal cut off (0=low pass)
 * Fb=High freq ideal cut off (Fs/2=high pass)
 * Att=Minimum stop band attenuation (>21dB)
 * M=Number of points in filter (ODD number)
 * H[] holds the output coefficients (they are symetric only half generated)
 */
struct filter_state *filter_create(enum filter_type type, double cutoff, unsigned M, unsigned Fs, double Att)
{
    double Fa = cutoff, Fb = cutoff;
    if (type == FILTER_TYPE_LOW_PASS)
        Fa = 0;
    else if (type == FILTER_TYPE_HIGH_PASS)
        Fb = (double)Fs / 2;
    else
        goto badparams;

    if (M % 2 == 0 || M > 1024) // arbitrary upper limit
        goto badparams;

    double *H = malloc(M * sizeof *H);
    int Np = (M - 1) / 2;
    { // scope for VLA
        double A[Np + 1];

        // Calculate the impulse response of the ideal filter
        A[0] = 2 * (Fb - Fa) / Fs;
        for (int j = 1; j <= Np; j++)
            A[j] = (sin(2 * j * M_PI * Fb / Fs) - sin(2 * j * M_PI * Fa / Fs)) / (j * M_PI);

        // Calculate the desired shape factor for the Kaiser - Bessel window
        double Alpha = Att < 21 ? 0
                     : Att > 50 ? 0.1102 * (Att - 8.7)
                     : 0.5842 * pow((Att - 21), 0.4) + 0.07886 * (Att - 21);

        // Window the ideal response with the Kaiser - Bessel window
        double Inoalpha = j0(Alpha);
        for (int j = 0; j <= Np; j++)
            H[Np + j] = A[j] * j0(Alpha * sqrt(1 - ((double)j * j / (Np * Np)))) / Inoalpha;
    }

    for (int j = 0; j < Np; j++)
        H[j] = H[M - 1 - j];

    struct filter_state *s = malloc(sizeof *s);
    struct filter_entry *e = s->entry = malloc(sizeof *e);
    // depends on IEEE-754-like zeros
    s->history = calloc((e->tapcount = M), sizeof *s->history);
    s->last_index = 0;
    e->tapcount = M;
    e->taps = H;

    return s;
badparams:
    errno = EINVAL;
    return NULL;
}

void filter_put(struct filter_state *s, double input) {
    s->history[s->last_index++] = input;
    if (s->last_index == s->entry->tapcount)
        s->last_index = 0;
}

double filter_get(struct filter_state *s) {
    double acc = 0;
    int index = s->last_index;
    const struct filter_entry *e = s->entry;
    for (int i = 0; i < e->tapcount; ++i) {
        index = index != 0 ? index - 1 : e->tapcount - 1;
        acc += s->history[index] * e->taps[i];
    };

    return acc;
}

void filter_destroy(struct filter_state *s) {
    struct filter_entry *e = s->entry;
    free(e->taps);
    free(e);
    free(s->history);
    free(s);
}

