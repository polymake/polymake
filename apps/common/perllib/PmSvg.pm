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
   my $self=shift;
   my @xy=( $self->scaleX * ($_[0] - $self->minX) + $self->marginLeft,
       (-1)*($self->scaleY * ($_[1] - $self->minY) + $self->marginBottom) );
   wantarray ? @xy : sprintf("%.3f %3.f", @xy)
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
   $svg->title(id=>'document_title')->cdata($self->title);
   foreach (@{$self->geometries}) {
      $_->draw($self,$svg);
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
   '@coords',
   '@radius',
);

sub init {
   my $self=shift;
   my $P=$self->source;
   @{$self->coords} = map { [ @$_[0,1] ] } @{$P->Vertices}; # chop the z coordinate if any
   foreach my $coord (@{$self->coords}) {
      assign_min_max($self->minX, $self->maxX, $coord->[0]);
      assign_min_max($self->minY, $self->maxY, $coord->[1]);
   }
   return unless defined(wantarray);

   my $last_point=$#{$self->coords};
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
   return ($labelwidth, $max_radius);
}

sub new {
   my $self=&_new;
   my ($labelwidth, $max_radius)=$self->init;
   $self->marginLeft=$self->marginRight=
      max($max_radius, ($avg_char_width * $fontsize*$labelwidth)/2) + $text_spacing/2;
   $self->marginBottom= $max_radius + $text_spacing/2;
   $self->marginTop=$self->marginBottom + $fontsize + $text_spacing;
   $self;
}

sub draw_points {
   my ($self, $svg)=@_;
   my $P=$self->source;
   my $get_color=$P->VertexColor;
   my $static_color;
   if (defined($get_color) && !is_code($get_color)) {
      $static_color=$get_color->toInt;
   }
   my $p = 0;
   foreach my $coord (@{$self->coords}){
      my $r=$self->radius->[$p] or next;
      my $color = $static_color // $get_color->($p)->toInt;
      $color =~ tr/" "/","/;
      $svg->circle(
         cx => $coord->[0],
         cy => $coord->[1],
         r  => $r,
         style => {
            fill => "rgb(" . "$color" . ")" 
         }
      );
      if (defined($P->VertexLabels)) {
         my $vlabel = $P->VertexLabels->($p);
         $svg->text(
            x => $coord->[0], 
            y => $coord->[1] - $text_spacing - $r,
            'text-anchor' => "middle",
            'font-family' => $fontname,
            'font-size' => $fontsize
         )->cdata($vlabel);
      }
      ++$p;
   }
}

sub draw_lines { }

sub draw {
   my ($self,$f, $svg)=@_;
   foreach my $p (@{$self->coords}) {
      @$p=$f->transform(@$p);
   }
   $self->draw_lines($svg);  
   $self->draw_points($svg) if @{$self->radius};
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
      if (defined $edge_thickness) {
    $lw=$edge_thickness*$line_width;
      }
   }
   if (defined $edge_color) {
      $edge_color =~ tr/" "/","/;
   }
   if (defined $facet_color) {
      $facet_color =~ tr/" "/","/;
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

sub draw_lines {
   my ($self, $svg)=@_;
   my $P=$self->source;
   my $i=0;
   foreach my $polygon (@{$P->Facets}) {
      draw_facet($svg, (map { my $decor=$P->$_; is_code($decor) ? $decor->($i) : $decor }
                  qw( FacetColor FacetTransparency FacetStyle EdgeColor EdgeThickness EdgeStyle )),
             [ @{$self->coords}[ @{$polygon} ] ]);
      ++$i;
   }
}

1;

# Local Variables:
# c-basic-offset:3
# End:
