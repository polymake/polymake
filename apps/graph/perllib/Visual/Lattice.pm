#  Copyright (c) 1997-2015
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

package Visual::Lattice;

use Polymake::Struct (
   [ '@ISA' => 'Graph' ],
   [ '$NodeLabels' => '$this->create_node_labels(#%)', default =>'undef' ],
   [ '$NodeColor' => 'unify_decor(#%)', default => 'undef' ],
   [ '$NodeBorderColor' => 'unify_decor(#%)', default => '"0 0 0"' ],
   [ '$top_node' => '#%', default => 'undef' ],
   [ '$bottom_node' => '#%', default => 'undef'],
   [ '$Dims' => '#%', default => 'undef' ],
   [ '$Mode' => '#%', default => '"primal"' ],
   [ '$Faces' => '#%', default => 'croak("Faces missing")' ],
   [ '$AtomLabels' => '#%', default => 'undef' ],

);

sub create_node_labels {
   my ($self, undef, $labels) = @_;
   if(!defined($labels)){
      my $get_face;
      if(defined($self->AtomLabels)) {
         my @atom_labels=@{$self->AtomLabels};
         my $n = 0;
         foreach (@atom_labels) { s/^_.*/\#$n/; ++$n; }
         $get_face = sub { join(" ", @atom_labels[${$self->Faces->[shift]}])};
      }
      else {
         $get_face = sub { join(" ", @{$self->Faces->[shift]})};
      }
      $labels = [ 
         map { ($_ == $self->top_node or $_ == $self->bottom_node)? " " : &$get_face($_) } 0 .. $#{$self->Faces}
      ];
   }
   sub { $labels->[shift] }
}

# $matching -> HasseDiagram with boolean edge attributes, like MORSE_MATCHING
sub add_matching {
   my ($self, $matching, $decor)=@_;
   my @hd = @$matching;
   shift @hd; pop @hd;
   $hd[0] =~ s/^<//;

   my (%edge_color, %edge_style, %arrow_style);
   my $matched_color=$decor->{EdgeColor};
   my $matched_style=$decor->{EdgeStyle};

   my $n=0;
   foreach (@hd) {
      my ($face, $edges) = m/^\(\{ (.*?) \} \s+ \{ (.*?) \}\)$/x;
      my @edges = $edges =~ m/\( (\d+) \s+ ([01]) \)/gx;
      while (my ($to_node, $matched) = splice @edges, 0, 2) {
	 if ($matched) {
	    $edge_color{"$n $to_node"} = $matched_color if defined $matched_color;
	    $edge_style{"$n $to_node"} = $matched_style if defined $matched_style;
	    $arrow_style{"$n $to_node"} = -1;
	 } else {
	    $arrow_style{"$n $to_node"} = 1;
	 }
      }
      ++$n;
   }

   local_scalar($self->EdgeStyleSymmetric,-1);
   $self->merge( ArrowStyle => \%arrow_style );
   $self->EdgeStyleSymmetric=1;
   $self->merge( defined($matched_color) ? (EdgeColor => \%edge_color) : (),
		 defined($matched_style) ? (EdgeStyle => \%edge_style) : () );
}


sub add_faces {
   my ($self, $HD, $faces, $decor)=@_;
   my (@node_color, @node_style);
   my $matched_color=$decor->{NodeColor};
   my $matched_style=$decor->{NodeStyle};

   for (my ($n,$last)=(1, $HD->ADJACENCY->nodes-2); $n<=$last; ++$n) {
      foreach my $face (@$subcomplex) {
	 if ($HD->FACES->[$n]==$face) {
	    $node_color[$n]=$matched_color if defined $matched_color;
	    $node_style[$n]=$matched_style if defined $matched_style;
	 }
      }
   }

   $self->merge( defined($matched_color) ? (NodeColor => \@node_color) : (),
		 defined($matched_style) ? (NodeStyle => \@node_style) : () );
}


sub add_subcomplex {
   my ($self, $HD, $subcomplex, $decor)=@_;

   # show_filter: mark HD faces including faces of the given subcomplex
   # !show_filter: mark faces of the subcomplex and their subfaces
   my $want_incl= delete $decor->{show_filter} ? 1 : -1;

   my ($marked_nodes, $marked_edges)=$self->add_decor_filters($HD->ADJACENCY, $decor);

   for (my ($n,$last)=(1, $HD->ADJACENCY->nodes-2); $n<=$last; ++$n) {
      foreach my $face (@$subcomplex) {
	 my $incl=incl($HD->FACES->[$n], $face);
	 next if $incl && $incl!=$want_incl;

	 if (defined $marked_nodes) {
	    $marked_nodes->{$n}=1;
	 }
	 if (defined $marked_edges) {
	    if ($want_incl>0) {
	       for (my $e=args::entire($HD->ADJACENCY->out_edges($n)); $e; ++$e) {
		  $marked_edges->{$$e}=1;
	       }
	    } else {
	       for (my $e=args::entire($HD->ADJACENCY->in_edges($n)); $e; ++$e) {
		  $marked_edges->{$$e}=1;
	       }
	    }
	 }
      }
   }
}


1;

# Local Variables:
# mode: perl
# c-basic-offset:3
# End:
