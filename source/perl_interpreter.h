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


#if defined (FORTRAN_INTERPRETER_INTERFACE)
void create_interpreter_(int *error);
#else /* defined (FORTRAN_INTERPRETER_INTERFACE) */
int initialise_interpreter(void);
#endif /* defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 19 May 2000

DESCRIPTION:
Takes a <command_string>, processes this through the F90 interpreter
and then executes the returned strings
==============================================================================*/

#if defined (FORTRAN_INTERPRETER_INTERFACE)
void destroy_interpreter_(int *error);
#else /* defined (FORTRAN_INTERPRETER_INTERFACE) */
int destroy_interpreter(void);
#endif /* defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 19 May 2000

DESCRIPTION:
Takes a <command_string>, processes this through the F90 interpreter
and then executes the returned strings
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
