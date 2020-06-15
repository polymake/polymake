use application "common";

sub x3dprint_curve_opts {
   # only for x3d ! Other backends will produce different results
	# @params Number desired absolute vertex diameter in the x3d file
	# @params Number desired absolute edge diameter in the x3d file
   my ($vertex_diam,$edges_diam) = @_;
   my $opts = {VertexLabels=>'hidden',EdgeLabels=>'hidden'};
   $opts->{EdgeThickness} = convert_to<Float>(($edges_diam/2)/($X3d::edge_radius*$X3d::scale));
   $opts->{VertexThickness} = convert_to<Float>(($vertex_diam/2)/($X3d::point_radius*$X3d::scale));
   return $opts;
}

sub random_color {
	my $r = randomInteger(255,3);
	return new RGB(@$r);
}


# for solidifying 2d surfaces
sub substitute_points {
	my ($pts, $sub) = @_;
	my @newpts;
	foreach my $point (@{$pts}) {
		if ($point->[0]!=0) {
			push @newpts, map {$_ + $point} @$sub;
		} else {
			push @newpts, $point;
		}
	}
	return new Matrix(@newpts);
}

sub solidify_2d_polytope {
	my ($p,$thickness)=@_;
	my $pah = convert_to<Rational>(normalized(convert_to<Float>($p->AFFINE_HULL->minor(All,~[0]))))*$thickness/2;
	my $sub = new Matrix(zero_vector()|($pah/-$pah)); 
	my $newpts = substitute_points($p->VERTICES, $sub);
	my $ls = $p->lookup("LINEALITY_SPACE");
	return new polytope::Polytope(POINTS=>$newpts, INPUT_LINEALITY=>$ls);
}

sub solidify_2d_pcom {
	my ($pcom, $thickness) = @_;
	my $spolys = [];
	foreach my $i (0..$pcom->N_MAXIMAL_POLYTOPES-1) {
		my $p = $pcom->polytope($i);
		push @$spolys, solidify_2d_polytope($p,$thickness);
	}
	return $spolys;
}

sub pcom_common_ref {
	my ($pcom1, $pcom2) = @_;
   return new fan::PolyhedralComplex(fan::common_refinement($pcom1, $pcom2));
}

__END__
