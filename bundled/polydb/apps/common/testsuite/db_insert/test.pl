# this test only works with polydb write access
$common::db_user ne "polymake" or disable_test("Needs database write access and no Ticket #000.") and return;

my $now = DateTime->now;
my $date = DateTime->now->date;
use Sys::Hostname;
my $name = Sys::Hostname::hostname;
my $col_name = "test-".$name."-".$now;
	
my $p1 = load("1.poly");
db_insert($p1, db=>"Test", collection=>$col_name, id => "test1", contributor => "test");
my $o1 = db_query_one({"_id" => "test1"}, db => "Test", collection => $col_name);
$o1->remove_attachment("polyDB");
		
my $count1i = db_count({}, db => "Test", collection => $col_name);
	
db_insert_from_file("2.poly", db=>"Test", collection=>$col_name, id => "test2", contributor => $name);
my $o2 = db_query_one({"_id" => "test2"}, db => "Test", collection => $col_name);
$o2->remove_attachment("polyDB");

db_insert_from_file("3.poly", db=>"Test", collection=>$col_name, id => "cube", contributor => "test");
my $o3 = db_query_one({"_id" => "cube"}, db => "Test", collection => $col_name);
$o3->remove_attachment("polyDB");

my $count3i = db_count({}, db => "Test", collection => $col_name);
print $count3i;

db_remove("test1", db=>"Test", collection=>$col_name);
db_remove("test2", db=>"Test", collection=>$col_name);
my $count1r = db_count({}, db => "Test", collection => $col_name);

$p1->dont_save;

compare_values( '1-insert', 1, $count1i );
compare_object( '1out.poly', $o1);
compare_object( '2.poly', $o2 );
compare_object( '3out.poly', $o3);
compare_values( '3-insert', 3, $count3i );
compare_values( 'remove', 1, $count1r );
db_clean_up_test();

