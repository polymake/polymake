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

###############################################################################
#
#  Common base for Visual::Wire and Visual::Graph

package Visual::WireBase;

use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
   [ '$EdgeColor' => 'unify_edge_decor(#%)', merge => 'unify_edge_decor(#%)', default => '$Visual::Color::edges' ],
   [ '$EdgeThickness' => 'unify_edge_decor(#%)', merge => 'unify_edge_decor(#%)', default => 'undef' ],
   [ '$EdgeLabels' => 'unify_edge_labels(#%)', default => '"hidden"' ],
   [ '$EdgeStyle' => 'unify_edge_decor(#%)', merge => 'unify_edge_decor(#%)', default => 'undef' ],
);

#  the same as Visual::unify_decor, but with extra dereferencing
#  in the cases of array and hash access.

sub unify_edge_decor {
   my ($name, $decor, $default)=@_;
   my $unified;
   if (is_like_array($decor)) {
      $unified=&unify_decor;
      sub { $unified->(${$_[0]}) }
   } elsif (is_like_hash($decor)) {
      if (is_code($default)) {
         # assuming that $default was already filtered thru unify_edge_decor,
         # we must avoid repeated dereferencing
         $unified=unify_decor($name,$decor);
         sub { $unified->(${$_[0]}) // &$default }
      } else {
         $unified=&unify_decor;
         sub { $unified->(${$_[0]}) }
      }
   } else {
      &unify_decor
   }
}

sub unify_edge_labels {
   my ($name, $labels)=@_;
   if (is_code($labels)) {
      $labels
   } else {
      $labels=&unify_labels
      and sub { $labels->(${$_[0]}) }
   }
}

sub ArrowStyle { undef }

###############################################################################

package Visual::Wire::EdgeIterator;
use Polymake::Struct (
   [ new => '$' ],
   [ '@edges' => '#1' ],
   [ '$index' => '0' ],
   [ '$end' => '$#{ #1 }' ],
);

use overload
   bool => sub { my $self=shift; $self->index <= $self->end },
   '++' => sub { $_[0]->index++ },
   '""' => sub { $_[0]->index },
   '${}' => sub { \($_[0]->index) },
   '@{}' => sub { my $self=shift; $self->edges->[$self->index] },
   '=' => sub { bless [ @{(shift)} ] },
   falback => 0;


package Visual::Wire;
use Polymake::Struct (
   [ '@ISA' => 'WireBase' ],
   [ '$Edges' => 'check_edges(#%)', default => 'croak("Edges missing")' ],
);

sub check_edges {
   my ($name, $edges)=@_;
   is_like_array($edges) ? $edges : croak( "$name is not an array" );
}

sub n_nodes { scalar(@{(shift)->Vertices}) }
sub n_edges { scalar(@{(shift)->Edges}) }

sub all_edges { new EdgeIterator((shift)->Edges) }


1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
