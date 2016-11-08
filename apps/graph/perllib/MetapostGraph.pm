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

use strict;

package Metapost::File;         # siehe Visual.pm : visualize_explicit()

use Polymake::Struct (
   [ '$title' => 'undef' ],
   '@figures',
   [ '$unnamed' => '0' ],
);


sub new_drawing {
   my ($self, $title)=@_;
   push @{$self->figures}, new Figure( scalar(@{$self->figures}), $title );
   $self;
}

sub append {
   my $self = shift;
   $self->figures->[-1]->append(@_);
}


sub header {
   my ($self) = @_;
   my $who=$ENV{USER};
   my $when=localtime();
   my $title=$self->title;
   if (!length($title) && @{$self->figures}) {
      $title=$self->figures->[0]->title || "unnamed";
   }
   return <<".";
% produced by polymake for $who
% date: $when
% $title

verbatimtex
%&latex
\\documentclass{article}
\\usepackage{amsmath,amssymb,amsfonts}
\\begin{document}
etex

outputtemplate := "%j-%c.mps";

.
}

sub trailer {
   return <<".";

end
% EOF
.
}

my $global_tuning_params=<<'.';
% general
  numeric u, normalpen, mediumpen, thickpen, dotsize;
  u         := 1cm; % global unit parameter, reduce this if mpost issues '! Value is too large'
  normalpen := .5pt;
  mediumpen := 1pt;
  thickpen  := 2pt;
  dotsize   := 5pt;
.

sub toString {
   my $self=shift;
   my $preamble=$self->header;
   my $content=join("", map { $_->toString } @{$self->figures});
   my (%macros, %tuning);
   foreach my $fig (@{$self->figures}) {
      foreach (@{$fig->elements}) {
         push %macros, %{$_->macros};
         push %tuning, %{$_->tuning};
      }
   }
   $tuning{default} ||= $global_tuning_params;
   if (keys %tuning) {
      # We sort the values to have consistent output for all perl versions (especially >=5.18)
      $preamble .= "% tuning parameters\n" . join("\n", sort values %tuning) . "% end of tuning parameters\n\n";
   }
   if (keys %macros) {
      # We sort the values to have consistent output for all perl versions (especially >=5.18)
      $preamble .= "% macro definitions\n" . join("\n", sort values %macros) . "% end of macro definitions\n\n";
   }
   $preamble . $content . $self->trailer;
}

##############################################################################################
#
#  A Metapost figure
#
package Metapost::Figure;

use Polymake::Struct (
   [ new => '$;$' ],
   '@elements',
   [ '$id' => '#1' ],
   [ '$title' => '#2' ],
);

sub append {
   my ($self, $el)=@_;
   push @{$self->elements}, $el;
   $self->title ||= $el->source->Name;
   $self;
}

sub header {
   my ($self)=@_;
   my $title=$self->title || "anonymous figure";
   my $id=$self->id;
   return <<".";
beginfig($id); % $title
  save p, n;
  pair p[];    % global array of vertices/nodes
  path n[];    % circles or boxes around the nodes
.
}

sub trailer
{
   return <<".";
endfig;

.
}

sub toString {
   my $self=shift;
   my $text=$self->header;
   my $n_points=0;
   foreach my $el (@{$self->elements}) {
      # we suppose all elements are derived from Metapost::Element
      $text .= $el->pointArray($n_points);
      $n_points += @{$el->source->Vertices};
   }
   $n_points=0;
   foreach my $el (@{$self->elements}) {
      $text .= $el->toString($n_points);
      $n_points += @{$el->source->Vertices};
   }
   $text . $self->trailer;
}


##############################################################################################
#
#  Basis class for all figure objects handled by Metapost
#
package Metapost::Element;

use Polymake::Struct (
   [ new => '$' ],
   [ '$source' => '#1' ],
   '%macros',
   '%tuning',
);

sub pointArray {
   my ($self, $start_index)=@_;
   my $text="\n% vertex coordinates for ".$self->source->Name."\n";
   my $k=$start_index;
   my $last_point=$start_index+$#{$self->source->Vertices};

   foreach my $point (@{$self->source->Vertices}) {
      $text .= sprintf("  p[$k] := (%.3f,%.3f)*u;\n", @$point);
      ++$k;
   }

   my $style=$self->source->VertexStyle;
   if (is_code($style) || $style !~ $Visual::hidden_re) {
      my $thickness=$self->source->VertexThickness;
      if (!is_code($style) && !is_code($thickness)) {
         # all points have the same shape
         if (defined($thickness) && $thickness!=1) {
            $thickness=($thickness*1)."*dotsize";       # ensure numeric value
         } else {
            $thickness="dotsize";
         }
         $text .= <<".";
  for i=$start_index upto $last_point:
    n[i] := fullcircle scaled($thickness) shifted p[i];
  endfor
.
      } else {
         for ($k=$start_index; $k<=$last_point; ++$k) {
            if (is_code($style) && $style->($k-$start_index) =~ $Visual::hidden_re) {
               $text .= "  n[$k] := p[$k];\n";
            } else {
               my $th=is_code($thickness) ? $thickness->($k-$start_index) : $thickness;
               if (defined($th) && $th!=1) {
                  $th=($th*1)."*dotsize";       # ensure numeric value
               } else {
                  $th="dotsize";
               }
               $text .= "  n[$k] := fullcircle scaled($th) shifted p[$k];\n";
            }
         }
      }
   }
   return $text;
}

sub draw_vertices {
   my ($self, $start_index)=@_;
   my $text="\n% points/nodes for ".$self->source->Name."\n";
   my @options;
   my $style=$self->source->VertexStyle;
   undef $style unless is_code($style);
   my $color=$self->source->VertexColor;
   my $default_color_is_fill;
   my $border_color=$self->source->VertexBorderColor;
   my $border_thickness=$self->source->VertexBorderThickness;
   my $draw_border= defined($border_color) || defined($border_thickness);
   if (defined($color) && !is_code($color)) {
      $default_color_is_fill=1;
      push @options, "withcolor(".$color->toFloat(",").")";
      undef $color;
      if (defined($border_color) && !is_code($border_color)) {
         $border_color=$border_color->toFloat(",");
      }
   } elsif (defined($border_color) && !is_code($border_color)) {
      push @options, "withcolor(".$border_color->toFloat(",").")";
      undef $border_color;
   }
   if (defined($border_thickness) && !is_code($border_thickness)) {
      push @options, "withpen pencircle scaled(".($border_thickness*1)."*normalpen)";
      undef $border_thickness;
   } elsif ($draw_border) {
      push @options, "withpen pencircle scaled normalpen";
   }
   $text .= "  drawoptions(@options);\n";

   my $last_point=$start_index+$#{$self->source->Vertices};
   if (defined($style) || defined($color) || is_code($border_color) || defined($border_thickness)) {
      # need to emit an individual draw statement for each vertex
      for (my $i=$start_index; $i<=$last_point; ++$i) {
         next if defined($style) && $style->($i-$start_index) =~ $Visual::hidden_re;
         my $bw=$border_thickness && $border_thickness->($i-$start_index);
         my $bc=is_code($border_color) ? $border_color->($i-$start_index) : $border_color;
         my $c=$color && $color->($i-$start_index);
         my $draw_this_border= defined($bw) ? $bw>0 : $draw_border;
         if ($draw_this_border && !defined($c) && !$default_color_is_fill) {
            $text .= "  unfill n[$i]";
         } else {
            $text .= "  fill n[$i]";
            if (defined($c)) { $text .= " withcolor(".$c->toFloat(",").")"; }
         }
         if ($draw_this_border) {
            $text .= ";  draw n[$i]";
            if (defined $bc) {
               $text .= " withcolor(".$bc->toFloat(",").")";
            }
            if ($bw) {
               $text .= " withpen pencircle scaled($bw*normalpen)";
            }
         }
         $text .= ";\n";
      }

   } else {
      # generate a MetaPost loop for all vertices
      $text .= "  for i=$start_index upto $last_point:\n";
      if (!$draw_border || $default_color_is_fill) {
         $text .= "    fill n[i];\n";
      } else {
         $text .= "    unfill n[i];\n";
      }
      if ($draw_border) {
         if ($default_color_is_fill) {
            $text .= "    draw n[i] withcolor($border_color);\n";
         } else {
            $text .= "    draw n[i];\n";
         }
      }
      $text .= "  endfor;\n";
   }

   return $text;
}

sub toString {
   my ($self, $start_index)=@_;
   my $style=$self->source->VertexStyle;
   if (is_code($style) || $style !~ $Visual::hidden_re) {
      my $text=&draw_vertices;

      if (defined (my $labels=$self->source->VertexLabels)) {
         $text .= "\n% vertex labels for ".$self->source->Name."\n  drawoptions(withcolor black);\n";
         my $last_point=$start_index+$#{$self->source->Vertices};
         for (my $i=$start_index; $i<=$last_point; ++$i) {
            my $l=$labels->($i-$start_index);
            if ($l=~/\S/) {
               $text .= "  label.lrt(\"$l\", lrcorner n[$i]);\n";
            }
         }
      }

      $text;
   } else {
      "\n% nodes for ".$self->source->Name." are hidden\n"
   }
}

##############################################################################################
#
#  Graph
#

package Metapost::Graph;
use Polymake::Struct (
   [ '@ISA' => 'Element' ],
);

my $arrow_tuning=<<'.';
  numeric arrow_w, arrow_l;
  arrow_w := 2pt;   % half width
  arrow_l := 8pt;   % length
.
my $graph_arrow_macro=<<'.';
  path parrow;
  parrow := origin -- (-arrow_l,-arrow_w){dir 60} .. {dir 120}(-arrow_l,arrow_w) -- cycle;

  primarydef from_node ==> to_node =
    begingroup
    save a, edge, tip;
    picture a;  path edge;  pair tip;
    edge := center from_node -- center to_node cutbefore from_node cutafter to_node;
    tip := point infinity of edge;
    a := nullpicture;
    addto a doublepath edge;
    addto a contour parrow rotated angle(tip-center from_node) shifted tip;
    a
    endgroup
  enddef;
.

sub draw_edges {
   my ($self, $start_index)=@_;
   my $text="\n% edges for ".$self->source->Name."\n";
   my @options;

   my $color=$self->source->EdgeColor;
   my $thickness=$self->source->EdgeThickness;
   if (defined($color) && !is_code($color)) {
      push @options, "withcolor(".$color->toFloat(",").")";
      undef $color;
   } else {
      push @options, "withcolor black";
   }
   if (defined($thickness) && !is_code($thickness)) {
      push @options, "withpen pencircle scaled($thickness*normalpen)";
      undef $thickness;
   } else {
      push @options, "withpen pencircle scaled normalpen";
   }

   $text .= "  drawoptions(@options);\n";

   my $labels=$self->source->EdgeLabels;
   my $style=$self->source->EdgeStyle;
   my $arrows=$self->source->ArrowStyle;
   if ($arrows) {
      $self->macros->{"graph:arrow"} ||= $graph_arrow_macro;
      $self->tuning->{"graph:arrow"} ||= $arrow_tuning;
   }

   for (my $edge=$self->source->all_edges; $edge; ++$edge) {
      next if is_code($style) && $style->($edge) =~ $Visual::hidden_re;
      my ($s,$t)=@$edge;
      $s+=$start_index; $t+=$start_index;
      my ($pen,$cl)=("","");

      if (defined($thickness) && defined (my $th=$thickness->($edge))) {
         $pen=" withpen pencircle scaled($th*normalpen)";
      }
      if (defined($color) && defined (my $c=$color->($edge))) {
         $cl=" withcolor(".$c->toFloat(",").")";
      }
      if (my $arrow= is_code($arrows) ? $arrows->($edge) : $arrows) {
         ($t,$s)=($s,$t) if $arrow<0;
         $text .= "  draw n[$s]==>n[$t]$pen$cl;";
      } else {
         $text .= "  draw p[$s]--p[$t]$pen$cl;";
      }
      if (defined($labels) && (my $l=$labels->($edge))=~/\S/) {
         $text .= "  label.lrt(\"$l\", 0.5[p[$s],p[$t]]) withcolor(black);"
      }
      $text.="\n";
   }

   return $text;
}

sub toString {
   my ($self, $start_index)=@_;
   my $style=$self->source->EdgeStyle;
   if (is_code($style) || $style !~ $Visual::hidden_re) {
      &draw_edges . $self->SUPER::toString($start_index);
   } else {
      "\n% edges for ".$self->source->Name." are hidden\n" . $self->SUPER::toString($start_index);
   }
}

##############################################################################################
#
#  Face Lattice
#

package Metapost::Lattice;
use Polymake::Struct (
   [ '@ISA' => 'Graph' ],
);

my $lattice_node_macro=<<'.';
  numeric x_k;
  def max_k_row(expr ifirst)(text ilist) =
    begingroup
    save i, j, max_k; numeric i, k, max_k;
    i := ifirst;
    max_k := 0;
    for j=ilist:
      k := (node_gap+xpart(urcorner n[i] - ulcorner n[j]))/xpart(p[j]-p[i]);
      max_k := max(max_k, k);
      i := j;
    endfor
    max_k
    endgroup
  enddef;
  def scale_nodes(expr b,e) =
    for i=b upto e:
      p[i] := (x_k*xpart p[i], y_stretch*ypart p[i]);
      n[i] := n[i] shifted p[i];
      l[i] := l[i] shifted p[i];
    endfor
  enddef;
  def lattice_node(suffix lab) =
    (ulcorner lab+(-label_gap,label_gap)) -- (urcorner lab+(label_gap,label_gap)) --
    (lrcorner lab+(label_gap,-label_gap)) -- (llcorner lab+(-label_gap,-label_gap)) -- cycle
  enddef;
.

sub pointArray {
   my ($self, $start_index)=@_;
   $self->tuning->{"lattice"} ||= <<'.';
% for face lattice drawings
  numeric y_stretch, label_gap;
  y_stretch := 50;      % vertical stretch
  label_gap := 3pt;     % between label text and surrounding box
  node_gap  := 5pt;     % between two nodes in a layer
.
   my $name=$self->source->Name;
   my $text=<<".";

% node coordinates for $name
  save l;
  picture l[];  % node labels
.
   if (is_object(my $embedding=$self->source->Coord)) {
      # expecting Visual::GraphEmbedding here
      my @label_width=map {
         # label spacing: twice char width between the boxes, half char width between border and label
         length($self->source->NodeLabels->($_))+3
      } 0..$self->source->n_nodes-1;
      $embedding->label_width=\@label_width;
      $embedding->options->{dual}= $self->source->Mode eq "dual";
   }

   my $k=$start_index;
   my $last_point=$start_index+$#{$self->source->Vertices};
   foreach my $point (@{$self->source->Vertices}) {
      $text .= sprintf("  p[$k] := (%.3f,%.3f);\n", @$point);
      ++$k;
   }
   my %sorted_by_y;
   my $style=$self->source->NodeStyle;
   my $thickness=$self->source->NodeThickness;
   for ($k=$start_index; $k<=$last_point; ++$k) {
      next if is_code($style) && $style->($k-$start_index) =~ $Visual::hidden_re;
      my ($x,$y)=@{$self->source->Coord->[$k-$start_index]};
      my $label=$self->source->VertexLabels->($k-$start_index);
      $text .= "  l[$k] := thelabel(\"$label\",origin);";
      if (is_code($thickness)) {
         if (defined (my $th=$thickness->($k-$start_index))) {
            $text .= "  n[$k] := lattice_node(l[$k]) scaled($th);\n";
         } else {
            $text .= "  n[$k] := lattice_node(l[$k]);\n";
         }
      } else {
         $text .= "\n";
      }
      push @{$sorted_by_y{$y}}, [ $x, $k ];
   }
   unless (is_code($thickness)) {
      $text .= <<".";
  for i=$start_index upto $last_point:
    n[i] := lattice_node(l[i]);
  endfor
.
   }

   $self->macros->{"lattice:node"} ||= $lattice_node_macro;
   $text .= "\n  x_k := max( "
          . join(",\n              ",
                 map {
                    "max_k_row(" . join(",", map { $_->[1] } sort { $a->[0] <=> $b->[0] } @$_) . ")"
                 # We sort the keys to have consistent output for all perl versions (especially >=5.18)
                 } grep { $#$_>0 } map { $sorted_by_y{$_} } sort keys %sorted_by_y)
          . " );\n  scale_nodes($start_index,$last_point);\n";
}

sub toString {
   my ($self, $start_index)=@_;
   my $text= &draw_edges . &draw_vertices .
      "\n% node labels for ".$self->source->Name."\n  drawoptions(withcolor black);\n";

   my $labels=$self->source->NodeLabels;
   my $last_point=$start_index+$#{$self->source->Vertices};
   $text .= <<".";
  for i=$start_index upto $last_point:
    if known l[i]: draw l[i]; fi
  endfor
.
}


1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
