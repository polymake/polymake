#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/Rational.h"

namespace polymake { namespace common {

//FIXME: add const Array<std::string>& row_labels
template <typename Scalar>
void print_constraints_sub(const Matrix<Scalar>& M, const bool are_eqs, const Array<std::string>& coord_labels)
{
   for (int i=0; i < M.rows(); ++i) {
      cout <<  i << ": ";
      if (M.row(i).slice(range(1,M.cols()-1)) == zero_vector<Rational>(M.cols()-1)) {
	 cout << "0";
      } else {
	 bool first=true;
	 for (int j=1;j<M.cols();++j) {
	    Scalar cur_coeff = M.row(i)[j];
	    if ( cur_coeff != 0 ) {
	       if ( ! first )
		  cout << " ";
	       if ( cur_coeff > 0 ) {
		  if ( ! first )
		     cout << "+ ";
		  if ( cur_coeff != 1 )
		     cout << std::setprecision(16) << cur_coeff << " ";
	       }
	       if ( cur_coeff < 0 ) {
		  if ( ! first )
		     cout << "- ";
		  else
		     cout << "-";
		  if ( cur_coeff != -1 )
		     cout << std::setprecision(16) << -cur_coeff << " ";
	       }
	       first=false;
	       cout << coord_labels[j-1];
	    }
	 }
      }
      if ( are_eqs )
	 cout << " = ";
      else
	 cout << " >= ";
      Scalar neg_rhs = M.row(i)[0];
      cout << std::setprecision(16) << (neg_rhs!=0 ? -neg_rhs : neg_rhs) << '\n';
   }
   cout << endl;
}

} }
