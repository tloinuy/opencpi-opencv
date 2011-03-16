/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Mar 08 14:37:21 2011 EST
 * BASED ON THE FILE: canny.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: canny
 */
#include <math.h>
#include <string.h>
#include "canny_Worker.h"

// Algorithm specifics:
typedef uint8_t Pixel;      // the data type of pixels
typedef double PixelTemp;  // the data type for intermediate pixel math
#define MAX UINT8_MAX       // the maximum pixel value

// Define state between runs.  Will be initialized to zero for us.
#define LINE_BYTES (p->width * sizeof(Pixel))
#define HISTORY_SIZE 3
typedef struct {
  unsigned inLine;                 // The next line# of input I expect
  RCCBuffer buffers[HISTORY_SIZE]; // buffer history (ring)
} CannyState;

static uint32_t sizes[] = {sizeof(CannyState), 0};

CANNY_METHOD_DECLARATIONS;
RCCDispatch canny = {
  /* insert any custom initializations here */
	.memSizes = sizes,
  CANNY_DISPATCH
};

// TODO
// - add variables here

// Compute one line of output
inline void
doLine(Pixel *l0, Pixel *l1, Pixel *l2, Pixel *out, unsigned width) {
  // TODO
}

// A run condition for flushing zero-length message
// Basically we only wait for an output buffer, not an input
// This will be unnecessary when we have a separate EOS indication.
static RCCPortMask endMask[] = {1 << CANNY_OUT, 0};
static RCCRunCondition end = {endMask, 0, 0};

/*
 * Methods to implement for worker canny, based on metadata.
*/

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  CannyProperties *p = self->properties;
  CannyState *s = self->memories[0];
  RCCPort *in = &self->ports[CANNY_IN],
          *out = &self->ports[CANNY_OUT];
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

	// First line: init
  if(s->inLine == 0) {
    initKernel(p->sigmaX, p->sigmaY);
  }
	// Second line
	else if(s->inLine == 1) {
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
			p->width);
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
