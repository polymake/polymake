sub deformed_cyclic{
    my($n,$d) = @_;
    my $m = monomials<Rational,Rational>(1);

    my $vm = new Matrix<PuiseuxFraction<Min>>();
    for (my $k=1; $k<=$n; $k++) {
    	my @l = (1);
    	for (my $i=1; $i<=$d; $i++) {
    	    push @l, $m^($k*$i); #($m+$k)^($i);  #
    	    push @l, $m^(-$k*$i); #1/(($m+$k)^($i));  #
    	}
    	my $v = new Vector<PuiseuxFraction<Min>>(@l);
	$vm /= $v;
    }
    my $um = unit_matrix<PuiseuxFraction<Min>>(2*$d+1);
    $vm /= ($um->minor(~[0],All));
    return $vm;
}
