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

package JReality::File;

use Polymake::Struct (
   [ new => '$;$' ],
   [ '$title' => '#1' ],
   '@geometries',
   [ '$unnamed' => '0' ],
   [ '$obj_name' => '"geom" . #2' ],
   [ '$transform' => 'undef' ],
);

sub append {
   my $self=shift;
   push @{$self->geometries}, @_;
   foreach (@_) {
      if (length($_->name)) {
         $self->title ||= $_->name;
      } else {
         $_->name="unnamed__" . ++$self->unnamed;
      }
   }
}

sub header {
   my $self=shift;
   my $title = (defined($self->title))?$self->title:"unnamed";
   my $obj_name=$self->obj_name;
   return <<".";
/*--------------------HEADER $title--------------------*/
import de.jreality.shader.CommonAttributes;
setAccessibility(true);

synchronized(root) {

$obj_name = new SceneGraphComponent();
$obj_name.setName("$title");
root.addChild($obj_name);
/*----------------------------------------*/
.
}

sub trailer {
   return &print_transformation . <<".";
root.notify();
} 
return;
.
}

sub toString {
   my ($self)=@_;
   my $number=0;
   $self->header . join("", map { $_->toString($number++, $self->obj_name) } @{$self->geometries}) . $self->trailer;
}

sub print_transformation {
   my ($self)=@_;
   my $text="";
   if (defined (my $tr=$self->transform)) {
      $text .= "tr=new double[]{\n " . join(",", @{concat_rows($tr)}[5,6,7,1, 9,10,11,2, 13,14,15,3, 4,8,12,0]) . " };\n";
      $text .= $self->obj_name . ".setTransformation(new Transformation(\"content trafo\", tr));\n";
   }
   $text
}

##############################################################################################
#
#  Basis class for all graphical objects handled by jreality
#

package JReality::PointSet;
use Polymake::Struct (
   [ new => '$' ],
   [ '$source' => '#1' ],
   [ '$name' => '#1 ->Name || "geometry"' ],
);

sub appearance {
   my ($self, $obj_name)=@_;
   my $point_style=$self->source->VertexStyle;
   my $show_points= is_code($point_style) || $point_style !~ $Visual::hidden_re ? "true" : "false";
   is_object($JReality::default::points_color) or $JReality::default::points_color=Visual::get_RGB($JReality::default::points_color);

   my $text=<<".";
appearance = new Appearance();
appearance.setAttribute(CommonAttributes.VERTEX_DRAW, $show_points);
appearance.setAttribute(CommonAttributes.SPHERES_DRAW, $show_points);
appearance.setAttribute(CommonAttributes.DEPTH_FUDGE_FACTOR,0.999999);
.
   my $points_color="(float)".$JReality::default::points_color->toFloat(",(float)");
   $text .= <<".";
appearance.setAttribute(CommonAttributes.POINT_SHADER +"."+ CommonAttributes.DIFFUSE_COLOR,
  new java.awt.Color($points_color));
.

  $text .= <<".";
$obj_name.setAppearance(appearance);
.
   return $text;
}


sub add_vertex_colors {
   my ($self,$factory_name) = @_;
   my $color=$self->source->VertexColor;
   my $different_colors=is_code($color);
   if(!defined($color)) {
     $color = $JReality::default::points_color;
   }
   

   my $text;
#   if($different_colors) {
      $text=<<'.';
colors = new double[]{
.
      $text .= join(",\n", map { "  ".( is_code($color) ? $color->($_) : $color)->toFloat(",") } 0..$#{$self->source->Vertices});
      $text .= "};\n";
      $text .= "$factory_name.setVertexColors(new DoubleArrayArray.Inlined(colors, 3) );\n";
#   } else {
#      my $points_color="(float)".$color->toFloat(",(float)");
#      $text .= <<".";
# appearance.setAttribute(CommonAttributes.POINT_SHADER +"."+ CommonAttributes.DIFFUSE_COLOR,
#   new java.awt.Color($points_color));
# .
#    }
   return $text;
}

sub add_vertex_thickness {
   my ($self,$factory_name) = @_;
   my $thickness=$self->source->VertexThickness;
   my $text = <<".";
pts_thicknesses = new double[] {
.
   $text .= join(",", map { 
      (defined($thickness)?
          (is_code($thickness) ? &$thickness($_) : $thickness) : 1
      ) * $JReality::default::points_thickness
   } 0..$#{$self->source->Vertices});
   $text .= <<".";
 };
$factory_name.setVertexRelativeRadii( pts_thicknesses );
.
   return $text;
}

sub add_vertex_labels {
   my ($self,$factory_name) = @_;
   my $name=$self->name;
   my $labels = $self->source->VertexLabels;
   my $text = "labels = new String[]{";
   if (defined($labels)) {
      $text .= join(",", map { '"'.$labels->($_).'"' } 0..$#{$self->source->Vertices});
   }
   $text .= "};\n$factory_name.setVertexLabels(labels);\n";
   return $text;
}

sub add_vertex_coordinates {
   my ($self,$factory_name)=@_;
   my $name=$self->name;
   my $n_points = @{$self->source->Vertices};
   my $text=<<".";
/*--------------------$name--------------------*/
$factory_name.setVertexCount($n_points);
pts = new double[][]{
.
   $text .= join(",\n  ", map { ref($_) ? "{".Visual::print3dcoords($_,",")."}" : "{$_, 0, 0}" } @{$self->source->Vertices});
   $text .= <<".";
};//pts
$factory_name.setVertexCoordinates(pts);
.
}

sub vertices_to_string {
   my ($self,$factory_name)=@_;
   my $name=$self->name;
   my $n_points = @{$self->source->Vertices};
   
   my $text .= $self->add_vertex_coordinates($factory_name);
   $text .= $self->add_vertex_colors($factory_name);
   $text .= $self->add_vertex_thickness($factory_name);
   $text .= $self->add_vertex_labels($factory_name) if (defined($self->source->VertexLabels));
   return $text;
}

sub create_PointSetFactory {
  my ($self,$factory_name) = @_;
  return "$factory_name = new PointSetFactory();\n";
}

# use only for displaying pointsets
sub trailer {
   my ($self, $number, $obj_name,$factory_name)=@_;
   my $name = $self->name;
   my $text .= <<".";
$factory_name.update();
part1=new SceneGraphComponent();
part1.setGeometry($factory_name.getGeometry());
part1.setName("$name");
part1.setAppearance(appearance);
$obj_name.addChild(part1);
/*----------------------------------------*/
.
}

sub toString {
  my ($self, $number, $obj_name)=@_;
  $self->appearance($obj_name) . 
  $self->create_PointSetFactory("psf") . 
  $self->vertices_to_string("psf") . 
  $self->trailer($number, $obj_name,"psf");
}

##############################################################################################
#
#  Wire model (e.g. a graph)
#
package JReality::Wire;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
);

sub appearance {
   my ($self, $obj_name)=@_;
   my $text = $self->SUPER::appearance($obj_name);

   my $edge_style=$self->source->EdgeStyle;
   my $show_edges= is_code($edge_style) || $edge_style !~ $Visual::hidden_re ? "true" : "false";
   my $edge_color=$self->source->EdgeColor;
   is_object($JReality::default::lines_color) or $JReality::default::lines_color=Visual::get_RGB($JReality::default::lines_color);
   $edge_color="(float)".brighten_black_line(is_code($edge_color) ? undef : $edge_color)->toFloat(",(float)");

   $text .= <<".";
appearance.setAttribute(CommonAttributes.TUBES_DRAW,$show_edges);
appearance.setAttribute(CommonAttributes.EDGE_DRAW,$show_edges);
.
   my $edges_color="(float)".$JReality::default::lines_color->toFloat(",(float)");
      $text .= <<".";
appearance.setAttribute(CommonAttributes.LINE_SHADER +"."+ CommonAttributes.DIFFUSE_COLOR,
  new java.awt.Color($edges_color));
.
}

sub JReality::brighten_black_line {
   my $c=shift;
   defined($c) && ($c->[0] || $c->[1] || $c->[2]) ? $c : $JReality::default::lines_color;
}

sub add_line_color {
   my ($self,$factory_name)=@_;
   my $color=$self->source->EdgeColor;
   my $different_colors=is_code($color) or
   $color=brighten_black_line($color);
   my $text;
   if($different_colors) {
      $text=<<'.';
lineColors = new double[]{
.
      for (my $e=$self->source->all_edges; $e; ++$e) {
         $text.= "  ".($different_colors ? brighten_black_line($color->($e)) : $color)->toFloat(",").",\n";
      }
      substr($text,-2)=<<'.';

};
.
      $text .= "$factory_name.setEdgeColors( new DoubleArrayArray.Inlined(lineColors, 3) );\n";
      
   } else {
      my $edges_color="(float)".brighten_black_line($color)->toFloat(",(float)");
      $text .= <<".";
appearance.setAttribute(CommonAttributes.LINE_SHADER +"."+ CommonAttributes.DIFFUSE_COLOR,
  new java.awt.Color($edges_color));
.
   }
   return $text;
}

sub add_line_thickness {
   my ($self,$factory_name)=@_;
   my $thickness=$self->source->EdgeThickness;
   my @thicknesses = ();
   my $text =<< ".";
lines_thicknesses = new double[] {
.
   $text .= join(",", map { 
      (defined($thickness)?
          (is_code($thickness) ? &$thickness($_) : $thickness) : 1
      ) * $JReality::default::lines_thickness 
   } 0..($self->source->n_edges-1));
   $text .= <<".";
 };
$factory_name.setEdgeRelativeRadii( lines_thicknesses );
.
   return $text;
}

sub add_line_labels {
  my ($self,$factory_name)=@_;
  my $labels = $self->source->EdgeLabels;
  my $text = "line_labels = new String[] {\n  ";
  my @labels = ();
  for (my $e=$self->source->all_edges; $e; ++$e) {
    if (defined($labels)) {
      push @labels,$labels->($e);
    }
  }
  map { $_ = "\"$_\"" } @labels;
  $text .= join(",\n  ",@labels) . <<".";
};
$factory_name.setEdgeLabels(line_labels);
.
  return $text;
}

sub lines_to_string {
  my ($self,$factory_name)=@_;
  my $n_lines = $self->source->n_edges;
  my $text= <<".";
$factory_name.setEdgeCount($n_lines);
lines = new int[][]{
.
   my @lines = ();
   for (my $e=$self->source->all_edges; $e; ++$e) {
      my ($s,$t)=@$e;
      push @lines, "  { $t,$s }";
   }
   $text.= join(",\n",@lines);
   $text .=<<'.';
};
.
   $text .=<<".";
$factory_name.setEdgeIndices(lines);
.

   $text .= $self->add_line_color("ilsf");
   $text .= $self->add_line_thickness("ilsf");
   $text .= $self->add_line_labels($factory_name) if (defined($self->source->EdgeLabels));
   $text;
}

sub create_LineSetFactory {
  my ($self,$factory_name) = @_;
  return "$factory_name = new IndexedLineSetFactory();\n";
}

sub trailer {
   my ($self, $number, $obj_name, $factory_name) = @_;
   my $name = $self->name;
   my $text = <<".";
$factory_name.update();
part1=new SceneGraphComponent();
part1.setGeometry($factory_name.getGeometry());
part1.setAppearance(appearance);
part1.setName("$name");
$obj_name.addChild(part1);
/*----------------------------------------*/
.
}

sub toString {
   my ($self, $number, $obj_name)=@_;
   $self->appearance($obj_name) .
     $self->create_LineSetFactory("ilsf") .
     $self->vertices_to_string("ilsf") . 
     $self->lines_to_string("ilsf") . 
     $self->trailer($number, $obj_name,"ilsf");
}


################################################################
#
#  Solid 2-d or 3-d body
#

package JReality::Solid;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ]
);

sub create_FaceSetFactory {
  my ($self,$factory_name) = @_;
  return "$factory_name = new IndexedFaceSetFactory();\n";
}

sub add_face_colors {
   my ($self,$factory_name)=@_;
   my $color=$self->source->FacetColor;
   my $different_colors=is_code($color);
   if(!defined($color)) {
      $color = $JReality::default::faces_color;
   }

   my $text;
#   if($different_colors) {
      $text=<<'.';
faceColors = new double[]{
.
      $text .= join(",\n", map { "  ".($different_colors ? $color->($_) : $color)->toFloat(",") } 0..$#{$self->source->Facets}) . <<'.';

};
.
      $text .= "$factory_name.setFaceColors( new DoubleArrayArray.Inlined(faceColors, 3));\n";
#    } else {
#       my $faces_color="(float)".$color->toFloat(",(float)");
#       $text .= <<".";
# appearance.setAttribute(CommonAttributes.POLYGON_SHADER +"."+ CommonAttributes.DIFFUSE_COLOR,
#   new java.awt.Color($faces_color));
# .
#    }
   return $text;
}

sub add_face_labels {
   my ($self,$factory_name) = @_;
   my $name=$self->name;
   my $labels = $self->source->FacetLabels;
   my $text = "facetLabels = new String[]{";
   if (defined($labels)) {
      $text .= join(",", map { '"'.$labels->($_).'"' } 0..$#{$self->source->Facets});
   } else {
      $text .= join(",", map { '"'.$_.'"' } 0..$#{$self->source->Facets});
   }
   $text .= "};\n$factory_name.setFaceLabels(facetLabels);\n";
   return $text;
}


sub add_lines_thicknesses {
   my ($self,$factory_name)=@_;
   my $n_edges = $self->source->NEdges;
   my $thickness=$self->source->EdgeThickness;
   my @thicknesses = ();
   my $text =<< ".";
lines_thicknesses = new double[] {
.
   $text .= "  ".join(",", map { 
       (defined($thickness)?
        (is_code($thickness) ? &$thickness($_) : $thickness) : 1
       ) * $JReality::default::lines_thickness 
   } 0..($n_edges-1));
   $text .= <<".";
 };
edges = IndexedFaceSetUtility.edgesFromFaces(faces);
$factory_name.setEdgeCount(edges.length);
$factory_name.setEdgeIndices(edges);
$factory_name.setEdgeRelativeRadii( lines_thicknesses );
.
   return $text;
}


sub appearance {
   my ($self, $obj_name) = @_;
   my $text = $self->SUPER::appearance($obj_name);

   is_object($JReality::default::faces_color) or $JReality::default::faces_color=Visual::get_RGB($JReality::default::faces_color);
   is_object($JReality::default::lines_color) or $JReality::default::lines_color=Visual::get_RGB($JReality::default::lines_color);

   my $facet_style=$self->source->FacetStyle;
   my $edge_style=$self->source->EdgeStyle;
   my $show_faces= is_code($facet_style) || $facet_style !~ $Visual::hidden_re ? "true" : "false";
   my $show_edges= $edge_style !~ $Visual::hidden_re ? "true" : "false";

   $text .= <<".";
appearance.setAttribute(CommonAttributes.POLYGON_SHADER +"."+CommonAttributes.SMOOTH_SHADING,false);
appearance.setAttribute(CommonAttributes.FACE_DRAW, $show_faces);
appearance.setAttribute(CommonAttributes.TUBES_DRAW,$show_edges);
appearance.setAttribute(CommonAttributes.EDGE_DRAW,$show_edges);
.

   my $transp=$self->source->FacetTransparency;
   if (defined($transp) && !is_code($transp)) {
      $text .= <<".";
appearance.setAttribute(CommonAttributes.OPAQUE_TUBES_AND_SPHERES, true);
appearance.setAttribute(CommonAttributes.TRANSPARENCY_ENABLED,true);
appearance.setAttribute(CommonAttributes.POLYGON_SHADER +"."+ CommonAttributes.TRANSPARENCY,$transp);
.
   }

   my $edges_color="(float)".brighten_black_line($self->source->EdgeColor)->toFloat(",(float)");
      $text .= <<".";
appearance.setAttribute(CommonAttributes.LINE_SHADER +"."+ CommonAttributes.DIFFUSE_COLOR,
  new java.awt.Color($edges_color));
.
   my $faces_color="(float)".$JReality::default::faces_color->toFloat(",(float)");
      $text .= <<".";
appearance.setAttribute(CommonAttributes.POLYGON_SHADER +"."+ CommonAttributes.DIFFUSE_COLOR,
  new java.awt.Color($faces_color));
.


}

sub faces_to_string {
   my ($self,$factory_name) = @_;
   my $n_faces = @{$self->source->Facets};
   my $text .=<<".";
$factory_name.setGenerateFaceNormals(true);
$factory_name.setGenerateVertexNormals(true);
$factory_name.setFaceCount($n_faces);
.
   $text.=<<".";
faces = new int[][]{
.
   $text .= join(",\n", map { "  { ".join(",", @$_)." }" } @{$self->source->Facets});
   $text .= <<".";
};
$factory_name.setFaceIndices(faces);
.
   $text .= $self->add_face_colors("ifsf");
   $text .= $self->add_face_labels("ifsf") if (defined($self->source->FacetLabels));
   $text .= $self->add_lines_thicknesses("ifsf");
}

sub trailer {
   my ($self, $number, $obj_name,$factory_name)=@_;
   my $name = $self->name;

   my $text .= <<".";
$factory_name.update();
part1=new SceneGraphComponent();
part1.setGeometry(ifsf.getGeometry());
part1.setName("$name");
part1.setAppearance(appearance);
$obj_name.addChild(part1);
/*----------------------------------------*/
.
}

sub toString {
   my ($self, $number, $obj_name)=@_;
   $self->appearance($obj_name) .
      $self->create_FaceSetFactory("ifsf") .
      $self->vertices_to_string("ifsf") . 
      $self->faces_to_string("ifsf") .
      $self->trailer($number, $obj_name,"ifsf");
}


1

# Local Variables:
# mode:perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
