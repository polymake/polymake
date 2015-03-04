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
#-----------------------------------------------------------------------------
#

use application 'polytope';

# Determines the minimal entry of a vector.
sub vecMin {
    my $vec=$_[0];
    my $min=$vec->[0]; 
    foreach(@$vec){
	if($_<$min){
	    $min=$_;
	}
    }
    return $min;
}

# Removes the (automatically added) trivial inequality
# 1,0,...,0 from a given set of inequalities. 
sub removeTrivIneq {
    my $ineqs_in=$_[0];
    my $ineqs_wo_triv=$ineqs_in;

if($ineqs_in->cols!=0){    
my $triv_ineq_row=-1;
    my $triv_ineq=new Vector<Rational>(zero_vector<Rational>($ineqs_in->cols-1));
    $triv_ineq=1|$triv_ineq;
    for(my $i=0;$i<$ineqs_in->rows;$i++){
	if($ineqs_in->row($i)==$triv_ineq){
	    $triv_ineq_row=$i;
	}
    }    
    if($triv_ineq_row!=-1){
	my $indices=new Set<Int>(0..$ineqs_in->rows-1);
	$indices-=$triv_ineq_row;
        $ineqs_wo_triv=new Matrix<Rational>($ineqs_in->minor($indices,All));
    }
}
    return $ineqs_wo_triv;
}

# Converts a (rational) matrix (of inequalities) to an integer matrix
# and translates the inequality system into a system of equations
# by adding slack variables.
sub ineqs2IntEqsWithSlacks {
    my ($ineqs_in) = @_;
    my $ineqs=convert_to<Int>(primitive($ineqs_in));
    my $slacks=new Matrix<Int>(unit_matrix<Int>($ineqs->rows));
    my $eqs=$ineqs|(-$slacks);
    return $eqs;
}

# Checks whether a matrix contains only nonnegative entries.
# @return Bool true, if all entries are greater or equal than zero.
sub checkNonNeg {
    my ($eqs)=@_;
    my $answer=1;
    for(my $i=0;$i<$eqs->rows;$i++){
	for(my $j=0;$j<$eqs->cols;$j++){
	    if($eqs->[$i]->[$j]<0){
		$answer=0;
		return $answer;
	    }
	}
    }
    return $answer;
}

# Generates the (Conti-Traverso) ideal <f_i-x_i> 
# (or <f_i-x_i,w_1*...*w_m*t> for matrices with negative entries)
# and the monomial for the right-hand side
# from a given (integer) system of equations. 
sub eqSys2ContiTrav {
    my ($eqs_in)=@_; #eqs must be an integer matrix!
    my $eqs=convert_to<Int>($eqs_in);
    if($eqs_in!=$eqs){
	die "Input matrix was not integer!";
    }
    $eqs->col(0)=-$eqs->col(0);# the first col is the right hand side of an eq system and must be multiplied by -1
    my $isNonNeg=checkNonNeg($eqs);
    my @vars=map { "w$_" } (1..$eqs->rows);
    if(!$isNonNeg){
	@vars = ( @vars, "t");
    }
    @vars=( @vars , map { "x$_" } (1..$eqs->cols-1) );
    my $r=new Ring(@vars);
    my @ideal;
    my $isRHS=1;# the first col is the right hand side of eqs
    my $bPoly;
    for(my $j=0;$j<$eqs->cols;$j++){
	my $monomVec=$eqs->col($j);
	if(!$isNonNeg){
	    $monomVec=$monomVec|(new Vector<Int>(zero_vector<Int>(1)));
	    my $minEntry=vecMin($monomVec);
	    if($minEntry<0){
		$monomVec=$monomVec-$minEntry*ones_vector<Int>($eqs->rows+1);
	    }
	}
	$monomVec=$monomVec|(new Vector<Int>(zero_vector<Int>($eqs->cols)));
	if($isRHS){
	    $bPoly=new Monomial($monomVec,$r);
	    $isRHS=0;
	} else {
	my $x_i_pos=$eqs->rows+(1-$isNonNeg)+($j-1);
	my $x_i_mono=new Vector<Int>(unit_vector<Int>($monomVec->dim,$x_i_pos));
	my $polyMatrix=new Matrix<Int>($monomVec,$x_i_mono);
	my $coeffs=new Vector<Rational>(1,-1);
	my $poly=new Polynomial($polyMatrix,$coeffs,$r);
	push(@ideal,$poly);
	}
    }

    if(!$isNonNeg){	
	my $negMonoVec=new Vector<Int>((ones_vector<Int>($eqs->rows+1))|(zero_vector<Int>($eqs->cols)));
	my $negMono=new Monomial($negMonoVec,$r);
	my $negPoly=$negMono-1;
	push(@ideal,$negPoly);
    }

    return [\@ideal,$bPoly,$r,\@vars];

}

# Produces the Singular input file for the Conti-Traverso procedure
# from the ideal and the RHS monomial generated in 'eqSys2ContiTrav'.
# The given cost vector c gives rise to an appropriate monomial ordering.
sub generateContiTravSingularInput {
    my ($varsRef,$xcols,$c,$idealRef,$bPoly,$filename)=@_; #FIXME: pass ring if it is possible to get number of variables in some way
    my @ideal=@$idealRef;
    my @vars=@$varsRef;
    open(OUT, ">$filename")
    or die "can't create outputfile $filename: $!";
    print(OUT "ring R = 0, (". join(",",@vars) . "), (lp(" . (scalar(@vars)-$xcols) . "),Wp(". join(",",@$c[1..$c->dim-1]) . "));\n");
    print(OUT "ideal I = ". join(",",@ideal)  .";\n");
    print(OUT "ideal G = groebner(I);\n");
    print(OUT "print(\"groebner:\");\n");
    print(OUT "print(G);\n");
    print(OUT "print(\"reduced RHS:\");\n");
    print(OUT "reduce(" . $bPoly . ",G);\n");
    print(OUT "quit;\n");
    close(OUT);
    return [$idealRef,$bPoly,$varsRef];
}

# Main function.
# Prepares the given H-description of a polyhedron //H_poly// for the 
# Conti-Traverso procedure and calls the subroutines
# 'eqSys2ContiTrav' and 'generateContiTravSingularInput'.
# @param Polytope H_poly a polyhedron given in terms of inequalities and equations
# @param Rational c the (nonnegative) cost vector
# @param String filename the filename for the Singular input file
# @return [$idealRef,$bPoly,$varsRef]
sub contiTraverso {
    my ($H_poly,$c,$filename)=@_;
    my $facets_pre=$H_poly->FACETS | INEQUALITIES;
    my $facets=removeTrivIneq($facets_pre);
    my $aff_hull=$H_poly->AFFINE_HULL | EQUATIONS;
    my $N_cols=max($facets->cols,$aff_hull->cols); #If FACETS or AFFINE_HULL are empty their matrix has zero cols!
    my $eqs_in=convert_to<Int>(primitive($aff_hull));
    my $c_in=convert_to<Int>(primitive($c));
    if($facets->rows!=0){ #translate ineqs into eqs and adapt size of aff_hull 
	my $slackedIneqs=ineqs2IntEqsWithSlacks($facets);
	my $N_slacks=$slackedIneqs->cols-$N_cols;
    	$eqs_in|=new Matrix<Int>($aff_hull->rows,$N_slacks);
	$eqs_in/=$slackedIneqs;
	$c_in|=new Vector<Int>(zero_vector<Int>($N_slacks));
    }
    my ($idealRef,$bPoly,$r,$varsRef) = @{eqSys2ContiTrav($eqs_in)};
    my $xcols=$eqs_in->cols-1;
    return generateContiTravSingularInput($varsRef,$xcols,$c_in,$idealRef,$bPoly,$filename);
}

################################################################################################
# End
################################################################################################


1; #to avoid return error


################################################################################################
# End of Code
################################################################################################



# Local Variables:
# c-basic-offset:3
# mode: perl
# End:
