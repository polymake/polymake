/* Copyright (c) 1997-2014
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

#include "polymake/hash_map"
#include "polymake/FaceMap.h"
#include "polymake/Bitset.h"
#include <sstream>

namespace polymake { namespace topaz {

template <typename Complex>
HasseDiagram hasse_diagram(const Complex& C, const int d, const int end_dim)
{
   // sort facets according to their dimension
   Array< std::list< Set<int> > > facets_of_dim(d+1);
   int dim=-1;

   for (typename Entire<Complex>::const_iterator c_it=entire(C); !c_it.at_end(); ++c_it) {
      typename Complex::value_type f=*c_it;
      while (f.size() > facets_of_dim.size())
         facets_of_dim.resize(2*facets_of_dim.size());
      if (f.size()-1 > dim)
         dim = f.size()-1;

      facets_of_dim[f.size()-1].push_back(f);
   } 

   // fill the hasse diagram
   HasseDiagram HD;
   HasseDiagram::_filler HD_filler(HD,false);
   HD_filler.add_node(sequence(0,0));           // node #0 is the top node representing the whole complex
   HD_filler.increase_dim();
   int face_end_this_dim=1, face_index=1;
  
   if (C.empty())                                // if c is empty, then HD is a single node only
      return HD;
  
   FaceMap<> FM;
   const int last_level= end_dim<0 ? dim + end_dim : end_dim;
   for (int d=dim; d>=last_level; --d) {

      // add facets of dimension d
      int i=face_end_this_dim;
      for (Entire< std::list< Set<int> > >::const_iterator f_it=entire(facets_of_dim[d]);
           !f_it.at_end(); ++f_it, ++i) {
         HD_filler.add_node(*f_it);
         HD_filler.add_edge(i,0);
      }

      // add faces of dimension d which are not a facet
      if (d < dim) {
         while (face_index < face_end_this_dim) {
            const Subsets_less_1< Set<int> > faces_1_below(HD.face(face_index));
            // add faces one below
            for (Entire< Subsets_less_1< Set<int> > >::const_iterator fbi=entire(faces_1_below);
                 !fbi.at_end(); ++fbi) {
               int &face = FM[*fbi];
               if (face==-1) {
                  face=HD_filler.add_node(*fbi);
               }
               HD_filler.add_edge(face, face_index);
            }
            ++face_index;
         }
      }

      face_end_this_dim=HD.nodes();
      HD_filler.increase_dim();
   }
  
   HD_filler.add_node(sequence(0,0));  // the bottom node (empty face)
   while (face_index < face_end_this_dim) {
      HD_filler.add_edge(face_end_this_dim, face_index);
      ++face_index;
   }
  
   return HD;
}

template <typename Complex>
HasseDiagram pure_hasse_diagram(const Complex& C, const int end_dim)
{
   // works for pure complexes only
   HasseDiagram HD;
   HasseDiagram::_filler HD_filler(HD,false);
   HD_filler.add_node(sequence(0,0));           // node #0 is the top node representing the whole complex
   HD_filler.increase_dim();

   if (C.empty())                                // if c is empty, the HD is a single node only
      return HD;

   const int dim=C.front().size()-1;

   // add facets of C
   HD_filler.add_nodes(C.size(), C.begin());
   HD_filler.increase_dim();
   int face_index=1, face_end_this_dim=C.size()+1;
   for (int i=1; i<face_end_this_dim; ++i)
      HD_filler.add_edge(i,0);
  
   // add faces of dimension d= dim-1,..0
   FaceMap<> FM;

   const int last_level= end_dim<0 ? dim + end_dim : end_dim;
   for (int d=dim-1; d>=last_level; --d) { 
      while (face_index < face_end_this_dim) {
         const Subsets_less_1< Set<int> > faces_1_below(HD.face(face_index));
         // add faces one below
         for (Entire< Subsets_less_1< Set<int> > >::const_iterator fbi=entire(faces_1_below);
              !fbi.at_end(); ++fbi) {
            int &face = FM[*fbi];
            if (face==-1) {
               face=HD_filler.add_node(*fbi);
            }
            HD_filler.add_edge(face, face_index);
         }
         ++face_index;
      }
      face_end_this_dim=HD.nodes();
      HD_filler.increase_dim();
   }

   HD_filler.add_node(sequence(0,0));  // the bottom node (empty face)
   while (face_index < face_end_this_dim) {
      HD_filler.add_edge(face_end_this_dim, face_index);
      ++face_index;
   }

   return HD;
}

template <typename Complex, typename Set>
bool adj_numbering(Complex& C, const Set& V)
{
   if (V.empty())
      return false;

   const bool renumber= V.front()!=0 || V.back()+1!=V.size();

   if (renumber) {
      hash_map<int, int> vertex_map(V.size());
      int count=0;
      for (typename Entire< Set >::const_iterator s_it=entire(V); !s_it.at_end(); ++s_it, ++count)
         vertex_map[*s_it]=count;
      
      for (typename Entire<Complex>::iterator c_it=entire(C); !c_it.at_end(); ++c_it) {
         typedef typename Complex::value_type Facet;
         Facet f; 
         for (typename Entire<Facet>::iterator s_it=entire(*c_it); !s_it.at_end(); ++s_it)
            f += vertex_map[*s_it];
         *c_it = f;
      }
   }

   return renumber;
}

template <typename OutputIterator>
bool is_pseudo_manifold(const HasseDiagram& HD, bool known_pure, OutputIterator boundary_consumer, int *bad_face_p)
{
   if (HD.in_degree(HD.top_node())==0)
      return true;

   if (!known_pure && !is_pure(HD)) {
      if (bad_face_p) *bad_face_p=-1;
      return false;
   }

   for (typename Entire<HasseDiagram::nodes_of_dim_set>::const_iterator it=entire(HD.nodes_of_dim(-2)); !it.at_end(); ++it) {
      const int d = HD.out_degree(*it);
      if ( d > 2 ) {
         if (bad_face_p) *bad_face_p=*it;
         return false;
      }
      if (!pm::derived_from_instance<OutputIterator, pm::black_hole>::value && d == 1 )
         *boundary_consumer++ = HD.face(*it);
   }

   return true;
}

// return values: 1=true, 0=false, -1=undef
template <typename Complex, int d>
int is_ball_or_sphere(const Complex& C, int2type<d>)
{
   if (POLYMAKE_DEBUG) {
      if (C.empty())
         throw std::runtime_error("is_ball_or_sphere: empty complex");
   }
   // compute the vertex set and test whether C is a pure d-complex
   Set<int> V;
   for (typename Entire<Complex>::const_iterator c_it=entire(C); !c_it.at_end(); ++c_it) {
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
   return is_ball_or_sphere(C,V,int2type<d>());
}

// return values: 1=true, 0=false, -1=undef
template <typename Complex, int d>
int is_manifold(const Complex& C, int2type<d>, int *bad_link_p)
{
   if (POLYMAKE_DEBUG) {
      if (C.empty())
         throw std::runtime_error("is_manifold: empty complex");
   }
   // compute the vertex set and test whether C is a pure 1-complex
   Set<int> V;
   for (typename Entire<Complex>::const_iterator c_it=entire(C); !c_it.at_end(); ++c_it) {
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
   return is_manifold(C,V,int2type<d>(),bad_link_p);
}

// return values: 1=true, 0=false, -1=undef
template <typename Complex, typename VertexSet, int d>
int is_manifold(const Complex& C, const GenericSet<VertexSet>& V, int2type<d>, int *bad_link_p)
{
   // iterate over the vertices and test if their links are (d-1)-balls or (d-1)-spheres
   for (typename Entire<VertexSet>::const_iterator it=entire(V.top()); !it.at_end(); ++it) {
      const int bos=is_ball_or_sphere(link(C,scalar2set(*it)), int2type<d-1>());
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
