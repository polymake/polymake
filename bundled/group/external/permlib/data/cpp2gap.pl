#!/usr/bin/perl
use strict;
use warnings;

my $filename = shift @ARGV or die 'no file given';

if ($filename =~ /^G_/) {
    open (GROUP, $filename);
    #<GROUP>;
    <GROUP>;
    print "Group(";
    my @lines = ();
    while (my $line = <GROUP>) {
        chomp $line;
        $line =~ s/,/)(/g;
        $line =~ s/ /,/g;
        push @lines, "($line)";
    }
    print join ',', @lines;
    print ");\n";
    close GROUP;
} else {
    open (SET, $filename);
    <SET>;
    my $set = <SET>;
    chomp $set;
    $set =~ s/ /,/g;
    print "[$set]\n";
    close SET;
}
