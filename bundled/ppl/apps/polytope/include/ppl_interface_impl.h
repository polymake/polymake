/* Copyright (c) 1997-2021
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#pragma once

#include <cstddef> // needed for gcc 4.9, see http://gcc.gnu.org/gcc-4.9/porting_to.html

#if defined(__APPLE__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include <gmpxx.h> //for mpz/mpq-handling

#if defined(__APPLE__)
#pragma clang diagnostic pop
#endif

#include "polymake/ListMatrix.h"
#include "polymake/internal/gmpxx_traits.h"
#include "polymake/polytope/ppl_interface.h"
#include "polymake/common/lattice_tools.h"
#include "polymake/linalg.h"
#include "polymake/hash_set"
#include "polymake/polytope/compress_incidence.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <ppl.hh>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <fenv.h>

namespace PPL = Parma_Polyhedra_Library;

namespace polymake { namespace polytope { namespace ppl_interface {

//! PPL tends to modify the floating point operation mode, in particular, the rounding direction.
//! This seems to happen during initial load of the ppl library,
//! therefore the mode preferred by PPL is captured in a singleton living in this shared object (polytope).
//! Since this shared object depends on libppl, its global constructors will always be executed after libppl is loaded.

class fp_mode_setter
{
public:
   fp_mode_setter()
   {
      fesetround(captured.mode);
   }

   ~fp_mode_setter()
   {
      fesetround(FE_TONEAREST);
   }

private:
   class init
   {
   public:
      init()
         : version(PPL::version_major()) // ensure libppl is really loaded
         , mode(fegetround())
      {
         fesetround(FE_TONEAREST);
      }

      const int version;
      const int mode;
   };

   static init captured;
};

namespace {

// Constructs an (integral) mpz-vector by multiplying with lcm of denominators.
// Note: this is different from 'primitive', e.g. (2/3,4/3)->(2,4), not (1,2)
template <typename Scalar>
std::vector<mpz_class> convert_to_mpz(const Vector<Scalar>& v, const Integer& denom)
{
   Vector<Integer> v_multi(denom*v); //This cast works since denom*v is integral by construction!!

   std::vector<mpz_class> mpz_vec(v.dim());
   for (Int i = 0; i < v.dim(); ++i) {
      mpz_vec[i] = mpz_class(v_multi[i].get_rep());
   }
   return mpz_vec;
}

// Translates a ppl generator into a (homogenized) polymake vector,
// depending on its type (point, ray, line).
template <typename Scalar>
Vector<Scalar> ppl_gen_to_vec(const PPL::Generator& gen, const bool isCone)
{
   const Int dim = gen.space_dimension()+1;
   Vector<Scalar> vec(dim);

   for (Int i = 1; i < dim; ++i)
      vec[i] = Integer(gen.coefficient(PPL::Variable(i-1)));

   if (gen.is_point()) { // in case of isCone, generators are rays or lines except for (0,...,0)

      // gen.divisor only works for PPL-(closure-)points
      vec /= Integer(gen.divisor());
      vec[0] = 1;
   } else {
      assert(gen.is_ray() || gen.is_line());
   }

   return vec;
}

// Translates a ppl constraint (n entries, inhomogeneous term as an attribute)
// into a polymake vector.
template <typename Scalar>
Vector<Scalar> ppl_constraint_to_vec(const PPL::Constraint& cons, const bool isCone)
{
   const Int dim = cons.space_dimension()+1;
   Vector<Scalar> vec(dim);
   vec[0] = cons.inhomogeneous_term(); // will be overwritten if isCone = true
   for (Int i = 1; i < dim; ++i)
      vec[i] = Integer(cons.coefficient(PPL::Variable(i-1)));

   return vec;
}


template <typename Scalar>
PPL::C_Polyhedron construct_ppl_polyhedron_H(const Matrix<Scalar>& Inequalities, const Matrix<Scalar>& Equations, const bool isCone)
{
   /* Linear expressions in H representations store the inhomogeneous term at index 0,
    * and the variables' coefficients at indices 1, 2, ..., space_dim.
    */

   PPL::Constraint_System cs;

   const Int dim = std::max(Inequalities.cols(), Equations.cols())-1;

   cs.set_space_dimension(dim);

   // insert inequalities
   for (auto row_it = entire(rows(Inequalities)); !row_it.at_end(); ++row_it) {
      Integer lcm_of_row_denom(lcm(denominators(*row_it)));
      std::vector<mpz_class> coefficients = convert_to_mpz<Scalar>(*row_it, lcm_of_row_denom);
      // PPL variables have indices 0, 1, ..., space_dim-1.
      PPL::Linear_Expression e;
      for (Int j = dim; j >= 1; --j) {
         e += coefficients[j] * PPL::Variable(j-1);
      }
      e += coefficients[0];

      cs.insert(e >= 0);
   }

   // insert equations
   for (auto row_it = entire(rows(Equations)); !row_it.at_end(); ++row_it) {
      Integer lcm_of_row_denom(lcm(denominators(*row_it)));
      std::vector<mpz_class> coefficients = convert_to_mpz<Scalar>(*row_it, lcm_of_row_denom);
      // PPL variables have indices 0, 1, ..., space_dim-1.
      PPL::Linear_Expression e;
      for (Int j = dim; j >= 1; --j) {
         e += coefficients[j] * PPL::Variable(j-1);
      }
      e += coefficients[0];

      cs.insert(e == 0);
   }

   PPL::C_Polyhedron ppl_poly(cs);
   return ppl_poly;
}


template <typename Scalar>
PPL::C_Polyhedron construct_ppl_polyhedron_V(const Matrix<Scalar>& Points, const Matrix<Scalar>& Lineality, const bool isCone)
{
   // The V representation
   PPL::Generator_System gs;
   const Int dim = std::max(Points.cols(), Lineality.cols())-1; // necessary if only one matrix has entries

   gs.set_space_dimension(dim);

   /* Cones need an additional point (0,...,0) in ppl.
    * Furthermore, the cone is translated into a polytope.
    */
   if (isCone) {
      PPL::Generator v = PPL::point(0*PPL::Variable(dim-1)); //origin
      gs.insert(v);
   }

   // insert points/rays
   for (auto row_it : attach_selector(rows(Points), operations::non_zero())) {
      Integer lcm_of_row_denom(lcm(denominators(row_it)));
      std::vector<mpz_class> coefficients = convert_to_mpz<Scalar>(row_it, lcm_of_row_denom);
      // PPL variables have indices 0, 1, ..., space_dim-1.
      PPL::Linear_Expression e;
      for (Int j = dim; j >= 1; --j) {
         e += coefficients[j] * PPL::Variable(j-1);
      }

      if (coefficients[0] != 0) {
         PPL::Generator v = PPL::point(e, lcm_of_row_denom.gmp() ); // v is a point
         gs.insert(v);
      } else {
         PPL::Generator v = PPL::ray(e); // v is a ray
         gs.insert(v);
      }
   }

   // insert linealities
   for (auto row_it : attach_selector(rows(Lineality), operations::non_zero())) {
      Integer lcm_of_row_denom(lcm(denominators(row_it)));
      std::vector<mpz_class> coefficients = convert_to_mpz<Scalar>(row_it, lcm_of_row_denom);
      // PPL variables have indices 0, 1, ..., space_dim-1.
      PPL::Linear_Expression e;
      for (Int j = dim; j >= 1; --j) {
         e += coefficients[j] * PPL::Variable(j-1);
      }
      PPL::Generator l = line(e);
      gs.insert(l);
   }

   PPL::C_Polyhedron ppl_poly(gs);
   return ppl_poly;
}

} // End of namespace


/* FIXME (1): Don't know how to handle Float and Rational
     simultaneously w.r.t. defining the optimal value */

template <typename Scalar>
convex_hull_result<Scalar>
ConvexHullSolver<Scalar>::enumerate_facets(const Matrix<Scalar>& Points, const Matrix<Scalar>& Lineality, const bool isCone) const
{
   const Int num_columns = std::max(Points.cols(), Lineality.cols());

   PPL::C_Polyhedron polyhedron = construct_ppl_polyhedron_V(Points, Lineality, isCone);
   Set<Int> far_face(far_points(Points));

   PPL::Constraint_System cs = polyhedron.minimized_constraints();
   ListMatrix< Vector<Scalar> > facet_list(0, num_columns);
   ListMatrix< Vector<Scalar> > affine_hull_list(0, num_columns);

   const auto triv_ineq=unit_vector<Scalar>(num_columns, 0);

   for (PPL::Constraint_System::const_iterator csi = cs.begin(); csi != cs.end(); ++csi) {
      const PPL::Constraint& c = *csi;
      Vector<Scalar> row = ppl_constraint_to_vec<Scalar>(c, isCone);
      if (!(isCone && row == triv_ineq )) {
         if (c.is_inequality()) {
            // TODO: std::move(row) when move constructors implemented for vector classes
            facet_list /= row;
         } else {
            assert(c.is_equality());
            // TODO: std::move(row) when move constructors implemented for vector classes
            affine_hull_list  /= row;
         }
      }
   }

   // ppl seems to compute the far face inequality (shown in cs.ascii_dump())
   // but the iterator above skips it...
   // So we use the following to determine whether it is needed and add it manually:

   // We use the rank of the far-face rays to determine
   // whether we need to add the trivial inequality as facet.
   // The case that p is just a point is also covered by this!
   if (!isCone && rank(Points.minor(far_face,All)/Lineality)+1 == num_columns - affine_hull_list.rows()) {
      facet_list /= triv_ineq;
   }

   return { Matrix<Scalar>(facet_list), Matrix<Scalar>(affine_hull_list) };
}


template <typename Scalar>
convex_hull_result<Scalar>
ConvexHullSolver<Scalar>::enumerate_vertices(const Matrix<Scalar>& Inequalities, const Matrix<Scalar>& Equations, const bool isCone) const
{
   const Int num_columns = std::max(Inequalities.cols(), Equations.cols());
   // an empty exterior description defines the empty (infeasible) polytope
   // (ppl would return the whole space)
   // for cones this is the full space and correctly handled later on
   if (!isCone && Inequalities.rows() + Equations.rows() == 0)
      return { Matrix<Scalar>(0, num_columns), Matrix<Scalar>(0, num_columns) };

   PPL::C_Polyhedron polyhedron = construct_ppl_polyhedron_H(Inequalities, Equations, isCone);
   PPL::Generator_System gs = polyhedron.minimized_generators();
   ListMatrix<Vector<Scalar>> vertex_list(0,num_columns);
   ListMatrix<Vector<Scalar>> lin_space_list(0,num_columns);

   const auto cone_origin=unit_vector<Scalar>(num_columns, 0);

   for (PPL::Generator_System::const_iterator gsi = gs.begin(); gsi != gs.end(); ++gsi) {
      const PPL::Generator& g = *gsi;
      Vector<Scalar> row = ppl_gen_to_vec<Scalar>(g, isCone);
      if (!(isCone && row == cone_origin)) {
         if (g.is_point() || g.is_ray()) {
            // TODO: std::move(row) when move constructors implemented for vector classes
            vertex_list /= row;
         } else {
            assert(g.is_line());
            // TODO: std::move(row) when move constructors implemented for vector classes
            lin_space_list  /= row;
         }
      }
   }

   return { Matrix<Scalar>(vertex_list), Matrix<Scalar>(lin_space_list) };
}

template <typename Scalar>
LP_Solution<Scalar>
LP_Solver<Scalar>::solve(const Matrix<Scalar>& Inequalities, const Matrix<Scalar>& Equations,
                         const Vector<Scalar>& Objective, bool maximize, bool) const
{
   // establish the PPL rounding mode temporarily
   fp_mode_setter fp_mode;

   LP_Solution<Scalar> result;

   const Int num_columns = std::max(Inequalities.cols(), Equations.cols())-1;
   if (num_columns == -1) {
      result.status = LP_status::infeasible;
      return result;
   }

   PPL::C_Polyhedron polyhedron = construct_ppl_polyhedron_H(Inequalities, Equations, 0); // isCone = 0

   // Linear program
   const Integer lcm_of_obj_denom = lcm(denominators(Objective));
   std::vector<mpz_class> objective = convert_to_mpz<Scalar>(Objective, lcm_of_obj_denom);

   PPL::Linear_Expression e;
   for (Int j = num_columns; j >= 1; --j) {
      e += objective[j] * PPL::Variable(j-1);
   }
   e += objective[0];
   PPL::Coefficient bound_n, bound_d;   // same as mpz_class
   bool is_opt;
   PPL::Generator g_opt = PPL::point();
   const bool solvable = maximize ? polyhedron.maximize(e, bound_n, bound_d, is_opt, g_opt)
                                  : polyhedron.minimize(e, bound_n, bound_d, is_opt, g_opt);

   if (!solvable) { // ppl returns false if input is infeasible OR unbounded!
      result.status = polyhedron.is_empty() ? LP_status::infeasible : LP_status::unbounded;
   } else {
      result.status = LP_status::valid;
      result.solution = ppl_gen_to_vec<Scalar>(g_opt, false);

      // opt_val needs to be divided additionally by the lcm of the denominators
      // of the objective vector since the constructed Linear_Expression e
      // has been multiplied by this factor in 'convert_to_mpz'.
      // Note that ppl seems to work only with integral objective functions.
      result.objective_value.set(Integer(bound_n), Integer(bound_d)*lcm_of_obj_denom);
   }
   return result;
}

} } }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
