
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


#include <OcpiOsDebug.h>
#include <OcpiOsSemaphore.h>
#include <OcpiOsThreadManager.h>
#include "OcpiUtilTest.h"

namespace SemaphoreTests {

  /*
   * ----------------------------------------------------------------------
   * Test 1: Waiting for a semaphore
   * ----------------------------------------------------------------------
   */

  class Test1 : public OCPI::Util::Test::Test {
  public:
    Test1 ()
      : OCPI::Util::Test::Test ("Waiting for semaphore")
    {
    }

    void run ()
    {
      OCPI::OS::Semaphore s; // semaphore value is now 1
      s.wait ();            // semaphore value is now 0
      pass ();
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 2: Posting and waiting for semaphore
   * ----------------------------------------------------------------------
   */

  class Test2 : public OCPI::Util::Test::Test {
  public:
    Test2 ()
      : OCPI::Util::Test::Test ("Posting and waiting for semaphore")
    {
    }

    void run ()
    {
      OCPI::OS::Semaphore s (0); // semaphore value is now 0
      s.post ();                // semaphore value is now 1
      s.wait ();                // semaphore value is now 0
      pass ();
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 3: Attempting to wait for semaphore in another thread
   * ----------------------------------------------------------------------
   */

  struct Test3Data {
    OCPI::OS::Semaphore m;
    bool flag;
  };

  void
  test3Thread (void * opaque)
  {
    Test3Data * t3d = reinterpret_cast<Test3Data *> (opaque);
    t3d->m.wait ();
    t3d->flag = true;
  }

  class Test3 : public OCPI::Util::Test::Test {
  public:
    Test3 ()
      : OCPI::Util::Test::Test ("Attempting to wait for semaphore in another thread")
    {
    }

    void run ()
    {
      Test3Data t3d;    // semaphore value is now 1
      t3d.flag = false;
      t3d.m.wait ();    // semaphore value is now 0
      OCPI::OS::ThreadManager tm (test3Thread, &t3d);
      test (!t3d.flag);
      t3d.m.post ();    // wakes other thread
      tm.join ();       // semaphore value is now 0
      test (t3d.flag);
    }
  };

  /*
   * ----------------------------------------------------------------------
   * Test 4: Post to semaphore with nobody waiting
   * ----------------------------------------------------------------------
   */

  struct Test4Data {
    OCPI::OS::Semaphore m;
    bool flag;
  };

  void
  test4Thread (void * opaque)
  {
    Test4Data * t4d = reinterpret_cast<Test4Data *> (opaque);
    t4d->m.wait ();
    t4d->flag = true;
  }

  class Test4 : public OCPI::Util::Test::Test {
  public:
    Test4 ()
      : OCPI::Util::Test::Test ("Post to semaphore with nobody waiting")
    {
    }

    void run ()
    {
      Test4Data t4d;    // semaphore value is now 1
      t4d.flag = false;
      t4d.m.wait ();    // semaphore value is now 0
      t4d.m.post ();    // semaphore value is now 1 -- and nobody is listening
      OCPI::OS::ThreadManager tm (test4Thread, &t4d);
      tm.join ();       // semaphore value is now 0
      test (t4d.flag);
    }
  };

}

static
int
testSemaphoreInt (int, char *[])
{
  OCPI::Util::Test::Suite tests ("Semaphore tests");
  int n_failed;
  tests.add_test (new SemaphoreTests::Test1);
  tests.add_test (new SemaphoreTests::Test2);
  tests.add_test (new SemaphoreTests::Test3);
  tests.run ();
  n_failed = tests.report ();
  return n_failed;
}

/*
 * Entrypoint for the VxWorks command line.
 */

extern "C" {
  int
  testSemaphore (int argc, char * argv[])
  {
    return testSemaphoreInt (argc, argv);
  }
}

/*
 * Entrypoint for everybody else.
 */

int
main (int argc, char * argv[])
{
#if !defined (NDEBUG)
  {
    for (int i=1; i<argc; i++) {
      if (std::strcmp (argv[i], "--break") == 0) {
        OCPI::OS::debugBreak ();
        break;
      }
    }
  }
#endif

  return testSemaphoreInt (argc, argv);
}
