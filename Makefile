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
ARCH_DIR = $(SYSNAME)-$(ABI) #default
ifeq ($(SYSNAME:IRIX%=),) #SGI
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
      ABI = $(OBJECT_MODE)
    else
      ABI = 64
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
    ifeq ($(NODENAME),hpc1)
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
        PERL = /usr/opt/perl-5.6.1/bin/perl
      else
        PERL = /usr/opt/perl-5.6.1/bin-$(ABI)/perl
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

PERL_ARCHNAME = $(shell $(PERL) -MConfig -e 'print "$$Config{archname}\n"')
ifeq ($(PERL_ARCHNAME),)
  $(error problem with $(PERL))
endif
PERL_ARCHLIB = $(shell $(PERL) -MConfig -e 'print "$$Config{archlibexp}\n"')
ifeq ($(PERL_ARCHLIB),)
  $(error problem with $(PERL))
endif
DYNALOADER_LIB = $(PERL_ARCHLIB)/auto/DynaLoader/DynaLoader.a
PERL_WORKING_DIR = Perl_cmiss/generated/$(PERL_ARCHNAME)
PERL_CMISS_MAKEFILE = $(PERL_WORKING_DIR)/Makefile
PERL_CMISS_LIB = $(PERL_WORKING_DIR)/auto/Perl_cmiss/Perl_cmiss.a
PERL_LIB = $(PERL_ARCHLIB)/CORE/libperl.a

C_INCLUDE_DIRS = $(PERL_ARCHLIB)/CORE $(WORKING_DIR)

#-----------------------------------------------------------------------------
# compiling commands

CC = cc
LD_RELOCATABLE = ld -r $(LD_FLGS)
AR = ar
# option lists
CCFLAGS = $(C_FLGS)
CPPFLAGS := $(addprefix -I, $(C_INCLUDE_DIRS) ) '-DABI_ENV="$(ABI_ENV)"'
ARFLAGS = -cr
ifneq ($(DEBUG),false)
  CCFLAGS += $(DBGCC_FLGS)
else
  CCFLAGS += $(OPTCC_FLGS)
endif
# suboption lists
C_FLGS = -c $(CLD_FLGS) $(CE_FLGS)
CE_FLGS =
LD_FLGS = $(CLD_FLGS)
CLD_FLGS =
DBGCC_FLGS = $(DBGC_FLGS)
DBGC_FLGS = -g
OPTCC_FLGS = $(OPTC_FLGS)
OPTC_FLGS = -O $(OPT_FLGS)
OPT_FLGS =

ifeq ($(SYSNAME:IRIX%=),)
  C_FLGS += -use_readonly_const
  DBGC_FLGS += -DEBUG:trap_uninitialized:subscript_check:verbose_runtime
  # warning 158 : Expecting MIPS3 objects: ... MIPS4.
  LD_FLGS += -rdata_shared -DEBUG:error=158 -woff 47
  CLD_FLGS := -$(ABI) -$(INSTRUCTION)
  OPTC_FLGS = -O3 -OPT:Olimit=8000
endif
ifeq ($(SYSNAME),Linux)
  CPPFLAGS += -Dbool=char -DHAS_BOOL
  OPTC_FLGS = -O2
endif
ifeq ($(SYSNAME),SunOS)
  # need arch_flags after -fast
  OPT_FLGS = -fast $(CLD_FLGS)
  ifeq ($(ABI),64)
    CE_FLGS := -xarch=native64
  endif
endif
ifeq ($(SYSNAME),AIX)
  CC = xlc
  # 1506-743 (I) 64-bit portability: possible change of result through conversion ...
  # FD_SET in sys/time.h does this
  CCFLAGS += -qinfo=ini:por:pro:trd:tru:use -qsuppress=1506-743
  ifeq ($(ABI),64)
    ARFLAGS += -X64
  endif
  # may want -qsrcmsg
  C_FLGS += -qfullpath
  CE_FLGS += -q$(ABI) -qarch=auto
  LD_FLGS += -b$(ABI)
  ifeq ($(ABI),64)
    C_FLGS += -qwarn64
  endif
  OPTC_FLGS = -O3 -qtune=auto -qstrict -qmaxmem=-1
endif

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

  .PHONY : main tidy clean allclean \
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
	rm -rf $(PERL_WORKING_DIR) $(WORKING_DIR) $(LIBRARY)

  allclean:
	@echo "Cleaning house ..."
	rm -rf Perl_cmiss/generated/* generated/* lib/*

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
	(grep pmh $@.tmp | grep makedepend | nawk -F "[ ,]" '{printf("%s.%s:",substr($$4, 1, length($$4) - 2),"o"); for(i = 1 ; i <= NF ; i++)  { if (match($$i,"pmh")) printf(" source/%s", substr($$i, 2, length($$i) -2)) } printf("\n");}' | sed -e 's%^source\([^ ]*\).o%$$(WORKING_DIR)\1.o $$(WORKING_DIR)\1.d%' | sed -e 's%source\([^ ]*\).pmh%$$(WORKING_DIR)\1.pmh%' >> $@)

$(WORKING_DIR)/%.pmh : $(SOURCE_DIR)/%.pm
	utilities/pm2pmh $< > $@

#-----------------------------------------------------------------------------
endif

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

ifeq ($(TASK),library)
#-----------------------------------------------------------------------------

  # explicit rule for making the library
  # Including all necessary objects from archives into output archive.
  # This is done by producing a relocatable object first.
  # Is there a better way?

  $(LIBRARY) : $(C_OBJ)
	$(AR) $(ARFLAGS) $@ $^

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
	$(CC) -o $@ $(CPPFLAGS) $(CCFLAGS) $<

#-----------------------------------------------------------------------------
endif
