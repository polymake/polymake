/* Copyright (c) 1997-2019
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

#include "polymake/hash_map"
#include "polymake/FaceMap.h"
#include "polymake/Bitset.h"
#include <sstream>

namespace polymake { namespace topaz {

template <typename Complex, typename Set>
bool adj_numbering(Complex& C, const Set& V)
{
   if (V.empty())
      return false;

   const bool renumber= V.front()!=0 || V.back()+1!=V.size();

   if (renumber) {
      hash_map<int, int> vertex_map(V.size());
      int count=0;
      for (auto s_it=entire(V); !s_it.at_end(); ++s_it, ++count)
         vertex_map[*s_it]=count;

      for (auto c_it=entire(C); !c_it.at_end(); ++c_it) {
         typename Complex::value_type f;
         for (auto s_it=entire(*c_it); !s_it.at_end(); ++s_it)
            f += vertex_map[*s_it];
         *c_it = f;
      }
   }

   return renumber;
}

template <typename OutputIterator>
bool is_pseudo_manifold(const Lattice<BasicDecoration>& HD, bool known_pure, OutputIterator boundary_consumer, int *bad_face_p)
{
   if (HD.in_degree(HD.top_node())==0)
      return true;

   if (!known_pure && !is_pure(HD)) {
      if (bad_face_p) *bad_face_p=-1;
      return false;
   }

   for (const auto n : HD.nodes_of_rank(HD.rank()-2)) {
      const int d = HD.out_degree(n);
      if (d > 2) {
         if (bad_face_p) *bad_face_p=n;
         return false;
      }
      if (!is_derived_from_instance_of<OutputIterator, pm::black_hole>::value && d == 1)
         *boundary_consumer++ = HD.face(n);
   }

   return true;
}

// return values: 1=true, 0=false, -1=undef
template <typename Complex, int d>
int is_ball_or_sphere(const Complex& C, int_constant<d>)
{
   if (POLYMAKE_DEBUG) {
      if (C.empty())
         throw std::runtime_error("is_ball_or_sphere: empty complex");
   }
   // compute the vertex set and test whether C is a pure d-complex
   Set<int> V;
   for (auto c_it=entire(C); !c_it.at_end(); ++c_it) {
      V += *c_it;
      if (POLYMAKE_DEBUG) {
         if (c_it->size() > d+1) {
            std::ostringstream err;
            pm::wrap(err) << "is_ball_or_sphere: Dimension of " << *c_it << " is greater than " << d;
            throw std::runtime_error(err.str());
         }
      }
      if (c_it->size()!=d+1)  // complex is not pure
         return 0;
   }
   return is_ball_or_sphere(C, V, int_constant<d>());
}

// return values: 1=true, 0=false, -1=undef
template <typename Complex, int d>
int is_manifold(const Complex& C, int_constant<d>, int* bad_link_p)
{
   if (POLYMAKE_DEBUG) {
      if (C.empty())
         throw std::runtime_error("is_manifold: empty complex");
   }
   // compute the vertex set and test whether C is a pure 1-complex
   Set<int> V;
   for (auto c_it=entire(C); !c_it.at_end(); ++c_it) {
      V+=*c_it;
      if (POLYMAKE_DEBUG) {
         if (c_it->size() > d+1) {
            std::ostringstream err;
            err << "is_manifold: Dimension of " << *c_it << " is greater than " << d;
            throw std::runtime_error(err.str());
         }
      }
      if (c_it->size()!=d+1) { // complex is not pure
         if (bad_link_p) *bad_link_p=-1;
         return 0;
      }
   }
   return is_manifold(C, V, int_constant<d>(), bad_link_p);
}

// return values: 1=true, 0=false, -1=undef
template <typename Complex, typename VertexSet, int d>
int is_manifold(const Complex& C, const GenericSet<VertexSet>& V, int_constant<d>, int* bad_link_p)
{
   // iterate over the vertices and test if their links are (d-1)-balls or (d-1)-spheres
   for (auto it=entire(V.top()); !it.at_end(); ++it) {
      const int bos=is_ball_or_sphere(link(C, scalar2set(*it)), int_constant<d-1>());
      if (bos<=0) { // false or undef
         if (bad_link_p) *bad_link_p=*it;
         return bos;
      }
   }
   return 1;
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
