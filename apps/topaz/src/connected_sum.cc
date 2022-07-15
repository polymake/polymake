/* Copyright (c) 1997-2022
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
#include "polymake/topaz/connected_sum.h"

namespace polymake { namespace topaz {

BigObject connected_sum_complex(BigObject p_in1, BigObject p_in2, const Int f1, const Int f2, OptionSet options)
{
   const bool no_labels=options["no_labels"];
   const Array<Set<Int>> C1 = p_in1.give("FACETS");
   Array<std::string> L1;  // connected_sum interprets empty labels as no labels
   if (!no_labels) p_in1.give("VERTEX_LABELS") >> L1;

   const Array<Set<Int>> C2 = p_in2.give("FACETS");
   Array<std::string> L2;
   if (!no_labels) p_in2.give("VERTEX_LABELS") >> L2;
   
   if (f1 >= C1.size()) {
      std::ostringstream e;
      e << "connected_sum: " << f1 << " is not a facet.";
      throw std::runtime_error(e.str());
   }

   if (f2 >= C2.size()) {
      std::ostringstream e;
      e << "connected_sum: " << f2 << " is not a facet.";
      throw std::runtime_error(e.str());
   }

   if (C1[f1].size() != C2[f2].size()) {
      std::ostringstream e;
      e << "connected_sum: " << f1 << " and " << f2
        << " do not have the same dimension.";
      throw std::runtime_error(e.str());
   }


   // set permutation: connected_sum interprets empty permutation as trivial
   hash_map<Int, Int> Perm;

   BigObject p_out("SimplicialComplex");

   // testing P
   Array<Int> P;
   if (options["permutation"]>>P) {

      Set<Int> V;
      accumulate_in(entire(P), operations::add(), V);
      
      if (incl(V, C1[f1]) != 0) {
         std::ostringstream e;
         e << "connected_sum: Specified permutation does not match facets " << f1 << ".";
         throw std::runtime_error(e.str());
      }
      
      auto v = C1[f1].begin();
      for (auto p = entire(P); !p.at_end(); ++p, ++v)
         Perm[*v] = *p;
      p_out.set_description() << "Connected sum of " << p_in1.name() << ", using facet " << f1
                              << ",\nand " << p_in2.name() << ", using facet " << f2 << "." << endl
                              << "The vertices of facet " << f2 << " of " << p_in2.name() << " are permuted " << P << "."<<endl;
   } else {
      p_out.set_description() << "Connected sum of " << p_in1.name() << ", using facet " << f1
                              << ",\nand " << p_in2.name() << ", using facet " << f2 << "." << endl;
   }
   p_out.take("FACETS") << as_array(connected_sum(C1, C2, f1, f2, L1, L2, Perm));

   if (!no_labels)
      p_out.take("VERTEX_LABELS") << L1; 
   return p_out;
}

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Compute the __connected sum__ of two complexes.\n"
                  "# "
                  "# Parameters //f_1// and //f_2// specify which facet of the first and second complex correspondingly are glued together.\n"
                  "# Default is the 0-th facet of both.\n"
                  "# "
                  "# The vertices in the selected facets are identified with each other according to their order in the facet\n"
                  "# (that is, in icreasing index order). The glueing facet iteself is not included in the connected sum.\n"
                  "#  The option //permutation// allows one to get an alternative identification. It should specify a\n"
                  "# permutation of the vertices of the second facet.\n"
                  "# "
                  "# The vertices of the new complex get the original labels with ''_1'' or ''_2'' appended, according to the input complex\n"
                  "#  they came from.  If you set the //no_labels// flag, the label generation will be omitted.\n"
                  "# @param SimplicialComplex complex1"
                  "# @param SimplicialComplex complex2"
                  "# @param Int f1 default: 0"
                  "# @param Int f2 default: 0"
                  "# @option Array<Int> permutation"
                  "# @option Bool no_labels"
                  "# @return SimplicialComplex"
                  "# @example Glueing together two tori to make a genus 2 double torus, rotating the second one clockwise:"
                  "# > $cs = connected_sum(torus(),torus(),permutation=>[1,2,0]);"
                  "# > print $cs->SURFACE.','.$cs->GENUS;"
                  "# | 1,2",
                  &connected_sum_complex,"connected_sum(SimplicialComplex SimplicialComplex; $=0,$=0, { permutation => undef, no_labels => 0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
