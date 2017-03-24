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
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/graph/GraphIso.h"
#include "polymake/linalg.h"
#include "polymake/TropicalNumber.h"
#include "polymake/graph/lattice_builder.h"
#include "polymake/fan/hasse_diagram.h"
#include "polymake/tropical/covectors.h"

namespace polymake { namespace tropical {

   using namespace graph;
   using namespace graph::lattice;

   // Keep only cells lying in the tropical span, i.e. whose covector has no empty entries
   class CovectorCut {
      public:
         CovectorCut() {}
         inline bool operator()(const CovectorDecoration& data) const {
            for(auto r_it = entire(rows(data.covector)); !r_it.at_end(); ++r_it)
               if(r_it->size() == 0) return false;
            return true;
         }
   };

   // Computes face and rank as normal and the covector from the face
   template <typename Addition, typename Scalar>
      class CovectorDecorator {
         protected :
            const Array<IncidenceMatrix<> >& pseudovertex_covectors;
            const Matrix<TropicalNumber<Addition,Scalar> > points;
            const Set<int> artificial_set;
         public :
            typedef graph::lattice::BasicClosureOperator<CovectorDecoration>::ClosureData FaceData; 
            CovectorDecorator( const Matrix<TropicalNumber<Addition,Scalar> >& p_arg,
                               const Array<IncidenceMatrix<> >& p_cov,
                               const Set<int>& artificial_set) :
               pseudovertex_covectors(p_cov), points(p_arg), artificial_set(artificial_set) {}

            /*
             * @brief Takes a list of covectors and returns the intersection of those elements specified by s
             */
            inline IncidenceMatrix<> covector_from_atoms(const Set<int> &s) const {
               return accumulate( select( pseudovertex_covectors, s), operations::mul());
            }

            const CovectorDecoration compute_initial_decoration(const FaceData& face) const {
               IncidenceMatrix<> initial_covector(points.cols(), points.rows());
               for(auto p_it = ensure(rows(points), (pm::cons<pm::indexed, pm::end_sensitive>*)0).begin(); \
                     !p_it.at_end(); ++p_it) {
                  initial_covector.col(p_it.index()) = support(*p_it);
               }
               return CovectorDecoration( Set<int>(), 0, initial_covector);
            }

            const CovectorDecoration compute_decoration(const FaceData& face,
                  const CovectorDecoration& predecessor_data) const {
               if(face.get_dual_face().size() == 0) {
                  //This can only occur in the torus case, where this represents the artificial top node
                  return CovectorDecoration( artificial_set, predecessor_data.rank+1,
                        IncidenceMatrix<>( points.cols(), points.rows()));
               }
               IncidenceMatrix<> face_covector = covector_from_atoms( face.get_face() );
               return CovectorDecoration( face.get_face(), predecessor_data.rank+1, face_covector);
            }

            const CovectorDecoration compute_artificial_decoration(
                  const NodeMap<Directed, CovectorDecoration> &decor, const std::list<int> &max_nodes) const {
               IncidenceMatrix<> artificial_covector(points.cols(), points.rows());
               auto max_list = attach_member_accessor( select(decor, max_nodes),
                     ptr2type<CovectorDecoration, int, &CovectorDecoration::rank>()
                     );
               int top_rank = accumulate(max_list, operations::max()) + 1;
               return CovectorDecoration( artificial_set, top_rank, artificial_covector);
            }
      };


