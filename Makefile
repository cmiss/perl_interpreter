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
ARCH_DIR := $(SYSNAME)-$(ABI)# default
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
    MIPS = 4
    ifeq ($(filter-out esu%,$(NODENAME)),)
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
  ARCH_DIR := i686-linux
endif
ifeq ($(SYSNAME),SunOS)
  ARCH_DIR := solaris-$(ABI)
endif
ifeq ($(SYSNAME),AIX)
  ifndef ABI
    ifdef OBJECT_MODE
      ifneq ($(OBJECT_MODE),32_64)
        ABI = $(OBJECT_MODE)
      endif
    endif
  endif
  ARCH_DIR := aix-$(ABI)
endif

ifndef ABI
  ABI = 32# for preprocess_fortran.pl
endif

ifneq ($(DEBUG),false)
  OPT_SUFFIX = -debug
else
  OPT_SUFFIX = -opt
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
  USE_DYNAMIC_LOADER = false
endif

#This routine is recursivly called for each possible dynamic version
#with SHARED_OBJECT set to true.  That builds the corresponding 
#libperlinterpereter.so
ifndef SHARED_OBJECT
  SHARED_OBJECT = false
endif

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
  # Specify the perl on some platforms so that everyone builds with the same.
  ifeq ($(filter-out IRIX%,$(SYSNAME)),)# SGI
    ifeq ($(filter-out esu%,$(NODENAME)),)
      ifeq ($(INSTRUCTION),mips3)
        PERL = ${CMISS_ROOT}/bin/perl 
      else
        ifeq ($(ABI),n32)
          PERL = ${CMISS_ROOT}/bin/perl
        else
          PERL = ${CMISS_ROOT}/bin/mips-irix/perl64
        endif
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
  ifeq ($(SYSNAME),AIX)
      ifeq ($(ABI),32)
        PERL = ${CMISS_ROOT}/bin/perl
      else
        PERL = ${CMISS_ROOT}/bin/perl64
      endif
  endif
  ifeq ($(filter-out esp56%,$(NODENAME)),)
    PERL = ${CMISS_ROOT}/bin/i686-linux/perl
  endif
  ifndef PERL
    ifeq ($(ABI),64)
      # Need a perl of the same ABI
      PERL = perl64
    else
      # Assume 32-bit and first perl in path is suitable for this architecture
      PERL = perl
    endif
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
PERL_VERSION := $(shell $(PERL) -MConfig -e 'print "$$Config{version}\n"')
ifeq ($(PERL_VERSION),)
  $(error problem with $(PERL))
endif
PERL_CFLAGS := $(shell $(PERL) -MConfig -e 'print "$$Config{ccflags}\n"')
ifeq ($(PERL_CFLAGS),)
  $(error problem with $(CFLAGS))
endif
DYNALOADER_LIB = $(PERL_ARCHLIB)/auto/DynaLoader/DynaLoader.a
PERL_WORKING_DIR = Perl_cmiss/generated/$(PERL_VERSION)/$(PERL_ARCHNAME)
PERL_CMISS_MAKEFILE = $(PERL_WORKING_DIR)/Makefile
PERL_CMISS_LIB = $(PERL_WORKING_DIR)/auto/Perl_cmiss/Perl_cmiss.a
ifneq ($(SHARED_OBJECT), true)
   STATIC_PERL_LIB = $(wildcard $(PERL_ARCHLIB)/CORE/libperl.a)
   ifneq ($(USE_DYNAMIC_LOADER), true)
      ifeq ($(STATIC_PERL_LIB),)
         $(error 'Static $(PERL_ARCHLIB)/CORE/libperl.a not found for ${PERL} which is required for a non dynamic loading perl interpreter.')
      endif
   endif
else
   STATIC_PERL_LIB = 
endif
PERL_EXP = $(wildcard $(PERL_ARCHLIB)/CORE/perl.exp)

