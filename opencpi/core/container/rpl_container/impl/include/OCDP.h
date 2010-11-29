
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

#ifndef OCDP_H
#define OCDP_H
struct OcdpProperties {
  uint32_t
  nLocalBuffers,        // 0x00
    nRemoteBuffers,        // 0x04
    localBufferBase,        // 0x08
    localMetadataBase,        // 0x0c
    localBufferSize,        // 0x10
    localMetadataSize,        // 0x14
    nRemoteDone,        // 0x18 written indicating remote action on local buffers
    rsvd,                // 0x1c
    nReady;             // 0x20 read by remote to know local buffers for remote action
  const uint32_t
    foodFace,                // 0x24 constant 0xf00dface
    debug[9];                // 0x28/2c/30/34/38/3c/40/44/48
  uint32_t
    memoryBytes,        // 0x4c
    remoteBufferBase,        // 0x50
    remoteMetadataBase, // 0x54
    remoteBufferSize,   // 0x58
    remoteMetadataSize, // 0x5c
    remoteFlagBase,     // 0x60
    remoteFlagPitch,    // 0x64
    control,            // 0x68
    flowDiagCount;      // 0x6c
};
#define OCDP_CONTROL_DISABLED 0
#define OCDP_CONTROL_PRODUCER 1
#define OCDP_CONTROL_CONSUMER 2
#define OCDP_CONTROL(dir, role) (((dir) << 2) | (role))
#define OCDP_LOCAL_BUFFER_ALIGN 16     // The constraint on the alignment of local buffers
#define OCDP_FAR_BUFFER_ALIGN 4        // The constraint on the alignment of far buffers

struct OcdpMetadata {
  uint32_t
    length,
    opCode,
    tag,
    interval;
};
#define OCDP_METADATA_SIZE sizeof(OcdpMetadata)
enum OcdpRole {
  OCDP_PASSIVE = 0,
  OCDP_ACTIVE_MESSAGE = 1,
  OCDP_ACTIVE_FLOWCONTROL = 2
};

#endif
