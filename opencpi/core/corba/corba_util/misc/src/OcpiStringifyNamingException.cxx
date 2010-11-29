
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
#include "OcpiStringifyNamingException.h"

#if defined (OCPI_USES_TAO)
#include <sstream>
#endif

std::string
OCPI::CORBAUtil::Misc::
stringifyNamingException (const CORBA::Exception & ex,
                          CosNaming::NamingContext_ptr ns)
  throw ()
{
  std::string reason;

  const CosNaming::NamingContext::NotFound * nf =
    CosNaming::NamingContext::NotFound::_downcast (&ex);
  const CosNaming::NamingContext::CannotProceed * cp =
    CosNaming::NamingContext::CannotProceed::_downcast (&ex);

  if (nf) {
    reason = "Not found, ";

    if (nf->why == CosNaming::NamingContext::missing_node) {
      reason += "missing node";
    }
    else if (nf->why == CosNaming::NamingContext::not_context) {
      reason += "object where context was expected";
    }
    else if (nf->why == CosNaming::NamingContext::not_object) {
      reason += "context where object was expected";
    }

    CosNaming::NamingContextExt_var nc =
      CosNaming::NamingContextExt::_narrow (ns);

    if (!CORBA::is_nil (nc) && nf->rest_of_name.length()) {
      CORBA::String_var rest;

      try {
        rest = nc->to_string (nf->rest_of_name);
      }
      catch (...) {
        rest = const_cast<const char *> ("(unknown)");
      }

      reason += ", remaining name \"";
      reason += rest.in();
      reason += "\"";
    }
  }
  else if (cp) {
    reason = "Cannot proceed";

    CosNaming::NamingContextExt_var nc =
      CosNaming::NamingContextExt::_narrow (ns);

    if (!CORBA::is_nil (nc) && cp->rest_of_name.length()) {
      CORBA::String_var rest;

      try {
        rest = nc->to_string (cp->rest_of_name);
      }
      catch (...) {
        rest = const_cast<const char *> ("(unknown)");
      }

      reason += ", remaining name \"";
      reason += rest.in();
      reason += "\"";
    }
  }
  else if (CosNaming::NamingContext::InvalidName::_downcast (&ex)) {
    reason = "Invalid name";
  }
  else {
    reason = stringifyCorbaException (ex);
  }

  return reason;
}

