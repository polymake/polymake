/* Copyright (c) 1997-2019
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
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/vector"
#include "polymake/PowerSet.h"
#include "polymake/common/lattice_tools.h"

// FIXME: use std::vector instead of naked heap allocations when all containers are movable

namespace polymake { namespace polytope {
namespace {

Matrix<Rational> vertices;

class Face {
public:
   Vector<Rational> projFace;
   Set<int> vertices;

   Face(int coord, Set<int>& vert, Face* lower, Face* upper)
   {
      vertices = vert;
      projFace = lower->projFace;
      if (projFace[coord] != 0)
         projFace -= (projFace[coord]/upper->projFace[coord]) * (upper->projFace);
      projFace = common::primitive(projFace);
   }
   Face(Set<int>& vert, Vector<Rational>& proj)
   {
      vertices = vert;
      projFace = proj;
   }
};

void createChildren( std::vector<Face>* lowerFaces, std::vector<Face>* upperFaces, int faceDim, int coord,
                     std::vector<Face>* lowerChildren, std::vector<Face>* upperChildren, int verbose)
{
   if (verbose)
      cout << "creating faces of dim " << faceDim << " from " << lowerFaces->size()<<"+"
           <<upperFaces->size() << " faces of dim " << faceDim+1 << " using coord " << coord << " ... ";
   if (lowerFaces->size() < 1 || upperFaces->size() < 1)
      throw std::runtime_error("lattice_points_via_projection: too few faces!");

   // search through all pairs of faces of lower and upper hull to find pairs
   // which intersect in a lower dimensional face
   for (std::vector<Face>::iterator lowerFaceIt=lowerFaces->begin();
        lowerFaceIt!=lowerFaces->end(); ++lowerFaceIt)
   {
      for (std::vector<Face>::iterator upperFaceIt=upperFaces->begin();
           upperFaceIt!=upperFaces->end(); ++upperFaceIt)
      {
         Set<int> child = (lowerFaceIt->vertices)*(upperFaceIt->vertices);
         if (child.size() >= faceDim+1 && pm::rank(vertices.minor(child,range(0,coord-1))) >= faceDim+1)
         {
            Face childFace(coord, child, &(*lowerFaceIt), &(*upperFaceIt));

            // we want to have each face only once
            // if we have this vector (projFace) already, add the vertices defining this face to the existing one
            std::vector<Face> * childList = (childFace.projFace[coord-1] < 0) ? upperChildren : lowerChildren;
            bool add = true;
            for (std::vector<Face>::iterator childIt = childList->begin();
                 childIt != childList->end(); ++childIt)
            {
               if (childFace.projFace == childIt->projFace) {
                  childIt->vertices += child;
                  add = false;
                  break;
               }
            }
            if (add)
               childList->push_back(childFace);
         }
      }
   }
   if (verbose)
      cout << "done creating " << lowerChildren->size() <<"+" << upperChildren->size() << " new faces."<< endl;
}

// try to find a row R in affine hull that has a nonzero value at the current coordinate
// if found reduce all other rows at this coordinate using R and return R
Vector<Rational>* tryAffineHull(Matrix<Rational>** affineHull, int coord, int verbose)
{
   if(verbose)
      cout << "trying to find affine hull row for coord "<< coord << " ... ";
   for (int row = 0; row < (*affineHull)->rows(); ++row)
   {
      if ((((*affineHull)->row(row))[coord]) != 0)
      {
         Matrix<Rational> * newAffineHull = new Matrix<Rational>((*affineHull)->minor(~scalar2set(row),All));
         Vector<Rational> * ahrow = new Vector<Rational>((*affineHull)->row(row));
         for (auto r = entire(rows(*newAffineHull)); !r.at_end(); ++r) {
            if (!is_zero((*r)[coord])) {
               (*r) -= ((*r)[coord] / (*ahrow)[coord]) * (*ahrow);
            }
         }
         delete (*affineHull);
         (*affineHull) = newAffineHull;
         if (verbose)
            cout << "using row " << row << ", done updating matrix." << endl;
         return ahrow;
      }
   }
   if(verbose)
      cout << "none found" << endl;
   return NULL;
}

// reduce all faces using the affine hull row and resort them according to the next coordinate
void affineProjection(std::vector<Face>* Faces, Vector<Rational>* affineHullRow, int coord,
                      std::vector<Face>* lowerChildren, std::vector<Face>* upperChildren, int verbose)
{
   for (std::vector<Face>::iterator faceIt=Faces->begin();
        faceIt!=Faces->end(); ++faceIt)
   {
      Face f(*faceIt);
      if (f.projFace[coord] != 0)
      {
         f.projFace -= (f.projFace[coord] / (*affineHullRow)[coord] ) * (*affineHullRow);
      }
      if (f.projFace[coord-1] >= 0) 
         lowerChildren->push_back(f);
      else
         upperChildren->push_back(f);
   }
}

// find upper and lower bound in the next coordinate for all projected points
// then create matrix of all lifted points
Matrix<Integer>* liftPoints(Matrix<Integer>* projPoints, 
                            std::vector<Face>* lowerFaces, std::vector<Face>* upperFaces, 
                            int coord, int verbose)
{
   int npoints = projPoints->rows();
   if (verbose)
      cout << "lifting " << npoints << " points using coord " << coord << " ... ";
    
   // first compute where all faces intersect the lines above all projected lattice points
   Matrix<Rational> lowerBoundMatrix(lowerFaces->size(),npoints);
   Matrix<Rational> upperBoundMatrix(upperFaces->size(),npoints);
    
   int nlower = 0;
   for(std::vector<Face>::iterator lowerF=lowerFaces->begin(); 
       lowerF!=lowerFaces->end(); ++lowerF)
   {
      if (lowerF->projFace[coord] > 0) {
         lowerBoundMatrix[nlower] = -(*projPoints)*(lowerF->projFace) / lowerF->projFace[coord];
         ++nlower;
      }
   }
   lowerBoundMatrix = lowerBoundMatrix.minor(sequence(0,nlower),All);
   int nupper = 0;
   for(std::vector<Face>::iterator upperF=upperFaces->begin(); 
       upperF!=upperFaces->end(); ++upperF)
   {
      upperBoundMatrix[nupper] = -(*projPoints)*(upperF->projFace) / upperF->projFace[coord];
      ++nupper;
   }
    
   if (verbose>=2)
      cout << "[nlower=" << nlower << ",nupper=" << nupper << "] ";
   // find the bounds for each lattice point
   Vector<Integer> lowerBounds(lowerBoundMatrix.cols(), std::numeric_limits<Integer>::min());
   Vector<Integer>::iterator lowerBound = lowerBounds.begin();

   for (auto LBcol=entire(cols(lowerBoundMatrix)); !LBcol.at_end(); ++LBcol, ++lowerBound) {
      for (auto LBcolentry = entire(*LBcol); !LBcolentry.at_end(); ++LBcolentry) {
         if (*lowerBound < *LBcolentry)
            *lowerBound = ceil(*LBcolentry);
      }
   }
    
   if (verbose>=2)
      cout << "[apply] ";
   Vector<Integer> upperBounds(upperBoundMatrix.cols(), std::numeric_limits<Integer>::max());
   Vector<Integer>::iterator upperBound = upperBounds.begin();

   for (auto UBcol=entire(cols(upperBoundMatrix)); !UBcol.at_end(); ++UBcol, ++upperBound) {
      for (auto UBcolentry = entire(*UBcol); !UBcolentry.at_end(); ++UBcolentry) {
         if (*upperBound > *UBcolentry)
            *upperBound = floor(*UBcolentry);
      }
   }
    
   if (verbose>=2)
      cout << "[matrix] ";
   // generate matrix containing all lifted lattice points, using the projected lattice point with all valid values in the (coord) coordinate
   const int count = int(accumulate(upperBounds - lowerBounds, operations::add())) + npoints;
   Matrix<Integer>* points = new Matrix<Integer>(count, projPoints->cols());
   Rows< Matrix<Integer> >::iterator point = rows(*points).begin();
   for(int i = 0; i < npoints; ++i) 
   {
      for(Integer j = lowerBounds[i]; j <= upperBounds[i]; ++j, ++point) 
      {
         *point = projPoints->row(i);
         (*point)[coord] = j;
      }
   }
   if (verbose)
      cout << "created " << points->rows() << " points." << endl;
   return points;
}
  
// calculate the next coordinate using an affine hull row
Matrix<Integer>* liftPointsAffine(Matrix<Integer>* projPoints, Vector<Rational>* affineHullRow, int coord, int verbose)
{
   if(verbose)
      cout << "calculating coord "<< coord << " for "<< projPoints->rows() <<" points using affine hull row ... " ;
   Set<int> noninteger;
   for(int i = 0; i < projPoints->rows(); i++)
   {
      Rational entry = (*projPoints)[i] * (*affineHullRow) / (*affineHullRow)[coord];
      if (entry != 0) {
         if (entry.is_integral())
            (*projPoints)[i][coord] = -numerator(entry);
         else 
            noninteger += i;
      }
   }
   if (noninteger.size() > 0) 
   {
      Matrix<Integer>* liftedPoints = new Matrix<Integer>(projPoints->minor(~noninteger,All));
      delete projPoints;
      if(verbose)
         cout << "done, removed " << noninteger.size() << " non-integer points." << endl;
      return liftedPoints;
   }
   else
   {
      if(verbose)
         cout << "done." << endl;
      return projPoints;
   }
}

// main recursion calling the projection and lifting functions
Matrix<Integer> * points(std::vector<Face>* lowerFaces, std::vector<Face>* upperFaces, 
                         Matrix<Rational> * affineHull, int faceDim, int coord, int totalDim, int verbose) 
{
   Matrix<Integer> * projPoints;
   Vector<Rational> * affineHullRow = NULL;
   if (coord > 1)
   {
      // create lists of upper and lower hull in one dimension lower
      std::vector<Face> * lowerChildren = new std::vector<Face>();
      std::vector<Face> * upperChildren = new std::vector<Face>();
      
      affineHullRow = tryAffineHull(&affineHull,coord,verbose);
      
      if (affineHullRow != NULL) 
      {
         if(verbose)
            cout << "reducing coord "<< coord << " using affine hull ... ";
         affineProjection(lowerFaces,affineHullRow,coord,lowerChildren,upperChildren,verbose);
         affineProjection(upperFaces,affineHullRow,coord,lowerChildren,upperChildren,verbose);
         if(verbose)
            cout << "done." << endl;
      }
      else
      {
         faceDim--;
         createChildren(lowerFaces,upperFaces,faceDim,coord,lowerChildren,upperChildren, verbose);
      }
      
      // main recursion
      projPoints = points(lowerChildren,upperChildren,affineHull,faceDim,coord-1,totalDim,verbose);
      delete lowerChildren;
      delete upperChildren;
   }
   else
   {
      if (verbose) 
         cout << "*** projecting done ***" << endl << endl << "*** lifting points now ***" << endl;
      // create one lattice point for the zero dimensional projection
      projPoints = new Matrix<Integer>(1,totalDim+1);
      (*projPoints)(0,0)=1;
      if (faceDim == -1)
         affineHullRow = new Vector<Rational>(affineHull->row(0));
      delete affineHull;
   }
    
   // lift points up one dimension
   Matrix<Integer> * liftedPoints;
   if (affineHullRow != NULL) 
   {
      liftedPoints = liftPointsAffine(projPoints,affineHullRow,coord,verbose);
      delete affineHullRow;
   }
   else
   {
      liftedPoints = liftPoints(projPoints, lowerFaces, upperFaces, coord, verbose);
      delete projPoints;  
   }
    
   return liftedPoints;
}
  
} // end anonymous namespace

Matrix<Integer> integer_points_projection(perl::Object p, int verbose)
{
   const int ambient = p.call_method("AMBIENT_DIM");
   const int dim = p.call_method("DIM");

   if (dim == -1)
      return Matrix<Integer>();

   if (ambient == 0)
   {
      Matrix<Integer> LP = unit_matrix<Integer>(1);
      return LP;
   }

   const Matrix<Rational> facets = p.give("FACETS");
   p.give("VERTICES") >> vertices;
   const Matrix<Rational> AH = p.give("AFFINE_HULL");
   const IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");
   Matrix<Rational>* affineHull = new Matrix<Rational>(AH);

   // create first faces = facets
   std::vector<Face> * lowerFaces = new std::vector<Face>();
   std::vector<Face> * upperFaces = new std::vector<Face>();
   for(int i = 0; i < facets.rows(); ++i)
   {
      Vector<Rational> facet(facets[i]);
      Set<int> vif(VIF[i]);
      if(facet[ambient] >= 0)
         lowerFaces->push_back(Face(vif,facet));
      else
         upperFaces->push_back(Face(vif,facet));
   }

   if (verbose)
      cout << "*** projecting faces ***" << endl;
   // start recursion (repeated projection of the faces and lifting of the lattice points of the projected polytope)
   Matrix<Integer> * latticePoints_ptr = points(lowerFaces,upperFaces,affineHull,dim-1,ambient,ambient,verbose);
   delete lowerFaces;
   delete upperFaces;
   if (verbose)
      cout << "*** done lifting ***" << endl;

   Matrix<Integer> latticePoints = (*latticePoints_ptr);
   delete latticePoints_ptr;

   return latticePoints;
}

Function4perl(&integer_points_projection, "integer_points_projection(Polytope; $=0)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
