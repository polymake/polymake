#  Copyright (c) 1997-2020
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

# Interface to Sketch.
#
# This file only provides the basic functionality.  Visualization of polymake's various object types
# triggers code implemented in apps/*/rules/sketch.rules.

package Sketch::File;

use Polymake::Struct (
   [ new => '$' ],
   [ '$title' => '#1' ],
   '@geometries',
   [ '$unnamed' => '0' ],
   [ '$transform' => 'undef' ],
);

sub append {
   my $self = shift;
   push @{$self->geometries}, @_;
   foreach (@_) {
      if (length($_->name)) {
         $self->title //= $_->name;
      } else {
         $_->name="unnamed__" . ++$self->unnamed;
      }
   }
}

sub header {
   my $self = shift;
   my $who=$ENV{USER};
   my $when=localtime();
   my $title=defined($self->title) ? $self->title : "unnamed";
   return <<".";
% polymake for $who
% $when
% $title

.
}

sub trailer {
   return <<".";
global { language tikz }

% EOF
.
}

sub toString {
   my ($self)=@_;
   my $tr_inv= defined($self->transform) ? inv($self->transform) : undef;
   $self->header . join("", map { $_->toString($tr_inv) } @{$self->geometries}) . $self->trailer;
}

##############################################################################################
#
#  Basis class for all graphical objects handled by sketch
#
package Sketch::PointSet;
use Polymake::Struct (
   [ new => '$' ],
   [ '$source' => '#1' ],
   [ '$name' => '#1 ->Name' ],
);

# name without whitespace and such
# notice that "-", "{", and "}" occur in names of simplices of a TRIANGULATION
sub id {
   my ($self)=@_;
   my $id=$self->name;
   $id =~ s/[\s\{\}-]+//g;
   return $id;
}

sub header {
   my ($self)=@_;
   my $id=$self->id;
   return <<".";
def poly_$id {
.
}

sub trailer {
   my ($self, $inv_transform)=@_;
   my $id=$self->id;
   my ($view_point, $view_direction, $view_up, $scale)=$self->source->transform2view($inv_transform, \%Sketch::default::);
   $_=join ",", @$_ for ($view_point, $view_direction, $view_up);
   return <<".";
}

put { scale($scale) then view ( ($view_point), ($view_direction), [$view_up]  ) } {poly_$id}

.
}

sub pointsToString {
   my ($self)=@_;
   my $id = $self->id; # this is used to make all Sketch ids globally unique

   my $d= is_object($self->source->Vertices) ? $self->source->Vertices->cols : 3;
   my $style=$self->source->VertexStyle;
   my $point_flag= is_code($style) || $style !~ $Visual::hidden_re ? "show" : "hide";
   my $color=$self->source->VertexColor;
   my $color_flag= is_code($color) ? "show" : "hide";
   my $k=0;
   my $labels=$self->source->VertexLabels;
#---
   my $thickness=$self->source->VertexThickness;
   my $thickness_flag= is_code($thickness) ? "show" : "hide";
   my @vertexcolor = ($color_flag eq "show") ? split(/ /, ($color->(0) // new RGB("0 0 0"))->toFloat) :  split(/ /, $color->toFloat);
   my $vcstring = join ",", @vertexcolor;
   
   my $text = "";


   # Point coloring
 if ($point_flag eq "show"){    
    if ($color_flag eq "show"){
    my $i = 0;
    foreach my $e(@{$self->source->Vertices}){
      my @pcolor = $color->($i);
      my @own_color_array = split(/ /, ($color->($i) // new RGB("0 0 0"))->toFloat);
      my $ocstring = join ",", @own_color_array;
      $text .= "\nspecial|\\definecolor{pointcolor_$id"."_$i}{rgb}{ $ocstring }\n|[lay=under]";
      ++$i;
    }}

   $text.= <<".";

  special|\\definecolor{pointcolor_$id}{rgb}{ $vcstring }\n|[lay=under]
  special|\\tikzstyle{pointstyle_$id} = [fill=pointcolor_$id]\n|[lay=under]

.
 }

   # Point definitions
    foreach (@{$self->source->Vertices}) {
      my $point=ref($_) ? Visual::print_coords($_) : "$_".(" 0"x($d-1));
      $point =~ s/\s+/, /g;
      $text .= "def v$k ($point)";
      ++$k;
   }
   $text .= "\n";

   # Point drawing (with labels)
 if ($point_flag eq "show"){    
   for (my $i=0; $i<$k; ++$i) {
      if (defined($labels)) {
        $text .= 'special|\node at #1 [inner sep=0.5pt, above right] {\tiny{$'.$labels->($i).'$}};'."\n|[lay=in] (v$i)";
      }
    
       my $pthick = ($thickness_flag eq "show") ? $thickness->($i) : $thickness;      
       my $option_string = "dotsize=$pthick pt, style=pointstyle_$id";
       $option_string .= ($color_flag eq "show") ? ", color=pointcolor_$id"."_$i" : "";

      $text .= "  dots [$option_string] (v$i)\n" if ($pthick ne "");
   }      
 }

   return $text;
}

sub toString {
   my ($self, $inv_transform)=@_;
   $self->header . $self->pointsToString . $self->trailer($inv_transform);
}

##############################################################################################
#
#  Wire model (e.g. a graph)

package Sketch::Wire;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
);

sub linesToString {
   my ($self)=@_;
   my $id=$self->id;

   my $arrows=$self->source->ArrowStyle;
   my $style=$self->source->EdgeStyle;
   my $line_flag= is_code($style) || $style !~ $Visual::hidden_re ? "show" : "hide";
   my $thickness=$self->source->EdgeThickness;
   my $thickness_flag= is_code($thickness) ? "show" : "hide";
   my $color=$self->source->EdgeColor;
   my $color_flag= is_code($color) ? "show" : "hide";
   my @linecolor = ($color_flag eq "show") ? split(/ /, $color->($self->source->all_edges)) : split(/ /, $color->toFloat);
   my $lcstring = join ",", @linecolor;
   my $lsstring = "color=linecolor_$id, thick";
   if($arrows==1) {
       $lsstring .= ", arrows = -stealth, shorten >= 1pt";
   } elsif($arrows==-1) {
       $lsstring .= ", arrows = stealth-, shorten >= 1pt";
   }
   my $text ="";

 if ($line_flag eq "show"){    
   $text = <<".";

  special|\\definecolor{linecolor_$id}{rgb}{ $lcstring }\n|[lay=under]
  special|\\tikzstyle{linestyle_$id} = [$lsstring]\n|[lay=under]

.

    if ($color_flag eq "show"){
  for (my $e=$self->source->all_edges; $e; ++$e) {
      my $a=$e->[0]; my $b=$e->[1];
      my $own_color =  $color->($e)->toFloat;
      my @own_color_array = split(/ /, $color->($e)->toFloat);
      my $ocstring = join ",", @own_color_array;
      $text .= "\nspecial|\\definecolor{linecolor_$id"."_v$a"."_v$b}{rgb}{ $ocstring }\n|[lay=under]";
}
}


   my $labels=$self->source->EdgeLabels;

  for (my $e=$self->source->all_edges; $e; ++$e) {
      my $a=$e->[0]; my $b=$e->[1];
      my $option_string = "line style=linestyle_$id";
      $option_string .= ($color_flag eq "show") ? ", color=linecolor_$id"."_v$a"."_v$b" : "";
      $text .= <<".";
  line [$option_string] (v$a) (v$b)
.
   }
 }

   return $text;
}

sub toString {
   my ($self, $inv_transform)=@_;
   $self->header . $self->pointsToString . $self->linesToString . $self->trailer($inv_transform);
}


##############################################################################################
#
#  Solid 2-d or 3-d body
#
package Sketch::Solid;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
);

sub facesToString {
   my ($self)=@_;
   my $id = $self->id; # this is used to make all Sketch ids globally unique

   my $style=$self->source->FacetStyle;
   my $transp=$self->source->FacetTransparency;
   my $face_flag= $style !~ $Visual::hidden_re ? "show" : "hide";
   my $facet_color=$self->source->FacetColor;
   my $facet_color_flag= is_code($facet_color) ? "show" : "hide";
   my $edge_color=$self->source->EdgeColor;
   my $edge_color_flag= is_code($edge_color) ? "show" : "hide";

   my $edge_style=$self->source->EdgeStyle;
   my $edge_flag= is_code($edge_style) || $edge_style !~  $Visual::hidden_re ? "show" : "hide";
   my $backface_flag= $face_flag eq "show" && $self->source->Closed && !defined($transp) ? "hide" : "show";
   my $k=0;
   my $labels=$self->source->FacetLabels;

   my @facetcolor = ($facet_color_flag eq "show") ? split(/ /, $facet_color->(0)->toFloat) : split(/ /, $facet_color->toFloat);
   my $fcstring = join ",", @facetcolor;
   my @edgecolor = ($edge_color_flag eq "show") ? split(/ /, $edge_color->(0)->toFloat) : split(/ /, $edge_color->toFloat);
   my $egstring = join ",", @edgecolor;
   my $thickness = $self->source->EdgeThickness // 1;

   $transp //= 0.8;

   my $text = "";

if ($face_flag eq "show"){
  $text .= <<".";

  special|\\definecolor{facetcolor_$id}{rgb}{ $fcstring }\n|[lay=under]
  special|\\tikzstyle{facetstyle_$id} = [fill=facetcolor_$id, draw=none, opacity=$transp]\n|[lay=under]
.
  }

if ($edge_flag eq "show"){
  $text .= <<".";
  special|\\definecolor{edgecolor_$id}{rgb}{ $egstring }\n|[lay=under]
  special|\\tikzstyle{edgestyle_$id} = [draw=edgecolor_$id, line width=$thickness pt, line cap=round]\n|[lay=under]

.
  }

if ($facet_color_flag eq "show" && $face_flag eq "show"){
    my $i = 0;
  foreach my $e(@{$self->source->Facets}){
      my @own_color_array = split(/ /, $facet_color->($i)->toFloat);
      my $ocstring = join ",", @own_color_array;
      $text .= "\nspecial|\\definecolor{facetcolor_$id"."_$i}{rgb}{ $ocstring }\n|[lay=under]";
      ++$i;
}
$text .="\n";
}


  if ($edge_flag eq "show"){
    my $i = 0;
   foreach my $facet(@{$self->source->Facets}) {
       my $option_string = "line style=edgestyle_$id";
       $text .= "  line [$option_string]";
       foreach my $vertex(@$facet) {
           $text.=" (v$vertex)";
       }
       my $first_v=$facet->[0];
       $text.=" (v$first_v)";
       $text.="\n";
       ++$i;
   }
}
   
  if ($face_flag eq "show"){
    my $i = 0;
    foreach my $facet(@{$self->source->Facets}) {
       my $option_string = "cull=false, fill style=facetstyle_$id";
       $option_string .= ($facet_color_flag eq "show") ? ", color=facetcolor_$id"."_$i" : "";      
       $text .= "  polygon [$option_string]";
       foreach my $vertex(@$facet) {
           $text.=" (v$vertex)";
       }
       $text.="\n";
       ++$i;
   }
  }

   return $text;
}

sub toString {
   my ($self, $inv_transform)=@_;
   return $self->header . $self->pointsToString . $self->facesToString . $self->trailer($inv_transform);
}

1

# Local Variables:
# mode:perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
