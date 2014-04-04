compare_output {
    my $s1   = new boost_dynamic_bitset([1,2,3]);
    my $s1_cp = new boost_dynamic_bitset([1,2,3]);
    print $s1 == $s1_cp, "\n";

    $s1 += 256;
    $s1 -= 3;
    $s1 -= 256;
    $s1_cp -= 3;
    print $s1,"\n", $s1_cp, "\n";
    print $s1 == $s1_cp, "\n";
    
    $s1   = new boost_dynamic_bitset([1,2,3]);
    $s1_cp = new boost_dynamic_bitset([1,2,3]);
    
    my $s2   = new boost_dynamic_bitset([1,2,3,4]);
    my $a1 = new Array<boost_dynamic_bitset>([$s1, $s2]);
    print $a1, "\n";
    
    my $a1_cp = new Array<boost_dynamic_bitset>([$s1, $s2]);
    print $a1 == $a1_cp, "\n";

    my $s3 = new boost_dynamic_bitset([5,6,7]);
    my $a2 = new Array<boost_dynamic_bitset>([$s1, $s3]);
    my $aa    = new Array<Array<boost_dynamic_bitset>>($a1,$a2);
    print $aa, "\n";

    my $aa_cp = new Array<Array<boost_dynamic_bitset>>($a1,$a2);
    print $aa == $aa_cp, "\n";
} '1'
