use strict;
use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

my $name = 'Perl_cmiss';
my $xs = "$name.xs";
unless( exists $ENV{WORKING_DIR} ) {
    die "$0: environment variable WORKING_DIR not set\n";
}
my $working_dir = $ENV{WORKING_DIR};
unless( exists $ENV{HOSTTYPE} ) {
    die "$0: environment variable HOSTTYPE not set\n";
}
my $hosttype = $ENV{HOSTTYPE};
unless( -d $working_dir ) {
    use File::Path;
    # make and print subdirectories
    mkpath $working_dir,1 or
	die "$0: can't make $working_dir: $!\n";
}
my $xs_link = "$working_dir/$xs";
unless( -l $xs_link ) {
    print "making link $xs_link\n";
    symlink "$ENV{PWD}/$xs",$xs_link or
	die "$0: can't link $xs_link to $ENV{PWD}/$xs: $!\n";
}
my $object = "$working_dir/$name.o";
WriteMakefile(
    'NAME'	=> $name,
    'VERSION_FROM' => "$name.pm", # finds $VERSION
#      'XS'        => {'Perl_cmiss.xs' => "$working_dir/Perl_cmiss.c"},
    'LIBS'	=> [''],   # e.g., '-lm'
#      'DEFINE'	=> '' $hosttype eq 'i386-linux' ? '-Dbool=char -DHAS_BOOL' : '',
    'MYEXTLIB'  => '',# to prevent undefined warnings in Makefile
    'INC'	=> '-I.',     # e.g., '-I/usr/include/other'
    'CCCDLFLAGS' => '-o$*.o', # for producing .o in directory of .c
    'OBJECT'    => $object,
    'LINKTYPE'  => 'static',
    'INST_ARCHLIB' => "$ENV{PWD}/$working_dir"
#      'depend'    => {'$(OBJECT)' => "$working_dir/Perl_cmiss.c",
#  		    "$working_dir/Perl_cmiss.c", 'Perl_cmiss.xs'}
);
