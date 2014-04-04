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
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/list"
#include "polymake/hash_set"
#include <sstream>

namespace polymake { namespace topaz {



perl::Object stellar_subdivision(perl::Object p_in, const Array<Set<int> >& subd_faces, perl::OptionSet options)
{
   const bool is_PC= !p_in.isa("topaz::SimplicialComplex");

  
   Array< Set<int> > C_in;
   if (is_PC)  p_in.give("TRIANGULATION.FACETS")>>C_in;
   else 
      p_in.give("FACETS")>>C_in;
   int n_vert;
   if (is_PC)p_in.give("N_POINTS")>>n_vert;
   else
      p_in.give("N_VERTICES")>>n_vert;
  
   // compute new complex
   std::list< Set<int> > C;
  
   for (int i=0; i<subd_faces.size(); ++i) {
      const Set<int> F = subd_faces[i];
      bool facet_found = false;
      for (Entire< Array< Set<int> > >::const_iterator f=entire(C_in);!f.at_end(); ++f) {
         Set<int> facet = *f;

         if (incl(F,facet)<1) {  // F contained in the facet
            facet_found = true;
            const int size = facet.size();
            facet -= F;
            facet += n_vert+i;
         
            // add new facets
            Subsets_of_k< const Set<int> > enumerator( F, size-facet.size() );     
            for (Entire< Subsets_of_k< const Set<int> > >::iterator s_it=entire(enumerator);
                 !s_it.at_end(); ++s_it)
               C.push_back(facet + *s_it);
         
         } else
            C.push_back(facet);
      }
     
      if (!facet_found) {
         throw std::runtime_error("stellar_subdivision: Input does not specify a face (of the complex generated so far).");
      }
      C_in = Array< Set<int> >(C.size(),C.begin());
      C.clear();
   }

   perl::Object p_out(p_in.type());
   p_out.set_description()<<"Obtained from " << p_in.name() << " by barycentric subdivision of the "
      "faces"<<endl<< subd_faces<<"."<<endl;
   if (is_PC) p_out.take("TRIANGULATION.FACETS") << as_array(C_in);
   else 
      p_out.take("FACETS") << as_array(C_in);
  
   // compute new coordinates
   if (is_PC) {
      Matrix<Rational> Coord = p_in.give("POINTS");
      Coord.resize(n_vert+subd_faces.size(),Coord.cols());
      for (int i=0; i<subd_faces.size(); ++i)
         Coord[i+n_vert] = average( rows(Coord.minor(subd_faces[i], All)) );
    
      p_out.take("POINTS") << Coord;
   }
   else 
      if (options["geometric_realization"]) {
         Matrix<Rational> Coord = p_in.give("COORDINATES");
         Coord.resize(n_vert+subd_faces.size(),Coord.cols());
         for (int i=0; i<subd_faces.size(); ++i)
            Coord[i+n_vert] = average( rows(Coord.minor(subd_faces[i], All)) );
    
         p_out.take("COORDINATES") << Coord;
      }
   
   // compute new label
   if (!options["no_labels"]) {
      Array<std::string> L;
      if (is_PC) p_in.give("LABELS")>>L;
      else 
         p_in.give("VERTEX_LABELS")>>L;
      hash_set<std::string> old_L(n_vert);
      for (Entire< Array<std::string> >::const_iterator l=entire(L); !l.at_end(); ++l)
         old_L.insert(*l);
    
      L.resize(n_vert+subd_faces.size());
      for (int i=0; i<subd_faces.size(); ++i) {
         std::ostringstream label;
         Entire< Set<int> >::const_iterator v=entire(subd_faces[i]);
         label << "{" << L[*v];  ++v;
         for ( ; !v.at_end(); ++v)
            label << "," << L[*v];
         label << "}";
         std::string l=label.str(), ll=l;
      
         // test if ll is unique
         int j=0;
         while (old_L.find(ll) != old_L.end()) {
            ++j;
            label.str("");
            label << l << "_" << j;
            ll=label.str();
         }
      
         L[n_vert+i] = ll;
      }
      if (is_PC) p_out.take("LABELS") << L;
      else
         p_out.take("VERTEX_LABELS") << L;
   }
   return p_out;
}

UserFunction4perl("# @category  Producing a new simplicial complex from others"
                  "# Computes the complex obtained by stellar subdivision of the given //faces// of the //complex//."
                  "# @param SimplicialComplex complex"
                  "# @param Array<Set<Int>> faces"
                  "# @option Bool no_labels"
                  "# @option Bool geometric_realization default 0"
                  "# @return SimplicialComplex",
                  &stellar_subdivision,"stellar_subdivision($,Array<Set<Int> > { no_labels => 0, geometric_realization => 0})"); 

InsertEmbeddedRule("# @category  Producing a new simplicial complex from others"
                   "# Computes the complex obtained by stellar subdivision of the given //face// of the //complex//."
                   "# @param SimplicialComplex complex"
                   "# @param Set<Int> face"
                   "# @option Bool no_labels"
                   "# @option Bool geometric_realization default 0"
                   "# @return SimplicialComplex\n"
                   "user_function stellar_subdivision(SimplicialComplex, Set<Int> { no_labels => 0, geometric_realization => 0}) { "
                   " my $a=new Array<Set<Int> >(1); "
                   " my $p=shift; "
                   " $a->[0]=shift; "
                   "stellar_subdivision($p,$a,@_); }\n");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
