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

#ifndef POLYMAKE_TOPAZ_CHAIN_COMPLEX_H
#define POLYMAKE_TOPAZ_CHAIN_COMPLEX_H

#include "polymake/list"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Smith_normal_form.h"
#include "polymake/GenericStruct.h"
#if POLYMAKE_DEBUG
#  include "polymake/client.h"
#  include "polymake/Rational.h"
#  include "polymake/linalg.h"
#endif

namespace polymake { namespace topaz {

template <typename E>
class HomologyGroup : public GenericStruct< HomologyGroup<E> > {
public:
   typedef std::list< std::pair<E,int> > torsion_list;
   DeclSTRUCT( DeclTemplFIELD(torsion, torsion_list)
               DeclTemplFIELD(betti_number, int) );
};

template <typename E>
class CycleGroup : public GenericStruct< CycleGroup<E> > {
public:
   typedef SparseMatrix<E> coeff_matrix;
   typedef Array< Set<int> > face_list;
   DeclSTRUCT( DeclTemplFIELD(coeffs, coeff_matrix)
               DeclTemplFIELD(faces, face_list) );
};

class nothing_logger : public pm::dummy_companion_logger {
public:
   explicit nothing_logger(const nothing*, const nothing*, const nothing* =0, const nothing* =0) {}
};

template <typename E>
class elimination_logger : public pm::SNF_companion_logger<E,false> {
   // R is inverted
   typedef pm::SNF_companion_logger<E,false> super;
public:
   elimination_logger(SparseMatrix<E> *L, SparseMatrix<E> *Rinv) : super(L,Rinv) {}

   template <typename Matrix>
   void from_right(const Matrix& U) const { this->R->multiply_from_left(this->inv(U)); }
};

template <typename E>
class Smith_normal_form_logger
   : public pm::SNF_companion_logger<E,false> {
protected:
   typedef pm::SNF_companion_logger<E,false> super;
   elimination_logger<E> elim2;
public:
   Smith_normal_form_logger(SparseMatrix<E> *L, SparseMatrix<E> *L2, SparseMatrix<E> *R, SparseMatrix<E> *Rinv)
      : super(L,R), elim2(L2, Rinv) {}

   template <typename Matrix>
   void from_left(const Matrix& U) const  { super::from_left(U);  elim2.from_left(U); }
   template <typename Matrix>
   void from_right(const Matrix& U) const { if (super::R) super::from_right(U), elim2.from_right(U); }
};

template <typename E, typename BaseComplex, bool with_cycles, bool dual>
class ChainComplex_iterator {
public:
   typedef std::forward_iterator_tag iterator_category;
   typedef HomologyGroup<E> value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef ChainComplex_iterator iterator;
   typedef ChainComplex_iterator const_iterator;

   typedef SparseMatrix<E> matrix;
   typedef typename pm::if_else<with_cycles, matrix, nothing>::type companion_type;

   ChainComplex_iterator() {}

   ChainComplex_iterator(const BaseComplex& complex_arg, int d_start_arg, int d_end_arg)
      : complex(&complex_arg),
        d_cur(dual ? d_end_arg : d_start_arg+1),
        d_end(dual ? d_start_arg+1 : d_end_arg)
   {
#if POLYMAKE_DEBUG
      debug_print = perl::get_debug_level() > 1;
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

   int dim() const { return d_cur-dual; }
   const companion_type& cycle_coeffs() const { return Cycles; }

protected:
   const BaseComplex *complex;
   int d_cur, d_end;
   HomologyGroup<E> hom_cur, hom_next;
   int rank_cur, rank_next;
   Bitset elim_rows, elim_cols;
   matrix delta;

   static const int R_inv_prev=0, L=1, LxR_prev=2, R_inv=3, companion_set=4;
   typedef matrix matrix_array[companion_set];
   struct nothing_array : nothing {
      const nothing& operator[] (int) const { return *this; }
      nothing* operator+ (int) { return this; }
   };
   typedef typename pm::if_else<with_cycles, matrix_array, nothing_array>::type companion_array;
   companion_array companions;
   companion_type Cycles;

   typedef typename pm::if_else<with_cycles, Smith_normal_form_logger<E>, nothing_logger>::type snf_logger_type;
   typedef typename pm::if_else<with_cycles, elimination_logger<E>, nothing_logger>::type elim_logger_type;

   void first_step();
   void step(bool first=false);

   void init_companion(const nothing*, int) {}
   void init_companion(matrix* M, int n) { *M=unit_matrix<E>(n); }
   void calculate_cycles(const nothing&) {}
   void calculate_cycles(matrix&);
   void prepare_LxR_prev(const nothing*) {}
   void prepare_LxR_prev(matrix*);

#if POLYMAKE_DEBUG
   SparseMatrix<Rational> r_delta, r_delta_next;
   bool debug_print;

   void debug1(int d, const matrix& _delta, const SparseMatrix<Rational>& _r_delta, const matrix* _companions) const
   {
      const SparseMatrix<Rational> r_L(_companions[L]), r_R_inv(_companions[R_inv]);
      cout << "elim[" << d << "]:\n" << std::setw(3) << _delta;
      if (r_L * _r_delta * inv(r_R_inv) != _delta) cout << "WRONG!\n";
      cout << "L:\n" << std::setw(3) << _companions[L];
      if (! abs_equal(det(r_L), 1)) cout << "NOT UNIMODULAR!\n";
      cout << "R_inv:\n" << std::setw(3) << _companions[R_inv];
      if (! abs_equal(det(r_R_inv), 1)) cout << "NOT UNIMODULAR!\n";
      cout << endl;
   }
   void debug2(const matrix*) const
   {
      cout << "cancel cols[" << d_cur << "]: " << elim_rows << endl;
      const SparseMatrix<Rational> r_L(companions[L]), r_R_inv(companions[R_inv]);
      if (r_L * r_delta * inv(r_R_inv) != delta) cout << "WRONG!\n";
      cout << "R_inv:\n" << std::setw(3) << companions[R_inv];
      if (! abs_equal(det(r_R_inv), 1)) cout << "NOT UNIMODULAR!\n";
   }
   void debug3(const matrix*) const
   {
      const SparseMatrix<Rational> r_L(companions[L]), r_R_inv(companions[R_inv]);
      if (r_L * r_delta * inv(r_R_inv) != delta) cout << "WRONG!\n";
      cout << "L:\n" << std::setw(3) << companions[L];
      if (! abs_equal(det(r_L), 1)) cout << "NOT UNIMODULAR!\n";
      cout << "R_inv:\n" << std::setw(3) << companions[R_inv];
      if (! abs_equal(det(r_R_inv), 1)) cout << "NOT UNIMODULAR!\n";
      cout << "LxR_prev:\n" << std::setw(3) << companions[LxR_prev];
   }
   void debug1(int d, const matrix& _delta, const SparseMatrix<Rational>& _r_delta, const nothing*) const
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

template <typename E, typename BaseComplex, bool with_cycles, bool dual>
struct check_iterator_feature<polymake::topaz::ChainComplex_iterator<E,BaseComplex,with_cycles,dual>, end_sensitive>
   : True {};

}
namespace polymake { namespace topaz {

template <typename E, typename BaseComplex, bool with_cycles, bool dual>
void ChainComplex_iterator<E,BaseComplex,with_cycles,dual>::first_step()
{
   if (dual) {
      delta=T(complex->template boundary_matrix<E>(d_cur));
   } else {
      delta=complex->template boundary_matrix<E>(d_cur);
   }
#if POLYMAKE_DEBUG
   if (debug_print) {
      cout << (dual ? "dual delta[" : "delta[") << d_cur << "]:\n" << std::setw(3) << delta << endl;
      r_delta=SparseMatrix<Rational>(delta);
   }
#endif
   init_companion(companions+L, delta.rows());
   init_companion(companions+R_inv, delta.cols());
   rank_cur=eliminate_ones(delta, elim_rows, elim_cols, elim_logger_type(companions+L, companions+R_inv));
   companions[LxR_prev]=companions[L];
#if POLYMAKE_DEBUG
   if (debug_print) debug1(d_cur, delta, r_delta, companions+0);
#endif
   step(true);
}

template <typename E, typename BaseComplex, bool with_cycles, bool dual>
void ChainComplex_iterator<E,BaseComplex,with_cycles,dual>::step(bool first)
{
   companion_array companions_next;
   matrix delta_next;

   companion_type *cLxR_prev=0, *cR_inv=0;
   int rank_next=0;
   if (d_cur!=d_end) {
      if (dual) {
         delta_next=T(complex->template boundary_matrix<E>(d_cur+1));
      } else {
         delta_next=complex->template boundary_matrix<E>(d_cur-1);
      }
#if POLYMAKE_DEBUG
      if (debug_print) {
         cout << (dual ? "dual delta[" : "delta[") << (dual ? d_cur+1 : d_cur-1) << "]:\n" << std::setw(3) << delta_next << endl;
         r_delta_next=SparseMatrix<Rational>(delta_next);
         cout << "cancel rows[" << (dual ? d_cur+1 : d_cur-1) << "]: " << elim_cols << endl;
      }
#endif
      delta_next.minor(elim_cols,All).clear();
      init_companion(companions_next+LxR_prev, delta_next.rows());
      init_companion(companions_next+R_inv, delta_next.cols());
      rank_next=eliminate_ones(delta_next, elim_rows, elim_cols, elim_logger_type(companions+R_inv, companions_next+R_inv));
      companions_next[L]=companions[R_inv];
#if POLYMAKE_DEBUG
      if (debug_print) debug1((dual ? d_cur+1 : d_cur-1), delta_next, r_delta_next, companions_next+0);
#endif
      delta.minor(All,elim_rows).clear();
#if POLYMAKE_DEBUG
      if (debug_print) debug2(companions+0); 
#endif
      cLxR_prev=companions_next+LxR_prev;
      cR_inv=companions+R_inv;
   }

   rank_cur += smith_normal_form(delta, hom_next.torsion,
                                 snf_logger_type(companions+L, companions+LxR_prev, cLxR_prev, cR_inv), pm::False());
#if POLYMAKE_DEBUG
   if (debug_print) {
      cout << "snf[" << d_cur << "]:\n" << std::setw(3) << delta;
      if (cR_inv) debug3(companions+0);
   }
#endif
   hom_next.betti_number=-rank_cur;
   if (!first) {
      prepare_LxR_prev(cLxR_prev);
      hom_cur.betti_number+=delta.rows()-rank_cur;
      calculate_cycles(Cycles);
      pm::compress_torsion(hom_cur.torsion);
   }
   delta=delta_next;
   rank_cur=rank_next;
#if POLYMAKE_DEBUG
   if (debug_print) r_delta=r_delta_next;
#endif
   companions[R_inv_prev]=companions[R_inv];
   companions[L]=companions_next[L];
   companions[LxR_prev]=companions_next[LxR_prev];
   companions[R_inv]=companions_next[R_inv];
}

template <typename E, typename BaseComplex, bool with_cycles, bool dual> inline
void ChainComplex_iterator<E,BaseComplex,with_cycles,dual>::prepare_LxR_prev(matrix *pLxR_prev)
{
   if (pLxR_prev)
      for (typename Entire< Cols<matrix> >::iterator c=entire(cols(delta)); !c.at_end(); ++c)
         if (!c->empty())
            pLxR_prev->col(c.index()).clear();
}

template <typename E, typename BaseComplex, bool with_cycles, bool dual>
void ChainComplex_iterator<E,BaseComplex,with_cycles,dual>::calculate_cycles(matrix& C)
{
   Cycles.resize(hom_cur.betti_number + hom_cur.torsion.size(), delta.rows());
   typename Entire< Rows<matrix> >::iterator r=entire(rows(Cycles));
   for (typename HomologyGroup<E>::torsion_list::iterator t=hom_cur.torsion.begin(), t_end=hom_cur.torsion.end();
        t != t_end; ++t, ++r)
      *r = companions[R_inv_prev].row(t->second);

   typename Rows<matrix>::iterator r_d=rows(delta).begin();
   while (!r.at_end()) {
      while (! r_d->empty()) ++r_d;
      if (! companions[LxR_prev].row(r_d.index()).empty()) {
         *r = companions[L].row(r_d.index());
         ++r;
      }
      ++r_d;
   }
}

template <typename R, typename BaseComplex>
class ChainComplex {
protected:
   const BaseComplex& complex;
   int dim_high, dim_low;
public:
   explicit ChainComplex(const BaseComplex& complex_arg,
                         int dim_high_arg=-1, int dim_low_arg=0)
      : complex(complex_arg), dim_high(dim_high_arg), dim_low(dim_low_arg)
   {
      int d=dim();
      if (dim_high<0) dim_high+=d+1;
      if (dim_low<0) dim_low+=d+1;
      if (dim_high<dim_low || dim_high>d || dim_low<0)
         throw std::runtime_error("ChainComplex - dimensions out of range");
   }

