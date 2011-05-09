/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue Dec 14 15:57:31 2010 EST
 * BASED ON THE FILE: sobel.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: sobel
 */
#include <string.h>
#include "sobel_Worker.h"

// Algorithm specifics:
typedef uint8_t Pixel;      // the data type of pixels
typedef int16_t PixelTemp;  // the data type for intermediate pixel math
#define MAX UINT8_MAX       // the maximum pixel value
#define KERNEL_SIZE 3       // the size of the kernel

// Define state between runs.  Will be initialized to zero for us.
#define LINE_BYTES (p->width * sizeof(Pixel))
#define HISTORY_SIZE KERNEL_SIZE
typedef struct {
  unsigned inLine;                 // The next line# of input I expect
  RCCBuffer buffers[HISTORY_SIZE]; // buffer history (ring)
} SobelState;

static uint32_t sizes[] = {sizeof(SobelState), 0};

SOBEL_METHOD_DECLARATIONS;
RCCDispatch sobel = {
  /* insert any custom initializations here */
  .memSizes = sizes,
  SOBEL_DISPATCH
};

// X derivative
inline static void
doLineX(Pixel *l0, Pixel *l1, Pixel *l2, Pixel *out, unsigned width) {
  unsigned i; // don't depend on c99 yet
  for (i = 1; i < width - 1; i++) {
    PixelTemp t =
      -1 * l0[i-1] + 1 * l0[i+1] +
      -2 * l1[i-1] + 2 * l1[i+1] +
      -1 * l2[i-1] + 1 * l2[i+1];
    if(t < 0) t = -t;
    if(t > MAX) t = MAX;
    out[i] = t;
    // out[i] = t < 0 ? 0 : (t > MAX ? MAX : t);
  }
  out[0] = out[width-1] = 0; // boundary conditions
}

// Y derivative
inline static void
doLineY(Pixel *l0, Pixel *l1, Pixel *l2, Pixel *out, unsigned width) {
  unsigned i; // don't depend on c99 yet
  for (i = 1; i < width - 1; i++) {
    PixelTemp t =
      -1 * l0[i-1] + 1 * l2[i-1] +
      -2 * l0[i]   + 2 * l2[i]   +
      -1 * l0[i+1] + 1 * l2[i+1];
    out[i] = t < 0 ? 0 : (t > MAX ? MAX : t);
  }
  out[0] = out[width-1] = 0; // boundary conditions
}

// Compute one line of output
inline void
doLine(Pixel *l0, Pixel *l1, Pixel *l2, Pixel *out, unsigned width, unsigned xderiv) {
	if(xderiv)
		doLineX(l0, l1, l2, out, width);
	else
		doLineY(l0, l1, l2, out, width);
}

// A run condition for flushing zero-length message
// Basically we only wait for an output buffer, not an input
// This will be unnecessary when we have a separate EOS indication.
static RCCPortMask endMask[] = {1 << SOBEL_OUT, 0};
static RCCRunCondition end = {endMask, 0, 0};

/*
 * Methods to implement for worker sobel, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  SobelProperties *p = self->properties;
  SobelState *s = self->memories[0];
  RCCPort *in = &self->ports[SOBEL_IN],
          *out = &self->ports[SOBEL_OUT];
  const RCCContainer *c = self->container;
  
  (void)timedOut;

  // End state:  just send the zero length message to indicate "done"
  // This will be unnecessary when EOS indication is fixed
  if (self->runCondition == &end) {
    out->output.length = 0;
    c->advance(out, 0);
    return RCC_DONE;
  }

	// Current buffer
	unsigned cur = s->inLine % HISTORY_SIZE;

	// First line: do nothing
	// Second line
	if(s->inLine == 1) {
		memset(out->current.data, 0, LINE_BYTES);
    out->output.length = LINE_BYTES;
		c->advance(out, LINE_BYTES);
	}
	// Middle line
	else if(s->inLine > 1) {
		doLine(s->buffers[(cur - 2 + HISTORY_SIZE) % HISTORY_SIZE].data,
			s->buffers[(cur - 1 + HISTORY_SIZE) % HISTORY_SIZE].data,
			in->current.data,
			out->current.data,
			p->width,
			p->xderiv);
    out->output.length = LINE_BYTES;
		c->advance(out, LINE_BYTES);
	}

	// Go to next
	unsigned prev = (cur - 2 + HISTORY_SIZE) % HISTORY_SIZE;
	if(s->inLine < HISTORY_SIZE - 1)
		c->take(in, NULL, &s->buffers[cur]);
	else
		c->take(in, &s->buffers[prev], &s->buffers[cur]);
	s->inLine++;

  // Arrange to send the zero-length message after the last line of last image
  // This will be unnecessary when EOS indication is fixed
  if (in->input.length == 0) {
    self->runCondition = &end;
    *newRunCondition = 1;
  }
  return RCC_OK;
}
