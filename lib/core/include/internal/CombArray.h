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

#ifndef POLYMAKE_INTERNAL_COMB_ARRAY_H
#define POLYMAKE_INTERNAL_COMB_ARRAY_H

#include "polymake/list"
#include "polymake/SelectedSubset.h"
#include "polymake/Array.h"

namespace pm {

template <typename Row, int _is_rev> class comb_iterator;
template <typename Row, int _is_rev> class CombArray;
template <typename Row, int _is_rev,
          bool _is_sparse=check_container_ref_feature<Row,sparse>::value ||
                          identical<typename object_traits<typename deref<Row>::type>::generic_tag, is_set>::value>
class CombArray_helper;

template <typename Row>
class CombArray_helper<Row,2,false> {   // !reversed, !sparse
protected:
   typedef Array<typename container_traits<Row>::iterator> it_container;

   template <typename Iterator>
   void init_begin(it_container& itc, Iterator& src)
   {
      for (typename Entire<it_container>::iterator it=entire(itc); !it.at_end(); ++it, ++src)
         *it=src->begin();
   }
   template <typename Iterator>
   void init_end(it_container& itc, Iterator& src)
   {
      for (typename Entire<it_container>::iterator it=entire(itc); !it.at_end(); ++it, ++src)
         *it=src->end();
   }
};

template <typename Row>
class CombArray_helper<Row,3,false> {   // reversed, !sparse
protected:
   typedef Array<typename container_traits<Row>::reverse_iterator> it_container;

   template <typename Iterator>
   void init_begin(it_container& itc, Iterator& src)
   {
      for (typename Entire<it_container>::iterator it=entire(itc); !it.at_end(); ++it, ++src)
         *it=src->rbegin();
   }
   template <typename Iterator>
   void init_end(it_container& itc, Iterator& src)
   {
      for (typename Entire<it_container>::iterator it=entire(itc); !it.at_end(); ++it, ++src)
         *it=src->rend();
   }
};

template <typename Row, int _is_rev>
class CombArray_helper<Row, _is_rev, false>
   : public CombArray_helper<Row, _is_rev+2, false> {
protected:
   typedef typename CombArray_helper<Row, _is_rev+2, false>::it_container it_container;

   void incr(it_container& itc, int& index)
   {
      for (typename Entire<it_container>::iterator it=entire(itc); !it.at_end(); ++it)
         ++(*it);
      !_is_rev ? ++index : --index;
   }
public:
   typedef cons< Container< it_container >,
                 Operation< BuildUnary<operations::dereference> > >
      manip_traits;

   it_container& get_container() { return static_cast<CombArray<Row,_is_rev>*>(this)->it_array; }
   const it_container& get_container() const { return static_cast<const CombArray<Row,_is_rev>*>(this)->it_array; }
};

template <typename Row>
class CombArray_helper<Row,2,true> {    // !reversed, sparse
protected:
   typedef Array< std::pair<typename container_traits<Row>::iterator, Row*> > it_container;

   template <typename Iterator>
   void init_begin(it_container& itc, Iterator& src)
   {
      for (typename Entire<it_container>::iterator it=entire(itc); !it.at_end(); ++it, ++src)
         it->first=(it->second=&*src)->begin();
   }
   template <typename Iterator>
   void init_end(it_container& itc, Iterator& src)
   {
      for (typename Entire<it_container>::iterator it=entire(itc); !it.at_end(); ++it, ++src)
         it->first=(it->second=&*src)->end();
   }
};

template <typename Row>
class CombArray_helper<Row,3,true> {    // reversed, sparse
protected:
   typedef Array< std::pair<typename container_traits<Row>::reverse_iterator, Row*> > it_container;

   template <typename Iterator>
   void init_begin(it_container& itc, Iterator& src)
   {
      for (typename Entire<it_container>::iterator it=entire(itc); !it.at_end(); ++it, ++src)
         it->first=(it->second=&*src)->rbegin();
   }
   template <typename Iterator>
   void init_end(it_container& itc, Iterator& src)
   {
      for (typename Entire<it_container>::iterator it=entire(itc); !it.at_end(); ++it, ++src)
         it->first=(it->second=&*src)->rend();
   }
};

template <typename Row, int _is_rev>
class CombArray_helper<Row, _is_rev, true>
   : public CombArray_helper<Row, _is_rev+2, true> {
protected:
   typedef typename CombArray_helper<Row, _is_rev+2, true>::it_container it_container;

   class it_filter {
   public:
      typedef typename it_container::value_type::first_type argument_type;
      typedef bool result_type;

      it_filter(int index_arg=0) : index(index_arg) {}

      bool operator() (const argument_type& it) const
      {
         return !it.at_end() && test(it, identical<typename object_traits<typename deref<Row>::type>::generic_tag, is_set>());
      }
   protected:
      int index;

      bool test(const argument_type& it, False) const { return it.index()==index; }
      bool test(const argument_type& it, True) const { return *it==index; }
   };

   static it_container& get_c(CombArray<Row,_is_rev>& cv) { return cv.it_array; }
   static const it_container& get_c(const CombArray<Row,_is_rev>& cv) { return cv.it_array; }
   static int get_i(const CombArray<Row,_is_rev>& cv) { return cv.index; }

   class intermediate
      : public modified_container_impl< intermediate,
                                        list( Container< it_container >,
                                              Operation< operations::composed11<BuildUnary<operations::take_first>,
                                                                                it_filter> >,
                                              IteratorConstructor< unary_predicate_selector_constructor >,
                                              ExpectedFeatures< indexed >,
                                              Hidden< CombArray<Row,_is_rev> > ) > {
      typedef modified_container_impl<intermediate> _super;
   public:
      it_container& get_container() { return get_c(this->hidden()); }
      const it_container& get_container() const { return get_c(this->hidden()); }
      typename _super::operation get_operation() const { return it_filter(get_i(this->hidden())); }
   };

   void incr(it_container& itc, int& index)
   {
      const it_filter cf(index);
      for (typename Entire<it_container>::iterator it=entire(itc); !it.at_end(); ++it)
         if (cf(it->first)) ++it->first;
      !_is_rev ? ++index : --index;
   }

public:
   typedef typename if_else< identical<typename object_traits<typename deref<Row>::type>::generic_tag, is_set>::value,
                             BuildUnaryIt<operations::index2element>,
                             operations::composed11<BuildUnary<operations::take_first>, 
                                                    BuildUnary<operations::dereference> > >::type
      access_op;

   typedef cons< Container<intermediate>, Operation<access_op> > manip_traits;

   intermediate& get_container() { return reinterpret_cast<intermediate&>(*this); }
   const intermediate& get_container() const { return reinterpret_cast<const intermediate&>(*this); }

   void insert(int i)
   {
      typename it_container::reference it_vec=this->hidden().it_array[i];
      it_vec.first=it_vec.second->insert(it_vec.first, this->hidden().index);
   }
   template <typename Iterator>
   void insert(const Iterator&, int i,
               typename enable_if<void**, derived_from<Iterator, typename intermediate::iterator>::value>::type =0)
   {
      insert(i);
   }
   template <typename Iterator, typename Data>
   void insert(const Iterator&, int i, const Data& d,
               typename enable_if<void**, derived_from<Iterator, typename intermediate::iterator>::value>::type =0)
   {
      typename it_container::reference it_vec=this->hidden().it_array[i];
      it_vec.first=it_vec.second->insert(it_vec.first, this->hidden().index, d);
   }
   void erase(int i)
   {
      typename it_container::reference it_vec=this->hidden().it_array[i];
      it_vec.second->erase(it_vec.first++);
   }
   template <typename Iterator>
   void erase(const Iterator& pos,
              typename enable_if<void**, derived_from<Iterator, typename intermediate::iterator>::value>::type =0)
   {
      erase(pos.index());
   }
};

template <typename Row, int _is_rev>
class CombArray
   : public CombArray_helper<Row,_is_rev>,
     public modified_container_impl< CombArray<Row,_is_rev>,
                                     typename CombArray_helper<Row,_is_rev>::manip_traits >,
     public inherit_generic< CombArray<Row,_is_rev>, typename deref<Row>::type >::type {

   typedef modified_container_impl<CombArray> _super;
   typedef CombArray_helper<Row,_is_rev> _helper;
   friend class comb_iterator<Row,_is_rev>;
   template <typename, int> friend class CombArray;
   friend class CombArray_helper<Row,_is_rev>;
protected:
   typename _helper::it_container it_array;
   int index;

   typedef typename inherit_const<CombArray<typename deref<Row>::type,_is_rev>, Row>::type nonconst_arg;
public:
   CombArray() {}

   CombArray(const CombArray& other)
      : it_array(other.it_array)
      , index(other.index) {}

   CombArray(nonconst_arg& other)
      : it_array(reinterpret_cast<const typename _helper::it_container&>(other.it_array))
      , index(other.index) {}

   CombArray& operator= (const CombArray& other)
   {
      return CombArray::generic_type::operator=(other);
   }
   using CombArray::generic_type::operator=;

   template <typename Iterator>
   CombArray(int n, Iterator src)
      : it_array(n), index(!_is_rev ? 0 : n-1)
   {
      this->init_begin(it_array, src);
   }

   template <typename Iterator>
   CombArray(int n, maximal<int>, Iterator src)
      : it_array(n), index(!_is_rev ? n : -1)
   {
      this->init_end(it_array, src);
   }

   template <typename Iterator>
   CombArray(int n, int index_arg, Iterator src)
      : it_array(n), index(index_arg)
   {
      for (typename Entire<typename _helper::it_container>::iterator it=entire(it_array); !it.at_end(); ++it, ++src)
         *it=src->begin()+index;
   }

   int dim() const { return it_array.size(); }

protected:
   void copy(const CombArray& other)
   {
      it_array=other.it_array;
      index=other.index;
   }
   void copy(nonconst_arg& other)
   {
      it_array=reinterpret_cast<const typename _helper::it_container&>(other.it_array);
      index=other.index;
   }
   void incr()
   {
      _helper::incr(it_array, index);
   }
};

template <typename Row, int _is_rev>
class comb_iterator {
public:
   typedef CombArray<Row,_is_rev> value_type;
   typedef typename inherit_const<value_type, Row>::type& reference;
   typedef typename inherit_const<value_type, Row>::type* pointer;
   typedef forward_iterator_tag iterator_category;
   typedef ptrdiff_t difference_type;
   typedef comb_iterator<typename deref<Row>::type, _is_rev> iterator;
   typedef comb_iterator<const typename deref<Row>::type, _is_rev> const_iterator;

   comb_iterator() {}
   comb_iterator(const iterator& it)
      : cv(it.value) {}

   comb_iterator& operator= (const const_iterator& it)
   {
      cv.copy(it.cv);
      return *this;
   }

   comb_iterator& operator= (typename assign_const<iterator, !identical<iterator,comb_iterator>::value>::type& it)
   {
      cv.copy(it.cv);
      return *this;
   }

   template <typename Iterator>
   comb_iterator(int m, int n, Iterator src)
      : cv(m,src), _last(n) {}

   template <typename Iterator>
   comb_iterator(int m, int n, maximal<int>, Iterator src)
      : cv(m,maximal<int>(),src), _last(n) {}

   template <typename Iterator>
   comb_iterator(int m, int n, int index_arg, Iterator src)
      : cv(m,index_arg,src), _last(n) {}

   reference operator* () const
   {
      return const_cast<reference>(cv);
   }
   pointer operator-> () const
   {
      return const_cast<pointer>(&cv);
   }

   comb_iterator& operator++ () { cv.incr(); return *this; }
   const comb_iterator operator++ (int) { comb_iterator copy=*this; operator++(); return copy; }

   bool operator== (const const_iterator& it) const { return index()==it.index(); }
   bool operator== (typename assign_const<iterator, !identical<iterator,comb_iterator>::value>::type& it) { return index()==it.index(); }
   bool operator!= (const const_iterator& it) const { return !operator==(it); }
   bool operator!= (typename assign_const<iterator, !identical<iterator,comb_iterator>::value>::type& it) { return !operator==(it); }

   bool at_end() const { return index()==_last; }
   int index() const { return cv.index; }
protected:
   value_type cv;
   int _last;
};

template <typename Row, int _is_rev>
struct check_iterator_feature<comb_iterator<Row, _is_rev>, end_sensitive> : True {};
template <typename Row, int _is_rev>
struct check_iterator_feature<comb_iterator<Row, _is_rev>, indexed> : True {};

template <typename Row, int _is_rev>
struct check_container_feature<CombArray<Row, _is_rev>, sparse>
   : check_container_ref_feature<Row, sparse> {};

template <typename Row, int _is_rev>
struct check_container_feature<CombArray<Row, _is_rev>, pure_sparse>
   : check_container_ref_feature<Row, pure_sparse> {};

template <typename Row>
struct ListMatrix_data {
   std::list<Row> R;
   int dimr, dimc;

   ListMatrix_data() : dimr(0), dimc(0) {}

   void clear()
   {
      dimr=dimc=0;
      R.clear();
   }
};

template <typename Matrix, typename Row, typename Category=typename container_traits<Row>::category>
class ListMatrix_cols {
protected:
   typedef Matrix hidden_type;
   hidden_type& hidden() { return reinterpret_cast<hidden_type&>(*this); }
   const hidden_type& hidden() const { return reinterpret_cast<const hidden_type&>(*this); }
public:
   typedef comb_iterator<Row,0> iterator;
   typedef comb_iterator<const Row,0> const_iterator;
   typedef CombArray<Row,0> value_type;
   typedef value_type reference;
   typedef CombArray<const Row,0> const_reference;
   typedef Category container_category;

   iterator begin()
   {
      return iterator(hidden().rows(), hidden().cols(), hidden().data->R.begin());
   }
   iterator end()
   {
      return iterator(hidden().rows(), hidden().cols(), maximal<int>(), hidden().data->R.begin());
   }
   const_iterator begin() const
   {
      return const_iterator(hidden().rows(), hidden().cols(), hidden().data->R.begin());
   }
   const_iterator end() const
   {
      return const_iterator(hidden().rows(), hidden().cols(), maximal<int>(), hidden().data->R.begin());
   }

   reference front()
   {
      return reference(hidden().rows(), hidden().data->R.begin());
   }
   const_reference front() const
   {
      return const_reference(hidden().rows(), hidden().data->R.begin());
   }

   int size() const { return hidden().cols(); }
   bool empty() const { return size()==0; }
   void resize(int n) { hidden().resize(hidden().rows(), n); }
};

template <typename Matrix, typename Row>
class ListMatrix_cols<Matrix, Row, bidirectional_iterator_tag>
   : public ListMatrix_cols<Matrix, Row, forward_iterator_tag> {
public:
   typedef comb_iterator<Row,1> reverse_iterator;
   typedef comb_iterator<const Row,1> const_reverse_iterator;
   typedef bidirectional_iterator_tag container_category;

   reverse_iterator rbegin()
   {
      return reverse_iterator(this->hidden().rows(), this->hidden().cols(), this->hidden().data->R.begin());
   }
   reverse_iterator rend()
   {
      return reverse_iterator(this->hidden().rows(), this->hidden().cols(), maximal<int>(), this->hidden().data->R.begin());
   }
   const_reverse_iterator rbegin() const
   {
      return const_reverse_iterator(this->hidden().rows(), this->hidden().cols(), this->hidden().data->R.begin());
   }
   const_reverse_iterator rend() const
   {
      return reverse_iterator(this->hidden().rows(), this->hidden().cols(), maximal<int>(), this->hidden().data->R.begin());
   }

   CombArray<Row,1> back()
   {
      return CombArray<Row,1>(this->hidden().rows(), this->hidden().data->R.begin());
   }
   CombArray<const Row,1> back() const
   {
      return CombArray<const Row,1>(this->hidden().rows(), this->hidden().data->R.begin());
   }
};

template <typename Matrix, typename Row>
class ListMatrix_cols<Matrix, Row, random_access_iterator_tag>
   : public ListMatrix_cols<Matrix, Row, bidirectional_iterator_tag> {
public:
   typedef random_access_iterator_tag container_category;

   CombArray<Row,0> operator[] (int c)
   {
      return CombArray<Row,0>(this->hidden().rows(), c, this->hidden().data->R.begin());
   }
   CombArray<const Row,0> operator[] (int c) const
   {
      return CombArray<const Row,0>(this->hidden().rows(), c, this->hidden().data->R.begin());
   }
};

} // end namespace pm

namespace polymake {
   using pm::CombArray;
}

#endif // POLYMAKE_INTERNAL_COMB_ARRAY_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
