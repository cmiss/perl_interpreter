#*******************************************************************************
#FILE : Perl_cmiss.pm
#
#LAST MODIFIED : 22 August 2000
#
#DESCRIPTION :
#With perl_interpreter.c provides an interface between cmiss and a 
#Perl interpreter.
#===============================================================================

package Perl_cmiss;

$VERSION = '0.01';
bootstrap Perl_cmiss $VERSION;

# Preloaded methods go here.

#Using a hash so that the strategy for action could be placed with
#the word.  For now only one action.
my %keywords;
my @command_list = ();
my $block_count = 0;
my $block_required = 0;
my $echo_commands = 0;
my $echo_prompt = "";
my $cmiss_debug = 0;

sub set_INC_for_platform
  {
    my ($abi_env) = @_;

    my $perlinc;

# see if an environment variable is set to override @INC.
    foreach my $varname ("CMISS${abi_env}_PERLINC","CMISS_PERLINC")
	  {
		if( exists $ENV{$varname} )
		  {
			$perlinc = $ENV{$varname};
			last;
		  }
      } 
    unless( $perlinc )
	  {
		# no environment variable is set for @INC.
		# run the local perl to get its @INC so we can use its modules
		my $perl = 'perl';
		# see if an environment variable specifies which local perl.
		foreach my $varname ("CMISS${abi_env}_PERL","CMISS_PERL")
		  {
			if( exists $ENV{$varname} )
			  {
				$perl = $ENV{$varname};
				last;
			  }
		  }
		# fork and catch STDOUT from the child.
		local *PERLOUT;
		unless( defined( my $pid = open PERLOUT, '-|' ) )
		  {
			print STDERR "$0: fork failed: $!\n";
		  }
		elsif( ! $pid ) #child
		  {
			exec $perl, '-e', 'print join ":", @INC' or exit $!;
		  }
		else # parent
		  { 
			my $perlout = <PERLOUT>;
			# check child completed successfully
			unless( close PERLOUT )
			  {
				$! = $? >> 8;
				print STDERR "$0: exec $perl failed: $!\n";
			  }
			else
			  {
				# perl has given us its include list.
				$perlinc = $perlout;
				# include $ENV{CMISS_ROOT}/perl_lib if exists
				if( exists $ENV{CMISS_ROOT} )
				  {
# GBS 6-March-2000  Directory changed
# 			  my $cmiss_perl_lib = $ENV{CMISS_ROOT}.'/perl_lib';
# 				my $cmiss_perl_lib = $ENV{CMISS_ROOT}.'/perl/lib';
					my $cmiss_perl_lib = $ENV{CMISS_ROOT}.'/cmiss_perl/lib';
					if( -d $cmiss_perl_lib )
					  {
						$perlinc = $cmiss_perl_lib.':'.$perlinc;
					  }
				  }
			  }
		  }
      }

    if( defined $perlinc ) { @INC = split /:/, $perlinc }
  }

sub register_keyword
  {
	 my $word = shift;
	 
	 #print \"register $word\\n\";
	 
	 $keywords{$word} = 1;
  }

sub call_command
  {
	 local $command = shift;
	 {
		package cmiss;
		*{cmiss::cmiss} = \&{Perl_cmiss::cmiss};
		# Catch all warnings as errors */
		local $SIG{__WARN__} = sub { die $_[0] };
		eval ($Perl_cmiss::command);
	 }
  }

sub cmiss_array
  {
	 my $command = "";
	 my $token2;
	 my $ref_type;
	 my $token;
	 my $subtoken;
	 my $first;
	 my $return_code;

	 for $token (@_)
		{
		  $ref_type = ref $token;
		  if ($token =~ /^[\s;]+$/)
			 {
				#This is just a delimiter
				$command = $command . $token;
			 }
		  elsif ("ARRAY" eq $ref_type)
			 {
				$first = 1;
				for $subtoken (@{$token})
				  {
					 if ($first)
						{
						  $first = 0;
						}
					 else
						{
						  $command = $command . ",";
						}
					 if ($subtoken =~ /[\s;]+/)
						{
						  $token2 = $subtoken;
						  #These delimiters need to be quoted and therefore the quotes and 
						  #escape characters contained within must be escaped.
						  $token2 =~ s/\\/\\\\/g;
						  $token2 =~ s/\"/\\\"/g;
						  $command = $command . "\"$token2\"";
						}
					 else
						{
						  $command = $command . $subtoken;
						}
				  }
			 }
		  elsif ($token =~ /[\s;]+/)
			 {
				$token2 = $token;
				#These delimiters need to be quoted and therefore the quotes and 
				#escape characters contained within must be escaped.
				$token2 =~ s/\\/\\\\/g;
				$token2 =~ s/\"/\\\"/g;
				$command = $command . "\"$token2\"";
			 }
		  else
			 {
				#This is just a plain word
				$command = $command . $token;
			 }
		}

	 if ($cmiss_debug)
		{
		  print "Perl_cmiss::cmiss_array final: $command\n";
		}
	 {
		package cmiss;
		$return_code = Perl_cmiss::cmiss($command);
	 }
	 if ($cmiss_debug)
		{
		  print "Perl_cmiss::cmiss_array cmiss return_code $return_code\n";
		}
	 return ($return_code);
  }

sub execute_command
  {
	 my $command = shift;
	 my $command2 = $command;
	 $command2 =~ s%'%\\'%g;
	 $command2 = "print '$echo_prompt$command2' . \"\\n\";";
	 my $token = "";
	 my $part_token;
	 my $token2;
	 my $lc_token;
	 my $match_string = join ("|", keys %keywords);
#	 my @tokens = &parse_line('\\s*[\\{\\}\\(\\)]\\s*', \"delimiters\", $command);
#	 my @tokens; push (@tokens, $command);
	 my @tokens = ();
	 my $extracted;
	 my $lc_command;
	 my $continue;
	 my $reduced_command;
	 my $print_command_after = 0;
	 my $is_perl_token;
	 my $simple_perl;

	 $simple_perl = 0;
	 while ($command ne "")
		{
		  $lc_command = lc ($command);
		  if ($cmiss_debug)
			 {
				print "$command   ";
			 }
		  if ($command =~ s%^(\s+)%%)
			 {
				if ($cmiss_debug)
				  {
					 print "space: $1\n";
				  }
				$token = $token . $1;
			 }
		  elsif ($command =~ s%^(#.*)%%)
			 {
			  if ($cmiss_debug)
			    {
					print "comment: $1\n";
				 }
			  if ($simple_perl && (!$block_required) && (! ($token =~ m/;\s*$/)))
			  {
				 $token = $token . ";";
			  }
			  if ($token ne "")
			  {
				 push(@tokens, $token);
			  }
			  $token = "";
			 }
		  else
			 {
				$simple_perl = 0;
				if ($command =~ s%^({)%%)
				  {
					 if ($cmiss_debug)
						{
						  print "open bracket: $1\n";
						}
					 if ($token ne "")
						{
						  push(@tokens, $token);
						}
					 $block_required = 0;
					 $block_count++;
					 $print_command_after = 1;
					 $token = "";
					 push(@tokens, $1);
				  }
				elsif ($command =~ s%^(})%%)
				  {
					 if ($cmiss_debug)
						{
						  print "close bracket: $1\n";
						}
					 if ($token ne "")
						{
						  push(@tokens, $token);
						}
					 if ($block_count > 0)
						{
						  $block_count--;
						}
					 $print_command_after = 0;
					 $token = "";
					 push(@tokens, $1);
				  }
				elsif (($token =~ m/(^|\W)$/) && 
				  ($command =~ s%^(if|while|unless|until|for|foreach|elsif|else|continue|sub)\b%%))
				  {
					 if ($cmiss_debug)
						{
						  print "control keyword: $1\n";
						}
					 $token = $token . $1;
					 $block_required = 1;
				  }
				elsif (($token =~ m/^\s*$/) && ($lc_command =~ m/^(itp)?\s*(ass\w*|set)?\s*(ech\w*|deb\w*)?\s*(\?+)/))
				  {
					 my $first_word = $1;
					 my $second_word = $2;
					 my $third_word = $3;
					 my $fourth_word = $4;
					 print "itp\n";
					 if ((! $second_word) || ($second_word =~ m/ass\w*/))
						{
						  print "  assert blocks closed\n";
						}
					 if ((! $second_word) || ($second_word =~ m/set/))
						{
						  print "  set\n";
						  if ((! $third_word) || ($third_word =~ m/ech\w*/))
							 {
								print "    echo\n";
								print "      <on>\n";
								print "      <off>\n";
								print "      <prompt PROMPT_STRING>\n";
							 }
						  if ((! $third_word) || ($third_word =~ m/deb\w*/))
							 {						  
								print "    debug\n";
								print "      <on>\n";
								print "      <off>\n";
							 }
						}
					 if (! $first_word)
						{
						  #Call Cmiss with the help command
						  $token .= "Perl_cmiss::cmiss_array(\"$fourth_word\")";
						  push(@tokens, $token);
						  $token = "";						  
						}
					 $command =~ s/^([^}#]*)//;
					 if ($cmiss_debug)
						{
						  print "itp?: $1\n";
						}
				  }
				elsif ($lc_command =~ m/^itp/)
				  {
					 if ($lc_command =~ m/^itp\s+ass\w*\s+blo\w*\s+clo\w*/)
						{
						  if ($block_required || $block_count)
							 {
								$block_required = 0;
								$block_count = 0;
								@command_list = ();
								die ("itp assert blocks closed failed\n");
							 }
						}
					 elsif ($lc_command =~ m/^itp\s+set\s+echo\s*(\w*)\s*(?:[\"\']([^\"\']*)[\"\']|([^\"\']+\S*)|)/)
						{
						  my $first_word = $1;
						  my $second_word = $2 ? $2 : $3;
						  if ($first_word =~ m/on/)
							 {
								$echo_commands = 1;
							 }
						  elsif ($first_word =~ m/off/)
							 {
								$echo_commands = 0;
							 }
						  elsif ($first_word =~ m/pro/)
							 {
								$echo_prompt = $second_word;
							 }
						  else
							 {
								$echo_commands = ! $echo_commands;
							 }
						}
					 elsif ($lc_command =~ m/^itp\s+set\s+debug\s*(\w*)/)
						{
						  if ($1 =~ m/on/)
							 {
								$cmiss_debug = 1;
							 }
						  elsif ($1 =~ m/off/)
							 {
								$cmiss_debug = 0;
							 }
						  else
							 {
								$cmiss_debug = ! $cmiss_debug;
							 }
						}
					 else
						{
						  die ("Unknown itp environment command\n");
						}
					 $command =~ s/^([^}#]*)//;
					 if ($cmiss_debug)
						{
						  print "itp: $1\n";
						}
				  }
				else
				  {
					 $continue = 1;
					 if ($token =~ m/^\s*$/)
						{
						  if (($lc_command =~ m/^(?:$match_string)\b/)
								|| ($lc_command =~ m/^q$/))
							 {
								$token = $token . "(Perl_cmiss::cmiss_array(";
								$part_token = "";
								$token2 = "";
								$is_perl_token = 1;
								$is_simple_token = 1;
								while (($command ne "") && !($command =~ m/(^[}	#])/))
								  {
									 if ($cmiss_debug)
										{
										  print "cmiss $command   ";
										}
									 if ($command =~ s%^([\s;]+)%%)
										{
										  if ($cmiss_debug)
											 {
												print "cmiss space: $1\n";
											 }
										  if (!$is_simple_token && $is_perl_token)
											 {
												# Let Perl parse this into a string array
												$token = $token . "[$part_token],\"$1\",";
											 }
										  else
											 {
												# Add it as a string 
												# Escape \\ and " characters
												$part_token =~ s/\\/\\\\/g;
												$part_token =~ s/\"/\\\"/g;
												$token = $token . "\"$part_token\",\"$1\",";
											 }
										  $token2 = $token2 . $part_token . $1;
										  $is_perl_token = 1;
										  $is_simple_token = 1;
										  $part_token = "";
										}
									 elsif (($part_token eq "") && ($command =~ s%^(\?+|[\-]?[.,0-9:]+)%%))
										{
										  if ($cmiss_debug)
											 {
												print "cmiss number/operator: $1\n";
											 }
										  $part_token = $part_token . $1;
										}
									 elsif ($command =~ s%^([.,0-9:]+)%%)
										{
										  if ($cmiss_debug)
											 {
												print "cmiss number/operator: $1\n";
											 }
										  $part_token = $part_token . $1;
										}
									 elsif ($command =~ s%^([+\-*=/\\<>!()?])%%)
										{
										  if ($cmiss_debug)
											 {
												print "cmiss perl number/operator: $1\n";
											 }
										  $part_token = $part_token . $1;
										  $is_simple_token = 0;
										}
									 elsif ($command =~ s%^(\w+\()%%)
										{
										  if ($cmiss_debug)
											 {
												print "cmiss function: $1\n";
											 }
										  $part_token = $part_token . $1;
										  $is_simple_token = 0;
										}
									 else
										{
										  $is_simple_token = 0;
										  ($extracted, $reduced_command) = 
											 Text::Balanced::extract_variable($command);
										  if ($extracted)
											 {
												$command = $reduced_command;
												$part_token = $part_token . $extracted;
												if ($cmiss_debug)
												  {
													 print "cmiss variable: $extracted\n";
												  }
											 }
										  else
											 {
												($extracted, $reduced_command) =
												  Text::Balanced::extract_delimited($command, '\'"`');
												if ($extracted)
												  {
													 $command = $reduced_command;
													 #Escape " and \ characters except for the start and end ones
													 #$extracted =~ s/(?<=.)\\(?=.)/\\\\/g;
													 #$extracted =~ s/(?<=.)\"(?=.)/\\\"/g;
													 $part_token = $part_token . $extracted;
													 if ($cmiss_debug)
														{
														  print "cmiss delimited: $extracted\n";
														}
												  }
												else
												  {
													 if ($cmiss_debug)
														{
														  print "cmiss character: ".substr($command, 0, 1)."\n";
														}
													 $part_token = $part_token . substr($command, 0, 1);
													 $command = substr($command, 1);
													 $is_perl_token = 0;
												  }
											 }
										}
								  }
							  $token2 = $token2 . $part_token;
							  $token2 =~ s/\\/\\\\/g;
							  $token2 =~ s/\"/\\\"/g;
#							  if ($cmiss_debug)
#							    {
#									print "token2 $token2\n";
#								 }
							  if (!$is_simple_token && $is_perl_token)
								 {
									# Let Perl parse this into a string array
									$token = $token . "[$part_token])) || die(\"Error in cmiss command \\\"$token2\\\".\\n\");";
								 }
							  else
								 {
									# Add it as a string 
									# Escape \\ and " characters
									$part_token =~ s/\\/\\\\/g;
									$part_token =~ s/\"/\\\"/g;
									$token = $token . "\"$part_token\")) || die(\"Error in cmiss command \\\"$token2\\\".\\n\");";
								 }
							  if ($cmiss_debug)
								 {
									print "cmiss: $token\n";
								 }
							  push(@tokens, $token);
							  $token = "";
						     $continue = 0;
                    }
				    }
				  if ($continue)
					 {
						$simple_perl = 1;
						if ($command =~ s/^(\d+)\s*\.\.\s*(\d+)\s*:\s*(\d+)//)
						  {
							 my $remainder_start = $1 % $3;
							 my $remainder_finish = ($2 - $remainder_start) % $3;
							 my $list_start = ($1 - $remainder_start) / $3;
							 my $list_finish = ($2 - $remainder_start - $remainder_finish)/ $3;
							 my $new_list_operator = "(map {\$_ * $3 + $remainder_start} $list_start..$list_finish)";
							 $token = $token . $new_list_operator;
							 if ($cmiss_debug)
								{
								  print "step sequence: $new_list_operator\n";
								}
						  }
						else
						  {
							 ($extracted, $reduced_command) =
								Text::Balanced::extract_variable($command);
							 if ($extracted)
								{
								  $command = $reduced_command;
								  if ($cmiss_debug)
									 {
										print "variable: $extracted\n";
									 }
								  $token = $token . $extracted;
								}
							 else
								{
								  ($extracted, $reduced_command) =
									 Text::Balanced::extract_quotelike($command);
								  if ($extracted)
									 {
										$command = $reduced_command;
										if ($cmiss_debug)
										  {
											 print "quotelike: $extracted\n";
										  }
										$token = $token . $extracted;
									 }
								  else
									 {
										if ($cmiss_debug)
										  {
											 print "character: " . substr($command, 0, 1) . "\n";
										  }
										$token = $token . substr($command, 0, 1);
										$command = substr($command, 1);
									 }
								}
						  }
					 }
				}
			}
		}
	 if ($token ne "")
		{
		  #Add a semicolon if not already there.
		  if ($simple_perl && (!$block_required) && (! ($token =~ m/;\s*$/)))
			 {
				$token = $token . ";";
			 }
		  push(@tokens, $token);
		}
						  
	$command = join ("", @tokens);
	if ($cmiss_debug)
	  {
		 print "Perl_cmiss::execute_command parsed $command\n";
	  }
	if ($echo_commands && (! $print_command_after))
     {
		 push (@command_list, $command2);
	  }
   push (@command_list, $command);
   if ($echo_commands && $print_command_after)
     {
		 push (@command_list, $command2);
	  }

#	 print \"$block_count $block_required\\n\";

	 if ((!($block_count))&&(!($block_required)))
		{
		  $command = join ("\n", @command_list);
		  #Must reset this before the eval as it may call this function
		  #recursively before returning from this function
		  @command_list = ();
		  call_command($command);
		  if ($@)
			 {
				#Trim the useless line number info if it has been added.
				$@ =~ s/ at \(eval \d+\) line \d+//;
				die("$@\n");
			 }
		  print "";
		}
  }

### Local Variables: 
### tab-width: 4
### End: 
