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

package Visual::Graph;

use Polymake::Struct (
   [ '@ISA' => 'WireBase' ],
   [ aliases => (Coord => 'Vertices') ],
   [ '$VertexLabels | NodeLabels' => '$this->prep_node_decor(#%)', default => 'undef' ],
   [ '$VertexColor | NodeColor' => '$this->prep_node_decor(#%)', default => '$Visual::Color::vertices' ],
   [ '$VertexThickness | NodeThickness' => '$this->prep_node_decor(#%)', default => '1' ],
   [ '$VertexBorderColor | NodeBorderColor' => '$this->prep_node_decor(#%)', default => 'undef' ],
   [ '$VertexBorderThickness | NodeBorderThickness' => '$this->prep_node_decor(#%)', default => 'undef' ],
   [ '$VertexStyle | NodeStyle' => '$this->prep_node_decor(#%)', default => 'undef' ],

   [ '$EdgeColor' => '$this->prep_edge_decor(#%)', default => '$Visual::Color::edges' ],
   [ '$EdgeThickness' => '$this->prep_edge_decor(#%)', default => 'undef' ],
   [ '$EdgeLabels' => '$this->prep_edge_decor(#%)', default => '"hidden"' ],
   [ '$EdgeStyle' => '$this->prep_edge_decor(#%)', default => 'undef' ],

   [ '$Graph' => '#%', default => 'croak("Graph missing")' ],
   [ '$renumber_nodes' => 'undef' ],
   [ '$inv_renumber_nodes' => 'undef' ],

   # 0: no arrows
   # 1: from source to sink
   # -1: from sink to source
   [ '$ArrowStyle' => '$this->prep_edge_decor(#%)', merge => 'unify_edge_decor(#%)', default => '$this->is_directed' ],
);

############################################################################

sub representative { $_[0]->Graph; }

sub is_directed {
   (shift)->Graph->ADJACENCY->type->params->[0] == Directed->type
}

sub n_nodes { (shift)->Graph->ADJACENCY->nodes }
sub n_edges { (shift)->Graph->ADJACENCY->edges }

sub is_embedded { defined((shift)->Coord) }

sub needs_renumbering {
   my $self=shift;
   unless (defined($self->renumber_nodes)) {
      if ($self->Graph->ADJACENCY->has_gaps) {
         $self->renumber_nodes=[ @{nodes($self->Graph->ADJACENCY)} ];
         my $n=0;
         $self->inv_renumber_nodes=[ ];
         $self->inv_renumber_nodes->[$_]=$n++ for @{$self->renumber_nodes};
      } else {
         $self->renumber_nodes=0;
      }
   }
   $self->renumber_nodes
}

sub prep_node_decor : method {
   my $self=shift;
   my ($name, $decor, $default)=@_;
   my $unified= $name =~ /Labels$/ ? &unify_labels : &unify_decor;
   if (is_code($unified)  and
       !is_like_array($decor) || @$decor > $self->n_nodes  and
       my $renumber=$self->needs_renumbering) {
      sub { $unified->($renumber->[shift]) }
   } else {
      $unified
   }
}

sub prep_edge_decor : method {
   my $self=shift;
   my ($name, $decor)=@_;
   my $unified= $name =~ /Labels$/ ? &unify_edge_labels : &unify_edge_decor;
   if (is_code($unified)) {
      $self->Graph->ADJACENCY->init_edge_map(new Set);
   }
   $unified
}

sub all_edges {
   my $self=shift;
   if ($self->needs_renumbering) {
      my $it=bless(entire(edges($self->Graph->ADJACENCY)), "Visual::Graph::EdgeIteratorWithGaps");
      $it->hidden=$self->inv_renumber_nodes;
      $it
   } else {
      bless(entire(edges($self->Graph->ADJACENCY)), "Visual::Graph::EdgeIterator");
   }
}

sub has_edge {
   my ($self, $s, $t)=@_;
   my $G=$self->Graph->ADJACENCY;
   $s < $G->nodes && $t < $G->nodes && $G->edge_exists($s,$t);
}

############################################################################
package Visual::Graph::EdgeIterator;
@ISA=qw( Polymake::common::EdgeIterator );

use overload
   '@{}' => sub {
      my $self=shift;
      [ $self->from_node, $self->to_node ]
   };

package Visual::Graph::EdgeIteratorWithGaps;
@ISA=qw( Polymake::common::EdgeIterator );

use overload
   '@{}' => sub {
      my $self=shift;
      [ @{$self->hidden}[ $self->from_node, $self->to_node ] ]
   };

############################################################################
package Visual::Graph;

sub add_decor_filters {
   my ($self, $Graph, $decor)=@_;

   my $marked_nodes= (grep { /^Node/ } keys %$decor) ? createNodeHashMap<Bool>($Graph) : undef;
   my $marked_edges= (grep { /^Edge/ } keys %$decor) ? createEdgeHashMap<Bool>($Graph) : undef;

   my @decor_filters;
   while (my ($name, $value)=each %$decor) {
      if ($name =~ /^(?:Node|Vertex)/) {
         push @decor_filters, $name, sub { $marked_nodes->{$_[0]} && $value };
      } elsif ($name =~ /^(?:Edge|Arrow)/) {
         push @decor_filters, $name, sub { $marked_edges->{${$_[0]}} && $value };
      }
   }

   $self->merge(@decor_filters);
   ($marked_nodes, $marked_edges);
}

sub add_node_subset {
   my ($self, $subset, $decor)=@_;
   # 1: outgoing edges, -1: ingoing edges, 0: both
   my $which_edges=!($self->is_directed) || $decor->{edges} || 1;
   delete $decor->{edges};

   my ($marked_nodes, $marked_edges)=$self->add_decor_filters($self->Graph->ADJACENCY, $decor);

   if (defined($marked_nodes)) {
      $marked_nodes->{$_}=1 for @$subset;
   }
   if (defined($marked_edges)) {
      foreach (@$subset) {
         if ($edges>=0) {
            for (my $e=$self->Graph->ADJACENCY->out_edges->($_); $e; ++$e) { $marked_edges->{$$e}=1 }
         }
         if ($edges<=0) {
            for (my $e=$self->Graph->ADJACENCY->in_edges->($_); $e; ++$e) { $marked_edges->{$$e}=1 }
         }
      }
   }
}

1

# Local Variables:
# mode: perl
# cperl-indent-level: 3
# indent-tabs-mode:nil
# End:
