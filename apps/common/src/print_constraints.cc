#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include <fstream>
#include "polymake/common/print_constraints.h"

namespace polymake { namespace common {

   template <typename Scalar>
   void print_constraints(const Matrix<Scalar>& M, const int are_eqs, perl::OptionSet options){

    Array<std::string> coord_labels;
    std::string var="x";
    if(options["coord_labels"] >> coord_labels){
      if(coord_labels.size()!=M.cols()-1) throw std::runtime_error("print_constraints - Wrong number of variables!");
    } else {
      coord_labels.resize(M.cols()-1);
      for(int i=1;i<M.cols();++i){
	std::ostringstream var_name;
	var_name << var << i;
	coord_labels[i-1]=var_name.str();
      }
    }
    print_constraints_sub(M,are_eqs,coord_labels);

   }




   UserFunctionTemplate4perl("# @category Formatting"
			     "# Write the rows of a matrix //M// as inequalities (//are_eqs=0//)"
			     "# or equations (//are_eqs=1//) in a readable way."
			     "# It is possible to specify labels for the coordinates via"
			     "# an optional array //coord_labels//."
			     "# @param Matrix<Scalar> M the matrix whose rows are to be written"
			     "# @param Int are_eqs optional; needs to be set to 1 if the rows"
			     "#    represent equations instead of inequalities"
			     "# @option Array<String> coord_labels changes the labels of the coordinates"
			     "#    from 'x<sub>1</sub>,...,x<sub>n</sub>' to the specified names",
			     "print_constraints<Scalar>(Matrix<Scalar>; $=0, { coord_labels => undef }) : void");



}}
