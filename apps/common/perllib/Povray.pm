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

use Visual::Drawing;

################################################################################
#
# A povray file
#
package Povray::File;

use Polymake::Struct (
   [ '@ISA' => 'Visual::Drawing' ],
   [ '$extents' => 'new Matrix<Float>()' ],
);

sub header {
   my ($self)=@_;
   my $who=$ENV{USER};
   my $when=localtime();
   my $title=$self->title;
   if (!length($title)) {
      $title="unnamed";
   }
   my $text = "/*\n  This povray file was created using polymake.\n  It may be rendered using the following command:\n\n";
   $text .= "  >povray +Iinput.pov +Ooutput.tga $Polymake::Resources/povray/polypov.ini\n*/\n";
   $text .= "#include \"$Polymake::Resources/povray/polymake-scene.pov\"\n\n";

   $text .= "#declare MIN_EXTENT = <".Visual::print_coords($self->extents->row(0),",").">;\n";
   $text .= "#declare MAX_EXTENT = <".Visual::print_coords($self->extents->row(1),",").">;\n";
   $text .= "#declare ORIGIN     = MIN_EXTENT + (MAX_EXTENT-MIN_EXTENT)/2;\n";
   $text .= "#declare SCALE      = 1/vlength((MAX_EXTENT-MIN_EXTENT)/2);\n\n";
}

sub trailer { "" }

sub append {
   my $self=shift;
   $self->SUPER::append(@_);
   foreach my $geometry (@_) {
      extend_bounding_box($self->extents, bounding_box($geometry->source->Points));
   }
}

##############################################################################################
#
#  Basis class for all graphical objects handled by povray
#
package Povray::PointSet;
use Polymake::Struct (
   [ new => '$' ],
   [ '$source' => '#1' ],
   [ '$name' => '#1 ->Name' ],
);

sub new {
   my $self=&_new;
   $self->name =~ s|[\s.,;{}/-]|_|g;
   $self->name =~ s/\#/nr/g;
   $self->name =~ s/^(?=\d)/n_/;
   $self
}

sub header { "// ".(shift)->source->Name." ---BEGIN\n" }

sub trailer { "// ".(shift)->source->Name." ---END\n" }

sub pointColor {
   "texture { T_Polytope_nodes pigment { color rgb <" . (shift)->toFloat(",") . "> } }"
}

sub edgeColor {
   "texture { T_Polytope_wires pigment { color rgb <" . (shift)->toFloat(",") . "> } }"
}

sub pointsToString {
   my ($self)=@_;
   return "" if $self->source->Hidden;
   my $name=$self->name;
   my $n_points=@{$self->source->Points};
   my $text = "#declare vertex_list_$name = array[$n_points] {\n" .
              join(",\n", map { "<" . Visual::print_coords($_,",") . ">" } @{$self->source->Points}) .
              "}\n\n";

   my $style=$self->source->PointStyle;
   if (is_code($style) || $style !~ $Visual::hidden_re) {
      my $thickness=$self->source->PointThickness;
      my $color=$self->source->PointColor;
      my $diff_colors=is_code($color);
      $text .= "#declare vertices_of_$name = object {\n";
      unless (is_code($thickness)) {
         ($thickness ||= 1.0) *= $points_thickness;
         $text .= "  #local pth = $thickness/SCALE;\n";
      }
      $text .= "  union {\n";
      foreach my $i (0..$n_points-1) {
         next if is_code($style) && $style->($i) =~ $Visual::hidden_re;
         $text .= "    sphere { vertex_list_${name}[$i], ";
         if (is_code($thickness)) {
            $text .= (($thickness->($i) || 1.0)*$points_thickness)."/SCALE";
         } else {
            $text .= "pth";
         }
         $text .= " ".pointColor($color->($i)) if $diff_colors;
         $text .= " }\n";
      }
      if ($diff_colors) {
         $text .= "}}\n";
      } else {
         $text .= "  }\n  ".pointColor($color)."\n}\n";
      }
      $text .= <<".";
object {
  vertices_of_$name
  translate -ORIGIN
  scale SCALE
}

.
   }
   $text;
}

sub toString {
   my ($self)=@_;
   $self->header . $self->pointsToString . $self->trailer;
}

##############################################################################################
#
#  Solid 2-d or 3-d body
#
package Povray::Solid;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
);

sub facetColor {
   my ($c, $t)=@_;
   $t+=0;
   "texture { T_Polytope_solid pigment { color rgbt <" . $c->toFloat(",") . ",$t> } }"
}

sub facesToString {
   my ($self) = @_;
   my $text="";
   return $text if $self->source->Hidden;
   my $style=$self->source->FacetStyle;
   my $name=$self->name;

   if (is_code($style) || $style !~ $Visual::hidden_re) {
      my $color=$self->source->FacetColor;
      my $transp=$self->source->FacetTransparency;
      my $diff_colors=is_code($color) || is_code($transp);

      $text .= <<".";
#declare faces_${name} = object {
  union {
.
      my $i=0;
      foreach my $face (@{$self->source->Facets}) {
         next if is_code($style) && $style->($i) =~ $Visual::hidden_re;
         $text .= "    polygon { " . join(", ", @$face+1, map { "vertex_list_${name}[$_]" } (@$face, $face->[0]));
         if ($diff_colors) {
            $text .= " ".facetColor( is_code($color) ? $color->($i) : $color,
                                     is_code($transp) ? $transp->($i) : $transp );
         }
         $text .= " }\n";
      } continue { ++$i }
      if ($diff_colors) {
         $text .= "}}\n";
      } else {
         $text .= "  }\n  ".facetColor($color,$transp)."\n}\n";
      }

      $text .= <<".";
object {
  faces_$name
  translate -ORIGIN
  scale SCALE
}

.
   }

   # add cylinders for edges
   $style=$self->source->EdgeStyle;
   if (is_code($style) || $style !~ $Visual::hidden_re) {
      my $thickness=$self->source->EdgeThickness;
      ($thickness ||= 1.0) *= $lines_thickness;
      $text .= <<".";
#declare edges_${name} = object {
  #local eth = $thickness/SCALE;
  union {
.
      foreach my $face (@{$self->source->Facets}) {
         for (my $i=$#$face; $i>=0; --$i) {
            $text .= "    capsule ( vertex_list_${name}[$face->[$i]], vertex_list_${name}[$face->[$i-1]], eth )\n";
         }
      }
      $text .= "  }\n  ".edgeColor($self->source->EdgeColor)."\n}\n" . <<".";
object {
  edges_$name
  translate -ORIGIN
  scale SCALE
}

.
   }

   $text;
}

sub toString {
   my ($self)=@_;
   $self->header . $self->pointsToString . $self->facesToString . $self->trailer;
}

##############################################################################################
#
#  Wire model (e.g. a graph)
#
package Povray::Wire;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
);

sub linesToString {
   my ($self)=@_;
   my $text="";
   my $style=$self->source->EdgeStyle;
   return $text if $self->source->Hidden || !is_code($style) && $style =~ $Visual::hidden_re;
   my $name=$self->name;
   my $color=$self->source->EdgeColor;
   my $arrows=$self->source->ArrowStyle;
   my $thickness=$self->source->EdgeThickness;
   $text .= "#declare lines_${name} = object {\n";
   unless (is_code($thickness)) {
      ($thickness ||= 1.0) *= $lines_thickness;
      $text .= "  #local lth = $thickness/SCALE;\n";
   }
   if (defined $arrows) {
      $text .= <<".";
  #local al = $headLength;
  #local aw = $headWidth;
.
   }
   $text .= "  union {\n";
   for (my $e=$self->source->all_edges; $e; ++$e) {
      my $th= is_code($thickness) ? (($thickness->($e) || 1.0)*$lines_thickness)."/SCALE" : "lth";
      my @v=@$e;
      $text .= "    object { ";
      if (my $arrow= is_code($arrows) ? $arrows->($e) : $arrows) {
         @v=reverse(@v) if $arrow<0;
         $text .= "arrow(vertex_list_${name}[$v[0]], vertex_list_${name}[$v[1]], $th, al, aw)";
      } else {
         $text .= "capsule (vertex_list_${name}[$v[0]], vertex_list_${name}[$v[1]], $th)";
      }
      if (is_code($color)) {
         $text .= " ".edgeColor($color->($e));
      }
      $text .= " }\n";
   }
   if (is_code($color)) {
      $text .= "}}\n";
   } else {
      $text .= "  }\n  ".edgeColor($color)."\n}\n";
   }
   $text .= <<".";
object {
  lines_$name
  translate -ORIGIN
  scale SCALE
}

.
}

sub toString {
   my ($self)=@_;
   $self->header . $self->pointsToString . $self->linesToString . $self->trailer;
}

1

# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
