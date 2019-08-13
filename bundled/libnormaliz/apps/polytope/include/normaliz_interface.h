#ifndef POLYMAKE_POLYTOPE_NORMALIZ_INTERFACE_H
#define POLYMAKE_POLYTOPE_NORMALIZ_INTERFACE_H

namespace polymake { namespace polytope {

   Matrix<Integer> normaliz_compute_lattice(const Matrix<Integer> & V, int thread = 0);

}}

#endif
