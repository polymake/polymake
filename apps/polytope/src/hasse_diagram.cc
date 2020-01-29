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

#include "polymake/polytope/hasse_diagram.h"

namespace polymake { namespace polytope {

using graph::lattice::BasicDecorator;
using graph::lattice::BasicClosureOperator;
using graph::lattice::TrivialCut;
using graph::lattice::SetAvoidingCut;
using graph::lattice::RankCut;
using graph::lattice::CutAnd;

BigObject hasse_diagram(const IncidenceMatrix<>& VIF, const Int cone_dim)
{
   const bool is_dual = VIF.rows() < VIF.cols();
   if (is_dual) {
      const Int total = VIF.rows();
      BasicClosureOperator<> cop(total, T(VIF));
      TrivialCut<BasicDecoration> cut;
      BasicDecorator<> dec(VIF.cols(), cone_dim, Set<Int>());

      Lattice<BasicDecoration, Sequential> init_lattice;
      Lattice<BasicDecoration, Sequential> result(graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(
         cop, cut, dec, 0, graph::lattice_builder::Dual(), init_lattice));
      sort_vertices_and_facets(result, VIF);
      return static_cast<BigObject>(result);
   } else {
      const Int total = VIF.cols();
      BasicClosureOperator<> cop(total, VIF);
      TrivialCut<BasicDecoration> cut;
      BasicDecorator<> dec(0, Set<Int>());

      Lattice<BasicDecoration, Sequential> init_lattice;
      Lattice<BasicDecoration, Sequential> result(graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(
         cop, cut, dec, 0, graph::lattice_builder::Primal(), init_lattice));
      sort_vertices_and_facets(result, VIF);
      return static_cast<BigObject>(result);
   }
}


Lattice<BasicDecoration, Nonsequential>
bounded_hasse_diagram_computation(const IncidenceMatrix<>& VIF, const Set<Int>& far_face, const Int boundary_dim)
{
   using bounded_cut_type = SetAvoidingCut<BasicDecoration>;
   using rank_cut_type = RankCut<BasicDecoration, graph::lattice::RankCutType::LesserEqual>;
   Int total = VIF.cols();

   BasicClosureOperator<> cop(total, VIF);
   bounded_cut_type bounded_cut(far_face);
   rank_cut_type rank_cut(boundary_dim);
   CutAnd< bounded_cut_type, rank_cut_type> bounded_and_rank_cut(bounded_cut, rank_cut);
   BasicDecorator<> dec(0, scalar2set(-1));

   Lattice<BasicDecoration, Nonsequential> init_lattice;
   if (boundary_dim == -1) {
      return graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(
               cop, bounded_cut,dec, 1, graph::lattice_builder::Primal(), init_lattice);
   } else {
      return graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(
               cop, bounded_and_rank_cut,dec, 1, graph::lattice_builder::Primal(), init_lattice);
   }
}

BigObject bounded_hasse_diagram(const IncidenceMatrix<>& VIF,
                                   const Set<Int>& far_face,
                                   const Int boundary_dim)
{
  // Special check for empty polytope
  if (std::min(VIF.rows(), VIF.cols()) == 0) {
    return hasse_diagram(VIF, 0);
  }
  return static_cast<BigObject>(bounded_hasse_diagram_computation(VIF, far_face, boundary_dim));
}

BigObject rank_bounded_hasse_diagram(const IncidenceMatrix<>& VIF,
                                        Int cone_dim, Int boundary_dim, bool from_above)
{
   if (from_above) {
      const Int total = VIF.rows();
      BasicClosureOperator<> cop(total, T(VIF));
      BasicDecorator<> dec(VIF.cols(), cone_dim, scalar2set(-1));
      const auto cut_above = RankCut<BasicDecoration,graph::lattice::RankCutType::GreaterEqual>(boundary_dim);

      Lattice<BasicDecoration, Sequential> init_lattice;
      return static_cast<BigObject>(graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(
                cop, cut_above, dec, 1, graph::lattice_builder::Dual(), init_lattice));
   } else {
      const Int total = VIF.cols();
      BasicClosureOperator<> cop(total, VIF);
      BasicDecorator<> dec(0, scalar2set(-1));
      const auto cut_below = RankCut<BasicDecoration,graph::lattice::RankCutType::LesserEqual>(boundary_dim);

      Lattice<BasicDecoration, Sequential> init_lattice;
      return static_cast<BigObject>(graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(
                cop, cut_below, dec, 1, graph::lattice_builder::Primal(), init_lattice));
   }
}


FunctionTemplate4perl("hasse_diagram(IncidenceMatrix, $)");
FunctionTemplate4perl("bounded_hasse_diagram(IncidenceMatrix, Set<Int>; $=-1)");
FunctionTemplate4perl("lower_hasse_diagram(IncidenceMatrix, $)");
FunctionTemplate4perl("upper_hasse_diagram(IncidenceMatrix, $,$)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
