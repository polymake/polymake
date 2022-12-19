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

package TikZ::PhylogeneticTree;
use Polymake::Struct (
   [ '@ISA' => 'Wire' ],
);

sub helperCoordinatesToString {
    # Coordinate definitions
    # we only need those if edges are not straight lines
    my ($self)=@_;
    my $id = $self->id; # this is used to make all TikZ ids globally unique
    my $text = "\n  % DEF HELPER COORDINATES\n";
    foreach (my $e=$self->source->all_edges; $e; ++$e) {
        my $a=$e->[0]; my $b=$e->[1];
        my $va = $self->source->Vertices->[$a];
        my $vb = $self->source->Vertices->[$b];
        my $point = new Vector<Float>($vb);
        if ($va->[1] != $vb->[1]) {
           # only if the y coordinates differ the helper point is different from the predecessor node
           $point->[0] = $vb->[0]; $point->[1] = $va->[1];
        }
        my $coordstring = Visual::print_coords($point);
        $coordstring =~ s/\s+/, /g;
        $text .= "  \\coordinate (v$b\_$a$id) at ($coordstring);\n";
    }
    $text .= "\n";

    return $text;
}

sub tikzedge {
    my ($self,$e,$label) = @_;
    my $a=$e->[0]; my $b=$e->[1];
    my $id = $self->id;
    my $text = "";
    my $tikzid = "edgestyle$id";
    $tikzid .= "_$a\_$b" if ($self->edgecode);
    $text .= "  \\draw[$tikzid] (v$b$id) -- (v$b\_$a$id) -- (v$a$id)";
    $text .= " node [edgelabelstyle$id\_$a\_$b] {$label}" if (defined($label) && $label ne "");
    $text .= ";\n";
    return $text;
}

sub drawAllEdges {
    my ($self) = @_;
    my @edges;
    foreach (my $e=$self->source->all_edges; $e; ++$e) {
        push @edges, $e;    
    } 
    $self->edgesToString(\@edges);
}

sub toString {
    my ($self, $trans)=@_;
    my $text = "";
    my $drawvertices = $self->source->VertexStyle !~ $Visual::hidden_re;
    my $drawedges = $self->source->EdgeStyle !~ $Visual::hidden_re;
    $text .= $self->header;
    $text .= $self->coordinatesToString; 
    $text .= $self->helperCoordinatesToString; 
    $text .= $self->vertexStylesToString("right") if ($drawvertices);
    $text .= $self->edgeStylesToString . $self->drawAllEdges if ($drawedges);
    $text .= $self->drawAllPoints if ($drawvertices);
    $text .= $self->trailer;
    return $text;
}

###########################################
1


