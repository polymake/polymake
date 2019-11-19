/* Copyright (c) 1997-2019
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

#ifndef POLYMAKE_FIBONACCI_H
#define POLYMAKE_FIBONACCI_H

#include "polymake/internal/iterators.h"
#include <limits>

namespace pm
{
   template <typename Number=int> class FibonacciNumbers;

   // An infinite source of Fibonacci numbers - as iterator
   template <typename Number=int>
   class Fibonacci_iterator {
   public:
      typedef forward_iterator_tag iterator_category;
      typedef Number value_type;
      typedef const Number& reference;
      typedef const Number* pointer;
      typedef ptrdiff_t difference_type;
      typedef Fibonacci_iterator iterator;
      typedef iterator const_iterator;

      Fibonacci_iterator() : prev(0), cur(1) { }

      reference operator* () const { return cur; }
      pointer operator-> () const { return &cur; }

      iterator& operator++ () { const Number p=prev; prev=cur; cur+=p; return *this; }
      const iterator operator++ (int) { iterator copy(*this); operator++(); return copy; }

      bool operator== (const iterator& it) const { return prev==it.prev && cur==it.cur; }
      bool operator!= (const iterator& it) const { return !operator==(it); }

      void rewind() { prev=0; cur=1; }
   protected:
      Number prev, cur;

      Fibonacci_iterator(bool at_end) : prev(0), cur(0) { }
      friend class FibonacciNumbers<Number>;
   };


   // An infinite source of Fibonacci numbers - as container
   template <typename Number>
   class FibonacciNumbers {
   public:
      typedef Number value_type;
      typedef const Number& reference;
      typedef reference const_reference;
      typedef Fibonacci_iterator<Number> iterator;
      typedef iterator const_iterator;

      iterator begin() const { return iterator(); }
      iterator end() const { return iterator(true); }
      reference front() const { static Number one(1); return one; }
      size_t size() const { return std::numeric_limits<size_t>::max(); }
      bool empty() const { return false; }
   };

   template <typename Number>
   struct check_iterator_feature<Fibonacci_iterator<Number>, unlimited> : std::true_type { };

   template <typename Number>
   struct check_iterator_feature<Fibonacci_iterator<Number>, rewindable> : std::true_type { };

   template <typename Number>
   struct spec_object_traits< FibonacciNumbers<Number> > : spec_object_traits<is_container> { };

   template <typename Number> inline
   Fibonacci_iterator<Number> fibonacci_numbers() { return Fibonacci_iterator<Number>(); }

   inline
   Fibonacci_iterator<> fibonacci_numbers() { return Fibonacci_iterator<>(); }

   template <typename Number> inline
   Number fibonacci_number(int i)
   {
      Fibonacci_iterator<Number> it;
      while (--i>=0) ++it;
      return *it;
   }

   inline int fibonacci_number(int i) { return fibonacci_number<int>(i); }
}

namespace polymake {
   using pm::Fibonacci_iterator;
   using pm::FibonacciNumbers;
   using pm::fibonacci_numbers;
   using pm::fibonacci_number;
}

#endif // POLYMAKE_FIBONACCI_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
