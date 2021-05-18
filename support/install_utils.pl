#  Copyright (c) 1997-2021
#  Ewgenij Gawrilow, Michael Joswig, and the polymake team
#  Technische Universität Berlin, Germany
#  https://polymake.org
#
#  This program is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by the
#  Free Software Foundation; either version 2, or (at your option) any
#  later version: http://www.gnu.org/licenses/gpl.txt.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#-------------------------------------------------------------------------------

# utilities used in generate_ninja_targets.pl and install.pl

use strict;

sub extract_polymake_version {
  my ($root)=@_;
  open my $pm, "$root/perllib/Polymake.pm"
    or die "can't read $root/perllib/Polymake.pm: $!\n";
  local $/="\n";
  local $_;
  while (<$pm>) {
    if (/^\s*declare\s+\$Version\s*=\s*(['"])(\d+\.\d+)(\.\d+)*\1\s*;\s*$/) {
      return "$2";
    }
  }
  die "could not find polymake version in $root/perllib/Polymake.pm\n";
}

sub load_config_file {
  my ($config_file, $root)=@_;
  my %values;
  open my $cf, $config_file
    or die "configuration file $config_file is missing: have you run configure?\n";
  local $_;
  while (<$cf>) {
     if (m{^\s*(?!super.)([\w.]+)\s*=\s*(.*(?<!\s))\s*$}) {
        $values{$1}=$2;
     }
  }
  close $cf;
  $root eq $values{root}
     or die "configuration file $config_file does not match the top directory $root\n";
  %values;
}

sub basename {
  if ((my ($name, $suffix)=@_)==2) {
    if ($name =~ m{/(([^/.]+)\.$suffix)$}) {
      return ($1, $2);
    }
  } elsif ($name =~ m{/([^/]+)$}) {
    return $1;
  }
  $_[0];
}

sub dirname {
  my $basename=&basename;
  $basename ne $_[0] && substr($_[0], 0, length($_[0])-length($basename)-1)
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
