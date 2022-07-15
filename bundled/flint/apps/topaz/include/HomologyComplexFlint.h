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

#include "polymake/list"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Smith_normal_form.h"
#include "polymake/common/FlintSNF.h"
#include "polymake/GenericStruct.h"
#include "polymake/topaz/HomologyComplex.h"
#include "polymake/topaz/SimplicialComplex_as_FaceMap.h"
#if POLYMAKE_DEBUG
#  include "polymake/client.h"
#  include "polymake/Rational.h"
#  include "polymake/linalg.h"
#endif

namespace polymake { namespace topaz {


template <typename E, typename MatrixType, typename BaseComplex, bool with_cycles, bool dual>
class FlintComplex_iterator {
public:
   typedef std::forward_iterator_tag iterator_category;
   typedef HomologyGroup<E> value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef FlintComplex_iterator iterator;
   typedef FlintComplex_iterator const_iterator;

   typedef SparseMatrix<E> matrix;


   FlintComplex_iterator() {}

   //for cohomology start at the other end
   FlintComplex_iterator(const BaseComplex& complex_arg, Int d_start_arg, Int d_end_arg)
      : complex(&complex_arg),
        d_cur(dual ? d_end_arg : d_start_arg+1),
        d_end(dual ? d_start_arg+1 : d_end_arg)
   {
#if POLYMAKE_DEBUG
      debug_print = get_debug_level();
#endif
      if (!at_end()) {
         first_step(); operator++();
      }
   }

   reference operator* () const { return hom_cur; }
   pointer operator-> () const { return &hom_cur; }

   iterator& operator++ ()
   {
      dual ? ++d_cur : --d_cur;
      if (!at_end()) {
         hom_cur=hom_next; step();
      }
      return *this;
   }
   const iterator operator++ (int) { iterator copy=*this;  operator++(); return copy; }

   bool operator== (const iterator& it) const { return d_cur==it.d_cur; }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   bool at_end() const
   {
      return dual ? d_cur>d_end : d_cur<d_end;
   }

   Int dim() const { return d_cur-dual; }


protected:
   const BaseComplex* complex;
   Int d_cur, d_end;
   HomologyGroup<E> hom_cur, hom_next;
   Int rank_cur = 0;
   Bitset elim_rows, elim_cols;
   typename MatrixType::persistent_type delta;

   enum { R_inv_prev=0, L=1, LxR_prev=2, R_inv=3 };

   void first_step();
   void step(bool first=false);

#if POLYMAKE_DEBUG
   Int debug_print;


   void debug1(Int d, const GenericMatrix<MatrixType,E>& _delta, const SparseMatrix<Rational>& _r_delta, const nothing*) const
   {
      cout << "elim[" << d << "]:\n" << std::setw(3) << _delta << endl;
   }
   void debug2(const nothing*) const
   {
      cout << "cancel cols[" << d_cur << "]: " << elim_rows << endl;
   }
   void debug3(const nothing*) const {}
#endif
};
} }
namespace pm {

template <typename E, typename MatrixType, typename BaseComplex, bool with_cycles, bool dual>
struct check_iterator_feature<polymake::topaz::FlintComplex_iterator<E, MatrixType, BaseComplex, with_cycles, dual>, end_sensitive> : std::true_type {};

}
namespace polymake { namespace topaz {

template <typename E, typename MatrixType, typename BaseComplex, bool with_cycles, bool dual>
void FlintComplex_iterator<E,MatrixType,BaseComplex,with_cycles,dual>::first_step()
{ 
   //get bd matrices for first step
      delta=dual ? T(complex->template boundary_matrix<E>(d_cur)) : complex->template boundary_matrix<E>(d_cur);
#if POLYMAKE_DEBUG
   if (debug_print) {
      cout << (dual ? "dual delta[" : "delta[") << d_cur << "]:\n";
      if (debug_print > 1)
         cout << std::setw(3) << delta << endl;
      else
         cout << delta.rows() << "x" << delta.cols() << endl;
   }
#endif


   using elim_logger_type = nothing_logger;

   


   //elim_ones optimization only works for matrices with entries from {0,+1,-1}
   if(pm::is_derived_from_instance_of<BaseComplex,SimplicialComplex_as_FaceMap>::value)
      rank_cur=eliminate_ones(delta, elim_rows, elim_cols, elim_logger_type(nullptr,nullptr));


   step(true);
}

//each step calculates (co-)homology (and cycles) for the current dimension
template <typename E, typename MatrixType, typename BaseComplex, bool with_cycles, bool dual>
void FlintComplex_iterator<E,MatrixType,BaseComplex,with_cycles,dual>::step(bool first)
{

   typename MatrixType::persistent_type delta_next;
   

   Int rank_next = 0;
   if (d_cur != d_end) {
      if (dual) {
         delta_next = T(complex->template boundary_matrix<E>(d_cur+1));
      } else {
         delta_next = complex->template boundary_matrix<E>(d_cur-1);
      }
#if POLYMAKE_DEBUG
      if (debug_print) {
         cout << (dual ? "dual delta[" : "delta[") << (dual ? d_cur+1 : d_cur-1) << "]:\n";
         if (debug_print > 1)
            cout << std::setw(3) << delta_next << endl;
         else
            cout << delta_next.rows() << "x" << delta_next.cols() << endl;
         cout << "cancel rows[" << (dual ? d_cur+1 : d_cur-1) << "]: " << elim_cols << endl;
      }
#endif
      delta_next.minor(elim_cols,All).clear();



      using elim_logger_type = nothing_logger; 
      
//elim_ones optimization only works for matrices with entries from {0,+1,-1}
      if(pm::is_derived_from_instance_of<BaseComplex,SimplicialComplex_as_FaceMap>::value)
         rank_next=eliminate_ones(delta_next, elim_rows, elim_cols, elim_logger_type(nullptr,nullptr)); 
      


      delta.minor(All,elim_rows).clear();

   }
#if POLYMAKE_DEBUG
   if (debug_print) {
      cout << "calling flint with " << delta.rows() << "x" << delta.cols()<< endl;
   }
#endif
   const SparseMatrix<Integer> snf = common::smith_normal_form_flint(delta);
#if POLYMAKE_DEBUG
   if (debug_print) {
      cout << "  flint done: " << snf.rows() << "x" << snf.cols() << endl;
   }
#endif
   hom_next.torsion.clear();

   for (const auto& t : snf.diagonal()) {
      if (abs(t) == 0){
         break;
      } else {
         ++rank_cur;
         if (abs(t) > 1) {
            hom_next.torsion.push_back(std::pair<Integer, Int>(t, 1));
         }
      }	
   }

#if POLYMAKE_DEBUG
   if (debug_print) {
      cout << "snf[" << d_cur << "]:\n";
      if (debug_print > 1)
         cout << std::setw(3) << delta << endl;
      else
         cout << delta.rows() << "x" << delta.cols() << endl;
   }
#endif
   hom_next.betti_number=-rank_cur;
   if (!first) {
      //for cols of delta that are empty, delete corresponding cols in cLxR_prev
      hom_cur.betti_number+=snf.rows()-rank_cur;
      pm::compress_torsion(hom_cur.torsion);
   }
   delta=delta_next;
   rank_cur=rank_next;

}


//this class manages efficient homology computation of arbitrary chain complexes (e.g. simplicial complexes).
template <typename R, typename MatrixType, typename BaseComplex>
class HomologyComplexFlint {
protected:
   const BaseComplex& complex;
   Int dim_high, dim_low;
public:
   explicit HomologyComplexFlint(const BaseComplex& complex_arg,
                                 Int dim_high_arg = -1, Int dim_low_arg = 0)
      : complex(complex_arg), dim_high(dim_high_arg), dim_low(dim_low_arg)
   {
      Int d = dim();
      if (dim_high<0) dim_high+=d+1;
      if (dim_low<0) dim_low+=d+1;
      if (dim_high<dim_low || dim_high>d || dim_low<0)
         throw std::runtime_error("HomologyComplexFlint - dimensions out of range");
   }

   Int dim() const { return complex.dim(); }
   Int size() const { return dim_high-dim_low+1; }
   const BaseComplex& get_complex() const { return complex; }

   typedef HomologyGroup<R> homology_type;
   typedef CycleGroup<R> cycle_type;

   template <bool with_cycles, bool dual> class as_container;

   friend const as_container<false,false>& homologies(const HomologyComplexFlint& cc)
   {
      return reinterpret_cast<const as_container<false,false>&>(cc);
   }

   friend const as_container<true,false>& homologies_and_cycles(const HomologyComplexFlint& cc)
   {
      return reinterpret_cast<const as_container<true,false>&>(cc);
   }

   friend const as_container<false,true>& cohomologies(const HomologyComplexFlint& cc)
   {
      return reinterpret_cast<const as_container<false,true>&>(cc);
   }

   friend const as_container<true,true>& cohomologies_and_cocycles(const HomologyComplexFlint& cc)
   {
      return reinterpret_cast<const as_container<true,true>&>(cc);
   }
};

template <typename R, typename MatrixType, typename BaseComplex>
template <bool with_cycles, bool dual>
class HomologyComplexFlint<R,MatrixType,BaseComplex>::as_container : public HomologyComplexFlint<R,MatrixType,BaseComplex> {
protected:
   as_container();
   ~as_container();
public:
   typedef FlintComplex_iterator<R, MatrixType, BaseComplex, with_cycles, dual> iterator;
   typedef iterator const_iterator;
   typedef typename iterator::value_type value_type;
   typedef typename iterator::reference reference;
   typedef reference const_reference;

   iterator begin() const
   {
      return iterator(complex,dim_high,dim_low);
   }
   iterator end() const
   {
      return iterator(complex,dim_low-1,dim_low);
   }
};

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
