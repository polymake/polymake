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

/** @file IncidenceMatrix.h
    @brief Implementation of pm::IncidenceMatrix class
*/


#ifndef POLYMAKE_INCIDENCE_MATRIX_H
#define POLYMAKE_INCIDENCE_MATRIX_H

#include "polymake/internal/sparse2d.h"
#include "polymake/Set.h"
#include "polymake/GenericIncidenceMatrix.h"
#include "polymake/permutations.h"

namespace pm {

template <typename Set>
class incidence_proxy_base {
protected:
   Set* s;
   int j;

   bool get() const { return s->exists(j); }

   void insert() { s->insert(j); }

   void erase() { s->erase(j); }

   void toggle() { s->toggle(j); }
public:
   typedef bool value_type;

   incidence_proxy_base(Set& s_arg, int j_arg)
      : s(&s_arg), j(j_arg) {}
};

template <typename TreeRef> class incidence_line;
template <bool rowwise, typename BaseRef=void> class incidence_line_factory;
template <typename symmetric> class IncidenceMatrix_base;

template <typename TreeRef>
struct incidence_line_params
   : mlist_concat< typename sparse2d::line_params<TreeRef>::type,
                   OperationTag< BuildUnaryIt<operations::index2element> > > {};

template <typename TreeRef>
class incidence_line_base
   : public modified_tree< incidence_line<TreeRef>, typename incidence_line_params<TreeRef>::type > {
protected:
   typedef nothing first_arg_type;
   typedef nothing second_arg_type;
   ~incidence_line_base();
public:
   int index() const { return this->get_container().get_line_index(); }
};

template <typename Tree>
class incidence_line_base<Tree&>
   : public modified_tree< incidence_line<Tree&>, typename incidence_line_params<Tree&>::type > {
protected:
   using tree_type = typename deref<Tree>::type;
   using symmetric = std::conditional_t<Tree::symmetric, Symmetric, NonSymmetric>;
   using matrix_ref = typename inherit_ref<IncidenceMatrix_base<symmetric>, Tree&>::type;
   using alias_t = alias<matrix_ref>;

   alias_t matrix;
   int line_index;

public:
   template <typename Arg, typename=std::enable_if_t<std::is_constructible<alias_t, Arg>::value>>
   incidence_line_base(Arg&& matrix_arg, int index_arg)
      : matrix(std::forward<Arg>(matrix_arg))
      , line_index(index_arg) {}

   decltype(auto) get_container()
   {
      return matrix->get_table().get_line(line_index, (tree_type*)0);
   }
   decltype(auto) get_container() const
   {
      return matrix->get_table().get_line(line_index, (tree_type*)0);
   }

   int index() const { return line_index; }
};

template <typename TreeRef>
class incidence_line
   : public incidence_line_base<TreeRef>
   , public GenericMutableSet<incidence_line<TreeRef>, int, operations::cmp>
{
   using base_t = incidence_line_base<TreeRef>;
   friend class GenericMutableSet<incidence_line>;
   template <typename> friend class IncidenceMatrix;
   template <sparse2d::restriction_kind> friend class RestrictedIncidenceMatrix;
public:
   using incidence_line_base<TreeRef>::incidence_line_base;

   incidence_line& operator= (const incidence_line& other)
   {
      return incidence_line::generic_mutable_type::operator=(other);
   }

   // TODO: investigate whether the dimension check is active?
   template <typename Set>
   incidence_line& operator= (const GenericSet<Set, int, operations::cmp>& other)
   {
      return incidence_line::generic_mutable_type::operator=(other);
   }

   incidence_line& operator= (std::initializer_list<int> l)
   {
      return incidence_line::generic_mutable_type::operator=(l);
   }

protected:
   template <typename Iterator>
   void fill(Iterator src)
   {
      this->clear();
      for (; !src.at_end(); ++src)
         this->insert(*src);
   }
};

template <typename TreeRef>
struct check_container_feature<incidence_line<TreeRef>, sparse_compatible> : std::true_type {};

template <typename TreeRef>
struct spec_object_traits< incidence_line<TreeRef> >
   : spec_object_traits<is_container> {
   static constexpr bool is_temporary = attrib<TreeRef>::is_reference, is_always_const = attrib<TreeRef>::is_const;
   using masquerade_for = std::conditional_t<is_temporary, void, typename deref<TreeRef>::type>;
   static constexpr int is_resizeable=0;
};

template <typename Iterator>
using is_sequence_of_sets=std::is_same<typename object_traits<typename iterator_traits<Iterator>::value_type>::generic_tag, is_set>;

template <typename Container>
using fits_for_append_to_IM
   = mlist_or<isomorphic_to_container_of<Container, int, allow_conversion>,
              isomorphic_to_container_of<Container, Set<int>, allow_conversion>,
              isomorphic_to_container_of<Container, IncidenceMatrix<>, allow_conversion>,
              std::is_same<typename object_traits<Container>::generic_tag, is_incidence_matrix> >;

template <sparse2d::restriction_kind restriction=sparse2d::only_rows>
class RestrictedIncidenceMatrix
   : public matrix_methods<RestrictedIncidenceMatrix<restriction>, bool> {
protected:
   typedef sparse2d::restriction_const<restriction> my_restriction;
   typedef sparse2d::restriction_const<(restriction==sparse2d::only_rows ? sparse2d::only_cols : sparse2d::only_rows)> cross_restriction;

   typedef sparse2d::Table<nothing, false, restriction> table_type;
   table_type data;

   table_type& get_table() { return data; }
   const table_type& get_table() const { return data; }

   template <typename Iterator, typename TLines>
   static
   void copy_linewise(Iterator&& src, TLines& lines, my_restriction, std::true_type)
   {
      copy_range(std::forward<Iterator>(src), entire(lines));
   }

   template <typename Iterator, typename TLines>
   static
   void copy_linewise(Iterator&& src, TLines& lines, my_restriction, std::false_type)
   {
      for (auto l_i=entire(lines); !l_i.at_end(); ++l_i, ++src)
         l_i->fill(entire(*src));
   }

   template <typename Iterator, typename TLines, typename TSourceOrdered>
   static
   void copy_linewise(Iterator&& src, TLines& lines, cross_restriction, TSourceOrdered)
   {
      for (int i=0; !src.at_end(); ++src, ++i)
         append_across(lines, *src, i);
   }

   template <typename TLines, typename TSet>
   static
   void append_across(TLines& lines, const TSet& set, int i)
   {
      for (auto s=entire(set); !s.at_end(); ++s)
         lines[*s].push_back(i);
   }

   using proxy_base = incidence_proxy_base< incidence_line<typename table_type::primary_tree_type> >;

public:
   using value_type = bool;
   using reference = sparse_elem_proxy<proxy_base>;
   using const_reference = bool;

   explicit RestrictedIncidenceMatrix(int n=0) : data(n) {}

   RestrictedIncidenceMatrix(int r, int c) : data(r,c) {}

   template <typename Iterator, typename How,
             typename=std::enable_if_t<is_among<How, sparse2d::rowwise, sparse2d::columnwise>::value &&
                                                assess_iterator_value<Iterator, can_initialize, Set<int>>::value &&
                                                (How::value==restriction || assess_iterator<Iterator, check_iterator_feature, end_sensitive>::value)>>
   RestrictedIncidenceMatrix(int n, How how, Iterator&& src)
      : data(n)
   {
      copy_linewise(ensure_private_mutable(std::forward<Iterator>(src)), lines(*this, my_restriction()),
                    how, is_sequence_of_sets<Iterator>());
   }

   template <typename Iterator, typename How,
             typename=std::enable_if_t<is_among<How, sparse2d::rowwise, sparse2d::columnwise>::value &&
                                                assess_iterator_value<Iterator, can_initialize, Set<int>>::value &&
                                                (How::value==restriction || assess_iterator<Iterator, check_iterator_feature, end_sensitive>::value)>>
   RestrictedIncidenceMatrix(int r, int c, How how, Iterator&& src)
      : data(r, c)
   {
      copy_linewise(ensure_private_mutable(std::forward<Iterator>(src)), lines(*this, my_restriction()),
                    how, is_sequence_of_sets<Iterator>());
   }

   template <typename How, typename... Sources,
             typename=std::enable_if_t<is_among<How, sparse2d::rowwise, sparse2d::columnwise>::value &&
                                                mlist_and_nonempty<fits_for_append_to_IM<Sources>...>::value>>
   RestrictedIncidenceMatrix(How how, const Sources&... src)
      : data(0)
   {
      append_impl(how, src...);
   }

   RestrictedIncidenceMatrix(std::initializer_list<std::initializer_list<int>> l)
      : data(l.size())
   {
      static_assert(restriction==sparse2d::only_rows, "a column-only restricted incidence matrix can't be constructed from an initializer list");
      copy_linewise(l.begin(), pm::rows(*this), my_restriction(), std::false_type());
   }

   RestrictedIncidenceMatrix(RestrictedIncidenceMatrix&& M)
      : data(std::move(M.data)) {}

   void swap(RestrictedIncidenceMatrix& M) { data.swap(M.data); }

   void clear() { data.clear(); }

protected:
   proxy_base random_impl(int i, int j, std::false_type)
   {
      return proxy_base(this->row(i), j);
   }
   proxy_base random_impl(int i, int j, std::true_type)
   {
      return proxy_base(this->col(j), i);
   }
   bool random_impl(int i, int j, std::false_type) const
   {
      return this->row(i).exists(j);
   }
   bool random_impl(int i, int j, std::true_type) const
   {
      return this->col(j).exists(i);
   }
public:
   reference operator() (int i, int j)
   {
      return random_impl(i, j, bool_constant<restriction==sparse2d::only_cols>());
   }
   const_reference operator() (int i, int j) const
   {
      return random_impl(i, j, bool_constant<restriction==sparse2d::only_cols>());
   }

   bool exists(int i, int j) const
   {
      return random_impl(i, j, bool_constant<restriction==sparse2d::only_cols>());
   }

private:
   auto append_lines_start(sparse2d::rowwise, int n)
   {
      const int oldrows=data.rows();
      data.resize_rows(oldrows+n);
      return pm::rows(*this).begin()+oldrows;
   }

   auto append_lines_start(sparse2d::columnwise, int n)
   {
      const int oldcols=data.cols();
      data.resize_cols(oldcols+n);
      return pm::cols(*this).begin()+oldcols;
   }

   template <typename Container, typename... MoreSources>
   auto append_lines_start(my_restriction how,
                           std::enable_if_t<isomorphic_to_container_of<Container, int, allow_conversion>::value, int> n,
                           const Container& c, MoreSources&&... more_src)
   {
      return append_lines_start(how, n+1, std::forward<MoreSources>(more_src)...);
   }

   template <typename Container, typename... MoreSources>
   auto append_lines_start(my_restriction how,
                           std::enable_if_t<isomorphic_to_container_of<Container, Set<int>, allow_conversion>::value, int> n,
                           const Container& c, MoreSources&&... more_src)
   {
      return append_lines_start(how, n+c.size(), std::forward<MoreSources>(more_src)...);
   }

   template <typename TMatrix, typename... MoreSources>
   auto append_lines_start(my_restriction how, int n, const GenericIncidenceMatrix<TMatrix>& m, MoreSources&&... more_src)
   {
      return append_lines_start(how, n+(restriction==sparse2d::only_rows ? m.rows() : m.cols()), std::forward<MoreSources>(more_src)...);
   }

   template <typename Container, typename... MoreSources>
   auto append_lines_start(my_restriction how,
                           std::enable_if_t<isomorphic_to_container_of<Container, IncidenceMatrix<>, allow_conversion>::value, int> n,
                           const Container& c, MoreSources&&... more_src)
   {
      for (const auto& m : c)
         n += (restriction==sparse2d::only_rows ? m.rows() : m.cols());
      return append_lines_start(how, n, std::forward<MoreSources>(more_src)...);
   }

   template <typename... Sources>
   int append_lines_start(cross_restriction, int, Sources&&...)
   {
      return restriction==sparse2d::only_rows ? data.cols() : data.rows();
   }

   template <typename Iterator, typename TSet>
   void append_lines_from(my_restriction, Iterator& dst, const GenericSet<TSet, int, operations::cmp>& s)
   {
      *dst=s.top();
      ++dst;
   }

   template <typename Iterator, typename Container>
   std::enable_if_t<isomorphic_to_container_of<Container, int, is_set>::value>
   append_lines_from(my_restriction, Iterator& dst, const Container& c)
   {
      dst->fill(entire(c));
      ++dst;
   }

   template <typename THow, typename Iterator, typename TMatrix>
   void append_lines_from(THow how, Iterator& dst, const GenericIncidenceMatrix<TMatrix>& m)
   {
      for (auto src=entire(sparse2d::lines(m.top(), how)); !src.at_end(); ++src)
         append_lines_from(how, dst, *src);
   }

   template <typename Iterator, typename Container>
   std::enable_if_t<isomorphic_to_container_of<Container, Set<int>, allow_conversion>::value ||
                    isomorphic_to_container_of<Container, IncidenceMatrix<>, allow_conversion>::value>
   append_lines_from(my_restriction how, Iterator& dst, const Container& c)
   {
      for (auto src=entire(c); !src.at_end(); ++src)
         append_lines_from(how, dst, *src);
   }

   template <typename Container>
   std::enable_if_t<isomorphic_to_container_of<Container, int, allow_conversion>::value>
   append_lines_from(cross_restriction, int& r, const Container& c)
   {
      append_across(sparse2d::lines(*this, my_restriction()), c, r);
      ++r;
   }

   template <typename How, typename Iterator>
   void append_lines(How, Iterator&) {}

   template <typename How, typename Iterator, typename Source, typename... MoreSources>
   void append_lines(How how, Iterator&& dst, const Source& src, MoreSources&&... more_src)
   {
      append_lines_from(how, dst, src);
      append_lines(how, dst, std::forward<MoreSources>(more_src)...);
   }

   template <typename How, typename... Sources>
   void append_impl(How how, Sources&&... src)
   {
      append_lines(how, append_lines_start(how, 0, std::forward<Sources>(src)...), std::forward<Sources>(src)...);
   }

public:
   template <typename TMatrix>
   RestrictedIncidenceMatrix& operator/= (const GenericIncidenceMatrix<TMatrix>& m)
   {
      append_impl(sparse2d::rowwise(), m);
      return *this;
   }

   template <typename TSet>
   RestrictedIncidenceMatrix& operator/= (const GenericSet<TSet, int, operations::cmp>& s)
   {
      append_impl(sparse2d::rowwise(), s.top());
      return *this;
   }

   template <typename TMatrix>
   RestrictedIncidenceMatrix& operator|= (const GenericIncidenceMatrix<TMatrix>& m)
   {
      append_impl(sparse2d::columnwise(), m);
      return *this;
   }

   template <typename TSet>
   RestrictedIncidenceMatrix& operator|= (const GenericSet<TSet, int, operations::cmp>& s)
   {
      append_impl(sparse2d::columnwise(), s.top());
      return *this;
   }

   /// append one or more rows
   template <typename... Sources,
             typename=std::enable_if_t<mlist_and_nonempty<fits_for_append_to_IM<Sources>...>::value>>
   void append_rows(const Sources&... src)
   {
      append_impl(sparse2d::rowwise(), src...);
   }

   /// append one or more columns
   template <typename... Sources,
             typename=std::enable_if_t<mlist_and_nonempty<fits_for_append_to_IM<Sources>...>::value>>
   void append_columns(const Sources&... src)
   {
      append_impl(sparse2d::columnwise(), src...);
   }

   void squeeze() { data.squeeze(); }

   template <typename Permutation>
   std::enable_if_t<isomorphic_to_container_of<Permutation, int>::value>
   permute_rows(const Permutation& perm)
   {
      data.permute_rows(perm, std::false_type());
   }

   template <typename Permutation>
   std::enable_if_t<isomorphic_to_container_of<Permutation, int>::value>
   permute_cols(const Permutation& perm)
   {
      data.permute_cols(perm, std::false_type());
   }

   template <typename InvPermutation>
   std::enable_if_t<isomorphic_to_container_of<InvPermutation, int>::value>
   permute_inv_rows(const InvPermutation& inv_perm)
   {
      data.permute_rows(inv_perm, std::true_type());
   }

   template <typename InvPermutation>
   std::enable_if_t<isomorphic_to_container_of<InvPermutation, int>::value>
   permute_inv_cols(const InvPermutation& inv_perm)
   {
      data.permute_cols(inv_perm, std::true_type());
   }

#if POLYMAKE_DEBUG
   void check() const { data.check(); }
#endif
   friend class Rows<RestrictedIncidenceMatrix>;
   friend class Cols<RestrictedIncidenceMatrix>;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Rows;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Cols;
   template <typename> friend class IncidenceMatrix;
};

template <sparse2d::restriction_kind restriction>
class Rows< RestrictedIncidenceMatrix<restriction> >
   : public sparse2d::Rows< RestrictedIncidenceMatrix<restriction>, nothing, false, restriction,
                            operations::masquerade<incidence_line> > {
protected:
   ~Rows();
public:
   using container_category = std::conditional_t<restriction==sparse2d::only_rows, random_access_iterator_tag, output_iterator_tag>;
};

template <sparse2d::restriction_kind restriction>
class Cols< RestrictedIncidenceMatrix<restriction> >
   : public sparse2d::Cols< RestrictedIncidenceMatrix<restriction>, nothing, false, restriction,
                            operations::masquerade<incidence_line> > {
protected:
   ~Cols();
public:
   using container_category = std::conditional<restriction==sparse2d::only_cols, random_access_iterator_tag, output_iterator_tag>;
};

template <sparse2d::restriction_kind restriction>
struct spec_object_traits< RestrictedIncidenceMatrix<restriction> >
   : spec_object_traits<is_container> {
   static const int dimension=2;

   using serialized = std::conditional_t<restriction==sparse2d::only_rows,
                                         Rows< RestrictedIncidenceMatrix<restriction> >,
                                         Cols< RestrictedIncidenceMatrix<restriction> >>;

   static serialized& serialize(RestrictedIncidenceMatrix<restriction>& M)
   {
      return reinterpret_cast<serialized&>(M);
   }
   static const serialized& serialize(const RestrictedIncidenceMatrix<restriction>& M)
   {
      return reinterpret_cast<const serialized&>(M);
   }
};

template <typename symmetric>
class IncidenceMatrix_base {
protected:
   using table_type = sparse2d::Table<nothing, symmetric::value>;
   shared_object<table_type, AliasHandlerTag<shared_alias_handler>> data;

   table_type& get_table() { return *data; }
   const table_type& get_table() const { return *data; }

   friend IncidenceMatrix_base& make_mutable_alias(IncidenceMatrix_base& alias, IncidenceMatrix_base& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }
   IncidenceMatrix_base() = default;

   IncidenceMatrix_base(int r, int c)
      : data(r, c) {}

   template <sparse2d::restriction_kind restriction>
   explicit IncidenceMatrix_base(sparse2d::Table<nothing, symmetric::value, restriction>&& input_data)
      : data(std::move(input_data)) {}

   template <typename> friend class Rows;
   template <typename> friend class Cols;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Rows;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Cols;
   template <bool, typename> friend class incidence_line_factory;
   template <typename> friend class incidence_line_base;
   template <typename, alias_kind> friend class alias;
};

template <typename symmetric>
class Rows< IncidenceMatrix_base<symmetric> >
   : public sparse2d::Rows< IncidenceMatrix_base<symmetric>, nothing, symmetric::value, sparse2d::full,
                            operations::masquerade<incidence_line> > {
protected:
   ~Rows();
};

template <typename symmetric>
class Cols< IncidenceMatrix_base<symmetric> >
   : public sparse2d::Cols< IncidenceMatrix_base<symmetric>, nothing, symmetric::value, sparse2d::full,
                            operations::masquerade<incidence_line> > {
protected:
   ~Cols();
};


/** @class IncidenceMatrix
    @brief 0/1 incidence matrix.

   The only @ref persistent class from the incidence matrix family.
   The implementation is based on a two-dimensional grid of <a href="AVL.html">balanced binary search (AVL) trees</a>,
   the same as for @see SparseMatrix. The whole internal data structure
   is attached to a smart pointer with @see {reference counting}.

   A symmetric incidence matrix is a square matrix whose elements `(i,j)` and `(j,i)`
   are always equal. Internally it is stored in a triangular form, avoiding redundant elements, but appears as a full square.

*/

template <typename symmetric>
class IncidenceMatrix
   : public IncidenceMatrix_base<symmetric>
   , public GenericIncidenceMatrix< IncidenceMatrix<symmetric> > {
protected:
   using base_t = IncidenceMatrix_base<symmetric>;

   friend IncidenceMatrix& make_mutable_alias(IncidenceMatrix& alias, IncidenceMatrix& owner)
   {
      return static_cast<IncidenceMatrix&>(make_mutable_alias(static_cast<base_t&>(alias), static_cast<base_t&>(owner)));
   }

   /// initialize from a dense boolean sequence in row order
   template <typename Iterator>
   void init_impl(Iterator&& src, std::true_type)
   {
      const int n=this->cols();
      for (auto r_i=entire(pm::rows(static_cast<base_t&>(*this))); !r_i.at_end(); ++r_i) {
         int i=0;
         if (symmetric::value) {
            i=r_i.index();
            std::advance(src,i);
         }
         for (; i<n; ++i, ++src)
            if (*src) r_i->push_back(i);
      }
   }

   /// input already ordered
   template <typename Iterator>
   void init_rowwise(Iterator&& src, std::true_type)
   {
      copy_range(std::forward<Iterator>(src), entire(pm::rows(static_cast<base_t&>(*this))));
   }

   /// input in uncertain order
   template <typename Iterator>
   void init_rowwise(Iterator&& src, std::false_type)
   {
      for (auto r_i=entire(pm::rows(static_cast<base_t&>(*this))); !r_i.at_end(); ++r_i, ++src)
         r_i->fill(entire(*src));
   }

   /// initialize rowwise from a sequence of sets
   template <typename Iterator>
   void init_impl(Iterator&& src, std::false_type)
   {
      init_rowwise(std::forward<Iterator>(src), is_sequence_of_sets<Iterator>());
   }

   typedef incidence_proxy_base< incidence_line<typename base_t::table_type::primary_tree_type> > proxy_base;
public:
   using unknown_columns_type = std::conditional_t<symmetric::value, void, RestrictedIncidenceMatrix<>>;
   using value_type = bool;
   using reference = sparse_elem_proxy<proxy_base>;
   using const_reference = bool;

   /// Create an empty IncidenceMatrix.
   IncidenceMatrix() {}

   /// Create an empty IncidenceMatrix with @a r rows and @a c columns initialized with zeroes.
   IncidenceMatrix(int r, int c)
      : base_t(r,c) {}

   /** @brief Create an IncidenceMatrix IncidenceMatrix with @a r rows and @a c columns and initialize it from a data sequence.

       @a src should iterate either over @a r&times;@a c boolean values, corresponding to the
       elements in the row order (the column index changes first,) or over @a r sets with
       integer elements (or convertible to integer), which are assigned to the matrix rows.

       In the symmetric case the redundant elements must be present in the input sequence; their values are ignored.

       @param r the number of rows
       @param c the number of columns
       @param src an iterator
   */
   template <typename Iterator>
   IncidenceMatrix(int r, int c, Iterator&& src)
      : base_t(r, c)
   {
      init_impl(ensure_private_mutable(std::forward<Iterator>(src)),
                bool_constant<(object_traits<typename iterator_traits<Iterator>::value_type>::total_dimension==0)>());
   }

   IncidenceMatrix(const GenericIncidenceMatrix<IncidenceMatrix>& M)
      : base_t(M.top()) {}

   template <typename Matrix2, typename=std::enable_if_t<IncidenceMatrix::template compatible_symmetry_types<Matrix2>()>>
   IncidenceMatrix(const GenericIncidenceMatrix<Matrix2>& M)
      : base_t(M.rows(), M.cols())
   {
      init_impl(pm::rows(M).begin(), std::false_type());
   }

   template <sparse2d::restriction_kind restriction, typename=std::enable_if_t<!symmetric::value && restriction != sparse2d::full>>
   explicit IncidenceMatrix(RestrictedIncidenceMatrix<restriction>&& M)
      : base_t(std::move(M.data)) {}

   /// Construct a matrix by rowwise or columnwise concatenation of given matrices and/or sets.
   /// Dimensions are set automatically to encompass all input elements.
   template <typename How, typename... Sources,
             typename=std::enable_if_t<!symmetric::value && is_among<How, sparse2d::rowwise, sparse2d::columnwise>::value &&
             mlist_and_nonempty<fits_for_append_to_IM<Sources>...>::value>>
   IncidenceMatrix(How how, const Sources&... src)
      : base_t(RestrictedIncidenceMatrix<>(how, src...).data) {}

   /// Construct a matrix from a given sequence of row sets.
   /// Number of columns is set automatically to encompass all input elements.
   template <typename Container, typename=std::enable_if_t<!symmetric::value && isomorphic_to_container_of<Container, Set<int>, allow_conversion>::value>>
   explicit IncidenceMatrix(const Container& src)
      : base_t(RestrictedIncidenceMatrix<>(src.size(), sparse2d::rowwise(), src.begin()).data) {}

   /// Construct a matrix with a prescribed number of columns from a given sequence of row sets
   template <typename Container,
             typename=std::enable_if_t<!symmetric::value && isomorphic_to_container_of<Container, Set<int>, allow_conversion>::value>>
   IncidenceMatrix(const Container& src, int c)
      : base_t(src.size(), c)
   {
      init_impl(src.begin(), std::false_type());
   }

   IncidenceMatrix(int r, int c, std::initializer_list<bool> l)
      : base_t(r, c)
   {
      if (POLYMAKE_DEBUG && r*c != l.size())
         throw std::runtime_error("initializer_list size does not match the dimensions");
      init_impl(l.begin(), std::true_type());
   }

   IncidenceMatrix(std::initializer_list<std::initializer_list<int>> l)
      : base_t(RestrictedIncidenceMatrix<>(l).data) {}

   IncidenceMatrix& operator= (const IncidenceMatrix& other) { assign(other); return *this; }
   using IncidenceMatrix::generic_type::operator=;

   template <sparse2d::restriction_kind restriction, typename=std::enable_if_t<!symmetric::value && restriction != sparse2d::full>>
   IncidenceMatrix& operator= (RestrictedIncidenceMatrix<restriction>&& M)
   {
      this->data.replace(std::move(M.data));
      return *this;
   }

   /// Swap the contents with that of another matrix in an efficient way.
   void swap(IncidenceMatrix& M) { this->data.swap(M.data); }

   friend void relocate(IncidenceMatrix* from, IncidenceMatrix* to)
   {
      relocate(&from->data, &to->data);
   }

   /**  @brief Extend or truncate to new dimensions (@a m rows, @a n columns).
        Surviving elements keep their values, new elements are implicitly @c false.

        @c IncidenceMatrix deploys an adaptive reallocation strategy similar to @c std::vector,
        reserving additional stock memory by every reallocation. If you repeatedly increase the matrix dimensions by one,
        the amortized reallocation costs will be proportional to the logarithm of the final dimension.

        A special case, looking at the first glance like a "no operation": @c{ M.resize(M.rows(), M.cols()) },
        gets rid of this extra allocated storage.
   */
   void resize(int m, int n) { this->data->resize(m,n); }

   /// Clear contents.
   void clear() { this->data.apply(shared_clear()); }

   /// Clear contents.
   void clear(int r, int c) { this->data.apply(typename base_t::table_type::shared_clear(r,c)); }

   /// Entry at row i column j.
   reference operator() (int i, int j)
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=this->rows() || j<0 || j>=this->cols())
            throw std::runtime_error("IncidenceMatrix::operator() - index out of range");
      }
      return proxy_base(pm::rows(static_cast<base_t&>(*this))[i],j);
   }

   /// Entry at row i column j (const).
   const_reference operator() (int i, int j) const
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=this->rows() || j<0 || j>=this->cols())
            throw std::runtime_error("IncidenceMatrix::operator() - index out of range");
      }
      return pm::rows(static_cast<const base_t&>(*this))[i].exists(j);
   }

   /// Returns the entry at position (i,j).
   bool exists(int i, int j) const { return operator()(i,j); }

   template <typename row_number_consumer, typename col_number_consumer>
   void squeeze(const row_number_consumer& rnc, const col_number_consumer& cnc) { this->data->squeeze(rnc,cnc); }

   template <typename row_number_consumer>
   void squeeze(const row_number_consumer& rnc) { this->data->squeeze(rnc); }

   /// Delete empty rows and columns, renumber the rest and reduce the dimensions.
   void squeeze() { this->data->squeeze(); }

   template <typename row_number_consumer>
   void squeeze_rows(const row_number_consumer& rnc) { this->data->squeeze_rows(rnc); }

   /// Delete empty rows, renumber the rest and reduce the dimensions.
   void squeeze_rows() { this->data->squeeze_rows(); }

   template <typename col_number_consumer>
   void squeeze_cols(const col_number_consumer& cnc) { this->data->squeeze_cols(cnc); }

   /// Delete empty columns, renumber the rest and reduce the dimensions.
   void squeeze_cols() { this->data->squeeze_cols(); }

   /// Permute the rows according to the given permutation.
   template <typename Permutation>
   std::enable_if_t<isomorphic_to_container_of<Permutation, int>::value>
   permute_rows(const Permutation& perm)
   {
      this->data->permute_rows(perm, std::false_type());
   }

   /// Permute the columns according to the given permutation.
   template <typename Permutation>
   std::enable_if_t<isomorphic_to_container_of<Permutation, int>::value>
   permute_cols(const Permutation& perm)
   {
      this->data->permute_cols(perm, std::false_type());
   }

   /// Permute the rows according to the inverse of the given permutation.
   template <typename InvPermutation>
   std::enable_if_t<isomorphic_to_container_of<InvPermutation, int>::value>
   permute_inv_rows(const InvPermutation& inv_perm)
   {
      this->data->permute_rows(inv_perm, std::true_type());
   }

   /// Permute the columns according to the inverse of the given permutation.
   template <typename InvPermutation>
   std::enable_if_t<isomorphic_to_container_of<InvPermutation, int>::value>
   permute_inv_cols(const InvPermutation& inv_perm)
   {
      this->data->permute_cols(inv_perm, std::true_type());
   }

   template <typename Permutation, typename InvPermutation,
             typename=std::enable_if_t<symmetric::value, typename mproject1st<void, Permutation>::type>>
   IncidenceMatrix copy_permuted(const Permutation& perm, const InvPermutation& inv_perm) const
   {
      const int n=this->rows();
      IncidenceMatrix result(n,n);
      result.data.get()->copy_permuted(*this->data, perm, inv_perm);
      return result;
   }

#if POLYMAKE_DEBUG
   void check() const { this->data->check(); }
#endif
protected:
   void assign(const GenericIncidenceMatrix<IncidenceMatrix>& M) { this->data=M.top().data; }

   template <typename Matrix>
   void assign(const GenericIncidenceMatrix<Matrix>& M)
   {
      if (this->data.is_shared() || this->rows() != M.rows() || this->cols() != M.cols())
         // circumvent the symmetry checks, they are already done in GenericIncidenceMatrix methods
         assign(IncidenceMatrix(M.rows(), M.cols(), pm::rows(M).begin()));
      else
         GenericIncidenceMatrix<IncidenceMatrix>::assign(M);
   }

   template <typename Matrix2>
   void append_rows(const Matrix2& m)
   {
      const int old_rows=this->rows();
      this->data.apply(typename base_t::table_type::shared_add_rows(m.rows()));
      copy_range(entire(pm::rows(m)), pm::rows(static_cast<base_t&>(*this)).begin()+old_rows);
   }

   template <typename Set2>
   void append_row(const Set2& s)
   {
      const int old_rows=this->rows();
      this->data.apply(typename base_t::table_type::shared_add_rows(1));
      this->row(old_rows)=s;
   }

   template <typename Matrix2>
   void append_cols(const Matrix2& m)
   {
      const int old_cols=this->cols();
      this->data.apply(typename base_t::table_type::shared_add_cols(m.cols()));
      copy_range(entire(pm::cols(m)), pm::cols(static_cast<base_t&>(*this)).begin()+old_cols);
   }

   template <typename Set2>
   void append_col(const Set2& s)
   {
      const int old_cols=this->cols();
      this->data.apply(typename base_t::table_type::shared_add_cols(1));
      this->col(old_cols)=s;
   }

   template <typename> friend class GenericIncidenceMatrix;
   friend class Rows<IncidenceMatrix>;
   friend class Cols<IncidenceMatrix>;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Rows;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Cols;
   template <typename, typename> friend class BlockMatrix;
};

template <typename symmetric>
struct check_container_feature< IncidenceMatrix<symmetric>, Symmetric >
   : bool_constant<symmetric::value> {};

template <bool rowwise, typename BaseRef>
class incidence_line_factory {
public:
   typedef BaseRef first_argument_type;
   typedef int second_argument_type;
   using tree_type = std::conditional_t<rowwise, typename pure_type_t<BaseRef>::table_type::row_tree_type,
                                                 typename pure_type_t<BaseRef>::table_type::col_tree_type>;
   typedef incidence_line<typename inherit_ref<tree_type, BaseRef>::type> result_type;

   result_type operator() (BaseRef matrix, int index) const
   {
      return result_type(matrix, index);
   }
};

template <bool rowwise>
class incidence_line_factory<rowwise, void> : public operations::incomplete {};

template <bool rowwise, typename BaseRef>
struct operation_cross_const_helper< incidence_line_factory<rowwise, BaseRef> > {
   typedef incidence_line_factory<rowwise, typename attrib<BaseRef>::minus_const> operation;
   typedef incidence_line_factory<rowwise, typename attrib<BaseRef>::plus_const> const_operation;
};

template <bool rowwise, typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< incidence_line_factory<rowwise>, Iterator1, Iterator2, Reference1, Reference2>
   : empty_op_builder< incidence_line_factory<rowwise,Reference1> > {};

template <typename TSymmetric>
class Rows< IncidenceMatrix<TSymmetric> >
   : public modified_container_pair_impl< Rows< IncidenceMatrix<TSymmetric> >,
                                          mlist< Container1Tag< same_value_container< IncidenceMatrix_base<TSymmetric>& > >,
                                                 Container2Tag< sequence >,
                                                 OperationTag< pair< incidence_line_factory<true>,
                                                                     BuildBinaryIt<operations::dereference2> > >,
                                                 MasqueradedTop > > {
protected:
   ~Rows();
public:
   auto get_container1()
   {
      return same_value_container< IncidenceMatrix_base<TSymmetric>& >(this->hidden());
   }
   auto get_container1() const
   {
      return same_value_container< const IncidenceMatrix_base<TSymmetric>& >(this->hidden());
   }
   sequence get_container2() const
   {
      return sequence(0, this->hidden().get_table().rows());
   }
   void resize(int n)
   {
      this->hidden().get_table().resize_rows(n);
   }
};

template <typename TSymmetric>
class Cols< IncidenceMatrix<TSymmetric> >
   : public modified_container_pair_impl< Cols< IncidenceMatrix<TSymmetric> >,
                                          mlist< Container1Tag< same_value_container< IncidenceMatrix_base<TSymmetric>& > >,
                                                 Container2Tag< sequence >,
                                                 OperationTag< pair< incidence_line_factory<false>,
                                                                     BuildBinaryIt<operations::dereference2> > >,
                                                 MasqueradedTop > > {
protected:
   ~Cols();
public:
   auto get_container1()
   {
      return same_value_container< IncidenceMatrix_base<TSymmetric>& >(this->hidden());
   }
   auto get_container1() const
   {
      return same_value_container< const IncidenceMatrix_base<TSymmetric>& >(this->hidden());
   }
   sequence get_container2() const
   {
      return sequence(0, this->hidden().get_table().cols());
   }
   void resize(int n)
   {
      this->hidden().get_table().resize_cols(n);
   }
};

/// Convolution of two incidence relations.
template <typename Matrix1, typename Matrix2>
IncidenceMatrix<>
convolute(const GenericIncidenceMatrix<Matrix1>& m1, const GenericIncidenceMatrix<Matrix2>& m2)
{
   if (POLYMAKE_DEBUG || is_wary<Matrix1>() || is_wary<Matrix2>()) {
      if (m1.cols() != m2.rows())
         throw std::runtime_error("convolute - dimension mismatch");
   }
   IncidenceMatrix<> result(m1.rows(), m2.cols());
   auto r1=rows(m1).begin();
   for (auto dst=entire(rows(result)); !dst.at_end();  ++dst, ++r1)
      accumulate_in(entire(rows(m2.minor(*r1,All))), BuildBinary<operations::add>(), *dst);

   return result;
}

template <typename TMatrix, typename Permutation>
std::enable_if_t<!TMatrix::is_symmetric, typename TMatrix::persistent_type>
permuted_rows(const GenericIncidenceMatrix<TMatrix>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_rows - dimension mismatch");
   }
   return IncidenceMatrix<>(RestrictedIncidenceMatrix<sparse2d::only_rows>(m.rows(), m.cols(), sparse2d::rowwise(), select(rows(m), perm).begin()));
}

template <typename TMatrix, typename Permutation>
std::enable_if_t<!TMatrix::is_symmetric, typename TMatrix::persistent_type>
permuted_cols(const GenericIncidenceMatrix<TMatrix>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.cols() != perm.size())
         throw std::runtime_error("permuted_cols - dimension mismatch");
   }
   return IncidenceMatrix<>(RestrictedIncidenceMatrix<sparse2d::only_cols>(m.rows(), m.cols(), sparse2d::columnwise(), select(cols(m), perm).begin()));
}

template <typename TMatrix, typename Permutation>
std::enable_if_t<!TMatrix::is_symmetric, typename TMatrix::persistent_type>
permuted_inv_rows(const GenericIncidenceMatrix<TMatrix>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_inv_rows - dimension mismatch");
   }
   RestrictedIncidenceMatrix<sparse2d::only_rows> result(m.rows(), m.cols());
   copy_range(entire(rows(m)), select(rows(result), perm).begin());
   return IncidenceMatrix<>(std::move(result));
}

template <typename TMatrix, typename Permutation>
std::enable_if_t<!TMatrix::is_symmetric, typename TMatrix::persistent_type>
permuted_inv_cols(const GenericIncidenceMatrix<TMatrix>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.cols() != perm.size())
         throw std::runtime_error("permuted_inv_cols - dimension mismatch");
   }
   RestrictedIncidenceMatrix<sparse2d::only_cols> result(m.rows(), m.cols());
   copy_range(entire(cols(m)), select(cols(result), perm).begin());
   return IncidenceMatrix<>(std::move(result));
}

template <typename TMatrix, typename Permutation>
std::enable_if_t<TMatrix::is_symmetric, typename TMatrix::persistent_type>
permuted_rows(const GenericIncidenceMatrix<TMatrix>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_rows - dimension mismatch");
   }
   std::vector<int> inv_perm(m.rows());
   inverse_permutation(perm,inv_perm);
   return m.top().copy_permuted(perm,inv_perm);
}

template <typename TMatrix, typename Permutation>
std::enable_if_t<TMatrix::is_symmetric && container_traits<Permutation>::is_random, typename TMatrix::persistent_type>
permuted_inv_rows(const GenericIncidenceMatrix<TMatrix>& m, const Permutation& inv_perm)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.rows() != inv_perm.size())
         throw std::runtime_error("permuted_inv_rows - dimension mismatch");
   }
   std::vector<int> perm(m.rows());
   inverse_permutation(inv_perm,perm);
   return m.copy_permuted(perm,inv_perm);
}

template <typename TMatrix, typename Permutation>
std::enable_if_t<TMatrix::is_symmetric && !container_traits<Permutation>::is_random, typename TMatrix::persistent_type>
permuted_inv_rows(const GenericIncidenceMatrix<TMatrix>& m, const Permutation& inv_perm)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.rows() != inv_perm.size())
         throw std::runtime_error("permuted_inv_rows - dimension mismatch");
   }
   std::vector<int> inv_perm_copy(inv_perm.size());
   copy_range(entire(inv_perm), inv_perm_copy.begin());
   return permuted_inv_rows(m,inv_perm_copy);
}

template <typename TMatrix, typename Permutation>
std::enable_if_t<TMatrix::is_symmetric, typename TMatrix::persistent_type>
permuted_cols(const GenericIncidenceMatrix<TMatrix>& m, const Permutation& perm)
{
   return permuted_rows(m,perm);
}

template <typename TMatrix, typename Permutation>
std::enable_if_t<TMatrix::is_symmetric, typename TMatrix::persistent_type>
permuted_inv_cols(const GenericIncidenceMatrix<TMatrix>& m, const Permutation& inv_perm)
{
   return permuted_inv_rows(m,inv_perm);
}

} // end namespace pm

namespace polymake {
   using pm::IncidenceMatrix;
   using pm::RestrictedIncidenceMatrix;
}

namespace std {
   template <typename symmetric>
   void swap(pm::IncidenceMatrix<symmetric>& M1, pm::IncidenceMatrix<symmetric>& M2) { M1.swap(M2); }

   template <pm::sparse2d::restriction_kind restriction>
   void swap(pm::RestrictedIncidenceMatrix<restriction>& M1,
             pm::RestrictedIncidenceMatrix<restriction>& M2)
   {
      M1.swap(M2);
   }

   template <typename TreeRef>
   void swap(pm::incidence_line<TreeRef>& l1, pm::incidence_line<TreeRef>& l2)
   {
      l1.swap(l2);
   }
}

#endif // POLYMAKE_INCIDENCE_MATRIX_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
