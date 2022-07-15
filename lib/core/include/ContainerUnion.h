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

#include "polymake/internal/type_union.h"
#include "polymake/internal/iterators.h"

namespace pm {

/* -----------------
    iterator_union
   ----------------- */

template <typename T1, typename T2,
          bool is_viable = std::is_reference<typename compatible<T1, T2>::type>::value ||
                           same_pure_type<T1, T2>::value>
struct union_reference : compatible<T1, T2> {};

template <typename T1, typename T2>
struct union_reference<T1*, T2*, false> : compatible<T1*, T2*> {};

template <typename T1, typename T2>
struct union_reference<T1*&, T2*&, false> : compatible<T1*, T2*> {};

template <typename T1, typename T2>
struct union_reference<T1*&, T2* const&, false> : compatible<T1*, T2*> {};

template <typename T1, typename T2>
struct union_reference<T1* const&, T2*&, false> : compatible<T1*, T2*> {};

template <typename T1, typename T2>
struct union_reference<T1* const&, T2* const&, false> : compatible<T1*, T2*> {};

template <typename T>
struct extract_union_list {
   using type = T;
};

template <typename TypeList>
struct extract_union_list< type_union<TypeList> > {
   using type = TypeList;
};

template <typename T1, typename T2,
          typename Model1 = typename object_traits<typename deref<T1>::type>::model,
          typename Model2 = typename object_traits<typename deref<T2>::type>::model>
struct union_reference_helper {
   using type = type_union< typename mlist_union<typename extract_union_list<T1>::type,
                                                 typename extract_union_list<T2>::type>::type >;
};

template <typename T1, typename T2>
struct union_reference<T1, T2, false> : union_reference_helper<T1, T2> {};

template <typename Iterator>
struct union_iterator_element_traits {
   struct type : iterator_traits<Iterator> {
      using iterator_list = typename iterator_traits<Iterator>::iterator;
      using const_iterator_list = typename iterator_traits<Iterator>::const_iterator;
   };
};

template <typename Traits1, typename Traits2>
struct combine_union_iterator_traits {
   struct type {
      using iterator_category = typename least_derived_class<typename Traits1::iterator_category, typename Traits2::iterator_category>::type;
      using reference = typename union_reference<typename Traits1::reference, typename Traits2::reference>::type;
      using value_type = pure_type_t<reference>;
      using pointer = value_type*;
      using difference_type = typename std::common_type<typename Traits1::difference_type, typename Traits2::difference_type>::type;

      using iterator_list = typename mlist_union<typename Traits1::iterator_list, typename Traits2::iterator_list>::type;
      using const_iterator_list = typename mlist_union<typename Traits1::const_iterator_list, typename Traits2::const_iterator_list>::type;
   };
};

template <typename IteratorList>
using union_iterator_traits = typename mlist_fold_transform<typename mlist_reverse<IteratorList>::type, union_iterator_element_traits, combine_union_iterator_traits>::type;

namespace unions {

template <typename Iterator>
struct iterator_basics : basics<Iterator> {
   static const typename iterator_traits<Iterator>::iterator& get_alt(const char* src)
   {
      return *reinterpret_cast<const typename iterator_traits<Iterator>::iterator*>(src);
   }
};

struct alt_copy_constructor : copy_constructor {
   template <typename Iterator>
   static void execute(char* dst, const char* src)
   {
      basics<Iterator>::construct(dst, iterator_basics<Iterator>::get_alt(src));
   }
};

struct assignment : copy_constructor {
   template <typename Iterator>
   static void execute(char* dst, const char* src)
   {
      basics<Iterator>::get(dst)=basics<Iterator>::get(src);
   }
};

struct alt_assignment : copy_constructor {
   template <typename Iterator>
   static void execute(char* dst, const char* src)
   {
      basics<Iterator>::get(dst)=iterator_basics<Iterator>::get_alt(src);
   }
};

template <typename Ref>
struct star {
   static Ref null(const char* it) { invalid_null_op(); }

   template <typename Iterator>
   static Ref execute(const char* it)
   {
      return *basics<Iterator>::get(it);
   }
};

template <typename Ptr>
struct arrow {
   static Ptr null(const char* it) { invalid_null_op(); }

   template <typename Iterator>
   static Ptr execute(const char* it)
   {
      return basics<Iterator>::get(it).operator->();
   }
};

struct increment {
   static void null(char* it) { invalid_null_op(); }

   template <typename Iterator>
   static void execute(char* it)
   {
      ++basics<Iterator>::get(it);
   }
};

struct decrement : increment {
   template <typename Iterator>
   static void execute(char* it)
   {
      --basics<Iterator>::get(it);
   }
};

struct advance_plus {
   static void null(char* it, Int i) { invalid_null_op(); }

