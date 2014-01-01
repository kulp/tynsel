#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "audio.h"
#include "decode.h"
#include "filters.h"
#include "streamdecode.h"

#define countof(X) (sizeof (X) / sizeof (X)[0])

extern int read_file(struct audio_state *a, const char *filename, size_t size, double input[size]);
extern int write_file_pcm(struct audio_state *a, const char *filename, size_t size, double output[size]);

static int emit(void *userdata, int status, int data)
{
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
    if (argc != 2) {
        fprintf(stderr, "Supply input filename\n");
        return EXIT_FAILURE;
    }

    const char *filename_in  = argv[1];

    size_t bufsiz = 80000;
    double *buf = malloc(bufsiz * sizeof *buf);
    struct audio_state _as = {
        .baud_rate   = 300,
        .start_bits  = 1,
        .data_bits   = 7,
        .stop_bits   = 2,
        .parity_bits = 1,
    }, *as = &_as;
    bufsiz = read_file(as, filename_in, bufsiz, buf);

    struct stream_state *sd;
    streamdecode_init(&sd, as, NULL, emit);
    unsigned much;
    srand(time(NULL));
    int j = 0;
    for (unsigned i = 0; i < bufsiz; i += much) {
        printf("iteration %d\n", j++);
        do much = 0.1 * bufsiz * rand() / (double)RAND_MAX; while (much + i > bufsiz);
        streamdecode_process(sd, much, &buf[i]);
    }
    streamdecode_fini(sd);

    return 0;
}

