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

#ifndef POLYMAKE_CASCADED_CONTAINER_H
#define POLYMAKE_CASCADED_CONTAINER_H

#include "polymake/internal/iterators.h"
#include "polymake/internal/converters.h"

namespace pm {

template <typename Iterator, typename ExpectedFeatures, int depth> class cascaded_iterator;

template <typename Iterator, typename ExpectedFeatures, int depth>
struct cascaded_iterator_traits {
   typedef typename ensure_features<typename attrib<typename iterator_traits<Iterator>::reference>::minus_ref,
                                    ExpectedFeatures>::iterator
      down_iterator;
   typedef cascaded_iterator<down_iterator, ExpectedFeatures, depth-1> super;

   static bool super_init(super& it, typename iterator_traits<Iterator>::reference c)
   {
      it.cur=ensure(c,(ExpectedFeatures*)0).begin();
      return it.init();
   }

   static bool super_incr(super& it)
   {
      return it.incr();
   }
};

template <typename Iterator, typename ExpectedFeatures>
struct cascaded_iterator_traits<Iterator,ExpectedFeatures,2> {
   typedef typename ensure_features<typename attrib<typename iterator_traits<Iterator>::reference>::minus_ref,
                                    ExpectedFeatures>::iterator
      down_iterator;

   static const bool accumulate_offset= check_container_feature<typename iterator_traits<Iterator>::value_type, sparse>::value ||
                                        list_search<ExpectedFeatures, indexed, absorbing_feature>::value;
   typedef typename if_else<accumulate_offset, typename mix_features<ExpectedFeatures,indexed>::type, ExpectedFeatures>::type
      super_features;
   typedef cascaded_iterator<down_iterator,super_features,1> super;

   static bool super_init(super& it, typename iterator_traits<Iterator>::reference c)
   {
      it.index_store.store_dim(c);
      static_cast<down_iterator&>(it)=ensure(c,(super_features*)0).begin();
      return !it.at_end() || (it.index_store.adjust_offset(), false);
   }

   static bool super_incr(super& it)
   {
      ++it;
      return !it.at_end() || (it.index_store.adjust_offset(), false);
   }
};

template <typename Iterator, typename ExpectedFeatures, int depth>
class cascaded_iterator
   : public cascaded_iterator_traits<Iterator,ExpectedFeatures,depth>::super {
   typedef cascaded_iterator_traits<Iterator,ExpectedFeatures,depth> traits;
public:
   typedef cascaded_iterator<typename iterator_traits<Iterator>::iterator, ExpectedFeatures, depth>
      iterator;
   typedef cascaded_iterator<typename iterator_traits<Iterator>::const_iterator, ExpectedFeatures, depth>
      const_iterator;
   typedef typename traits::super super;
protected:
   template <typename,typename,int> friend struct cascaded_iterator_traits;
   template <typename,typename,int> friend class cascaded_iterator;
   Iterator cur;

   bool _at_end(True) const { return false; }
   bool _at_end(False) const { return cur.at_end(); }

   bool init()
   {
      while (!at_end()) {
         if (traits::super_init(*this,*cur)) return true;
         ++cur;
      }
      return false;
   }

   bool incr() { return traits::super_incr(*this) || (++cur, init()); }

public:
   cascaded_iterator() {}
   cascaded_iterator(const Iterator& cur_arg)
      : cur(cur_arg) { init(); }
   cascaded_iterator(typename alt_constructor<Iterator>::arg_type& cur_arg)
      : cur(cur_arg) { init(); }

   cascaded_iterator(const iterator& it)
      : super(it), cur(it.cur) {}

   cascaded_iterator& operator= (const iterator& it)
   {
      cur=it.cur;
      super::operator=(it);
      return *this;
   }

   cascaded_iterator& operator++ () { incr(); return *this; }
   const cascaded_iterator operator++ (int) { cascaded_iterator copy=*this; operator++(); return copy; }

   bool operator== (const iterator& it) const
   {
      return cur==it.cur && super::operator==(it);
   }
   bool operator== (typename assign_const<const_iterator, !identical<iterator,const_iterator>::value>::type& it)
   {
      return cur==it.cur && super::operator==(it);
   }

   bool operator!= (const iterator& it) const { return !operator==(it); }
   bool operator!= (typename assign_const<const_iterator, !identical<iterator,const_iterator>::value>::type& it) { return !operator==(it); }

   bool at_end() const { return _at_end(bool2type<check_iterator_feature<Iterator,unlimited>::value>()); }

   void rewind()
   {
      typedef typename enable_if_iterator<Iterator,rewindable>::type error_if_unimplemented __attribute__((unused));
      cur.rewind(); init();
   }
};

template <bool _need_index>
struct cascaded_iterator_index_store {
   template <typename Container>
   void store_dim(const Container&) {}
   void adjust_offset() {}
};

template <>
struct cascaded_iterator_index_store<true> {
   int offset, dim;

   cascaded_iterator_index_store() : offset(0) {}

   template <typename Container>
   void store_dim(const Container& c) { dim=get_dim(c); }
   void adjust_offset() { offset+=dim; }
};

template <typename Iterator, typename ExpectedFeatures>
class cascaded_iterator<Iterator, ExpectedFeatures, 1>
   : public Iterator {
public:
   typedef Iterator super;
protected:
   cascaded_iterator_index_store<list_search_all<ExpectedFeatures, indexed, absorbing_feature>::value> index_store;
   template <typename,typename,int> friend struct cascaded_iterator_traits;
   template <typename,typename,int> friend class cascaded_iterator;
public:
   typedef forward_iterator_tag iterator_category;

   cascaded_iterator() {}
   cascaded_iterator(const cascaded_iterator<typename iterator_traits<Iterator>::iterator, ExpectedFeatures, 1>& it)
      : super(it), index_store(it.index_store) {}

   cascaded_iterator& operator= (const cascaded_iterator<typename iterator_traits<Iterator>::iterator, ExpectedFeatures, 1>& it)
   {
      super::operator=(it);
      index_store=it.index_store;
      return *this;
   }

   int index() const { return super::index()+index_store.offset; }
};

template <typename Iterator, typename ExpectedFeatures, int depth, typename Feature>
struct check_iterator_feature<cascaded_iterator<Iterator, ExpectedFeatures, depth>, Feature> {
   typedef cons<end_sensitive, cons<rewindable, unlimited> > top_level_features;
   typedef typename if_else<list_contains<top_level_features, Feature>::value, Iterator,
                            typename cascaded_iterator<Iterator, ExpectedFeatures, depth>::super>::type check_iterator;
   static const bool value=check_iterator_feature<check_iterator, Feature>::value;
};

template <typename> class CascadeDepth {};

template <typename Container, int depth>
struct cascade_traits : cascade_traits<typename container_traits<Container>::value_type, depth-1> {
   typedef typename least_derived< cons<typename container_traits<Container>::category,
                                        typename cascade_traits<typename container_traits<Container>::value_type, depth-1>::category> >::type
      category;
};

template <typename Container>
struct cascade_traits<Container, 1> : container_traits<Container> {
   typedef typename least_derived< cons<typename container_traits<Container>::category,
                                        bidirectional_iterator_tag> >::type
      category;
   typedef Container base_container;
};

template <typename Top, typename Params>
class cascade_typebase : public manip_container_top<Top, Params> {
   typedef manip_container_top<Top,Params> _super;
public:
   typedef typename extract_type_param<Params, Container, typename _super::hidden_type>::type container_ref;
   typedef typename deref<container_ref>::minus_ref container;
   typedef typename temp_ref<container_ref>::type container_temp_ref;

   static const int depth=extract_int_param<Params, CascadeDepth>::value;
   typedef dense can_enforce_features;
   typedef typename mix_features<typename _super::expected_features, end_sensitive>::type needed_features;
   typedef typename list_search_all<rewindable, needed_features, absorbing_feature>::negative2 needed_down_features;
   typedef cascaded_iterator<typename ensure_features<container,needed_features>::iterator, needed_down_features, depth>
      iterator;
   typedef cascaded_iterator<typename ensure_features<container,needed_features>::const_iterator, needed_down_features, depth>
      const_iterator;
   typedef typename iterator::reference reference;
   typedef typename const_iterator::reference const_reference;
   typedef typename iterator::value_type value_type;

