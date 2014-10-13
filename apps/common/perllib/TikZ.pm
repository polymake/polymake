#  Copyright (c) 1997-2014
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

# Interface to TikZ.
#
# This file only provides the basic functionality.  Visualization of polymake's various object types
# triggers code implemented in apps/*/rules/tikz.rules.

package TikZ::File;

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
   my ($self,$trans) = @_;
   my $who=$ENV{USER};
   my $when=localtime();
   my $title=$self->title // "unnamed";

   my $xaxis = $trans->col(1);
   my $yaxis = $trans->col(2);
   my $zaxis = $trans->col(3);
   return <<".";
% polymake for $who
% $when
% $title

\\begin{tikzpicture}[x  = {($xaxis->[1]cm,$xaxis->[2]cm)},
                    y  = {($yaxis->[1]cm,$yaxis->[2]cm)},
                    z  = {($zaxis->[1]cm,$zaxis->[2]cm)},
                    scale = 1,
                    color = {lightgray}]

.
}

sub trailer {
    return <<".";

\\end{tikzpicture}
.
}

sub toString {
   my ($self)=@_;
   my $trans= defined($self->transform) ? (new Matrix<Float>($self->transform)) : (new Matrix<Float>([[1,0,0,0],[0,0.9,-0.06,-0.44],[0,-0.076,0.95,-0.29],[0,0.43,0.3,0.85]]));
   $trans->row(0) = unit_vector<Float>(4,0);
   $self->header($trans) . join("", map { $_->toString($trans) } @{$self->geometries}) . $self->trailer;
}

##############################################################################################
#
#  Basis class for all graphical objects handled by tikz
#
package TikZ::PointSet;
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
}

sub trailer {
}

sub pointsToString {
    my ($self)=@_;
    my $id = $self->id; # this is used to make all TikZ ids globally unique

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
    my @vertexcolor = ($color_flag eq "show") ? split(/ /, $color->(0)->toFloat) :  split(/ /, $color->toFloat);
    my $vcstring = join ",", @vertexcolor;
    
    my $text = "";


    # Point coloring
    if ($point_flag eq "show"){ 
        $text .= "\n  % POINTS STYLE\n";
        if ($color_flag eq "show"){
            my $i = 0;
            foreach my $e(@{$self->source->Vertices}){
                my @own_color_array = split(/ /, $color->($i)->toFloat);
                my $ocstring = join ",", @own_color_array;
                $text .= "  \\definecolor{pointcolor\_$id"."_$i}{rgb}{ $ocstring }\n";
                ++$i;
            }}

        $text.= <<".";
  \\definecolor{pointcolor\_$id}{rgb}{ $vcstring }
  \\tikzstyle{pointstyle\_$id} = [fill=pointcolor\_$id]

.
    }

    # Point definitions
    $text .= "\n  % DEF POINTS\n";
    foreach (@{$self->source->Vertices}) {
        my $point=ref($_) ? Visual::print_coords($_) : "$_".(" 0"x($d-1));
        $point =~ s/\s+/, /g;
        $text .= "  \\coordinate (v$k\_$id) at ($point);\n";
        ++$k;
    }
    $text .= "\n";

    return $text;
}

sub toString {
    my ($self, $trans)=@_;
    $self->header . $self->pointsToString . $self->trailer;
}

##############################################################################################
#
#  Wire model (e.g. a graph)

package TikZ::Wire;
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
    my $lsstring = "color=linecolor\_$id, thick";
    if($arrows==1) {
        $lsstring .= ", arrows = -stealth, shorten >= 1pt";
    } elsif($arrows==-1) {
        $lsstring .= ", arrows = stealth-, shorten >= 1pt";
    }

#points stuff
    my $point_style=$self->source->VertexStyle;
    my $point_flag= is_code($point_style) || $point_style !~ $Visual::hidden_re ? "show" : "hide";
    my $point_color=$self->source->VertexColor;
    my $point_color_flag= is_code($point_color) ? "show" : "hide";
    my $point_labels=$self->source->VertexLabels;
