compare_data( '1', db_query({"DIM"=>3},db=>"LatticePolytopes", collection=>"SmoothReflexive", sort_by=>{"_id"=>1}) );
#compare_data( '2', db_query({"DIM"=>7},db=>"LatticePolytopes", collection=>"SmoothReflexive", limit=>10, sort_by=>{"_id"=>1}, skip=>20) );

