
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

#include <OcpiUtilZipFs.h>
#include <OcpiUtilFileFs.h>
#include <OcpiOsFileSystem.h>
#include <iostream>
#include <iomanip>
#include <string>

void
copyFilesRecursively (OCPI::Util::Vfs::Vfs * fs1, const std::string & name1,
                      OCPI::Util::Vfs::Vfs * fs2, const std::string & name2)
{
  OCPI::Util::Vfs::Iterator * it = fs1->list (name1);

  while (!it->end()) {
    std::string targetName =
      OCPI::Util::Vfs::joinNames (name2, it->relativeName());

    if (it->isDirectory()) {
      std::cout << "Recursing into "
                << targetName
                << "/"
                << std::endl;

      copyFilesRecursively (fs1, it->absoluteName(),
                            fs2, targetName);

      std::cout << "Done with "
                << targetName
                << "/"
                << std::endl;
    }
    else {
      std::cout << "Copying "
                << targetName
                << " (" << it->size() << " bytes) ... "
                << std::flush;
      fs1->copy (it->absoluteName(), fs2, targetName);
      std::cout << "done." << std::endl;
    }

    it->next ();
  }

  fs1->closeIterator (it);
}

int
main (int argc, char *argv[])
{
  if (argc != 3) {
    std::cout << "usage: " << argv[0] << " <zipfile> <pattern>" << std::endl;
    return 1;
  }

  OCPI::Util::FileFs::FileFs localFs ("/");
  std::string zipName = OCPI::OS::FileSystem::fromNativeName (argv[1]);
  std::string absZipName = OCPI::OS::FileSystem::absoluteName (zipName);
  std::string patName = OCPI::OS::FileSystem::fromNativeName (argv[2]);
  std::string absPatName = OCPI::OS::FileSystem::absoluteName (patName);
  std::string patDirName = OCPI::OS::FileSystem::directoryName (absPatName);
  std::string patRelName = OCPI::OS::FileSystem::relativeName (absPatName);
  OCPI::Util::ZipFs::ZipFs zipFs;

  std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out;

  if (localFs.exists (absZipName)) {
    std::cout << "Using " << absZipName << std::endl;
  }
  else {
    std::cout << "Creating " << absZipName << std::endl;
    mode |= std::ios_base::trunc;
  }

  try {
    zipFs.openZip (&localFs, absZipName, mode);
  }
  catch (const std::string & oops) {
    std::cout << "error: " << oops << std::endl;
    return 1;
  }

  OCPI::Util::Vfs::Iterator * toAdd = localFs.list (patDirName, patRelName);

  try {
    while (!toAdd->end()) {
      if (toAdd->isDirectory()) {
        std::cout << "Recursing into "
                  << toAdd->relativeName()
                  << "/"
                  << std::endl;
        copyFilesRecursively (&localFs,
                              toAdd->absoluteName(),
                              &zipFs,
                              toAdd->relativeName());
        std::cout << "Done with "
                  << toAdd->relativeName()
                  << "/"
                  << std::endl;
      }
      else {
        std::cout << "Copying "
                  << toAdd->relativeName()
                  << " (" << toAdd->size() << " bytes) ... "
                  << std::flush;
        localFs.copy (toAdd->absoluteName(),
                      &zipFs,
                      toAdd->relativeName());
        std::cout << "done." << std::endl;
      }

      toAdd->next ();
    }
  }
  catch (const std::string & oops) {
    std::cout << "oops: " << oops << std::endl;
  }

  localFs.closeIterator (toAdd);
  return 0;
}
