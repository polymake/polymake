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
#include "polymake/vector"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/common/labels.h"
#include <sstream>

namespace polymake { namespace polytope {
namespace {

struct wreath_label {
   typedef std::string first_argument_type;
   typedef std::string second_argument_type;
   typedef std::string result_type;
    
   result_type operator() (first_argument_type l1, second_argument_type l2) const
   {
      return l2 + '^' + l1;
   }
};

template <typename Coord>
Matrix<Coord>
wreath_coord(const Matrix<Coord>& V1, const Matrix<Coord>& V2)
{
   const Int n_vertices1 = V1.rows(), n_vertices2 = V2.rows(),
      dim1 = V1.cols()-1, dim2 = V2.cols()-1,
      d = n_vertices2*dim1+dim2+1;
   Matrix<Coord> V(n_vertices1*n_vertices2, d);
    
   for (Int i = 0; i < n_vertices2; ++i) {
      for (Int k = 0; k < n_vertices1; ++k) {
         const Int ik = i*n_vertices1+k;
         V(ik,0) = Coord(1);
         const Int id1 = i*dim1; 
         Int j = 1;

         //v_{ik,j} = 0 for 0 <= j <= k*dim1
         for ( ; j<=id1; ++j) V(ik,j)=0;

         //v_{ik,j} = v1_{i, j-k*dim1} for k*dim1 < j <= (k+1)*dim1
         for ( ; j<=id1+dim1; ++j) V(ik,j)=V1(k,j-id1)/V1(k,0);

         // v_{ik,j} = 0 for (k+1)*dim1 < j <= n_vertices*dim1
         for ( ; j<=n_vertices2*dim1; ++j) V(ik,j)=0;
        
         //v_{ik,j} = v2_{k,j-n_vertices2*dim1} for n_vertices2*dim1 < j < d
         for ( ; j<d; ++j) V(ik,j)=V2(i,j-n_vertices2*dim1)/V2(i,0);
      }
   }
   return V;
}
  
} // end unnamed namespace

template <typename Coord>
BigObject wreath(BigObject p_in1, BigObject p_in2, OptionSet options)
{
   const bool dual=options["dual"];
   const bool relabel=!options["no_labels"];

   const bool bounded1=p_in1.give("BOUNDED"),
      centered1 = p_in1.give("CENTERED");
   if (!bounded1 || !centered1) {
      throw std::runtime_error("wreath: first polytope must be BOUNDED and CENTERED.");
   }
   const bool bounded2=p_in2.give("BOUNDED"),
      centered2 = p_in2.give("CENTERED");
   if (!bounded2 || !centered2) {
      throw std::runtime_error("wreath: second polytope must be BOUNDED and CENTERED.");
   }
  
   const std::string coord_section(dual ? "FACETS" : "VERTICES");

   const Matrix<Coord> M1=p_in1.give(coord_section),
                       M2=p_in2.give(coord_section);
   const Int n_rows1 = M1.rows(),
             n_rows2 = M2.rows(),
             n_rows_out = n_rows1*n_rows2;

   const Matrix<Coord> M_out = wreath_coord<Coord>(M1, M2);

   BigObject p_out("Polytope");
   p_out.take(coord_section) << M_out;

   if (relabel) {
      const AnyString labels_section = dual ? AnyString("FACET_LABELS") : AnyString("VERTEX_LABELS");
      const std::vector<std::string> labels1 = common::read_labels(p_in1, labels_section, n_rows1);
      const std::vector<std::string> labels2 = common::read_labels(p_in2, labels_section, n_rows2);
      std::vector<std::string> labels_out(n_rows_out);
      copy_range(entire(product(labels2, labels1, wreath_label())), labels_out.begin());
      p_out.take(labels_section) << labels_out;
   }

   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Construct a new polytope as the wreath product of two input polytopes //P1//, //P2//."
                          "# //P1// and //P2// have to be [[BOUNDED]]."
                          "# @param Polytope P1"
                          "# @param Polytope P2"
                          "# @option Bool dual invokes the computation of the dual wreath product"
                          "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytopes. default: 0"
                          "#   the label of a new vertex corresponding to v<sub>1</sub> &oplus; v<sub>2</sub> will" 
                          "#   have the form LABEL_1*LABEL_2."
                          "# @return Polytope",
                          "wreath<Coord>(Polytope<type_upgrade<Coord>> Polytope<type_upgrade<Coord>> { dual => 0, no_labels => 0})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
