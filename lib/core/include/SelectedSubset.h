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

#ifndef POLYMAKE_SELECTED_SUBSET_H
#define POLYMAKE_SELECTED_SUBSET_H

#include "polymake/Series.h"
#include "polymake/internal/modified_containers.h"
#include "polymake/internal/comparators_ops.h"

namespace pm {

/** Iterator modifier skipping some elements.
 *  Only elements evaluated by the given functor to TRUE are shown.
 *  @tparam Iterator iterator over the source sequence.
 *  @tparam Predicate unary boolean functor.
 */
template <typename Iterator, typename Predicate>
class unary_predicate_selector : public Iterator {
protected:
   typedef Iterator super;
   typedef unary_helper<Iterator, Predicate> helper;
   typename helper::operation pred;

   void valid_position()
   {
      while (!this->at_end() && !pred(*helper::get(*this))) super::operator++();
   }

   template <typename,typename> friend class unary_predicate_selector;
public:
   typedef typename least_derived< cons<typename iterator_traits<Iterator>::iterator_category,
                                        bidirectional_iterator_tag> >::type
      iterator_category;
   typedef unary_predicate_selector<typename iterator_traits<Iterator>::iterator, Predicate> iterator;
   typedef unary_predicate_selector<typename iterator_traits<Iterator>::const_iterator, Predicate> const_iterator;

   unary_predicate_selector() {}

   template <typename Predicate2>
   unary_predicate_selector(const unary_predicate_selector<typename iterator_traits<Iterator>::iterator, Predicate2>& it)
      : super(it),
        pred(helper::create(it.pred)) {}

   template <typename Predicate2>
   explicit unary_predicate_selector(const unary_predicate_selector<typename iterator_reversed<Iterator>::type, Predicate2>& it)
      : super(iterator_reversed<Iterator>::make(it)),
        pred(helper::create(it.pred)) {}

   unary_predicate_selector(const Iterator& cur_arg, const Predicate& pred_arg=Predicate(), bool at_valid_position=false)
      : super(cur_arg),
        pred(helper::create(pred_arg))
   {
      if (!at_valid_position) valid_position();
   }

   unary_predicate_selector& operator++ ()
   {
      super::operator++();
      valid_position();
      return *this;
   }

   const unary_predicate_selector operator++ (int)
   {
      unary_predicate_selector copy=*this;  operator++();  return copy;
   }

   void rewind()
   {
      typedef typename enable_if<super, check_iterator_feature<super, rewindable>::value>::type error_if_unimplemented __attribute__((unused));
      super::rewind();
      valid_position();
   }

   // it's the applications' responsibility not to call this at the first position
   unary_predicate_selector& operator-- ()
   {
      typedef typename enable_if<super, iterator_traits<super>::is_bidirectional>::type error_if_unimplemented __attribute__((unused));
      do
         error_if_unimplemented::operator--();
      while (!pred(*helper::get(*this)));
      return *this;
   }

   const unary_predicate_selector operator-- (int)
   {
      unary_predicate_selector copy=*this;  operator--();  return copy;
   }

private:
   // make them undefined for the case of random_access Iterator
   void operator+=(int);
   void operator-=(int);
   void operator+(int);
   void operator-(int);
   void operator[](int);
};

template <typename Iterator, typename Predicate, typename Feature>
struct check_iterator_feature<unary_predicate_selector<Iterator, Predicate>, Feature>
   : check_iterator_feature<Iterator, Feature> {};

struct unary_predicate_selector_constructor {
   template <typename Iterator, typename Predicate, typename ExpectedFeatures>
   struct defs {
      typedef typename mix_features<ExpectedFeatures, end_sensitive>::type
         needed_features;
      typedef unary_predicate_selector<Iterator, Predicate> iterator;
   };
};

/// Convenience function creating an iterator with element selection.
template <typename Iterator, typename Predicate> inline
unary_predicate_selector<Iterator, Predicate>
make_unary_predicate_selector(Iterator it, const Predicate& pred)
{
   return unary_predicate_selector<Iterator, Predicate>(it, pred);
}


/** Iterator modifier truncating the trailing part of the sequence.
 *  Iterating is stopped at the first element evaluated to FALSE by the given functor.
 *  @tparam Iterator iterator over the source sequence.
 *  @tparam Predicate unary boolean functor.
 */
template <typename Iterator, typename Predicate>
class input_truncator : public Iterator {
protected:
   typedef Iterator super;
   typedef unary_helper<Iterator,Predicate> helper;
   typename helper::operation pred;

   template <typename,typename> friend class input_truncator;
public:
   typedef forward_iterator_tag iterator_category;
   typedef input_truncator<typename iterator_traits<Iterator>::iterator, Predicate> iterator;
   typedef input_truncator<typename iterator_traits<Iterator>::const_iterator, Predicate> const_iterator;

   input_truncator() {}

   template <typename Predicate2>
   input_truncator(const input_truncator<typename iterator_traits<Iterator>::iterator, Predicate2>& it)
      : super(it),
        pred(helper::create(it.pred)) {}

   input_truncator(const Iterator& cur_arg, const Predicate& pred_arg=Predicate())
      : super(cur_arg),
        pred(helper::create(pred_arg)) {}

   input_truncator& operator++ ()
   {
      super::operator++();
      return *this;
   }
   const input_truncator operator++ (int)
   {
      input_truncator copy=*this; operator++(); return copy;
   }

   bool at_end() const
   {
      return super::at_end() || !pred(*helper::get(*this));
   }

   bool operator== (const iterator& it) const
   {
      return at_end() ? it.at_end() : static_cast<const Iterator&>(*this)==it;
   }
   bool operator== (typename assign_const<const_iterator, !identical<iterator,const_iterator>::value>::type& it) const
   {
      return at_end() ? it.at_end() : static_cast<const Iterator&>(*this)==it;
   }
   bool operator!= (const iterator& it) const
   {
      return !operator==(it);
   }
   bool operator!= (typename assign_const<const_iterator, !identical<iterator,const_iterator>::value>::type& it) const
   {
      return !operator==(it);
   }
private:
   // make them undefined for the case of bidirectional or random_access Iterator
   void operator--();
   void operator--(int);
   void operator+=(int);
   void operator-=(int);
   void operator+(int);
   void operator-(int);
   void operator[](int);
};

template <typename Iterator, typename Predicate, typename Feature>
struct check_iterator_feature<input_truncator<Iterator, Predicate>, Feature> :
   check_iterator_feature<Iterator, Feature> {};

struct input_truncator_constructor {
   template <typename Iterator, typename Predicate, typename ExpectedFeatures>
   struct defs : public unary_predicate_selector_constructor::defs<Iterator, Predicate, ExpectedFeatures> {
      typedef input_truncator<Iterator, Predicate> iterator;
   };
};


/** Iterator modifier contracting subsequences of equivalent elements to single representatives.
 *  @tparam Iterator iterator over the source sequence.
 *  @tparam Predicate binary boolean functor evaluating two equivalent elements to TRUE.
 */
template <typename Iterator, typename Predicate>
class range_contractor : public Iterator {
   template <typename,typename> friend class range_contractor;
protected:
   typedef Iterator super;
   typedef binary_op_builder<Predicate, Iterator, Iterator> op_helper;
   typename op_helper::operation pred;
public:
   typedef forward_iterator_tag iterator_category;
   typedef range_contractor<typename iterator_traits<Iterator>::iterator, Predicate> iterator;
   typedef range_contractor<typename iterator_traits<Iterator>::const_iterator, Predicate> const_iterator;

   range_contractor() {}

   template <typename Predicate2>
   range_contractor(const range_contractor<typename iterator_traits<Iterator>::iterator, Predicate2>& it) :
      super(it),
      pred(op_helper::create(it.pred)) {}

   range_contractor(const Iterator& start, const Predicate& pred_arg=Predicate()) :
      super(start),
      pred(op_helper::create(pred_arg)) {}

   range_contractor& operator++ ()
   {
      const typename super::value_type shown = *(*this);
      do super::operator++();
      while (!this->at_end() && pred(shown, *(*this)));
      return *this;
   }

   const range_contractor operator++ (int)
   {
      range_contractor copy=*this;  operator++();  return copy;
   }

   void rewind()
   {
      typedef typename enable_if<super, check_iterator_feature<super, rewindable>::value>::type error_if_unimplemented __attribute__((unused));
      super::rewind();
   }

private:
   // make them undefined for the case of bidirectional or random_access Iterator
   void operator--();
   void operator+=(int);
   void operator-=(int);
   void operator+(int);
   void operator-(int);
   void operator[](int);
};

template <typename Iterator, typename Predicate, typename Feature>
struct check_iterator_feature<range_contractor<Iterator, Predicate>, Feature>
   : check_iterator_feature<Iterator, Feature> {};

struct range_contractor_constructor {
   template <typename Iterator, typename Predicate, typename ExpectedFeatures>
   struct defs {
      typedef typename mix_features<ExpectedFeatures, end_sensitive>::type
         needed_features;
      typedef range_contractor<Iterator, Predicate> iterator;
   };
};

template <typename Iterator, typename Predicate> inline
range_contractor<Iterator, Predicate>
make_range_contractor(Iterator it, const Predicate& pred)
{
   return range_contractor<Iterator, Predicate>(it, pred);
}

template <typename Iterator> inline
range_contractor<Iterator, BuildBinary<operations::eq> >
make_equal_range_contractor(Iterator it)
{
   return range_contractor<Iterator, BuildBinary<operations::eq> >(it);
}


template <typename Iterator, typename FoldingOperation>
class range_folder : public Iterator {
   template <typename,typename> friend class range_folder;
protected:
   typedef Iterator super;
   typedef unary_helper<Iterator, FoldingOperation> helper;

   typename helper::operation op;
   bool _at_end;

   void valid_position()
   {
      op.reset(*helper::get(static_cast<const super&>(*this)));

      while (!(++static_cast<super&>(*this)).at_end() &&
             op(*helper::get(static_cast<const super&>(*this)))) ;
   }

public:
   typedef forward_iterator_tag iterator_category;
   typedef typename helper::operation::value_type value_type;
   typedef typename helper::operation::reference reference;
   typedef range_folder<typename iterator_traits<Iterator>::iterator, FoldingOperation> iterator;
   typedef range_folder<typename iterator_traits<Iterator>::const_iterator, FoldingOperation> const_iterator;

   range_folder() {}

   range_folder(const iterator& it) :
      super(it),
      op(helper::create(it.op)),
      _at_end(it._at_end) {}

   range_folder(const Iterator& start, const FoldingOperation& op_arg=FoldingOperation()) :
      super(start),
      op(helper::create(op_arg)),
      _at_end(super::at_end())
   {
      if (!_at_end) valid_position();
   }

   reference operator* () const { return op.get(); }
   int index() const { return op.get_index(); }

   range_folder& operator++ ()
   {
      if (super::at_end())
         _at_end=true;
      else
         valid_position();
      return *this;
   }

   const range_folder operator++ (int) { range_folder copy(*this);  operator++();  return copy; }

   bool at_end() const { return _at_end; }

