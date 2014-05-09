use strict;
use ExtUtils::MakeMaker;
use Config;

# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

my $name = 'Perl_cmiss';
my $xs = "$name.xs";

my $num_args = $#ARGV + 1;
if ( $num_args != 3 ) {
	die "requires 3 parameters CMISS_PERL_CALLBACK, WORKING_DIR and build type \(debug/release\), in that order";
}

my $osname = $^O;

my $working_dir = $ARGV[1];#$ENV{WORKING_DIR} or
#  die "required environment variable WORKING_DIR not set";
unless( -d $working_dir ) {
    use File::Path;
    # make and print subdirectories
    mkpath $working_dir,1 or
	die "$0: can't make $working_dir: $!\n";
}

print "$0: Entering directory `$working_dir'\n";
chdir $working_dir or die "$0: can't chdir to $working_dir";
my $source_dir = '../../..';

foreach my $link ( 'Makefile.PL', $xs ) {
	my $source = "$source_dir/$link";
	my $dest = "$link";
	if ($link eq 'Makefile.PL') {
		$dest = 'Makefile.PL';
	}
	unless( -l $dest ) {
		print "Making link $source, $dest\n";

		#Win32 does not have symlink in perl so lets fallback to copy
		defined eval { symlink $source,$dest } or
			(! eval { $source =~ s/\//\\/g } ) or
			(! system ("copy $source $dest")) or
			die "$0: can't link $source, $dest: $!\n";
	}
}

my $define_list;
if ($num_args == 3)
  {
	 $define_list .= " -DCMISS_PERL_CALLBACK=\"$ARGV[0]\"";
# I don't think this is necessary because XS_Perl_cmiss_cmiss is a static
# function that is handled by function pointers.
	 $define_list .= " -DXS_Perl_cmiss_cmiss=\"XS_Perl_cmiss_$ARGV[0]\"";
  }
else
  {
	 die ("Required environment variable CMISS_PERL_CALLBACK not set by Makefile");
  }

#my $object = "$name.o";
my $c_flags .= $Config{ccflags};
my $c_flags_opt = '';
if( $osname eq 'MSWin32' ){
	sub MY::const_loadlibs { "" }
	#$c_flags .= perl '-V:ccflags';
	if( "$ARGV[2]" eq "Debug" ) {
		$c_flags =~ s/-MD/-MDd/;
		$c_flags =~ s/-DNDEBUG/-D_DEBUG/;
		$c_flags =~ s/-O1/-Od/;
	}
	$c_flags_opt = '-DWIN32';
	$c_flags =~ s/-GL//;
	#print "config win32 c_flags ----- $c_flags\n";
}
if( $osname eq 'darwin' ){
	$c_flags =~ s/-arch x86_64//;
	$c_flags =~ s/-arch ppc//;
	if( "$ARGV[2]" eq "Debug" ) {
		$c_flags =~ s/-O3/-g/;
	}
	#print "config darwin c_flags ----- $c_flags\n";
}

WriteMakefile(
	'NAME' => $name,
	# ??? Shouldn't this be "$source_dir/../source/$name.pm" or should that be moved here.
	'VERSION_FROM' => "$source_dir/$name.pm", # finds $VERSION
#    'EXTRALIBS' => '',# to prevent undefined warnings in Makefile
	'MYEXTLIB'  => '',# to prevent undefined warnings in Makefile
	'INC' => "-I$source_dir",     # e.g., '-I/usr/include/other'
	'DEFINE' => $define_list,
	'LINKTYPE' => 'static',
	'CCFLAGS' => $c_flags,
	'OPTIMIZE' => $c_flags_opt,
	'INST_ARCHLIB' => '.'
);

print "$0: Leaving directory `$working_dir'\n";