#Make architecture directory names and lib name
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
WORKING_DIR := generated/$(PERL_VERSION)/$(PERL_ARCHNAME)$(OPT_SUFFIX)$(SHARED_SUFFIX)
C_INCLUDE_DIRS = $(PERL_ARCHLIB)/CORE $(WORKING_DIR)

LIBRARY_ROOT_DIR := lib/$(ARCH_DIR)
LIBRARY_VERSION := $(PERL_VERSION)/$(PERL_ARCHNAME)$(SHARED_LIB_SUFFIX)
LIBRARY_DIR := $(LIBRARY_ROOT_DIR)/$(LIBRARY_VERSION)
ifneq ($(SHARED_OBJECT), true)
   LIBRARY_SUFFIX = .a
else
   LIBRARY_SUFFIX = .so
endif
LIBRARY_NAME := libperlinterpreter$(OPT_SUFFIX)$(LIBRARY_SUFFIX)
LIBRARY := $(LIBRARY_DIR)/$(LIBRARY_NAME)
LIBRARY_LINK := $(LIBRARY_ROOT_DIR)/libperlinterpreter$(OPT_SUFFIX)$(LIBRARY_SUFFIX)
LIB_EXP := $(patsubst %$(LIBRARY_SUFFIX), %.exp, $(LIBRARY))

