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
#include "polymake/graph/HasseDiagram.h"
#include "polymake/linalg.h"
#include "polymake/TropicalNumber.h"
#include "polymake/polytope/face_lattice_tools.h"

namespace polymake { namespace tropical {

	using polymake::graph::HasseDiagram;
	using polymake::polytope::face_lattice::faces_one_above_iterator;
	using polymake::polytope::face_lattice::c;

	typedef HasseDiagram::_filler FillerType;
	typedef NodeMap< Directed, IncidenceMatrix<> > CovectorMap;

	//The return type for add_node_to_both
	struct AddNodeResult {
		bool is_in_span;
		int torus_node_index;
		int cone_node_index;
	};


	/*
	 * @brief Computes for a given cell with given covector, whether it is in the tropical span.
	 * I.e. checks whether the IncidenceMatrix has no empty row
	 * */
	bool is_in_tropical_span( const IncidenceMatrix<> &covector) {
		for(Entire<Rows<IncidenceMatrix<> > >::const_iterator r = entire(rows(covector)); !r.at_end(); r++) {
			if( r->size() == 0) return false;
		}
		return true;
	}

	/*
	 * @brief Adds a new node to the two Hasse diagrams of the torus decomposition and the cone
	 * decomposition. The node is described by the corresponding face ( a set) and its covector.
	 * It is only added to the torus decomposition, if it should be computed. It is only added to the
	 * tropical span, if the face actually lies in the tropical span. It will also connect the new nodes to
	 * possible existing nodes given by their indices (-1 if no edge should be added)
	 * @return Whether the face is in the tropical span and what its index in the respective HasseDiagrams is.
	 */
	AddNodeResult add_node_to_both(FillerType &torus_filler, FillerType &cone_filler, CovectorMap &torus_covector_map, CovectorMap &cone_covector_map, bool compute_only_tropical_span, const Set<int> &face, const IncidenceMatrix<> &covector,int connect_to_torus_node, int connect_to_cone_node) {
		AddNodeResult result;	
		result.is_in_span = false;
		if(!compute_only_tropical_span) {
			result.torus_node_index = torus_filler.add_node(face);
			torus_covector_map[result.torus_node_index] = covector;
			if(connect_to_torus_node != -1) torus_filler.add_edge(connect_to_torus_node, result.torus_node_index);
		}
		if(is_in_tropical_span(covector)) {
			result.cone_node_index = cone_filler.add_node(face);
			cone_covector_map[result.cone_node_index] = covector; 
			if(connect_to_cone_node != -1) cone_filler.add_edge(connect_to_cone_node,result.cone_node_index);
			result.is_in_span = true;
		}
		return result;
	}

	/*
	 * @brief Increases dimensions of the fillers as necessary
	 */
	void increase_dims(FillerType &torus_filler, FillerType &cone_filler, bool compute_only_tropical_span) {
		if(!compute_only_tropical_span) torus_filler.increase_dim();
		cone_filler.increase_dim();
	}

	/*
	 * @brief Takes a list of covectors and returns the intersection of those elements specified by s
	 */
	IncidenceMatrix<> covector_from_atoms(const Array<IncidenceMatrix<> > &atom_covectors, const Set<int> &s) {
		IncidenceMatrix<> result;
		int i = 0;
		for(Entire<Set<int> >::const_iterator sit = entire(s); !sit.at_end(); sit++, i++) {
			if(i == 0) result = atom_covectors[*sit];
			else result *= atom_covectors[*sit];
		}
		return result;
	}

	//FIXME This should be replaced by a general refactoring of the Hasse diagram (issue #851)
	// Currently this is copy & paste & modify from compute_bounded in polytope/face_lattice_tools.h


	/*
	 * @brief Computes the covector decomposition of the tropical span and possibly of the tropical
	 * torus induced by a list of tropical points.
	 * @param tropical::Cone
	 * @param bool compute_only_tropical_span If true, only the covector decomposition of the tropical span is
	 * computed. If false, the covector decomposition of the torus is also computed.
	 * @return Nothing. Sets the properties [[CONE_COVECTOR_DECOMPOSITION]], [[CONE_MAXIMAL_COVECTOR_CELLS]] 
	 * and (if compute_only_tropical_span=false), also [[TORUS_COVECTOR_DECOMPOSITION]].
	 */
	template <typename Addition, typename Scalar>
		void compute_covector_decomposition(perl::Object cone, bool compute_only_tropical_span) {

			Matrix<Rational> pseudovertices = cone.give("PSEUDOVERTICES");
			Set<int> bounded_pvs = support(pseudovertices.col(0));
			Array<IncidenceMatrix<> > pseudovertex_covectors = cone.give("PSEUDOVERTEX_COVECTORS");
			IncidenceMatrix<> max_covector_cells = cone.give("MAXIMAL_COVECTOR_CELLS");
			Matrix<TropicalNumber<Addition,Scalar> > points = cone.give("POINTS");

			HasseDiagram HD_torus;
			HasseDiagram HD_cone;

			FillerType torus_filler = filler(HD_torus,true);
			FillerType cone_filler = filler(HD_cone,true);
			NodeMap< Directed, IncidenceMatrix<> > torus_covector_map( torus_filler.graph());
			NodeMap< Directed, IncidenceMatrix<> > cone_covector_map( cone_filler.graph());

			//Queue of faces. The boolean flag indicates whether the node is in the tropical span. 
			std::list<std::pair<Set<int>, bool> > Q;
			//These will be used to store indices of nodes
			FaceMap<> torus_faces;
			FaceMap<> cone_faces;

			//The bottom node: empty set
			IncidenceMatrix<> empty_set_covector(points.cols(), points.rows());
			int c_index = 0;
			for(typename Entire<Rows<Matrix<TropicalNumber<Addition,Scalar> > > >::iterator r = entire(rows(points));
					!r.at_end(); r++,c_index++) {
				empty_set_covector.col(c_index) = support(r->top());
			}
			add_node_to_both(torus_filler,cone_filler, torus_covector_map, cone_covector_map, 
					compute_only_tropical_span,Set<int>(), empty_set_covector,-1,-1);
			increase_dims(torus_filler, cone_filler,compute_only_tropical_span);
			
			//Index savers: Counts when we reach the last of a node of given
			// dimension in the queue. (Can be interpreted as an "index" in Q).
			// Also counts the number of inclusion-maximal faces
			int end_this_dim = 0, end_next_dim = 0, torus_max_faces_cnt = 0, cone_max_faces_cnt = 0;

			//The first level: vertices (only the bounded ones) 
			Set<int> tropical_atoms;
			for(Entire<Set<int> >::iterator v = entire(bounded_pvs); !v.at_end(); v++) {
				Set<int> atom_set = scalar2set(*v);
				AddNodeResult atom_result = 
					add_node_to_both(torus_filler, cone_filler, torus_covector_map, cone_covector_map,
						compute_only_tropical_span, atom_set, pseudovertex_covectors[*v],0,0);
				if(atom_result.is_in_span || !compute_only_tropical_span) 
					Q.push_back(std::make_pair(atom_set,atom_result.is_in_span));
				if(atom_result.is_in_span) tropical_atoms += *v;
			}
			increase_dims(torus_filler, cone_filler,compute_only_tropical_span);

			end_this_dim = end_next_dim = 
				compute_only_tropical_span? tropical_atoms.size() : bounded_pvs.size();

			//These are the indices of the node we're currently investigating in the queue.
			// (They might differ in the two lattices!)
			int cone_index = 1;
			int torus_index = 1;

			bool added_tropical_for_next_level = false;

			while(true) {
				std::pair<Set<int>, bool> current_node = Q.front(); Q.pop_front();
				bool is_maximal_torus_face = true, is_maximal_cone_face = true;
				for(faces_one_above_iterator<Set<int>, IncidenceMatrix<> > 
						faces(current_node.first, max_covector_cells); !faces.at_end(); ++faces) {
					Set<int> intersecting_cells = c(faces->second,max_covector_cells);
					int &cone_node_ref = cone_faces[intersecting_cells];
					int &torus_node_ref = torus_faces[intersecting_cells];
					if(cone_node_ref == -1) { //I.e. we're adding a new node
						IncidenceMatrix<> face_covector = 
							covector_from_atoms(pseudovertex_covectors, faces->second);
						AddNodeResult face_result = add_node_to_both(torus_filler, cone_filler,
								torus_covector_map, cone_covector_map, compute_only_tropical_span, 
								faces->second,face_covector,torus_index, cone_index);
						if(!compute_only_tropical_span) torus_node_ref = face_result.torus_node_index;
						else torus_node_ref = -2;
						if(face_result.is_in_span) cone_node_ref = face_result.cone_node_index;
						else cone_node_ref = -2;
						if(face_result.is_in_span || !compute_only_tropical_span) {
							++end_next_dim;
							Q.push_back( std::make_pair(faces->second, face_result.is_in_span));
							added_tropical_for_next_level = added_tropical_for_next_level | face_result.is_in_span;
						}
					}
					else {
						//If it's not new, it might still be a node we've already seen and determined we don't want
						if(cone_node_ref != -2) { 
							is_maximal_cone_face = false;
							cone_filler.add_edge(cone_index, cone_node_ref);
						}
						if(torus_node_ref != -2) {
							is_maximal_torus_face = false;
							if(!compute_only_tropical_span) torus_filler.add_edge(torus_index, torus_node_ref);
						}
					}
				}//END iterate faces one above
				if(is_maximal_cone_face) cone_max_faces_cnt++;
				if(is_maximal_torus_face) torus_max_faces_cnt++;

				if(current_node.second) cone_index++;
				torus_index++;
				if(torus_index > end_this_dim) {
					if(Q.empty()) break;
					if(!compute_only_tropical_span) torus_filler.increase_dim();
					if(added_tropical_for_next_level) cone_filler.increase_dim();
					added_tropical_for_next_level = false;
					end_this_dim = end_next_dim;
				}

			}//END while Q not empty

		
			//Connect the top node 
			Set<int> max_cone_nodes;
			Set<int> all_tropical_indices; //To include the unbounded rays
			Vector<Set<int> > max_cone_faces;
			Vector<IncidenceMatrix<> > max_cone_covectors;
			for(int i=0; i < cone_filler.graph().nodes(); i++) {
				if(cone_filler.graph().out_degree(i) == 0) {
					max_cone_nodes += i;
					max_cone_faces |= cone_filler.faces()[i];
					max_cone_covectors |= cone_covector_map[i];
				}
				all_tropical_indices += cone_filler.faces()[i];
			}

			//If both have the same dimension 
			//we're also computing the torus subdivision, we need to increase dims once more
			//if( !compute_only_tropical_span && HD_cone.dim() == HD_torus.dim() -1)
			//	cone_filler.increase_dim();

			int top_cone_index = cone_filler.add_node( all_tropical_indices );
			cone_covector_map[top_cone_index] = covector_from_atoms(pseudovertex_covectors, all_tropical_indices);
			for(Entire<Set<int> >::iterator mcn = entire(max_cone_nodes); !mcn.at_end(); mcn++) {
				cone_filler.add_edge(*mcn, top_cone_index);
			}


			Vector<Set<int> > max_torus_faces;
			Vector<IncidenceMatrix<> > max_torus_covectors;
			if(!compute_only_tropical_span) {
				int top_torus_index = torus_filler.add_node(sequence(0, pseudovertex_covectors.size()));
				torus_covector_map[top_torus_index] = accumulate( pseudovertex_covectors, operations::mul());
				for(int i=0; i < top_torus_index;++i) {
					if(torus_filler.graph().out_degree(i)==0) {
						torus_filler.add_edge(i,top_torus_index);
						max_torus_faces |= torus_filler.faces()[i];
						max_torus_covectors |= torus_covector_map[i];
					}
				}
			}

			//Create result
			perl::Object cone_covector_lattice("CovectorLattice");
				cone_covector_lattice.take("ADJACENCY") << HD_cone.graph();
				cone_covector_lattice.take("FACES") << HD_cone.faces();
				cone_covector_lattice.take("DIMS") << HD_cone.dims();
				cone_covector_lattice.take("COVECTORS") << cone_covector_map;
			cone.take("POLYTOPE_COVECTOR_DECOMPOSITION") << cone_covector_lattice;
			cone.take("POLYTOPE_MAXIMAL_COVECTOR_CELLS") << max_cone_faces;
			cone.take("POLYTOPE_MAXIMAL_COVECTORS") << max_cone_covectors;

			if(!compute_only_tropical_span) {
				IncidenceMatrix<> max_torus_incidence(max_torus_faces);
				Array<int> torus_face_perm = graph::find_row_col_permutation( max_torus_incidence, max_covector_cells).first;

				perl::Object torus_covector_lattice("CovectorLattice");
				torus_covector_lattice.take("ADJACENCY") << HD_torus.graph();
				torus_covector_lattice.take("FACES") << HD_torus.faces();
				torus_covector_lattice.take("DIMS") << HD_torus.dims();
				torus_covector_lattice.take("COVECTORS") << torus_covector_map;
				cone.take("TORUS_COVECTOR_DECOMPOSITION") << torus_covector_lattice;
				cone.take("MAXIMAL_COVECTORS") << permuted(max_torus_covectors, torus_face_perm);
			}

		}

	FunctionTemplate4perl("compute_covector_decomposition<Addition,Scalar>(Polytope<Addition,Scalar>, $) : void");


}}
	
