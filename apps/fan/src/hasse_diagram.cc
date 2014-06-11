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
#include "polymake/hash_map"
#include "polymake/FaceMap.h"
#include "polymake/Bitset.h"
#include "polymake/FacetList.h"
#include "polymake/graph/HasseDiagram.h"

#include <sstream>

namespace polymake { namespace fan {

// constructs the Hasse diagram of a polyhedral fan F
// construction always done from maximal faces to empty set (dual approach)
// FIXME allowing non-pure and non-complete fans requires additional rather expensive checks
//       we should have specialized versions for pure or complete fans
//       in an improved version we might want to list the maximal cones at the beginning of the level they appear in, 
//       in the order they are listed in MAXIMAL_CONES
//       the algorithm below does not support this 
perl::Object hasse_diagram(const IncidenceMatrix<> &MaximalCones, 
                           const Array<IncidenceMatrix<> > &ListOfCones, 
                           const Array<int> dims, 
                           const int dim) {

   // sort facets according to their dimension
   Array<  Set<int> > facets_of_dim(dim+2);
      
   for ( int i = 0; i < dims.size(); ++i )
      facets_of_dim[dims[i]+1] += i;
      
   // fill the hasse diagram
   graph::HasseDiagram HD;
   graph::HasseDiagram::_filler HD_filler(HD);

   // the number of the next node added in the Hasse diagram
   int i=0;

   // node 0 is an artificial top node representing the whole fan
   HD_filler.add_node(sequence(i++,MaximalCones.cols()));        
   HD_filler.increase_dim();

   // the trivial case
   if (ListOfCones.size()==0)  
      return HD.makeObject();

   // a list of the facets of all cones (facet wrt the dim of the cone) of dimension >= the current layer
   // adjusted in each step
   // used to compute the list of faces below the current layer
   FacetList ActiveConeFacets;

   // the nodes for the rays of the fan are added separately to ensure 
   // that they appear in the same order as the rays in the RAYS section
   // so we skip the following loop over the layers if no maximal cone contains more than one ray
   if (__builtin_expect(dim>0, 1)) {      
      // add the top layer and
      // initialize ActiveConeFacets with the facets of these cones
      // i is the index of the next node that will be created
      for (Entire< Set<int> >::const_iterator f_it=entire(facets_of_dim[dim+1]);
           !f_it.at_end(); ++f_it, ++i) {
         HD_filler.add_node(MaximalCones[*f_it]);
         HD_filler.add_edge(i,0);
         for ( Entire<Rows<IncidenceMatrix<> > >::const_iterator c_it = entire(rows(ListOfCones[*f_it]));
               !c_it.at_end(); ++c_it ) 
            ActiveConeFacets.insertMax(*c_it);
      }
      HD_filler.increase_dim();

      // in each round we construct a list of all faces in the layer below the current
      // initially these are the facets of the cones of maximal dimension
      FacetList FacesBelow(ActiveConeFacets);

      // go down dimension-wise
      // in each iteration we add the cones with the current dimension, and the faces in FacesBelow
      // we stop at the 2-dimensional faces and add the rays in the order of RAYS below.
      for (int d=dim; d>1; --d) {
         
         // add the cones that first appear in dimension dimension d
         // FBelow collects the faces in the level below the current one
         FacetList FBelow(ActiveConeFacets.cols()); // we need at least that many columns
         
         // add the faces in FacesBelow
         for (Entire< FacetList >::const_iterator f_it=entire(FacesBelow);
              !f_it.at_end(); ++f_it, ++i) {
            HD_filler.add_node(*f_it);
            
            // for each face we add we iterate over the faces in the previous layer to check which edges we have to add
            // FIXME this is not efficient, many inclusion tests are unnecessary
            //       maybe introduce data structure recording inclusion relation during construction of FBelow below
            for ( Entire< graph::HasseDiagram::nodes_of_dim_set >::const_iterator h_it = entire(HD.nodes_of_dim(0)); 
                  !h_it.at_end(); ++h_it )
               if ( incl(*f_it,HD.face(*h_it) ) <= 0 ) 
                  HD_filler.add_edge(i,*h_it);
         }
         
         // now find the faces in the layer below the current one (and some face of FacesBelow) and add them to FBelow
         // only necessary up to the 3-dim faces, as we already know the rays
         if (__builtin_expect(d>2, 1)) {
            for ( Entire< FacetList >::const_iterator fb_it = entire(FacesBelow); !fb_it.at_end(); ++fb_it ) {
               for ( Entire< FacetList >::const_iterator acf_it = entire(ActiveConeFacets); !acf_it.at_end(); ++acf_it) {
                  Set<int> fb = (*fb_it) * ( *acf_it );
                  if ( fb.size() && fb.size() < fb_it->size() )
                     FBelow.replaceMax(fb);
               }
            }
         }
         
         for (Entire< Set<int> >::const_iterator f_it=entire(facets_of_dim[d]);
              !f_it.at_end(); ++f_it, ++i) {
            HD_filler.add_node(MaximalCones[*f_it]);
            // they are connected to the artificial top node (breaks HasseDiagram convention...)
            HD_filler.add_edge(i,0);                  
            
            // now add their facets to ActiveConeFacets and
            // insert the facets to FBelow to add them in the next iteration
            // only necessary up to the 3-dim faces, as we already know the rays
            if (__builtin_expect(d>2, 1)) {
               for ( Entire<Rows<IncidenceMatrix<> > >::const_iterator c_it = entire(rows(ListOfCones[*f_it]));
                     !c_it.at_end(); ++c_it ) {
                  ActiveConeFacets.insertMax(*c_it);  // they could potentially add new rays to the incidence matrix
                  FBelow.insertMax(*c_it);            // --------------------------- " -----------------------------
               }
            }
         }
         
         // increase the dimension
         HD_filler.increase_dim();
         // prepare FacesBelow for the next iteration
         FacesBelow = FBelow;
      }
   } // end of the loop over the inner layers of the Hasse diagram

   // collect isolated rays
   // we have to add an edge to the top node for those
   Set<int> isolated_rays;
   if (dim >-1)
      for (Entire< Set<int> >::const_iterator f_it=entire(facets_of_dim[1]); !f_it.at_end(); ++f_it) 
         if ( MaximalCones[*f_it].size() == 1 )
            isolated_rays += *(MaximalCones[*f_it].begin());

   // record the first node in the ray level
   int node_in_ray_level = i;
   // add the rays in the order they appear in RAYS
   for ( int j = 0; j < MaximalCones.cols(); ++j, ++i ) {
      HD_filler.add_node(scalar2set(j));

      // add edge to top node if ray is a maximal cone
      if ( isolated_rays.contains(j) ) 
         HD_filler.add_edge(i,0);
      
      for ( Entire< graph::HasseDiagram::nodes_of_dim_set >::const_iterator h_it = entire(HD.nodes_of_dim(0)); 
            !h_it.at_end(); ++h_it ) 
         if ( HD.face(*h_it).contains(j) )
            HD_filler.add_edge(i,*h_it);
   }

   HD_filler.increase_dim();

   // add the origin and connect it to the rays
   HD_filler.add_node(sequence(0,0));  
   while (node_in_ray_level < HD.nodes()-1 ) {
      HD_filler.add_edge(i, node_in_ray_level);
      ++node_in_ray_level;
   }
   
   // add an edge between the origin and the artificial node for fans
   // without rays
   if (dim == -1)
      HD_filler.add_edge(i,0);
      
   return HD.makeObject();
}

Function4perl(&hasse_diagram, "hasse_diagram($,$,$,$)");
    
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
