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


void create_interpreter_(int argc, char **argv, const char *initial_comfile, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define create_interpreter create_interpreter_
#endif /* defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 24 July 2001

DESCRIPTION:
Creates the interpreter for processing commands.
<argc>, <argv> and <initial_comfile> are used to initialise some internal variables.
If <*warnings_flag> is true then perl is started with its -w option on..
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
#define redirect_interpreter_output redirect_interpreter_output_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 25 August 2000

DESCRIPTION:
This redirects the output from stdout to a pipe so that the handle_output
routine can write this to the command window.
==============================================================================*/

void interpreter_evaluate_integer_(char *exprssion, int *result, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_evaluate_integer interpreter_evaluate_integer_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 6 September 2000

DESCRIPTION:
Use the perl_interpreter to evaluate the given string <expression> and return 
its value as an integer <result>.  If the string <expression> does not evaluate
as an integer then <status> will be set to zero.
==============================================================================*/

void interpreter_set_integer_(char *variable_name, int *value, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_set_integer interpreter_set_integer_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 6 September 2000

DESCRIPTION:
Sets the value of the scalar variable cmiss::<variable_name> to be <value>.
==============================================================================*/

void interpreter_evaluate_double_(char *expression, double *result, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_evaluate_double interpreter_evaluate_double_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 7 September 2000

DESCRIPTION:
Use the perl_interpreter to evaluate the given string <expression> and return 
its value as an double <result>.  If the string <expression> does not evaluate
as an double then <status> will be set to zero.
==============================================================================*/

void interpreter_set_double_(char *variable_name, double *value, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_set_double interpreter_set_double_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 7 September 2000

DESCRIPTION:
Sets the value of the scalar variable cmiss::<variable_name> to be <value>.
==============================================================================*/

void interpreter_evaluate_string_(char *expression, char **result, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_evaluate_string interpreter_evaluate_string_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 7 September 2000

DESCRIPTION:
Use the perl_interpreter to evaluate the given string <expression> and return 
its value as an string in <result>.  If the string <expression> does not evaluate
as an string then <status> will be set to zero.
==============================================================================*/

void interpreter_destroy_string_(char *string);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_destroy_string interpreter_destroy_string_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 7 September 2000

DESCRIPTION :
Frees the memory associated with a string allocated by the interpreter.
==============================================================================*/

void interpreter_set_string_(char *variable_name, char *value, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_set_string interpreter_set_string_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 7 September 2000

DESCRIPTION:
Sets the value of the scalar variable cmiss::<variable_name> to be <value>.
==============================================================================*/

#if ! defined (MESSAGE_H)
/*
From message.h:
===============
*/

/*
Global types
------------
*/

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
