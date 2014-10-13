use Benchmark qw(:all);
use application 'ideal';
#CREDIT macaulay
#  Macaulay2 is a software system devoted to supporting research in algebraic
#  geometry and commutative algebra.
#  Copyright by Daniel R. Grayson and Michael E.Stillman.
#  http://www.math.uiuc.edu/Macaulay2/
    
my $tempname = new Tempfile;
my $macaulay2= "M2";
my $loop=1;
my $time=-1;


sub help_grassmannian {
    print "grassmannian(d,n) calles the corresponding function from Macaulay2 and\n returns a list of Polynomials which generates the pluecker ideal.";
}

sub help_pluecker_ideal {
    print "pluecker_ideal(d,n,v,char) returns the pluecker ideal.\n Where the ordering is given by the vector v, of lenght (n choose d). char is a parameter for the characterisc and char=0 is over the Integers and char=-1 over the Rationals.\n(only char=-1 coinsides with the corosponding ring)\n The computations are made with Macaulay2.";
}

#generates a M2 file
sub write_macaulay2_grassmannian($$$$$){
    # d < n, calc_gb = boolean, vector = Vector<Rational>, char -1=Q 0=Z p=Z/pZ
    my ($d,$n,$calc_gb,$vector,$char)=@_;
    my $vec_string=join ",", (map {$vector->[ChangeOrder($d,$n,$_)]} 0..$vector->dim()-1);

    open(my $P, ">$tempname")or die "can't create temporary file $tempname: $!";

    print $P "I=Grassmannian(".($d-1).",".($n-1).");\n";
    if($char==0){
	print $P "R=newRing(ring I,MonomialOrder=>{Weights => {".$vec_string."}},Global=>false);\n";
    }else{
	if($char==-1){
	    print $P "R=newRing((ring I)**QQ,MonomialOrder=>{Weights => {".$vec_string."}},Global=>false);\n";
	}else{
	    print $P "K=(ZZ/".$char.")(monoid ring I);\n";
	    print $P "R=newRing(K,MonomialOrder=>{Weights => {".$vec_string."}},Global=>false);\n";
	}
    }
    print $P "J=sub(I,R);\n";
    if($calc_gb==0){
	print $P "print toExternalString(J_*)";
    }else{
	print $P "cp1=cpuTime();\n";
	print $P "for i from 1 to $loop do GB=gens gb J;\n";
	print $P "cp2=cpuTime();\n";
	print $P "print toString(GB);\n";
	print $P "print (cp2-cp1);\n";
    }
    close $P;
}

# calles the corresponding function from Macaulay2.
# @param Int d, Int n  with d smaller than n
# @reurn perl::array of polynomials
sub grassmannian($$){
    my ($d,$n)=@_;
    my $ring=createRing($d,$n);
    my @polys=();
    write_macaulay2_grassmannian($d,$n,0,new Vector(ones_vector(binomial($n,$d))),0);
    open my $P, "$macaulay2 --script $tempname 2>&1 |"
	or die "couldn't run Macaulay2";
    local $_;
    while (<$P>) {
	s/ //g;
	s/^{//g;
	s/}$//g;
	s/\),/);/g;
	my @gens = split ";";
	foreach my $g (@gens) {
	        my @list = $g =~ m/([*+-]?)p_\(([\d,]+)\)/gc; #TODO
		my $dim=new Int(@list/4);
		my $vec=new Vector<Int>($dim);
		my $mat=new Matrix<Int>($dim,new Int(binomial($n,$d)));
		for(my $i=0;$i<@list/1;$i=$i+4){
		    if($list[$i] eq "-"){
			$vec->[$i/4]=-1;
		    }else{
			$vec->[$i/4]=1;
		    }
		    my @first=split ",", $list[$i+1];
		    my @last=split ",", $list[$i+3];
		    my $m_vec=unit_vector<Int>(binomial($n,$d),parse($d,$n,\@first));
		       $m_vec=new Vector($m_vec+unit_vector<Int>(binomial($n,$d),parse($d,$n,\@last)));
		       $mat->row($i/4)=$m_vec;
		}
		#push @polys, new Polynomial($mat, $vec, $ring);#<Integer ?
		push @polys, new Polynomial($ring,$g);
	}
    }
    close $P;
    return @polys;
}

sub pluecker_ideal($$$$){ # nur char=-1 passt mit dem Ring zusammen!!!
    # d < n, vector = Vector<Int>, char -1=Q 0=Z p=Z/pZ
    my ($d,$n,$vector,$char)=@_;
    my $ring=createRing($d,$n);
    #greate order_martix:
    my $vector=new Vector<Int>($vector); #just to be sure
    my $num=new Int(binomial($n,$d));
    my $ord_matrix=new Matrix<Int>($num,$num);
    $ord_matrix->row(0)=$vector;
    my $j=1;
    my $i=2;
    if($vector==$vector->[0]*ones_vector($vector->dim)){
	$i=1;
	$j=$num+1;
    }else{
        $ord_matrix->row(1)=ones_vector<Int>($vector->dim);
	my $w=$vector-$vector->[0]*ones_vector<Int>($vector->dim);
	while($w->[$j]==0){
	    ++$j;
	}
    }
    $j=$num-$j+1;
    for(;$i<$num;++$i){
	if($j>$i){
	    $ord_matrix->row($i)=-1*unit_vector<Int>($num,ChangeOrder($d,$n,$num+1-$i));	
	}
	if($i>=$j){
	    $ord_matrix->row($i)=-1*unit_vector<Int>($num,ChangeOrder($d,$n,$num-$i));
	}
    }
    my @polys=();
    write_macaulay2_grassmannian($d,$n,1,new Vector($vector),$char);
    open my $P, "$macaulay2 --script $tempname 2>&1 |"
	or die $!." couldn't run Macaulay2";
    local $_;
    $_=<$P>;
 	s/ //g;
	s/^matrix{{//g;
	s/}}$//g;
	s/(\)[^()]*),/$1;/g;
	my @gens = split ";";
	foreach my $g (@gens) {
	    push @polys, new Polynomial($ring,$g);
	}
    $time= <$P>;
    close $P;
    my $GB=new Groebner(BASIS=>\@polys,ORDER_MATRIX=>$ord_matrix);
    my $J = new Ideal(GENERATORS=>\@polys,GROEBNER=>$GB);
    return $J;
}


sub createRing($$){
    my ($d,$n)=@_;
    my $p=new Array<Int>($d);
    my @v=();
    my $s="0";
    $p->[0]=0;
    for(my $i=1;$i<$d;++$i){
	$p->[$i]=$i;
	$s=$s.",".($p->[$i]);
    }
    push @v, "p_(".$s.")";
    while($p->[0]!=$n-$d){
	if($p->[$d-1]==$n-1){
	    my $j=2;
	    while($p->[$d-$j]==$n-$j){
		++$j;
	    }
	    $p->[$d-$j]=$p->[$d-$j]+1;
	    while($j>1){
		--$j;
		$p->[$d-$j]=$p->[$d-$j-1]+1;
	    }
	}else{
	    $p->[$d-1]=$p->[$d-1]+1;
	}
	$s=$p->[0];
	for(my $i=1;$i<$d;++$i){
	    $s=$s.",".($p->[$i]);
	}
	push @v, "p_(".$s.")";
    }
    return new Ring(\@v);  #only field Q yet
}

#not in use:
sub createRingM2($$){
    my ($d,$n)=@_;
    my $p=new Array<Int>($d);
    my @v=();
    my $s="0";
    $p->[0]=0;
    for(my $i=1;$i<$d;++$i){
	$p->[$i]=$i;
	$s=$s.",".($p->[$i]);
    }
    push @v, "p_(".$s.")";
    for(my $j=1;$j<binomial($n,$d);++$j){
	my $k=0;
	while($k<$d-1){
	    if($p->[$k]+1!=$p->[$k+1]){
		if($k!=0){
		    for(my $i=0;$i<$k;++$i){
			$p->[$i]=$i;
		    }
		}
		$p->[$k]=$p->[$k]+1;
		$k=$d;
	    }
	    ++$k;
	}
	if($k==$d-1){
	    for(my $i=0;$i<$d-1;++$i){
		$p->[$i]=$i;
	    }
	    $p->[$d-1]=$p->[$d-1]+1;
	}
	$s=$p->[0];
	for(my $i=1;$i<$d;++$i){
	    $s=$s.",".($p->[$i]);
	}
	push @v, "p_(".$s.")";
    }
    return new Ring(\@v);#Integer ?
}

#gives the Index for the var in the ring
sub parse($$$){
	my ($d,$n,$A)=@_;
	my @B=@{$A};
	my @X=@{$A};
	my $temp=0;
	my $i=0;
	my $sum=0;
	foreach(@B){
		$X[$i]=$B[$i]-$temp;
		$temp=$B[$i]+1;
		++$i;
	}
	for(my $j=0;$j<$d;++$j){
		if($X[$j]!=0){
			my $N=$n;
			if($j!=0){
				$N=$n-$B[$j-1]-1;
			}
			for(my $k=1;$k<=$X[$j];++$k){
				$sum=$sum+binomial($N-$k,$d-$j-1);			
			}
		}
	}
	return $sum;
}

sub parseM2($$){
	my ($d,$A)=@_;
	my @B=@{$A};
	my $sum=0;
	for(my $j=0;$j<$d;++$j){
	    if($B[$j]!=0){
		$sum+=binomial($B[$j],$B[$j]-$j-1);
	    }
	}
	return $sum;
}

sub ChangeOrder($$$){
    my ($d,$n,$k) = @_;

    my @array = ();

    for(my $i=$d-1; $i >= 0; --$i) {
	my $j = $i;
	my $sum = 1;

	while($sum <= $k) {
	    ++$j;
	    $sum += binomial($j, $j - $i);
	}
	$k -= ($sum -binomial($j, $j-$i));
	unshift @array, $j; 
    }
    return new Int(parse($d,$n,\@array));
#    return @array;
}


sub time_it_grassmannian($$$$){
    my ($d,$n,$l,$bool)=@_;
    $loop=$l;
    my 	@gens=grassmannian($d,$n);
    my @vec=();
    push @vec, ones_vector(binomial($n,$d));
    if($bool){
	if($d==3 and $n==8){
	    #a few rays from http://svenherrmann.net/DR38/dr38.html
	    push @vec, new Vector("-3 -3 -3 -3 -24 -24 -3 -3 -3 11 11 -3 -3 11 11 -3 11 11 11 11 -10 -3 -3 -3 11 11 -3 -3 11 11 -3 11 11 11 11 -10 -3 -3 11 11 -3 -24 -24 11 11 -10 -3 11 11 -24 -24 -10 11 11 -10 -10");
	    push @vec, new Vector("1 1 1 8 8 1 1 1 8 8 1 1 8 8 1 8 8 1 -90 8 8 -6 -6 1 1 -6 -6 1 1 -6 1 1 -6 8 1 1 -6 1 1 -6 1 1 -6 8 1 1 1 1 -6 8 1 1 8 1 1 8");
	    push @vec, new Vector("-3 -3 -3 -3 -24 -24 -3 -3 -3 11 11 -3 -3 11 11 -3 11 11 11 11 -10 -3 -3 -3 11 11 -3 -3 11 11 -3 11 11 11 11 -10 -3 -3 11 11 -3 -24 -24 11 11 -10 -3 11 11 -24 -24 -10 11 11 -10 -10");
	    push @vec, new Vector("-32 -32 31 31 31 31 -32 31 31 31 31 -4 -4 -4 -4 -116 24 24 24 24 -116 -144 24 24 24 24 24 24 24 24 17 17 -88 -88 17 17 24 24 24 24 17 -88 17 17 -88 17 -18 17 17 17 17 -18 10 10 10 10");
	    push @vec, new Vector("-3 -3 -3 -3 -24 -24 -3 -3 -3 11 11 -3 -3 11 11 -3 11 11 11 11 -10 -3 -3 -3 11 11 -3 -3 11 11 -3 11 11 11 11 -10 -3 -3 11 11 -3 -24 -24 11 11 -10 -3 11 11 -24 -24 -10 11 11 -10 -10");
	}
	if($d==3 and $n==7){
	    # some rays from http://www.uni-math.gwdg.de/jensen/Research/G3_7/grassmann3_7.html
	    push @vec, new Vector([0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,1]);
	    push @vec, new Vector([2,1,1,0,0,1,1,0,0,3,1,1,1,1,2,2,3,1,1,1,3,2,0,0,1,1,2,3,0,0,1,1,1,2,3]);
	    push @vec, new Vector([3,1,1,0,0,1,1,0,0,3,1,1,1,1,2,2,2,1,1,1,3,2,0,0,1,1,2,3,0,0,1,1,1,2,3]);
	    push @vec, new Vector([1,1,1,1,2,0,2,0,1,0,3,2,0,1,2,0,3,0,1,0,4,2,0,1,2,1,2,4,1,2,3,2,3,4,4]);
	    push @vec, new Vector([1,1,1,1,2,0,3,0,1,0,3,2,0,1,2,0,2,0,1,0,4,2,0,1,2,1,2,4,1,2,3,2,3,4,4]);
	    push @vec, new Vector([2,1,1,0,0,1,1,0,0,3,1,1,1,1,3,3,2,2,2,1,4,3,0,0,2,1,3,4,0,0,2,1,1,3,4]);
	    push @vec, new Vector([0,0,0,0,0,0,1,0,0,0,1,2,0,0,1,0,0,0,0,1,3,2,1,1,2,0,1,3,0,0,1,2,2,3,3]);
	    push @vec, new Vector([0,0,1,1,2,1,2,0,1,3,0,1,1,2,3,2,0,3,2,0,4,2,1,2,3,1,2,4,0,1,2,0,1,2,4]);
	    push @vec, new Vector([1,1,0,0,2,1,3,2,2,0,0,3,1,1,1,1,0,0,2,3,4,3,2,2,2,0,0,4,1,1,1,2,2,2,4]);
	    push @vec, new Vector([0,0,0,0,0,1,2,0,1,1,0,2,0,1,0,0,0,0,0,1,2,1,1,2,1,1,0,3,0,1,0,1,2,1,3]);
#	    push @vec, new Vector([]);
	}
	if($d==3 and $n==6){
	    # rays from http://www.uni-math.gwdg.de/jensen/Research/G3_7/grassmann3_6.html
	    push @vec, new Vector([0,0,1,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0]);
	    push @vec, new Vector([0,0,3,1,2,1,0,1,0,2,1,0,2,0,3,1,3,1,0,0]);
	    push @vec, new Vector([0,0,3,1,2,1,0,1,0,2,2,0,3,0,4,1,2,2,0,0]);
	    push @vec, new Vector([1,0,2,0,0,1,0,0,0,0,1,1,1,0,2,0,0,1,0,0]);
	    push @vec, new Vector([1,0,1,0,0,2,0,0,0,0,1,1,1,0,2,0,0,1,0,0]);
	    push @vec, new Vector([3,0,2,0,0,2,0,2,1,2,2,3,2,0,4,0,0,3,0,1]);
	    push @vec, new Vector([4,0,4,0,0,4,0,3,3,3,4,4,4,0,4,0,0,4,0,3]);
	}
    }
    my $num=new Int(binomial($n,$d));
    my $ord_matrix=new Matrix<Rational>($num,$num);
    
    my $td1=timeit(10,"grassmannian($d,$n)");
    print "  grassmannian(".$d.",".$n."): ".timestr($td1)."\n";

    foreach(@vec){
	my $v=$_;
	#greate order_martix:
	$ord_matrix->row(0)=$v;
	my $vector=new Vector<Rational>($v);
	my $num=new Int(binomial($n,$d));
	my $ord_matrix=new Matrix<Rational>($num,$num);
	$ord_matrix->row(0)=$vector;
	my $j=1;
	my $i=2;
	if($vector==$vector->[0]*ones_vector($vector->dim)){
	    $i=1;
	    $j=$num+1;
	}else{
	    $ord_matrix->row(1)=ones_vector($vector->dim);
	    my $w=$vector-$vector->[0]*ones_vector($vector->dim);
	    while($w->[$j]==0){
		++$j;
	    }
	}
	$j=$num-$j+1;
	for(;$i<$num;++$i){
	    if($j>$i){
		$ord_matrix->row($i)=-unit_vector($num,$num+1-$i);	
	    }
	    if($i>=$j){
		$ord_matrix->row($i)=-unit_vector($num,$num-$i);
	    }
	}

	my $t0 = Benchmark->new;
	my $pl=pluecker_ideal($d,$n,$v,-1);
	my $t1 = Benchmark->new;
	my $td = timediff($t1, $t0);
	print "pluecker_ideal(".$d.",".$n.",[".$v."],-1):".timestr($td)." M2time=".$time;

	if($d==3 and $n==8){
	    save($pl,"pluecker_ideal");
	}

	my $size=$pl->GROEBNER(ORDER_MATRIX=>$ord_matrix)->BASIS->size;
	my $t0 = Benchmark->new;
	pluecker_ideal($d,$n,$v,2);
	my $t1 = Benchmark->new;
	my $td = timediff($t1, $t0);
	print "pluecker_ideal(".$d.",".$n.",[".$v."],2): ".timestr($td)." M2time=".$time;

	my $t0 = Benchmark->new;
	pluecker_ideal($d,$n,$v,0);
	my $t1 = Benchmark->new;
	my $td = timediff($t1, $t0);
	print "pluecker_ideal(".$d.",".$n.",[".$v."],0): ".timestr($td)." M2time=".$time;

	if($d<3 or $n<7){
	    my $t0 = Benchmark->new;
	    for(my $k=0;$k<$loop;++$k){
		my $K=new Ideal(GENERATORS=>\@gens);
		$K->GROEBNER(ORDER_MATRIX=>$ord_matrix)->BASIS;
	    }
	    my $t1 = Benchmark->new;
	    my $td = timediff($t1, $t0);
	    print "pluecker ideal(".$d.",".$n.",[".$v."],0): ".timestr($td)." berechnet mit polymake via singular\n";

	    my $J=new Ideal(GENERATORS=>\@gens);
	    $size=$J->GROEBNER(ORDER_MATRIX=>$ord_matrix)->BASIS->size;
	    print "singular size=".$size."\n";
	}
	print "M2       size=".$size."\n";
    }
    print "loop=$loop\n";
    $loop=1;
    $time=-1;
}


