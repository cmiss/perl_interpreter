/*******************************************************************************
FILE : perl_interpreter.h

LAST MODIFIED : 22 August 2000

DESCRIPTION :
Provides an interface between cmiss and a Perl interpreter.
==============================================================================*/

typedef void (*execute_command_function_type)(char *, void *, int *, int *);

void interpret_command_(char *command_string, void *user_data, int *quit,
  execute_command_function_type execute_command_function, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpret_command interpret_command_
#endif /* defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 19 May 2000

DESCRIPTION:
==============================================================================*/


void create_interpreter_(int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define create_interpreter create_interpreter_
#endif /* defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 22 August 2000

DESCRIPTION:
Takes a <command_string>, processes this through the Perl interpreter
and then executes the returned strings.  If <*redirect_output> is true then
the output from the perl_interpreter will be redirected from the shell to
the display_message routine.
==============================================================================*/

void destroy_interpreter_(int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define destroy_interpreter destroy_interpreter_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 19 May 2000

DESCRIPTION:
Takes a <command_string>, processes this through the F90 interpreter
and then executes the returned strings
==============================================================================*/

void redirect_interpreter_output_(int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define redirect_output redirect_output_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 25 August 2000

DESCRIPTION:
This redirects the output from stdout to a pipe so that the handle_output
routine can write this to the command window.
==============================================================================*/

/*
From message.h:
===============
*/

/*
Global types
------------
*/
#if ! defined (CMGUI)
enum Message_type
/*******************************************************************************
LAST MODIFIED : 31 May 1996

DESCRIPTION :
The different message types.
==============================================================================*/
{
	ERROR_MESSAGE,
	INFORMATION_MESSAGE,
	WARNING_MESSAGE
}; /* enum Message_type */
#endif /* ! defined (CMGUI) */

/*
Functions called from perl_interpreter.c
========================================
*/

int display_message(enum Message_type message_type,char *format, ... );
