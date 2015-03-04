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

#ifndef POLYMAKE_GENERIC_INCIDENCE_MATRIX_H
#define POLYMAKE_GENERIC_INCIDENCE_MATRIX_H

#include "polymake/internal/matrix_methods.h"
#include "polymake/GenericIO.h"

namespace pm {

/* ------------------------
 *  GenericIncidenceMatrix
 * ------------------------ */

template <typename _symmetric=NonSymmetric> class IncidenceMatrix;

template <typename Matrix, typename Other>
struct expandable_incidence_matrix
   : public enable_if<typename Unwary<Matrix>::type, (!Matrix::is_symmetric && object_traits<Matrix>::is_resizeable!=0)> {};

template <typename Matrix>
class GenericIncidenceMatrix
   : public Generic<Matrix>,
     public matrix_methods<Matrix, bool>,
     public operators::base {
   template <typename> friend class GenericIncidenceMatrix;
protected:
   GenericIncidenceMatrix() {}
   GenericIncidenceMatrix(const GenericIncidenceMatrix&) {}
#if POLYMAKE_DEBUG
   ~GenericIncidenceMatrix() { POLYMAKE_DEBUG_METHOD(GenericIncidenceMatrix,dump); }
   void dump() const { cerr << this->top() << std::flush; }
#endif

public:
   typedef bool element_type;
   static const bool is_symmetric=check_container_feature<Matrix, Symmetric>::value,
                          is_flat=check_container_feature<Matrix, FlatStorage>::value;
   typedef typename if_else<is_symmetric, Symmetric, NonSymmetric>::type symmetric;
   typedef typename Generic<Matrix>::top_type top_type;
   typedef IncidenceMatrix<symmetric> persistent_type;
   typedef GenericIncidenceMatrix generic_type;
protected:
   template <typename Matrix2>
   void assign(const GenericIncidenceMatrix<Matrix2>& m)
   {
      copy(pm::rows(m).begin(), entire(pm::rows(*this)));
   }

   template <typename Matrix2, typename Operation>
   void assign_op(const GenericIncidenceMatrix<Matrix2>& m, const Operation& op, True)
   {
      perform_assign(pm::entire(pm::rows(*this)), pm::rows(m).begin(), op);
   }

   template <typename Matrix2, typename Operation>
   void assign_op(const GenericIncidenceMatrix<Matrix2>& m, const Operation& op, False)
   {
      assign_op(m+T(m), op, True());
   }

   template <typename Matrix2>
   bool trivial_assignment(const GenericIncidenceMatrix<Matrix2>&) const { return false; }

   bool trivial_assignment(const GenericIncidenceMatrix& m) const { return this==&m; }

public:
   top_type& operator= (const GenericIncidenceMatrix& m)
   {
      if (!trivial_assignment(m)) {
         if (!object_traits<Matrix>::is_resizeable && (POLYMAKE_DEBUG || !Unwary<Matrix>::value)) {
            if (this->rows() != m.rows() || this->cols() != m.cols())
               throw std::runtime_error("GenericIncidenceMatrix::operator= - dimension mismatch");
         }
         this->top().assign(m.top());
      }
      return this->top();
   }

   template <typename Matrix2>
   typename enable_if<top_type, !is_symmetric || Matrix2::is_symmetric>::type&
   operator= (const GenericIncidenceMatrix<Matrix2>& m)
   {
      if (!object_traits<Matrix>::is_resizeable && (POLYMAKE_DEBUG || !Unwary<Matrix>::value)) {
         if (this->rows() != m.rows() || this->cols() != m.cols())
            throw std::runtime_error("GenericIncidenceMatrix::operator= - dimension mismatch");
      }
      this->top().assign(m.top());
      return this->top();
   }

   template <typename Matrix2>
   typename disable_if<top_type, !is_symmetric || Matrix2::is_symmetric>::type&
   operator= (const GenericIncidenceMatrix<Matrix2>& m)
   {
      if (!object_traits<Matrix>::is_resizeable && (POLYMAKE_DEBUG || !Unwary<Matrix>::value)) {
         if (this->rows() != m.rows() || this->cols() != m.cols())
            throw std::runtime_error("GenericIncidenceMatrix::operator= - dimension mismatch");
      }
      this->top().assign(m+T(m));
      return this->top();
   }

   template <typename Matrix2>
   top_type& operator+= (const GenericIncidenceMatrix<Matrix2>& m)
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value || !Unwary<Matrix2>::value) {
         if (this->rows() != m.rows() || this->cols() != m.cols())
            throw std::runtime_error("GenericIncidenceMatrix::operator+= - dimension mismatch");
      }
      this->top().assign_op(m.top(), BuildBinary<operations::add>(), bool2type<!is_symmetric || Matrix2::is_symmetric>());
      return this->top();
   }

   template <typename Matrix2>
   top_type& operator-= (const GenericIncidenceMatrix<Matrix2>& m)
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value || !Unwary<Matrix2>::value) {
         if (this->rows() != m.rows() || this->cols() != m.cols())
            throw std::runtime_error("GenericIncidenceMatrix::operator-= - dimension mismatch");
      }
      this->top().assign_op(m.top(), BuildBinary<operations::sub>(), bool2type<!is_symmetric || Matrix2::is_symmetric>());
      return this->top();
   }

   template <typename Matrix2>
   top_type& operator*= (const GenericIncidenceMatrix<Matrix2>& m)
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value || !Unwary<Matrix2>::value) {
         if (this->rows() != m.rows() || this->cols() != m.cols())
            throw std::runtime_error("GenericIncidenceMatrix::operator*= - dimension mismatch");
      }
      this->top().assign_op(m.top(), BuildBinary<operations::mul>(), bool2type<!is_symmetric || Matrix2::is_symmetric>());
      return this->top();
   }

   template <typename Matrix2>
   top_type& operator^= (const GenericIncidenceMatrix<Matrix2>& m)
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value || !Unwary<Matrix2>::value) {
         if (this->rows() != m.rows() || this->cols() != m.cols())
            throw std::runtime_error("GenericIncidenceMatrix::operator^= - dimension mismatch");
      }
      this->top().assign_op(m.top(), BuildBinary<operations::bitwise_xor>(), bool2type<!is_symmetric || Matrix2::is_symmetric>());
      return this->top();
   }

   template <typename Matrix2>
   typename expandable_incidence_matrix<Matrix,Matrix2>::type&
   operator/= (const GenericIncidenceMatrix<Matrix2>& m)
   {
      if (m.rows()) {
         if (POLYMAKE_DEBUG || !Unwary<Matrix>::value || !Unwary<Matrix2>::value) {
            if (this->cols() != m.cols())
               throw std::runtime_error("GenericIncidenceMatrix::operator/= - dimension mismatch");
         }
         this->top().append_rows(m.top());
      }
      return this->top();
   }

   template <typename Set2>
   typename expandable_incidence_matrix<Matrix,Set2>::type&
   operator/= (const GenericSet<Set2, int, operations::cmp>& s)
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value || !Unwary<Set2>::value) {
         if (!s.top().empty() && (s.top().front() < 0 || s.top().back() >= this->cols()))
            throw std::runtime_error("GenericMatrix::operator/= - set elements out of range");
      }
      this->top().append_row(s.top());
      return this->top();
   }

   template <typename Set2>
   typename expandable_incidence_matrix<Matrix,Set2>::type&
   operator/= (const Complement<Set2, int, operations::cmp>& s)
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
         if (!s.base().empty() && (s.base().front() < 0 || s.base().back() >= this->cols()))
            throw std::runtime_error("GenericMatrix::operator/= - set elements out of range");
      }
      this->top().append_row(sequence(0,this->cols()) * s);
   }

   template <typename Matrix2>
   typename expandable_incidence_matrix<Matrix,Matrix2>::type&
   operator|= (const GenericIncidenceMatrix<Matrix2>& m)
   {
      if (m.cols()) {
         if (POLYMAKE_DEBUG || !Unwary<Matrix>::value || !Unwary<Matrix2>::value) {
            if (this->rows() != m.rows())
               throw std::runtime_error("GenericIncidenceMatrix::operator|= - dimension mismatch");
         }
         this->top().append_cols(m.top());
      }
      return this->top();
   }

   template <typename Set2>
   typename expandable_incidence_matrix<Matrix,Set2>::type&
   operator|= (const GenericSet<Set2, int, operations::cmp>& s)
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value || !Unwary<Set2>::value) {
         if (!s.top().empty() && (s.top().front() < 0 || s.top().back() >= this->rows()))
            throw std::runtime_error("GenericMatrix::operator|= - set elements out of range");
      }
      this->top().append_col(s.top());
      return this->top();
   }

   template <typename Set2>
   typename expandable_incidence_matrix<Matrix,Set2>::type&
   operator|= (const Complement<Set2, int, operations::cmp>& s)
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
         if (!s.base().empty() && (s.base().front() < 0 || s.base().back() >= this->rows()))
            throw std::runtime_error("GenericMatrix::operator|= - set elements out of range");
      }
      this->top().append_col(sequence(0,this->rows()) * s);
   }

   template <typename Matrix2>
   void swap(GenericIncidenceMatrix<Matrix2>& m, typename enable_if<void**, is_symmetric==Matrix2::is_symmetric>::type=0)
   {
      if (trivial_assignment(m)) return;

      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value || !Unwary<Matrix2>::value) {
         if (this->rows() != m.rows() || this->cols() != m.cols())
            throw std::runtime_error("GenericIncidenceMatrix::swap - dimension mismatch");
      }
      swap_ranges(entire(pm::rows(*this)), pm::rows(m).begin());
   }

   top_type& transitive_closure();

   template <typename Result>
   struct rebind_generic {
      typedef GenericIncidenceMatrix<Result> type;
   };
};

struct is_incidence_matrix;

template <typename Matrix>
struct spec_object_traits< GenericIncidenceMatrix<Matrix> >
   : spec_or_model_traits<Matrix,is_container> {
private:
   typedef spec_or_model_traits<Matrix,is_container> _super;
public:
   static const int dimension=2,
                    is_symmetric=check_container_feature<Matrix,Symmetric>::value,
                    is_resizeable= _super::is_specialized ? _super::is_resizeable : 2-is_symmetric;
   typedef is_incidence_matrix generic_tag;
};

template <typename Matrix>
typename GenericIncidenceMatrix<Matrix>::top_type& GenericIncidenceMatrix<Matrix>::transitive_closure()
{
   if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
      if (this->rows() != this->cols())
         throw std::runtime_error("GenericIncidenceMatrix::transitive_closure - matrix not square");
   }
   bool changed;
   do {
      changed=false;
      for (typename Entire< Rows<Matrix> >::iterator r_i=entire(pm::rows(this->top()));  !r_i.at_end();  ++r_i) {
         const int ones=r_i->size();
         for (typename Matrix::row_type::iterator e=r_i->begin(); !e.at_end(); ++e)
            if (e.index() != r_i.index())
               (*r_i) += this->row(e.index());
         if (r_i->size() > ones) changed=true;
      }
   } while (changed);
   return this->top();
}

/* ----------------------------------------
 *  SingleIncidenceRow, SingleIncidenceCol
 * ---------------------------------------- */

template <typename SetRef>
class SingleIncidenceRow
   : protected single_line_matrix<SetRef>,
     public GenericIncidenceMatrix< SingleIncidenceRow<SetRef> > {
   typedef single_line_matrix<SetRef> _super;
public:
   typedef bool value_type;
   typedef bool reference;
   typedef const bool const_reference;
   SingleIncidenceRow(typename _super::arg_type arg) : _super(arg) {}

   friend class Rows<SingleIncidenceRow>;
};

template <typename SetRef>
class SingleIncidenceCol
   : protected single_line_matrix<SetRef>,
     public GenericIncidenceMatrix< SingleIncidenceCol<SetRef> > {
   typedef single_line_matrix<SetRef> _super;
public:
   typedef bool value_type;
   typedef bool reference;
   typedef const bool const_reference;
   SingleIncidenceCol(typename _super::arg_type arg) : _super(arg) {}
};

template <typename SetRef>
struct spec_object_traits< SingleIncidenceRow<SetRef> >
   : spec_object_traits< single_value_container<SetRef> > {};

template <typename SetRef>
struct spec_object_traits< SingleIncidenceCol<SetRef> >
   : spec_object_traits< single_value_container<SetRef> > {};

class SingleElementIncidenceLine
   : public modified_container_impl< SingleElementIncidenceLine,
                                     list( Container< single_value_container<int, true> >,
                                           Operation< BuildUnaryIt<operations::index2element> > ) >,
     public GenericSet<SingleElementIncidenceLine, int, operations::cmp> {
protected:
   single_value_container<int, true> _index;
public:
   SingleElementIncidenceLine() {}
   SingleElementIncidenceLine(int i) : _index(i) {}

   const single_value_container<int, true>& get_container() const { return _index; }
   int dim() const { return 1; }
};

template <>
struct check_container_feature<SingleElementIncidenceLine, sparse_compatible> : True {};

template <typename Iterator1Ref, typename Iterator2Ref>
struct SingleElementIncidenceLine_factory {
   typedef Iterator1Ref first_argument_type;
   typedef Iterator2Ref second_argument_type;
   typedef SingleElementIncidenceLine result_type;

   result_type operator() (Iterator1Ref it, Iterator2Ref) const
   {
      return *it;
   }
   template <typename Iterator1, typename Iterator2>
   result_type operator() (operations::partial, const Iterator1&, const Iterator2&) const
   {
      return result_type();
   }
};

