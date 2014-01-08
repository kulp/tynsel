#ifndef FILTERS_H_
#define FILTERS_H_

enum filter_type {
	FILTER_TYPE_invalid,

	FILTER_TYPE_LOW_PASS,
	FILTER_TYPE_HIGH_PASS,

	FILTER_TYPE_max
};

struct filter_state;

struct filter_state *filter_create(enum filter_type type, double cutoff, unsigned length, unsigned sample_rate, double attenuation);
void filter_put(struct filter_state *s, double input);
double filter_get(struct filter_state *s);
void filter_destroy(struct filter_state *s);

#endif

