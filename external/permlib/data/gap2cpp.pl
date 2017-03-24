#!/usr/bin/perl -w

use strict;
use warnings;

my $line_buffer;
my @gens = ();
my $order = 0;

while (<>) {
    chomp;
    my $line_buffer = $_;
        $line_buffer =~ s/\s+//g;
        $line_buffer =~ s/,/ /g;
        $line_buffer =~ s/\)\(/,/g;
        $line_buffer =~ s/\(|\)//g;
        foreach my $num (split /,| /, $line_buffer) {
            $order = $num if $num > $order;
        }
        push @gens, $line_buffer;
}

print "$order\n";
foreach my $g (@gens) {
    print "$g\n";
}
