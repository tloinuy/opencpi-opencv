/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue May 10 01:54:59 2011 EDT
 * BASED ON THE FILE: corner_eigen_vals_vecs.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: corner_eigen_vals_vecs
 */
#include "corner_eigen_vals_vecs_Worker.h"

CORNER_EIGEN_VALS_VECS_METHOD_DECLARATIONS;
RCCDispatch corner_eigen_vals_vecs = {
  /* insert any custom initializations here */
  CORNER_EIGEN_VALS_VECS_DISPATCH
};

/*
 * Methods to implement for worker corner_eigen_vals_vecs, based on metadata.
*/

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  return RCC_ADVANCE;
}
