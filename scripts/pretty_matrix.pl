#  Copyright (c) 1997-2018
#  Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
#  http://www.polymake.org
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

# pretty matrix output
sub pretty_matrix($;$){
  my ($M, $options) = @_;
  # set defaults for options

  my $spaces = " ";
  if (defined($options->{"spaces"})) {
    $spaces= " " x $options->{"spaces"};
  }

  my @len = (0) x $M->cols();
  for (my $j=0; $j<$M->cols(); ++$j) {
    for (my $i=0; $i<$M->rows(); ++$i){
      $len[$j] = max($len[$j], length($M->elem($i,$j)));
    }
  }

  for (my $i=0; $i<$M->rows(); ++$i) {
    for (my $j=0; $j<$M->cols(); ++$j) {
      my $collen = $len[$j];
      printf("%${collen}s%s", $M->elem($i,$j), $spaces);
    }
    print "\n";
  }
}


# pretty sign matrix output
sub sign_matrix($){
  my ($M) = @_;

  for (my $i=0; $i<$M->rows(); ++$i) {
    for (my $j=0; $j<$M->cols(); ++$j) {
      if ($M->elem($i,$j) == 0) {
	print "  ";
      } else {
	my $sgn = $M->elem($i,$j) > 0 ? "+ " : "- ";
	print $sgn;
      }
    }
    print "\n";
  }
}

# Local Variables:
# mode: perl
# cperl-indent-level:2
# indent-tabs-mode:nil
# End:
