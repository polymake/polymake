#!/usr/bin/perl

#
# converts a polyhedron in polymake format (.poly)
# into lrs/cdd format (.ine/.ext)
#
# call:
#  poly2ineext.pl $polyFile
# will write corresponding .ine and/or .ext files
#

use strict;
use warnings;

my $file = $ARGV[0] or die 'no .poly file given';

my $ineName = $file;
$ineName =~ s/\.poly$/.ine/;
my $extName = $file;
$extName =~ s/\.poly$/.ext/;

my @vertices = ();
my @facets = ();

open(POLY, $file);
my $mode = 0;
while(<POLY>) {
    chomp;
    my $line = $_;
    if ($line eq 'POINTS') {
        $mode = 'v';
        next;
    } elsif ($line eq 'FACETS') {
        $mode = 'h';
        next;
    }

    push @vertices, $line if ($mode eq 'v');
    push @facets, $line if ($mode eq 'h');

    if ($mode and length($line) == 0) {
        $mode = 0;
    }
}
close POLY;


sub writeArray {
    my ($mode, $list) = @_;
    my @realList = @$list;
    my $str = '';
    $str .= "$mode-representation\n";
    $str .= "begin\n";
    $str .= $#realList . ' ';
    $str .= (my @dummy = split /\s+/, $realList[0]) . " rational\n";
    $str .= join("\n", @realList);
    $str .= "end\n";
    return $str;
}

sub writeFile {
    my ($mode, $list) = @_;
    my $file = '';
    $file = $ineName if $mode eq 'H';
    $file = $extName if $mode eq 'V';

    if (-e $file) {
        print "File $file already exists; skipping.\n";
        return -1;
    }
    print "Writing file $file.\n";
    open(DAT, ">$file");
    print DAT writeArray($mode, $list);
    close(DAT);
}

if ($#facets >= 0) {
    writeFile('H', \@facets);
}
if ($#vertices >= 0) {
    writeFile('V', \@vertices);
}

