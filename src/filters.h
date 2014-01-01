#ifndef FILTERS_H_
#define FILTERS_H_

enum filter_ident {
	FILTER_invalid,

	FILTER_PASS_CHAN0,
	FILTER_PASS_CHAN0BIT0,
	FILTER_PASS_CHAN0BIT1,

	FILTER_PASS_CHAN1,
	FILTER_PASS_CHAN1BIT0,
	FILTER_PASS_CHAN1BIT1,

	FILTER_max
};

struct filter_state;

struct filter_entry {
	enum filter_ident ident;
	int tapcount;
	double *taps;
};

struct filter_state *filter_create(enum filter_ident which);
void filter_put(struct filter_state *s, double input);
double filter_get(struct filter_state *s);
void filter_destroy(struct filter_state *s);

#endif

