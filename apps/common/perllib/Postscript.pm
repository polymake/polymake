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

package Postscript;

# the average ratio of the glyph weight and height
declare $avg_char_width=0.7;

################################################################################
# useful small utilities

sub draw_poly_line {
   my $first=shift;
   if (is_array($first)) {
      " newpath @$first moveto " . join(" ", map { "@$_ lineto" } @_) . " "
   } else {
      " newpath $first moveto " . join(" ", map { "$_ lineto" } @_) . " "
   }
}

################################################################################
#
#  an abstract base class for various drawables (Graph, Polygon, Gale, etc.)
#
package Postscript::Element;

use Polymake::Struct (
   [ '$marginLeft'   => 'undef' ],
   [ '$marginRight'  => 'undef' ],
   [ '$marginBottom' => 'undef' ],
   [ '$marginTop'    => 'undef' ],
   [ '$minX' => 'undef' ],
   [ '$maxX' => 'undef' ],
   [ '$minY' => 'undef' ],
   [ '$maxY' => 'undef' ],
   [ '$locked' => '1' ],	# the scale for X and Y dimensions must be the same
);

################################################################################

package Postscript::Page;

use Polymake::Struct (
   [ new => '$' ],
   [ '@ISA' => 'Element' ],
   [ '$locked' => '0' ],
   '$scaleX',
   '$scaleY',
   [ '$canvas_width' => '$Wpaper-2*$Wmargin' ],
   [ '$canvas_height' => '$Hpaper-2*$Hmargin' ],
   [ '$title' => '#1' ],
   '%procsets',
   '$setup',
   '%dict',
   '$code',
   '@elements',
);

# real world coordinates => PostScript sheet coordinates
sub transform {
   my $self=shift;
   my @xy=( $self->scaleX * ($_[0] - $self->minX) + $self->marginLeft,
	    $self->scaleY * ($_[1] - $self->minY) + $self->marginBottom );
   wantarray ? @xy : sprintf("%.3f %3.f", @xy)
}

