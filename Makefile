# For use with GNU make.
# no builtin implicit rules
MAKEFLAGS = --no-builtin-rules --warn-undefined-variables
#-----------------------------------------------------------------------------

ifndef TASK
  TASK =#
endif

ifndef SYSNAME
  SYSNAME := $(shell uname)
  ifeq ($(SYSNAME),)
    $(error error with shell command uname)
  endif
  ifneq ($(filter MINGW32%,$(SYSNAME)),)
    SYSNAME=win32
  endif
endif

ifndef NODENAME
  NODENAME := $(shell uname -n)
  ifeq ($(NODENAME),)
    $(error error with shell command uname -n)
  endif
endif

ifndef MACHNAME
  MACHNAME := $(shell uname -m)
  ifeq ($(MACHNAME),)
    $(error error with shell command uname -m)
  endif
endif

ifndef DEBUG
  ifndef OPT
    OPT := false
  endif
  ifeq ($(OPT),false)
    DEBUG := true
  else
    DEBUG := false
  endif
endif

# set architecture dependent directories and default options

# defaults
INSTRUCTION=$(MACHNAME)
BIN_ARCH_DIR = $(INSTRUCTION)-$(OPERATING_SYSTEM)
LIB_ARCH_DIR = $(INSTRUCTION)-$(ABI)-$(OPERATING_SYSTEM)

ifeq ($(filter-out IRIX%,$(SYSNAME)),)# SGI
  # Specify what application binary interface (ABI) to use i.e. 32, n32 or 64
  ifndef ABI
    ifdef SGI_ABI
      ABI := $(patsubst -%,%,$(SGI_ABI))
    else
      ABI = n32
    endif
  endif
  # Specify which instruction set to use i.e. -mips#
  ifndef MIPS
    # Using mips3 for most basic version on esu* machines
    # as there are still some Indys around.
    # Although mp versions are unlikely to need mips3 they are made this way
    # because it makes finding library locations easier.
    MIPS = 4
    ifeq ($(filter-out esu%,$(NODENAME)),)
      ifeq ($(ABI),n32)
        ifneq ($(DEBUG),false)
          MIPS=3
        endif
      endif
    endif
  endif
  INSTRUCTION := mips
  OPERATING_SYSTEM := irix
endif
ifeq ($(SYSNAME),Linux)
  OPERATING_SYSTEM := linux
  ifeq ($(filter-out i%86,$(MACHNAME)),)
    INSTRUCTION := i686
  endif
  ifneq ($(filter $(INSTRUCTION),i686 ia64 x86_64),)# i686, ia64, x86_64 
    LIB_ARCH_DIR = $(INSTRUCTION)-$(OPERATING_SYSTEM)# no ABI required
  endif
  ifndef ABI
    ABI=32
    ifeq ($(filter-out i%86,$(MACHNAME)),)
      ABI=32
    endif
    ifneq (,$(filter $(MACHNAME),ia64 x86_64))# ia64 or x86_64
      ABI=64
    endif
  endif
endif
ifeq ($(SYSNAME),win32)
  ABI=32
  INSTRUCTION := i386
  OPERATING_SYSTEM := win32
  LIB_ARCH_DIR = $(INSTRUCTION)-$(OPERATING_SYSTEM)# no ABI
endif
ifeq ($(SYSNAME),SunOS)
  OPERATING_SYSTEM := solaris
endif
ifeq ($(SYSNAME),AIX)
  ifndef ABI
    ifdef OBJECT_MODE
      ifneq ($(OBJECT_MODE),32_64)
        ABI = $(OBJECT_MODE)
      endif
    else
      ABI = 32
    endif
  endif
  INSTRUCTION := rs6000
  OPERATING_SYSTEM := aix
endif
ifeq ($(SYSNAME),Darwin)
  OPERATING_SYSTEM := darwin
  LIB_ARCH_DIR = ppc-32-$(OPERATING_SYSTEM)
  ABI = 32
endif

ifneq ($(DEBUG),false)
  DEBUG_SUFFIX = -debug
else
  DEBUG_SUFFIX =
endif

#This is now the default build version, each libperlinterpreter.so
#that is found in the lib directories is converted into a base64
#c string (.soh) and included into the interpreter and the one that
#matches the machine that it is running on loaded dynamically at runtime.

#If you want this perl_interpreter to be as portable as possible then
#you will want to provide as many different perl versions to compile
#against as you can.

#If you want to build an old non dynamic loader version you will 
#need to override this to false and you must have the corresponding
#static libperl.a
ifndef USE_DYNAMIC_LOADER
  ifeq ($(SYSNAME),AIX)
    #AIX distributions have a static perl and even if I build
    #a shared "libperl.a" I cannot seem to dlopen it.
    USE_DYNAMIC_LOADER = false
  else
    ifeq ($(OPERATING_SYSTEM),win32)
      #I have not tried to make a dynamic perl interpreter in win32,
      #I have not even been including a dynaloader at all so far.
      USE_DYNAMIC_LOADER = false
    else
      USE_DYNAMIC_LOADER = maybe# if shared libraries are found
    endif
  endif
endif

#This routine is recursivly called for each possible dynamic version
#with SHARED_OBJECT set to true.  That builds the corresponding 
#libperlinterpereter.so
ifndef SHARED_OBJECT
  SHARED_OBJECT = false
endif

# Include the static perl library or not
ifndef INCLUDE_PERL
  INCLUDE_PERL = false
endif
ifneq ($(INCLUDE_PERL),false)
  INCLUDE_PERL_SUFFIX = -includeperl
