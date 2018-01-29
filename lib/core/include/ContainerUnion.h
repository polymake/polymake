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

#ifndef POLYMAKE_CONTAINER_UNION_H
#define POLYMAKE_CONTAINER_UNION_H

#include "polymake/internal/type_union.h"
#include "polymake/internal/iterators.h"

namespace pm {

/* -----------------
    iterator_union
   ----------------- */

template <typename T1, typename T2,
          bool _viable = attrib<typename compatible<T1,T2>::type>::is_reference ||
                         identical_minus_const_ref<T1,T2>::value>
struct union_reference : compatible<T1,T2> {};

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
          typename Model1=typename object_traits<typename deref<T1>::type>::model,
          typename Model2=typename object_traits<typename deref<T2>::type>::model>
struct union_reference_helper {
   using type = type_union< typename merge_list<typename extract_union_list<T1>::type,
                                                typename extract_union_list<T2>::type, std::is_same>::type >;
};

template <typename T1, typename T2>
struct union_reference<T1, T2, false> : union_reference_helper<T1, T2> {};

template <typename Iterator>
struct union_iterator_traits : iterator_traits<Iterator> {
   using iterator_list = typename iterator_traits<Iterator>::iterator;
   using const_iterator_list = typename iterator_traits<Iterator>::const_iterator;
};

template <typename Head, typename Tail>
struct union_iterator_traits< cons<Head,Tail> > {
   using traits1 = union_iterator_traits<Head>;
   using traits2 = union_iterator_traits<Tail>;

   using iterator_category = typename least_derived_class<typename traits1::iterator_category, typename traits2::iterator_category>::type;
   using reference = typename union_reference<typename traits1::reference, typename traits2::reference>::type;
   using value_type = typename deref<reference>::type;
   using pointer = value_type*;
   using difference_type = typename std::common_type<typename traits1::difference_type, typename traits2::difference_type>::type;

   using iterator_list = typename merge_list<typename traits1::iterator_list, typename traits2::iterator_list, std::is_same>::type;
   using const_iterator_list = typename merge_list<typename traits1::const_iterator_list, typename traits2::const_iterator_list, std::is_same>::type;
};

namespace virtuals {

template <typename Iterator>
struct iterator_basics : basics<Iterator> {
   static const typename iterator_traits<Iterator>::iterator& get_alt(const char* src)
   {
      return *reinterpret_cast<const typename iterator_traits<Iterator>::iterator*>(src);
   }
};
template <typename Iterator>
struct alt_copy_constructor {
   static void _do(char* dst, const char* src)
   {
      basics<Iterator>::construct(dst, iterator_basics<Iterator>::get_alt(src));
   }
};
template <typename Iterator>
struct assignment {
   static void _do(char* dst, const char* src)
   {
      basics<Iterator>::get(dst)=basics<Iterator>::get(src);
   }
};
template <typename Iterator>
struct alt_assignment {
   static void _do(char* dst, const char* src)
   {
      basics<Iterator>::get(dst)=iterator_basics<Iterator>::get_alt(src);
   }
};
template <typename Iterator>
struct increment {
   static void _do(char* it)
   {
      ++basics<Iterator>::get(it);
   }
};
template <typename Iterator>
struct decrement {
   static void _do(char* it)
   {
      --basics<Iterator>::get(it);
   }
};
template <typename Iterator>
struct advance_plus {
   static void _do(char* it, int i)
   {
      basics<Iterator>::get(it)+=i;
   }
};
template <typename Iterator>
struct advance_minus {
   static void _do(char* it, int i)
   {
      basics<Iterator>::get(it)-=i;
   }
};
template <typename Iterator>
struct equality {
   static bool _do(const char* it1, const char* it2)
   {
      return basics<Iterator>::get(it1) == basics<Iterator>::get(it2);
   }
};
template <typename Iterator>
struct difference {
   static typename iterator_traits<Iterator>::difference_type
   _do(const char* it1, const char* it2)
   {
      return basics<Iterator>::get(it1) - basics<Iterator>::get(it2);
   }
};
template <typename Iterator>
struct index {
   static int _do(const char* it)
   {
      return basics<Iterator>::get(it).index();
   }
};
template <typename Iterator>
struct at_end {
   static bool _do(const char* it)
   {
      return basics<Iterator>::get(it).at_end();
   }
};
template <typename Iterator>
struct rewind {
   static void _do(char* it)
   {
      basics<Iterator>::get(it).rewind();
   }
};

template <typename IteratorList>
struct iterator_union_functions : type_union_functions<IteratorList> {
   using base_t = type_union_functions<IteratorList>;
   using traits = union_iterator_traits<IteratorList>;

   template <int discr>
   struct basics : virtuals::iterator_basics<typename n_th<IteratorList,discr>::type> {};

   struct alt_copy_constructor : base_t::length_def {
      template <int discr> struct defs : virtuals::alt_copy_constructor<typename n_th<IteratorList,discr>::type> {};
      using fpointer = void (*)(char*, const char*);
      static fpointer no_op() { return &empty_union_def::trivial_op2; }
   };
   struct assignment : base_t::length_def {
      template <int discr> struct defs : virtuals::assignment<typename n_th<IteratorList,discr>::type> {};
      using fpointer = void (*)(char*, const char*);
      static fpointer no_op() { return &empty_union_def::trivial_op2; }
   };
   struct alt_assignment : base_t::length_def {
      template <int discr> struct defs : virtuals::alt_assignment<typename n_th<IteratorList,discr>::type> {};
      using fpointer = void (*)(char*, const char*);
      static fpointer no_op() { return &empty_union_def::trivial_op2; }
   };
   struct dereference : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::reference _do(const char* it)
         {
            return *basics<discr>::get(it);
         }
      };
      using fpointer = typename traits::reference (*)(const char*);
   };
   struct increment : base_t::length_def {
      template <int discr> struct defs : virtuals::increment<typename n_th<IteratorList,discr>::type> {};
      using fpointer = void (*)(char*);
   };
   struct decrement : base_t::length_def {
      template <int discr> struct defs : virtuals::decrement<typename n_th<IteratorList,discr>::type> {};
      using fpointer = void (*)(char*);
   };
   struct advance_plus : base_t::length_def {
      template <int discr> struct defs : virtuals::advance_plus<typename n_th<IteratorList,discr>::type> {};
      using fpointer = void (*)(char*, int);
   };
   struct advance_minus : base_t::length_def {
      template <int discr> struct defs : virtuals::advance_minus<typename n_th<IteratorList,discr>::type> {};
      using fpointer = void (*)(char*, int);
   };
   struct equality : base_t::length_def {
      template <int discr> struct defs : virtuals::equality<typename n_th<IteratorList,discr>::type> {};
      using fpointer = bool (*)(const char*, const char*);
   };
   struct difference : base_t::length_def {
      template <int discr> struct defs : virtuals::difference<typename n_th<IteratorList,discr>::type> {};
      using fpointer = typename traits::difference_type (*)(const char*, const char*);
   };
   struct random : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::reference _do(const char* it, int i)
         {
            return basics<discr>::get(it)[i];
         }
      };
      using fpointer = typename traits::reference (*)(const char*, int);
   };
   struct index : base_t::length_def {
      template <int discr> struct defs : virtuals::index<typename n_th<IteratorList,discr>::type> {};
      using fpointer = int (*)(const char*);
   };
   struct at_end : base_t::length_def {
      template <int discr> struct defs : virtuals::at_end<typename n_th<IteratorList,discr>::type> {};
      using fpointer = bool (*)(const char*);
   };
   struct rewind : base_t::length_def {
      template <int discr> struct defs : virtuals::rewind<typename n_th<IteratorList,discr>::type> {};
      using fpointer = void (*)(char*);
   };
};
} // end namespace virtuals

template <typename IteratorList, typename Category=typename union_iterator_traits<IteratorList>::iterator_category>
class iterator_union : public type_union<IteratorList> {
protected:
   using base_t = type_union<IteratorList>;
   using traits = union_iterator_traits<IteratorList>;
   using Functions = virtuals::iterator_union_functions<IteratorList>;
   template <int discr> struct basics : virtuals::basics<typename n_th<IteratorList,discr>::type> {};

   template <typename Iterator, int own_discr, int alt_discr>
   void init_from_value(const Iterator& it, cons< int_constant<own_discr>, int_constant<alt_discr> >)
   {
      const int discr=const_first_nonnegative<own_discr,alt_discr>::value;
      this->discriminant=discr;
      basics<discr>::construct(this->area,it);
   }

   template <typename OtherList>
   void init_from_value(const iterator_union<OtherList>& it, cons< int_constant<-1>, int_constant<-1> >)
   {
      init_from_union(it, bool_constant< list_mapping<OtherList, IteratorList>::mismatch >(),
                          bool_constant< list_mapping<OtherList, typename traits::iterator_list>::mismatch >() );
   }

   template <typename OtherList, typename discr2>
   void init_from_union(const iterator_union<OtherList>& it, std::false_type, discr2)
   {
      base_t::init_from_union(it, std::false_type());
   }

   template <typename OtherList>
   void init_from_union(const iterator_union<OtherList>& it, std::true_type, std::false_type)
   {
      this->discriminant=virtuals::mapping< typename list_mapping<OtherList, typename traits::iterator_list>::type >::table[it.discriminant];
      virtuals::table<typename Functions::alt_copy_constructor>::call(this->discriminant)(this->area,it.area);
   }

   template <typename Iterator>
   void init_impl(const Iterator& it, std::false_type, std::false_type)
   {
      init_from_value(it, cons< int_constant<list_search<IteratorList, Iterator, std::is_same>::pos>,
                                int_constant<list_search<typename traits::iterator_list, Iterator, std::is_same>::pos> >());
   }

   template <typename Iterator, typename discr2>
   void init_impl(const Iterator& it, std::true_type, discr2)
   {
      base_t::init_impl(it, std::true_type());
   }

   template <typename Iterator>
   void init_impl(const Iterator& it, std::false_type, std::true_type)
   {
      this->discriminant=it.discriminant;
      virtuals::table<typename Functions::alt_copy_constructor>::call(this->discriminant)(this->area,it.area);
   }

   template <typename Iterator, int own_discr, int alt_discr>
   void assign_value(const Iterator& it, cons< int_constant<own_discr>, int_constant<alt_discr> >)
   {
      const int discr=const_first_nonnegative<own_discr,alt_discr>::value;
      if (this->discriminant==discr) {
         virtuals::table<typename Functions::assignment>::call(this->discriminant)(this->area,it);
      } else {
         virtuals::table<typename Functions::destructor>::call(this->discriminant)(this->area);
         this->discriminant=discr;
         basics<discr>::construct(this->area,it);
      }
   }

   template <typename OtherList>
   void assign_value(const iterator_union<OtherList>& it, cons< int_constant<-1>, int_constant<-1> >)
   {
      assign_union(it, bool_constant< list_mapping<OtherList, IteratorList>::mismatch >(),
                       bool_constant< list_mapping<OtherList, typename traits::iterator_list>::mismatch >() );
   }

   template <typename OtherList, typename discr2>
   void assign_union(const iterator_union<OtherList>& it, std::false_type, discr2)
   {
      base_t::assign_union(it, std::false_type());
   }

   template <typename OtherList>
   void assign_union(const iterator_union<OtherList>& it, std::true_type, std::false_type)
   {
      const int discr=virtuals::mapping< typename list_mapping<OtherList, typename traits::iterator_list>::type >::table[it.discriminant];
      if (this->discriminant==discr) {
         virtuals::table<typename Functions::alt_assignment>::call(this->discriminant)(this->area,it.area);
      } else {
         virtuals::table<typename Functions::destructor>::call(this->discriminant)(this->area);
         this->discriminant=discr;
         virtuals::table<typename Functions::alt_copy_constructor>::call(this->discriminant)(this->area,it.area);
      }
   }

   template <typename Iterator>
   void assign_impl(const Iterator& it, std::false_type, std::false_type)
   {
      assign_value(it, cons< int_constant<list_search<IteratorList, Iterator, std::is_same>::pos>,
                             int_constant<list_search<typename traits::iterator_list, Iterator, std::is_same>::pos> >());
   }

   template <typename Iterator, typename discr2>
   void assign_impl(const Iterator& src, std::true_type, discr2)
   {
      base_t::assign_impl(src, std::true_type());
   }

   template <typename Iterator>
   void assign_impl(const Iterator& it, std::false_type, std::true_type)
   {
      if (this->discriminant==it.discriminant) {
         virtuals::table<typename Functions::alt_assignment>::call(this->discriminant)(this->area,it.area);
      } else {
         virtuals::table<typename Functions::destructor>::call(this->discriminant)(this->area);
         init_impl(it, std::false_type(), std::true_type());
      }
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

   template <typename Iterator>
   iterator_union(const Iterator& it)
   {
      init_impl(it, is_derived_from<Iterator, iterator_union>(),
                    is_derived_from<Iterator, iterator>());
   }

   iterator_union& operator= (const iterator_union& it)
   {
      base_t::assign_impl(it, std::true_type());
      return *this;
   }

   template <typename Iterator>
   iterator_union& operator= (const Iterator& it)
   {
      assign_impl(it, is_derived_from<Iterator, iterator_union>(),
                      is_derived_from<Iterator, iterator>());
      return *this;
   }

   reference operator* () const
   {
      return virtuals::table<typename Functions::dereference>::call(this->discriminant)(this->area);
   }
   pointer operator-> () const { return &(operator*()); }

   me& operator++ ()
   {
      virtuals::table<typename Functions::increment>::call(this->discriminant)(this->area);
      return static_cast<me&>(*this);
   }
   const me operator++(int) { me copy=static_cast<me&>(*this); operator++(); return copy; }

   bool operator== (const iterator_union& it) const
   {
      return this->discriminant==it.discriminant &&
             virtuals::table<typename Functions::equality>::call(this->discriminant)(this->area,it.area);
   }
   bool operator!= (const iterator_union& it) const { return !operator==(it); }

   bool at_end() const
   {
      static_assert(check_iterator_feature<iterator_union, end_sensitive>::value, "iterator is not end-sensitive");
      return virtuals::table<typename Functions::at_end>::call(this->discriminant)(this->area);
   }

   int index() const
   {
      static_assert(check_iterator_feature<iterator_union, indexed>::value, "iterator is not indexed");
      return virtuals::table<typename Functions::index>::call(this->discriminant)(this->area);
   }

   void rewind()
   {
      static_assert(check_iterator_feature<iterator_union, rewindable>::value, "iterator is not rewindable");
      virtuals::table<typename Functions::rewind>::call(this->discriminant)(this->area);
   }
};

template <typename IteratorList>
class iterator_union<IteratorList, bidirectional_iterator_tag>
   : public iterator_union<IteratorList, forward_iterator_tag> {
   using base_t = iterator_union<IteratorList, forward_iterator_tag> ;
public:
   using me = iterator_union<IteratorList> ;

   iterator_union() {}

   template <typename Iterator>
   iterator_union(const Iterator& it) : base_t(it) {}

   template <typename Iterator>
   iterator_union& operator= (const Iterator& it)
   {
      base_t::operator=(it);
      return *this;
   }

   me& operator-- ()
   {
      virtuals::table<typename base_t::Functions::decrement>::call(this->discriminant)(this->area);
      return static_cast<me&>(*this);
   }
   me operator--(int) { me copy=static_cast<me&>(*this); operator--(); return copy; }
};

template <typename IteratorList>
class iterator_union<IteratorList, random_access_iterator_tag>
   : public iterator_union<IteratorList, bidirectional_iterator_tag> {
   using base_t = iterator_union<IteratorList, bidirectional_iterator_tag>;
public:
   iterator_union() {}

   template <typename Iterator>
   iterator_union(const Iterator& it) : base_t(it) {}

   template <typename Iterator>
   iterator_union& operator= (const Iterator& it)
   {
      base_t::operator=(it);
      return *this;
   }

   iterator_union& operator+= (int i)
   {
      virtuals::table<typename base_t::Functions::advance_plus>::call(this->discriminant)(this->area);
      return *this;
   }
   iterator_union& operator-= (int i)
   {
      virtuals::table<typename base_t::Functions::advance_minus>::call(this->discriminant)(this->area);
      return *this;
   }
   iterator_union operator+ (int i) const { iterator_union copy=*this; return copy+=i; }
   iterator_union operator- (int i) const { iterator_union copy=*this; return copy-=i; }
   friend iterator_union operator+ (int i, const iterator_union& it) { return it+i; }

   typename base_t::difference_type operator- (const iterator_union& it) const
   {
      return virtuals::table<typename base_t::Functions::difference>::call(this->discriminant)(this->area);
   }

   typename base_t::reference operator[] (int i) const
   {
      return virtuals::table<typename base_t::Functions::random>::call(this->discriminant)(this->area,i);
   }
};

template <typename IteratorList, typename Feature, typename Category>
struct check_iterator_feature<iterator_union<IteratorList,Category>, Feature>
   : list_accumulate_binary<list_and, check_iterator_feature, IteratorList, same<Feature> > {};

template <typename IteratorList, typename Category>
struct extract_union_list< iterator_union<IteratorList,Category> > {
   using type = IteratorList;
};

/* -----------------
    ContainerUnion
   ----------------- */

template <typename ContainerRef, typename Features,
          bool _reversible=container_traits<ContainerRef>::is_bidirectional>
struct union_container_traits_helper : ensure_features<typename deref<ContainerRef>::minus_ref, Features> {
   using base_t = ensure_features<typename deref<ContainerRef>::minus_ref, Features>;
   using iterator_list = typename extract_union_list<typename base_t::iterator>::type;
   using const_iterator_list = typename extract_union_list<typename base_t::const_iterator>::type ;
   static const bool
      is_reversible=_reversible,   // = false
      is_resizeable=object_traits<typename deref<ContainerRef>::type>::is_resizeable==1,
      is_always_const=effectively_const<ContainerRef>::value;
};

template <typename ContainerRef, typename Features>
struct union_container_traits_helper<ContainerRef, Features, true>
   : union_container_traits_helper<ContainerRef, Features, false> {
   using base_t = union_container_traits_helper<ContainerRef, Features, false>;
   using reverse_iterator_list = typename extract_union_list<typename base_t::reverse_iterator>::type;
   using const_reverse_iterator_list = typename extract_union_list<typename base_t::const_reverse_iterator>::type;
   static const bool is_reversible=true;
};

template <typename ContainerRef, typename Features=void>
struct union_container_traits : union_container_traits_helper<ContainerRef, Features> {};

template <typename C1, typename C2, typename Features,
          bool need_union=!std::is_same<typename union_container_traits<C1, Features>::iterator,
                                        typename union_container_traits<C2, Features>::iterator>::value,
          bool reversible=union_container_traits<C1, Features>::is_reversible &&
                          union_container_traits<C2, Features>::is_reversible>
struct union_container_traits_helper2
   : union_container_traits<C1, Features> {};

template <typename C1, typename C2, typename Features>
struct union_container_traits_helper2<C1, C2, Features, true, false> {
   using iterator_list = typename merge_list<typename union_container_traits<C1, Features>::iterator_list,
                                             typename union_container_traits<C2, Features>::iterator_list, std::is_same>::type;
   using const_iterator_list = typename merge_list<typename union_container_traits<C1, Features>::const_iterator_list,
                                                   typename union_container_traits<C2, Features>::const_iterator_list, std::is_same>::type;
   using iterator = iterator_union<iterator_list>;
   using const_iterator = iterator_union<const_iterator_list>;
};

template <typename C1, typename C2, class Features>
struct union_container_traits_helper2<C1, C2, Features, true, true>
   : union_container_traits_helper2<C1, C2, Features, true, false> {
   using reverse_iterator_list = typename merge_list<typename union_container_traits<C1, Features>::reverse_iterator_list,
                                                     typename union_container_traits<C2, Features>::reverse_iterator_list, std::is_same>::type;
   using const_reverse_iterator_list = typename merge_list<typename union_container_traits<C1, Features>::const_reverse_iterator_list,
                                                           typename union_container_traits<C2, Features>::const_reverse_iterator_list, std::is_same>::type;
   using reverse_iterator = iterator_union<reverse_iterator_list> ;
   using const_reverse_iterator = iterator_union<const_reverse_iterator_list> ;
};

template <typename Head, typename Tail, class Features>
struct union_container_traits<cons<Head, Tail>, Features>
   : union_container_traits_helper2<Head, Tail, Features> {
   using traits1 = union_container_traits<Head, Features>;
   using traits2 = union_container_traits<Tail, Features>;
   using category = typename least_derived_class<typename traits1::category, typename traits2::category>::type;
   using reference = typename union_reference<typename traits1::reference, typename traits2::reference>::type;
   using const_reference = typename union_reference<typename traits1::const_reference, typename traits2::const_reference>::type;
   static const bool
      is_reversible=traits1::is_reversible && traits2::is_reversible,
      is_resizeable=traits1::is_resizeable && traits2::is_resizeable,
      is_always_const=traits1::is_always_const || traits2::is_always_const;
};

namespace virtuals {

template <typename Container>
struct size {
   static int _do(const char *c)
   {
      return basics<Container>::get(c).size();
   }
};
template <typename Container>
struct dim {
   static int _do(const char *c)
   {
      return get_dim(basics<Container>::get(c));
   };
};
template <typename Container>
struct empty {
   static bool _do(const char *c)
   {
      return basics<Container>::get(c).empty();
   }
};
template <typename Container>
struct resize {
   static void _do(char *c, int n)
   {
      basics<Container>::get(c).resize(n);
   }
};

template <typename ContainerList, typename Features>
struct container_union_functions : type_union_functions<ContainerList> {
   using base_t = type_union_functions<ContainerList>;
   using traits = union_container_traits<ContainerList, Features>;

   template <int discr>
   struct basics : virtuals::basics<typename n_th<ContainerList,discr>::type> {};

   struct size : base_t::length_def {
      template <int discr> struct defs : virtuals::size<typename n_th<ContainerList,discr>::type> {};
      using fpointer = int (*)(const char*);
   };
   struct dim : base_t::length_def {
      template <int discr> struct defs : virtuals::dim<typename n_th<ContainerList,discr>::type> {};
      using fpointer = int (*)(const char*);
   };
   struct empty : base_t::length_def {
      template <int discr> struct defs : virtuals::empty<typename n_th<ContainerList,discr>::type> {};
      using fpointer = bool (*)(const char*);
   };
   struct resize : base_t::length_def {
      template <int discr> struct defs : virtuals::resize<typename n_th<ContainerList,discr>::type> {};
      using fpointer = void (*)(char*, int);
   };
   struct begin : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::iterator _do(char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).begin();
         }
      };
      using fpointer = typename traits::iterator (*)(char*);
   };
   struct end : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::iterator _do(char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).end();
         }
      };
      using fpointer = typename traits::iterator (*)(char*);
   };
   struct const_begin : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::const_iterator _do(const char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).begin();
         }
      };
      using fpointer = typename traits::const_iterator (*)(const char*);
   };
   struct const_end : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::const_iterator _do(const char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).end();
         }
      };
      using fpointer = typename traits::const_iterator (*)(const char*);
   };
   struct rbegin : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::reverse_iterator _do(char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).rbegin();
         }
      };
      using fpointer = typename traits::reverse_iterator (*)(char*);
   };
   struct rend : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::reverse_iterator _do(char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).rend();
         }
      };
      using fpointer = typename traits::reverse_iterator (*)(char*);
   };
   struct const_rbegin : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::const_reverse_iterator _do(const char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).rbegin();
         }
      };
      using fpointer = typename traits::const_reverse_iterator (*)(const char*);
   };
   struct const_rend : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::const_reverse_iterator _do(const char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).rend();
         }
      };
      using fpointer = typename traits::const_reverse_iterator (*)(const char*);
   };
   struct front : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::reference _do(char* c)
         {
            return basics<discr>::get(c).front();
         }
      };
      using fpointer = typename traits::reference (*)(char*);
   };
   struct const_front : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::const_reference _do(const char* c)
         {
            return basics<discr>::get(c).front();
         }
      };
      using fpointer = typename traits::const_reference (*)(const char*);
   };
   struct back : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::reference _do(char* c)
         {
            return basics<discr>::get(c).back();
         }
      };
      using fpointer = typename traits::reference (*)(char*);
   };
   struct const_back : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::const_reference _do(const char* c)
         {
            return basics<discr>::get(c).back();
         }
      };
      using fpointer = typename traits::const_reference (*)(const char*);
   };
   struct random : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::reference _do(char* c, int i)
         {
            return basics<discr>::get(c)[i];
         }
      };
      using fpointer = typename traits::reference (*)(char*, int);
   };
   struct const_random : base_t::length_def {
      template <int discr> struct defs {
         static typename traits::const_reference _do(const char* c, int i)
         {
            return basics<discr>::get(c)[i];
         }
      };
      using fpointer = typename traits::const_reference (*)(const char*, int);
   };
};
} // end namespace virtuals

template <typename ContainerList, typename ProvidedFeatures,
          bool _enable=union_container_traits<ContainerList,ProvidedFeatures>::is_resizeable>
class container_union_resize {};

template <typename ContainerList, typename ProvidedFeatures,
          typename Category=typename union_container_traits<ContainerList,ProvidedFeatures>::category>
class container_union_elem_access {
protected:
   static const bool provide_sparse =
      list_accumulate_binary<list_or, check_container_ref_feature, ContainerList, same<sparse> >::value &&
      !list_accumulate_binary<list_and, check_container_ref_feature, ContainerList, same<sparse> >::value &&
      !list_search<ProvidedFeatures, dense, std::is_same>::value;
   using needed_features = typename std::conditional<provide_sparse,
                                                     typename mix_features<ProvidedFeatures, sparse_compatible>::type,
                                                     ProvidedFeatures>::type;
   using traits = union_container_traits<ContainerList, needed_features>;
   using Functions = virtuals::container_union_functions<ContainerList, needed_features>;
public:
   using reference = typename traits::reference;
   using const_reference = typename traits::const_reference;
   using value_type = typename deref<reference>::type;
   using container_category = typename traits::category;

   friend class container_union_resize<ContainerList,ProvidedFeatures>;
};

template <typename ContainerList, typename ProvidedFeatures=void>
class ContainerUnion
   : public type_union<ContainerList>
   , public container_union_elem_access<ContainerList,ProvidedFeatures>
   , public container_union_resize<ContainerList,ProvidedFeatures>
   , public inherit_generic<ContainerUnion<ContainerList,ProvidedFeatures>,
                            typename list_transform_unary<deref,ContainerList>::type>::type {
   using base_t = type_union<ContainerList>;
   using access_t = container_union_elem_access<ContainerList,ProvidedFeatures>;

   template <typename,typename,typename> friend class container_union_elem_access;
   friend class container_union_resize<ContainerList,ProvidedFeatures>;
public:
   using iterator = typename access_t::traits::iterator;
   using const_iterator = typename access_t::traits::const_iterator;

   ContainerUnion() {}

   template <typename T>
   ContainerUnion(const T& src) : base_t(src) {}

   template <typename T>
   ContainerUnion& operator= (const T& src)
   {
      base_t::operator=(src);
      return *this;
   }

   iterator begin()
   {
      return virtuals::table<typename access_t::Functions::begin>::call(this->discriminant)(this->area);
   }
   iterator end()
   {
      return virtuals::table<typename access_t::Functions::end>::call(this->discriminant)(this->area);
   }
   const_iterator begin() const
   {
      return virtuals::table<typename access_t::Functions::const_begin>::call(this->discriminant)(this->area);
   }
   const_iterator end() const
   {
      return virtuals::table<typename access_t::Functions::const_end>::call(this->discriminant)(this->area);
   }
   int size() const
   {
      return virtuals::table<typename access_t::Functions::size>::call(this->discriminant)(this->area);
   }
   bool empty() const
   {
      return virtuals::table<typename access_t::Functions::empty>::call(this->discriminant)(this->area);
   }
   int dim() const
   {
      return virtuals::table<typename access_t::Functions::dim>::call(this->discriminant)(this->area);
   }
};

template <typename ContainerList, typename ProvidedFeatures>
class container_union_elem_access<ContainerList, ProvidedFeatures, forward_iterator_tag>
   : public container_union_elem_access<ContainerList, ProvidedFeatures, input_iterator_tag> {
   using base_t = container_union_elem_access<ContainerList, ProvidedFeatures, input_iterator_tag>;
protected:
   using master = ContainerUnion<ContainerList,ProvidedFeatures>;
public:
   typename base_t::reference front()
   {
      master& me=static_cast<master&>(*this);
      return virtuals::table<typename base_t::Functions::front>::call(me.discriminant)(me.area);
   }
   typename base_t::const_reference front() const
   {
      const master& me=static_cast<const master&>(*this);
      return virtuals::table<typename base_t::Functions::const_front>::call(me.discriminant)(me.area);
   }
};

template <class ContainerList, class ProvidedFeatures>
class container_union_elem_access<ContainerList, ProvidedFeatures, bidirectional_iterator_tag>
   : public container_union_elem_access<ContainerList, ProvidedFeatures, forward_iterator_tag> {
   using base_t = container_union_elem_access<ContainerList, ProvidedFeatures, forward_iterator_tag>;
public:
   using reverse_iterator = typename base_t::traits::reverse_iterator;
   using const_reverse_iterator = typename base_t::traits::const_reverse_iterator;

   reverse_iterator rbegin()
   {
      auto& me=static_cast<typename base_t::master&>(*this);
      return virtuals::table<typename base_t::Functions::rbegin>::call(me.discriminant)(me.area);
   }
   reverse_iterator rend()
   {
      auto& me=static_cast<typename base_t::master&>(*this);
      return virtuals::table<typename base_t::Functions::rend>::call(me.discriminant)(me.area);
   }
   const_reverse_iterator rbegin() const
   {
      auto& me=static_cast<const typename base_t::master&>(*this);
      return virtuals::table<typename base_t::Functions::const_rbegin>::call(me.discriminant)(me.area);
   }
   const_reverse_iterator rend() const
   {
      auto& me=static_cast<const typename base_t::master&>(*this);
      return virtuals::table<typename base_t::Functions::const_rend>::call(me.discriminant)(me.area);
   }

   typename base_t::reference back()
   {
      auto& me=static_cast<typename base_t::master&>(*this);
      return virtuals::table<typename base_t::Functions::back>::call(me.discriminant)(me.area);
   }
   typename base_t::const_reference back() const
   {
      auto& me=static_cast<const typename base_t::master&>(*this);
      return virtuals::table<typename base_t::Functions::const_back>::call(me.discriminant)(me.area);
   }
};

template <typename ContainerList, typename ProvidedFeatures>
class container_union_elem_access<ContainerList, ProvidedFeatures, random_access_iterator_tag>
   : public container_union_elem_access<ContainerList, ProvidedFeatures, bidirectional_iterator_tag> {
   using base_t = container_union_elem_access<ContainerList, ProvidedFeatures, bidirectional_iterator_tag>;
public:
   typename base_t::reference operator[] (int i)
   {
      auto& me=static_cast<typename base_t::master&>(*this);
      return virtuals::table<typename base_t::Functions::random>::call(me.discriminant)(me.area, i);
   }
   typename base_t::const_reference operator[] (int i) const
   {
      auto& me=static_cast<const typename base_t::master&>(*this);
      return virtuals::table<typename base_t::Functions::const_random>::call(me.discriminant)(me.area, i);
   }
};

template <typename ContainerList, typename ProvidedFeatures>
class container_union_resize<ContainerList, ProvidedFeatures, true> {
protected:
   using master = ContainerUnion<ContainerList,ProvidedFeatures>;
   using base_t = container_union_elem_access<ContainerList, ProvidedFeatures>;
public:
   void resize(int n)
   {
      master& me=static_cast<master&>(*this);
      virtuals::table<typename base_t::Functions::resize>::call(me.discriminant)(me.area, n);
   }
};

template <typename ContainerList, typename ProvidedFeatures, typename Features>
struct enforce_features<ContainerUnion<ContainerList,ProvidedFeatures>, Features> {
   using container = ContainerUnion<ContainerList, typename mix_features<ProvidedFeatures, Features>::type>;
};

template <typename ContainerList, typename ProvidedFeatures>
struct spec_object_traits< ContainerUnion<ContainerList,ProvidedFeatures> >
   : spec_object_traits<is_container> {
   static const int is_resizeable    = union_container_traits<ContainerList,ProvidedFeatures>::is_resizeable;
   static const bool is_always_const = union_container_traits<ContainerList,ProvidedFeatures>::is_always_const,
                     is_persistent=false;
};

template <typename ContainerList, typename ProvidedFeatures, typename Feature>
struct check_container_feature<ContainerUnion<ContainerList,ProvidedFeatures>, Feature> {
   static const bool value=
      list_accumulate_binary<list_and, check_container_ref_feature, ContainerList, same<Feature> >::value ||
      list_accumulate_binary<list_or, absorbing_feature, ProvidedFeatures, same<Feature> >::value;
};

template <typename ContainerList, typename ProvidedFeatures>
struct check_container_feature<ContainerUnion<ContainerList,ProvidedFeatures>, sparse> {
   static const bool value=
      list_accumulate_binary<list_or, check_container_ref_feature, ContainerList, same<sparse> >::value &&
      !list_search<ProvidedFeatures, dense, std::is_same>::value;
};

template <typename ContainerList, typename ProvidedFeatures>
struct check_container_feature<ContainerUnion<ContainerList,ProvidedFeatures>, sparse_compatible>
   : check_container_feature<ContainerUnion<ContainerList,ProvidedFeatures>, sparse> {};

template <typename ContainerList, typename ProvidedFeatures>
struct extract_union_list< ContainerUnion<ContainerList, ProvidedFeatures> > {
   using type = ContainerList;
};

template <typename T1, typename T2>
struct union_reference_helper<T1, T2, is_container, is_container> {
   using type = ContainerUnion< typename merge_list<typename extract_union_list<T1>::type,
                                                    typename extract_union_list<T2>::type, std::is_same>::type >;
};

} // end namespace pm

namespace polymake {

using pm::ContainerUnion;

}

#endif // POLYMAKE_CONTAINER_UNION_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
