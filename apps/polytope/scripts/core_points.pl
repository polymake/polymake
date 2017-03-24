#########################################################
# ------------------------------------------------------
# This script contains functions for working with
# core points and orbit polytopes.
# ---
# The orbit polytope Orb_g(v) of a point v with respect
# to a permutation group g is the convex hull of all
# points in the orbit of v under g. Note that the
# group g acts on the coordinates.
#
# A core point v with respect to a group g is a
# point whose orbit polytope does not contain any
# integer points besides its vertices 
# (that is, it is lattice-free).
#
# A universal core point is a point whose
# coordinates are in {t,t+1} for some integer t.
#
# A certifier for v w.r.t. g is a lattice point in 
# the orbit polytope of v under g.
#
# The k-th 1-layer is the affine hyperplane orthogonal
# to the all-ones vector with \sum x_i = k for all
# points x in the 1-layer.
#
# ------------------------------------------------------
# subs:
# - is_core_point($v, $g)
# - inner_certifiers($v, $g)
# - one_universal_cp_in_layer($amb_dim, $layer)
# - get_universal_cp_reps($g, $layer)
# - getOnesLayer($vec)
# - isContainedInPoly($point, $p)
# - findOrbitConvCombPoly($v, $g, $contained_vec)
# ------------------------------------------------------
#########################################################


# Answers whether the given point //v// is a core point
# with respect to the group //g//.
# The decision is made via comparing the number of vertices 
# with the number of lattice points.
# @param Vector v the given point
# @param group::Group g a group acting on the coordinates 
# @return Bool true if //v// is a core point with respect to //g//
sub is_core_point {
    my ($v, $g) = @_;	
    my $p = orbit_polytope($v, $g->COORDINATE_ACTION);
    return ($p->N_LATTICE_POINTS == $p->N_VERTICES);
}


# Computes all certifiers for the point //v// with respect to
# the group //g// that are not vertices of the orbit polytope
# of //v// under //g// (They may be contained in a facet, though.)
# @param Vector v the given point
# @param group::Group g a group acting on the coordinates 
sub inner_certifiers {
    my ($v, $g) = @_;
    my $p = orbit_polytope($v, $g->COORDINATE_ACTION);
    my $latpoints = $p->LATTICE_POINTS;
    if ( $p->N_POINTS == $latpoints->rows ) {
	return new Matrix(0,$latpoints->cols);
    } else {
	my $certifiers = new Set<Int>(0..$latpoints->rows-1);
	for ( my $i = 0; $i < $p->N_POINTS; ++$i ) { # POINTS=VERTICES
	    my $point = $p->POINTS->[$i];
	    foreach (@$certifiers) {
		if ($point == $latpoints->[$_]) {
		    $certifiers-=$_;
		    last;
		}
	    }
	}
	return new Matrix($latpoints->minor($certifiers,All));
    }
}


# Returns one universal core point in dimension 
# //amb_dim// in the given 1-//layer//.
# @param Int amb_dim the dimension
# @param Int layer the number of the 1-layer
# @return Vector a universal core point
sub one_universal_cp_in_layer {
    my ($amb_dim, $layer) = @_;
    my $univ_cp = new Vector(unit_vector($amb_dim+1,0));
    for (my $i = 0; $i<$layer; ++$i) {
	$univ_cp->[$i+1] = 1;
    }
    return $univ_cp;
}


# Computes all representatives of universal core points
# in the given 1-//layer// w.r.t. the given group //g//.
# @param group::Group g a group acting on the coordinates 
# @param Int layer the number of the 1-layer
# @return Array<Vector> an array containing all representatives 
sub get_universal_cp_reps {
    my ($g, $layer) = @_;
    my $degree = $g->COORDINATE_ACTION->DEGREE;
    my $sym_g = new group::Group(COORDINATE_ACTION=>symmetric_group($degree)->PERMUTATION_ACTION);

#    my $p = new Polytope( GENERATING_GROUP=>$sym, GEN_POINTS=>[one_universal_cp_in_layer($degree, $layer)] );
    my $p = new Polytope;
    $p->GROUP = $sym_g;
    $p->GROUP->COORDINATE_ACTION->VERTICES_GENERATORS = new Matrix([one_universal_cp_in_layer($degree, $layer)]);
    $p->VERTICES;

    my $g_copy = new group::Group(COORDINATE_ACTION=>$g->COORDINATE_ACTION); # otherwise, orbit decomposition is stored in $g!
    $g_copy->name = "g_copy";

    $p->add("GROUP", $g_copy); #pos 0
    return $p->GROUP("g_copy")->EXPLICIT_ORBIT_REPRESENTATIVES;
}

# Determines the number of the 1-layer in which 
# the given vector is contained.
# @param Vector vec the vector whose 1-layer is to be determined
# @return Rational the number of the 1-layer
sub getOnesLayer {
    my $vec=$_[0];
    my $dehom_vec=$vec->slice(1);
    my $allOnes=ones_vector<Rational>($vec->dim-1);
    return $dehom_vec*$allOnes;
}

# Checks whether a given //point// is contained in a
# given polytope //p//.
# @param Vector point the point to be checked
# @param Polytope p the given polytope
# @return Bool true, iff //point// is contained in //p//
sub isContainedInPoly {
    my ($point, $p) = @_;
    for ( my $i=0; $i<$p->N_FACETS; ++$i) {
	if($p->FACETS->row($i)*$point<0) {
	    return 0;
	}
    }

    for ( my $i=0; $i<$p->AFFINE_HULL->rows; ++$i) {
	if($p->AFFINE_HULL->row($i)*$point!=0) {
	    return 0;
	}
    }
    return 1;
}


# Computes all convex combination of elements of the orbit 
# of the point //v// under the group //g// 
# that represent the given point //contained_vec//. 
# The problem is solved via describing a suitable
# polytope.
# @param Vector v the generating point of the orbit polytope
# @param group::Group g a group acting on the coordinates 
# @param Vector contained_vec a vector which is contained in 
#             the orbit polytope Orb_g(v)
# @return Polytope a polytope whose points represent convex 
#             combination of elements of the orbit of //v//
#             which represent //contained_vec//.
sub findOrbitConvCombPoly {
   my ($v, $g, $containedVec) = @_;
   my $p = orbit_polytope($v, $g);
   my $eqs = transpose( new Matrix ( $p->POINTS->minor(All, (new Set<Int>(1..$p->POINTS->cols - 1) ) ) ) );
   my $eqs_w_rhs = new Matrix(-($containedVec->slice(1,$containedVec->dim-1)) | $eqs);
   my $pos_orth = new Matrix( zero_vector($eqs->cols) | unit_matrix($eqs->cols) );
   return new Polytope(EQUATIONS=>$eqs_w_rhs, INEQUALITIES=>$pos_orth);
}


##################################
# End
##################################



# Local Variables:
# c-basic-offset:3
# mode: perl
# End:
