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
#include "polymake/linalg.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"

namespace polymake { namespace polytope {

template <typename Coord>
void rel_int_point(perl::Object p)
{
   std::string f;
   Matrix<Coord> F=p.give_with_property_name("FACETS | INEQUALITIES",f);
   Matrix<Coord>  E;
   const Vector<Coord> v0=p.give("ONE_VERTEX");      
   int n = v0.dim()-1;
   bool unbounded = !p.give("BOUNDED");

   ListMatrix<Vector <Coord> > verts(0,n); //the linear space corresponding to the affine space spanned by the vertices
   ListMatrix<Vector <Coord> > equats(0,n+1);//equations for the polytope, linear independent
   bool ah_eq=false;

   std::string got_property;
   if (p.lookup_with_property_name("AFFINE_HULL | EQUATIONS", got_property)>>E) {
     if (got_property == "EQUATIONS") {
       E=E.minor(basis_affine(E).first,All);
     }
     equats=E;
     ah_eq=true;
  }


      
   if (unbounded) {
      const Vector<Coord> VEC1=-v0.slice(1)+ones_vector<Coord>(n);
      const Vector<Coord> VEC2=v0.slice(1)+ones_vector<Coord>(n);
      F/=(VEC1|unit_matrix<Coord>(n))/(VEC2|-unit_matrix<Coord>(n));
   }
  
   int n_verts,n_equats; 
   while ((n_verts=verts.rows())+(n_equats=equats.rows())<n) {
      Matrix<Coord> ort=null_space(verts);

      bool dep=true;      
      
      Matrix <Coord> A(0,n);
      if (equats.cols()!=0) { A = dehomogenize(equats); };
      //we try to find a vector orthogonal to the space of vertices and not yet contained in equats
      int i=0;
      for (; dep;++i) {
         if (rank(A/ort.row(i))==n_equats+1) dep=false;
      }
      i--;
      //we solve the linear program for this direction
      perl::Object p_new(p.type());

      // this should be improved...
      if (unbounded) {
         if (ah_eq)
            // we need to take care of the restriction that AFFINE_HULL may only be written if F contains the FACETS
            p_new.take(f=="FACETS" ? "AFFINE_HULL" : "EQUATIONS")<<E;  
         p_new.take(f)<<F;
      } else {
         if (ah_eq)
            p_new.take("EQUATIONS")<<E;
         p_new.take("INEQUALITIES")<<F;
      }

      p_new.take("ONE_VERTEX")<<v0;
      // Either it was bounded to start, or we forced it to be bounded
      p_new.take("BOUNDED")<<1;

      // In the bounded case, we have a subset of constraints; in the
      // unbounded case we take care to preserve our one vertex
      p_new.take("FEASIBLE")<<1;

      p_new.take("LP.LINEAR_OBJECTIVE")<<(0|ort.row(i));
      const Coord min=p_new.give("LP.MINIMAL_VALUE");
      const Coord max=p_new.give("LP.MAXIMAL_VALUE");

      //if min and max are equal we have found an equation
      if (min==max)
         equats/=(-min|ort.row(i));
      //else we get a new vertex
      else {
         const Vector<Coord> M=p_new.give("LP.MINIMAL_VERTEX");
         const Matrix<Coord> Mat=verts/(M-v0).slice(1);
         if (rank(Mat)==n_verts+1)
            verts/=(M-v0).slice(1);
         else {
            const Vector<Coord> M2=p_new.give("LP.MAXIMAL_VERTEX");
            verts/=(M2-v0).slice(1);
         }
      }
   }

   p.take("CONE_DIM")<<verts.rows()+1;
   p.take("REL_INT_POINT") << (v0+(0|average(rows(verts/zero_vector<Coord>(n)))));
}

FunctionTemplate4perl("rel_int_point<Coords>(Polytope<Coords>) : void");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
