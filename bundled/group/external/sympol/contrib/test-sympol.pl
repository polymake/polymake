#!/usr/bin/perl

# Rudimentary test script for SymPol.
# Runs a number of tests, measuring time.
#
# Parameters:
#  build name (optional, defaults to 'release'):  this script uses the sympol files in ../build/$BUILDNAME
#
# Return code:
#  0 if all tests were successful, 1 otherwise
#

use strict;
use warnings;
use File::Find;

my $sympol_root = '../build/release';

if ($#ARGV >= 0) {
    $sympol_root = $ARGV[0];
}
print "using root $sympol_root\n";

sub test_polyhedron {
	my ($parameters, $filename, $expected_rays) = @_;
	print "$filename \@ $parameters";
	my $cmd = "${sympol_root}/sympol/sympol -t $parameters -i ${sympol_root}/data/$filename";
    my $sympol = `$cmd`;
	my $time = '';
	if ($sympol =~ /elapsed time: (\d+\.?\d*) seconds/) {
		$time = $1;
	}
	if ($sympol =~ /(\d+)\s+(\d+)\s+rational/) {
		my $number_of_rays = $1;
		my $dimension = $2;
		if ($number_of_rays != $expected_rays) {
			warn "  failed: $number_of_rays != $expected_rays";
			return 0;
		} else {
			print "  PASS  $time";
		}
	} else {
		warn "  output format mismatch";
		print $sympol;
		return 0;
	}
	print "\n";
	return 1;
}

my $success = 1;

$success &= test_polyhedron('-d', 'hirsch_counterexample/santos_prismatoid.ext', 6);
$success &= test_polyhedron('-d --cdd', 'hirsch_counterexample/santos_prismatoid.ext', 6);
$success &= test_polyhedron('-d', 'hirsch_counterexample/santos_prismatoid-reduced_symmetry.ext', 12);
$success &= test_polyhedron('-d --cdd', 'hirsch_counterexample/santos_prismatoid-reduced_symmetry.ext', 12);

$success &= test_polyhedron('-d --cdd', 'metric/metric_5.ine', 2);
$success &= test_polyhedron('-d', 'metric/metric_5.ine', 2);

$success &= test_polyhedron('-d', 'cyclic/cyclic4-5.ext', 1);
$success &= test_polyhedron('-d --cdd', 'cyclic/cyclic4-5.ext', 1);
# homogenized polar .ine contains cone apex
$success &= test_polyhedron('-d', 'cyclic/cyclic4-5.ine', 2);

$success &= test_polyhedron('-d --cdd', 'metric/metric_6.ine', 3);
$success &= test_polyhedron('-a --cdd', 'metric/metric_6.ine', 3);
$success &= test_polyhedron('-a', 'metric/metric_6.ine', 3);
$success &= test_polyhedron('--idm-adm-level 1 2', 'metric/metric_6.ine', 3);
$success &= test_polyhedron('--cdd --idm-adm-level 1 1', 'metric/metric_6.ine', 3);

$success &= test_polyhedron('-d --cdd', 'voronoi_cones/d4.ine', 2);
$success &= test_polyhedron('-d', 'voronoi_cones/d4.ine', 2);

$success &= test_polyhedron('-d --cdd', 'voronoi_cones/d5.ine', 3);
$success &= test_polyhedron('-d', 'voronoi_cones/d5.ine', 3);

$success &= test_polyhedron('-d', 'voronoi_cones/e6.ine', 12);
$success &= test_polyhedron('-a', 'voronoi_cones/e6.ine', 12);
$success &= test_polyhedron('--idm-adm-level 0 2', 'voronoi_cones/e6.ine', 12);
$success &= test_polyhedron('--idm-adm-level 0 1', 'voronoi_cones/e6.ine', 12);
$success &= test_polyhedron('--cdd --idm-adm-level 0 1', 'voronoi_cones/e6.ine', 12);
$success &= test_polyhedron('--idm-adm-level 1 2', 'voronoi_cones/e6.ine', 12);

$success &= test_polyhedron('-d', 'permutation_polytopes/cross-polytope4.ext', 61);
$success &= test_polyhedron('-a', 'permutation_polytopes/cross-polytope4.ext', 61);
$success &= test_polyhedron('-d', 'permutation_polytopes/cyclic-2-3-5.ext', 17);
$success &= test_polyhedron('-a', 'permutation_polytopes/cyclic-2-3-5.ext', 17);

exit 1 unless $success;
