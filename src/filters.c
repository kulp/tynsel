#include "filters.h"
#include <errno.h>
#include <stdlib.h>

struct filter_state {
    const struct filter_entry *entry;
    double *history;
    int last_index;
};

// Use :AlignCtrl Irlp0P0 for aligning taps
static struct filter_entry filters[] = {
    [FILTER_PASS_CHAN0] = {
        .ident = FILTER_PASS_CHAN0,
        .tapcount = 27,
        .taps = (double[27]){
            -0.0056015351311259325,
            -0.029637140108221473 ,
            -0.04703242493634984  ,
            -0.047212665488529565 ,
            -0.01673263372785522  ,
             0.025956777608056633 ,
             0.04395124859339777  ,
             0.013002854747559788 ,
            -0.04631079600871891  ,
            -0.07303225161865606  ,
            -0.012505259548689938 ,
             0.13042561783370019  ,
             0.2813726462298767   ,
             0.3459148430740437   ,
             0.2813726462298767   ,
             0.13042561783370019  ,
            -0.012505259548689938 ,
            -0.07303225161865606  ,
            -0.04631079600871891  ,
             0.013002854747559788 ,
             0.04395124859339777  ,
             0.025956777608056633 ,
            -0.01673263372785522  ,
            -0.047212665488529565 ,
            -0.04703242493634984  ,
            -0.029637140108221473 ,
            -0.0056015351311259325
        },
    },
    [FILTER_PASS_CHAN0BIT0] = {
        .ident = FILTER_PASS_CHAN0BIT0,
        .tapcount = 27,
        .taps = (double[27]){
            -0.0007124926963727803,
             0.027958179571406967 ,
             0.05243587160917998  ,
             0.07557980227488914  ,
             0.07882392266463276  ,
             0.05259743344683504  ,
             0.0026436265570192463,
            -0.04835998030749869  ,
            -0.0695630046200249   ,
            -0.0380640494339432   ,
             0.04634635446285935  ,
             0.15685603783775337  ,
             0.25008406007446965  ,
             0.2864731120471816   ,
             0.25008406007446965  ,
             0.15685603783775337  ,
             0.04634635446285935  ,
            -0.0380640494339432   ,
            -0.0695630046200249   ,
            -0.04835998030749869  ,
             0.0026436265570192463,
             0.05259743344683504  ,
             0.07882392266463276  ,
             0.07557980227488914  ,
             0.05243587160917998  ,
             0.027958179571406967 ,
            -0.0007124926963727803
        },
    },
    [FILTER_PASS_CHAN0BIT1] = {
        .ident = FILTER_PASS_CHAN0BIT1,
        .tapcount = 27,
        .taps = (double[27]){
            -0.005023284276188728,
            -0.09387216495974861 ,
             0.14741922267749277 ,
             0.002325511923249027,
            -0.05179316927195578 ,
            -0.052481601255380594,
            -0.014314971650090736,
             0.037462189543502764,
             0.06856431941370533 ,
             0.047392566350220884,
            -0.03491304510338438 ,
            -0.15288504135926237 ,
            -0.25744932891159283 ,
             0.7009867058756108  ,
            -0.25744932891159283 ,
            -0.15288504135926237 ,
            -0.03491304510338438 ,
             0.047392566350220884,
             0.06856431941370533 ,
             0.037462189543502764,
            -0.014314971650090736,
            -0.052481601255380594,
            -0.05179316927195578 ,
             0.002325511923249027,
             0.14741922267749277 ,
            -0.09387216495974861 ,
            -0.005023284276188728
        },
    },
};

// The following functions mangled from sources produced by
// http://t-filter.appspot.com/fir/index.html
struct filter_state *filter_create(enum filter_ident which) {
    if (which <= FILTER_invalid || which >= FILTER_max) {
        errno = EINVAL;
        return NULL;
    }

    struct filter_state *s = malloc(sizeof *s);
    const struct filter_entry *e = s->entry = &filters[which];
    s->history = malloc(e->tapcount * sizeof *e->taps);
    for (int i = 0; i < e->tapcount; i++)
        s->history[i] = 0;

    s->last_index = 0;

    return s;
}

void filter_put(struct filter_state *s, double input) {
    s->history[s->last_index++] = input;
    if (s->last_index == s->entry->tapcount)
        s->last_index = 0;
}

double filter_get(struct filter_state *s) {
    double acc = 0;
    int index = s->last_index;
    const struct filter_entry *e = s->entry;
    for (int i = 0; i < e->tapcount; ++i) {
        index = index != 0 ? index - 1 : e->tapcount - 1;
        acc += s->history[index] * e->taps[i];
    };

    return acc;
}

void filter_destroy(struct filter_state *s) {
    free(s->history);
    free(s);
}

