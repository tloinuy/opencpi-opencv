/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue April 26 01:55:32 2011 EDT
 * BASED ON THE FILE: min_eigen_val.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: min_eigen_val
 */
#include "min_eigen_val_Worker.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

MIN_EIGEN_VAL_METHOD_DECLARATIONS;
RCCDispatch min_eigen_val = {
  /* insert any custom initializations here */
  MIN_EIGEN_VAL_DISPATCH
};

static void
calc_min_eigen_val( int H, int W, const float* _cov, float* _dst )
{
    int i, j;
    // Size size = _cov.size();

    for( i = 0; i < H; i++ )
    {
        // const float* cov = (const float*)(_cov.data + _cov.step*i);
        // float* dst = (float*)(_dst.data + _dst.step*i);
        const float* cov = (const float*) (_cov + i*W*3);
        float* dst = (float*) (_dst + i*W);
        for( j = 0; j < W; j++ )
        {
            double a = cov[j*3]*0.5;
            double b = cov[j*3+1];
            double c = cov[j*3+2]*0.5;
            dst[j] = (float)((a + c) - sqrt((a - c)*(a - c) + b*b));
        }
    }
}


/*
 * Methods to implement for worker min_eigen_val, based on metadata.
*/

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  ( void ) timedOut;
  ( void ) newRunCondition;
  Min_eigen_valProperties *p = self->properties;
  RCCPort *in = &self->ports[MIN_EIGEN_VAL_IN],
          *out = &self->ports[MIN_EIGEN_VAL_OUT];
  const RCCContainer *c = self->container;

  calc_min_eigen_val( p->height, p->width,
                    (float *) in->current.data,
                    (float *) out->current.data);

  out->output.u.operation = in->input.u.operation;
  out->output.length = p->height * p->width * sizeof(float);

  return RCC_ADVANCE;
}
