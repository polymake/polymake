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

package Graphviz::File;

use Polymake::Struct (
   [ '$title' => 'undef' ],
   '$Graph',
   '$reverse_arrows',
   '$type',
   '$edge_symbol',
   '$options',
   '$command',
);

sub process_fill_color {
   my ($attrs, $color)=@_;
   if (defined $color) {
      push @{$attrs->{style}}, "filled";
      $attrs->{fillcolor}='"'.$color->toHex.'"';
   }
}

sub process_color {
   my ($attrs, $color)=@_;
   if (defined $color) {
      $attrs->{color}='"'.$color->toHex.'"';
   }
}

sub process_line_thickness {
   my ($attrs, $thickness)=@_;
   if (defined $thickness) {
      push @{$attrs->{style}}, "setlinewidth($thickness)";
   }
}

sub process_box_thickness {
   my ($attrs, $thickness)=@_;
   if (defined $thickness) {
      $attrs->{fontsize}=int($Postscript::fontsize*$thickness);
   }
}

sub process_style {
   my ($attrs, $style)=@_;
   if ($style =~ $Visual::hidden_re) {
      $attrs->{style}=[ "invis" ];
   }
}

sub process_dir {
   my ($attrs, $dir)=@_;
   $attrs->{dir}= $dir>0 ? "forward" : $dir<0 ? "back" : "none";
}

sub attrs2text {
   my $attrs=shift;
   join(", ", map {
      "$_=" . do {
         my $val = $attrs->{$_};
         is_array($val) ? '"'.join(",", @$val).'"' : $val
      }
   # We sort the keys to have consistent output for all perl versions (especially >=5.18)
   } sort keys %$attrs)
}

