
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
 # ***** END LICENSE BLOCK ***** */

# This perl interpreter build is only designed to work with perl version
# greater than or equal to 5.10.0

CMAKE_MINIMUM_REQUIRED( VERSION 2.6.2 )

# The three paths that we are interested in
FILE( TO_CMAKE_PATH "${PERL_INTERPRETER_INSTALL_PREFIX}" PERL_INTERPRETER_INSTALL_PREFIX )
FILE( TO_CMAKE_PATH "${PERL_INTERPRETER_MODULE_PATH}" PERL_INTERPRETER_MODULE_PATH )
FILE( TO_CMAKE_PATH "${PERL_INTERPRETER_ROOT}" PERL_INTERPRETER_ROOT )

# We need a path to our own CMake modules.  This has the MacroDefinitions.cmake file
# that contains common macro defintions that are shared between projects.
SET( FOUND_MACRODEFINITIONS_MODULE NO )
SET( PERL_INTERPRETER_DEFAULT_MODULES_PATH "utilities/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules" )
IF( EXISTS "${PERL_INTERPRETER_ROOT}" AND NOT PERL_INTERPRETER_INSTALL_PREFIX )
	SET( PERL_INTERPRETER_INSTALL_PREFIX "${PERL_INTERPRETER_ROOT}/install" )
ENDIF()
IF( EXISTS "${PERL_INTERPRETER_ROOT}" AND NOT PERL_INTERPRETER_MODULE_PATH )
	SET( PERL_INTERPRETER_MODULE_PATH "${PERL_INTERPRETER_ROOT}/${PERL_INTERPRETER_DEFAULT_MODULES_PATH}" )
ENDIF()

IF( PERL_INTERPRETER_INSTALL_PREFIX OR PERL_INTERPRETER_MODULE_PATH )
	IF( NOT EXISTS "${PERL_INTERPRETER_MODULE_PATH}/MacroDefinitions.cmake" )
		SET( PERL_INTERPRETER_MODULE_PATH "${PERL_INTERPRETER_INSTALL_PREFIX}/${PERL_INTERPRETER_DEFAULT_MODULES_PATH}" )
	ENDIF()
	IF( EXISTS "${PERL_INTERPRETER_MODULE_PATH}/MacroDefinitions.cmake" )
		SET( FOUND_MACRODEFINITIONS_MODULE YES )
		IF( NOT PERL_INTERPRETER_INSTALL_PREFIX )
			# Check the install path on all operating systems
			SET( PERL_INTERPRETER_INSTALL_PREFIX "/usr/local" )
		ENDIF()
	ENDIF()
ENDIF()

IF( NOT FOUND_MACRODEFINITIONS_MODULE )
	SET( PERL_INTERPRETER_MODULE_PATH "" CACHE PATH "PERL_INTERPRETER_MODULE_PATH path." )
	SET( PERL_INTERPRETER_INSTALL_PREFIX "" CACHE PATH "PERL_INTERPRETER_INSTALL_PREFIX path." )
	SET( PERL_INTERPRETER_ROOT "" CACHE PATH "PERL_INTERPRETER_ROOT path." )
	MESSAGE( FATAL_ERROR "Dude! I have not been able to find a path to the CMISS CMake modules.  Try setting PERL_INTERPRETER_INSTALL_PREFIX and/or PERL_INTERPRETER_MODULE_PATH." )
ELSEIF( NOT EXISTS "${PERL_INTERPRETER_ROOT}" AND NOT FOUND_MACRODEFINITIONS_MODULE )
	SET( PERL_INTERPRETER_ROOT "" CACHE PATH "PERL_INTERPRETER_ROOT path." )
	MESSAGE( FATAL_ERROR "What! I need PERL_INTERPRETER_ROOT set to somewhere that exists so I know where to dump all this stuff." )
ENDIF()

IF( NOT IS_ABSOLUTE "${PERL_INTERPRETER_INSTALL_PREFIX}" )
	GET_FILENAME_COMPONENT(PERL_INTERPRETER_INSTALL_PREFIX "${PERL_INTERPRETER_INSTALL_PREFIX}" REALPATH)
ENDIF()

IF( NOT IS_ABSOLUTE "${PERL_INTERPRETER_ROOT}" )
	GET_FILENAME_COMPONENT(PERL_INTERPRETER_ROOT "${PERL_INTERPRETER_ROOT}" REALPATH)
