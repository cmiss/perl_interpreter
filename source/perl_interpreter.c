/*******************************************************************************
FILE : perl_interpreter.c

LAST MODIFIED : 24 January 2005

DESCRIPTION :
Provides an interface between cmiss and a Perl interpreter.
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

#ifdef WIN32
/* Need this to make static linking work... */
#define PERLDLL
#endif

#include "EXTERN.h"               /* from the Perl distribution     */
#include "perl.h"                 /* from the Perl distribution     */
#ifdef SHARED_OBJECT /* This condition should really be `shared libperl' */
/* binary compatible accessor functions for perl variables
	 (from the Perl distribution) */
#  include "perlapi.h"
#endif
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include "perl_interpreter.h"

#if defined(WIN32) && !defined(perl_get_av)
#define perl_get_av(a,b) Perl_get_av(aTHX_ a, b)
#define perl_get_sv(a,b) Perl_get_sv(aTHX_ a, b)
#define perl_eval_pv(a,b) Perl_eval_pv(aTHX_ a, b)
#endif

struct Interpreter 
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION :
Internal data storage for this interpreter.
==============================================================================*/
{
	 Interpreter_display_message_function *display_message_function;

	 /***    The Perl interpreter    ***/
	 PerlInterpreter *my_perl;

	 /* perl keeps the argv argument to perl_parse and uses it to modify the
			string argv[0][] when the perl variable $0 is set.  argv[0] and the
		  argv[0][] must therefore continue to be available until the
			interpreter is destroyed. */
	 char *argv[4];
	/*
		perl feels free to modify the strings argv[n][] if they are contiguous
  	with argv[0][].  Allocating space for argv[0][] in this struct ensures that
  	it is not contiguous with argv[1][].
	*/
	 char argv0[2];

	 int perl_interpreter_filehandle_in;
	 int perl_interpreter_filehandle_out;
	 int perl_interpreter_kept_quit;
	 void *perl_interpreter_kept_user_data; 
	 execute_command_function_type kept_execute_command_function;
	 int keep_stdout;
	 int keep_stderr;
}; /* struct Interpreter */

/*
	argv[0][] to perl_parse cannot be just a NUL terminator (zero strlen) as
	from version 5.8.1 (and still 5.8.7) perl tries to set the last non-NUL char
	to NUL in Perl_magic_set.  If there are no non-NUL chars then it sets
	argv[0][-1] to 0.  (5.8.0 just reset the NUL terminator to NUL.)
	Using an arbitrary unlikely string that can be detected later.
*/
static const char initial_argv0[] = "/";
/*
	These are really constants but they are passed to perl_parse as argv[1] and
	argv[2].  perl currently (5.8.7) doesn't modify them (as they are not
	contiguous with argv[0][]), but there is no guarantee that it won't be
	modified in the future.
*/
static char e_string[] = "-e";
static char zero_string[] = "0";

static void xs_init(pTHX);

void boot_Perl_cmiss (pTHX_ CV* cv);
#ifndef WIN32
#  ifndef USE_DYNAMIC_LOADER
void boot_DynaLoader (pTHX_ CV* cv);
#  endif /* ndef USE_DYNAMIC_LOADER */
#endif /* ndef WIN32 */

static void xs_init(pTHX)
{
	char *file_name = __FILE__;
	newXS("Perl_cmiss::bootstrap", boot_Perl_cmiss, file_name);
#ifndef WIN32
	/*
		If we have the ability to load a shared libperl then we cannot export
		symbols from a static perl (because they will mask those in libperl) and
		trying to load module shared objects causes an abort on unresolved
		symbols, so don't include the DynaLoader in these cases.
	*/
#  ifndef USE_DYNAMIC_LOADER
	/* DynaLoader is a special case */
	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file_name);
#  endif /* ndef USE_DYNAMIC_LOADER */
#endif /* ndef WIN32 */
}

static int interpreter_display_message(enum Message_type message_type,
	const char *format, ... )
/*******************************************************************************
LAST MODIFIED : 24 January 2005

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

#if defined (USE_DYNAMIC_LOADER)
/* Mangle the function names from now on so that function loaders 
	 from the perl_interpreter_dynamic module are the ones that CMISS connects to. */
#define create_interpreter_ __create_interpreter_
#define interpreter_destroy_string_ __interpreter_destroy_string_
#define destroy_interpreter_ __destroy_interpreter_
#define redirect_interpreter_output_ __redirect_interpreter_output_
#define interpreter_set_display_message_function_ __interpreter_set_display_message_function_
#define interpret_command_ __interpret_command_
#define interpreter_evaluate_integer_ __interpreter_evaluate_integer_
#define interpreter_set_integer_ __interpreter_set_integer_
#define interpreter_evaluate_double_ __interpreter_evaluate_double_
#define interpreter_set_double_ __interpreter_set_double_
#define interpreter_evaluate_string_ __interpreter_evaluate_string_
#define interpreter_set_string_ __interpreter_set_string_
#define interpreter_set_pointer_ __interpreter_set_pointer_
#endif /* defined (USE_DYNAMIC_LOADER) || defined (SHARED_OBJECT) */

