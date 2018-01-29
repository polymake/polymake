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

#ifndef POLYMAKE_GRAPH_LATTICE_CLOSURE_H
#define POLYMAKE_GRAPH_LATTICE_CLOSURE_H

#include "polymake/list"
#include "polymake/FaceMap.h"
#include "polymake/FacetList.h"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace graph { namespace lattice {

  /*
   * This stores the indexing data of a node in the Hasse diagram algorithm.
   * It informs, whether the node is: Unknown so far, has been marked unwanted or what its index is.
   * It also includes methods to set the index or mark a node as unwanted.
   */
  	struct FaceIndexingData {
		int& index;
		bool is_unknown;
		bool is_marked_unwanted;
		FaceIndexingData(int& i, bool iu, bool im) : index(i), is_unknown(iu), is_marked_unwanted(im) {}
		void set_index(int j) { index = j; is_unknown = false;}
		void mark_face_as_unwanted() { index = -2; is_marked_unwanted = true; }
   };

	/*
    * Given a closure operator, this can iterate over all closed sets lying "above" a given set (from the point of
    * view of the algorithm).
    */
   template <typename ClosureOperator>
      class closures_above_iterator {

         public:
				typedef typename ClosureOperator::ClosureData ClosureData;
            typedef std::forward_iterator_tag iterator_category;
            typedef ClosureData value_type;
            typedef const value_type& reference;
            typedef const value_type* pointer;
            typedef ptrdiff_t difference_type;

            closures_above_iterator() {}
            closures_above_iterator( const ClosureOperator& cop,
                                     const ClosureData &H_arg,
                                     const Set<int>& relevant_candidates) :
               H(&H_arg), CO(&cop), total_size(cop.total_set_size()), candidates(relevant_candidates - H->get_face()), done(false)  {
                  find_next();
               }

            reference operator* () const { return result; }
            pointer operator->() const { return &result; }

            closures_above_iterator& operator++ () { find_next(); return *this; }

            const closures_above_iterator operator++ (int) { closures_above_iterator copy = *this; operator++(); return copy; }

            bool operator== (const closures_above_iterator& it) const { return candidates==it.candidates; }
            bool operator!= (const closures_above_iterator& it) const { return !operator==(it); }
            bool at_end() const { return done; }

         protected:
            void find_next()
            {
               while (!candidates.empty()) {
                  int v=candidates.front();  candidates.pop_front();
                  result= ClosureData( *CO, H->get_dual_face() * CO->get_facets().col(v)); 
                  //The full set is rarely the minimal set - and if so, it is so for the last candidate
                  //This saves a lot of checks in the next step.
                  const Set<int>& rface = result.get_face();
                  if(rface.size() == total_size && !candidates.empty()) continue;
                  if ((rface * candidates).empty() && (rface * minimal).empty()) {
                     minimal.push_back(v);
                     return;
                  }
               }
               done=true;
            }

            const ClosureData* H;
            const ClosureOperator* CO;
            const int total_size;
            Set<int> candidates, minimal;
            value_type result;
            bool done;
      };

	/*
    * A closure operator needs to provide the following interface:
    *
    * - It needs to define a type ClosureData (e.g. via a typedef or as a nested class). This type represents
    *   all the operator needs to associate with a node in the lattice algorithm.
    * - It is templated with the type of a Decoration
    * - It needs to have the following methods
    *   - const ClosureData closure_of_empty_set() const: Computes the initial node in the algorithm
    *   - FaceIndexingData get_indexing_data(const ClosureData& d): Given a node, this computes the indexing
    *     data (i.e. potential index, whether it is new, etc...)
    *   - const ClosureData compute_closure_data(const Decoration& face) const: Given a decorated node (e.g. from
    *     an initial lattice given to the algorithm), this computes all the necessary closure data.
    *   - Iterator get_closure_iterator(const ClosureData& d) const: Given a node in the lattice, this provides
    *     an iterator over all the nodes lying "above" this node.
    */


	/*
	 * The basic closure operator: The closure of a set is the intersection of all "facets" containing it.
	 * If no facet contains the set, the closure is the full set.
	 */
	template <typename Decoration = BasicDecoration>
		class BasicClosureOperator {
			public:

            // The basic closure data consists of a face and its dual face (i.e. the intersection over
            // all columns indexed by the elements of the face).
            // Since computing the dual face is much cheaper, the primal face is computed lazily.
				class ClosureData {
					protected:
						mutable Set<int> face;
						Set<int> dual_face;
						mutable bool primal_computed;
						const BasicClosureOperator<Decoration>* parent;

					public:
						ClosureData() {}
                  ClosureData(const ClosureData& other) {
                     dual_face = other.dual_face;
                     face = other.face;
                     primal_computed = other.primal_computed;
                     parent = other.parent;
                  }

						ClosureData(const BasicClosureOperator<Decoration>& parent, const Set<int>& df) : dual_face(df), primal_computed(false), parent(&parent) {}

						template <typename TSet1, typename TSet2>
							ClosureData(const GenericSet<TSet1,int>& f, const GenericSet<TSet2,int>& df) : face(f), dual_face(df), primal_computed(true), parent(0) {}

                  bool has_face() const { return primal_computed; }
						const Set<int>& get_dual_face() const { return dual_face;}
						const Set<int>& get_face() const {
							if(!primal_computed) {
								if(dual_face.empty())
                           face = parent->get_total_set();
                        else 
                           face = accumulate(rows(parent->get_facets().minor(dual_face,All)),
                                 operations::mul());
								primal_computed = true;
							}
							return face;
						}
				};

            // Constructors

				BasicClosureOperator() {}
				BasicClosureOperator(const int total, const IncidenceMatrix<> &fct) : facets(fct),
				total_size(total), total_set(sequence(0,total_size)), total_data(total_set, Set<int>()) {}

            // Closure operator interface

				const ClosureData closure_of_empty_set() const {
					return ClosureData(accumulate(rows(facets), operations::mul()), sequence(0,facets.rows()));
				}

				closures_above_iterator<BasicClosureOperator<Decoration> > get_closure_iterator(const ClosureData& face) const {
					return closures_above_iterator<BasicClosureOperator<Decoration> >(*this,face, total_set);
				}

				const ClosureData compute_closure_data(const Decoration &face) const {
					return ClosureData(face.face,
							accumulate( cols(facets.minor(All,face.face)), operations::mul()));
				}

				FaceIndexingData get_indexing_data(const ClosureData& data) {
					int& fi = face_index_map[data.get_dual_face()];
					return FaceIndexingData(fi, fi == -1, fi == -2);
				}
				
            // Auxiliary methods

            int total_set_size() const { return total_size;}
            const IncidenceMatrix<>& get_facets() const { return facets;}
            const Set<int>& get_total_set() const { return total_set;}

			protected:
				IncidenceMatrix<> facets;
				int total_size;
				Set<int> total_set;
				ClosureData total_data;
				FaceMap<> face_index_map;

		};


}}}

#endif
