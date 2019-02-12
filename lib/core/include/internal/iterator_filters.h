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

#ifndef POLYMAKE_INTERNAL_ITERATOR_FILTERS_H
#define POLYMAKE_INTERNAL_ITERATOR_FILTERS_H

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
   typedef Iterator base_t;
   typedef unary_helper<Iterator, Predicate> helper;
   typename helper::operation pred;

   void valid_position()
   {
      while (!this->at_end() && !pred(*helper::get(*this))) base_t::operator++();
   }

   template <typename, typename> friend class unary_predicate_selector;
public:
   typedef typename least_derived_class<typename iterator_traits<Iterator>::iterator_category, bidirectional_iterator_tag>::type iterator_category;
   typedef unary_predicate_selector<typename iterator_traits<Iterator>::iterator, Predicate> iterator;
   typedef unary_predicate_selector<typename iterator_traits<Iterator>::const_iterator, Predicate> const_iterator;

   unary_predicate_selector() = default;

   template <typename Predicate2>
   unary_predicate_selector(const unary_predicate_selector<typename iterator_traits<Iterator>::iterator, Predicate2>& it)
      : base_t(static_cast<const typename std::remove_reference_t<decltype(it)>::base_t&>(it))
      , pred(helper::create(it.pred)) {}

   template <typename Predicate2>
   explicit unary_predicate_selector(const unary_predicate_selector<typename iterator_reversed<Iterator>::type, Predicate2>& it)
      : base_t(iterator_reversed<Iterator>::reverse(it))
      , pred(helper::create(it.pred)) {}

   template <typename SourceIterator,
             typename suitable=typename std::enable_if<std::is_default_constructible<Predicate>::value,
                                                       typename suitable_arg_for_iterator<SourceIterator, Iterator>::type>::type>
   unary_predicate_selector(const SourceIterator& cur_arg, bool at_valid_position=false)
      : base_t(prepare_iterator_arg<Iterator>(cur_arg))
      , pred(helper::create(Predicate()))
   {
      if (!at_valid_position) valid_position();
   }

   template <typename SourceIterator,
             typename suitable=typename suitable_arg_for_iterator<SourceIterator, Iterator>::type>
   unary_predicate_selector(const SourceIterator& cur_arg, const Predicate& pred_arg, bool at_valid_position=false)
      : base_t(prepare_iterator_arg<Iterator>(cur_arg))
      , pred(helper::create(pred_arg))
   {
      if (!at_valid_position) valid_position();
   }

   unary_predicate_selector& operator++ ()
   {
      base_t::operator++();
      valid_position();
      return *this;
   }

   const unary_predicate_selector operator++ (int)
   {
      unary_predicate_selector copy=*this;  operator++();  return copy;
   }

   void rewind()
   {
      static_assert(check_iterator_feature<base_t, rewindable>::value, "iterator is not rewindable");
      base_t::rewind();
      valid_position();
   }

   // it's the applications' responsibility not to call this at the first position
   unary_predicate_selector& operator-- ()
   {
      static_assert(iterator_traits<base_t>::is_bidirectional, "iterator is not bidirectional");
      do
         base_t::operator--();
      while (!pred(*helper::get(*this)));
      return *this;
   }

   const unary_predicate_selector operator-- (int)
   {
      unary_predicate_selector copy=*this;  operator--();  return copy;
   }

private:
   // make them undefined for the case of random_access Iterator
   void operator+=(int) = delete;
   void operator-=(int) = delete;
   void operator+(int) = delete;
   void operator-(int) = delete;
   void operator[](int) = delete;
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
auto make_unary_predicate_selector(Iterator&& it, const Predicate& pred)
{
   return unary_predicate_selector<pointer2iterator_t<Iterator>, Predicate>(pointer2iterator(std::forward<Iterator>(it)), pred);
}

/** Iterator modifier truncating the trailing part of the sequence.
 *  Iterating is stopped at the first element evaluated to FALSE by the given functor.
 *  @tparam Iterator iterator over the source sequence.
 *  @tparam Predicate unary boolean functor.
 */
template <typename Iterator, typename Predicate>
class input_truncator : public Iterator {
protected:
   typedef Iterator base_t;
   typedef unary_helper<Iterator,Predicate> helper;
   typename helper::operation pred;

   template <typename, typename> friend class input_truncator;
public:
   typedef forward_iterator_tag iterator_category;
   typedef input_truncator<typename iterator_traits<Iterator>::iterator, Predicate> iterator;
   typedef input_truncator<typename iterator_traits<Iterator>::const_iterator, Predicate> const_iterator;

   input_truncator() = default;

   template <typename Predicate2>
   input_truncator(const input_truncator<typename iterator_traits<Iterator>::iterator, Predicate2>& it)
      : base_t(static_cast<const typename std::remove_reference_t<decltype(it)>::base_t&>(it))
      , pred(helper::create(it.pred)) {}

   template <typename SourceIterator, typename suitable=typename suitable_arg_for_iterator<SourceIterator, Iterator>::type>
   input_truncator(const SourceIterator& cur_arg, const Predicate& pred_arg=Predicate())
      : base_t(prepare_iterator_arg<Iterator>(cur_arg))
      , pred(helper::create(pred_arg)) {}

   input_truncator& operator++ ()
   {
      base_t::operator++();
      return *this;
   }
   const input_truncator operator++ (int)
   {
      input_truncator copy=*this; operator++(); return copy;
   }

   bool at_end() const
   {
      return base_t::at_end() || !pred(*helper::get(*this));
   }

   template <typename Other>
   typename std::enable_if<is_among<Other, iterator, const_iterator>::value, bool>::type
   operator== (const Other& it) const
   {
      return at_end() ? it.at_end() : static_cast<const Iterator&>(*this)==it;
   }

   template <typename Other>
   typename std::enable_if<is_among<Other, iterator, const_iterator>::value, bool>::type
   operator!= (const Other& it) const
   {
      return !operator==(it);
   }
private:
   // make them undefined for the case of bidirectional or random_access Iterator
   void operator--() = delete;
   void operator--(int) = delete;
   void operator+=(int) = delete;
   void operator-=(int) = delete;
   void operator+(int) = delete;
   void operator-(int) = delete;
   void operator[](int) = delete;
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
   typedef Iterator base_t;
protected:
   typedef binary_op_builder<Predicate, Iterator, Iterator> op_helper;
   typename op_helper::operation pred;
public:
   typedef forward_iterator_tag iterator_category;
   typedef range_contractor<typename iterator_traits<Iterator>::iterator, Predicate> iterator;
   typedef range_contractor<typename iterator_traits<Iterator>::const_iterator, Predicate> const_iterator;

   range_contractor() = default;

   template <typename Predicate2>
   range_contractor(const range_contractor<typename iterator_traits<Iterator>::iterator, Predicate2>& it)
      : base_t(static_cast<const typename std::remove_reference_t<decltype(it)>::base_t&>(it))
      , pred(op_helper::create(it.pred)) {}

   template <typename SourceIterator, typename suitable=typename suitable_arg_for_iterator<SourceIterator, Iterator>::type>
   range_contractor(const SourceIterator& start, const Predicate& pred_arg=Predicate())
      : base_t(prepare_iterator_arg<Iterator>(start))
      , pred(op_helper::create(pred_arg)) {}

   range_contractor& operator++ ()
   {
      const auto& shown = *(*this);
      do base_t::operator++();
      while (!this->at_end() && pred(shown, *(*this)));
      return *this;
   }

   const range_contractor operator++ (int)
   {
      range_contractor copy=*this;  operator++();  return copy;
   }

   void rewind()
   {
      static_assert(check_iterator_feature<base_t, rewindable>::value, "iterator is not rewindable");
      base_t::rewind();
   }

private:
   // make them undefined for the case of bidirectional or random_access Iterator
   void operator--() = delete;
   void operator+=(int) = delete;
   void operator-=(int) = delete;
   void operator+(int) = delete;
   void operator-(int) = delete;
   void operator[](int) = delete;
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
auto make_range_contractor(Iterator&& it, const Predicate& pred)
{
   return range_contractor<pointer2iterator_t<Iterator>, Predicate>(pointer2iterator(std::forward<Iterator>(it)), pred);
}

template <typename Iterator> inline
auto make_equal_range_contractor(Iterator&& it)
{
   return range_contractor<pointer2iterator_t<Iterator>, BuildBinary<operations::eq> >(pointer2iterator(std::forward<Iterator>(it)));
}


template <typename Iterator, typename FoldingOperation>
class range_folder : public Iterator {
   template <typename, typename> friend class range_folder;
   typedef Iterator base_t;
protected:
   typedef unary_helper<Iterator, FoldingOperation> helper;

   typename helper::operation op;
   bool _at_end;

   void valid_position()
   {
      op.reset(*helper::get(static_cast<const base_t&>(*this)));

      while (!(++static_cast<base_t&>(*this)).at_end() &&
             op(*helper::get(static_cast<const base_t&>(*this)))) ;
   }

public:
   typedef forward_iterator_tag iterator_category;
   typedef typename helper::operation::value_type value_type;
   typedef typename helper::operation::reference reference;
   typedef range_folder<typename iterator_traits<Iterator>::iterator, FoldingOperation> iterator;
   typedef range_folder<typename iterator_traits<Iterator>::const_iterator, FoldingOperation> const_iterator;

   range_folder() = default;

   range_folder(const iterator& it)
      : base_t(static_cast<const typename std::remove_reference_t<decltype(it)>::base_t&>(it))
      , op(helper::create(it.op))
      , _at_end(it._at_end) {}

   template <typename SourceIterator, typename suitable=typename suitable_arg_for_iterator<SourceIterator, Iterator>::type>
   range_folder(const SourceIterator& start, const FoldingOperation& op_arg=FoldingOperation())
      : base_t(prepare_iterator_arg<Iterator>(start))
      , op(helper::create(op_arg))
      , _at_end(base_t::at_end())
   {
      if (!_at_end) valid_position();
   }

   reference operator* () const { return op.get(); }
   int index() const { return op.get_index(); }

   range_folder& operator++ ()
   {
      if (base_t::at_end())
         _at_end=true;
      else
         valid_position();
      return *this;
   }

   const range_folder operator++ (int) { range_folder copy(*this);  operator++();  return copy; }

   bool at_end() const { return _at_end; }

   void rewind()
   {
      static_assert(check_iterator_feature<base_t, rewindable>::value, "iterator is not rewindable");
      base_t::rewind();
      if (!(_at_end=base_t::at_end())) valid_position();
   }

private:
   // make them undefined for the case of bidirectional or random_access Iterator
   void operator--() = delete;
   void operator--(int) = delete;
   void operator+=(int) = delete;
   void operator-=(int) = delete;
   void operator+(int) = delete;
   void operator-(int) = delete;
   void operator[](int) = delete;
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
auto make_range_folder(Iterator&& it, const FoldingOperation& op)
{
   return range_folder<pointer2iterator_t<Iterator>, FoldingOperation>(pointer2iterator(std::forward<Iterator>(it)), op);
}

/** Selecting output iterator

    This is a combination of an output iterator (called below `basis iterator') and a predicate.

    When a data item is assigned to the output iterator, it is first evaluated by the predicate object.
    Only items mapped to TRUE are passed through to the assignment method of the basis iterator.
*/
template <typename Iterator, typename Predicate>
class output_predicate_selector : public Iterator {
protected:
   Predicate pred;
   typedef Iterator base_t;
public:
   typedef output_iterator_tag iterator_category;
   typedef typename iterator_traits<Iterator>::value_type value_type;

   output_predicate_selector() = default;

   output_predicate_selector(const Iterator& cur_arg, const Predicate& pred_arg=Predicate())
      : base_t(cur_arg)
      , pred(pred_arg) {}

   output_predicate_selector& operator= (typename function_argument<value_type>::type arg)
   {
      if (pred(arg))
         static_cast<base_t&>(*this)=arg;
      return *this;
   }

   template <typename Arg>
   output_predicate_selector& operator= (const Arg& arg)
   {
      if (pred(arg))
         static_cast<base_t&>(*this)=arg;
      return *this;
   }

   output_predicate_selector& operator* () { return *this; }
   output_predicate_selector& operator++ () { return *this; }
   output_predicate_selector& operator++ (int) { return *this; }
};

template <typename Iterator, typename Predicate> inline
auto make_output_predicate_selector(Iterator&& it, Predicate pred)
{
   return output_predicate_selector<Iterator, Predicate>(it, pred);
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
   typedef IteratorPair base_t;
protected:
   typedef binary_helper<IteratorPair,Predicate> helper;
   typename helper::operation pred;

   void valid_position()
   {
      while (!this->at_end() && !pred(*helper::get1(*this), *helper::get2(this->second)))
         base_t::operator++();
   }

   template <typename, typename> friend class binary_predicate_selector;
public:
   typedef typename least_derived_class<typename IteratorPair::iterator_category, bidirectional_iterator_tag>::type iterator_category;
   typedef binary_predicate_selector<typename iterator_traits<IteratorPair>::iterator, Predicate> iterator;
   typedef binary_predicate_selector<typename iterator_traits<IteratorPair>::const_iterator, Predicate> const_iterator;

   binary_predicate_selector() = default;

   template <typename Predicate2>
   binary_predicate_selector(const binary_predicate_selector<typename iterator_traits<IteratorPair>::iterator, Predicate2>& it)
      : base_t(static_cast<const typename std::remove_reference_t<decltype(it)>::base_t&>(it))
      , pred(helper::create(it.pred)) {}

   template <typename SourceIteratorPair,
             typename suitable=typename std::enable_if<std::is_default_constructible<Predicate>::value,
                                                       typename suitable_arg_for_iterator<SourceIteratorPair, IteratorPair>::type>::type>
   binary_predicate_selector(const SourceIteratorPair& cur_arg,
                             bool at_valid_position=false)
      : base_t(prepare_iterator_arg<IteratorPair>(cur_arg))
      , pred(helper::create(Predicate()))
   {
      if (!at_valid_position) valid_position();
   }

   template <typename SourceIteratorPair,
             typename suitable=typename suitable_arg_for_iterator<SourceIteratorPair, IteratorPair>::type>
   binary_predicate_selector(const SourceIteratorPair& cur_arg,
                             const Predicate& pred_arg,
                             bool at_valid_position=false)
      : base_t(prepare_iterator_arg<IteratorPair>(cur_arg))
      , pred(helper::create(pred_arg))
   {
      if (!at_valid_position) valid_position();
   }

   template <typename SourceIterator1, typename SourceIterator2,
             typename suitable1=typename std::enable_if<std::is_default_constructible<Predicate>::value,
                                                        typename suitable_arg_for_iterator<SourceIterator1, typename IteratorPair::first_type>::type>::type,
             typename suitable2=typename suitable_arg_for_iterator<SourceIterator2, typename IteratorPair::second_type>::type>
   binary_predicate_selector(const SourceIterator1& first_arg,
                             const SourceIterator2& second_arg,
                             bool at_valid_position=false)
      : base_t(prepare_iterator_arg<typename IteratorPair::first_type>(first_arg), prepare_iterator_arg<typename IteratorPair::second_type>(second_arg))
      , pred(helper::create(Predicate()))
   {
      if (!at_valid_position) valid_position();
   }

   template <typename SourceIterator1, typename SourceIterator2,
             typename suitable1=typename suitable_arg_for_iterator<SourceIterator1, typename IteratorPair::first_type>::type,
             typename suitable2=typename suitable_arg_for_iterator<SourceIterator2, typename IteratorPair::second_type>::type>
   binary_predicate_selector(const SourceIterator1& first_arg,
                             const SourceIterator2& second_arg,
                             const Predicate& pred_arg,
                             bool at_valid_position=false)
      : base_t(prepare_iterator_arg<typename IteratorPair::first_type>(first_arg), prepare_iterator_arg<typename IteratorPair::second_type>(second_arg))
      , pred(helper::create(pred_arg))
   {
      if (!at_valid_position) valid_position();
   }

   binary_predicate_selector& operator++ ()
   {
      base_t::operator++();
      valid_position();
      return *this;
   }
   const binary_predicate_selector operator++ (int)
   {
      binary_predicate_selector copy=*this; operator++(); return copy;
   }

   binary_predicate_selector& operator-- ()
   {
      static_assert(iterator_traits<base_t>::is_bidirectional, "iterator is not bidirectional");
      do
         base_t::operator--();
      while (!pred(*helper::get1(*this), *helper::get2(this->second)));
      return *this;
   }
   const binary_predicate_selector operator-- (int)
   {
      binary_predicate_selector copy=*this; operator--(); return copy;
   }

   void rewind()
   {
      static_assert(check_iterator_feature<base_t, rewindable>::value, "iterator is not rewindable");
      base_t::rewind();
      valid_position();
   }
private:
   void operator+=(int) = delete;
   void operator-=(int) = delete;
   void operator+(int) = delete;
   void operator-(int) = delete;
   void operator[](int) = delete;
};

template <typename IteratorPair, typename Predicate, typename Feature>
struct check_iterator_feature<binary_predicate_selector<IteratorPair, Predicate>, Feature>
   : check_iterator_feature<IteratorPair, Feature> {};

template <typename Iterator1, typename Iterator2, typename Predicate> inline
auto make_binary_predicate_selector(Iterator1& first, Iterator2& second, const Predicate& pred)
{
   return binary_predicate_selector<iterator_pair<pointer2iterator_t<Iterator1>, pointer2iterator_t<Iterator2>>, Predicate>
      (pointer2iterator(std::forward<Iterator1>(first)), pointer2iterator(std::forward<Iterator2>(second)), pred);
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

}

namespace polymake {

using pm::make_unary_predicate_selector;
using pm::make_range_contractor;
using pm::make_equal_range_contractor;
using pm::make_range_folder;
using pm::make_binary_predicate_selector;
using pm::make_output_predicate_selector;

}

#endif // POLYMAKE_INTERNAL_ITERATOR_FILTERS_H
// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
