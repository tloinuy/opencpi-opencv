
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef RCC_WORKER_FILESOURCE_H__
#define RCC_WORKER_FILESOURCE_H__

/*
 * Header file for worker FileSource.
 * Generated at 2009-06-23 09:04:52
 * from FileSource.scd.xml
 */

#include <RCC_Worker.h>

#if defined (__cplusplus)
extern "C" {
#endif

/*
 * Worker port ordinals.
 */

enum FileSourcePortOrdinal {
  FILESOURCE_DATAOUT
};

#define FILESOURCE_N_INPUT_PORTS 0
#define FILESOURCE_N_OUTPUT_PORTS 1

/*
 * Worker property space.
 */

typedef struct {
  RCCChar portName[256];
  RCCChar fileName[256];
  uint32_t bytesPerPacket;
  uint32_t offset;
  uint32_t errnoValue;
  RCCBoolean atEof;
  RCCBoolean verbose;
} FileSourceProperties;

#if defined (__cplusplus)
}
#endif
#endif
