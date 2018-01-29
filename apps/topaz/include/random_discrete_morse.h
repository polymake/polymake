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

#ifndef POLYMAKE_TOPAZ_RANDOM_DISCRETE_MORSE_H
#define POLYMAKE_TOPAZ_RANDOM_DISCRETE_MORSE_H

namespace polymake { namespace topaz {

Map< Array<int>,int > random_discrete_morse(const Lattice<BasicDecoration> orig_HD, UniformlyRandom<long> seed, const int strategy, const bool verbose, const int rounds, const Array<int> try_until_reached,  const Array<int> try_until_exception, std::string save_to_filename );

}}

#endif // POLYMAKE_TOPAZ_RANDOM_DISCRETE_MORSE_H

