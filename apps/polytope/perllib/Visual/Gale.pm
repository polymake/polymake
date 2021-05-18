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

package Visual::Gale;
use Math::Trig;

use Polymake::Struct (
   [ '@ISA' => 'Object' ],
   [ '$Polytope' => '#%' ],
   [ '$VertexLabels' => 'unify_labels(#%)', default => 'undef' ],
   '@points',
   '@colors',
   '$dim',
   [ '$different_x_y' => 'new Map<Vector<Float>, Set<Int>>' ],
   '%different_angles',
   [ '$whites' => 'new Map<Vector<Float>, Int>' ],
   [ '$blacks' => 'new Map<Vector<Float>, Int>' ],
   '@loops',
   '@gale_lines',
);

declare %decorations=( Name => undef, Title => undef, VertexLabels => enum("hidden") );

sub new {
   my $self=&_new;

   my $Transform=new Matrix<Float>($self->Polytope->GALE_TRANSFORM);
   $self->dim=$Transform->cols-1;

   # get affine coordinates
   my $v=0;
   foreach my $gv (@{$self->Polytope->GALE_VERTICES}) {
      my $color=$gv->[0];
      my $point=$gv->slice(range_from(1));
      push @{$self->colors}, $color;
      push @{$self->points}, $point;
      if ($color) {
         if ($self->dim==1) {
            my $angle=atan2($Transform->elem($v,1), $Transform->elem($v,0));
            push @{$self->different_angles->{$angle}}, $v;
         }
         $self->different_x_y->{$point} += $v;
         if ($color>0) {
            $self->whites->{$point}=1;
            delete $self->blacks->{$point};
         } elsif (!exists $self->whites->{$point}) {
            $self->blacks->{$point}=1;
         }
      } else {
         push @{$self->loops}, $v;
      }
      ++$v;
   }

   # for each facet find a triple of b/w points not belonging to it
   foreach my $compl_list (@{~ $self->Polytope->VERTICES_IN_FACETS}) {
      my @gale_line=@$compl_list;
      if (@gale_line==3) {
         push @{$self->gale_lines}, \@gale_line;
      }
   }

   $self;
}


1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
