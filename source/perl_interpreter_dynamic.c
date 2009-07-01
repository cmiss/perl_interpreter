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
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdarg.h>
#include "static_version.h"       /* for NO_STATIC_FALLBACK */
#include "perl_interpreter.h"

/******************************************************************************

Types for functions to make inquiries of shared libperls.

These could use information from perl.h, but I don't want to include perl.h as
this module should be independent of the perl binary api and perl.h is not.
Fortunately the api to these functions is fairly simple and has not changed
too much (from 5.6.2 to 5.8.7 at least).

******************************************************************************/

typedef void PerlInterpreter;
/* I haven't checked if the api to XSINIT_t is consistent but we don't need it
	 anyway */
typedef void (*XSINIT_t) ( PerlInterpreter* interp );

typedef PerlInterpreter* (*perl_alloc_t)(void);
typedef void (*perl_construct_t)( PerlInterpreter* interp );
/* perl_destruct actually returns an int from 5.8.0 (but not 5.6.2) but is
	 only useful if PERL_EXIT_DESTRUCT_END is added to PL_exit_flags. */
typedef void (*perl_destruct_t)( PerlInterpreter* interp );
typedef void (*perl_free_t)( PerlInterpreter* interp );
typedef int (*perl_run_t)( PerlInterpreter* interp );
/* perl 5.8.7 at least writes to argv[] in Perl_magic_set when setting $0 */
typedef int (*perl_parse_t)( PerlInterpreter* interp, XSINIT_t xsinit,
								int argc, char** argv, char** env );

/******************************************************************************/

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

	/* dlopened symbols */
	void *interpreter_handle;
	void *perl_handle;

	void(*create_interpreter_handle)(int argc, char **argv, const char *initial_comfile,
		struct Interpreter **interpreter, int *status);
	void (*interpreter_destroy_string_handle)(struct Interpreter *interpreter, char *string);
	void (*destroy_interpreter_handle)(struct Interpreter *interpreter, int *status);
	void (*redirect_interpreter_output_handle)(struct Interpreter *interpreter, int *status);
	void (*interpreter_set_display_message_function_handle)
		(struct Interpreter *interpreter, Interpreter_display_message_function *function, int *status);
	void (*interpret_command_handle)(struct Interpreter *interpreter, 
		const char *command_string, void *user_data, int *quit,
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
		const char *variable_name, const char *value, int *status);
	void (*interpreter_set_pointer_handle)(struct Interpreter *interpreter,
		const char *variable_name, const char *class_name, void *value, int *status);

	struct Interpreter *real_interpreter;
}; /* struct Interpreter */

