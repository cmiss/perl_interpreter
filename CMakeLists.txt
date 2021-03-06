
 # ***** BEGIN LICENSE BLOCK *****
 # Version: MPL 1.1/GPL 2.0/LGPL 2.1
 #
 # The contents of this file are subject to the Mozilla Public License Version
 # 1.1 (the "License"); you may not use this file except in compliance with
 # the License. You may obtain a copy of the License at
 # http://www.mozilla.org/MPL/
 #
 # Software distributed under the License is distributed on an "AS IS" basis,
 # WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 # for the specific language governing rights and limitations under the
 # License.
 #
 # The Original Code is perl interpreter
 #
 # The Initial Developer of the Original Code is
 # Auckland Uniservices Ltd, Auckland, New Zealand.
 # Portions created by the Initial Developer are Copyright (C) 2005
 # the Initial Developer. All Rights Reserved.
 #
 # Contributor(s):
 #
 # Alternatively, the contents of this file may be used under the terms of
 # either the GNU General Public License Version 2 or later (the "GPL"), or
 # the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 # in which case the provisions of the GPL or the LGPL are applicable instead
 # of those above. If you wish to allow use of your version of this file only
 # under the terms of either the GPL or the LGPL, and not to allow others to
 # use your version of this file under the terms of the MPL, indicate your
 # decision by deleting the provisions above and replace them with the notice
 # and other provisions required by the GPL or the LGPL. If you do not delete
 # the provisions above, a recipient may use your version of this file under
 # the terms of any one of the MPL, the GPL or the LGPL.
 #
 # ***** END LICENSE BLOCK *****

# This perl interpreter build is only designed to work with perl version
# greater than or equal to 5.10.0
# Use the following variables when configuring the build from the command line to
# set the corresponding cache variables.
# INSTALL_PREFIX ==> PERL_INTERPRETER_INSTALL_PREFIX
# ROOT ==> PERL_INTERPRETER_ROOT
# EXTRA_PERLS ==> PERL_INTERPRETER_EXTRA_PERLS
# STATIC_LIBPERL ==> PERL_INTERPRETER_STATIC_LIBPERL

CMAKE_MINIMUM_REQUIRED( VERSION 3.10 )

project(CmissPerlInterpreter C)

SET( PERL_INTERPRETER_INSTALL_PREFIX "" CACHE PATH "Installation prefix for the perl interpreter." )
SET( PERL_INTERPRETER_ROOT "" CACHE PATH "Root path for the perl interpreter." )
SET( PERL_INTERPRETER_EXTRA_PERLS "" CACHE STRING "A list of paths to other perls for inclusion in the perl interpreter." )
SET( PERL_INTERPRETER_STATIC_LIBPERL "" CACHE PATH "Path to a perl executable that has a static perl library." )
IF( INSTALL_PREFIX )
	SET( PERL_INTERPRETER_INSTALL_PREFIX "${INSTALL_PREFIX}" CACHE PATH "Installation prefix for the perl interpreter." FORCE )
ENDIF()
IF( ROOT )
	SET( PERL_INTERPRETER_ROOT "${ROOT}" CACHE PATH "Root path for the perl interpreter." FORCE )
ENDIF()
IF( EXTRA_PERLS )
	SET( PERL_INTERPRETER_EXTRA_PERLS "${EXTRA_PERLS}" CACHE STRING "A list of paths to other perls for inclusion in the perl interpreter." FORCE )
ENDIF()
IF( STATIC_LIBPERL )
	SET( PERL_INTERPRETER_STATIC_LIBPERL "${STATIC_LIBPERL}" CACHE PATH "Path to a perl executable that has a static perl library." FORCE )
ENDIF()
unset(STATIC_LIBPERL CACHE)

# The twopaths that we are interested in
FILE( TO_CMAKE_PATH "${PERL_INTERPRETER_INSTALL_PREFIX}" PERL_INTERPRETER_INSTALL_PREFIX )
FILE( TO_CMAKE_PATH "${PERL_INTERPRETER_ROOT}" PERL_INTERPRETER_ROOT )

# We need a path to our own CMake modules.  This has the MacroDefinitions.cmake file
# that contains common macro defintions that are shared between projects.
SET( FOUND_MACRODEFINITIONS_MODULE NO )
SET( PERL_INTERPRETER_DEFAULT_MODULES_PATH "utilities/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules" )
IF( EXISTS "${PERL_INTERPRETER_ROOT}" AND NOT PERL_INTERPRETER_INSTALL_PREFIX )
	SET( PERL_INTERPRETER_INSTALL_PREFIX "${PERL_INTERPRETER_ROOT}/install" CACHE PATH "Installation prefix for the perl interpreter." FORCE )
ENDIF()

SET( PERL_INTERPRETER_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" CACHE PATH "Installation prefix for the perl interpreter.")

IF( NOT IS_ABSOLUTE "${PERL_INTERPRETER_INSTALL_PREFIX}" )
	GET_FILENAME_COMPONENT(PERL_INTERPRETER_INSTALL_PREFIX "${PERL_INTERPRETER_INSTALL_PREFIX}" REALPATH)
ENDIF()

IF( NOT IS_ABSOLUTE "${PERL_INTERPRETER_ROOT}" )
	GET_FILENAME_COMPONENT(PERL_INTERPRETER_ROOT "${PERL_INTERPRETER_ROOT}" REALPATH)
ENDIF()

SET( CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" )
INCLUDE( PerlInterpreterMacros )

# Set the project name different from the target name otherwise confusion
# insues with IDEs, thus the lack of "_" separator
PROJECT( "PerlInterpreter" )

SET( CMAKE_DEBUG_POSTFIX "d" )
SET( CMAKE_MFC_FLAG 0 )

# We must use either nmake or make for the perl interpreter
# as this has to match what the Perl library was built with.
IF( WIN32 )
	SET( MAKE_PROGRAM nmake )
ELSE( WIN32 )
	SET( MAKE_PROGRAM make )
ENDIF( WIN32 )

# Define the system architecture
STRING( TOLOWER ${CMAKE_SYSTEM_NAME} OPERATING_SYSTEM )
set(BIN_ARCH_DIR ${CMAKE_SYSTEM_PROCESSOR}-${OPERATING_SYSTEM})
#message(STATUS "BIN_ARCH_DIR: ${BIN_ARCH_DIR}")
#SET( LIB_ARCH_DIR ${CMAKE_SYSTEM_PROCESSOR}${ABI}-${OPERATING_SYSTEM} )

SET( PERL_INTERPRETER_INSTALL_PREFIX_TEMP ${PERL_INTERPRETER_INSTALL_PREFIX} )

option(WITH_DEBUG_OUTPUT "Turn on debug output printing to stdout." OFF)
OPTION( USE_ARCHITECTURE_DIR "Set true if cross-compiling or colocating libraries from different systems." OFF )
# Set build type and install location
IF( USE_ARCHITECTURE_DIR )
message(STATUS "USE_ARCHITECTURE_DIR is disabled.")
#	STRING( FIND "${PERL_INTERPRETER_INSTALL_PREFIX_TEMP}"
#		"/${ARCHITECTURE_DIR}" CONTAINS )
#	IF( ${CONTAINS} LESS 0 )
#		SET( PERL_INTERPRETER_INSTALL_PREFIX_TEMP
#			"${PERL_INTERPRETER_INSTALL_PREFIX_TEMP}/${ARCHITECTURE_DIR}" )
#	ELSE( ${CONTAINS} LESS 0 )
#		SET( PERL_INTERPRETER_INSTALL_PREFIX
#			"${PERL_INTERPRETER_INSTALL_PREFIX_TEMP}" )
#	ENDIF( ${CONTAINS} LESS 0 )
ELSE( USE_ARCHITECTURE_DIR )
#	STRING( REGEX REPLACE "/${ARCHITECTURE_DIR}" ""
#		PERL_INTERPRETER_INSTALL_PREFIX_TEMP ${PERL_INTERPRETER_INSTALL_PREFIX_TEMP} )
ENDIF( USE_ARCHITECTURE_DIR )
UNSET( PERL_INTERPRETER_INSTALL_PREFIX CACHE )
set( PERL_INTERPRETER_INSTALL_PREFIX "${PERL_INTERPRETER_INSTALL_PREFIX_TEMP}"
	CACHE PATH "Where do you want your perl interpreter libraries/executable installed?" )
SET( CMAKE_INSTALL_PREFIX ${PERL_INTERPRETER_INSTALL_PREFIX} CACHE INTERNAL "Internalise CMAKE_INSTALL_PREFIX, manipulate via CMGUI_INSTALL_PREFIX" FORCE )
MARK_AS_ADVANCED( CMAKE_INSTALL_PREFIX )

IF( NOT BUILD_TYPE )
	SET( BUILD_TYPE Release )
ENDIF()
set( PERL_INTERPRETER_BUILD_TYPE "${BUILD_TYPE}"
	CACHE STRING "What configuration do you want to build: Debug, Release, MinSizeRel, RelWithDebugInfo" )
SET( CMAKE_BUILD_TYPE ${PERL_INTERPRETER_BUILD_TYPE} CACHE INTERNAL "Internalise CMAKE_BUILD_TYPE, manipulate via CMGUI_BUILD_TYPE" FORCE )
MARK_AS_ADVANCED( CMAKE_BUILD_TYPE )

IF( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
	SET( DEBUG_TARGET_SUFFIX "d" )
ENDIF( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )

# Actually start the perl interpreter build
# Set the ABI for the build
SET( ABI_ENV_DEFINE ABI_ENV=\"${CMAKE_SYSTEM_PROCESSOR}\" )#${CMAKE_SYSTEM_PROCESSOR} )

# Find the system perl
#unset(PERL_EXECUTABLE CACHE)
find_package(Perl REQUIRED)
if (APPLE)
  if (NOT PERL_EXECUTABLE OR PERL_EXECUTABLE STREQUAL "/usr/bin/perl")
    # This won't do we cannot use the macOS system perl.
    # The macOS system perl does not provide EXETERN.h header file (among others).
    message(STATUS "Rejecting current perl '${PERL_EXECUTABLE}'.")
    unset(PERL_EXECUTABLE CACHE)
    # Let's try and find a perlbrew perl.
    file(GLOB children LIST_DIRECTORIES TRUE $ENV{HOME}/perl*/perlbrew/perls/*)
    if (children)
      list(GET children 0 first_perl_directory)
      set(_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
      set(CMAKE_PREFIX_PATH ${first_perl_directory})
      find_package(Perl REQUIRED)
      set(CMAKE_PREFIX_PATH ${_CMAKE_PREFIX_PATH})
    endif()
    # If that failed start searching for brew installed perl?
    if (NOT PERL_EXECUTABLE)
      set(_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
      set(CMAKE_PREFIX_PATH /usr/local/bin/)
      find_package(Perl REQUIRED)
      set(CMAKE_PREFIX_PATH ${_CMAKE_PREFIX_PATH})
    endif()
  endif()
endif()

#FIND_PACKAGE( PerlLibs REQUIRED )

# Find the perls listed under PERL_INTERPRETER_EXTRA_PERLS
# ... How to do this, set the variable PERL_INTERPRETER_EXTRA_PERLS to a list of
# perl install directories.  For instance the prefix used to build your own
# perl.  Each directory should be separated by a semi-colon. e.g.
#SET( PERL_INTERPRETER_EXTRA_PERLS "$HOME/perls/5.14;$HOME/perls/5.16" )

# Set the sources for the perl interpreter
SET( PERL_INTERPRETER_SRCS source/perl_interpreter.c )

# Set the perl modules
SET( PERL_MODULES strict.pm Balanced.pm Perl_cmiss.pm )

# Set necessary os specific libraries
IF( WIN32 )
	SET( OS_SPECIFIC_LIBRARIES wsock32 )
ENDIF()

ADD_DEFINITIONS( -DBUILD_WITH_CMAKE )
IF( WIN32 )
	ADD_DEFINITIONS( -D_CRT_SECURE_NO_WARNINGS )
ENDIF()
if(WITH_DEBUG_OUTPUT)
	ADD_DEFINITIONS( -DCMISS_DEBUG )
ENDIF()

# Build bin2base64str executable
SET( BIN2BASE64H_EXECUTABLE_NAME bin2base64str )
SET( CMAKE_OSX_ARCHITECTURES x86_64 )
ADD_EXECUTABLE( ${BIN2BASE64H_EXECUTABLE_NAME} utilities/bin2base64str.c source/base64.c )
SET_TARGET_PROPERTIES( ${BIN2BASE64H_EXECUTABLE_NAME} PROPERTIES
	INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/source )

GET_FILENAME_COMPONENT( PERL_EXECUTABLE_PATH ${PERL_EXECUTABLE} PATH )
LIST( APPEND DYNAMIC_PERLS ${PERL_INTERPRETER_EXTRA_PERLS} ${PERL_EXECUTABLE_PATH} )
SET( SYSTEM_DYNAMIC_PERL "${PERL_EXECUTABLE_PATH}" )

SET( USE_DYNAMIC_LOADER FALSE )
SET( SHARED_OBJECT TRUE ) # May not be a good idea for Windows
SET( LIBRARY_TYPE_SUFFIX "-shared" )

SET( SHARED_PERLINTERPRETER_LIBRARY_TARGETS )
SET( INDEX 1 )
FOREACH( dynamic_perl ${DYNAMIC_PERLS})
	SET( cur_perl_exe_name PERL_EXECUTABLE_${INDEX} )
	# Reset the search for this perl
	UNSET( ${cur_perl_exe_name} CACHE )
	FIND_PROGRAM( ${cur_perl_exe_name}
		NAMES perl
		PATHS ${dynamic_perl}/bin
		${dynamic_perl}
		NO_DEFAULT_PATH)
	MARK_AS_ADVANCED( ${cur_perl_exe_name} )
	SET( cur_perl_exe ${${cur_perl_exe_name}} )
	IF( cur_perl_exe )
	
		# Get the perl strings for the current perl
		DEFINE_PERL_ARCHLIB_DIR( ${cur_perl_exe} )
		DEFINE_PERL_API_STRING( ${cur_perl_exe} )
		DEFINE_PERL_C_FLAGS( ${cur_perl_exe} )
		# Separate perl c flags into include directories, link
		# libraries and compiler flags.
		# Sets PERL_INCLUDE_DIRS, PERL_LINK_LIBRARIES and PERL_COMPILER_FLAGS
		SEPARATE_PERL_C_FLAGS( ${PERL_C_FLAGS} ${cur_perl_exe} )
	
		LIST(FIND PERL_LINK_LIBRARIES "crypt" CRYPT_INDEX)
		SET( CRYPT_LIBRARIES )
		IF( NOT CRYPT_INDEX EQUAL -1 )
			SET( CRYPT_LIBRARIES crypt )
		ENDIF()
		
		IF( ${dynamic_perl} STREQUAL ${SYSTEM_DYNAMIC_PERL} )
			SET( NO_DEFAULT_PATH FALSE )
		ELSE()
			SET( NO_DEFAULT_PATH TRUE )
		ENDIF()
		FIND_PERL_LIBRARY( ${cur_perl_exe} ${NO_DEFAULT_PATH} )
                if (NOT PERL_LIBRARY)
                  continue()
                  # Leave this idea for another day.
                  # If I cannot find a dynamic perl library can I make one from a static perl library?
                  # I would need to store this a base64 a la the perl interpreter.
                  # set(ACTIVE_DYNAMIC_PERL_TARGET dynamicperl_${INDEX})
                  # add_library(${ACTIVE_DYNAMIC_PERL_TARGET} SHARED source/empty.c)
                  # target_link_libraries(${ACTIVE_DYNAMIC_PERL_TARGET} ${PERL_STATIC_LIBRARY})
                  # target_link_options(${ACTIVE_DYNAMIC_PERL_TARGET} PRIVATE LINKER:-all_load)
                  # set(PERL_LIBRARY ${ACTIVE_DYNAMIC_PERL_TARGET})
                endif()
		SET( PERL_LIBRARIES ${PERL_LIBRARY} )
                IF(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX))
			# We actually don't need the perl libraries in this situation so we won't add them and thus remove
			# the necessity for rpath information.
			SET( PERL_LIBRARIES )
		ENDIF()
		# Set the perl API string with valid characters for use in c functions
		STRING( REGEX REPLACE "(\\.|-)" "_" PERL_API_STRING_STANDARD_CHARACTERS ${PERL_API_STRING} )
		SET( PERL_VERSION_ARCHNAME_DEFINE PERL_VERSION_ARCHNAME=\"${PERL_API_STRING}\" )
	
		SET( CMISS_PERL_CALLBACK cmiss_perl_callback_${PERL_API_STRING_STANDARD_CHARACTERS} )
		SET( CMISS_PERL_WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Perl_cmiss/generated_cmake/${BIN_ARCH_DIR}/${PERL_API_STRING} )
		# Set the Perl_cmiss library /*install*/ location on a build type basis
		SET( PERL_CMISS_LIB
			${CMISS_PERL_WORKING_DIRECTORY}/auto/Perl_cmiss/Perl_cmiss${CMAKE_STATIC_LIBRARY_SUFFIX} )
	
		# this version of the perl cmiss library won't need to be installed to be used as it is
		# already included in the shared object perl interpreter!
		# Can't use symlinks on Windows so using the copy version for windows and linux alike
		ADD_CUSTOM_TARGET( PERL_CMISS_MAKEFILE_${INDEX}
			COMMAND ${CMAKE_COMMAND} -E make_directory Perl_cmiss
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/Perl_cmiss/Makefile_cmake.PL ${PROJECT_BINARY_DIR}/Perl_cmiss/Makefile.PL
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/Perl_cmiss/perl_cmiss.h ${PROJECT_BINARY_DIR}/Perl_cmiss/perl_cmiss.h
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/Perl_cmiss/Perl_cmiss.pm ${PROJECT_BINARY_DIR}/Perl_cmiss/Perl_cmiss.pm
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/Perl_cmiss/Perl_cmiss.xs ${PROJECT_BINARY_DIR}/Perl_cmiss/Perl_cmiss.xs
			COMMAND ${CMAKE_COMMAND} -E chdir ${PROJECT_BINARY_DIR}/Perl_cmiss ${cur_perl_exe} Makefile.PL ${CMISS_PERL_CALLBACK} ${CMISS_PERL_WORKING_DIRECTORY} ${CMAKE_BUILD_TYPE}
			COMMENT "Running PERL_CMISS_MAKEFILE CMISS_PERL_CALLBACK=${CMISS_PERL_CALLBACK} WORKING_DIR=${CMISS_PERL_WORKING_DIRECTORY} ${CMAKE_BUILD_TYPE}" )
		ADD_CUSTOM_TARGET( MAKE_PERL_CMISS_${INDEX}
			COMMAND ${MAKE_PROGRAM} static
			WORKING_DIRECTORY ${CMISS_PERL_WORKING_DIRECTORY}
			COMMENT "Making ${PERL_API_STRING}${LIBRARY_TYPE_SUFFIX}" )
		ADD_DEPENDENCIES( MAKE_PERL_CMISS_${INDEX} PERL_CMISS_MAKEFILE_${INDEX} )
		SET( GENERATED_FILES_DIR ${PROJECT_BINARY_DIR}/generated/${BIN_ARCH_DIR}/${PERL_API_STRING}${DEBUG_TARGET_SUFFIX}${LIBRARY_TYPE_SUFFIX} )
		FILE( MAKE_DIRECTORY "${GENERATED_FILES_DIR}" )
	
		IF( USE_DYNAMIC_LOADER )
			SET( DYNAMIC_LOADER_DEFINE USE_DYNAMIC_LOADER)
		ELSE()
			SET( DYNAMIC_LOADER_DEFINE INCLUDE_DYNALOADERPMH )
		ENDIF()
	
		IF( SHARED_OBJECT )
			SET( SHARED_OBJECT_DEFINE SHARED_OBJECT )
		ELSE()
			SET( SHARED_OBJECT_DEFINE )
		ENDIF()
	                                   
		SET( REQUIRE_MKSTEMP_DEFINITION )
		SET( REQUIRE_MKSTEMP_DECLARATION )
	
		IF(WIN32)
			# Assuming here that perl include dirs is only one directory
			TRY_RUN(RUN_RESULT_VAR COMPILE_RESULT_VAR "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/utilities/mkstemp_check.c" 
				COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT
				COMPILE_DEFINITIONS "/I${PERL_INCLUDE_DIRS}"
				LINK_LIBRARIES ${PERL_LIBRARY})
			STRING(FIND "${COMPILE_OUTPUT}" "mkstemp already defined" MKSTEMP_ALREADY_DEFINED_INDEX)
			IF (MKSTEMP_ALREADY_DEFINED_INDEX EQUAL -1)
				SET(REQUIRE_MKSTEMP_DEFINITION REQUIRE_MKSTEMP_DEFINITION)
			ELSE()
				SET( REQUIRE_MKSTEMP_DECLARATION REQUIRE_MKSTEMP_DECLARATION )
			ENDIF()
		ENDIF()
	
		SET( LIBRARY_DEFINITIONS ${PERL_VERSION_ARCHNAME_DEFINE} ABI_ENV=\"64\" ${DYNAMIC_LOADER_DEFINE}
			bool=char ${SHARED_OBJECT_DEFINE} HAS_BOOL CMISS_PERL_CALLBACK=${CMISS_PERL_CALLBACK} 
			${REQUIRE_MKSTEMP_DEFINITION} ${REQUIRE_MKSTEMP_DECLARATION})
	
		# These are the files we want to convert to header? files
		SET( PMH_FILES )
		SET( PM_FILES DynaLoader.pm strict.pm Balanced.pm Perl_cmiss.pm )
		FOREACH( PM_FILE ${PM_FILES} )
			# This is where we want the converted files to go
			SET( PMH_FILE ${GENERATED_FILES_DIR}/${PM_FILE}h )
			# This is the location of the files to be converted
			SET( PM_FILE_ABSPATH ${PROJECT_SOURCE_DIR}/source/${PM_FILE} )
			# DynaLoader.pm comes from somewhere in the system
			IF( ${PM_FILE} MATCHES "DynaLoader.pm" )
				SET( PM_FILE_ABSPATH ${PERL_ARCHLIB_DIR}/DynaLoader.pm )
			ENDIF() # ${PM_FILE} MATCHES "DynaLoader.pm"
	
			# Execute the pm2pmh command, is ">" redirection portable?
			# no probably not, going for modifying pm2pmh perl script
			ADD_CUSTOM_COMMAND( OUTPUT ${PMH_FILE}
				COMMAND ${cur_perl_exe} bin2hex.pl ${PM_FILE_ABSPATH} ${PMH_FILE}
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/utilities )
	
			# Trying various ways to make CMake run this custom command before
			# compiling the source code.  Setting the generated header files directly
			# into the library compilation works, but is it the best way? Yes it is actually.
			SET_SOURCE_FILES_PROPERTIES( ${PMH_FILE} PROPERTIES HEADER_FILE_ONLY TRUE
				GENERATED TRUE )
			SET( PMH_FILES ${PMH_FILES} ${PMH_FILE} )
		ENDFOREACH( PM_FILE $PM_FILES )
	
		#IF( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
#			SET_SOURCE_FILES_PROPERTIES( ${SOURCES}
#				PROPERTIES COMPILE_FLAGS "-fPIC" )
		#ENDIF( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
	
		SET( LIBRARY_NAME "perl_interpreter_${INDEX}" )
		ADD_LIBRARY( ${LIBRARY_NAME} SHARED ${PERL_INTERPRETER_SRCS} ${PMH_FILES} )
	
		ADD_DEPENDENCIES( ${LIBRARY_NAME} MAKE_PERL_CMISS_${INDEX} )
	
		TARGET_LINK_LIBRARIES( ${LIBRARY_NAME} ${PERL_LIBRARIES} ${OS_SPECIFIC_LIBRARIES} ${PERL_CMISS_LIB} ${CRYPT_LIBRARIES} )
	
		SET( LIBRARY_OUTPUT_LOCATION ${CMAKE_BINARY_DIR}/lib/${BIN_ARCH_DIR}/${PERL_API_STRING}${DEBUG_TARGET_SUFFIX}${LIBRARY_TYPE_SUFFIX} )
		# We don't want any rpath information stored in the dynamic perl libraries
		# as they do not get recreated where they were built so we set SKIP_BUILD_RPATH TRUE.
		SET_TARGET_PROPERTIES( ${LIBRARY_NAME} PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_LOCATION}
			RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_LOCATION}
			INCLUDE_DIRECTORIES "${PERL_INCLUDE_DIRS};${PROJECT_SOURCE_DIR}/source;${GENERATED_FILES_DIR}"
			COMPILE_DEFINITIONS "${LIBRARY_DEFINITIONS}" )
		IF(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX))
			SET_TARGET_PROPERTIES( ${LIBRARY_NAME} PROPERTIES
				LINK_FLAGS -Wl,-symbolic )
		ENDIF()
	
		# Create shared object header from shared object library using the bin2base64h command
		SET( PERLINTERPRETER_SHARED_LIB
			${PROJECT_BINARY_DIR}/lib/${BIN_ARCH_DIR}/${PERL_API_STRING}${DEBUG_TARGET_SUFFIX}${LIBRARY_TYPE_SUFFIX}/${CMAKE_SHARED_LIBRARY_PREFIX}${LIBRARY_NAME}${DEBUG_TARGET_SUFFIX}${CMAKE_SHARED_LIBRARY_SUFFIX} )
	
		SET( PERLINTERPRETER_SHARED_LIB_HEADER ${PERLINTERPRETER_SHARED_LIB}h )
		ADD_CUSTOM_COMMAND( OUTPUT ${PERLINTERPRETER_SHARED_LIB_HEADER}
			COMMAND ${BIN2BASE64H_EXECUTABLE_NAME} ${PERLINTERPRETER_SHARED_LIB} ${PERLINTERPRETER_SHARED_LIB_HEADER}
			DEPENDS ${BIN2BASE64H_EXECUTABLE_NAME}
			WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
			COMMENT "Creating a shared object header '${PERLINTERPRETER_SHARED_LIB_HEADER}'" )
		SET_SOURCE_FILES_PROPERTIES( ${PERLINTERPRETER_SHARED_LIB_HEADER} PROPERTIES HEADER_FILE_ONLY TRUE
			GENERATED TRUE )
	
		LIST( APPEND PERLINTERPRETER_SHARED_LIB_HEADERS ${PERLINTERPRETER_SHARED_LIB_HEADER} )
		LIST( APPEND SHARED_PERLINTERPRETER_LIBRARY_TARGETS ${LIBRARY_NAME} )
		MATH( EXPR INDEX "${INDEX}+1" )
		SET( STATIC_CHAR_LIBPERL_INTERPRETERS "${STATIC_CHAR_LIBPERL_INTERPRETERS}static char libperl_interpreter${PERL_API_STRING_STANDARD_CHARACTERS}[] =\n#include \"${PERLINTERPRETER_SHARED_LIB_HEADER}\"\n;\n\n" )
		SET( PERL_INTERPRETER_LIBRARY_STRINGS "${PERL_INTERPRETER_LIBRARY_STRINGS}\t{\"${PERL_API_STRING}\", libperl_interpreter${PERL_API_STRING_STANDARD_CHARACTERS} },\n" )
	
	ELSE()
		MESSAGE( WARNING "Failed to find a dynamic perl executable: ${dynamic_perl}." )
	ENDIF()
ENDFOREACH()
# end loop here  ==========================

SET( NO_STATIC_FALLBACK TRUE )
## Start building a static perl interpreter backup
IF( PERL_INTERPRETER_STATIC_LIBPERL )

	SET( cur_perl_exe_name PERL_EXECUTABLE_STATIC )
	UNSET( ${cur_perl_exe_name} CACHE )
	FIND_PROGRAM( ${cur_perl_exe_name}
		NAMES perl
		PATHS ${PERL_INTERPRETER_STATIC_LIBPERL}/bin
		${PERL_INTERPRETER_STATIC_LIBPERL}
		NO_DEFAULT_PATH)
	SET( cur_perl_exe ${${cur_perl_exe_name}} )
	MARK_AS_ADVANCED( ${cur_perl_exe_name} )

	DEFINE_PERL_ARCHLIB_DIR( ${cur_perl_exe} )
	FIND_PERL_LIBRARY( ${cur_perl_exe} TRUE )
	IF( PERL_STATIC_LIBRARY )
		SET( STATIC_PERL_LIBRARIES ${PERL_STATIC_LIBRARY} )
		SET( NO_STATIC_FALLBACK FALSE )
		SET( BACKUP_PERL_INTERPRETER_SRCS source/perl_interpreter.c )
		SET( USE_DYNAMIC_LOADER TRUE )
		SET( SHARED_OBJECT FALSE )
		SET( LIBRARY_TYPE_SUFFIX "-static" )
		
		# Get the perl strings for the current perl
		DEFINE_PERL_API_STRING( ${cur_perl_exe} )
		DEFINE_PERL_C_FLAGS( ${cur_perl_exe} )
		# Separate perl c flags into include directories, link
		# libraries and compiler flags.
		# Sets PERL_INCLUDE_DIRS, PERL_LINK_LIBRARIES and PERL_COMPILER_FLAGS
		SEPARATE_PERL_C_FLAGS( ${PERL_C_FLAGS} ${cur_perl_exe} )

		LIST(FIND PERL_LINK_LIBRARIES "crypt" CRYPT_INDEX)
		SET( CRYPT_LIBRARIES )
		IF( NOT CRYPT_INDEX EQUAL -1 )
			SET( CRYPT_LIBRARIES crypt )
		ENDIF()

		SET( REQUIRE_MKSTEMP_DEFINITION )
		SET( REQUIRE_MKSTEMP_DECLARATION )

		IF(WIN32)
			# Assuming here that perl include dirs is only one directory
			TRY_RUN(RUN_RESULT_VAR COMPILE_RESULT_VAR "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/utilities/mkstemp_check.c" 
				COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT
				COMPILE_DEFINITIONS "/I${PERL_INCLUDE_DIRS}"
				LINK_LIBRARIES ${PERL_STATIC_LIBRARY})
			STRING(FIND "${COMPILE_OUTPUT}" "mkstemp already defined" MKSTEMP_ALREADY_DEFINED_INDEX)
			IF (MKSTEMP_ALREADY_DEFINED_INDEX EQUAL -1)
				SET(REQUIRE_MKSTEMP_DEFINITION REQUIRE_MKSTEMP_DEFINITION)
			ELSE()
				SET( REQUIRE_MKSTEMP_DECLARATION REQUIRE_MKSTEMP_DECLARATION )
			ENDIF()
		ENDIF()

		SET( PERL_VERSION_ARCHNAME_DEFINE PERL_VERSION_ARCHNAME=\"${PERL_API_STRING}\" )
		SET( CMISS_PERL_WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Perl_cmiss/generated_cmake/${BIN_ARCH_DIR}/${PERL_API_STRING}-static )
		SET( CMISS_PERL_CALLBACK cmiss_perl_callback_static )
		SET( PERL_CMISS_LIB
			${CMISS_PERL_WORKING_DIRECTORY}/auto/Perl_cmiss/Perl_cmiss${CMAKE_STATIC_LIBRARY_SUFFIX} )
		IF( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
			SET( PERL_CMISS_LIB_INSTALL
				${CMAKE_INSTALL_PREFIX}/lib/${PERL_API_STRING}/${CMAKE_STATIC_LIBRARY_PREFIX}Perl_cmiss${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX} )
		ELSE( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
			SET( PERL_CMISS_LIB_INSTALL
				${CMAKE_INSTALL_PREFIX}/lib/${PERL_API_STRING}/${CMAKE_STATIC_LIBRARY_PREFIX}Perl_cmiss${CMAKE_STATIC_LIBRARY_SUFFIX} )
		ENDIF( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )

		# We need to install the static library that we create here so that we can link to it later.
		# This is important because we cannot move the Perl_cmiss library once we have linked the static
		# perl interpreter library to it.
		CONFIGURE_FILE( InstallPerlCmissLib.in.cmake InstallPerlCmissLib.cmake )

		SET( PERL_CMISS_TARGET MAKE_PERL_CMISS_STATIC )
		# This version of the perl cmiss library will need to be installed as it is
		# required by the static library we are about to create.
		# Can't use symlinks on Windows so using the copy version for windows and linux alike
		ADD_CUSTOM_TARGET( PERL_CMISS_MAKEFILE_STATIC
			COMMAND ${CMAKE_COMMAND} -E make_directory Perl_cmiss
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/Perl_cmiss/Makefile_cmake.PL ${PROJECT_BINARY_DIR}/Perl_cmiss/Makefile.PL
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/Perl_cmiss/perl_cmiss.h ${PROJECT_BINARY_DIR}/Perl_cmiss/perl_cmiss.h
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/Perl_cmiss/Perl_cmiss.pm ${PROJECT_BINARY_DIR}/Perl_cmiss/Perl_cmiss.pm
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/Perl_cmiss/Perl_cmiss.xs ${PROJECT_BINARY_DIR}/Perl_cmiss/Perl_cmiss.xs
			COMMAND ${CMAKE_COMMAND} -E chdir ${PROJECT_BINARY_DIR}/Perl_cmiss ${cur_perl_exe} Makefile.PL ${CMISS_PERL_CALLBACK} ${CMISS_PERL_WORKING_DIRECTORY} ${CMAKE_BUILD_TYPE}
			COMMENT "Running PERL_CMISS_MAKEFILE CMISS_PERL_CALLBACK=${CMISS_PERL_CALLBACK} WORKING_DIR=${CMISS_PERL_WORKING_DIRECTORY} ${CMAKE_BUILD_TYPE}" )
		ADD_CUSTOM_TARGET( ${PERL_CMISS_TARGET}
			COMMAND ${MAKE_PROGRAM} static
			COMMAND ${CMAKE_COMMAND} -P ${PROJECT_BINARY_DIR}/InstallPerlCmissLib.cmake
			WORKING_DIRECTORY ${CMISS_PERL_WORKING_DIRECTORY}
			COMMENT "Making ${PERL_API_STRING}${LIBRARY_TYPE_SUFFIX}" )
		ADD_DEPENDENCIES( ${PERL_CMISS_TARGET} PERL_CMISS_MAKEFILE_STATIC )

		SET( GENERATED_FILES_DIR ${PROJECT_BINARY_DIR}/generated/${BIN_ARCH_DIR}/${PERL_API_STRING}${DEBUG_TARGET_SUFFIX}${LIBRARY_TYPE_SUFFIX} )
		FILE( MAKE_DIRECTORY "${GENERATED_FILES_DIR}" )

		IF( USE_DYNAMIC_LOADER )
			SET( DYNAMIC_LOADER_DEFINE USE_DYNAMIC_LOADER) #  INCLUDE_DYNALOADERPMH
		ELSE()
			SET( DYNAMIC_LOADER_DEFINE )
		ENDIF()

		IF( SHARED_OBJECT )
			SET( SHARED_OBJECT_DEFINE SHARED_OBJECT )
		ELSE()
			SET( SHARED_OBJECT_DEFINE )
		ENDIF()

		SET( LIBRARY_DEFINITIONS ${PERL_VERSION_ARCHNAME_DEFINE} ABI_ENV=\"64\" ${DYNAMIC_LOADER_DEFINE}
			bool=char ${SHARED_OBJECT_DEFINE} HAS_BOOL CMISS_PERL_CALLBACK=${CMISS_PERL_CALLBACK} 
			${REQUIRE_MKSTEMP_DEFINITION} ${REQUIRE_MKSTEMP_DECLARATION} )

		# These are the files we want to convert to header? files
		SET( PMH_FILES )
		SET( PM_FILES strict.pm Balanced.pm Perl_cmiss.pm ) # DynaLoader.pm
		FOREACH( PM_FILE ${PM_FILES} )
			# This is where we want the converted files to go
			SET( PMH_FILE ${GENERATED_FILES_DIR}/${PM_FILE}h )
			# This is the location of the files to be converted
			SET( PM_FILE_ABSPATH ${PROJECT_SOURCE_DIR}/source/${PM_FILE} )
			# DynaLoader.pm comes from somewhere in the system
			IF( ${PM_FILE} MATCHES "DynaLoader.pm" )
				SET( PM_FILE_ABSPATH ${PERL_ARCHLIB_DIR}/DynaLoader.pm )
			ENDIF() # ${PM_FILE} MATCHES "DynaLoader.pm"

			# Execute the pm2pmh command, is ">" redirection portable?
			# no probably not, going for modifying pm2pmh perl script
			ADD_CUSTOM_COMMAND( OUTPUT ${PMH_FILE}
				COMMAND ${cur_perl_exe} bin2hex.pl ${PM_FILE_ABSPATH} ${PMH_FILE}
				WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/utilities )

			# Trying various ways to make CMake run this custom command before
			# compiling the source code.  Setting the generated header files directly
			# into the library compilation works, but is it the best way? Yes it is actually.
			SET_SOURCE_FILES_PROPERTIES( ${PMH_FILE} PROPERTIES HEADER_FILE_ONLY TRUE
				GENERATED TRUE )
			SET( PMH_FILES ${PMH_FILES} ${PMH_FILE} )
		ENDFOREACH( PM_FILE $PM_FILES )

#		IF( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
#			SET_SOURCE_FILES_PROPERTIES( ${SOURCES}
#				PROPERTIES COMPILE_FLAGS "-fPIC" )
#		ENDIF( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
	ELSE()
		MESSAGE( WARNING "Failed to find a static perl library: ${PERL_INTERPRETER_STATIC_LIBPERL}\nNo backup perl will be available in this perl interpreter." )
	ENDIF()
ENDIF()

CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/templates/static_version.h.template"
	"${GENERATED_FILES_DIR}/static_version.h" )
CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/templates/dynamic_versions.h.template"
	"${GENERATED_FILES_DIR}/dynamic_versions.h" )

# Set the sources for the static library that contains all the shared object perl interpreters
SET( STATIC_PERL_INTERPRETER_SRCS
	${BACKUP_PERL_INTERPRETER_SRCS}
	source/perl_interpreter_dynamic.c
	source/base64.c )

SET( CMISS_PERL_INTERPRETER cmiss_perl_interpreter )
ADD_LIBRARY( ${CMISS_PERL_INTERPRETER} STATIC ${STATIC_PERL_INTERPRETER_SRCS}
	${PMH_FILES} ${PERLINTERPRETER_SHARED_LIB_HEADERS} )

#SET_TARGET_PROPERTIES( ${CMISS_PERL_INTERPRETER} PROPERTIES
#	INCLUDE_DIRECTORIES "${PERL_INCLUDE_DIRS};${PROJECT_SOURCE_DIR}/source;${GENERATED_FILES_DIR}"
#	COMPILE_DEFINITIONS "${LIBRARY_DEFINITIONS}" )
target_include_directories(${CMISS_PERL_INTERPRETER}
    PUBLIC
        ${PERL_INCLUDE_DIRS}
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>
        $<BUILD_INTERFACE:${GENERATED_FILES_DIR}>
        $<INSTALL_INTERFACE:include/cmiss_perl_interpreter>
    )
TARGET_LINK_LIBRARIES( ${CMISS_PERL_INTERPRETER} ${PERL_CMISS_LIB_INSTALL} ${STATIC_PERL_LIBRARIES} ${STATIC_PERL_DEPENDENT_LIBRARIES} ${CRYPT_LIBRARIES})
target_compile_definitions(${CMISS_PERL_INTERPRETER} PRIVATE ${LIBRARY_DEFINITIONS})
ADD_DEPENDENCIES( ${CMISS_PERL_INTERPRETER} ${PERL_CMISS_TARGET} ${SHARED_PERLINTERPRETER_LIBRARY_TARGETS} )

SET( LIBRARY_OUTPUT_LOCATION ${CMAKE_BINARY_DIR}/lib/${BIN_ARCH_DIR} )
SET_TARGET_PROPERTIES( ${CMISS_PERL_INTERPRETER} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_LOCATION} )

# Install stuff
INSTALL( TARGETS ${CMISS_PERL_INTERPRETER} EXPORT ${CMISS_PERL_INTERPRETER}-targets
        DESTINATION lib )
IF( WIN32 )
	INSTALL( FILES ${LIBRARY_OUTPUT_LOCATION}/${CMISS_PERL_INTERPRETER}${CMAKE_DEBUG_POSTFIX}.pdb
                DESTINATION lib
		CONFIGURATIONS Debug )
ENDIF( WIN32 )
INSTALL( FILES ${CMAKE_SOURCE_DIR}/source/perl_interpreter.h DESTINATION include/cmiss_perl_interpreter )

if(WIN32)
  set(_PACKAGE_CONFIG_DIR cmake)
elseif(UNIX)
  set(_PACKAGE_CONFIG_DIR share/cmake/${CMISS_PERL_INTERPRETER})
endif()
INSTALL(EXPORT ${CMISS_PERL_INTERPRETER}-targets
        DESTINATION ${_PACKAGE_CONFIG_DIR})

STRING( TOUPPER ${CMISS_PERL_INTERPRETER} UPPERCASE_LIBRARY_NAME )
SET( CONFIG_FILE_CONTENTS
	"\nIF( NOT DEFINED _${UPPERCASE_LIBRARY_NAME}_CONFIG_CMAKE )"
	"\nSET( _${UPPERCASE_LIBRARY_NAME}_CONFIG_CMAKE TRUE )"
	"\nGET_FILENAME_COMPONENT( SELF_DIR \"\${CMAKE_CURRENT_LIST_FILE}\" PATH )"
	"\nINCLUDE( \${SELF_DIR}/${CMISS_PERL_INTERPRETER}-targets.cmake )"
        "\nGET_FILENAME_COMPONENT( ${UPPERCASE_LIBRARY_NAME}_INCLUDE_DIRS \"\${SELF_DIR}/../../../include/cmiss_perl_interpreter\" ABSOLUTE )"
	"\nSET( ${UPPERCASE_LIBRARY_NAME}_LIBRARIES ${CMISS_PERL_INTERPRETER} )"
	"\nSET( ${UPPERCASE_LIBRARY_NAME}_FOUND TRUE )"
	"\nENDIF( NOT DEFINED _${UPPERCASE_LIBRARY_NAME}_CONFIG_CMAKE )"
	"\n\n" )

SET( CONFIG_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/${CMISS_PERL_INTERPRETER}-config.cmake )
FILE( WRITE ${CONFIG_FILE_NAME} ${CONFIG_FILE_CONTENTS} )
INSTALL(FILES ${CONFIG_FILE_NAME} DESTINATION ${_PACKAGE_CONFIG_DIR})