ENDIF()

IF( NOT IS_ABSOLUTE "${PERL_INTERPRETER_MODULE_PATH}" )
	GET_FILENAME_COMPONENT(PERL_INTERPRETER_MODULE_PATH "${PERL_INTERPRETER_MODULE_PATH}" REALPATH)
ENDIF()

SET( CMAKE_MODULE_PATH "${PERL_INTERPRETER_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}" )
INCLUDE( PerlInterpreterMacros )
INCLUDE( MacroDefinitions )

# Set the project name different from the target name otherwise confusion
# insues with IDEs, thus the lack of "_" separator
PROJECT( "PerlInterpreter" )

SET( CMAKE_OSX_ARCHITECTURES i386 )
SET( CMAKE_DEBUG_POSTFIX "d" )
SET( CMAKE_MFC_FLAG 0 )

IF( WIN32 )
	   SET( MAKE_PROGRAM nmake )
ELSE( WIN32 )
	   SET( MAKE_PROGRAM make )
ENDIF( WIN32 )

# Define the system architecture
DEFINE_ARCHITECTURE_DIR( )
SET( BIN_ARCH_DIR ${ARCHITECTURE_DIR} )
#SET( LIB_ARCH_DIR ${CMAKE_SYSTEM_PROCESSOR}${ABI}-${OPERATING_SYSTEM} )

SET( PERL_INTERPRETER_INSTALL_PREFIX_TEMP ${PERL_INTERPRETER_INSTALL_PREFIX} )
OPTION_WITH_DEFAULT( USE_ARCHITECTURE_DIR "Set true if cross-compiling or colocating libraries from different systems." OFF )
# Set build type and install location
IF( USE_ARCHITECTURE_DIR )
	STRING( FIND "${PERL_INTERPRETER_INSTALL_PREFIX_TEMP}"
		"/${ARCHITECTURE_DIR}" CONTAINS )
	IF( ${CONTAINS} LESS 0 )
		SET( PERL_INTERPRETER_INSTALL_PREFIX_TEMP
			"${PERL_INTERPRETER_INSTALL_PREFIX_TEMP}/${ARCHITECTURE_DIR}" )
	ELSE( ${CONTAINS} LESS 0 )
		SET( PERL_INTERPRETER_INSTALL_PREFIX
			"${PERL_INTERPRETER_INSTALL_PREFIX_TEMP}" )
	ENDIF( ${CONTAINS} LESS 0 )
ELSE( USE_ARCHITECTURE_DIR )
	STRING( REGEX REPLACE "/${ARCHITECTURE_DIR}" ""
		PERL_INTERPRETER_INSTALL_PREFIX_TEMP ${PERL_INTERPRETER_INSTALL_PREFIX_TEMP} )
ENDIF( USE_ARCHITECTURE_DIR )
UNSET( PERL_INTERPRETER_INSTALL_PREFIX CACHE )
CACHE_VAR_WITH_DEFAULT( PERL_INTERPRETER_INSTALL_PREFIX "${PERL_INTERPRETER_INSTALL_PREFIX_TEMP}"
	PATH "Where do you want your perl interpreter libraries/executable installed?" )
SET( CMAKE_INSTALL_PREFIX ${PERL_INTERPRETER_INSTALL_PREFIX} CACHE INTERNAL "Internalise CMAKE_INSTALL_PREFIX, manipulate via CMGUI_INSTALL_PREFIX" FORCE )

CACHE_VAR_WITH_DEFAULT( PERL_INTERPRETER_BUILD_TYPE "Release"
	STRING "What configuration do you want to build: Debug, Release, MinSizeRel, RelWithDebugInfo" )
SET( CMAKE_BUILD_TYPE ${PERL_INTERPRETER_BUILD_TYPE} CACHE INTERNAL "Internalise CMAKE_BUILD_TYPE, manipulate via CMGUI_BUILD_TYPE" FORCE )

IF( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
	SET( DEBUG_TARGET_SUFFIX "d" )
ENDIF( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )

# Actually start the perl interpreter build
# Set the ABI for the build
SET( ABI_ENV_DEFINE ABI_ENV=\"${CMAKE_SYSTEM_PROCESSOR}\" )#${CMAKE_SYSTEM_PROCESSOR} )

# Find the system perl and the libraries for perl
FIND_PACKAGE( Perl REQUIRED )
#FIND_PACKAGE( PerlLibs REQUIRED )

# Find the perl listed under PERL_INTERPRETER_PERLS
# ... How to do this, set the variable PERL_INTERPRETER_PERLS to a list of
# perl install directories.  For instance the prefix used to build your own
# perl.  Each directory should be separated by a semi-colon. e.g.
#SET( PERL_INTERPRETER_PERLS "$HOME/perls/5.14;$HOME/perls/5.16" )

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

# Build bin2base64str executable
SET( BIN2BASE64H_EXECUTABLE_NAME bin2base64str )
SET( CMAKE_OSX_ARCHITECTURES x86_64 )
ADD_EXECUTABLE( ${BIN2BASE64H_EXECUTABLE_NAME} utilities/bin2base64str.c source/base64.c )
SET_TARGET_PROPERTIES( ${BIN2BASE64H_EXECUTABLE_NAME} PROPERTIES 
	INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/source )
SET( CMAKE_OSX_ARCHITECTURES i386 )

GET_FILENAME_COMPONENT( PERL_EXECUTABLE_PATH ${PERL_EXECUTABLE} PATH )
LIST( APPEND PERL_INTERPRETER_PERLS ${PERL_EXECUTABLE_PATH} )

SET( SHARED_PERLINTERPRETER_LIBRARY_TARGETS )
SET( INDEX 1 )
FOREACH( perl_interpreter_perl ${PERL_INTERPRETER_PERLS})
	SET( cur_perl_exe_name PERL_EXECUTABLE_${INDEX} )
	UNSET( ${cur_perl_exe_name} CACHE )
	FIND_PROGRAM( ${cur_perl_exe_name}
		NAMES perl
		PATHS ${perl_interpreter_perl}/bin
		${perl_interpreter_perl}
		NO_DEFAULT_PATH)
	SET( cur_perl_exe ${${cur_perl_exe_name}} )

	# Get the perl strings for the current perl
	DEFINE_PERL_ARCHLIB_DIR(  ${cur_perl_exe} )
	DEFINE_PERL_API_STRING( ${cur_perl_exe} )
	DEFINE_PERL_C_FLAGS( ${cur_perl_exe} )
	# Separate perl c flags into include directories, link
	# libraries and compiler flags.
	# Sets PERL_INCLUDE_DIRS, PERL_LINK_LIBRARIES and PERL_COMPILER_FLAGS
	SEPARATE_PERL_C_FLAGS( ${PERL_C_FLAGS} ${cur_perl_exe} )

	FIND_PERL_LIBRARY( ${cur_perl_exe} TRUE )
	SET( PERL_LIBRARIES ${PERL_LIBRARY} )
	# Set the perl API string with valid characters for use in c functions
	STRING( REGEX REPLACE "(\\.|-)" "_" PERL_API_STRING_STANDARD_CHARACTERS ${PERL_API_STRING} )
	SET( PERL_VERSION_ARCHNAME_DEFINE PERL_VERSION_ARCHNAME=\"${PERL_API_STRING}\" )

	SET( USE_DYNAMIC_LOADER TRUE )
	SET( SHARED_OBJECT TRUE ) # May not be a good idea for Windows
	SET( LIBRARY_TYPE_SUFFIX "-shared" )

	SET( CMISS_PERL_CALLBACK cmiss_perl_callback_${PERL_API_STRING_STANDARD_CHARACTERS} )
	SET( CMISS_PERL_WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Perl_cmiss/generated_cmake/${BIN_ARCH_DIR}/${PERL_API_STRING} )
	# Set the Perl_cmiss library /*install*/ location on a build type basis
	IF( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
		SET( PERL_CMISS_LIB
			${CMISS_PERL_WORKING_DIRECTORY}/auto/Perl_cmiss/Perl_cmiss${CMAKE_STATIC_LIBRARY_SUFFIX} )
	ELSE( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
		SET( PERL_CMISS_LIB
			${CMISS_PERL_WORKING_DIRECTORY}/auto/Perl_cmiss/Perl_cmiss${CMAKE_STATIC_LIBRARY_SUFFIX} )
	ENDIF( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )

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
#CC="C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC/bin/amd64/nmake.exe" 
	SET( GENERATED_FILES_DIR ${PROJECT_BINARY_DIR}/generated/${BIN_ARCH_DIR}/${PERL_API_STRING}${DEBUG_TARGET_SUFFIX}${LIBRARY_TYPE_SUFFIX} )
	FILE( MAKE_DIRECTORY "${GENERATED_FILES_DIR}" )

	IF( USE_DYNAMIC_LOADER )
		SET( DYNAMIC_LOADER_DEFINE INCLUDE_DYNALOADERPMH USE_DYNAMIC_LOADER)
	ENDIF()

	IF( SHARED_OBJECT )
		SET( SHARED_OBJECT_DEFINE SHARED_OBJECT )
	ENDIF()

	SET( LIBRARY_DEFINITIONS ${PERL_VERSION_ARCHNAME_DEFINE} ABI_ENV=\"64\"
		bool=char ${SHARED_OBJECT_DEFINE} HAS_BOOL CMISS_PERL_CALLBACK=${CMISS_PERL_CALLBACK} )

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

	# replace this with setting target properties so they can all be different
	#ADD_DEFINITIONS( ${LIBRARY_DEFINITIONS} ${PERL_COMPILER_FLAGS} )
	#INCLUDE_DIRECTORIES( ${GENERATED_FILES_DIR}
	#	 ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/source ${PERL_INCLUDE_DIRS})

	IF( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )
		SET_SOURCE_FILES_PROPERTIES( ${SOURCES}
			PROPERTIES COMPILE_FLAGS "-fPIC" )
	ENDIF( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" )

	SET( LIBRARY_NAME "perl_interpreter_${INDEX}" )
	ADD_LIBRARY( ${LIBRARY_NAME} SHARED ${PERL_INTERPRETER_SRCS} ${PMH_FILES} )

	SET_TARGET_PROPERTIES( ${LIBRARY_NAME} PROPERTIES INCLUDE_DIRECTORIES "${PERL_INCLUDE_DIRS};${PROJECT_SOURCE_DIR}/source;${GENERATED_FILES_DIR}" ) 
	SET_TARGET_PROPERTIES( ${LIBRARY_NAME} PROPERTIES COMPILE_DEFINITIONS "${LIBRARY_DEFINITIONS}" )
	ADD_DEPENDENCIES( ${LIBRARY_NAME} MAKE_PERL_CMISS_${INDEX} )

	TARGET_LINK_LIBRARIES( ${LIBRARY_NAME} ${PERL_LIBRARIES} ${PERL_CMISS_LIB} ${OS_SPECIFIC_LIBRARIES} )

	SET( LIBRARY_OUTPUT_LOCATION ${CMAKE_BINARY_DIR}/lib/${BIN_ARCH_DIR}/${PERL_API_STRING}${DEBUG_TARGET_SUFFIX}${LIBRARY_TYPE_SUFFIX} )
	SET_TARGET_PROPERTIES( ${LIBRARY_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_LOCATION} )
	SET_TARGET_PROPERTIES( ${LIBRARY_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_LOCATION} )

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

ENDFOREACH()
# end loop here  ==========================

# Set the library type suffix for this build, maybe this should be static?
SET( LIBRARY_TYPE_SUFFIX "-dynamic" )

# Perl no longer comes with a static library
SET( NO_STATIC_FALLBACK TRUE )
CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/templates/static_version.h.template"
	"${GENERATED_FILES_DIR}/static_version.h" )
CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/templates/dynamic_versions.h.template"
	"${GENERATED_FILES_DIR}/dynamic_versions.h" )


SET( CMISS_PERL_CALLBACK cmiss_perl_callback_${PERL_API_STRING_STANDARD_CHARACTERS} )
SET( CMISS_PERL_WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/Perl_cmiss/generated_cmake/${BIN_ARCH_DIR}/${PERL_API_STRING}-system )
# Set the Perl_cmiss library /*install*/ location on a build type basis
IF( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
	SET( PERL_CMISS_LIB
		${CMAKE_INSTALL_PREFIX}/lib/${PERL_API_STRING}/${CMAKE_STATIC_LIBRARY_PREFIX}Perl_cmiss${CMAKE_STATIC_LIBRARY_SUFFIX} )
#		${CMAKE_INSTALL_PREFIX}/lib/${PERL_API_STRING}/${CMAKE_STATIC_LIBRARY_PREFIX}Perl_cmiss${CMAKE_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX} )
ELSE( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
	SET( PERL_CMISS_LIB
		${CMAKE_INSTALL_PREFIX}/lib/${PERL_API_STRING}/${CMAKE_STATIC_LIBRARY_PREFIX}Perl_cmiss${CMAKE_STATIC_LIBRARY_SUFFIX} )
ENDIF( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )

# We need to install the static library that we create here so that we can link to it later.
# This is important because we cannot move the Perl_cmiss library once we have linked the static
# perl interpreter library to it.  But hold on maybe the static perl interpreter doesn't link to this
# library but the shared object one does, so it won't need to be installed to be used as it is
# already included in the shared object perl interpreter!
CONFIGURE_FILE( InstallPerlCmissLib.in.cmake InstallPerlCmissLib.cmake )
# using the copy version for windows and linux alike
ADD_CUSTOM_TARGET( PERL_CMISS_MAKEFILE_SYSTEM
	COMMAND ${CMAKE_COMMAND} -E make_directory Perl_cmiss
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/Perl_cmiss/Makefile_cmake.PL ${PROJECT_BINARY_DIR}/Perl_cmiss/Makefile.PL
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/Perl_cmiss/perl_cmiss.h ${PROJECT_BINARY_DIR}/Perl_cmiss/perl_cmiss.h
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/Perl_cmiss/Perl_cmiss.pm ${PROJECT_BINARY_DIR}/Perl_cmiss/Perl_cmiss.pm
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/Perl_cmiss/Perl_cmiss.xs ${PROJECT_BINARY_DIR}/Perl_cmiss/Perl_cmiss.xs
	COMMAND ${CMAKE_COMMAND} -E chdir ${PROJECT_BINARY_DIR}/Perl_cmiss ${PERL_EXECUTABLE} Makefile.PL ${CMISS_PERL_CALLBACK} ${CMISS_PERL_WORKING_DIRECTORY} ${CMAKE_BUILD_TYPE}
	COMMENT "Running PERL_CMISS_MAKEFILE CMISS_PERL_CALLBACK=${CMISS_PERL_CALLBACK} WORKING_DIR=${CMISS_PERL_WORKING_DIRECTORY} ${CMAKE_BUILD_TYPE}" )
ADD_CUSTOM_TARGET( MAKE_PERL_CMISS_SYSTEM
	COMMAND ${MAKE_PROGRAM} static
	COMMAND ${CMAKE_COMMAND} -P ${PROJECT_BINARY_DIR}/InstallPerlCmissLib.cmake
	WORKING_DIRECTORY ${CMISS_PERL_WORKING_DIRECTORY}
	COMMENT "Making and installing ${PERL_API_STRING}${LIBRARY_TYPE_SUFFIX}" )
ADD_DEPENDENCIES( MAKE_PERL_CMISS_SYSTEM PERL_CMISS_MAKEFILE_SYSTEM )

# These are the files we want to convert to header? files
SET( PMH_FILES )
SET( PM_FILES strict.pm Balanced.pm Perl_cmiss.pm )
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
		COMMAND ${PERL_EXECUTABLE} bin2hex.pl ${PM_FILE_ABSPATH} ${PMH_FILE}
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/utilities )

	# Trying various ways to make CMake run this custom command before
	# compiling the source code.  Setting the generated header files directly
	# into the library compilation works, but is it the best way?
	SET_SOURCE_FILES_PROPERTIES( ${PMH_FILE} PROPERTIES HEADER_FILE_ONLY TRUE
		GENERATED TRUE )
	SET( PMH_FILES ${PMH_FILES} ${PMH_FILE} )
ENDFOREACH( PM_FILE $PM_FILES )

#INCLUDE_DIRECTORIES( ${GENERATED_FILES_DIR} )

# Set the sources for the static library that contains all the shared object perl interpreters
SET( SHARED_OBJECT_PERL_INTERPRETER_SRCS
	source/perl_interpreter_dynamic.c
	source/base64.c )

SET( CMISS_PERL_INTERPRETER cmiss_perl_interpreter )
ADD_LIBRARY( ${CMISS_PERL_INTERPRETER} STATIC ${SHARED_OBJECT_PERL_INTERPRETER_SRCS}
	${PMH_FILES} ${PERLINTERPRETER_SHARED_LIB_HEADERS} )

IF( USE_DYNAMIC_LOADER )
	SET( DYNAMIC_LOADER_DEFINE INCLUDE_DYNALOADERPMH USE_DYNAMIC_LOADER)
ENDIF()

IF( SHARED_OBJECT )
	SET( SHARED_OBJECT_DEFINE SHARED_OBJECT )
ENDIF()

SET( LIBRARY_DEFINITIONS ${PERL_VERSION_ARCHNAME_DEFINE} ABI_ENV=\"64\" ${DYNAMIC_LOADER_DEFINE}
	bool=char ${SHARED_OBJECT_DEFINE} HAS_BOOL CMISS_PERL_CALLBACK=${CMISS_PERL_CALLBACK} )

SET_TARGET_PROPERTIES( ${CMISS_PERL_INTERPRETER} PROPERTIES INCLUDE_DIRECTORIES "${PERL_INCLUDE_DIRS};${PROJECT_SOURCE_DIR}/source;${GENERATED_FILES_DIR}" ) 
SET_TARGET_PROPERTIES( ${CMISS_PERL_INTERPRETER} PROPERTIES
	COMPILE_DEFINITIONS "${LIBRARY_DEFINITIONS}" )
TARGET_LINK_LIBRARIES( ${CMISS_PERL_INTERPRETER} ${PERL_CMISS_LIB} )

ADD_DEPENDENCIES( ${CMISS_PERL_INTERPRETER} ${SHARED_PERLINTERPRETER_LIBRARY_TARGETS} MAKE_PERL_CMISS_SYSTEM )

SET( LIBRARY_OUTPUT_LOCATION ${CMAKE_BINARY_DIR}/lib/${BIN_ARCH_DIR}/${PERL_API_STRING}${DEBUG_TARGET_SUFFIX}${LIBRARY_TYPE_SUFFIX} )
SET_TARGET_PROPERTIES( ${CMISS_PERL_INTERPRETER} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_LOCATION} )


# Install stuff
INSTALL( TARGETS ${CMISS_PERL_INTERPRETER} EXPORT ${CMISS_PERL_INTERPRETER}-targets
	DESTINATION ${CMAKE_INSTALL_PREFIX}/lib )
IF( WIN32 )
	INSTALL( FILES ${LIBRARY_OUTPUT_LOCATION}/${CMISS_PERL_INTERPRETER}${CMAKE_DEBUG_POSTFIX}.pdb
		DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
		CONFIGURATIONS Debug )
ENDIF( WIN32 )
INSTALL( FILES ${CMAKE_SOURCE_DIR}/source/perl_interpreter.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/cmiss_perl_interpreter )
INSTALL( EXPORT ${CMISS_PERL_INTERPRETER}-targets
	DESTINATION lib/cmake )

STRING( TOUPPER ${CMISS_PERL_INTERPRETER} UPPERCASE_LIBRARY_NAME )
SET( CONFIG_FILE_CONTENTS
	"\nIF( NOT DEFINED _${UPPERCASE_LIBRARY_NAME}_CONFIG_CMAKE )"
	"\nSET( _${UPPERCASE_LIBRARY_NAME}_CONFIG_CMAKE TRUE )"
	"\nGET_FILENAME_COMPONENT( SELF_DIR \"\${CMAKE_CURRENT_LIST_FILE}\" PATH )"
	"\nINCLUDE( \${SELF_DIR}/${CMISS_PERL_INTERPRETER}-targets.cmake )"
	"\nGET_FILENAME_COMPONENT( ${UPPERCASE_LIBRARY_NAME}_INCLUDE_DIRS \"\${SELF_DIR}/../../include/cmiss_perl_interpreter\" ABSOLUTE )"
	"\nSET( ${UPPERCASE_LIBRARY_NAME}_LIBRARIES ${CMISS_PERL_INTERPRETER} )"
	"\nSET( ${UPPERCASE_LIBRARY_NAME}_FOUND TRUE )"
	"\nENDIF( NOT DEFINED _${UPPERCASE_LIBRARY_NAME}_CONFIG_CMAKE )"
	"\n\n" )

SET( CONFIG_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/${CMISS_PERL_INTERPRETER}-config.cmake )
FILE( WRITE ${CONFIG_FILE_NAME} ${CONFIG_FILE_CONTENTS} )
INSTALL( FILES ${CONFIG_FILE_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake )
