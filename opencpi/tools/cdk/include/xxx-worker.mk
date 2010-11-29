
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

# Makefile fragment for all workers
# TODO:  implement a library search path so only library names can be used.
include $(OCPI_CDK_DIR)/include/util.mk
# Default is that you are building in a subdirectory of all implementations
ifneq ($(Worker),)
ifneq ($(Workers),)
$(error Cannot set both Worker and Workers variables in Makefile)
else
Workers=$(Worker)
endif
else
ifeq ($(Workers),)
Worker=$(basename $(CwdName))
$(info Worker name is $(Worker) from its directory name $(CwdName))
Workers=$(Worker)
endif
endif
GeneratedDir=$(OutDir)gen
TargetDir=$(OutDir)target-$(Target)
ImplXmlFiles=$(Workers:%=%.xml)
ifndef Application
ImplHeaderFiles=$(foreach w,$(Workers),$(GeneratedDir)/$(w)$(ImplSuffix))
SkelFiles=$(foreach w,$(Workers),$(GeneratedDir)/$(w)$(SkelSuffix))
ifeq ($(origin WorkerSourceFiles),undefined)
WorkerSourceFiles=$(foreach w,$(Workers),$(w)$(SourceSuffix))
endif
AuthoredSourceFiles=$(sort $(SourceFiles) $(WorkerSourceFiles))
endif
ifndef BinaryName
BinaryName=$(word 1,$(Workers))
endif
BinaryFile=$(TargetDir)/$(BinaryName)$(BF)
ObjectFiles=$(foreach s,$(AuthoredSourceFiles) $(GeneratedSourceFiles),\
              $(TargetDir)/$(basename $(notdir $(s)))$(OBJ))
#AEPLibraries=
#OtherLibraries=

ToolsTarget=$(HostTarget)
ToolsDir=$(OCPI_CDK_DIR)/bin/$(HostTarget)
IncludeDirs:=$(OCPI_CDK_DIR)/include/$(Model) $(GeneratedDir) $(IncludeDirs)
CleanFiles += $(GeneratedSourceFiles)
override XmlIncludeDirs:=. $(XmlIncludeDirs)
# We assume all outputs are in the generated directory, so -M goes in that directory
ifeq ($(HostSystem),darwin)
DYN_PREFIX=DYLD_LIBRARY_PATH=$(OCPI_CDK_DIR)/../lib/$(HostTarget)-bin
else
DYN_PREFIX=LD_LIBRARY_PATH=$(OCPI_CDK_DIR)/../lib/$(HostTarget)-bin
endif
OcpiGen= $(DYN_PREFIX) $(ToolsDir)/ocpigen -M $(GeneratedDir)/$(@F).deps -D $(GeneratedDir) $(XmlIncludeDirs:%=-I%)

.SUFFIXES:

# If we are supposed to deposit a link into a library directory, then it should be the first/default target
ifdef LibDir
all: $(LibDir)/$(Target)/$(notdir $(BinaryFile))


$(LibDir)/$(Target)/$(notdir $(BinaryFile)): $(BinaryFile) | $(LibDir)/$(Target)
	$(AT)echo Creating link from $(LibDir)/$(Target) to $(BinaryFile) to expose the component binary.
	$(AT)$(call MakeSymLink,$(BinaryFile),$(LibDir)/$(Target))
else
all: $(ModelSpecificBuildHook) $(BinaryFile)
endif

# If the binary file is the same as the artifact file, then build it here
ifeq ($(BinaryFile),$(ArtifactFile))
$(BinaryFile): $(filter-out $(BinaryFile),$(ObjectFiles)) $(OtherLibraries) $(AEPLibraries) $(ArtifactXmlFile)
	$(LinkBinary)
	$(AT)if test "$(ArtifactXmlFile)"; then \
		(cat $(ArtifactXmlFile); sh -c 'echo X$$4' `ls -l $(ArtifactXmlFile)`) >> $@; \
	fi
endif

# All individual object files are created based on compiling
# based on suffix
define doSource
$(TargetDir)/$(basename $(notdir $(1)))$(OBJ): $(1)
	$(Compile_$(subst .,,$(suffix $(1))))
endef
$(foreach s,$(AuthoredSourceFiles) $(GeneratedSourceFiles),$(eval $(call doSource,$(s))))

ifndef Application
# worker object files need impl headers
$(foreach w,$(Workers),$(TargetDir)/$(w)$(OBJ)): $(TargetDir)/%$(OBJ): $(GeneratedDir)/%$(ImplSuffix)

$(ImplHeaderFiles): $(GeneratedDir)/%$(ImplSuffix) : %.xml
	$(AT)echo Generating the implementation header file: $@
	$(AT)$(OcpiGen) -i  $<

$(SkelFiles): $(GeneratedDir)/%$(SkelSuffix) : %.xml
	$(AT)echo Generating the implementation skeleton file: $@
	$(AT)$(OcpiGen) -s $<
endif

$(ObjectFiles): $(ImplHeaderFiles) $(SkelFiles)
$(ObjectFiles) $(BinaryFile): | $(TargetDir)

$(SkelFiles) $(ImplHeaderFiles): | $(GeneratedDir)

$(TargetDir) $(GeneratedDir): | $(OutDir)

$(TargetDir): | $(GeneratedDir)
	$(AT)echo Target directory \($@\) does not exist -- creating it.
	$(AT)mkdir $@

$(GeneratedDir):
	$(AT)echo Generated directory \($@\) does not exist -- creating it.
	$(AT)mkdir $@

# only when source file does not exist
ifndef Application

ifeq ($(origin ModelSpecificBuildHook),undefined)

$(WorkerSourceFiles): %$(SourceSuffix) : $(GeneratedDir)/%$(SkelSuffix)
	$(AT)if test ! -e $@; then \
		echo No source file exists. Copying skeleton \($<\) to $@. ; \
		cp $< $@;\
	fi
endif

endif

clean:
	$(AT)for s in $(AuthoredSourceFiles); do \
	    sk=$(GeneratedDir)/`echo $$s | sed s~$(SourceSuffix)$$~$(SkelSuffix)~`; \
	    if test -e $$s -a -e $$sk; then \
	        sed 's/GENERATED ON.*$$//' < $$s > $(GeneratedDir)/$$s; \
	        if (sed 's/GENERATED ON.*$$//' < $$sk | cmp -s - $(GeneratedDir)/$$s); then \
		    echo Source file \($$s\) identical to skeleton file \($$sk\).  Removing it.; \
	            rm $$s; \
		fi; \
	    fi; \
	done
	$(AT)rm -r -f $(ObjectFiles) $(ImplHeaderFiles) $(SkelFiles) $(GeneratedDir) $(TargetDir) $(CleanFiles)
	$(AT)for t in $(OutDir)target-*-*; do \
	  if test -d $$t; then \
	    rm -r -f $$t; \
	  fi; \
	done
	$(AT)rm -r -f $(OutDir)
	$(ModelSpecificCleanupHook)

-include $(GeneratedDir)/*.deps
