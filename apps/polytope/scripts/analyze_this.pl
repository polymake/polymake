#  Copyright (c) 1997-2018
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

# This script is meant for non-expert users to get some idea about a polytope they are
# dealing with.  Supposed to become (a bit) more sophisticated.

use application "polytope";

sub header() {
  return << ".";
\\documentclass{article}
\\usepackage{tikz,graphicx,xspace,url}
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

sub tikzfigure($$$) {
  my ($poly,$name,$tikzfile)=@_;
  tikz($poly->VISUAL(VertexLabels=>"hidden"),File=>$tikzfile);
  return << ".";
\\begin{figure}[ht]
\\centering
\\resizebox{0.67\\textwidth}{!}{\\input $tikzfile}
\\caption{This is how $name looks like}
\\label{fig}
\\end{figure}
.
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
The $name is a $DIM-dimensional polyhedron, it lives in $AMBIENT_DIM-space, and it is
.
  if ($poly->BOUNDED) {
    print TEX << ".";
bounded.
.
  } else {
    print TEX << ".";
unbounded.  All combinatorial data refers to the projective closure of the quotient by the lineality space.
This is always a polytope. This means that \\polymake treats unbounded polyhedra as polytopes with a marked face (at infinity).
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
  if ($AMBIENT_DIM<=4) { # allows for direct visualization
    if ($AMBIENT_DIM<=3) {
      print TEX << ".";
A visualization is shown in Figure~\\ref{fig}.
.
    } else {
      print TEX << ".";
A Schlegel diagram, projected onto the facet which happens to appear first in the list of \\texttt{FACETS},
is shown in Figure~\\ref{fig}.  Notice that projecting onto another facet may result in a very different
picture.
.
    }
    print TEX tikzfigure($poly,$name,$tikzfile);
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
