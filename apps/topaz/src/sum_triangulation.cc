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
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/topaz/sum_triangulation_tools.h"

namespace polymake { namespace topaz {

namespace {

// this function takes a facet F and glues it around the boundary of
// the ball defined by the WEB. It creates a new simplicial complex
// which contains all the facets indicated by WEB. 
void glue_facet(const Set<int>& _F, 
                const Array<int>& F_vertex_indices, 
                const Array<Set<int> >& facets, 
                const Array<int>& facets_vertex_indices, 
                const Set<int>& web, 
                int shift, 
                bool shift_facet,
                std::vector<Set<int> >& result)
{
   // gather facets
   Array<Set<int> > starF(web.size());
   int j = 0;
   for (Entire<Set<int> >::const_iterator webit = entire(web); !webit.at_end(); ++webit) {
      starF[j++] = facets[*webit];
   }

   // create the boundary complex
   perl::Object sComplex("topaz::SimplicialComplex");
   sComplex.take("INPUT_FACES") << starF;
   Array<Set<int> > boundary_facets = sComplex.give("BOUNDARY.FACETS");

   // taking care of index shifting for unused points
   Array<int> vertex_indices = sComplex.give("VERTEX_INDICES");
   Array<int> boundary_indices = sComplex.give("BOUNDARY.VERTEX_INDICES");

   // this is kind of lazy, but we do not need real permutations.
   // we just need some index shifting for which everything
   // which is created after the resize is redundant anyway.
   // the next line is to make sure the arrays have the same size
   boundary_indices.resize(vertex_indices.size());
   vertex_indices = permuted(vertex_indices, boundary_indices);

   // take care of the vertex indices of facets
   vertex_indices.resize(facets_vertex_indices.size());
   vertex_indices = permuted(facets_vertex_indices, vertex_indices);

   // take into account the vertex indices of F
   Set<int> F(permuted_inv(_F,F_vertex_indices));
   
   // shift the indices of F or boundary facet
   if (shift_facet) {
      F = attach_operation(F, operations::fix2<int,operations::add>(operations::fix2<int,operations::add>(shift)));
   } else {
      // we shift the vertex indices of the boundary facet via the
      // vertex permutation so that we don't have to do it later
      vertex_indices = attach_operation(vertex_indices, operations::fix2<int,operations::add>(operations::fix2<int,operations::add>(shift)));
   }

   // glue everything together
   for (Entire<Array<Set<int> > >::const_iterator bfit = entire(boundary_facets); !bfit.at_end(); ++bfit){
      result.push_back(F + permuted_inv(*bfit, vertex_indices));
   }    
}

} // end anonymous namespace

template<typename Scalar>  
perl::Object sum_triangulation(perl::Object p_in, 
                               perl::Object q_in, 
                               const IncidenceMatrix<> webOfStars_in,
                               perl::OptionSet options)
{
   const Matrix<Scalar> 
      pVert = p_in.give("COORDINATES"), 
      qVert = q_in.give("COORDINATES");

   const Array<Set<int> > 
      facetsP = p_in.give("FACETS"), 
      facetsQ = q_in.give("FACETS");

   Map<Set<int>, int> index_of;
   int facet_index(0);
   for (Entire<Array<Set<int> > >::const_iterator rit = entire(facetsQ); !rit.at_end(); ++rit)
      index_of[*rit] = facet_index++;

   const bool origin_first = options["origin_first"];

   Array<int> pVertexIndices, qVertexIndices;
   if (!(p_in.lookup("VERTEX_INDICES") >> pVertexIndices)) {
      pVertexIndices = sequence(0, pVert.rows());
   }
   if (!(q_in.lookup("VERTEX_INDICES") >> qVertexIndices)) {
      qVertexIndices = sequence(0, qVert.rows());
   }

   
   const Set<Set<int> > star_Q_0(star_of_zero_impl(qVert, facetsQ, false));
   Set<int> indices_of_star_Q_0;
   for (Entire<Set<Set<int> > >::const_iterator sit = entire(star_Q_0); !sit.at_end(); ++sit)
      indices_of_star_Q_0 += index_of[*sit];
   
   // make sure webOfStars has the right dimensions (fill with 0s if needed)
   IncidenceMatrix<> webOfStars;
   if (webOfStars_in.rows()) {
      webOfStars = webOfStars_in;
      webOfStars.resize(facetsP.size(),facetsQ.size());
   } else {
      webOfStars = IncidenceMatrix<>(facetsP.size(),facetsQ.size());
      for (Entire<Rows<IncidenceMatrix<> > >::iterator rit = entire(rows(webOfStars)); !rit.at_end(); ++rit)
         *rit = indices_of_star_Q_0;
   }

   std::vector<Set<int> > output_list;

   bool is_P_sum(true);

   // build simplices from p_in to q_in
   // according to WEB
   for(int i=0; i<facetsP.size(); ++i) {
      const Set<int>&
         F(facetsP[i]), 
         web(webOfStars.row(i));
      if (web.empty()) {
         is_P_sum = false; // if it were a P-sum-triangulation, web wouldn't be empty
         continue;
      }
      glue_facet(F, pVertexIndices, facetsQ, qVertexIndices, web, pVert.rows(), false, output_list);
   }

   // build simplices from q_in to p_in
   // deduce the web in the other direction by using WEB
   // compatibility tells us that the inverse web function is
   // invert(transpose(web))

   const IncidenceMatrix<> negatedWebOfStars(~webOfStars);

   for(int i=0; i<facetsQ.size(); ++i) {
      const Set<int>& F(facetsQ[i]);
      const Set<int> web (negatedWebOfStars.col(i));
      if (web.empty()) {
         // if it is a Q-sum-triangulation this wouldn't happen
         if (!is_P_sum) throw std::runtime_error("sum_triangulation: web of stars do not belong to a compatible pair");
         continue;
      }
      
      glue_facet(F, qVertexIndices, facetsP, pVertexIndices, web, pVert.rows(), true, output_list);
   }


   // OUTPUT
   perl::Object pSumTri(perl::ObjectType::construct<Scalar>("topaz::GeometricSimplicialComplex"));
   pSumTri.set_description() << "a P sum triangulation of " << p_in.name() << " and " << q_in.name() << "." << endl;

   Matrix<Scalar> sumVert = (pVert | zero_matrix<Scalar>(pVert.rows(), qVert.cols())) / 
                            (zero_matrix<Scalar>(qVert.rows(), pVert.cols()) | qVert);
   IncidenceMatrix<> output_facets(output_list);
   output_facets.resize(output_facets.rows(), pVert.rows()+qVert.rows());

   const int 
      p_zero_index(index_of_zero(pVert, false)), 
      q_zero_index(index_of_zero(qVert, false, pVert.rows()));

   // Take care of the origin
   if (!origin_first) {
      int zero_index(is_P_sum ? q_zero_index : p_zero_index);

      if (zero_index != -1) {
         sumVert = sumVert.minor(~scalar2set(zero_index), All);
         output_facets = output_facets.minor(All, ~scalar2set(zero_index));
      }
   } else {
      Set<int> ball_of_zero;
      if (q_zero_index != -1) {
         sumVert = sumVert.minor(~scalar2set(q_zero_index), All);
         if (!is_P_sum) ball_of_zero = Set<int>(output_facets.col(q_zero_index));
         output_facets = output_facets.minor(All, ~scalar2set(q_zero_index));
      }
      if (p_zero_index != -1) {
         sumVert = sumVert.minor(~scalar2set(p_zero_index), All);
         if (is_P_sum) ball_of_zero = Set<int>(output_facets.col(p_zero_index));
         output_facets = output_facets.minor(All, ~scalar2set(p_zero_index));
      }

      sumVert = zero_vector<Scalar>()/sumVert;
      output_facets = ball_of_zero | output_facets;

   }

   pSumTri.take("COORDINATES") << sumVert;
   pSumTri.take("INPUT_FACES") << rows(output_facets);

   return pSumTri;
}

UserFunctionTemplate4perl("# @category Producing a new simplicial complex from others\n"
                          "# Produce a specific P-sum-triangulation of two given triangulations\n"
                          "# and a WebOfStars.\n"
                          "# @param GeometricSimplicialComplex P First complex which will be favoured."
                          "# @param GeometricSimplicialComplex Q second complex."
                          "# @param IncidenceMatrix WebOfStars Every row corresponds to a full dimensional simplex in P and every column to a full dimensional simplex in Q."
                          "# @option Bool origin_first decides if the origin should be the first point in the resulting complex. Default=0"
                          "# @return GeometricSimplicialComplex",
                          "sum_triangulation<Scalar>(GeometricSimplicialComplex<type_upgrade<Scalar>> GeometricSimplicialComplex<type_upgrade<Scalar>>; IncidenceMatrix=new IncidenceMatrix() { origin_first => 0 })"); 

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
