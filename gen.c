#include <stdlib.h>
#include <getopt.h>
#include <sndfile.h>
#include <math.h>

static int sample_rate = 44100;
#define PERBIN          ((double)sample_rate / SIZE)
#define BAUD_RATE       300
#define SAMPLES_PER_BIT ((double)sample_rate / BAUD_RATE)
static int start_bits  = 1,
           data_bits   = 8,
           parity_bits = 0,
           stop_bits   = 2;
#define ALL_BITS        (start_bits + data_bits + parity_bits + stop_bits)

#define countof(X) (sizeof (X) / sizeof (X)[0])

static const double freqs[2][2] = {
    { 1070., 1270. },
    //{ 2025., 2225. },
    { 1350., 1850. },
};

static int verbosity;

int main(int argc, char* argv[])
{
    int sample_offset = 0;

    const char *output_file = NULL;
    unsigned channel = 1;

    int ch;
    while ((ch = getopt(argc, argv, "C:S:T:P:D:s:o:v")) != -1) {
        switch (ch) {
            case 'C': channel       = strtol(optarg, NULL, 0); break;
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

    if (channel > countof(freqs)) {
        fprintf(stderr, "Invalid channel %d\n", channel);
        return -1;
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

        double last_sample = 0.;
        int last_quadrant = 0;
        double last_freq = 0.;
        double last_offset = 0.;
        double leftover = 0.;
        double left = 0.;
        for (unsigned bit_index = 0; bit_index < data_bits; bit_index++) {
            unsigned bit = !!(byte & (1 << bit_index));
            double freq = freqs[channel][bit];
            double inverse = asin(last_sample);
            //double offset = (inverse - last_offset) / freq;
            //double offset = asin(last_sample) + (.5 * M_1_PI * last_quadrant);
            // thanks to Forest Belton for helping me with the maths
            //  t = (B_1*t_c + C_1 - C_2)/B_2
            // where
            //       t   is the new offset
            //       B_1 is last_freq / sample_rate
            //       B_2 is freq / sample_rate
            //       t_c is SAMPLES_PER_BIT
            //       C_1 is last_offset
            //       C_2 is 0

            double offset = (last_freq / freq) * SAMPLES_PER_BIT + last_offset + left;
            //double offset = 0;
            double samples_per_cycle = sample_rate / freq;
            printf("samples_per_cycle = %f\n", samples_per_cycle);
            printf("used = %f\n", samples_per_cycle * SAMPLES_PER_BIT);

            for (unsigned sample_index = 0; sample_index < SAMPLES_PER_BIT; sample_index++) {
                int quadrant = sample_index * 4 / SAMPLES_PER_BIT;
                int slope = 1 - (quadrant % 2 * 2); // +1 or -1
                double proportion = (double)sample_index / sample_rate;
                double radians = proportion * 2. * M_PI;
                double sample = sin(radians * freq + offset);

                sf_write_double(sf, &sample, 1);

                last_quadrant = quadrant;
                last_sample = sample;
            }

            //leftover = SAMPLES_PER_BIT - ((unsigned)(SAMPLES_PER_BIT / samples_per_cycle) * samples_per_cycle);
            double ipart;
            double frac = modf(SAMPLES_PER_BIT / samples_per_cycle, &ipart);
            left = samples_per_cycle * frac;
            printf("samples left = %f\n", left);
            printf("leftover = %f\n", leftover);
            last_offset = offset;
            last_freq = freq;
            #if 0
            sf_write_double(sf, (double[]){ -1 }, 1);
            sf_write_double(sf, (double[]){ -1 }, 1);
            sf_write_double(sf, (double[]){ -1 }, 1);
            sf_write_double(sf, (double[]){ -1 }, 1);
            #endif
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
