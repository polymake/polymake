/* Copyright (c) 1997-2020
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

#include "polymake/client.h"
#include "polymake/polytope/separating_hyperplane.h"

namespace polymake { namespace polytope {

namespace {

template <typename TMatrix>
decltype(auto) first_non_ray(const GenericMatrix<TMatrix>& V)
{
   for (auto r = entire(rows(V)); !r.at_end(); ++r) {
      if (!is_zero(r->front()))
         return *r;
   }
   throw std::runtime_error("all VERTICES | POINTS are rays");
}

// translate all entries of the matrix that do not have 0 as first coordinate (i.e. non-rays) by t
template <typename Scalar>
Matrix<Scalar> translate_non_rays(const Matrix<Scalar>& M, const Vector<Scalar>& t)
{
   Matrix<Scalar> tM(M.rows(), M.cols());
   auto tr = rows(tM).begin();
   for (auto r = entire(rows(M)); !r.at_end(); ++r, ++tr) {
      if (is_zero(r->front()))
         *tr = *r;
      else
         *tr = *r-t;
   }
   return tM;
}

template <typename Scalar>
Vector<Scalar> separate_strong(BigObject p1, BigObject p2)
{
   const Matrix<Scalar>
      V = p1.give("VERTICES | POINTS"),
      W = p2.give("VERTICES | POINTS"),
      L1 = p1.give("LINEALITY_SPACE | INPUT_LINEALITY"),
      L2 = p2.give("LINEALITY_SPACE | INPUT_LINEALITY");

   // origin needs to be on the positive side of the separating plane in order for the LP to work.
   // translate the whole thing s.t. v_0 is the origin
   Vector<Scalar> t = zero_value<Scalar>() | first_non_ray(V).slice(range_from(1));

   const Matrix<Scalar> ineqs( ( (translate_non_rays(V,t) / -translate_non_rays(W,t)) | //V is on the positive, W on the negative side of the hyperplane
                                 ( -ones_vector<Scalar>(V.rows()+W.rows()))) //vz - eps >= 0 and wz + eps <= 0
                               /  unit_vector<Scalar>(V.cols()+1, V.cols()) //eps >=0
                               / (unit_vector<Scalar>(V.cols()+1, 0) - unit_vector<Scalar>(V.cols()+1, V.cols()))); //eps <= z_0 to ensure boundedness (the solver sets z_0 = 1 or z_0 = 0 always)

   Matrix<Scalar> eqs(0, V.cols());
   if (L1.rows()) eqs /= L1;
   if (L2.rows()) eqs /= L2;
   eqs = eqs | zero_vector<Scalar>();

   const auto S = solve_LP(ineqs, eqs, unit_vector<Scalar>(V.cols()+1, V.cols()), true);  // maximize eps
   if (S.status != LP_status::valid || S.objective_value == 0)
      throw infeasible();  // the only separating plane is a weak one

   Vector<Scalar> sol = S.solution.slice(sequence(0, V.cols()));

   // translate sol back to the original polytope position
   for (Int i = 1; i < sol.dim(); ++i) {
      if (!is_zero(sol[i])) {
         t[i] -= sol[0] / sol[i];
         break;
      }
   }
   t[0] = 1;
   sol[0] = -(sol*t - sol[0]);
   return sol;
}

// finds a weak separating plane for a full-dimensional polytope p1 from a polytope p2
// t is an inner point of p1 that is not a vertex of p2
// called from separate_weak
template <typename Scalar>
Vector<Scalar> separate_weak(BigObject p1, BigObject p2, Vector<Scalar> t)
{
   const Matrix<Scalar>
      V = p1.give("VERTICES | POINTS"),
      W = p2.give("VERTICES | POINTS"),
      L1 = p1.give("LINEALITY_SPACE | INPUT_LINEALITY"),
      L2 = p2.give("LINEALITY_SPACE | INPUT_LINEALITY");

   // the solution of the LP is supposed to define the separating hyperplane.
   // it thus must be constructed in a way that rules out the origin as a solution
   // as that does not define a plane. therefore the following two choices were made.

   // origin needs to be on the positive side of the separating plane and must not be a vertex of p1
   // in order for the LP to work. translate the whole thing s.t. the origin lies in the interior of p1
   t[0] = zero_value<Scalar>();

   // obj must not be a vertex of p1 or p2, so we choose an inner point of the translated polytope.
   const Vector<Scalar> obj = one_value<Scalar>() | ((first_non_ray(V) - t)/2).slice(range_from(1));

   const Matrix<Scalar> ineqs( ( translate_non_rays(V,t) / -translate_non_rays(W,t)) //V is on the positive, W on the negative side of the hyperplane
                               / (2 | obj.slice(range_from(1)))); //obj*z >= -1 to ensure boundedness

   Matrix<Scalar> eqs(0, V.cols());
   if (L1.rows()) eqs /= L1;
   if (L2.rows()) eqs /= L2;

   const auto S = solve_LP(ineqs, eqs, obj, false);  // minimize obj*z
   if (S.status != LP_status::valid)
      throw infeasible();

   Vector<Scalar> sol = S.solution;

   // translate sol back to the original polytope position
   for (Int i = 1; i < sol.dim(); ++i) {
      if (!is_zero(sol[i])) {
         t[i] -= sol[0] / sol[i];
         break;
      }
   }
   t[0] = one_value<Scalar>();
   sol[0] = -(sol*t - sol[0]);
   return sol;
}


// the separating plane is found using an LP whose solution is the normal vector.
// to_solver forces the first coodinate of the solution to be 1, so the separating plane
// cannot pass through the origin (as that would mean first coordinate 0).
// to avoid that, we translate the polytopes such that the origin lies in the interior
// of the polytope that is supposed to lie on the positive side of the plane.
// in case both polytopes are low dimensional, we have to take extra care (see below)
template <typename Scalar>
Vector<Scalar> separate_weak(BigObject p1, BigObject p2)
{
   const Matrix<Scalar>
      V = p1.give("VERTICES | POINTS"),
      W = p2.give("VERTICES | POINTS"),
      L1 = p1.give("LINEALITY_SPACE | INPUT_LINEALITY"),
      L2 = p2.give("LINEALITY_SPACE | INPUT_LINEALITY");

   Matrix<Scalar> ker1 = null_space(V/L1);

   if (!ker1.rows()) { //p1 is fulldim
      Vector<Scalar> inn = p1.give("REL_INT_POINT");
      return separate_weak<Scalar>(p1,p2,inn);
   }

   Matrix<Scalar> ker2 = null_space(W/L2);

   if (!ker2.rows()) { //p2 is fulldim
      Vector<Scalar> inn = p2.give("REL_INT_POINT");
      return -separate_weak<Scalar>(p2,p1,inn); //p2 is fulldim => swap
   }

   // both are lowdim

   if (V.rows() != 1) {  // p1 is not a point
      try {
         // we need to find an inner point of p1 that is not a vertex of p2.
         // the loop terminates after a maximum of 2 restarts as there is a maximum of
         // 2 vertices of p2 on the line from inn to V[i]

         const auto non_ray = first_non_ray(V);

         Vector<Scalar> inn = p1.give("REL_INT_POINT");
         for (auto r = entire(rows(W)); !r.at_end(); ) {
            if (*r == inn) {
               inn = one_value<Scalar>() | (inn+(non_ray - inn)/2).slice(range_from(1));
               r = entire(rows(W));
            }
            else
               ++r;
         }

         return separate_weak<Scalar>(p1, p2, inn);
      } catch(const infeasible&) { }
   }

   if (W.rows() != 1) {  // p2 is not a point
      try {
         const auto non_ray = first_non_ray(W);

         // maybe the inner point lies on the separating plane!
         // try the same for p1 and p2 swapped:
         Vector<Scalar> inn = p2.give("REL_INT_POINT");
         for (auto r = entire(rows(V)); !r.at_end(); ) {
            if (*r == inn) {
               inn = one_value<Scalar>() | (inn+(non_ray - inn)/2).slice(range_from(1));
               r = entire(rows(V));
            }
            else ++r;
         }
         return -separate_weak<Scalar>(p2, p1, inn);
      } catch (const infeasible&) { }
   }

   // there are only two options left:
   // either both inner points lie on the separating plane, so p1 and p2 lie in a common hyperplane,
   // or they are inseparable.
   if ((W*T(ker1)).non_zero()) throw infeasible(); // no common plane
   return ker1[0];
}

}

// separate two polytopes
template< typename Scalar>
Vector<Scalar> separating_hyperplane(BigObject p1, BigObject p2, OptionSet options)
{
   bool strong = options["strong"];
   Vector<Scalar> sol;
   try {
      if (strong)
         sol = separate_strong<Scalar>(p1,p2);
      else
         sol = separate_weak<Scalar>(p1,p2);
   }
   catch (const infeasible&) {
      throw std::runtime_error("separating_hyperplane: the given polytopes cannot be separated");
   }
   return sol;
}

template <typename Scalar>
bool cone_contains_point(BigObject p, const Vector<Scalar> & q, OptionSet options)
{
   // the LP constructed here is supposed to find a conical/convex combination of the rays/vertices
   // of cone/polytope p that equals q. if the in_interior flag is set, all coefficients are
   // required to be strictly positive.
   Matrix<Scalar> P = p.give("RAYS | INPUT_RAYS");

   Matrix<Scalar> eqs = -q | T(P) ; //sum z_i p_i = q
   //(if p_i are given in hom.coords, also sum z_i = 1 for all i whose p_i are not rays)

   //lookup prevents computation of this property in case it is not given.
   Matrix<Scalar> E = p.lookup("LINEALITY_SPACE | INPUT_LINEALITY");
   if (E.rows()) {
      // for polytopes, replace first col of E with 0, so we have sum z_i = 1 only for points
      if (p.isa("Polytope")) E = zero_vector<Scalar>() | E.minor(All,range(1,E.cols()-1));

      eqs = eqs | T(E) | T(-E);
   }
   eqs = eqs | zero_vector<Scalar>();

   Int n = P.rows();
   Matrix<Scalar> ineqs = zero_vector<Scalar>(n) | unit_matrix<Scalar>(n) | zero_matrix<Scalar>(n, 2*E.rows()) | -ones_vector<Scalar>(n); //z_i >= eps for rays

   Int c = n+2+2*E.rows();
   ineqs = ineqs / unit_vector<Scalar>(c, c-1) //eps >= 0
                 / (unit_vector<Scalar>(c, 0) - unit_vector<Scalar>(c, c-1)); // eps <= 1

   const auto S = solve_LP(ineqs, eqs, unit_vector<Scalar>(c, c-1), true);  // maximize eps
   if (S.status != LP_status::valid)
      return false;

   bool in = options["in_interior"];
   return !(in && S.objective_value == 0);  // the only separating plane is a weak one
}

// check for separability of a point and a set of points
template <typename Scalar>
bool separable(BigObject p, const Vector<Scalar>& q, OptionSet options)
{
   bool strong = options["strong"];

   return !cone_contains_point<Scalar>(p, q, OptionSet("in_interior", !strong));
}

FunctionTemplate4perl("cone_contains_point<Scalar> [ is_ordered_field_with_unlimited_precision(type_upgrade<Scalar, Rational>) ](Cone<Scalar>, Vector<Scalar>, {in_interior=>0})");

UserFunctionTemplate4perl("# @category Geometry"
                          "# Checks whether there exists a hyperplane separating a given point //q//"
                          "# from a polytope/cone //P// by solving a suitable LP."
                          "# If true, //q// is a vertex of the polytope defined by //q// and the vertices of //P//."
                          "# To get the separating hyperplane, use __separating_hyperplane__."
                          "# Works without knowing the facets of P!"
                          "# @param Vector q the vertex (candidate) which is to be separated from //P//"
                          "# @param Cone P the polytope/cone from which //q// is to be separated"
                          "# @option Bool strong Test for strong separability. default: true"
                          "# @return Bool 'true' if //q// is separable from //p//"
                          "# @example"
                          "# > $q = cube(2)->VERTICES->row(0);"
                          "# > print separable(cube(2), $q, strong=>0);"
                          "# | true",
                          "separable<Scalar>(Cone<type_upgrade<Scalar>>, Vector<type_upgrade<Scalar>>, {strong=>1})");

UserFunctionTemplate4perl("# @category Optimization"
                          "# Computes (the normal vector of) a hyperplane which separates a given point //q//"
                          "# from //points// via solving a suitable LP. The scalar product of the normal vector"
                          "# of the separating hyperplane and a point in //points// is greater or equal than 0"
                          "# (same behavior as for facets!)."
                          "# If //q// is not a vertex of P=conv(//points//,//q//),"
                          "# the function throws an //infeasible// exception."
                          "# Works without knowing the facets of P!"
                          "# @param Vector q the vertex (candidate) which is to be separated from //points//"
                          "# @param Matrix points the points from which //q// is to be separated"
                          "# @return Vector sep_hyp"
                          "# @example The following stores the result in the List @r and then prints the answer and"
                          "# a description of the hyperplane separating the zeroth vertex of the square from the others."
                          "# > $q = cube(2)->VERTICES->row(0);"
                          "# > $points = cube(2)->VERTICES->minor(sequence(1,3),All);"
                          "# > print separating_hyperplane($q,$points);"
                          "# | 0 1/2 1/2",
                          "separating_hyperplane<Scalar>(Vector<type_upgrade<Scalar>>, Matrix<type_upgrade<Scalar>>)");

UserFunctionTemplate4perl("# @category Optimization"
                          "# Computes (the normal vector of) a hyperplane which separates two given polytopes"
                          "# //p1// and //p2// if possible. Works by solving a linear program, not by facet enumeration."
                          "# @param Polytope p1 the first polytope, will be on the positive side of the separating hyperplane"
                          "# @param Polytope p2 the second polytope"
                          "# @option Bool strong If this is set to true, the resulting hyperplane will be strongly separating,"
                          "#  i.e. it wont touch either of the polytopes. If such a plane does not exist, an exception"
                          "#  will be thrown. default: true"
                          "# @return Vector a hyperplane separating //p1// from //p2//\n",
                          "separating_hyperplane<Scalar>(Polytope<type_upgrade<Scalar>>, Polytope<type_upgrade<Scalar>>, {strong=>1})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
