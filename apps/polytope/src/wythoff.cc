/* Copyright (c) 1997-2023
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
#include "polymake/Rational.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Map.h"
#include "polymake/Array.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/ListMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/polytope/simple_roots.h"
#include "polymake/list"
#include "polymake/group/permlib.h"
#include "polymake/polytope/solve_MILP.h"
#include <sstream>
#include <vector>

namespace polymake { namespace polytope {

namespace {

typedef QuadraticExtension<Rational> QE;

template <typename E>
SparseVector<E> find_regular_initial_point(const Set<Int>& rings, const SparseMatrix<E>& R, Int d, char type)
{
   SparseMatrix<E> equations (R);

   if (rings.size() == 1     // we can weasel out because the point is only off of one hyperplane
       ||
       ((type == 'B' || type == 'b' ||
         type == 'C' || type == 'c') &&
        incl(rings, sequence(0, d-1)) <= 0) // we can weasel out because these vectors all have the same length
       ||
       ((type == 'F' || type == 'f') &&
        (incl(rings, sequence(0, 2)) <= 0 ||
         incl(rings, sequence(2, 2)) <= 0)) // we can weasel out because these vectors all have the same length
       ||
       type != 'B' && type != 'b' &&
       type != 'C' && type != 'c' &&
       type != 'F' && type != 'f') {
      for (const auto& r: rings)
         equations(r, 0) = E(-1); // we look for a point not on any hyperplane indexed by rings, but on all others
   } else {
      // the root systems of type B, C, F, G have vectors of different length, so we need to impose equations
      // that put the initial point at equal distance to each ringed hyperplane.
      // However, the normal vectors for type G2 have norms sqrt(2) and sqrt(6), and we can't handle that; 
      // therefore, the orbit polytopes for G2 are not maximally symmetric.

      Vector<QE> rhs(same_element_vector<QE>(QE(0,-1,2), R.rows())); // initialize to a vector of all sqrt{2}'s
      switch (type) {         
      case 'B':
      case 'b':
         rhs[d-1] = QE(-1,0,0); 
         break;

      case 'C':
      case 'c':
         rhs[d-1] = QE(-2,0,0); 
         break;  

      case 'F':
      case 'f':
         rhs[2] = rhs[3] = QE(-1,0,0);
         break;

      default:
         throw std::runtime_error("This shouldn't happen");
      }
      for (const auto& r: rings)
         equations(r, 0) = rhs[r]; // we look for a point not on any hyperplane indexed by rings, but on all others
   } 
   if (type == 'A' || type == 'a')
      equations /= SparseVector<E> (E(0) | ones_vector<E>(d));
   SparseVector<E> p0(null_space(equations)[0]);
   p0.dehomogenize();
   return p0;
}

template <typename E>
SparseVector<E> find_integral_initial_point(const Set<Int>& rings, const SparseMatrix<E>& R, Int d, char type)
{
   if ( 'h' == type ||
        'H' == type )
      throw std::runtime_error("wythoff: The root systems of type H are not crystallographic, so the roots don't generate a lattice.");

   /*
     We want the initial point to be a lattice point in the root lattice.
     For this, we impose the incidences demanded by the set of rings (a point should be off the hyperplane indexed j iff j in rings),
     and use a MIP solver to make that point an integral combination of the root vectors.

     Let the root vectors be v_i. We need to solve the MIP
     
        min sum_j lambda_j
        s.t.
        <v_i, x>   = 0     for i notin rings
        <v_i, x>  >= 1     for i    in rings
        x = sum_j lambda_j v_j
        lambda_j >= 0
        lambda_j in Z

     We rewrite this in terms of the lambda_j:

        min sum_j lambda_j
        s.t.
        sum_j lambda_j <v_i,v_j>   = 0    for i notin rings
        sum_j lambda_j <v_i,v_j>  >= 1    for i    in rings
        lambda_j >= 0
        lambda_j in Z

    Also, TOSimplex needs explicit upper and lower bounds on the variables.
    Here, we use 0 <= lambda_j <= 1 000 000.
   */

   const SparseMatrix<E> Gramian(R * T(R));
   const Matrix<E>
        eqs(    zero_vector<E>(d - rings.size()) | Gramian.minor(~rings,All) ),
      ineqs(( - ones_vector<E>(    rings.size()) | Gramian.minor( rings,All) ) /   // real inequalities
            ( zero_vector<E>(Gramian.cols())                     |   unit_matrix<E>(Gramian.cols()) ) /   // lambda >= 0
            ( same_element_vector<E>(E(1000000), Gramian.cols()) | - unit_matrix<E>(Gramian.cols()) ) );  // lambda <= 1 000 000
   
   const Set<Int> integer_variables(entire(sequence(0,d+1)));

   const auto milp_sol = solve_MILP(ineqs, eqs, ones_vector<E>(d+1), integer_variables, false); // false == minimize

   if (milp_sol.status == LP_status::unbounded)
      throw std::runtime_error("wythoff::find_integral_initial_point: unbounded polyhedron or computation failed");

   if (milp_sol.status == LP_status::infeasible)
      throw std::runtime_error("wythoff::find_integral_initial_point: infeasible problem");

   Vector<Integer> lambdas (ineqs.cols(), entire(milp_sol.solution));
   if (is_zero(lambdas[0]))
      throw std::runtime_error("wythoff::find_integral_initial_point: unexpected infinite solution");

   lambdas.dehomogenize();
   Vector<E> solution(lambdas.slice(range_from(1)) * R);
   solution[0] = 1;
   return solution;
}
   
template <typename E>
SparseMatrix<E> orbit (const SparseVector<E>& p0, 
                       const SparseMatrix<E>& simple_roots, 
                       hash_map<SparseVector<E>, Int>& index_of)
{
   index_of[p0] = 0;
   Int new_point_index = 1;
   const Int n_simple_roots = simple_roots.rows();

   typedef std::pair<Int, SparseVector<E>> key_type; // (root_index, point) not yet reflected
   std::list<key_type> point_queue; 
   for (Int i = 0; i < n_simple_roots; ++i) 
      point_queue.push_back(key_type(i, p0)); 

   while (!point_queue.empty()) {
      const key_type a(point_queue.front());  point_queue.pop_front();
      const Int root_index = a.first;
      const SparseVector<E> new_point(reflect(a.second, simple_roots[root_index]));
      if (!index_of.exists(new_point)) {
         for (Int i = 0; i < n_simple_roots; ++i)
            if (i != root_index)
               point_queue.push_back(key_type(i, new_point));
         index_of[new_point] = new_point_index++;
      }
   }

   SparseMatrix<E> V(index_of.size(), index_of.begin()->first.dim());
   for (const auto& io: index_of)
      V[io.second] = io.first;
   return V;
}

} // end anonymous namespace

template <typename E>
void wythoff(const std::string& type, 
             const Set<Int>& rings, 
             const SparseMatrix<E>& R,
             const bool lattice,
             SparseMatrix<E>& V, 
             Array<Array<Int>>& generators)
{
   const Int n_simple_roots = R.rows();
   if (accumulate(rings, operations::max()) >= n_simple_roots)
      throw std::runtime_error("Set specifies non-existing rows of the root matrix");
   const Int d = R.cols()-1;

   const SparseVector<E> p0 = lattice
      ? find_integral_initial_point(rings, R, d, type[0])
      : find_regular_initial_point (rings, R, d, type[0]);
   if (p0 == unit_vector<E>(d+1, 0))
      throw std::runtime_error("Could not calculate a valid initial point");

   hash_map<SparseVector<E>, Int> index_of;
   V = orbit(p0, R, index_of);
   const Int n_points = V.rows();
   generators = Array<Array<Int>>(n_simple_roots);
   for (Int i = 0; i < n_simple_roots; ++i) {
      Array<Int> g(n_points);
      for (Int j = 0; j < n_points; ++j) 
         g[j] = index_of[reflect(V.row(j), R.row(i))];
      generators[i] = g;
   }
}

BigObject wythoff_dispatcher(const std::string& type,
                             const Set<Int>& rings,
                             const bool lattice = false)
{
   if (type.size() < 2)
      throw std::runtime_error("Type needs single letter followed by rank.");

   const char t(type[0]);
   Int n;
   std::istringstream is (type.substr(1));
   is >> n;

   Array<Array<Int>> generators;

   SparseMatrix<Rational> RM;
   SparseMatrix<QE> EM;

   bool needs_QE = false;

   if ((t >= 'A' && t <= 'G') || (t >= 'a' && t <= 'g')) {
      if (t == 'A' || t == 'a') {
         if (n >= 1)
            wythoff<Rational>(type, rings, simple_roots_type_A(n), lattice, RM, generators);
         else
            throw std::runtime_error("Coxeter group of type A requires rank >= 1.");
      } else if (t == 'B' || t == 'b') {
         if (n >= 2) {
            if (rings.size() == 1 ||
                incl(rings, sequence(0, n-1)) <= 0 ||
                lattice) {
               wythoff<Rational>(type, rings, simple_roots_type_B(n), lattice, RM, generators);
            } else {
               wythoff<QE>(type, rings, SparseMatrix<QE>(simple_roots_type_B(n)), lattice, EM, generators);
               needs_QE = true;
            }
         } else
            throw std::runtime_error("Coxeter group of type B requires rank >= 2.");
      } else if (t == 'C' || t == 'c') {
         if (n >= 2) {
            if (rings.size() == 1 ||
                incl(rings, sequence(0, n-1)) <= 0 ||
                lattice) {
               wythoff<Rational>(type, rings, simple_roots_type_C(n), lattice, RM, generators);
            } else {
               wythoff<QE>(type, rings, SparseMatrix<QE>(simple_roots_type_C(n)), lattice, EM, generators);
               needs_QE = true;
            }
         } else
            throw std::runtime_error("Coxeter group of type C requires rank >= 2.");
      } else if (t == 'D' || t == 'd') {
         if (n >= 3)
            wythoff<Rational>(type, rings, simple_roots_type_D(n), lattice, RM, generators);
         else 
            throw std::runtime_error("Coxeter group of type D requires rank >= 3.");
      } else if (t == 'E' || t == 'e') {
         if (n==6) {
            needs_QE = true;
            wythoff<QE>(type, rings, simple_roots_type_E6(), lattice, EM, generators);
          } else if (n==7){
            needs_QE = true;
            wythoff<QE>(type, rings, simple_roots_type_E7(), lattice, EM, generators);
          } else if (n==8)
            wythoff<Rational>(type, rings, simple_roots_type_E8(), lattice, RM, generators);
         else throw std::runtime_error("Coxeter group of type E requires rank 6, 7 or 8.");
      }
      else if (t == 'F' || t == 'f') {
         if (n == 4) {
            if (rings.size() == 1 ||
                incl(rings, sequence(0, 2)) <= 0 ||
                incl(rings, sequence(2, 2)) <= 0 ||
                lattice) {
               wythoff<Rational>(type, rings, simple_roots_type_F4(), lattice, RM, generators);
            } else {
               wythoff<QE>(type, rings, SparseMatrix<QE>(simple_roots_type_F4()), lattice, EM, generators);
               needs_QE = true;
            }
         } else throw std::runtime_error("Coxeter group of type F requires rank == 4.");
      } else if (t == 'G' || t == 'g') {
         if (n == 2) {
            wythoff<Rational>(type, rings, simple_roots_type_G2(), lattice, RM, generators);
         } else throw std::runtime_error("Coxeter group of type G requires rank == 2.");
      } else
         throw std::runtime_error("Did not recognize crystallographic Coxeter group.");

   } else if (t == 'H' || t == 'h') {
      needs_QE = true;

      switch(n) {
      case 3: {
         wythoff<QE>(type, rings, simple_roots_type_H3(), lattice, EM, generators);
         break;
      }
      case 4: {
         wythoff<QE>(type, rings, simple_roots_type_H4(), lattice, EM, generators);
         break;
      }
      default:
         throw std::runtime_error("Coxeter group of type H requires rank 3 or 4.");
      }

   } else
      throw std::runtime_error("Did not recognize the Coxeter group.");

   BigObject p;

   if (needs_QE) {
      p = BigObject("Polytope<QuadraticExtension>");
      p.take("VERTICES") << EM;
      p.take("N_VERTICES") << EM.rows();
   } else {
      p = BigObject("Polytope<Rational>");
      p.take("VERTICES") << RM;
      p.take("N_VERTICES") << RM.rows();
   }
   p.set_description() << "Wythoff polytope of type " << type << " with rings " << rings << endl;

   const Int ambient_dim = (t == 'A' || t == 'a') ? n+2 : n+1;

   p.take("CONE_AMBIENT_DIM") << ambient_dim;
   p.take("CONE_DIM") << n+1;
   p.take("AFFINE_HULL") << ((t == 'A' || t == 'a') ? vector2row(-1 | ones_vector<Rational>(n+1)) : Matrix<Rational>(0, ambient_dim));
   p.take("LINEALITY_SPACE") << Matrix<Rational>(0, ambient_dim);
   p.take("BOUNDED") << true;
   p.take("FEASIBLE") << true;
   p.take("POINTED") << true;
   p.take("CENTERED") << true;

   BigObject a("group::PermutationAction", "GENERATORS", generators);
   p.take("GROUP.VERTICES_ACTION") << a;

   return p;
}

// wythoff_dispatcher("A3", Set<Int>(0)) gives tetrahedron, but embedded in 4-space
// That's why we explictly give a full-dimensional representation here.

template <typename Scalar>
BigObject tetrahedron()
{
   Matrix<Scalar> RM(same_element_matrix(1,4,4));
   RM(0,2) = RM(0,3) = RM(1,1) = RM(1,3) = RM(2,1) = RM(2,2) = -1;

   BigObject p("Polytope", mlist<Scalar>(),
               "VERTICES", RM,
               "N_VERTICES", 4,
               "LINEALITY_SPACE", Matrix<Scalar>(0, 4),
               "CONE_AMBIENT_DIM", 4,
               "CONE_DIM", 4,
               "BOUNDED", true,
               "FEASIBLE", true,
               "POINTED", true,
               "CENTERED", true);
   p.set_description() << "regular tetrahedron" << endl;
   return p;
}

// wythoff_dispatcher("B3", Set<Int>(0)) gives octahedron
//
BigObject truncated_cube()
{
   BigObject p(wythoff_dispatcher("B3", {1, 2}));
   p.set_description("= truncated cube",true);
   return p;
}

BigObject cuboctahedron()
{
   BigObject p(wythoff_dispatcher("B3", {1}));
   p.set_description("= cuboctahedron",true);
   return p;
}

// wythoff_dispatcher("B3", Set<Int>(2)) gives cube

BigObject truncated_octahedron()
{
   BigObject p(wythoff_dispatcher("B3", {0, 1}));
   p.set_description("= truncated octahedron",true);
   return p;
}

BigObject truncated_cuboctahedron()
{
   BigObject p(wythoff_dispatcher("B3", {0, 1, 2}));
   p.set_description("= truncated cuboctahedron",true);
   return p;
}

BigObject rhombicuboctahedron()
{
   BigObject p(wythoff_dispatcher("B3", {0, 2}));
   p.set_description("= rhombicuboctahedron",true);
   return p;
}

BigObject regular_24_cell()
{
   BigObject p(wythoff_dispatcher("F4", {0}));
   p.set_description("= regular 24-cell",true);
   return p;
}

BigObject regular_120_cell()
{
   BigObject p(wythoff_dispatcher("H4", {0}));
   p.set_description("= regular 120-cell",true);
   return p;
}

BigObject regular_600_cell()
{
   BigObject p(wythoff_dispatcher("H4", {3}));
   p.set_description("= regular 600-cell",true);
   return p;
}

BigObject dodecahedron()
{
   BigObject p(wythoff_dispatcher("H3", {0}));
   p.set_description("= regular dodecahedron",true);
   return p;
}

