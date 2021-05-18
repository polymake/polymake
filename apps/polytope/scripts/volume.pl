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

use application "polytope";

# volume of a union of polytopes (in the same space); by inclusion-exclusion
sub vol_union(@) {
  my $n = scalar(@_);
  die "usage: vol_union(poly1,poly2,...)\n" unless $n>0;
  my $total = 0;
  my $sign = 1;
  for (my $k=1; $k<=$n; ++$k) {
    my $v = 0;
    foreach my $subset (@{all_subsets_of_k(sequence(0, $n), $k)}) {
      $v += intersection(@_[@$subset])->VOLUME; 
    }
    $total += $sign * $v;
    $sign = -$sign;
  }
  return $total;
}

# Local Variables:
# mode: perl
# c-basic-offset:3
# End:
