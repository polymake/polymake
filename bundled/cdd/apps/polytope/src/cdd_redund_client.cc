/* Copyright (c) 1997-2015
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

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
#define GMPRATIONAL
#include "polymake/polytope/cdd_interface.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

// function only computes rays, object must be a polytope or known to be pointed
// input_rays/input_lineality remain untouched
template<typename Scalar>
void cdd_eliminate_redundant_points(perl::Object p)
{
   cdd_interface::solver<Scalar> solver;
   Matrix<Scalar> P=p.give("INPUT_RAYS");

   const bool isCone = !p.isa("Polytope");
   if (isCone && P.cols())
      P = zero_vector<Scalar>()|P;
   const typename cdd_interface::solver<Scalar>::non_redundant non_red=solver.find_vertices_among_points(P);

   if ( isCone ) {
      p.take("RAYS") << P.minor(non_red.first,~scalar2set(0));
      p.take("RAY_SEPARATORS") << non_red.second.minor(All,~scalar2set(0));
   } else {
      p.take("RAYS") << P.minor(non_red.first,All);
      p.take("RAY_SEPARATORS") << non_red.second;
   }
   Matrix<Scalar> empty_lin_space(0,P.cols());	
   p.take("LINEALITY_SPACE") << empty_lin_space;	
}

// compute rays separators from rays
// FIXME: assumption: normal returned by cdd is contained in affine hull of rays. true?
template<typename Scalar>
void cdd_vertex_normals(perl::Object p)
{
   cdd_interface::solver<Scalar> solver;
   Matrix<Scalar> P=p.give("RAYS");
   const bool isCone = !p.isa("Polytope");
   if (isCone && P.cols())
      P = zero_vector<Scalar>()|P;
   const typename cdd_interface::solver<Scalar>::non_redundant non_red=solver.find_vertices_among_points(P);//   ( p.type() == pobjtype) );
   if ( isCone )
      p.take("RAY_SEPARATORS") << non_red.second.minor(All,~scalar2set(0));
   else
      p.take("RAY_SEPARATORS") << non_red.second;
}

// remove redundancies from INPUT_RAYS/INPUT_LINEALITY
// move implicit linealities from INPUT_RAYS to LINEALITY_SPACE
template <typename Scalar>
void cdd_canonicalize(perl::Object p, bool primal = true)
{
   cdd_interface::solver<Scalar> solver;
   Matrix<Scalar> P,L;
   if ( primal ) {
      p.give("INEQUALITIES") >> P;
      p.lookup("EQUATIONS") >> L;
   } else {
      p.give("INPUT_RAYS") >> P;
      p.lookup("INPUT_LINEALITY") >> L;
   }

   const bool isCone = !p.isa("Polytope");
   if (isCone) {
      if (P.cols())
         P = zero_vector<Scalar>()|P;
      if (L.cols())
         L = zero_vector<Scalar>()|L;
   }

   if (P.cols() != L.cols() &&
         P.cols() && L.cols()) {
      throw std::runtime_error("cdd_canonicalize - dimension mismatch between input properties");
   }

   // FIXME: better solution: do it similar to facets_and_ah() ?
   const Matrix<Scalar> PL = L.rows() ? Matrix<Scalar>(P/L) : P;

   if ( PL.rows() ) {
      const typename cdd_interface::solver<Scalar>::non_redundant_canonical non_red=solver.canonicalize(P, L, primal);
      if ( primal ) {
         if ( isCone ) {
            p.take("FACETS") << PL.minor(non_red.first,~scalar2set(0));
            p.take("LINEAR_SPAN") << PL.minor(non_red.second,~scalar2set(0));
         } else {
            // cdd doesn't handle the case of an empty polytope in the same way as polymake
            // so first check for an infeasible system
            if ( is_zero(null_space(PL.minor(non_red.second,All)).col(0)) ) {
               p.take("AFFINE_HULL") << PL.minor(basis_rows(PL),All);
               p.take("FACETS") << Matrix<Scalar>(0,PL.cols());
            } else {
               p.take("FACETS") << PL.minor(non_red.first,All);
               p.take("AFFINE_HULL") << PL.minor(non_red.second,All);
            }
         }
      } else {
         if ( isCone ) {
            p.take("RAYS") << PL.minor(non_red.first,~scalar2set(0));
            p.take("LINEALITY_SPACE") << PL.minor(non_red.second,~scalar2set(0));
         } else {
            p.take("RAYS") << PL.minor(non_red.first,All);
            p.take("LINEALITY_SPACE") << PL.minor(non_red.second,All);
         }
         p.take("POINTED") << non_red.second.empty();
      }
   } else {
      p.take(primal ? Str("FACETS") : Str("RAYS")) << Matrix<Scalar>();
      p.take(primal ? Str("LINEAR_SPAN") : Str("LINEALITY_SPACE")) << Matrix<Scalar>();
   }
}

// find implicit linealities in INPUT_RAYS and write LINEALITY_SPACE
template<typename Scalar>
void cdd_canonicalize_lineality(perl::Object p, bool primal = true)
{
   cdd_interface::solver<Scalar> solver;
   Matrix<Scalar> P,L;
   if ( primal ) {
      p.give("INEQUALITIES") >> P;
      p.lookup("EQUATIONS") >> L;
   } else {
      p.give("INPUT_RAYS") >> P;
      p.lookup("INPUT_LINEALITY") >> L;
   }

   const bool isCone = !p.isa("Polytope");
   if (isCone) {
      if (P.cols())
         P = zero_vector<Scalar>()|P;
      if (L.cols())
         L = zero_vector<Scalar>()|L;
   }

   if (P.cols() != L.cols() &&
         P.cols() && L.cols()) {
      throw std::runtime_error("cdd_canonicalize_lineality - dimension mismatch between input properties");
   }

   // FIXME: better solution: do it similar to facets_and_ah() ?
   const Matrix<Scalar> PL = P/L;

   Bitset lineality=solver.canonicalize_lineality(P, L, primal);
   if ( primal ) {
      if ( isCone ) {
         p.take("LINEAR_SPAN") << PL.minor(lineality,~scalar2set(0));
      } else {
         // cdd doesn't handle the case of an empty polytope in the same way as polymake
         // so first check for an infeasible system
         if ( is_zero(null_space(PL.minor(lineality,All)).col(0)) )
            p.take("AFFINE_HULL") << PL.minor(basis_rows(PL),All);
         else
            p.take("AFFINE_HULL") << PL.minor(lineality,All);
      }
   } else {
      if ( isCone )
         p.take("LINEALITY_SPACE") << PL.minor(lineality,~scalar2set(0));
      else
         p.take("LINEALITY_SPACE") << PL.minor(lineality,All);
      p.take("POINTED") << lineality.empty();
   }
}


FunctionTemplate4perl("cdd_eliminate_redundant_points<Scalar>(Cone<Scalar>) : void");
FunctionTemplate4perl("cdd_canonicalize<Scalar>(Cone<Scalar>;$=1) : void");
FunctionTemplate4perl("cdd_vertex_normals<Scalar>(Cone<Scalar>) : void");
FunctionTemplate4perl("cdd_canonicalize_lineality<Scalar>(Cone<Scalar>;$=1) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