   typedef cascade_traits<container,depth> base_traits;
   typedef typename base_traits::category container_category;
};

template <typename Top, typename Params=typename Top::manipulator_params,
          typename Category=typename cascade_typebase<Top,Params>::container_category>
class cascade_impl
   : public cascade_typebase<Top,Params> {
   typedef cascade_typebase<Top,Params> _super;
public:
   typedef Params manipulator_params;
   typedef cascade_impl<Top,Params> manipulator_impl;
   typedef typename _super::iterator iterator;
   typedef typename _super::const_iterator const_iterator;

   template <typename FeatureCollector>
   struct rebind_feature_collector {
      typedef cascade_impl<FeatureCollector,Params> type;
   };

   iterator begin()
   {
      typename _super::container_temp_ref c=this->manip_top().get_container();
      return iterator(ensure(c, (typename _super::needed_features*)0).begin());
   }
   iterator end()
   {
      typename _super::container_temp_ref c=this->manip_top().get_container();
      return iterator(ensure(c, (typename _super::needed_features*)0).end());
   }
   const_iterator begin() const
   {
      return const_iterator(ensure(this->manip_top().get_container(), (typename _super::needed_features*)0).begin());
   }
   const_iterator end() const
   {
      return const_iterator(ensure(this->manip_top().get_container(), (typename _super::needed_features*)0).end());
   }

   int size() const
   {
      return cascade_size(this->manip_top().get_container(), int2type<_super::depth>());
   }
   bool empty() const
   {
      return cascade_empty(this->manip_top().get_container(), int2type<_super::depth>());
   }
   int dim() const
   {
      return cascade_dim(this->manip_top().get_container(), int2type<_super::depth>());
   }
};

template <typename Top, typename Params>
class cascade_impl<Top, Params, forward_iterator_tag>
   : public cascade_impl<Top, Params, input_iterator_tag> {
   typedef cascade_impl<Top, Params, input_iterator_tag> _super;
public:
   typename _super::reference front() { return *this->begin(); }
   typename _super::const_reference front() const { return *this->begin(); }
};

template <typename Top, typename Params>
class cascade_impl<Top, Params, bidirectional_iterator_tag>
   : public cascade_impl<Top, Params, forward_iterator_tag> {
   typedef cascade_impl<Top, Params, forward_iterator_tag> _super;
public:
   typedef typename toggle_features<typename _super::needed_features, _reversed>::type
      needed_reverse_features;
   typedef typename toggle_features<typename _super::needed_down_features, _reversed>::type
      needed_reverse_down_features;
   typedef cascaded_iterator<typename ensure_features<typename _super::container, needed_reverse_features>::iterator,
                             needed_reverse_down_features, cascade_typebase<Top,Params>::depth>
      reverse_iterator;
   typedef cascaded_iterator<typename ensure_features<typename _super::container, needed_reverse_features>::const_iterator,
                             needed_reverse_down_features, cascade_typebase<Top,Params>::depth>
      const_reverse_iterator;

   reverse_iterator rbegin()
   {
      typename _super::container_temp_ref c=this->manip_top().get_container();
      return reverse_iterator(ensure(c, (needed_reverse_features*)0).begin());
   }
   reverse_iterator rend()
   {
      typename _super::container_temp_ref c=this->manip_top().get_container();
      return reverse_iterator(ensure(c, (needed_reverse_features*)0).end());
   }
   const_reverse_iterator rbegin() const
   {
      return const_reverse_iterator(ensure(this->manip_top().get_container(), (needed_reverse_features*)0).begin());
   }
   const_reverse_iterator rend() const
   {
      return const_reverse_iterator(ensure(this->manip_top().get_container(), (needed_reverse_features*)0).end());
   }

   typename _super::reference back() { return *rbegin(); }
   typename _super::const_reference back() const { return *rbegin(); }
};

template <typename Container, int depth> inline
int cascade_size(const Container& c, int2type<depth>)
{
   int size=0;
   for (typename Entire<Container>::const_iterator i=entire(c); !i.at_end(); ++i)
      size+=cascade_size(*i, int2type<depth-1>());
   return size;
}

template <typename Container> inline
int cascade_size(const Container& c, int2type<1>)
{
   return c.size();
}

template <typename Container, int depth> inline
int cascade_dim(const Container& c, int2type<depth>)
{
   int d=0;
   for (typename Entire<Container>::const_iterator i=entire(c); !i.at_end(); ++i)
      d+=cascade_dim(*i, int2type<depth-1>());
   return d;
}

template <typename Container> inline
int cascade_dim(const Container& c, int2type<1>)
{
   return get_dim(c);
}

template <typename Container, int depth> inline
bool cascade_empty(const Container& c, int2type<depth>)
{
   for (typename Entire<Container>::const_iterator i=entire(c); !i.at_end(); ++i)
      if (!cascade_empty(*i, int2type<depth-1>())) return false;
   return true;
}

template <typename Container> inline
bool cascade_empty(const Container& c, int2type<1>)
{
   return c.empty();
}

template <typename ContainerRef, int depth=object_traits<typename deref<ContainerRef>::type>::nesting_level>
class CascadedContainer
   : public cascade_impl< CascadedContainer<ContainerRef,depth>,
                          list( Container< ContainerRef >,
                                CascadeDepth< int2type<depth> > ) > {
protected:
   typedef typename alias<ContainerRef>::arg_type arg_type;
   alias<ContainerRef> src;
public:
   CascadedContainer(arg_type src_arg) : src(src_arg) {}

   typename alias<ContainerRef>::reference get_container() { return *src; }
   typename alias<ContainerRef>::const_reference get_container() const { return *src; }
};

template <typename ContainerRef, int depth>
struct check_container_feature<CascadedContainer<ContainerRef, depth>, sparse>
   : check_container_feature<typename CascadedContainer<ContainerRef,depth>::base_traits::base_container, sparse> {};

template <typename ContainerRef, int depth>
struct check_container_feature<CascadedContainer<ContainerRef, depth>, pure_sparse>
   : check_container_feature<typename CascadedContainer<ContainerRef,depth>::base_traits::base_container, pure_sparse> {};

template <typename ContainerRef, int depth>
struct spec_object_traits< CascadedContainer<ContainerRef, depth> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true;
};

template <typename Container> inline
CascadedContainer<Container&>
cascade(Container& c) { return c; }

template <typename Container> inline
CascadedContainer<const Container&>
cascade(const Container& c) { return c; }

template <int depth, typename Container> inline
CascadedContainer<Container&, depth>
cascade(Container& c, int2type<depth>) { return c; }

template <int depth, typename Container> inline
CascadedContainer<const Container&, depth>
cascade(const Container& c, int2type<depth>) { return c; }

template <typename Container, typename ExpectedFeatures, int depth> inline
cascaded_iterator<typename Container::iterator, typename mix_features<ExpectedFeatures, end_sensitive>::type, depth>
make_cascade_iterator(Container& src, ExpectedFeatures*, int2type<depth>)
{
   return entire(src);
}

template <typename Container, typename ExpectedFeatures, int depth> inline
cascaded_iterator<typename Container::const_iterator, typename mix_features<ExpectedFeatures, end_sensitive>::type, depth>
make_cascade_iterator(const Container& src, ExpectedFeatures*, int2type<depth>)
{
   return entire(src);
}


template <typename Iterator, typename Target, typename ExpectedFeatures=void,
          typename ValueModel=typename object_traits<typename iterator_traits<Iterator>::value_type>::model,
          typename is_enabled=void>
struct construct_cascaded_iterator {};

template <typename Iterator, typename Target, typename ExpectedFeatures, typename ValueModel>
struct construct_cascaded_iterator<Iterator, Target, ExpectedFeatures, ValueModel,
                                   typename enable_if<void, (isomorphic_types<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                             convertible_to<typename iterator_traits<Iterator>::value_type, Target>::value)>::type>
{
   typedef void** enabled;
   static const int depth=1;
   static const int convertible_to=1;
   typedef typename iterator_traits<Iterator>::value_type value_type;
   typedef Iterator iterator;

   Iterator& operator() (Iterator& src) const { return src; }
};

template <typename Iterator, typename Target, typename ExpectedFeatures, typename ValueModel>
struct construct_cascaded_iterator<Iterator, Target, ExpectedFeatures, ValueModel,
                                   typename enable_if<void, (isomorphic_types<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                             explicitly_convertible_to<typename iterator_traits<Iterator>::value_type, Target>::value)>::type>
{
   typedef void** enabled;
   static const int depth=1;
   static const int convertible_to=2;
   typedef typename iterator_traits<Iterator>::value_type value_type;
   typedef unary_transform_iterator<Iterator, conv<value_type, Target> > iterator;

   iterator operator() (const Iterator& src) const { return src; }
};

template <typename Iterator, typename Target, typename ExpectedFeatures>
struct construct_cascaded_iterator<Iterator, Target, ExpectedFeatures, is_container,
                                   typename enable_if<void, (!isomorphic_types<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                             construct_cascaded_iterator<typename iterator_traits<Iterator>::value_type::iterator, Target>::convertible_to==1)>::type> :
   construct_cascaded_iterator<typename iterator_traits<Iterator>::value_type::iterator, Target>
{
   typedef construct_cascaded_iterator<typename iterator_traits<Iterator>::value_type::iterator, Target> super;
   static const int depth=super::depth+1;
   typedef cascaded_iterator<Iterator, typename mix_features<ExpectedFeatures, end_sensitive>::type, depth> iterator;

   iterator operator() (const Iterator& src) const { return src; }
};

template <typename Iterator, typename Target, typename ExpectedFeatures>
struct construct_cascaded_iterator<Iterator, Target, ExpectedFeatures, is_container,
                                   typename enable_if<void, (!isomorphic_types<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                             construct_cascaded_iterator<typename iterator_traits<Iterator>::value_type::iterator, Target>::convertible_to==2)>::type> :
   construct_cascaded_iterator<typename iterator_traits<Iterator>::value_type::iterator, Target>
{
   typedef construct_cascaded_iterator<typename iterator_traits<Iterator>::value_type::iterator, Target> super;
   static const int depth=super::depth+1;
   typedef cascaded_iterator<Iterator, typename mix_features<ExpectedFeatures, end_sensitive>::type, depth> inner;
   typedef unary_transform_iterator<inner, conv<typename super::value_type, Target> > iterator;

   iterator operator() (const Iterator& src) const { return inner(src); }
};

} // end namespace pm

namespace polymake {
   using pm::cascade;
   using pm::make_cascade_iterator;
}

#endif // POLYMAKE_CASCADED_CONTAINER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
