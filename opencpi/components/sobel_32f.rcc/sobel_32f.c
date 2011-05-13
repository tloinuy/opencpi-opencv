/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri May 6 03:40:31 2011 EDT
 * BASED ON THE FILE: sobel_32f.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: sobel_32f
 */
#include "sobel_32f_Worker.h"

#include <string.h>
#include <stdlib.h>

typedef uint8_t Pixel;      // the data type of pixels

SOBEL_32F_METHOD_DECLARATIONS;
RCCDispatch sobel_32f = {
  /* insert any custom initializations here */
  SOBEL_32F_DISPATCH
};

// X derivative
inline static void
doLineX(Pixel *l0, Pixel *l1, Pixel *l2, float *out, unsigned width) {
  unsigned i; // don't depend on c99 yet
  for (i = 1; i < width - 1; i++) {
    float t = (float)
      -1 * l0[i-1] + 1 * l0[i+1] +
      -2 * l1[i-1] + 2 * l1[i+1] +
      -1 * l2[i-1] + 1 * l2[i+1];
    out[i] = t;
  }
  out[0] = out[width-1] = 0; // boundary conditions
}

// Y derivative
inline static void
doLineY(Pixel *l0, Pixel *l1, Pixel *l2, float *out, unsigned width) {
  unsigned i; // don't depend on c99 yet
  for (i = 1; i < width - 1; i++) {
    float t = (float)
      -1 * l0[i-1] + 1 * l2[i-1] +
      -2 * l0[i]   + 2 * l2[i]   +
      -1 * l0[i+1] + 1 * l2[i+1];
    out[i] = t;
  }
  out[0] = out[width-1] = 0; // boundary conditions
}

unsigned lineAt = 0;
Pixel *img = NULL;

static unsigned 
update(int H, int W, Pixel *src, float *dst, unsigned lines, unsigned xderiv) {
  if( !img ) {
    img = malloc(H * W * sizeof(Pixel));
  }

  unsigned i, j;
  for( i = 0; i < lines; i++ ) {
    int at = (lineAt + i) * W;
    int src_at = i * W;
    for( j = 0; j < W; j++ )
      img[at+j] = src[src_at+j];
  }

  unsigned produced = 0;
  for( i = 0; i < lines; i++ ) {
    int at = lineAt + i;
    // at = 0 begin (zero lines)
    if( at == 1 ) {
      // first line
      memset(dst, 0, sizeof(float) * W);
      dst += W;
      produced++;
    }
    else if( at > 1 ) {
      if( xderiv )
        doLineX( img - W, img, img + W, dst, W );
      else
        doLineY( img - W, img, img + W, dst, W );
      dst += W;
      produced++;
    }

    if( at == H - 1 ) {
      // end (two lines)
      memset(dst, 0, sizeof(float) * W);
      dst += W;
      produced++;
    }
  }
  return produced;
}


/*
 * Methods to implement for worker sobel_32f, based on metadata.
*/

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  ( void ) timedOut;
  ( void ) newRunCondition;
  Sobel_32fProperties *p = self->properties;
  RCCPort *in = &self->ports[SOBEL_32F_IN],
          *out = &self->ports[SOBEL_32F_OUT];
  const RCCContainer *c = self->container;

  // update and produce gradients
  unsigned lines = in->input.length / p->width;
  unsigned produced = update( p->height, p->width, in->current.data,
          (float *) out->current.data, lines, p->xderiv );

  lineAt += lines;
  // next image
  if( lineAt == p->height ) {
    lineAt = 0;
  }

  printf(">>> PRODUCED: %d\n", produced);
  printf(">>> H, W: %d %d\n", p->height, p->width);

  out->output.u.operation = in->input.u.operation;
  out->output.length = produced * p->width * sizeof(float);
  return RCC_ADVANCE;
}
