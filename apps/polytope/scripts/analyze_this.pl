#  Copyright (c) 1997-2023
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

# This script is meant for non-expert users to get some idea about a polytope they are
# dealing with.  Supposed to become (a bit) more sophisticated.

use application "polytope";

sub header() {
  return << ".";
\\documentclass{article}
\\usepackage{tikz,graphicx,xspace,url,csquotes}
\\newcommand\\polymake{\\texttt{polymake}\\xspace}
\\title{\\polymake description of a big object of type \\texttt{Polytope}}
\\begin{document}
\\maketitle
\\noindent
.
}

sub footer() {
  return << ".";

For details on how to use \\polymake see the tutorial section at \\url{www.polymake.org}
and the forum at \\url{forum.polymake.org}.
\\end{document}
.
}

sub tikzfigure($$$$) {
  my ($poly,$vis,$name,$tikzfile)=@_;
  tikz($poly->$vis(VertexLabels=>"hidden"),File=>$tikzfile);
  return << ".";
\\begin{figure}[ht]
\\centering
\\resizebox{0.67\\textwidth}{!}{\\input $tikzfile}
\\caption{This is how \\enquote{$name} looks like}
\\label{fig}
\\end{figure}
.
}

sub palindromic($) {
  my ($v)=@_;
  my $len=$v->dim();
  my $pal=true;
  for (my $i=0; $i<$len/2; ++$i) {
    $pal = false if $v->[$i] != $v->[$len-$i-1];
  }
  return $pal;
}

sub analyze_this($) {
  my ($poly) = @_;
  my $pdflatex = $Visual::pdflatex;
  my $pdfviewer = $Visual::pdfviewer;
  my $tempfile = new Tempfile();
  my $latextemplate = $tempfile.".tex";
  my $tikzfile = $tempfile.".tikz";
  my $pdfout = $tempfile.".pdf";
  my $pdfout_dir = $tempfile->dirname;
  my $pdfout_name = $tempfile->basename;

  print STDERR "writing to $latextemplate\n";
  
  my $name = $poly->description();
  if ($name eq "") {
    $name = defined($poly->name)? $poly->name : "unnamed polyhedron";
  }
  my ($AMBIENT_DIM, $DIM) = map { $poly->$_ } ("AMBIENT_DIM", "DIM");
  my $simplicial = $poly->SIMPLICIAL ? "simplicial" : "not simplicial";
  my $simple = $poly->SIMPLE ? "simple" : "not simple";
  my $cubical = $poly->CUBICAL ? "cubical" : "not cubical";
  open TEX, ">$latextemplate" or die "analyze_this: cannot write $latextemplate";
  print TEX header();

  print TEX << ".";
The object called \\enquote{$name} describes a $DIM-dimensional polyhedron, which lives in $AMBIENT_DIM-space, and which is
.
  if ($poly->BOUNDED) {
    print TEX << ".";
bounded.
.
  } else {
    print TEX << ".";
unbounded.  All combinatorial data refers to the projective closure of the quotient by the lineality space.
This is always a polytope. This means that \\polymake treats unbounded polyhedra like polytopes with a marked face (at infinity).
.
  }
  print TEX << ".";
Its \$f\$-vector reads
\\[
.
  print TEX "(", (join ",", @{$poly->F_VECTOR}), ") \\enspace .";
  print TEX << ".";
\\]
The polytope is $simplicial, $simple and $cubical.
.
  if (palindromic($poly->F_VECTOR)) {
    if (isomorphic($poly->VERTICES_IN_FACETS,$poly->FACETS_THRU_VERTICES)) {
      print TEX << ".";
It is selfdual, which is a rather rare property.
.
    } else {
      print TEX << ".";
While the \$f\$-vector is palindromic, the polytope is not selfdual.
.
    }
  }
  print TEX << ".";

.
  if ($AMBIENT_DIM<=3) {
    print TEX << ".";
A direct visualization in $3$-space is shown in Figure~\\ref{fig}.
.
    print TEX tikzfigure($poly,"VISUAL",$name,$tikzfile);
  } elsif ($AMBIENT_DIM<=4) {
    print TEX << ".";
A Schlegel diagram, projected onto the facet which happens to appear first in the list of \\texttt{FACETS},
is shown in Figure~\\ref{fig}.  Notice that projecting onto another facet may result in a very different
picture.
.
    print TEX tikzfigure($poly,"VISUAL",$name,$tikzfile);
  } else {
    print TEX << ".";
The dimension of the polytope is too high to allow for any canonical visualization. 
Yet it may be possible that there are interesting pictures which reveal geometric or combinatorial properties.
One way is to visualize the dual graph, and this is shown in Figure~\\ref{fig}; but this choice is rather arbitrary.
.
    print TEX tikzfigure($poly,"VISUAL_DUAL_GRAPH",$name,$tikzfile);
  }

  print TEX footer();
  close TEX;

  # LaTeX twice to get references right
  system("$pdflatex --output-directory=$pdfout_dir --jobname=$pdfout_name $latextemplate 1>/dev/null");
  system("$pdflatex --output-directory=$pdfout_dir --jobname=$pdfout_name $latextemplate 1>/dev/null");

  print STDERR "viewing $pdfout\n";
  system("$pdfviewer $pdfout");
}


# Local Variables:
# c-basic-offset:3
# mode: cperl
# End:
