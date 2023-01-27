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
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

template <typename Scalar>
BigObject polarize(BigObject p_in, OptionSet options)
{
   bool
      RIF_property_read = false,
      rays_property_read = false,
      facets_property_read = false;
   const bool no_coordinates = options["no_coordinates"];
   const bool isCone = !p_in.isa("Polytope");

   BigObjectType t = p_in.type();
   BigObject p_out(t);
   if (isCone)
      p_out.set_description() << "Cone dualized from " << p_in.name() << endl;
   else
      p_out.set_description() << "Polytope polarized from " << p_in.name() << endl;

   if (no_coordinates || p_in.exists("RAYS_IN_FACETS")) {
      const IncidenceMatrix<> VIF = p_in.give("RAYS_IN_FACETS");
      p_out.take("RAYS_IN_FACETS") << T(VIF);
      RIF_property_read = true;
   }

   if (!no_coordinates) {
      const Int ambient_dim = p_in.give("CONE_AMBIENT_DIM");

      if ( !isCone ) {
         const bool is_centered = p_in.give("WEAKLY_CENTERED");
         if (!is_centered)
            throw std::runtime_error("polarize: polytope must contain the origin");
      }

      Matrix<Scalar> ineq, eq, pts, lin;
      std::string
         rays_property,
         facets_property,
         eq_property;

      // We first read the input properties that are present,
      // and convert RAYS|INPUT_RAYS to FACETS|INEQUALITIES.

      if (p_in.lookup_with_property_name("RAYS | INPUT_RAYS", rays_property) >> ineq) {
         p_out.take( rays_property == "RAYS" || rays_property == "VERTICES"
                     ? Str("FACETS")
                     : Str("INEQUALITIES") ) << ineq;
         rays_property_read = true;
      }

      // Next, we read the FACETS|INEQUALITIES, but don't write them out yet.
      // The reason is that if the polytope is not full-dimensional,
      // we have to project any FACETS|INEQUALITIES to the AFFINE_SPAN before
      // writing them out as RAYS|INPUT_RAYS.

      if (p_in.lookup_with_property_name("FACETS | INEQUALITIES", facets_property) >> pts)
         facets_property_read = true;


      // if none of these properties has been read,
      // the input does not define a valid geometric Cone or Polytope
      if (!RIF_property_read && !rays_property_read && !facets_property_read)
         throw std::runtime_error("polarize: no useful coordinate section found in input");


      // the following properties may exist, and if so, are essential to describe the polyhedron
      // but they are not valid without one of the previous properties
      if (p_in.lookup_with_property_name("LINEALITY_SPACE | INPUT_LINEALITY", eq_property) >> eq)
         p_out.take( eq_property=="LINEALITY_SPACE" ? Str("LINEAR_SPAN") : Str("EQUATIONS")) << eq;


      // now we write out the lineality
      if (p_in.lookup_with_property_name("LINEAR_SPAN | EQUATIONS", eq_property) >> lin)
         p_out.take( eq_property=="LINEAR_SPAN" || eq_property=="AFFINE_HULL" ? Str("LINEALITY_SPACE") : Str("INPUT_LINEALITY")) << lin;

      // ... and the facet property
      if (facets_property_read) {
         orthogonalize(entire(rows(lin)));
         project_to_orthogonal_complement(pts, lin); // cheap if lin==0
         p_out.take( facets_property == "FACETS" ? Str("RAYS") : Str("INPUT_RAYS") ) << remove_zero_rows(pts);
      }

      // if p was obtained by some transformation,
      // then the reverse transformation also makes sense for the polar
      Matrix<Scalar> tau;
      if (p_in.get_attachment("REVERSE_TRANSFORMATION") >> tau)
         p_out.attach("REVERSE_TRANSFORMATION") << T(inv(tau));

      p_out.take("CONE_AMBIENT_DIM") << ambient_dim;

      Int ldim, cdim;
      if (p_in.lookup("LINEALITY_DIM") >> ldim)
         p_out.take("CONE_DIM") << ambient_dim-ldim;

      if (p_in.lookup("CONE_DIM") >> cdim)
         p_out.take("LINEALITY_DIM") << ambient_dim-cdim;
   }

   // generate the combinatorial symmetry group on the facets
   Array<Array<Int>> gens;
   if (p_in.lookup("GROUP.FACETS_ACTION.GENERATORS") >> gens) {
      BigObject a("group::PermutationAction", "GENERATORS", gens);
      BigObject g("group::Group", "fullCombinatorialGroupOnRays");
      g.set_description() << "symmetry group on vertices of polar of" << p_in.name() << endl;
      p_out.take("GROUP") << g;
      if (isCone) {
         p_out.take("GROUP.RAYS_ACTION") << a;
      } else {
         p_out.take("GROUP.VERTICES_ACTION") << a;
      }
   }
   else if (p_in.lookup("GROUP.RAYS_ACTION.GENERATORS") >> gens) {
      BigObject a("group::PermutationAction", "GENERATORS", gens);
      BigObject g("group::Group", "fullCombinatorialGroupOnFacets");
      g.set_description() << "symmetry group on facets of polar of" << p_in.name() << endl;
      g.take("FACETS_ACTION") << a;
      p_out.take("GROUP") << g;
   }

   Array<std::string> labels;
   if (p_in.lookup("RAY_LABELS") >> labels)
      p_out.take("FACET_LABELS") << labels;

   if (p_in.lookup("FACET_LABELS") >> labels)
      p_out.take("RAY_LABELS") << labels;

   return p_out;
}