static int interpreter_display_message(enum Message_type message_type,
	const char *format, ... )
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
void __interpret_command_(struct Interpreter *interpreter, const char *command_string, 
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
void __interpreter_set_string_(struct Interpreter *interpreter, const char *variable_name,
	const char *value, int *status);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __interpreter_set_string_(interpreter, variable_name, value, status);
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void __interpreter_set_pointer_(struct Interpreter *interpreter, const char *variable_name,
	const char *class_name, void *value, int *status);
#else /* ! defined (NO_STATIC_FALLBACK) */
#define __interpreter_set_pointer_(interpreter, variable_name, class_name, value, status);
#endif /* ! defined (NO_STATIC_FALLBACK) */

void interpret_command_(struct Interpreter *interpreter, const char *command_string,
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

/* int type is for equivalence with execvp */ 

static int exec_libperl( const char *libperlname, char *argv[] )
/*******************************************************************************
LAST MODIFIED : 3 November 2005

DESCRIPTION : Run a perlinterpreter from a shared libperl, specified as a
filename for dlopen.  Exits with the exit status from the perlinterpreter or
just EXIT_FAILURE if the perlinterpreter can't be run.
==============================================================================*/
/*
	A libperl can be opened and run just like the perl executable.
	"libraries are programs with multiple entry points, and more
	formally defined interfaces" - libtool.

	See perldoc perlembed and perlapi and (mini)perlmain.c from perl for more
	information.
*/
{
	int argc;
	int exitstatus;
	perl_alloc_t perl_alloc;
	perl_construct_t perl_construct;
  perl_parse_t perl_parse;
  perl_run_t perl_run;
  perl_destruct_t perl_destruct;
  perl_free_t perl_free;
  PerlInterpreter *my_perl;
	void* libperl;

  dlerror(); /* Clear any existing error */

  if( !( libperl = dlopen( libperlname, RTLD_LAZY ) ) )
	{
		interpreter_display_message(ERROR_MESSAGE, "%s\n", dlerror() );
		exit(EXIT_FAILURE);
	}

#ifndef FANCY_STUFF
	if( !( (perl_alloc = (perl_alloc_t) dlsym( libperl, "perl_alloc" ))
				 && (perl_construct = (perl_construct_t) dlsym( libperl, "perl_construct" ))
				 && (perl_parse = (perl_parse_t) dlsym( libperl, "perl_parse" ))
				 && (perl_run = (perl_run_t) dlsym( libperl, "perl_run" ))
				 && (perl_destruct = (perl_destruct_t) dlsym( libperl, "perl_destruct" ))
				 && (perl_free = (perl_free_t) dlsym( libperl, "perl_free" )) ) )
#else

# define LOAD_DL_SYM(handle,symbol) \
  ( symbol = ( symbol ## _t ) dlsym( handle, #symbol ) )

		if( !( LOAD_DL_SYM( libperl, perl_alloc )
					 && LOAD_DL_SYM( libperl, perl_construct )
					 && LOAD_DL_SYM( libperl, perl_parse )
					 && LOAD_DL_SYM( libperl, perl_run )
					 && LOAD_DL_SYM( libperl, perl_destruct )
					 && LOAD_DL_SYM( libperl, perl_free ) ) )

# undef LOAD_DL_SYM
#endif
		{
			char* error = dlerror();
			interpreter_display_message
				( ERROR_MESSAGE, "%s",
					error ? error : "required libperl function has NULL reference" );
			exit(EXIT_FAILURE);
		}

	my_perl = (perl_alloc)();
	if( !my_perl )
		exit(EXIT_FAILURE);

	(perl_construct)( my_perl );

	argc = 0;
	while( argv[argc] != NULL )
		argc++;

	exitstatus =
		(perl_parse)( my_perl, (XSINIT_t)NULL, argc, argv, (char **)NULL);

	if ( !exitstatus )
		exitstatus = (perl_run)(my_perl);

	(perl_destruct)( my_perl );
	(perl_free)( my_perl );

	exit(exitstatus);
	/* 	return(exitstatus); */
}


static ssize_t fork_read_stdout(/*
																	int (*execvp)
																	( const char *file, char *const argv[] )
																	doesn't quite match
																	int (*exec_libperl)
																	( const char *file, char *argv[] )
																	so the argument prototypes are not here because
																	of the warning from gcc.
																*/
																int (*exec_function)(),
																const char *file,
																char *argv[],
																char *buffer,
																size_t buffer_size )
/*******************************************************************************
LAST MODIFIED : 8 November 2005

DESCRIPTION : Forks and runs a function such as execv or execvp in the child
while reading up to count bytes from the stdout in the parent.  Exits 1 if the
child exits EXIT_SUCCESS and less than count bytes are read.  The child is
killed if more than buffer_size bytes are read or it does not respond quickly.

==============================================================================*/
{
	ssize_t number_read;
	pid_t process_id;
	fd_set readfds;
	int stdout_pipe[2];
	struct timeval timeout_struct;

	number_read = -1; /* Assume error until success */

	if (-1 == pipe(stdout_pipe))
	{
		interpreter_display_message
			( ERROR_MESSAGE, "Unable to create pipe: %s", strerror(errno) );
	}
	else
	{
		process_id = fork();

		if (0 == process_id)
		{ /* Child process comes here */
			int stdin_fd;

			close(stdout_pipe[0]); /* For the parent */
				
			/* The child shouldn't read anything */
			/* Is this the best way to redirect stdin to /dev/null? */
			stdin_fd = open ("/dev/null", O_RDONLY);
			dup2 (stdin_fd,STDIN_FILENO);
			close(stdin_fd);
			/* Redirect stdout */
			dup2 (stdout_pipe[1],STDOUT_FILENO);
			close(stdout_pipe[1]);

			/* Execute the perl */
			/* !!! Should first ensure that all non-system file descriptors are closed! */

			(exec_function)( file, argv );
			/* execvp only returns on error
				 (because on success the process gets overlayed). */
			interpreter_display_message
				( ERROR_MESSAGE, "Failed to exec %s: %s", file, strerror(errno) );
			exit(EXIT_FAILURE);
		}

		/* Parent (or no fork) */

		close( stdout_pipe[1] ); /* This was for the child. */

		if( -1 == process_id )
		{
			interpreter_display_message
				( ERROR_MESSAGE, "Unable to fork process: %s", strerror(errno) );
			close( stdout_pipe[0] );
		}
		else /* Have child */
		{
			int status, select_code, this_read;

			FD_ZERO(&readfds);
			FD_SET(stdout_pipe[0], &readfds);
			timeout_struct.tv_sec = 5;
			timeout_struct.tv_usec = 0;

			number_read = 0;

			do
			{
				this_read = -1; /* Assume failure until success */

				do
				{
					select_code =
						select( FD_SETSIZE, &readfds, NULL, NULL,	&timeout_struct );
				}
				while( select_code == -1 && errno == EINTR );

				if( select_code < 0 )
				{
					interpreter_display_message
						( ERROR_MESSAGE, "select failed: %s", strerror(errno) );
				}
				else if( select_code == 0 )
				{
					interpreter_display_message
						( ERROR_MESSAGE, "Timed out waiting for \"%s\"", file );
				}
				else
				{
					do
					{
						this_read = read( stdout_pipe[0], buffer + number_read,
															buffer_size - number_read );
					} while( this_read == -1 && errno == EINTR );
				}

				if( this_read < 0 )
				{
					number_read = this_read;
				}
				else
				{
					number_read += this_read;
				}
			}	while( this_read > 0 && number_read < buffer_size );

			close(stdout_pipe[0]);

			/* Reap the child */

			/* Assuming that if we got what we expected from the
				 child, then it is probably going to exit.  Is this
				 reasonable? */
			if( number_read < 0 || number_read == buffer_size )
			{
				/* Otherwise tell the child to finish.  If it won't
					 accept a SIGTERM then does it have a good reason
					 not to exit yet or should we send a SIGKILL? */
				kill (process_id, SIGTERM);
			}

			/* Do we want to loop until status does not reflect SIGSTOP or
				 similar? */
			waitpid (process_id, &status, 0);

			if( WIFEXITED(status) )
			{
				if( WEXITSTATUS(status) != EXIT_SUCCESS )
				{
					interpreter_display_message
						( ERROR_MESSAGE,
							"\"%s\" failed with status %i", file, WEXITSTATUS(status) );
					number_read = -1;
				}
			}
			else if( WIFSIGNALED(status) )
			{
				interpreter_display_message
					( ERROR_MESSAGE,
						"\"%s\" received signal %i", file, WTERMSIG(status) );
				number_read = -1;
			}
			else
			{
				interpreter_display_message
					( ERROR_MESSAGE,
						"Unexpected status %i from \"%s\"", status, file );
				number_read = -1;
			}
		}
	}

 	return(number_read);
}


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
	char *return_string,*total_bin,
		*total_bin_ptr;
	FILE *bin_file;	
	size_t bin_length, string_length;
	int bytes_in, bytes_out, i, j;
	long bin_long_data;
	char template_name[]="/tmp/perl_interpreterXXXXXX";
	int temp_fd;

	if (base64_string)
	{
		string_length=strlen(base64_string);
		temp_fd=mkstemp(template_name);
		if ((temp_fd != -1) && (bin_file=fdopen(temp_fd, "w"))
			&& (total_bin = (char *)malloc(string_length+1))
			&& (return_string = (char *)malloc(strlen(template_name)+1)))
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
			strcpy(return_string, template_name);
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
	char perl_executable_default[] = "perl";
	char *perl_executable;
	char *library, *perl_api_string,
		*perl_interpreter_string,
		*perl_archlib, *perl_libperl;
	int number_of_perl_interpreters, return_code;
	ssize_t number_read;
	void *interpreter_handle, *perl_handle;

	if (*interpreter = (struct Interpreter *)malloc (sizeof(struct Interpreter)))
	{
		const size_t perl_result_buffer_size = 500;
		char perl_result_buffer[perl_result_buffer_size];

		(*interpreter)->use_dynamic_interpreter = 0;
		(*interpreter)->display_message_function = interpreter_display_message;
		(*interpreter)->real_interpreter = (struct Interpreter *)NULL;

		perl_api_string = (char *)NULL;
		perl_interpreter_string = (char *)NULL;
		library = (char *)NULL;

		interpreter_handle = NULL;
		perl_handle = NULL;

		number_of_perl_interpreters = sizeof(interpreter_strings) /
			sizeof(struct Interpreter_library_strings);

		if (number_of_perl_interpreters)
		{
			char *perl_argv[5];

			if (!(perl_executable = getenv("CMISS" ABI_ENV "_PERL")))
			{
				if (!(perl_executable = getenv("CMISS_PERL")))
				{
					perl_executable = perl_executable_default;
				}
			}

			perl_argv[0] = perl_executable;
			perl_argv[1] = "-MConfig";
			perl_argv[2] = "-e";
			/*
				 api_versionstring specifies the binary interface version.
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
			/*
				$Config{libperl} is usually libperl.a if $Config{useshrplib} is
				false.

				perl 5.8.2 and 5.8.6 on AIX 5.3 have $Config{useshrplib} = true and
				$Config{libperl} = libperl.a but this file is an archive.  The
				shared library that can be dlopened is called libperl.o (from
				obj_ext) even though $Config{dlext} = so.
			*/
			perl_argv[3] = "print join( '-',"
				"$Config{api_versionstring}||$Config{apiversion}||$],"
				"grep {$Config{\"use$_\"}}"
				"qw(threads multiplicity 64bitall longdouble perlio) ),"
				"\"\\0$Config{installarchlib}\\0\","
				"$Config{useshrplib} eq 'true' && $Config{libperl},"
				"\"\\0\"";
			perl_argv[4] = (char *)NULL;

			number_read =
				fork_read_stdout( execvp, perl_executable, perl_argv,
													perl_result_buffer, perl_result_buffer_size );

			/* Error already reported with number_read < 0 */
			if( number_read == 0 )
			{
				((*interpreter)->display_message_function)
					(ERROR_MESSAGE,
					 "No API information received from \"%s\"",
					 perl_executable);
			}
			else if( number_read > 0 )
			{
				{
					size_t i = 0;

					perl_api_string = perl_result_buffer;
					while( i < perl_result_buffer_size && perl_result_buffer[i] )
					{
						i++;
					}
					i++;

					perl_archlib = perl_result_buffer + i;
					while( i < perl_result_buffer_size && perl_result_buffer[i] )
					{
						i++;
					}
					i++;

					perl_libperl = perl_result_buffer + i;
					while( i < perl_result_buffer_size && perl_result_buffer[i] )
					{
						i++;
					}
					if( i >= perl_result_buffer_size )
					{
						((*interpreter)->display_message_function)
							(ERROR_MESSAGE,
							 "Unexpected result for API information from \"%s\"",
							 perl_executable);
						perl_api_string = (char *)NULL;
						perl_archlib = (char *)NULL;
						perl_libperl = (char *)NULL;
					}
					else if( perl_libperl[0] == 0 )
					{
						perl_libperl = "libperl.so";
					}
							
				}

				if( perl_api_string )
				{
					int i;

					for (i = 0 ; i < number_of_perl_interpreters ; i++)
					{
						if (0 == strcmp(perl_api_string, interpreter_strings[i].api_string))
						{
							perl_interpreter_string = interpreter_strings[i].base64_string;
						}
					}
				}
			}
		}

		return_code = 0;

		if (perl_interpreter_string)
		{
			const char core_subdir[] = "CORE";
			char full_libperl_name[ strlen(perl_archlib) + 1
															+ sizeof(core_subdir)
															+ strlen(perl_libperl) + 1 ];
			char *libperl_name = (char *)NULL;
			struct stat stat_buf;

			sprintf( full_libperl_name, "%s/%s/%s",
							 perl_archlib, core_subdir, perl_libperl);

			if( 0 == stat( full_libperl_name, &stat_buf ) )
			{
				libperl_name = full_libperl_name;
			}
			else
			{
				/*
					There is no dynamic libperl in the place where the perl distribution
					would normally install it.

					On Debian (including Ubuntu), libperl.so is in /usr/lib (if it
					exists) and there is no soft link in $Config{archlib}/CORE.
					/usr/lib/libperl.so.N.N is in the libperl58 package but the
					/usr/lib/libperl.so soft link is part of libperl-dev. There is no
					libperl.so.N.  Fortunately, $Config{libperl} is currently set.  The
					Debian perl source package ensures that the shared perl is built
					last so Config.pm is for the shared perl even if the installed perl
					is statically linked (which is the case on i386 only).

					We could try opening libperl.so(.N(|.N)) and checking its api if
					there is no suitable file in $Config{archlib}/CORE.

					Perhaps a CMISS_LIBPERL environment variable should be checked
					before CMISS_PERL?
				*/

				char *perl_argv[5];
				const size_t libperl_result_buffer_size = 500;
				char libperl_result_buffer[libperl_result_buffer_size];

				perl_argv[0] = perl_libperl;
				perl_argv[1] = "-MConfig";
				perl_argv[2] = "-e";
				perl_argv[3] = "print "
#ifdef CHECK_API_ONLY
					"join( '-',"
					"$Config{api_versionstring}||$Config{apiversion}||$],"
					"grep {$Config{\"use$_\"}}"
					"qw(threads multiplicity 64bitall longdouble perlio) ),"
					"\"\\0\""
#else /* ! CHECK_API */
				"\"$Config{installarchlib}\\0\""
#endif
					;
				perl_argv[4] = (char *)NULL;

				number_read =
					fork_read_stdout( exec_libperl, perl_libperl, perl_argv,
														libperl_result_buffer, libperl_result_buffer_size );

				/* Error already reported with number_read < 0 */
				if( number_read == 0 )
				{
					((*interpreter)->display_message_function)
						(ERROR_MESSAGE,
						 "No API information received from \"%s\"",
						 perl_libperl);
				}
				else if( number_read > 0 )
				{
					{
						size_t i = 0;

						while( i < libperl_result_buffer_size && libperl_result_buffer[i] )
						{
							i++;
						}
						if( i >= libperl_result_buffer_size )
						{
							((*interpreter)->display_message_function)
								(ERROR_MESSAGE,
								 "Unexpected result for API information from \"%s\"",
								 perl_libperl);
						}
#ifdef CHECK_API_ONLY
						/*
							If the api of the libperl is the same as the selected perl
							executable, then it is probably better to use this libperl than
							any builtin libperl, but @INC needs to be set to use the
							modules from the selected perl if they are in a different
							location to those of the libperl.
						*/
						else if( 0 == strcmp( libperl_result_buffer, perl_api_string ) )
#else /* ! CHECK_API_ONLY */
						/*
							This essentially checks that the libperl is using modules from
							the same place as the perl executable (which probably means
							they are the same version built with the same options).
						*/
						else if( 0 == strcmp( libperl_result_buffer, perl_archlib ) )
#endif
						{
							/* libperl matches perl */
							libperl_name = perl_libperl;
						}
						else
						{
							((*interpreter)->display_message_function)
								(ERROR_MESSAGE,
								 "\"%s\" does not match your perl \"%s\".",
								 perl_libperl, perl_executable);

						}
					}
				}
			}

			if( libperl_name )
			{
				if( !( perl_handle = dlopen( libperl_name, RTLD_LAZY | RTLD_GLOBAL ) ) )
				{
					((*interpreter)->display_message_function)
						( ERROR_MESSAGE, "%s", dlerror() );
				}
				else if( !( library =
										write_base64_string_to_binary_file
										(*interpreter, perl_interpreter_string ) ) )
				{
					/* error message already displayed */
				}
				else if( !(interpreter_handle = dlopen(library, RTLD_LAZY)) )
				{
					((*interpreter)->display_message_function)
						( ERROR_MESSAGE, "%s", dlerror() );
				}
				else
				{
					return_code = 1;
				}
			}
		}

		if (!return_code)
		{
			/* ??? This message may only be necessary if perl_api_string and
				 !perl_interpreter_string. */
			((*interpreter)->display_message_function)(ERROR_MESSAGE,
				"Unable to open a dynamic perl_interpreter to match your perl \"%s\".",
				perl_executable);

			if ( perl_api_string && !perl_interpreter_string )
				{
					int i;

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
			(*interpreter)->interpreter_handle = interpreter_handle;
			(*interpreter)->perl_handle = perl_handle;
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
		if (interpreter->interpreter_handle)
		{
			 dlclose(interpreter->interpreter_handle);
		}
		if (interpreter->perl_handle)
		{
			 dlclose(interpreter->perl_handle);
		}
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
	const char *variable_name, const char *value, int *status)
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
	const char *variable_name, const char *class_name, void *value, int *status)
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
	??? Is there a suitable c-file-style?
	Local Variables: 
	tab-width: 2
	c-file-offsets: ((substatement-open . 0))
	End: 
*/
