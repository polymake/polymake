my $cursor = database_cursor({"DIM"=>4}, db => "LatticePolytopes", collection => "SmoothReflexive", sort_by => {"_id"=>1}, skip => 20);
my $p1 = $cursor->next;
my $p2 = $cursor->next;

my $cursor2 = database_cursor({"DIM"=>5, "N_VERTICES"=>20}, db => "LatticePolytopes", collection => "SmoothReflexive", sort_by => {"_id"=>-1});
my $p3 = $cursor2->next;
my $p4 = $cursor2->next;


compare_object( '1.poly', $p1, ignore => ['date']);
compare_object( '2.poly', $p2, ignore => ['date']);
compare_values( 'COUNT1', 104, $cursor->count );
compare_object( '3.poly', $p3, ignore => ['date']);
compare_object( '4.poly', $p4, ignore => ['date']);
compare_values( 'COUNT2', 26, $cursor2->count );
