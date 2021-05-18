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

package Postscript::Gale;
use Math::Trig;

###########################################################################
#  for both dimensions 1 and 2

my $common_procs = <<".";
% color x y ->
/Circle {
   newpath
   $point_radius 0 360 arc
   dup 0 gt
   % white
   {
      gsave 1 setgray fill grestore stroke
      pop
   } {
      1 add 0.5 mul setgray   % -1 => black, 0 => 50% grey
      fill
      0 setgray
   } ifelse
} bind def

% [ colors ] x y ->
/Points {
   gsave
   translate
   dup length 1 eq
   {
      0 get 0 0 Circle  % lone circle
   } {
      dup length 360 exch div  % angle between centers
      exch { $point_radius 0 Circle dup rotate } forall
      pop
   } ifelse
   grestore
} bind def

% string ->
/Label {
   gsave
   1 setgray 3.5 setlinewidth 1 setlinecap 1 setlinejoin
   dup false charpath stroke
   grestore show
} bind def

/min { 2 copy gt { exch } if pop } bind def
/max { 2 copy lt { exch } if pop } bind def

% x y string -> x' y'
/Loop {
   3 dict begin
   /label exch def  /y exch def  /x exch def
   0 x y Circle
   x $text_spacing sub  y $point_radius add $text_spacing add  moveto
   label show
   label stringwidth pop $point_radius 5 mul max  x add  y
   end
} def
.

###########################################################################
#  for dimension 1 only

my $dim1_procs = <<".";
% angle ->
/Arrow {
   gsave
   rotate
   0 0 moveto
   big_radius 0 lineto stroke
   big_radius arrow_length sub arrow_radius sub  0 arrow_radius arrow_angle neg arrow_angle arc
   big_radius 0 lineto
   closepath fill
   grestore
} def

% << x_out y_out x_in y_in >> label ->
/ArrowLabel {
   exch begin
   x_out x_in ge
   {  % right side
      dup stringwidth pop dup x_out add
      % ... width x_right
      big_radius le { x_out y_out moveto pop } { neg x_in add y_in moveto } ifelse
   } { % left side
      dup stringwidth pop neg x_out add
      % ... x_left
      dup big_radius ge { y_out moveto } { pop x_in y_in moveto } ifelse
   } ifelse
   show
   end
} def
.

###########################################################################

sub loop_box_height() { 6*$point_radius + $fontsize + $text_spacing }

###########################################################################

use Polymake::Struct (
   [ new => '$' ],
   [ '@ISA' => 'Element' ],
   [ '$locked' => '0' ],
   [ '$Gale' => '#1' ],
   '$big_radius',
);

sub new {
   my $self=&_new;
   my $G=$self->Gale;
   my $dim=$G->dim;

   $self->marginLeft=$self->marginRight=$self->marginBottom=2*$point_radius;
   $self->marginTop=$self->marginBottom + $fontsize + $text_spacing;
   $self->marginTop += loop_box_height + $Hmargin/2 if @{$G->loops};
   if ($dim==1) {
      $self->big_radius= min( $Wpaper-2*$Wmargin, $Hpaper-2*$Hmargin )/2;
      $self->marginBottom += $Hmargin + 2*$self->big_radius;
   }

   # We sort the keys to have consistent output for all perl versions (especially >=5.18)
   foreach my $p (values %{$G->different_x_y}) {
      my ($x, $y)=@{$G->points->[$p->[0]]};
      assign_min_max($self->minX, $self->maxX, $x);
      assign_min_max($self->minY, $self->maxY, $y) if $dim==2;
   }
   $self->minY=$self->maxY=0 if $dim==1;

   return $self;
}
###########################################################################
sub labels {
   my $G=shift;
   $G->VertexLabels
   ?  (map { $G->VertexLabels->($_) } @_)
   :  ()
}

sub draw {
   my ($self, $page)=@_;
   my $G=$self->Gale;
   my $dim=$G->dim;
   my $code="";

   if ($dim==2) {
      # connect the pairs of vertices of the same color with dashed lines
      $code .= "gsave [1 4] 0 setdash 0.5 setgray\n";
      foreach my $bw ($G->whites, $G->blacks) {
         # We sort the keys to have consistent output for all perl versions (especially >=5.18)
         my @pts=map { scalar($page->transform(@$_)) } sort keys %$bw;
         foreach my $i (0 .. $#pts-1) {
            foreach my $j ($i+1 .. $#pts) {
               $code .= "newpath $pts[$i] moveto $pts[$j] lineto stroke\n";
            }
         }
      }
      $code .= "grestore\n";

      # for each facet find a triple of b/w points not belonging to it and connect them with a solid line
      foreach my $line (@{$G->gale_lines}) {
         $code .= draw_poly_line(map { scalar($page->transform(@{$G->points->[$_]})) } @$line)  . "stroke\n";
      }

   } else {	# dim==1
      # draw a big circle
      $code .= <<".";
gsave
left_edge right_edge add 2 div  big_radius $Hmargin add  translate
newpath 0 0 big_radius 0 360 arc stroke
.
      # draw the arrows and the labels
      # We sort the keys to have consistent output for all perl versions (especially >=5.18)
      foreach my $angle (sort keys %{$G->different_angles}) {
         my $pts = $G->different_angles->{$angle};
         my ($sin, $cos) = (sin($angle), cos($angle));
         # label box corner outside the big circle
         my $x_out = ($self->big_radius + $text_spacing) * $cos;
         my $y_out = ($self->big_radius + $text_spacing) * $sin;
         # alternative label box corner inside the big circle (near the point of an arrow wing)
         my $x_in_off = -$arrowheadlength/2;
         my $y_in_off = $arrowheadwidth/2 + $text_spacing;
         $y_in_off = -$y_in_off if $sin*$cos < 0;
         my $x_in = ($self->big_radius + $x_in_off) * $cos - $y_in_off * $sin;
         my $y_in = ($self->big_radius + $x_in_off) * $sin + $y_in_off * $cos;
         if ($angle<0) {
            $y_out -= $fontsize; $y_in -= $fontsize;
         }
         my $a=rad2deg($angle);
         my $label = join(",", labels($G, @$pts));
         $_=sprintf("%.3f",$_) for ($a, $x_out, $y_out, $x_in, $y_in);
         $code .= <<".";
$a Arrow
<< /x_out $x_out /y_out $y_out /x_in $x_in /y_in $y_in >> ($label) ArrowLabel
.
      }

      # draw a single horizontal line above
      $code .= <<".";
grestore
left_edge y1 moveto  right_edge y1 lineto  stroke
.
   }

   # draw the points as bunches of black and/or white circles
   while (my ($v, $pts)=each %{$G->different_x_y}) {
      my ($x,$y)=$page->transform(@$v);
      my $xy= $dim==1 ? sprintf("%.3f y1",$x) : sprintf("%.3f %.3f",$x,$y);
      $code .= "[ " . join(" ", @{$G->colors}[@$pts]) . " ] $xy Points\n";
      my $x_off = -$text_spacing;
      my $y_off = $point_radius + $text_spacing;
      $x_off -= $point_radius if $#$pts>0;
      $y_off += $point_radius if $#$pts>1;
      my $label = join(",", labels($G, @$pts));
      $code .= "$xy moveto $x_off $y_off rmoveto ($label) Label\n\n";
   }

# draw the loop points in a separate box at the top of the page
   if (@{$G->loops}) {
      my $h_box = loop_box_height;
      my $y_box = $page->canvas_height + $Hmargin/2;
      my $x_loops = $page->marginLeft + 4*$point_radius;
      my $y_loops = $y_box + 4*$point_radius;
      $code .=
         "$x_loops $y_loops\n" .
      join("", map { "($_) Loop\n" } labels($G, @{$G->loops})) .
      <<".";
pop $Wmargin sub $point_radius sub
$Wmargin exch $y_box exch $h_box rectstroke
.
   }

   $page->code .= $code;

   if ($dim == 1) {
      my $d=$arrowheadlength * $arrowheaddent;
      $page->dict->{arrow_length}=$arrowheadlength * (1-$arrowheaddent);
      $page->dict->{arrow_radius}=($arrowheadwidth**2 / (8 * $d)) + $d/2;
      $page->dict->{arrow_angle}=rad2deg(atan($arrowheadwidth * $d / ($arrowheadwidth**2 /4  - $d**2)));
      $page->dict->{big_radius}=$self->big_radius;
      $page->dict->{left_edge}=$Wmargin;
      $page->dict->{right_edge}=$Wpaper - $Wmargin;
      $page->dict->{y1}=2*($Hmargin+$self->big_radius);
   }
}
###########################################################################

package Postscript::Page;
sub addGale {
   my ($self, $Gale)=@_;
   $self->title ||= $Gale->Title;
   push @{$self->elements}, new Postscript::Gale($Gale);
   $self->procsets->{'Gale::common'}=$common_procs;
   if ($Gale->dim == 1) {
      $self->procsets->{'Gale::dim1'}=$dim1_procs;
   }
}

1;

# Local Variables:
# c-basic-offset:3
# End:
