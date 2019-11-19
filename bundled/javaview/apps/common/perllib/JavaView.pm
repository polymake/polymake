#  Copyright (c) 1997-2019
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

use Visual::Drawing;

package JavaView::File;
use Polymake::Struct (
   [ '@ISA' => 'Visual::Drawing' ],
);

sub header {
   my ($self) = @_;
   my $who=$ENV{USER};
   my $when=localtime();
   my $title=$self->title // "unnamed";
   return <<".";
<?xml version="1.0" encoding="utf-8" standalone="yes"?>
<!DOCTYPE jvx-model SYSTEM "http://www.javaview.de/rsrc/jvx.dtd">
<jvx-model>
  <meta generator="polymake for $who"/>
  <meta date="$when"/>
  <version type="dump">1.0</version>
  <title>$title</title>
  <geometries>
.
}

sub trailer {
   return <<".";
  </geometries>
</jvx-model>
.
}

##############################################################################################
#
#  Basis class for all graphical objects handled by javaview
#
package JavaView::PointSet;
use Polymake::Struct (
   [ new => '$' ],
   [ '$source' => '#1' ],
   [ '$name' => '#1 ->Name' ],
);

sub header {
   my ($self)=@_;
   my $attr=' name="'.$self->name.'"';
   if ($self->source->Hidden) {
      $attr .= ' visible="hide"';
   }
   return <<".";
    <geometry$attr>
.
}

sub trailer {
   return <<".";
    </geometry>
.
}

sub pointsToString {
   my ($self)=@_;
   my $d= is_object($self->source->Vertices) ? $self->source->Vertices->cols : 3;
   my $style=$self->source->VertexStyle;
   my $point_flag= is_code($style) || $style !~ $Visual::hidden_re ? "show" : "hide";
   my $thickness=$self->source->VertexThickness;
   my $thickness_flag= is_code($thickness) ? "show" : "hide";
   my $color=$self->source->VertexColor;
   my $color_flag= is_code($color) ? "show" : "hide";
   my $text= <<".";
      <pointSet dim="$d" point="$point_flag" color="$color_flag" thicknesses="$thickness_flag">
        <points>
.
   my $k=0;
   my $labels=$self->source->VertexLabels;
   foreach (@{$self->source->Vertices}) {
      # animated contents have point indices instead of real coords
      my $point=ref($_) ? Visual::print_coords($_) : "$_".(" 0"x($d-1));
      my $attr="";
      if (defined($labels)) {
         $attr=' name="' . $labels->($k) . '"';
      }
      $text .= <<".";
          <p$attr>$point</p>
.
      ++$k;
   }

   my $th= $thickness_flag eq "hide" ? ($thickness // 1)*$JavaView::default::points_thickness : $JavaView::default::points_thickness;
   $text .= <<".";
          <thickness>$th</thickness>
.

   if ($color_flag eq "hide" && defined($color)) {
      my $c=$color->toInt;
      $text .= <<".";
          <color type="rgb">$c</color>
.
   }
   my $show_labels= defined($labels) ? "show" : "hide";
   $text .= <<".";
          <labelAtt visible="$show_labels"/>
        </points>
.
   if ($color_flag eq "show") {
      $text .= <<".";
        <colors>
.
      for (my $i=0; $i<$k; ++$i) {
         my $c=$color->($i)->toInt;
         $text .= <<".";
          <c>$c</c>
.
      }
      $text .= <<'.';
        </colors>
.
   }
   if ($thickness_flag eq "show") {
      $text .= <<'.';
        <thicknesses>
.
      for (my $i=0; $i<$k; ++$i) {
         my $th=($thickness->($i) // 1)+0;
         $text .= <<".";
          <th>$th</th>
.
      }
      $text .= <<'.';
        </thicknesses>
.
   }
   $text .= <<'.';
      </pointSet>
.
}

sub toString {
   my ($self)=@_;
   $self->header . $self->pointsToString . $self->trailer;
}

##############################################################################################
#
#  Solid 2-d or 3-d body
#
package JavaView::Solid;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
);

sub facesToString {
   my ($self)=@_;

   my $style=$self->source->FacetStyle;
   my $transp=$self->source->FacetTransparency;
   if (ref($transp) || ref($style)) {
      croak( "JavaView does not support different transparency values for facets in the same geometry object" );
   }
   my $face_flag= $style !~ $Visual::hidden_re ? "show" : "hide";
   my $color=$self->source->FacetColor;
   my $color_flag= is_code($color) ? "show" : "hide";
   my $edge_style=$self->source->EdgeStyle;
   my $edge_flag= is_code($edge_style) || $edge_style !~  $Visual::hidden_re ? "show" : "hide";
   my $backface_flag= $face_flag eq "show" && $self->source->Closed && !defined($transp) ? "hide" : "show";
   my $text= <<".";
      <faceSet face="$face_flag" edge="$edge_flag" backface="$backface_flag" color="$color_flag">
        <faces>
.
   my $k=0;
   my $labels=$self->source->FacetLabels;
   foreach (@{$self->source->Facets}) {
      my $attr="";
      if (defined($labels)) {
         $attr=' name="' . $labels->($k) . '"';
      }
      $text .= <<".";
          <f$attr>@$_</f>
.
      ++$k;
   }
   if ($color_flag eq "hide" && defined($color)) {
      my $c=$color->toInt;
      $text .= <<".";
          <color type="rgb">$c</color>
.
   }
   my $show_labels= defined($labels) ? "show" : "hide";
   $text .= <<".";
          <labelAtt visible="$show_labels"/>
        </faces>
.
   if (defined($self->source->FacetNeighbors)) {
      $text .= <<".";
        <neighbours>
.
      foreach (@{$self->source->FacetNeighbors}) {
         my ($first, @rest)=@$_;        # javaview expects the neighbors in a rather weird order
         $text .= <<".";
          <nb>@rest $first</nb>
.
      }
      $text .= <<".";
        </neighbours>
.
   }
   if ($edge_flag eq "show") {
      my $c=$self->source->EdgeColor->toInt;
      $text .= <<".";
        <edges>
           <color type="rgb">$c</color>
.
      if (defined($self->source->EdgeThickness)) {
         my $th=$self->source->EdgeThickness*$JavaView::default::lines_thickness;
         $text .= <<".";
           <thickness>$th</thickness>
.
      }
      $text .= <<".";
        </edges>
.
   }
   if ($color_flag eq "show") {
      $text .= <<".";
        <colors>
.
      for (my $i=0; $i<$k; ++$i) {
         my $c=$color->($i)->toInt;
         $text .= <<".";
          <c>$c</c>
.
      }
      $text .= <<".";
        </colors>
.
   }
   $text .= <<".";
      </faceSet>
.
   if ($transp) {
      my @c=map {
         is_object($_) or $_=Visual::get_RGB($_);  $_->toInt
      } $JavaView::default::diffuseColor, $JavaView::default::emissiveColor, $JavaView::default::specularColor;
      $text .= <<".";
      <material>
        <ambientIntensity>$JavaView::default::ambientIntensity</ambientIntensity>
        <diffuse>
          <color type="rgb">$c[0]</color>
        </diffuse>
        <emissive>
          <color type="rgb">$c[1]</color>
        </emissive>
        <shininess>$JavaView::default::shininess</shininess>
        <specular>
          <color type="rgb">$c[2]</color>
        </specular>
        <transparency visible="show">$transp</transparency>
      </material>
.
   }
   return $text;
}

sub toString {
   my ($self)=@_;
   return $self->header . $self->pointsToString . $self->facesToString . $self->trailer;
}

##############################################################################################
#
#  Wire model (e.g. a graph)
#
package JavaView::Wire;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
);

sub linesToString {
   my ($self)=@_;
   my $style=$self->source->EdgeStyle;
   my $line_flag= is_code($style) || $style !~ $Visual::hidden_re ? "show" : "hide";
   my $arrow_flag= $self->source->ArrowStyle && !is_code($self->source->ArrowStyle) ? "show" : "hide";
   my $thickness=$self->source->EdgeThickness;
   my $thickness_flag= is_code($thickness) ? "show" : "hide";
   my $color=$self->source->EdgeColor;
   my $color_flag= is_code($color) ? "show" : "hide";
   my $text= <<".";
      <lineSet line="$line_flag" arrow="$arrow_flag" color="$color_flag" thicknesses="$thickness_flag">
        <lines>
.
   my $labels=$self->source->EdgeLabels;
   for (my $e=$self->source->all_edges; $e; ++$e) {
      next if is_code($style) && $style->($e) =~ $Visual::hidden_re;
      my $attr="";
      if (defined($labels)) {
         $attr=' name="' . $labels->($e) . '"';
      }
      $text .= <<".";
          <l$attr>@$e</l>
.
   }

   my $th=$thickness_flag eq "hide" ? ($thickness // 1)*$JavaView::default::lines_thickness : $JavaView::default::lines_thickness;
   $text .= <<".";
          <thickness>$th</thickness>
.

   if ($color_flag eq "hide" && defined($color)) {
      my $c=$color->toInt;
      $text .= <<".";
          <color type="rgb">$c</color>
.
   }
   my $show_labels= defined($labels) ? "show" : "hide";
   $text .= <<".";
          <labelAtt visible="$show_labels"/>
        </lines>
.
   if ($color_flag eq "show") {
      $text .= <<'.';
        <colors>
.
      for (my $e=$self->source->all_edges; $e; ++$e) {
         next if is_code($style) && $style->($e) =~ $Visual::hidden_re;
         my $c=$color->($e)->toInt;
         $text .= <<".";
          <c>$c</c>
.
      }
      $text .= <<'.';
        </colors>
.
   }
   if ($thickness_flag eq "show") {
      $text .= <<'.';
        <thicknesses>
.
      for (my $e=$self->source->all_edges; $e; ++$e) {
         next if is_code($style) && $style->($e) =~ $Visual::hidden_re;
         my $th=($thickness->($e) // 1)+0;
         $text .= <<".";
          <th>$th</th>
.
      }
      $text .= <<'.';
        </thicknesses>
.
   }
   $text .= <<'.';
      </lineSet>
.
}

sub toString {
   my ($self)=@_;
   $self->header . $self->pointsToString . $self->linesToString . $self->trailer;
}


1

# Local Variables:
# mode:perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
