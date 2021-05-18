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

#include "polymake/topaz/complex_tools.h"
#include "polymake/FacetList.h"
#include "polymake/RandomSubset.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/list"
#include "polymake/hash_map"

namespace polymake { namespace topaz {

class BistellarComplex {
protected:
   typedef std::pair<Set<Int>, Set<Int>> option;

   class OptionsList {
   protected:
      Int the_size;
      hash_map<Set<Int>, Int> index_map;
      Array<option> the_options;

   public:
      OptionsList() : the_size(0) {};

      Int size() const
      {
         return the_size;
      }

      void insert(const option& opt)
      {
         if (the_options.size()==0)
            the_options.resize(1);
         if (the_size>=the_options.size())
            the_options.resize(2*the_options.size());
         the_options[the_size]=opt;
         index_map[opt.first]=the_size;
         ++the_size;
      }

      void insert(const Set<Int>& f, const Set<Int>& V)
      {
         insert(option(f, V));
      }

      void remove(const Set<Int>& f)
      {
         hash_map<Set<Int>, Int>::const_iterator find = index_map.find(f);
         if (find != index_map.end()) {
            const Int index = find->second;
            the_options[index] = the_options[the_size-1];
            index_map[the_options[the_size-1].first] = index;
            index_map.erase(f);
            --the_size;
         }
      }

      Array<option> options() const
      {
         return Array<option>(the_size, the_options.begin());
      }
   };

   void init(const Lattice<BasicDecoration>& HD);

   FacetList the_facets;
   UniformlyRandom<Integer> random_source;
   Int dim;
   Int verts;
   option next_move;
   Array<OptionsList> raw_options;
   Set<Int> rev_move;
   Array<Int> the_flip_vector;
   Int apex;
   bool allow_rev_move;
   bool verbose;
   bool closed;

public:
   BistellarComplex() : dim(0), verts(0), apex(0), allow_rev_move(false), verbose(false), closed(true) {}

   BistellarComplex(const Lattice<BasicDecoration>& HD, const RandomSeed& seed=RandomSeed(),
                    const bool verb=false, const bool in_closed=false, const bool in_allow_rev_move=false)
      : random_source(seed), dim(HD.rank()-2), verts(0),
        raw_options(dim+1), the_flip_vector((dim+1)/2),
        apex(0), allow_rev_move(in_allow_rev_move), verbose(verb), closed(in_closed)
   {
      init(HD);
   }

   BistellarComplex(const Lattice<BasicDecoration>& HD, const pm::SharedRandomState& random_arg,
                    const bool verb=false, const bool in_closed=false, const bool in_allow_rev_move=false)
      : random_source(random_arg), dim(HD.rank()-2), verts(0),
        raw_options(dim+1), the_flip_vector((dim+1)/2),
        apex(0), allow_rev_move(in_allow_rev_move), verbose(verb), closed(in_closed)
   {
      init(HD);
   }

   Int n_raw_options_of_dim(const Int d) const
   {
      return raw_options[d].options().size();
   }

   Int find_move(const Int dim_min, const Int dim_max);

   // Finds a reversed move of minimal dimension >= dim_min
   // and returns the dimension of the move. The move is != next_move.second
   // The move is stored in next_move.
   Int find_move(const Int dim_min)
   {
      return find_move(dim_min, dim);
   }

   Int find_move()
   {
      return find_move(0, dim);
   }

protected:
   // Determines if a face is an option.
   bool is_option(const Set<Int>& f, Set<Int>& V) const;

public:
   // Executes what ever move is set by find_move() or find_move(const Int).
   // You MUST set a move by using find_move() or find_move(const Int) before executing it.
   void execute_move();

   // Finds minimal revers move >= dim_min and executes it. Return the dimension of the move.
   Int min_max_rev_move(const Int dim_min, const Int dim_max)
   {
      const Int d = find_move(dim_min, dim_max);
      execute_move();
      return d;
   }

   // Finds minimal revers move >= dim_min and executes it. Return the dimension of the move.
   Int min_rev_move(const Int dim_min)
   {
      const Int d = find_move(dim_min);
      execute_move();
      return d;
   }

   Int min_rev_move()
   {
      return min_rev_move(0);
   }

   // A zero move.
   void zero_move()
   {
      min_rev_move(dim);
   }

   // The facets. Vertices are NOT numbered according to the topaz standard.
   FacetList facets() const
   {
      if (closed)
         return the_facets;

      // remove star(apex)
      FacetList real_facets = the_facets;
      real_facets.eraseSupersets(scalar2set(apex));

      return real_facets;
   }

   IncidenceMatrix<> as_incidence_matrix() const
   {
      FacetList F=facets();
      F.squeeze();
      return IncidenceMatrix<>(F.size(), F.cols(), F.begin());
   }

   Int n_facets() const
   {
      if (closed)
         return the_facets.size();

      return facets().size();
   }

   const Array<Int>& flip_vector() const
   {
      return the_flip_vector;
   }
};

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