else
  INCLUDE_PERL_SUFFIX =
endif

# ABI string for environment variables
# (for location of perl librarys in execuatable)
ABI_ENV = $(ABI)
ifeq ($(ABI),n32)
  ABI_ENV = N32
endif

# Location of perl.
# Try to determine from environment.
# !!! if this perl is built with gcc -fPIC and its static library is linked
# into the executable its symbols will be exported and override those in any
# shared library loaded.

# gmake doesn't do what I want with this:
# ifdef CMISS$(ABI_ENV)_PERL
ifneq ($(origin CMISS$(ABI_ENV)_PERL),undefined)
  PERL := $(CMISS$(ABI_ENV)_PERL)
else
  ifdef CMISS_PERL
    PERL := $(CMISS_PERL)
  else
    # defaults first
    PERL = perl# first perl in path
    ifeq ($(ABI),64)
      # Need a perl of the same ABI
      ifeq (,$(filter $(MACHNAME),ia64 x86_64))# not ia64 or x86_64
        PERL = perl64
      endif
    endif
    # Specify the perl on some platforms so that everyone builds with the same.
    ifeq ($(filter-out IRIX%,$(SYSNAME)),)# SGI
      ifeq ($(filter-out esu%,$(NODENAME)),)
        ifeq ($(ABI),n32)
          PERL = ${CMISS_ROOT}/bin/mips-irix/perl
        else
          PERL = ${CMISS_ROOT}/bin/mips-irix/perl64
        endif
      endif
      ifeq ($(NODENAME),hpc2)
        ifeq ($(ABI),n32)
          PERL = ${CMISS_ROOT}/bin/perl
        else
          PERL = ${CMISS_ROOT}/bin/perl64
        endif
      endif
      # What to oxford NODENAMEs look like?
      CMISS_LOCALE ?=
      ifeq (${CMISS_LOCALE},OXFORD)
        ifeq ($(ABI),n32)
          PERL = /usr/paterson/local/bin/perl
        else
          PERL = /usr/paterson/local64/bin/perl
        endif
      endif
    endif
    ifeq ($(SYSNAME),SunOS)
        ifeq ($(ABI),32)
          PERL = ${CMISS_ROOT}/bin/perl
        else
          PERL = ${CMISS_ROOT}/bin/$(ABI)/perl
        endif
    endif
    ifeq ($(SYSNAME),win32)
	PERL = ${CMISS_ROOT}/perl/bin/perl
    endif
    ifeq ($(filter-out esp56%,$(NODENAME)),)
      PERL = ${CMISS_ROOT}/bin/i686-linux/perl
    endif
  endif
endif

#Make api directory names and lib name

# There may be an issue with the perl 5.8.1 threaded build if Perl_cmiss calls
# "certain re-entrant system calls".  See perldoc perl582delta.
get_perl_api_string = $(shell $1 -MConfig -e 'print join "-", $$Config{api_versionstring}, grep { $$Config{"use$$_"} } qw(threads multiplicity 64bitall longdouble perlio)')
PERL_API_STRING := $(call get_perl_api_string,$(PERL))
ifeq ($(PERL_API_STRING),)
  $(error problem with $(PERL))
endif
ifeq ($(SYSNAME),win32)
  PERL_ARCHLIB := $(subst \,/,$(shell $(PERL) -MConfig -e 'print "$$Config{installarchlib}\n"'))
else
  PERL_ARCHLIB := $(shell $(PERL) -MConfig -e 'print "$$Config{installarchlib}\n"')
endif
ifeq ($(PERL_ARCHLIB),)
  $(error problem with $(PERL))
endif
# $Config{libperl,useshrplib} may be useful.  If an archname is required
# $Config{targetarch} || $Config{myarchname} gives perl's Configure's
# interpretation of the archname which could be less distribution specific
# than $Config{archname}.
PERL_CFLAGS := $(shell $(PERL) -MConfig -e 'print "$$Config{ccflags}\n"')
ifeq ($(PERL_CFLAGS),)
  $(error problem with $(CFLAGS))
endif

# I don't think there is any point in having a unique name here unless we also
# have a unique name for boot_Perl_cmiss, and building the dso with -Bsymbolic
# should make this unnecessary.
#Mangle the callback name so that we don't pick up the wrong version even when it is accidently visible
ifeq ($(SHARED_OBJECT), true)
   CMISS_PERL_CALLBACK_SUFFIX := $(subst .,_,$(PERL_API_STRING))
   CMISS_PERL_CALLBACK_SUFFIX := $(subst -,_,$(CMISS_PERL_CALLBACK_SUFFIX))
   PERL_CMISS_WORKING_DIR = Perl_cmiss/generated/$(BIN_ARCH_DIR)/$(PERL_API_STRING)
else
   CMISS_PERL_CALLBACK_SUFFIX := static
   PERL_CMISS_WORKING_DIR = Perl_cmiss/generated/$(BIN_ARCH_DIR)/$(PERL_API_STRING)-static
endif
CMISS_PERL_CALLBACK=cmiss_perl_callback_$(CMISS_PERL_CALLBACK_SUFFIX)
PERL_CMISS_MAKEFILE = $(PERL_CMISS_WORKING_DIR)/Makefile
PERL_CMISS_LIB = $(PERL_CMISS_WORKING_DIR)/auto/Perl_cmiss/Perl_cmiss.a
ifeq ($(TASK),)
  ifeq ($(USE_DYNAMIC_LOADER),false)
    SHARED_PERL_API_STRINGS=
  else # USE_DYNAMIC_LOADER is true or maybe
    SHARED_PERL_EXECUTABLES =
    ifneq ($(wildcard ${CMISS_ROOT}/perl),)
      ifeq ($(SYSNAME),Linux)
        ifeq ($(filter-out i%86,$(MACHNAME)),)
          SHARED_PERL_EXECUTABLES += $(wildcard $(foreach version,5.6.2 5.8.0,${CMISS_ROOT}/perl/lib/$(version)/i686-linux*/bin/perl))
        else
          SHARED_PERL_EXECUTABLES += $(wildcard $(foreach version,5.6.2 5.8.0,${CMISS_ROOT}/perl/lib/$(version)/$(MACHNAME)-linux*/bin/perl))
        endif
      endif
#       ifeq ($(SYSNAME),AIX)
#          SHARED_PERL_EXECUTABLES += $(wildcard ${CMISS_ROOT}/perl/bin-5.?.?-rs6000-${ABI}*/perl)
#       endif
      ifeq ($(filter-out IRIX%,$(SYSNAME)),)# SGI
        ifeq ($(ABI),64)
          SHARED_PERL_EXECUTABLES += $(wildcard $(foreach version,5.6.0 5.8.2,${CMISS_ROOT}/perl/lib/$(version)/irix-$(ABI)*/bin/perl))
        else
          SHARED_PERL_EXECUTABLES += $(wildcard $(foreach version,5.6.0 5.8.0,${CMISS_ROOT}/perl/lib/$(version)/irix-$(ABI)*/bin/perl))
        endif
      endif
    endif
    #Check the CMISS_PERL version isn't already included from CMISS_ROOT perls
    #Could additionally check between the versions listed above and also check that
    #the executable actually runs.
    SHARED_PERL_API_STRINGS := $(foreach perl_executable,$(SHARED_PERL_EXECUTABLES),$(call get_perl_api_string,$(perl_executable)))
    ifneq ($(filter-out $(SHARED_PERL_API_STRINGS),$(PERL_API_STRING)),)
      # There is no check for the existence of libperl.so as it is not
      # required to build.  Debian libperl does not put libperl.so in
      # $(PERL_ARCHLIB)/CORE but only in /usr/lib, which is checked at run
      # time in perl_interpreter_dynamic.c.
      SHARED_PERL_EXECUTABLES += ${PERL}
      SHARED_PERL_API_STRINGS += $(PERL_API_STRING)
    endif
    ifeq ($(USE_DYNAMIC_LOADER),maybe)
      ifeq ($(SHARED_PERL_EXECUTABLES),)
        USE_DYNAMIC_LOADER = false
      else
        USE_DYNAMIC_LOADER = true
      endif
    endif
  endif
endif

# if either SHARED_OBJECT is false or INCLUDE_PERL is true, set STATIC_PERL_LIB 
SET_STATIC_PERL_LIB = false
ifneq ($(SHARED_OBJECT),true)
  SET_STATIC_PERL_LIB = true
else
  ifeq ($(INCLUDE_PERL),true)
    SET_STATIC_PERL_LIB = true
  endif
endif

ifeq ($(SET_STATIC_PERL_LIB),true)
   #!!! Ubuntu has a libperl.a in /usr/lib and no link in $(PERL_ARCHLIB)/CORE.
   # But how can we know what version /usr/lib/libperl.a corresponds to?
   # Assume it is the same as /usr/bin/perl?
   # Or /usr/lib/libperl.so is probably more likely to be the same?
   STATIC_PERL_LIB = $(firstword $(wildcard $(PERL_ARCHLIB)/CORE/libperl.a) $(wildcard $(PERL_ARCHLIB)/CORE/libperl56.a))
   ifneq ($(USE_DYNAMIC_LOADER), true)
      ifeq ($(STATIC_PERL_LIB),)
         $(error 'Static $(PERL_ARCHLIB)/CORE/libperl.a not found for ${PERL} which is required for a non dynamic loading perl interpreter.')
      endif
   endif
else
   STATIC_PERL_LIB = 
endif
PERL_EXP = $(wildcard $(PERL_ARCHLIB)/CORE/perl.exp)

# If we intend to load a shared libperl then we cannot export symbols from a
# static perl (because they will mask those in libperl) so there is no point
# in including DynaLoader.
ifeq ($(USE_DYNAMIC_LOADER), true)
  DYNALOADER_LIB =
else

# The DynaLoader lib built with perl is built with -DPERL_CORE because it is
# only intended to be linked with the corresponding libperl.a, and therefore
# does not include the perlapi.h adjustments for binary compatibility.
# (Changes in thrdvar.h between 5.8.0 and 5.8.1 mean that perl libraries built
# with threading have different offsets for the members of struct interpreter.)
#
# If we are building for a shared libperl then the version of the libperl may
# differ from the perl version being used to build our perl_interpreter.  To
# maintain binary compatibility for the DynaLoader, it is necessary to build
# a DynaLoader without PERL_CORE defined.
#
# If DynaLoader is built here, then the version from 5.8.7 doesn't build with
# perl 5.6 executables, so at least two different copies may be required.  But
# changes in struct interpreter from 5.6.0 to 5.6.2 have been more consistent
# so it is possible that it is not an issue for the 5.005 api.  For now build
# perl with the one line change.  (see perl/build/Makefile.pl.)

# DYNALOADER_MAKEFILE =
# DYNALOADER_WORKING_DIR =
  ifeq ($(SYSNAME),win32)
    # there is probably no point in including the dynaloader_lib unless it
    # is possible to load binary perl modules from the static perl lib.
    DYNALOADER_LIB =
  else
#   ifeq ($(STATIC_PERL_LIB),)
#     DYNALOADER_WORKING_DIR = DynaLoader/generated/$(BIN_ARCH_DIR)/$(PERL_API_STRING)
#     DYNALOADER_MAKEFILE = $(DYNALOADER_WORKING_DIR)/Makefile
#     DYNALOADER_LIB = $(DYNALOADER_WORKING_DIR)/auto/DynaLoader/DynaLoader.a
#   else
    DYNALOADER_LIB = $(PERL_ARCHLIB)/auto/DynaLoader/DynaLoader.a
#   endif
  endif
endif

SOURCE_DIR = source
ifneq ($(USE_DYNAMIC_LOADER), true)
   ifneq ($(SHARED_OBJECT), true)
      SHARED_SUFFIX = 
   else
      SHARED_SUFFIX = -shared
   endif
   SHARED_LIB_SUFFIX =
else
   SHARED_SUFFIX = -dynamic
   SHARED_LIB_SUFFIX = -dynamic
endif

WORKING_DIR := generated/$(BIN_ARCH_DIR)/$(PERL_API_STRING)$(DEBUG_SUFFIX)$(SHARED_SUFFIX)
C_INCLUDE_DIRS = $(PERL_ARCHLIB)/CORE $(WORKING_DIR) .

LIBRARY_ROOT_DIR := lib/$(LIB_ARCH_DIR)
LIBRARY_VERSION := $(PERL_API_STRING)$(SHARED_LIB_SUFFIX)
LIBRARY_DIR := $(LIBRARY_ROOT_DIR)/$(LIBRARY_VERSION)
ifneq ($(SHARED_OBJECT), true)
   LIBRARY_SUFFIX = .a
else
   LIBRARY_SUFFIX = .so
endif
LIBRARY_NAME := libperlinterpreter$(INCLUDE_PERL_SUFFIX)$(DEBUG_SUFFIX)$(LIBRARY_SUFFIX)
LIBRARY := $(LIBRARY_DIR)/$(LIBRARY_NAME)
LIBRARY_LINK := $(LIBRARY_ROOT_DIR)/libperlinterpreter$(INCLUDE_PERL_SUFFIX)$(DEBUG_SUFFIX)$(LIBRARY_SUFFIX)
LIB_EXP := $(patsubst %$(LIBRARY_SUFFIX), %.exp, $(LIBRARY_LINK))

SOURCE_FILES := $(notdir $(wildcard $(SOURCE_DIR)/*.*) )

# DynaLoader module should be same version as library.  (It changed
# incompatibly between perl 5.8.3 and 5.8.4.)
# Is there a reason why the other modules are not just taken from the perl?
PMH_FILES := $(patsubst %.pm, %.pmh, $(filter %.pm, $(SOURCE_FILES)))
ifneq ($(DYNALOADER_LIB),)
  PMH_FILES += DynaLoader.pmh
endif

ifneq ($(STATIC_PERL_LIB),)# have static perl
  C_SOURCES := perl_interpreter.c
else
  ifeq ($(SHARED_OBJECT),true)
    C_SOURCES := perl_interpreter.c
  else
    C_SOURCES :=
  endif
endif
ifeq ($(USE_DYNAMIC_LOADER), true)
   C_SOURCES += perl_interpreter_dynamic.c
endif
C_UNITS := $(basename $(C_SOURCES) )
DEPEND_FILES := $(foreach unit, $(C_UNITS), $(WORKING_DIR)/$(unit).d )
OBJECTS := $(foreach unit, $(C_UNITS), $(WORKING_DIR)/$(unit).o )

C_OBJ := $(WORKING_DIR)/libperlinterpreter.o


#-----------------------------------------------------------------------------
# compiling commands

CC = cc
LD_SHARED = ld -shared $(CFL_FLGS) $(L_FLGS)
SHARED_LINK_LIBRARIES = 
AR = ar
# Option lists
# (suboption lists become more specific so that later ones overrule previous)
CFLAGS = $(strip $(CFL_FLGS) $(CFE_FLGS) $(CF_FLGS)) '-DCMISS_PERL_CALLBACK=$(CMISS_PERL_CALLBACK)'
CPPFLAGS := $(addprefix -I, $(C_INCLUDE_DIRS) ) '-DABI_ENV="$(ABI_ENV)"'# -DPERL_NO_SHORT_NAMES
ARFLAGS = -cr
ifneq ($(DEBUG),false)
  CFLAGS += $(strip $(DBGCF_FLGS) $(DBGC_FLGS))
else
  CFLAGS += $(strip $(OPTCFE_FLGS) $(OPTCF_FLGS) $(OPTC_FLGS))
endif
# suboption lists
CFL_FLGS =#	flags for C fortran and linking
L_FLGS =#	flags for linking only
CFE_FLGS =#	flags for C fortran and linking executables only
CF_FLGS = -c#	flags for C and fortran only
DBGCF_FLGS = -g#OPT=false flags for C and fortran
DBGC_FLGS =#	OPT=false flags for C only
OPTCFE_FLGS =#	OPT=true flags for C and fortran and linking executables
OPTCF_FLGS = -O#OPT=true flags for C and fortran only
OPTC_FLGS =#	OPT=true flags for C only

ifeq ($(filter-out IRIX%,$(SYSNAME)),)# SGI
  # The following warning means that the execution of the program is seriously
  # different from that intended:
  # cc-1999 cc: WARNING File = zle_tricky.c, Line = 2145
  # "jumping out of a block containing VLAs" is not currently implemented
  CFLAGS += -DEBUG:error=1999
  CF_FLGS += -use_readonly_const -fullwarn
  DBGCF_FLGS += -DEBUG:trap_uninitialized:subscript_check:verbose_runtime
  # warning 158 : Expecting MIPS3 objects: ... MIPS4.
  L_FLGS += -rdata_shared -DEBUG:error=158
  CFL_FLGS = -$(ABI) -mips$(MIPS)
  OPTCF_FLGS = -O3 -OPT:Olimit=8000
  LD_SHARED += -Bsymbolic
  ifeq ($(ABI),n32)
    LD_SHARED += -check_registry /usr/lib32/so_locations
  else
    LD_SHARED += -check_registry /usr/lib64/so_locations
  endif
endif
ifeq ($(SYSNAME),Linux)
#  ifneq ($(filter $(MACHNAME),ia64 x86_64),)# ia64 or x86_64
  ifneq ($(filter $(MACHNAME),ia64),)# ia64 only
    # Use Intel compilers if available
    # (icc -V sends output to STDERR and exits with error).
    ifneq (,$(shell icc -V 2>&1 | grep -i intel))
      CC = icc
    endif
  endif
  ifeq ($(CC),icc)
    # Intel compilers
    CFLAGS += -w2# -Wall
# This doesn't seem to do anything
#     ifeq ($(ABI),64)
#       CF_FLGS += -size_lp64
#     endif
  else
    CC=gcc
    # Wparantheses seems good but our code doesn't do things that way.
    # Wunitialized often gives warnings where things are valid.
    CFLAGS += -Wall -Wno-parentheses -Wno-uninitialized
    CPPFLAGS += -Dbool=char -DHAS_BOOL
    ifeq ($(filter $(INSTRUCTION),i686 ia64),)# not i686 nor ia64
      CFE_FLGS += -m$(ABI)
    endif
#   DBGCF_FLGS = -g3
  endif
  # Position independent code is only required for shared objects.
  ifeq ($(SHARED_OBJECT),true)
    CFE_FLGS += -fPIC
    # gcc 3.3.3 documentation recommends using the same code generation
    # flags when linking shared libraries as when compiling.
    # Linker option -Bsymbolic binds references to global symbols to those
    # within the shared library, if any.  This avoids picking up the symbols
    # like boot_Perl_cmiss from the static interpreter.
    LD_SHARED = $(CC) -shared -Wl,-Bsymbolic $(CFE_FLGS)
  endif
  OPTCF_FLGS = -O3
  ifeq ($(MACHNAME),ppc64)
    ifeq ($(ABI),64)
      L_FLGS += -m elf64ppc
    # note for ABI=32 -m elf32ppclinux is probably appropriate (but default)
    endif
  endif
  # A (-L $(PERL_ARCHLIB)/CORE) -lperl dependency might be useful here but
  # perl_interpreter_dynamic.c needs to be modified so that (the right)
  # libperl.so can be found when dlopening the perl_interpreter.
  # And we might need to check that soname version numbers don't interfere
  # with the ability to load the library as version numbers could depend on
  # the distribution.
  # But any dynamic symbols in the executable override any loaded from these
  # dependencies so there seems no advantage over merely dlopening the right
  # libperl before dlopening the perl_interpreter.
  SHARED_LINK_LIBRARIES += -lcrypt -ldl
endif
ifeq ($(SYSNAME),win32)
  CC = gcc -mms-bitfields
endif
ifeq ($(SYSNAME),SunOS)
  # need -xarch=native after -fast
  OPTCFE_FLGS += -fast $(CFE_FLGS)
  ifeq ($(ABI),64)
    CFE_FLGS += -xarch=native64
  endif
endif
ifeq ($(SYSNAME),AIX)
  # _r for reentrant - without this:
  # "...lib/5.8.6/aix-thread-multi-64all/CORE/reentr.h", line 775.16: 1506-007 (S) "struct random_data" is undefined.
  CC = xlc_r
  # no -qinfo=gen because perl redefines many symbols
  CFLAGS += -qinfo=ini:por:pro:trd:tru:use
  AR += -X$(ABI)
  # may want -qsrcmsg
  CF_FLGS += -qfullpath
  CFE_FLGS += -q$(ABI) -qarch=auto -qhalt=e
  L_FLGS += -b$(ABI)
  ifeq ($(ABI),64)
    # 1506-743 (I) 64-bit portability: possible change of result through conversion ...
    # These don't seem to serious.  Truncations are reported separately.
    # FD_SET in sys/time.h does this
    CF_FLGS += -qwarn64 -qsuppress=1506-743
  endif
  # lapack's dlamch performs an underflow so we don't check that.
  DBGCF_FLGS += -qfullpath -C -qflttrap=inv:en
  # -qinitauto for C is bytewise: 7F gives large integers.
  DBGC_FLGS += -qinitauto=7F
  OPTCF_FLGS = -O3 -qmaxmem=12000 -qtune=auto
  OPTC_FLGS += -qnoignerrno
  LD_SHARED += -bsymbolic
endif
ifeq ($(SHARED_OBJECT), true)
  CPPFLAGS += -DSHARED_OBJECT
endif
ifeq ($(USE_DYNAMIC_LOADER), true)
  CPPFLAGS += -DUSE_DYNAMIC_LOADER
endif

SHARED_LINK_LIBRARIES += -lc
CFLAGS += $(PERL_CFLAGS)
.PHONY : main

vpath $(PERL) $(subst :, ,$(PATH))

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

ifeq ($(TASK),)
#-----------------------------------------------------------------------------

ifeq ($(OPERATING_SYSTEM),win32)
  $(warning *******************************)
  $(warning This still does not compile win32 out of the box)
  $(warning The generated Perl_cmiss Makefile ends up with many \ where there need to be / which seems to work with dmake but the command called from this makefile fails with a -c error but works fine when executed in a shell)
  $(warning The library built does not have the perl or Perl_cmiss in it, these only seem to work when linked in the final link)
  $(warning *******************************)
  $(warning)
endif

  .NOTPARALLEL:

  TMP_FILES := $(notdir $(wildcard $(WORKING_DIR)/*.* ) )
  OLD_FILES := $(filter-out $(PMH_FILES) $(foreach unit,$(C_UNITS),$(unit).%), \
    $(TMP_FILES))

  .PHONY : tidy clean allclean \
	all debug opt debug64 opt64

  define VERSION_MESSAGE
    @echo '     $(call get_perl_api_string,${perl_executable}) from ${perl_executable}'

  endef
  ifeq ($(USE_DYNAMIC_LOADER),true)
   #Dynamic loading perl interpreter
   #Note that the blank line in the define is useful.
   define SHARED_BUILD_RULE
      $(MAKE) --no-print-directory USE_DYNAMIC_LOADER=false SHARED_OBJECT=true CMISS$(subst n,N,${ABI})_PERL=$(perl_executable)

   endef
   SHARED_INTERPRETER_BUILDS = $(foreach perl_executable, $(SHARED_PERL_EXECUTABLES), $(SHARED_BUILD_RULE))
   ifneq ($(STATIC_PERL_LIB),)
      define SUB_WRITE_BUILD_MESSAGE
         @echo 'The static fallback perl built into the interpreter is:'
         $(foreach perl_executable, $(PERL), $(VERSION_MESSAGE))
      endef
   else
      define SUB_WRITE_BUILD_MESSAGE
         @echo
         @echo '  YOU HAVE NOT INCLUDED A STATIC FALLBACK PERL SO ANY'
         @echo '  EXECUTABLE BUILT WITH THIS PERL INTERPRETER WILL NOT'
         @echo '  RUN AT ALL UNLESS ONE OF THE ABOVE VERSIONS OF PERL'
         @echo '  IS FIRST IN YOUR PATH.'
      endef
   endif
   define WRITE_BUILD_MESSAGE
	   @echo
	   @echo '======================================================'
	   @echo 'Congratulations, you have built a dynamic perl interpreter.'
	   @echo '     $(LIBRARY_LINK)'
      @echo 'It will work dynamically with the following perl APIs:'
      $(foreach perl_executable, $(SHARED_PERL_EXECUTABLES), $(VERSION_MESSAGE))
      ${SUB_WRITE_BUILD_MESSAGE}
   endef
#       @echo 'and has compatibility defined for the following versions:'
# 		@cat compatible_versions.txt
  else
   SHARED_INTERPRETER_BUILDS =
   ifeq ($(SHARED_OBJECT),true)
      #This is an intermediate step and so doesn't write a message
      WRITE_BUILD_MESSAGE =
   else
      #Old style static perl interpreter
      define WRITE_BUILD_MESSAGE
	      @echo
	      @echo '======================================================'
	      @echo 'You have built a non dynamic loading perl interpreter.'
	      @echo '     $(LIBRARY_LINK)'
	      @echo 'It will always run on any machine but will only'
	      @echo 'be able to load binary perl modules if they are the correct '
	      @echo 'version.  The version you have built with is:'
         $(foreach perl_executable, $(PERL), $(VERSION_MESSAGE))
      endef
   endif
  endif

  main : $(PERL_CMISS_MAKEFILE) $(WORKING_DIR) $(LIBRARY_DIR)# $(DYNALOADER_MAKEFILE)
  ifeq ($(USE_DYNAMIC_LOADER),true)
	$(SHARED_INTERPRETER_BUILDS)
  endif
	@echo
	@echo 'Building library ${LIBRARY}'
	@echo
  ifneq ($(OPERATING_SYSTEM),win32)
	$(MAKE) --directory=$(PERL_CMISS_WORKING_DIR) static
#   ifneq ($(DYNALOADER_WORKING_DIR),)
# 	$(MAKE) --directory=$(DYNALOADER_WORKING_DIR) DynaLoader.pm static
#   endif
  else
   #Use dmake as it supports back slashes for paths
	cd $(PERL_CMISS_WORKING_DIR) ; unset SHELL ; $(CMISS_ROOT)/perl/build/dmake-4.1pl1-win32/dmake static
  endif
	$(MAKE) --no-print-directory USE_DYNAMIC_LOADER=$(USE_DYNAMIC_LOADER) \
	  SHARED_PERL_API_STRINGS='$(SHARED_PERL_API_STRINGS)' TASK=source
	$(MAKE) --no-print-directory USE_DYNAMIC_LOADER=$(USE_DYNAMIC_LOADER) \
	  TASK=library
	$(WRITE_BUILD_MESSAGE)

  tidy :
  ifneq ($(OLD_FILES),)
	rm $(foreach file,$(OLD_FILES), $(WORKING_DIR)/$(file) )
  endif

  $(PERL_CMISS_MAKEFILE) : $(PERL) Perl_cmiss/Makefile.PL
	cd Perl_cmiss && CMISS_PERL_CALLBACK=$(CMISS_PERL_CALLBACK) WORKING_DIR=$(subst Perl_cmiss/,,$(PERL_CMISS_WORKING_DIR)) $(PERL) Makefile.PL

#   $(DYNALOADER_MAKEFILE) : $(PERL) DynaLoader/Makefile.PL
# 	cd DynaLoader && WORKING_DIR=$(subst DynaLoader/,,$(DYNALOADER_WORKING_DIR)) $(PERL) Makefile.PL

  $(WORKING_DIR) :
	mkdir -p $@

  $(LIBRARY_DIR) :
	mkdir -p $@

clean:
	@echo "Cleaning house ..."
	-rm -rf Perl_cmiss/generated/$(BIN_ARCH_DIR) generated/$(BIN_ARCH_DIR) $(LIBRARY_ROOT_DIR)
#$(DYNALOADER_WORKING_DIR)

allclean:
	@echo "Cleaning all houses ..."
	-rm -rf Perl_cmiss/generated generated lib
#DynaLoader/generated/* 

debug opt debug64 opt64:
	$(MAKE) --no-print-directory DEBUG=$(DEBUG) ABI=$(ABI)

  debug debug64: DEBUG=true
  opt opt64: DEBUG=false
  ifneq (,$(filter $(MACHNAME),ia64 x86_64))# ia64 or x86_64
    debug opt: ABI=64
  else
  ifeq ($(filter-out IRIX%,$(SYSNAME)),) #SGI
    debug opt: ABI=n32
  else
    debug opt: ABI=32
  endif
  endif
  debug64 opt64: ABI=64

all : debug opt
  ifneq ($(SYSNAME),Linux)
    all: debug64 opt64
  endif

update :
	cmissmake perl_interpreter

#-----------------------------------------------------------------------------
endif

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

ifeq ($(TASK),source)
#-----------------------------------------------------------------------------

  main : $(DEPEND_FILES) \
    $(foreach file,$(PMH_FILES), $(WORKING_DIR)/$(file) )

  # !!! Todo: include a file that contains the perl version so that version-
  # (rather than api-) sensitive files are updated when that changes.
  # e.g. Dynaloader module and library.

  # include the depend file dependencies
  ifneq ($(DEPEND_FILES),)
    sinclude $(DEPEND_FILES)
  endif

  # implicit rules for making the dependency files

  # KAT I think Solaris needed nawk rather than awk, but nawk is not usually
  # avaiable on Mandrake.  I don't have a Sun to try this out so I'll get it
  # working with awk on the machines I have.

  # wildcard in the .d file removes files that no longer exist
  # (that may no longer be needed because they were included in .h files that
  #  have changed).
  $(WORKING_DIR)/%.d : $(SOURCE_DIR)/%.c
	makedepend $(CPPFLAGS) -f- -Y $< 2> $@.tmp | \
	  sed -e 's%^$(SOURCE_DIR)\([^ ]*\).o: \(.*\)%$$(WORKING_DIR)\1.o $$(WORKING_DIR)\1.d: $$(wildcard \2)%' > $@
# See if there is a dependency on perl
	@if grep /perl\\.h $@ > /dev/null; then set -x; echo '$$(WORKING_DIR)/perl_interpreter.o $$(WORKING_DIR)/perl_interpreter.d: $$(PERL)' >> $@; fi
	(grep pmh $@.tmp | grep makedepend | awk -F "[ ,]" '{printf("%s.%s:",substr($$4, 1, length($$4) - 2),"o"); for(i = 1 ; i <= NF ; i++)  { if (match($$i,"pmh")) printf(" source/%s", substr($$i, 2, length($$i) -2)) } printf("\n");}' | sed -e 's%^$(SOURCE_DIR)\([^ ]*\).o%$$(WORKING_DIR)\1.o $$(WORKING_DIR)\1.d%' | sed -e 's%$(SOURCE_DIR)\([^ ]*\).pmh%$$(WORKING_DIR)\1.pmh%' >> $@)

  vpath %.pm $(SOURCE_DIR) $(PERL_ARCHLIB)# $(DYNALOADER_WORKING_DIR)

$(WORKING_DIR)/%.pmh : %.pm
	$(PERL) utilities/pm2pmh $< > $@

#Dynamic loader code for putting shared objects into the interpreter
ifeq ($(USE_DYNAMIC_LOADER),true)
   ifeq ($(SHARED_PERL_API_STRINGS),)
      $(error Missing list of SHARED_PERL_API_STRINGS in source stage)
   endif
   get_shared_lib_header = $(LIBRARY_ROOT_DIR)/$1/libperlinterpreter$(DEBUG_SUFFIX).soh
   SHARED_LIBRARY_HEADERS = $(foreach api_string, $(SHARED_PERL_API_STRINGS),\
	$(call get_shared_lib_header,$(api_string)))

   UID2UIDH = $(CMISS_ROOT)/utilities/bin/$(BIN_ARCH_DIR)/bin2base64h

  .SUFFIXES : .so .soh

  # implicit rules for making the objects
  %.soh : %.so
	$(UID2UIDH) $< $@

  #Always regenerate the versions files as they have recorded for
  #us the versions that are built into this executable
  STATIC_HEADER := $(WORKING_DIR)/static_version.h
  DYNAMIC_VERSIONS_HEADER := $(WORKING_DIR)/dynamic_versions.h
  VERSION_HEADERS := $(DYNAMIC_VERSIONS_HEADER) $(STATIC_HEADER)
  $(DYNAMIC_VERSIONS_HEADER) : $(SHARED_LIBRARY_HEADERS)
	{ \
	$(foreach api_string, $(SHARED_PERL_API_STRINGS), \
      echo 'static char libperlinterpreter$(subst .,_,$(subst -,_,$(api_string)))[] = ' && \
      echo '#include "$(call get_shared_lib_header,$(api_string))"' && \
      echo ';' && ) \
	echo 'static struct Interpreter_library_strings interpreter_strings[] = {' && \
	$(foreach api_string, $(SHARED_PERL_API_STRINGS), \
      echo '{"$(api_string)", libperlinterpreter$(subst .,_,$(subst -,_,$(api_string))) },' && ) \
	echo '};'; \
	} > $@.new;
	@set -xe && \
	if [ ! -f $@ ] || ! diff $@ $@.new > /dev/null ; then \
		mv $@.new $@ ; \
	else \
		rm $@.new; \
	fi
# 	$(foreach compatible_versions,$(wildcard compatible_versions.txt), \
# 		$(foreach header, $(SHARED_LIBRARY_HEADERS), \
# 			cat $(compatible_versions) | $(PERL) -ne '{$$version="$(word 3, $(subst /,' ',$(header)))";$$arch="$(word 4, $(subst /,' ',$(header)))";@words = split;if ($$words[2] eq $$version) { $$words[2]=~s/[-.]/_/g;$$archsub=$$arch;$$archsub=~s/[-.]/_/g;printf("{\"%s\",\"%s\", libperlinterpreter%s%s},\n",$$words[0],$$arch,$$words[2],$$archsub)}}' && )) \

  $(STATIC_HEADER):
  ifeq ($(STATIC_PERL_LIB),)
	echo '#define NO_STATIC_FALLBACK' > $@.new;
  else
	echo '/* undef NO_STATIC_FALLBACK */' > $@.new;
  endif
	@set -xe && \
	if [ ! -f $@ ] || ! diff $@ $@.new > /dev/null; then \
		mv $@.new $@; \
	else \
		rm $@.new; \
	fi

#Always build the .new and see if they should be updated.
#    .PHONY: version_headers
    $(VERSION_HEADERS): force
    .PHONY: force
    force: ;

    # version headers must exist for makedepend
    $(DEPEND_FILES): $(DYNAMIC_VERSIONS_HEADER) $(STATIC_HEADER)
endif
#-----------------------------------------------------------------------------
endif

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

ifeq ($(TASK),library)
#-----------------------------------------------------------------------------

  main : $(LIBRARY)
   #Always update the link for the .a libraries.
ifneq ($(SHARED_OBJECT), true)
	if [ -L $(LIBRARY_LINK) ] || [ -e $(LIBRARY_LINK) ] ; then \
		rm $(LIBRARY_LINK) ; \
	fi
	cd $(LIBRARY_ROOT_DIR); ln -s $(LIBRARY_VERSION)/$(LIBRARY_NAME) $(LIBRARY_NAME); cd ../..
endif

  # explicit rule for making the library
  ifneq ($(SHARED_OBJECT), true)

    $(LIBRARY) : $(OBJECTS)

    ARCHIVE_MEMBERS := $(OBJECTS)

    ifneq ($(STATIC_PERL_LIB),) # have a static perl

      LIBRARY_LIBS := $(DYNALOADER_LIB) $(STATIC_PERL_LIB) $(CURDIR)/$(PERL_CMISS_LIB)

      $(LIBRARY) : $(LIBRARY_LIBS)

      MEMBERS_DIR := $(WORKING_DIR)/archive_members
      # Hopefully there are not too many objects for the command line
      ARCHIVE_MEMBERS += $(MEMBERS_DIR)/*

      # If there is an export file for libperl.a then use it for this library.
      ifneq ($(PERL_EXP),)
        main : $(LIB_EXP)

        $(LIB_EXP) : $(PERL_EXP)
		cp -f $^ $@
      endif

    endif

    $(LIBRARY):
    ifneq ($(STATIC_PERL_LIB),) # have a static perl
	mkdir -p $(MEMBERS_DIR)
    # Hopefully none of the libraries have objects of the same name
	cd $(MEMBERS_DIR) && \
          for lib in $(LIBRARY_LIBS); do $(AR) x $$lib || exit 1; done
    endif
	$(AR) $(ARFLAGS) $@.build $(ARCHIVE_MEMBERS)
	mv $@.build $@
    ifneq ($(STATIC_PERL_LIB),) # have a static perl
	rm -r $(MEMBERS_DIR)
    endif

  else# shared object
    $(LIBRARY) : $(OBJECTS) \
         $(DYNALOADER_LIB) $(PERL_CMISS_LIB) $(STATIC_PERL_LIB)
		$(LD_SHARED) -o $@ $^ $(SHARED_LINK_LIBRARIES)
  endif

  # include the object dependencies
  ifneq ($(DEPEND_FILES),)
    include $(DEPEND_FILES)
  endif

  # implicit rules for making the objects
  $(WORKING_DIR)/%.o : $(SOURCE_DIR)/%.c
  ifeq ($(DEBUG),false)
	$(CC) -o $@ $(CPPFLAGS) $(CFLAGS) $<
  else
# Useful when using the debugger to find out which subroutine of the same name.
	[ -L $(@D)/$*.c ] || ln -s $(CURDIR)/$< $(@D)/$*.c
	$(CC) -o $@ $(CPPFLAGS) -I$(<D) $(CFLAGS) $(@D)/$*.c
  endif

#-----------------------------------------------------------------------------
endif

#-----------------------------------------------------------------------------

