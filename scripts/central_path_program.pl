use Benchmark qw(:all);
use File::Path qw(mkpath);
use application 'polytope';

# prints a progressbar
# @param Int got : the amount of stuff you have already got
# @param Int total : the total amount of stuff
# @param Int width : the width of the progressbar
# @param Char char : a character which symbolize done stuff
# @param String text : additional text behind the bar.
sub progress_bar {
    my ( $got, $total, $width, $char, $text) = @_;
    local $| = 1;
    printf "|%-${width}s| $got/$total | %-80s \r",
    $char x (($width)*$got/$total). '>', $text;
}

# initialize the central path polytope from 'Long and winding central paths'
# take the defining inequalities from page 18 (arxiv version)
# @param Int r parameter for the number of inequalities and variables
sub fill_inequalities_cpp($) {
    my ($r) = @_;  
 
    my $time;

    # homo. , u_0 , v_0, ... , u_r , v_r 
    my $m = new Matrix<ValuatedRationalFunction<Max, Rational, Rational> >(3*$r+4, 2*$r+3);
    my $t=new UniMonomial<Rational,Rational>(1);
    
    $m->(0,0)=$t;    $m->(0,1)   =-1; # u_0 =< t
    $m->(1,0)=$t*$t; $m->(1,2) = -1;  # v_0 =< t^2

    for(my $i=1; $i < $r+1; ++$i){
	my $s = new UniMonomial<Rational,Rational>(1-1/(2**$i));
	$m->(3*$i-1,2*$i-1) = $t; $m->(3*$i-1,2*$i+1) = -1;                        # u_i =< t*u_(i-1)
	$m->(3*$i,2*$i) = $t; $m->(3*$i,2*$i+1) = -1;                              # u_i =< t*v_(i-1)
	$m->(3*$i+1,2*$i+2) = -1; $m->(3*$i+1,2*$i-1)= $s; $m->(3*$i+1,2*$i) = $s; # v_i =< s*( u_(i-1)+v_(i-1) )
    }
    
    $m->(3*$r+2, 2*$r+1) = 1; # u_r >= 0
    $m->(3*$r+3, 2*$r+2) = 1; # v_r >= 0

    return new Polytope<ValuatedRationalFunction<Max, Rational, Rational> >(INEQUALITIES=>$m);
}

sub test_example_paper {
    my $cpp1 = fill_inequalities_cpp(1);
    my $F1 = $cpp1->FACETS;
    my $V1 = $cpp1->VERTICES;
    
    my $ts = new UniMonomial<Rational, Rational>(2);
    my $tt = new UniMonomial<Rational, Rational>(3/2);
    my $tr = new UniMonomial<Rational, Rational>(5/2);

    my $ww = new Vector<ValuatedRationalFunction<Max, Rational, Rational>>([1,0,$ts,0,$tt]);
    my $vv = new Vector<ValuatedRationalFunction<Max, Rational, Rational>>([1,0,$ts,0,$tr]);
    print "FACETS:\n$F1\nVERTICES:\n$V1\n\nevaluation of the vertex $ww from the paper:\n",$F1*$ww,"\n\nevaluation of the vertex $vv computed by polymake:\n",$F1*$vv;
}    

		       
