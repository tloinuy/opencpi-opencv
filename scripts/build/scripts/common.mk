
# ####
#
#  This file is licensed from, and is a trade secret of:
#
#    Mercury Computer Systems, Incorporated
#    199 Riverneck Road
#    Chelmsford, Massachusetts 01824-2820
#    United States of America
#    Telephone + 1 978 256-1300
#    Telecopy/FAX + 1 978 256-3599
#    US Customer Support (800) 872-0040
#
#  Refer to your Software License Agreements for provisions on use,
#  duplication, third party rights and disclosure. This file: (a) was
#  developed at private expense, and no part was developed with government
#  funds, (b) is a trade secret of Mercury Computer Systems, Inc. for the
#  purposes of the Freedom of Information Act, (c) is "commercial computer
#  software" subject to limited utilization as provided in the above noted
#  Software License Agreements and (d) is in all respects data belonging
#  exclusively to either Mercury Computer Systems, Inc. or its third party
#  providers.
#
#  Copyright (c) Mercury Computer Systems, Inc., Chelmsford MA., 1984-2009,
#  and all third party embedded software sources. All rights reserved under
#  the Copyright laws of US. and international treaties.
#
########################################################################### #

# ####
#
#  Makefile fragment with build rules to create an executable for x86
#  Power PC, or Solaris with the GNU GCC compiler.
#
########################################################################### #

# ---- Determine the architecture ----------------------------------------- #

# RUNTIME_ARCH is the architecture of the machine where you intend to run
# your code.
# BUILD_ARCH is the architecture of the machine where you build
# your code.
# For example, for a cross compile on an x86 box for a Power PC
# you would set RUNTIME_ARCH = ppc in your Makefile and the BUILD_ARCH
# would be set below to the architecture of the system where you are
# running the Makefile. The Makefile will detect the difference between
# RUNTIME_ARCH and BUILD_ARCH and use the cross tools one the BUILD_ARCH
# to cross build for the RUNTIME_ARCH.

# For a cross build you must set RUNTIME_ARCH in your Makefile.
ifeq "$(RUNTIME_ARCH)" ""
	RUNTIME_ARCH := $(shell uname -m)
endif

BUILD_ARCH := $(shell uname -m)

ifeq "$(RUNTIME_ARCH)" "x86_64"
	TARGET := x86
else
	ifeq "$(RUNTIME_ARCH)" "i686"
		TARGET := x86
		WORDSIZE := 32
	else
		ifeq "$(RUNTIME_ARCH)" "ppc64"
			TARGET := ppc
		else
			ifeq "$(RUNTIME_ARCH)" "ppc"
				TARGET := ppc
				WORDSIZE := 32
			else
				ifeq "$(RUNTIME_ARCH)" "sun4u"
					TARGET := sparc
					WORDSIZE := 32
				else
					ifeq "$(RUNTIME_ARCH)" "i386"
						TARGET := x86
						WORDSIZE := 32
					else
						$(error "Unable to determine build target")
					endif
				endif
			endif
		endif
	endif
endif

# ---- Test if compile is for a 32-bit or a 64-bit application ------------ #

# Default to 64-bit application (except for know 32-bit platforms)

ifeq "$(WORDSIZE)" ""
	WORDSIZE := 64
endif

# ---- Select tools ------------------------------------------------------- #

ifeq "$(TARGET)" "x86"
	TOOLS_PATH := /usr/bin/
	ifeq "$(WORDSIZE)" "64"
		EMULATION := -m elf_x86_64
	else
		EMULATION := -m elf_i386
	endif
else
	ifeq "$(TARGET)" "ppc"
		ifeq "$(BUILD_ARCH)" "$(RUNTIME_ARCH)"
			TOOLS_PATH := /usr/bin/
		else
			TOOLS_PATH := /opt/timesys/toolchains/ppc86xx-linux/bin/ppc86xx-linux-
		endif
		EMULATION := -m elf$(WORDSIZE)ppc
	else # sparc
		ifeq "$(shell hostname)" "pagan"
			TOOLS_PATH := /export/home/gcc-4.1.2/bin/
		else
			TOOLS_PATH := /usr/local/bin/
		endif
		EMULATION :=
	endif
endif

RM := rm -f
CP := cp -p

AR := $(TOOLS_PATH)ar

AS := $(TOOLS_PATH)gcc -m$(WORDSIZE)

LD := $(TOOLS_PATH)gcc -m$(WORDSIZE) -shared $(EMULATION)

CC_CX := $(TOOLS_PATH)gcc -m$(WORDSIZE)
CC_CC := $(TOOLS_PATH)g++ -m$(WORDSIZE)

# Cuda tools
NV_TOOLS_PATH = /usr/local/cuda/bin/

CC_NV := $(NV_TOOLS_PATH)nvcc
LD_NV := $(NV_TOOLS_PATH)nvcc

# Open CL tools

OPENCL_TOOLS_PATH = /h/mpepe/projects/opencl/opencl_sdk/AMD_CPU_OpenCL_0.4/OpenCL-0.4/bin/

CC_CL := $(OPENCL_TOOLS_PATH)clc
LD_CL := $(OPENCL_TOOLS_PATH)clc

# ---- Set flags for the tools -------------------------------------------- #

ARFLAGS := -cru
ASFLAGS := $(ASFLAGS)
SOFLAGS := -shared
LDFLAGS := $(LDFLAGS)

ifeq ($(MAKECMDGOALS),debug)
	CPPFLAGS := $(CPPFLAGS) -DDEBUG
else
	CPPFLAGS := $(CPPFLAGS) -DNDEBUG
endif

CFLAGS   := -Wall -Wextra -Wshadow -Wfloat-equal -ansi -fPIC $(CPPFLAGS) $(CFLAGS) -std=gnu99
CXXFLAGS := -Wall -Wextra -Wshadow -Wfloat-equal -ansi -fPIC $(CPPFLAGS) $(CXXFLAGS)

# -Weffc++

# -pedantic

ifeq ($(MAKECMDGOALS),debug)
	build_mode =" Debug mode"
	CFLAGS   := $(CFLAGS) -g -fno-inline
	CXXFLAGS := $(CXXFLAGS) -g -fno-inline
	LDFLAGS  := $(LDFLAGS) -g
else
	build_mode =" Release mode"
	CFLAGS   := $(CFLAGS) -O3
	CXXFLAGS := $(CXXFLAGS) -O3
endif

NVFLAGS := $(NVFLAGS)

CLFLAGS := $(CLFLAGS)

# ---- Targets ------------------------------------------------------------ #

all: $(LIBRARY_STATIC) $(LIBRARY_SHARED) $(PROGRAMS)

debug: $(LIBRARY_STATIC) $(LIBRARY_SHARED) $(PROGRAMS)

# ---- Functions ---------------------------------------------------------- #

get_name = $(notdir $(1))

GENERATE_OBJECT_LIST = \
	@if [ "$(OBJS_$(basename $@))" != "" ]; \
	then \
		objs="$(filter $(addprefix %/,$(OBJS_$(basename $@))) \
					$(OBJS_$(basename $@)),$^)"; \
	else \
		objs="$(filter $(addprefix %/,$(addsuffix .o, $(basename $@))) \
					$(addsuffix .o, $(basename $@)),$^)"; \
	fi; \

GET_SONAME = \
	if [ "$(SONAME_$(basename $@))" != "" ]; \
	then \
		soname="-soname $(SONAME_$(basename $@))"; \
	else \
		soname=""; \
	fi; \

# ---- Program list ------------------------------------------------------- #

_PROGRAMS := $(call get_name, $(PROGRAMS))

# ---- Library list (static) ---------------------------------------------- #

_LIBRARY_STATIC := $(call get_name, $(LIBRARY_STATIC))

# ---- Library list (shared) ---------------------------------------------- #

_LIBRARY_SHARED := $(call get_name, $(LIBRARY_SHARED))

# ---- Program object list ------------------------------------------------ #

PROGRAM_OBJECTS := $(foreach program, \
										$(basename $(_PROGRAMS)),\
										$(if $(OBJS_$(program)),\
										$(OBJS_$(program)),$(program).o))

$(PROGRAM_OBJECTS): %.o: %.d

# ---- Library object list (static) --------------------------------------- #

LIBRARY_STATIC_OBJECTS := $(foreach library, \
													$(basename $(_LIBRARY_STATIC)), \
													$(if $(OBJS_$(library)), \
													$(OBJS_$(library)),$(library).o))

$(LIBRARY_STATIC_OBJECTS): %.o: %.d

# ---- Library object list (shared) --------------------------------------- #

LIBRARY_SHARED_OBJECTS := $(foreach library, \
													$(basename $(_LIBRARY_SHARED)), \
													$(if $(OBJS_$(library)), \
													$(OBJS_$(library)),$(library).o))

$(LIBRARY_SHARED_OBJECTS): %.o: %.d

# ---- Library path ------------------------------------------------------- #

LD_PATH += $(LIBRARY_PATH)

ifneq "$(LIBRARY_SHARED_OBJECTS)" ""
	LD_PATH += -L./
# -Wl,-rpath=./
else
	ifneq "$(LIBRARY_STATIC_OBJECTS)" ""
		LD_PATH += -L./
	endif
endif

# ---- Per program import (libraries) list -------------------------------- #

PROGRAM_IMPORTS := $(foreach program, \
									 $(basename $(_PROGRAMS)), \
									 $(IMPORTS_$(program)))

# ---- Deterimine the dependent libraries --------------------------------- #

DEPENDENT_IMPORTS := $(filter-out -l%, $(PROGRAM_IMPORTS) $(IMPORTS))

# ---- Determine if a CUDA compiler must be used -------------------------- #

NV_FILES := $(wildcard \
						$(addsuffix .cu, \
						$(basename \
						$(PROGRAM_OBJECTS) \
						$(LIBRARY_STATIC_OBJECTS) \
						$(LIBRARY_SHARED_OBJECTS))))

# ---- Determine if an Open CL compiler must be used ---------------------- #

CL_FILES := $(wildcard \
						$(addsuffix .cl, \
						$(basename \
						$(PROGRAM_OBJECTS) \
						$(LIBRARY_STATIC_OBJECTS) \
						$(LIBRARY_SHARED_OBJECTS))))

# ---- Determine if a C++ compiler must be used --------------------------- #

CC_FILES := $(wildcard \
						$(addsuffix .cc, \
						$(basename \
						$(PROGRAM_OBJECTS) \
						$(LIBRARY_STATIC_OBJECTS) \
						$(LIBRARY_SHARED_OBJECTS))) \
						$(addsuffix .cpp, \
						$(basename \
						$(PROGRAM_OBJECTS) \
						$(LIBRARY_STATIC_OBJECTS) \
						$(LIBRARY_SHARED_OBJECTS))) \
						$(addsuffix .cxx, \
						$(basename \
						$(PROGRAM_OBJECTS) \
						$(LIBRARY_STATIC_OBJECTS) \
						$(LIBRARY_SHARED_OBJECTS))))

ifneq "$(NV_FILES)" ""
	CC := $(CC_NV)
	LD := $(LD_NV)
else
	ifeq "$(CC_FILES)" ""
		CC := $(CC_CX)
	else
		CC := $(CC_CC)
	endif
endif

# ---- Build rule with dependency generation ------------------------------ #

%.d : %.c
	@set -e; $(RM) $@; \
	$(CC_CX) $(CFLAGS) $(INCLUDE_PATH) -MM -E -MT $*.o $< > $@.$$$$; \
	sed 's,:, $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

%.d : %.cc
	@set -e; $(RM) $@; \
	$(CC_CC) $(CXXFLAGS) $(INCLUDE_PATH) -MM -E -MT $*.o $< > $@.$$$$; \
	sed 's,:, $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

%.d : %.cpp
	@set -e; $(RM) $@; \
	$(CC_CC) $(CXXFLAGS) $(INCLUDE_PATH) -MM -E -MT $*.o $< > $@.$$$$; \
	sed 's,:, $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

%.d : %.cxx
	@set -e; $(RM) $@; \
	$(CC_CC) $(CXXFLAGS) $(INCLUDE_PATH) -MM -E -MT $*.o $< > $@.$$$$; \
	sed 's,:, $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

%.d : %.cu
	$(RM) $@; \
	$(CC_NV) $(INCLUDE_PATH) -M $< $(NVFLAGS) > $@

%.d : %.cl
	$(RM) $@; \
	$(CC_CL) $(INCLUDE_PATH) -M $< $(CLFLAGS) > $@

%.o: %.c
	$(CC_CX) $(CFLAGS) -D"OCPI_XM_MAINROUTINE=ocpi_xm_mainroutine_$(basename $(notdir $*))" $(INCLUDE_PATH) -o $*.o -c $<;

%.o: %.cc
	$(CC_CC) $(CXXFLAGS) -D"OCPI_XM_MAINROUTINE=ocpi_xm_mainroutine_$(basename $(notdir $*))" $(INCLUDE_PATH) -o $*.o -c $<;

