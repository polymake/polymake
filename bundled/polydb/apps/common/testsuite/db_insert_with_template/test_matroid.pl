# this test only works with polydb write access
$common::db_user ne "polymake" or disable_test("Needs database write access and no Ticket #000.") and return;

my $now = DateTime->now;
my $date = DateTime->now->date;
use Sys::Hostname;
my $name = Sys::Hostname::hostname;
my $col_name = "test-".$name."-".$now."-matroid";

my $t = db_get_type_info("Matroids", "Small", template_key=>"default");
# avoid dependency on bundled group
delete $t->{template}->{N_AUTOMORPHISMS};

db_set_type_info("Test", $col_name, template=>$t->{template}, template_key=>$col_name, app=>"matroid", basic_type=>"matroid::Matroid", id=>$col_name);
my $s = db_get_type_info("Test", $col_name, template_key=>$col_name);

my $fano = load("fano.mat");
$fano->dont_save;
db_insert($fano, db=>"Test", collection=>$col_name, use_type_info=>1, template_key=>$col_name);

my $count = db_count({}, db=>"Test", collection=>$col_name);

my $retarray = db_query({}, db=>"Test", collection=>$col_name, sort_by=>{"_id"=>1});

foreach (@{db_ids({}, db=>"Test", collection=>$col_name)}) {
	db_remove($_, db=>"Test", collection=>$col_name);
}

compare_values( 'count', 1, $count );
compare_data('1mat',$retarray);
compare_values( 'remove', 0, db_count({}, db => "Test", collection => $col_name) );
db_clean_up_test();