static char *interpreter_duplicate_string(struct Interpreter *interpreter,
	 char *source_string, size_t length)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION :
Returns an allocated copy of <source_string>, or NULL in case of error.  If
<length> is greater than zero than this is the maximum number of characters
copied and the NULL termination is added after that length.
==============================================================================*/
{
	char *copy_of_string;

	if (source_string)
	{
		if (length)
		{
			/* Can't use ALLOCATE as this library is used by CM as well */
			if (copy_of_string = (char *)malloc(length+1))
			{
				strncpy(copy_of_string,source_string,length);
				copy_of_string[length] = 0;
			}
			else
			{
				(interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_duplicate_string.  "
					 "Not enough memory");
			}
		}
		else
		{
			if (copy_of_string = (char *)malloc(strlen(source_string)+1))
			{
				strcpy(copy_of_string,source_string);
			}
			else
			{
				(interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_duplicate_string.  "
					 "Not enough memory");
			}
		}
	}
	else
	{
		(interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_duplicate_string.  "
			 "Invalid argument(s)");
		copy_of_string=(char *)NULL;
	}

	return (copy_of_string);
} /* interpreter_duplicate_string */

void interpreter_destroy_string_(struct Interpreter *interpreter, char *string)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION :
Frees the memory associated with a string allocated by the interpreter.
==============================================================================*/
{
	if (string)
	{
		free(string);
	}
	else
	{
		(interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_duplicate_string.  Invalid argument(s)");
	}
} /* interpreter_duplicate_string */

void create_interpreter_(int argc, char **argv, const char *initial_comfile, 
	 struct Interpreter **interpreter, int *status)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Creates the interpreter for processing commands.
==============================================================================*/
{
  const char *load_commands[] =
  {
		"local $SIG{__WARN__} = sub { die $_[0] };\n"
		"BEGIN {\n"
#include "strict.pmh"
		"import strict \"subs\";\n"
		"}\n"
		"$| = 1;\n"
		,
#include "Balanced.pmh"
		,
#include "Perl_cmiss.pmh"
		,
#if ! defined (WIN32)
    /* This code is not working in Win32 at the moment */
#if ! defined (SHARED_OBJECT)
		/* Using a built-in perl */
		"Perl_cmiss::set_INC_for_platform('" ABI_ENV "')",
#endif /* defined (SHARED_OBJECT) */
		"Perl_cmiss::add_cmiss_perl_to_INC('" ABI_ENV "','" PERL_VERSION_ARCHNAME "')",
#endif /* ! defined (WIN32) */
		"Perl_cmiss::register_keyword assign",
		"Perl_cmiss::register_keyword attach",
		"Perl_cmiss::register_keyword cell",
		"Perl_cmiss::register_keyword command_window",
		"Perl_cmiss::register_keyword create",
		"Perl_cmiss::register_keyword define",
		"Perl_cmiss::register_keyword detach",
		"Perl_cmiss::register_keyword fem",
		"Perl_cmiss::register_keyword gen",
		"Perl_cmiss::register_keyword gfx",
		"Perl_cmiss::register_keyword help",
		"Perl_cmiss::register_keyword imp",
		"Perl_cmiss::register_keyword iterate",
		"Perl_cmiss::register_keyword 'open'",
		"Perl_cmiss::register_keyword 'quit'",
		"Perl_cmiss::register_keyword list",
		"Perl_cmiss::register_keyword list_memory",
		"Perl_cmiss::register_keyword optimise",
		"Perl_cmiss::register_keyword 'read'",
		"Perl_cmiss::register_keyword refresh",
		"Perl_cmiss::register_keyword set",
		"Perl_cmiss::register_keyword unemap",
		"Perl_cmiss::register_keyword var"};
#if ! defined (WIN32)
	/* If there are shared perl interpreters, then there is no static
		 dynaloader linked as it doesn't work with the shared perl.
	*/ 
#  ifndef USE_DYNAMIC_LOADER
#    ifdef INCLUDE_DYNALOADERPMH
	const char DynaLoader_pm[] =
#    include "DynaLoader.pmh"
		;
#    endif /* INCLUDE_DYNALOADERPMH */
#  endif /* ! defined (USE_DYNAMIC_LOADER) */
#endif /* ! defined (WIN32) */

  int i, number_of_load_commands, return_code;

  return_code = 1;

#ifdef WIN32
	PERL_SYS_INIT(&argc, &argv);
#endif

	if (*interpreter = (struct Interpreter *)malloc(sizeof (struct Interpreter)))
	{
		 PerlInterpreter *my_perl;
		 
		 (*interpreter)->display_message_function = interpreter_display_message;
		 (*interpreter)->perl_interpreter_filehandle_in = 0;
		 (*interpreter)->perl_interpreter_filehandle_out = 0;
		 (*interpreter)->keep_stdout = 0;
		 (*interpreter)->keep_stderr = 0;

		 (*interpreter)->my_perl = perl_alloc();
		 my_perl = (*interpreter)->my_perl;
		 /* !!! Check perl_alloc was successful */

		 /*
			 If the first element(s) argv[][] to perl_parse is contiguous with
			 environ (as is likely the case when argv[0] is main's argv[0] and
			 main's argc = 1) then perl 5.8.6 to at least 5.8.7 modify the first
			 environment variable if perl's $0 is set.  (The rest of the environment
			 is safe because it is copied in mg_set from S_init_postdump_symbols in
			 perl.c.)  This effectively removes the first environment variable.

			 To avoid this either:

			   a) Don't use main's argv[0].  (Setting perl's $0 would then no longer
			   alter process information.)  Another string could be supplied
			   and $^X set explicitly if perl does not use /proc/self/exe to set
			   $^X.  Or a copy of main's argv[0] could be used and $^X would always
			   be set appropriately by perl_parse.
			   
				 b) Swap the first two pointers in environ if environ[1] is greater
			   than environ[0] so the first is not contiguous with main's argv[].
				 
				 c) Set PL_use_safe_putenv to 0 so the whole environment is copied.
			   This seems to have been the default prior to perl 5.8.6.  But the
			   copy is only used until perl_destruct is called, at which point the
			   copy is removed and the whole environ is reset to point back at a
			   corrupted environment, removing all environment variables.

			 Approach (a) is used here.
		 */

		 perl_construct(my_perl);
		 (*interpreter)->argv[0] = (*interpreter)->argv0;
		 (*interpreter)->argv[1] = e_string;
		 (*interpreter)->argv[2] = zero_string;
		 /* The NULL terminator doesn't seem used in perl 5.8.7 but seems
				consistent with the description for main in man execve. */
		 (*interpreter)->argv[3] = (char *)NULL;

		 strcpy( (*interpreter)->argv0, initial_argv0 );

		 perl_parse(my_perl, xs_init, 3, (*interpreter)->argv, NULL);
		 perl_run(my_perl);
		 
		 {
			 SV* caret_x = get_sv("\030"/* $^X */,/* create = */TRUE);

			 if( 0 == strcmp( SvPV_nolen(caret_x), initial_argv0 ) )
			 {	/* Perl didn't set $^X */
				 sv_setpv(caret_x,argv[0]);
			 }
		 }

		 {
				STRLEN n_a;
				dSP ;
	  
				ENTER ;
				SAVETMPS;

				PUSHMARK(sp) ;

				/* Override the $0 variable without actually executing the file */

#if 0 && ! defined (WIN32)
				/* This code is not working in Win32 at the moment */
				/* Causes perl to segfault */
				{
					char *perl_invoke_command;

					if (initial_comfile)
					{
						perl_invoke_command = (char *)malloc(20 + strlen(initial_comfile));
						sprintf(perl_invoke_command, "$0 = '%s';\n", initial_comfile);
					}
					else
					{
						perl_invoke_command = (char *)malloc(20);
						sprintf(perl_invoke_command, "$0 = '';\n");
					}
					perl_eval_pv(perl_invoke_command, FALSE);
					free(perl_invoke_command);
				}
#endif /* ! defined (WIN32) */

				sv_setpv( get_sv("0",/* create = */TRUE),
									initial_comfile ? initial_comfile : "" );

				if (argc > 1)
				{
					 AV *perl_argv;
					 if (perl_argv = perl_get_av("ARGV", FALSE))
					 {
							for (i = 1 ; i < argc ; i++)
							{
								 av_push(perl_argv, newSVpv(argv[i], 0));
							}
					 }
					 else
					 {
							((*interpreter)->display_message_function)(ERROR_MESSAGE,"initialise_interpreter.  "
								 "Unable to get ARGV\n") ;
					 }
				}

				number_of_load_commands = sizeof (load_commands) / sizeof (char *);
				for (i = 0 ; i < number_of_load_commands && return_code ; i++)
				{
					 perl_eval_pv(load_commands[i], FALSE);
					 if (SvTRUE(ERRSV))
					 {
							((*interpreter)->display_message_function)(ERROR_MESSAGE,"initialise_interpreter.  "
								 "Uh oh - %s\n", SvPV(ERRSV, n_a)) ;
							/* !!! What does this pop? */
							POPs ;
							*interpreter = (struct Interpreter *)NULL;
							return_code = 0;
					 }
				}

#if ! defined (WIN32)				
#  ifndef USE_DYNAMIC_LOADER
#    ifdef INCLUDE_DYNALOADERPMH
				/* Load the DynaLoader module now so that the module is the same
					 version as the library linked in.  (A hook early in @INC might work
					 also.) */
				/* Dynaloader requires Config so don't print the same error message
					 again on failure. */
				if( SvTRUE( get_sv( "Perl_cmiss::use_config",/* create = */FALSE ) ) )
					{
						/* Load the DynaLoader module. */
						/* Should we check the return code as well as $@ */
						perl_eval_pv( DynaLoader_pm,/* croak_on_error = */FALSE );
						if (SvTRUE(ERRSV))
							{
								((*interpreter)->display_message_function)
									( ERROR_MESSAGE,"initialise_interpreter.  "
										"Failed to load DynaLoader: %s\n", SvPV( ERRSV, n_a ) ) ;
							}
						else
							{
								/* Record that this is loaded so nothing tries to load it
									 again.  If this value doesn't contain "DynaLoader.pm",
									 AutoLoader will look for a file with the name of this
									 string (v5.8.7).
								*/
								const char name[] = "DynaLoader.pm";
								size_t len = strlen(name);
								(void)hv_store( get_hv( "INC", /* create = */FALSE ),
														name, len,
														newSVpv( name, len ),
														0 );
							}
					}
#    endif /* INCLUDE_DYNALOADERPMH */
#  endif /* ! defined (USE_DYNAMIC_LOADER) */
#endif /* ! defined (WIN32) */

				{
					 SV *sv_variable;
					 
					 /* Store this interpreter structure pointer in an int in the
							interpreter so it can be passed by the callback function,
							preferable to having it global in this file. */
					 sv_variable = perl_get_sv("Perl_cmiss::internal_interpreter_structure", TRUE);
					 sv_setiv(sv_variable, (IV)(*interpreter));
				}

				/* !!! Destroy the interpreter if !return_code */

				FREETMPS ;
				LEAVE ;

		 }
		 

	}
	else
	{
		 *interpreter = (struct Interpreter *)NULL;
		 return_code = 0;
	}

	*status = return_code;
}

void destroy_interpreter_(struct Interpreter *interpreter, int *status)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Takes a <command_string>, processes this through the F90 interpreter
and then executes the returned strings
==============================================================================*/
{
	 if (interpreter && (interpreter->my_perl))
	 {
			perl_destruct(interpreter->my_perl);
			perl_free(interpreter->my_perl);

			if(interpreter->perl_interpreter_filehandle_in)
			{
				 close(interpreter->perl_interpreter_filehandle_in);
				 interpreter->perl_interpreter_filehandle_in = 0;
			}
			if(interpreter->perl_interpreter_filehandle_out)
			{
				 close(interpreter->perl_interpreter_filehandle_out);
				 interpreter->perl_interpreter_filehandle_out = 0;
			}
			if (interpreter->keep_stdout)
			{
				 close(interpreter->keep_stdout);
				 interpreter->keep_stdout = 0;
			}
			if (interpreter->keep_stderr)
			{
				 close(interpreter->keep_stderr);
				 interpreter->keep_stderr = 0;
			}

			free (interpreter);
			*status = 1;
	 }
	 else
	 {
			*status = 0;
	 }
}

void interpreter_set_display_message_function_(struct Interpreter *interpreter, 
	 Interpreter_display_message_function *function, int *status)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Sets the function that will be called whenever the Interpreter wants to report
information.
==============================================================================*/
{
	 int return_code;

	 return_code = 1;

	 if (function)
	 {
			interpreter->display_message_function = function;
	 }
	 else
	 {
			interpreter->display_message_function = interpreter_display_message;			
	 }

	 *status = return_code;
}

void redirect_interpreter_output_(struct Interpreter *interpreter, int *status)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
This redirects the output from stdout to a pipe so that the handle_output
routine can write this to the command window.
==============================================================================*/
{
  int filehandles[2], return_code;

  return_code = 1;

#if ! defined (WIN32)
	/* Windows is not yet working with the redirect but that is OK */
  if (0 == pipe (filehandles))
  {
	  interpreter->perl_interpreter_filehandle_in = filehandles[1];
	  interpreter->perl_interpreter_filehandle_out = filehandles[0];

	  interpreter->keep_stdout = dup(STDOUT_FILENO);
		interpreter->keep_stderr = dup(STDERR_FILENO);
  }
  else
  {
	  (interpreter->display_message_function)(ERROR_MESSAGE,"redirect_interpreter_output.  "
		  "Unable to create pipes") ;
	  return_code = 0;
  }
#endif /* ! defined (WIN32) */

  *status = return_code;
}

static int handle_output(struct Interpreter *interpreter)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
==============================================================================*/
{
	char buffer[1000];
	fd_set readfds;
	int flags, return_code;
#if defined (WIN32)
#define ssize_t long
#endif
  ssize_t read_length;
	struct timeval timeout_struct;
	
	return_code = 1;

	/* if (perl_interpreter_filehandle_in)
	{
		fsync(perl_interpreter_filehandle_in);
		}*/
	if (interpreter->perl_interpreter_filehandle_out)
	{
		FD_ZERO(&readfds);
		FD_SET(interpreter->perl_interpreter_filehandle_out, &readfds);
		timeout_struct.tv_sec = 0;
		timeout_struct.tv_usec = 0;
		 
#if ! defined (WIN32)
		/* Empty the output buffer and send it to the screen */
		flags = fcntl (interpreter->perl_interpreter_filehandle_out, F_GETFL);
		/*???DB.  Wouldn't compile at home.  O_NDELAY is equivalent to
		  FNDELAY */
		/*					flags &= FNDELAY;*/
		flags &= O_NDELAY;
		flags &= O_NONBLOCK;
		fcntl (interpreter->perl_interpreter_filehandle_out, F_SETFL, flags);
		/* fsync(perl_interpreter_filehandle_out); */
#endif /* ! defined (WIN32) */
		while (select(FD_SETSIZE, &readfds, NULL, NULL, &timeout_struct))
		{
			if (read_length = read(interpreter->perl_interpreter_filehandle_out, (void *)buffer, sizeof(buffer) - 1))
			{
				buffer[read_length] = 0;
				(interpreter->display_message_function)(INFORMATION_MESSAGE,
					"%s", buffer) ;				
			}
		}
	}
	return (return_code);
} /* handle_output */

/* This function is specified in a compile time define so that it can be mangled to match only
   the corresponding Perl_cmiss XS code */
int CMISS_PERL_CALLBACK(void *interpreter_void, char *command_string)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Takes a <command_string>, processes this through the F90 interpreter
and then executes the returned strings
==============================================================================*/
{
	int quit, return_code;
	struct Interpreter *interpreter;

	if (command_string)
	{
		 interpreter = (struct Interpreter *)interpreter_void;

		/* Change STDOUT and STDERR back to the standard pipes */
		if (interpreter->keep_stdout)
		{
			dup2(interpreter->keep_stdout, STDOUT_FILENO);
		}
		if (interpreter->keep_stderr)
		{
			dup2(interpreter->keep_stderr, STDERR_FILENO);
		}

#if defined (CMISS_DEBUG)
		printf("cmiss_perl_callback: %s\n", command_string);
#endif /* defined (CMISS_DEBUG) */

		handle_output(interpreter);

		quit = interpreter->perl_interpreter_kept_quit;

		(interpreter->kept_execute_command_function)(command_string, 
			 interpreter->perl_interpreter_kept_user_data, &quit, &return_code);

		interpreter->perl_interpreter_kept_quit = quit;

#if defined (CMISS_DEBUG)
		printf("cmiss_perl_callback code: %d (%s)\n", return_code, command_string);
#endif /* defined (CMISS_DEBUG) */

		/* Put the redirection back on again */
		if (interpreter->perl_interpreter_filehandle_in)
		{
			dup2(interpreter->perl_interpreter_filehandle_in, STDOUT_FILENO);
			dup2(interpreter->perl_interpreter_filehandle_in, STDERR_FILENO);
		}
	}
	else
	{
		(interpreter->display_message_function)(ERROR_MESSAGE,"cmiss_perl_callback.  "
			 "Missing command_data");
		return_code=0;
	}

	return (return_code);
} /* cmiss_perl_callback */

void interpret_command_(struct Interpreter *interpreter, char *command_string, 
	 void *user_data, int *quit,
  execute_command_function_type execute_command_function, int *status)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Takes a <command_string>, processes this through the Perl interpreter.
==============================================================================*/
{
	char *escaped_command, *new_pointer, *old_pointer, *quote_pointer,
		*slash_pointer, *wrapped_command;
	int escape_symbols, return_code;
	PerlInterpreter *my_perl;

	if (interpreter && (my_perl = interpreter->my_perl))
	{
		 STRLEN n_a;
		 dSP ;
 
		 ENTER ;
		 SAVETMPS;

		 if (command_string)
		 {
				PUSHMARK(sp) ;
				interpreter->perl_interpreter_kept_user_data = user_data;
				interpreter->perl_interpreter_kept_quit = *quit;

				interpreter->kept_execute_command_function = execute_command_function;

				return_code = 1;

				escape_symbols = 0;
				if ((quote_pointer = strchr (command_string, '\'')) ||
					 (slash_pointer = strchr (command_string, '\\')))
				{
					 /* Count how many things we are going to escape */
					 quote_pointer = command_string;
					 while (quote_pointer = strchr (quote_pointer, '\\'))
					 {
							quote_pointer++;
							escape_symbols++;
					 }
					 quote_pointer = command_string;
					 while (quote_pointer = strchr (quote_pointer, '\''))
					 {
							quote_pointer++;
							escape_symbols++;
					 }
				}

				if (wrapped_command = (char *)malloc(strlen(command_string) + 
					 escape_symbols + 100))
				{
					 /* Escape any 's in the string */
					 if (escape_symbols)
					 {
							if (escaped_command = (char *)malloc(strlen(command_string) + 
								 escape_symbols + 10))
							{
								 slash_pointer = strchr (command_string, '\\');
								 new_pointer = escaped_command;
								 old_pointer = command_string;
								 strcpy(new_pointer, old_pointer);
								 while (slash_pointer)
								 {
										new_pointer += slash_pointer - old_pointer;
										old_pointer = slash_pointer;
										*new_pointer = '\\';
										new_pointer++;
					  
										strcpy(new_pointer, old_pointer);

										slash_pointer = strchr (slash_pointer + 1, '\\');
								 }
								 strcpy(wrapped_command, escaped_command);
								 new_pointer = escaped_command;
								 old_pointer = wrapped_command;
								 quote_pointer = strchr (wrapped_command, '\'');
								 while (quote_pointer)
								 {
										new_pointer += quote_pointer - old_pointer;
										old_pointer = quote_pointer;
										*new_pointer = '\\';
										new_pointer++;
					  
										strcpy(new_pointer, old_pointer);

										quote_pointer = strchr (quote_pointer + 1, '\'');
								 }
								 sprintf(wrapped_command, "Perl_cmiss::execute_command('%s')",
										escaped_command);

								 free (escaped_command);
							}
							else
							{
								 (interpreter->display_message_function)(ERROR_MESSAGE,"cmiss_perl_execute_command.  "
										"Unable to allocate escaped_string");
								 return_code=0;
							}
					 }
					 else
					 {
							sprintf(wrapped_command, "Perl_cmiss::execute_command('%s')",
								 command_string);
					 }
#if defined (CMISS_DEBUG)
					 printf("cmiss_perl_execute_command: %s\n", wrapped_command);
#endif /* defined (CMISS_DEBUG) */

					 if (interpreter->perl_interpreter_filehandle_in)
					 {
							/* Redirect STDOUT and STDERR */
							dup2(interpreter->perl_interpreter_filehandle_in, STDOUT_FILENO);
							dup2(interpreter->perl_interpreter_filehandle_in, STDERR_FILENO);
					 }

					 perl_eval_pv(wrapped_command, FALSE);

					 handle_output(interpreter);

					 if (SvTRUE(ERRSV))
					 {
							(interpreter->display_message_function)(ERROR_MESSAGE,
								 "%s", SvPV(ERRSV, n_a)) ;
							POPs ;
							return_code = 0;
					 }

					 /* Change STDOUT and STDERR back again */
					 if (interpreter->keep_stdout)
					 {
							dup2(interpreter->keep_stdout, STDOUT_FILENO);
					 }
					 if (interpreter->keep_stderr)
					 {
							dup2(interpreter->keep_stderr, STDERR_FILENO);
					 }

					 *quit = interpreter->perl_interpreter_kept_quit;
 
					 /*  This command needs to get the correct response from a 
							 partially complete command before it is useful
							 if (!SvTRUE(cvrv))
							 {
							 (interpreter->display_message_function)(ERROR_MESSAGE,
							 "Unable to compile command: %s\n", wrapped_command) ;
							 POPs ;
							 }*/

					 free (wrapped_command);
				}
				else
				{
					 (interpreter->display_message_function)(ERROR_MESSAGE,"interpret_command.  "
							"Unable to allocate wrapped_string");
					 return_code=0;
				}
		 }
		 else
		 {
				(interpreter->display_message_function)(ERROR_MESSAGE,"interpret_command.  "
					 "Missing command_data");
				return_code=0;
		 }

		 FREETMPS ;
		 LEAVE ;	

	}
	else
	{
		 (interpreter->display_message_function)(ERROR_MESSAGE,"interpret_command.  "
				"Missing interpreter");
		 return_code=0;
	}
	
	*status = return_code;
} /* interpret_command_ */

void interpreter_evaluate_integer_(struct Interpreter *interpreter, 
	 char *expression, int *result, int *status)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Use the perl_interpreter to evaluate the given string <expression> and return 
its value as an integer <result>.  If the string <expression> does not evaluate
as an integer then <status> will be set to zero.
==============================================================================*/
{
	int return_code;
	PerlInterpreter *my_perl;

	return_code = 1;

	if (interpreter && (my_perl = interpreter->my_perl))
	{
		 STRLEN n_a;
		 dSP ;
		 SV *sv_result;

		 ENTER ;
		 SAVETMPS;

		 if (expression && result && status)
		 {
				if (interpreter->perl_interpreter_filehandle_in)
				{
					 /* Redirect STDOUT and STDERR */
					 dup2(interpreter->perl_interpreter_filehandle_in, STDOUT_FILENO);
					 dup2(interpreter->perl_interpreter_filehandle_in, STDERR_FILENO);
				}
				
				sv_result = perl_eval_pv(expression, FALSE);

				/* Change STDOUT and STDERR back again */
				if (interpreter->keep_stdout)
				{
					 dup2(interpreter->keep_stdout, STDOUT_FILENO);
				}
				if (interpreter->keep_stderr)
				{
					 dup2(interpreter->keep_stderr, STDERR_FILENO);
				}
 
				handle_output(interpreter);

				if (SvTRUE(ERRSV))
				{
					 (interpreter->display_message_function)(ERROR_MESSAGE,
							"%s", SvPV(ERRSV, n_a)) ;
					 POPs ;
					 return_code = 0;
				}
				else
				{
					 if (SvIOK(sv_result))
					 {
							*result = SvIV(sv_result);
							return_code = 1;
					 }
					 else
					 {
							(interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_integer.  "
								 "String \"%s\" does not evaluate to an integer.", expression);
							return_code = 0;
					 }
				}
		 }
		 else
		 {
				(interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_integer.  "
					 "Invalid arguments.") ;
				return_code = 0;
		 }
		 
		 FREETMPS ;
		 LEAVE ;
	}
	else
	{
		 (interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_integer.  "
				"Missing interpreter");
		 return_code=0;
	}

	*status = return_code;
} /* interpreter_evaluate_integer_ */

void interpreter_set_integer_(struct Interpreter *interpreter, 
	 char *variable_name, int *value, int *status)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Sets the value of the scalar variable cmiss::<variable_name> to be <value>.
To override the cmiss:: package specify the full name in the string.
==============================================================================*/
{
	int return_code;
	PerlInterpreter *my_perl;

	return_code = 1;

	if (interpreter && (my_perl = interpreter->my_perl))
	{
		 SV *sv_variable;

		 ENTER ;
		 SAVETMPS;

		 if (variable_name && value && status)
		 {
				sv_variable = perl_get_sv(variable_name, TRUE);
				sv_setiv(sv_variable, *value);
		 }
		 else
		 {
				(interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_set_integer.  "
					 "Invalid arguments.") ;
				return_code = 0;
		 }

		 FREETMPS ;
		 LEAVE ;
 	}
	else
	{
		 (interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_set_integer.  "
				"Missing interpreter");
		 return_code=0;
	}
 
	*status = return_code;
} /* interpreter_set_integer_ */

void interpreter_evaluate_double_(struct Interpreter *interpreter, 
	 char *expression, double *result, int *status)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Use the perl_interpreter to evaluate the given string <expression> and return 
its value as an double <result>.  If the string <expression> does not evaluate
as an double then <status> will be set to zero.
==============================================================================*/
{
	int return_code;
	PerlInterpreter *my_perl;

	return_code = 1;

	if (interpreter->my_perl)
	{
		 STRLEN n_a;
		 dSP ;
		 SV *sv_result;
 
		 ENTER ;
		 SAVETMPS;

		 if (expression && result && status)
		 {
				if (interpreter->perl_interpreter_filehandle_in)
				{
					 /* Redirect STDOUT and STDERR */
					 dup2(interpreter->perl_interpreter_filehandle_in, STDOUT_FILENO);
					 dup2(interpreter->perl_interpreter_filehandle_in, STDERR_FILENO);
				}
				
				sv_result = perl_eval_pv(expression, FALSE);

				/* Change STDOUT and STDERR back again */
				if (interpreter->keep_stdout)
				{
					 dup2(interpreter->keep_stdout, STDOUT_FILENO);
				}
				if (interpreter->keep_stderr)
				{
					 dup2(interpreter->keep_stderr, STDERR_FILENO);
				}
 
				handle_output(interpreter);

				if (SvTRUE(ERRSV))
				{
					 (interpreter->display_message_function)(ERROR_MESSAGE,
							"%s", SvPV(ERRSV, n_a)) ;
					 POPs ;
					 return_code = 0;
				}
				else
				{
					 if (SvNOK(sv_result))
					 {
							*result = SvNV(sv_result);
							return_code = 1;
					 }
					 else
					 {
							(interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_double.  "
								 "String \"%s\" does not evaluate to a double.", expression);
							return_code = 0;
					 }
				}
		 }
		 else
		 {
				(interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_double.  "
					 "Invalid arguments.") ;
				return_code = 0;
		 }

		 FREETMPS ;
		 LEAVE ;
 	}
	else
	{
		 (interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_double.  "
				"Missing interpreter");
		 return_code=0;
	}

	*status = return_code;
} /* interpreter_evaluate_double_ */

void interpreter_set_double_(struct Interpreter *interpreter, 
	 char *variable_name, double *value, int *status)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Sets the value of the scalar variable cmiss::<variable_name> to be <value>.
To override the cmiss:: package specify the full name in the string.
==============================================================================*/
{
	int return_code;
	PerlInterpreter *my_perl;

	return_code = 1;

	if (interpreter && (my_perl = interpreter->my_perl))
	{
		 SV *sv_variable;

		 ENTER ;
		 SAVETMPS;

		 if (variable_name && value && status)
		 {
				sv_variable = perl_get_sv(variable_name, TRUE);
				sv_setnv(sv_variable, *value);
		 }
		 else
		 {
				(interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_set_double.  "
					 "Invalid arguments.") ;
				return_code = 0;
		 }
 
		 FREETMPS ;
		 LEAVE ;
 	}
	else
	{
		 (interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_set_double.  "
				"Missing interpreter");
		 return_code=0;
	}

	*status = return_code;
} /* interpreter_set_double_ */

void interpreter_evaluate_string_(struct Interpreter *interpreter, 
	 char *expression, char **result, int *status)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Use the perl_interpreter to evaluate the given string <expression> and return 
its value as an string in <result>.  The string is allocated and it is up to 
the calling routine to release the string with Interpreter_destroy_string when
it is done.  If the string <expression> does not evaluate
as an string then <status> will be set to zero and <*result> will be NULL.
==============================================================================*/
{
	char *internal_string;
	int return_code;
	PerlInterpreter *my_perl;

	return_code = 1;

	*result = (char *)NULL;
	if (interpreter && (my_perl = interpreter->my_perl))
	{ 
		 STRLEN n_a, string_length;
		 dSP ;
		 SV *sv_result;

		 ENTER ;
		 SAVETMPS;

		 if (expression && result && status)
		 {
				if (interpreter->perl_interpreter_filehandle_in)
				{
					 /* Redirect STDOUT and STDERR */
					 dup2(interpreter->perl_interpreter_filehandle_in, STDOUT_FILENO);
					 dup2(interpreter->perl_interpreter_filehandle_in, STDERR_FILENO);
				}
				
				sv_result = perl_eval_pv(expression, FALSE);

				/* Change STDOUT and STDERR back again */
				if (interpreter->keep_stdout)
				{
					 dup2(interpreter->keep_stdout, STDOUT_FILENO);
				}
				if (interpreter->keep_stderr)
				{
					 dup2(interpreter->keep_stderr, STDERR_FILENO);
				}
 
				handle_output(interpreter);

				if (SvTRUE(ERRSV))
				{
					 (interpreter->display_message_function)(ERROR_MESSAGE,
							"%s", SvPV(ERRSV, n_a)) ;
					 POPs ;
					 return_code = 0;
				}
				else
				{
					 if (SvPOK(sv_result))
					 {
							internal_string = SvPV(sv_result, string_length);
							if (*result = interpreter_duplicate_string(interpreter,
								 internal_string, string_length))
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
							(interpreter->display_message_function)(ERROR_MESSAGE,
								 "interpreter_evaluate_string.  "
								 "String \"%s\" does not evaluate to a string.",
								 expression);
							return_code = 0;
					 }
				}
		 }
		 else
		 {
				(interpreter->display_message_function)(ERROR_MESSAGE,
					 "interpreter_evaluate_string.  Invalid arguments.") ;
				return_code = 0;
		 }
		 
		 FREETMPS ;
		 LEAVE ;
 	}
	else
	{
		 (interpreter->display_message_function)(ERROR_MESSAGE,
				"interpreter_evaluate_string.  Missing interpreter");
		 return_code=0;
	}

	*status = return_code;
} /* interpreter_evaluate_string_ */

void interpreter_set_string_(struct Interpreter *interpreter, 
	 char *variable_name, char *value, int *status)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Sets the value of the scalar variable cmiss::<variable_name> to be <value>.
To override the cmiss:: package specify the full name in the string.
==============================================================================*/
{
	int return_code;
	PerlInterpreter *my_perl;

	return_code = 1;

	if (interpreter && (my_perl = interpreter->my_perl))
	{
		 SV *sv_variable;

		 ENTER ;
		 SAVETMPS;

		 if (variable_name && value && status)
		 {
				sv_variable = perl_get_sv(variable_name, TRUE);
				sv_setpv(sv_variable, value);
		 }
		 else
		 {
				(interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_set_string.  "
					 "Invalid arguments.") ;
				return_code = 0;
		 }

		 FREETMPS ;
		 LEAVE ;
 	}
	else
	{
		 (interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_set_string.  Missing interpreter");
		 return_code=0;
	}

	*status = return_code;
} /* interpreter_set_string_ */

void interpreter_set_pointer_(struct Interpreter *interpreter,
	 char *variable_name, char *class_name, void *value,int *status)
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Sets the value of the scalar variable cmiss::<variable_name> to be <value> and 
sets the class of that variable to be <class_name>.
To override the cmiss:: package specify the full name in the string.
==============================================================================*/
{
	int return_code;
	PerlInterpreter *my_perl;

	return_code = 1;

	if (interpreter && (my_perl = interpreter->my_perl))
	{
		 SV *sv_variable;

		 ENTER ;
		 SAVETMPS;

		 if (variable_name && value && status)
		 {
				sv_variable = perl_get_sv(variable_name, TRUE);
				sv_setref_pv(sv_variable, class_name, value);
		 }
		 else
		 {
				(interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_set_string.  "
					 "Invalid arguments.") ;
				return_code = 0;
		 }

		 FREETMPS ;
		 LEAVE ;
 	}
	else
	{
		 (interpreter->display_message_function)(ERROR_MESSAGE,"interpreter_set_string.  Missing interpreter");
		 return_code=0;
	}

	*status = return_code;
} /* interpreter_set_string_ */

/*
	Local Variables: 
	tab-width: 2
	c-file-offsets: ((substatement-open . 0))
	End: 
*/
