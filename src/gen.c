/*
 * Copyright (c) 2012-2020 Darren Kulp
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

#include "encode.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

typedef ENCODE_DATA_TYPE DATA_TYPE;

struct encode_state {
    SERIAL_CONFIG serial;
    BYTE_STATE byte_state;
    int verbosity;
    float gain;
    bool realtime;
};

static int parse_opts(struct encode_state *s, int argc, char *argv[], const char **outfile)
{
    int ch;
    while ((ch = getopt(argc, argv, "C:G:S:T:P:D:p:o:r:" "v")) != -1) {
        switch (ch) {
            case 'C': s->byte_state.channel  = strtol(optarg, NULL, 0); break;
            case 'G': s->gain                = strtof(optarg, NULL);    break;
            case 'T': s->serial.stop_bits    = strtol(optarg, NULL, 0); break;
            case 'P': s->serial.parity_bits  = strtol(optarg, NULL, 0); break;
            case 'D': s->serial.data_bits    = strtol(optarg, NULL, 0); break;
            case 'p': s->serial.parity       = strtol(optarg, NULL, 0); break;
            case 'o': *outfile               = optarg;                  break;
            case 'r': s->realtime            = strtol(optarg, NULL, 0); break;

            case 'v': s->verbosity++;                                   break;
            default: fprintf(stderr, "args error before argument index %d\n", optind); return -1;
        }
    }

    return 0;
}

static void null_handler(int ignored)
{
    (void)ignored;
}

// returns zero on failure
int main(int argc, char* argv[])
{
    const char *output_file = NULL;
    struct encode_state _s = {
        .serial = {
            .data_bits   = 8,
            .parity_bits = 0,
            .stop_bits   = 2,
        },
        .byte_state = {
            .channel = 0,
        },
        .verbosity = 0,
        .gain      = 0.5,
    }, *s = &_s;

    int rc = parse_opts(s, argc, argv, &output_file);
    if (rc)
        return rc;

    if (s->byte_state.channel > 1) {
        fprintf(stderr, "Invalid channel %d\n", s->byte_state.channel);
        return -1;
    }

    if (!output_file && s->verbosity)
        fprintf(stderr, "No file specified to generate -- using stdout\n");

    FILE *output_stream = output_file ? fopen(output_file, "w") : stdout;
    if (! output_stream) {
        fprintf(stderr, "Failed to open `%s' : %s\n", output_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    DATA_TYPE (*sines)[WAVE_TABLE_SIZE];
    init_sines(&sines, s->gain);
    s->byte_state.bit_state.sample_state.quadrant = sines;

    if (s->realtime) {
        struct timeval tv = { .tv_usec = 1.0 / SAMPLE_RATE * 1000000 };
        struct itimerval it = { .it_interval = tv, .it_value = tv };

        // Do not buffer output at all
        setvbuf(stdout, NULL, _IONBF, 0);

        // Ignore SIGALRM except to wake us up
        signal(SIGALRM, null_handler);

        // Set up periodic SIGALRM
        setitimer(ITIMER_REAL, &it, NULL);

        static struct termios tio;
        tcgetattr(STDIN_FILENO, &tio);
        // Enable extreme non-blocking (return from read() as quickly as possible,
        // possibly with no data)
        tio.c_lflag &= ~ICANON;
        tio.c_cc[VMIN] = 0;
        tio.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &tio);

        sigset_t set;
        sigemptyset(&set);

        DATA_TYPE out = 0;
        while (true) { // detecting EOF is impossible, so Ctrl-C or Ctrl-\ is expected
            fwrite(&out, sizeof out, 1, output_stream);

            char ch = 0;
            sigsuspend(&set);
            int result = read(STDIN_FILENO, &ch, 1);

            // The ignoring of the return value of encode_bytes implies that we
            // do not expect the input to arrive at a higher rate than we can
            // encode (about 300/11=27 characters per second). If data does in
            // fact arrive faster than we can process it, bytes that are not
            // accepted by encode_bytes will be dropped (because they will be
            // replaced with a new byte by the next time encode_bytes is
            // called). This is a reasonable behavior for "realtime" mode.
            (void)encode_bytes(&s->serial, &s->byte_state, result > 0, s->byte_state.channel, ch, &out);

            if (result < 0)
                break;
        }
    } else {
        for (size_t i = 0; i < SAMPLES_PER_BIT; /* incremented inside loop */) {
            DATA_TYPE out = 0;
            if (encode_carrier(&s->serial, &s->byte_state, true, s->byte_state.channel, &out))
                i++;
            fwrite(&out, sizeof out, 1, output_stream);
        }

        char ch = 0;
        rc = fread(&ch, 1, 1, stdin);
        while (!feof(stdin)) {
            DATA_TYPE out = 0;
            if (encode_bytes(&s->serial, &s->byte_state, true, s->byte_state.channel, ch, &out))
                rc = fread(&ch, 1, 1, stdin);
            fwrite(&out, sizeof out, 1, output_stream);
        }

        for (size_t i = 0; i < SAMPLES_PER_BIT; /* incremented inside loop */) {
            DATA_TYPE out = 0;
            if (encode_carrier(&s->serial, &s->byte_state, true, s->byte_state.channel, &out))
                i++;
            fwrite(&out, sizeof out, 1, output_stream);
        }
    }

    // drain the encoder
    {
        DATA_TYPE out = 0;
        while (! encode_carrier(&s->serial, &s->byte_state, true, s->byte_state.channel, &out))
            fwrite(&out, sizeof out, 1, output_stream);
    }

    return 0;
}
