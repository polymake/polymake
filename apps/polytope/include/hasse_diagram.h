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

#ifndef POLYMAKE_POLYTOPE_HASSE_DIAGRAM_H
#define POLYMAKE_POLYTOPE_HASSE_DIAGRAM_H

#include "polymake/client.h"
#include "polymake/graph/Decoration.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/BasicLatticeTypes.h"
#include "polymake/graph/lattice_builder.h"
#include "polymake/graph/LatticePermutation.h"

namespace polymake { namespace polytope {

   using graph::Lattice;
   using graph::lattice::BasicDecoration;
   using graph::lattice::BasicDecorator;
   using graph::lattice::BasicClosureOperator;
   using graph::lattice::TrivialCut;
   using graph::lattice::SetAvodiginCut;
   using graph::lattice::RankCut;
   using graph::lattice::CutAnd;
   using graph::lattice::Sequential;
   using graph::lattice::Nonsequential;

	// Compute full Hasse diagram of a cone
   template <typename IMatrix>
      perl::Object hasse_diagram(const GenericIncidenceMatrix<IMatrix> &VIF, int cone_dim) {
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

   //Compute Hasse diagram of bounded faces of a polytope (possibly up to a dimension)
   template <typename IMatrix, typename TSet>
      Lattice<BasicDecoration, Nonsequential> bounded_hasse_diagram_computation(const GenericIncidenceMatrix<IMatrix> &VIF,
            const TSet& far_face,
            const int boundary_dim = -1) {


         typedef SetAvodiginCut<BasicDecoration> bounded_cut_type;
         typedef RankCut<BasicDecoration, graph::lattice::RankCutType::LesserEqual> rank_cut_type;
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

   template <typename IMatrix, typename TSet>
      perl::Object bounded_hasse_diagram(
				const GenericIncidenceMatrix<IMatrix> &VIF,
            const TSet& far_face,
            const int boundary_dim = -1) {
			//Special check for empty polytope
         if(std::min(VIF.rows(), VIF.cols()) == 0) {
            return hasse_diagram(VIF,0);
         }return bounded_hasse_diagram_computation(VIF, far_face, boundary_dim).makeObject();
		}


   template <typename IMatrix>
      perl::Object rank_bounded_hasse_diagram(const GenericIncidenceMatrix<IMatrix>& VIF,
            int cone_dim, int boundary_dim, bool from_above) {
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

   template <typename IMatrix>
      perl::Object lower_hasse_diagram(const GenericIncidenceMatrix<IMatrix>& VIF, int boundary_dim) {
         return rank_bounded_hasse_diagram(VIF, 0,boundary_dim, false);
      }

   template <typename IMatrix>
      perl::Object upper_hasse_diagram(const GenericIncidenceMatrix<IMatrix>& VIF, int cone_dim, int boundary_dim) {
         return rank_bounded_hasse_diagram(VIF, cone_dim, boundary_dim, true);
      }

}}

#endif
