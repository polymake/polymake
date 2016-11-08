compare_object( '1.poly', db_query_one({"_id"=>"F.5D.0000"}, db=>"LatticePolytopes", collection=>"SmoothReflexive"), ignore => ["date"] );
compare_object( '2.poly', db_query_one({"_id"=>"F.4D.0000"}, db=>"LatticePolytopes", collection=>"SmoothReflexive"), ignore => ["date"] );
compare_object( '3.poly', db_query_one({"DIM"=>4, "N_VERTICES"=>23}, db=>"LatticePolytopes", collection=>"SmoothReflexive"), ignore => ["date"] );

