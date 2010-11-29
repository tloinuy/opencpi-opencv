
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

#############################################################################

  The Makefile fragments common.mk contain platform specific settings
  for the tools, libraries, and include paths that are required
  to build on x86 (32/64-bit), PowerPC (32/64-bit), and Solaris (sun4).

  By default, the Makefiles will build for a target using the GNU
  tools in "release mode" (no -g, use -O3 and -NDEBUG). One can build for
  "debug mode" with by specifying the "debug" target (use -g, no -O3).

  The Makefile can:

  1. Build one or more executables at a time.
     The executables can be compiled from a single for or a set of
     unique source files. Each executable can be linked with unique
     user specified libraries.
  2. Build one or more static libraries at a time.
  3. Build one or more shared libraries at a time.
  4. Add user defined include search paths.
  5. Add user defined library search paths.
  6. Add user defined macros and compiler flags.
  7. Cross compile.

#############################################################################

  The minimum Makefile

     The minimum content of a s Makefile is an assignment that sets the
     value of PROGRAMS to the name of the executable. The basename must be
     the name of the source file, unless an explicit objects list is
     provided. The include statement at the bottom of the file brings in
     the build rules for the target.

  # ---- Name of the executable ------------------------------------------- #

  PROGRAMS = myprogram.exe

  # ---- Makefile fragment with the build rules --------------------------- #

  include <path to>/scripts/tools.mk

#############################################################################

  32-bit Applications

    Setting the environment variable WORDSIZE to 32 will cause the
    Makefiles to build 32-bit objects and link with the 32-bit
    system libraries to produce a 32-bit executable. The default is to
    build for 64-bits on 64-bit architectures and for 32-bits on 32-bit
    architectures.

    $ export WORDSIZE=32

    or

    $ setenv WORDSIZE 32

#############################################################################

  Debug or Release build.

    By default the command "make" will build all of the libraries and
    then the programs listed in the Makefile in "release" mode. This
    means each object will be compiled with -O3 and -NDEBUG.
    The command "make debug" will build for "debug mode". This means
    each object will be compiled with -g. The library and program targets
    are either all built for release or all built for debug you cannot
    mix and match.

#############################################################################

  Linking with other libraries.

    1.  To link with system libraries.

       To link all of the programs built by the Makefile with
       system libraries define IMPORTS, and list the system libraries with
       which the programs will be linked.

        IMPORTS := -l<system lib0> -l<system lib1>

       Example:

        IMPORTS := -lpthread -lm

       To link programs built by the Makefile with different system
       libraries define IMPORTS_<program name> for each program built
       by the Makefile, and list the system libraries with which the
       programs will be linked.

         IMPORTS_<program0 name> := -l<system lib0> -l<system lib1>

         IMPORTS_<program1 name> := -l<system libA> -l<system libB>

    2.  To link with your own libraries.

        listed above, and instead of using -l<short library name> you must
        list the relative path to the library.

          IMPORTS := /<path to the library>/<library name>

        or

          IMPORTS_<program/task name> := /<path to library>/<library name>

#############################################################################

  Compile multiple executables in one Makefile.

    List the names of the executables after PROGRAMS to build more than
    one executable in a Makefile.

    PROGRAMS := myprogram_1.exe myprogram_2.exe myprogram_3.exe

#############################################################################

  Compile an executable or library with more than one source file.

    To build an executable or library with more than one source file you
    must define a list of object files that will comprise the executable
    or library The name of the object file list is
    OBJS_<name of executable or library without suffix>

      PROGRAMS := myprogram_1.exe

      OBJS_myprogram_1 := source_file_1.o source_file_2.o source_file_3.o

    If the Makefile is building multiple executables then each executable
    will need its own object file list.

      PROGRAMS := myprogram_1.exe myprogram_2.exe

      OBJS_myprogram_1 := src_file_1.o src_file_2.o src_file_3.o

      OBJS_myprogram_2 := src_file_A.o src_file_B.o src_file_C.o

    If the Makefile is building multiple libraries then each executable
    will need its own object file list.

      LIBRARY_STATIC := libmylib_1.exe libmylib_2.exe

      OBJS_libmylib_1 := src_file_5.o src_file_6.o src_file_7.o

      OBJS_libmylib_2 := src_file_X.o src_file_Y.o src_file_X.o

    If source files are placed in a subdirectory, there is no need to
    prefix the path with ./.

    OBJS_myprogram := subdir/src_file_B.o subdir/src_file_C.o

#############################################################################

  Link an executable with a user specified library.

    In the Makefile, set the variable IMPORTS to the library that all the
    executable will be linked with.

      IMPORTS = <relative or absolute path to user library>/libsomelib.a

    System libraries like libpthead.a and libmath.a can be specified
    with just -lpthread or -lm.

      IMPORTS = -lm -lpthread

#############################################################################

  Link each executable with a different user specified library.

   In the Makefile, set the variable IMPORTS_<name of executable>
   to the library that the specified executable will be linked with.

   PROGRAMS := myprogram_1.exe  myprogram_2.exe

   IMPORTS_myprogram_1 := <relative or absolute path>/libsomelibA.a

   IMPORTS_myprogram_2 := <relative or absolute path>/libsomelibB.a

#############################################################################

  Build a static library or a shared library.

    Define LIBRARY_STATIC in the Makefile to build a static library and
    define LIBRARY_SHARED in the Makefile to build a shared library.
    An OBJS_<name of library without suffix> list must be provided for
    each static and shared library.

    # Build a static library (must have a .a suffix).

      LIBRARY_STATIC := libmystaticlib.a

      OBJS_libmystaticlib := src_file_1.o src_file_2.o src_file_3.o

    # Build a shared library (must have a .so suffix).

      LIBRARY_SHARED := libmysharedlib.so

      OBJS_libmysharedlib := src_file_A.o src_file_B.o src_file_C.o

    By default, shared libraries are built without a "soname".  One
    can specify a soname for a shared library by adding

      SONAME_libmysharedlib := libmysharedlib.so.X.Y.Z

    Where X.Y.Z is the desired soname number. If ones specifies a soname
    the build will create a soft link from libmysharedlib.so to
    libmysharedlib.so.X.Y.Z.

    The Makefile specifies -Wl,-rpath=./ when linking each executable in
    a Makefile that builds a shared library to cause the shared library
    mechanism to search the current directory for the shared library
    when the executable is run.

#############################################################################

  Add user defined include search paths.

    Define INCLUDE_PATH in the Makefile to add additional include paths
    to the search list.

      INCLUDE_PATH = -I../another_include_directory

#############################################################################

  Add user defined library search paths.

    Define LIBRARY_PATH in the Makefile to add additional library search
    paths to the search list.

      LIBRARY_PATH = -L../directory_to_search -L../another_directory_to_search

#############################################################################

  Add user defined macros and compiler flags.

    Define CPPFLAGS in the Makefile to add user defined macros or
    compiler flags.

    Below is an example that adds the compiler flag -Wdisabled-optimization
    and defines a compile-time macro called USR_DEFINED_MACRO and
    sets its value to foo.

      CPPFLAGS += -DUSR_DEFINED_MACRO=foo -Wdisabled-optimization

#############################################################################

  Miscellaneous

    The include <path to>/scripts/common.mk statement must be the very last
    line in the Makefile. All Makefile variables like INCLUDE_PATH,
    IMPORTS, and LIBRARY_STATIC must appear before the include of the
    common.mk include.

    Assignments to a Makefile variable can be made in two forms:

    1. Assignment with multiples values per line

       INCLUDE_PATH = -I../include   -I../../include

    2. Concatenation

       INCLUDE_PATH = -I../include

       INCLUDE_PATH += -I../../include

       Note the += performs concatenation.

    The two examples listed above are equivalent.

#############################################################################

  Cross compile

  To cross compile set RUNTIME_ARCH at the top of your Makefile to the
  architecture where your code  will be run.

  RUNTIME_ARCH must be set to one of:

  1. ppc
  2. ppc64

  At this time the Makefiles only support a cross compile on x86 for
  Power PC with the TimeSys tools. I would be easy to add other tools
  and target to the common.mk file.

#############################################################################
