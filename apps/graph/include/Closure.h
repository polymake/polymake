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

#pragma once

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
  Int& index;
  bool is_unknown;
  bool is_marked_unwanted;

  FaceIndexingData(Int& i, bool iu, bool im)
    : index(i)
    , is_unknown(iu)
    , is_marked_unwanted(im) {}

  void set_index(Int j)
  {
    index = j;
    is_unknown = false;
  }
  void mark_face_as_unwanted()
  {
    index = -2;
    is_marked_unwanted = true;
  }
};

/*
 * Given a closure operator, this can iterate over all closed sets lying "above" a given set (from the point of
 * view of the algorithm).
 */
template <typename ClosureOperator>
class closures_above_iterator {
public:
  using ClosureData = typename ClosureOperator::ClosureData;
  using iterator_category = std::forward_iterator_tag;
  using value_type = ClosureData;
  using reference = const value_type&;
  using pointer = const value_type*;
  using difference_type = ptrdiff_t;

  closures_above_iterator() = default;

  closures_above_iterator(const ClosureOperator& cop,
                          const ClosureData& H_arg,
                          const Set<Int>& relevant_candidates)
    : H(&H_arg)
    , CO(&cop)
    , total_size(cop.total_set_size())
    , candidates(relevant_candidates - H->get_face())
    , done(false)
  {
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
      const Int v = candidates.front();  candidates.pop_front();
      result = ClosureData(*CO, H->get_dual_face() * CO->get_facets().col(v));
      // The full set is rarely the minimal set - and if so, it is so for the last candidate
      // This saves a lot of checks in the next step.
      const Set<Int>& rface = result.get_face();
      if (rface.size() == total_size && !candidates.empty()) continue;
      if ((rface * candidates).empty() && (rface * minimal).empty()) {
        minimal.push_back(v);
        return;
      }
    }
    done=true;
  }

  const ClosureData* H;
  const ClosureOperator* CO;
  const Int total_size;
  Set<Int> candidates, minimal;
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
    mutable Set<Int> face;
    Set<Int> dual_face;
    mutable bool primal_computed;
    const BasicClosureOperator<Decoration>* parent;

  public:
    ClosureData() = default;
    ClosureData(const ClosureData& other) = default;

    ClosureData(const BasicClosureOperator<Decoration>& parent_, const Set<Int>& df)
      : dual_face(df)
      , primal_computed(false)
      , parent(&parent_)
    {}

    template <typename TSet1, typename TSet2>
    ClosureData(const GenericSet<TSet1, Int>& f, const GenericSet<TSet2, Int>& df)
      : face(f)
      , dual_face(df)
      , primal_computed(true)
      , parent(nullptr)
    {}

    bool has_face() const { return primal_computed; }
    const Set<Int>& get_dual_face() const { return dual_face; }

    const Set<Int>& get_face() const
    {
      if (!primal_computed) {
        if (dual_face.empty())
          face = parent->get_total_set();
        else 
          face = accumulate(rows(parent->get_facets().minor(dual_face,All)), operations::mul());
        primal_computed = true;
      }
      return face;
    }
  };

  // Constructors

  BasicClosureOperator() = default;

  BasicClosureOperator(const Int total, const IncidenceMatrix<>& fct)
    : facets(fct)
    , total_size(total)
    , total_set(sequence(0, total_size))
    , total_data(total_set, Set<Int>())
  {}

  // Closure operator interface

  ClosureData closure_of_empty_set() const
  {
    return ClosureData(accumulate(rows(facets), operations::mul()), sequence(0,facets.rows()));
  }

  auto get_closure_iterator(const ClosureData& face) const
  {
    return closures_above_iterator<BasicClosureOperator<Decoration>>(*this,face, total_set);
  }

  ClosureData compute_closure_data(const Decoration &face) const
  {
    return ClosureData(face.face, accumulate( cols(facets.minor(All,face.face)), operations::mul()));
  }

  FaceIndexingData get_indexing_data(const ClosureData& data)
  {
    Int& fi = face_index_map[data.get_dual_face()];
    return FaceIndexingData(fi, fi == -1, fi == -2);
  }

  // Auxiliary methods

  Int total_set_size() const { return total_size; }
  const IncidenceMatrix<>& get_facets() const { return facets; }
  const Set<Int>& get_total_set() const { return total_set; }

protected:
  IncidenceMatrix<> facets;
  Int total_size;
  Set<Int> total_set;
  ClosureData total_data;
  FaceMap<> face_index_map;
};

} } }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