BigObject icosidodecahedron()
{
   BigObject p(wythoff_dispatcher("H3", {1}));
   p.set_description("= icosidodecahedron",true);
   return p;
}

BigObject icosahedron()
{
   BigObject p(wythoff_dispatcher("H3", {2}));
   p.set_description("= regular icosahedron",true);
   return p;
}

BigObject truncated_dodecahedron()
{
   BigObject p(wythoff_dispatcher("H3", {0, 1}));
   p.set_description("= truncated dodecahedron",true);
   return p;
}

BigObject rhombicosidodecahedron()
{
   BigObject p(wythoff_dispatcher("H3", {0, 2}));
   p.set_description("= rhombicosidodecahedron",true);
   return p;
}

BigObject truncated_icosahedron()
{
   BigObject p(wythoff_dispatcher("H3", {1, 2}));
   p.set_description("= truncated icosahedron",true);
   return p;
}

BigObject truncated_icosidodecahedron()
{
   BigObject p(wythoff_dispatcher("H3", {0, 1, 2}));
   p.set_description("= truncated icosidodecahedron",true);
   return p;
}


Function4perl(&wythoff_dispatcher, "wythoff_dispatcher($ Set<Int>; $=1)");

UserFunctionTemplate4perl("# @category Producing regular polytopes and their generalizations"
                          "# Create regular tetrahedron.  A Platonic solid."
                          "# @return Polytope",
                          "tetrahedron<Scalar=Rational>()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create truncated cube.  An Archimedean solid."
                  "# @return Polytope",
                  &truncated_cube, "truncated_cube()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create cuboctahedron.  An Archimedean solid."
                  "# @return Polytope",
                  &cuboctahedron, "cuboctahedron()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create truncated cuboctahedron.  An Archimedean solid."
                  "# This is actually a misnomer.  The actual truncation of a cuboctahedron"
                  "# is normally equivalent to this construction, "
                  "# but has two different edge lengths. This construction has regular 2-faces."
                  "# @return Polytope",

                  &truncated_cuboctahedron, "truncated_cuboctahedron()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create rhombicuboctahedron.  An Archimedean solid."
                  "# @return Polytope",
                  &rhombicuboctahedron, "rhombicuboctahedron()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create truncated octahedron.  An Archimedean solid."
                  "# Also known as the 3-permutahedron."
                  "# @return Polytope",
                  &truncated_octahedron, "truncated_octahedron()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create regular 24-cell."
                  "# @return Polytope",
                  &regular_24_cell, "regular_24_cell()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create exact regular 120-cell in Q(sqrt{5})."
                  "# @return Polytope",
                  &regular_120_cell, "regular_120_cell()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create exact regular 600-cell in Q(sqrt{5})."
                  "# @return Polytope",
                  &regular_600_cell, "regular_600_cell()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create exact regular dodecahedron in Q(sqrt{5}).  A Platonic solid."
                  "# @return Polytope",
                  &dodecahedron, "dodecahedron()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create exact icosidodecahedron in Q(sqrt{5}).  An Archimedean solid."
                  "# @return Polytope",
                  &icosidodecahedron, "icosidodecahedron()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create exact regular icosahedron in Q(sqrt{5}).  A Platonic solid."
                  "# @return Polytope",
                  &icosahedron, "icosahedron()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create exact truncated dodecahedron in Q(sqrt{5}).  An Archimedean solid."
                  "# @return Polytope",
                  &truncated_dodecahedron, "truncated_dodecahedron()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create exact rhombicosidodecahedron in Q(sqrt{5}).  An Archimedean solid."
                  "# @return Polytope",
                  &rhombicosidodecahedron, "rhombicosidodecahedron()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create exact truncated icosahedron in Q(sqrt{5}).  An Archimedean solid."
                  "# Also known as the soccer ball."
                  "# @return Polytope",
                  &truncated_icosahedron, "truncated_icosahedron()");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create exact truncated icosidodecahedron in Q(sqrt{5}).  An Archimedean solid."
                  "# @return Polytope",
                  &truncated_icosidodecahedron, "truncated_icosidodecahedron()");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
