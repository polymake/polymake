/* Copyright (c) 1997-2018
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

#include "polymake/polytope/hasse_diagram.h"

namespace polymake { namespace polytope {

using graph::lattice::BasicDecorator;
using graph::lattice::BasicClosureOperator;
using graph::lattice::TrivialCut;
using graph::lattice::SetAvoidingCut;
using graph::lattice::RankCut;
using graph::lattice::CutAnd;

perl::Object hasse_diagram(const IncidenceMatrix<>& VIF, const int cone_dim)
{
   const bool is_dual = VIF.rows() < VIF.cols();
   const int total = (is_dual? VIF.rows() : VIF.cols());

   BasicClosureOperator<> cop = is_dual?
      BasicClosureOperator<>(total,T(VIF)) : BasicClosureOperator<>(total,VIF);
   TrivialCut<BasicDecoration> cut;
   BasicDecorator<> dec = is_dual?
      BasicDecorator<>(VIF.cols(), cone_dim, Set<int>()) :
      BasicDecorator<>(0, Set<int>());

   Lattice<BasicDecoration, Sequential> init_lattice;
   Lattice<BasicDecoration, Sequential> result = (is_dual?
          graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(cop, cut, dec, 0, graph::lattice_builder::Dual(), init_lattice) :
          graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(cop, cut, dec, 0, graph::lattice_builder::Primal(), init_lattice));
   sort_vertices_and_facets(result, VIF);
   return result.makeObject();
}


Lattice<BasicDecoration, Nonsequential> bounded_hasse_diagram_computation(
  const IncidenceMatrix<>& VIF,
  const Set<int>& far_face,
  const int boundary_dim)
{
   using bounded_cut_type = SetAvoidingCut<BasicDecoration>;
   using rank_cut_type = RankCut<BasicDecoration, graph::lattice::RankCutType::LesserEqual>;
   int total = VIF.cols();

   BasicClosureOperator<> cop(total, VIF);
   bounded_cut_type bounded_cut(far_face);
   rank_cut_type rank_cut(boundary_dim);
   CutAnd< bounded_cut_type, rank_cut_type> bounded_and_rank_cut(bounded_cut, rank_cut);
   BasicDecorator<> dec(0, scalar2set(-1));

   Lattice<BasicDecoration, Nonsequential> init_lattice;
   Lattice<BasicDecoration, Nonsequential> result = (boundary_dim == -1) ?
      graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(
                  cop, bounded_cut,dec, 1, graph::lattice_builder::Primal(), init_lattice) :
      graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(
                  cop, bounded_and_rank_cut,dec, 1, graph::lattice_builder::Primal(), init_lattice);
   return result;
}

perl::Object bounded_hasse_diagram(const IncidenceMatrix<>& VIF,
                                   const Set<int>& far_face,
                                   const int boundary_dim)
{
  // Special check for empty polytope
  if (std::min(VIF.rows(), VIF.cols()) == 0) {
    return hasse_diagram(VIF, 0);
  }
  return bounded_hasse_diagram_computation(VIF, far_face, boundary_dim).makeObject();
}

perl::Object rank_bounded_hasse_diagram(const IncidenceMatrix<>& VIF,
                                        int cone_dim, int boundary_dim, bool from_above)
{
   bool is_dual = from_above;
   int total = (is_dual? VIF.rows() : VIF.cols());
   BasicClosureOperator<> cop = is_dual?
      BasicClosureOperator<>(total,T(VIF)) : BasicClosureOperator<>(total,VIF);
   auto cut_above = RankCut<BasicDecoration,graph::lattice::RankCutType::GreaterEqual>(boundary_dim);
   auto cut_below = RankCut<BasicDecoration,graph::lattice::RankCutType::LesserEqual>(boundary_dim);
   BasicDecorator<> dec = is_dual?
      BasicDecorator<>(VIF.cols(), cone_dim, scalar2set(-1)) :
      BasicDecorator<>(0, scalar2set(-1));

   Lattice<BasicDecoration, Sequential> init_lattice;
   Lattice<BasicDecoration, Sequential> result = from_above?
      graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(
                  cop, cut_above, dec, 1, graph::lattice_builder::Dual(), init_lattice) :
      graph::lattice_builder::compute_lattice_from_closure<BasicDecoration>(
                  cop, cut_below, dec, 1, graph::lattice_builder::Primal(), init_lattice);
   return result.makeObject();
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
