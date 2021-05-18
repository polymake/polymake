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

#include "polymake/client.h"
#include "polymake/ListMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace fan {

template <typename Scalar>
BigObject arrangement_from_cones(const Array<BigObject>& pp_in, OptionSet options)
{
   auto p_in = entire(pp_in);
   
   if (p_in.at_end())
      throw std::runtime_error("empty input");

   const Int dim = p_in->give("CONE_AMBIENT_DIM");

   /* 
   bool containsPolytope = false;
   bool containsCone = false;
   */

   ListMatrix< Vector<Scalar> > Inequalities(0, dim), Equations(0, dim);
   std::string descr_names;

   const Int k = pp_in.size(); // number of cones
   Array<Int>
      n_inequalities_per_cone(k), // number of inequalities contributed by i-th cone
      n_equations_per_cone(k); // number of equations contributed by i-th cone

   Int i=0;
   while (! p_in.at_end()) {
      // check if dimensions match
      const Int d = p_in->give("CONE_AMBIENT_DIM");
      if (d != dim) throw std::runtime_error("dimension mismatch");
      
      // take care of description
      if (!descr_names.empty())
         descr_names+=", ";
      descr_names+=p_in->name();

      // append inequalities and equations
      try {
         const Matrix<Scalar>
            F=p_in->give("FACETS | INEQUALITIES"),  // try to force the computation of facets
            AH=p_in->lookup("LINEAR_SPAN | EQUATIONS");
         Inequalities /= F;
         Inequalities /= AH;
         n_inequalities_per_cone[i] = F.rows();
         n_equations_per_cone[i] = AH.rows();
         // Equations /= AH;
      } catch (const Undefined&) {
         const Matrix<Scalar> AH=p_in->give("LINEAR_SPAN | EQUATIONS");
         Inequalities /= AH;
         n_inequalities_per_cone[i] = 0;
         n_equations_per_cone[i] = AH.rows();
         // Equations /= AH;
      }

      ++p_in;  ++i;
   }

   BigObject HA;

   // the optional arrangemnt ...
   if (options.exists("hyar")) {
      BigObject extra;
      options["hyar"] >> extra;
      Matrix<Scalar> extra_hyperplanes = extra.give("HYPERPLANES"); // ... may provide more hyperplanes
      Inequalities /= extra_hyperplanes;
      BigObject Supp;
      if (extra.lookup("SUPPORT") >> Supp) // ... or a support cone
         HA = BigObject("HyperplaneArrangement", mlist<Scalar>(), "HYPERPLANES", Inequalities, "SUPPORT", Supp);
      else 
         HA = BigObject("HyperplaneArrangement", mlist<Scalar>(), "HYPERPLANES", Inequalities);
      HA.attach("N_EXTRA_HYPERPLANES") << extra_hyperplanes.rows();
   } else {
      HA = BigObject("HyperplaneArrangement", mlist<Scalar>(), "HYPERPLANES", Inequalities);
      HA.attach("N_EXTRA_HYPERPLANES") << 0;
   }
   
   HA.attach("N_INEQUALITIES_PER_CONE") << n_inequalities_per_cone;
   HA.attach("N_EQUATIONS_PER_CONE") << n_equations_per_cone;
   
   return HA;
}


template <typename Scalar>
BigObject union_of_cones(const Array<BigObject>& pp_in, OptionSet options)
{
   BigObject HA = arrangement_from_cones<Scalar>(pp_in, options);
   const Int
      n_cones = pp_in.size(),
      dim = pp_in[0].give("CONE_AMBIENT_DIM");
   const Matrix<Scalar>& Hyperplanes = HA.give("HYPERPLANES");
   const Matrix<Scalar> NegativeHyperplanes = -Hyperplanes;
   const Int n_hyperplanes = Hyperplanes.rows();
   const Set<Int> all_hyperplanes = sequence(0,n_hyperplanes);
   const IncidenceMatrix<> Signatures = HA.give("CHAMBER_SIGNATURES");
   const Array<Int>
      n_inequalities_per_cone = HA.get_attachment("N_INEQUALITIES_PER_CONE"),
      n_equations_per_cone = HA.get_attachment("N_EQUATIONS_PER_CONE");

   if (n_inequalities_per_cone.size()!=n_cones || n_equations_per_cone.size()!=n_cones)
      throw std::runtime_error("length mismatch");

   ListMatrix< Vector<Scalar> > Rays(0, dim); // current version lists the same rays too often
   std::list<Set<Int>> rays_per_cell;

   // For each input cone we search for the cells contained.
   // It would be simpler, if all cones were full-dimensional, which we do not assume.
   // Here, however, we need to construct the lower-dimensional cells as faces of maximal cells of the arrangement.

   Set<Int> maximal_cells_found;
   
   Int nh=0, nr=0;
   for (Int i=0; i<n_cones; ++i) {
      const Int
         ni = n_inequalities_per_cone[i],
         ne = n_equations_per_cone[i];

      // maximal cells of arrangement contained in cone, provided that the cone is full-dimensional
      const Set<Int> cells_in_cone = accumulate(cols(Signatures.minor(All,sequence(nh,ni+ne))), operations::mul());

      if (pp_in[i].give("FULL_DIM"))
         maximal_cells_found += cells_in_cone; // take care of the maximal cells later (to avoid duplication)
      else {
         for (auto c_it(entire(cells_in_cone)); !c_it.at_end(); ++c_it) {
            Set<Int> these_hyperplanes = Signatures.row(*c_it);
            // take inequalities of maximal cell and intersect with equations from cone
            BigObject C("Cone", mlist<Scalar>(),
                        "INEQUALITIES", Hyperplanes.minor(these_hyperplanes,All) / NegativeHyperplanes.minor(all_hyperplanes-these_hyperplanes,All), 
                        "EQUATIONS", Hyperplanes.minor(sequence(nh+ni,ne),All));
            const Matrix<Scalar>& these_rays = C.give("RAYS");
            Rays /= these_rays;
            const Int n_these_rays = these_rays.rows();;
            rays_per_cell.push_back(sequence(nr,n_these_rays));
            nr += n_these_rays;
         }
      }
      
      nh += ni+ne;
   }

   // finally process the maximal cells
   for (auto c_it(entire(maximal_cells_found)); !c_it.at_end(); ++c_it) {
      Set<Int> these_hyperplanes = Signatures.row(*c_it);
      BigObject C("Cone", mlist<Scalar>(),
                  "INEQUALITIES", Hyperplanes.minor(these_hyperplanes,All) / NegativeHyperplanes.minor(all_hyperplanes-these_hyperplanes,All));
      const Matrix<Scalar>& these_rays = C.give("RAYS");
      Rays /= these_rays;
      const Int n_these_rays = these_rays.rows();;
      rays_per_cell.push_back(sequence(nr,n_these_rays));
      nr += n_these_rays;
   }

   BigObject U("PolyhedralFan", mlist<Scalar>(), "INPUT_RAYS", Rays, "INPUT_CONES", rays_per_cell);
   return U;
}
      
UserFunctionTemplate4perl("# @category Producing a hyperplane arrangement"
                          "# Construct a new hyperplane arrangement from the exterior descriptions of given cones."
                          "# Optional HyperplaneArrangemnt for further subdivision or support."
                          "# Also applies to polytopes, via homogenization.  The output is always homogeneous."
                          "# Works only if all [[CONE_AMBIENT_DIM]] values are equal."
                          "# @param Cone C ... cones to be added to arrangement"
                          "# @option HyperplaneArrangement hyar"
                          "# @return HyperplaneArrangement"
                          "# @example [prefer cdd]"
                          "# > $C = new Cone(INPUT_RAYS=>[[1,0],[2,3]]); $D = new Cone(INPUT_RAYS=>[[0,1],[3,2]]);"
                          "# > $HA = arrangement_from_cones($C,$D);"
                          "# > print $HA->HYPERPLANES;"
                          "# | 3/2 -1"
                          "# | 0 1"
                          "# | 1 0"
                          "# | -1 3/2"
                          "# > print $HA->get_attachment(\"N_INEQUALITIES_PER_CONE\");"
                          "# | 2 2"
                          "# > print $HA->get_attachment(\"N_EQUATIONS_PER_CONE\");"
                          "# | 0 0",
                          "arrangement_from_cones<Scalar>(Cone<type_upgrade<Scalar>> +; { hyar => undef } )");
      
UserFunctionTemplate4perl("# @category Producing a fan"
                          "# Construct a new polyhedral fan whose support is the union of given cones."
                          "# Optional HyperplaneArrangemnt for further subdivision or support."
                          "# Also applies to polytopes, via homogenization.  The output is always homogeneous."
                          "# Works only if all [[CONE_AMBIENT_DIM]] values are equal."
                          "# @param Cone C ... cones to be added to union"
                          "# @option HyperplaneArrangement arr"
                          "# @return PolyhedralFan"
                          "# @example [prefer cdd] [require bundled:cdd]"
                          "# > $C = new Cone(INPUT_RAYS=>[[1,0],[2,3]]); $D = new Cone(INPUT_RAYS=>[[0,1],[3,2]]);"
                          "# > $U = union_of_cones($C,$D);"
                          "# > print rows_numbered($U->RAYS);"
                          "# | 0:1 2/3"
                          "# | 1:1 0"
                          "# | 2:1 3/2"
                          "# | 3:0 1"
                          "# > print $U->MAXIMAL_CONES;"
                          "# | {0 1}"
                          "# | {0 2}"
                          "# | {2 3}",
                          "union_of_cones<Scalar>(Cone<type_upgrade<Scalar>> +; { hyar => undef } )");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
