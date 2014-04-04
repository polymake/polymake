#ifndef POLYMAKE_POLYTOPE_SEPARATING_HYPERPLANE_H
#define POLYMAKE_POLYTOPE_SEPARATING_HYPERPLANE_H

namespace polymake { namespace polytope {

void is_vertex_sub(const Vector<Rational>& q, const Matrix<Rational>& points, bool& answer, Vector<Rational>& sep_hyp);

}}

#endif
