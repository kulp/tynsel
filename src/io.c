/*
 * Copyright (c) 2012 Darren Kulp
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
#include "decode.h"

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
            fprintf(stderr, "Failed to open `%s' : %s\n", filename, strerror(errno));
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
        do {
            double tmp[sinfo.channels];
            count = sf_read_double(sf, tmp, sinfo.channels);
            size_t where = (size_t)SAMPLES_PER_BIT(a) + index++;
            input[where] = tmp[0];
        } while (count && index < size);

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