%.o: %.cpp
	$(CC_CC) $(CXXFLAGS) $(INCLUDE_PATH) -o $*.o -c $<;

%.o: %.cxx
	$(CC_CC) $(CXXFLAGS) $(INCLUDE_PATH) -o $*.o -c $<;

%.o: %.mac
	$(CP) $*.mac $*.S;
	$(CC_CX) $(ASFLAGS) $(INCLUDE_PATH) -o $*.s -E $*.S;
	$(AS) $(ASFLAGS) -o $*.o -c $*.s;
	$(RM) *.s *.S

%.o: %.S
	$(AS) $(ASFLAGS) $(INCLUDE_PATH) -o $*.o -c $<;

%.o: %.cu
	$(CC_NV) $(NVFLAGS) $(INCLUDE_PATH) -o $*.o -c $<;

%.o: %.cl
	$(CC_CL) $(CLFLAGS)  $(INCLUDE_PATH) -o $*.tmp.cxx $<;
	$(CC_CX) $(CXXFLAGS) $(INCLUDE_PATH) -o $*.o -c $*.tmp.cxx;
	$(RM) $*.tmp.cxx

# ---- Program target rule ------------------------------------------------ #

$(_PROGRAMS): Makefile $(PROGRAM_OBJECTS) $(DEPENDENT_IMPORTS)
	$(GENERATE_OBJECT_LIST) \
	program_syslibs="$(filter -l%, $(IMPORTS) $(IMPORTS_$(basename $@)))"; \
	program_imports="$(filter-out -l%, $(IMPORTS) $(IMPORTS_$(basename $@)))"; \
	imports="$$program_imports $$program_syslibs"; \
	echo " "; echo "-- Building executable" $@ "$(build_mode) --"; echo " "; \
	echo "$(CC) $(LDFLAGS) -o $@ $$objs $(LD_PATH) $$imports"; \
	$(CC) $(LDFLAGS) -o $@ $$objs $(LD_PATH) $$imports

# ---- Library target rule (static) --------------------------------------- #

$(_LIBRARY_STATIC): Makefile $(LIBRARY_STATIC_OBJECTS)
	$(GENERATE_OBJECT_LIST) \
	echo ""; echo "-- Archiving static library" $@ " --"; echo " "; \
	echo "$(AR) $(ARFLAGS) $@ $$objs"; \
	$(AR) $(ARFLAGS) $@ $$objs

# ---- Library target rule (shared) --------------------------------------- #

$(_LIBRARY_SHARED): Makefile $(LIBRARY_SHARED_OBJECTS)
	$(GENERATE_OBJECT_LIST) \
	$(GET_SONAME) \
	echo ""; echo "-- Archiving shared library" $@ " --"; echo " "; \
	echo "$(LD) $(SOFLAGS) $$soname -o $@ $$objs"; \
	$(LD) $(SOFLAGS) $$soname -o $@ $$objs; \
	if [ "$(SONAME_$(basename $@))" != "" ]; \
	then \
		echo "ln -s $@ $(SONAME_$(basename $@))"; \
		rm -rf $(SONAME_$(basename $@)); \
		ln -s $@ $(SONAME_$(basename $@)); \
	fi;

# ---- Clean rule --------------------------------------------------------- #

clean:
	$(RM) $(PROGRAMS) $(PROGRAM_OBJECTS) $(PROGRAM_OBJECTS:.o=.d) $(PROGRAM_OBJECTS:.o=.d.*) $(LIBRARY_STATIC) $(LIBRARY_STATIC_OBJECTS) $(LIBRARY_STATIC_OBJECTS:.o=.d) $(LIBRARY_STATIC_OBJECTS:.o=.d.*) $(LIBRARY_SHARED:.so=.so*) $(LIBRARY_SHARED_OBJECTS) $(LIBRARY_SHARED_OBJECTS:.o=.d) $(LIBRARY_SHARED_OBJECTS:.o=.d.*)

# ---- Dependency list ---------------------------------------------------- #

ifneq "$(MAKECMDGOALS)" "clean"
	DEPEND_FILES := $(foreach file, \
									$(PROGRAM_OBJECTS:.o=.d) \
									$(LIBRARY_STATIC_OBJECTS:.o=.d) \
									$(LIBRARY_SHARED_OBJECTS:.o=.d), \
									$(wildcard $(file)))
	ifneq "$(DEPEND_FILES)" ""
			include $(DEPEND_FILES)
	endif
endif

# ------------------------------------------------------------------------- #

