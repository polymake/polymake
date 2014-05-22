sub cycNotation2StringArray{
    my ($cyc_not, $offset) = @_;
    my $input_cyc_not = $cyc_not;
    my $degree=1; #if string empty, the trivial group of degree 1 is created
    my @gens=();
    if ($offset != 1) {
        my $delta = 1 - $offset;
        $cyc_not =~ s/(\d+)/$delta + $1/ge;
    }
    $cyc_not =~ s/\s+//g;
    $cyc_not =~ s/,/ /g;
    $cyc_not =~ s/\)\(/,/g;
    $cyc_not =~ s/\) \(/;/g;
    $cyc_not =~ s/\(|\)//g;
    foreach my $num (split /;| |,/, $cyc_not) {
        $degree = $num if $num > $degree;
        if ($num < 1) {
            # numbers in cyc_not are shifted by 1-$offset above,
            # so all numbers should be positive integers
            croak("$input_cyc_not is not a $offset-based permutation");
        }
    }
    @gens=split /;/, $cyc_not;
    my $gens=new Array<String>(\@gens);
    return [$gens,$degree];
}

sub group_from_cyclic_notation_helper {
   my ($cyc_not, $offset) = @_;
   my $result = cycNotation2StringArray($cyc_not, $offset);
   my $g = group_from_permlib_cyclic_notation($result->[0],$result->[1]);
   return $g;
}

1;

# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
