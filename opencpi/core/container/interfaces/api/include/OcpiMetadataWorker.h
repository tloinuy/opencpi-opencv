
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

/*

 Definitions for worker metadata encoding,decoding.
 Offline, in tools, this information is encoded into a string format 
 suitable for program level arguments (argv).  All properties are encoded into
 a single string.  This relieves runtime of any parsing overhead (time or space)
 or dependencies on external parsing libraries.

 The "scope" of this property support is configuration properties for CP289
 components.  Thus it is not (yet) intended to support SCA GPP components.

 This file defines the binary (non-string) format of SCA component properties,
 as well as the functions to encode (binary to string) and decode 
 (string to binary).

 
*/
#ifndef OCPI_WORKER_METADATA_H
#define OCPI_WORKER_METADATA_H

#include <OcpiOsAssert.h>
#include <OcpiUtilException.h>
#include <OcpiUtilProperty.h>
#include "ezxml.h"

#define OCPI_CONTROL_OPS                                                        \
  CONTROL_OP(initialize,     Initialize,     INITIALIZED, EXISTS,      NONE,        NONE) \
  CONTROL_OP(start,          Start,          OPERATING,   SUSPENDED,   INITIALIZED, NONE) \
  CONTROL_OP(stop,           Stop,           SUSPENDED,   OPERATING,   NONE,        NONE) \
  CONTROL_OP(release,        Release,        EXISTS,      INITIALIZED, OPERATING,   SUSPENDED) \
  CONTROL_OP(beforeQuery,    BeforeQuery,    NONE,        INITIALIZED, OPERATING,   SUSPENDED) \
  CONTROL_OP(afterConfigure, AfterConfigure, NONE,        INITIALIZED, OPERATING,   SUSPENDED) \
  CONTROL_OP(test,           Test,           NONE,        INITIALIZED, NONE,        NONE) \
  /**/

namespace OCPI {
  namespace Metadata {

    class Port {
      friend class Worker;
      // Describe a port
      bool decode(ezxml_t x, int pid);
    public:
      const char *   name;
      int            m_pid;
      bool           provider;
      bool           twoway;


      static const unsigned
        DEFAULT_NBUFFERS = 1,
        DEFAULT_BUFFER_SIZE = 2*1024;
      uint32_t minBufferSize;
      uint32_t minBufferCount;
      uint32_t maxBufferSize;

      ezxml_t        myXml;

    Port(bool prov=true): name(NULL),m_pid(0),provider(prov),twoway(false), minBufferSize(DEFAULT_BUFFER_SIZE), minBufferCount(1), maxBufferSize(0), myXml(0){}

    };

    class Test {
      friend class Worker;
      unsigned int testId;
      unsigned int numInputs, numResults;
      unsigned int * inputValues;  // reference to property[n]
      unsigned int * resultValues;
    };

    // Dealing with worker meta as a bundle
    typedef OCPI::Util::Prop::Property Property;
    class Worker {
      Property *myProps;
      Port *myPorts;
      Test *myTests;
      unsigned nProps, nPorts, nTests, size;
      Test &findTest(unsigned int testId);
    public:
      inline Property *getProperties(unsigned &np) {
        np = nProps;
        return myProps;
      }
      inline Property &
        property(unsigned long which)
      {
        ocpiAssert(which < nProps);
        return myProps[which];
      }
      Port *findPort(const char *id);
      inline Port & 
        port(unsigned long which) 
      {
        ocpiAssert(which < nPorts);
        return myPorts[which];
      }
      inline Port* getPorts ( unsigned int& n_ports )
      {
        n_ports = nPorts;
        return myPorts;
      }
      enum ControlState {
        EXISTS,
        INITIALIZED,
        OPERATING,
        SUSPENDED,
        UNUSABLE,
        NONE
      };
#define CONTROL_OP(x, c, t, s1, s2, s3)  Op##c,
      enum Ops {
      OCPI_CONTROL_OPS
      OpsLimit
      };
#undef CONTROL_OP
      Property &findProperty(const char *id);
      unsigned whichProperty(const char *id);
      //      Worker(const char *workerMetadata);
      Worker(ezxml_t xml);
      ~Worker();
    };
  }
#if 0
  namespace Container {
    class ApiError : public OCPI::Util::EmbeddedException {
    public:
      ApiError(const char *err, ...);
    };
  }
#endif
}
#endif
