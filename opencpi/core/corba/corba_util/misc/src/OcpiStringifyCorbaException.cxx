
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


#include <string>
#include <corba.h>
#include <OcpiUtilMisc.h>
#include "OcpiStringifyCorbaException.h"

#if defined (OCPI_USES_TAO)
#include <sstream>
#endif

std::string
OCPI::CORBAUtil::Misc::
stringifyCorbaException (const CORBA::Exception & ex)
  throw ()
{
  std::string res;

  const CORBA::SystemException * sysex =
    CORBA::SystemException::_downcast (&ex);

  if (sysex) {
    res = sysex->_name ();
    res += " (";

    switch (sysex->completed()) {
    case CORBA::COMPLETED_YES:
      res += "COMPLETED_YES";
      break;

    case CORBA::COMPLETED_NO:
      res += "COMPLETED_NO";
      break;

    case CORBA::COMPLETED_MAYBE:
      res += "COMPLETED_MAYBE";
      break;

    default:
      res += "COMPLETED_???";
      break;
    }

    /*
     * 20 high order bits of the minor code contain the "Vendor Minor
     * Codeset Id". The lower 12 bits contain the "real" minor code.
     */

    CORBA::ULong vmcid = sysex->minor() >> 12;
    CORBA::ULong minorCode = sysex->minor() & 0xfff;

    if (vmcid == 0x4f4d0) { /* OMG VMCID */
      res += ", OMG VMCID";
    }
    else if (vmcid) {
      res += ",";

      /*
       * Recognize some known values for the VMCID.
       * See http://doc.omg.org/vendor-tags
       */

      if (vmcid == 0x50540 || vmcid == 0x58500) {
        res += " <PrismTech>";
      }
      else if (vmcid == 0x4f490 || vmcid == 0x4f495) {
        res += " <OIS>";
      }
      else if (vmcid == 0x54410) {
        res += " <TAO>";
      }

      res += " VMCID 0x";
      res += OCPI::Util::Misc::unsignedToString ((unsigned)vmcid, 16);
    }

    res += ", minor code ";
    res += OCPI::Util::Misc::unsignedToString ((unsigned)minorCode);
    res += ")";

    return res;
  }

  res = ex._rep_id ();

#if defined (OCPI_USES_TAO)
  std::ostringstream exInfoStream;
  exInfoStream << ex;

  res += " (";
  res += exInfoStream.str ();
  res += ")";
#endif

  return res;
}
