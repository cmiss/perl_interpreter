#!/usr/local/bin/gmake -f
# no builtin implicit rules
MAKEFLAGS = --no-builtin-rules -I

#-----------------------------------------------------------------------------

# files and directories

SOURCE_DIR = source
# set architecture dependent directory
ifeq (${HOSTTYPE},iris4d)
  # Specify what application binary interface (ABI) to use i.e. 32, n32 or 64
  ABI = n32
  # Specify what sort of instruction set to use i.e. -mips#
  MIPS = 4
  INSTRUCTION := mips$(MIPS)
  WORKING_DIR := generated/$(INSTRUCTION)-$(ABI)-debug
endif
ifeq (${HOSTTYPE},i386-linux)
  WORKING_DIR = generated/linux-debug
endif
LIBRARY_DIR = $(WORKING_DIR)

C_SOURCES := $(wildcard $(SOURCE_DIR)/*.c )
F_SOURCES := $(wildcard $(SOURCE_DIR)/*.f90 )
C_UNITS := $(basename $(notdir $(C_SOURCES) ) )
F_UNITS := $(basename $(notdir $(F_SOURCES) ) )

C_OBJ = $(WORKING_DIR)/libperlinterpreter.o
F_OBJ = $(WORKING_DIR)/libperlinterpreter_f.o
LIBRARY = $(LIBRARY_DIR)/libperlinterpreter.a

ifeq (${HOSTTYPE},iris4d)
  ifeq ($(INSTRUCTION),mips4)
    ifeq ($(ABI),n32)
      PERL = /usr/local/perl5.6/bin/perl
    else
      PERL = /usr/local/perl5.6/bin-$(ABI)/perl
    endif
  else
    PERL = /usr/local/perl5.6/bin-$(INSTRUCTION)/perl
  endif
endif
ifeq (${HOSTTYPE},i386-linux)
  PERL = /usr/bin/perl
endif

PERL_ARCHLIB = $(shell $(PERL) -MConfig -e 'print "$$Config{archlib}\n"')
DYNALOADER_LIB = $(PERL_ARCHLIB)/auto/DynaLoader/DynaLoader.a
PERL_CMISS_MAKEFILE = $(WORKING_DIR)/Perl_cmiss.make
PERL_CMISS_LIB = $(WORKING_DIR)/auto/Perl_cmiss/Perl_cmiss.a
PERL_LIB = $(PERL_ARCHLIB)/CORE/libperl.a

CINCLUDE_DIRS = $(PERL_ARCHLIB)/CORE

#-----------------------------------------------------------------------------
# compiling commands

CC = cc -c
CFLAGS = -g
CPPFLAGS = $(addprefix -I, $(CINCLUDE_DIRS) $(WORKING_DIR) )
ifeq (${HOSTTYPE},i386-linux)
F90C = pgf90 -c
else
F90C = f90 -c
endif
F90FLAGS = -g
LDFLAGS = -r
ifeq (${HOSTTYPE},iris4d)
CFLAGS += -$(ABI) -$(INSTRUCTION)
F90FLAGS += -$(ABI) -$(INSTRUCTION)
LDFLAGS += -$(ABI) -$(INSTRUCTION)
else
CPPFLAGS += -Dbool=char -DHAS_BOOL
endif
AR = ar
ARFLAGS = -cr

#-----------------------------------------------------------------------------
# retain intermediate files

.PRECIOUS : $(WORKING_DIR)/%.o $(WORKING_DIR)/%.mod_date $(WORKING_DIR)/%.mod_record $(WORKING_DIR)/%.mod

#-----------------------------------------------------------------------------
# explicit rule for making the library
# Including all necessary objects from archives into output archive.
# This is done by producing a relocatable object first.
# Is there a better way?

$(LIBRARY) : $(C_OBJ) $(F_OBJ)
	@if [ ! -d $(LIBRARY_DIR) ]; then echo mkdir -p $(LIBRARY_DIR); mkdir -p $(LIBRARY_DIR); fi
	$(AR) $(ARFLAGS) $@ $^

$(C_OBJ) : $(foreach unit, $(C_UNITS), $(WORKING_DIR)/$(unit).o ) $(DYNALOADER_LIB) $(PERL_CMISS_LIB) $(PERL_LIB)
	ld $(LDFLAGS) -o $@ $^

$(F_OBJ) : $(foreach unit, $(F_UNITS), $(WORKING_DIR)/$(unit).o )
	ld $(LDFLAGS) -o $@ $^

# $(LIBRARY) : $(foreach unit, $(UNITS), $(WORKING_DIR)/$(unit).o ) $(DYNALOADER_LIB) $(PERL_CMISS_LIB) $(PERL_LIB)
# 	@if [ ! -d $(LIBRARY_DIR) ]; then echo mkdir -p $(LIBRARY_DIR); mkdir -p $(LIBRARY_DIR); fi
# 	ld $(LDFLAGS) -o $(TEMP_OBJ) $^
# 	$(AR) $(ARFLAGS) $@ $(TEMP_OBJ)

$(PERL_CMISS_LIB) : $(PERL) Perl_cmiss/Makefile.PL
	cd Perl_cmiss ; $(PERL) Makefile.PL LINKTYPE=static INST_ARCHLIB=../$(WORKING_DIR) FIRST_MAKEFILE=../$(PERL_CMISS_MAKEFILE)
	$(MAKE) --directory=Perl_cmiss --file=../$(PERL_CMISS_MAKEFILE) CCFLAGS="$(CFLAGS) $(CPPFLAGS)" static

#-----------------------------------------------------------------------------
# implicit rules for making the object dependencies

$(WORKING_DIR)/%.d : $(SOURCE_DIR)/%.c
	@if [ ! -d $(WORKING_DIR) ]; then echo mkdir -p $(WORKING_DIR); mkdir -p $(WORKING_DIR); fi
	makedepend $(CPPFLAGS) -f- -Y $< 2> $@.tmp | sed -e 's%^source\([^ ]*\).o%$$(WORKING_DIR)\1.o $$(WORKING_DIR)\1.d%' > $@
	(grep pmh $@.tmp | grep makedepend | awk -F "[ ,]" '{printf("%s.%s:",substr($$4, 1, length($$4) - 2),"o"); for(i = 1 ; i <= NF ; i++)  { if (match($$i,"pmh")) printf(" source/%s", substr($$i, 2, length($$i) -2)) } printf("\n");}' | sed -e 's%^source\([^ ]*\).o%$$(WORKING_DIR)\1.o $$(WORKING_DIR)\1.d%' | sed -e 's%source\([^ ]*\).pmh%$$(WORKING_DIR)\1.pmh%' >> $@)
#	rm $@.tmp

# $(WORKING_DIR)/%_.d : $(SOURCE_DIR)/%.c
# 	@if [ ! -d $(WORKING_DIR) ]; then echo mkdir -p $(WORKING_DIR); mkdir -p $(WORKING_DIR); fi
# 	makedepend $(CPPFLAGS) -DFORTRAN_INTERPRETER_INTERFACE -f- $< | sed -e 's%^source\([^ ]*\).o%$$(WORKING_DIR)\1.o $$(WORKING_DIR)\1.d%' > $@

$(WORKING_DIR)/%.d : $(SOURCE_DIR)/%.f90
	@if [ ! -d $(WORKING_DIR) ]; then echo mkdir -p $(WORKING_DIR); mkdir -p $(WORKING_DIR); fi
	make/f90makedepend.pl $< > $@

$(WORKING_DIR)/%.pmh : $(SOURCE_DIR)/%.pm
	@if [ ! -d $(WORKING_DIR) ]; then echo mkdir -p $(WORKING_DIR); mkdir -p $(WORKING_DIR); fi
	utilities/pm2pmh $< > $@

# $(WORKING_DIR)/%.d $(WORKING_DIR)/%.o : $(SOURCE_DIR)/%.c
# 	@if [ ! -d $(WORKING_DIR) ]; then echo mkdir -p $(WORKING_DIR); mkdir -p $(WORKING_DIR); fi
# 	$(CC) -o $(WORKING_DIR)/$*.o $(CPPFLAGS) $(CFLAGS) -MDupdate $(WORKING_DIR)/$*.d_tmp $<
# 	sed -e 's%^%$(WORKING_DIR)/%' \
# 		$(WORKING_DIR)/$*.d_tmp > $(WORKING_DIR)/$*.d
# 	rm $(WORKING_DIR)/$*.d_tmp

#-----------------------------------------------------------------------------
# include the object (and .mod) dependencies)

include $(foreach unit, $(C_UNITS) $(F_UNITS), $(WORKING_DIR)/$(unit).d )

#-----------------------------------------------------------------------------
# implicit rules for making the objects

$(WORKING_DIR)/%.o : $(SOURCE_DIR)/%.c
	$(CC) -o $@ $(CPPFLAGS) $(CFLAGS) $<

# $(WORKING_DIR)/%_.o : $(SOURCE_DIR)/%.c
# 	$(CC) -o $@ $(CPPFLAGS) $(CFLAGS) -DFORTRAN_INTERPRETER_INTERFACE $<

$(WORKING_DIR)/%.o : $(SOURCE_DIR)/%.f90
	cd $(WORKING_DIR); $(F90C) $(F90FLAGS) ${PWD}/$<

#-----------------------------------------------------------------------------
# implicit rules for the modules
# that keep the last modification times of module files as old as possible

$(WORKING_DIR)/%.mod_date : $(WORKING_DIR)/%.mod_record ;

$(WORKING_DIR)/%.mod_record : $(WORKING_DIR)/%.mod
	@if [ -e $@ ] && cmp $@ $<; then touch $@; else echo $< has changed; cp $< $@; touch $<_date; fi

$(WORKING_DIR)/%.mod : ;
