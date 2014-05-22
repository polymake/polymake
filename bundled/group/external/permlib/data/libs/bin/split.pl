#!/usr/bin/perl
use strict;
use warnings;

sub name {
    my ($num) = @_;
    return sprintf(">G_%04d", $num);
}

my $cnt = 1;
open(GROUP, name($cnt));
my $linebuf;
while(<>) {
    next if /^\.$/;
    if ($_ =~ /^\-+$/) {
        print GROUP $linebuf;
        close GROUP;
        open(GROUP, name(++$cnt));
        $linebuf = '';
    } else {
        $linebuf .= $_;
    }
}
print GROUP $linebuf;
close GROUP;