#---
    my $point_thickness=$self->source->VertexThickness;
    my $point_thickness_flag= is_code($point_thickness) ? "show" : "hide";
    my @vertexcolor = ($point_color_flag eq "show") ? split(/ /, $point_color->(0)->toFloat) :  split(/ /, $point_color->toFloat);
    my $vcstring = join ",", @vertexcolor;

    my $vertex_count = new Array<Int>($self->source->n_nodes);
    for(my $e =$self->source->all_edges; $e; ++$e){
        ++$vertex_count->[$e->[0]];
        ++$vertex_count->[$e->[1]];
    }

    my $text ="";

    if ($line_flag eq "show"){
        $text .= "\n  % EDGE STYLE\n";    
        $text = <<".";
  \\definecolor{linecolor\_$id}{rgb}{ $lcstring }
  \\tikzstyle{linestyle\_$id} = [$lsstring]

.

            if ($color_flag eq "show"){
                for (my $e=$self->source->all_edges; $e; ++$e) {
                    my $a=$e->[0]; my $b=$e->[1];
                    my $own_color =  $color->($e)->toFloat;
                    my @own_color_array = split(/ /, $color->($e)->toFloat);
                    my $ocstring = join ",", @own_color_array;
                    $text .= "\n  \\definecolor{linecolor\_$id"."_v$a"."_v$b}{rgb}{ $ocstring }\n";
                }
        }


        my $labels=$self->source->EdgeLabels;

        $text .= "\n  % EDGES\n";    
        for (my $e=$self->source->all_edges; $e; ++$e) {
            my @vertices_to_draw=();
            my $a=$e->[0]; my $b=$e->[1];
            my $option_string = "linestyle\_$id";
            $option_string .= ($color_flag eq "show") ? ", linecolor\_$id"."_v$a"."_v$b" : "";
            $text .= <<".";
  \\draw[$option_string] (v$a\_$id) -- (v$b\_$id);
.
                if (!--$vertex_count->[$a]){
                    push @vertices_to_draw, $a;
                }
                if (!--$vertex_count->[$b]){
                    push @vertices_to_draw, $b;
                }
            ###
            # Drawing points
            if (scalar(@vertices_to_draw) && $point_flag eq "show"){
                $text.="\n\n%POINTS\n";
                foreach my $i (@vertices_to_draw) {
                    if (defined($point_labels)) {
                        $text .= "  \\node at (v$i\_$id) [inner sep=0.5pt, above right, black] {\\tiny{".'$'.$point_labels->($i).'$}};'."\n";
                    }
                    my $option_string = ($point_color_flag eq "show") ? "pointcolor\_$id"."\_$i" : "pointcolor\_$id";
                    my $pthick = ($point_thickness_flag eq "show") ? $point_thickness->($i) : $point_thickness;
                    $text .= "  \\fill[$option_string] (v$i\_$id) circle ($pthick pt);\n" if ($pthick ne "");
                }
                $text.="\n\n%EDGES\n";
            }
        }
    }

    return $text;
}

sub toString {
    my ($self, $trans)=@_;
    $self->header . $self->pointsToString . $self->linesToString . $self->trailer;
}


##############################################################################################
#
#  Solid 2-d or 3-d body
#
package TikZ::Solid;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
);

