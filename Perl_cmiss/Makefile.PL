use strict;
use ExtUtils::MakeMaker;
use Config;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

my $name = 'Perl_cmiss';
my $xs = "$name.xs";

my $osname = $^O;

my $working_dir = $ENV{WORKING_DIR} or
  die "required environment variable WORKING_DIR not set";
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
        unless( -l $link ) {
        print "making link $link\n";
        defined eval { symlink $source,$link } or
          #Win32 does not have symlink in perl so lets fallback to cp
	  (! system ("cp $source $link")) or
	    die "$0: can't link $source, $link: $!\n";
    }
}

my $define_list;
if (defined $ENV{CMISS_PERL_CALLBACK})
  {
	 $define_list .= " -DCMISS_PERL_CALLBACK=\"$ENV{CMISS_PERL_CALLBACK}\"";
# I don't think this is necessary because XS_Perl_cmiss_cmiss is a static
# function that is handled by function pointers.
	 $define_list .= " -DXS_Perl_cmiss_cmiss=\"XS_Perl_cmiss_$ENV{CMISS_PERL_CALLBACK}\"";
  }
else
  {
	 die ("Required environment variable CMISS_PERL_CALLBACK not set by Makefile");
  }

my $c_flags .= $Config{ccflags};
if( $osname eq 'darwin' ){
        $c_flags =~ s/-arch x86_64//;
        $c_flags =~ s/-arch ppc//;
        if( "$ARGV[2]" eq "Debug" ) {
                $c_flags =~ s/-O3/-g/;
        }
        print "config darwin c_flags ----- $c_flags\n";
}

#my $object = "$name.o";
WriteMakefile(
    'NAME'	=> $name,
    # ??? Shouldn't this be "$source_dir/../source/$name.pm" or should that be moved here.
    'VERSION_FROM' => "$source_dir/$name.pm", # finds $VERSION
    'MYEXTLIB'  => '',# to prevent undefined warnings in Makefile
    'INC'	=> "-I$source_dir",     # e.g., '-I/usr/include/other'
    'DEFINE'	=> $define_list,
    'CCFLAGS'	=> $c_flags,
    'LINKTYPE'  => 'static',
    'INST_ARCHLIB' => '.'
);

print "$0: Leaving directory `$working_dir'\n";
