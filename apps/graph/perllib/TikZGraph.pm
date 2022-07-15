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

package TikZ::Lattice;
use Polymake::Struct (
   [ '@ISA' => 'Wire' ],
);

sub new {
   my $self=&_new;
   my $Graph=$self->source;
   # we need more room for sloped edgelabels
   my $widthfactor = 0.8;
   if (defined($self->source->EdgeLabels)) {
      $widthfactor*=2;
   }
   my @label_width=map {
      (line_break_label_length($Graph->NodeLabels->($_))+1)*$widthfactor;
   } 0..$Graph->n_nodes-1;

   my $embedding=$Graph->Coord;
   if (is_object($embedding)) {
      # expecting Visual::GraphEmbedding here
      $embedding->label_width=\@label_width;
      $embedding->options->{dual}= $Graph->Mode eq "dual";
   }

   $self;
}

sub vertexStylesToString {
    my ($self)=@_;
    my $id = $self->id;
    my $text = "";
    my $number = @{$self->source->Vertices};
    
    my ($vcolortext,$vcolors) = $self->uniqueColors("VertexColor",new RGB("255 255 255"),$number);        
    $text .= $vcolortext;
    my ($bdcolortext,$bdcolors) = $self->uniqueColors("VertexBorderColor",new RGB("0 0 0"),$number);        
    $text .= $bdcolortext;
    $text .= "\n  % DEF VERTEXSTYLES\n";
    my $bdthickness = $self->source->VertexBorderThickness;
    my $labels = $self->source->VertexLabels;
    my $label_flag = (defined($labels) && $labels !~ $Visual::hidden_re && $labels ne "") ? 1 : 0;

    if (is_code($vcolors) || is_code($bdcolors) || is_code($bdthickness)) {
        $self->nodecode = 1; 
        foreach my $i (0..$number-1) {
            my $optionstring = "text=black, inner sep=2pt, rectangle, rounded corners=3pt,";
            my $colid = is_code($vcolors) ? $vcolors->($i) : $vcolors;
            my $bdcolid = is_code($bdcolors) ? $bdcolors->($i) : $bdcolors;
            my $bdthick = is_code($bdthickness) ? $bdthickness->($i) : $bdthickness;
            $optionstring .= "fill=$colid, draw=$bdcolid,";
            $optionstring .= ", line width=$bdthick"."pt" if (defined($bdthick));
            $text .= $self->tikzstyle("vertexstyle$id\_$i",$optionstring);
        }
    } else {
        my $optionstring = "inner sep=2pt, rectangle, rounded corners=3pt,";
        $optionstring .= "fill=$vcolors, draw=$bdcolors,";
        $optionstring .= ", line width=$bdthick"."pt" if (defined($bdthickness));
        $text .= $self->tikzstyle("vertexstyle$id",$optionstring);
    }
    return $text;
}

sub drawAllPoints {
    my ($self,$trans,$pointset) = @_;
    my $npoints = @{$self->source->Vertices};
    my @idlist = defined($pointset) ? @$pointset : (0..$npoints-1);
    return $self->pointsToString(\@idlist,1);
}

sub line_break_label_length {
	return max(map {length($_)} split(/\\\\/,shift));
}

1


