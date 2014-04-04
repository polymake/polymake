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
#include "polymake/Rational.h"
#include "polymake/PowerSet.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/ListMatrix.h"
#include "polymake/hash_set"

namespace polymake { namespace tropical {
   
   
   Set<int> check_minimality(const Array< Set<int> > type, const Set<int> I, const int n) 
   {
      Set<int> violated_indices;
      int d=type.size();
      
      // check (i): type(I)=[n]
      Set<int> type_entries_of_I;
      for(Entire< Set< int > >::const_iterator si = entire(I); !si.at_end(); ++si)
         type_entries_of_I+=type[*si];
      if(type_entries_of_I.size()!=n)
         return I;
      
      // check (ii): for each j in I^C there is an i in I s.t. type(j)\cap type(i)
      // I^C is the complement of I in [0...d-1]
      Set<int> J=sequence(0,d)-I;
//       bool violated_II=false;
      for(Entire< Set< int > >::const_iterator sj = entire(J); !sj.at_end(); ++sj){
         bool violated_II_j=true;
         for(Entire< Set< int > >::const_iterator si = entire(I); !si.at_end(); ++si)
            if(violated_II_j)
            {
               Set<int> inters=type[*si]*type[*sj];
               if(!inters.empty())
                  violated_II_j=false; // there is an index i in I s.t. T_i*T_j is not empty
            }
         if(violated_II_j){
            violated_indices+=(*sj);
//             violated_II=true;
	   
	  }
      }
      
      //check(iii) for each i in I there is an j in I^C s.t. type(j)\cap type(i) is not contained in type(I\setminus\{i\})
//       bool violated_III=false;
      for(Entire< Set< int > >::const_iterator si = entire(I); !si.at_end(); ++si){
         bool violated_III_i=true;
         for(Entire< Set< int > >::const_iterator sj = entire(J); !sj.at_end(); ++sj){
            if(violated_III_i)
            {
               Set<int> type_entries;
               Set<int> I_minus_i=I-(*si);
               for(Entire< Set< int > >::const_iterator sii = entire(I_minus_i); !sii.at_end(); ++sii)
                  type_entries+=type[*sii];
               Set<int> inters=type[*si]*type[*sj];
               int l=incl(inters,type_entries);
               if(l>0)violated_III_i=false;// inters is not contained in type_entries
            }
         }
         if(violated_III_i){
            violated_indices+=(*si);
//             violated_III=true;
         }
      }
      return violated_indices;      
   } 
 
   template <typename Coord>
   void remove_redundant_sectors(const Vector<Coord> np, const Array<Set<int> > npt, const Set<int> I, const int n, hash_set< std::pair<Vector<Coord>,Set<int> > >& min_hs)
   {
      typedef std::pair<Vector<Coord>,Set<int> > ths; // tropical halfspaces are encoded as pair a pair of an apex and an index set of sectors
      if(I.empty())
         return;
      
      //check_minimality_III würde reichen
      Set<int> violated_indices=check_minimality(npt,I,n);
      if(!violated_indices.empty())
      {
         for(Entire< Set< int > >::const_iterator sj = entire(violated_indices); !sj.at_end(); ++sj)
         {
            Set<int> I_new=I-(*sj);
            remove_redundant_sectors(np,npt,I_new,n,min_hs);
         }
      }
      else
      {
         ths new_min_hs(np,I);
         if(min_hs.find(new_min_hs)==min_hs.end())
            min_hs.insert(new_min_hs);         
      }
      return;   
   }
   
   
   template <typename Coord>
   hash_set< std::pair<Vector<Coord>,Set<int> > > minimal_tropical_halfspaces(perl::Object t_in)
   {
      // tropical halfspaces are encoded as pairs of an apex and an index set of sectors
      typedef std::pair<Vector<Coord>,Set<int> > ths; 
      
      const Matrix<Coord> V=t_in.give("VERTICES");
      const int d=V.cols();
      const int n_vert=V.rows();
      
      // the minimal tropical halfspaces
      hash_set<ths> min_hs;
      
      // cornered hull of t_in
      Matrix<Coord> corners=CallPolymakeFunction("get_corners",V);
      perl::Object ch =CallPolymakeFunction("cornered_hull_poly",t_in);
      int ch_dim=ch.CallPolymakeMethod("DIM");
      
      // insert the cornered halfspaces
      for(int i=0;i<d;++i)
         min_hs.insert(ths(corners[i],sequence(i,1)));
            
      // intersect each maximal polytope of the tropical complex of T 
      // with the cornered hull of T
      // check the corners of the intersections if they are apices 
      // of minimal tropical hyperplanes

      perl::Object pc=CallPolymakeFunction("tropical_complex",V);
      Array<Set<int> > pc_cells = pc.give("MAXIMAL_POLYTOPES");
      Matrix<Coord> pc_vertices = pc.give("VERTICES");
      int n_cells=pc.give("N_MAXIMAL_POLYTOPES");
      
      for (int i=0;i<n_cells;++i)
      {
         perl::Object cell("polytope::Polytope");      
         cell.take("POINTS")<<pc_vertices.minor(pc_cells[i],All);
         perl::Object inters=CallPolymakeFunction("polytope::intersection",ch,cell);
         int dim=inters.CallPolymakeMethod("DIM");
         if(dim==ch_dim)
         {
            Vector<Coord> rel_int_point=inters.give("REL_INT_POINT");
            rel_int_point[0]=0;
            Matrix<Coord> M = vector2row(rel_int_point);
            Array<Array<Set<int> > > cell_type= CallPolymakeFunction("types",M,V);
            Set<int> empty_entries;
            for(int j=0;j<d;++j)
               if(cell_type[0][j].empty())
               {
                  empty_entries+=j;
               }
            
            if(!empty_entries.empty())
            {
               perl::Object tropical_cell=CallPolymakeFunction("poly2trop",inters);
               Matrix<Coord> cell_corners=CallPolymakeFunction("get_corners",tropical_cell.give("VERTICES"));
               
               for(Entire< Set< int > >::const_iterator si = entire(empty_entries); !si.at_end(); ++si)
               {
                  Vector<Coord> corner=cell_corners[*si];
                  Vector<Coord> np=CallPolymakeFunction("nearest_point",t_in,corner);
                  Matrix<Coord> M_np = vector2row(np);
                  Array<Array<Set<int> > > npt=CallPolymakeFunction("types",M_np,V); 
                  
                  Set<int> I=sequence(0,d)-empty_entries;
                  remove_redundant_sectors(np,npt[0],I,n_vert,min_hs);               
               }           
            }
         }
      }
      return min_hs; 
   }
   
   
   UserFunction4perl("# @category Other"   
                     "# Checks the three criteria of Gaubert and Katz to be" 
                     "# the type //T// of an apex of a minimal tropical halfspace."
                     "# It is assumed that the points that the type refers to are given by 0,...,//n//-1"
                     "# and that the index set //I// is a subset of 0,...,//d//-1"
                     "# where //d// is the [[AMBIENT_DIM]] of the tropical polytope."
                     "# If the input fulfills all criteria, the output set is empty."
                     "# If the input doesn't fulfill the first criterion the whole set 0,...,//d//-1 is given back."
                     "# If the input doesn't fulfill the second and third criterion, then the violating indices are stored."
                     "# @param Array<Set> T"
                     "# @param Set I" //the index set //I// s.t. the union of the type entries indexed by //I// is 0,...,//n//-1"
                     "# @param Integer n"// //n// the number of points that the type refers to"
                     "# @return Set",// the set of violating indices",
                     &check_minimality,"check_minimality");

   UserFunctionTemplate4perl("# @category Tropical convex hulls"
                             "# Computes the minimal tropical halfspaces of a"
                             "# tropical polytope //T//."
                             "# @param TropicalPolytope T"
                             "# @return hash_set< Pair<Vector<Coord>,Set<Int> > >"
                             "# @author Katja Kulas",
                             "minimal_tropical_halfspaces<Coord>(TropicalPolytope<Coord>)");
   
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
