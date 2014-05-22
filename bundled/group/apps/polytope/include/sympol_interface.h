#ifndef POLYMAKE_POLYTOPE_SYMPOL_INTERFACE_H
#define POLYMAKE_POLYTOPE_SYMPOL_INTERFACE_H

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/group/permlib.h"

namespace sympol {
   class Polyhedron;
   class QArray;
}

namespace polymake { namespace polytope { namespace sympol_interface {

enum SympolRayComputationMethod {
	lrs,
	cdd,
	beneath_beyond
};

class sympol_wrapper {
public: 
   static group::PermlibGroup compute_linear_symmetries (const Matrix<Rational>& inequalities, const Matrix<Rational>& equations);
   /**
    * @param inequalities inequalities of the polytope
    * @param equations equations of the polytope
    * @param symmetry_group symmetry group acting on the inequality/equation indices
    * @param rayCompMethod the library to use for convex hull computations (e.g. lrs or cdd)
    * @param idmLevel recursion level up to which IDM is used (must not be negative)
    * @param admLevel recursion level up to which ADM is used (must not be negative, must be greater or equal than //idmLevel//)
    * @param dual true iff input are rays and lineality space
    * @param out_inequalities vertices of the polytope up to symmetry
    * @param out_equations affine hull of the polytope up to symmetry
    * @return true iff SymPol computation was successful
    */
   static bool computeFacets(const Matrix<Rational>& inequalities, const Matrix<Rational>& equations, const group::PermlibGroup& symmetry_group, SympolRayComputationMethod rayCompMethod, int idmLevel, int admLevel, bool dual, Matrix<Rational>& out_inequalities, Matrix<Rational>& out_equations);
   static bool computeFacets(const Matrix<Rational>& inequalities, const Matrix<Rational>& equations, const group::PermlibGroup& symmetry_group, SympolRayComputationMethod rayCompMethod, bool dual, Matrix<Rational>& out_inequalities, Matrix<Rational>& out_equations) {
     return computeFacets(inequalities, equations, symmetry_group, rayCompMethod, 0, 1, dual, out_inequalities, out_equations);
   }
   
   static sympol::Polyhedron* assembleSympolPolyhedron(const Matrix<Rational>& inequalities, const Matrix<Rational>& equations, bool dual, bool& is_homogeneous);
   
   static std::list<sympol::QArray> matrix2QArray(const Matrix<Rational>& A, bool& is_homogeneous);

};



} } }

#endif // POLYMAKE_POLYTOPE_SYMPOL_INTERFACE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
