/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue May 10 01:56:18 2011 EDT
 * BASED ON THE FILE: good_features_to_track.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: good_features_to_track
 */
#include "good_features_to_track_Worker.h"

GOOD_FEATURES_TO_TRACK_METHOD_DECLARATIONS;
RCCDispatch good_features_to_track = {
  /* insert any custom initializations here */
  GOOD_FEATURES_TO_TRACK_DISPATCH
};

/*
 * Methods to implement for worker good_features_to_track, based on metadata.
*/

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  return RCC_ADVANCE;
}
