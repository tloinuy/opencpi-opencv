
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

include $(OCPI_CDK_DIR)/include/util.mk
# Makefile fragment for HDL workers

Model=hdl
SourceSuffix=.v
LibraryAccessTarget=$(if $(findstring xc5,$(1))$(findstring virtex5,$(1)),virtex5,virtex6)
LibraryFileTarget=$(if $(filter virtex5,$(1))$(findstring xc5,$(1)),hdllib.ref,$(LibName).sdbl)
LibraryCleanTargets=$(if $(filter virtex5,$(1))$(findstring xc5,$(1)),hdllib.ref vlg??,*.sdb?)
ifeq ($(origin SourceFiles),undefined)
SourceFiles=$(wildcard *.[vV])
endif
ifndef LibName
LibName=work
else
ifndef InstallDir
InstallDir=$(OCPI_CDK_DIR)/lib/hdl/$(LibName)
$(OCPI_CDK_DIR)/lib/hdl $(OCPI_CDK_DIR)/lib $(InstallDir):
	$(AT)mkdir $@

$(InstallDir): | $(OCPI_CDK_DIR)/lib/hdl
$(OCPI_CDK_DIR)/lib/hdl: | $(OCPI_CDK_DIR)/lib

endif
endif
include $(OCPI_CDK_DIR)/include/hdl/xst.mk

