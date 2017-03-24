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

#include "polymake/client.h"
#include "polymake/graph/Lattice.h"

namespace polymake { namespace graph {

   template <typename Decoration, typename SeqType>
      Array<Set<int> > lattice_dual_faces(perl::Object lattice_obj) {
         return Lattice<Decoration, SeqType>(lattice_obj).dual_faces();
      }

   template <typename Decoration, typename SeqType, typename Permutation>
      perl::Object lattice_permuted_faces(perl::Object lattice_obj, const Permutation& perm) {
         return (Lattice<Decoration, SeqType>(lattice_obj)).permuted_faces(perm).makeObject();
      }

   FunctionTemplate4perl("lattice_dual_faces<Decoration, SeqType>(Lattice<Decoration, SeqType>)");
   FunctionTemplate4perl("lattice_permuted_faces<Decoration, SeqType, Permutation>(Lattice<Decoration,SeqType>, Permutation)");
}}
