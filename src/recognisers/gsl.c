/*
 * Copyright (C) 2012-                        Darren Kulp (http://kulp.ch/)
 * Copyright (C) 1996, 1997, 1998, 1999, 2000 Brian Gough
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "decode.h"
#include "common.h"

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>

#include <gsl/gsl_multifit.h>

#define PERBIN          ((double)s->audio.sample_rate / size)

// fit_degree is ((degree of the equation) + 1)
static const unsigned fit_degree = 3;

static double evaluate(int vecsize, gsl_vector *vec, double x)
{
    double result = 0.;

    for (int i = 0; i < vecsize; i++)
        result += gsl_vector_get(vec, i) * pow(x, i);

    return result;
}

int decode_bit_gsl(struct decode_state *s, size_t size, const double samples[size], int *channel, int *bit, double *prob)
{
    unsigned minch = 0,
             maxch = 1;

    // If we were given a channel, contrain to that one
    if (*channel >= 0 && *channel < 2)
        minch = maxch = *channel;

    double maxprob = 0;
    for (unsigned ch = minch; ch <= maxch; ch++) {
        double freq[] = { s->audio.freqs[ch][0], s->audio.freqs[ch][1] };

        size_t minbucket = (freq[0] + PERBIN / 2) / PERBIN - 1,
               maxbucket = (freq[1] + PERBIN / 2) / PERBIN + 1;

        unsigned n = maxbucket - minbucket + 1;

        // Adapted from doc/examples/fitting2.c in the GSL 1.9 distribution
        // Thus this file is under the GNU GPL (probably : the original file has
        // no specific COPYING information).
        gsl_vector *c = gsl_vector_alloc(fit_degree);
        {
            gsl_matrix *X = gsl_matrix_alloc(n, fit_degree);
            gsl_vector *y = gsl_vector_alloc(n);
            gsl_vector *w = gsl_vector_alloc(n);

            // XXX not sure what the parameters to the covariant alloc should
            // be
            gsl_matrix *cov = gsl_matrix_alloc(fit_degree, fit_degree);

            for (unsigned i = 0; i < n; i++) {
                unsigned ii = minbucket + i;
                double xi = ii * PERBIN;
                double yi = samples[ii];
                double ei = yi / 100.; // XXX use a real error if we can compute one

                for (unsigned p = 0; p < fit_degree; p++)
                    gsl_matrix_set(X, i, p, pow(xi, p));

                gsl_vector_set(y, i, yi);
                gsl_vector_set(w, i, 1. / (ei * ei));
            }

            double chisq;
            gsl_multifit_linear_workspace *work = gsl_multifit_linear_alloc(n, fit_degree);
            gsl_multifit_wlinear(X, w, y, c, cov, &chisq, work);
            gsl_multifit_linear_free(work);

            gsl_matrix_free(X);
            gsl_vector_free(y);
            gsl_vector_free(w);
            gsl_matrix_free(cov);
        }

        double y[] = { evaluate(fit_degree, c, freq[0]), evaluate(fit_degree, c, freq[1]) };
        int idx = y[1] > y[0];
        double tprob = (y[idx] - y[1 - idx]) / y[idx];
        if (tprob > maxprob) {
            *channel = ch;
            *prob = maxprob = tprob;
            *bit = idx;
        }

        gsl_vector_free(c);
    }

    return 0;
}
