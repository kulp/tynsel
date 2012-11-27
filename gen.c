#include <stdlib.h>
#include <getopt.h>
#include <sndfile.h>

static int sample_rate = 44100;
#define PERBIN          ((double)sample_rate / SIZE)
#define LOWEND          (freqs[2] - PERBIN)
#define HIGHEND         (freqs[3] + PERBIN)
#define BAUD_RATE       300
#define SAMPLES_PER_BIT ((double)sample_rate / BAUD_RATE)
static int start_bits  = 1,
           data_bits   = 8,
           parity_bits = 0,
           stop_bits   = 2;
#define ALL_BITS        (start_bits + data_bits + parity_bits + stop_bits)

#define countof(X) (sizeof (X) / sizeof (X)[0])

static const double freqs[] = {
    1070., 1270.,
    2025., 2225.,
};

static int verbosity;

int main(int argc, char* argv[])
{
    int sample_offset = 0;

    const char *output_file = NULL;

    int ch;
    while ((ch = getopt(argc, argv, "S:T:P:D:s:o:v")) != -1) {
        switch (ch) {
            case 'S': start_bits    = strtol(optarg, NULL, 0); break;
            case 'T': stop_bits     = strtol(optarg, NULL, 0); break;
            case 'P': parity_bits   = strtol(optarg, NULL, 0); break;
            case 'D': data_bits     = strtol(optarg, NULL, 0); break;
            case 's': sample_rate   = strtol(optarg, NULL, 0); break;
            case 'o': output_file   = optarg;                  break;
            case 'v': verbosity++; break;
            default: fprintf(stderr, "args error before argument index %d\n", optind); return -1;
        }
    }

    if (!output_file) {
        fprintf(stderr, "No file specified to generate -- use `-o'\n");
        return -1;
    }

    SF_INFO sinfo = {
        .samplerate = sample_rate,
        .channels   = 1,
        .format     = SF_FORMAT_WAV | SF_FORMAT_PCM_16,
    };
    SNDFILE *sf = sf_open(output_file, SFM_WRITE, &sinfo);

    sf_count_t count = 0;
    size_t index = 0;

    for (unsigned byte_index = 0; byte_index < argc - optind; byte_index++) {
        char *next = NULL;
        char *thing = argv[byte_index + optind];
        unsigned byte = strtol(thing, &next, 0);
        if (next == thing) {
            fprintf(stderr, "Error parsing argument at index %d, `%s'\n", optind, thing);
            return -1;
        }
    }

    //count = sf_read_double(sf, &_input[(size_t)SAMPLES_PER_BIT + index++], 1);

    if (verbosity) {
        printf("read %zd items\n", index);
        printf("sample rate is %4d Hz\n", sample_rate);
        printf("baud rate is %4d\n", BAUD_RATE);
        printf("samples per bit is %4.0f\n", SAMPLES_PER_BIT);
    }

    sf_close(sf);

    return 0;
}
