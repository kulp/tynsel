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

#include "common.h"
#include "audio.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sndfile.h>

int read_file(struct audio_state *a, const char *filename, size_t size, double input[size])
{
    size_t index = 0;
    SNDFILE *sf = NULL;
    SF_INFO sinfo = { .format = 0 };
    {
        sf = sf_open(filename, SFM_READ, &sinfo);
        if (!sf) {
            fprintf(stderr, "Failed to open `%s' for reading : %s\n", filename, strerror(errno));
            exit(EXIT_FAILURE);
        }

        // getting the sample rate from the file means right now the `-s' option on
        // the command line has no effect. In the future it might be removed, or it
        // might be necessary when the audio input has no accompanying sample-rate
        // information.
        a->sample_rate = sinfo.samplerate;
    }

    {
        sf_count_t count = 0;
        size_t per = (size_t)SAMPLES_PER_BIT(a);
        do {
            double tmp[sinfo.channels];
            count = sf_read_double(sf, tmp, sinfo.channels);
            size_t where = per + index++;
            input[where] = tmp[0];
        } while (count && (index + per) < size);

        if (sf_error(sf))
            sf_perror(sf);

        if (index >= size) {
            fprintf(stderr, "Warning, ran out of buffer space before reaching end of file\n");
            index = size;
        }

        sf_close(sf);
    }

    return index;
}

int write_file_pcm(struct audio_state *a, const char *filename, size_t size, double output[size])
{
    size_t index = 0;
    SNDFILE *sf = NULL;
    SF_INFO sinfo = {
                .samplerate = a->sample_rate,
                .channels   = 1,
                .format     = SF_FORMAT_WAV | SF_FORMAT_FLOAT, //SF_FORMAT_PCM_U8,
            };
    {
        sf = sf_open(filename, SFM_WRITE, &sinfo);
        if (!sf) {
            fprintf(stderr, "Failed to open `%s' for writing : %s (%s)\n", filename, sf_strerror(sf), strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    sf_write_double(sf, output, size);

    if (sf_error(sf))
        sf_perror(sf);

    sf_close(sf);

    return index;
}

