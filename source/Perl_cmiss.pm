#*******************************************************************************
#FILE : Perl_cmiss.pm
#
#LAST MODIFIED : 9 March 2001
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
# Try to use Config.pm but only print a failure message once
$use_config = 1;

sub set_INC_for_platform ($)
  {
	# Call this if the perl interpreter does not match the modules on the
	# local system (i.e. it is a built-in interpreter.
    my ($abi_env) = @_;

    my $perlinc;
	my ($module_perl_version,$module_numeric_version);# perl module versions

	# Run the local perl to get its version and @INC so we can use its
	# modules.

	# Default is first perl in path:
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
		print STDERR "$^X: fork failed: $!\n";
	  }
	elsif( ! $pid ) #child
	  {
		exec $perl, '-e',
		  'print join ":", $^V ? sprintf("v%vd",$^V) : "undef", $], @INC'
		  or exit $!;
	  }
	else # parent
	  {
		my $perlout = <PERLOUT>;
		# check child completed successfully
		unless( close PERLOUT )
		  {
			$! = $? >> 8;
			print STDERR "$^X: exec $perl failed: $!\n";
			$use_config = 0;
		  }
		else
		  {
			# perl has given us its version include list.
			($module_perl_version,$module_numeric_version,@INC) =
			  split /:/,$perlout;
		  }
	  }

	# See if an environment variable is set to override @INC.
    foreach my $varname ("CMISS${abi_env}_PERLINC","CMISS_PERLINC")
	  {
		if( exists $ENV{$varname} )
		  {
			@INC = split /:/,$ENV{$varname};
			last;
		  }
      }

	# Dynaloader.pm uses Config.pm.

	# Config.pm checks with the version of the interpreter.  We can lie about
	# our version so that this check passes.  Do this here so that require
	# Config works seemlessly elsewhere.
	# Should we check binary compatibility first?

	if( defined $module_perl_version )
	  {
		{
		  # Convert version to a version literal for $^V
		  local $^V = eval $module_perl_version;
		  local $] = $module_numeric_version;
		  eval { require Config };
		}

		if ( $@ )
		  {
			warn $@;
			$use_config = 0;
		  }
# 		else
# 		  {
		    # Now that we have Config, it would be an easy and good time to
			# check that we are binary compatible.  We could even pretend to
			# unrequire Config if we are not.
# 		  }
	  }
  }

sub add_cmiss_perl_to_INC
  {
	{
	  #By not using "use" we can avoid the BEGIN and therefore the $use_config
	  #variable will be in the correct scope.  Because of this the automatic imports
	  #will not happen, so below we just use the qualified name for the Config hash.
	  eval { require Config };
	  if ( $@ )
	  {
		warn $@;
		$use_config = 0;
	  }
	}

    my ($abi_env) = @_;

	my @path_list;

	#Check the environment variables which specify this lib
    foreach my $varname ("CMISS${abi_env}_PERLLIB","CMISS_PERLLIB")
	{
	  if ( exists $ENV{$varname} )
	 {
		my @env_list = split (':', $ENV{$varname});
		if ( $use_config )
		{
		  #Add in a version/archname specific derivative of any paths if they exist
		  # !!! Should use inc_version_list here (or just use lib.pm)
		  # or even choose our own old versions.
		  @path_list = (@env_list,
			 grep { -d $_ } map { "$_/$Config::Config{version}/$Config::Config{archname}" } @env_list);
		}
		last;
	  }
    }
	# If no CMISS*_PERLLIB variable is set then see if there is a CMISS_ROOT version
	if (! @path_list)
	{
	  if (exists $ENV{CMISS_ROOT})
	  {
		my $cmissroot_perl_lib = "$ENV{CMISS_ROOT}/cmiss_perl/lib";
		@path_list = ();
		if (-d $cmissroot_perl_lib)
		{
		  push @path_list, $cmissroot_perl_lib;
		}
		if ( $use_config )
		{
		  my $cmissroot_perl_lib_arch = "$ENV{CMISS_ROOT}/cmiss_perl/lib/$Config::Config{version}/$Config::Config{archname}";
		  if (-d $cmissroot_perl_lib_arch)
		  {
			push @path_list, $cmissroot_perl_lib_arch;
		  }
		}
	  }
	}
	# If a CMISS_PERLLIB has been found prepend it to the path.
	if (@path_list)
	{
	  unshift @INC, @path_list;
	}
 }

sub register_keyword
  {
	 my $word = shift;

	 #print \"register $word\\n\";

	 $keywords{$word} = 1;
  }

sub get_keywords_hash_reference
  {
	return \%keywords;
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
		  if (! defined $token)
		  {
			 print ("Undefined variable referenced in command\n");
			 return (0);
		  }
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
					if (! defined $subtoken)
					  {
						print ("Undefined variable referenced in command\n");
						return(0);
					  }
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
				elsif( $token =~ m/^\s*$/ &&
					   $lc_command =~ m/^(itp(\s+(ass\w*)(?:\s+blo\w*(?:\s+clo\w*)?)?|\s+(set)(\s+(ech\w*)|\s+(deb\w*))?)?\s+)?\?+/ )
			    {
				    my $itp = defined $1;
					my $second_word = defined $2;
					my $assert = defined $3;
					my $set = defined $4;
					my $third_word = defined $5;
					my $echo = defined $6;
					my $debug = defined $7;

					print "itp\n";
					if ( ! $second_word || $assert ) {
						print "  assert blocks closed\n";
					}
					if ( ! $second_word || $set ) {
						print "  set\n";
						if ( ! $third_word || $echo ) {
							print "    echo\n";
							print "      <on>\n";
							print "      <off>\n";
							print "      <prompt PROMPT_STRING>\n";
						}
						if ( ! $third_word || $debug ) {
							print "    debug\n";
							print "      <on>\n";
							print "      <off>\n";
						}
					}
					if ( ! $itp ) {
						#Call Cmiss with the help command
						$token .= "Perl_cmiss::cmiss_array(\"$lc_command\")";
						push(@tokens, $token);
						$token = "";
					}
					$command =~ s/^([^}#]*)//;
					if ( $cmiss_debug ) {
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
											 Perl_cmiss::Text::Balanced::extract_variable($command);
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
												  Perl_cmiss::Text::Balanced::extract_delimited($command, '\'"`');
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
							  $token2 =~ s/\$/\\\$/g;
							  $token2 =~ s/\@/\\\@/g;
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
								Perl_cmiss::Text::Balanced::extract_variable($command);
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
									 Perl_cmiss::Text::Balanced::extract_quotelike($command);
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
									  if ($command =~ s/^(\w+)//)
										{
										  if ($cmiss_debug)
											{
											  print "characters: " . $1 . "\n";
											}
										  $token = $token . $1;
										}
									  else
										{
										  if ($cmiss_debug)
											{
											  print "punctuation: " . substr($command, 0, 1) . "\n";
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