   template <typename Iterator>
   static void execute(char* it, Int i)
   {
      basics<Iterator>::get(it) += i;
   }
};

struct advance_minus : advance_plus {
   template <typename Iterator>
   static void execute(char* it, Int i)
   {
      basics<Iterator>::get(it) -= i;
   }
};

struct equality {
   static bool null(const char* it1, const char* it2) { invalid_null_op(); }

   template <typename Iterator>
   static bool execute(const char* it1, const char* it2)
   {
      return basics<Iterator>::get(it1) == basics<Iterator>::get(it2);
   }
};

template <typename DiffType>
struct difference {
   DiffType null(const char* it1, const char* it2) { invalid_null_op(); }

   template <typename Iterator>
   DiffType execute(const char* it1, const char* it2)
   {
      return basics<Iterator>::get(it1) - basics<Iterator>::get(it2);
   }
};

struct index {
   static Int null(const char* it) { invalid_null_op(); }

   template <typename Iterator>
   static Int execute(const char* it)
   {
      return basics<Iterator>::get(it).index();
   }
};

struct at_end {
   static bool null(const char* it) { invalid_null_op(); }

   template <typename Iterator>
   static bool execute(const char* it)
   {
      return basics<Iterator>::get(it).at_end();
   }
};

struct rewind {
   static void null(char* it) { invalid_null_op(); }

   template <typename Iterator>
   static void execute(char* it)
   {
      basics<Iterator>::get(it).rewind();
   }
};

template <typename Ref>
struct random_it {
   static Ref null(const char* it, Int i) { invalid_null_op(); }

   template <typename Iterator>
   static Ref execute(const char* it, Int i)
   {
      return basics<Iterator>::get(it)[i];
   }
};

} // end namespace unions

template <typename IteratorList, typename Category=typename union_iterator_traits<IteratorList>::iterator_category>
class iterator_union : public type_union<IteratorList> {
protected:
   using base_t = type_union<IteratorList>;
   using traits = union_iterator_traits<IteratorList>;
   using alt_it_list = typename traits::iterator_list;

   template <int discr>
   using basics = typename base_t::template basics<discr>;

   template <typename Operation>
   using function = typename base_t::template function<Operation>;

   template <typename Iterator>
   using mapping = typename base_t::template mapping<Iterator>;

   template <typename Iterator>
   using alt_mapping = unions::Mapping<alt_it_list, typename unions::is_smaller_union<Iterator, alt_it_list>::type_list>;

   template <typename Iterator, int own_discr, int alt_discr>
   void init_from_value(Iterator&& it, mlist<int_constant<own_discr>, int_constant<alt_discr>>)
   {
      constexpr int discr = own_discr >= 0 ? own_discr : alt_discr;
      this->discriminant = discr;
      basics<discr>::construct(this->area, std::forward<Iterator>(it));
   }

   template <typename Iterator>
   std::enable_if_t<unions::is_smaller_union<Iterator, IteratorList>::value>
   init_from_value(const Iterator& it, mlist<int_constant<-1>, int_constant<-1>>)
   {
      this->discriminant = mapping<Iterator>::get(it.discriminant);
      function<unions::copy_constructor>::get(this->discriminant)(this->area, it.area);
   }

   template <typename Iterator>
   std::enable_if_t<!std::is_same<IteratorList, alt_it_list>::value &&
                    unions::is_smaller_union<Iterator, alt_it_list>::value>
   init_from_value(const Iterator& it, mlist<int_constant<-1>, int_constant<-1>>)
   {
      this->discriminant = alt_mapping<Iterator>::get(it.discriminant);
      function<unions::alt_copy_constructor>::get(this->discriminant)(this->area, it.area);
   }

   template <typename Iterator>
   void init_impl(Iterator&& it, std::false_type, std::false_type)
   {
      init_from_value(std::forward<Iterator>(it), mlist< int_constant<mlist_find<IteratorList, pure_type_t<Iterator>>::pos>,
                                                         int_constant<mlist_find<alt_it_list, pure_type_t<Iterator>>::pos>>());
   }

   template <typename Iterator, typename discr2>
   void init_impl(Iterator&& it, std::true_type, discr2)
   {
      base_t::init_impl(std::forward<Iterator>(it), std::true_type());
   }

   template <typename Iterator>
   void init_impl(const Iterator& it, std::false_type, std::true_type)
   {
      this->discriminant = it.discriminant;
      function<unions::alt_copy_constructor>::get(this->discriminant)(this->area, it.area);
   }

   template <typename Iterator, int own_discr, int alt_discr>
   void assign_value(Iterator&& it, mlist<int_constant<own_discr>, int_constant<alt_discr>>)
   {
      constexpr int discr = own_discr >= 0 ? own_discr : alt_discr;
      if (this->discriminant == discr) {
         basics<discr>::get(this->area) = std::forward<Iterator>(it);
      } else {
         this->destroy();
         this->discriminant = discr;
         basics<discr>::construct(this->area, std::forward<Iterator>(it));
      }
   }

   template <typename Iterator>
   std::enable_if_t<unions::is_smaller_union<Iterator, IteratorList>::value>
   assign_value(const Iterator& it, mlist<int_constant<-1>, int_constant<-1>>)
   {
      constexpr int discr=mapping<Iterator>::get(it.discriminant);
      if (this->discriminant == discr) {
         function<unions::assignment>::get(discr)(this->area, it.area);
      } else {
         this->destroy();
         this->discriminant = discr;
         function<unions::copy_constructor>::get(discr)(this->area, it.area);
      }
   }

   template <typename Iterator>
   std::enable_if_t<!std::is_same<IteratorList, alt_it_list>::value &&
                    unions::is_smaller_union<Iterator, alt_it_list>::value>
   assign_value(const Iterator& it, mlist<int_constant<-1>, int_constant<-1>>)
   {
      constexpr int discr = alt_mapping<Iterator>::get(it.discriminant);
      if (this->discriminant == discr) {
         function<unions::alt_assignment>::get(discr)(this->area, it.area);
      } else {
         this->destroy();
         this->discriminant = discr;
         function<unions::alt_copy_constructor>::get(this->discriminant)(this->area, it.area);
      }
   }

   template <typename Iterator>
   static constexpr bool valid_assignment()
   {
      return is_derived_from_any<Iterator, iterator_union, iterator>::value ||
             mlist_contains<IteratorList, Iterator>::value ||
             mlist_contains<alt_it_list, Iterator>::value ||
             unions::is_smaller_union<Iterator, IteratorList>::value ||
             unions::is_smaller_union<Iterator, alt_it_list>::value;
   }

   template <typename Iterator>
   void assign_impl(Iterator&& it, std::false_type)
   {
      assign_value(std::forward<Iterator>(it), mlist<int_constant<mlist_find<IteratorList, pure_type_t<Iterator>>::pos>,
                                                     int_constant<mlist_find<typename traits::iterator_list, pure_type_t<Iterator>>::pos>>());
   }

   template <typename Iterator>
   void assign_impl(Iterator&& it, std::true_type)
   {
      base_t::assign_impl(std::forward<Iterator>(it), std::true_type());
   }

   template <typename,typename> friend class iterator_union;
public:
   using iterator_category = typename traits::iterator_category;
   using value_type = typename traits::value_type;
   using reference = typename traits::reference;
   using pointer = typename traits::pointer;
   using difference_type = typename traits::difference_type;
   using iterator = iterator_union<typename traits::iterator_list>;
   using const_iterator = iterator_union<typename traits::const_iterator_list>;
   using me = iterator_union<IteratorList>;

   iterator_union() {}

   iterator_union(const iterator_union& it)
   {
      base_t::init_impl(it, std::true_type());
   }

   iterator_union(iterator_union&& it)
   {
      base_t::init_impl(std::move(it), std::true_type());
   }

   template <typename Iterator, typename=std::enable_if_t<valid_assignment<pure_type_t<Iterator>>()>>
   iterator_union(Iterator&& it)
   {
      init_impl(std::forward<Iterator>(it), is_derived_from<pure_type_t<Iterator>, iterator_union>(),
                                            is_derived_from<pure_type_t<Iterator>, iterator>());
   }

   iterator_union& operator= (const iterator_union& it)
   {
      base_t::assign_impl(it, std::true_type());
      return *this;
   }

   iterator_union& operator= (iterator_union&& it)
   {
      base_t::assign_impl(std::move(it), std::true_type());
      return *this;
   }

   template <typename Iterator>
   std::enable_if_t<valid_assignment<pure_type_t<Iterator>>(), iterator_union&>
   operator= (Iterator&& it)
   {
      assign_impl(std::forward(it), is_derived_from_any<pure_type_t<Iterator>, iterator_union, iterator>());
      return *this;
   }

   reference operator* () const
   {
      return function<unions::star<reference>>::get(this->discriminant)(this->area);
   }
   pointer operator-> () const
   {
      return function<unions::arrow<pointer>>::get(this->discriminant)(this->area);
   }

   me& operator++ ()
   {
      function<unions::increment>::get(this->discriminant)(this->area);
      return static_cast<me&>(*this);
   }
   const me operator++(int) { me copy=static_cast<me&>(*this); operator++(); return copy; }

   bool operator== (const iterator_union& it) const
   {
      return this->discriminant==it.discriminant &&
             this->discriminant >= 0 &&
             function<unions::equality>::get(this->discriminant)(this->area, it.area);
   }
   bool operator!= (const iterator_union& it) const { return !operator==(it); }

   bool at_end() const
   {
      static_assert(check_iterator_feature<iterator_union, end_sensitive>::value, "iterator is not end-sensitive");
      return function<unions::at_end>::get(this->discriminant)(this->area);
   }

   Int index() const
   {
      static_assert(check_iterator_feature<iterator_union, indexed>::value, "iterator is not indexed");
      return function<unions::index>::get(this->discriminant)(this->area);
   }

   void rewind()
   {
      static_assert(check_iterator_feature<iterator_union, rewindable>::value, "iterator is not rewindable");
      function<unions::rewind>::get(this->discriminant)(this->area);
   }
};

template <typename IteratorList>
class iterator_union<IteratorList, bidirectional_iterator_tag>
   : public iterator_union<IteratorList, forward_iterator_tag> {
   using base_t = iterator_union<IteratorList, forward_iterator_tag> ;
public:
   using iterator_union<IteratorList, forward_iterator_tag>::iterator_union;
   using typename base_t::me;

   template <typename Iterator>
   std::enable_if_t<std::is_assignable<base_t&, Iterator&&>::value, iterator_union&>
   operator= (Iterator&& it)
   {
      base_t::operator=(std::forward<Iterator>(it));
      return *this;
   }

   me& operator-- ()
   {
      base_t::template function<unions::decrement>::get(this->discriminant)(this->area);
      return static_cast<me&>(*this);
   }
   me operator--(int) { me copy=static_cast<me&>(*this); operator--(); return copy; }
};

template <typename IteratorList>
class iterator_union<IteratorList, random_access_iterator_tag>
   : public iterator_union<IteratorList, bidirectional_iterator_tag> {
   using base_t = iterator_union<IteratorList, bidirectional_iterator_tag>;
public:
   using iterator_union<IteratorList, bidirectional_iterator_tag>::iterator_union;
   using typename base_t::reference;
   using typename base_t::difference_type;

   template <typename Iterator>
   std::enable_if_t<std::is_assignable<base_t&, Iterator&&>::value, iterator_union&>
   operator= (Iterator&& it)
   {
      base_t::operator=(std::forward<Iterator>(it));
      return *this;
   }

   iterator_union& operator+= (Int i)
   {
      base_t::template function<unions::advance_plus>::get(this->discriminant)(this->area);
      return *this;
   }
   iterator_union& operator-= (Int i)
   {
      base_t::template function<unions::advance_minus>::get(this->discriminant)(this->area);
      return *this;
   }
   iterator_union operator+ (Int i) const { iterator_union copy=*this; return copy+=i; }
   iterator_union operator- (Int i) const { iterator_union copy=*this; return copy-=i; }
   friend iterator_union operator+ (Int i, const iterator_union& it) { return it+i; }

   difference_type operator- (const iterator_union& it) const
   {
      return base_t::template function<unions::difference<difference_type>>::get(this->discriminant)(this->area);
   }

   reference operator[] (Int i) const
   {
      return base_t::template function<unions::random_it<reference>>::get(this->discriminant)(this->area,i);
   }
};

template <typename IteratorList, typename Feature, typename Category>
struct check_iterator_feature<iterator_union<IteratorList, Category>, Feature>
   : mlist_and<typename mlist_transform_binary<IteratorList, mrepeat<Feature>, check_iterator_feature>::type> {};

template <typename IteratorList, typename Category>
struct extract_union_list< iterator_union<IteratorList, Category> > {
   using type = IteratorList;
};

/* -----------------
    ContainerUnion
   ----------------- */
template <typename ContainerRef>
struct union_container_element_traits {
   struct type {
      using base_t = container_traits<ContainerRef>;
      using reference = typename base_t::reference;
      using const_reference = typename base_t::const_reference;
      using value_type = typename base_t::value_type;
      using category = typename base_t::category;
      static constexpr bool is_always_const = is_effectively_const<ContainerRef>::value;
   };
};

template <typename ContainerRef, typename Features,
          bool is_bidir=container_traits<ContainerRef>::is_bidirectional>
struct union_iterable_container_element_traits {
   struct type : union_container_element_traits<ContainerRef>::type {
      using base_t = ensure_features<std::remove_reference_t<ContainerRef>, Features>;
      using iterator_list = typename extract_union_list<typename base_t::iterator>::type;
      using const_iterator_list = typename extract_union_list<typename base_t::const_iterator>::type;
      static constexpr bool
         is_bidirectional = is_bidir,   // false
         is_resizeable = object_traits<pure_type_t<ContainerRef>>::is_resizeable==1;
   };
};

template <typename ContainerRef, typename Features>
struct union_iterable_container_element_traits<ContainerRef, Features, true> {
   struct type : union_iterable_container_element_traits<ContainerRef, Features, false>::type {
      using base_t = ensure_features<std::remove_reference_t<ContainerRef>, Features>;
      using reverse_iterator_list = typename extract_union_list<typename base_t::reverse_iterator>::type;
      using const_reverse_iterator_list = typename extract_union_list<typename base_t::const_reverse_iterator>::type;
      static constexpr bool is_bidirectional = true;
   };
};

template <typename Traits1, typename Traits2>
struct combine_union_container_traits {
   struct type {
      using reference = typename union_reference<typename Traits1::reference, typename Traits2::reference>::type;
      using const_reference = typename union_reference<typename Traits1::const_reference, typename Traits2::const_reference>::type;
      using category = typename least_derived_class<typename Traits1::category, typename Traits2::category>::type;
      using value_type = pure_type_t<reference>;
      static constexpr bool is_always_const = Traits1::is_always_const || Traits2::is_always_const;
   };
};

template <typename Traits1, typename Traits2,
          bool is_bidir=Traits1::is_bidirectional && Traits2::is_bidirectional>
struct combine_union_iterable_container_traits {
   struct type : combine_union_container_traits<Traits1, Traits2>::type {
      using iterator_list = typename mlist_union<typename Traits1::iterator_list, typename Traits2::iterator_list>::type;
      using const_iterator_list = typename mlist_union<typename Traits1::const_iterator_list, typename Traits2::const_iterator_list>::type;
      static constexpr bool
         is_bidirectional = is_bidir, // false
         is_resizeable = Traits1::is_resizeable && Traits2::is_resizeable;
   };
};

template <typename Traits1, typename Traits2>
struct combine_union_iterable_container_traits<Traits1, Traits2, true> {
   struct type : combine_union_iterable_container_traits<Traits1, Traits2, false>::type {
      using reverse_iterator_list = typename mlist_union<typename Traits1::reverse_iterator_list, typename Traits2::reverse_iterator_list>::type;
      using const_reverse_iterator_list = typename mlist_union<typename Traits1::const_reverse_iterator_list, typename Traits2::const_reverse_iterator_list>::type;
      static constexpr bool is_bidirectional = true;
   };
};

template <typename ContainerList>
struct prepare_union_container_traits {
   template <typename ContainerRef>
   using element_traits = union_container_element_traits<ContainerRef>;
   template <typename Traits1, typename Traits2>
   using combine_traits = combine_union_container_traits<Traits1, Traits2>;