template <typename SetRef>
class IncidenceLines_across
   : public modified_container_pair_impl< IncidenceLines_across<SetRef>,
                                          list( Container1< sequence >,
                                                Container2< SetRef >,
                                                IteratorCoupler< zipping_coupler<operations::cmp, set_union_zipper> >,
                                                Operation< BuildBinaryIt<SingleElementIncidenceLine_factory> >,
                                                Hidden< single_line_matrix<SetRef> > ) > {
public:
   sequence get_container1() const { return sequence(0, size()); }
   typename single_value_container<SetRef>::const_reference get_container2() const { return this->hidden().get_line(); }
   int size() const { return this->hidden().get_line().dim(); }
};

template <typename SetRef>
class Rows< SingleIncidenceRow<SetRef> >
   : public redirected_container< Rows< SingleIncidenceRow<SetRef> >,
                                  list( Container< single_value_container<SetRef> >,
                                        MasqueradedTop ) > {
protected:
   ~Rows();
public:
   const single_value_container<SetRef>& get_container() const { return this->hidden()._line; }
};

template <typename SetRef>
class Cols< SingleIncidenceRow<SetRef> > : public IncidenceLines_across<SetRef> {
protected:
   ~Cols();
};

template <typename SetRef>
class Rows< SingleIncidenceCol<SetRef> > : public IncidenceLines_across<SetRef> {
protected:
   ~Rows();
};

template <typename SetRef>
class Cols< SingleIncidenceCol<SetRef> > : public Rows< SingleIncidenceRow<SetRef> > {
protected:
   ~Cols();
};

template <typename ContainerRef1, typename ContainerRef2>
class IncidenceLineChain
   : public modified_container_impl< IncidenceLineChain<ContainerRef1,ContainerRef2>,
                                     list( Container< ContainerChain<ContainerRef1,ContainerRef2> >,
                                           Operation< BuildUnaryIt<operations::index2element> >,
                                           ExpectedFeatures< indexed > ) >,
     public GenericSet<IncidenceLineChain<ContainerRef1,ContainerRef2>, int, operations::cmp> {
   typedef modified_container_impl<IncidenceLineChain> _super;
protected:
   typename _super::container chain;
public:
   const typename _super::container& get_container() const { return chain; }

   IncidenceLineChain(typename _super::container::first_arg_type arg1,
                      typename _super::container::second_arg_type arg2)
      : chain(arg1,arg2) {}

   int dim() const
   {
      return chain.get_container1().dim() + chain.get_container2().dim();
   }
};

template <typename ContainerRef1, typename ContainerRef2>
struct check_container_feature<IncidenceLineChain<ContainerRef1, ContainerRef2>, sparse_compatible> : True {};

template <typename ContainerRef1, typename ContainerRef2>
struct spec_object_traits< IncidenceLineChain<ContainerRef1, ContainerRef2> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=true;
};

/* ----------------------------
 *  SameElementIncidenceMatrix
 * ---------------------------- */

template <bool elem>
class SameElementIncidenceMatrix
   : public GenericIncidenceMatrix<SameElementIncidenceMatrix<elem> > {
public:
   typedef bool value_type;
   typedef bool reference;
   typedef const bool const_reference;

   SameElementIncidenceMatrix(int r_arg, int c_arg) : r(r_arg), c(c_arg) {}
protected:
   void stretch_rows(int new_r) const
   {
      r=new_r;
   }
   void stretch_cols(int new_c) const
   {
      c=new_c;
   }

   mutable int r, c;

   friend class Rows<SameElementIncidenceMatrix>;
   friend class Cols<SameElementIncidenceMatrix>;
   template <typename, typename> friend class RowChain;
   template <typename, typename> friend class ColChain;
};

template <bool elem>
class SameElementIncidenceLine
   : public modified_container_impl< SameElementIncidenceLine<elem>,
                                     list( Container< sequence >,
                                           Operation< pair<nothing, operations::identity<int> > >,
                                           ExpectedFeatures< end_sensitive > ) >,
     public GenericSet<SameElementIncidenceLine<elem>, int, operations::cmp> {
public:
   int dim() const { return reinterpret_cast<const int&>(*this); }
   int size() const { return elem ? dim() : 0; }
   bool empty() const { return !elem; }

   sequence get_container() const { return sequence(0, size()); }
};

template <bool elem>
struct check_container_feature< SameElementIncidenceLine<elem>, sparse_compatible> : True {};

template <bool elem>
class Rows< SameElementIncidenceMatrix<elem> >
   : public redirected_container< Rows< SameElementIncidenceMatrix<elem> >,
                                  list( Container< constant_masquerade_container< SameElementIncidenceLine<elem> > >,
                                        MasqueradedTop ) > {
   typedef redirected_container<Rows> _super;
protected:
   ~Rows();
public:
   const typename _super::container& get_container() const
   {
      return reinterpret_cast<const typename _super::container&>(this->hidden().c);
   }
   int size() const { return this->hidden().r; }
   bool empty() const { return !size(); }
};