sub finish {
   my ($self)=@_;
   return unless @{$self->elements};

   foreach my $e (@{$self->elements}) {
      assign_max($self->marginLeft,   $e->marginLeft);
      assign_max($self->marginRight,  $e->marginRight);
      assign_max($self->marginTop,    $e->marginTop);
      assign_max($self->marginBottom, $e->marginBottom);
      assign_min($self->minX,  $e->minX);
      assign_max($self->maxX,  $e->maxX);
      assign_min($self->minY,  $e->minY);
      assign_max($self->maxY,  $e->maxY);
      $self->locked ||= $e->locked;
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

   foreach my $e (@{$self->elements}) {
      $e->draw($self);
   }
   @{$self->elements}=();
}

sub toString {
   my ($self, $ord)=@_;
   my $bb=sprintf("%.3f %.3f %.3f %.3f",
		  $Wmargin-$line_width, $Hmargin-$line_width,
		  $Wmargin+$self->canvas_width+$line_width, $Hmargin+$self->canvas_height+$line_width );
   my $title=$self->title;
   my $text=<<".";
%%Page: $title $ord
%%PageBoundingBox: $bb
%%BeginPageSetup
/$fontname $fontsize selectfont
$line_width setlinewidth
0 setgray
.
   $text.=$self->setup;
   my $dictsize=keys %{$self->dict};
   if ($dictsize) {
      $text.="$dictsize dict begin\n".
              # We sort the keys to have consistent output for all perl versions (especially >=5.18)
              join("", map { "/$_ " . $self->dict->{$_} . " def\n" } sort keys %{$self->dict});
   }
   $text.=<<".";
%%EndPageSetup
.
   $text.=$self->code;
   $text.="showpage\n";
   $text.="end\n" if $dictsize;
   $text
}

###########################################################################
package Postscript::File;

use Polymake::Struct (
   '$title',
   '@pages',
   '%procsets',		# 'name' => 'text'
   '@procset_names',	# 'name'
);

sub encode_text {
   if ($_[0] =~ /\s/ || /^\d*$/ || /^\(/) {
      $_[0]="($_[0])";
   }
}

sub finish_page {
   my ($self)=@_;
   return unless @{$self->pages};
   my $page=$self->pages->[-1];
   $page->finish;

   if (length(my $t=$page->title)) {
      encode_text($t);
      $page->title=$t;
      $self->title ||= $t;
   } else {
      $page->title=scalar(@{$self->pages});
   }

   # We sort the keys to have consistent output for all perl versions (especially >=5.18)
   foreach my $name (sort keys %{$page->procsets}) {
      $self->procsets->{$name} ||= do {
	 push @{$self->procset_names}, $name;
	 $page->procsets->{$name}
      }
   }
   %{$page->procsets}=();
}

sub new_page {
   my $self=shift;
   finish_page($self);
   push @{$self->pages}, new Postscript::Page(@_);
}

sub toString {
   my ($self)=@_;
   finish_page($self);

   my $n_pages=@{$self->pages};
   my $title=$self->title;
   my $text=<<".";
%!PS-Adobe-3.0
%%Creator: polymake
%%Title: $title
%%Pages: $n_pages
%%BoundingBox: (atend)
.
   if (my $declare_procsets=join("%%+", map { " procset $_ 0\n" } @{$self->procset_names})) {
      $text.="%%DocumentSuppliedResources:$declare_procsets";
   }
   $text.=<<".";
%%EndComments
%%BeginProlog
.
   foreach my $pn (@{$self->procset_names}) {
      my $ps=$self->procsets->{$pn};
      $text.=<<".";
%%BeginResource: procset $pn 0
$ps
%%EndResource
.
   }
   $text.=<<".";
%%EndProlog
.

   my $ord=1;
   my @bb=($Wmargin-$line_width, $Hmargin-$line_width, 0, 0);
   foreach my $p (@{$self->pages}) {
      assign_max($bb[2], $p->canvas_width);
      assign_max($bb[3], $p->canvas_height);
      $text.=$p->toString($ord++);
   }

   $bb[2] += $Wmargin+$line_width;
   $bb[3] += $Hmargin+$line_width;
   my $bb=sprintf("%.3f %.3f %.3f %.3f", @bb);
   $text.=<<".";
%%Trailer
%%BoundingBox: $bb
%%EOF
.
}

###########################################################################
#
#  Drawing of some graphical primitives
#

package Postscript::PointSet;

use Polymake::Struct (
   [ new => '$' ],
   [ '@ISA' => 'Element' ],
   [ '$source' => '#1' ],
   '@coords',
   '@radius',
);

sub init {
   my $self=shift;
   my $P=$self->source;
   @{$self->coords} = map { [ @$_[0,1] ] } @{$P->Vertices};	# chop the z coordinate if any
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
   my ($self, $page)=@_;
   my $P=$self->source;
   my $get_color=$P->VertexColor;
   my $static_color;
   if (defined($get_color) && !is_code($get_color)) {
      $static_color=$get_color->toFloat;
   }

   for (my ($p, $last_point)=(0, $#{$self->coords}); $p<=$last_point; ++$p) {
      my $r=$self->radius->[$p] or next;
      $page->code .= ($static_color // $get_color->($p)->toFloat) .
                     " $r (" . (defined($P->VertexLabels) ? $P->VertexLabels->($p) : " ") . ") " . sprintf("%.3f %.3f", @{$self->coords->[$p]}) . " Point\n";
   }
}

sub draw_lines { }

sub draw {
   my ($self, $page)=@_;
   foreach my $p (@{$self->coords}) {
      @$p=$page->transform(@$p);
   }
   $self->draw_lines($page);
   $self->draw_points($page) if @{$self->radius};
}

###########################################################################
package Postscript::Polygon;

use Polymake::Struct [ '@ISA' => 'PointSet' ];

sub draw_facet {
   my ($facet_color, $facet_style, $edge_color, $edge_thickness, $edge_style, $points)=@_;
   my $lw;
   if ($facet_style =~ $Visual::hidden_re) {
      $edge_color //= $facet_color;
      undef $facet_color;
   } elsif (defined $facet_color) {
      $facet_color=$facet_color->toFloat;
   } else {
      $facet_color=get_RGB($Visual::Color::facets)->toFloat;
   }
   if ($edge_style =~ $Visual::hidden_re) {
      $lw=0;
   } else {
      if (defined $edge_color) {
	 $edge_color=$edge_color->toFloat;
      } else {
	 $edge_color=get_RGB($Visual::Color::edges)->toFloat;
      }
      if (defined $edge_thickness) {
	 $lw=$edge_thickness*$line_width;
      } elsif ($line_width != 1) {
	 $lw=$line_width;
      }
   }
   my $gsave=($lw || !defined($lw) && $edge_color) && "gsave";
   my $code=$gsave . draw_poly_line(@$points) . "closepath\n";
   if (defined $facet_color) {
      $code .= <<".";
gsave $facet_color setrgbcolor eofill grestore
.
   }
   if (!defined($lw) || $lw) {
      if (defined $edge_color) {
         $code .= "$edge_color setrgbcolor ";
      }
      if ($lw) {
         $code .= "$lw setlinewidth ";
      }
      $code .= "stroke\n";
   }
   $code .= $gsave && "grestore\n";
}

sub draw_lines {
   my ($self, $page)=@_;
   my $P=$self->source;
   my $facet=$P->Facet;
   $page->code .= draw_facet( (map { $P->$_ } qw( FacetColor FacetStyle EdgeColor EdgeThickness EdgeStyle )),
			      defined($facet) ? [ @{$self->coords}[@$facet] ] : $self->coords);
}

###########################################################################
package Postscript::Polygons;

use Polymake::Struct [ '@ISA' => 'Polygon' ];

sub draw_lines {
   my ($self, $page)=@_;
   my $P=$self->source;
   my $i=0;
   foreach my $polygon (@{$P->Facets}) {
      $page->code .= draw_facet( (map { my $decor=$P->$_; is_code($decor) ? $decor->($i) : $decor }
				      qw( FacetColor FacetStyle EdgeColor EdgeThickness EdgeStyle )),
				 [ @{$self->coords}[ @{$polygon} ] ]);
      ++$i;
   }
}

###########################################################################

package Postscript::Page;

my $pointset_procs=<<'.';
% R G B raduis (label) x y ->
/Point {
   gsave translate
   dup stringwidth pop 2 div neg  2 index text_spacing add  moveto show
   newpath 0 0 3 -1 roll 0 360 arc
   gsave setrgbcolor fill grestore stroke
   grestore
} def
.

sub addPointSet {
   my ($self, $elem)=@_;
   $self->title ||= $elem->source->Name;
   push @{$self->elements}, $elem;
   $self->procsets->{'Common::pointset'}=$pointset_procs;
   $self->dict->{text_spacing}=$text_spacing;
}

1

# Local Variables:
# c-basic-offset:3
# End:
