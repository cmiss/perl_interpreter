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
LAST MODIFIED : 9 June 2000

DESCRIPTION:
Takes a <command_string>, processes this through the F90 interpreter
and then executes the returned strings
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
	 "my $filedescriptor = shift;\n"
	 /* Redirect all STDOUT and STDERR to a pipe */
/*  	 "open (STDOUT, \">&=4\") || die \"can't open fd 4: $!\";\n" */
	 "$| = 1;\n"
	 "package Perl_cmiss;\n"

	 "$VERSION = '0.01';\n"
	 "bootstrap Perl_cmiss $VERSION;\n"

	 "# Preloaded methods go here.\n"

	 "#Using a hash so that the strategy for action could be placed with\n"
	 "#the word.  For now only one action.\n"
	 "my %keywords;\n"
	 "my @command_list = ();\n"
	 "my $block_count = 0;\n"
	 "my $block_required = 0;\n"

 	 "sub register_keyword\n"
	 "  {\n"
	 "	 my $word = shift;\n"

	 "#	 print \"register $word\\n\";\n"

	 "	 $keywords{$word} = 1;\n"
	 "  }\n"

	 "sub execute_command\n"
	 "  {\n"
	 "	 my $command = shift;\n"
	 "  my $command2 = $command;\n"
	 "  $command2 =~ s%'%\\\\'%g;\n"
	 "  $command2 = \"print '>  $command2' . \\\"\\\\n\\\";\";\n"
	 "	 my $token;\n"
	 "	 my $lc_token;\n"
	 "	 my $match_string = join (\"|\", keys %keywords);\n"
#if defined (OLD_CODE)
	 "	 my @tokens = &parse_line('\\s*[\\{\\}\\(\\)]\\s*', \"delimiters\", $command);\n"
	 "	 my @tokens; push (@tokens, $command);\n"
#endif /* defined (OLD_CODE) */
	 "  my @tokens = ();\n"
	 "  my $part_token = \"\";\n"
	 "  my $extracted;\n"
	 "  my $lc_command;\n"
	 "  my $continue;\n"
	 "  my $reduced_command;\n"
	 /* #define CMISS_DEBUG_EXTRACT */
	 "  while ($command ne \"\")\n"
	 "  {\n"
#if defined (CMISS_DEBUG_EXTRACT)
	 "    print \"$command   \";\n"
#endif /* defined (CMISS_DEBUG_EXTRACT) */
	 "    if ($command =~ s%^(\\s+)%%)\n"
	 "    {\n"
#if defined (CMISS_DEBUG_EXTRACT)
	 "       print \"space: $1\\n\";\n"
#endif /* defined (CMISS_DEBUG_EXTRACT) */
	 "       $part_token = $part_token . $1;\n"
	 "    }\n"
	 "    elsif ($command =~ s%^({)%%)\n"
	 "    {\n"
#if defined (CMISS_DEBUG_EXTRACT)
	 "       print \"open bracket: $1\\n\";\n"
#endif /* defined (CMISS_DEBUG_EXTRACT) */
	 "       if ($part_token ne \"\")\n"
	 "       {\n"
	 "          push(@tokens, $part_token);\n"
	 "       }\n"
	 "       $part_token = \"\";\n"
	 "       push(@tokens, $1);\n"
	 "    }\n"
	 "    elsif ($command =~ s%^(#.*)%%)\n"
	 "    {\n"
#if defined (CMISS_DEBUG_EXTRACT)
	 "       print \"comment: $1\\n\";\n"
#endif /* defined (CMISS_DEBUG_EXTRACT) */
	 "       if ($part_token ne \"\")\n"
	 "       {\n"
	 "          push(@tokens, $part_token);\n"
	 "       }\n"
	 "       $part_token = \"\";\n"
	 "    }\n"
	 "    elsif ($command =~ s%^(})%%)\n"
	 "    {\n"
#if defined (CMISS_DEBUG_EXTRACT)
	 "       print \"close bracket: $1\\n\";\n"
#endif /* defined (CMISS_DEBUG_EXTRACT) */
	 "       if ($part_token ne \"\")\n"
	 "       {\n"
	 "          push(@tokens, $part_token);\n"
	 "       }\n"
	 "       $part_token = \"\";\n"
	 "       push(@tokens, $1);\n"
	 "    }\n"
	 "    else\n"
    "    {\n"
	 "       $continue = 1;\n"
    "       if ($part_token eq \"\")\n"
	 "       {"
	 "          $lc_command = lc ($command);\n"
    "          if (($lc_command =~ m/^(?:$match_string)/) "
    "          || ($lc_command =~ m/^\\?$/))\n"
	 "          {\n"
	 "             while (($command ne \"\") && !($command =~ m/(^[}#])/))\n"
	 "             {\n"
	 "                ($extracted, $reduced_command) = "
	 "                   Text::Balanced::extract_variable($command);\n"
	 "                if ($extracted)\n" 
	 "                {\n"
	 "                   $command = $reduced_command;\n"
	 "                   $part_token = $part_token . $extracted;\n"
	 "                }\n"
	 "                else\n"
	 "                {\n"
	 "                   ($extracted, $reduced_command) = "
	 "                      Text::Balanced::extract_delimited($command, q{'\"`});\n"
	 "                   if ($extracted)\n" 
	 "                   {\n"
	 "                      $command = $reduced_command;\n"
	 "                      $part_token = $part_token . $extracted;\n"
	 "                   }\n"
	 "                   else\n"
	 "                   {\n"
	 "                      $part_token = $part_token . substr($command, 0, 1);\n"
	 "                      $command = substr($command, 1);\n"
	 "                   }\n"
	 "                }\n"
	 "             }\n"
#if defined (CMISS_DEBUG_EXTRACT)
	 "             print \"cmiss: $part_token\\n\";\n"
#endif /* defined (CMISS_DEBUG_EXTRACT) */
	 "             push(@tokens, $part_token);\n"
	 "             $part_token = \"\";\n"
	 "             $continue = 0;\n"
	 "          }\n"
    "          if ($lc_command =~ m/^(?:assert)/)\n"
	 "          {\n"
	 "             $command =~ s/^([^}#]*)//;\n"
#if defined (CMISS_DEBUG_EXTRACT)
	 "             print \"assert: $1\\n\";\n"
#endif /* defined (CMISS_DEBUG_EXTRACT) */
	 "             push(@tokens, $1);\n"
	 "             $continue = 0;\n"
	 "          }\n"
	 "       }\n"
	 "       if ($continue)\n"
	 "       {\n"

	 "       ($extracted, $reduced_command) = "
	 "          Text::Balanced::extract_variable($command);\n"
    "       if ($extracted)\n"
	 "       {\n"
	 "          $command = $reduced_command;\n"
#if defined (CMISS_DEBUG_EXTRACT)
	 "          print \"variable: $extracted\\n\";\n"
#endif /* defined (CMISS_DEBUG_EXTRACT) */
	 "          $part_token = $part_token . $extracted;\n"
	 "       }\n"
	 "       else\n"
	 "       {\n"
	 "          ($extracted, $reduced_command) = "
	 "             Text::Balanced::extract_quotelike($command);\n"
	 "          if ($extracted)\n"
	 "          {\n"
	 "             $command = $reduced_command;\n"
#if defined (CMISS_DEBUG_EXTRACT)
	 "             print \"quotelike: $extracted\\n\";\n"
#endif /* defined (CMISS_DEBUG_EXTRACT) */
	 "             $part_token = $part_token . $extracted;\n"
	 "          }\n"
	 "          else\n"
	 "          {\n"
#if defined (CMISS_DEBUG_EXTRACT)
	 "             print \"character: \" . substr($command, 0, 1) . \"\\n\";\n"
#endif /* defined (CMISS_DEBUG_EXTRACT) */
	 "             $part_token = $part_token . substr($command, 0, 1);\n"
	 "             $command = substr($command, 1);\n"
	 "          }\n"
	 "       }\n"
	 "       }\n"
	 "    }\n"
	 "  }\n"
	 "  if ($part_token ne \"\")\n"
	 "  {\n"
	 "     push(@tokens, $part_token);\n"
	 "  }\n"


	 "  my $print_command_after = 0;\n"

	 "	 for ($j = 0 ; $j <= $#tokens ; $j++)\n"
	 "		{\n"
	 "      $token = $tokens[$j];"
	 "		  $lc_token = lc ($token);\n"
#if defined (CMISS_DEBUG)
	 "		  print \">  $token\\n\";\n"
#endif /* defined (CMISS_DEBUG) */
	 "		  if ($lc_token =~ m/^ass\\w+\\sblo\\w+\\sclos\\w+/)\n"
	 "			 {\n"
	 "           if ($block_required || $block_count)\n"
	 "              {\n"
	 "                 $block_required = 0;\n"
	 "                 $block_count = 0;\n"
	 "                 @command_list = ();\n"
	 "                 die (\"assert blocks closed failed\");\n\n"
	 "              }\n"
	 "           $token = \"\";\n"
	 "        }\n"
	 "		  elsif ($lc_token =~ m/^\\s*(?:$match_string)/ || $lc_token =~ m/^\\s*\\?$/)\n"
	 "			 {\n"
	 "          $token =~ s/\\\"/\\\\\"/g;\n"
	 "          my $token2 = $token;\n"
	 "          my @subtokens = split(\" \", $token);\n"
	 "          my $subtoken;\n"
	 "          for $subtoken (@subtokens)\n"
	 "             {\n"
	 "                if (!(($subtoken =~ m%^\\w+$%) || ($subtoken =~ m%^[\\d.]+$%)))\n"
	 "                  {\n"
#if defined (OLD_CODE)
	 "                     print \"subtoken ^$subtoken^\n\";\n"
#endif /* defined (OLD_CODE) */
	 "                     $subtoken =~ s`^((?:(?:\\w*\\()|(?:\\$\\w*)|(?:\\{[^\\}]*\\})|(?:@\\w*)|(?:%\\w*)|[/\\d<>\\+\\-\\*\\%!\\\\~\\|\\(\\)\\{\\}\\{\\}\\$\\^\\&\\.=%])+)$`\\\".(\\1).\\\"`;\n"
	 "                  }\n"
#if defined (OLD_CODE)
	 "                  {\n"
	 "                     print \"subtoken ^$subtoken^\n\";\n"
	 "                     $subtoken =~ s`^([/\\d<>\\+\\-\\*\\%!\\\\~\\|\\(\\)\\{\\}\\{\\}\\$\\^\\&\\.=%]+)$`\\\".(\\1).\\\"`;\n"
	 "                  }\n"
#endif /* defined (OLD_CODE) */
	 "             }\n"
	 "          $token = join (\" \", @subtokens)\n;"
#if defined (CMISS_DEBUG)
	 "				$token = \"(\\$return_code = Perl_cmiss::cmiss(\\\"$token\\\")) || die(\\\"Error in cmiss command \\$return_code\\\");\";\n"
#else /* defined (CMISS_DEBUG) */
	 "				$token = \"Perl_cmiss::cmiss(\\\"$token\\\") || die(\\\"Error in cmiss command $token2\\\");\";\n"
#endif /* defined (CMISS_DEBUG) */
	 "			 }\n"
	 "		  else\n"
	 "        {\n"
	 "           while ($token =~ m/(\\d+)\\s*\\.\\.\\s*(\\d+)\\s*:\\s*(\\d+)/g)\n"
	 "	        	   {\n"
	 "                my $remainder_start = $1 % $3;\n"
	 "                my $remainder_finish = ($2 - $remainder_start) % $3;\n"
	 "                my $list_start = ($1 - $remainder_start) / $3;\n"
	 "                my $list_finish = ($2 - $remainder_start - $remainder_finish)/ $3;\n"
	 "                my $new_list_operator = \"(map {\\$_ * $3 + $remainder_start} $list_start..$list_finish)\";\n"
	 "                $token =~ s/\\d+\\s*\\.\\.\\s*\\d+\\s*:\\s*\\d+/$new_list_operator/;\n"
	 "             }\n"
	 "           if ($token =~ m/^\\s*(?:if|while|unless|until|for|foreach|elsif|else|continue|sub)/)\n"
	 "			      {\n"
	 "			     	   $block_required = 1;\n"
	 "			      }\n"
	 "		       elsif ($token =~ m/^\\s*\\{/)\n"
	 "			      {\n"
	 "				     $block_required = 0;\n"
	 "				     $block_count++;\n"
	 "               $print_command_after = 1;\n"
	 "			      }\n"
	 "		       elsif ($token =~ m/^\\s*\\}/)\n"
	 "			      {\n"
	 "				     if ($block_count > 0)\n"
	 "				       {\n"
	 "					      $block_count--;\n"
	 "				       }\n"
	 "               $print_command_after = 0;\n"
	 "			      }\n"
	 "		       elsif (($j == $#tokens) && ($token) && (! ($token =~ m/;\\s*$/)))\n"
	 "            {\n"
	 "              $token = $token . \";\";\n"
	 "			     }\n"
	 "			 }\n"
	 "      $tokens[$j] = $token;\n"
	 "		}\n"
	 "	 $command = join (\"\", @tokens);\n"
#if defined (CMISS_DEBUG)
	 "	 print \"Perl_cmiss::execute_command parsed $command\\n\";\n"
	 "	 print \"Perl_cmiss::execute_command parsed $command2\\n\";\n"
#endif /* defined (CMISS_DEBUG) */
#if defined (NEW_CODE)
	 "  if (! $print_command_after)\n"
	 "    {\n"
	 "      push (@command_list, $command2);\n"
	 "    }\n"
#endif /* defined (NEW_CODE) */
	 "	 push (@command_list, $command);\n"
#if defined (NEW_CODE)
	 "  if ($print_command_after)\n"
	 "    {\n"
	 "      push (@command_list, $command2);\n"
	 "    }\n"
#endif /* defined (NEW_CODE) */

	 "#	 print \"$block_count $block_required\\n\";\n"

	 "	 if ((!($block_count))&&(!($block_required)))\n"
	 "		{\n"
	 "		  $command = join (\"\\n\", @command_list);\n"
	 "		  #Must reset this before the eval as it may call this function\n"
	 "		  #recursively before returning from this function\n"
	 "		  @command_list = ();\n"
	 /* Catch all warnings as errors */
	 "      local $SIG{__WARN__} = sub { die $_[0] };\n"
	 "		  eval ($command);\n"
	 "      if ($@)\n"
    "        {\n"
    "          die;\n"
    "        }\n"
	 "		  print \"\";\n"
	 "		}\n"
	 "  }\n";
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
  int i, filehandles[2], number_of_load_commands, return_code;
  IV perl_filehandle;

  return_code = 1;

  if (0 == pipe (filehandles))
  {
	  perl_interpreter = perl_alloc();
	  perl_construct(perl_interpreter);
	  perl_parse(perl_interpreter, xs_init, 3, embedding, NULL);
	  perl_run(perl_interpreter);

	  {
		  
		  dSP ;
	  
		  perl_interpreter_filehandle = filehandles[0];
	  
		  ENTER ;
		  SAVETMPS;

		  PUSHMARK(sp) ;
		  perl_filehandle = filehandles[1];
		  XPUSHs(sv_2mortal(newSViv(perl_filehandle)));
		  PUTBACK;
		  perl_eval_pv(perl_start_code, FALSE);
		  if (SvTRUE(GvSV(errgv)))
		  {
			  display_message(ERROR_MESSAGE,"initialise_interpreter.  "
				  "Uh oh - %s\n", SvPV(GvSV(errgv), na)) ;
			  POPs ;
			  return_code = 0;
		  }

		  number_of_load_commands = sizeof (load_commands) / sizeof (char *);
		  for (i = 0 ; i < number_of_load_commands && return_code ; i++)
		  {
			  perl_eval_pv(load_commands[i], FALSE);
			  if (SvTRUE(GvSV(errgv)))
			  {
				  display_message(ERROR_MESSAGE,"initialise_interpreter.  "
					  "Uh oh - %s\n", SvPV(GvSV(errgv), na)) ;
				  POPs ;
				  return_code = 0;
			  }
		  }

		  FREETMPS ;
		  LEAVE ;
	  }

  }
  else
  {
	  display_message(ERROR_MESSAGE,"initialise_interpreter.  "
		  "Unable to create pipes") ;
	  return_code = 0;
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

			/* Check the eval first */
			if (SvTRUE(GvSV(errgv)))
			{
				display_message(ERROR_MESSAGE,
				  "%s", SvPV(GvSV(errgv), na)) ;
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
