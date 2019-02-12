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

#ifndef POLYMAKE_TOPAZ_SIMPLICIAL_COMPLEX_AS_FACE_MAP_H
#define POLYMAKE_TOPAZ_SIMPLICIAL_COMPLEX_AS_FACE_MAP_H

#include "polymake/FaceMap.h"
#include "polymake/PowerSet.h"
#include "polymake/Bitset.h"
#include "polymake/SparseMatrix.h"
#include "polymake/vector"
#include <stdexcept>

namespace polymake { namespace topaz {

template <typename Vertex>
class SimplexEnumerator {
protected:
   std::vector<int> dim_cnt;

   void resize(int d) { dim_cnt.resize(d+1, 0); }

   int visit(int& face_id, int d)
   {
      if (face_id<0) face_id=dim_cnt[d]++;
      return face_id;
   }

   SimplexEnumerator() : dim_cnt(1,0) {}
public:
   typedef pm::face_map::index_traits<Vertex> face_traits;

   int dim() const { return dim_cnt.size()-1; }
   int size() const { return accumulate(dim_cnt, operations::add()); }
   int size_of_dim(int d) const { return dim_cnt[d]; }
};

template <typename Vertex=int, typename Visitor=SimplexEnumerator<Vertex> >
class SimplicialComplex_as_FaceMap
   : public FaceMap< typename Visitor::face_traits >,
     public Visitor {
protected:
   typedef FaceMap< typename Visitor::face_traits > super;

   Bitset faces_complete;

   template <typename Iterator>
   void insert_faces(Iterator src, int d)
   {
      for (; !src.at_end(); ++src)
         insert_face(*src, d);
   }

   template <typename SetTop>
   int insert_face(const GenericSet<SetTop, Vertex, operations::cmp>& f, int d)
   {
      return this->visit((*this)[f], d);
   }
public:
   typedef Vertex vertex_type;
   typedef typename super::iterator iterator;
   typedef typename super::const_iterator const_iterator;
   typedef typename super::reference reference;
   typedef typename super::const_reference const_reference;

   SimplicialComplex_as_FaceMap()
      : faces_complete(scalar2set(0)) {}

   template <typename Container>
   SimplicialComplex_as_FaceMap(const Container& src,
                                typename std::enable_if<pm::isomorphic_to_container_of<Container, Set<Vertex> >::value, nothing*>::type=nullptr)
      : faces_complete(scalar2set(0))
   {
      for (auto f=entire(src); !f.at_end(); ++f)
         insert_face(*f);
   }

   template <typename Iterator>
   void insert_faces(Iterator src)
   {
      for (; !src.at_end(); ++src)
         insert_face(*src);
   }

   template <typename SetTop>
   int insert_face(const GenericSet<SetTop, Vertex, operations::cmp>& f)
   {
      int d=f.top().size()-1;
      if (d<0) return -1;       // ignore empty sets
      if (! faces_complete.contains(d)) {
         if (d>dim()) {
            Visitor::resize(d);
            faces_complete = scalar2set(d);
         }
      }
      return insert_face(f,d);
   }

   int size() const
   {
      int n=Visitor::size();
      if (n<0) n=super::size();
      return n;
   }

   int size_of_dim(int d) const
   {
      if (d<0) d+=dim()+1;
      int n=Visitor::size_of_dim(d);
      if (n<0) n=super::faces_of_dim(d);
      return n;
   }

   int dim() const
   {
      int n=Visitor::dim();
      if (n<0) n=faces_complete.back();
      return n;
   }

   class Faces_of_Dim {
   protected:
      const SimplicialComplex_as_FaceMap* master;
      int d;

      Faces_of_Dim(const SimplicialComplex_as_FaceMap* master_arg, int d_arg)
         : master(master_arg), d(d_arg) {}

      friend class SimplicialComplex_as_FaceMap;
   public:
      typedef typename super::iterator iterator;
      typedef iterator const_iterator;
      typedef typename iterator::value_type value_type;
      typedef typename iterator::reference reference;
      typedef reference const_reference;

      iterator begin() const { return master->super::begin_of_dim(d); }
      iterator end() const { return master->super::end_of_dim(d); }

      int size() const { return master->size_of_dim(d); }
      bool empty() const { return size()==0; }
   };

   /** Select the faces of dimension d only.
       d<0 means co-dimension.
       The implicitly included lower-dimensional faces are not built automatically,
       call complete_facets(d) before if you want to access them all!
   */
   Faces_of_Dim faces_of_dim(int d) const
   {
      if (d<0) d+=dim()+1;
      if (POLYMAKE_DEBUG) {
         if (d<0 || d>dim())
            throw std::runtime_error("SimplicialComplex_as_FaceMap::faces_of_dim - dimension out of range");
      }
      return Faces_of_Dim(this,d);
   }
private:
   template <typename InputCursor>
   void _input(InputCursor& is)
   {
      Set<Vertex> f;
      while (!is.at_end()) {
         is >> f;
         insert_face(f);
      }
   }

   template <typename Input>
   void input(Input& is)
   {
      this->clear();
      _input(is.begin_list((pm::array_traits< Set<Vertex> >*)0).set_option());
   }
public:
   template <typename Input> friend
   Input& operator>> (GenericInput<Input>& is, SimplicialComplex_as_FaceMap& sc)
   {
      sc.input(is.top());
      return is.top();
   }

   // The complex is not changed as a geometrical object, only some low-dimensional faces become explicit.
   // Therefore these public methods are declared `const'.

   void complete_faces(int d) const
   {
      if (d<0) d+=dim()+1;
      if (POLYMAKE_DEBUG) {
         if (d<0 || d>dim())
            throw std::runtime_error("SimplicialComplex_as_FaceMap::complete_faces - dimension out of range");
      }
      const_cast<SimplicialComplex_as_FaceMap*>(this)->_complete_faces(d);
   }

   void complete_faces(int d_high, int d_low) const
   {
      if (d_high<0) d_high+=dim()+1;
      if (d_low<0) d_low+=dim()+1;
      if (POLYMAKE_DEBUG) {
         if (d_high<d_low || d_low<0 || d_high>dim())
            throw std::runtime_error("SimplicialComplex_as_FaceMap::complete_faces - invalid dimension range");
      }
      const_cast<SimplicialComplex_as_FaceMap*>(this)->_complete_faces(d_high,d_low);
   }

   template <typename R>
   SparseMatrix<R> boundary_matrix(int d) const
   {
      if (d<0) d+=dim()+1;
      if (POLYMAKE_DEBUG) {
         if (d<0 || d>dim()+1)
            throw std::runtime_error("SimplicialComplex_as_FaceMap::boundary_matrix - dimension out of range");
      }
      return const_cast<SimplicialComplex_as_FaceMap*>(this)->template _boundary_matrix<R>(d);
   }

protected:
   void _complete_faces(int d);
   void _complete_faces(int d_high, int d_low);
   template <class R>
   SparseMatrix<R> _boundary_matrix(int d);
};

template <typename Vertex, typename Visitor>
void SimplicialComplex_as_FaceMap<Vertex, Visitor>::_complete_faces(int d)
{
   if (faces_complete.contains(d)) return;

   int k=d+1, d_above=k;
   while (!faces_complete.contains(d_above)) ++d_above;
   for (iterator face=super::begin_of_dim(d_above); !face.at_end(); ++face) {
      insert_faces(entire(all_subsets_of_k(*face,k)), d);
   }
   faces_complete += d;
}

template <typename Vertex, typename Visitor>
void SimplicialComplex_as_FaceMap<Vertex, Visitor>::_complete_faces(int d_high, int d_low)
{
   _complete_faces(d_high);

   for (--d_high; d_high>=d_low; --d_high) {
      if (! faces_complete.contains(d_high)) {
         for (iterator face=super::begin_of_dim(d_high+1); !face.at_end(); ++face) {
            insert_faces(entire(all_subsets_less_1(*face)), d_high);
         }
         faces_complete += d_high;
      }
   }
}

template <typename Vertex, typename Visitor> template <typename R>
SparseMatrix<R> SimplicialComplex_as_FaceMap<Vertex, Visitor>::_boundary_matrix(int d)
{
   if (d>dim()) return zero_matrix<R>(1, this->dim_cnt[d-1]);
   _complete_faces(d);
   if (d==0) return ones_matrix<R>(this->dim_cnt[0], 1);

   RestrictedSparseMatrix<R> Delta(this->dim_cnt[d]);
   for (iterator face=super::begin_of_dim(d); !face.at_end(); ++face) {
      int r=face.data();
      R entry=R(1);
      for (auto face_below=entire(all_subsets_less_1(*face)); !face_below.at_end(); ++face_below) {
         int c=insert_face(*face_below, d-1);
         Delta(r,c)=entry;
         entry=-entry;
      }
   }
   faces_complete += d-1;
   return SparseMatrix<R>(std::move(Delta));
}

} }
namespace pm {

template <typename Vertex, typename Visitor>
struct spec_object_traits< polymake::topaz::SimplicialComplex_as_FaceMap<Vertex,Visitor> >
   : spec_object_traits<is_container> {};

}

#endif // POLYMAKE_TOPAZ_SIMPLICIAL_COMPLEX_AS_FACE_MAP_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
