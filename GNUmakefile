#!/usr/local/bin/gmake -f
# no builtin implicit rules
MAKEFLAGS = --no-builtin-rules -I

#-----------------------------------------------------------------------------
# files and directories

SOURCE_DIR = source
WORKING_DIR = generated/mips3-n32-debug
LIBRARY_DIR = $(WORKING_DIR)

# SOURCES := $(wildcard $(SOURCE_DIR)/*.c )
SOURCES := $(wildcard $(SOURCE_DIR)/*.c $(SOURCE_DIR)/*.f90 )
UNITS := $(basename $(notdir $(SOURCES) ) )

TEMP_OBJ = $(WORKING_DIR)/libperlinterpreter.o
LIBRARY = $(LIBRARY_DIR)/libperlinterpreter.a

PERL_PATH = /usr/local/perl5/lib/5.00503
DYNALOADER_LIB = $(PERL_PATH)/irix-n32/auto/DynaLoader/DynaLoader.a
PERL_CMISS_LIB = /usr/people/blackett/cmgui/source/command/Perl_cmiss/blib/arch/auto/Perl_cmiss/Perl_cmiss.a
PERL_LIB = $(PERL_PATH)/irix-n32/CORE/libperl.a

CINCLUDE_DIRS = $(PERL_PATH)/irix-n32/CORE

#-----------------------------------------------------------------------------
# compiling commands

CC = cc
CFLAGS = -c -g -mips3
CPPFLAGS = $(addprefix -I, $(CINCLUDE_DIRS) )
F90C = f90
F90FLAGS = -c -g -mips3
LDFLAGS = -r -mips3
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

$(LIBRARY) : $(foreach unit, $(UNITS), $(WORKING_DIR)/$(unit).o ) $(DYNALOADER_LIB) $(PERL_CMISS_LIB) $(PERL_LIB)
	@if [ ! -d $(LIBRARY_DIR) ]; then echo mkdir -p $(LIBRARY_DIR); mkdir -p $(LIBRARY_DIR); fi
	ld $(LDFLAGS) -o $(TEMP_OBJ) $^
	$(AR) $(ARFLAGS) $@ $(TEMP_OBJ)
# 	rm $(TEMP_OBJ)

#-----------------------------------------------------------------------------
# implicit rules for making the object dependencies

$(WORKING_DIR)/%.d : $(SOURCE_DIR)/%.c
	@if [ ! -d $(WORKING_DIR) ]; then echo mkdir -p $(WORKING_DIR); mkdir -p $(WORKING_DIR); fi
	makedepend $(CPPFLAGS) -f- $< | sed -e 's%^source\([^ ]*\).o%$$(WORKING_DIR)\1.o $$(WORKING_DIR)\1.d%' > $@

# $(WORKING_DIR)/%_.d : $(SOURCE_DIR)/%.c
# 	@if [ ! -d $(WORKING_DIR) ]; then echo mkdir -p $(WORKING_DIR); mkdir -p $(WORKING_DIR); fi
# 	makedepend $(CPPFLAGS) -DFORTRAN_INTERPRETER_INTERFACE -f- $< | sed -e 's%^source\([^ ]*\).o%$$(WORKING_DIR)\1.o $$(WORKING_DIR)\1.d%' > $@

$(WORKING_DIR)/%.d : $(SOURCE_DIR)/%.f90
	@if [ ! -d $(WORKING_DIR) ]; then echo mkdir -p $(WORKING_DIR); mkdir -p $(WORKING_DIR); fi
	make/f90makedepend.pl $< > $@

# $(WORKING_DIR)/%.d $(WORKING_DIR)/%.o : $(SOURCE_DIR)/%.c
# 	@if [ ! -d $(WORKING_DIR) ]; then echo mkdir -p $(WORKING_DIR); mkdir -p $(WORKING_DIR); fi
# 	$(CC) -o $(WORKING_DIR)/$*.o $(CPPFLAGS) $(CFLAGS) -MDupdate $(WORKING_DIR)/$*.d_tmp $<
# 	sed -e 's%^%$(WORKING_DIR)/%' \
# 		$(WORKING_DIR)/$*.d_tmp > $(WORKING_DIR)/$*.d
# 	rm $(WORKING_DIR)/$*.d_tmp

#-----------------------------------------------------------------------------
# include the object (and .mod) dependencies)

include $(foreach unit, $(UNITS), $(WORKING_DIR)/$(unit).d )

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
