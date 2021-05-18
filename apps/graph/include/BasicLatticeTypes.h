/* Copyright (c) 1997-2021
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

#include "polymake/client.h"
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/vector"
#include "polymake/IncidenceMatrix.h"
#include <algorithm>
#include "polymake/graph/Decoration.h"
#include "polymake/graph/Closure.h"

namespace polymake { namespace graph { namespace lattice {

// Every CrossCut needs to provide an operator
//
// bool operator()(const Decoration& data), which accepts a decoration and then returns whether this
// node should be kept (true) or discarded (false).

/*
 * The trivial cut - accepts everything
 */
template <typename Decoration>
class TrivialCut {
public:
  TrivialCut() = default;
  bool operator()(const Decoration& data) const { return true; }
};

/*
 * Only keeps sets not intersecting a fixed set
 */
template <typename Decoration>
class SetAvoidingCut {
protected:
  const Set<Int> avoid;
public:
  SetAvoidingCut(const Set<Int>& set_arg) : avoid(set_arg) {}

  bool operator()(const Decoration &data) const
  {
    return (data.face * avoid).empty();
  }
};

/*
 * Only keeps sets smaller than the full set.
 */
template <typename Decoration>
class NotFullSetCut {
protected:
  const Int full_set_size;
public:
  NotFullSetCut(const Int full_set_size_)
    : full_set_size(full_set_size_) {}

  bool operator()(const Decoration& data) const
  {
    return data.face.size() < full_set_size;
  }
};

namespace RankCutType {

const bool GreaterEqual = false;
const bool LesserEqual = true;

}

/*
 * Only keeps nodes whose rank is greater equal (or lesser equal) a certain number.
 */
template <typename Decoration, bool lesser_equal>
class RankCut {
protected:
  const Int compare;
public:
  RankCut(Int comp_arg)
    : compare(comp_arg) {}

  bool operator()(const Decoration& data) const
  {
    return lesser_equal ? data.rank <= compare : data.rank >= compare;
  }
};

/*
 * This encapsulates various parameters for restricting the rank of a lattice.
 */
struct RankRestriction {
  Int boundary_rank;            // up to which dimension (included!)
  bool rank_restricted;         // Do we want to restrict the rank?
  bool rank_restriction_type;   // RankCutType::LesserEqual or ::GreaterEqual

  RankRestriction()
    : boundary_rank(0)
    , rank_restricted(false)
    , rank_restriction_type(false) {}

  RankRestriction(bool res_arg, bool res_type, Int dim_arg)
    : boundary_rank(dim_arg)
    , rank_restricted(res_arg)
    , rank_restriction_type(res_type) {}
};

/*
 * Makes a cut from two cuts: Both have to return true for a final true value
 */
template <typename C1, typename C2>
class CutAnd {
protected:
  const C1& c1;
  const C2& c2;
public:
  CutAnd(const C1& a1, const C2& a2)
    : c1(a1), c2(a2) {}

  template <typename Decoration>
  bool operator()(const Decoration& data) const
  {
    return c1(data) && c2(data);
  }
};

/*
 * In the lattice building algorithm, the data handled by the closure operator can be different
 * from the data one wants to attach to the final lattice.
 * The decorator converts the closure data to the actual decoration. 
 * It needs to provide the following interface:
 * - const Decoration compute_initial_decoration(const ClosureData& face) const:
 *   Here, Decoration needs to match the Decoration of the corresponding lattice and
 *   ClosureData must be the ClosureData returned by the closure operator. The function computes
 *   the decoration of the initial node.
 * - const Decoration compute_decoration(const ClosureData& face, const Decoration& predecessor_data) const:
 *   Given the closure data of a node and the decoration of *one of* its predecessors, this computes
 *   the decoration of this node.
 * - const Decoration compute_artificial_decoration(const NodeMap<Directed, BasicDecoration>& decor,
 *                                                  const std::list<Int>& max_nodes) const
 *   In some cases, an artificial top node is added to the lattice at the end of the algorithm. This
 *   computes its decor from a node map containing all previous decorations and a list of indices of all
 *   nodes which are maximal.
 */

/*
 * The basic decorator: Faces are either the sets computed during the closure algorithm
 * or the intersection of the corresponding coatoms (if built dually).
 * Ranks are computed as heights in the lattice plus a potential shift.
 */
template <typename FaceData = BasicClosureOperator<BasicDecoration>::ClosureData>
class BasicDecorator {
protected:
  const Int total_size;
  const Int initial_rank;
  const bool built_dually;
  const Set<Int> artificial_set;

public:
  // dual type
  BasicDecorator(Int n_vertices, Int top_rank, const Set<Int> artificial)
    : total_size(n_vertices)
    , initial_rank(top_rank)
    , built_dually(true)
    , artificial_set(artificial) {}

  //primal type
  BasicDecorator(Int bottom_rank, const Set<Int> artificial)
    : total_size(0)
    , initial_rank(bottom_rank)
    , built_dually(false)
    , artificial_set(artificial) {}

  BasicDecoration compute_decoration(const FaceData& face,
                                     const BasicDecoration& predecessor_data) const
  {
    BasicDecoration data;
    data.rank = predecessor_data.rank + (built_dually ? -1 : 1);
    data.face = built_dually ? face.get_dual_face() : face.get_face();
    return data;
  }

  BasicDecoration compute_initial_decoration(const FaceData& face) const
  {
    BasicDecoration data;
    data.rank = initial_rank;
    data.face = built_dually? face.get_dual_face() : face.get_face();
    return data;
  }

  BasicDecoration compute_artificial_decoration(const NodeMap<Directed, BasicDecoration>& decor,
                                                const std::list<Int>& max_nodes) const
  {
    BasicDecoration data;
    auto max_list = attach_member_accessor( select(decor, max_nodes),
                                            ptr2type<BasicDecoration, Int, &BasicDecoration::rank>()
                                            );
    data.rank = built_dually ?
      accumulate(max_list, operations::min())-1 :
      accumulate(max_list, operations::max())+1 ;
    data.face = artificial_set;
    return data;
  }
};

} } }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
