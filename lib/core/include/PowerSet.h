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

#ifndef POLYMAKE_POWERSET_H
#define POLYMAKE_POWERSET_H

#include "polymake/vector"
#include "polymake/Set.h"
#include "polymake/SelectedSubset.h"
#if POLYMAKE_DEBUG
# include <sstream>
#endif

namespace pm {

template <typename ContainerRef>
class Subsets_of_1
   : public modified_container_impl< Subsets_of_1<ContainerRef>,
                                     list( Container< ContainerRef >,
                                           Operation< operations::construct_unary2<
                                                         SingleElementSetCmp,
                                                         typename generic_of_subsets< Subsets_of_1<ContainerRef>,
                                                                                      typename deref<ContainerRef>::type >::subset_element_comparator
                                                      > > ) >,
     public generic_of_subsets< Subsets_of_1<ContainerRef>, typename deref<ContainerRef>::type > {
   typedef modified_container_impl<Subsets_of_1> _super;
protected:
   typedef alias<typename attrib<ContainerRef>::plus_const> alias_type;
   typedef typename alias_type::arg_type arg_type;
   alias_type base;
public:
   Subsets_of_1(arg_type base_arg) : base(base_arg) {}

   const typename _super::container& get_container() const { return *base; }
};

template <typename ContainerRef>
struct spec_object_traits< Subsets_of_1<ContainerRef> >
  : spec_object_traits<is_container> {
  static const bool is_temporary=true, is_always_const=true;
};

template <typename Container> inline
const Subsets_of_1<const Container&>
all_subsets_of_1(const Container& c) { return c; }

template <typename SkipIterator,
          bool _from_std=derived_from_instance<SkipIterator, std::reverse_iterator>::value>
class skip_predicate {
public:
   typedef void argument_type;
   typedef bool result_type;

   skip_predicate(const SkipIterator& skip_arg) : skip(skip_arg) {}

   template <typename Iterator>
   bool operator() (const Iterator& it) const { return it!=skip; }
private:
   SkipIterator skip;
};

template <typename SkipIterator>
class skip_predicate<SkipIterator, true> {
public:
   typedef typename SkipIterator::iterator_type BaseIterator;
   typedef void argument_type;
   typedef bool result_type;

   skip_predicate(const SkipIterator& skip_arg)
      : skip(skip_arg), skip_base(skip.base())
   {
      --skip_base;
   }

   bool operator() (const SkipIterator& it) const { return it!=skip; }
   bool operator() (const BaseIterator& it) const { return it!=skip_base; }

   typedef typename if_else< identical<BaseIterator, typename iterator_traits<BaseIterator>::derivable_type>::value,
                             BaseIterator, const typename iterator_traits<BaseIterator>::derivable_type>::type
      alt_arg_type;
   bool operator() (alt_arg_type& it) const { return it!=skip_base; }
private:
   SkipIterator skip;
   BaseIterator skip_base;
};

template <typename ContainerRef, typename SkipIterator> class Subsets_less_1_iterator;

template <typename ContainerRef, typename SkipIterator,
          typename Category=typename least_derived< cons<typename container_traits<ContainerRef>::category,
                                                         bidirectional_iterator_tag> >::type>
class Subset_less_1
   : public generic_of_subset< Subset_less_1<ContainerRef,SkipIterator>, typename deref<ContainerRef>::type> {
   friend class Subsets_less_1_iterator<ContainerRef, SkipIterator>;
protected:
   typedef alias<typename attrib<ContainerRef>::plus_const> alias_type;
   alias_type base;
   SkipIterator skip;

public:
   typedef typename deref<ContainerRef>::type::value_type value_type;
   typedef typename container_traits<ContainerRef>::const_reference const_reference;
   typedef const_reference reference;

   Subset_less_1(const alias_type& base_arg, const SkipIterator& skip_arg)
      : base(base_arg), skip(skip_arg) {}

   typedef unary_predicate_selector<typename ensure_features<typename deref<ContainerRef>::type, end_sensitive>::const_iterator,
                                    skip_predicate<SkipIterator> >
      const_iterator;
   typedef const_iterator iterator;

   iterator begin() const
   {
      return iterator(ensure(*base, (end_sensitive*)0).begin(), skip);
   }
   iterator end() const
   {
      return iterator(ensure(*base, (end_sensitive*)0).end(), skip);
   }
   reference front() const
   {
      typename container_traits<ContainerRef>::const_iterator b=base->begin();
      if (b==skip) ++b;
      return *b;
   }

   int size() const { return base->size()-1; }
   bool empty() const { return base->empty() || base->size()<=1; }

   typename iterator_traits<SkipIterator>::reference skipped_element() const { return *skip; }
};

template <typename ContainerRef, typename SkipIterator>
class Subset_less_1<ContainerRef, SkipIterator, bidirectional_iterator_tag>
   : public Subset_less_1<ContainerRef, SkipIterator, forward_iterator_tag> {
   typedef Subset_less_1<ContainerRef, SkipIterator, forward_iterator_tag> _super;
   friend class Subsets_less_1_iterator<ContainerRef, SkipIterator>;
public:
   Subset_less_1(const typename _super::alias_type& base_arg, const SkipIterator& skip_arg)
      : _super(base_arg, skip_arg) {}

   typedef unary_predicate_selector<typename ensure_features<typename deref<ContainerRef>::type, end_sensitive>::const_reverse_iterator,
                                    skip_predicate<SkipIterator> >
      const_reverse_iterator;
   typedef const_reverse_iterator reverse_iterator;

   reverse_iterator rbegin() const
   {
      return reverse_iterator(ensure(*this->base, (end_sensitive*)0).rbegin(), this->skip);
   }
   reverse_iterator rend() const
   {
      return reverse_iterator(ensure(*this->base, (end_sensitive*)0).rend(), this->skip);
   }
};

template <typename ContainerRef, typename SkipIterator>
class Subsets_less_1_iterator {
public:
   typedef forward_iterator_tag iterator_category;
   typedef Subset_less_1<ContainerRef, SkipIterator> value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef Subsets_less_1_iterator iterator;
   typedef Subsets_less_1_iterator const_iterator;

   Subsets_less_1_iterator(const typename value_type::alias_type& base_arg, const SkipIterator& skip_arg)
      : value(base_arg, skip_arg) {}

   reference operator* () const { return value; }
   pointer operator-> () const { return &value; }

   iterator& operator++ () { ++value.skip; return *this; }
   const iterator operator++ (int) { iterator copy=*this; operator++(); return copy; }

   bool operator== (const iterator& it) const { return value.skip==it.value.skip; }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   bool at_end() const { return value.skip.at_end(); }
protected:
   value_type value;
};

template <typename ContainerRef>
class Subsets_less_1
   : public generic_of_subsets<Subsets_less_1<ContainerRef>, typename deref<ContainerRef>::type> {
protected:
   typedef alias<typename attrib<ContainerRef>::plus_const> alias_type;
   typedef typename alias_type::arg_type arg_type;
   alias_type base;

   typedef cons<_reversed, end_sensitive> req_features;
   typedef typename ensure_features<typename deref<ContainerRef>::type, req_features>::const_iterator skip_iterator;
public:
   typedef Subsets_less_1_iterator<ContainerRef,skip_iterator> iterator;
   typedef typename iterator::value_type value_type;
   typedef typename iterator::reference reference;
   typedef iterator const_iterator;
   typedef reference const_reference;

   Subsets_less_1(arg_type base_arg) : base(base_arg) {}

   int size() const { return base->size(); }
   bool empty() const { return base->empty(); }

   iterator begin() const
   {
      return iterator(base, ensure(*base, (req_features*)0).begin());
   }
   iterator end() const
   {
      return iterator(base, ensure(*base, (req_features*)0).end());
   }
};

template <typename ContainerRef, typename SkipIterator>
struct check_iterator_feature<Subsets_less_1_iterator<ContainerRef, SkipIterator>, end_sensitive> : True {};

template <typename ContainerRef>
struct spec_object_traits< Subsets_less_1<ContainerRef> >
  : spec_object_traits<is_container> {
  static const bool is_temporary=true, is_always_const=true;
};

template <typename Container> inline
const Subsets_less_1<const Container&>
all_subsets_less_1(const Container& c) { return c; }

template <typename ContainerRef> class Subsets_of_k_iterator;
template <typename ContainerRef> class AllSubsets_iterator;

template <typename BaseContainer>
class PointedSubset
   : public modified_container_impl< PointedSubset<BaseContainer>,
                                     list( Container< std::vector<typename container_traits<BaseContainer>::const_iterator> >,
                                           Operation< BuildUnary<operations::dereference> > ) >,
     public generic_of_subset<PointedSubset<BaseContainer>, BaseContainer> {
   typedef modified_container_impl<PointedSubset> _super;

   template <typename> friend class Subsets_of_k_iterator;
   template <typename> friend class AllSubsets_iterator;
protected:
   typedef typename container_traits<BaseContainer>::const_iterator elem_iterator;
   typedef typename _super::container it_vector;
   shared_object<typename _super::container> ptr;
public:
   const it_vector& get_container() const { return *ptr; }

   PointedSubset() {}

   explicit PointedSubset(int k)
   {
      ptr->reserve(k);
   }

   PointedSubset(const BaseContainer& src, int k)
      : ptr(make_constructor(k, (it_vector*)0))
   {
      elem_iterator e=src.begin();
      for (typename Entire<it_vector>::iterator pp=entire(*ptr);  !pp.at_end(); ++e, ++pp)
         *pp=e;
   }
};

template <typename ContainerRef>
class Subsets_of_k_iterator {
public:
   typedef forward_iterator_tag iterator_category;
   typedef PointedSubset<typename deref<ContainerRef>::type> value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef Subsets_of_k_iterator iterator;
   typedef Subsets_of_k_iterator const_iterator;

   typedef alias<typename attrib<ContainerRef>::plus_const> alias_type;

   Subsets_of_k_iterator() {}

   Subsets_of_k_iterator(const alias_type& base_arg, int k, bool at_end_arg=false)
      : base(base_arg), value(*base,k), e_end(base->end()), _at_end(at_end_arg) {}

   reference operator* () const { return value; }
   pointer operator-> () const { return &value; }

   iterator& operator++()
   {
      typename value_type::elem_iterator stop=e_end;
      for (typename value_type::container::iterator ptr_first=value.ptr->begin(), ptr_last=value.ptr->end(), ptr_i=ptr_last;
           ptr_i != ptr_first; --ptr_i) {
         typename value_type::elem_iterator new_stop=(ptr_i[-1])++;
         if (ptr_i[-1] != stop) {
            for (; ptr_i != ptr_last; ++ptr_i)
               ++(*ptr_i=ptr_i[-1]);
            return *this;
         }
         stop=new_stop;
      }
      _at_end=true;
      return *this;
   }

   const iterator operator++ (int) { iterator copy=*this; operator++(); return copy; }

   bool operator== (const iterator& it) const
   {
      return _at_end==it._at_end && (_at_end || equal(entire(*value.ptr), it.value.ptr->begin()));
   }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   bool at_end() const { return _at_end; }
protected:
   alias_type base;
   value_type value;
   typename value_type::elem_iterator e_end;
   bool _at_end;
};

template <typename ContainerRef>
class Subsets_of_k
   : public generic_of_subsets< Subsets_of_k<ContainerRef>, typename deref<ContainerRef>::type> {
public:
   typedef Subsets_of_k_iterator<ContainerRef> iterator;
   typedef typename iterator::value_type value_type;
   typedef typename iterator::reference reference;
   typedef iterator const_iterator;
   typedef reference const_reference;

   typedef typename iterator::alias_type alias_type;

   Subsets_of_k(typename alias_type::arg_type base_arg, int k_arg)
      : base(base_arg), k(k_arg)
   {
      if (POLYMAKE_DEBUG) {
         if (k<0 || k>base->size())
            throw std::runtime_error("Subsets_of_k - invalid size");
      }
   }

   int size() const
   {
      // n over k
      int n=base->size(), s=1;
      for (int i=0, i_end= k*2<n ? k : n-k; i<i_end; ) {
         s*=n-i; s/=++i;
      }
      return s;
   }

   bool empty() const { return false; }

   iterator begin() const { return iterator(base, k); }
   iterator end() const { return iterator(base, k, true); }
   value_type front() const { return value_type(base, k); }
protected:
   alias_type base;
   int k;
};

template <typename ContainerRef>
class AllSubsets_iterator {
public:
   typedef forward_iterator_tag iterator_category;
   typedef PointedSubset<typename deref<ContainerRef>::type> value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;
   typedef AllSubsets_iterator iterator;
   typedef AllSubsets_iterator const_iterator;

   typedef alias<typename attrib<ContainerRef>::plus_const> alias_type;

   AllSubsets_iterator() {}

   AllSubsets_iterator(const alias_type& base_arg, bool at_end_arg=false)
      : base(base_arg),
        value(base->size()),
        e_next(base->begin()),
        e_end(base->end()),
        _at_end(at_end_arg) {}

   reference operator* () const { return value; }
   pointer operator-> () const { return &value; }

   iterator& operator++()
   {
      if (e_next != e_end) {
         value.ptr->push_back(e_next);
         ++e_next;
      } else {
         if (!value.ptr->empty())       // special precaution for the case of an empty base set
            value.ptr->pop_back();
         if (value.ptr->empty()) {
            _at_end=true;
         } else {
            e_next=++value.ptr->back();
            ++e_next;
         }
      }
      return *this;
   }

   const iterator operator++ (int) { iterator copy=*this; operator++(); return copy; }

   bool operator== (const iterator& it) const
   {
      return _at_end==it._at_end && (_at_end || equal(entire(*value.ptr), it.value.ptr->begin()));
   }
   bool operator!= (const iterator& it) const { return !operator==(it); }

   bool at_end() const { return _at_end; }
protected:
   alias_type base;
   value_type value;
   typename value_type::elem_iterator e_next, e_end;
   bool _at_end;
};

template <typename ContainerRef>
class AllSubsets
   : public generic_of_subsets< AllSubsets<ContainerRef>, typename deref<ContainerRef>::type> {
public:
   typedef AllSubsets_iterator<ContainerRef> iterator;
   typedef typename iterator::value_type value_type;
   typedef typename iterator::reference reference;
   typedef iterator const_iterator;
   typedef reference const_reference;

   typedef typename iterator::alias_type alias_type;

   AllSubsets(typename alias_type::arg_type base_arg)
      : base(base_arg) {}

   int size() const { return 1 << base->size(); }
   bool empty() const { return false; }

   iterator begin() const { return iterator(base); }
   iterator end() const { return iterator(base, true); }
   value_type front() const { return value_type(); }
protected:
   alias_type base;
};

template <typename ContainerRef>
struct check_iterator_feature<Subsets_of_k_iterator<ContainerRef>, end_sensitive> : True {};

template <typename ContainerRef>
struct check_iterator_feature<AllSubsets_iterator<ContainerRef>, end_sensitive> : True {};

template <typename ContainerRef>
struct spec_object_traits< Subsets_of_k<ContainerRef> >
  : spec_object_traits<is_container> {
  static const bool is_temporary=true, is_always_const=true;
};

template <typename ContainerRef>
struct spec_object_traits< AllSubsets<ContainerRef> >
  : spec_object_traits<is_container> {
  static const bool is_temporary=true, is_always_const=true;
};

template <typename Container> inline
const Subsets_of_k<const Container&>
all_subsets_of_k(const Container& c, int k)
{
   return Subsets_of_k<const Container&>(c,k);
}

template <typename Container> inline
const AllSubsets<const Container&>
all_subsets(const Container& c)
{
   return AllSubsets<const Container&>(c);
}

template <typename PowerSet, typename ElementSet>
int insertMax(PowerSet& power_set, const GenericSet<ElementSet>& _element_set)
{
   typename Diligent<const ElementSet&>::type element_set=diligent(_element_set);
   if (element_set.empty()) return -1;
   for (typename Entire<PowerSet>::iterator e=entire(power_set); !e.at_end(); ) {
      int inc=incl(element_set,*e);
      // found a subset containing or equal to set
      if (inc<=0) return inc;
      // found a subset being contained in set
      if (inc==1) power_set.erase(e++);
      else ++e;
   }
   power_set.insert(element_set);
   return 1;
}

template <typename PowerSet, typename ElementSet>
int insertMin(PowerSet& power_set, const GenericSet<ElementSet>& _element_set)
{
   typename Diligent<const ElementSet&>::type element_set=diligent(_element_set);
   if (element_set.empty()) return -1;
   for (typename Entire<PowerSet>::iterator e=entire(power_set); !e.at_end(); ) {
      int inc=incl(*e,element_set);
      // found a subset containing or equal to set
      if (inc<=0) return inc;
      // found a subset being contained in set
      if (inc==1) power_set.erase(e++);
      else ++e;
   }
   power_set.insert(element_set);
   return 1;
}

template <typename E, typename Comparator>
class PowerSet : public Set< Set<E,Comparator> > {
   typedef Set< Set<E,Comparator> > super;
public:
   /// Create as empty.
   PowerSet() {}

   template <typename Top>
   PowerSet(const GenericSet<Top, Set<E,Comparator> >& S) : super(S) {}

   template <typename Top>
   PowerSet& operator= (const GenericSet<Top, Set<E,Comparator> >& S)
   {
      super::operator=(S);
      return *this;
   }

   /** Reads subsets from an input sequence.
       They must be sorted in the lexicographical order.
   */
   template <typename Iterator>
   PowerSet(Iterator first, Iterator last)
   {
      std::copy(first, last, std::back_inserter(*this));
   }

   template <typename InputIterator>
   explicit PowerSet(InputIterator src, typename enable_if_iterator<InputIterator,end_sensitive>::type=0)
   {
      copy(src, std::back_inserter(*this));
   }

   /** Adds an independent subset.
       The new subset will not be added if there is already another subset that includes the given one.
       All subsets in the PowerSet that are included in the new one are removed.
       @returnval  1 new subset inserted, smaller subsets possibly deleted
       @returnval  0 new subset not inserted, because the same subset was already there
       @returnval -1 new subset not inserted, because there already is a bigger one
   */
   template <typename Top>
   int insertMax(const GenericSet<Top,E,Comparator>& s)
   {
      return pm::insertMax(*this,s);
   }

   /** Adds an independent subset.
       The new subset will not be added if there is already another subset that is contained in the given one.
       All subsets in the PowerSet that contain in the new one are removed.
       @returnval  1 new subset inserted, larger subsets possibly deleted
       @returnval  0 new subset not inserted, because the same subset was already there
       @returnval -1 new subset not inserted, because there already is a smaller one
   */
   template <typename Top>
   int insertMin(const GenericSet<Top,E,Comparator>& s)
   {
      return pm::insertMin(*this,s);
   }
#if POLYMAKE_DEBUG
   void check(const char* label) const
   {
      super::check(label);
      for (typename super::const_iterator e=this->begin(); !e.at_end(); ++e) {
         std::ostringstream elabel;
         wrap(elabel) << label << '[' << *e << ']';
         e->check(elabel.str().c_str());
      }
   }
#endif
};

// Gather all independent intersections of subsets from the given PowerSet.
template <typename Iterator>
PowerSet<typename iterator_traits<Iterator>::value_type::element_type,
         typename iterator_traits<Iterator>::value_type::element_comparator>
ridges(Iterator set)
{
   typedef typename iterator_traits<Iterator>::value_type::element_type element_type;
   typedef typename iterator_traits<Iterator>::value_type::element_comparator element_comparator;
   PowerSet<element_type, element_comparator> R;
   for (; !set.at_end(); ++set) {
      Iterator set2=set;
      for (++set2; !set2.at_end(); ++set2) {
         Set<int> ridge=(*set) * (*set2);
         R.insertMax(ridge);
      }
   }
   return R;
}

template <typename E, typename Comparator, typename Permutation> inline
PowerSet<E,Comparator> permuted(const PowerSet<E,Comparator>& s, const Permutation& perm)
{
   PowerSet<E,Comparator> result;
   for (typename Entire< PowerSet<E,Comparator> >::const_iterator it=entire(s);  !it.at_end();  ++it)
      result += permuted(*it,perm);
   return result;
}

template <typename E, typename Comparator, typename Permutation> inline
PowerSet<E,Comparator> permuted_inv(const PowerSet<E,Comparator>& s, const Permutation& perm)
{
   PowerSet<E,Comparator> result;
   for (typename Entire< PowerSet<E,Comparator> >::const_iterator it=entire(s);  !it.at_end();  ++it)
      result += permuted_inv(*it,perm);
   return result;
}

} // end namespace pm

namespace polymake {
   using pm::PowerSet;
   using pm::ridges;
   using pm::Subsets_of_1;
   using pm::all_subsets_of_1;
   using pm::Subsets_less_1;
   using pm::all_subsets_less_1;
   using pm::Subsets_of_k;
   using pm::all_subsets_of_k;
   using pm::AllSubsets;
   using pm::all_subsets;
}

#endif // POLYMAKE_POWERSET_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
