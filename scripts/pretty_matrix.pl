use application 'ideal';

# pretty matrix output
sub pretty_matrix($;$){
  my ($M, $options) = @_;
  # set defaults for options

  my $spaces = " ";
  if (defined($options->{"spaces"})) {
      $spaces= " " x $options->{"spaces"};
  }

  my @len = map{0}(1..$M->cols());
    for(my $j=0; $j<$M->cols(); ++$j){
  for(my $i=0; $i<$M->rows(); ++$i){
      $len[$j] = maximum([$len[$j], length($M->($i,$j))]);
    }
  }

  for(my $i=0; $i<$M->rows(); ++$i){
    for(my $j=0; $j<$M->cols(); ++$j){
	my $collen = $len[$j];
	printf("%${collen}s%s", $M->($i,$j), $spaces);
    }
    print "\n";
  }
}


# pretty sign matrix output
sub sign_matrix($){
  my ($M) = @_;

  for(my $i=0; $i<$M->rows(); ++$i){
    for(my $j=0; $j<$M->cols(); ++$j){
	if($M->($i,$j) == 0){
	    print "  ";
	} else {
	   my $sgn = ($M->($i,$j) >0)?"+ ":"- ";
	   print $sgn;
	}
    }
    print "\n";
  }
}
