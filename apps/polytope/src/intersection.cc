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

namespace polymake { namespace polytope {

template <typename Scalar>
BigObject intersection(const Array<BigObject>& pp_in)
{
   auto p_in = entire(pp_in);

   if (p_in.at_end()) throw std::runtime_error("empty input");

   bool containsPolytope = false;
   bool containsCone = false;
   const Int dim = p_in->give("CONE_AMBIENT_DIM");
   ListMatrix< Vector<Scalar> > Inequalities(0, dim), Equations(0, dim);
   std::string descr_names;

   while (! p_in.at_end()) {
      // check if dimensions match
      const Int d = p_in->give("CONE_AMBIENT_DIM");
      if (d != dim) throw std::runtime_error("dimension mismatch");

      // check if Polytope or true Cone
      if (p_in->isa("Polytope")) {
         containsPolytope=true;
      } else {
         containsCone=true;
      }

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
         Equations /= AH;
      } catch (const Undefined&) {
         const Matrix<Scalar> AH=p_in->give("LINEAR_SPAN | EQUATIONS");
         Equations /= AH;
      }

      ++p_in;
   }

   BigObjectType t(containsPolytope ? Str("Polytope") : Str("Cone"), mlist<Scalar>());
   BigObject p_out(t,
                   "INEQUALITIES", Inequalities,
                   "EQUATIONS", Equations,
                   "CONE_AMBIENT_DIM", dim);

   if (containsCone) {
      if (containsPolytope)
         p_out.set_description() << "Intersection of cones and polytopes " << descr_names << endl;
      else
         p_out.set_description() << "Intersection of cones " << descr_names << endl;
   } else {
      // since input non-empty the only case left ...
      p_out.set_description() << "Intersection of polytopes " << descr_names << endl;
   }

   return p_out;
}

template <typename Scalar, typename Matrix1, typename Matrix2, typename Matrix3>
bool
cone_intersects_subspace(const GenericMatrix<Matrix1,Scalar> &cone_ineqs,
                         const GenericMatrix<Matrix2,Scalar> &cone_eqs,
                         const GenericMatrix<Matrix3,Scalar> &subspace_eqs)
{
   return true;
}
      
      
UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Construct a new polyhedron or cone as the intersection of given polyhedra and/or cones."
                          "# Works only if all [[CONE_AMBIENT_DIM]] values are equal."
                          "# If the input contains both cones and polytopes, the output will be a polytope."
                          "# @param Cone C ... polyhedra and cones to be intersected"
                          "# @return Cone"
                          "# @example [prefer cdd] [require bundled:cdd]"
                          "# > $p = intersection(cube(2), cross(2,3/2));"
                          "# > print $p->VERTICES;"
                          "# | 1 -1/2 1"
                          "# | 1 -1 1/2"
                          "# | 1 1/2 1"
                          "# | 1 1 1/2"
                          "# | 1 1/2 -1"
                          "# | 1 1 -1/2"
                          "# | 1 -1/2 -1"
                          "# | 1 -1 -1/2",
                          "intersection<Scalar>(Cone<type_upgrade<Scalar>> +)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
