#!/usr/local/bin/gmake -f
# no builtin implicit rules
MAKEFLAGS = --no-builtin-rules -I

#-----------------------------------------------------------------------------
# files and directories

SOURCE_DIR = source
WORKING_DIR = generated/mips3-n32-debug
LIBRARY_DIR = $(WORKING_DIR)

C_SOURCES := $(wildcard $(SOURCE_DIR)/*.c )
F_SOURCES := $(wildcard $(SOURCE_DIR)/*.f90 )
C_UNITS := $(basename $(notdir $(C_SOURCES) ) )
F_UNITS := $(basename $(notdir $(F_SOURCES) ) )

C_OBJ = $(WORKING_DIR)/libperlinterpreter.o
F_OBJ = $(WORKING_DIR)/libperlinterpreter_f.o
LIBRARY = $(LIBRARY_DIR)/libperlinterpreter.a

PERL_PATH = $(shell perl -MConfig -e 'print "$$Config{archlib}\n"')
DYNALOADER_LIB = $(PERL_PATH)/auto/DynaLoader/DynaLoader.a
PERL_CMISS_LIB = Perl_cmiss/blib/arch/auto/Perl_cmiss/Perl_cmiss.a
PERL_LIB = $(PERL_PATH)/CORE/libperl.a

CINCLUDE_DIRS = $(PERL_PATH)/CORE

#-----------------------------------------------------------------------------
# compiling commands

CC = cc
CFLAGS = -c -g -mips3
CPPFLAGS = $(addprefix -I, $(CINCLUDE_DIRS) $(WORKING_DIR) )
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

$(PERL_CMISS_LIB) : 
	cd Perl_cmiss ; perl Makefile.PL ; $(MAKE) static

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