sub toString {
   my ($self)=@_;

   my %node_attrs=( shape=>"box", height=>0.1, width=>0.1 );
   my %edge_attrs;
   my $node_color=$self->Graph->NodeColor;
   if (instanceof RGB($node_color)) {
      process_fill_color(\%node_attrs, $node_color);
      undef $node_color;
   }
   my $node_border_color=$self->Graph->NodeBorderColor;
   if (instanceof RGB($node_border_color)) {
      process_color(\%node_attrs, $node_border_color);
      undef $node_border_color;
   }
   my $node_thickness=$self->Graph->NodeThickness;
   if (!ref($node_thickness)) {
      process_box_thickness(\%node_attrs, $node_thickness);
      undef $node_thickness;
   }
   my $node_border_thickness=$self->Graph->NodeBorderThickness;
   if (!ref($node_border_thickness)) {
      process_line_thickness(\%node_attrs, $node_border_thickness);
      undef $node_border_thickness;
   }
   my $node_style=$self->Graph->NodeStyle;
   if (!ref($node_style)) {
      process_style(\%node_attrs, $node_style);
      undef $node_style;
   }
   my $edge_color=$self->Graph->EdgeColor;
   if (instanceof RGB($edge_color)) {
      process_color(\%edge_attrs, $edge_color);
      undef $edge_color;
   }
   my $edge_thickness=$self->Graph->EdgeThickness;
   if (!ref($edge_thickness)) {
      process_line_thickness(\%edge_attrs, $edge_thickness);
      undef $edge_thickness;
   }
   my $edge_style=$self->Graph->EdgeStyle;
   if (!ref($edge_style)) {
      process_style(\%edge_attrs, $edge_style);
      undef $edge_style;
   }
   my $arrows=$self->Graph->ArrowStyle;
   if (!ref($arrows)) {
      process_dir(\%edge_attrs, $arrows);
      undef $arrows;
   }

   my $text= $self->type . ' "' . $self->title . "\" {\n" .
             "  fontname=\"$Postscript::fontname\"; " . $self->options . ";\n" .
             "  node [" . attrs2text(\%node_attrs) . "];\n";
   if (keys %edge_attrs) {
      $text.="  edge [" . attrs2text(\%edge_attrs) . "];\n";
   }

   my $node_labels=$self->Graph->NodeLabels;
   my $edge_labels=$self->Graph->EdgeLabels;

   for (my ($n, $end)=(0, $self->Graph->n_nodes); $n<$end; ++$n) {
      %node_attrs=();
      if (defined($node_labels)) {
         $node_attrs{label}='"'.$node_labels->($n).'"';
      }
      if (defined($node_color)) {
         process_fill_color(\%node_attrs, $node_color->($n));
      }
      if (defined($node_border_color)) {
         process_color(\%node_attrs, $node_border_color->($n));
      }
      if (defined($node_thickness)) {
         process_box_thickness(\%node_attrs, $node_thickness->($n));
      }
      if (defined($node_border_thickness))  {
         process_line_thickness(\%node_attrs, $node_border_thickness->($n));
      }
      if (defined($node_style)) {
         process_style(\%node_attrs, $node_style->($n));
      }
      $text.="  n$n" . (keys %node_attrs ? " [" . attrs2text(\%node_attrs) . "];\n" : ";\n");
   }

   for (my $e=$self->Graph->all_edges; $e; ++$e) {
      %edge_attrs=();
      if (defined($edge_labels)) {
         $edge_attrs{label}='"'.$edge_labels->($e).'"';
      }
      if (defined($edge_color)) {
         process_color(\%edge_attrs, $edge_color->($e));
      }
      if (defined($edge_thickness)) {
         process_line_thickness(\%edge_attrs, $edge_thickness->($e));
      }
      if (defined($edge_style)) {
         process_style(\%edge_attrs, $edge_style->($e));
      }
      if (defined($arrows)) {
         process_dir(\%edge_attrs, $arrows->($e));
      }
      my ($s,$t)=$self->reverse_arrows ? reverse(@$e) : @$e;
      $text.="  n$s " . $self->edge_symbol . " n$t" .
             (keys %edge_attrs ? " [" . attrs2text(\%edge_attrs) . "];\n" : ";\n");
   }
   if (instanceof Visual::Lattice($self->Graph) && $self->Graph->Mode eq "primal") {
      if(defined($self->Graph->Dims)) {
         my $dimmap = $self->Graph->Dims;
         my $rankcommands = "";
         for(my $map_it = entire($dimmap->get_map()); $map_it; $map_it++) {
            # If a level is not connected to the next level, we need an invisible edge
            my @thislevelnodes = @{ $dimmap->nodes_of_rank($$map_it->first)};
            next if scalar(@thislevelnodes) == 0;
            my @nextlevelnodes = @{ $dimmap->nodes_of_rank($$map_it->first+1) // []};
            my $has_edge = 1;
            if(scalar(@nextlevelnodes)) {
               $has_edge = 0;
               for my $tln (@thislevelnodes) {
                  for my $nln (@nextlevelnodes) {
                     if($self->Graph->has_edge($tln, $nln)) {
                        $has_edge = 1; last;
                     }
                  }
                  last if $has_edge;
               }
            }
            if(!$has_edge) {
               $text.= "n".$nextlevelnodes[0]." -> n".$thislevelnodes[0]." [style=\"invis\"];\n";
            }
            # Tell graphviz about nodes of same rank
            $rankcommands.=("{ rank=same; ".join("; ", map { "n".$_ } @thislevelnodes)." };\n");
         }
         $text.=$rankcommands;
      }
   }

   $text.="}\n";
}

sub extract_seed {
   my $Graph=shift;
   if (is_object(my $emb=$Graph->Coord)) {
      if (instanceof Visual::GraphEmbedding($emb) && defined (my $seed=$emb->options->{seed})) {
         return "; start=$seed";
      }
   }
   "";
}

sub addGraph {
   my ($self, $Graph)=@_;
   $self->Graph=$Graph;
   if (ref($Graph->ArrowStyle) || $Graph->ArrowStyle) {
      $self->type="digraph";
      $self->edge_symbol="->";
   } else {
      $self->type="graph";
      $self->edge_symbol="--";
   }
   $self->options="overlap=scale; nodesep=1" . extract_seed($Graph);
   $self->command=$neato;
}

sub addLattice {
   my ($self, $Lattice)=@_;
   $self->Graph=$Lattice;
   $self->reverse_arrows=$Lattice->Mode eq "primal";
   $self->type="digraph";
   $self->edge_symbol="->";
   $self->options="ranksep=2.5";
   $self->command=$dot;
}

1

# Local Variables:
# mode: perl
# cperl-indent-level: 3
# indent-tabs-mode:nil
# End:
