# For use with GNU make.
# no builtin implicit rules
MAKEFLAGS = --no-builtin-rules --warn-undefined-variables

#-----------------------------------------------------------------------------

ifndef SYSNAME
  SYSNAME := $(shell uname)
  ifeq ($(SYSNAME),)
    $(error error with shell command uname)
  endif
endif

ifndef NODENAME
  NODENAME := $(shell uname -n)
  ifeq ($(NODENAME),)
    $(error error with shell command uname -n)
  endif
endif

ifndef DEBUG
  ifndef OPT
    OPT = false
  endif
  ifeq ($(OPT),false)
    DEBUG=true
  else
    DEBUG=false
  endif
endif

# set architecture dependent directories and default options
ARCH_DIR = $(SYSNAME)-$(ABI)# default
ifeq ($(SYSNAME:IRIX%=),)# SGI
  # Specify what application binary interface (ABI) to use i.e. 32, n32 or 64
  ifndef ABI
    ifeq ($(SGI_ABI),-64)
      ABI = 64
    else
      ABI = n32
    endif
  endif
  # Specify which instruction set to use i.e. -mips#
  ifndef MIPS
    # Using mips3 for most basic version on esu* machines
    # as there are still some Indys around.
    MIPS = 4
    ifeq ($(NODENAME:esu%=),)
      ifeq ($(ABI),n32)
        ifneq ($(DEBUG),false)
          MIPS=3
        endif
      endif
    endif
  endif
  INSTRUCTION := mips$(MIPS)
  ARCH_DIR := $(INSTRUCTION)-$(ABI)
endif
ifeq ($(SYSNAME),Linux)
  ARCH_DIR := linux86
endif
ifeq ($(SYSNAME),SunOS)
  ARCH_DIR = solaris-$(ABI)
endif
ifeq ($(SYSNAME),AIX)
  ifndef ABI
    ifdef OBJECT_MODE
      ifneq ($(OBJECT_MODE),32_64)
        ABI = $(OBJECT_MODE)
      endif
    endif
  endif
  ARCH_DIR = aix-$(ABI)
endif

ifndef ABI
  ABI = 32# for preprocess_fortran.pl
endif

ifneq ($(DEBUG),false)
  OPT_SUFFIX = -debug
else
  OPT_SUFFIX = -opt
endif

SOURCE_DIR = source
WORKING_DIR := generated/$(ARCH_DIR)$(OPT_SUFFIX)
LIBRARY_DIR := lib/$(ARCH_DIR)

SOURCE_FILES := $(notdir $(wildcard $(SOURCE_DIR)/*.*) )
PMH_FILES := $(patsubst %.pm, %.pmh, $(filter %.pm, $(SOURCE_FILES)))
C_SOURCES := $(filter %.c, $(SOURCE_FILES) )
C_UNITS := $(basename $(C_SOURCES) )
DEPEND_FILES := $(foreach unit, $(C_UNITS), $(WORKING_DIR)/$(unit).d )

C_OBJ := $(WORKING_DIR)/libperlinterpreter.o
LIBRARY := $(LIBRARY_DIR)/libperlinterpreter$(OPT_SUFFIX).a
LIB_EXP := $(patsubst %.a, %.exp, $(LIBRARY))

# ABI string for environment variables
# (for location of perl librarys in execuatable)
ifeq ($(ABI),n32)
  ABI_ENV=N32#
else
  ABI_ENV=$(ABI)#
endif

# Location of perl.
# Try to determine from environment.
# gmake doesn't do what I want with this:
# ifdef CMISS$(ABI_ENV)_PERL
ifneq ($(origin CMISS$(ABI_ENV)_PERL),undefined)
  PERL := $(CMISS$(ABI_ENV)_PERL)
else
  # need a perl of the same architecture (and ABI) type
  # so don't just find the first perl in PATH
  ifneq ($(findstring IRIX,$(SYSNAME)),)
    ifeq ($(NODENAME:esu%=),)
      ifeq ($(INSTRUCTION),mips3)
        PERL = /usr/local/perl5.6/bin-$(INSTRUCTION)/perl
      else
        ifeq ($(ABI),n32)
          PERL = /usr/local/perl5.6/bin/perl
        else
          PERL = /usr/local/perl5.6/bin-$(ABI)/perl
        endif
      endif
    endif
    ifeq ($(NODENAME),hpc2)
      ifeq ($(ABI),n32)
        PERL = /usr/local/perl5.6/bin/perl
      else
        PERL = /usr/local/perl5.6/bin-$(ABI)/perl
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
  ifeq ($(SYSNAME),AIX)
      ifeq ($(ABI),32)
        PERL = ${CMISS_ROOT}/bin/perl
      else
        PERL = ${CMISS_ROOT}/bin/$(ABI)/perl
      endif
  endif
  ifeq ($(SYSNAME),Linux)
    ifeq ($(NODENAME:esp56%=),)
      PERL = /usr/local/cmiss/bin/perl
    else
      #only 32-bit, assume first perl in path is suitable for this architecture
      PERL = $(firstword $(wildcard $(subst :,/perl ,$(PATH))))
    endif
  endif
  ifndef PERL
    $(error PERL not defined)
  endif
endif

PERL_ARCHNAME := $(shell $(PERL) -MConfig -e 'print "$$Config{archname}\n"')
ifeq ($(PERL_ARCHNAME),)
  $(error problem with $(PERL))
endif
PERL_ARCHLIB := $(shell $(PERL) -MConfig -e 'print "$$Config{archlibexp}\n"')
ifeq ($(PERL_ARCHLIB),)
  $(error problem with $(PERL))
endif
DYNALOADER_LIB = $(PERL_ARCHLIB)/auto/DynaLoader/DynaLoader.a
PERL_WORKING_DIR = Perl_cmiss/generated/$(PERL_ARCHNAME)
PERL_CMISS_MAKEFILE = $(PERL_WORKING_DIR)/Makefile
PERL_CMISS_LIB = $(PERL_WORKING_DIR)/auto/Perl_cmiss/Perl_cmiss.a
PERL_LIB = $(PERL_ARCHLIB)/CORE/libperl.a
PERL_EXP = $(wildcard $(PERL_ARCHLIB)/CORE/perl.exp)

C_INCLUDE_DIRS = $(PERL_ARCHLIB)/CORE $(WORKING_DIR)

#-----------------------------------------------------------------------------
# compiling commands

CC = cc
LD_RELOCATABLE = ld -r $(CFL_FLGS) $(L_FLGS)
AR = ar
# Option lists
# (suboption lists become more specific so that later ones overrule previous)
CFLAGS = $(strip $(CFL_FLGS) $(CFE_FLGS) $(CF_FLGS))
CPPFLAGS := $(addprefix -I, $(C_INCLUDE_DIRS) ) '-DABI_ENV="$(ABI_ENV)"'
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

ifeq ($(SYSNAME:IRIX%=),)
  CF_FLGS += -use_readonly_const
  DBGCF_FLGS += -DEBUG:trap_uninitialized:subscript_check:verbose_runtime
  # warning 158 : Expecting MIPS3 objects: ... MIPS4.
  L_FLGS += -rdata_shared -DEBUG:error=158 -woff 47
  CFL_FLGS = -$(ABI) -$(INSTRUCTION)
  OPTCF_FLGS = -O3 -OPT:Olimit=8000
endif
ifeq ($(SYSNAME),Linux)
  CPPFLAGS += -Dbool=char -DHAS_BOOL
  OPTCF_FLGS = -O2
endif
ifeq ($(SYSNAME),SunOS)
  # need -xarch=native after -fast
  OPTCFE_FLGS += -fast $(CFE_FLGS)
  ifeq ($(ABI),64)
    CFE_FLGS += -xarch=native64
  endif
endif
ifeq ($(SYSNAME),AIX)
  CC = xlc
  # 1506-743 (I) 64-bit portability: possible change of result through conversion ...
  # FD_SET in sys/time.h does this
  # no -qinfo=gen because perl redefines many symbols
  CFLAGS += -qinfo=ini:por:pro:trd:tru:use -qsuppress=1506-743
  ARFLAGS += -X$(ABI)
  # may want -qsrcmsg
  CF_FLGS += -qfullpath
  CFE_FLGS += -q$(ABI) -qarch=auto
  L_FLGS += -b$(ABI)
  ifeq ($(ABI),64)
    CF_FLGS += -qwarn64
  endif
  OPTCF_FLGS = -O3 -qmaxmem=12000 -qtune=auto
  OPTC_FLGS += -qnoignerrno
endif

.PHONY : main
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

ifndef TASK
  TASK =#
endif
ifeq ($(TASK),)
#-----------------------------------------------------------------------------

  .NOTPARALLEL:

  TMP_FILES := $(notdir $(wildcard $(WORKING_DIR)/*.* ) )
  OLD_FILES := $(filter-out $(PMH_FILES) $(foreach unit,$(C_UNITS),$(unit).%), \
    $(TMP_FILES))

  .PHONY : tidy clean allclean \
	all debug opt debug64 opt64

  main : $(PERL_CMISS_MAKEFILE) $(WORKING_DIR) $(LIBRARY_DIR)
	$(MAKE) --directory=$(PERL_WORKING_DIR) static
	$(MAKE) --no-print-directory TASK=source
	$(MAKE) --no-print-directory TASK=library

  tidy :
  ifneq ($(OLD_FILES),)
	rm $(foreach file,$(OLD_FILES), $(WORKING_DIR)/$(file) )
  endif

  $(PERL_CMISS_MAKEFILE) : $(PERL) Perl_cmiss/Makefile.PL
	cd Perl_cmiss ; $(PERL) Makefile.PL

  $(WORKING_DIR) :
	mkdir -p $@

  $(LIBRARY_DIR) :
	mkdir -p $@

  clean:
	@echo "Cleaning house ..."
	-rm -rf $(PERL_WORKING_DIR) $(WORKING_DIR) $(LIBRARY) $(LIB_EXP)

  allclean:
	@echo "Cleaning house ..."
	-rm -rf Perl_cmiss/generated/* generated/* lib/*

  debug opt debug64 opt64:
	$(MAKE) --no-print-directory DEBUG=$(DEBUG) ABI=$(ABI)

  debug debug64: DEBUG=true
  opt opt64: DEBUG=false
  ifeq ($(SYSNAME:IRIX%=),) #SGI
    debug opt: ABI=n32
  else
    debug opt: ABI=32
  endif
  debug64 opt64: ABI=64

  all : debug opt
  ifneq ($(SYSNAME),Linux)
    all: debug64 opt64
  endif

#-----------------------------------------------------------------------------
endif

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

ifeq ($(TASK),source)
#-----------------------------------------------------------------------------

  main : $(DEPEND_FILES) \
    $(foreach file,$(PMH_FILES), $(WORKING_DIR)/$(file) )

  # implicit rules for making the dependency files

  $(WORKING_DIR)/%.d : $(SOURCE_DIR)/%.c
	makedepend $(CPPFLAGS) -f- -Y $< 2> $@.tmp | sed -e 's%^source\([^ ]*\).o%$$(WORKING_DIR)\1.o $$(WORKING_DIR)\1.d%' > $@
	(grep pmh $@.tmp | grep makedepend | nawk -F "[ ,]" '{printf("%s.%s:",substr($$4, 1, length($$4) - 2),"o"); for(i = 1 ; i <= NF ; i++)  { if (match($$i,"pmh")) printf(" source/%s", substr($$i, 2, length($$i) -2)) } printf("\n");}' | sed -e 's%^$(SOURCE_DIR)\([^ ]*\).o%$$(WORKING_DIR)\1.o $$(WORKING_DIR)\1.d%' | sed -e 's%$(SOURCE_DIR)\([^ ]*\).pmh%$$(WORKING_DIR)\1.pmh%' >> $@)

$(WORKING_DIR)/%.pmh : $(SOURCE_DIR)/%.pm
	utilities/pm2pmh $< > $@

#-----------------------------------------------------------------------------
endif

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

ifeq ($(TASK),library)
#-----------------------------------------------------------------------------

  main : $(LIBRARY)

  # explicit rule for making the library
  # Including all necessary objects from archives into output archive.
  # This is done by producing a relocatable object first.
  # Is there a better way?

  $(LIBRARY) : $(C_OBJ)
	$(AR) $(ARFLAGS) $@ $^

  # If there is an export file for libperl.a then use it for this library.
  ifneq ($(PERL_EXP),)
    main : $(LIB_EXP)

    $(LIB_EXP) : $(PERL_EXP)
	cp -f $^ $@
  endif

  # don't retain these relocatable objects
  .INTERMEDIATE : $(C_OBJ)

  $(C_OBJ) : $(foreach unit, $(C_UNITS), $(WORKING_DIR)/$(unit).o ) \
    $(DYNALOADER_LIB) $(PERL_CMISS_LIB) $(PERL_LIB)
	$(LD_RELOCATABLE) -o $@ $^

  # include the object dependencies
  ifneq ($(DEPEND_FILES),)
    include $(DEPEND_FILES)
  endif

  # implicit rules for making the objects
  $(WORKING_DIR)/%.o : $(SOURCE_DIR)/%.c
	$(CC) -o $@ $(CPPFLAGS) $(CFLAGS) $<

#-----------------------------------------------------------------------------
endif
