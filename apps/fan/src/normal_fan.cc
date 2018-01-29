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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/permutations.h"

namespace polymake { namespace fan {
      
template <typename Coord>
perl::Object normal_fan(perl::Object p)
{
   perl::ObjectType t=perl::ObjectType::construct<Coord>("PolyhedralFan");
   perl::Object f(t);

   const int dim   = p.call_method("AMBIENT_DIM");
   const int p_dim = p.call_method("DIM");
   const int ldim  = p.give       ("LINEALITY_DIM");

   if (!p.give("FEASIBLE")) {
      f.take("FAN_AMBIENT_DIM") << dim;
      f.take("FAN_DIM")         << -1;
      f.take("RAYS")            << Matrix<Coord>(0,dim);
      f.take("MAXIMAL_CONES")   << IncidenceMatrix<>(0,0);
      f.take("REGULAR")         << true;

      f.take("LINEALITY_SPACE") << Matrix<Coord>(0,dim);
      f.take("COMPLETE")        << false;
      return f;
   }
   
   Matrix<Coord> facets   = p.give("FACETS");
   IncidenceMatrix<> ftv  = p.give("FACETS_THRU_VERTICES");
   const Matrix<Coord> ls = p.give("AFFINE_HULL");
   const bool bounded     = p.give("BOUNDED");
   
   Array<Array<int>> gens;
   const bool has_group = p.lookup("GROUP");
   if (has_group) {
      gens = p.give("GROUP.FACETS_ACTION.GENERATORS");
   }
   
   struct id_collector {
      mutable Set<int> oldids;
      void operator() (const int& i, const int& j) const {
         oldids += i;
      }
   };

   // we remove the rows that correspond to far vertices
   // then we squeeze the cols to remove the far face
   // this also removes the facet inequality of polytopes with combinatorial dim zero
   if (!bounded || p_dim == 0) {
      const Set<int> bounded_verts = p.call_method("BOUNDED_VERTICES");
      ftv = ftv.minor(bounded_verts,All);

      // we only squeeze the cols since sometimes we want to keep an empty set
      // as this might be needed for the {0} cone
      id_collector coll;
      ftv.squeeze_cols(coll);
      facets = facets.minor(coll.oldids, ~scalar2set(0));

      if (has_group)
         gens = permutation_subgroup_generators(gens, coll.oldids);

   } else {
      facets = facets.minor(All,~scalar2set(0));
   }
   
   f.take("RAYS")            << facets;
   f.take("MAXIMAL_CONES")   << ftv;
   f.take("REGULAR")         << bounded;
   f.take("PSEUDO_REGULAR")  << true;
  
   f.take("LINEALITY_SPACE") << ls.minor(All, ~scalar2set(0));

   f.take("COMPLETE")        << bounded;
   f.take("FAN_DIM")         << dim - ldim;
   f.take("FAN_AMBIENT_DIM") << dim;

   if (has_group && gens.size()) {
      perl::Object a("group::PermutationAction");
      a.take("GENERATORS") << gens;

      perl::Object g("group::Group");
      g.set_description() << "symmetry group induced by the group of the original polytope" << endl;
      g.set_name("Aut");
      g.take("RAYS_ACTION") << a;
      f.take("GROUP") << g;
   }
   
   return f;
}

UserFunctionTemplate4perl("# @category Producing a fan"
                          "# Computes the normal fan of //p//."
                          "# @param Polytope p"
                          "# @tparam Coord"
                          "# @return PolyhedralFan",
                          "normal_fan<Coord>(polytope::Polytope<Coord>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
