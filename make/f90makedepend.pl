#!/usr/bin/perl -w

# usage: f90makedepend.pl <input-file>

# Writes dependencies for <input-file> to stdout.

# Written: Karl Tomlinson 31/5/00

use strict;

# collect arguments

if( @ARGV != 1 ) {
    die "usage: f90makedepend.pl <input-file>\n";
}
my ($input_file) = @ARGV;
unless( $input_file ) {
    die "empty input file name";
}
my $module_name = &basename( &tail( $input_file ) );

unless( exists $ENV{HOSTTYPE} ) {
    die "$0: environment variable HOSTTYPE not set\n";
}
my $hosttype = $ENV{HOSTTYPE};
my $uppercase = $hosttype eq 'iris4d';

# open input file

open( INPUT_FILE, $input_file ) or die "Couldn't open $input_file, $!\n";

# make files in the local output directory

my (%defined_modules,%used_modules);
while( defined(my $line = <INPUT_FILE>) ) {
    if( $line =~ /^ *MODULE +(\w*)/i ) {
	$defined_modules{ &case_module( $1 ) } = 1;
    } elsif( $line =~ /^ *USE +(\w*)/i ) {
	$used_modules{ &case_module( $1 ) } = 1;
    }
}

# dependencies for the object file
print "\$(WORKING_DIR)/$module_name.o : $input_file";
foreach( keys %used_modules ) {
    if( ! exists $defined_modules{ $_ } ) {
	print " \\\n\t\$(WORKING_DIR)/$_.mod_date";
    }
}
print "\n";


# .mod files that depend on the compiling of the fortran file
if( %defined_modules ) {
    foreach( keys %defined_modules ) {
	print "\$(WORKING_DIR)/$_.mod \\\n\t";
    }
    print ": \$(WORKING_DIR)/$module_name.o\n";
}

sub case_module {
    return $uppercase ? uc $_[0] : lc $_[0];
}

sub basename {
    my ($filename) = @_;
    my $lastdot = rindex( $filename, '.' );
    if( $lastdot != -1 ) {
	return substr( $filename, 0, $lastdot );
    }
    else {
	return $filename;
    }
}
sub tail {
    my ($filename) = @_;
    my $lastslash = rindex( $filename, '/' );
    if( $lastslash != -1 ) {
	return substr( $filename, $lastslash+1 );
    }
    else {
	return $filename;
    }
}
