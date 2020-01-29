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

   my $result = <<".";
% polymake for $who
% $when
% $title

.

   if ($self->geometries->[0]->isa("TikZ::Lattice")) {
      $result .= <<".";
\\begin{tikzpicture}[x  = {(1em, 0em)},
                    y  = {(0em, 10em)},
                    scale = 1,
                    color = {lightgray}]

.
   } else {
      my $xaxis = $trans->col(1);
      my $yaxis = $trans->col(2);
      my $zaxis = $trans->col(3);

      $result .= <<".";
\\begin{tikzpicture}[x  = {($xaxis->[1]cm,$xaxis->[2]cm)},
                    y  = {($yaxis->[1]cm,$yaxis->[2]cm)},
                    z  = {($zaxis->[1]cm,$zaxis->[2]cm)},
                    scale = 1,
                    color = {lightgray}]

.
   }
   return $result;
}

sub trailer {
    return <<".";

\\end{tikzpicture}
.
}

sub toString {
   my ($self)=@_;
   my $trans;
   my $object = $self->geometries->[0];
   if (defined($self->transform)) {
       # if a transfomation is there, then just take it and remove translation
       $trans = new Matrix<Float>($self->transform);
       $trans->row(0) = unit_vector<Float>(4,0);
   } elsif (is_object($object->source->Vertices) && $object->source->Vertices->cols < 3) {
       # if dimension is < 3 take the unit matrix as transformation
       # so that nothing gets skew
       $trans = dense(unit_matrix<Float>(4));
   } else {
       # if no transformation is given and we have something 3 dimensional
       # take a little skew look at the object.
       $trans = new Matrix<Float>([[1,0,0,0],[0,0.9,-0.06,-0.44],[0,-0.076,0.95,-0.29],[0,0.43,0.3,0.85]]);
   }

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
# notice that "-", "{", and "}" occur in names of simplices of a TRIANGULATION and "#" in unnamed subobjects
sub id {
   my ($self)=@_;
   my $id=$self->name;
   $id =~ s/[\#\s\{\}-]+//g;
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
    my $point_flag= $style !~ $Visual::hidden_re ? 1 : 0;
    my $color=$self->source->VertexColor // new RGB("255 255 255");
    my $labels=$self->source->VertexLabels;
    my $thickness=$self->source->VertexThickness;

    my $text = "";


    # Point coloring
    if ($point_flag){
        $text .= "\n  % POINTS STYLE\n";
        if(is_code($color)){
            my $i = 0;
            foreach (@{$self->source->Vertices}){
                my $vcstring = join ",", split(/ /, ($color->($i) // new RGB("255 255 255"))->toFloat);
                $text .= "  \\definecolor{pointcolor\_$id"."_$i}{rgb}{ $vcstring }\n" unless ($color->($i) =~ $Visual::hidden_re);
                ++$i;
            }
        } elsif($color !~ $Visual::hidden_re){
            my $vcstring = join ",", split(/ /, $color->toFloat);
            $text .= "  \\definecolor{pointcolor\_$id}{rgb}{ $vcstring }\n";
            $text .= "  \\tikzstyle{pointstyle\_$id} = [fill=pointcolor\_$id]\n";
        }
    }
    # node border coloring
    if (defined($color=$self->source->VertexBorderColor)){
        $text .= "\n  % LABEL BORDER STYLE\n";
        if(is_code($color)){
            my $i = 0;
            foreach (@{$self->source->Vertices}){
                my $vcstring = join ",", split(/ /, ($color->($i) // new RGB("255 255 255"))->toFloat);
                $text .= "  \\definecolor{vertexbordercolor\_$id"."_$i}{rgb}{ $vcstring }\n" unless ($color->($i) =~ $Visual::hidden_re);
                ++$i;
            }
        } elsif($color !~ $Visual::hidden_re){
            my $vcstring = join ",", split(/ /, $color->toFloat);
            $text .= "  \\definecolor{vertexbordercolor\_$id}{rgb}{ $vcstring }\n";
        }
    }

    # Point definitions
    $text .= "\n  % DEF POINTS\n";
    my $i = 0;
    foreach (@{$self->source->Vertices}) {
        my $point=ref($_) ? Visual::print_coords($_) : "$_".(" 0"x($d-1));
        $point =~ s/\s+/, /g;
        $text .= "  \\coordinate (v$i\_$id) at ($point);\n";
        ++$i;
    }
    $text .= "\n";

    return $text;
}

sub drawPoint {
    my ($self,$i) = @_;
    my $id=$self->id;

    my $point_style=$self->source->VertexStyle;
    return "" if ($point_style =~ $Visual::hidden_re);
    return "" if (is_code($point_style) && ($point_style->($i) =~ $Visual::hidden_re));

    my $point_color=$self->source->VertexColor;
    my $point_labels=$self->source->VertexLabels;
    my $point_thickness=$self->source->VertexThickness;
    my $border_color=$self->source->VertexBorderColor;
    my $border_width=$self->source->VertexBorderThickness;
    my $alignment = $self->source->LabelAlignment;

    $border_width=$border_width->($i) if is_code($border_width);
    $point_labels=$point_labels->($i) if is_code($point_labels);

    my $text;

    my $option_string = (is_code($point_color)) ? "pointcolor\_$id\_$i" : "pointcolor\_$id";
    my $pthick = (is_code($point_thickness)) ? $point_thickness->($i) : $point_thickness;

    my $label_style = "text=black";
    if (defined($border_color) || defined($border_width)) {
        $label_style .= ", inner sep=2pt, rectangle, rounded corners=3pt, fill=$option_string";
        if (defined($border_color)) {
           $label_style .= ", draw=vertexbordercolor_$id";
           $label_style .= "_$i" if (is_code($border_color));
        }
        $label_style .= ", line width=${border_width}pt" if (defined($border_width));
    } else {
        $text .= "  \\fill[$option_string] (v$i\_$id) circle ($pthick pt);\n" if ($pthick ne "");
        $label_style .= ", inner sep=0.5pt, above right, draw=none";
    }


    if (defined($point_labels) && $point_labels !~ $Visual::hidden_re) {
        # heuristic to add math mode for labels: _ or ^ without \ before or $ in the string
        $point_labels = '$'.$point_labels.'$' if ($point_labels !~ /\$/ && $point_labels =~ /(?<!\\)[_^]/);
        $text .= "  \\node at (v$i\_$id) [$label_style, align=$alignment] {$point_labels};\n";
    }
    return $text;
}

sub drawAllPoints {
    my ($self, $trans, $pointset) = @_;
    my $npoints = $self->source->Vertices->rows;
    my @idlist = defined($pointset) ? @$pointset : (0..$npoints-1);
    if ($self->source->Vertices->cols == 3) {
        my $z = (defined($trans) ? $self->source->Vertices * $trans->minor(~[0],~[0]) : $self->source->Vertices)->col(2);
        @idlist = sort { $z->[$b] <=> $z->[$a] } @idlist;
    }
    return join "\n", map {$self->drawPoint($_)} @idlist;
}

sub toString {
    my ($self, $trans)=@_;
    $self->header . $self->pointsToString . $self->drawAllPoints . $self->trailer;
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
    my $line_flag= $style !~ $Visual::hidden_re ? 1 : 0;
    my $thickness=$self->source->EdgeThickness;
    if (!is_code($thickness)){
        $thickness = defined($thickness) ? "line width=".$thickness."pt" : "thick";
    }
    my $color=$self->source->EdgeColor;


    # degree of the vertices, to know when to draw a vertex
    my $vertex_count = new Array<Int>($self->source->n_nodes);
    for(my $e =$self->source->all_edges; $e; ++$e){
        ++$vertex_count->[$e->[0]];
        ++$vertex_count->[$e->[1]];
    }

    my $text ="\n";

    if ($line_flag){
        $text .= "\n  % EDGE STYLE\n";
        if(is_code($color)){
            for (my $e=$self->source->all_edges; $e; ++$e) {
                my $lcstring = join ",", split(/ /, $color->($e)->toFloat);
                $text .= "  \\definecolor{".$self->edgeColorString($e)."}{rgb}{ $lcstring }\n";
            }
        } elsif($color !~ $Visual::hidden_re){
            my $lcstring = join ",", $color->toFloat;
            $text .= "  \\definecolor{".$self->edgeColorString(undef)."}{rgb}{ $lcstring }\n";
        }
        $text .= "\n";

        if(is_code($color) || is_code($thickness) || is_code($arrows)){
            for (my $e=$self->source->all_edges; $e; ++$e) {
               my $lsstring = "color=".$self->edgeColorString($e).", ".$self->edgeThicknessString($e).$self->edgeArrowStyleString($e);
               $text .= "  \\tikzstyle{".$self->edgeStyleString($e)."} = [$lsstring]\n";
            }
        } else {
            my $lsstring = "color=".$self->edgeColorString(undef).", ".$self->edgeThicknessString(undef).$self->edgeArrowStyleString(undef);
            $text .= "  \\tikzstyle{".$self->edgeStyleString(undef)."} = [$lsstring]\n";
        }
        $text .= "\n";

    }

    my $labels=$self->source->EdgeLabels;

    $text .= "\n  % EDGES\n";
    my $c = 0;
    for (my $e=$self->source->all_edges; $e; ++$e) {
        my @vertices_to_draw=();
        my $a=$e->[0]; my $b=$e->[1];
        my $option_string = $self->edgeStyleString($e);
        my $labelstring = defined($labels) ? " node [".$TikZ::default::edgelabelstyle."] {".$labels->($c)."}" : "";
        ++$c;
        if ($line_flag && (!is_code($style) || ($style->($i) !~ $Visual::hidden_re))){
            $text .= "  \\draw[$option_string] (v$a\_$id) -- (v$b\_$id)$labelstring;\n";
        }
        if (!--$vertex_count->[$a]){
            push @vertices_to_draw, $a;
        }
        if (!--$vertex_count->[$b]){
            push @vertices_to_draw, $b;
        }

        ###
        # Drawing points
        if (scalar(@vertices_to_draw)){
            $text.="\n\n  %POINTS\n";
            foreach my $i (@vertices_to_draw) {
                $text .= $self->drawPoint($i);
            }
            $text.="\n\n  %EDGES\n";
        }
    }

    return $text;
}

#TODO: does not work with user specified input
sub edgeArrowStyleString {
   my ($self,$e)=@_;
   my $arrows=$self->source->ArrowStyle;
   my $arrowstring;
   if ((is_code($arrows) && ($arrows->($e)>0)) || (!is_code($arrows) && ($arrows>0))) {
      $arrowstring .= ", arrows = -stealth, shorten >= 1pt";
   } elsif ((is_code($arrows) && ($arrows->($e)<0)) || (!is_code($arrows) && ($arrows<0))) {
      $arrowstring .= ", arrows = stealth-, shorten >= 1pt";
   }
   return $arrowstring;
}

sub edgeColorString {
    my ($self,$e)=@_;
    my $id=$self->id;
    my $color=$self->source->EdgeColor;

    if(is_code($color)){
        my $a=$e->[0]; my $b=$e->[1];
        return "linecolor\_$id\_v$a\_v$b";
    }

    return "linecolor\_$id";
}

sub edgeStyleString {
    my ($self,$e)=@_;
    my $id=$self->id;
    my $color=$self->source->EdgeColor;
    my $thickness=$self->source->EdgeThickness;

    if(is_code($color) || is_code($thickness)){
        my $a=$e->[0]; my $b=$e->[1];
        return "linestyle\_$id\_v$a\_v$b"
    }

    return "linestyle\_$id";
}

sub edgeThicknessString {
    my ($self,$e)=@_;

    my $thickness=$self->source->EdgeThickness;
    return "thick" if (!defined($thickness));

    return (is_code($thickness)) ? "line width=".$thickness->($e)."pt" : "line width=".$thickness."pt";
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
    my $color=$self->source->FacetColor;
    my $face_flag= ($style !~ $Visual::hidden_re) ? 1 : 0;

    my $edge_color=$self->source->EdgeColor;
    my $edge_style=$self->source->EdgeStyle;
    my $edge_flag= ($edge_style !~  $Visual::hidden_re) ? 1 : 0;

    ####
    # Sort Facets (behind to front)
    my @sorted_facets =();
    my @vertex_count=();
    my $facets = new Array<Array<Int>>($self->source->Facets);

    my $vif = new IncidenceMatrix($facets);
    map { push @vertex_count, $vif->col($_)->size } (0..$vif->cols()-1);

    my $nonfacetverts = new Set(sequence(0,$vif->cols));
    $nonfacetverts -= new Set($_) foreach (@$facets);

    my $max_back_facet = $vif->rows-1;

    if (defined($self->source->FacetNormals)){
      my $f = convert_to<Float>(-$self->source->FacetNormals) if defined($self->source->FacetNormals);
      my $m = $f*inv($trans);

      my %hash = ();
      push @{$hash{$m->row($_)->[$m->cols()-1]}},$_ foreach (0..$m->rows()-1);
      @sorted_facets = map { $max_back_facet = $hash{$_}->[-1] if ($_ < 0); @{$hash{$_}}; } sort keys %hash;
    } else {
      @sorted_facets = (0..$vif->rows()-1);
    }

    my $text = "";

    ####
    # Edge Style
    my $edge_optionstring = ", draw=none";
    if($edge_flag){
        # only one edge and thickness color is allowed when visualizing polytopes
        my @edgecolor = (is_code($edge_color)) ? split(/ /, $edge_color->(0)->toFloat) : split(/ /, $edge_color->toFloat);
        my $egstring = join ",", @edgecolor;
        my $thickness = $self->source->EdgeThickness // 1;
        $edge_optionstring = ", draw=edgecolor\_$id, line width=$thickness pt, line cap=round, line join=round";
        $text .= "\n  % EDGES STYLE\n";
        $text .= "  \\definecolor{edgecolor\_$id}{rgb}{ $egstring }\n";
    }


    ####
    # Facet Style
    if ($face_flag){
        $text .= "\n  % FACES STYLE\n";
        # Color definition
        if (is_code($color)){
            foreach my $f(@sorted_facets) {
                my $fcstring = join ",", split(/ /, $color->($f)->toFloat);
                $text .= "  \\definecolor{".$self->facetColorString($f)."}{rgb}{ $fcstring }\n";
            }
        } else {
            my $fcstring = join ",", split(/ /, $color->toFloat);
            $text .= "  \\definecolor{".$self->facetColorString(undef)."}{rgb}{ $fcstring }\n";
        }
        $text .= "\n";
    }

    # Style definition
    if (is_code($color) || is_code($transp) || is_code($style)){
        foreach my $f (@sorted_facets) {
            my $option_string = "fill=".$self->facetColorString($f).", fill opacity=".$self->facetTransp($f);
            $option_string .= $edge_optionstring;
            $text .= "  \\tikzstyle{".$self->facetStyleString($f)."} = [$option_string]\n";
        }
    } else {
        my $option_string = "fill=".$self->facetColorString(undef).", fill opacity=".$self->facetTransp(undef);
        if($self->facetColorString(undef) eq "none"){
           my $thickness = $self->source->EdgeThickness // 1;
           $option_string .=  ", preaction={draw=white, line cap=round, line width=".(1.5*$thickness)." pt}" 
        }
        $option_string .= $edge_optionstring;
        $text .= "  \\tikzstyle{".$self->facetStyleString(undef)."} = [$option_string]\n";
    }
    $text .= "\n";

    ####
    # Draw Facets Edges and Points
    $text .= "\n  % FACES and EDGES and POINTS in the right order\n";
    foreach my $facet(@sorted_facets) {
        my @vertices_to_draw=();
        my $option_string = $self->facetStyleString($facet);
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
        if (scalar(@vertices_to_draw)){
            $text.="\n\n  %POINTS\n";
            foreach my $i (@vertices_to_draw) {
                $text .= $self->drawPoint($i);
            }
            $text.="\n\n  %FACETS\n";
        }
        if ($max_back_facet == $facet && $nonfacetverts->size > 0) {
            my $allpoints = $self->drawAllPoints($trans,$nonfacetverts);
            if ($allpoints !~ /^\s*$/) {
                $text .= "\n%ALL NON-VERTICES\n";
                $text .= $allpoints."\n";
            }
        }
    }
    return $text;
}

sub facetColorString {
    my ($self,$f)=@_;
    my $id=$self->id;
    my $color=$self->source->FacetColor;
    my $style=$self->source->FacetStyle;

    if ($color =~ $Visual::hidden_re || $style =~ $Visual::hidden_re){
        return "none"
    }

    return "facetcolor\_$id" if (!defined($f));

    if (is_code($style) && ($style->($f) =~ $Visual::hidden_re)){
        return "none"
    }

    if (is_code($color)){
        return "facetcolor\_$id\_$f";
    }
    return "facetcolor\_$id";
}

sub facetStyleString {
    my ($self,$f)=@_;
    my $id=$self->id;
    my $color=$self->source->FacetColor;
    my $transp=$self->source->FacetTransparency;
    my $style=$self->source->FacetStyle;

    if(is_code($color) || is_code($transp) || is_code($style)){
        return "facestyle\_$id\_$f";
    }
    return "facestyle\_$id";
}

sub facetTransp {
    my ($self,$f)=@_;

    my $transp=$self->source->FacetTransparency;
    return "0.85" if (!defined($transp));

    return (is_code($transp)) ? "".$transp->($f) : "".$transp;
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