   void rewind()
   {
      super::rewind();
      if (!(_at_end=super::at_end())) valid_position();
   }

private:
   // make them undefined for the case of bidirectional or random_access Iterator
   void operator--();
   void operator--(int);
   void operator+=(int);
   void operator-=(int);
   void operator+(int);
   void operator-(int);
   void operator[](int);
};

template <typename Iterator, typename FoldingOperation, typename Feature>
struct check_iterator_feature<range_folder<Iterator, FoldingOperation>, Feature> :
   check_iterator_feature<Iterator, Feature> {};

struct range_folder_constructor {
   template <typename Iterator, typename FoldingOperation, typename ExpectedFeatures>
   struct defs {
      typedef typename mix_features<ExpectedFeatures, end_sensitive>::type
         needed_features;
      typedef range_folder<Iterator, FoldingOperation> iterator;
   };
};

template <typename Iterator, typename FoldingOperation> inline
range_folder<Iterator, FoldingOperation>
make_range_folder(Iterator it, const FoldingOperation& op)
{
   return range_folder<Iterator, FoldingOperation>(it, op);
}


template <typename ContainerRef, typename Predicate>
class SelectedSubset
   : public modified_container_base<ContainerRef, Predicate>,
     public modified_container_impl< SelectedSubset<ContainerRef, Predicate>,
                                     list( Container< ContainerRef >,
                                           Operation< Predicate >,
                                           IteratorConstructor< unary_predicate_selector_constructor > ) >,
     public generic_of_subset< SelectedSubset<ContainerRef, Predicate>,
                               typename deref<ContainerRef>::type> {
   typedef modified_container_base<ContainerRef, Predicate> _base;
   typedef modified_container_impl<SelectedSubset> _super;
public:
   typedef typename least_derived< cons<bidirectional_iterator_tag, typename _super::container_category> >::type
      container_category;

   SelectedSubset(typename _base::arg_type src_arg, const Predicate& pred_arg=Predicate()) :
      _base(src_arg, pred_arg) {}

   using _base::get_operation;
};

template <typename ContainerRef, typename Predicate>
class TruncatedContainer
   : public modified_container_base<ContainerRef, Predicate>,
     public modified_container_impl< TruncatedContainer<ContainerRef, Predicate>,
                                     list( Container< ContainerRef >,
                                           Operation< Predicate >,
                                           IteratorConstructor< input_truncator_constructor > ) >,
     public generic_of_subset< TruncatedContainer<ContainerRef, Predicate>,
                               typename deref<ContainerRef>::type> {
   typedef modified_container_base<ContainerRef, Predicate> _base;
   typedef modified_container_impl<TruncatedContainer> _super;
public:
   typedef forward_iterator_tag container_category;

   TruncatedContainer(typename _base::arg_type src_arg, const Predicate& pred_arg=Predicate()) :
      _base(src_arg, pred_arg) {}

   using _base::get_operation;

   typename _super::reference front()
   {
      return this->get_container().front();
   }
   typename _super::const_reference front() const
   {
      return this->get_container().front();
   }
};


template <typename ContainerRef, typename Predicate>
class ContractedRanges
   : public modified_container_base<ContainerRef, Predicate>,
     public modified_container_impl< ContractedRanges<ContainerRef, Predicate>,
                                     list( Container< ContainerRef >,
                                           Operation< Predicate >,
                                           IteratorConstructor< range_contractor_constructor > ) > {
   typedef modified_container_base<ContainerRef, Predicate> _base;
public:
   typedef forward_iterator_tag container_category;

   ContractedRanges(typename _base::arg_type src_arg, const Predicate& pred_arg=Predicate()) :
      _base(src_arg, pred_arg) {}

   using _base::get_operation;
};


template <typename ContainerRef, typename FoldingOperation>
class FoldedRanges
   : public modified_container_base<ContainerRef, FoldingOperation>,
     public modified_container_impl< FoldedRanges<ContainerRef, FoldingOperation>,
                                     list( Container< ContainerRef >,
                                           Operation< FoldingOperation >,
                                           IteratorConstructor< range_folder_constructor > ) > {
   typedef modified_container_base<ContainerRef, FoldingOperation> _base;
public:
   typedef forward_iterator_tag container_category;

   FoldedRanges(typename _base::arg_type src_arg, const FoldingOperation& op_arg=FoldingOperation()) :
      _base(src_arg, op_arg) {}

   using _base::get_operation;
};


template <typename ContainerRef, typename Predicate>
struct spec_object_traits< SelectedSubset<ContainerRef, Predicate> > :
   spec_object_traits<is_container> {
   static const bool is_temporary=true,
                     is_always_const=effectively_const<ContainerRef>::value;
};

template <typename ContainerRef, typename Predicate>
struct spec_object_traits< TruncatedContainer<ContainerRef, Predicate> > :
   spec_object_traits<is_container> {
   static const bool is_temporary=true,
                     is_always_const=effectively_const<ContainerRef>::value;
};

template <typename ContainerRef, typename Predicate>
struct spec_object_traits< ContractedRanges<ContainerRef, Predicate> > :
   spec_object_traits<is_container> {
   static const bool is_temporary=true,
                     is_always_const=true;
};

template <typename ContainerRef, typename FoldingOperation>
struct spec_object_traits< FoldedRanges<ContainerRef, FoldingOperation> > :
   spec_object_traits<is_container> {
   static const bool is_temporary=true,
                     is_always_const=true;
};


class index_truncator {
protected:
   int last;
public:
   index_truncator(int l=-1) : last(l) {}

   typedef void argument_type;
   typedef bool result_type;

   template <typename Iterator>
   bool operator() (const Iterator& it) const
   {
      return it.index() <= last;
   }
};


class equal_index_folder {
public:
   typedef void argument_type;
   typedef int value_type;
   typedef const int& reference;

   template <typename Iterator>
   void reset(const Iterator& it)
   {
      cnt=1;  index=it.index();
   }

   template <typename Iterator>
   bool operator() (const Iterator& it)
   {
      if (it.index()==index) {
         ++cnt;
         return true;
      } else {
         return false;
      }
   }

   const int& get() const { return cnt; }
   int get_index() const { return index; }

private:
   int index;
   int cnt;
};


template <typename Container, typename Predicate> inline
SelectedSubset<Container&, Predicate>
attach_selector(Container& c, const Predicate& pred)
{
   return SelectedSubset<Container&, Predicate>(c, pred);
}

template <typename Container, typename Predicate> inline
SelectedSubset<const Container&, Predicate>
attach_selector(const Container& c, const Predicate& pred)
{
   return SelectedSubset<const Container&, Predicate>(c, pred);
}

template <typename Container, typename Predicate> inline
TruncatedContainer<Container&, Predicate>
attach_truncator(Container& c, const Predicate& pred)
{
   return TruncatedContainer<Container&, Predicate>(c, pred);
}

template <typename Container, typename Predicate> inline
TruncatedContainer<const Container&, Predicate>
attach_truncator(const Container& c, const Predicate& pred)
{
   return TruncatedContainer<const Container&, Predicate>(c, pred);
}

template <typename Container, typename Predicate> inline
ContractedRanges<const Container&, Predicate>
contract_ranges(const Container& c, const Predicate& pred)
{
   return ContractedRanges<const Container&, Predicate>(c, pred);
}

template <typename Container> inline
ContractedRanges<const Container&, BuildBinary<operations::eq> >
contract_equal_ranges(const Container& c)
{
   return ContractedRanges<const Container&, BuildBinary<operations::eq> >(c);
}

template <typename Container, typename Operation> inline
FoldedRanges<const Container&, Operation>
fold_ranges(const Container& c, const Operation& op)
{
   return FoldedRanges<const Container&, Operation>(c, op);
}


/** Selecting output iterator

    This is a combination of an output iterator (called below `basis iterator') and a predicate.

    When a data item is assigned to the output iterator, it is first evaluated by the predicate object.
    Only items mapped to TRUE are passed through to the assignment method of the basis iterator.
*/
template <typename Iterator, typename Predicate>
class output_predicate_selector : public iterator_traits<Iterator>::derivable_type {
protected:
   Predicate pred;

   typedef typename iterator_traits<Iterator>::derivable_type super;
public:
   typedef output_iterator_tag iterator_category;
   typedef typename iterator_traits<Iterator>::value_type value_type;

   output_predicate_selector() {}

   output_predicate_selector(const Iterator& cur_arg, const Predicate& pred_arg=Predicate())
      : super(cur_arg), pred(pred_arg) {}

   output_predicate_selector& operator= (typename function_argument<value_type>::type arg)
   {
      if (pred(arg))
         *static_cast<super*>(this)=arg;
      return *this;
   }

   template <class Arg>
   output_predicate_selector& operator= (const Arg& arg)
   {
      if (pred(arg))
         *static_cast<super*>(this)=arg;
      return *this;
   }

   output_predicate_selector& operator* () { return *this; }
   output_predicate_selector& operator++ () { return *this; }
   output_predicate_selector& operator++ (int) { return *this; }
};

template <typename Iterator, typename Predicate> inline
output_predicate_selector<Iterator,Predicate>
make_output_predicate_selector(Iterator it, Predicate pred)
{
   return output_predicate_selector<Iterator,Predicate>(it,pred);
}

struct output_predicate_selector_constructor {
   template <typename Iterator, typename Predicate, typename ExpectedFeatures>
   struct defs {
      typedef ExpectedFeatures needed_features;
      typedef output_predicate_selector<Iterator,Predicate> iterator;
   };
};

template <typename IteratorPair, typename Predicate>
class binary_predicate_selector : public IteratorPair {
protected:
   typedef IteratorPair super;
   typedef binary_helper<IteratorPair,Predicate> helper;
   typename helper::operation pred;

   void valid_position()
   {
      while (!this->at_end() && !pred(*helper::get1(*this), *helper::get2(this->second)))
         super::operator++();
   }

   template <typename,typename> friend class binary_predicate_selector;
public:
   typedef typename least_derived< cons<typename IteratorPair::iterator_category,
                                        bidirectional_iterator_tag> >::type
      iterator_category;
   typedef binary_predicate_selector<typename iterator_traits<IteratorPair>::iterator, Predicate> iterator;
   typedef binary_predicate_selector<typename iterator_traits<IteratorPair>::const_iterator, Predicate> const_iterator;

   binary_predicate_selector() {}

   template <typename Predicate2>
   binary_predicate_selector(const binary_predicate_selector<typename iterator_traits<IteratorPair>::iterator, Predicate2>& it)
      : super(it),
        pred(helper::create(it.pred)) {}

   binary_predicate_selector(const IteratorPair& cur_arg, const Predicate& pred_arg=Predicate(), bool at_valid_position=false)
      : super(cur_arg),
        pred(helper::create(pred_arg))
   {
      if (!at_valid_position) valid_position();
   }

   binary_predicate_selector(const typename IteratorPair::first_type& first_arg,
                             const typename IteratorPair::second_type& second_arg,
                             const Predicate& pred_arg=Predicate(), bool at_valid_position=false)
      : super(first_arg,second_arg),
        pred(helper::create(pred_arg))
   {
      if (!at_valid_position) valid_position();
   }

   binary_predicate_selector(typename alt_constructor<typename IteratorPair::first_type>::arg_type& first_arg,
                             typename alt_constructor<typename IteratorPair::second_type>::arg_type& second_arg,
                             const Predicate& pred_arg=Predicate(), bool at_valid_position=false)
      : super(first_arg,second_arg),
        pred(helper::create(pred_arg))
   {
      if (!at_valid_position) valid_position();
   }

   binary_predicate_selector& operator++ ()
   {
      super::operator++();
      valid_position();
      return *this;
   }
   const binary_predicate_selector operator++ (int)
   {
      binary_predicate_selector copy=*this; operator++(); return copy;
   }

   binary_predicate_selector& operator-- ()
   {
      typedef typename enable_if<super, iterator_traits<super>::is_bidirectional>::type error_if_unimplemented __attribute__((unused));
      do
         super::operator--();
      while (!pred(*helper::get1(*this), *helper::get2(this->second)));
      return *this;
   }
   const binary_predicate_selector operator-- (int)
   {
      binary_predicate_selector copy=*this; operator--(); return copy;
   }

   void rewind()
   {
      typedef typename enable_if<super, check_iterator_feature<super, rewindable>::value>::type error_if_unimplemented __attribute__((unused));
      super::rewind();
      valid_position();
   }
private:
   void operator+=(int);
   void operator-=(int);
   void operator+(int);
   void operator-(int);
   void operator[](int);
};

template <typename IteratorPair, typename Predicate, typename Feature>
struct check_iterator_feature<binary_predicate_selector<IteratorPair, Predicate>, Feature>
   : check_iterator_feature<IteratorPair, Feature> {};

template <typename Iterator1, typename Iterator2, typename Predicate> inline
binary_predicate_selector<iterator_pair<Iterator1, Iterator2>, Predicate>
make_binary_predicate_selector(Iterator1 first, Iterator2 second, const Predicate& pred)
{
   return binary_predicate_selector<iterator_pair<Iterator1, Iterator2>, Predicate>(first,second,pred);
}

struct binary_predicate_selector_constructor {
   template <typename IteratorPair, typename Predicate, typename ExpectedFeatures>
   struct defs {
      typedef binary_helper<IteratorPair,Predicate> helper;
      typedef typename mix_features<ExpectedFeatures, end_sensitive>::type
         needed_pair_features;
      typedef void needed_features1;
      typedef void needed_features2;
      typedef binary_predicate_selector<IteratorPair, Predicate> iterator;
   };
};

template <typename ContainerRef1, typename ContainerRef2, typename Predicate>
class SelectedContainerPairSubset
   : public modified_container_pair_base<ContainerRef1, ContainerRef2, Predicate>,
     public modified_container_pair_impl< SelectedContainerPairSubset<ContainerRef1,ContainerRef2,Predicate>,
                                          list( Container1< ContainerRef1 >,
                                                Container2< ContainerRef2 >,
                                                IteratorConstructor< binary_predicate_selector_constructor >,
                                                Operation< Predicate > ) >,
     public generic_of_subset< SelectedContainerPairSubset<ContainerRef1,ContainerRef2,Predicate>,
                               typename deref<ContainerRef1>::type > {
   typedef modified_container_pair_base<ContainerRef1, ContainerRef2, Predicate> _base;
   typedef modified_container_pair_impl<SelectedContainerPairSubset> _super;
public:
   typedef typename least_derived< cons<bidirectional_iterator_tag, typename _super::container_category> >::type
      container_category;

   SelectedContainerPairSubset(typename _base::first_arg_type src1_arg, typename _base::second_arg_type src2_arg,
                               const Predicate& pred_arg=Predicate())
      : _base(src1_arg,src2_arg,pred_arg) {}

   using _base::get_operation;
};

template <typename ContainerRef1, typename ContainerRef2, typename Predicate>
struct spec_object_traits< SelectedContainerPairSubset<ContainerRef1, ContainerRef2, Predicate> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true,
                     is_always_const=effectively_const<ContainerRef1>::value;
};