SOURCE_FILES := $(notdir $(wildcard $(SOURCE_DIR)/*.*) )
PMH_FILES := $(patsubst %.pm, %.pmh, $(filter %.pm, $(SOURCE_FILES)))
C_SOURCES := $(filter %.c, $(SOURCE_FILES) )
C_UNITS := $(basename $(C_SOURCES) )
DEPEND_FILES := $(foreach unit, $(C_UNITS), $(WORKING_DIR)/$(unit).d )

C_OBJ := $(WORKING_DIR)/libperlinterpreter.o


#-----------------------------------------------------------------------------
# compiling commands

CC = cc
LD_RELOCATABLE = ld -r $(CFL_FLGS) $(L_FLGS)
LD_SHARED = ld -shared $(CFL_FLGS) $(L_FLGS)
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

ifeq ($(filter-out IRIX%,$(SYSNAME)),)# SGI
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
ifeq ($(SHARED_OBJECT), true)
  CPPFLAGS += -DSHARED_OBJECT
endif
ifeq ($(USE_DYNAMIC_LOADER), true)
  CPPFLAGS += -DUSE_DYNAMIC_LOADER
endif
ifneq ($(SHARED_OBJECT), true)
  ifeq ($(STATIC_PERL_LIB),)
    CPPFLAGS += -DNO_STATIC_FALLBACK
  endif
endif
CFLAGS += $(PERL_CFLAGS)
.PHONY : main

vpath $(PERL) $(subst :, ,$(PATH))

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

SHARED_PERL_EXECUTABLES =
define VERSION_MESSAGE
   @echo '     Version $(shell ${perl_executable} -MConfig -e 'print "$$Config{version} $$Config{archname}"') ${perl_executable}'

endef
ifeq ($(USE_DYNAMIC_LOADER),true)
   #Dynamic loading perl interpreter
   #Note that the blank line in the define is useful.
   define SHARED_BUILD_RULE
      $(MAKE) --no-print-directory USE_DYNAMIC_LOADER=false SHARED_OBJECT=true CMISS32_PERL=$(perl_executable)

   endef
   ifneq ($(wildcard ${CMISS_ROOT}/perl),)
      SHARED_PERL_EXECUTABLES += $(wildcard ${CMISS_ROOT}/perl/bin-5.?.?-i386-linux*/perl)
      SHARED_PERL_EXECUTABLES += $(wildcard ${CMISS_ROOT}/perl/bin-5.?.?-i686-linux*/perl)
   else
      SHARED_PERL_EXECUTABLES += ${PERL}
   endif
   SHARED_INTERPRETER_BUILDS = $(foreach perl_executable, $(SHARED_PERL_EXECUTABLES), $(SHARED_BUILD_RULE))
   SHARED_VERSION_STRINGS = $(foreach perl_executable, $(SHARED_PERL_EXECUTABLES), $(shell ${perl_executable} -MConfig -e 'print "$$Config{version}/$$Config{archname}"'))
   SHARED_LIBRARIES = $(foreach version_string, $(SHARED_VERSION_STRINGS), $(LIBRARY_ROOT_DIR)/$(version_string)/libperlinterpreter$(OPT_SUFFIX).so)
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
      @echo 'It will work dynamically with the following versions of perl:'
      $(foreach perl_executable, $(SHARED_PERL_EXECUTABLES), $(VERSION_MESSAGE))
      ${SUB_WRITE_BUILD_MESSAGE}
   endef
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

  main : $(PERL_CMISS_MAKEFILE) $(PERL_WORKING_DIR) $(WORKING_DIR) $(LIBRARY_DIR)
ifeq ($(USE_DYNAMIC_LOADER),true)
	$(SHARED_INTERPRETER_BUILDS)
endif
	@echo
	@echo 'Building library ${LIBRARY}'
	@echo
	$(MAKE) --directory=$(PERL_WORKING_DIR) static
ifeq ($(USE_DYNAMIC_LOADER),true)
	$(MAKE) --no-print-directory TASK=source SHARED_LIBRARIES='$(SHARED_LIBRARIES)'
else
	$(MAKE) --no-print-directory TASK=source
endif
	$(MAKE) --no-print-directory TASK=library
	$(WRITE_BUILD_MESSAGE)

  tidy :
  ifneq ($(OLD_FILES),)
	rm $(foreach file,$(OLD_FILES), $(WORKING_DIR)/$(file) )
  endif

  $(PERL_CMISS_MAKEFILE) : $(PERL) Perl_cmiss/Makefile.PL
	cd Perl_cmiss ; $(PERL) Makefile.PL

  $(PERL_WORKING_DIR) :
	mkdir -p $@

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
  ifeq ($(filter-out IRIX%,$(SYSNAME)),) #SGI
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

  # include the depend file dependencies
  ifneq ($(DEPEND_FILES),)
    sinclude $(DEPEND_FILES)
  endif

  # implicit rules for making the dependency files

  # KAT I think Solaris needed nawk rather than awk, but nawk is not usually
  # avaiable on Mandrake.  I don't have a Sun to try this out so I'll get it
  # working with awk on the machines I have.
  $(WORKING_DIR)/%.d : $(SOURCE_DIR)/%.c
	makedepend $(CPPFLAGS) -f- -Y $< 2> $@.tmp | sed -e 's%^source\([^ ]*\).o%$$(WORKING_DIR)\1.o $$(WORKING_DIR)\1.d%' > $@
# See if there is a dependency on perl
	@if grep /perl\\.h $@ > /dev/null; then set -x; echo '$$(WORKING_DIR)/perl_interpreter.o $$(WORKING_DIR)/perl_interpreter.d: $$(PERL)' >> $@; fi
	(grep pmh $@.tmp | grep makedepend | awk -F "[ ,]" '{printf("%s.%s:",substr($$4, 1, length($$4) - 2),"o"); for(i = 1 ; i <= NF ; i++)  { if (match($$i,"pmh")) printf(" source/%s", substr($$i, 2, length($$i) -2)) } printf("\n");}' | sed -e 's%^$(SOURCE_DIR)\([^ ]*\).o%$$(WORKING_DIR)\1.o $$(WORKING_DIR)\1.d%' | sed -e 's%$(SOURCE_DIR)\([^ ]*\).pmh%$$(WORKING_DIR)\1.pmh%' >> $@)

$(WORKING_DIR)/%.pmh : $(SOURCE_DIR)/%.pm
	utilities/pm2pmh $< > $@

#Dynamic loader code for putting shared objects into the interpreter
ifeq ($(USE_DYNAMIC_LOADER),true)
   ifeq ($(SHARED_LIBRARIES),)
      $(error Missing list of SHARED_LIBRARIES in source stage)
   endif
   SHARED_LIBRARY_HEADERS = $(patsubst %.so, %.soh, $(SHARED_LIBRARIES))

   UID2UIDH = ${CMISS_ROOT}/cmgui/utilities/i686-linux/uid2uidh

  .SUFFIXES : .so .soh

  # implicit rules for making the objects
  %.soh : %.so
	$(UID2UIDH) $< $@ libperlinterpreter

  #Always regenerate the dynamic_versions file as it has recorded for
  #us the versions that are built into this executable
  $(WORKING_DIR)/dynamic_versions.h.new : $(SHARED_LIBRARY_HEADERS)
	echo -n > $@;
	$(foreach header, $(SHARED_LIBRARY_HEADERS), \
      echo '#define libperlinterpreter_uidh libperlinterpreter$(word 3, $(subst /,' ',$(subst .,_,$(header))))$(word 4, $(subst /,' ',$(subst -,_,$(header))))' >> $@; \
      echo '#include "../../../$(header)"' >> $@; \
      echo '#undef libperlinterpreter_uidh' >> $@; )
	echo 'static struct Interpreter_library_strings interpreter_strings[] = {' >> $@;
	$(foreach header, $(SHARED_LIBRARY_HEADERS), \
      echo '{"$(word 3, $(subst /,' ',$(header)))","$(word 4, $(subst /,' ',$(header)))", libperlinterpreter$(word 3, $(subst /,' ',$(subst .,_,$(header))))$(word 4, $(subst /,' ',$(subst -,_,$(header)))) },' >> $@; )
	echo '};' >> $@;
	if [ ! -f $(WORKING_DIR)/dynamic_versions.h ] || ! diff $(WORKING_DIR)/dynamic_versions.h $(WORKING_DIR)/dynamic_versions.h.new > /dev/null ; then \
		mv $(WORKING_DIR)/dynamic_versions.h.new $(WORKING_DIR)/dynamic_versions.h ; \
	else \
		rm $(WORKING_DIR)/dynamic_versions.h.new; \
	fi

#Always build the .new and see if it should be updated.
   .PHONY: $(WORKING_DIR)/dynamic_versions.h.new
   main: $(WORKING_DIR)/dynamic_versions.h.new
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
	ln -s $(LIBRARY_VERSION)/$(LIBRARY_NAME) $(LIBRARY_LINK)
endif

  # explicit rule for making the library
  # Including all necessary objects from archives into output archive.
  # This is done by producing a relocatable object first.
  # Is there a better way?

  ifneq ($(SHARED_OBJECT), true)
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
         $(DYNALOADER_LIB) $(PERL_CMISS_LIB) $(STATIC_PERL_LIB)
		$(LD_RELOCATABLE) -o $@ $^
  else
    $(LIBRARY) : $(foreach unit, $(C_UNITS), $(WORKING_DIR)/$(unit).o ) \
         $(DYNALOADER_LIB) $(PERL_CMISS_LIB) $(STATIC_PERL_LIB)
		$(LD_SHARED) -o $@ $^ -lcrypt -lc
  endif

  # include the object dependencies
  ifneq ($(DEPEND_FILES),)
    include $(DEPEND_FILES)
  endif

  # implicit rules for making the objects
  $(WORKING_DIR)/%.o : $(SOURCE_DIR)/%.c
	$(CC) -o $@ $(CPPFLAGS) $(CFLAGS) $<

#Dynamic loader code for putting shared objects into the interpreter
ifeq ($(USE_DYNAMIC_LOADER),true)

  $(WORKING_DIR)/perl_interpreter.o : $(WORKING_DIR)/dynamic_versions.h
endif


#-----------------------------------------------------------------------------
endif

#-----------------------------------------------------------------------------