sub facesToString {
    my ($self, $trans)=@_;
    my $id = $self->id; # this is used to make all TikZ ids globally unique

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

    my $transp = $self->source->FacetTransparency // 0.8;


#points stuff
    my $point_style=$self->source->VertexStyle;
    my $point_flag= is_code($point_style) || $point_style !~ $Visual::hidden_re ? "show" : "hide";
    my $point_color=$self->source->VertexColor;
    my $point_color_flag= is_code($point_color) ? "show" : "hide";
    my $point_labels=$self->source->VertexLabels;
#---
    my $point_thickness=$self->source->VertexThickness;
    my $point_thickness_flag= is_code($point_thickness) ? "show" : "hide";
    my @vertexcolor = ($point_color_flag eq "show") ? split(/ /, $point_color->(0)->toFloat) :  split(/ /, $point_color->toFloat);
    my $vcstring = join ",", @vertexcolor;



    my $text = "";

    ####
    # Facet and Edges Style
    if ($face_flag eq "show" || $edge_flag eq "show"){
        $text .= "\n  % FACES STYLE\n";  
        my $option_string = "";
        $option_string .= ($face_flag eq "show") ? "fill=facetcolor\_$id, opacity=$transp," : "";
        $option_string .= ($edge_flag eq "show") ? "draw=edgecolor\_$id, line width=$thickness pt, line cap=round, line join=round" : "draw=none";
        $text .= ($face_flag eq "show") ? "  \\definecolor{facetcolor\_$id}{rgb}{ $fcstring }\n" : "";
        $text .= ($edge_flag eq "show") ? "  \\definecolor{edgecolor\_$id}{rgb}{ $egstring }\n" : "";
        $text .= <<".";
  \\tikzstyle{facetstyle\_$id} = [$option_string]

.
    }


    my @sorted_facets =();
    my @vertex_count=();
    my $facets = new Array<Array<Int>>($self->source->Facets);
    
    my $vif = new IncidenceMatrix($facets);
    map { push @vertex_count, $vif->col($_)->size } (0..$vif->cols()-1);

    if (defined($self->source->FacetNormals)){
      my $f = convert_to<Float>(-$self->source->FacetNormals) if defined($self->source->FacetNormals);
      my $m = $f*inv($trans);

      my %hash = ();
      map { push @{$hash{$m->row($_)->[$m->cols()-1]}},$_; } (0..$m->rows()-1);
      map { push @sorted_facets, @{$hash{$_}}; } sort keys %hash;
    } else {
      @sorted_facets = (0..$vif->rows()-1);
    }

    ####
    # Individual Facet Colors
    if ($facet_color_flag eq "show" && $face_flag eq "show"){
        $text .= "\n  % FACE COLORS\n";    
        foreach my $e(@sorted_facets){
            my @own_color_array = split(/ /, $facet_color->($i)->toFloat);
            my $ocstring = join ",", @own_color_array;
            $text .= "\n\\definecolor{facetcolor\_$id"."_$e}{rgb}{ $ocstring }\n";
        }
        $text .="\n";
    }


    ####
    # Draw Facets Edges and Points
    if ($face_flag eq "show"|| $edge_flag eq "show"){
        $text .= "\n  % FACES and EDGES and POINTS in the right order\n";    
        foreach my $facet(@sorted_facets) {
            my @vertices_to_draw=();
            my $option_string = "fill,facetstyle\_$id";
            $option_string .= ($facet_color_flag eq "show") ? ", facetcolor\_$id"."_$facet" : "";      
            $text .= "  \\draw[$option_string]";
            foreach my $vertex(@{$facets->[$facet]}) {
                $text.=" (v$vertex\_$id) --";
                if (!--$vertex_count[$vertex]){
                    push @vertices_to_draw, $vertex;
                }
            }
            my $first_v=$facets->[$facet]->[0];
            $text.=" (v$first_v\_$id) -- cycle;";
            $text.="\n";

            ###
            # Drawing points
            if (scalar(@vertices_to_draw) && $point_flag eq "show"){
                $text.="\n\n%POINTS\n";
                foreach my $i (@vertices_to_draw) {
                    if (defined($point_labels)) {
                        $text .= "  \\node at (v$i\_$id) [inner sep=0.5pt, above right, black] {\\tiny{".'$'.$point_labels->($i).'$}};'."\n";
                    }
                    my $option_string = ($point_color_flag eq "show") ? "pointcolor\_$id"."\_$i" : "pointcolor\_$id";
                    $text .= "  \\fill[$option_string] (v$i\_$id) circle ($point_thickness pt);\n";
                }
                $text.="\n\n%FACETS\n";
            }
        }
    }

    return $text;
}

sub toString {
   my ($self, $transform)=@_;
   return $self->header . $self->pointsToString . $self->facesToString($transform) . $self->trailer;
}

1

# Local Variables:
# mode:perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