template <bool elem>
class Cols< SameElementIncidenceMatrix<elem> >
   : public redirected_container< Cols< SameElementIncidenceMatrix<elem> >,
                                  list( Container< constant_masquerade_container< SameElementIncidenceLine<elem> > >,
                                        MasqueradedTop ) > {
   typedef redirected_container<Cols> _super;
protected:
   ~Cols();
public:
   const typename _super::container& get_container() const
   {
      return reinterpret_cast<const typename _super::container&>(this->hidden().r);
   }
   int size() const { return this->hidden().c; }
   bool empty() const { return !size(); }
};

template <typename Matrix1, typename Matrix2> inline
RowChain< ColChain<const typename Unwary<Matrix1>::type&, SameElementIncidenceMatrix<false> >,
          ColChain<SameElementIncidenceMatrix<false>, const typename Unwary<Matrix2>::type&> >
diag(const GenericIncidenceMatrix<Matrix1>& m1, const GenericIncidenceMatrix<Matrix2>& m2)
{
   return RowChain< ColChain<const typename Unwary<Matrix1>::type&, SameElementIncidenceMatrix<false> >,
                    ColChain<SameElementIncidenceMatrix<false>, const typename Unwary<Matrix2>::type&> >
             ( ColChain<const typename Unwary<Matrix1>::type&, SameElementIncidenceMatrix<false> >
                  ( m1.top(), SameElementIncidenceMatrix<false>(m1.rows(), m2.cols()) ),
               ColChain<SameElementIncidenceMatrix<false>, const typename Unwary<Matrix2>::type&>
                  ( SameElementIncidenceMatrix<false>(m2.rows(), m1.cols()), m2.top() )
             );
}

/// the same, but with corner blocks filled with 1
template <typename Matrix1, typename Matrix2> inline
RowChain< ColChain<const typename Unwary<Matrix1>::type&, SameElementIncidenceMatrix<true> >,
          ColChain<SameElementIncidenceMatrix<true>, const typename Unwary<Matrix2>::type&> >
diag_1(const GenericIncidenceMatrix<Matrix1>& m1, const GenericIncidenceMatrix<Matrix2>& m2)
{
   return RowChain< ColChain<const typename Unwary<Matrix1>::type&, SameElementIncidenceMatrix<true> >,
                    ColChain<SameElementIncidenceMatrix<true>, const typename Unwary<Matrix2>::type&> >
             ( ColChain<const typename Unwary<Matrix1>::type&, SameElementIncidenceMatrix<true> >
                  ( m1.top(), SameElementIncidenceMatrix<true>(m1.rows(), m2.cols()) ),
               ColChain<SameElementIncidenceMatrix<true>, const typename Unwary<Matrix2>::type&>
                  ( SameElementIncidenceMatrix<true>(m2.rows(), m1.cols()), m2.top() )
             );
}

/* ------------
 *  complement
 * ------------ */

template <typename Matrix>
class ComplementIncidenceMatrix
   : public GenericIncidenceMatrix< ComplementIncidenceMatrix<Matrix> > {
protected:
   ComplementIncidenceMatrix();
   ~ComplementIncidenceMatrix();
public:
   typedef bool value_type;
   typedef bool reference;
   typedef const bool const_reference;

   typedef Matrix hidden_type;
   const Matrix& hidden() const { return reinterpret_cast<const Matrix&>(*this); }
};

template <typename Matrix>
struct spec_object_traits< ComplementIncidenceMatrix<Matrix> >
   : spec_object_traits<is_container> {
   static const bool is_always_const=true;
   typedef Matrix masquerade_for;
};

template <typename Matrix>
struct check_container_feature< ComplementIncidenceMatrix<Matrix>, Symmetric >
   : check_container_feature<Matrix, Symmetric> {};

template <typename Matrix>
class matrix_random_access_methods< ComplementIncidenceMatrix<Matrix> > {
public:
   bool operator() (int i, int j) const
   {
      return ! reinterpret_cast<const Matrix&>(*this)(i,j);
   }
};

template <typename LineRef>
struct ComplementIncidenceLine_factory {
   typedef LineRef argument_type;
   typedef LazySet2<sequence, LineRef, set_difference_zipper> result_type;

   result_type operator() (typename function_argument<LineRef>::const_type l) const
   {
      return result_type(sequence(0,l.dim()), l);
   }
};

template <typename Matrix>
class Rows< ComplementIncidenceMatrix<Matrix> >
   : public modified_container_impl< Rows< ComplementIncidenceMatrix<Matrix> >,
                                     list( Hidden< Rows<Matrix> >,
                                           Operation< BuildUnary<ComplementIncidenceLine_factory> > ) > {
protected:
   ~Rows();
};

template <typename Matrix>
class Cols< ComplementIncidenceMatrix<Matrix> >
   : public modified_container_impl< Cols< ComplementIncidenceMatrix<Matrix> >,
                                     list( Hidden< Cols<Matrix> >,
                                           Operation< BuildUnary<ComplementIncidenceLine_factory> > ) > {
protected:
   ~Cols();
};

/* --------------------------------------------
 *  boolean operations with incidence matrices
 * -------------------------------------------- */

template <typename MatrixRef1, typename MatrixRef2, typename Controller>
class LazyIncidenceMatrix2
   : public modified_container_pair_base<MatrixRef1, MatrixRef2, Controller>,
     public GenericIncidenceMatrix< LazyIncidenceMatrix2<MatrixRef1,MatrixRef2,Controller> > {
   typedef modified_container_pair_base<MatrixRef1, MatrixRef2, Controller> _base;
public:
   typedef bool value_type;
   typedef bool reference;
   typedef const bool const_reference;

   LazyIncidenceMatrix2(typename _base::first_arg_type src1_arg, typename _base::second_arg_type src2_arg, const Controller& c_arg=Controller())
      : _base(src1_arg, src2_arg, c_arg) {}
};

template <typename MatrixRef1, typename MatrixRef2, typename Controller>
struct spec_object_traits< LazyIncidenceMatrix2<MatrixRef1,MatrixRef2,Controller> >
   : spec_object_traits<is_container> {
   static const bool is_lazy=true, is_temporary=true, is_always_const=true;
};

template <typename MatrixRef1, typename MatrixRef2, typename Controller>
struct check_container_feature< LazyIncidenceMatrix2<MatrixRef1,MatrixRef2,Controller>, Symmetric > {
   static const bool value= check_container_ref_feature<MatrixRef1,Symmetric>::value &&
                            check_container_ref_feature<MatrixRef2,Symmetric>::value;
};

template <typename MatrixRef1, typename MatrixRef2, typename Controller>
class matrix_random_access_methods< LazyIncidenceMatrix2<MatrixRef1,MatrixRef2,Controller> > {
   typedef LazyIncidenceMatrix2<MatrixRef1,MatrixRef2,Controller> master;
public:
   bool operator() (int i, int j) const
   {
      const master& me=static_cast<const master&>(*this);
      return me.get_operation().contains(me.get_container1()(i,j), me.get_container2()(i,j));
   }
};

template <typename MatrixRef1, typename MatrixRef2, typename Controller>
class Rows< LazyIncidenceMatrix2<MatrixRef1, MatrixRef2, Controller> >
   : public modified_container_pair_impl< Rows< LazyIncidenceMatrix2<MatrixRef1,MatrixRef2,Controller> >,
                                          list( Container1< masquerade<pm::Rows,MatrixRef1> >,
                                                Container2< masquerade<pm::Rows,MatrixRef2> >,
                                                Operation< operations::construct_binary2_with_arg<LazySet2,Controller> >,
                                                MasqueradedTop ) > {
   typedef modified_container_pair_impl<Rows> _super;
protected:
   ~Rows();
public:
   const typename _super::container1& get_container1() const
   {
      return rows(this->hidden().get_container1());
   }
   const typename _super::container2& get_container2() const
   {
      return rows(this->hidden().get_container2());
   }
   typename _super::operation get_operation() const
   {
      return this->hidden().get_operation();
   }
};

template <typename MatrixRef1, typename MatrixRef2, typename Controller>
class Cols< LazyIncidenceMatrix2<MatrixRef1, MatrixRef2, Controller> >
   : public modified_container_pair_impl< Cols< LazyIncidenceMatrix2<MatrixRef1,MatrixRef2,Controller> >,
                                          list( Container1< masquerade<pm::Cols,MatrixRef1> >,
                                                Container2< masquerade<pm::Cols,MatrixRef2> >,
                                                Operation< operations::construct_binary2_with_arg<LazySet2,Controller> >,
                                                MasqueradedTop ) > {
   typedef modified_container_pair_impl<Cols> _super;
protected:
   ~Cols();
public:
   const typename _super::container1& get_container1() const
   {
      return cols(this->hidden().get_container1());
   }
   const typename _super::container2& get_container2() const
   {
      return cols(this->hidden().get_container2());
   }
   typename _super::operation get_operation() const
   {
      return this->hidden().get_operation();
   }
};

/* ------------------------------------
 *  operations building block matrices
 * ------------------------------------ */

namespace operations {

template <typename OpRef>
struct bitwise_inv_impl<OpRef, is_incidence_matrix>
   : reinterpret_impl<OpRef, const ComplementIncidenceMatrix<typename deref<typename Unwary<OpRef>::type>::type>&> {};

template <typename Matrix>
struct bitwise_inv_impl<const ComplementIncidenceMatrix<Matrix>&, is_incidence_matrix> {
   typedef const ComplementIncidenceMatrix<Matrix>& argument_type;
   typedef const Matrix& result_type;

   result_type operator() (argument_type x) const
   {
      return x.hidden();
   }
};

template <typename LeftRef, typename RightRef>
struct add_impl<LeftRef, RightRef, cons<is_incidence_matrix, is_incidence_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyIncidenceMatrix2<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                                typename attrib<typename Unwary<RightRef>::type>::plus_const, set_union_zipper>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.rows() != r.rows() || l.cols() != r.cols())
            throw std::runtime_error("operator+(GenericIncidenceMatrix,GenericIncidenceMatrix) - dimension mismatch");
      }
      return result_type(unwary(l), unwary(r));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l+=r;
   }
};

template <typename LeftRef, typename RightRef>
struct sub_impl<LeftRef, RightRef, cons<is_incidence_matrix, is_incidence_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyIncidenceMatrix2<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                                typename attrib<typename Unwary<RightRef>::type>::plus_const, set_difference_zipper>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.rows() != r.rows() || l.cols() != r.cols())
            throw std::runtime_error("operator-(GenericIncidenceMatrix,GenericIncidenceMatrix) - dimension mismatch");
      }
      return result_type(unwary(l), unwary(r));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l-=r;
   }
};

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_incidence_matrix, is_incidence_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyIncidenceMatrix2<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                                typename attrib<typename Unwary<RightRef>::type>::plus_const, set_intersection_zipper>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.rows() != r.rows() || l.cols() != r.cols())
            throw std::runtime_error("operator*(GenericIncidenceMatrix,GenericIncidenceMatrix) - dimension mismatch");
      }
      return result_type(unwary(l), unwary(r));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l*=r;
   }
};

template <typename LeftRef, typename RightRef>
struct bitwise_xor_impl<LeftRef, RightRef, cons<is_incidence_matrix, is_incidence_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazyIncidenceMatrix2<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                                typename attrib<typename Unwary<RightRef>::type>::plus_const, set_symdifference_zipper>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (l.rows() != r.rows() || l.cols() != r.cols())
            throw std::runtime_error("operator^(GenericIncidenceMatrix,GenericIncidenceMatrix) - dimension mismatch");
      }
      return result_type(unwary(l), unwary(r));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l^=r;
   }
};

template <typename LeftRef, typename RightRef>
struct concat_impl<LeftRef, RightRef, cons<is_set, is_set> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef IncidenceLineChain<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                              typename attrib<typename Unwary<RightRef>::type>::plus_const>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(unwary(l), unwary(r));
   }
};

template <typename LeftRef, typename RightRef>
struct div_impl<LeftRef, RightRef, cons<is_incidence_matrix, is_incidence_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef RowChain<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                    typename attrib<typename Unwary<RightRef>::type>::plus_const> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(unwary(l), unwary(r));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r)
   {
      l/=r;
   }
};

template <typename LeftRef, typename RightRef>
struct div_impl<LeftRef, RightRef, cons<is_incidence_matrix, is_set> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef Set_with_dim<typename attrib<typename Unwary<RightRef>::type>::plus_const> Right;
   typedef RowChain<typename attrib<typename Unwary<LeftRef>::type>::plus_const, SingleIncidenceRow<Right> > result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r)
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (!set_within_range(r, l.cols()))
            throw std::runtime_error("operator/(GenericIncidenceMatrix,GenericSet) - dimension mismatch");
      }
      return result_type(unwary(l), Right(unwary(r), l.cols()));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r)
   {
      l/=r;
   }
};

template <typename LeftRef, typename RightRef>
struct div_impl<LeftRef, RightRef, cons<is_set, is_incidence_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef Set_with_dim<typename attrib<typename Unwary<LeftRef>::type>::plus_const> Left;
   typedef RowChain<SingleIncidenceRow<Left>, typename attrib<typename Unwary<RightRef>::type>::plus_const> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r)
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (!set_within_range(l, r.cols()))
            throw std::runtime_error("operator/(GenericSet,GenericIncidenceMatrix) - dimension mismatch");
      }
      return result_type(Left(unwary(l), r.cols()), unwary(r));
   }
};

template <typename LeftRef, typename RightRef>
struct bitwise_or_impl<LeftRef, RightRef, cons<is_incidence_matrix, is_incidence_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef ColChain<typename attrib<typename Unwary<LeftRef>::type>::plus_const,
                    typename attrib<typename Unwary<RightRef>::type>::plus_const> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(unwary(l), unwary(r));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r)
   {
      l|=r;
   }
};

template <typename LeftRef, typename RightRef>
struct bitwise_or_impl<LeftRef, RightRef, cons<is_incidence_matrix, is_set> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef Set_with_dim<typename attrib<typename Unwary<RightRef>::type>::plus_const> Right;
   typedef ColChain<typename attrib<typename Unwary<LeftRef>::type>::plus_const, SingleIncidenceCol<Right> > result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r)
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (!set_within_range(r, l.rows()))
            throw std::runtime_error("operator|(GenericIncidenceMatrix,GenericSet) - dimension mismatch");
      }
      return result_type(unwary(l), Right(unwary(r), l.rows()));
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r)
   {
      l|=r;
   }
};

template <typename LeftRef, typename RightRef>
struct bitwise_or_impl<LeftRef, RightRef, cons<is_set, is_incidence_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef Set_with_dim<typename attrib<typename Unwary<LeftRef>::type>::plus_const> Left;
   typedef ColChain<SingleIncidenceCol<Left>, typename attrib<typename Unwary<RightRef>::type>::plus_const> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r)
   {
      if (POLYMAKE_DEBUG || !Unwary<LeftRef>::value || !Unwary<RightRef>::value) {
         if (!set_within_range(l, r.rows()))
            throw std::runtime_error("operator|(GenericSet,GenericIncidenceMatrix) - dimension mismatch");
      }
      return result_type(Left(unwary(l), r.rows()), unwary(r));
   }
};

} // end namespace operations
namespace operators {

template <typename Matrix1, typename Matrix2> inline
typename operations::add_impl<const Matrix1&, const Matrix2&>::result_type
operator+ (const GenericIncidenceMatrix<Matrix1>& l, const GenericIncidenceMatrix<Matrix2>& r)
{
   operations::add_impl<const Matrix1&, const Matrix2&> op;
   return op(concrete(l), concrete(r));
}

template <typename Matrix1, typename Matrix2> inline
typename operations::sub_impl<const Matrix1&, const Matrix2&>::result_type
operator- (const GenericIncidenceMatrix<Matrix1>& l, const GenericIncidenceMatrix<Matrix2>& r)
{
   operations::sub_impl<const Matrix1&, const Matrix2&> op;
   return op(concrete(l), concrete(r));
}

template <typename Matrix1, typename Matrix2> inline
bool operator== (const GenericIncidenceMatrix<Matrix1>& l, const GenericIncidenceMatrix<Matrix2>& r)
{
   if ((l.rows()==0 && r.rows()==0) || (l.cols()==0 && r.cols()==0)) return true;
   if (l.rows() != r.rows() || l.cols() != r.cols()) return false;
   operations::eq<const typename Unwary<Matrix1>::type&, const typename Unwary<Matrix2>::type&> op;
   return op(l.top(), r.top());
}

template <typename Matrix1, typename Matrix2> inline
bool operator< (const GenericIncidenceMatrix<Matrix1>& l, const GenericIncidenceMatrix<Matrix2>& r)
{
   if ((l.rows()==0 && r.rows()==0) || (l.cols()==0 && r.cols()==0)) return false;
   if (POLYMAKE_DEBUG || !Unwary<Matrix1>::type || !Unwary<Matrix2>::type) {
      if (l.rows() != r.rows() || l.cols() != r.cols())
         throw std::runtime_error("operator<(GenericIncidenceMatrix,GenericIncidenceMatrix) - dimension mismatch");
   }
   operations::lt<const typename Unwary<Matrix1>::type&, const typename Unwary<Matrix2>::type&> op;
   return op(l.top(), r.top());
}

} // end namespace operators
} // end namespace pm

namespace polymake {
   using pm::GenericIncidenceMatrix;
}

namespace std {
   template <typename Matrix1, typename Matrix2> inline
   void swap(pm::GenericIncidenceMatrix<Matrix1>& m1, pm::GenericIncidenceMatrix<Matrix2>& m2)
   {
      m1.top().swap(m2.top());
   }
}

#ifdef POLYMAKE_GENERIC_MATRIX_H
# include "polymake/internal/Incidence_and_SparseMatrix.h"
#endif
#endif // POLYMAKE_GENERIC_INCIDENCE_MATRIX_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
