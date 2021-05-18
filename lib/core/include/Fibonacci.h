/* Copyright (c) 1997-2021
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

#include "polymake/internal/iterators.h"
#include "polymake/Integer.h"

namespace pm {
   template <typename Number = Int> class FibonacciNumbers;

   // An infinite source of Fibonacci numbers - as iterator
   template <typename Number = Int>
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

      explicit Fibonacci_iterator(unsigned long start_index)
      {
         if (start_index == 0) {
            rewind();
         } else {
            const auto start = Integer::fibonacci2(start_index);
            cur = static_cast<Number>(start.first);
            prev = static_cast<Number>(start.second);
         }
      }

      reference operator* () const { return cur; }
      pointer operator-> () const { return &cur; }

      iterator& operator++ () { const Number p = prev; prev = cur; cur += p; return *this; }
      const iterator operator++ (int) { iterator copy(*this); operator++(); return copy; }

      bool operator== (const iterator& it) const { return cur == it.cur; }
      bool operator!= (const iterator& it) const { return !operator==(it); }

      void rewind() { prev = 0; cur = 1; }
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
      reference front() const { return one_value<Number>(); }
      Int size() const { return std::numeric_limits<Int>::max(); }
      bool empty() const { return false; }
   };

   template <typename Number>
   struct check_iterator_feature<Fibonacci_iterator<Number>, unlimited> : std::true_type { };

   template <typename Number>
   struct check_iterator_feature<Fibonacci_iterator<Number>, rewindable> : std::true_type { };

   template <typename Number>
   struct spec_object_traits< FibonacciNumbers<Number> > : spec_object_traits<is_container> { };

   template <typename Number = Int>
   Fibonacci_iterator<Number> fibonacci_numbers() { return Fibonacci_iterator<Number>(); }
}

namespace polymake {
   using pm::Fibonacci_iterator;
   using pm::FibonacciNumbers;
   using pm::fibonacci_numbers;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