template <typename Container1, typename Container2, typename Predicate> inline
SelectedContainerPairSubset<Container1&, Container2&, Predicate>
attach_selector(Container1& c1, Container2& c2, const Predicate& pred)
{
   return SelectedContainerPairSubset<Container1&, Container2&, Predicate> (c1,c2,pred);
}

template <typename Container1, typename Container2, typename Predicate> inline
SelectedContainerPairSubset<const Container1&, Container2&, Predicate>
attach_selector(const Container1& c1, Container2& c2, Predicate pred)
{
   return SelectedContainerPairSubset<const Container1&, Container2&, Predicate> (c1,c2,pred);
}

template <typename Container1, typename Container2, typename Predicate> inline
SelectedContainerPairSubset<Container1&, const Container2&, Predicate>
attach_selector(Container1& c1, const Container2& c2, Predicate pred)
{
   return SelectedContainerPairSubset<Container1&, const Container2&, Predicate> (c1,c2,pred);
}

template <typename Container1, typename Container2, typename Predicate> inline
SelectedContainerPairSubset<const Container1&, const Container2&, Predicate>
attach_selector(const Container1& c1, const Container2& c2, Predicate pred)
{
   return SelectedContainerPairSubset<const Container1&, const Container2&, Predicate> (c1,c2,pred);
}

template <typename Container1, typename Container2> inline
SelectedContainerPairSubset<Container1&, const Container2&, operations::apply2< BuildUnaryIt<operations::dereference> > >
attach_mask(Container1& data, const Container2& boolean)
{
   return SelectedContainerPairSubset<Container1&, const Container2&, operations::apply2< BuildUnaryIt<operations::dereference> > >
          (data,boolean);
}

template <typename Container1, typename Container2> inline
SelectedContainerPairSubset<const Container1&, const Container2&, operations::apply2< BuildUnaryIt<operations::dereference> > >
attach_mask(const Container1& data, const Container2& boolean)
{
   return SelectedContainerPairSubset<const Container1&, const Container2&, operations::apply2< BuildUnaryIt<operations::dereference> > >
          (data,boolean);
}

} // end namespace pm

namespace polymake {

using pm::SelectedSubset;
using pm::TruncatedContainer;
using pm::ContractedRanges;
using pm::FoldedRanges;
using pm::SelectedContainerPairSubset;
using pm::make_unary_predicate_selector;
using pm::make_range_contractor;
using pm::make_equal_range_contractor;
using pm::make_range_folder;
using pm::make_binary_predicate_selector;
using pm::make_output_predicate_selector;
using pm::attach_selector;
using pm::attach_mask;
using pm::contract_ranges;
using pm::contract_equal_ranges;
using pm::fold_ranges;

} // end namespace polymake

#endif // POLYMAKE_SELECTED_SUBSET_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
