/*******************************************************************************
FILE : perl_interpreter.c

LAST MODIFIED : 22 August 2000

DESCRIPTION :
Provides an interface between cmiss and a Perl interpreter.
==============================================================================*/

#include <EXTERN.h>               /* from the Perl distribution     */
#include <perl.h>                 /* from the Perl distribution     */
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#if defined (CMGUI)
#include "command/cmiss.h"
#include "user_interface/message.h"
#include "command/perl_interpreter.h"
#else /* defined (CMGUI) */
#include "perl_interpreter.h"
#endif /* defined (CMGUI) */

static PerlInterpreter *perl_interpreter;  /***    The Perl interpreter    ***/

static void xs_init _((void));

void boot_DynaLoader _((CV* cv));
void boot_Perl_cmiss _((CV* cv));
static int perl_interpreter_filehandle = 0;
static int perl_interpreter_kept_quit;
static void *perl_interpreter_kept_user_data; 
static execute_command_function_type kept_execute_command_function;

void xs_init()
{
	char *file = __FILE__;
	newXS("Perl_cmiss::bootstrap", boot_Perl_cmiss, file);
	/* DynaLoader is a special case */
	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
}

void create_interpreter_(int *status)
/*******************************************************************************
LAST MODIFIED : 22 August 2000

DESCRIPTION:
Takes a <command_string>, processes this through the Perl interpreter
and then executes the returned strings.  If <*redirect_output> is true then
the output from the perl_interpreter will be redirected from the shell to
the display_message routine.
==============================================================================*/
{
  char *embedding[] = { "", "-e", "0" };
  char perl_start_code[] = 
	 "local $SIG{__WARN__} = sub { die $_[0] };\n"
	 "BEGIN {\n"
#include "strict.pmh"
	 ";\n"
	 "import strict \"subs\";\n"
	 "}\n"
#include "Balanced.pmh"
#include "Perl_cmiss.pmh"
    ;

  char *load_commands[] =
  { "import cmiss",
	 "Perl_cmiss::register_keyword assign",
	 "Perl_cmiss::register_keyword cell",
	 "Perl_cmiss::register_keyword command_window",
	 "Perl_cmiss::register_keyword create",
	 "Perl_cmiss::register_keyword define",
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

  perl_interpreter = perl_alloc();
  perl_construct(perl_interpreter);
  perl_parse(perl_interpreter, xs_init, 3, embedding, NULL);
  perl_run(perl_interpreter);
  
  {
	 STRLEN n_a;
	 dSP ;
	  
	 ENTER ;
	 SAVETMPS;

	 PUSHMARK(sp) ;
	 perl_eval_pv(perl_start_code, FALSE);
	 if (SvTRUE(ERRSV))
		{
		  display_message(ERROR_MESSAGE,"initialise_interpreter.  "
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
				display_message(ERROR_MESSAGE,"initialise_interpreter.  "
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

void destroy_interpreter_(int *status)
/*******************************************************************************
LAST MODIFIED : 8 June 2000

DESCRIPTION:
Takes a <command_string>, processes this through the F90 interpreter
and then executes the returned strings
==============================================================================*/
{
	perl_destruct(perl_interpreter);
	perl_free(perl_interpreter);

	*status = 1;
}

void redirect_interpreter_output_(int *status)
/*******************************************************************************
LAST MODIFIED : 25 August 2000

DESCRIPTION:
This redirects the output from stdout to a pipe so that the handle_output
routine can write this to the command window.
==============================================================================*/
{
  int filehandles[2], return_code;
  IV perl_filehandle;

  return_code = 1;

  if (0 == pipe (filehandles))
  {
	 dSP ;
	 
	 perl_interpreter_filehandle = filehandles[0];
	  
	 ENTER ;
	 SAVETMPS;

	 PUSHMARK(sp) ;
	 perl_filehandle = filehandles[1];
	 XPUSHs(sv_2mortal(newSViv(perl_filehandle)));
	 PUTBACK;
	 perl_call_pv("Perl_cmiss::remap_descriptor", G_DISCARD);
	 
	 FREETMPS ;
	 LEAVE ;

  }
  else
  {
	  display_message(ERROR_MESSAGE,"redirect_interpreter_output.  "
		  "Unable to create pipes") ;
	  return_code = 0;
  }

  *status = return_code;
}

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
	int flags, read_length, return_code;
	struct timeval timeout_struct;
	
	return_code = 1;

	if (perl_interpreter_filehandle)
	  {
		 FD_ZERO(&readfds);
		 FD_SET(perl_interpreter_filehandle, &readfds);
		 timeout_struct.tv_sec = 0;
		 timeout_struct.tv_usec = 0;
		 
		 /* Empty the output buffer and send it to the screen */
		 flags = fcntl (perl_interpreter_filehandle, F_GETFL);
		 /*???DB.  Wouldn't compile at home.  O_NDELAY is equivalent to
			FNDELAY */
		 /*					flags &= FNDELAY;*/
		 flags &= O_NDELAY;
		 flags &= O_NONBLOCK;
		 fcntl (perl_interpreter_filehandle, F_SETFL, flags);
		 while (select(FD_SETSIZE, &readfds, NULL, NULL, &timeout_struct))
			{
			  if (read_length = read(perl_interpreter_filehandle, (void *)buffer, sizeof(buffer) - 1))
				 {
					buffer[read_length] = 0;
					display_message(INFORMATION_MESSAGE,
					  "%s", buffer) ;				
				 }
			}
	  }
	return (return_code);
} /* handle_output */

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
#if defined (CMISS_DEBUG)
		printf("cmiss_perl_callback: %s\n", command_string);
#endif /* defined (CMISS_DEBUG) */

		handle_output();

		kept_execute_command_function(command_string, perl_interpreter_kept_user_data,
		  &quit, &return_code);

		perl_interpreter_kept_quit = quit;

#if defined (CMISS_DEBUG)
		printf("cmiss_perl_callback code: %d (%s)\n", return_code, command_string);
#endif /* defined (CMISS_DEBUG) */
	}
	else
	{
		printf("cmiss_perl_callback.  Missing command_data");
		return_code=0;
	}

	return (return_code);
} /* cmiss_perl_callback */

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
	SV *cvrv;
	dSP ;
 
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
				}
				else
				{
					printf("cmiss_perl_execute_command.  Unable to allocate escaped_string");
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
			cvrv = perl_eval_pv(wrapped_command, FALSE);

			*quit = perl_interpreter_kept_quit;
 
			handle_output();

			if (SvTRUE(ERRSV))
			{
				display_message(ERROR_MESSAGE,
				  "%s", SvPV(ERRSV, n_a)) ;
				POPs ;
				return_code = 0;
			}

			/*  This command needs to get the correct response from a 
				 partially complete command before it is useful
			if (!SvTRUE(cvrv))
			{
				display_message(ERROR_MESSAGE,
				  "Unable to compile command: %s\n", wrapped_command) ;
				POPs ;
			}*/

			free (wrapped_command);
		}
		else
		{
			printf("cmiss_perl_execute_command.  Unable to allocate wrapped_string");
			return_code=0;
		}
	}
	else
	{
		printf("cmiss_perl_execute_command.  Missing command_data");
		return_code=0;
	}

	FREETMPS ;
	LEAVE ;

	*status = return_code;
} /* interpret_command_ */
