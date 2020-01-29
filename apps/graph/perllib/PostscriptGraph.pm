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

package Postscript::Graph;

use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
   '$directed',
);

sub new {
   my $self=&_new;
   $self->init;

   $self->marginLeft=$text_spacing/2 + $line_width;
   $self->marginTop=($text_spacing+$fontsize)/2 + $line_width;

   if (defined (my $labels=$self->source->NodeLabels)) {
      my $node_thickness=$self->source->NodeThickness;
      my $border_thickness=$self->source->NodeBorderThickness;
      for (my ($n,$last)=(0,$#{$self->coords}); $n<=$last; ++$n) {
         my $lw=length($labels->($n));
         my $th=is_code($node_thickness) ? $node_thickness->($n) : $node_thickness;
         $th //= 1;
         my $bw=is_code($border_thickness) ? $border_thickness->($n) : $border_thickness;
         $bw //= 1;
         assign_max($self->marginLeft, ($lw*$avg_char_width*$fontsize+$text_spacing)*$th/2+$line_width*$bw);
         assign_max($self->marginTop, ($fontsize+$text_spacing)*$th/2+$line_width*$bw);
      }
   }
   $self->marginRight=$self->marginLeft;
   $self->marginBottom=$self->marginTop;
   $self->directed=ref($self->source->ArrowStyle) || $self->source->ArrowStyle;
   $self;
}

sub draw_edge {
   my ($self, $edge, $for_lattice)=@_;
   my ($s, $t)=@$edge;
   #   my $label=$Graph->get_edge_label($edge);
   my $style=$self->source->EdgeStyle;
   $style=$style->($edge) if is_code($style);
   if ($style =~ $Visual::hidden_re) {
      return "";
   }
   my $lw=$line_width;
   my $thickness=$self->source->EdgeThickness;
   $thickness=$thickness->($edge) if is_code($thickness);
   if (defined($thickness)) {
      return "" if $thickness==0;
      $lw*=$thickness;
   }
   my $color=$self->source->EdgeColor;
   $color=$color->($edge) if is_code($color);
   $color //= get_RGB($Visual::Color::edges);
   my $arrow=$self->source->ArrowStyle;
   $arrow=$arrow->($edge) if is_code($arrow);
   my $lh="";
   if ($arrow) {
      ($t,$s)=($s,$t) if $arrow<0;
      $lh=$self->source->NodeThickness;
      $lh=$lh->($t) if is_code($lh);
      $lh=" labelheight ".(defined($lh) && $lh!=1 && "$lh mul ")."2 div ".($for_lattice ? "true" : "false");
   }
   $color->toFloat . sprintf(" $lw$lh %.3f %.3f %.3f %.f ", @{$self->coords->[$s]}, @{$self->coords->[$t]}) . ($lh && "dir_")."edge\n";
}

sub node_decorations {
   my ($self, $node)=@_;
   my $style=$self->source->NodeStyle;
   $style=$style->($node) if is_code($style);
   if ($style =~ $Visual::hidden_re) {
      return ();
   }

   my $fill_color=$self->source->NodeColor;
   $fill_color=$fill_color->($node) if is_code($fill_color);
   my $border_color=$self->source->NodeBorderColor;
   $border_color=$border_color->($node) if is_code($border_color);
   my $border_width=$self->source->NodeBorderThickness;
   $border_width=$border_width->($node) if is_code($border_width);
   my $thickness=$self->source->NodeThickness;
   $thickness=$thickness->($node) if is_code($thickness);
   my $labels=$self->source->NodeLabels;

   return ( defined($border_color) ? "[ ".$border_color->toFloat." ]" : "null",
            defined($border_width) ? $border_width*$line_width : $line_width,
            defined($fill_color) ? "[ ".$fill_color->toFloat." ]" : "null",
            "(".(defined($labels) ? $labels->($node) : " ").")",
            $thickness // 1
          );
}

sub draw_node {
   my ($self, $node)=@_;
   my @dec=$self->node_decorations($node);
   return @dec ? "@dec ".sprintf("%.3f %.3f", @{$self->coords->[$node]})." CenteredLabel\n" : "";
}

sub draw {
   my ($self, $page, $for_lattice)=@_;
   foreach my $p (@{$self->coords}) {
      @$p=$page->transform(@$p);
   }

   my $Graph=$self->source;
   my $nodes=@{$self->coords};

   local $"=" ";        # for any case ...
   for (my $edge=$Graph->all_edges; $edge; ++$edge) {
      $page->code .= $self->draw_edge($edge,$for_lattice);
   }
   for (my $n=0; $n<$nodes; ++$n) {
      $page->code .= $self->draw_node($n);
   }
}

my $common_procs=<<"-----";
/text_spacing $text_spacing def
/labelheight $fontsize text_spacing add def

% create rectangular path of size width*height
% centered at the current position
% width height ->
/box {
   1 index 2 div neg 1 index 2 div neg rmoveto
   0 1 index rlineto
   exch 0 rlineto
   0 exch neg rlineto
   closepath
} bind def

% draw the colored box with black border (if fill=true) or the white box with colored border (if fill=false)
% centered at (x,y)
% [ R G B /*border*/ ] borderwidth [ R G B /*fill*/ ]|null width height x y ->
/BorderedBox {
   gsave
   newpath moveto box
   dup null eq { pop 1 setgray } { aload pop setrgbcolor } ifelse
   gsave fill grestore
   setlinewidth
   dup null eq { pop 0 setgray } { aload pop setrgbcolor } ifelse
   stroke
   grestore
} bind def
-----

my $dir_procs=<<"-----";
/arrowheadlength $arrowheadlength def
/arrowheadwidth $arrowheadwidth def

% draw edge (arrow) from (x1,y1) to (x2,y2)
% r g b linewidth lh is_box x1 y1 x2 y2 ->
/dir_edge {
   gsave
   9 dict begin
   /y2 exch def
   /x2 exch def
   /y1 exch def
   /x1 exch def
   /dx x2 x1 sub def
   /dy y2 y1 sub def
   /angle dy dx atan def
   { angle sin abs div } if neg dx dx mul dy dy mul add sqrt add /arrowlength exch def
   dup arrowheadwidth mul 2 div /aw exch def
   setlinewidth setrgbcolor
   x1 y1 translate
   angle rotate
   0 0 moveto arrowlength 0 lineto stroke
   arrowlength arrowheadlength sub aw moveto 
   arrowlength 0 lineto
   arrowlength arrowheadlength sub aw neg lineto
   closepath fill
   end
   grestore
} def
-----

my $undir_procs=<<"-----";
% draw edge from (x1,y1) to (x2,y2)
% r g b linewidth x1 y1 x2 y2 ->
/edge {
   gsave newpath moveto lineto setlinewidth setrgbcolor stroke grestore
} bind def
-----

my $graph_procs=<<"-----";
% draw a boxed label with center (x,y) and appropriate width
% [ R G B (border) ]|null borderwidth [ R G B (fill) ]|null (label) magn x y ->
/CenteredLabel {
   gsave
   5 dict begin
   /y exch def
   /x exch def
   /magn exch def
   magn 1 ne { currentfont magn scalefont setfont } if
   dup stringwidth pop /lw exch def
   /label exch def
   lw text_spacing magn mul add labelheight magn mul x y BorderedBox
   x lw 2 div sub y $fontsize 0.3 mul magn mul sub moveto
   label show
   end
   grestore
} def
-----

###########################################################################
package Postscript::Lattice;

use Polymake::Struct (
   [ '@ISA' => 'Graph' ],
   [ '$locked' => '0' ],
   '%sorted',
);

sub new {
   my $self=&_new;
   my $Graph=$self->source;
   $self->directed = ref($Graph->ArrowStyle) || $Graph->ArrowStyle;
   my $node_thickness=$self->source->NodeThickness;
   my $border_thickness=$self->source->NodeBorderThickness;
   my @label_width=map {
      my $th=is_code($node_thickness) ? $node_thickness->($_) : $node_thickness;
      $th //= 1;
      my $bw=is_code($border_thickness) ? $border_thickness->($_) : $border_thickness;
      $bw //= 1;
      assign_max($self->marginTop, ($fontsize+$text_spacing)*$th/2+$line_width*$bw);
      (($avg_char_width * $fontsize * length($Graph->NodeLabels->($_)) + $text_spacing)*$th + 2*$line_width*$bw)*(1+$face_spacing);
   } 0..$Graph->n_nodes-1;
   $label_width[0]=$Wpaper-2*$Wmargin;

   my $embedding=$Graph->Coord;
   if (is_object($embedding)) {
      # expecting Visual::GraphEmbedding here
      $embedding->label_width=\@label_width;
      $embedding->options->{dual}= $Graph->Mode eq "dual";
   }
   @{$self->coords}=@$embedding;

   my $style=$Graph->NodeStyle;
   foreach my $n (0..$#label_width) {
      if (!is_code($style) || $style->($n) !~ $Visual::hidden_re) {
         my ($x, $y)=@{$self->coords->[$n]};
         assign_min($self->minX, $x-$label_width[$n]/2);
         assign_max($self->maxX, $x+$label_width[$n]/2);
         assign_min_max($self->minY, $self->maxY, $y);
      }
   }

   $self->marginBottom=$self->marginTop;
   $self->marginLeft=$self->marginRight= 0.5*$text_spacing + $line_width;
   $self;
}

sub draw_node {
   my ($self, $node)=@_;
   my ($x, $y)=@{$self->coords->[$node]};
   if (my @decor=$self->node_decorations($node)) {
      unshift @decor, $x;
      push @{$self->sorted->{$y}}, \@decor;
   }
   return "";   # will produce PostScript code later
}

sub draw {
   my ($self, $page)=@_;
   Graph::draw(@_,1);

   my $code="[\n";
   # We sort the keys to have consistent output for all perl versions (especially >=5.18)
   foreach my $y (sort keys %{$self->sorted}) {
      my $list = $self->sorted->{$y};
      my @sorted_by_x=sort { $list->[$a]->[0] <=> $list->[$b]->[0] } 0..$#$list;
      my $min_gap=$Wpaper;
      foreach my $i (1..$#sorted_by_x-1) {
         assign_min($min_gap, $list->[$sorted_by_x[$i]]->[0] - $list->[$sorted_by_x[$i-1]]->[0]);
         assign_min($min_gap, $list->[$sorted_by_x[$i+1]]->[0] - $list->[$sorted_by_x[$i]]->[0]);
      }
      foreach (@$list) {
         $_->[0]=sprintf("%.3f",$_->[0]);
      }
      $code .= sprintf("  [ %.3f %.3f\n",$y,$min_gap) .
               "    [" . join(" ",  map { $list->[$_]->[0] } @sorted_by_x) . "]\n" .            # x coords of nodes
               "    [" . join("",  map { " [@{$list->[$_]}[1,2,3]] " } @sorted_by_x) . "]\n" .  # border_color, border_width, fill_color of nodes
               "    [" . join("",  map { " [@{$list->[$_]}[4,5]] " } @sorted_by_x) . "]\n" .    # label, magnification
               "  ]\n";
   }
   $code .= "] draw_nodes\n";
   $page->code .= $code;
   $page->dict->{face_spacing}=1+$face_spacing;
}

my $lattice_procs=<<"-----";
/min { 2 copy gt { exch } if pop } bind def
/max { 2 copy lt { exch } if pop } bind def

% [ [ y gap [ x ] [ [colors] ] [ [label magn] ] ] ... ] ->
/draw_nodes {
   7 dict begin
   /label 0 def  /RGB 0 def  /x 0 def  /gap 0 def  /y 0 def
   /font_scale 1 def  /i 0 def

   % find the minimal text scale suitable for all layers
   dup
   {
      dup 1 get
      % param_array, layer_array, gap ->
      0 2 index 4 get
      % param_array, layer_array, gap, 0, label_array ->
      dup length 1 eq { pop pop labelheight } { { aload pop exch stringwidth pop text_spacing add mul max } forall } ifelse  % find max label width
      % param_array, layer_array, gap, max_width ->
      2 copy face_spacing mul div dup font_scale lt { /font_scale exch store } { pop } ifelse  % if does not fit in the gap, then scale the text down
      exch pop 1 exch put  % store the max width in the gap slot
   } forall

   % draw the layers
   {
      aload pop
      /label exch store  /RGB exch store  /x exch store  /gap exch store  /y exch store
      0 1 label length 1 sub {
         /i exch store
         RGB i get aload pop  gap font_scale mul  label i get 1 get labelheight mul  x i get y  BorderedBox
         x i get y moveto
         gsave
         font_scale font_scale scale
         label i get 0 get dup stringwidth pop -2 div $fontsize -0.3 mul rmoveto show
         grestore
      } for
   } forall
   end
} def
-----

###########################################################################
package Postscript::Page;

sub addGraph {
   my ($self, $Graph)=@_;
   $self->title ||= $Graph->Title;
   return if $Graph->Hidden;

   push @{$self->elements}, new Postscript::Graph($Graph);

   $self->procsets->{'Graph::common'}=$common_procs;
   $self->procsets->{'Graph::graph'}=$graph_procs;
   if ($self->elements->[-1]->directed) {
      $self->procsets->{'Graph::directed'}=$dir_procs;
   } else {
      $self->procsets->{'Graph::undirected'}=$undir_procs;
   }
}

sub addLattice {
   my ($self, $Lattice)=@_;
   $self->title ||= $Lattice->Title;
   return if $Lattice->Hidden;

   push @{$self->elements}, new Postscript::Lattice($Lattice);

   %{$self->procsets}=( 'Graph::common'=>$common_procs,  'Graph::lattice'=>$lattice_procs);
   if ($self->elements->[-1]->directed) {
      $self->procsets->{'Graph::directed'}=$dir_procs;
   } else {
      $self->procsets->{'Graph::undirected'}=$undir_procs;
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
