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
   typedef T type;
};

template <typename TypeList>
struct extract_union_list< type_union<TypeList> > {
   typedef TypeList type;
};

template <typename T1, typename T2,
          typename Model1=typename object_traits<typename deref<T1>::type>::model,
          typename Model2=typename object_traits<typename deref<T2>::type>::model>
struct union_reference_helper {
   typedef type_union< typename merge_list<typename extract_union_list<T1>::type,
                                           typename extract_union_list<T2>::type, identical>::type >
      type;
};

template <typename T1, typename T2>
struct union_reference<T1, T2, false> : union_reference_helper<T1, T2> {};

template <typename Iterator>
struct union_iterator_traits : iterator_traits<Iterator> {
   typedef typename iterator_traits<Iterator>::iterator iterator_list;
   typedef typename iterator_traits<Iterator>::const_iterator const_iterator_list;
};

template <typename Head, typename Tail>
struct union_iterator_traits< cons<Head,Tail> > {
   typedef union_iterator_traits<Head> traits1;
   typedef union_iterator_traits<Tail> traits2;

   typedef typename least_derived< cons<typename traits1::iterator_category, typename traits2::iterator_category> >::type
      iterator_category;
   typedef typename union_reference<typename traits1::reference, typename traits2::reference>::type reference;
   typedef typename deref<reference>::type value_type;
   typedef value_type *pointer;
   typedef typename identical<typename traits1::difference_type, typename traits2::difference_type>::type difference_type;

   typedef typename merge_list<typename traits1::iterator_list, typename traits2::iterator_list, identical>::type
      iterator_list;
   typedef typename merge_list<typename traits1::const_iterator_list, typename traits2::const_iterator_list, identical>::type
      const_iterator_list;
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
   typedef type_union_functions<IteratorList> _super;
   typedef union_iterator_traits<IteratorList> traits;

   template <int discr>
   struct basics : virtuals::iterator_basics<typename n_th<IteratorList,discr>::type> {};

   struct alt_copy_constructor : _super::length_def {
      template <int discr> struct defs : virtuals::alt_copy_constructor<typename n_th<IteratorList,discr>::type> {};
      typedef void (*fpointer)(char*, const char*);
   };
   struct assignment : _super::length_def {
      template <int discr> struct defs : virtuals::assignment<typename n_th<IteratorList,discr>::type> {};
      typedef void (*fpointer)(char*, const char*);
   };
   struct alt_assignment : _super::length_def {
      template <int discr> struct defs : virtuals::alt_assignment<typename n_th<IteratorList,discr>::type> {};
      typedef void (*fpointer)(char*, const char*);
   };
   struct dereference : _super::length_def {
      template <int discr> struct defs {
         static typename traits::reference _do(const char* it)
         {
            return *basics<discr>::get(it);
         }
      };
      typedef typename traits::reference (*fpointer)(const char*);
   };
   struct increment : _super::length_def {
      template <int discr> struct defs : virtuals::increment<typename n_th<IteratorList,discr>::type> {};
      typedef void (*fpointer)(char*);
   };
   struct decrement : _super::length_def {
      template <int discr> struct defs : virtuals::decrement<typename n_th<IteratorList,discr>::type> {};
      typedef void (*fpointer)(char*);
   };
   struct advance_plus : _super::length_def {
      template <int discr> struct defs : virtuals::advance_plus<typename n_th<IteratorList,discr>::type> {};
      typedef void (*fpointer)(char*, int);
   };
   struct advance_minus : _super::length_def {
      template <int discr> struct defs : virtuals::advance_minus<typename n_th<IteratorList,discr>::type> {};
      typedef void (*fpointer)(char*, int);
   };
   struct equality : _super::length_def {
      template <int discr> struct defs : virtuals::equality<typename n_th<IteratorList,discr>::type> {};
      typedef bool (*fpointer)(const char*, const char*);
   };
   struct difference : _super::length_def {
      template <int discr> struct defs : virtuals::difference<typename n_th<IteratorList,discr>::type> {};
      typedef typename traits::difference_type (*fpointer)(const char*, const char*);
   };
   struct random : _super::length_def {
      template <int discr> struct defs {
         static typename traits::reference _do(const char* it, int i)
         {
            return basics<discr>::get(it)[i];
         }
      };
      typedef typename traits::reference (*fpointer)(const char*, int);
   };
   struct index : _super::length_def {
      template <int discr> struct defs : virtuals::index<typename n_th<IteratorList,discr>::type> {};
      typedef int (*fpointer)(const char*);
   };
   struct at_end : _super::length_def {
      template <int discr> struct defs : virtuals::at_end<typename n_th<IteratorList,discr>::type> {};
      typedef bool (*fpointer)(const char*);
   };
   struct rewind : _super::length_def {
      template <int discr> struct defs : virtuals::rewind<typename n_th<IteratorList,discr>::type> {};
      typedef void (*fpointer)(char*);
   };
};
} // end namespace virtuals

template <typename IteratorList, typename Category=typename union_iterator_traits<IteratorList>::iterator_category>
class iterator_union : public type_union<IteratorList> {
protected:
   typedef type_union<IteratorList> super;
   typedef union_iterator_traits<IteratorList> traits;
   typedef virtuals::iterator_union_functions<IteratorList> Functions;
   template <int discr> struct basics : virtuals::basics<typename n_th<IteratorList,discr>::type> {};

   template <typename Iterator, int own_discr, int alt_discr>
   void _init_from_value(const Iterator& it, cons< int2type<own_discr>, int2type<alt_discr> >)
   {
      const int discr=const_first_nonnegative<own_discr,alt_discr>::value;
      this->discriminant=discr;
      basics<discr>::construct(this->area,it);
   }

   template <typename OtherList>
   void _init_from_value(const iterator_union<OtherList>& it, cons< int2type<-1>, int2type<-1> >)
   {
      _init_from_union(it, bool2type< list_mapping<OtherList, IteratorList>::mismatch >(),
                           bool2type< list_mapping<OtherList, typename traits::iterator_list>::mismatch >() );
   }

   template <typename OtherList, typename _discr2>
   void _init_from_union(const iterator_union<OtherList>& it, False, _discr2)
   {
      super::_init_from_union(it, False());
   }

   template <typename OtherList>
   void _init_from_union(const iterator_union<OtherList>& it, True, False)
   {
      this->discriminant=virtuals::mapping< typename list_mapping<OtherList, typename traits::iterator_list>::type >::table[it.discriminant];
      virtuals::table<typename Functions::alt_copy_constructor>::call(this->discriminant)(this->area,it.area);
   }

   template <typename Iterator>
   void _init(const Iterator& it, False, False)
   {
      _init_from_value(it, cons< int2type<list_search<IteratorList, Iterator, identical>::pos>,
                                 int2type<list_search<typename traits::iterator_list, Iterator, identical>::pos> >());
   }

   template <typename Iterator, typename _discr2>
   void _init(const Iterator& it, True, _discr2)
   {
      super::_init(it, True());
   }

   template <typename Iterator>
   void _init(const Iterator& it, False, True)
   {
      this->discriminant=it.discriminant;
      virtuals::table<typename Functions::alt_copy_constructor>::call(this->discriminant)(this->area,it.area);
   }

   template <typename Iterator, int own_discr, int alt_discr>
   void _assign_value(const Iterator& it, cons< int2type<own_discr>, int2type<alt_discr> >)
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
   void _assign_value(const iterator_union<OtherList>& it, cons< int2type<-1>, int2type<-1> >)
   {
      _assign_union(it, bool2type< list_mapping<OtherList, IteratorList>::mismatch >(),
                        bool2type< list_mapping<OtherList, typename traits::iterator_list>::mismatch >() );
   }

   template <typename OtherList, typename _discr2>
   void _assign_union(const iterator_union<OtherList>& it, False, _discr2)
   {
      super::_assign_union(it, False());
   }

   template <typename OtherList>
   void _assign_union(const iterator_union<OtherList>& it, True, False)
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
   void _assign(const Iterator& it, False, False)
   {
      _assign_value(it, cons< int2type<list_search<IteratorList, Iterator, identical>::pos>,
                              int2type<list_search<typename traits::iterator_list, Iterator, identical>::pos> >());
   }

   template <typename Iterator, typename _discr2>
   void _assign(const Iterator& src, True, _discr2)
   {
      super::_assign(src, True());
   }

   template <typename Iterator>
   void _assign(const Iterator& it, False, True)
   {
      if (this->discriminant==it.discriminant) {
         virtuals::table<typename Functions::alt_assignment>::call(this->discriminant)(this->area,it.area);
      } else {
         virtuals::table<typename Functions::destructor>::call(this->discriminant)(this->area);
         _init(it, False(), True());
      }
   }

   template <typename,typename> friend class iterator_union;
public:
   typedef typename traits::iterator_category iterator_category;
   typedef typename traits::value_type value_type;
   typedef typename traits::reference reference;
   typedef typename traits::pointer pointer;
   typedef typename traits::difference_type difference_type;
   typedef iterator_union<typename traits::iterator_list> iterator;
   typedef iterator_union<typename traits::const_iterator_list> const_iterator;
   typedef iterator_union<IteratorList> me;

   iterator_union() {}

   iterator_union(const iterator_union& it)
   {
      super::_init(it, True());
   }

   template <typename Iterator>
   iterator_union(const Iterator& it)
   {
      _init(it, derived_from<Iterator,iterator_union>(),
                derived_from<Iterator,iterator>());
   }

   iterator_union& operator= (const iterator_union& it)
   {
      super::_assign(it, True());
      return *this;
   }

   template <typename Iterator>
   iterator_union& operator= (const Iterator& it)
   {
      _assign(it, derived_from<Iterator,iterator_union>(),
                  derived_from<Iterator,iterator>());
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
      typedef typename enable_if<void, check_iterator_feature<iterator_union, end_sensitive>::value>::type
         error_if_unimplemented __attribute__((unused));
      return virtuals::table<typename Functions::at_end>::call(this->discriminant)(this->area);
   }

   int index() const
   {
      typedef typename enable_if<void, check_iterator_feature<iterator_union, indexed>::value>::type
         error_if_unimplemented __attribute__((unused));
      return virtuals::table<typename Functions::index>::call(this->discriminant)(this->area);
   }

   void rewind()
   {
      typedef typename enable_if<void, check_iterator_feature<iterator_union, rewindable>::value>::type
         error_if_unimplemented __attribute__((unused));
      virtuals::table<typename Functions::rewind>::call(this->discriminant)(this->area);
   }
};

template <typename IteratorList>
class iterator_union<IteratorList, bidirectional_iterator_tag>
   : public iterator_union<IteratorList, forward_iterator_tag> {
   typedef iterator_union<IteratorList, forward_iterator_tag> _super;
public:
   typedef iterator_union<IteratorList> me;

   iterator_union() {}

   template <typename Iterator>
   iterator_union(const Iterator& it) : _super(it) {}

   template <typename Iterator>
   iterator_union& operator= (const Iterator& it)
   {
      _super::operator=(it);
      return *this;
   }

   me& operator-- ()
   {
      virtuals::table<typename _super::Functions::decrement>::call(this->discriminant)(this->area);
      return static_cast<me&>(*this);
   }
   me operator--(int) { me copy=static_cast<me&>(*this); operator--(); return copy; }
};

template <typename IteratorList>
class iterator_union<IteratorList, random_access_iterator_tag>
   : public iterator_union<IteratorList, bidirectional_iterator_tag> {
   typedef iterator_union<IteratorList, bidirectional_iterator_tag> _super;
public:
   iterator_union() {}

   template <typename Iterator>
   iterator_union(const Iterator& it) : _super(it) {}

   template <typename Iterator>
   iterator_union& operator= (const Iterator& it)
   {
      _super::operator=(it);
      return *this;
   }

   iterator_union& operator+= (int i)
   {
      virtuals::table<typename _super::Functions::advance_plus>::call(this->discriminant)(this->area);
      return *this;
   }
   iterator_union& operator-= (int i)
   {
      virtuals::table<typename _super::Functions::advance_minus>::call(this->discriminant)(this->area);
      return *this;
   }
   const iterator_union operator+ (int i) const { iterator_union copy=*this; return copy+=i; }
   const iterator_union operator- (int i) const { iterator_union copy=*this; return copy-=i; }
   friend const iterator_union operator+ (int i, const iterator_union& it) { return it+i; }

   typename _super::difference_type operator- (const iterator_union& it) const
   {
      return virtuals::table<typename _super::Functions::difference>::call(this->discriminant)(this->area);
   }

   typename _super::reference operator[] (int i) const
   {
      return virtuals::table<typename _super::Functions::random>::call(this->discriminant)(this->area,i);
   }
};

template <typename IteratorList, typename Feature, typename Category>
struct check_iterator_feature<iterator_union<IteratorList,Category>, Feature>
   : list_accumulate_binary<list_and, check_iterator_feature, IteratorList, same<Feature> > {};

template <typename IteratorList, typename Category>
struct extract_union_list< iterator_union<IteratorList,Category> > {
   typedef IteratorList type;
};

/* -----------------
    ContainerUnion
   ----------------- */

template <typename ContainerRef, typename Features,
          bool _reversible=container_traits<ContainerRef>::is_bidirectional>
struct union_container_traits_helper : ensure_features<typename deref<ContainerRef>::minus_ref, Features> {
   typedef ensure_features<typename deref<ContainerRef>::minus_ref, Features> _super;
   typedef typename extract_union_list<typename _super::iterator>::type iterator_list;
   typedef typename extract_union_list<typename _super::const_iterator>::type const_iterator_list;
   static const bool
      is_reversible=_reversible,   // = false
      is_resizeable=object_traits<typename deref<ContainerRef>::type>::is_resizeable==1,
      is_always_const=effectively_const<ContainerRef>::value;
};

template <typename ContainerRef, typename Features>
struct union_container_traits_helper<ContainerRef, Features, true>
   : union_container_traits_helper<ContainerRef, Features, false> {
   typedef union_container_traits_helper<ContainerRef, Features, false> _super;
   typedef typename extract_union_list<typename _super::reverse_iterator>::type reverse_iterator_list;
   typedef typename extract_union_list<typename _super::const_reverse_iterator>::type const_reverse_iterator_list;
   static const bool is_reversible=true;
};

template <typename ContainerRef, typename Features=void>
struct union_container_traits : union_container_traits_helper<ContainerRef, Features> {};

template <typename C1, typename C2, typename Features,
          bool _need_union=!identical<typename union_container_traits<C1, Features>::iterator,
                                      typename union_container_traits<C2, Features>::iterator>::value,
          bool _reversible=union_container_traits<C1, Features>::is_reversible &&
                           union_container_traits<C2, Features>::is_reversible>
struct union_container_traits_helper2
   : union_container_traits<C1, Features> {};

template <typename C1, typename C2, typename Features>
struct union_container_traits_helper2<C1, C2, Features, true, false> {
   typedef typename merge_list<typename union_container_traits<C1, Features>::iterator_list,
                               typename union_container_traits<C2, Features>::iterator_list, identical>::type
      iterator_list;
   typedef typename merge_list<typename union_container_traits<C1, Features>::const_iterator_list,
                               typename union_container_traits<C2, Features>::const_iterator_list, identical>::type
      const_iterator_list;
   typedef iterator_union<iterator_list> iterator;
   typedef iterator_union<const_iterator_list> const_iterator;
};

template <typename C1, typename C2, class Features>
struct union_container_traits_helper2<C1, C2, Features, true, true>
   : union_container_traits_helper2<C1, C2, Features, true, false> {
   typedef typename merge_list<typename union_container_traits<C1, Features>::reverse_iterator_list,
                               typename union_container_traits<C2, Features>::reverse_iterator_list, identical>::type
      reverse_iterator_list;
   typedef typename merge_list<typename union_container_traits<C1, Features>::const_reverse_iterator_list,
                               typename union_container_traits<C2, Features>::const_reverse_iterator_list, identical>::type
      const_reverse_iterator_list;
   typedef iterator_union<reverse_iterator_list> reverse_iterator;
   typedef iterator_union<const_reverse_iterator_list> const_reverse_iterator;
};

template <typename Head, typename Tail, class Features>
struct union_container_traits<cons<Head, Tail>, Features>
   : union_container_traits_helper2<Head, Tail, Features> {
   typedef union_container_traits<Head, Features> traits1;
   typedef union_container_traits<Tail, Features> traits2;
   typedef typename least_derived< cons<typename traits1::category, typename traits2::category> >::type
      category;
   typedef typename union_reference<typename traits1::reference, typename traits2::reference>::type
      reference;
   typedef typename union_reference<typename traits1::const_reference, typename traits2::const_reference>::type
      const_reference;
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
   typedef type_union_functions<ContainerList> _super;
   typedef union_container_traits<ContainerList, Features> traits;

   template <int discr>
   struct basics : virtuals::basics<typename n_th<ContainerList,discr>::type> {};

   struct size : _super::length_def {
      template <int discr> struct defs : virtuals::size<typename n_th<ContainerList,discr>::type> {};
      typedef int (*fpointer)(const char*);
   };
   struct dim : _super::length_def {
      template <int discr> struct defs : virtuals::dim<typename n_th<ContainerList,discr>::type> {};
      typedef int (*fpointer)(const char*);
   };
   struct empty : _super::length_def {
      template <int discr> struct defs : virtuals::empty<typename n_th<ContainerList,discr>::type> {};
      typedef bool (*fpointer)(const char*);
   };
   struct resize : _super::length_def {
      template <int discr> struct defs : virtuals::resize<typename n_th<ContainerList,discr>::type> {};
      typedef void (*fpointer)(char*, int);
   };
   struct begin : _super::length_def {
      template <int discr> struct defs {
         static typename traits::iterator _do(char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).begin();
         }
      };
      typedef typename traits::iterator (*fpointer)(char*);
   };
   struct end : _super::length_def {
      template <int discr> struct defs {
         static typename traits::iterator _do(char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).end();
         }
      };
      typedef typename traits::iterator (*fpointer)(char*);
   };
   struct const_begin : _super::length_def {
      template <int discr> struct defs {
         static typename traits::const_iterator _do(const char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).begin();
         }
      };
      typedef typename traits::const_iterator (*fpointer)(const char*);
   };
   struct const_end : _super::length_def {
      template <int discr> struct defs {
         static typename traits::const_iterator _do(const char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).end();
         }
      };
      typedef typename traits::const_iterator (*fpointer)(const char*);
   };
   struct rbegin : _super::length_def {
      template <int discr> struct defs {
         static typename traits::reverse_iterator _do(char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).rbegin();
         }
      };
      typedef typename traits::reverse_iterator (*fpointer)(char*);
   };
   struct rend : _super::length_def {
      template <int discr> struct defs {
         static typename traits::reverse_iterator _do(char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).rend();
         }
      };
      typedef typename traits::reverse_iterator (*fpointer)(char*);
   };
   struct const_rbegin : _super::length_def {
      template <int discr> struct defs {
         static typename traits::const_reverse_iterator _do(const char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).rbegin();
         }
      };
      typedef typename traits::const_reverse_iterator (*fpointer)(const char*);
   };
   struct const_rend : _super::length_def {
      template <int discr> struct defs {
         static typename traits::const_reverse_iterator _do(const char* c)
         {
            return ensure(basics<discr>::get(c), (Features*)0).rend();
         }
      };
      typedef typename traits::const_reverse_iterator (*fpointer)(const char*);
   };
   struct front : _super::length_def {
      template <int discr> struct defs {
         static typename traits::reference _do(char* c)
         {
            return basics<discr>::get(c).front();
         }
      };
      typedef typename traits::reference (*fpointer)(char*);
   };
   struct const_front : _super::length_def {
      template <int discr> struct defs {
         static typename traits::const_reference _do(const char* c)
         {
            return basics<discr>::get(c).front();
         }
      };
      typedef typename traits::const_reference (*fpointer)(const char*);
   };
   struct back : _super::length_def {
      template <int discr> struct defs {
         static typename traits::reference _do(char* c)
         {
            return basics<discr>::get(c).back();
         }
      };
      typedef typename traits::reference (*fpointer)(char*);
   };
   struct const_back : _super::length_def {
      template <int discr> struct defs {
         static typename traits::const_reference _do(const char* c)
         {
            return basics<discr>::get(c).back();
         }
      };
      typedef typename traits::const_reference (*fpointer)(const char*);
   };
   struct random : _super::length_def {
      template <int discr> struct defs {
         static typename traits::reference _do(char* c, int i)
         {
            return basics<discr>::get(c)[i];
         }
      };
      typedef typename traits::reference (*fpointer)(char*, int);
   };
   struct const_random : _super::length_def {
      template <int discr> struct defs {
         static typename traits::const_reference _do(const char* c, int i)
         {
            return basics<discr>::get(c)[i];
         }
      };
      typedef typename traits::const_reference (*fpointer)(const char*, int);
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
   static const bool _provide_sparse=
      list_accumulate_binary<list_or, check_container_ref_feature, ContainerList, same<sparse> >::value &&
      !list_accumulate_binary<list_and, check_container_ref_feature, ContainerList, same<sparse> >::value &&
      !list_search<ProvidedFeatures, dense, identical>::value;
   typedef typename if_else<_provide_sparse, typename mix_features<ProvidedFeatures, sparse_compatible>::type,
                                             ProvidedFeatures>::type
      needed_features;

   typedef union_container_traits<ContainerList, needed_features> traits;
   typedef virtuals::container_union_functions<ContainerList, needed_features> Functions;
public:
   typedef typename traits::reference reference;
   typedef typename traits::const_reference const_reference;
   typedef typename deref<reference>::type value_type;
   typedef typename traits::category container_category;

   friend class container_union_resize<ContainerList,ProvidedFeatures>;
};

template <typename ContainerList, typename ProvidedFeatures=void>
class ContainerUnion
   : public type_union<ContainerList>,
     public container_union_elem_access<ContainerList,ProvidedFeatures>,
     public container_union_resize<ContainerList,ProvidedFeatures>,
     public inherit_generic<ContainerUnion<ContainerList,ProvidedFeatures>,
                            typename list_transform_unary<deref,ContainerList>::type>::type {
   typedef type_union<ContainerList> super;
   typedef container_union_elem_access<ContainerList,ProvidedFeatures> _super;

   template <typename,typename,typename> friend class container_union_elem_access;
   friend class container_union_resize<ContainerList,ProvidedFeatures>;
public:
   typedef typename _super::traits::iterator iterator;
   typedef typename _super::traits::const_iterator const_iterator;

   ContainerUnion() {}

   template <typename T>
   ContainerUnion(const T& src) : super(src) {}

   template <typename T>
   ContainerUnion& operator= (const T& src)
   {
      super::operator=(src);
      return *this;
   }

   iterator begin()
   {
      return virtuals::table<typename _super::Functions::begin>::call(this->discriminant)(this->area);
   }
   iterator end()
   {
      return virtuals::table<typename _super::Functions::end>::call(this->discriminant)(this->area);
   }
   const_iterator begin() const
   {
      return virtuals::table<typename _super::Functions::const_begin>::call(this->discriminant)(this->area);
   }
   const_iterator end() const
   {
      return virtuals::table<typename _super::Functions::const_end>::call(this->discriminant)(this->area);
   }
   int size() const
   {
      return virtuals::table<typename _super::Functions::size>::call(this->discriminant)(this->area);
   }
   bool empty() const
   {
      return virtuals::table<typename _super::Functions::empty>::call(this->discriminant)(this->area);
   }
   int dim() const
   {
      return virtuals::table<typename _super::Functions::dim>::call(this->discriminant)(this->area);
   }
};

template <typename ContainerList, typename ProvidedFeatures>
class container_union_elem_access<ContainerList, ProvidedFeatures, forward_iterator_tag>
   : public container_union_elem_access<ContainerList, ProvidedFeatures, input_iterator_tag> {
   typedef container_union_elem_access<ContainerList, ProvidedFeatures, input_iterator_tag> _super;
protected:
   typedef ContainerUnion<ContainerList,ProvidedFeatures> master;
public:
   typename _super::reference front()
   {
      master& me=static_cast<master&>(*this);
      return virtuals::table<typename _super::Functions::front>::call(me.discriminant)(me.area);
   }
   typename _super::const_reference front() const
   {
      const master& me=static_cast<const master&>(*this);
      return virtuals::table<typename _super::Functions::const_front>::call(me.discriminant)(me.area);
   }
};

template <class ContainerList, class ProvidedFeatures>
class container_union_elem_access<ContainerList, ProvidedFeatures, bidirectional_iterator_tag>
   : public container_union_elem_access<ContainerList, ProvidedFeatures, forward_iterator_tag> {
   typedef container_union_elem_access<ContainerList, ProvidedFeatures, forward_iterator_tag> _super;
public:
   typedef typename _super::traits::reverse_iterator reverse_iterator;
   typedef typename _super::traits::const_reverse_iterator const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typename _super::master& me=static_cast<typename _super::master&>(*this);
      return virtuals::table<typename _super::Functions::rbegin>::call(me.discriminant)(me.area);
   }
   reverse_iterator rend()
   {
      typename _super::master& me=static_cast<typename _super::master&>(*this);
      return virtuals::table<typename _super::Functions::rend>::call(me.discriminant)(me.area);
   }
   const_reverse_iterator rbegin() const
   {
      const typename _super::master& me=static_cast<const typename _super::master&>(*this);
      return virtuals::table<typename _super::Functions::const_rbegin>::call(me.discriminant)(me.area);
   }
   const_reverse_iterator rend() const
   {
      const typename _super::master& me=static_cast<const typename _super::master&>(*this);
      return virtuals::table<typename _super::Functions::const_rend>::call(me.discriminant)(me.area);
   }

   typename _super::reference back()
   {
      typename _super::master& me=static_cast<typename _super::master&>(*this);
      return virtuals::table<typename _super::Functions::back>::call(me.discriminant)(me.area);
   }
   typename _super::const_reference back() const
   {
      const typename _super::master& me=static_cast<const typename _super::master&>(*this);
      return virtuals::table<typename _super::Functions::const_back>::call(me.discriminant)(me.area);
   }
};

template <typename ContainerList, typename ProvidedFeatures>
class container_union_elem_access<ContainerList, ProvidedFeatures, random_access_iterator_tag>
   : public container_union_elem_access<ContainerList, ProvidedFeatures, bidirectional_iterator_tag> {
   typedef container_union_elem_access<ContainerList, ProvidedFeatures, bidirectional_iterator_tag> _super;
public:
   typename _super::reference operator[] (int i)
   {
      typename _super::master& me=static_cast<typename _super::master&>(*this);
      return virtuals::table<typename _super::Functions::random>::call(me.discriminant)(me.area, i);
   }
   typename _super::const_reference operator[] (int i) const
   {
      const typename _super::master& me=static_cast<const typename _super::master&>(*this);
      return virtuals::table<typename _super::Functions::const_random>::call(me.discriminant)(me.area, i);
   }
};

template <typename ContainerList, typename ProvidedFeatures>
class container_union_resize<ContainerList, ProvidedFeatures, true> {
protected:
   typedef ContainerUnion<ContainerList,ProvidedFeatures> master;
   typedef container_union_elem_access<ContainerList, ProvidedFeatures> _super;
public:
   void resize(int n)
   {
      master& me=static_cast<master&>(*this);
      virtuals::table<typename _super::Functions::resize>::call(me.discriminant)(me.area, n);
   }
};

template <typename ContainerList, typename ProvidedFeatures, typename Features>
struct enforce_features<ContainerUnion<ContainerList,ProvidedFeatures>, Features> {
   typedef ContainerUnion<ContainerList, typename mix_features<ProvidedFeatures, Features>::type>
      container;
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
      !list_search<ProvidedFeatures, dense, identical>::value;
};

template <typename ContainerList, typename ProvidedFeatures>
struct check_container_feature<ContainerUnion<ContainerList,ProvidedFeatures>, sparse_compatible>
   : check_container_feature<ContainerUnion<ContainerList,ProvidedFeatures>, sparse> {};

template <typename ContainerList, typename ProvidedFeatures>
struct extract_union_list< ContainerUnion<ContainerList, ProvidedFeatures> > {
   typedef ContainerList type;
};

template <typename T1, typename T2>
struct union_reference_helper<T1, T2, is_container, is_container> {
   typedef ContainerUnion< typename merge_list<typename extract_union_list<T1>::type,
                                               typename extract_union_list<T2>::type, identical>::type >
      type;
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
