/* Copyright (c) 1997-2014
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
#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {
  
perl::Object stars(perl::Object p_in, const Set<int> &F,perl::OptionSet options)
{
   const bool relabel=options["labels"];
   const Array< Set<int> > C = p_in.give("FACETS");
   //   const Array<std::string> L = p_in.give("VERTEX_LABELS");
   const int n_vert = p_in.give("N_VERTICES");
   
   //   if (F.empty())  // face spefified by labels
   //  F = transform_label_args(L, labeled_F);

   if (F.front()<0 || F.back()>n_vert-1)
      throw std::runtime_error("t_star: Specified vertex indices out of range");

   std::list< Set<int> > Star;
   copy(entire(star(C,F)), std::back_inserter(Star));

   if (Star.empty()) {
      std::ostringstream e;
      wrap(e) << "t_star: " << F << " does not specify a face.";
      throw std::runtime_error(e.str());
   }
   
   const Set<int> V = accumulate(Star, operations::add());
   adj_numbering(Star,V);
   
   perl::Object p_out("topaz::SimplicialComplex");
   p_out.set_description()<<"Star of " << F << " in " << p_in.name()<< "."<<endl;

   p_out.take("FACETS") << as_array(Star);
   
   if (relabel) {
      const Array<std::string> L = p_in.give("VERTEX_LABELS");
      const Array<std::string> new_L(V.size(), select(L,V).begin());
      p_out.take("VERTEX_LABELS") << new_L;
   }
   return p_out;
}

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Produce the __star__ of the //face// of the //complex//.\n"
                  "# @option Bool labels creates [[VERTEX_LABELS]].\n"
                  "# @param SimplicialComplex complex"
                  "# @param Set<int> face"
                  "# @return SimplicialComplex",
                  &stars, "star(SimplicialComplex $ { labels => 0 })"); 

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
