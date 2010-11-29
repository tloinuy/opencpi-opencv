
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

#ifndef OCPIOSEVENT_H__
#define OCPIOSEVENT_H__

/**
 * \file
 *
 * \brief A signaling mechanism between threads.
 *
 * Revision History:
 *
 *     06/30/2005 - Frank Pilhofer
 *                  Use 64-bit type for our opaque data, to ensure
 *                  alignment.
 *
 *     04/20/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <OcpiOsDataTypes.h>
#include <string>

namespace OCPI {
  namespace OS {

    /**
     * \brief A signaling mechanism between threads.
     *
     * A signaling mechanism between threads within the same process. An
     * event is a boolean variable that signals an "interesting" event.
     * Zero or more threads can wait (sleep) for an event. If another
     * thread then signals the event by calling set(), a single thread
     * is awakened. If signaled while no thread is waiting, the first
     * thread to call wait() will continue without blocking.
     *
     * A single instance is shared between the threads that want to
     * synchronize.
     */

    class Event {
    public:
      /**
       * Constructor: Initializes the event.
       *
       * \param[in] initial If true, the event is initialized in the signaled
       *               state. Otherwise, the event is initialized in the
       *               unsignaled state.
       * \throw std::string Operating system error creating the event object.
       */

      Event (bool initial = false)
        throw (std::string);

      /**
       * Destructor.
       */

      ~Event ()
        throw ();

      /**
       * \brief Sets the event to the signaled state.
       *
       * \post The event is in the signaled state.
       * \throw std::string Operating system error.
       */

      void set ()
        throw (std::string);

      /**
       * \brief Wait for the event to be signaled.
       *
       * If the event is in the signaled state, resets the event
       * to the unsignaled state, and returns without blocking.
       * If the event is in the unsignaled state, block until
       * another thread sets the event to the signaled state.
       * At that point, all threads that called wait() compete
       * for the event. Only one "winning" thread is allowed
       * to continue, resetting the signal to the unsignaled
       * state.
       *
       * \post The event is in the unsignaled state.
       * \throw std::string Operating system error.
       */

      void wait ()
        throw (std::string);

      /**
       * \brief Timed wait for the event to be signaled.
       *
       * This function has the same semantics as wait() except that
       * it does not wait indefinitely for the event to be signaled
       * but eventually times out.
       *
       * \param[in] timeout The timeout, in milliseconds.
       * \return            true if the event was signaled within
       *                    the timeout period and if this is the
       *                    winning thread (if multiple threads
       *                    were waiting for the event).  false if
       *                    the timeout expired.
       *
       * \post The event is in the unsignaled state.
       * \throw std::string Operating system error.
       */

      bool wait (unsigned int timeout)
        throw (std::string);

    private:
      OCPI::OS::uint64_t m_osOpaque[15];

    private:
      /**
       * Not implemented.
       */

      Event (const Event &);

      /**
       * Not implemented.
       */

      Event & operator= (const Event &);
    };

  }
}

#endif