sub time_3_8{
    my $d=3;
    my $n=8;
    my $loop=2;
    print "loop=$loop\n";
    my 	@gens=grassmannian($d,$n);
    my @vec=();
    push @vec, new Vector("-3 -3 -3 -3 -24 -24 -3 -3 -3 11 11 -3 -3 11 11 -3 11 11 11 11 -10 -3 -3 -3 11 11 -3 -3 11 11 -3 11 11 11 11 -10 -3 -3 11 11 -3 -24 -24 11 11 -10 -3 11 11 -24 -24 -10 11 11 -10 -10");
    push @vec, ones_vector(binomial($n,$d));
    push @vec, new Vector("1 1 1 8 8 1 1 1 8 8 1 1 8 8 1 8 8 1 -90 8 8 -6 -6 1 1 -6 -6 1 1 -6 1 1 -6 8 1 1 -6 1 1 -6 1 1 -6 8 1 1 1 1 -6 8 1 1 8 1 1 8");
    push @vec, new Vector("-3 -3 -3 -3 -24 -24 -3 -3 -3 11 11 -3 -3 11 11 -3 11 11 11 11 -10 -3 -3 -3 11 11 -3 -3 11 11 -3 11 11 11 11 -10 -3 -3 11 11 -3 -24 -24 11 11 -10 -3 11 11 -24 -24 -10 11 11 -10 -10");
    push @vec, new Vector("-32 -32 31 31 31 31 -32 31 31 31 31 -4 -4 -4 -4 -116 24 24 24 24 -116 -144 24 24 24 24 24 24 24 24 17 17 -88 -88 17 17 24 24 24 24 17 -88 17 17 -88 17 -18 17 17 17 17 -18 10 10 10 10");
    my $num=new Int(binomial($n,$d));
    my $ord_matrix=new Matrix<Rational>($num,$num);
    
    foreach(@vec){
	my $v=$_;
	#greate order_martix:
	$ord_matrix->row(0)=$v;
	my $vector=new Vector<Rational>($v);
	my $num=new Int(binomial($n,$d));
	my $ord_matrix=new Matrix<Rational>($num,$num);
	$ord_matrix->row(0)=$vector;
	my $j=1;
	my $i=2;
	if($vector==$vector->[0]*ones_vector($vector->dim)){
	    $i=1;
	    $j=$num+1;
	}else{
	    $ord_matrix->row(1)=ones_vector($vector->dim);
	    my $w=$vector-$vector->[0]*ones_vector($vector->dim);
	    while($w->[$j]==0){
		++$j;
	    }
	}
	$j=$num-$j+1;
	for(;$i<$num;++$i){
	    if($j>$i){
		$ord_matrix->row($i)=-unit_vector($num,ChangeOrder($d,$n,$num+1-$i));	
	    }
	    if($i>=$j){
		$ord_matrix->row($i)=-unit_vector($num,ChangeOrder($d,$n,$num-$i));
	    }
	}

	my $t0 = Benchmark->new;
	my $pl=pluecker_ideal($d,$n,$v,2);
	my $t1 = Benchmark->new;
	my $td = timediff($t1, $t0);
	my $size=$pl->GROEBNER(ORDER_MATRIX=>$ord_matrix)->BASIS->size;
	print "pluecker_ideal(".$d.",".$n.",[".$v."],2):".timestr($td)." M2time=".$time."  size=".$size."\n";
	#save($pl,"GB_grassmannian38_2");

	my $t0 = Benchmark->new;
	$pl=pluecker_ideal($d,$n,$v,3);
	my $t1 = Benchmark->new;
	my $td = timediff($t1, $t0);
	$size=$pl->GROEBNER(ORDER_MATRIX=>$ord_matrix)->BASIS->size;
	print "pluecker_ideal(".$d.",".$n.",[".$v."],3):".timestr($td)." M2time=".$time."  size=".$size."\n";
	#save($pl,"GB_grassmannian38_3");

	my $t0 = Benchmark->new;
	$pl=pluecker_ideal($d,$n,$v,5);
	my $t1 = Benchmark->new;
	my $td = timediff($t1, $t0);
	$size=$pl->GROEBNER(ORDER_MATRIX=>$ord_matrix)->BASIS->size;
	print "pluecker_ideal(".$d.",".$n.",[".$v."],5):".timestr($td)." M2time=".$time."  size=".$size."\n";
	#save($pl,"GB_grassmannian38_5");

	my $t0 = Benchmark->new;
	$pl=pluecker_ideal($d,$n,$v,7);
	my $t1 = Benchmark->new;
	my $td = timediff($t1, $t0);
	$size=$pl->GROEBNER(ORDER_MATRIX=>$ord_matrix)->BASIS->size;
	print "pluecker_ideal(".$d.",".$n.",[".$v."],7):".timestr($td)." M2time=".$time."  size=".$size."\n";
	#save($pl,"GB_grassmannian38_7");
    }
    $loop=1;
    $time=-1;
}
