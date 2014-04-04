#!/usr/bin/perl

use strict;
use warnings;
use File::Find;

my $directory = '';
die ('no directory given') unless $directory = shift @ARGV;

print "#!/bin/sh\n\n";
print "function sympol\n{\n  echo \"\$1\"; ./sympol/sympol -e \"\$1\";\n}\n\n";

find(\&wanted, ($directory));
sub wanted {
	return unless /(.*)\.(ine|ext)$/;
	my $correctedFile = "$1-corrected.$2";
	return if $File::Find::dir =~ /\.svn/;
	return if -e $correctedFile;
	
	print "sympol \"" . $File::Find::name . "\"\n";
}

