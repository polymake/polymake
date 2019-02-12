use application "polytope";

sub lpclosure
{
    my $p = shift;
    my $d = $p->AMBIENT_DIM;
    my $q = new Polytope<Rational>($p);
    for (my $k = 0; $k < $d; $k = $k+1)
    {
        if ( $q->DIM == -1 )         # can stop as soon as $q is empty
        {
             return $q;
        }
    
        # create reversed opposite inequalities of 0/1-cube and corresponding polyhedra
        my $v1 = new Vector<Rational>(0 | -unit_vector($d, $k));
        my $v2 = new Vector<Rational>(-1 | unit_vector($d, $k));
    
        # create intersection of corresponding polyhedra with iterated polyhedron $q
        my $b1 = new Polytope<Rational>(INEQUALITIES => $v1 / $q->FACETS);
        my $b2 = new Polytope<Rational>(INEQUALITIES => $v2 / $q->FACETS);
    
        if ( ($b1->DIM > -1) && ($b2->DIM > -1) )
        {
            my $c = conv($b1, $b2);
            $q = intersection($q, $c);
        }
        elsif ( ($b1->DIM > -1) && ($b2->DIM == -1) )
        {
            $q = intersection($q, $b1);
        }
        elsif ( ($b1->DIM == -1) && ($b2->DIM > -1) )
        {
            $q = intersection($q, $b2);
        }
    }
    return $q;
}
