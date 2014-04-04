#!/usr/bin/perl

use strict;
use warnings;

my $description = '?';
my $dimension = 0;
my $rows = 0;
my $modeExpectData = 0;
my $data = '';
my $filename;
die ("no filename given") unless $filename = shift @ARGV;
exit(0) if $filename =~ /corrected-\.(ine|ext)$/;
die ("unsupported file format $filename") unless $filename =~ /^(.*)\.(ine|ext)$/;
my $outputFilename = "$1-corrected.$2";

open(FILE, $filename) or die ("could not open $filename");
open(FILE_CORRECTED, ">$outputFilename") or die ("could not open output $outputFilename");

while(<FILE>) {
	chomp;
	my $line = $_;
	
	if ($line =~ /^(H|V)-representation/) {
		$description = $1;
		print FILE_CORRECTED "$line\n";
	} elsif ($line =~ /^begin/) {
		if ($description eq '?') {
			print FILE_CORRECTED "H-representation\n" if $filename =~ /\.ine$/;
			print FILE_CORRECTED "V-representation\n" if $filename =~ /\.ext$/;
		}
		print FILE_CORRECTED "$line\n";
		
		$line = <FILE>;
		if ($line =~ /^\s*(\d+)\s+(\d+)\s+(\w+)/) {
			$rows = $1;
			$dimension = $2;
			print FILE_CORRECTED "$1 $2 $3\n";
			
			$modeExpectData = 1;
		} else {
			die ("expected dimensions, got $line");
		}
	} elsif ($line =~ /^end/) {
		$data =~ s/^(\s*)(.*?)(\s*)$/$2/;
		my @numbers = split /\s+/, $data;
		for (my $i = 0; $i < $rows; ++$i) {
			for (my $j = 0; $j < $dimension; ++$j) {
				print FILE_CORRECTED ($numbers[$j + $i * $dimension] . " ");
			}
			print FILE_CORRECTED "\n";
		}
		$modeExpectData = 0;
		print FILE_CORRECTED "$line\n";
	} elsif ($modeExpectData) {
		$data .= "$line ";
	} else {
		print FILE_CORRECTED "$line\n";
	}
}

close(FILE);
close(FILE_CORRECTED);
