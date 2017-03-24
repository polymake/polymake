/* Copyright (c) 1997-2017
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

#ifndef POLYMAKE_GENERIC_IO_H
#define POLYMAKE_GENERIC_IO_H

#include "polymake/internal/sparse.h"
#include "polymake/internal/matrix_rows_cols.h"
#include <typeinfo>

namespace pm {

template <typename> class CheckEOF {};
template <typename> class TrustedValue {};
template <typename> class SparseRepresentation {};
template <typename> class LookForward {};

template <typename T>
struct ignore_in_composite : std::false_type {};

template <>
struct ignore_in_composite<nothing> : std::true_type {};

template <typename T>
struct ignore_in_composite<const T> : ignore_in_composite<T> {};

template <typename T>
struct ignore_in_composite<T&> : ignore_in_composite<T> {};

template <typename T,
          bool _fake=object_traits<typename attrib<T>::minus_const>::is_temporary || is_masquerade<T>::value>
struct item4insertion {
   typedef typename attrib<T>::minus_const type;
};

template <typename T1, typename T2>
struct item4insertion<pair<T1,T2>, false> {
   typedef pair<typename attrib<T1>::minus_const, typename attrib<T2>::minus_const> type;
};

template <typename T>
struct item4insertion<T, true> {
   typedef typename object_traits<T>::persistent_type type;
};

template <typename T, template <typename> class Property, typename Model=typename object_traits<T>::model>
struct structural_helper : std::false_type {};

template <typename T, template <typename> class Property>
struct structural_helper<T, Property, is_container>
   : Property<typename T::value_type> {};

template <typename T, template <typename> class Property>
struct structural_helper<T, Property, is_composite>
   : list_accumulate_unary<list_and, Property, typename list_transform_unary<deref, typename object_traits<T>::elements>::type> {};

template <typename T> struct is_parseable;
template <typename T> struct is_printable;
template <typename T> struct is_parseable_or_serializable;
template <typename T> struct is_printable_or_serializable;
template <typename T> struct is_readable;
template <typename T> struct is_writeable;

template <typename Top, typename Options, int subst_pos=0>
class GenericIOoptions {
   template <typename NewOptions>
   struct subst_helper : mreplace_template_parameter<Top, subst_pos, NewOptions> {};

   template <typename OptionInst>
   struct set_helper : subst_helper<typename mtagged_list_replace<Options, OptionInst>::type> {};

   template <typename OptionInst>
   struct get_helper;

   template <template <typename> class TTag, typename TDefault>
   struct get_helper<TTag<TDefault>>
      : mtagged_list_extract<Options, TTag, TDefault> {};

   template <template <typename> class NewOption, typename OptionInst>
   struct copy_helper : set_helper< NewOption<typename get_helper<OptionInst>::type> > {};

   template <template <typename> class NewOption, typename OptionInst>
   struct neg_helper : set_helper< NewOption<typename bool_not<typename get_helper<OptionInst>::type>::type> > {};
public:
   template <typename OptionInst>
   static constexpr bool get_option(OptionInst) { return get_helper<OptionInst>::type::value; }

   Top& set_option() { return static_cast<Top&>(*this); }

   template <typename OptionInst>
   typename set_helper<OptionInst>::type&
   set_option(OptionInst)
   {
      return reinterpret_cast<typename set_helper<OptionInst>::type&>(static_cast<Top&>(*this));
   }

   template <template <typename> class NewOption, typename OptionInst>
   typename copy_helper<NewOption,OptionInst>::type&
   copy_option(OptionInst)
   {
      return reinterpret_cast<typename copy_helper<NewOption, OptionInst>::type&>(static_cast<Top&>(*this));
   }

   template <template <typename> class NewOption, typename OptionInst>
   typename neg_helper<NewOption,OptionInst>::type&
   copy_neg_option(OptionInst)
   {
      return reinterpret_cast<typename neg_helper<NewOption, OptionInst>::type&>(static_cast<Top&>(*this));
   }
};

template <typename Input>
class GenericInput {
public:
   static bool serialized_value() { return false; }
   Input& top() { return static_cast<Input&>(*this); }
   Input& top() const { return static_cast<Input&>(const_cast<GenericInput&>(*this)); }
};

template <typename Output>
class GenericOutput {
public:
   static bool serialized_value() { return false; }
   Output& top() { return static_cast<Output&>(*this); }
   Output& top() const { return static_cast<Output&>(const_cast<GenericOutput&>(*this)); }
};

namespace io_test {

DeclTypedefCHECK(unknown_columns_type);

template <typename T, bool enable=has_unknown_columns_type<T>::value>
struct unknown_columns : int_constant<std::is_same<typename T::unknown_columns_type, void>::value+0> {};

template <typename T>
struct unknown_columns<T, false> : int_constant<-1> {};

struct fallback {
   template <typename Any>
   fallback(const Any&);
};

nothing& operator>> (const fallback&, const fallback&);
nothing& operator<< (const fallback&, const fallback&);

#define DeclHasIOoperatorCHECK(name,arrow)                        \
template <typename Data, typename Stream, typename Result=Stream> \
struct has_##name##_operator_impl {                               \
   static Stream& stream();                                       \
   static Data& data();                                           \
   struct helper {                                                \
      static derivation::yes Test(Result&);                       \
      static derivation::no Test(nothing&);                       \
   };                                                             \
   static const bool value= sizeof(helper::Test( stream() arrow data() )) == sizeof(derivation::yes); \
}

DeclHasIOoperatorCHECK(input,>>);
DeclHasIOoperatorCHECK(output,<<);

template <typename Input, typename Data>
struct has_generic_input_operator
   : bool_constant< has_input_operator_impl<Data,GenericInput<Input>,Input>::value > {};

template <typename Output, typename Data>
struct has_generic_output_operator
   : bool_constant< has_output_operator_impl<Data,GenericOutput<Output>,Output>::value > {};

template <typename Data>
struct has_std_input_operator
   : bool_constant< has_input_operator_impl<Data,std::istream>::value > {};

template <typename Data>
struct has_std_output_operator
   : bool_constant< has_output_operator_impl<Data,std::ostream>::value > {};

template <typename Container>
struct has_insert {
// std::<something>::insert takes a const_iterator since C++11
   typedef typename Container::const_iterator container_it;

   struct helper {
      static derivation::yes Test(const typename Container::iterator&);
      static derivation::yes Test(const std::pair<typename Container::iterator, bool>&); 
      static derivation::no Test(nothing&);
   };
   struct mix_in : public Container {
      using Container::insert;
      nothing& insert(const fallback&);
      nothing& insert(container_it, const fallback&);
   };
   static container_it it();
   static const typename Container::value_type& val();

   static const bool without_position= sizeof(helper::Test(std::declval<mix_in&>().insert(val()) )) == sizeof(derivation::yes),
                     with_position= sizeof(helper::Test(std::declval<mix_in&>().insert(it(),val()) )) == sizeof(derivation::yes);
};

template <typename Apparent> struct as_list {};
struct as_set {};
struct by_inserting : as_set {};
template <int TResizeable, bool TAllowSparse> struct as_array {};
template <int TResizeable> struct as_sparse : as_array<TResizeable, true> {};
template <int TResizeable> struct as_matrix {};

template <typename Data, bool trusted=true,
          int TDim=object_traits<Data>::dimension>
struct input_mode;

template <typename Data, bool trusted,
          bool TOrderedItems=(has_insert<Data>::with_position && trusted) || !has_insert<Data>::without_position,
          bool TMutable=is_mutable<typename iterator_traits<typename Data::iterator>::reference>::value>
struct input_list {
   // primary: ordered items, mutable
   typedef as_list<Data> type;
};

template <typename Data, bool trusted>
struct input_list<Data, trusted, true, false> {
   // ordered items, immutable
   typedef as_set type;
};

template <typename Data, bool trusted, bool TMutable>
struct input_list<Data, trusted, false, TMutable> {
   // !ordered items || !trusted
   typedef by_inserting type;
};

template <typename Data, bool trusted,
          bool TRandom=is_derived_from<typename container_traits<Data>::category, random_access_iterator_tag>::value,
          bool TSparse=check_container_feature<Data, sparse>::value,
          int TResizeable=object_traits<Data>::is_resizeable,
          bool TMutable=is_mutable<typename iterator_traits<typename Data::iterator>::reference>::value>
struct input_mode1 : input_list<Data, trusted> {};

template <typename Data, bool trusted, int TResizeable>
struct input_mode1<Data, trusted, true, false, TResizeable, true> {     // random, !sparse
   typedef as_array<TResizeable, object_traits<Data>::allow_sparse> type;
};

template <typename Data, bool trusted, bool TRandom, int TResizeable, bool TMutable>
struct input_mode1<Data, trusted, TRandom, true, TResizeable, TMutable> {       // sparse
   static const int element_dim=object_traits<typename Data::value_type>::total_dimension;
   typedef typename std::conditional<element_dim==0, as_sparse<TResizeable>, as_array<TResizeable, false>>::type type;
};

template <typename Data, bool trusted>
struct input_mode1<Data, trusted, false, false, 0, true> {      // !random, !resizeable, mutable
   typedef as_array<0, false> type;
};

template <typename Data, bool trusted>
struct input_mode<Data, trusted, 1> : input_mode1<Data, trusted> {};

template <typename Data, bool trusted>
struct input_mode<Data, trusted, 2> {
   typedef as_matrix<object_traits<Data>::is_resizeable> type;
};

} // end namespace io_test

template <typename Input>
class GenericInputImpl : public GenericInput<Input> {
   template <typename> friend class GenericInputImpl;
public:
   typedef GenericInputImpl generic_impl;

   template <typename Data>
   Input& operator>> (Data& data)
   {
      dispatch_generic_io(concrete(data), io_test::has_generic_input_operator<Input, Data>());
      return this->top();
   }

   template <typename Data, size_t n>
   Input& operator>> (Data (&a)[n])
   {
      retrieve_container(this->top(), array2container(a), io_test::as_array<0, false>());
      return this->top();
   }

protected:
   template <typename Data>
   void dispatch_generic_io(Data& data, std::true_type)
   {
      static_cast<GenericInput<Input>&>(*this) >> data;
   }

   template <typename Data>
   void dispatch_generic_io(Data& data, std::false_type)
   {
      // no generic input operator defined
      dispatch_serialized(data, has_serialized<Data>(), bool_constant<is_readable<Data>::value>());
   }

   template <typename Data>
   void dispatch_serialized(Data& data, std::true_type, std::true_type)
   {
      if (this->top().serialized_value())
         this->top() >> serialize(data);
      else
         dispatch_serialized(data, std::false_type(), bool_constant<is_parseable<Data>::value || structural_helper<Data, is_parseable_or_serializable>::value>());
   }

   template <typename Data>
   void dispatch_serialized(Data& data, std::false_type, std::true_type)
   {
      // no serialization, can read as is
      dispatch_retrieve(data, typename object_traits<Data>::model());
   }

   template <typename Data, typename THasSerialized>
   void dispatch_serialized(Data& data, THasSerialized, std::false_type)
   {
      // no serialization, no input operators
      throw std::invalid_argument((has_serialized<Data>::value ? "only serialized input possible for " : "no input operators known for ") + legible_typename<Data>());
   }

   template <typename Data, typename Model>
   void dispatch_retrieve(Data& data, Model)
   {
      this->top().fallback(data);
   }

   template <typename Data>
   void dispatch_retrieve(Data& data, is_container)
   {
      retrieve_container(this->top(), data, typename io_test::input_mode<Data, Input::get_option(TrustedValue<std::true_type>())>::type());
   }

   template <typename Data>
   void dispatch_retrieve(Data& data, is_composite)
   {
      retrieve_composite(this->top(), data);
   }
};

template <typename Input, typename Data, typename Masquerade>
int retrieve_container(Input& src, Data& data, io_test::as_list<Masquerade>)
{
   auto&& c=src.begin_list(reinterpret_cast<Masquerade*>(&data));
   typename Data::iterator dst=data.begin(), end=data.end();
   int size=0;

   while (dst != end && !c.at_end()) {
      c >> *dst;
      ++dst;  ++size;
   }

   if (c.at_end()) {
      data.erase(dst, end);
   } else {
      do {
         c >> *data.insert(end, typename Data::value_type());
         ++size;
      } while (!c.at_end());
   }

   return size;
}

template <typename Input, typename Data>
void retrieve_container(Input& src, Data& data, io_test::as_set)
{
   data.clear();
   auto&& c=src.begin_list(&data);
   typedef typename item4insertion<typename Data::value_type>::type item_type;
   item_type item=item_type();
   typename Data::iterator end=data.end();
   while (!c.at_end()) {
      c >> item;
      data.insert(end, item);
   }
}

template <typename Input, typename Data>
void retrieve_container(Input& src, Data& data, io_test::by_inserting)
{
   data.clear();
   auto&& c=src.begin_list(&data);
   typedef typename item4insertion<typename Data::value_type>::type item_type;
   item_type item=item_type();
   while (!c.at_end()) {
      c >> item;
      data.insert(item);
   }
}

template <typename Value, typename CursorRef>
class list_reader {
protected:
   alias<CursorRef> cursor;
   typedef typename deref<CursorRef>::type cursor_type;
   Value item;
   bool _end;

   void load()
   {
      typename attrib<cursor_type>::plus_ref c=*cursor;
      if (c.at_end())
         _end=true;
      else
         c >> item;
   }
public:
   list_reader(typename alias<CursorRef>::arg_type cursor_arg)
      : cursor(cursor_arg), _end(false) { load(); }

   typedef input_iterator_tag iterator_category;
   typedef Value value_type;
   typedef const Value& reference;
   typedef const Value* pointer;
   typedef ptrdiff_t difference_type;

   reference operator* () const { return item; }
   pointer operator-> () const { return &item; }

   bool at_end() const { return _end; }

   list_reader& operator++ () { load(); return *this; }
private:
   list_reader& operator++ (int);
};

template <typename Input, typename Data> inline
void fill_dense_from_dense(Input& src, Data& data)
{
   for (typename Entire<Data>::iterator dst=entire(data); !dst.at_end(); ++dst) {
      typename Entire<Data>::iterator::reference dst_item=*dst;
      src >> dst_item;
   }
   src.finish();
}

template <typename Input, typename Data>
void fill_dense_from_sparse(Input& src, Data& data, int d)
{
   operations::clear<typename Data::value_type> zero;
   typename Data::iterator dst=data.begin();
   int i=0;
   while (!src.at_end()) {
      const int pos=src.index();
      while (i<pos) {
         zero(*dst);
         ++dst; ++i;
      }
      src >> *dst;
      ++dst; ++i;
   }
   while (i<d) {
      zero(*dst);
      ++dst; ++i;
   }
}

template <typename Input, typename Data>
void fill_sparse_from_dense(Input& src, Data& data)
{
   typename Entire<Data>::iterator dst=entire(data);
   typename Data::value_type v;
   int i=-1;
   if (!dst.at_end()) {
      for (;;) {
         ++i; src >> v;
         if (!is_zero(v)) {
            if (dst.index() > i) {
               data.insert(dst, i, v);
            } else {
               *dst=v; ++dst;
               if (dst.at_end()) break;
            }
         } else if (dst.index() == i) {
            data.erase(dst++);
            if (dst.at_end()) break;
         }
      }
   }
   while (!src.at_end()) {
      ++i; src >> v;
      if (!is_zero(v))
         data.insert(dst, i, v);
   }
}

template <typename Input, typename Data, typename LimitDim>
void fill_sparse_from_sparse(Input& src, Data& data, const LimitDim& limit_dim)
{
   typename Entire<Data>::iterator dst=entire(data);
   if (!dst.at_end()) {
      while (!src.at_end()) {
         const int index=src.index();
         if (! Input::get_option(TrustedValue<std::true_type>()) &&
             (index<0 || index>=data.dim()))
            throw std::runtime_error("sparse input - element index out of range");
         if (index > dst.index()) {
            do
               data.erase(dst++);
            while (!dst.at_end() && index > dst.index());
            if (dst.at_end()) {
               src >> *data.insert(dst,index);
               break;
            }
         }
         if (index < dst.index()) {
            src >> *data.insert(dst,index);
         } else {
            src >> *dst;
            ++dst;
            if (dst.at_end()) break;
         }
      }
   }
   if (src.at_end()) {
      while (!dst.at_end())
         data.erase(dst++);
   } else {
      do {
         const int index=src.index();
         if (index > limit_dim) {
            if (!ignore_in_composite<typename Input::value_type>::value)
               src.skip_item();
            src.skip_rest(); src.finish();
            break;
         }
         src >> *data.insert(dst,index);
      } while (!src.at_end());
   }
}

template <typename Data> inline
maximal<int> get_input_limit(Data&) { return maximal<int>(); }

template <typename Input, typename Data> inline
void resize_and_fill_dense_from_dense(Input& src, Data& data)
{
   data.resize(src.size());
   fill_dense_from_dense(src, data);
}

template <typename Input, typename Data> inline
void check_and_fill_dense_from_dense(Input& src, Data& data)
{
   if (!Input::get_option(TrustedValue<std::true_type>()) &&
       src.size() != data.size())
      throw std::runtime_error("array input - dimension mismatch");
   fill_dense_from_dense(src, data);
}

template <typename Input, typename Data> inline
void resize_and_fill_dense_from_sparse(Input& src, Data& data)
{
   const int d=src.lookup_dim(false);
   data.resize(d);
   fill_dense_from_sparse(src, data, d);
}

template <typename Input, typename Data> inline
void check_and_fill_dense_from_sparse(Input& src, Data& data)
{
   const int d=src.lookup_dim(false);
   if (!Input::get_option(TrustedValue<std::true_type>()) &&
       d != data.size())
      throw std::runtime_error("sparse input - dimension mismatch");
   fill_dense_from_sparse(src, data, d);
}

template <typename Input, typename Data> inline
void resize_and_fill_sparse_from_dense(Input& src, Data& data, std::true_type)
{
   data.resize(src.size());
   fill_sparse_from_dense(src, data);
}

template <typename Input, typename Data> inline
void resize_and_fill_sparse_from_dense(Input&, Data&, std::false_type)
{
   throw std::runtime_error("expected sparse input");
}

template <typename Input, typename Data> inline
void check_and_fill_sparse_from_dense(Input& src, Data& data)
{
   if (!Input::get_option(TrustedValue<std::true_type>()) &&
       src.size() != data.dim())
      throw std::runtime_error("array input - dimension mismatch");
   fill_sparse_from_dense(src, data);
}

template <typename Input, typename Data> inline
void resize_and_fill_sparse_from_sparse(Input& src, Data& data, std::true_type)
{
   data.resize(src.lookup_dim(false));
   fill_sparse_from_sparse(src, data, maximal<int>());
}

template <typename Input, typename Data> inline
void resize_and_fill_sparse_from_sparse(Input& src, Data& data, std::false_type)
{
   fill_sparse_from_sparse(src, data, maximal<int>());
}

template <typename Input, typename Data> inline
void check_and_fill_sparse_from_sparse(Input& src, Data& data)
{
   if (!Input::get_option(TrustedValue<std::true_type>()) &&
       src.lookup_dim(false) != data.dim())
      throw std::runtime_error("sparse input - dimension mismatch");
   fill_sparse_from_sparse(src, data, get_input_limit(data));
}

// resizeable, sparse input allowed
template <typename Input, typename Data> inline
void retrieve_container(Input& src, Data& data, io_test::as_array<1, true>)
{
   auto&& c=src.begin_list(&data);
   if (c.sparse_representation())
      resize_and_fill_dense_from_sparse(c.set_option(SparseRepresentation<std::true_type>()), data);
   else
      resize_and_fill_dense_from_dense(c.set_option(SparseRepresentation<std::false_type>()), data);
}

// resizeable, only dense input allowed
template <typename Input, typename Data> inline
void retrieve_container(Input& src, Data& data, io_test::as_array<1, false>)
{
   auto&& c=src.begin_list(&data);
   if (!Input::get_option(TrustedValue<std::true_type>()) &&
       c.sparse_representation())
      throw std::runtime_error("sparse input not allowed");
   resize_and_fill_dense_from_dense(c.set_option(SparseRepresentation<std::false_type>()), data);
}

// non-resizeable, sparse input allowed
template <typename Input, typename Data> inline
void retrieve_container(Input& src, Data& data, io_test::as_array<0, true>)
{
   auto&& c=src.begin_list(&data);
   if (c.sparse_representation())
      check_and_fill_dense_from_sparse(c.set_option(SparseRepresentation<std::true_type>()), data);
   else
      check_and_fill_dense_from_dense(c.set_option(SparseRepresentation<std::false_type>()).template copy_neg_option<CheckEOF>(TrustedValue<std::true_type>()), data);
}

// non-resizeable, only dense input allowed
template <typename Input, typename Data>
void retrieve_container(Input& src, Data& data, io_test::as_array<0, false>)
{
   auto&& c=src.begin_list(&data);
   if (!Input::get_option(TrustedValue<std::true_type>()) &&
       c.sparse_representation())
      throw std::runtime_error("sparse input not allowed");
   check_and_fill_dense_from_dense(c.set_option(SparseRepresentation<std::false_type>()).template copy_neg_option<CheckEOF>(TrustedValue<std::true_type>()), data);
}

// resizeable
template <typename Input, typename Data, int is_resizeable> inline
void retrieve_container(Input& src, Data& data, io_test::as_sparse<is_resizeable>)
{
   auto&& c=src.begin_list(&data);
   if (c.sparse_representation())
      resize_and_fill_sparse_from_sparse(c.set_option(SparseRepresentation<std::true_type>()), data, bool_constant<(is_resizeable>0)>());
   else
      resize_and_fill_sparse_from_dense(c.set_option(SparseRepresentation<std::false_type>()), data, bool_constant<(is_resizeable>0)>());
}

// non-resizeable
template <typename Input, typename Data> inline
void retrieve_container(Input& src, Data& data, io_test::as_sparse<0>)
{
   auto&& c=src.begin_list(&data);
   if (c.sparse_representation())
      check_and_fill_sparse_from_sparse(c.set_option(SparseRepresentation<std::true_type>()), data);
   else
      check_and_fill_sparse_from_dense(c.set_option(SparseRepresentation<std::false_type>()).template copy_neg_option<CheckEOF>(TrustedValue<std::true_type>()), data);
}

// matrix can be read without knowing the number of columns in advance
template <typename Input, typename Data> inline
void resize_and_fill_matrix(Input& src, Data& data, int r, int_constant<0>)
{
   const int c=src.cols(!std::is_same<typename object_traits<typename Data::row_type>::generic_tag, is_set>::value);
   if (c>=0) {
      data.clear(r, c);
      fill_dense_from_dense(src, rows(data));
   } else {
      typename Data::unknown_columns_type raw_data(r);
      fill_dense_from_dense(src, rows(raw_data));
      data=std::move(raw_data);
   }
}

// the number of columns must be known in advance
template <typename Input, typename Data> inline
void resize_and_fill_matrix(Input& src, Data& data, int r, int_constant<-1>)
{
   const int c=src.cols(!std::is_same<typename object_traits<typename Data::row_type>::generic_tag, is_set>::value);
   if (c>=0) {
      data.clear(r, c);
      fill_dense_from_dense(src, rows(data));
   } else {
      throw std::runtime_error("can't determine the number of columns");
   }
}

// the number of columns is not interesting at all (e.g. a symmetric matrix)
template <typename Input, typename Data> inline
void resize_and_fill_matrix(Input& src, Data& data, int r, int_constant<1>)
{
   data.clear(r, 0);
   fill_dense_from_dense(src, rows(data));
}

// fully resizeable
template <typename Input, typename Data>
void retrieve_container(Input& src, Data& data, io_test::as_matrix<2>)
{
   auto&& c=src.begin_list((Rows<Data>*)0);
   if (!Input::get_option(TrustedValue<std::true_type>()) &&
       c.sparse_representation())
      throw std::runtime_error("sparse input not allowed");
   resize_and_fill_matrix(c, data, c.size(), io_test::unknown_columns<Data>());
}

// resizeable via rows()
template <typename Input, typename Data, int TResizeable>
typename std::enable_if<(TResizeable < 2)>::type
retrieve_container(Input& src, Data& data, io_test::as_matrix<TResizeable>)
{
   retrieve_container(src, rows(data), io_test::as_array<TResizeable, false>());
}

template <typename T, typename CursorRef>
class composite_reader : public composite_reader<typename list_tail<T>::type, CursorRef> {
protected:
   typedef composite_reader<typename list_tail<T>::type, CursorRef> super;
   static const bool is_last=false;
public:
   typedef typename list_head<T>::type element_type;
   typedef typename deref<element_type>::type value_type;

   composite_reader(typename alias<CursorRef>::arg_type cursor_arg)
      : super(cursor_arg) {}

   super& operator<< (typename attrib<element_type>::plus_ref elem)
   {
      typename attrib<CursorRef>::plus_ref c=*this->cursor;
      if (c.at_end()) {
         operations::clear<value_type> zero;
         zero(elem);
      } else {
         c >> elem;
      }
      if (super::is_last) c.finish();
      return *this;
   }
};

template <typename CursorRef>
class composite_reader<void, CursorRef> {
protected:
   alias<CursorRef> cursor;

   composite_reader(typename alias<CursorRef>::arg_type cursor_arg) : cursor(cursor_arg) {}

   static const bool is_last=true;
};

template <typename Input, typename Data>
void retrieve_composite(Input& src, Data& data)
{
   typedef typename Input::template composite_cursor<Data>::type cursor_type;
   cursor_type c=src.begin_composite(&data);
   composite_reader<typename object_traits<Data>::elements, typename attrib<cursor_type>::plus_ref> r(c);
   object_traits<Data>::visit_elements(data, r);
}

template <typename Output>
class GenericOutputImpl : public GenericOutput<Output> {
public:
   typedef GenericOutputImpl generic_impl;

   template <typename Data>
   Output& operator<< (const Data& data)
   {
      dispatch_generic_io(concrete(data), io_test::has_generic_output_operator<Output, Data>());
      return this->top();
   }

   template <typename Data, size_t n>
   Output& operator<< (const Data (&data)[n])
   {
      store_container(array2container(data), std::false_type());
      return this->top();
   }

   template <size_t n>
   Output& operator<< (const char (&s)[n])
   {
      this->top().fallback(s, n-1);
      return this->top();
   }

protected:
   template <typename Data>
   void dispatch_generic_io(const Data& data, std::true_type)
   {
      static_cast<GenericOutput<Output>&>(*this) << data;
   }

   template <typename Data>
   void dispatch_generic_io(const Data& data, std::false_type)
   {
      // no generic output operator defined
      dispatch_serialized(data, has_serialized<Data>(), bool_constant<is_writeable<Data>::value>());
   }

   template <typename Data>
   void dispatch_serialized(const Data& data, std::true_type, std::true_type)
   {
      if (this->top().serialized_value())
         this->top() << serialize(data);
      else
         dispatch_serialized(data, std::false_type(), bool_constant<is_printable<Data>::value || structural_helper<Data, is_printable_or_serializable>::value>());
   }

   template <typename Data>
   void dispatch_serialized(const Data& data, std::false_type, std::true_type)
   {
      // no serialization, can write as is
      dispatch_store(data, typename object_traits<Data>::model());
   }

   template <typename Data, typename THasSerialized>
   void dispatch_serialized(const Data& data, THasSerialized, std::false_type)
   {
      // no serialization, no output operators
      throw std::invalid_argument((has_serialized<Data>::value ? "only serialized output possible for " : "no output operators known for ") + legible_typename<Data>());
   }

   template <typename Data, typename Model>
   void dispatch_store(const Data& data, Model)
   {
      this->top().fallback(data);
   }

   template <typename Data>
   void dispatch_store(const Data& data, is_container)
   {
      dispatch_container(data, int_constant<object_traits<Data>::dimension>());
   }

   template <typename Data>
   void dispatch_container(const Data& data, int_constant<1>)
   {
      store_container(data, bool_constant<check_container_feature<Data, sparse>::value>());
   }

   template <typename Data>
   void dispatch_container(const Data& data, int_constant<2>)
   {
      dispatch_store(rows(data), is_container());
   }

   template <typename Data>
   void store_container(const Data& data, std::false_type)
   {
      store_list(data);
   }

   template <typename Data>
   void store_container(const Data& data, std::true_type)
   {
      const int choose_sparse=this->top().choose_sparse_representation();
      if (choose_sparse>0 || (choose_sparse==0 && data.prefer_sparse_representation()))
         store_sparse(data);
      else
         store_dense(data, typename object_traits<typename Data::value_type>::model());
   }

   template <typename Data>
   void store_dense(const Data& data, is_scalar)
   {
      store_list(data);
   }

   template <typename Data, typename Model>
   void store_dense(const Data& data, Model)
   {
      auto&& c=this->top().begin_list(&data);
      int ord=0;
      for (auto src=data.begin(); !src.at_end(); ++src, ++ord) {
         for (; ord < src.index(); ++ord)
            c.non_existent();
         c << *src;
      }
      for (const int d=data.dim(); ord<d; ++ord)
         c.non_existent();
   }

   template <typename Data>
   void dispatch_store(const Data& data, is_composite)
   {
      store_composite(data);
   }

   static constexpr int choose_sparse_representation() { return 0; }

public:
   template <typename Data>
   void store_list(const Data& data)
   {
      this->template store_list_as<Data>(data);
   }

   template <typename Data>
   void store_sparse(const Data& data)
   {
      this->template store_sparse_as<Data>(data);
   }

   template <typename Data>
   void store_composite(const Data& data);

   template <typename Masquerade, typename Data>
   void store_list_as(const Data& data);

   template <typename Masquerade, typename Data>
   void store_sparse_as(const Data& data);
};

template <typename Output>
template <typename Masquerade, typename Data>
void GenericOutputImpl<Output>::store_list_as(const Data& data)
{
   auto&& c=this->top().begin_list(reinterpret_cast<const Masquerade*>(&data));
   for (auto src=ensure(data, (cons<dense, end_sensitive>*)0).begin();
        !src.at_end(); ++src)
      c << *src;
   c.finish();
}

template <typename Iterator>
class indexed_pair : public Iterator {
protected:
   indexed_pair();
   ~indexed_pair();
};

template <typename Iterator>
struct spec_object_traits< indexed_pair<Iterator> >
   : spec_object_traits<is_composite> {
   typedef Iterator masquerade_for;
   typedef cons<int, typename iterator_traits<Iterator>::reference> elements;
   template <typename Visitor>
   static void visit_elements(const indexed_pair<Iterator>& it, Visitor& v)
   {
      v << it.index() << *it;
   }
};

template <typename Output>
template <typename Masquerade, typename Data>
void GenericOutputImpl<Output>::store_sparse_as(const Data& data)
{
   auto&& c=this->top().begin_sparse(reinterpret_cast<const Masquerade*>(&data));
   for (auto src=data.begin();  !src.at_end(); ++src)
      c << src;
   c.finish();
}

template <typename T, typename CursorRef>
class composite_writer : public composite_writer<typename list_tail<T>::type, CursorRef> {
protected:
   typedef composite_writer<typename list_tail<T>::type, CursorRef> super;
   static const bool is_last=false;
public:
   typedef typename list_head<T>::type element_type;

   composite_writer(typename alias<CursorRef>::arg_type cursor_arg) : super(cursor_arg) {}

   super& operator<< (typename attrib<element_type>::plus_const_ref elem)
   {
      typename attrib<CursorRef>::plus_ref c=*this->cursor;
      c << elem;
      if (super::is_last) c.finish();
      return *this;
   }
};

template <typename CursorRef>
class composite_writer<void, CursorRef> {
protected:
   alias<CursorRef> cursor;

   composite_writer(typename alias<CursorRef>::arg_type cursor_arg) : cursor(cursor_arg) {}

   static const bool is_last=true;
};

template <typename Output>
template <typename Data>
void GenericOutputImpl<Output>::store_composite(const Data& data)
{
   typedef typename Output::template composite_cursor<Data>::type cursor_type;
   cursor_type c=this->top().begin_composite(&data);
   composite_writer<typename object_traits<Data>::elements, typename attrib<cursor_type>::plus_ref> w(c);
   object_traits<Data>::visit_elements(data, w);
}

template <typename Output>
Output& operator<< (GenericOutput<Output>& os, const nothing&)
{
   return os.top();
}

template <typename Input>
Input& operator>> (GenericInput<Input>& is, const nothing&)
{
   return is.top();
}

// TODO: remove this
template <typename Container>
class IO_Array : public Container {
protected:
   IO_Array();
   ~IO_Array();

   template <typename Output>
   void output(Output& os) const
   {
      os.template store_list_as<IO_Array>(static_cast<const Container&>(*this));
   }
public:
   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& os, const IO_Array& x)
   {
      x.output(os.top());
      return os.top();
   }
};

template <typename Container>
struct redirect_object_traits< IO_Array<Container> > : object_traits<Container> {
   typedef Container masquerade_for;
   static const bool is_temporary=false;
   static const IO_separator_kind IO_separator=IO_sep_containers;
};

// TODO: remove this
template <typename Container>
class IO_List : public Container {
protected:
   IO_List();
   ~IO_List();

   template <typename Output>
   void output(Output& os) const
   {
      os.template store_list_as<IO_List>(static_cast<const Container&>(*this));
   }
public:
   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& os, const IO_List& x)
   {
      x.output(os.top());
      return os.top();
   }
};

template <typename Container>
struct redirect_object_traits< IO_List<Container> > : object_traits<Container> {
   typedef Container masquerade_for;
   static const bool is_temporary=false;
   static const IO_separator_kind IO_separator=IO_sep_inherit;
};

// TODO: remove this
template <typename Container>
class IO_Sparse : public ensure_features<Container, pure_sparse>::container {
protected:
   IO_Sparse();
   ~IO_Sparse();

   template <typename Output>
   void output(Output& os) const
   {
      os.template store_sparse_as<IO_Sparse>(static_cast<const typename ensure_features<Container, pure_sparse>::container&>(*this));
   }
public:
   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& os, const IO_Sparse& x)
   {
      x.output(os.top());
      return os.top();
   }
};

template <typename Container>
struct redirect_object_traits< IO_Sparse<Container> > : object_traits<Container> {
   typedef Container masquerade_for;
   static const bool is_temporary=false;
};

template <typename Container>
struct check_container_feature<IO_Sparse<Container>, pure_sparse> : std::true_type {};

template <typename Container> inline
const IO_Array<Container>& as_array(const Container& c)
{
   return reinterpret_cast<const IO_Array<Container>&>(c);
}

template <typename Container> inline
const IO_List<Container>& as_list(const Container& c)
{
   return reinterpret_cast<const IO_List<Container>&>(c);
}

template <typename Container> inline
const IO_Sparse<Container>& as_sparse(const Container& c)
{
   return reinterpret_cast<const IO_Sparse<Container>&>(c);
}

} // end namespace pm

namespace polymake {
   using pm::GenericInput;
   using pm::GenericOutput;
   using pm::as_array;
   using pm::as_list;
   using pm::as_sparse;
}

#include "polymake/internal/PlainParser.h"

namespace pm {

template <typename T>
struct is_parseable {
   typedef typename deref<T>::type value_type;
   static const bool value=io_test::has_std_input_operator<value_type>::value ||
                           io_test::has_generic_input_operator<PlainParser<>,value_type>::value ||
                           structural_helper<T, pm::is_parseable>::value;
};

template <typename T>
struct is_parseable_or_serializable {
   static const bool value=is_parseable<T>::value || has_serialized<T>::value;
};

template <typename T>
struct is_printable {
   typedef typename deref<T>::type value_type;
   static const bool value=io_test::has_std_output_operator<value_type>::value ||
                           io_test::has_generic_output_operator<PlainPrinter<>,value_type>::value ||
                           structural_helper<T, pm::is_printable>::value;
};

template <typename T>
struct is_printable< Serialized<T> > : std::false_type {};

template <typename T>
struct is_printable_or_serializable {
   static const bool value=is_printable<T>::value || has_serialized<T>::value;
};

template <typename T>
struct is_readable {
   static const bool value=is_parseable_or_serializable<T>::value || structural_helper<T, pm::is_readable>::value;
};

template <typename T>
struct is_writeable {
   static const bool value=is_printable_or_serializable<T>::value || structural_helper<T, pm::is_writeable>::value;
};

}

#endif // POLYMAKE_GENERIC_IO_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
