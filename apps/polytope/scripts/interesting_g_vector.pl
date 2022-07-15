#  Copyright (c) 1997-2022
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

use application 'polytope';

# produce interesting g_vectors of simplicial polytopes
# Ziegler: Lectures on Polytopes, §8.6

# Example with non-unimodal f-vector:
# $p=interesting_g_vector(20,169,4203045807457);

sub interesting_g_vector(@) {
  die "usage: interesting_g_vector(d,n,r)" unless scalar(@_)==3;
  my ($d,$n,$r) = @_;
  my $length=$d/2+1;
  my $g=new Vector<Integer>($length);
  $g->[0] = 1; $g->[1] = $n-$d-1+$r;
  for (my $k=2; $k<$length; ++$k) {
    $g->[$k] = binomial($n-$d-2+$k,$k);
  }
  return (new Polytope(G_VECTOR=>$g, COMBINATORIAL_DIM=>$d, SIMPLICIAL=>1));
}


# Local Variables:
# mode: perl
# c-basic-offset:3
# End:
