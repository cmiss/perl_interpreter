#!/usr/local/bin/gmake -f
# no builtin implicit rules
MAKEFLAGS = --no-builtin-rules --warn-undefined-variables

#-----------------------------------------------------------------------------

ifndef HOSTTYPE
  $(error HOSTTYPE not defined)
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

# set architecture dependent directories
ifeq (${HOSTTYPE},iris4d)
  # Specify what application binary interface (ABI) to use i.e. 32, n32 or 64
  ifndef ABI
    ABI = n32
  endif
  # Specify which instruction set to use i.e. -mips#
  ifndef MIPS
    # Using mips3 for most basic version on esu* machines
    # as there are still some Indys around.
    MIPS = 4
    ifeq ($(HOST:esu%=),)
      ifeq ($(ABI),n32)
        ifneq ($(DEBUG),false)
          MIPS=3
        endif
      endif
    endif
  endif
  INSTRUCTION := mips$(MIPS)
  ARCH_DIR := $(INSTRUCTION)-$(ABI)
else
  ifeq (${HOSTTYPE},i386-linux)
    ABI = 32# for preprocess_fortran.pl#
    ARCH_DIR := linux86
  else
    ifndef ABI
      ABI = 32# for preprocess_fortran.pl
    endif
    ARCH_DIR := $(ABI)
  endif
endif

ifneq ($(DEBUG),false)
  OPT_SUFFIX = -debug
else
  OPT_SUFFIX = -opt
endif

PWD = $(shell pwd)
ifeq ($(PWD),)
  $(error pwd failed)
endif

SOURCE_DIR = source

WORKING_DIR := generated/$(ARCH_DIR)$(OPT_SUFFIX)
LIBRARY_DIR := lib/$(ARCH_DIR)

SOURCE_FILES := $(notdir $(wildcard $(SOURCE_DIR)/*.*) )
PMH_FILES := $(patsubst %.pm, %.pmh, $(filter %.pm, $(SOURCE_FILES)))
C_SOURCES := $(filter %.c, $(SOURCE_FILES) )
F_SOURCES := $(filter %.f90, $(SOURCE_FILES) )
C_UNITS := $(basename $(C_SOURCES) )
F_UNITS := $(basename $(F_SOURCES) )
DEPEND_FILES := $(foreach unit, $(C_UNITS) $(F_UNITS), $(WORKING_DIR)/$(unit).d )

C_OBJ := $(WORKING_DIR)/libperlinterpreter.o
F_OBJ := $(WORKING_DIR)/libperlinterpreter_f.o
LIBRARY := $(LIBRARY_DIR)/libperlinterpreter$(OPT_SUFFIX).a

# ABI string for environment variables
# (for location of perl librarys in execuatable)
ifeq ($(ABI),n32)
  ABI_ENV=N32#
else
  ABI_ENV=$(ABI)#
endif

# location of perl.
# try to determine from environment
# gmake doesn't do what I want with this
# ifdef CMISS$(ABI_ENV)_PERL
ifneq ($(origin CMISS$(ABI_ENV)_PERL),undefined)
  PERL := $(CMISS$(ABI_ENV)_PERL)
# ifeq ($(ABI),n32)
#   ifdef CMISSN32_PERL
#     PERL := ${CMISSN32_PERL}
#   endif
# endif
# ifeq ($(ABI),64)
#   ifdef CMISS64_PERL
#     PERL := ${CMISS64_PERL}
#   endif
# endif
else
  ifdef CMISS_PERL
    PERL := ${CMISS_PERL}
  else
    ifeq (${HOSTTYPE},iris4d) 
      # need a perl of the same architecture (and ABI) type
      # so don't just find the first perl in PATH
      ifeq ($(HOST:esu%=),)
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
      ifeq ($(HOST),hpc1)
        ifeq ($(ABI),n32)
          PERL = /usr/local/perl5.6/bin/perl
        else
          PERL = /usr/local/perl5.6/bin-$(ABI)/perl
        endif
      endif
      # What to oxford HOSTs look like?
      CMISS_LOCALE ?=
      ifeq (${CMISS_LOCALE},OXFORD)
        ifeq ($(ABI),n32)
          PERL = /usr/paterson/local/bin/perl
        else
          PERL = /usr/paterson/local64/bin/perl
        endif
      endif
    endif
    ifeq (${HOSTTYPE},i386-linux)
      PERL = /usr/bin/perl
    endif
    ifndef PERL
      $(error PERL not defined)
    endif
  endif
endif

PERL_ARCHLIB = $(shell $(PERL) -MConfig -e 'print "$$Config{archlib}\n"')
ifeq ($(PERL_ARCHLIB),)
  $(error problem with $(PERL))
endif
DYNALOADER_LIB = $(PERL_ARCHLIB)/auto/DynaLoader/DynaLoader.a
PERL_CMISS_MAKEFILE = $(WORKING_DIR)/Makefile
PERL_CMISS_LIB = Perl_cmiss/$(WORKING_DIR)/auto/Perl_cmiss/Perl_cmiss.a
PERL_LIB = $(PERL_ARCHLIB)/CORE/libperl.a

C_INCLUDE_DIRS = $(PERL_ARCHLIB)/CORE

export WORKING_DIR HOSTTYPE# for Perl_cmiss/Makefile.PL

#-----------------------------------------------------------------------------
# compiling commands

CC = cc
CFLAGS = -c
F90FLAGS = -c
DEBUG_CFLAGS = -g
DEBUG_F90FLAGS = -g
DEFINE = '-DABI_ENV="$(ABI_ENV)"'
LD_RELOCATABLE = ld -r
AR = ar
ARFLAGS = -cr
ifeq (${HOSTTYPE},iris4d)
  F90C = f90
  CFLAGS += -$(ABI) -$(INSTRUCTION)
  F90FLAGS += -$(ABI) -$(INSTRUCTION)
  DEBUG_CFLAGS += -DEBUG:trap_uninitialized:subscript_check:verbose_runtime
  OPT_F90FLAGS = -O3
  OPT_CFLAGS = -O3 -OPT:Olimit=0
  ifeq ($(ABI),64)
    OPT_CFLAGS += -r10000
  endif
  F90FLAGS += -$(ABI) -$(INSTRUCTION)
  LD_RELOCATABLE += -$(ABI) -$(INSTRUCTION)
else
  ifeq (${HOSTTYPE},i386-linux)
    F90C = pgf90
    OPT_F90FLAGS = -fast
    DEFINE += -Dbool=char -DHAS_BOOL
  else
    F90C = f90
    OPT_F90FLAGS = -O2
  endif
  OPT_CFLAGS = -O2
endif
ifneq ($(DEBUG),false)
  CFLAGS += $(DEBUG_CFLAGS)
  FFLAGS += $(DEBUG_CFLAGS)
else
  CFLAGS += $(OPT_CFLAGS)
  FFLAGS += $(OPT_CFLAGS)
endif
CPPFLAGS = $(addprefix -I, $(C_INCLUDE_DIRS) $(WORKING_DIR) ) $(DEFINE)

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

ifndef TASK
  TASK =#
endif
ifeq ($(TASK),)
#-----------------------------------------------------------------------------

  .NOTPARALLEL:

# needs some thought
#   TMP_FILES := $(notdir $(wildcard $(WORKING_DIR)/*.* ) )
#   OLD_FILES := $(filter-out $(PMH_FILES) $(foreach unit,$(UNITS),$(unit).%), \
#     $(TMP_FILES))

  .PHONY : main clean all# tidy

  main : Perl_cmiss/$(PERL_CMISS_MAKEFILE) $(WORKING_DIR) $(LIBRARY_DIR)
	$(MAKE) --directory=Perl_cmiss --file=$(PERL_CMISS_MAKEFILE) \
	  CCFLAGS="$(CFLAGS)" DEFINE="$(DEFINE)" static
	$(MAKE) TASK=source
	$(MAKE) TASK=library

  all :
	$(MAKE)
	$(MAKE) OPT=
  ifeq (${HOSTTYPE},iris4d)
	$(MAKE) ABI=64
	$(MAKE) ABI=64 OPT=
  endif

  tidy :
  ifneq ($(OLD_FILES),)
	rm $(foreach file,$(OLD_FILES), $(WORKING_DIR)/$(file) )
  endif

  Perl_cmiss/$(PERL_CMISS_MAKEFILE) : $(PERL) Perl_cmiss/Makefile.PL
	cd Perl_cmiss ; $(PERL) Makefile.PL \
	  CCFLAGS="$(CFLAGS)" DEFINE="$(DEFINE)" \
	  FIRST_MAKEFILE=$(PERL_CMISS_MAKEFILE)

  $(WORKING_DIR) :
	mkdir -p $@

  $(LIBRARY_DIR) :
	mkdir -p $@

  clean:
	@echo "Cleaning house ..."
	rm -rf Perl_cmiss/$(WORKING_DIR) $(WORKING_DIR) $(LIBRARY)

#-----------------------------------------------------------------------------
endif

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

ifeq ($(TASK),source)
#-----------------------------------------------------------------------------

  F90MAKEDEPEND_SCRIPT = make/f90makedepend.pl
  F90MAKEDEPEND := $(PERL) -w $(F90MAKEDEPEND_SCRIPT)

  main : $(DEPEND_FILES) \
    $(foreach file,$(PMH_FILES), $(WORKING_DIR)/$(file) )

  # implicit rules for making the dependency files

  $(WORKING_DIR)/%.d : $(SOURCE_DIR)/%.c
	makedepend $(CPPFLAGS) -f- -Y $< 2> $@.tmp | sed -e 's%^source\([^ ]*\).o%$$(WORKING_DIR)\1.o $$(WORKING_DIR)\1.d%' > $@
	(grep pmh $@.tmp | grep makedepend | awk -F "[ ,]" '{printf("%s.%s:",substr($$4, 1, length($$4) - 2),"o"); for(i = 1 ; i <= NF ; i++)  { if (match($$i,"pmh")) printf(" source/%s", substr($$i, 2, length($$i) -2)) } printf("\n");}' | sed -e 's%^source\([^ ]*\).o%$$(WORKING_DIR)\1.o $$(WORKING_DIR)\1.d%' | sed -e 's%source\([^ ]*\).pmh%$$(WORKING_DIR)\1.pmh%' >> $@)

$(WORKING_DIR)/%.d : $(SOURCE_DIR)/%.f90
	$(F90MAKEDEPEND) $< > $@

$(WORKING_DIR)/%.pmh : $(SOURCE_DIR)/%.pm
	utilities/pm2pmh $< > $@

#-----------------------------------------------------------------------------
endif

#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

ifeq ($(TASK),library)
#-----------------------------------------------------------------------------

  # retain intermediate files
  .PRECIOUS : $(WORKING_DIR)/%.o $(WORKING_DIR)/%.mod_date \
    $(WORKING_DIR)/%.mod_record $(WORKING_DIR)/%.mod

  #-----------------------------------------------------------------------------

  # explicit rule for making the library
  # Including all necessary objects from archives into output archive.
  # This is done by producing a relocatable object first.
  # Is there a better way?

  $(LIBRARY) : $(C_OBJ) $(F_OBJ)
	$(AR) $(ARFLAGS) $@ $^

  $(C_OBJ) : $(foreach unit, $(C_UNITS), $(WORKING_DIR)/$(unit).o ) \
    $(DYNALOADER_LIB) $(PERL_CMISS_LIB) $(PERL_LIB)
	$(LD_RELOCATABLE) -o $@ $^

  $(F_OBJ) : $(foreach unit, $(F_UNITS), $(WORKING_DIR)/$(unit).o )
	$(LD_RELOCATABLE) -o $@ $^

# $(LIBRARY) : $(foreach unit, $(UNITS), $(WORKING_DIR)/$(unit).o ) $(DYNALOADER_LIB) $(PERL_CMISS_LIB) $(PERL_LIB)
# 	@if [ ! -d $(LIBRARY_DIR) ]; then echo mkdir -p $(LIBRARY_DIR); mkdir -p $(LIBRARY_DIR); fi
# 	ld $(LDFLAGS) -o $(TEMP_OBJ) $^
# 	$(AR) $(ARFLAGS) $@ $(TEMP_OBJ)

  #-----------------------------------------------------------------------------
#  # implicit rules for making the object dependencies

# $(WORKING_DIR)/%.d $(WORKING_DIR)/%.o : $(SOURCE_DIR)/%.c
# 	@if [ ! -d $(WORKING_DIR) ]; then echo mkdir -p $(WORKING_DIR); mkdir -p $(WORKING_DIR); fi
# 	$(CC) -o $(WORKING_DIR)/$*.o $(CPPFLAGS) $(CFLAGS) -MDupdate $(WORKING_DIR)/$*.d_tmp $<
# 	sed -e 's%^%$(WORKING_DIR)/%' \
# 		$(WORKING_DIR)/$*.d_tmp > $(WORKING_DIR)/$*.d
# 	rm $(WORKING_DIR)/$*.d_tmp

  #-----------------------------------------------------------------------------
  # include the object (and .mod) dependencies)
  ifneq ($(DEPEND_FILES),)
    include $(DEPEND_FILES)
  endif

  #-----------------------------------------------------------------------------
  # implicit rules for making the objects

  $(WORKING_DIR)/%.o : $(SOURCE_DIR)/%.c
	$(CC) -o $@ $(CPPFLAGS) $(CFLAGS) $<

# $(WORKING_DIR)/%_.o : $(SOURCE_DIR)/%.c
# 	$(CC) -o $@ $(CPPFLAGS) $(CFLAGS) -DFORTRAN_INTERPRETER_INTERFACE $<

  $(WORKING_DIR)/%.o : $(SOURCE_DIR)/%.f90
        # cd so that .mod files end up in WORKING_DIR
	cd $(WORKING_DIR); $(F90C) $(F90FLAGS) ${PWD}/$<

  #-----------------------------------------------------------------------------
  # implicit rules for the f90 modules
  # Keep the last modification times of module files as old as possible

  $(WORKING_DIR)/%.mod_date : $(WORKING_DIR)/%.mod_record ;

  $(WORKING_DIR)/%.mod_record : $(WORKING_DIR)/%.mod
	@if [ -e $@ ] && cmp $@ $<; then \
	  touch $@; \
	else \
	  echo $< has changed; cp $< $@; touch $<_date; \
	fi

  $(WORKING_DIR)/%.mod : ;

#-----------------------------------------------------------------------------
endif
