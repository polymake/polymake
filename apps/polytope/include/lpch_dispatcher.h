/* Copyright (c) 1997-2016
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

#ifndef POLYMAKE_POLYTOPE_LPCH_DISPATCHER_H
#define POLYMAKE_POLYTOPE_LPCH_DISPATCHER_H

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/internal/linalg_exceptions.h"

namespace polymake { namespace polytope {

class not_pointed : public linalg_error {
protected:
   int lin_dim;
public:
   not_pointed(int dim_arg)
      : linalg_error("polyhedron not pointed"),
        lin_dim(dim_arg) {}

   int lineality_dim() const { return lin_dim; }
};

class unbounded : public linalg_error {
public:
   unbounded() : linalg_error("unbounded linear program") {}
};

class dually_infeasible : public linalg_error {
public:
   dually_infeasible() : linalg_error("dual linear program infeasible") {}
};

template <typename Solver>
void ch_primal(perl::Object& p, Solver& solver)
{
   typedef typename Solver::coord_type coord_type;
   Matrix<coord_type> Points=p.give("RAYS | INPUT_RAYS"),
                      Lineality=p.lookup("LINEALITY_SPACE | INPUT_LINEALITY");
   const bool isCone = !p.isa("Polytope");
   if (isCone) {
      Points = zero_vector<coord_type>()|Points;
      if (Lineality.cols())
         Lineality = zero_vector<coord_type>()|Lineality;
   }

   if (Points.cols() != Lineality.cols() &&
       Points.cols() && Lineality.cols()) {
      throw std::runtime_error("ch_primal - dimension mismatch between Points and Lineality");
   }

   typename Solver::matrix_pair F=solver.enumerate_facets(Points, Lineality, isCone, false);
   if (isCone) {
      if (F.first.cols() > 1)
         p.take("FACETS") << F.first.minor(All, range(1, F.first.cols()-1));
      else
         p.take("FACETS") << Matrix<coord_type>();
      if (F.second.cols() > 1)
         p.take("LINEAR_SPAN") << F.second.minor(All, range(1, F.second.cols()-1));
      else
         p.take("LINEAR_SPAN") << Matrix<coord_type>();
   } else {
      p.take("FACETS") << F.first;
      p.take("LINEAR_SPAN") << F.second;
   }
}

// only lrs
template <typename Solver>
void count_facets(perl::Object& p, Solver& solver)
{
   typedef typename Solver::coord_type coord_type;
   Matrix<coord_type> Points=p.give("RAYS | INPUT_RAYS"),
                      Lineality=p.lookup("LINEALITY_SPACE | INPUT_LINEALITY");
   const bool isCone = !p.isa("Polytope");
   if (isCone) {
      Points = zero_vector<coord_type>()|Points;
      if (Lineality.cols())
         Lineality = zero_vector<coord_type>()|Lineality;
   }

   if (Points.cols() != Lineality.cols() &&
       Points.cols() && Lineality.cols()) {
      throw std::runtime_error("count_facets - dimension mismatch between Points and Lineality");
   }

   p.take("N_FACETS") << solver.count_facets(Points,Lineality,isCone);
}

template <typename Solver>
void ch_dual(perl::Object& p, Solver& solver)
{
   typedef typename Solver::coord_type coord_type;
   Matrix<coord_type> H=p.give("FACETS | INEQUALITIES"),
                      EQ=p.lookup("LINEAR_SPAN | EQUATIONS");

   // recall that all output properties lacking after the rule completion automatically get undefined values
   if (H.rows() || EQ.rows()) {
      try {
         // TODO: pass this as an input flag, providing overridable rules for Cone and Polytope
         const bool isCone = !p.isa("Polytope");
         if (isCone) {
            H = zero_vector<coord_type>()|H;
            if (EQ.cols())
               EQ = zero_vector<coord_type>()|EQ;
         }

         if (H.cols() != EQ.cols() &&
             H.cols() && EQ.cols()) {
            throw std::runtime_error("ch_dual - dimension mismatch between Inequalities and Equations");
         }

         typename Solver::matrix_pair VL=solver.enumerate_vertices(H, EQ, isCone, true);
         if (isCone) {
            if (VL.first.cols() > 1)
               p.take("RAYS") << VL.first.minor(All, range(1, VL.first.cols()-1));
            else
               p.take("RAYS") << Matrix<coord_type>();
            if (VL.second.cols() > 1)
               p.take("LINEALITY_SPACE") << VL.second.minor(All, range(1, VL.second.cols()-1));
            else
               p.take("LINEALITY_SPACE") << Matrix<coord_type>();
         } else {
            p.take("RAYS") << VL.first;
            p.take("LINEALITY_SPACE") << VL.second;
         }
         p.take("POINTED") << (VL.second.rows()==0);
         // TODO: enable this for Polytope
         // p.take("FEASIBLE") << false;
         p.take("LINEALITY_DIM") << VL.second.rows();
         return;
      }
      catch (infeasible) { }
   }
   const int d=H.cols();
   p.take("RAYS") << Matrix<coord_type>(0, d);
   p.take("LINEALITY_SPACE") << Matrix<coord_type>(0, d);
   p.take("LINEALITY_DIM") << 0;
   p.take("POINTED") << true;
   // TODO: enable this for Polytope
   // p.take("FEASIBLE") << false;
}

// only lrs
// FIXME maybe we should separate cone/polytope in these functions
template <typename Solver>
void count_vertices(perl::Object& p, Solver& solver, bool only_bounded=false)
{
   typedef typename Solver::coord_type coord_type;
   Matrix<coord_type> H=p.give("FACETS | INEQUALITIES"),
                      EQ=p.lookup("LINEAR_SPAN | EQUATIONS");
   const bool isCone = !p.isa("Polytope");
   if ( isCone && only_bounded )
      throw std::runtime_error("a cone has no bounded vertices");
   if (H.rows() || EQ.rows()) {
      try {
         if (isCone) {
            H = zero_vector<coord_type>()|H;
            if (EQ.cols())
               EQ = zero_vector<coord_type>()|EQ;
         }

         if (H.cols() != EQ.cols() &&
             H.cols() && EQ.cols()) {
            throw std::runtime_error("count_vertices - dimension mismatch between Inequalities and Equations");
         }

         typename Solver::vertex_count count=solver.count_vertices(H,EQ,only_bounded);
         if ( isCone ) {
            // lrs counts the origin
            // we have to substract this in our representation
            p.take("N_RAYS") << count.verts.first-1;
         } else {
            if (!only_bounded) p.take("N_VERTICES") << count.verts.first;
            p.take("N_BOUNDED_VERTICES") << count.verts.second;
         }
         p.take("POINTED") << (count.lin==0);
         p.take("LINEALITY_DIM") << count.lin;
      }
      catch (infeasible) {
         throw std::runtime_error("illegal empty polytope");
      }
   } else {
      p.take("N_RAYS") << 0;
      p.take("LINEALITY_DIM") << 0;
      p.take("POINTED") << 1;
   }
}

} }

#endif // POLYMAKE_POLYTOPE_LPCH_DISPATCHER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
