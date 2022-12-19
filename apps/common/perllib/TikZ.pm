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
   } elsif ($self->geometries->[0]->isa("TikZ::PhylogeneticTree")) {
      my $maxX;
      foreach my $coord (@{$self->geometries->[0]->source->Vertices}) {
         assign_max($maxX, $coord->[0]);
      }
      my $xscale = 1/$maxX*5;
      $result .= <<".";
\\begin{tikzpicture}[x  = {(1cm, 0em)},
                    y  = {(0em, 1cm)},
                    scale = 1,
                    xscale = $xscale,
                    color = {lightgray}]

\\tikzstyle{every node}=[font=\\small]

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
   [ '$nodecode' => '0' ],
);

# name without whitespace and such
# notice that "-", "{", and "}" occur in names of simplices of a TRIANGULATION and "#" in unnamed subobjects
sub id {
   # this is used to make all TikZ ids globally unique
   my ($self)=@_;
   my $id=$self->name;
   $id =~ s/[\#\s\{\}-]+//g;
   return "_$id";
}

sub header {
}

sub tikzstyle {
    my ($self,$tikzid,$optionstring) = @_;
    "  \\tikzstyle{$tikzid} = [$optionstring]\n"
}

sub trailer {
}

sub uniqueColors {
    #defines colors only once and sets colormap to either a static value or code 
    #useful only if tikzstyles per index/edge are defined and used later
    my ($self,$decorname,$default,$number) = @_;
    my $id = $self->id;
    my $color = $self->source->$decorname; 
    my $tikznamebase = lc($decorname)."$id"; 
    my $text = "\n  % ".uc($decorname)."\n";
    my $colormap;
    if (is_code($color)) {
        my %colorhash;
        # EdgeColor
        if ($decorname eq "EdgeColor") {
            for (my $e=$self->source->all_edges; $e; ++$e) {
                my $col = $color->($e) // $default;
                $col = $col->toFloat;
                if (!exists $colorhash{$col}) {
                    $colorhash{$col} = ();
                }
                push @{$colorhash{$col}}, $e;
            }
            my $c = 0;
            my $edgemap = {};
            my $size = keys %colorhash;
            foreach my $col (sort keys %colorhash) {
                my $tikzid = $tikznamebase;
                $tikzid .= ($size > 1) ? "_$c" : "";
                foreach my $e (@{$colorhash{$col}}) {
                    my $key = "@$e";
                    $edgemap->{$key} = $tikzid; 
                }
                $text .= "  \\definecolor{$tikzid}{rgb}{ $col }\n";
                ++$c;
            }
            $colormap = sub { my $e = shift; my $key = "@$e"; $edgemap->{$key} };   
            
        # VertexColor & VertexBorderColor & FacetColor
        } else {
            foreach (0..$number-1) {
                my $col = $color->($_) // $default;
                $col = $col->toFloat;
                if (!exists $colorhash{$col}) {
                    $colorhash{$col} = ();
                }
                push @{$colorhash{$col}}, $_
            }
            my $c = 0;
            my @arraymap;
            my $size = keys %colorhash;
            foreach my $col (sort keys %colorhash) {
                my $tikzid = $tikznamebase;
                $tikzid .= ($size > 1) ? "_$c" : "";
                foreach my $n (@{$colorhash{$col}}) {
                    $arraymap[$n] = $tikzid; 
                }
                $text .= "  \\definecolor{$tikzid}{rgb}{ $col }\n";
                ++$c;
            }
            $colormap = sub { my $i = shift; $arraymap[$i] };   
        }
    } else {
        my $col = $color // $default;
        $text .= "  \\definecolor{$tikznamebase}{rgb}{ $col }\n";
        $colormap = $tikznamebase;
    }
    return ($text,$colormap);
}

sub pointsToString {
    my ($self,$indices,$withlabels) = @_;
    my $text = "";
    $text .= "\n  % POINTS\n";
    my $labels = $self->source->VertexLabels;
    if (@$indices>=$TikZ::default::foreachminimum) {
        # consecutive check has a very small usecase atm   
        my $consecutive = 1;
        foreach my $i (0..$indices-2) {
            if ($indices->[$i+1] != $indices->[$i]+1) {
                $consecutive = 0;
                last;
            }
        }
        
        my $loopvars = "\\i";
        my $looptext = "";
        my $arraytext;
        if ($withlabels) {
            my @array;
            $loopvars .= "/\\label";
            my $labelvar = "\\label";
            foreach my $i (@$indices) {
                my $label = $labels->($i);
                $label = Utils::tikzlabel($label);
                $array[$i] = "$i/$label";
            }  
            $arraytext = join(",",@array);
            $looptext .= $self->tikznode("\\i",$labelvar);
        } else {
            $arraytext = ($consecutive) ? "$indices->[0]..$indices->[-1]" : join(",",@$indices);
            $looptext .= $self->tikznode("\\i");
        }
        $text .= "  \\foreach $loopvars in {$arraytext} {\n $looptext  }\n\n";
    } else {
        foreach my $i (@$indices) {
            my $label = ($withlabels) ? Utils::tikzlabel($labels->($i)) : undef;
            $text .= $self->tikznode($i,$label);
        }
        $text .= "\n";
    }
    return $text;
}

sub tikznode {
    my ($self,$index,$label) = @_;
    my $id = $self->id;
    my $tikzid = $id;
    $tikzid .= "_$index" if ($self->nodecode);
    "   \\node at (v$index$id) [vertexstyle$tikzid] {$label};\n";
}


sub vertexStylesToString {
    my ($self, $labelposition)=@_;
    my $id = $self->id;
    my $text = "";
    my $number = @{$self->source->Vertices};
    my ($vcolortext,$vcolors) = $self->uniqueColors("VertexColor",new RGB("255 255 255"),$number);        
    $text .= $vcolortext;
    $text .= "\n  % DEF VERTEXSTYLES\n";
    my $thickness = $self->source->VertexThickness // 1;
    my $labels = $self->source->VertexLabels;
    my $label_flag = (defined($labels) && $labels !~ $Visual::hidden_re && $labels ne "") ? 1 : 0;
    $labelposition //= "above right";
    my $alignment = $self->source->LabelAlignment;

    if (is_code($vcolors) || is_code($thickness) || ($label_flag && is_code($labels))) {
        $self->nodecode = 1; 
        foreach my $i (0..$number-1) {
            my $optionstring = "";
            my $colid = is_code($vcolors) ? $vcolors->($i) : $vcolors;
            my $thick = is_code($thickness) ? $thickness->($i) : $thickness;
            $thick /= 4;
            if ($thick) {
                $optionstring .= "circle, scale=$thick, fill=$colid,";
            } else {
                $optionstring .= "draw=none,";
            }
            if ($label_flag) {
                my $label = is_code($labels) ? $labels->($i) : $labels;
                $label = Utils::tikzlabel($label);
                $optionstring .= "label={[text=black, $labelposition, align=$alignment]:$label},"; 
            }
            $text .= $self->tikzstyle("vertexstyle$id\_$i",$optionstring);
        }
    } else {
        my $optionstring = "";
        $optionstring .= $thickness ? "circle, scale=".($thickness/4)."pt, fill=$vcolors," : "draw=none,";
        if ($label_flag) {
            #labels are not code
            my $label = Utils::tikzlabel($labels);
            $optionstring .= "label={[text=black, $labelposition, align=$alignment]:$label}"; 
        }
        $text .= $self->tikzstyle("vertexstyle$id",$optionstring);
    }
    return $text;
}

sub coordinatesToString {
    my ($self)=@_;
    my $id = $self->id; # this is used to make all TikZ ids globally unique
    # Coordinate definitions
    my $d= is_object($self->source->Vertices) ? $self->source->Vertices->cols : 3;
    my $text = "\n  % DEF COORDINATES\n";
    my $i = 0;
    foreach (@{$self->source->Vertices}) {
        my $point=ref($_) ? Visual::print_coords($_) : "$_".(" 0"x($d-1));
        $point =~ s/\s+/, /g;
        $text .= "  \\coordinate (v$i$id) at ($point);\n";
        ++$i;
    }
    $text .= "\n";

    return $text;
}

sub drawAllPoints {
    my ($self, $trans, $pointset) = @_;
    my $points = $self->source->Vertices;
    $points = is_object($points) && $points->isa("Visual::DynamicCoords") ? $points->get_matrix : $points; 
    my $npoints = @{$self->source->Vertices};
    my @idlist = defined($pointset) ? @$pointset : (0..$npoints-1);
    if ($self->source->Vertices->cols == 3) {
        my $z = (defined($trans) ? $points * $trans->minor(~[0],~[0]) : $points)->col(2);
        @idlist = sort { $z->[$b] <=> $z->[$a] } @idlist;
    }
    return $self->pointsToString(\@idlist);
}

sub toString {
    my ($self, $trans)=@_;
    my $text = "";
    $text .= $self->header;
    $text .= $self->coordinatesToString; 
    if ($self->source->VertexStyle !~ $Visual::hidden_re) {
        $text.= $self->vertexStylesToString . $self->drawAllPoints;
    }
    $text .= $self->trailer;
}

##############################################################################################

#  Wire model (e.g. a graph)

package TikZ::Wire;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
   [ '$edgecode' => '0' ],
);

sub tikzedge {
    my ($self,$e,$label) = @_;
    my $a=$e->[0]; my $b=$e->[1];
    my $id = $self->id;
    my $text = "";
    my $tikzid = "edgestyle$id";
    $tikzid .= "_$a\_$b" if ($self->edgecode);
    $text .= "  \\draw[$tikzid] (v$a$id) -- (v$b$id)";
    $text .= " node [edgelabelstyle$id\_$a\_$b] {$label}" if (defined($label) && $label ne "");
    $text .= ";\n";
    return $text;
}

sub edgesToString {
    # requires an array of edges
    my ($self,$edges) = @_;
    my $text = "";
    $text .= "\n  % EDGES\n";
    my $labels = $self->source->EdgeLabels;
    my $label_flag = (defined($labels) && $labels !~ $Visual::hidden_re && $labels ne "") ? 1 : 0;
    if (@$edges>=$TikZ::default::foreachminimum) {
        my $loopvars = "\\i/\\k";
        my $looptext = "";
        my $labeltext;
        my $arraytext;
        my @array = map { $_->[0]."/".$_->[1] } @$edges;
        if ($label_flag) {
            $loopvars .= "/\\label";
            $labeltext = "\\label";
            foreach my $i (0..@$edges-1) {
                my $label = $labels->($edges->[$i]);
                $label = Utils::tikzlabel($label);
                $array[$i] .= "/$label";
            }  
        }
        $looptext .= $self->tikzedge(["\\i","\\k"],$labeltext);
        
        $arraytext = join(",",@array);
        $text .= "\n  \\foreach $loopvars in {$arraytext} {\n $looptext  }\n\n";
    } else {
        foreach my $i (0..@$edges-1) {
            my $label = ($label_flag) ? Utils::tikzlabel($labels->($edges->[$i])) : undef;
            $text .= $self->tikzedge($edges->[$i],$label);
        }
        $text .= "\n";
    }
    return $text;
}

sub edgeStylesToString {
    my ($self)=@_;
    my $id = $self->id;
    my $text = "";
    my ($ecolortext,$ecolors) = $self->uniqueColors("EdgeColor",new RGB("0 0 0"));        
    $text .= $ecolortext;
    my $arrows=$self->source->ArrowStyle;
    my $thickness = $self->source->EdgeThickness;
    my $labels = $self->source->EdgeLabels;
    my $label_flag = (defined($labels) && $labels !~ $Visual::hidden_re && $labels ne "") ? 1 : 0;

    if (is_code($ecolors) || is_code($thickness) || is_code($arrows)) {
        $self->edgecode = 1; 
        foreach (my $e=$self->source->all_edges; $e; ++$e) {
            my $optionstring = "";
            my $colid = is_code($ecolors) ? $ecolors->($e) : $ecolors;
            my $thick = is_code($thickness) ? $thickness->($e) : $thickness;
            my $arrow = is_code($arrows) ? $arrows->($e) : $arrows;
            
            $thick = defined($thick) ? "line width=$thick"."pt," : "thick";
            $optionstring .= "$thick, color=$colid,";
            if ($arrow) {
                $optionstring .= ($arrow>0) ? ",arrows=-stealth,shorten>=1pt" : ",arrows=stealth-,shorten >=1pt";
            }
            $text .= $self->tikzstyle("edgestyle$id\_$e->[0]\_$e->[1]",$optionstring);
        }
    } else {
        my $optionstring = "";
        $optionstring .= defined($thickness) ? "line width=$thickness"."pt," : "thick";
        $optionstring .= ",color=$ecolors";
        if ($arrows) {
            $optionstring .= ($arrows>0) ? ",arrows=-stealth,shorten >= 1pt" : ",arrows=stealth-,shorten >= 1pt";
        }
        $text .= $self->tikzstyle("edgestyle$id",$optionstring);
    }
    
    if ($label_flag) {
        my $labeloptionstring = $TikZ::default::edgelabelstyle;
        foreach (my $e=$self->source->all_edges; $e; ++$e) {
            #simply write a styleid for easier repositioning of edgelabels by hand 
            $text .= $self->tikzstyle("edgelabelstyle$id\_$e->[0]\_$e->[1]",$labeloptionstring);
        }
    }
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
    $text .= $self->vertexStylesToString if ($drawvertices);
    $text .= $self->edgeStylesToString . $self->drawAllEdges if ($drawedges);
    $text .= $self->drawAllPoints if ($drawvertices);
    $text .= $self->trailer;
    return $text;
}


##############################################################################################
#
#  Solid 2-d or 3-d body
#
package TikZ::Solid;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
   [ '$facetcode' => '0' ],
);

sub drawFacet {
    my ($self, $index)=@_;
    my $id = $self->id;
    my $text = "";
    my $tikzid = "facetstyle$id";
    my $facet = $self->source->Facets->[$index];
    if ($self->facetcode) {
        $tikzid .= "_$index";
    }
    $text .= "  \\draw[$tikzid]";
    foreach my $vertex (@{$facet}) {
        $text.=" (v$vertex$id) --";
    }
    my $first_v=$facet->[0];
    $text.=" (v$first_v$id) -- cycle;\n";
    return $text; 
}

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
    my $point_style=$self->source->VertexStyle;
    my $point_flag= ($point_style !~ $Visual::hidden_re) ? 1 : 0;
    my $nfacets = @{$self->source->Facets};
    my $nvertices = $self->source->Vertices->rows;

    #### sorting
    my @bfacets = ();
    my @ffacets = ();
    my $bverts = new Set<Int>([0..$nvertices-1]);
    my $fverts = new Set<Int>();

    if (defined($self->source->FacetNormals)){
        my $f = convert_to<Float>($self->source->FacetNormals);
        my $m = $f*inv($trans);
        foreach my $i (0..$nfacets-1) {
            if ($m->row($i)->[-1] <= 0) {
                push @ffacets, $i;
                my $vset = new Set<Int>($self->source->Facets->[$i]);
                $fverts+=$vset;
                $bverts-=$vset;
            } else {
                push @bfacets, $i;
            }
        }
    } else {
        @bfacets = (0..$nfacets-1);
    }

    my $text = "";

    $text .= "\n  % FACES and EDGES and POINTS in the right order\n";
    if ($edge_flag || $face_flag) {
        # Draw rear facets and edges
        foreach my $i (@bfacets) {
            $text.=$self->drawFacet($i);
        }
    }
    if ($point_flag && @$bverts) {
        # Draw rear vertices in z order
        $text .= $self->drawAllPoints($trans,$bverts)   
    }
    if ($edge_flag || $face_flag) {
        # Draw front facets and edges
        foreach my $i (@ffacets) {
            $text .= $self->drawFacet($i);
        }
    }
    if ($point_flag && @$fverts) {
        # Draw front vertices in z order
        $text .= $self->drawAllPoints($trans,$fverts)   
    }
    return $text;
}

sub facetStylesToString {
    #this is also handling edgestyles
    my ($self)=@_;
    my $id = $self->id;
    my $style=$self->source->FacetStyle;
    my $face_flag= ($style !~ $Visual::hidden_re) ? 1 : 0;
    my $edge_style=$self->source->EdgeStyle;
    my $edge_flag= ($edge_style !~  $Visual::hidden_re) ? 1 : 0;
    if ($edge_flag || $face_flag) {
        my $text = "";
        my $nfacets = @{$self->source->Facets};
        my $transparency = $self->source->FacetTransparency // 0.85;
        my ($fcolortext,$fcolors) = $self->uniqueColors("FacetColor",new RGB("0 0 0"),$nfacets);        
        $text .= $fcolortext;
        my $edge_optionstring = ", draw=none";
        if ($edge_flag) {
            # only one edge thickness and color is allowed when visualizing polytopes
            my $edge_color = $self->source->EdgeColor;
            my $ecol = is_code($edge_color) ? $edge_color->(0) : $edge_color;
            $ecol = $ecol->toFloat;
            my $thickness = $self->source->EdgeThickness // 1;
            $edge_optionstring = ", draw=edgecolor$id, line width=$thickness pt, line cap=round, line join=round";
            $text .= "\n  % EDGECOLOR\n";
            $text .= "  \\definecolor{edgecolor$id}{rgb}{ $ecol }\n";
        }
        
        if ($face_flag) {
           if (is_code($fcolors) || is_code($transparency)) {
               $self->facetcode = 1; 
               foreach my $i (0..$nfacets-1) {
                   my $optionstring = "";
                   my $colid = is_code($fcolors) ? $fcolors->($i) : $fcolors;
                   my $transp = is_code($transparency) ? $transparency->($i) : $transparency;
                   
                   $optionstring .= "fill=$colid, fill opacity=$transp";
                   $optionstring .= $edge_optionstring;
                   
                   $text .= $self->tikzstyle("facetstyle$id\_$i",$optionstring);
               }
           } else {
               my $optionstring = "";
               $optionstring .= "fill=$fcolors, fill opacity=$transparency";
               $optionstring .= $edge_optionstring;
               $text .= $self->tikzstyle("facetstyle$id",$optionstring);
           }
        } elsif ($edge_flag) {
            #$option_string .=  ", preaction={draw=white, line cap=round, line width=".(1.5*$thickness)." pt}" 
            $text .= $self->tikzstyle("facetstyle$id","fill=none ,".$edge_optionstring);
        }
        return $text;
    } else {
        return ""
    }
}

sub toString {
    my ($self, $transform)=@_;
    my $text = "";
    $text .= $self->header;
    $text .= $self->coordinatesToString; 
    if ($self->source->VertexStyle !~ $Visual::hidden_re) {
        $text .= $self->vertexStylesToString;
    }
    $text .= $self->facetStylesToString;
    $text .= $self->facesToString($transform);
    $text .= $self->trailer;
}


package TikZ::Utils;

sub tikzlabel {
    my ($label) = @_;
    # heuristic to add math mode for labels: unescaped _ or ^ or / makes it a math mode label (if it isn't yet)
    $label = '$'.$label.'$' if ($label !~ /\$/ && $label =~ /(?<!\\)[_^\/]/);
    $label =~ s/\n/\\\\/g; # replace \n with \\ which will be used as a linebreak if align is set (doesn't work inside $$)
    # multiline mathmode labels are done like this: $firstline$\\$secondline$ 
    $label =~ s/\//\\slash/g; # replace \ with \slash (actually only required for labels in foreach loops ... edgelabels and HD-nodelabels)
    return $label;
}




1

# Local Variables:
# mode:perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
