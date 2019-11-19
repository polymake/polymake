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

#ifndef POLYMAKE_MULTI_DIM_COUNTER_H
#define POLYMAKE_MULTI_DIM_COUNTER_H

#include "polymake/Vector.h"

namespace pm {

template <bool left_to_right=true, typename number_type=int>
class MultiDimCounter {
public:
   typedef bidirectional_iterator_tag iterator_category;
   typedef ptrdiff_t difference_type;
   typedef Vector<number_type> value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef MultiDimCounter iterator;
   typedef MultiDimCounter const_iterator;

protected:
   value_type my_start, my_counter, my_limits;
   bool my_at_end;

public:
   MultiDimCounter() { my_at_end = true; }

   template <typename Container>
   explicit MultiDimCounter(const Container& limits)
      : my_start(limits.size()), my_counter(my_start.size()),
        my_limits(my_start.size(), limits.begin()),
        my_at_end(my_start.empty())
   {
      if (POLYMAKE_DEBUG) {
         for (int i=0, n=my_start.size(); i<n; ++i)
            if (my_limits[i] <= 0)
               throw std::runtime_error("MultiDimCounter: wrong limit value");
      }
   }

   template <typename Container1, typename Container2>
   MultiDimCounter(const Container1& start, const Container2& limits)
      : my_start(start.size(), start.begin()), my_counter(my_start),
        my_limits(my_start.size(), limits.begin()),
        my_at_end(my_start.empty())
   {
      if (POLYMAKE_DEBUG) {
         for (int i=0, n=my_start.size(); i<n; ++i)
            if (my_limits[i] <= my_start[i])
               throw std::runtime_error("MultiDimCounter: wrong limit/start values");
      }
   }

   MultiDimCounter& operator++ ();
   MultiDimCounter& operator-- ();
   const iterator operator++ (int) { iterator copy=*this; operator++(); return copy; }
   const iterator operator-- (int) { iterator copy=*this; operator--(); return copy; }

   bool operator== (const MultiDimCounter& count) const
   {
      return count->size() == my_counter.size()
             && equal_ranges(entire(my_counter), count->begin());
   }

   bool operator!= (const MultiDimCounter& count) const { return !operator==(count); }

   reference operator* () const { return my_counter; }
   pointer operator-> () const { return &my_counter; }

   void rewind()
   {
      my_counter=my_start;
      my_at_end=my_counter.empty();
   }

   template <typename Container>
   void set(const Container& values)
   {
      if (POLYMAKE_DEBUG) {
         if (values.size() != my_limits.size())
            throw std::runtime_error("MultiDimCounter::set - dimension mismatch");

         auto l=my_limits.begin();
         auto v=values.begin();
         for (auto s=entire(my_start);  !s.at_end();  ++s, ++l, ++v)
            if (*v < *s || *v >= *l)
               throw std::runtime_error("MultiDimCounter::set - value out of range");
      }
      copy_range(values.begin(), entire(my_counter));
   }

   void set_digit(int digit, const number_type& value)
   {
      if (POLYMAKE_DEBUG) {
         if (digit >= my_limits.size())
            throw std::runtime_error("MultiDimCounter::set_digit - index out of range");
         if (value < my_start[digit] || value >= my_limits[digit])
            throw std::runtime_error("MultiDimCounter::set_digit - value out of range");
      }
      my_counter[digit] = value;
   }

   bool at_end() const { return my_at_end; }

   reference limits() const { return my_limits; }
};

template <bool left_to_right, typename number_type>
struct check_iterator_feature<MultiDimCounter<left_to_right, number_type>, end_sensitive> : std::true_type { };

template <bool left_to_right, typename number_type>
struct check_iterator_feature<MultiDimCounter<left_to_right, number_type>, rewindable> : std::true_type { };

template <bool left_to_right, typename number_type>
MultiDimCounter<left_to_right, number_type>&
MultiDimCounter<left_to_right, number_type>::operator++ ()
{
   const int last = my_limits.size()-1;

   if (left_to_right)
      for (int digit = 0; (++my_counter[digit]) >= my_limits[digit]; ++digit)
         if (digit < last) {
            my_counter[digit] = my_start[digit];
         } else {
            my_at_end = true;  break;
         }

   else
      for (int digit = last; (++my_counter[digit]) >= my_limits[digit]; --digit)
         if (digit > 0) {
            my_counter[digit] = my_start[digit];
         } else {
            my_at_end = true;  break;
         }

   return *this;
}
   
template <bool left_to_right, typename number_type>
MultiDimCounter<left_to_right, number_type>&
MultiDimCounter<left_to_right, number_type>::operator-- ()
{
   const int last = my_limits.size()-1;

   if (left_to_right)
      for (int digit = 0; (--my_counter[digit]) < my_start[digit]; ++digit)
         if (digit < last) {
            my_counter[digit] = my_limits[digit]-1;
         } else {
            my_at_end = true;  break;
         }

   else
      for (int digit = last; (--my_counter[digit]) < my_start[digit]; --digit)
         if (digit > 0) {
            my_counter[digit] = my_limits[digit]-1;
         } else {
            my_at_end = true;  break;
         }

   return *this;
}
   
} // end namespace pm

namespace polymake {
   using pm::MultiDimCounter;
}

#endif // POLYMAKE_MULTI_DIM_COUNTER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
