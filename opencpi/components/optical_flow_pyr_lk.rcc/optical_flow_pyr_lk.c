/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri May 6 03:38:44 2011 EDT
 * BASED ON THE FILE: optical_flow_pyr_lk.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: optical_flow_pyr_lk
 */
#include "optical_flow_pyr_lk_Worker.h"

OPTICAL_FLOW_PYR_LK_METHOD_DECLARATIONS;
RCCDispatch optical_flow_pyr_lk = {
  /* insert any custom initializations here */
  OPTICAL_FLOW_PYR_LK_DISPATCH
};

/*
 * Methods to implement for worker optical_flow_pyr_lk, based on metadata.
*/

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  return RCC_ADVANCE;
}
