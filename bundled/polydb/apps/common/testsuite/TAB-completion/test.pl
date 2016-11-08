check_completion('0', "db_query({}, db=>", qw("));
check_completion('1', "db_query({}, db=>\"", qw(LatticePolytopes Matroids Tropical));
check_completion('2', "db_query({}, db=>\"L", qw(LatticePolytopes));
check_completion('3', "db_query({}, db=>\"LatticePolytopes\", collection=>\"", qw(SmoothReflexive));
check_completion('4', "db_query({}, db=>\"LatticePolytopes\", limit=>1, collection=>\"", qw(SmoothReflexive));
check_completion('5', "db_query({}, db=>\"LatticePolytopes\", user=>\"test\", collection=>", qw("));

