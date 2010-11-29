
# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #

This sdk directory contains tools and makefile scripts that comprise
the sdk.

The "export" subdirectory is a directory of hand-create
simlinks, NOT SOURCE FILES, that can be tar'd to export the sdk.

The top level "include" directory contains make scripts, and other
constant files.  Tools are in their own subdirectories built normally,
starting with ocpigen.  The target directory names for the sdk are
based on the `uname -s`-`uname -m` formula, while the overall OCPI
build files use things like linux-bin and macos-bin.  The export link
tree converts one to the other.

Thus the export tree is expected to be copied into a real
"installation" tree that might include stuff build from other areas.

Adding support for an authoring model:

Things are generally structured so that adding an authoring model is adding a directory in certain areas and populating those directories.
Some things are not ideal...

For "make" support, add the include scripts include/mmm/*.mk.
For model-specific include/header files, put them in include/mmm also.

For now, new models must be added to include/lib.mk also.

The ocpigen tool is partially, though not cleanly modular for adding authoring models.
TODO: the ocpigen tool will have a more open "framework" so that it is cleaner to add authoring models.