   using type = typename mlist_fold_transform<typename mlist_reverse<ContainerList>::type, element_traits, combine_traits>::type;
};

template <typename ContainerList, typename Features>
struct prepare_union_iterable_container_traits {
   template <typename ContainerRef>
   using element_traits = union_iterable_container_element_traits<ContainerRef, Features>;
   template <typename Traits1, typename Traits2>
   using combine_traits = combine_union_iterable_container_traits<Traits1, Traits2>;

   using type = typename mlist_fold_transform<typename mlist_reverse<ContainerList>::type, element_traits, combine_traits>::type;
   static constexpr bool
      need_iterator_union = mlist_length<typename type::iterator_list>::value > 1,
      is_bidirectional = type::is_bidirectional;
};

template <typename ContainerList, typename Features,
          bool need_union = prepare_union_iterable_container_traits<ContainerList, Features>::need_iterator_union,
          bool is_bidir = prepare_union_iterable_container_traits<ContainerList, Features>::is_bidirectional>
struct union_container_traits : prepare_union_iterable_container_traits<ContainerList, Features>::type {
   using base_t = typename prepare_union_iterable_container_traits<ContainerList, Features>::type;
   using iterator = typename mlist_unwrap<typename base_t::iterator_list>::type;
   using const_iterator = typename mlist_unwrap<typename base_t::const_iterator_list>::type;
};

template <typename ContainerList, typename Features>
struct union_container_traits<ContainerList, Features, false, true>
   : union_container_traits<ContainerList, Features, false, false> {
   using base_t = union_container_traits<ContainerList, Features, false, false>;
   using reverse_iterator = typename mlist_unwrap<typename base_t::reverse_iterator_list>::type;
   using const_reverse_iterator = typename mlist_unwrap<typename base_t::const_reverse_iterator_list>::type;
};

template <typename ContainerList, typename Features>
struct union_container_traits<ContainerList, Features, true, false>
   : union_container_traits<ContainerList, Features, false, false> {
   using base_t = union_container_traits<ContainerList, Features, false, false>;
   using iterator = iterator_union<typename base_t::iterator_list>;
   using const_iterator = iterator_union<typename base_t::const_iterator_list>;
};

template <typename ContainerList, typename Features>
struct union_container_traits<ContainerList, Features, true, true>
   : union_container_traits<ContainerList, Features, true, false> {
   using base_t = union_container_traits<ContainerList, Features, false, false>;
   using reverse_iterator = iterator_union<typename base_t::reverse_iterator_list>;
   using const_reverse_iterator = iterator_union<typename base_t::const_reverse_iterator_list>;
};

namespace unions {

struct size : index {
   template <typename Container>
   static Int execute(const char *c)
   {
      return basics<Container>::get(c).size();
   }
};

struct dim : index {
   template <typename Container>
   static Int execute(const char *c)
   {
      return get_dim(basics<Container>::get(c));
   };
};

struct empty : at_end {
   template <typename Container>
   static bool execute(const char *c)
   {
      return basics<Container>::get(c).empty();
   }
};

struct resize {
   static void null(char *c, Int n) { invalid_null_op(); }

   template <typename Container>
   static void execute(char *c, Int n)
   {
      basics<Container>::get(c).resize(n);
   }
};

template <typename Iterator, typename Features>
struct begin {
   static Iterator null(char* c) { invalid_null_op(); }

   template <typename Container>
   static Iterator execute(char* c)
   {
      return ensure(basics<Container>::get(c), Features()).begin();
   }
};

template <typename Iterator, typename Features>
struct end : begin<Iterator, Features> {
   template <typename Container>
   static Iterator execute(char* c)
   {
      return ensure(basics<Container>::get(c), Features()).end();
   }
};

template <typename Iterator, typename Features>
struct cbegin {
   static Iterator null(const char* c) { invalid_null_op(); }

   template <typename Container>
   static Iterator execute(const char* c)
   {
      return ensure(basics<Container>::get(c), Features()).begin();
   }
};

template <typename Iterator, typename Features>
struct cend : cbegin<Iterator, Features> {
   template <typename Container>
   static Iterator execute(const char* c)
   {
      return ensure(basics<Container>::get(c), Features()).end();
   }
};

template <typename Iterator, typename Features>
struct rbegin : begin<Iterator, Features> {
   template <typename Container>
   static Iterator execute(char* c)
   {
      return ensure(basics<Container>::get(c), Features()).rbegin();
   }
};

template <typename Iterator, typename Features>
struct rend : begin<Iterator, Features> {
   template <typename Container>
   static Iterator execute(char* c)
   {
      return ensure(basics<Container>::get(c), Features()).rend();
   }
};

template <typename Iterator, typename Features>
struct crbegin : cbegin<Iterator, Features> {
   template <typename Container>
   static Iterator execute(const char* c)
   {
      return ensure(basics<Container>::get(c), Features()).rbegin();
   }
};

template <typename Iterator, typename Features>
struct crend : cbegin<Iterator, Features> {
   template <typename Container>
   static Iterator execute(const char* c)
   {
      return ensure(basics<Container>::get(c), Features()).rend();
   }
};

template <typename Ref>
struct front {
   static Ref null(char* c) { invalid_null_op(); }

   template <typename Container>
   static Ref execute(char* c)
   {
      return basics<Container>::get(c).front();
   }
};

template <typename Ref>
struct cfront {
   static Ref null(const char* c) { invalid_null_op(); }

   template <typename Container>
   static Ref execute(const char* c)
   {
      return basics<Container>::get(c).front();
   }
};

template <typename Ref>
struct back : front<Ref> {
   template <typename Container>
   static Ref execute(char* c)
   {
      return basics<Container>::get(c).back();
   }
};

template <typename Ref>
struct cback : cfront<Ref> {
   template <typename Container>
   static Ref execute(const char* c)
   {
      return basics<Container>::get(c).back();
   }
};

template <typename Ref>
struct random {
   static Ref null(char* c, Int i) { invalid_null_op(); }

   template <typename Container>
   static Ref execute(char* c, Int i)
   {
      return basics<Container>::get(c)[i];
   }
};

template <typename Ref>
struct crandom {
   static Ref null(const char* c, Int i) { invalid_null_op(); }

   template <typename Container>
   static Ref execute(const char* c, Int i)
   {
      return basics<Container>::get(c)[i];
   }
};

} // end namespace unions

template <typename ContainerList, typename ProvidedFeatures,
          bool enable=union_container_traits<ContainerList, ProvidedFeatures>::is_resizeable>
class container_union_resize {};

template <typename ContainerList, typename ProvidedFeatures,
          typename Category=typename union_container_traits<ContainerList, ProvidedFeatures>::category>
class container_union_elem_access {
protected:
   static const bool provide_sparse =
      mlist_or<typename mlist_transform_binary<ContainerList, mrepeat<sparse>, check_container_ref_feature>::type>::value &&
      !mlist_and<typename mlist_transform_binary<ContainerList, mrepeat<sparse>, check_container_ref_feature>::type>::value &&
      !mlist_contains<ProvidedFeatures, dense>::value;
   using needed_features = std::conditional_t<provide_sparse,
                                              typename mix_features<ProvidedFeatures, sparse_compatible>::type,
                                              ProvidedFeatures>;
   using traits = union_container_traits<ContainerList, needed_features>;

   template <typename Operation>
   using function = unions::Function<ContainerList, Operation>;

   template <template <typename, typename> class Operation, typename Iterator>
   using it_function = function<Operation<Iterator, needed_features>>;

public:
   using reference = typename traits::reference;
   using const_reference = typename traits::const_reference;
   using value_type = typename traits::value_type;
   using container_category = typename traits::category;

   friend class container_union_resize<ContainerList, ProvidedFeatures>;
};

template <typename ContainerList, typename ProvidedFeatures=mlist<>>
class ContainerUnion
   : public type_union<ContainerList>
   , public container_union_elem_access<ContainerList, ProvidedFeatures>
   , public container_union_resize<ContainerList, ProvidedFeatures>
   , public inherit_generic<ContainerUnion<ContainerList, ProvidedFeatures>,
                            typename mlist_transform_unary<ContainerList, deref>::type>::type {
protected:
   using base_t = type_union<ContainerList>;
   using access_t = container_union_elem_access<ContainerList, ProvidedFeatures>;
   using typename access_t::traits;

   template <typename, typename, typename> friend class container_union_elem_access;
   friend class container_union_resize<ContainerList, ProvidedFeatures>;

   template <typename Operation>
   using function = typename access_t::template function<Operation>;

   template <template <typename, typename> class Operation, typename Iterator>
   using it_function = typename access_t::template it_function<Operation, Iterator>;

public:
   using iterator = typename traits::iterator;
   using const_iterator = typename traits::const_iterator;

   ContainerUnion() {}

   ContainerUnion(const ContainerUnion&) = default;
   ContainerUnion(ContainerUnion&&) = default;

   template <typename Source, typename=std::enable_if_t<std::is_constructible<base_t, Source>::value>>
   ContainerUnion(Source&& src)
      : base_t(std::forward<Source>(src)) {}

   ContainerUnion& operator= (const ContainerUnion&) = default;
   ContainerUnion& operator= (ContainerUnion&&) = default;

   template <typename Source>
   std::enable_if_t<std::is_assignable<base_t&, Source&&>::value, ContainerUnion&>
   operator= (Source&& src)
   {
      base_t::operator=(std::forward<Source>(src));
      return *this;
   }

   iterator begin()
   {
      return it_function<unions::begin, iterator>::get(this->discriminant)(this->area);
   }
   iterator end()
   {
      return it_function<unions::end, iterator>::get(this->discriminant)(this->area);
   }
   const_iterator begin() const
   {
      return it_function<unions::cbegin, const_iterator>::get(this->discriminant)(this->area);
   }
   const_iterator end() const
   {
      return it_function<unions::cend, const_iterator>::get(this->discriminant)(this->area);
   }
   Int size() const
   {
      return function<unions::size>::get(this->discriminant)(this->area);
   }
   bool empty() const
   {
      return function<unions::empty>::get(this->discriminant)(this->area);
   }
   Int dim() const
   {
      return function<unions::dim>::get(this->discriminant)(this->area);
   }
};

template <typename ContainerList, typename ProvidedFeatures>
class container_union_elem_access<ContainerList, ProvidedFeatures, forward_iterator_tag>
   : public container_union_elem_access<ContainerList, ProvidedFeatures, input_iterator_tag> {
   using base_t = container_union_elem_access<ContainerList, ProvidedFeatures, input_iterator_tag>;
protected:
   using master = ContainerUnion<ContainerList, ProvidedFeatures>;

   template <typename Operation>
   using function = typename base_t::template function<Operation>;

public:
   using typename base_t::reference;
   using typename base_t::const_reference;

   reference front()
   {
      master& me=static_cast<master&>(*this);
      return function<unions::front<reference>>::get(me.discriminant)(me.area);
   }
   const_reference front() const
   {
      const master& me=static_cast<const master&>(*this);
      return function<unions::cfront<const_reference>>::get(me.discriminant)(me.area);
   }
};

template <class ContainerList, class ProvidedFeatures>
class container_union_elem_access<ContainerList, ProvidedFeatures, bidirectional_iterator_tag>
   : public container_union_elem_access<ContainerList, ProvidedFeatures, forward_iterator_tag> {
   using base_t = container_union_elem_access<ContainerList, ProvidedFeatures, forward_iterator_tag>;
protected:
   template <typename Operation>
   using function = typename base_t::template function<Operation>;

   template <template <typename, typename> class Operation, typename Iterator>
   using it_function = typename base_t::template it_function<Operation, Iterator>;

public:
   using reverse_iterator = typename base_t::traits::reverse_iterator;
   using const_reverse_iterator = typename base_t::traits::const_reverse_iterator;
   using typename base_t::reference;
   using typename base_t::const_reference;

   reverse_iterator rbegin()
   {
      auto& me=static_cast<typename base_t::master&>(*this);
      return it_function<unions::rbegin, reverse_iterator>::get(me.discriminant)(me.area);
   }
   reverse_iterator rend()
   {
      auto& me=static_cast<typename base_t::master&>(*this);
      return it_function<unions::rend, reverse_iterator>::get(me.discriminant)(me.area);
   }
   const_reverse_iterator rbegin() const
   {
      auto& me=static_cast<const typename base_t::master&>(*this);
      return it_function<unions::crbegin, const_reverse_iterator>::get(me.discriminant)(me.area);
   }
   const_reverse_iterator rend() const
   {
      auto& me=static_cast<const typename base_t::master&>(*this);
      return it_function<unions::crend, const_reverse_iterator>::get(me.discriminant)(me.area);
   }

   reference back()
   {
      auto& me=static_cast<typename base_t::master&>(*this);
      return function<unions::back<reference>>::get(me.discriminant)(me.area);
   }
   const_reference back() const
   {
      auto& me=static_cast<const typename base_t::master&>(*this);
      return function<unions::cback<reference>>::get(me.discriminant)(me.area);
   }
};

template <typename ContainerList, typename ProvidedFeatures>
class container_union_elem_access<ContainerList, ProvidedFeatures, random_access_iterator_tag>
   : public container_union_elem_access<ContainerList, ProvidedFeatures, bidirectional_iterator_tag> {
   using base_t = container_union_elem_access<ContainerList, ProvidedFeatures, bidirectional_iterator_tag>;
protected:
   template <typename Operation>
   using function = typename base_t::template function<Operation>;
public:
   using typename base_t::reference;
   using typename base_t::const_reference;

   reference operator[] (Int i)
   {
      auto& me = static_cast<typename base_t::master&>(*this);
      return function<unions::random<reference>>::get(me.discriminant)(me.area, i);
   }
   const_reference operator[] (Int i) const
   {
      auto& me = static_cast<const typename base_t::master&>(*this);
      return function<unions::crandom<const_reference>>::get(me.discriminant)(me.area, i);
   }
};

template <typename ContainerList, typename ProvidedFeatures>
class container_union_resize<ContainerList, ProvidedFeatures, true> {
protected:
   using master = ContainerUnion<ContainerList,ProvidedFeatures>;
   using base_t = container_union_elem_access<ContainerList, ProvidedFeatures>;

   template <typename Operation>
   using function = typename base_t::template function<Operation>;
public:
   void resize(Int n)
   {
      master& me = static_cast<master&>(*this);
      function<unions::resize>::get(me.discriminant)(me.area, n);
   }
};

template <typename ContainerList, typename ProvidedFeatures, typename Features>
struct enforce_features<ContainerUnion<ContainerList, ProvidedFeatures>, Features> {
   using container = ContainerUnion<ContainerList, typename mix_features<ProvidedFeatures, Features>::type>;
};

template <typename ContainerList, typename ProvidedFeatures>
struct spec_object_traits< ContainerUnion<ContainerList, ProvidedFeatures> >
   : spec_object_traits<is_container> {
   static constexpr int is_resizeable    = union_container_traits<ContainerList, ProvidedFeatures>::is_resizeable;
   static constexpr bool is_always_const = union_container_traits<ContainerList, ProvidedFeatures>::is_always_const,
                         is_persistent=false;
};

template <typename ContainerList, typename ProvidedFeatures, typename Feature>
struct check_container_feature<ContainerUnion<ContainerList, ProvidedFeatures>, Feature>
   : mlist_or< mlist_and<typename mlist_transform_binary<ContainerList, mrepeat<Feature>, check_container_ref_feature>::type>,
               mlist_contains<ProvidedFeatures, Feature, absorbing_feature> > {};

template <typename ContainerList, typename ProvidedFeatures>
struct check_container_feature<ContainerUnion<ContainerList, ProvidedFeatures>, sparse>
   : mlist_and< mlist_or<typename mlist_transform_binary<ContainerList, mrepeat<sparse>, check_container_ref_feature>::type>,
                bool_not<mlist_contains<ProvidedFeatures, dense>> > {};

template <typename ContainerList, typename ProvidedFeatures>
struct check_container_feature<ContainerUnion<ContainerList, ProvidedFeatures>, sparse_compatible>
   : mlist_or< check_container_feature<ContainerUnion<ContainerList, ProvidedFeatures>, sparse>,
               mlist_and<typename mlist_transform_binary<ContainerList, mrepeat<sparse_compatible>, check_container_ref_feature>::type>,
               mlist_contains<ProvidedFeatures, sparse_compatible, absorbing_feature> > {};

template <typename ContainerList, typename ProvidedFeatures>
struct extract_union_list< ContainerUnion<ContainerList, ProvidedFeatures> > {
   using type = ContainerList;
};

template <typename T1, typename T2>
struct union_reference_helper<T1, T2, is_container, is_container> {
   using type = ContainerUnion< typename mlist_union<typename extract_union_list<T1>::type,
                                                     typename extract_union_list<T2>::type>::type >;
};

} // end namespace pm

namespace polymake {

using pm::ContainerUnion;

}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
