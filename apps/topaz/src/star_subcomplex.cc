/* Copyright (c) 1997-2023
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

#include "polymake/client.h"
#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {
  
BigObject star_subcomplex(BigObject p_in, const Set<Int> &F,OptionSet options)
{
   const bool relabel = !options["no_labels"];
   const Array<Set<Int>> C = p_in.give("FACETS");
   //   const Array<std::string> L = p_in.give("VERTEX_LABELS");
   const Int n_vert = p_in.give("N_VERTICES");
   
   //   if (F.empty())  // face spefified by labels
   //  F = transform_label_args(L, labeled_F);

   if (F.front()<0 || F.back()>n_vert-1)
      throw std::runtime_error("star_subcomplex: Specified vertex indices out of range");

   std::list<Set<Int>> Star;
   copy_range(entire(star(C,F)), std::back_inserter(Star));

   if (Star.empty()) {
      std::ostringstream e;
      wrap(e) << "star_subcomplex: " << F << " does not specify a face.";
      throw std::runtime_error(e.str());
   }
   
   const Set<Int> V = accumulate(Star, operations::add());
   adj_numbering(Star,V);
   
   BigObject p_out("topaz::SimplicialComplex");
   p_out.set_description()<<"Star of " << F << " in " << p_in.name()<< "."<<endl;

   p_out.take("FACETS") << as_array(Star);
   p_out.take("VERTEX_INDICES") << V;
   
   if (relabel) {
      const Array<std::string> L = p_in.give("VERTEX_LABELS");
      const Array<std::string> new_L(V.size(), select(L,V).begin());
      p_out.take("VERTEX_LABELS") << new_L;
   }
   return p_out;
}

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Produce the __star__ of the //face// of the //complex//.\n"
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "# @param SimplicialComplex complex"
                  "# @param Set<Int> face"
                  "# @return SimplicialComplex"
                  "# @example The following returns the cone over the 4-cycle obtained as the star of vertex 0 in the suspension over the triangle."
                  "# > $s = suspension(simplex(2) -> BOUNDARY);"
                  "# > $t = star_subcomplex($s, [0]);"
                  "# > print $t -> F_VECTOR;"
                  "# | 5 8 4",
                  &star_subcomplex, "star_subcomplex(SimplicialComplex $ { no_labels => 0 })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