   /*
	 * @brief Computes the covector decomposition of the tropical span or the tropical
	 * torus induced by a list of tropical points.
	 * @param perl::Object cone. A tropical::Polytope
	 * @option Bool compute_only_tropical_span If true, only the covector decomposition of the tropical span is
	 * computed. If false, the covector decomposition of the torus is also computed.
	 * @return Nothing. Sets the properties [[POLYTOPE_COVECTOR_DECOMPOSITION]], [[POLYTOPE_MAXIMAL_COVECTOR_CELLS]]
    * and [[POLYTOPE_MAXIMAL_COVECTORS]] if compute_only_tropical_span is true, otherwise it sets
	 * [[TORUS_COVECTOR_DECOMPOSITION]] and [[MAXIMAL_COVECTORS]].
	 */
   template <typename Addition, typename Scalar>
      void compute_covector_decomposition(perl::Object cone, perl::OptionSet options) {
         bool compute_only_tropical_span = options["compute_only_tropical_span"];

         const Array<IncidenceMatrix<> >& pseudovertex_covectors = cone.give("PSEUDOVERTEX_COVECTORS");
         const IncidenceMatrix<>& max_covector_cells = cone.give("MAXIMAL_COVECTOR_CELLS");
         const Matrix< TropicalNumber<Addition,Scalar> >& points = cone.give("POINTS");

         Lattice<CovectorDecoration> init_lattice;
         Set<int> queuing_nodes;
         if(!compute_only_tropical_span) {
            perl::Object cone_cov_obj = cone.give("POLYTOPE_COVECTOR_DECOMPOSITION");
            Graph<Directed> cone_cov_graph = cone_cov_obj.give("ADJACENCY");
            int top_node = cone_cov_obj.give("TOP_NODE");
            int bottom_node = cone_cov_obj.give("BOTTOM_NODE");
            // We need to queue all maximal faces and all atoms
            for(auto max_node : cone_cov_graph.in_adjacent_nodes(top_node)) {
               queuing_nodes += ( max_node > top_node? max_node-1 : max_node);
            }
            for(auto atom : cone_cov_graph.out_adjacent_nodes(bottom_node)) {
               queuing_nodes += ( atom > top_node? atom-1 : atom);
            }
            init_lattice = copy_all_but_top_node(Lattice<CovectorDecoration>(cone_cov_obj));
         }

         fan::lattice::ComplexPrimalClosure<CovectorDecoration> cop(max_covector_cells);
         TrivialCut<CovectorDecoration> trivial_cut;
         CovectorCut covector_cut;
         CovectorDecorator<Addition,Scalar> dec = CovectorDecorator<Addition,Scalar>( points, pseudovertex_covectors, scalar2set(-1));

         //Note: The additional top node is always attained through closure in the torus decomposition,
         //as we always have at least two cells
         // For cones, such a closure is cut, so we always need an artificial top node
         Lattice<CovectorDecoration> result = compute_only_tropical_span?
            lattice_builder::compute_lattice_from_closure<CovectorDecoration>(cop, covector_cut, dec, 1, lattice_builder::Primal()) :
            lattice_builder::compute_lattice_from_closure<CovectorDecoration>(cop, trivial_cut, dec, 0, lattice_builder::Primal(), init_lattice, queuing_nodes);

         int n_max_nodes = result.in_degree(result.top_node());
         auto max_nodes = select(result.decoration(), result.in_adjacent_nodes(result.top_node()));
         Array<IncidenceMatrix<> > max_covectors( n_max_nodes, entire(attach_member_accessor(max_nodes,
                     ptr2type<CovectorDecoration, IncidenceMatrix<>, &CovectorDecoration::covector>())));
         Array<Set<int> > cone_max_cells( n_max_nodes, entire(attach_member_accessor(max_nodes,
                     ptr2type<CovectorDecoration, Set<int>, &CovectorDecoration::face>())));
         if(compute_only_tropical_span) {
            cone.take("POLYTOPE_MAXIMAL_COVECTOR_CELLS") << cone_max_cells;
            cone.take("POLYTOPE_MAXIMAL_COVECTORS") << max_covectors;
            cone.take("POLYTOPE_COVECTOR_DECOMPOSITION") << result.makeObject();
         }
         else {
            //Torus cells might be in a different order
            Array<int> cell_perm = find_permutation( cone_max_cells, rows(max_covector_cells));
            cone.take("MAXIMAL_COVECTORS") << permuted(max_covectors, cell_perm);
            cone.take("TORUS_COVECTOR_DECOMPOSITION") << result.makeObject();
         }
      }

	FunctionTemplate4perl("compute_covector_decomposition<Addition,Scalar>(Polytope<Addition,Scalar> {compute_only_tropical_span => 0}) : void");


}}

