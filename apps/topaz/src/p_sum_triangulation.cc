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
#include "polymake/list"

namespace polymake { namespace topaz {

namespace{

// this function takes a facet F and glues it around the boundary of
// the ball defined by the WEB. It creates a new simplicial complex
// which contains all the facets indicated by WEB. It looks at the boundary
// complex and glues F along that boundary.
std::list<Set<int> > glue_facet(Set<int> F, Array<Set<int> > facets, Set<int> web, int shift, bool shift_facet)
{
   std::list<Set<int> > result;

   // gathering facets
   Array<Set<int> > starF(web.size());
   int j = 0;
   for (Entire<Set<int> >::const_iterator webit = entire(web); !webit.at_end(); ++webit){
      starF[j++] = facets[*webit];
   }
   // creating the boundary complex
   perl::Object sComplex("topaz::SimplicialComplex");
   sComplex.take("INPUT_FACES") << starF;
   sComplex.give("FACETS");
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
   
   // shift the indices of F or boundary facet
   if(shift_facet){
      F = attach_operation(F, operations::fix2<int,operations::add>(operations::fix2<int,operations::add>(shift)));
   } else {
      // we shift the vertex indices of the boundary facet via the
      // vertex permutation so that we don't have to do it later
      vertex_indices = attach_operation(vertex_indices, operations::fix2<int,operations::add>(operations::fix2<int,operations::add>(shift)));
   }
   
   // glue everything together
   for (Entire<Array<Set<int> > >::const_iterator bfit = entire(boundary_facets); !bfit.at_end(); ++bfit){
      Set<int> face(permuted_inv(*bfit, vertex_indices));
      result.push_back(F + face);
   }    

   return result;
}

}

template<typename Scalar>  
perl::Object p_sum_triangulation(perl::Object p_in, perl::Object q_in, const IncidenceMatrix<> &webOfStars)
{

   // gather information
   Matrix<Scalar> pVert, qVert;
   p_in.give("COORDINATES") >> pVert;
   q_in.give("COORDINATES") >> qVert;

   Array<Set<int> > facetsP, facetsQ;
   p_in.give("FACETS") >> facetsP;
   q_in.give("FACETS") >> facetsQ;


   std::list<Set<int> > output_list;

   // build simplices from p_in
   for(int i=0; i<facetsP.size(); ++i){
      Set<int> F(facetsP[i]), web(webOfStars.row(i));
      std::list<Set<int> > tmp=glue_facet(F,facetsQ,web,pVert.rows(), false);
      output_list.splice(output_list.end(), tmp);
   }

   // build simplices from q_in
   for(int i=0; i<facetsQ.size(); ++i){
      Set<int> F(facetsQ[i]), web((~webOfStars).col(i));
      if(web.empty()) continue;
      
      std::list<Set<int> > tmp=glue_facet(F,facetsP,web,pVert.rows(), true);
      output_list.splice(output_list.end(), tmp);
   }


   // OUTPUT
   perl::Object pSumTri(perl::ObjectType::construct<Scalar>("topaz::GeometricSimplicialComplex"));
   pSumTri.set_description()<<"a P sum triangulation of " << p_in.name() << " and " << q_in.name()<< "."<<endl;

   bool found_zero = false;
   int zero_index;
   
   for(zero_index=0; zero_index < qVert.cols(); ++zero_index){
      if(qVert.row(zero_index) == zero_vector<Scalar>(qVert.cols())){
         found_zero=true;
         break;
      }
   }

   Matrix<Scalar> sumVert;
   IncidenceMatrix<> output_facets(output_list);
   if(found_zero){
      sumVert = (pVert | zero_matrix<Scalar>(pVert.rows(), qVert.cols())) / (zero_matrix<Scalar>(qVert.rows()-1, pVert.cols()) | qVert.minor(~scalar2set(zero_index), All));
      output_facets = output_facets.minor(All, ~scalar2set(pVert.rows() + zero_index));
   } else {
      sumVert = (pVert | zero_matrix<Scalar>(pVert.rows(), qVert.cols())) / (zero_matrix<Scalar>(qVert.rows(), pVert.cols()) | qVert);
   }

   pSumTri.take("COORDINATES") << sumVert;
   pSumTri.take("INPUT_FACES") << rows(output_facets);

   return pSumTri;
}

UserFunctionTemplate4perl("# @category Producing a new simplicial complex from others\n"
                          "# Produce a specific P-sum-triangulation of two given triangulations\n"
                          "# and a WebOfStars.\n"
                          "# @param GeometricSimplicialComplex P First complex which will be favoured"
                          "# @param GeometricSimplicialComplex Q second complex"
                          "# @param IncidenceMatrix WebOfStars Every row corresponds to a full dimensional simplex in P and every column to a full dimensional simplex in Q."
                          "# @return GeometricSimplicialComplex",
                          "p_sum_triangulation<Scalar>(GeometricSimplicialComplex<type_upgrade<Scalar>> GeometricSimplicialComplex<type_upgrade<Scalar>> IncidenceMatrix)"); 

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
