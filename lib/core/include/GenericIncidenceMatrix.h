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

#define POLYMAKE_GENERIC_INCIDENCE_MATRIX_H

#include "polymake/internal/matrix_methods.h"
#include "polymake/GenericIO.h"

namespace pm {

/* ------------------------
 *  GenericIncidenceMatrix
 * ------------------------ */

template <typename TMatrix>
class GenericIncidenceMatrix;

template <typename symmetric=NonSymmetric> class IncidenceMatrix;

template <typename T>
using is_generic_incidence_matrix = is_derived_from_instance_of<pure_type_t<T>, GenericIncidenceMatrix>;

template <typename SetRef> class SingleIncidenceRow;
template <typename SetRef> class SingleIncidenceCol;
template <typename MatrixRef1, typename MatrixRef2, typename Controller> class LazyIncidenceMatrix2;
template <typename MatrixRef> class ComplementIncidenceMatrix;

template <typename TMatrix>
class GenericIncidenceMatrix
   : public Generic<TMatrix>
   , public matrix_methods<TMatrix, bool> {
   template <typename> friend class GenericIncidenceMatrix;
protected:
   GenericIncidenceMatrix() = default;
   GenericIncidenceMatrix(const GenericIncidenceMatrix&) = default;

public:
   using element_type = bool;
   static constexpr bool
      is_symmetric=check_container_feature<TMatrix, Symmetric>::value,
      is_flat=check_container_feature<TMatrix, FlatStorage>::value;
   using symmetric = std::conditional_t<is_symmetric, Symmetric, NonSymmetric>;
   using top_type = typename Generic<TMatrix>::top_type;
   using persistent_type = IncidenceMatrix<symmetric>;
   using generic_type = GenericIncidenceMatrix;

   template <typename Other>
   static constexpr bool is_expandable_by()
   {
      return !is_symmetric && object_traits<TMatrix>::is_resizeable!=0;
   }

   template <typename TMatrix2>
   static constexpr bool compatible_symmetry_types()
   {
      return !is_symmetric || TMatrix2::is_symmetric;
   }

protected:
   template <typename TMatrix2>
   void assign(const GenericIncidenceMatrix<TMatrix2>& m)
   {
      copy_range(pm::rows(m).begin(), entire(pm::rows(*this)));
   }

   template <typename TMatrix2, typename Operation>
   void assign_op(const GenericIncidenceMatrix<TMatrix2>& m, const Operation& op, std::true_type)
   {
      perform_assign(pm::entire(pm::rows(*this)), pm::rows(m).begin(), op);
   }

   template <typename TMatrix2, typename Operation>
   void assign_op(const GenericIncidenceMatrix<TMatrix2>& m, const Operation& op, std::false_type)
   {
      assign_op(m+T(m), op, std::true_type());
   }

   template <typename TMatrix2>
   constexpr bool trivial_assignment(const GenericIncidenceMatrix<TMatrix2>&) const { return false; }

   constexpr bool trivial_assignment(const GenericIncidenceMatrix& m) const { return this==&m; }

   template <typename TMatrix2>
   void assign_impl(const GenericIncidenceMatrix<TMatrix2>& m, std::false_type)
   {
      this->top().assign(m+T(m));
   }

   template <typename TMatrix2>
   void assign_impl(const GenericIncidenceMatrix<TMatrix2>& m, std::true_type)
   {
      this->top().assign(m.top());
   }

public:
   top_type& operator= (const GenericIncidenceMatrix& m)
   {
      if (!trivial_assignment(m)) {
         if (!object_traits<TMatrix>::is_resizeable && (POLYMAKE_DEBUG || is_wary<TMatrix>())) {
            if (this->rows() != m.rows() || this->cols() != m.cols())
               throw std::runtime_error("GenericIncidenceMatrix::operator= - dimension mismatch");
         }
         this->top().assign(m.top());
      }
      return this->top();
   }

   template <typename Matrix2>
   top_type& operator= (const GenericIncidenceMatrix<Matrix2>& m)
   {
      if (!object_traits<TMatrix>::is_resizeable && (POLYMAKE_DEBUG || is_wary<TMatrix>())) {
         if (this->rows() != m.rows() || this->cols() != m.cols())
            throw std::runtime_error("GenericIncidenceMatrix::operator= - dimension mismatch");
      }
      assign_impl(m, bool_constant<!is_symmetric || Matrix2::is_symmetric>());
      return this->top();
   }

protected:
   template <typename Left, typename Right, typename Controller, typename=void>
   struct lazy_op {};

   template <typename Left, typename Right, typename Controller>
   struct lazy_op<Left, Right, Controller,
                  std::enable_if_t<is_derived_from<pure_type_t<Left>, GenericIncidenceMatrix>::value &&
                                   is_generic_incidence_matrix<Right>::value>> {
      using type = LazyIncidenceMatrix2<add_const_t<unwary_t<Left>>, add_const_t<unwary_t<Right>>, Controller>;
   };

#define PmCheckMatrixDim(Left, l, Right, r, sign) \
   if (POLYMAKE_DEBUG || is_wary<Left>() || is_wary<Right>()) \
      if (l.rows() != r.rows() || l.cols() != r.cols()) \
         throw std::runtime_error("GenericIncidenceMatrix::operator" sign " - dimension mismatch")

public:
   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, set_union_zipper>::type
   operator+ (Left&& l, Right&& r)
   {
      PmCheckMatrixDim(Left, l, Right, r, "+");
      return typename lazy_op<Left, Right, set_union_zipper>::type(unwary(std::forward<Left>(l)),
                                                                   unwary(std::forward<Right>(r)));
   }

   template <typename Right>
   top_type& operator+= (const GenericIncidenceMatrix<Right>& r)
   {
      PmCheckMatrixDim(TMatrix, (*this), Right, r, "+=");
      this->top().assign_op(r, BuildBinary<operations::add>(), bool_constant<!is_symmetric || Right::is_symmetric>());
      return this->top();
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, set_difference_zipper>::type
   operator- (Left&& l, Right&& r)
   {
      PmCheckMatrixDim(Left, l, Right, r, "-");
      return typename lazy_op<Left, Right, set_difference_zipper>::type(unwary(std::forward<Left>(l)),
                                                                        unwary(std::forward<Right>(r)));
   }

   template <typename Right>
   top_type& operator-= (const GenericIncidenceMatrix<Right>& r)
   {
      PmCheckMatrixDim(TMatrix, (*this), Right, r, "-=");
      this->top().assign_op(r, BuildBinary<operations::sub>(), bool_constant<!is_symmetric || Right::is_symmetric>());
      return this->top();
   }

   template <typename Left, typename Right>
   friend
   typename lazy_op<Left, Right, set_intersection_zipper>::type
   operator* (Left&& l, Right&& r)
   {
      PmCheckMatrixDim(Left, l, Right, r, "*");
      return typename lazy_op<Left, Right, set_intersection_zipper>::type(unwary(std::forward<Left>(l)),
                                                                          unwary(std::forward<Right>(r)));
   }

   template <typename Right>
   top_type& operator*= (const GenericIncidenceMatrix<Right>& r)
   {
      PmCheckMatrixDim(TMatrix, (*this), Right, r, "*=");
      this->top().assign_op(r, BuildBinary<operations::mul>(), bool_constant<!is_symmetric || Right::is_symmetric>());
      return this->top();
   }

   template <typename Left, typename Right> friend
   typename lazy_op<Left, Right, set_symdifference_zipper>::type
   operator^ (Left&& l, Right&& r)
   {
      PmCheckMatrixDim(Left, l, Right, r, "^");
      return typename lazy_op<Left, Right, set_symdifference_zipper>::type(unwary(std::forward<Left>(l)),
                                                                           unwary(std::forward<Right>(r)));
   }

   template <typename Right>
   top_type& operator^= (const GenericIncidenceMatrix<Right>& r)
   {
      PmCheckMatrixDim(TMatrix, (*this), Right, r, "^=");
      this->top().assign_op(r, BuildBinary<operations::bitwise_xor>(), bool_constant<!is_symmetric || Right::is_symmetric>());
      return this->top();
   }

#undef PmCheckMatrixDim

   auto operator~ () const &
   {
      return ComplementIncidenceMatrix<diligent_ref_t<unwary_t<const TMatrix&>>>(diligent(unwary(*this)));
   }

   auto operator~ () &&
   {
      return ComplementIncidenceMatrix<diligent_ref_t<unwary_t<TMatrix>>>(diligent(unwary(std::move(*this))));
   }

protected:
   template <typename TSet>
   void check_append_row(const TSet& s) const
   {
      if (!s.empty() && (s.front() < 0 || s.back() >= this->cols()))
         throw std::runtime_error("GenericIncidenceMatrix::operator/= - row elements out of range");
   }

   template <typename SetRef>
   void check_append_row(const Complement<SetRef>& c) const
   {
      return check_append_row(c.base());
   }

   template <typename TSet>
   void check_append_col(const TSet& s) const
   {
      if (!s.empty() && (s.front() < 0 || s.back() >= this->rows()))
         throw std::runtime_error("GenericIncidenceMatrix::operator|= - column elements out of range");
   }

   template <typename SetRef>
   void check_append_col(const Complement<SetRef>& c) const
   {
      return check_append_col(c.base());
   }

   template <typename Left, typename Right, typename Rowwise, typename=void>
   struct block_matrix {};
   using rowwise = std::true_type;
   using columnwise = std::false_type;

   template <typename Left, typename Right, typename Rowwise>
   struct block_matrix<Left, Right, Rowwise, std::enable_if_t<is_derived_from<pure_type_t<Left>, GenericIncidenceMatrix>::value &&
                                                              is_generic_incidence_matrix<Right>::value>> {
      using m_m_type = typename chain_compose<BlockMatrix, true, Rowwise>::template with<unwary_t<Left>, unwary_t<Right>>;
   };

   template <typename Left, typename Right, typename Rowwise>
   struct block_matrix<Left, Right, Rowwise, std::enable_if_t<is_derived_from<pure_type_t<Left>, GenericIncidenceMatrix>::value &&
                                                              is_integer_set<Right>::value>> {
      using set_type = Set_with_dim<add_const_t<unwary_t<Right>>>;
      using line_type = std::conditional_t<Rowwise::value, SingleIncidenceRow<set_type>, SingleIncidenceCol<set_type>>;
      using m_s_type = typename chain_compose<BlockMatrix, true, Rowwise>::template with<unwary_t<Left>, line_type>;

      static m_s_type make(Left&& l, Right&& r, Int dim)
      {
         return m_s_type(unwary(std::forward<Left>(l)),
                         line_type(set_type(unwary(std::forward<Right>(r)), dim)));
      }
   };

   template <typename Left, typename Right, typename Rowwise>
   struct block_matrix<Left, Right, Rowwise, std::enable_if_t<is_integer_set<Left>::value &&
                                                              is_derived_from<pure_type_t<Right>, GenericIncidenceMatrix>::value>> {
      using set_type = Set_with_dim<add_const_t<unwary_t<Left>>>;
      using line_type = std::conditional_t<Rowwise::value, SingleIncidenceRow<set_type>, SingleIncidenceCol<set_type>>;
      using s_m_type = typename chain_compose<BlockMatrix, true, Rowwise>::template with<line_type, unwary_t<Right>>;

      static s_m_type make(Left&& l, Right&& r, Int dim)
      {
         return s_m_type(line_type(set_type(unwary(std::forward<Left>(l)), dim)),
                         unwary(std::forward<Right>(r)));
      }
   };

public:
   template <typename Left, typename Right>
   friend
   typename block_matrix<Left, Right, rowwise>::m_m_type operator/ (Left&& l, Right&& r)
   {
      return typename block_matrix<Left, Right, rowwise>::m_m_type(unwary(std::forward<Left>(l)),
                                                                   unwary(std::forward<Right>(r)));
   }

   template <typename Left, typename Right>
   friend
   typename block_matrix<Left, Right, rowwise>::m_s_type operator/ (Left&& l, Right&& r)
   {
      if (POLYMAKE_DEBUG || is_wary<Left>() || is_wary<Right>())
         l.check_append_row(r);
      return block_matrix<Left, Right, rowwise>::make(std::forward<Left>(l), std::forward<Right>(r), l.cols());
   }

   template <typename Left, typename Right>
   friend
   typename block_matrix<Left, Right, rowwise>::s_m_type operator/ (Left&& l, Right&& r)
   {
      if (POLYMAKE_DEBUG || is_wary<Left>() || is_wary<Right>())
         r.check_append_row(l);
      return block_matrix<Left, Right, rowwise>::make(std::forward<Left>(l), std::forward<Right>(r), r.cols());
   }

   template <typename Left, typename Right>
   friend
   typename block_matrix<Left, Right, columnwise>::m_m_type operator| (Left&& l, Right&& r)
   {
      return typename block_matrix<Left, Right, columnwise>::m_m_type(unwary(std::forward<Left>(l)),
                                                                      unwary(std::forward<Right>(r)));
   }

   template <typename Left, typename Right>
   friend
   typename block_matrix<Left, Right, columnwise>::m_s_type operator| (Left&& l, Right&& r)
   {
      if (POLYMAKE_DEBUG || is_wary<Left>() || is_wary<Right>())
         l.check_append_col(r);
      return block_matrix<Left, Right, columnwise>::make(std::forward<Left>(l), std::forward<Right>(r), l.rows());
   }

   template <typename Left, typename Right>
   friend
   typename block_matrix<Left, Right, columnwise>::s_m_type operator| (Left&& l, Right&& r)
   {
      if (POLYMAKE_DEBUG || is_wary<Left>() || is_wary<Right>())
         r.check_append_col(l);
      return block_matrix<Left, Right, columnwise>::make(std::forward<Left>(l), std::forward<Right>(r), r.rows());
   }

   template <typename Matrix2>
   std::enable_if_t<is_expandable_by<Matrix2>(), top_type&>
   operator/= (const GenericIncidenceMatrix<Matrix2>& m)
   {
      if (m.rows()) {
         if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<Matrix2>()) {
            if (this->cols() != m.cols())
               throw std::runtime_error("GenericIncidenceMatrix::operator/= - dimension mismatch");
         }
         this->top().append_rows(m.top());
      }
      return this->top();
   }

   template <typename TSet>
   std::enable_if_t<is_expandable_by<TSet>(), top_type&>
   operator/= (const GenericSet<TSet, Int, operations::cmp>& s)
   {
      if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<TSet>())
         check_append_row(s.top());
      this->top().append_row(prepare_index_set(s, [&](){ return this->cols(); }));
      return this->top();
   }

   template <typename Matrix2>
   std::enable_if_t<is_expandable_by<Matrix2>(), top_type&>
   operator|= (const GenericIncidenceMatrix<Matrix2>& m)
   {
      if (m.cols()) {
         if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<Matrix2>()) {
            if (this->rows() != m.rows())
               throw std::runtime_error("GenericIncidenceMatrix::operator|= - dimension mismatch");
         }
         this->top().append_cols(m.top());
      }
      return this->top();
   }

   template <typename TSet>
   std::enable_if_t<is_expandable_by<TSet>(), top_type&>
   operator|= (const GenericSet<TSet, Int, operations::cmp>& s)
   {
      if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<TSet>())
         check_append_col(s.top());
      this->top().append_col(prepare_index_set(s, [&](){ return this->rows(); }));
      return this->top();
   }

   template <typename Matrix2, typename=std::enable_if_t<is_symmetric==Matrix2::is_symmetric>>
   void swap(GenericIncidenceMatrix<Matrix2>& m)
   {
      if (trivial_assignment(m)) return;

      if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<Matrix2>()) {
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

   template <typename Matrix2>
   bool operator== (const GenericIncidenceMatrix<Matrix2>& m) const
   {
      if (this->rows() != m.rows() || this->cols() != m.cols()) return false;
      return operations::cmp_unordered()(rows(*this), rows(m)) == cmp_eq;
   }

   template <typename Matrix2>
   bool operator!= (const GenericIncidenceMatrix<Matrix2>& m) const
   {
      return !(*this == m);
   }

#if POLYMAKE_DEBUG
   void dump() const __attribute__((used)) { cerr << this->top() << std::flush; }
#endif
};

struct is_incidence_matrix;

template <typename TMatrix>
struct spec_object_traits< GenericIncidenceMatrix<TMatrix> >
   : spec_or_model_traits<TMatrix, is_container> {
private:
   typedef spec_or_model_traits<TMatrix, is_container> base_t;
public:
   static constexpr int dimension=2,
                        is_symmetric = check_container_feature<TMatrix, Symmetric>::value,
                        is_resizeable = base_t::is_specialized ? base_t::is_resizeable : 2-is_symmetric;
   typedef is_incidence_matrix generic_tag;
};


template <typename Result, typename TSet>
struct generic_of_repeated_line<Result, GenericSet<TSet, Int, operations::cmp>> {
   using type = GenericIncidenceMatrix<Result>;
};


template <typename TMatrix>
typename GenericIncidenceMatrix<TMatrix>::top_type& GenericIncidenceMatrix<TMatrix>::transitive_closure()
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (this->rows() != this->cols())
         throw std::runtime_error("GenericIncidenceMatrix::transitive_closure - matrix not square");
   }
   bool changed;
   do {
      changed = false;
      for (auto r_i = entire(pm::rows(this->top()));  !r_i.at_end();  ++r_i) {
         const Int ones = r_i->size();
         for (auto e = r_i->begin(); !e.at_end(); ++e)
            if (e.index() != r_i.index())
               (*r_i) += this->row(e.index());
         if (r_i->size() > ones) changed = true;
      }
   } while (changed);
   return this->top();
}

/* ----------------------------------------
 *  SingleIncidenceRow, SingleIncidenceCol
 * ---------------------------------------- */

template <typename SetRef>
class SingleIncidenceRow
   : protected repeated_line_matrix<SetRef>
   , public GenericIncidenceMatrix< SingleIncidenceRow<SetRef> > {
   using base_t = repeated_line_matrix<SetRef>;
public:
   using value_type = bool;
   using reference = bool;
   using const_reference = bool;

   template <typename Arg, typename = std::enable_if_t<std::is_constructible<base_t, Arg, Int>::value>>
   SingleIncidenceRow(Arg&& arg)
      : base_t(std::forward<Arg>(arg), 1) {}

   template <typename> friend class Rows;
   template <typename> friend class Cols;
};

template <typename SetRef>
class SingleIncidenceCol
   : protected repeated_line_matrix<SetRef>
   , public GenericIncidenceMatrix< SingleIncidenceCol<SetRef> > {
   using base_t = repeated_line_matrix<SetRef>;
public:
   using value_type = bool;
   using reference = bool;
   using const_reference = bool;

   template <typename Arg, typename = std::enable_if_t<std::is_constructible<base_t, Arg, Int>::value>>
   SingleIncidenceCol(Arg&& arg)
      : base_t(std::forward<Arg>(arg), 1) {}

   template <typename> friend class Rows;
   template <typename> friend class Cols;
};

template <typename SetRef>
struct spec_object_traits< SingleIncidenceRow<SetRef> >
   : spec_object_traits< repeated_value_container<SetRef> > {};

template <typename SetRef>
struct spec_object_traits< SingleIncidenceCol<SetRef> >
   : spec_object_traits< repeated_value_container<SetRef> > {};

class SingleElementIncidenceLine
   : public modified_container_impl< SingleElementIncidenceLine,
                                     mlist< ContainerTag< repeated_value_container<Int> >,
                                            OperationTag< BuildUnaryIt<operations::index2element> > > >
   , public GenericSet<SingleElementIncidenceLine, Int, operations::cmp> {
protected:
   repeated_value_container<Int> index;
public:
   SingleElementIncidenceLine(Int i, Int size)
      : index(+i, size) {}

   const repeated_value_container<Int>& get_container() const { return index; }
   Int dim() const { return 1; }
};

template <>
struct check_container_feature<SingleElementIncidenceLine, sparse_compatible> : std::true_type {};

template <typename Iterator1Ref, typename Iterator2Ref>
struct SingleElementIncidenceLine_factory {
   typedef Iterator1Ref first_argument_type;
   typedef Iterator2Ref second_argument_type;
   typedef SingleElementIncidenceLine result_type;

   result_type operator() (Iterator1Ref it, Iterator2Ref) const
   {
      return result_type(*it, 1);
   }
   template <typename Iterator1, typename Iterator2>
   result_type operator() (operations::partial, const Iterator1&, const Iterator2&) const
   {
      return result_type(-1, 0);
   }
};

template <typename SetRef>
class IncidenceLines_across
   : public modified_container_pair_impl< IncidenceLines_across<SetRef>,
                                          mlist< Container1RefTag< sequence >,
                                                 Container2RefTag< SetRef >,
                                                 IteratorCouplerTag< zipping_coupler<operations::cmp, set_union_zipper> >,
                                                 OperationTag< BuildBinaryIt<SingleElementIncidenceLine_factory> >,
                                                 HiddenTag< repeated_line_matrix<SetRef> > > > {
public:
   sequence get_container1() const { return sequence(0, size()); }
   decltype(auto) get_container2() const { return this->hidden().get_line(); }
   Int size() const { return this->hidden().get_line().dim(); }
};

template <typename SetRef>
class Rows< SingleIncidenceRow<SetRef> >
   : public redirected_container< Rows< SingleIncidenceRow<SetRef> >,
                                  mlist< ContainerRefTag< repeated_value_container<SetRef> >,
                                         MasqueradedTop > > {
protected:
   ~Rows();
public:
   decltype(auto) get_container() const
   {
      return this->hidden().get_line_container();
   }
};

template <typename SetRef>
class Cols< SingleIncidenceCol<SetRef> >
   : public redirected_container< Cols< SingleIncidenceCol<SetRef> >,
                                  mlist< ContainerRefTag< repeated_value_container<SetRef> >,
                                         MasqueradedTop > > {
protected:
   ~Cols();
public:
   decltype(auto) get_container() const
   {
      return this->hidden().get_line_container();
   }
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

template <typename ContainerList>
class IncidenceLineChain
   : public modified_container_impl< IncidenceLineChain<ContainerList>,
                                     mlist< ContainerTag< ContainerChain<ContainerList> >,
                                            OperationTag< BuildUnaryIt<operations::index2element> > > >
   , public GenericSet<IncidenceLineChain<ContainerList>, Int, operations::cmp> {
protected:
   using chain_t = ContainerChain<ContainerList>;
   chain_t chain;
public:
   const chain_t& get_container() const { return chain; }

   template <int i>
   decltype(auto) get_alias(int_constant<i>) const
   {
      return chain.get_alias(int_constant<i>());
   }

   // TODO: =delete
   IncidenceLineChain(const IncidenceLineChain&) = default;
   IncidenceLineChain(IncidenceLineChain&&) = default;

   template <typename... Args,
             typename=std::enable_if_t<std::is_constructible<chain_t, Args...>::value>>
   explicit IncidenceLineChain(Args&&... args)
      : chain(std::forward<Args>(args)...) {}

   Int dim() const
   {
      return chain.dim();
   }
};

template <typename ContainerList>
struct check_container_feature<IncidenceLineChain<ContainerList>, sparse_compatible> : std::true_type {};

template <typename ContainerList>
struct spec_object_traits< IncidenceLineChain<ContainerList> >
   : spec_object_traits<is_container> {
   static constexpr bool is_temporary = true, is_always_const = true;
};

template <typename TMatrix>
struct concat_lines_op<GenericIncidenceMatrix<TMatrix>> {
   using type = polymake::operations::concat_tuple<IncidenceLineChain>;
};

/* ----------------------------
 *  SameElementIncidenceMatrix
 * ---------------------------- */

template <bool elem>
class SameElementIncidenceMatrix
   : public GenericIncidenceMatrix<SameElementIncidenceMatrix<elem> > {
public:
   using value_type = bool;
   using reference = bool;
   using const_reference = bool;

   SameElementIncidenceMatrix(Int r_arg, Int c_arg)
      : r(r_arg) , c(c_arg) {}
protected:
   void stretch_rows(Int new_r)
   {
      r = new_r;
   }
   void stretch_cols(Int new_c)
   {
      c = new_c;
   }

   Int r, c;

   friend class Rows<SameElementIncidenceMatrix>;
   friend class Cols<SameElementIncidenceMatrix>;
   template <typename, typename> friend class BlockMatrix;
};

template <bool elem>
class SameElementIncidenceLine
   : public modified_container_impl< SameElementIncidenceLine<elem>,
                                     mlist< ContainerTag< sequence >,
                                            OperationTag< pair<nothing, operations::identity<Int> > >,
                                            ExpectedFeaturesTag< end_sensitive > > >,
     public GenericSet<SameElementIncidenceLine<elem>, Int, operations::cmp> {
public:
   Int dim() const { return reinterpret_cast<const Int&>(*this); }
   Int size() const { return elem ? dim() : 0; }
   bool empty() const { return !elem; }

   sequence get_container() const { return sequence(0, size()); }
};

template <bool elem>
struct check_container_feature< SameElementIncidenceLine<elem>, sparse_compatible> : std::true_type {};

template <bool elem>
class Rows< SameElementIncidenceMatrix<elem> >
   : public redirected_container< Rows< SameElementIncidenceMatrix<elem> >,
                                  mlist< ContainerRefTag< constant_masquerade_container< SameElementIncidenceLine<elem> > >,
                                         MasqueradedTop > > {
   typedef redirected_container<Rows> _super;
protected:
   ~Rows();
public:
   const typename _super::container& get_container() const
   {
      return reinterpret_cast<const typename _super::container&>(this->hidden().c);
   }
   Int size() const { return this->hidden().r; }
   bool empty() const { return !size(); }
};

template <bool elem>
class Cols< SameElementIncidenceMatrix<elem> >
   : public redirected_container< Cols< SameElementIncidenceMatrix<elem> >,
                                  mlist< ContainerRefTag< constant_masquerade_container< SameElementIncidenceLine<elem> > >,
                                         MasqueradedTop > > {
   typedef redirected_container<Cols> base_t;
protected:
   ~Cols();
public:
   const typename base_t::container& get_container() const
   {
      return reinterpret_cast<const typename base_t::container&>(this->hidden().r);
   }
   Int size() const { return this->hidden().c; }
   bool empty() const { return !size(); }
};

template <bool fill_value, typename Matrix1, typename Matrix2>
auto make_block_diag(Matrix1&& m1, Matrix2&& m2)
{
   using corner = SameElementIncidenceMatrix<fill_value>;
   using upper_row = BlockMatrix<mlist<Matrix1, corner>, std::false_type>;
   using lower_row = BlockMatrix<mlist<corner, Matrix2>, std::false_type>;
   using result = BlockMatrix<mlist<upper_row, lower_row>, std::true_type>;
   return result(upper_row(std::forward<Matrix1>(m1), corner(m1.rows(), m2.cols())),
                 lower_row(corner(m2.rows(), m1.cols()), std::forward<Matrix2>(m2)));
}

/// create a block-diagonal incidence matrix
template <typename Matrix1, typename Matrix2>
auto diag(const GenericIncidenceMatrix<Matrix1>& m1, const GenericIncidenceMatrix<Matrix2>& m2)
   // gcc5 needs this hint
   ->decltype(make_block_diag<false>(m1.top(), m2.top()))
{
   return make_block_diag<false>(m1.top(), m2.top());
}

/// create a block-diagonal incidence matrix, fill the corner blocks with 1's
template <typename Matrix1, typename Matrix2>
auto diag_1(const GenericIncidenceMatrix<Matrix1>& m1, const GenericIncidenceMatrix<Matrix2>& m2)
   // gcc5 needs this hint
   ->decltype(make_block_diag<true>(m1.top(), m2.top()))
{
   return make_block_diag<true>(m1.top(), m2.top());
}

/* ------------
 *  complement
 * ------------ */

template <typename MatrixRef>
class ComplementIncidenceMatrix
   : public GenericIncidenceMatrix< ComplementIncidenceMatrix<MatrixRef> > {
protected:
   using alias_t = alias<MatrixRef>;
   alias_t matrix;
public:
   using value_type = bool;
   using reference = bool;
   using const_reference = bool;

   template <typename Arg, typename = std::enable_if_t<std::is_constructible<alias_t, Arg>::value>>
   explicit ComplementIncidenceMatrix(Arg&& arg)
      : matrix(arg) {}

   decltype(auto) get_matrix() const { return *matrix; }

   decltype(auto) operator~ () const { return get_matrix(); }
};

template <typename MatrixRef>
struct spec_object_traits< ComplementIncidenceMatrix<MatrixRef> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=true;
};

template <typename MatrixRef>
struct check_container_feature< ComplementIncidenceMatrix<MatrixRef>, Symmetric >
   : check_container_feature<MatrixRef, Symmetric> {};

template <typename MatrixRef>
class matrix_random_access_methods< ComplementIncidenceMatrix<MatrixRef> > {
public:
   bool operator() (Int i, Int j) const
   {
      return !static_cast<const ComplementIncidenceMatrix<MatrixRef>&>(*this).get_matrix()(i, j);
   }
};

template <typename LineRef>
struct ComplementIncidenceLine_factory {
   typedef LineRef argument_type;
   typedef Complement<LineRef> result_type;

   template <typename L>
   result_type operator() (L&& l) const
   {
      return result_type(std::forward<L>(l));
   }
};

template <typename MatrixRef>
class Rows< ComplementIncidenceMatrix<MatrixRef> >
   : public modified_container_impl< Rows< ComplementIncidenceMatrix<MatrixRef> >,
                                     mlist< ContainerRefTag< masquerade<pm::Rows, MatrixRef> >,
                                            OperationTag< BuildUnary<ComplementIncidenceLine_factory> >,
                                            MasqueradedTop > > {
protected:
   ~Rows();
public:
   decltype(auto) get_container() const
   {
      return rows(this->hidden().get_matrix());
   }
};

template <typename MatrixRef>
class Cols< ComplementIncidenceMatrix<MatrixRef> >
   : public modified_container_impl< Cols< ComplementIncidenceMatrix<MatrixRef> >,
                                     mlist< ContainerRefTag< masquerade<pm::Cols, MatrixRef> >,
                                            OperationTag< BuildUnary<ComplementIncidenceLine_factory> >,
                                            MasqueradedTop > > {
protected:
   ~Cols();
public:
   decltype(auto) get_container() const
   {
      return cols(this->hidden().get_matrix());
   }
};

/* --------------------------------------------
 *  boolean operations with incidence matrices
 * -------------------------------------------- */

template <typename MatrixRef1, typename MatrixRef2, typename Controller>
class LazyIncidenceMatrix2
   : public modified_container_pair_base<MatrixRef1, MatrixRef2, Controller>,
     public GenericIncidenceMatrix< LazyIncidenceMatrix2<MatrixRef1,MatrixRef2,Controller> > {
   using base_t = modified_container_pair_base<MatrixRef1, MatrixRef2, Controller>;
public:
   using value_type = bool;
   using reference = bool;
   using const_reference = bool;
   using base_t::modified_container_pair_base;
};

template <typename MatrixRef1, typename MatrixRef2, typename Controller>
struct spec_object_traits< LazyIncidenceMatrix2<MatrixRef1, MatrixRef2, Controller> >
   : spec_object_traits<is_container> {
   static constexpr bool is_lazy = true, is_temporary = true, is_always_const = true;
};

template <typename MatrixRef1, typename MatrixRef2, typename Controller>
struct check_container_feature< LazyIncidenceMatrix2<MatrixRef1, MatrixRef2, Controller>, Symmetric >
   : mlist_and< check_container_ref_feature<MatrixRef1, Symmetric>,
                check_container_ref_feature<MatrixRef2, Symmetric> > {};

template <typename MatrixRef1, typename MatrixRef2, typename Controller>
class matrix_random_access_methods< LazyIncidenceMatrix2<MatrixRef1,MatrixRef2,Controller> > {
   typedef LazyIncidenceMatrix2<MatrixRef1,MatrixRef2,Controller> master;
public:
   bool operator() (Int i, Int j) const
   {
      const master& me=static_cast<const master&>(*this);
      return me.get_operation().contains(me.get_container1()(i, j), me.get_container2()(i, j));
   }
};

template <typename MatrixRef1, typename MatrixRef2, typename Controller>
class Rows< LazyIncidenceMatrix2<MatrixRef1, MatrixRef2, Controller> >
   : public modified_container_pair_impl< Rows< LazyIncidenceMatrix2<MatrixRef1,MatrixRef2,Controller> >,
                                          mlist< Container1RefTag< masquerade<pm::Rows, MatrixRef1> >,
                                                 Container2RefTag< masquerade<pm::Rows, MatrixRef2> >,
                                                 OperationTag< operations::construct_binary2_with_arg<LazySet2,Controller> >,
                                                 MasqueradedTop > > {
   typedef modified_container_pair_impl<Rows> base_t;
protected:
   ~Rows();
public:
   const typename base_t::container1& get_container1() const
   {
      return rows(this->hidden().get_container1());
   }
   const typename base_t::container2& get_container2() const
   {
      return rows(this->hidden().get_container2());
   }
   typename base_t::operation get_operation() const
   {
      return this->hidden().get_operation();
   }
};

template <typename MatrixRef1, typename MatrixRef2, typename Controller>
class Cols< LazyIncidenceMatrix2<MatrixRef1, MatrixRef2, Controller> >
   : public modified_container_pair_impl< Cols< LazyIncidenceMatrix2<MatrixRef1,MatrixRef2,Controller> >,
                                          mlist< Container1RefTag< masquerade<pm::Cols, MatrixRef1> >,
                                                 Container2RefTag< masquerade<pm::Cols, MatrixRef2> >,
                                                 OperationTag< operations::construct_binary2_with_arg<LazySet2,Controller> >,
                                                 MasqueradedTop > > {
   typedef modified_container_pair_impl<Cols> base_t;
protected:
   ~Cols();
public:
   const typename base_t::container1& get_container1() const
   {
      return cols(this->hidden().get_container1());
   }
   const typename base_t::container2& get_container2() const
   {
      return cols(this->hidden().get_container2());
   }
   typename base_t::operation get_operation() const
   {
      return this->hidden().get_operation();
   }
};

namespace operations {

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_incidence_matrix, is_incidence_matrix> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef decltype(std::declval<LeftRef>() * std::declval<RightRef>()) result_type;

   template <typename L, typename R>
   result_type operator() (L&& l, R&& r) const
   {
      return std::forward<L>(l) * std::forward<R>(r);
   }

   template <typename L, typename R>
   void assign(L&& l, const R& r) const
   {
      l *= r;
   }
};

template <typename LeftRef, typename RightRef>
struct concat_impl<LeftRef, RightRef, cons<is_set, is_set> > {
   typedef LeftRef  first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename chain_compose<IncidenceLineChain, true>::template with<unwary_t<LeftRef>, unwary_t<RightRef>>
      result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(unwary(l), unwary(r));
   }
};

} // end namespace operations

} // end namespace pm

namespace polymake {
   using pm::GenericIncidenceMatrix;
}

namespace std {
   template <typename Matrix1, typename Matrix2>
   void swap(pm::GenericIncidenceMatrix<Matrix1>& m1, pm::GenericIncidenceMatrix<Matrix2>& m2)
   {
      m1.top().swap(m2.top());
   }
}

#ifdef POLYMAKE_GENERIC_MATRIX_H
# include "polymake/internal/Incidence_and_SparseMatrix.h"
#endif

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
