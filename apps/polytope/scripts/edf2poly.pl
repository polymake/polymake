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

# Read file format for circuits and turn it into Valiant polytope.
#
# Valiant: Reducibility by algebraic projections, Enseign. Math. (2) 28 (1982):3-4, pp. 253-268, MR684236 (84d:68046b)
#
# File format for non-CNF logical expressions described in:
# http://www.satcompetition.org/2005/edimacs.pdf

use application "polytope";

declare $debug=0;

sub And($$$$) {
  my ($out, $in1, $in2, $d) = @_;
  my $m = new Matrix(3,$d+1);
  my ($ain1,$ain2)=(abs($in1),abs($in2));
  # 1 + out - in1 - in 2 >= 0
  $m->elem(0,0) = 1; $m->elem(0,$out) = 1; $m->elem(0,$ain1) = -1; $m->elem(0,$ain2) = -1;
  $m->elem(1,0) = 0; $m->elem(1,$out) = -1, $m->elem(1,$ain1) = 1;
  $m->elem(2,0) = 0; $m->elem(2,$out) = -1, $m->elem(2,$ain2) = 1;
  if ($in1<0) { # subs in1 -> (1-in1)
      $m->col(0) += new Vector([-1,1,0]);
      $m->col($ain1) *= -1;
  }
  if ($in2<0) { # subs in2 -> (1-in2)
      $m->col(0) += new Vector([-1,0,1]);
      $m->col($ain2) *= -1;
  }
  return $m;
}

sub Or($$$$) {
  my ($out, $in1, $in2, $d) = @_;
  my $m = new Matrix(3,$d+1);
  my ($ain1,$ain2)=(abs($in1),abs($in2));
  # - out + in1 + in 2 >= 0
  $m->elem(0,0) = 0; $m->elem(0,$out) = -1; $m->elem(0,$ain1) = 1; $m->elem(0,$ain2) = 1;
  $m->elem(1,0) = 0; $m->elem(1,$out) = 1, $m->elem(1,$ain1) = -1;
  $m->elem(2,0) = 0; $m->elem(2,$out) = 1, $m->elem(2,$ain2) = -1;
  if ($in1<0) { # subs in1 -> (1-in1)
      $m->col(0) += new Vector([1,-1,0]);
      $m->col($ain1) *= -1;
  }
  if ($in2<0) { # subs in2 -> (1-in2)
      $m->col(0) += new Vector([1,0,-1]);
      $m->col($ain2) *= -1;
  }
  return $m;
}

my %dispatch=(
	      '4',  \&And,
	      '6',  \&Or,
);

sub edf2poly($) {
    my @all_constraints;
    my $filename=$_[0];
    open EDF, $filename or die "edf2poly: cannot read $filename";
    my $header = <EDF>; # reads first line
    my $d=0;
    $d=$1 if $header =~ /p\s+noncnf\s+(\d+)/;
    print "header=$header\nd=$d\n" if $debug;
    die "edf2poly: header invalid or no variables" unless $d>0;
    foreach (<EDF>) {
	my ($op,$out,$in1,$in2) = m/^\s*(\d+)\s+-1\s+(\d+)\s+(-?\d+)\s+(-?\d+)\s+0\s*$/;
	print "op=$op out=$out in1=$in1 in2=$in2\n" if $debug;
	if (defined($in2)) {
	    my $these_constraints = &{$dispatch{$op}}($out,$in1,$in2,$d);
	    push @all_constraints, @{rows($these_constraints)};
	} else {
	    die "edf2poly: can't parse <$_>";
	}
    }
    close EDF;
    push @all_constraints, @{rows(cube($d,0)->FACETS)};
    return new Polytope(INEQUALITIES=>new Matrix(@all_constraints), CONE_AMBIENT_DIM=>$d+1);
}
