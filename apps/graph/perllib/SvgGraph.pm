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

package PmSvg::Lattice;
use Polymake::Struct (
   [ '@ISA' => 'Wire' ],
   [ '$locked' => '0' ],
   '@label_widths'
);

sub new {
   my $self=&_new;
   my $Graph=$self->source;
   my $node_thickness=$self->source->NodeThickness;
   my $border_thickness=$self->source->NodeBorderThickness;
   @{$self->label_widths}=map {
      my $th=is_code($node_thickness) ? $node_thickness->($_) : $node_thickness;
      $th //= 1;
      my $bw=is_code($border_thickness) ? $border_thickness->($_) : $border_thickness;
      $bw //= 1;
      assign_max($self->marginTop, ($fontsize+$text_spacing)*$th/2+$line_width*$bw);
      (($avg_char_width * $fontsize * $glyph_width * line_break_label_length($Graph->NodeLabels->($_)) + $text_spacing)*$th + 2*$line_width*$bw)*(1+$face_spacing);
   } 0..$Graph->n_nodes-1;
   my $embedding=$Graph->Coord;
   if (is_object($embedding)) {
      # expecting Visual::GraphEmbedding here
      $embedding->label_width=\@{$self->label_widths};
      $embedding->options->{dual}= $Graph->Mode eq "dual";
   }
   $self->init;
   @{$self->coords}=@$embedding;
   my $style=$Graph->NodeStyle;
   foreach my $n (0..scalar(@{$self->label_widths})-1) {
      if (!is_code($style) || $style->($n) !~ $Visual::hidden_re) {
         my ($x, $y)=@{$self->coords->[$n]};
         assign_min($self->minX, $x-@{$self->label_widths}[$n]/2);
         assign_max($self->maxX, $x+@{$self->label_widths}[$n]/2);
         assign_min_max($self->minY, $self->maxY, $y);
      }
   }
   $self->marginBottom=$self->marginTop;
   $self->marginLeft=$self->marginRight= 0.5*$text_spacing + $line_width;
   $self;
}

sub draw_points {
   my ($self, $svggroups, $scaleX)=@_;
   my $Graph=$self->source;
   my $node_color=$Graph->NodeColor // new RGB("255 255 255");
   my $border_color=$Graph->NodeBorderColor // new RGB("0 0 0");
   my $static_node_color;
   my $static_border_color;
   if (defined($node_color) && !is_code($node_color)) {
      $static_node_color=$node_color;
   }
   if (defined($border_color) && !is_code($border_color)) {
      $static_border_color=$border_color;
   }
   my $border_thickness=$self->source->NodeBorderThickness;
   my $p = 0;
   
   foreach my $coord (@{$self->coords}){
      my $r = $self->radius->[$p] or next;
      my $fill_color = $static_node_color // $node_color->($p) // new RGB("255 255 255");
      my $border_color = $static_border_color // $border_color->($p) // new RGB("0 0 0");
      my $label_width = @{$self->label_widths}[$p];
      my $bw=is_code($border_thickness) ? $border_thickness->($p) : $border_thickness;
      $bw //= 1;
      $bw = $scaleX<1 ? $scaleX*$bw : $bw;
      $label_width = $scaleX<1 ? $scaleX*$label_width : $label_width;
      $label_width /= (1+$face_spacing); 
      my $fsize = $scaleX<1 ? $scaleX*$fontsize : $fontsize;
      my $ts = $scaleX<1 ? $scaleX*$text_spacing : $text_spacing;
      @{$svggroups}[$p]->rectangle(
         x=>$coord->[0] - 0.5*$label_width,
         y=>$coord->[1] - 0.5*$fsize - $ts,
         width=>$label_width,
         height=>0.75*$fsize + 2*$ts,
         rx=>$roundness_x, ry=>$roundness_y,
         style => {
         'fill' => "rgb(" . parse_color($fill_color) . ")",
         'stroke' => "rgb(" . parse_color($border_color) . ")",
         'stroke-width' => $bw,
      }
      );
      
      if (defined($Graph->NodeLabels)) {
         my $vlabel = $Graph->NodeLabels->($p);
         @{$svggroups}[$p]->text(
            x => $coord->[0], 
            y => $coord->[1] + 0.25*$fsize,
            'text-anchor' => "middle",
            'font-family' => $fontname,
            'font-size' => $fsize,
         )->cdata($vlabel);
      }
   }
   continue {
      ++$p;
   }
}

sub parse_color() {
   my $color=shift;
   $color = $color->toInt;
   $color =~ tr/" "/","/;
   return "$color";
}

sub line_break_label_length() {
   my $face_label = shift;
   my @node_numbers = split(/\s/,$face_label);
   my $length = @node_numbers ? @node_numbers-1 : 2;
   foreach (@node_numbers) { 
      my $l = length($_);
      $length += $l+($l-1)*$glyph_seperator;
   }
   return $length;
}

1


