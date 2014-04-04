#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include <fstream>
#include "polymake/common/print_constraints.h"

namespace polymake { namespace polytope {


   template <typename Scalar>
   bool print_constraints(perl::Object P){
     
     Matrix<Scalar> Ineqs=P.give("FACETS|INEQUALITIES");    
     Matrix<Scalar> Eqs=P.give("AFFINE_HULL|EQUATIONS");
     Array<std::string> coord_labels;
     std::string var="x";
     if(P.exists("COORDINATE_LABELS")){
       coord_labels=P.give("COORDINATE_LABELS");
     } else {
       coord_labels.resize(Ineqs.cols()-1);
      for(int i=1;i<Ineqs.cols();++i){
	std::ostringstream var_name;
	var_name << var << i;
	coord_labels[i-1]=var_name.str();
      }
     }
     if(Ineqs.rows()>0){
       cout << (P.exists("FACETS") ? "Facets:" : "Inequalities:") << endl;
       common::print_constraints_sub(Ineqs,0,coord_labels);
     }
     if(Eqs.rows()>0){
       cout << (P.exists("AFFINE_HULL") ? "Affine hull:" : "Equations:") << endl;
       common::print_constraints_sub(Eqs,1,coord_labels);
     }
     return true;
   }



   UserFunctionTemplate4perl("# @category Optimization"
			     "# Write the [[FACETS]] / [[INEQUALITIES]] and the [[AFFINE_HULL]] / [[EQUATIONS]]"
			     "# of a polytope //P// in a readable way."
			     "# [[COORDINATE_LABELS]] are adopted if present."
			     "# @param Polytope<Scalar> P the given polytope"
			     "# @return bool",
			     "print_constraints<Scalar>(Polytope<Scalar>)");



}}
