# sub generate_sign_array($) {
#     my ( $d,$n ) = @_;
#     my @sl;
#     for (my $i = 0; $i < $n; $i++) {
# 	my $r = int(rand($d));
# 	$sl[$i] = $r;
#     }
#     return new Array<Int>(@sl);
# }

sub generate_apices($) {
    my ( $d,$n ) = @_;
    my $M =  10000000000;   # 30;
    my $inftycut = $M/10;  # 0; # $M/4;
    my $r;
    my @sl;
    my $m = (new Matrix($n,$d));
    for (my $j = 0; $j < $n; $j++) {
	# last coord never trop. negative -- otherwise use int(rand($d)); 
	$r = int(rand($d-1)); 
	$sl[$j] = $r;
    }
    for (my $j = 0; $j < $n; $j++) {
	for (my $i = 0; $i < $d; $i++) {
	    $r = int(rand($M));
	    if (($r > ($M-$inftycut))&($sl[$j]!=$i)) {$r = 'inf';}
	    if ($sl[$j] == $i) {$r *= -1;} # corresponds to positive scheduling times
	    $m->elem($j,$i) = $r;
	}
    }
    return (new Matrix<TropicalNumber<Min>>($m), new Array<Int>(@sl));
}

sub generate_cyclic_system($) {
    my ($d, $n, $M) = @_;
    my @signlist;
    my @baselist;
    my $signentry;
    my $base;
    my $r;
    my $m = (new Matrix($n,$d));
    for (my $j = 0; $j < $n; $j++) {
	$signentry = int(rand($d-1));
	$base = int(rand($M));
	$signlist[$j] = $signentry;
	$baselist[$j] = $base;
	for (my $i = 0; $i < $d; $i++) {
	    $r = $base ** $i;
	    if ($signlist[$j] == $i) {$r *= -1;} # corresponds to positive scheduling times
	    $m->elem($j,$i) = $r;
	}
    }
    return (new Matrix<TropicalNumber<Min>>($m), new Array<Int>(@signlist));
}

# script("random_tropical_linear_program.pl");

# $s = &generate_sign_array(4,16); $m = &generate_apices(4,16); $t = trop_witness($m,$s); print $t;

# print check_witness($t,$m,$s,0);
