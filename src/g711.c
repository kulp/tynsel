#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <sndfile.h>

int main(int argc, char* argv[])
{
    int ch;
    while ((ch = getopt(argc, argv, "")) != -1) {
        switch (ch) {
            default: fprintf(stderr, "args error before argument index %d\n", optind); return -1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "No files specified to process\n");
        return -1;
    }

    SF_INFO iinfo = { .format = 0 };
    const char *input  = argv[optind++],
               *output = argv[optind++];
    SNDFILE *sfi = sf_open(input , SFM_READ , &iinfo);

    SF_INFO oinfo = {
                .samplerate = iinfo.samplerate,
                .channels   = iinfo.channels,
                .format     = SF_FORMAT_WAV | SF_FORMAT_ULAW | SF_FORMAT_PCM_S8,
            };
    SNDFILE *sfo = sf_open(output, SFM_WRITE, &oinfo);

    if (!sfi) {
        fprintf(stderr, "Error opening `%s' for reading : %s\n", input, sf_strerror(sfi));
        exit(1);
    }

    if (!sfo) {
        fprintf(stderr, "Error opening `%s' for writing : %s\n", input, sf_strerror(sfo));
        exit(1);
    }

    double sample;
    while (sf_read_double(sfi, &sample, 1))
        sf_write_double(sfo, &sample, 1);

    sf_close(sfi);
    sf_close(sfo);

    return 0;
}


