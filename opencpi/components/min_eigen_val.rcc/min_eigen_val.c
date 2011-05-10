/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue May 10 01:55:32 2011 EDT
 * BASED ON THE FILE: min_eigen_val.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: min_eigen_val
 */
#include "min_eigen_val_Worker.h"

MIN_EIGEN_VAL_METHOD_DECLARATIONS;
RCCDispatch min_eigen_val = {
  /* insert any custom initializations here */
  MIN_EIGEN_VAL_DISPATCH
};

/*
 * Methods to implement for worker min_eigen_val, based on metadata.
*/

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  return RCC_ADVANCE;
}
