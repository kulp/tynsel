#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sndfile.h>
#include <errno.h>

#include "audio.h"
#include "decode.h"
#include "filters.h"
#include "streamdecode.h"

static int emit(void *userdata, int status, int data)
{
    (void)userdata;
    switch (status) {
        case STREAM_ERR_OK:
            printf("char '%c' (%d)\n", data, data);
            break;
        case STREAM_ERR_PARITY:
            printf("char '%c' (%d) (PARITY FAILED)\n", data, data);
            break;
        default:
            printf("unknown error\n");
            break;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Supply channel number and input filename\n");
        return EXIT_FAILURE;
    }

    int channel = strtol(argv[1], NULL, 0);
    const char *filename = argv[2];

    struct audio_state _as = {
        .baud_rate   = 300,
        .start_bits  = 1,
        .data_bits   = 7,
        .stop_bits   = 2,
        .parity_bits = 1,
    }, *as = &_as;

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
        as->sample_rate = sinfo.samplerate;
    }

    struct stream_state *sd;
    streamdecode_init(&sd, as, NULL, emit, channel);

    sf_count_t count = 0;
    if (sinfo.channels == 1) {
        do {
            double tmp[1024];
            count = sf_read_double(sf, tmp, 1024);
            streamdecode_process(sd, count, tmp);
        } while (count);
    } else {
        do {
            double tmp[sinfo.channels];
            count = sf_read_double(sf, tmp, sinfo.channels);
            streamdecode_process(sd, 1, tmp);
        } while (count);
    }

    streamdecode_fini(sd);

    if (sf_error(sf))
        sf_perror(sf);

    sf_close(sf);

    return 0;
}

