/*******************************************************************************
FILE : perl_interpreter.h

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

struct Interpreter;

typedef void (*execute_command_function_type)(char *, void *, int *, int *);

void interpret_command_(struct Interpreter *interpreter, 
	char *command_string, void *user_data, int *quit,
	execute_command_function_type execute_command_function, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpret_command interpret_command_
#endif /* defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
==============================================================================*/

void create_interpreter_(int argc, char **argv, const char *initial_comfile,
	struct Interpreter **interpreter, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define create_interpreter create_interpreter_
#endif /* defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Creates the interpreter for processing commands.

<argc>, <argv> and <initial_comfile> are used to initialise some internal
variables.

If <*warnings_flag> is true then perl is started with its -w option on..
==============================================================================*/

void destroy_interpreter_(struct Interpreter *interpreter, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define destroy_interpreter destroy_interpreter_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Takes a <command_string>, processes this through the F90 interpreter
and then executes the returned strings
==============================================================================*/

void redirect_interpreter_output_(struct Interpreter *interpreter,
	int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define redirect_interpreter_output redirect_interpreter_output_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
This redirects the output from stdout to a pipe so that the handle_output
routine can write this to the command window.
==============================================================================*/

void interpreter_evaluate_integer_(struct Interpreter *interpreter, 
	char *expression, int *result, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_evaluate_integer interpreter_evaluate_integer_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Use the perl_interpreter to evaluate the given string <expression> and return 
its value as an integer <result>.  If the string <expression> does not evaluate
as an integer then <status> will be set to zero.
==============================================================================*/

void interpreter_set_integer_(struct Interpreter *interpreter, 
	char *variable_name, int *value, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_set_integer interpreter_set_integer_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Sets the value of the scalar variable cmiss::<variable_name> to be <value>.
To override the cmiss:: package specify the full name in the string.
==============================================================================*/

void interpreter_evaluate_double_(struct Interpreter *interpreter, 
	char *expression, double *result, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_evaluate_double interpreter_evaluate_double_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Use the perl_interpreter to evaluate the given string <expression> and return 
its value as an double <result>.  If the string <expression> does not evaluate
as an double then <status> will be set to zero.
==============================================================================*/

void interpreter_set_double_(struct Interpreter *interpreter, 
	char *variable_name, double *value, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_set_double interpreter_set_double_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Sets the value of the scalar variable cmiss::<variable_name> to be <value>.
To override the cmiss:: package specify the full name in the string.
==============================================================================*/

void interpreter_evaluate_string_(struct Interpreter *interpreter, 
	char *expression, char **result, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_evaluate_string interpreter_evaluate_string_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Use the perl_interpreter to evaluate the given string <expression> and return 
its value as an string in <result>.  If the string <expression> does not evaluate
as an string then <status> will be set to zero.
==============================================================================*/

void interpreter_destroy_string_(struct Interpreter *interpreter, char *string);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_destroy_string interpreter_destroy_string_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION :
Frees the memory associated with a string allocated by the interpreter.
==============================================================================*/

void interpreter_set_string_(struct Interpreter *interpreter, 
	char *variable_name, char *value, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_set_string interpreter_set_string_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Sets the value of the scalar variable cmiss::<variable_name> to be <value>.
To override the cmiss: package specify the full name in the string.
==============================================================================*/

void interpreter_set_pointer_(struct Interpreter *interpreter, 
	char *variable_name, char *class_name, void *value,int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_set_pointer interpreter_set_pointer_
#endif /* ! defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Sets the value of the scalar variable cmiss::<variable_name> to be <value> and 
sets the class of that variable to be <class_name>.
To override the cmiss:: package specify the full name in the string.
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
#endif /* ! defined (MESSAGE_H) */

typedef int (Interpreter_display_message_function)(enum Message_type message_type,
	char *format, ... );

void interpreter_set_display_message_function_(struct Interpreter *interpreter,
	Interpreter_display_message_function *function, int *status);
#if ! defined (FORTRAN_INTERPRETER_INTERFACE)
#define interpreter_set_display_message_function interpreter_set_display_message_function_
#endif /* defined (FORTRAN_INTERPRETER_INTERFACE) */
/*******************************************************************************
LAST MODIFIED : 24 January 2005

DESCRIPTION:
Sets the function that will be called whenever the Interpreter wants to report
information.
==============================================================================*/

