# dgdfg
sub dual_properties_mono_cone($){
    my ($m) = @_;
    my $dd = tropical::monomial_dual_description($m);
    my $g = $dd->first;
    print $g;
    my $h = polytope::hasse_diagram($dd->second,$m->cols());
    print $h->DECORATION;
}
