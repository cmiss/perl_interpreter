/*******************************************************************************
FILE : perl_interpreter_dynamic.c

LAST MODIFIED : 25 January 2005

DESCRIPTION :
Puts a layer between cmiss and the perl interpreter which allows many different
perl interpreters to be included in the executable and the appropriate one
selected at runtime according to the perl found in the users path.
==============================================================================*/
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is cmgui.
 *
 * The Initial Developer of the Original Code is
 * Auckland Uniservices Ltd, Auckland, New Zealand.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdarg.h>
#include "static_version.h"       /* for NO_STATIC_FALLBACK */
#include "perl_interpreter.h"

struct Interpreter
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
The dynamic interpreter wrapper of an actual interpreter, both have the same
name so as to maintain an identical functional interface.
==============================================================================*/
{
	int use_dynamic_interpreter;
	Interpreter_display_message_function *display_message_function;

	void(*create_interpreter_handle)(int argc, char **argv, const char *initial_comfile,
		struct Interpreter **interpreter, int *status);
	void (*interpreter_destroy_string_handle)(struct Interpreter *interpreter, char *string);
	void (*destroy_interpreter_handle)(struct Interpreter *interpreter, int *status);
	void (*redirect_interpreter_output_handle)(struct Interpreter *interpreter, int *status);
	void (*interpreter_set_display_message_function_handle)
		(struct Interpreter *interpreter, Interpreter_display_message_function *function, int *status);
	void (*interpret_command_handle)(struct Interpreter *interpreter, 
		char *command_string, void *user_data, int *quit,
		execute_command_function_type execute_command_function, int *status);
	void (*interpreter_evaluate_integer_handle)(struct Interpreter *interpreter,
		char *expression, int *result, int *status);
	void (*interpreter_set_integer_handle)(struct Interpreter *interpreter,
		char *variable_name, int *value, int *status);
	void (*interpreter_evaluate_double_handle)(struct Interpreter *interpreter,
		char *expression, double *result, int *status);
	void (*interpreter_set_double_handle)(struct Interpreter *interpreter,
		char *variable_name, double *value, int *status);
	void (*interpreter_evaluate_string_handle)(struct Interpreter *interpreter,
		char *expression, char **result, int *status);
	void (*interpreter_set_string_handle)(struct Interpreter *interpreter,
		char *variable_name, char *value, int *status);
	void (*interpreter_set_pointer_handle)(struct Interpreter *interpreter,
		char *variable_name, char *class_name, void *value, int *status);

	struct Interpreter *real_interpreter;
}; /* struct Interpreter */

static int interpreter_display_message(enum Message_type message_type,
	char *format, ... )
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
The default interpreter_display_message_function.
==============================================================================*/
{
	int return_code;
	va_list ap;

	va_start(ap,format);
	return_code=vfprintf(stderr,format,ap);
	va_end(ap);
	fprintf(stderr,"\n");

	return (return_code);
} /* interpreter_display_message */

/* Used by dynamic_versions.h */
struct Interpreter_library_strings { char *api_string; char *base64_string; };
#include "dynamic_versions.h"

#define LOAD_FUNCTION(symbol) \
	if (return_code && (!((*interpreter)->symbol ## handle =	\
		(void (*)())dlsym(interpreter_handle, #symbol )))) \
	{ \
		((*interpreter)->display_message_function)(ERROR_MESSAGE,"Unable to find symbol %s", #symbol ); \
		return_code = 0; \
	}

#if ! defined (NO_STATIC_FALLBACK)
void __create_interpreter_(int argc, char **argv, const char *initial_comfile,
	struct Interpreter **interpreter, int *status);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __create_interpreter_(argc, argv, initial_comfile, interpreter, status)
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void __interpreter_destroy_string_(struct Interpreter *interpreter, char *string);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __interpreter_destroy_string_(interpreter, string)
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void __destroy_interpreter_(struct Interpreter *interpreter, int *status);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __destroy_interpreter_(interpreter, status)
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void __redirect_interpreter_output_(struct Interpreter *interpreter, int *status);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __redirect_interpreter_output_(interpreter, status)
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void __interpreter_set_display_message_function_(struct Interpreter *interpreter, 
	Interpreter_display_message_function *function, int *status);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __interpreter_set_display_message_function_(interpreter, function, status)
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void __interpret_command_(struct Interpreter *interpreter, char *command_string, 
	void *user_data, int *quit, execute_command_function_type execute_command_function,
	int *status);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __interpret_command_(interpreter, command_string, user_data, quit, \
	execute_command_function, status)
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void __interpreter_evaluate_integer_(struct Interpreter *interpreter, char *expression,
	int *result, int *status);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __interpreter_evaluate_integer_(interpreter, expression, result, status);
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void __interpreter_set_integer_(struct Interpreter *interpreter,
	char *variable_name, int *value, int *status);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __interpreter_set_integer_(interpreter, variable_name, value, status);
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void __interpreter_evaluate_double_(struct Interpreter *interpreter,
	char *expression, double *result, int *status);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __interpreter_evaluate_double_(interpreter, expression, result, status);
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void __interpreter_set_double_(struct Interpreter *interpreter, char *variable_name,
	double *value, int *status);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __interpreter_set_double_(interpreter, variable_name, value, status);
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void __interpreter_evaluate_string_(struct Interpreter *interpreter, char *expression, 
	char **result, int *status);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __interpreter_evaluate_string_(interpreter, expression, result, status);
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void __interpreter_set_string_(struct Interpreter *interpreter, char *variable_name,
	char *value, int *status);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __interpreter_set_string_(interpreter, variable_name, value, status);
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void __interpreter_set_pointer_(struct Interpreter *interpreter, char *variable_name,
	char *class_name, void *value, int *status);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __interpreter_set_pointer_(interpreter, variable_name, class_name, value, status);
#endif /* ! defined (NO_STATIC_FALLBACK) */

void interpret_command_(struct Interpreter *interpreter, char *command_string,
	void *user_data, int *quit,
	execute_command_function_type execute_command_function, int *status)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION:
Takes a <command_string>, processes this through the Perl interpreter.
==============================================================================*/
{
	if (interpreter->use_dynamic_interpreter)
	{
		(interpreter->interpret_command_handle)(interpreter->real_interpreter,
			command_string, user_data, quit, execute_command_function, status);
	}
	else
	{
		__interpret_command_(interpreter->real_interpreter, command_string,
			user_data, quit, execute_command_function, status);
	}
} /* interpret_command */

#if __GLIBC__ >= 2
#include <gnu/libc-version.h>
#endif
#if defined (BYTE_ORDER)
#if (1234==BYTE_ORDER)
static int glibc_version_greater_than_2_2_4(void)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
Need to read the glibc version so that we can determine if we need to 
swap the endianness of values going into a64l
==============================================================================*/
{
#if __GLIBC__ >= 2
	char *version_string;
	int major_version, minor_version, minor_sub_version;
#endif /* __GLIBC__ >= 2 */
	static int return_code = -1;

	/* This gets called a lot so lets make it fast */
	if (return_code == -1)
	{
#if __GLIBC__ >= 2
		version_string = (char *)gnu_get_libc_version();
		if (sscanf(version_string, "%d.%d.%d", &major_version, &minor_version, 
			&minor_sub_version))
		{
			
			if ((major_version > 2) ||
				((major_version == 2) && (minor_version > 2)) ||
				((major_version == 2) && (minor_version == 2) && (minor_sub_version > 4)))
			{
				return_code = 1;
			}
			else
			{
				return_code = 0;
			}
		}
		else
		{
			return_code = 0;
		}
#else /* __GLIBC__ >= 2 */
		return_code = 0;
#endif/* __GLIBC__ >= 2 */
	}
	return (return_code);
} /* get_glibc_version */
#endif /* (1234==BYTE_ORDER) */
#endif /* defined (BYTE_ORDER) */

static char *write_base64_string_to_binary_file(struct Interpreter *interpreter,
	char *base64_string)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
This wrapper allows the passing of the <base64_string> which is intended to
contain a binary file converted to base64.  This function converts it back to
binary and writes a temporary file for which the filename is returned.
It is up to the calling routine to free the string returned and to
remove the temporary file it refers to.
==============================================================================*/
{
	char *return_string,temp_bin_name[L_tmpnam],*total_bin,
		*total_bin_ptr;
	FILE *bin_file;
	size_t bin_length, string_length;
	int bytes_in, bytes_out, i, j;
	long bin_long_data;

	if (base64_string)
	{
		string_length=strlen(base64_string);
		if (tmpnam(temp_bin_name) && (bin_file=fopen(temp_bin_name, "w"))
			&& (total_bin = (char *)malloc(string_length+1))
			&& (return_string = (char *)malloc(strlen(temp_bin_name)+1)))
		{
			total_bin_ptr = total_bin;
			for (i=0;i<string_length;i+=6)
			{
				if (string_length - i < 6)
				{
					bytes_in = string_length - i;
					switch (bytes_in)
					{
						case 2:
						{
							bytes_out = 1;
						} break;
						case 3:
						{
							bytes_out = 2;
						} break;
						case 5:
						{
							bytes_out = 3;
						} break;
						default:
						{
							(*interpreter->display_message_function)(ERROR_MESSAGE,
								"write_base64_string_to_binary_file.  Unexpected remaining length: %d",
								bytes_in);
						} break;
					}
#if defined (BYTE_ORDER)
#if (1234==BYTE_ORDER)
					if (glibc_version_greater_than_2_2_4())
					{
						/* Don't need to swap now */
						bin_long_data=a64l(base64_string + i);
					}
					else
					{
						char tmp_string[6];
						for (j = 0 ; j < bytes_in ; j++)
						{
							tmp_string[j] = base64_string[i + bytes_in - 1 - j];
						}
						tmp_string[j] = 0;
						bin_long_data=a64l(tmp_string);
					}
#else /* (1234==BYTE_ORDER) */
					bin_long_data=a64l(base64_string + i);
#endif /* (1234==BYTE_ORDER) */
#else /* defined (BYTE_ORDER) */
					bin_long_data=a64l(base64_string + i);
#endif /* defined (BYTE_ORDER) */
					*total_bin_ptr = (char)(255 & bin_long_data);
					total_bin_ptr++;
					if (bytes_out > 1)
					{
						*total_bin_ptr = (char)(255 & (bin_long_data >> 8));
						total_bin_ptr++;
						if (bytes_out > 2)
						{
							*total_bin_ptr = (char)(255 & (bin_long_data >> 16));
							total_bin_ptr++;
						}
					}
				}
				else
				{
#if defined (BYTE_ORDER)
#if (1234==BYTE_ORDER)
					if (glibc_version_greater_than_2_2_4())
					{
						/* Don't need to swap now */
						bin_long_data=a64l(base64_string + i);
					}
					else
					{
						char tmp_string[6];
						tmp_string[0]=base64_string[i + 5];
						tmp_string[1]=base64_string[i + 4];
						tmp_string[2]=base64_string[i + 3];
						tmp_string[3]=base64_string[i + 2];
						tmp_string[4]=base64_string[i + 1];
						tmp_string[5]=base64_string[i];
						bin_long_data=a64l(tmp_string);
					}
#else /* (1234==BYTE_ORDER) */
					bin_long_data=a64l(base64_string + i);
#endif /* (1234==BYTE_ORDER) */
#else /* defined (BYTE_ORDER) */
					bin_long_data=a64l(base64_string + i);
#endif /* defined (BYTE_ORDER) */
					*total_bin_ptr = (char)(255 & bin_long_data);
					total_bin_ptr++;
					*total_bin_ptr = (char)(255 & (bin_long_data >> 8));
					total_bin_ptr++;
					*total_bin_ptr = (char)(255 & (bin_long_data >> 16));
					total_bin_ptr++;
					*total_bin_ptr = (char)(255 & (bin_long_data >> 24));
					total_bin_ptr++;
				}
			}
			bin_length = total_bin_ptr - total_bin;
			if (bin_length != fwrite(total_bin,1,bin_length,bin_file))
			{
				(*interpreter->display_message_function)(ERROR_MESSAGE,
					"write_base64_string_to_binary_file.  Short write of temporary binary file.");				
			}
			free(total_bin);
			fclose(bin_file);
			strcpy(return_string, temp_bin_name);
		}
		else
		{
			(*interpreter->display_message_function)(ERROR_MESSAGE,
				"write_base64_string_to_binary_file.  Invalid argument(s)");
			return_string = (char *)NULL;
		}
	}
	else
	{
		(*interpreter->display_message_function)(ERROR_MESSAGE,
			"write_base64_string_to_binary_file.  Invalid argument(s)");
		return_string = (char *)NULL;
	}

	return (return_string);
} /* write_base64_string_to_binary_file */

void create_interpreter_ (int argc, char **argv, const char *initial_comfile,
	struct Interpreter **interpreter, int *status)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION:
Dynamic loader wrapper which loads an appropriate interpreter, initialises all
the function pointers and then calls create_interpreter_ for that instance.
==============================================================================*/
{
	char *library, perl_api_string[200], *perl_executable,
		*perl_executable_default = "perl", *perl_interpreter_string,
		perl_archlib[300];
	fd_set readfds;
	int i, number_of_perl_interpreters, return_code,
		stdout_pipe[2], old_stdout;
	ssize_t number_read;
	struct timeval timeout_struct;
	void *interpreter_handle, *perl_handle;

	return_code = 0;
	if (*interpreter = (struct Interpreter *)malloc (sizeof(struct Interpreter)))
	{
		(*interpreter)->use_dynamic_interpreter = 0;
		(*interpreter)->display_message_function = interpreter_display_message;
		(*interpreter)->real_interpreter = (struct Interpreter *)NULL;

		perl_interpreter_string = (char *)NULL;
		library = (char *)NULL;

		*perl_api_string = 0;
		*perl_archlib = 0;

		interpreter_handle = NULL;
		perl_handle = NULL;

		number_of_perl_interpreters = sizeof(interpreter_strings) /
			sizeof(struct Interpreter_library_strings);
		if (number_of_perl_interpreters)
		{
			if (0 == pipe(stdout_pipe))
			{
				char command[500];

			  /* !!! We should fork here so that the writing doesn't block because the reading process is waiting */
				/* Redirect stdout */
				old_stdout = dup(STDOUT_FILENO);
				dup2(stdout_pipe[1], STDOUT_FILENO);

				/* We are only expecting a little bit of stuff so I am
					hoping that the pipe can buffer it */
				if (!(perl_executable = getenv("CMISS" ABI_ENV "_PERL")))
				{
					if (!(perl_executable = getenv("CMISS_PERL")))
					{
						perl_executable = perl_executable_default;
					}
				}

				/* api_versionstring specifies the binary interface version.
           5.005 series perls use apiversion.
					 It seems that versions prior to 5.005 did not have an api version,
					 but we don't support these anyway so just get the version for
					 the mismatch message below.
				   usethreads use64bitall use64bitint uselongdouble useperlio
				   usemultiplicity specify compile-time options affecting binary
				   compatibility.
					 $Config{version} is filesystem dependent so use
           $^V ? sprintf("%vd",$^V) : $] if the version is required, and
					 $Config{api_revision}.$Config{api_version}.$Config{api_subversion}
           may be better that $Config{api_version_string}.
				*/
				/* !!! length of perl_executable is not checked !!! */
				sprintf(command, "%s -MConfig -e "
								"'print $Config{api_versionstring}||$Config{apiversion}||$],"
								"(map{$Config{\"use$_\"}?\"-$_\":()}"
								"qw(threads multiplicity 64bitall longdouble perlio)),"
								"\" $Config{installarchlib}\"'", perl_executable);
				system(command);

				/* Set stdout back */
				dup2(old_stdout, STDOUT_FILENO);
				close(stdout_pipe[1]);

				FD_ZERO(&readfds);
				FD_SET(stdout_pipe[0], &readfds);
				timeout_struct.tv_sec = 2;
				timeout_struct.tv_usec = 0;
				if (select(FD_SETSIZE, &readfds, NULL, NULL, &timeout_struct))
				{
					char perl_result_buffer[500];
					if (number_read = read(stdout_pipe[0], perl_result_buffer, 499))
					{
						perl_result_buffer[number_read] = 0;
						if (2 == sscanf(perl_result_buffer, "%190s %290s",
														perl_api_string, perl_archlib))
						{
							for (i = 0 ; i < number_of_perl_interpreters ; i++)
							{
								if (0 == strcmp(perl_api_string, interpreter_strings[i].api_string))
								{
									perl_interpreter_string = interpreter_strings[i].base64_string;
								}
							}
						}
						else
						{
							((*interpreter)->display_message_function)(ERROR_MESSAGE,
								"Unexpected result from \"%s\"", command);
						}
					}
					else
					{
						((*interpreter)->display_message_function)(ERROR_MESSAGE,
							"No characters received from \"%s\"", command);
					}
				}
				else
				{
					((*interpreter)->display_message_function)(ERROR_MESSAGE,
						"Timed out executing \"%s\"", command);
				}
			}
		}

		if (perl_interpreter_string)
		{
			if (library = write_base64_string_to_binary_file(*interpreter,
					perl_interpreter_string))
			{
				char perl_shared_library[350];
				sprintf(perl_shared_library, "%s/CORE/libperl.so", perl_archlib);
				if (perl_handle = dlopen(perl_shared_library, RTLD_LAZY | RTLD_GLOBAL))
				{
					if (interpreter_handle = dlopen(library, RTLD_LAZY))
					{
						return_code = 1;
					}
				}
			}
		}

		if (!return_code)
		{
			((*interpreter)->display_message_function)(ERROR_MESSAGE,
				"Unable to open the dynamic perl_interpreter to match your perl \"%s\".",
				perl_executable);
			if (perl_interpreter_string)
			{
				((*interpreter)->display_message_function)(ERROR_MESSAGE,
					"dl library error: %s", dlerror());
			}
			else
			{
				if (*perl_api_string)
				{
					/* We didn't get a match so lets list all the versions strings */
					((*interpreter)->display_message_function)(ERROR_MESSAGE,
						"Your perl reported API version and options \"%s\".",
						perl_api_string);
					((*interpreter)->display_message_function)(ERROR_MESSAGE,
						"The APIs supported by this executable are:");
					for (i = 0 ; i < number_of_perl_interpreters ; i++)
					{
						((*interpreter)->display_message_function)(ERROR_MESSAGE,
							"                         %s",
							interpreter_strings[i].api_string);
					}
				}
			}
			if (perl_handle)
			{
				/* Don't do this as soon as the interpreter_handle fails otherwise this call 
					overwrites the dlerror message from the interpreter_handle */
				dlclose(perl_handle);
			}
		}

		if (return_code)
		{
			LOAD_FUNCTION(create_interpreter_);
			if (return_code)
			{
				((*interpreter)->create_interpreter_handle)(argc, argv, initial_comfile,
					&((*interpreter)->real_interpreter), status);
				return_code = *status;
			}
			LOAD_FUNCTION(interpreter_destroy_string_);
			LOAD_FUNCTION(destroy_interpreter_);
			LOAD_FUNCTION(redirect_interpreter_output_);
			LOAD_FUNCTION(interpreter_set_display_message_function_);
			LOAD_FUNCTION(interpret_command_);
			LOAD_FUNCTION(interpreter_evaluate_integer_);
			LOAD_FUNCTION(interpreter_set_integer_);
			LOAD_FUNCTION(interpreter_evaluate_double_);
			LOAD_FUNCTION(interpreter_set_double_);
			LOAD_FUNCTION(interpreter_evaluate_string_);
			LOAD_FUNCTION(interpreter_set_string_);
			LOAD_FUNCTION(interpreter_set_pointer_);
		}
		if (return_code)
		{
			/* All the functions should be valid if the return_code
				is still 1 */
			(*interpreter)->use_dynamic_interpreter = 1;
		}
		else
		{
#if ! defined (NO_STATIC_FALLBACK)
			((*interpreter)->display_message_function)(ERROR_MESSAGE, "Falling back to using the internal perl interpreter.");
			__create_interpreter_(argc, argv, initial_comfile,
				&((*interpreter)->real_interpreter), status);
			return_code = *status;
#else /* ! defined (NO_STATIC_FALLBACK) */
			((*interpreter)->display_message_function)(ERROR_MESSAGE,
				"No fallback static perl interpreter was included in this executable."
				"This executable will be unable to operate until your perl version matches one of the dynamically included versions.");
			return_code = 0;
			free (*interpreter);
			*interpreter = (struct Interpreter *)NULL;
#endif /* ! defined (NO_STATIC_FALLBACK) */
		}
		if (library)
		{
			/* Hopefully we can remove the file already and if the OS still
				wants it, it will just keep a handle */
			remove(library);
			free(library);
		}
	}
	else
	{
		((*interpreter)->display_message_function)(ERROR_MESSAGE,
			"Unable to allocate memory for internal dynamic perl interpreter structure.");
		return_code = 0;
	}
	*status = return_code;
}

void destroy_interpreter_(struct Interpreter *interpreter, int *status)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
Dynamic loader wrapper
==============================================================================*/
{
	if (interpreter->use_dynamic_interpreter)
	{
		(interpreter->destroy_interpreter_handle)(interpreter->real_interpreter, status);
	}
	else
	{
		__destroy_interpreter_(interpreter->real_interpreter, status);
	}
	free (interpreter);
} /* destroy_interpreter */

void interpreter_set_display_message_function_(struct Interpreter *interpreter, 
	Interpreter_display_message_function *function, int *status)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
Dynamic loader wrapper
==============================================================================*/
{
	/* Set the display message function in this module */
	if (function)
	{
		interpreter->display_message_function = function;
	}
	else
	{
		interpreter->display_message_function = interpreter_display_message;			
	}
	/* Now set it in the actual perl interpreter module */
	if (interpreter->use_dynamic_interpreter)
	{
		(interpreter->interpreter_set_display_message_function_handle)
			(interpreter->real_interpreter, function, status);
	}
	else
	{
		__interpreter_set_display_message_function_(interpreter->real_interpreter, function, status);
	}
} /* redirect_interpreter_output */

void redirect_interpreter_output_(struct Interpreter *interpreter, int *status)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
Dynamic loader wrapper
==============================================================================*/
{
	if (interpreter->use_dynamic_interpreter)
	{
		(interpreter->redirect_interpreter_output_handle)(interpreter->real_interpreter, status);
	}
	else
	{
		__redirect_interpreter_output_(interpreter->real_interpreter, status);
	}
} /* redirect_interpreter_output */

void interpreter_evaluate_integer_(struct Interpreter *interpreter, 
	char *expression, int *result, int *status)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
Dynamic loader wrapper
==============================================================================*/
{
	if (interpreter->use_dynamic_interpreter)
	{
		(interpreter->interpreter_evaluate_integer_handle)(interpreter->real_interpreter, expression, result,
			status);
	}
	else
	{
		__interpreter_evaluate_integer_(interpreter->real_interpreter, expression, result, status);
	}
} /* interpreter_evaluate_integer */

void interpreter_set_integer_(struct Interpreter *interpreter, 
	char *variable_name, int *value, int *status)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
Dynamic loader wrapper
==============================================================================*/
{
	if (interpreter->use_dynamic_interpreter)
	{
		(interpreter->interpreter_set_integer_handle)(interpreter->real_interpreter, variable_name, value,
			status);
	}
	else
	{
		__interpreter_set_integer_(interpreter->real_interpreter, variable_name, value, status);
	}
} /* interpreter_set_integer */

void interpreter_evaluate_double_(struct Interpreter *interpreter, 
	char *expression, double *result, int *status)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
Dynamic loader wrapper
==============================================================================*/
{
	if (interpreter->use_dynamic_interpreter)
	{
		(interpreter->interpreter_evaluate_double_handle)(interpreter->real_interpreter, expression, result,
			status);
	}
	else
	{
		__interpreter_evaluate_double_(interpreter->real_interpreter, expression, result, status);
	}
} /* interpreter_evaluate_double */

void interpreter_set_double_(struct Interpreter *interpreter, 
	char *variable_name, double *value, int *status)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
Dynamic loader wrapper
==============================================================================*/
{
	if (interpreter->use_dynamic_interpreter)
	{
		(interpreter->interpreter_set_double_handle)(interpreter->real_interpreter,
			variable_name, value, status);
	}
	else
	{
		__interpreter_set_double_(interpreter->real_interpreter, variable_name, value, status);
	}
} /* interpreter_set_double */

void interpreter_evaluate_string_(struct Interpreter *interpreter, 
	char *expression, char **result, int *status)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
Dynamic loader wrapper
==============================================================================*/
{
	if (interpreter->use_dynamic_interpreter)
	{
		(interpreter->interpreter_evaluate_string_handle)(
			interpreter->real_interpreter, expression, result, status);
	}
	else
	{
		__interpreter_evaluate_string_(interpreter->real_interpreter, expression,
			result, status);
	}
} /* interpreter_evaluate_string */

void interpreter_destroy_string_(struct Interpreter *interpreter, char *string)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
Dynamic loader wrapper
==============================================================================*/
{
	if (interpreter->use_dynamic_interpreter)
	{
		(interpreter->interpreter_destroy_string_handle)(
			interpreter->real_interpreter, string);
	}
	else
	{
		__interpreter_destroy_string_(interpreter->real_interpreter, string);
	}
} /* interpreter_destroy_string */

void interpreter_set_string_(struct Interpreter *interpreter, 
	char *variable_name, char *value, int *status)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
Dynamic loader wrapper
==============================================================================*/
{
	if (interpreter->use_dynamic_interpreter)
	{
		(interpreter->interpreter_set_string_handle)(interpreter->real_interpreter,
			variable_name, value, status);
	}
	else
	{
		__interpreter_set_string_(interpreter->real_interpreter, variable_name,
			value, status);
	}
} /* interpreter_set_string */

void interpreter_set_pointer_(struct Interpreter *interpreter, 
	char *variable_name, char *class_name, void *value, int *status)
/*******************************************************************************
LAST MODIFIED : 25 January 2005

DESCRIPTION :
Dynamic loader wrapper
==============================================================================*/
{
	if (interpreter->use_dynamic_interpreter)
	{
		(interpreter->interpreter_set_pointer_handle)(interpreter->real_interpreter,
			variable_name, class_name, value, status);
	}
	else
	{
		__interpreter_set_pointer_(interpreter->real_interpreter, variable_name,
			class_name, value, status);
	}
} /* interpreter_set_pointer */

/*
	Local Variables: 
	tab-width: 2
	End: 
*/
