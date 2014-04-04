####################################################################################################
#--------------------------------------------------------------------------------------------------
# This perl script provides functions for computing the 
# enumeration graph described in 
#   Sturmfels, 'Groebner Bases and Convex Polytopes'(Algorithm 5.7).
#--------------------------------------------------------------------------------------------------
####################################################################################################


#use application 'polytope';
#use application 'graph';

sub lex_sort {
    my ($vecs) = @_;
    my @vecs = map{ $vecs->[$_] } 0..$vecs->rows-1;
    my @sorted_vecs = sort{$a cmp $b} @vecs;
    return new Matrix<Rational>(\@sorted_vecs);
}

sub contained {
    my ($vec,$array_ref)=@_;
    my $is_contained=0;
    foreach (@$array_ref) {
	if ( ($vec cmp $_) == 0 ) {
	    $is_contained=1;
	}
    }
    return $is_contained;
}

sub is_greater_zero {
    my ($vec) = @_;
    my $is_greater=1;
    foreach( @$vec ) {
	if( $_ < 0 ) {
	    $is_greater=0;
	}
    }
    return $is_greater;
}

sub find_sink {
    my ($feas_sol, $groeb_elems) = @_;
    my $is_reducible=1;
    my $cur=$feas_sol;
    my $zero=new Vector<Int>(zero_vector($feas_sol->dim));
    while ($is_reducible) {
	foreach( @$groeb_elems ){
	    if( is_greater_zero($cur-$_->first) ) {
		print $cur. " -> ";
		$cur=find_sink($cur-$_->first+$_->second, $groeb_elems);
	    }
	}
	$is_reducible=0;
    }
    return $cur;
}

sub enum_graph {
    my ($feas_sol, $groeb_elems) = @_;
    my $sink = find_sink($feas_sol, $groeb_elems);
    print $sink."\n";
    my @active=($sink);
    my %passive=();
    my %nodes=();
    my $node_count=0;
    my @Coords=();

    while (@active) { #bfs queue not empty
	my $cur=shift(@active);
	push(@Coords,$cur);
	my $neighbors=new Set< Vector<Int> >();
	my $info=new Pair< Int, Set< Vector<Int> > >();
	foreach my $groeb_elem (@$groeb_elems) {
	    my $diff=$cur-$groeb_elem->second;
	    if ( is_greater_zero($diff) ) {
		my $new=$diff+$groeb_elem->first;
		$neighbors+=$new;

# FIXME: The 'contained'-function is just a bad work-around 
# because I don't know a better data structure for 'active'!
# Set does not work because I cannot ask whether a vector is contained in a set!
		if ( !exists( $passive{sprintf($new)} ) && !contained($new,\@active) ) { 
		    push(@active,$new);
		}
	    }
	}
	print $node_count.": ".$cur.": ".$neighbors."\n";
	$info->first=$node_count;
	$info->second=$neighbors;
	$passive{sprintf($cur)}=$info;
	$node_count++;
    }

    my $g=new props::Graph<Directed>($node_count);
    foreach ( values %passive ) {
	my $node=$_->first;
	my @neighbors=@{$_->second};
	foreach my $neighbor (@neighbors) {
	    $g->edge(($passive{sprintf($neighbor)})->first,$node);
	}
    }
    return [(new graph::Graph<Directed>(ADJACENCY=>$g)),\@Coords];
}




################################################################################################
# End
################################################################################################



# Local Variables:
# c-basic-offset:3
# mode: perl
# End:
