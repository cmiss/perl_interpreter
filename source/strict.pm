package strict;


$strict::VERSION = "1.01";

my %bitmask = (
refs => 0x00000002,
subs => 0x00000200,
vars => 0x00000400
);

sub bits {
    my $bits = 0;
    foreach my $s (@_){ $bits |= $bitmask{$s} || 0; };
    $bits;
}

sub import {
    shift;
    $^H |= bits(@_ ? @_ : qw(refs subs vars));
}

sub unimport {
    shift;
    $^H &= ~ bits(@_ ? @_ : qw(refs subs vars));
}
