
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

// -*- c++ -*-

#ifndef OCPIUTILAUTOWRLOCK_H__
#define OCPIUTILAUTOWRLOCK_H__

/**
 * \file
 * \brief Auto-release a OCPI::OS::RWLock write lock.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <OcpiOsRWLock.h>
#include <string>

namespace OCPI {
  namespace Util {

    /**
     * \brief Auto-release a OCPI::OS::RWLock write lock.
     *
     * Keeps track of the write-locked state of a OCPI::OS::RWLock object.  If
     * the lock is held at the time of destruction, the write lock is released.
     * This helps releasing the lock in all code paths, e.g., when an
     * exception occurs.
     */

    class AutoWRLock {
    public:
      /**
       * Constructor.
       *
       * \param[in] rwlock The lock object to manage.
       * \param[in] locked If true, calls lock().
       *
       * \note Throughout the life time of this object, this thread
       * shall not manipulate \a mutex directly.
       */

      AutoWRLock (OCPI::OS::RWLock & rwlock, bool locked = true)
        throw (std::string);

      ~AutoWRLock ()
        throw ();

      void lock ()
        throw (std::string);

      bool trylock ()
        throw (std::string);

      void unlock ()
        throw (std::string);

    private:
      bool m_locked;
      OCPI::OS::RWLock & m_rwlock;
    };

  }
}

#endif
