/* Copyright (c) 1997-2016
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

#ifndef POLYMAKE_PERL_ASSOC_H
#define POLYMAKE_PERL_ASSOC_H

#include "polymake/internal/iterators.h"
#include "polymake/perl/constants.h"
#include "polymake/perl/Value.h"

namespace pm { namespace perl {

template <typename Container, bool _simple=check_iterator_feature<typename container_traits<Container>::iterator, end_sensitive>::value>
class element_finder_helper {
protected:
   typename container_traits<Container>::iterator pos;
public:
   element_finder_helper(Container&, typename container_traits<Container>::iterator where)
      : pos(where) {}
};

template <typename Container>
class element_finder_helper<Container, false> {
protected:
   typename Entire<Container>::iterator pos;
public:
   element_finder_helper(Container& c, typename container_traits<Container>::iterator where)
      : pos(entire(c)) { pos=where; }
};

template <typename Container>
class element_finder
   : protected element_finder_helper<const Container>
   , public MaybeUndefined< element_finder<Container> > {
public:
   template <typename Key>
   element_finder(const Container& c, const Key& key)
      : element_finder_helper<const Container>(c, c.find(key)) {}

   bool defined() const { return !this->pos.at_end(); }

   decltype(auto) get_val() const { return this->pos->second; }
};

template <typename Container, typename Key> inline
element_finder<Container> find_element(const Container& c, const Key& key)
{
   return element_finder<Container>(c, key);
}

template <typename Container>
class delayed_eraser
   : protected element_finder_helper<Container>
   , public MaybeUndefined< delayed_eraser<Container> > {
public:
   template <typename Key>
   delayed_eraser(Container& c_arg, const Key& key)
      : element_finder_helper<Container>(c_arg, c_arg.find(key))
      , c(c_arg) {}

   bool defined() const { return !this->pos.at_end(); }

   decltype(auto) get_val() const { return std::move(this->pos->second); }

   ~delayed_eraser()
   {
      if (defined()) c.erase(this->pos);
   }
private:
   Container& c;
};

template <typename Container, typename Key> inline
delayed_eraser<Container> delayed_erase(Container& c, const Key& key)
{
   return delayed_eraser<Container>(c, key);
}

} }

#endif // POLYMAKE_PERL_ASSOC_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
