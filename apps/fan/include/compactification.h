/* Copyright (c) 1997-2022
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
#include "polymake/Matrix.h"
#include "polymake/Map.h"
#include "polymake/Set.h"
#include "polymake/linalg.h"
#include "polymake/graph/Decoration.h"
#include "polymake/graph/Lattice.h"
#include "polymake/fan/hasse_diagram.h"

namespace polymake { namespace fan{
namespace compactification {
	
using graph::Lattice;
using namespace graph::lattice;
using namespace fan::lattice;

struct IteratorWrap {
  FacetList groundSet;
  FacetList::const_iterator state;

  IteratorWrap(FacetList&& gs)
    : groundSet(std::move(gs))
    , state(groundSet.begin()) {}

  Set<Int> operator*()
  {
    return *state;
  }

  IteratorWrap& operator++()
  {
    ++state;
    return *this;
  }

  bool at_end()
  {
    return state == groundSet.end();
  }
};


template <typename DecorationType, typename Scalar>
class CellularClosureOperator {
private:
  FaceMap<> face_index_map;
  Map<Int, Set<Int>> int2vertices;
  Map<Set<Int>, Int> vertices2int;
  Int nVertices;
  Set<Int> farVertices;
  Matrix<Scalar> vertices;
  Lattice<BasicDecoration, Nonsequential> oldHasseDiagram;

public:
  typedef Set<Int> ClosureData;

  CellularClosureOperator(BigObject pc)
  {
    pc.give("FAR_VERTICES") >> farVertices;
    pc.give("VERTICES") >> vertices;
    pc.give("HASSE_DIAGRAM") >> oldHasseDiagram;
    nVertices = vertices.rows();
    Set<Int> topNode{-1};
    Int i = 0;

    // Build new vertices
    for (const auto& f : oldHasseDiagram.decoration()) {
      if (f.face != topNode) { 
        Int faceDim = f.rank-1;
        Int tailDim = rank(vertices.minor(f.face * farVertices, All));
        if (faceDim == tailDim) {
          int2vertices[i] = f.face;
          vertices2int[f.face] = i;
          ++i;
        }
      }
    }
  }

  Set<Int> old_closure(const Set<Int>& a) const
  {
    // We find a closure of a face in the old Hasse diagram by starting
    // at the top node and then descending into lower nodes whenever
    // they contain the given set of vertices. If no further descent is
    // possible, we terminate.
    Int currentNode = oldHasseDiagram.top_node();
    const Graph<Directed>& G(oldHasseDiagram.graph());
    bool found = true;
    while (found) {
      found = false;
      for (const auto p : G.in_adjacent_nodes(currentNode)) {
        const BasicDecoration& decor = oldHasseDiagram.decoration(p);
        if (incl(a, decor.face) <= 0) {
          found = true;
          currentNode = p;
          break;
        }
      }
    }
    return oldHasseDiagram.decoration(currentNode).face;
  }

  Set<Int> closure(const Set<Int>& a) const
  {
    Set<Int> originalRealisation;
    for (const auto i : a) {
      originalRealisation += int2vertices[i];
    }
    Set<Int> originalClosure = old_closure(originalRealisation);
    Set<Int> commonRays = originalRealisation * farVertices;
    for (const auto i : a) {
      commonRays = commonRays * int2vertices[i];
    }
    Set<Int> result;
    for (const auto& v : vertices2int) {
      if (incl(commonRays, v.first) <= 0 && incl(v.first, originalClosure) <= 0) {
        result += v.second;
      }
    }
    return result;
  }

  Set<Int> closure_of_empty_set()
  {
    return Set<Int>{};
  }

  FaceIndexingData get_indexing_data(const ClosureData& data)
  {
    Int& fi = face_index_map[data];
    return FaceIndexingData(fi, fi == -1, fi == -2);
  }

  Set<Int> compute_closure_data(const DecorationType& bd) const
  {
    return bd.face;
  }

  IteratorWrap get_closure_iterator(const Set<Int>& face) const
  {
    Set<Int> toadd = sequence(0, int2vertices.size())-face;
    FacetList result;
    for (auto i : toadd) {
      result.insertMin(closure(face+i));
    }
    return IteratorWrap(std::move(result));
  }

  const Map<Int, Set<Int>>& get_int2vertices() const
  {
    return int2vertices;
  }

  const Set<Int>& get_farVertices() const
  {
    return farVertices;
  }
};
   
   
struct SedentarityDecoration
  : public GenericStruct<SedentarityDecoration> {
     DeclSTRUCT( DeclFIELD(face, Set<Int>)
                 DeclFIELD(rank, Int)
                 DeclFIELD(realisation, Set<Int>) 
                 DeclFIELD(sedentarity, Set<Int>) );

     SedentarityDecoration() {}
     SedentarityDecoration(const Set<Int>& f, Int r, const Set<Int>& re, const Set<Int>& se)
       : face(f)
       , rank(r)
       , realisation(re)
       , sedentarity(se) {}
};

class SedentarityDecorator {
private:
  const Map<Int, Set<Int>>& int2vertices;
  const Set<Int>& farVertices;

  Set<Int> realisation(const Set<Int>& face) const
  {
    Set<Int> result;
    for (const auto& e : face) {
      result += int2vertices[e];
    }
    return result;
  }

  Set<Int> sedentarity(const Set<Int>& face) const
  {
    if (face.size() == 0) {
      return Set<Int>{};
    }
    Set<Int> result(farVertices);
    for (const auto& e:face){
      result *= int2vertices[e];
    }
    return result;
  }

public:
  typedef SedentarityDecoration DecorationType;
  SedentarityDecorator(const Map<Int, Set<Int>>& i2v, const Set<Int>& fv)
    : int2vertices(i2v)
    , farVertices(fv) {}

  SedentarityDecoration compute_initial_decoration(const Set<Int>& face) const
  {
    return SedentarityDecoration(face, 0, realisation(face), sedentarity(face));
  }

  SedentarityDecoration compute_decoration(const Set<Int>& face, const SedentarityDecoration& bd) const
  {
    return SedentarityDecoration(face, bd.rank+1, realisation(face), sedentarity(face));
  }

  SedentarityDecoration compute_artificial_decoration(const NodeMap<Directed, SedentarityDecoration>& decor, const std::list<Int>& max_faces) const
  {
    const Set<Int> D{-1};
    Int rank = 0;
    for (const auto& mf : max_faces) {
      assign_max(rank, decor[mf].rank);
    }
    ++rank;
    return SedentarityDecoration(D, rank, D, Set<Int>{});
  }
};

} } }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