template <typename Scalar>
BigObject dual_cone(BigObject p_in, OptionSet options)
{
   return polarize<Scalar>(p_in, options);
}

UserFunctionTemplate4perl("# @category Transformations"
                          "# This method takes either a polytope (1.) or a cone (2.) as input."
                          "# 1. Given a bounded, centered, not necessarily full-dimensional "
                          "# polytope //P//, produce its polar with respect to the "
                          "# standard Euclidean scalar product."
                          "# 2. Given a cone //C// produce its dual with respect to the "
                          "# standard Euclidean scalar product, i.e. all the vectors "
                          "# that evaluate positively on //C//."
                          "# Note that the definition of the polar has changed after version 2.10: "
                          "# the polar is reflected in the origin to conform with cone polarization"
                          "# If //P// is not full-dimensional, the output will contain lineality "
                          "# orthogonal to the affine span of //P//. "
                          "# In particular, polarize() of a pointed polytope will always produce "
                          "# a full-dimensional polytope. "
                          "# If you want to compute the polar inside the affine hull you may "
                          "# use the [[pointed_part]] client afterwards."
                          "# @param Cone C"
                          "# @option Bool no_coordinates only combinatorial information is handled"
                          "# @return Cone"
                          "# @example To save the polar of the square in the variable $p and then print its vertices, do this:"
                          "# > $p = polarize(cube(2));"
                          "# > print $p->VERTICES;"
                          "# | 1 1 0"
                          "# | 1 -1 0"
                          "# | 1 0 1"
                          "# | 1 0 -1"
                          "# @example [prefer cdd] [require bundled:cdd]"
                          "# To dualize the cone over a hexagon and print its rays, do this:"
                          "# > $c = new Cone(INPUT_RAYS=>[[1,0,0],[1,1,0],[1,2,1],[1,2,2],[1,1,2],[1,0,1]]);"
                          "# > $cd = polarize($c);"
                          "# > print $cd->RAYS;"
                          "# | 1 -1 1"
                          "# | 0 0 1"
                          "# | 0 1 0"
                          "# | 1 1 -1"
                          "# | 1 0 -1/2"
                          "# | 1 -1/2 0",
                          "polarize<Scalar> (Cone<Scalar>, { no_coordinates => 0 })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
