/*******************************************************************************
FILE : perl_interpreter.c

LAST MODIFIED : 26 March 2003

DESCRIPTION :
Provides an interface between cmiss and a Perl interpreter.
==============================================================================*/

#if ! defined (NO_STATIC_FALLBACK)
#include <EXTERN.h>               /* from the Perl distribution     */
#include <perl.h>                 /* from the Perl distribution     */
#endif /* ! defined (NO_STATIC_FALLBACK) */
#include <stdio.h>
#include <unistd.h>
#if defined (USE_DYNAMIC_LOADER)
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dlfcn.h>
#endif /* defined (USE_DYNAMIC_LOADER) */
#include <stdarg.h>
#include "perl_interpreter.h"

#if ! defined (NO_STATIC_FALLBACK)
/***    The Perl interpreter    ***/
PerlInterpreter *my_perl = (PerlInterpreter *)NULL;

static void xs_init(pTHX);

void boot_DynaLoader (pTHX_ CV* cv);
void boot_Perl_cmiss (pTHX_ CV* cv);
static int perl_interpreter_filehandle_in = 0;
static int perl_interpreter_filehandle_out = 0;
static int perl_interpreter_kept_quit;
static void *perl_interpreter_kept_user_data; 
static execute_command_function_type kept_execute_command_function;
static int keep_stdout = 0;
static int keep_stderr = 0;
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void xs_init(pTHX)
{
	char *file_name = __FILE__;
	newXS("Perl_cmiss::bootstrap", boot_Perl_cmiss, file_name);
	/* DynaLoader is a special case */
	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file_name);
}
#endif /* ! defined (NO_STATIC_FALLBACK) */

static int interpreter_display_message(enum Message_type message_type,
	char *format, ... )
/*******************************************************************************
LAST MODIFIED : 26 March 2003

DESCRIPTION :
The default interpreter_display_message_function.
==============================================================================*/
{
	int return_code;
	va_list ap;

	va_start(ap,format);
	return_code=vfprintf(stderr,format,ap);
	va_end(ap);

	return (return_code);
} /* interpreter_display_message */

static Interpreter_display_message_function *display_message_function =
   interpreter_display_message;

#if defined (USE_DYNAMIC_LOADER)
#include "perl_interpreter_dynamic.h"
#endif /* defined (USE_DYNAMIC_LOADER) */

#if defined (USE_DYNAMIC_LOADER) || defined (SHARED_OBJECT)
/* Mangle the function names from now on so that function loaders 
	 are the ones that CMISS connects to. */
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
#endif /* defined (USE_DYNAMIC_LOADER) || defined (SHARED_OBJECT) */

#if ! defined (NO_STATIC_FALLBACK)
static char *interpreter_duplicate_string(char *source_string, size_t length)
/*******************************************************************************
LAST MODIFIED : 7 September 2000

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
				(*display_message_function)(ERROR_MESSAGE,"interpreter_duplicate_string.  "
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
				(*display_message_function)(ERROR_MESSAGE,"interpreter_duplicate_string.  "
					 "Not enough memory");
			}
		}
	}
	else
	{
		(*display_message_function)(ERROR_MESSAGE,"interpreter_duplicate_string.  "
			 "Invalid argument(s)");
		copy_of_string=(char *)NULL;
	}

	return (copy_of_string);
} /* interpreter_duplicate_string */
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void interpreter_destroy_string_(char *string)
/*******************************************************************************
LAST MODIFIED : 7 September 2000

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
		(*display_message_function)(ERROR_MESSAGE,"interpreter_duplicate_string.  Invalid argument(s)");
	}
} /* interpreter_duplicate_string */
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void create_interpreter_(int argc, char **argv, const char *initial_comfile, int *status)
/*******************************************************************************
LAST MODIFIED : 24 July 2001

DESCRIPTION:
Creates the interpreter for processing commands.
==============================================================================*/
{
  char *embedding[3], e_string[] = "-e", zero_string[] = "0";
	char *perl_invoke_command;
  char perl_start_code[] = 
	 "local $SIG{__WARN__} = sub { die $_[0] };\n"
	 "BEGIN {\n"
#include "strict.pmh"
	 "import strict \"subs\";\n"
	 "}\n"
#include "Balanced.pmh"
#include "Perl_cmiss.pmh"
	 "$| = 1;\n"
    ;
  char *load_commands[] =
  {
#if ! defined (WIN32)
    /* This code is not working in Win32 at the moment */
#if ! defined (SHARED_OBJECT)
		"Perl_cmiss::set_INC_for_platform('" ABI_ENV "')",
#endif /* defined (SHARED_OBJECT) */
		"Perl_cmiss::add_cmiss_perl_to_INC('" ABI_ENV "')",
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
  int i, number_of_load_commands, return_code;

  return_code = 1;

  my_perl = perl_alloc();
  perl_construct(my_perl);
	embedding[0] = argv[0];
	embedding[1] = e_string;
	embedding[2] = zero_string;
  perl_parse(my_perl, xs_init, 3, embedding, NULL);
  perl_run(my_perl);
  
  {
	 STRLEN n_a;
	 dSP ;
	  
	 ENTER ;
	 SAVETMPS;

	 PUSHMARK(sp) ;
	 /* Override the $0 variable without actually executing the file */
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
				 (*display_message_function)(ERROR_MESSAGE,"initialise_interpreter.  "
						"Unable to get ARGV\n") ;
			}
	 }
	 perl_eval_pv(perl_start_code, FALSE);
	 if (SvTRUE(ERRSV))
		{
		  (*display_message_function)(ERROR_MESSAGE,"initialise_interpreter.  "
			 "Uh oh - %s\n", SvPV(ERRSV, n_a)) ;
		  POPs ;
		  return_code = 0;
		}

	 number_of_load_commands = sizeof (load_commands) / sizeof (char *);
	 for (i = 0 ; i < number_of_load_commands && return_code ; i++)
		{
		  perl_eval_pv(load_commands[i], FALSE);
		  if (SvTRUE(ERRSV))
			 {
				(*display_message_function)(ERROR_MESSAGE,"initialise_interpreter.  "
				  "Uh oh - %s\n", SvPV(ERRSV, n_a)) ;
				POPs ;
				return_code = 0;
			 }
		}

	 FREETMPS ;
	 LEAVE ;

  }

  *status = return_code;

}
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void destroy_interpreter_(int *status)
/*******************************************************************************
LAST MODIFIED : 8 June 2000

DESCRIPTION:
Takes a <command_string>, processes this through the F90 interpreter
and then executes the returned strings
==============================================================================*/
{
	 if (my_perl)
	 {
			perl_destruct(my_perl);
			perl_free(my_perl);

			if(perl_interpreter_filehandle_in)
			{
				 close(perl_interpreter_filehandle_in);
				 perl_interpreter_filehandle_in = 0;
			}
			if(perl_interpreter_filehandle_out)
			{
				 close(perl_interpreter_filehandle_out);
				 perl_interpreter_filehandle_out = 0;
			}
			if (keep_stdout)
			{
				 close(keep_stdout);
				 keep_stdout = 0;
			}
			if (keep_stderr)
			{
				 close(keep_stderr);
				 keep_stderr = 0;
			}
			*status = 1;
	 }
	 else
	 {
			*status = 0;
	 }
}
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void interpreter_set_display_message_function_(Interpreter_display_message_function *function,
	int *status)
/*******************************************************************************
LAST MODIFIED : 26 March 2003

DESCRIPTION:
Sets the function that will be called whenever the Interpreter wants to report
information.
==============================================================================*/
{
	 int return_code;

	 return_code = 1;

	 if (function)
	 {
			display_message_function = function;
	 }
	 else
	 {
			display_message_function = interpreter_display_message;			
	 }

	 *status = return_code;
}
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void redirect_interpreter_output_(int *status)
/*******************************************************************************
LAST MODIFIED : 25 August 2000

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
	  perl_interpreter_filehandle_in = filehandles[1];
	  perl_interpreter_filehandle_out = filehandles[0];

	  keep_stdout = dup(STDOUT_FILENO);
	  keep_stderr = dup(STDERR_FILENO);
  }
  else
  {
	  (*display_message_function)(ERROR_MESSAGE,"redirect_interpreter_output.  "
		  "Unable to create pipes") ;
	  return_code = 0;
  }
#endif /* ! defined (WIN32) */

  *status = return_code;
}
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
static int handle_output(void)
/*******************************************************************************
LAST MODIFIED : 19 May 2000

DESCRIPTION:
Takes a <command_string>, processes this through the F90 interpreter
and then executes the returned strings
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
	if (perl_interpreter_filehandle_out)
	{
		FD_ZERO(&readfds);
		FD_SET(perl_interpreter_filehandle_out, &readfds);
		timeout_struct.tv_sec = 0;
		timeout_struct.tv_usec = 0;
		 
#if ! defined (WIN32)
		/* Empty the output buffer and send it to the screen */
		flags = fcntl (perl_interpreter_filehandle_out, F_GETFL);
		/*???DB.  Wouldn't compile at home.  O_NDELAY is equivalent to
		  FNDELAY */
		/*					flags &= FNDELAY;*/
		flags &= O_NDELAY;
		flags &= O_NONBLOCK;
		fcntl (perl_interpreter_filehandle_out, F_SETFL, flags);
		/* fsync(perl_interpreter_filehandle_out); */
#endif /* ! defined (WIN32) */
		while (select(FD_SETSIZE, &readfds, NULL, NULL, &timeout_struct))
		{
			if (read_length = read(perl_interpreter_filehandle_out, (void *)buffer, sizeof(buffer) - 1))
			{
				buffer[read_length] = 0;
				(*display_message_function)(INFORMATION_MESSAGE,
					"%s", buffer) ;				
			}
		}
	}
	return (return_code);
} /* handle_output */
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
int cmiss_perl_callback(char *command_string)
/*******************************************************************************
LAST MODIFIED : 19 May 2000

DESCRIPTION:
Takes a <command_string>, processes this through the F90 interpreter
and then executes the returned strings
==============================================================================*/
{
	int quit, return_code;

	if (command_string)
	{
		/* Change STDOUT and STDERR back to the standard pipes */
		if (keep_stdout)
		{
			dup2(keep_stdout, STDOUT_FILENO);
		}
		if (keep_stderr)
		{
			dup2(keep_stderr, STDERR_FILENO);
		}

#if defined (CMISS_DEBUG)
		printf("cmiss_perl_callback: %s\n", command_string);
#endif /* defined (CMISS_DEBUG) */

		handle_output();

		quit = perl_interpreter_kept_quit;

		kept_execute_command_function(command_string, perl_interpreter_kept_user_data,
		  &quit, &return_code);

		perl_interpreter_kept_quit = quit;

#if defined (CMISS_DEBUG)
		printf("cmiss_perl_callback code: %d (%s)\n", return_code, command_string);
#endif /* defined (CMISS_DEBUG) */

		/* Put the redirection back on again */
		if (perl_interpreter_filehandle_in)
		{
			dup2(perl_interpreter_filehandle_in, STDOUT_FILENO);
			dup2(perl_interpreter_filehandle_in, STDERR_FILENO);
		}
	}
	else
	{
		(*display_message_function)(ERROR_MESSAGE,"cmiss_perl_callback.  "
			 "Missing command_data");
		return_code=0;
	}

	return (return_code);
} /* cmiss_perl_callback */
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void interpret_command_(char *command_string, void *user_data, int *quit,
  execute_command_function_type execute_command_function, int *status)
/*******************************************************************************
LAST MODIFIED : 19 May 2000

DESCRIPTION:
Takes a <command_string>, processes this through the Perl interpreter.
==============================================================================*/
{
	char *escaped_command, *new_pointer, *old_pointer, *quote_pointer,
		*slash_pointer, *wrapped_command;
	int return_code;
	STRLEN n_a;
	dSP ;
 
	if (my_perl)
	{
		 ENTER ;
		 SAVETMPS;

		 if (command_string)
		 {
				PUSHMARK(sp) ;
				perl_interpreter_kept_user_data = user_data;
				perl_interpreter_kept_quit = *quit;

				kept_execute_command_function = execute_command_function;

				return_code = 1;

				if (wrapped_command = (char *)malloc(strlen(command_string) + 100))
				{
					 /* Escape any 's in the string */
					 if ((quote_pointer = strchr (command_string, '\'')) ||
							(slash_pointer = strchr (command_string, '\\')))
					 {
							if (escaped_command = (char *)malloc(strlen(command_string) + 100))
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
								 (*display_message_function)(ERROR_MESSAGE,"cmiss_perl_execute_command.  "
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

					 if (perl_interpreter_filehandle_in)
					 {
							/* Redirect STDOUT and STDERR */
							dup2(perl_interpreter_filehandle_in, STDOUT_FILENO);
							dup2(perl_interpreter_filehandle_in, STDERR_FILENO);
					 }
				
					 perl_eval_pv(wrapped_command, FALSE);

					 handle_output();

					 if (SvTRUE(ERRSV))
					 {
							(*display_message_function)(ERROR_MESSAGE,
								 "%s", SvPV(ERRSV, n_a)) ;
							POPs ;
							return_code = 0;
					 }

					 /* Change STDOUT and STDERR back again */
					 if (keep_stdout)
					 {
							dup2(keep_stdout, STDOUT_FILENO);
					 }
					 if (keep_stderr)
					 {
							dup2(keep_stderr, STDERR_FILENO);
					 }

					 *quit = perl_interpreter_kept_quit;
 
					 /*  This command needs to get the correct response from a 
							 partially complete command before it is useful
							 if (!SvTRUE(cvrv))
							 {
							 (*display_message_function)(ERROR_MESSAGE,
							 "Unable to compile command: %s\n", wrapped_command) ;
							 POPs ;
							 }*/

					 free (wrapped_command);
				}
				else
				{
					 (*display_message_function)(ERROR_MESSAGE,"interpret_command.  "
							"Unable to allocate wrapped_string");
					 return_code=0;
				}
		 }
		 else
		 {
				(*display_message_function)(ERROR_MESSAGE,"interpret_command.  "
					 "Missing command_data");
				return_code=0;
		 }

		 FREETMPS ;
		 LEAVE ;	

	}
	else
	{
		 (*display_message_function)(ERROR_MESSAGE,"interpret_command.  "
				"Missing interpreter");
		 return_code=0;
	}
	
	*status = return_code;
} /* interpret_command_ */
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void interpreter_evaluate_integer_(char *expression, int *result, int *status)
/*******************************************************************************
LAST MODIFIED : 6 September 2000

DESCRIPTION:
Use the perl_interpreter to evaluate the given string <expression> and return 
its value as an integer <result>.  If the string <expression> does not evaluate
as an integer then <status> will be set to zero.
==============================================================================*/
{
	int return_code;

	STRLEN n_a;
	dSP ;
	SV *sv_result;

	return_code = 1;

	if (my_perl)
	{
		 ENTER ;
		 SAVETMPS;

		 if (expression && result && status)
		 {
				if (perl_interpreter_filehandle_in)
				{
					 /* Redirect STDOUT and STDERR */
					 dup2(perl_interpreter_filehandle_in, STDOUT_FILENO);
					 dup2(perl_interpreter_filehandle_in, STDERR_FILENO);
				}
				
				sv_result = perl_eval_pv(expression, FALSE);

				/* Change STDOUT and STDERR back again */
				if (keep_stdout)
				{
					 dup2(keep_stdout, STDOUT_FILENO);
				}
				if (keep_stderr)
				{
					 dup2(keep_stderr, STDERR_FILENO);
				}
 
				handle_output();

				if (SvTRUE(ERRSV))
				{
					 (*display_message_function)(ERROR_MESSAGE,
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
							(*display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_integer.  "
								 "String \"%s\" does not evaluate to an integer.", expression);
							return_code = 0;
					 }
				}
		 }
		 else
		 {
				(*display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_integer.  "
					 "Invalid arguments.") ;
				return_code = 0;
		 }
		 
		 FREETMPS ;
		 LEAVE ;
	}
	else
	{
		 (*display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_integer.  "
				"Missing interpreter");
		 return_code=0;
	}

	*status = return_code;
} /* interpreter_evaluate_integer_ */
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void interpreter_set_integer_(char *variable_name, int *value, int *status)
/*******************************************************************************
LAST MODIFIED : 6 September 2000

DESCRIPTION:
Sets the value of the scalar variable cmiss::<variable_name> to be <value>.
==============================================================================*/
{
	int return_code;

	SV *sv_variable;

	return_code = 1;

	if (my_perl)
	{
		 ENTER ;
		 SAVETMPS;

		 if (variable_name && value && status)
		 {
				sv_variable = perl_get_sv(variable_name, TRUE);
				sv_setiv(sv_variable, *value);
		 }
		 else
		 {
				(*display_message_function)(ERROR_MESSAGE,"interpreter_set_integer.  "
					 "Invalid arguments.") ;
				return_code = 0;
		 }

		 FREETMPS ;
		 LEAVE ;
 	}
	else
	{
		 (*display_message_function)(ERROR_MESSAGE,"interpreter_set_integer.  "
				"Missing interpreter");
		 return_code=0;
	}
 
	*status = return_code;
} /* interpreter_set_integer_ */
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void interpreter_evaluate_double_(char *expression, double *result, int *status)
/*******************************************************************************
LAST MODIFIED : 6 September 2000

DESCRIPTION:
Use the perl_interpreter to evaluate the given string <expression> and return 
its value as an double <result>.  If the string <expression> does not evaluate
as an double then <status> will be set to zero.
==============================================================================*/
{
	int return_code;

	STRLEN n_a;
	dSP ;
	SV *sv_result;
 
	return_code = 1;

	if (my_perl)
	{
		 ENTER ;
		 SAVETMPS;

		 if (expression && result && status)
		 {
				if (perl_interpreter_filehandle_in)
				{
					 /* Redirect STDOUT and STDERR */
					 dup2(perl_interpreter_filehandle_in, STDOUT_FILENO);
					 dup2(perl_interpreter_filehandle_in, STDERR_FILENO);
				}
				
				sv_result = perl_eval_pv(expression, FALSE);

				/* Change STDOUT and STDERR back again */
				if (keep_stdout)
				{
					 dup2(keep_stdout, STDOUT_FILENO);
				}
				if (keep_stderr)
				{
					 dup2(keep_stderr, STDERR_FILENO);
				}
 
				handle_output();

				if (SvTRUE(ERRSV))
				{
					 (*display_message_function)(ERROR_MESSAGE,
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
							(*display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_double.  "
								 "String \"%s\" does not evaluate to a double.", expression);
							return_code = 0;
					 }
				}
		 }
		 else
		 {
				(*display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_double.  "
					 "Invalid arguments.") ;
				return_code = 0;
		 }

		 FREETMPS ;
		 LEAVE ;
 	}
	else
	{
		 (*display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_double.  "
				"Missing interpreter");
		 return_code=0;
	}

	*status = return_code;
} /* interpreter_evaluate_double_ */
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void interpreter_set_double_(char *variable_name, double *value, int *status)
/*******************************************************************************
LAST MODIFIED : 6 September 2000

DESCRIPTION:
Sets the value of the scalar variable cmiss::<variable_name> to be <value>.
==============================================================================*/
{
	int return_code;

	SV *sv_variable;

	return_code = 1;

	if (my_perl)
	{
		 ENTER ;
		 SAVETMPS;

		 if (variable_name && value && status)
		 {
				sv_variable = perl_get_sv(variable_name, TRUE);
				sv_setnv(sv_variable, *value);
		 }
		 else
		 {
				(*display_message_function)(ERROR_MESSAGE,"interpreter_set_double.  "
					 "Invalid arguments.") ;
				return_code = 0;
		 }
 
		 FREETMPS ;
		 LEAVE ;
 	}
	else
	{
		 (*display_message_function)(ERROR_MESSAGE,"interpreter_set_double.  "
				"Missing interpreter");
		 return_code=0;
	}

	*status = return_code;
} /* interpreter_set_double_ */
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void interpreter_evaluate_string_(char *expression, char **result, int *status)
/*******************************************************************************
LAST MODIFIED : 7 September 2000

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

	STRLEN n_a, string_length;
	dSP ;
	SV *sv_result;

	return_code = 1;

	*result = (char *)NULL;
	if (my_perl)
	{ 
		 ENTER ;
		 SAVETMPS;

		 if (expression && result && status)
		 {
				if (perl_interpreter_filehandle_in)
				{
					 /* Redirect STDOUT and STDERR */
					 dup2(perl_interpreter_filehandle_in, STDOUT_FILENO);
					 dup2(perl_interpreter_filehandle_in, STDERR_FILENO);
				}
				
				sv_result = perl_eval_pv(expression, FALSE);

				/* Change STDOUT and STDERR back again */
				if (keep_stdout)
				{
					 dup2(keep_stdout, STDOUT_FILENO);
				}
				if (keep_stderr)
				{
					 dup2(keep_stderr, STDERR_FILENO);
				}
 
				handle_output();

				if (SvTRUE(ERRSV))
				{
					 (*display_message_function)(ERROR_MESSAGE,
							"%s", SvPV(ERRSV, n_a)) ;
					 POPs ;
					 return_code = 0;
				}
				else
				{
					 if (SvPOK(sv_result))
					 {
							internal_string = SvPV(sv_result, string_length);
							if (*result = interpreter_duplicate_string(internal_string, string_length))
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
							(*display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_string.  "
								 "String \"%s\" does not evaluate to a string.", expression);
							return_code = 0;
					 }
				}
		 }
		 else
		 {
				(*display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_string.  "
					 "Invalid arguments.") ;
				return_code = 0;
		 }
		 
		 FREETMPS ;
		 LEAVE ;
 	}
	else
	{
		 (*display_message_function)(ERROR_MESSAGE,"interpreter_evaluate_string.  "
				"Missing interpreter");
		 return_code=0;
	}

	*status = return_code;
} /* interpreter_evaluate_string_ */
#endif /* ! defined (NO_STATIC_FALLBACK) */

#if ! defined (NO_STATIC_FALLBACK)
void interpreter_set_string_(char *variable_name, char *value, int *status)
/*******************************************************************************
LAST MODIFIED : 7 September 2000

DESCRIPTION:
Sets the value of the scalar variable cmiss::<variable_name> to be <value>.
==============================================================================*/
{
	int return_code;

	SV *sv_variable;

	return_code = 1;

	if (my_perl)
	{
		 ENTER ;
		 SAVETMPS;

		 if (variable_name && value && status)
		 {
				sv_variable = perl_get_sv(variable_name, TRUE);
				sv_setpv(sv_variable, value);
		 }
		 else
		 {
				(*display_message_function)(ERROR_MESSAGE,"interpreter_set_string.  "
					 "Invalid arguments.") ;
				return_code = 0;
		 }

		 FREETMPS ;
		 LEAVE ;
 	}
	else
	{
		 (*display_message_function)(ERROR_MESSAGE,"interpreter_set_string.  Missing interpreter");
		 return_code=0;
	}

	*status = return_code;
} /* interpreter_set_string_ */
#endif /* ! defined (NO_STATIC_FALLBACK) */

/*
	Local Variables: 
	tab-width: 2
	End: 
*/
