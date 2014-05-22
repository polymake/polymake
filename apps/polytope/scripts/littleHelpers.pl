####################################################################################################
#--------------------------------------------------------------------------------------------------
# This perl script collects some small functions that facilitate working with polymake.
# The following functions are available:
#  - fstVecCoordToOne(Vector<Rational> vec)             -> Vector<Rational>
#  - allOnesVec(int dim)                                -> Vector<Rational>
#  - fstColToAllOnes(Matrix<Rational> matrix)           -> Matrix<Rational>
#  - matrixToArrayOfVec(Matrix<Rational> matrix)        -> Array<Vector<Rational>>
#  - arrayOfVecToMatrix(Array<Vector<Rational>>)        -> Matrix<Rational>
#  - addAllOnesToMatrix(Matrix<Rational> matrix)        -> Matrix<Rational>
#  - rationalToString(Rational rational)                -> String
#  - rationalVecToString(Vector<Rational> vec)          -> String
#  - getIndexFromMatrix(Vector vec, Matrix mat)         -> Int
#  - getArrayNumberFromArrayOfSets(Int index, Array<Set> arrOfSets) -> Int
#  - w2v(String)                                        -> Vector 
#  - string2Vec(String)                                 -> Vector
#  - matrix2Set(Matrix)                                 -> Set<Vector<Rational>>
#  - commonVectorsInMatrices                            -> Set<Vector<Rational>>
#  - compareMatrices                                    -> Int
#  - vlabels(Matrix vertices, Bool wo_zero)             -> arrayref
#--------------------------------------------------------------------------------------------------
####################################################################################################

# Sets the first coordinate of a given vector to 1.
# @param Vector<Rational> vec                   the vector to be changed
# @return Vector<Rational>                      the original vector with first coordinate 
#                                               set to 1
sub fstVecCoordToOne{
    my $vec=$_[0];
    my @vecArr=@$vec;
    return new Vector<Rational>(1|new Vector<Rational>(@vecArr[1..scalar(@vecArr)-1]));
 
}


# Creates a vector of given dimension with all elements equal to 1. 
# @param int dim                                the dimension
# @return Vector<Rational>                      an all-ones vector of dimension dim
sub allOnesVec{
    my $dim=$_[0];
    my @allOnesVec=();
    for(my $i=0;$i<$dim;$i++){
	push(@allOnesVec,1);
    }
    return new Vector<Rational>(@allOnesVec);
}


# Sets the first column of a given matrix to the appropriate all-ones vector.
# @param Matrix<Rational> matrix                the matrix to be changed
# @return Matrix<Rational>                      the original matrix with first column
#                                               changed to the all-ones vector
sub fstColToAllOnes{
    my $matrix=$_[0];
    my $newMatrix=new Matrix<Rational>($matrix->rows,$matrix->cols);
    $newMatrix->col(0)=allOnesVec($matrix->rows);
    for(my $i=1;$i<$matrix->cols;$i++){
	$newMatrix->col($i)=$matrix->col($i);	
    }
    return $newMatrix;
}


# Constructs an array which contains the rows of the given matrix.
# @param Matrix<Rational> matrix                the matrix to be transformed  
# @return Array<Vector<Rational>>               the array containing the rows of the matrix
sub matrixToArrayOfVec{
    my $matrix=$_[0];
    my @array=map{$matrix->row($_)} 0..$matrix->rows-1;
    return new Array<Vector<Rational>>(\@array);
}


# Constructs a matrix by taking the elements of an array as its rows.
# @param Array<Vector<Rational>> arrOfVec       the array of vectors to be transformed  
# @return Matrix<Rational>                      the corresponding matrix
sub arrayOfVecToMatrix{
    my $arrOfVec=$_[0];
    my @array=@$arrOfVec;
    my $matrix=new Matrix<Rational>(scalar(@array),$array[0]->dim);
    map{$matrix->row($_)=$array[$_]}0..$#array;
    return $matrix;
}


# Adds an all-ones-column to a given matrix (-> homogenization!).
# @param Matrix<Rational> matrix                the matrix to which the all-ones vector is added
# @return Matrix<Rational>                      the original matrix with an all-ones column in front
sub addAllOnesToMatrix{
    my $matrix=$_[0];
    my $vec=allOnesVec($matrix->rows);
    return new Matrix<Rational>($vec|$matrix);
}


# Produces a string from a rational number. 
# Supposed to be used for generating filenames for different parameter settings.
# @param Scalar rational the parameter to be translated into a string
# @return String the string generated from //rational//
sub rationalToString{
    my $rational_in=$_[0];
    my $rational=convert_to<Rational>($rational_in);
    my $string="";
    if($rational<0){
	$string.="m";
	$rational*=-1;
    }
    my $num=numerator($rational);
    my $denom=denominator($rational);
    $string.="".$num."";
    if($denom!=1){
	$string.="d".$denom."";
    }
    return $string;
}

# Produces a string from a vector of rational number. 
# Supposed to be used for generating filenames for different parameter settings.
# @param Vector vec the vector to be translated into a string
# @return String the string generated from //vec//
sub rationalVecToString {
    my $vec=$_[0];
    my $string="";
    foreach (@$vec) {
	$string.="".rationalToString($_)."";
    }
    return $string;
}

# Looks up the index of a given vector //vec// in 
# the matrix //mat//.
# @param Vector vec the vector whose index is to be determined
# @param Matrix mat the matrix which contains //vec//
# @return Int the index of //vec// in //mat//, or -1 if //vec// is not contained
sub getIndexFromMatrix {
    my ($vec, $mat) = @_;
    for ( my $i = 0; $i < $mat->rows; ++$i ) {
	if ( $vec == $mat->row($i) ) {
	    return $i;
	}
    }
    return -1;
}

# Looks for an integer //index// in the sets which are
# contained in the given array of sets //arrOfSets//.
# Returns the index of the set within the array
# which contains the given //index//.
# @param Int index the given index
# @param Array<Set> an array of sets
# @return Int the index of the set which contains //index//
sub getArrayNumberFromArrayOfSets {
    my ($index, $arrOfSets) = @_;
    my $found = 0;

    for ( my $i=0; $i<$arrOfSets->size; ++$i ) {
	my $set = $arrOfSets->[$i];  
	if ( $set->contains($index) ) {
	    $found = 1;
	    return $i;
	}
    }

    if ( !$found ) {
	croak("The index $index was not contained in the array of sets");
    }

}


# Transforms a string of numbers separated by whitespaces
# into a vector (e.g. for copying vectors from polymake output).
# @param String v the given string representing the vector
# @return Vector the string //v// converted to a vector
sub w2v {
	my $v = shift;
	return new Vector(split /[,\s]+/, $v);
}

# copied from w2v, see above
sub string2Vec { 
	my $v = shift;
	return new Vector(split /[,\s]+/, $v);
}

# Transforms a matrix //m// into a Set<Vector>.
# Help function for filter function 'commonVectorsInMatrices'
# and the compare function 'compareMatrices'.
# @param Matrix m the given matrix
# @return Set<Vector> the corresponding set of vectors
sub matrix2Set {
  my $m = $_[0];
  my @vecs = map {$m->[$_]} 0..$m->rows-1;
  return new Set< Vector<Rational> > (\@vecs);
}

# Filters out the common vectors in two given matrices //m1// and //m2//.
# @param Matrix m1 the first matrix
# @param Matrix m2 the second matrix
# @return Set<Vector> the set of common vectors in //m1// and //m2//
sub commonVectorsInMatrices {
  my ($m1,$m2) = @_;
  return matrix2Set($m1)*matrix2Set($m2);
}

# Compares the sets of rows of //m1// and //m2// via 'incl'.
# @param Matrix m1 the first matrix
# @param Matrix m2 the second matrix
# @return Int 1 if m1 contains m2
#            -1 if m2 contains m1
#             0 if m1 is equal to m2
#             2 otherwise
sub compareMatrices {
    my ($m1, $m2) = @_;    
    return incl(matrix2Set($m1),matrix2Set($m2));
}

# Transforms the coordinates of points (e.g. vertices)
# in a given matrix //vertices// into strings. 
# These can be used for instance for labeling 
# the vertices of a polytope during visualization:
# $p->VISUAL(VertexLabels=>vlabels($p->VERTICES,1)).
# The parameter //wo_zero// allows for omitting
# Coordinate 0 (used for homogenization) in the output.
# @param Matrix vertices the matrix of points
# @param Bool wo_zero omits Coordinate 0 if set to 1
# @return arrayref the labels of the given points as strings 
sub vlabels {
    my ($vertices, $wo_zero) = @_;
    my @vlabels=();
    if ($wo_zero) {
	@vlabels = map {"(".join(",",@{$vertices->[$_]->slice(1,$vertices->cols-1)}).")"} 0..$vertices->rows-1;
    } else {
	@vlabels = map {"(".join(",",@{$vertices->[$_]}).")"} 0..$vertices->rows-1;
    }
    return \@vlabels;
}


################################################################################################
# End
################################################################################################


1; #to avoid return error


################################################################################################
# End of Code
################################################################################################



# Local Variables:
# c-basic-offset:3
# mode: perl
# End:
