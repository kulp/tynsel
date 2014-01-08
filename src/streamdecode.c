/*
 * Copyright (c) 2012-2014 Darren Kulp
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "streamdecode.h"
#include "audio.h"
#include "filters.h"

#include <stdlib.h>
#include <math.h>

#define WINDOW_SIZE(as) ((int)(SAMPLES_PER_BIT(as) / 2))

struct stream_state {
    enum {
        STATE_invalid,

        STATE_NOSYNC, // before any synchronisation at all : needs a full character synch time
        STATE_BSYNC,  // after a character has been received : needs STOPBITS synch time

        STATE_START,
        STATE_DATA,
        STATE_PARITY,
        STATE_STOP,

        STATE_ERROR,

        STATE_max,
    } state;

    unsigned tick; // samples since last state change
    unsigned gbltick; // samples since beginning of stream

    struct audio_state as; // TODO redefine the audio_state struct ; we only want a subset
    streamdecode_callback *cb;
    void *userdata;

    struct filter_state *chan, *bit[2];
    double *ehist[2];
    double energy[2];

    int bitcount;
    unsigned charac;
    int parity; // counts the number of set bits in charac
    int levhist; // the last level seen, -1 if none seen
};

int streamdecode_init(struct stream_state **sp, struct audio_state *as, void *ud, streamdecode_callback *cb, int channel)
{
    if (channel != 0 && channel != 1)
        return -1;

    struct stream_state *s = *sp = malloc(sizeof *s);

    const int len = ((int)SAMPLES_PER_BIT(as)) | 1;
    struct argset {
        enum filter_type type;
        double freq;
        int len;
        int rate;
        double att;
    } args[][3] = {
        { { FILTER_TYPE_LOW_PASS , bell103_freqs[0][1] + 300, len, as->sample_rate, 21 },
          { FILTER_TYPE_LOW_PASS , bell103_freqs[0][0] + 100, len, as->sample_rate, 21 },
          { FILTER_TYPE_HIGH_PASS, bell103_freqs[0][0] + 100, len, as->sample_rate, 21 }, },

        { { FILTER_TYPE_HIGH_PASS, bell103_freqs[1][0] - 300, len, as->sample_rate, 21 },
          { FILTER_TYPE_LOW_PASS , bell103_freqs[1][0] + 100, len, as->sample_rate, 21 },
          { FILTER_TYPE_HIGH_PASS, bell103_freqs[1][0] + 100, len, as->sample_rate, 21 }, },
    };

    s->cb       = cb;
    s->userdata = ud;
    s->state    = STATE_NOSYNC;
    s->as       = *as;
    // should stop hard-coding channel 0
    const struct argset *arg = args[channel];
    s->chan     = filter_create(arg[0].type, arg[0].freq, arg[0].len, arg[0].rate, arg[0].att);
    s->bit[0]   = filter_create(arg[1].type, arg[1].freq, arg[1].len, arg[1].rate, arg[1].att);
    s->bit[1]   = filter_create(arg[2].type, arg[2].freq, arg[2].len, arg[2].rate, arg[2].att);
    // depends on IEEE-754-type zeros
    s->ehist[0] = calloc(WINDOW_SIZE(&s->as), sizeof *s->ehist[0]);
    s->ehist[1] = calloc(WINDOW_SIZE(&s->as), sizeof *s->ehist[1]);
    s->tick     = 0;
    s->levhist  = -1;
    s->gbltick  = 0;

    return 0;
}

static int state_update(struct stream_state *s)
{
    double perbit = SAMPLES_PER_BIT(&s->as);
    int level = s->energy[1] > s->energy[0];
    int charbits = s->as.start_bits +
                   s->as.data_bits +
                   s->as.parity_bits +
                   s->as.stop_bits;

    switch (s->state) {
        case STATE_NOSYNC:
            // a START edge requires at least CHARBITS ticks of ONE followed by ZERO
            // TODO robustify edge detection ; discard spurious edges
            if (s->tick < (charbits * perbit)) {
                if (level == 0)
                    s->tick = 0; // reset tick counter so we count only strings of ONE
            } else if (s->levhist == 1 && level == 0) {
                s->state    = STATE_START;
                s->tick     = 0;
                s->bitcount = 0;
                s->charac   = 0;
                s->parity   = 0;
            }
            break;
        case STATE_BSYNC:
            // a START edge requires at least STOPBITS ticks of ONE followed by ZERO
            // TODO robustify edge detection ; discard spurious edges
            if (s->tick < (s->as.stop_bits * perbit)) {
                if (level == 0)
                    s->tick = 0; // reset tick counter so we count only strings of ONE
            } else if (s->levhist == 1 && level == 0) {
                s->state    = STATE_START;
                s->tick     = 0;
                s->bitcount = 0;
                s->charac   = 0;
                s->parity   = 0;
            }
            break;
        case STATE_START:
            // TODO robustify start-bit detection (drop spurious edges)
            if (s->tick >= perbit) {
                s->tick  = 0;
                s->state = STATE_DATA;
            }
            break;
        case STATE_DATA:
            if (s->tick >= s->as.data_bits * perbit) {
                s->tick = 0;
                s->state = STATE_PARITY;
            } else {
                double bitoffset = fmod(s->tick, perbit);
                // for now we just check the value at the middle of the bit
                if (bitoffset >= (perbit / 2) && bitoffset < (perbit / 2) + 1) {
                    // bits are received little-end first
                    s->charac |= level << s->bitcount++;
                    s->parity += level;
                }
            }
            break;
        case STATE_PARITY:
            if (s->tick >= s->as.parity_bits * perbit) {
                s->tick = 0;
                s->state = STATE_STOP;
            } else {
                double bitoffset = fmod(s->tick, perbit);
                // for now we just check the value at the middle of the bit
                if (bitoffset >= (perbit / 2) && bitoffset < (perbit / 2) + 1) {
                    s->parity += level;
                }
            }
            break;
        case STATE_STOP:
            if (s->tick >= s->as.stop_bits * perbit) {
                // we do not reset the tick counter, as the samples in the
                // STOP bits are counted to determine when we get a valid
                // START edge
                s->state = STATE_BSYNC;
                if (s->as.parity_bits > 0 && s->parity & 1) {
                    // XXX allow parity to be adjustable instead of always EVEN
                    s->cb(s->userdata, STREAM_ERR_PARITY, s->charac);
                } else {
                    s->cb(s->userdata, 0, s->charac);
                }
            } else {
                double bitoffset = fmod(s->tick, perbit);
                if (bitoffset >= (perbit / 2) && bitoffset < (perbit / 2) + 1) {
                    // TODO handle bad stop bits
                }
            }
            break;
        default:
            // TODO handle error cases
            break;
    }

    s->levhist = level;

    return 0;
}

int streamdecode_process(struct stream_state *s, size_t count, double samples[count])
{
    unsigned window_size = WINDOW_SIZE(&s->as);
    for (unsigned i = 0; i < count; i++) {
        s->gbltick++;
        s->tick++;

        filter_put(s->chan, samples[i]);
        double bandpassed = filter_get(s->chan);
        for (int b = 0; b < 2; b++) {
            filter_put(s->bit[b], bandpassed);
            double *energy = &s->energy[b];
            double *trailing = &s->ehist[b][(i + window_size - 1) % window_size];
            *energy -= *trailing;

            double bitval = filter_get(s->bit[b]);
            double term = bitval * bitval;
            *trailing = term;
            *energy += term;
        }

        // drop the first WINDOW_SIZE samples to make energy readings meaningful
        if (s->gbltick > window_size)
            state_update(s);
    }

    return 0;
}

void streamdecode_fini(struct stream_state *s)
{
    filter_destroy(s->bit[1]);
    filter_destroy(s->bit[0]);
    filter_destroy(s->chan);

    free(s->ehist[1]);
    free(s->ehist[0]);

    free(s);
}