   int dim() const { return complex.dim(); }
   int size() const { return dim_high-dim_low+1; }
   const BaseComplex& get_complex() const { return complex; }

   typedef HomologyGroup<R> homology_type;
   typedef CycleGroup<R> cycle_type;

   template <bool with_cycles, bool dual> class as_container;

   friend const as_container<false,false>& homologies(const ChainComplex& cc)
   {
      return reinterpret_cast<const as_container<false,false>&>(cc);
   }

   friend const as_container<true,false>& homologies_and_cycles(const ChainComplex& cc)
   {
      return reinterpret_cast<const as_container<true,false>&>(cc);
   }

   friend const as_container<false,true>& cohomologies(const ChainComplex& cc)
   {
      return reinterpret_cast<const as_container<false,true>&>(cc);
   }

   friend const as_container<true,true>& cohomologies_and_cocycles(const ChainComplex& cc)
   {
      return reinterpret_cast<const as_container<true,true>&>(cc);
   }
};

template <typename R, typename BaseComplex>
template <bool with_cycles, bool dual>
class ChainComplex<R,BaseComplex>::as_container : public ChainComplex<R,BaseComplex> {
protected:
   as_container();
   ~as_container();
public:
   typedef ChainComplex_iterator<R, BaseComplex, with_cycles, dual> iterator;
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

template <typename R, typename BaseComplex> inline
ChainComplex<R, BaseComplex>
make_chain_complex(const BaseComplex& complex, int dim_high=-1, int dim_low=1)
{
   return ChainComplex<R, BaseComplex>(complex,dim_high,dim_low);
}

} }

#endif // POLYMAKE_TOPAZ_CHAIN_COMPLEX_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
