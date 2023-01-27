#  Copyright (c) 1997-2023
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

# Interface to SVG.
#
# This file only provides the basic functionality.

use SVG;

package PmSvg;

#the average ratio of the glyph weight and height
declare $avg_char_width=0.7;

################################################################################
# useful small utilities

sub pt {
   my $val = shift;
   return "$val" . "pt"
}

################################################################################
#
#  an abstract base class for various drawables (Graph, Polygon, Gale, etc.)
#
package PmSvg::Element;

use Polymake::Struct (
   [ '$marginLeft'   => 'undef' ],
   [ '$marginRight'  => 'undef' ],
   [ '$marginBottom' => 'undef' ],
   [ '$marginTop'    => 'undef' ],
   [ '$minX' => 'undef' ],
   [ '$maxX' => 'undef' ],
   [ '$minY' => 'undef' ],
   [ '$maxY' => 'undef' ],
   [ '$locked' => '1' ],   # the scale for X and Y dimensions must be the same
);

################################################################################

package PmSvg::File;

use Polymake::Struct (
   [ new => '$' ],
   [ '@ISA' => 'Element' ],
   '@geometries',
   [ '$unnamed' => '0' ],
   [ '$locked' => '0' ],
   '$scaleX',
   '$scaleY',
   [ '$canvas_width' => '$Wpaper-2*$Wmargin' ],
   [ '$canvas_height' => '$Hpaper-2*$Hmargin' ],
   [ '$title' => '#1' ],
);

# real world coordinates => Svg sheet coordinates
sub transform {
   my ($self,$points)=@_;
   foreach my $v (@$points) {
      $v->[0] = $self->scaleX * ($v->[0] - $self->minX) + $self->marginLeft;
      $v->[1] = (-1)*($self->scaleY * ($v->[1] - $self->minY) + $self->marginBottom);
   }
}

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

sub toString {
   my ($self)=@_;
   foreach my $g (@{$self->geometries}) {
      assign_max($self->marginLeft,   $g->marginLeft);
      assign_max($self->marginRight,  $g->marginRight);
      assign_max($self->marginTop,    $g->marginTop);
      assign_max($self->marginBottom, $g->marginBottom);
      assign_min($self->minX,  $g->minX);
      assign_max($self->maxX,  $g->maxX);
      assign_min($self->minY,  $g->minY);
      assign_max($self->maxY,  $g->maxY);
      $self->locked ||= $g->locked;
   }

   $self->scaleX= $self->maxX == $self->minX
                  ? 0
        : ($self->canvas_width - $self->marginLeft - $self->marginRight) /
                    ($self->maxX - $self->minX);
   $self->scaleY= $self->maxY == $self->minY
                  ? 0
        : ($self->canvas_height - $self->marginTop - $self->marginBottom) /
                    ($self->maxY - $self->minY);
   if ($self->locked) {
      if ($self->scaleX  and  !$self->scaleY || $self->scaleX <= $self->scaleY) {
    $self->scaleY=$self->scaleX;
    $self->canvas_height= $self->scaleY*($self->maxY-$self->minY) +
                               $self->marginBottom + $self->marginTop;
      } else {
    $self->scaleX=$self->scaleY;
    $self->canvas_width= $self->scaleX*($self->maxX-$self->minX) +
                              $self->marginLeft + $self->marginRight;
      }
   } else {
      if (!$self->scaleX) {
    $self->canvas_width= $self->marginLeft + $self->marginRight;
      }
      if (!$self->scaleY) {
    $self->canvas_height= $self->marginBottom + $self->marginTop;
      }
   }

   $self->marginLeft += $Wmargin;
   $self->marginBottom += $Hmargin;
   
   #create the SVG object and draw all geometries
   my $viewbox_width = $self->canvas_width + $self->marginLeft + $self->marginRight;
   my $viewbox_height = $self->canvas_height + $self->marginBottom + $self->marginTop;
   my $svg = SVG->new(id=>'document',width=>pt($Wpaper),height=>pt($Hpaper),viewBox=>"0 -$viewbox_height $viewbox_width $viewbox_height");
   my $title = $self->title // "unnamed";
   $svg->title(id=>'document_title')->cdata($title);

   foreach (@{$self->geometries}) {
      $_->draw($self,$svg,$self->scaleX);
   }
   return $svg->xmlify;
}

###########################################################################
#
#  Basis class for all graphical objects handled by svg
#

package PmSvg::PointSet;

use Polymake::Struct (
   [ new => '$' ],
   [ '@ISA' => 'Element' ],
   [ '$source' => '#1' ],
   [ '$name' => '#1 ->Name' ],
   '$coords',
   '@radius',
);

sub init {
   my $self=shift;
   my $P=$self->source;
   $self->coords = ($P->Vertices->isa("Visual::DynamicCoords")) ? new Matrix<Float>($P->Vertices->compute()) : new Matrix<Float>($P->Vertices);
   $self->coords = ($self->coords->cols == 3) ? $self->coords->minor(All,[0,1]) : $self->coords; # chop the z coordinate if any
   foreach my $coord (@{$self->coords}) {
      assign_min_max($self->minX, $self->maxX, $coord->[0]);
      assign_min_max($self->minY, $self->maxY, $coord->[1]);
   }

   my $last_point=$self->coords->rows;
   my ($labelwidth, $max_radius);
   my $style=$P->VertexStyle;
   if (is_code($style) || $style !~ $Visual::hidden_re) {
      my $thickness=$P->VertexThickness;
      if (is_code($thickness)) {
    @{$self->radius}=map {
       if (is_code($style) && $style->($_) =~ $Visual::hidden_re) {
          0
       } elsif (defined (my $th=$thickness->($_))) {
          $th*=$point_radius/2;
          assign_max($max_radius,$th);
          $th
       } else {
          $point_radius;
       }
    } 0..$last_point;
      } else {
    $max_radius= defined($thickness) ? ($thickness*$point_radius)/2 : $point_radius;
    if (is_code($style)) {
       @{$self->radius}=map { $style->($_) !~ $Visual::hidden_re && $max_radius } 0..$last_point;
    } else {
       @{$self->radius}=($max_radius) x ($last_point+1);
    }
      }
   }

   if (defined($P->VertexLabels)) {
      map { assign_max($labelwidth, length($P->VertexLabels->($_))) } 0..$#{$self->coords};
   }

   $self->marginLeft=$self->marginRight=
      max($max_radius, ($avg_char_width * $fontsize*$labelwidth)/2) + $text_spacing/2;
   $self->marginBottom= $max_radius + $text_spacing/2;
   $self->marginTop=$self->marginBottom + $fontsize + $text_spacing;
}

sub new {
   my $self=&_new;
   $self->init;
   $self;
}

sub draw_points {
   my ($self, $svggroups)=@_;
   my $P=$self->source;
   my $get_color=$P->VertexColor // new RGB("255 255 255");
   my $static_color;
   if (defined($get_color) && !is_code($get_color)) {
      $static_color=$get_color;
   }
   my $p = 0;
   foreach my $coord (@{$self->coords}){
      my $r=$self->radius->[$p] or next;
      my $color = $static_color // $get_color->($p) // new RGB("255 255 255");
      $color = $color->toInt;
      $color =~ tr/" "/","/;
      @{$svggroups}[$p]->circle(
         cx => $coord->[0],
         cy => $coord->[1],
         r  => $r,
         style => {
            fill => "rgb(" . "$color" . ")" 
         }
      );
      if (defined($P->VertexLabels)) {
         my $vlabel = $P->VertexLabels->($p);
         my %attribs = ( x => $coord->[0], y => $coord->[1] - $text_spacing - $r );
         $self->draw_label(@{$svggroups}[$p], $vlabel, \%attribs);
      }
   }
   continue {
      ++$p;
   }
}

sub draw_lines { }

sub draw_label {
      my ($self, $svg, $label, $attribs) = @_;
      $attribs->{'text-anchor'} //= "middle";
      $attribs->{'font-family'} //= $fontname;
      $attribs->{'font-size'  } //= $fontsize;

      my $text = $svg->text(%$attribs);
      my @lines = split("\n", $label);
      if ($#lines>0) {
         for my $i (0..$#lines) {
            $text->text(-type=>'span', x=>0, dy=>$fontsize*$i)->cdata($lines[$i]);
         }
      } else {
         $text->cdata($label);
      }
}

sub draw {
   my ($self,$f, $svg, $scaleX)=@_;
   $f->transform($self->coords);
   $self->draw_lines($svg);
   # No grouping here. All points are going to be drawn to $svg
   my @svggroups = map { $svg } @{$self->coords};
   $self->draw_points(\@svggroups,$scaleX) if @{$self->radius};
   $svg;
}


###########################################################################
package PmSvg::Solid;

use Polymake::Struct [ '@ISA' => 'PointSet' ];

sub draw_facet {
   my ($svg, $facet_color, $facet_transparency, $facet_style, $edge_color, $edge_thickness, $edge_style, $points)=@_;
   my $lw = $line_width;
   if ($facet_style =~ $Visual::hidden_re) {
      $edge_color //= $facet_color;
      undef $facet_color;
   } elsif (defined $facet_color) {
      $facet_color=$facet_color->toInt;
   } else {
      $facet_color=get_RGB($Visual::Color::facets)->toInt;
   }
   $facet_color =~ tr/" "/","/;
   if (!defined $facet_transparency) {
      $facet_transparency = 1;
   }
   if ($edge_style =~ $Visual::hidden_re) {
         $lw=0;
      } else {
         if (defined $edge_color) {
       $edge_color=$edge_color->toInt;
         } else {
       $edge_color=get_RGB($Visual::Color::edges)->toInt;
         }
         $edge_color =~ tr/" "/","/;
         if (defined $edge_thickness) {
       $lw=$edge_thickness*$line_width;
         }
      }   

   my @xv =  map { $_->[0] } @{$points}; 
   my @yv =  map { $_->[1] } @{$points}; 
   my $path = $svg->get_path(
      x => \@xv, y => \@yv,
      -type => 'polygon',
      -closed => 'true'
   );
   $svg->polygon(
      %$path,
      style => {
         'fill' => "rgb(" . "$facet_color" . ")",
         'fill-opacity' => $facet_transparency,
         'stroke' => "rgb(" . "$edge_color" . ")",
         'stroke-width' => $lw,
      });
}

sub draw {
   my ($self,$f, $svg, $scaleX)=@_;
   my $P = $self->source;
   $f->transform($self->coords);
   
   my $nfacets = scalar(@{$P->Facets});

   my @vertexgroups = map { $svg } @{$self->coords};
   
   # only group if there are facet normals 
   if (defined($P->FacetNormals)) {    
      # groups are rendered in the same order as they are created
      my $bfacets = $svg->group();
      my $bverts = $svg->group();
      my $ffacets = $svg->group();
      my $fverts = $svg->group();
      @vertexgroups = map { $bverts } @{$self->coords};

      # iterate over facets and draw them
      my $facetgroup;
      foreach my $i (0..$nfacets-1) {
         my $polygon = $P->Facets->[$i];
         my $facetnormal = $P->FacetNormals->[$i]; 
         if ($facetnormal->[-1] < 0) {
            $facetgroup = $bfacets;
         } else {
            $facetgroup = $ffacets;
            foreach my $vert (@{$polygon}){
               $vertexgroups[$vert] = $fverts;
            }
         }
         draw_facet($facetgroup, (map { my $decor=$P->$_; is_code($decor) ? $decor->($i) : $decor }
                  qw( FacetColor FacetTransparency FacetStyle EdgeColor EdgeThickness EdgeStyle )),
             [ @{$self->coords}[ @{$polygon} ] ]);
      } 
   } else {
      foreach my $i (0..$nfacets-1) {
         my $polygon = $P->Facets->[$i];
         draw_facet($svg, (map { my $decor=$P->$_; is_code($decor) ? $decor->($i) : $decor }
                  qw( FacetColor FacetTransparency FacetStyle EdgeColor EdgeThickness EdgeStyle )),
             [ @{$self->coords}[ @{$polygon} ] ]);
      }
   }
   $self->draw_points(\@vertexgroups,$scaleX) if @{$self->radius};
   $svg;
}
    

      
##############################################################################################
#
#  Wire model (e.g. a graph)

package PmSvg::Wire;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
);

use Math::Trig;

sub draw_edge {
   my ($self, $svg, $edge, $arrows, $label)=@_;
   my ($s, $t)= $arrows!=-1 ? @$edge : reverse @$edge;
   my $style=$self->source->EdgeStyle;
   $style=$style->($edge) if is_code($style);
   if ($style =~ $Visual::hidden_re) {
      return;
   }
   my $lw=$line_width;
   my $thickness=$self->source->EdgeThickness;
   $thickness=$thickness->($edge) if is_code($thickness);
   if (defined($thickness)) {
      $lw*=$thickness;
   }
   my ($coord_s,$coord_t) = ($self->coords->[$s],$self->coords->[$t]);
   if ($lw) {
      my $color=$self->source->EdgeColor;
      $color=$color->($edge) if is_code($color);
      $color //= get_RGB($Visual::Color::edges);
      $color=$color->toInt(); $color =~ tr/" "/","/;
      my %line_style = (
            'stroke' => "rgb(" . "$color" . ")",
            'stroke-width' => $lw
            );
      if ($arrows!=0) {
         $line_style{"marker-end"}="url(#arrow)";
      }
      my $line = $svg->line(
            x1=>$coord_s->[0], y1=>$coord_s->[1],
            x2=>$coord_t->[0], y2=>$coord_t->[1],
            %line_style
         );
   }
   if (defined($label) && $label !~ $Visual::hidden_re && $label ne "") {
      my $d = $coord_t-$coord_s;
      my $rot = 360*atan2($d->[1],$d->[0])/(2*pi);
      if ($d->[0] < 0) {
         $rot -= 180;
      }

      my $offset = normalized(null_space($d))->row(0)*$fontsize;
      my $pos = $coord_s + $d/2 + $offset;
      my %attribs = ( transform => "translate($pos->[0],$pos->[1]) rotate($rot)" );
      $self->draw_label($svg, $label, \%attribs);
   }
}

sub draw_lines {
   my ($self, $svg)=@_;
   my $arrows=$self->source->ArrowStyle;
   my $labels=$self->source->EdgeLabels;
   my $arrow_color= new RGB(@arrowheadcolor);
   $arrow_color =~ tr/" "/","/;
   if (is_code($arrows) || $arrows!=0) {
      my $arrow_marker = $svg->defs()->marker(
         id=>"arrow",
         markerWidth=>$arrowheadlength,
         markerHeight=>$arrowheadwidth,
         refX=>$arrowheadoffset,
         refY=>$arrowheadwidth/2,
         orient=>"auto",
         markerUnits=>"strokeWidth"
      );
      $arrow_marker->path(
         d=>"M0,0 L0,$arrowheadwidth L$arrowheadlength," . $arrowheadwidth/2 . "z",
         fill=>"rgb(" . "$arrow_color" . ")"
      );
   }
   for (my $e=$self->source->all_edges; $e; ++$e) {
      my $arrow = is_code($arrows) ? $arrows->($e) : $arrows;      
      my $label = is_code($labels) ? $labels->($e) : $labels;      
      $self->draw_edge($svg,$e,$arrow,$label);
   }
}

1;

# Local Variables:
# c-basic-offset:3
# End:
