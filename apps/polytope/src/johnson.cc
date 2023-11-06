/* Copyright (c) 1997-2023
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   Author: Olivia Röhrig

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
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Matrix.h"
#include "polymake/SparseVector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/hash_map"
#include <cmath>

namespace polymake { namespace polytope {

typedef QuadraticExtension<Rational> QE;

namespace {

double norm(Vector<double> v)
{
  double n = 0;
  Int d = v.size();
  Int i = 0;
  if (d == 4) i = 1; //hom.coords...
  for (; i < v.size(); ++i) {
    n += sqr(v[i]);
  }
  return sqrt(n);
}

Vector<double> cross_product(const Vector<double>& a, const Vector<double>& b)
{
   Vector<double> result(4);
   result[1]=a[2]*b[3]-a[3]*b[2];
   result[2]=a[3]*b[1]-a[1]*b[3];
   result[3]=a[1]*b[2]-a[2]*b[1];
   return result;
}

Vector<double> find_facet_normal(Matrix<double> V, const Set<Int>& f_vert)
{
   Matrix<double> FV = V.minor(f_vert,All);
   Vector<double> cp = cross_product(FV[2]-FV[0],FV[1]-FV[0]);

   Vector<double> non_facet_point = V[(sequence(0,V.rows()) - f_vert).front()];
   //if nfp lies on the negative side of the plane, the normal vector points inwards
   if (cp*(non_facet_point - FV[0]) < 0)
      cp *= -1;

   cp /= norm(cp); //unit facet normal

   return cp;
}

//get the indices of the facet sorted in counterclockwise order.
Array<Int> neighbors(const Matrix<double>& V, const Set<Int>& f_vert)
{
   Matrix<double> FV = V.minor(f_vert,All);

   // project the points to a plane to make sure we get one polygon
   // even though the coords are inexact
   Matrix<double> basis(2, 4);
   basis[0] = FV[2] - FV[0];
   basis[1] = FV[1] - FV[0];
   FV = ones_vector<double>() | FV * T(basis);
   BigObject facet("Polytope<Float>", "VERTICES", FV);

   // sort VIF lexicographically to ensure the facet polytope is
   // independent of permutations through convex hull algos
   IncidenceMatrix<> VIF = facet.give("VERTICES_IN_FACETS");
   Set<Set<Int>> VIF_sorted;
   for (auto f = entire(rows(VIF)); !f.at_end(); ++f)
      VIF_sorted += *f;

   BigObject facetlex("Polytope<Float>", "VERTICES", FV, "VERTICES_IN_FACETS", VIF_sorted);

   //vertex indices in counterclockwise order
   Array<Array<Int>> FVIF = facetlex.give("VIF_CYCLIC_NORMAL");
   Array<Int> perm = FVIF[0];
   const Int n = perm.size();
   Array<Int> result(n);
   Array<Int> farr(f_vert);
   for (Int i = 0; i < n; ++i)
      result[i] = farr[perm[i]];
   return result;
}

// rotates vectors in V by angle a about axis u (given in hom. coords) using rodrigues' rotation formula
Matrix<double> rotate(const Matrix<double>& V, Vector<double> u, const double a)
{
   u[0] = 0; //projection to plane
   u /= norm(u); //unit length vector

   Matrix<double> K(3,3); //cross product matrix of axis u
   K(1,0) = u[3];
   K(0,2) = u[2];
   K(2,1) = u[1];
   K(0,1) = -u[3];
   K(2,0) = -u[2];
   K(1,2) = -u[1];

   Matrix<double> R = unit_matrix<double>(3) + sin(a)*K;
   R += (1-cos(a))*K*K;

   return V.minor(All, sequence(0,1)) | V.minor(All,sequence(1,3))*T(R);
}

template<typename T>
Matrix<T> create_square_vertices()
{
   Matrix<T> V(4,3);

   V(0,0)=V(1,0)=V(2,0)=V(3,0)=V(1,1)=V(2,2)=V(3,1)=V(3,2)=1;
   V(1,2)=V(2,1)=V(0,1)=V(0,2)=-1;

   return V;
}

//FIXME: coordinates #830
//creates vertices of regular n-gon with the origin as center, radius r and angle of first vertex s
Matrix<double> create_regular_polygon_vertices(const Int n, const double r, const double s)
{
   if (n < 3)
      throw std::runtime_error("At least three vertices required.");
   if (r <= 0)
      throw std::runtime_error("Radius must be >0");
   Matrix<double> V(n, 3);
   V.col(0).fill(1);

   double phi = 2*M_PI/double(n);
   for (Int i = 0; i < n; ++i) {
     V(i, 1) = r*cos(s+double(i)*phi);
     V(i, 2) = r*sin(s+double(i)*phi);
   }
   return V;
}

//creates an exact octagonal prism with z-coordinates z_1 and z_2
BigObject exact_octagonal_prism(QE z_1, QE z_2)
{
  Matrix<QE> V(16,4);
  V.col(0).fill(1);
  for (Int i = 0; i < 8; ++i) {
    V(i,3) = z_1;
    V(i+8,3) = z_2;
  }

  QE q(1,1,2);
  V(0,1)=V(1,2)=V(3,1)=V(6,2)=V(8,1)=V(9,2)=V(11,1)=V(14,2)=1;
  V(2,2)=V(4,1)=V(5,2)=V(7,1)=V(10,2)=V(12,1)=V(13,2)=V(15,1)=-1;
  V(0,2)=V(1,1)=V(2,1)=V(7,2)=V(8,2)=V(9,1)=V(10,1)=V(15,2)=q;
  V(3,2)=V(4,2)=V(5,1)=V(6,1)=V(11,2)=V(12,2)=V(13,1)=V(14,1)=-q;

  return BigObject("Polytope<QuadraticExtension>", "VERTICES", V);
}


//augments the facet given as a vertex index set
//uses sqrt and cos to calculate height, thus not exact
BigObject augment(BigObject p, const Set<Int>& f_vert)
{
   Matrix<double> V = p.give("VERTICES");

   Array<Int> neigh = neighbors(V,f_vert);
   double side_length = norm(V[neigh[0]]-V[neigh[1]]);

   Matrix<double> FV = V.minor(f_vert,All);
   Vector<double> bary = average(rows(FV));

   Vector<double> normal = find_facet_normal(V,f_vert);

   const Int n_vert = f_vert.size();
   if (n_vert == 3 || n_vert == 4 || n_vert == 5) { // pyramid
      double height = side_length*sqrt(1-1/(2-2*cos(2*M_PI/double(n_vert))));
      normal *= height;
      V /= (bary - normal);
   } else {
      Matrix<double> trans = zero_vector<double>( V.rows()) | repeat_row(-average(rows(FV)), V.rows()).minor(All,sequence(1,3));
      V += trans; //translate barycenter to zero
      double r = (side_length/(2*sin(M_PI/(double(n_vert)/2)))) ; //circumradius of n-gon with side_length
      Matrix<double> H(0, 4);
      Vector<double> v;
      for (Int i = 0; i < n_vert-1; i += 2) {
         v = (V.row(neigh[i])+V.row(neigh[i+1]))/2;
         H /= r*v/norm(v);
      }
      H = ones_vector<double>(H.rows()) | H.minor(All,sequence(1,3)); //adjust hom.coords
      V -= trans;
      H -= trans.minor(sequence(0,H.rows()),All);

      double h = sqrt(side_length*side_length - r*r); //height of regular pyramid
      H -= repeat_row(h*normal, H.rows());

      V /= H;
   }
   BigObject p_out("Polytope<Float>");
   p_out.take("VERTICES") << V;

   return p_out;
}

//places a rotunda on a facet given as a vertex index set (must be a decagonal facet)
//not exact
BigObject rotunda(BigObject p, const Set<Int>& f_vert)
{
   if (10 != f_vert.size()) throw std::runtime_error("Facet must be decagon.");

   Matrix<double> V = p.give("VERTICES");
   Array<Int> neigh = neighbors(V,f_vert);
   double side_length = norm(V[neigh[0]]-V[neigh[1]]);

   Vector<double> normal = find_facet_normal(V,f_vert);

   Matrix<double> FV = V.minor(f_vert,All);
   Vector<double> bary = average(rows(FV));
   Matrix<double> trans = zero_vector<double>( V.rows()) | repeat_row(-average(rows(FV)), V.rows()).minor(All,sequence(1,3));
   V += trans; //translate barycenter to zero

   double r_l = (1+sqrt(5))*sqrt(10*(5+sqrt(5)))*side_length/20;
   double r_s = sqrt((5+sqrt(5))/10)*side_length;
   Matrix<double> H_l(0,4);
   Matrix<double> H_s(0,4);
   Vector<double> v;
   for (Int i = 0; i < 9; i += 2) {
      v = (V.row(neigh[i])+V.row(neigh[i+1]))/2;
      H_l /= r_l*v/norm(v);
      if(i!=8) v = (V.row(neigh[i+1])+V.row(neigh[i+2]))/2;
      else v = (V.row(neigh[9])+V.row(neigh[0]))/2;
      H_s /= r_s*v/norm(v);
   }

   double r_i = norm(v); //radius of incircle

   V -= trans;
   H_l -= trans.minor(sequence(0,H_l.rows()), All);
   H_s -= trans.minor(sequence(0,H_s.rows()), All);

   double h_l = sqrt(3*side_length*side_length/4-(r_i-r_l)*(r_i-r_l));
   double h_s =  sqrt(1+2/sqrt(5))*side_length;
   H_l -= repeat_row(h_l*normal, 5);
   H_s -= repeat_row(h_s*normal, 5);
   Matrix<double>H = ones_vector<double>(10) | (H_l.minor(All,sequence(1,3)) / H_s.minor(All,sequence(1,3))); //adjust hom.coords
   V /= H;

   BigObject p_out("Polytope<Float>");
   p_out.take("VERTICES") << V;

   return p_out;
}

//places a rotated rotunda on a facet given as vertex index set (must be a decagonal facet)
//inexact
BigObject gyrotunda(BigObject p, const Set<Int>& f_vert)
{
   if (10 != f_vert.size()) throw std::runtime_error("Facet must be decagon.");

   Matrix<double> V = (p.give("VERTICES"));
   Array<Int> neigh = neighbors(V,f_vert);
   double side_length = norm(V[neigh[0]]-V[neigh[1]]);

   Vector<double> normal = find_facet_normal(V,f_vert);

   Matrix<double> FV = V.minor(f_vert,All);
   Vector<double> bary = average(rows(FV));
   Matrix<double> trans = zero_vector<double>( V.rows()) | repeat_row(-average(rows(FV)), V.rows()).minor(All,sequence(1,3));
   V += trans; //translate barycenter to zero

   double r_l = (1+sqrt(5))*sqrt(10*(5+sqrt(5)))*side_length/20;
   double r_s = sqrt((5+sqrt(5))/10)*side_length;
   Matrix<double> H_l(0,4);
   Matrix<double> H_s(0,4);
   Vector<double> v;
   for (Int i = 0; i < 9; i += 2) {
      v = (V.row(neigh[i])+V.row(neigh[i+1]))/2;
      H_s /= r_s*v/norm(v);
      if(i!=8) v = (V.row(neigh[i+1])+V.row(neigh[i+2]))/2;
      else v = (V.row(neigh[9])+V.row(neigh[0]))/2;
      H_l /= r_l*v/norm(v);
   }

   double r_i = norm(v); //radius of incircle

   V -= trans;
   H_l -= trans.minor(sequence(0,H_l.rows()), All);
   H_s -= trans.minor(sequence(0,H_s.rows()), All);

   double h_l = sqrt(3*side_length*side_length/4-(r_i-r_l)*(r_i-r_l));
   double h_s =  sqrt(1+2/sqrt(5))*side_length;
   H_l -= repeat_row(h_l*normal, 5);
   H_s -= repeat_row(h_s*normal, 5);
   Matrix<double>H = ones_vector<double>(10) | (H_l.minor(All,sequence(1,3)) / H_s.minor(All,sequence(1,3))); //adjust hom.coords
   V /= H;

   BigObject p_out("Polytope<Float>");
   p_out.take("VERTICES") << V;

   return p_out;
}

//elongates the facet given as vertex index set
//uses sqrt to calculate side length, thus not exact
BigObject elongate(BigObject p, const Set<Int>& f_vert)
{
   Matrix<double> V = (p.give("VERTICES"));

   Matrix<double> FV = V.minor(f_vert,All);

   Array<Int> neigh = neighbors(V,f_vert);
   double side_length = norm(V[neigh[0]]-V[neigh[1]]);

   Int n_vert = f_vert.size();

   BigObject p_out("Polytope<Float>");

   Vector<double> normal = find_facet_normal(V,f_vert);
   normal *= side_length; //facet normal of length side_length
   p_out.take("VERTICES") << (V / (FV - repeat_row(normal, n_vert)));

   return p_out;
}

//rotates the facet given as vertex index set by angle a
//uses sqrt to calculate side length, thus not exact
BigObject rotate_facet(BigObject p, const Set<Int>& f_vert, double a)
{
  const Matrix<double> V = p.give("VERTICES");
  const Matrix<double> FV = V.minor(f_vert, All);
  const Matrix<double> W = V.minor(~f_vert, All);
  const Vector<double> normal = find_facet_normal(V,f_vert);

  const Matrix<double> trans = zero_vector<double>(FV.rows()) | repeat_row(normal - average(rows(FV)), FV.rows()).minor(All,sequence(1,3)); // translate barycenter to axis
  const Matrix<double> Rot = rotate(FV+trans, normal, a)-trans;

  BigObject p_out("Polytope<Float>");
  p_out.take("VERTICES") << (W/Rot);

  return p_out;
}

//gyroelongates the facet given as vertex index set
//uses sqrt to calculate side length, thus not exact
BigObject gyroelongate(BigObject p, const Set<Int>& f_vert)
{
  Matrix<double> V = p.give("VERTICES");

  Matrix<double> FV = V.minor(f_vert,All);
  Vector<double> normal = find_facet_normal(V,f_vert);

  const Int n_vert = FV.rows();
  Matrix<double> trans = zero_vector<double>(n_vert) | repeat_row(normal - average(rows(FV)), FV.rows()).minor(All,sequence(1,3)); //translate barycenter to axis
  FV = rotate(FV+trans, normal, M_PI/double(n_vert))-trans; //rotate the facet by pi/n

  Array<Int> neigh = neighbors(V,f_vert);
  double side_length = norm(V[neigh[0]]-V[neigh[1]]);

  // wikipedia seems to be wrong https://en.wikipedia.org/wiki/Antiprism
  //   double height = 2*sqrt((cos(M_PI/n_vert)-cos(2.0*M_PI/n_vert))/2);
  //   the reciprocal might be correct
  // source: http://mathworld.wolfram.com/Antiprism.html
  double height = sqrt(1-1.0/(4.0*cos(M_PI/double(2*n_vert))*cos(M_PI/double(2*n_vert))));
  normal = (side_length*height)*normal; //facet normal of right length (height of a triangle)

  BigObject p_out("Polytope<Float>");
  p_out.take("VERTICES") << (V / (FV - repeat_row(normal, n_vert)));

  return p_out;
}

//creates regular n-gonal prism
BigObject create_prism(Int n)
{
   Matrix<double> V = create_regular_polygon_vertices(n, 1, 0);
   const double side_length = norm(V[0]-V[1]);
   return BigObject("Polytope<Float>",
                    "VERTICES", (V | zero_vector<double>()) / (V | same_element_vector(side_length, n)));
}

//removes the facet given as vertex index set
template <typename T>
BigObject diminish(BigObject p, const Set<Int>& f_vert)
{
  Matrix<T> V = p.give("VERTICES");

  Set<Int> v = sequence(0, V.rows());
  v -= f_vert;

  BigObject p_out("Polytope", mlist<T>());
  p_out.take("VERTICES") << V.minor(v,All);

  return p_out;
}

Matrix<QE> truncated_cube_vertices()
{
  Matrix<QE> V = exact_octagonal_prism(QE(0,0,0),QE(2,2,2)).give("VERTICES");
  Matrix<QE> W(8,4);
  W.col(0).fill(1);
  W(0,1)=W(1,1)=W(2,2)=W(3,2)=QE(2,1,2);
  W(4,1)=W(5,1)=W(6,2)=W(7,2)=-QE(2,1,2);
  W(0,3)=W(2,3)=W(4,3)=W(6,3)=QE(0,1,2);
  W(1,3)=W(3,3)=W(5,3)=W(7,3)=QE(2,1,2);

  return V / W;
}

template <typename E>
void centralize(BigObject& p)
{
  p.take("AFFINE_HULL") << Matrix<E>(0, 4);
  p = call_function("center", p);
}

template <typename E>
BigObject build_from_vertices(const Matrix<E>& V, bool do_centralize=true)
{
  BigObject p("Polytope", mlist<E>());
  p.take("VERTICES") << V;
  if (do_centralize) centralize<E>(p);
  return p;
}

} // end anonymous namespace

BigObject square_pyramid()
{
  Vector<QE> tip(4);
  tip[0]=1;
  tip[1]=tip[2]=0;
  tip[3]=QE(0,1,2);

  Matrix<QE> V((create_square_vertices<QE>() | zero_vector<QE>()) / tip);
  BigObject p=build_from_vertices(V);
  p.set_description() << "Johnson solid J1: Square pyramid" << endl;

  return p;
}

BigObject pentagonal_pyramid()
{
  BigObject ico = call_function("icosahedron");
  Matrix<QE> V = ico.give("VERTICES");
  V = V.minor(sequence(0,6),All);

  BigObject p=build_from_vertices(V);
  p.set_description() << "Johnson solid J2: Pentagonal pyramid" << endl;

  return p;
}

BigObject triangular_cupola()
{
  BigObject cub = call_function("cuboctahedron");
  Matrix<Rational> V = cub.give("VERTICES");
  V = V.minor(sequence(0,9),All);

  BigObject p=build_from_vertices(V);
  p.set_description() << "Johnson solid J3: Triangular cupola" << endl;

  return p;
}

BigObject square_cupola_impl(bool centered)
{
  BigObject base = exact_octagonal_prism(QE(0,0,0),QE(1,0,0));

  Matrix<QE> B =  base.give("VERTICES");
  Matrix<QE> V = B.minor(sequence(0,8),All);

  QE height(0,1,2);

  Matrix<QE> W(4,4);
  W.col(0).fill(1);
  W.col(3).fill(height);
  W(0,1) = W(0,2) = W(1,1) = W(2,2) = 1;
  W(1,2) = W(2,1) = W(3,1) = W(3,2) = -1;

  V /= W;

  BigObject p = build_from_vertices(V, centered);
  p.set_description() << "Johnson solid J4: Square cupola" << endl;
  return p;
}

BigObject square_cupola()
{
  return square_cupola_impl(true);
}

BigObject pentagonal_cupola()
{
  BigObject rico = call_function("rhombicosidodecahedron");
  Matrix<QE> V = rico.give("VERTICES");
  V = V.minor(sequence(0,7),All) / V.minor(sequence(8,3),All) / V.row(13) / V.row(14) / V.row(18) / V.row(19) / V.row(24);

  BigObject p = build_from_vertices(V);
  p.set_description() << "Johnson solid J5: Pentagonal cupola" << endl;
  return p;
}

BigObject pentagonal_rotunda()
{
  BigObject ico = call_function("icosidodecahedron");
  Matrix<QE> V = ico.give("VERTICES");
  V = V.minor(sequence(0,17),All) / V.row(18) / V.row(19) / V.row(21);

  BigObject p = build_from_vertices(V);
  p.set_description() << "Johnson solid J6: Pentagonal rotunda" << endl;

  return p;
}

BigObject elongated_triangular_pyramid()
{
  QE c(Rational(-1,3),0,0);

  QE s(0,Rational(1,3),6);

  Matrix<QE> V( ones_vector<QE>(7) | (same_element_vector(c,3) / unit_matrix<QE>(3) / (unit_matrix<QE>(3) + same_element_matrix(s, 3, 3))));

  BigObject p = build_from_vertices(V);
  p.set_description() << "Johnson solid J7: Elongated triangular pyramid" << endl;
  return p;
}

BigObject elongated_square_pyramid_impl(bool centered)
{
  Matrix<QE> square_vertices =  create_square_vertices<QE>();

  Vector<QE> tip(4);
  tip[0] = 1;
  tip[3] = QE(0,1,2);

  Matrix<QE> V( ((square_vertices | zero_vector<QE>(4)) / (square_vertices | -2*ones_vector<QE>(4))) / tip );

  BigObject p = build_from_vertices(V, centered);
  p.set_description() << "Johnson solid J8: Elongated square pyramid" << endl;
  return p;
}

BigObject elongated_square_pyramid()
{
  return elongated_square_pyramid_impl(true);
}

//FIXME: #830 coordinates
BigObject elongated_pentagonal_pyramid()
{
  BigObject p = pentagonal_pyramid();
  p = elongate(p, sequence(1,5));

  IncidenceMatrix<> VIF{ {6,7,8,9,10},
                         {1,3,6,8},
                         {3,5,8,10},
                         {0,3,5},
                         {0,1,3},
                         {0,4,5},
                         {4,5,9,10},
                         {0,2,4},
                         {0,1,2},
                         {1,2,6,7},
                         {2,4,7,9} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J9: Elongated pentagonal pyramid" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject gyroelongated_square_pyramid()
{
   BigObject p = square_pyramid();
   p = gyroelongate(p, sequence(0,4));

   IncidenceMatrix<> VIF{ {1,3,4},
                          {2,3,8},
                          {2,3,4},
                          {2,7,8},
                          {0,2,7},
                          {0,5,7},
                          {0,2,4},
                          {0,1,5},
                          {0,1,4},
                          {1,5,6},
                          {3,6,8},
                          {1,3,6},
                          {5,6,7,8} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J10: Gyroelongated square pyramid" << endl;

   return p;
}

BigObject gyroelongated_pentagonal_pyramid()
{
   BigObject ico = call_function("icosahedron");
   Matrix<QE> V = ico.give("VERTICES");
   V = V.minor(sequence(0,11),All);

   BigObject p = build_from_vertices(V);
   p.set_description() << "Johnson solid J11: Gyroelongated pentagonal pyramid" << endl;
   return p;
}

BigObject triangular_bipyramid()
{
  Rational c(-1,3);

  Matrix<Rational> V( ones_vector<Rational>(5) | unit_matrix<Rational>(3) / ones_vector<Rational>(3) / same_element_vector(c, 3));

  BigObject p = build_from_vertices(V);
  p.set_description() << "Johnson solid J12: Triangular bipyramid" << endl;
  return p;
}

//FIXME: coordinates #830
BigObject pentagonal_bipyramid()
{
  BigObject p = pentagonal_pyramid();
  p = augment(p, sequence(1,5));

  IncidenceMatrix<> VIF { {0,4,5},
                          {4,5,6},
                          {3,5,6},
                          {1,3,6},
                          {0,1,3},
                          {0,3,5},
                          {0,1,2},
                          {1,2,6},
                          {2,4,6},
                          {0,2,4} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J13: Pentagonal bipyramid" << endl;

  return p;
}

BigObject elongated_triangular_bipyramid()
{
  QE c(Rational(-1,3),0,0);

  QE s(0,Rational(1,3),6);

  Matrix<QE> V( ones_vector<QE>() | (same_element_vector(1+s, 3) / (same_element_vector(c, 3) / unit_matrix<QE>(3) / (unit_matrix<QE>(3) + same_element_matrix(s, 3, 3)))));

  BigObject p = build_from_vertices(V);
  p.set_description() << "Johnson solid J14: Elongated triangular bipyramid" << endl;
  return p;
}

BigObject elongated_square_bipyramid()
{
  BigObject esp = elongated_square_pyramid_impl(false);

  Matrix<QE> esp_vertices =  esp.give("VERTICES");

  Vector<QE> tip(4);
  tip[0]=1;
  tip[1]=tip[2]=0;
  tip[3]=QE(-2,-1,2);

  Matrix<QE> V = esp_vertices / tip;

  BigObject p = build_from_vertices(V);
  p.set_description() << "Johnson solid J15: Elongated square bipyramid" << endl;
  return p;
}

//FIXME: coordinates #830
BigObject elongated_pentagonal_bipyramid()
{
  BigObject p = elongated_pentagonal_pyramid();

  p = augment(p,sequence(6,5));

  IncidenceMatrix<> VIF{ {7,9,11},
                         {6,7,11},
                         {9,10,11},
                         {1,3,6,8},
                         {3,5,8,10},
                         {8,10,11},
                         {6,8,11},
                         {0,3,5},
                         {0,1,3},
                         {0,4,5},
                         {4,5,9,10},
                         {0,2,4},
                         {0,1,2},
                         {1,2,6,7},
                         {2,4,7,9} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J16: Elongated pentagonal bipyramid" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject gyroelongated_square_bipyramid()
{
   BigObject p = gyroelongated_square_pyramid();
   p = augment(p,sequence(5,4));

   IncidenceMatrix<> VIF{ {1,3,4},
                          {2,3,8},
                          {2,3,4},
                          {7,8,9},
                          {2,7,8},
                          {5,7,9},
                          {0,5,7},
                          {0,2,7},
                          {0,2,4},
                          {0,1,5},
                          {0,1,4},
                          {5,6,9},
                          {1,5,6},
                          {6,8,9},
                          {3,6,8},
                          {1,3,6} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J17: Gyroelongated square bipyramid" << endl;

   return p;
}

//FIXME: coordinates #830
BigObject elongated_triangular_cupola()
{
   BigObject p = triangular_cupola();
   p = elongate(p,sequence(3,6));

   IncidenceMatrix<> VIF{ {1,2,6,8},
                          {1,5,6},
                          {5,6,11,12},
                          {4,7,10,13},
                          {7,8,13,14},
                          {6,8,12,14},
                          {2,7,8},
                          {9,10,11,12,13,14},
                          {3,4,9,10},
                          {3,5,9,11},
                          {0,3,4},
                          {0,1,2},
                          {0,2,4,7},
                          {0,1,3,5} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J18: Elongated triangular cupola" << endl;

   return p;
}


BigObject elongated_square_cupola_impl(bool centered)
{
  BigObject base = exact_octagonal_prism(QE(-2,0,0),QE(0,0,0));
  Matrix<QE> V =  base.give("VERTICES");
  Matrix<QE> T = square_cupola_impl(false).give("VERTICES");
  V /= T.minor(sequence(8,4),All);

  BigObject p = build_from_vertices(V, centered);
  p.set_description() << "Johnson solid J19: Elongated square cupola" << endl;
  return p;
}

BigObject elongated_square_cupola()
{
   return elongated_square_cupola_impl(true);
}

//FIXME: coordinates #830
BigObject elongated_pentagonal_cupola()
{
   BigObject p = pentagonal_cupola();
   p = elongate(p, Set<Int>{2,4,5,7,8,10,11,12,13,14});

   IncidenceMatrix<> VIF{ {15,16,17,18,19,20,21,22,23,24},
                          {0,1,2,4},
                          {1,6,8,10},
                          {1,4,8},
                          {8,10,19,20},
                          {10,13,20,23},
                          {6,10,13},
                          {13,14,23,24},
                          {4,8,16,19},
                          {6,9,13,14},
                          {9,12,14},
                          {12,14,22,24},
                          {2,4,15,16},
                          {0,1,3,6,9},
                          {3,9,11,12},
                          {11,12,21,22},
                          {0,2,5},
                          {2,5,15,17},
                          {3,7,11},
                          {0,3,5,7},
                          {7,11,18,21},
                          {5,7,17,18} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J20: Elongated pentagonal cupola" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject elongated_pentagonal_rotunda()
{
   BigObject p = pentagonal_rotunda();
   p = elongate(p, Set<Int>{7,9,10,12,13,15,16,17,18,19});

   IncidenceMatrix<> VIF{ {20,21,22,23,24,25,26,27,28,29},
                          {7,10,20,22},
                          {3,7,10},
                          {16,17,26,27},
                          {0,1,2,4,6},
                          {0,1,3},
                          {4,6,14},
                          {6,11,14,18,19},
                          {9,13,21,24},
                          {5,9,13},
                          {18,19,28,29},
                          {2,5,11,13,15},
                          {11,15,18},
                          {15,18,25,28},
                          {13,15,24,25},
                          {2,6,11},
                          {0,2,5},
                          {14,17,19},
                          {17,19,27,29},
                          {7,9,20,21},
                          {0,3,5,7,9},
                          {1,4,8},
                          {4,8,14,16,17},
                          {8,12,16},
                          {1,3,8,10,12},
                          {12,16,23,26},
                          {10,12,22,23} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J21: Elongated pentagonal rotunda" << endl;
   return p;
}

//FIXME: coordinates #830
BigObject gyroelongated_triangular_cupola()
{
  BigObject p = triangular_cupola();

  p = gyroelongate(p,sequence(3,6));

  IncidenceMatrix<> VIF{ {1,2,6,8},
                         {1,5,6},
                         {5,11,12},
                         {5,6,12},
                         {2,7,8},
                         {6,12,14},
                         {6,8,14},
                         {8,13,14},
                         {7,8,13},
                         {7,10,13},
                         {4,7,10},
                         {4,9,10},
                         {9,10,11,12,13,14},
                         {3,4,9},
                         {3,9,11},
                         {3,5,11},
                         {0,3,4},
                         {0,1,2},
                         {0,2,4,7},
                         {0,1,3,5} };

  p.take("VERTICES_IN_FACETS") << VIF;
  centralize<double>(p);

  p.set_description() << "Johnson solid J22: Gyroelongated triangular cupola" << endl;
  return p;
}

//FIXME: coordinates #830
BigObject gyroelongated_square_cupola()
{
  Matrix<double> V = square_cupola_impl(false).give("VERTICES");
  const double height = -2*sqrt(1-1.0/(4.0*cos(M_PI/16)*cos(M_PI/16)));
  const Matrix<double> W = create_regular_polygon_vertices(8, sqrt(2)*sqrt(2+sqrt(2)), 0);
  const Matrix<double> X = (W.minor(All, sequence(0,3)) | same_element_vector(height, 8)) / V;

  IncidenceMatrix<> VIF{ {0,1,9},
                         {1,2,8},
                         {2,3,15},
                         {3,4,14},
                         {4,5,13},
                         {5,6,12},
                         {6,7,11},
                         {0,7,10},
                         {1,8,9},
                         {0,9,10},
                         {7,10,11},
                         {6,11,12},
                         {5,12,13},
                         {4,13,14},
                         {3,14,15},
                         {2,8,15},
                         {0,1,2,3,4,5,6,7},
                         {9,10,16,17},
                         {8,9,16},
                         {13,14,18,19},
                         {8,15,16,18},
                         {10,11,17},
                         {11,12,17,19},
                         {14,15,18},
                         {12,13,19},
                         {16,17,18,19} };

  BigObject p("Polytope<Float>");
  p.take("VERTICES") << X;
  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J23: Gyroelongated square cupola" << endl;
  return p;
}

//FIXME: coordinates #830
BigObject gyroelongated_pentagonal_cupola()
{
  BigObject p = pentagonal_cupola();
  p = gyroelongate(p, Set<Int>{2,4,5,7,8,10,11,12,13,14});

  IncidenceMatrix<> VIF{ {15,16,17,18,19,20,21,22,23,24},
                         {5,17,18},
                         {2,5,17},
                         {0,2,5},
                         {2,15,17},
                         {0,1,3,6,9},
                         {2,4,15},
                         {4,15,16},
                         {14,23,24},
                         {4,8,16},
                         {6,10,13},
                         {13,20,23},
                         {10,19,20},
                         {10,13,20},
                         {8,10,19},
                         {8,16,19},
                         {13,14,23},
                         {1,4,8},
                         {1,6,8,10},
                         {6,9,13,14},
                         {12,14,24},
                         {9,12,14},
                         {0,1,2,4},
                         {12,22,24},
                         {11,12,22},
                         {3,9,11,12},
                         {11,21,22},
                         {3,7,11},
                         {7,11,21},
                         {5,7,18},
                         {7,18,21},
                         {0,3,5,7} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J24: Gyroelongated pentagonal cupola" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject gyroelongated_pentagonal_rotunda()
{
   BigObject p = pentagonal_rotunda();
   p = gyroelongate(p, Set<Int>{7,9,10,12,13,15,16,17,18,19});

   IncidenceMatrix<> VIF{ {20,21,22,23,24,25,26,27,28,29},
                          {10,22,23},
                          {4,8,14,16,17},
                          {1,4,8},
                          {16,17,27},
                          {17,27,29},
                          {0,3,5,7,9},
                          {7,9,20},
                          {0,2,5},
                          {6,11,14,18,19},
                          {2,6,11},
                          {18,19,28},
                          {18,25,28},
                          {13,15,24},
                          {15,18,25},
                          {15,24,25},
                          {11,15,18},
                          {13,21,24},
                          {2,5,11,13,15},
                          {9,13,21},
                          {5,9,13},
                          {19,28,29},
                          {9,20,21},
                          {17,19,29},
                          {14,17,19},
                          {4,6,14},
                          {0,1,3},
                          {0,1,2,4,6},
                          {7,20,22},
                          {3,7,10},
                          {7,10,22},
                          {16,26,27},
                          {8,12,16},
                          {12,16,26},
                          {10,12,23},
                          {12,23,26},
                          {1,3,8,10,12} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J25: Gyroelongated pentagonal rotunda" << endl;
   return p;
}

BigObject gyrobifastigium(){

  QE o(0), l(1), h(0,1,3);
  Matrix<QE> V{ {l, -l, -l,  o},
                {l,  l, -l,  o},
                {l, -l,  l,  o},
                {l,  l,  l,  o},
                {l,  l,  o,  h},
                {l,  o,  l, -h},
                {l, -l,  o,  h},
                {l,  o, -l, -h} };
  BigObject p=build_from_vertices(V);
  p.set_description() << "Johnson solid J26: Gyrobifastigium" << endl;

  return p;
}

BigObject triangular_orthobicupola()
{
  QE s(0,Rational(-1,9),3);
  Vector<QE> trans(4); //unit normal vector
  trans[0]=0;
  trans[1]=trans[2]=s;
  trans[3]=-5*s;

  Matrix<QE> triangle_V(3,4);
  triangle_V.col(0).fill(1);

  triangle_V(0,1)=triangle_V(0,3)=1;
  triangle_V(0,2)=4;
  triangle_V(1,1)=8;
  triangle_V(1,2)=triangle_V(1,3)=2;
  triangle_V(2,1)=6;
  triangle_V(2,2)=9;
  triangle_V(2,3)=3;

  Matrix<QE> hexagon_V(6,4);
  hexagon_V.col(0).fill(1);
  hexagon_V(1,1)=hexagon_V(2,2)=7;
  hexagon_V(1,2)=hexagon_V(2,1)=-2;
  hexagon_V(1,3)=hexagon_V(2,3)=1;
  hexagon_V(3,2)=hexagon_V(3,3)=hexagon_V(4,1)=hexagon_V(4,3)=3;
  hexagon_V(3,1)=hexagon_V(4,2)=12;
  hexagon_V(5,1)=hexagon_V(5,2)=10;
  hexagon_V(5,3)=4;

  Matrix<QE> V=( hexagon_V / (triangle_V + repeat_row(6*trans,3)) / (triangle_V - repeat_row(6*trans,3)));

  BigObject p=build_from_vertices(V);
  p.set_description() << "Johnson solid J27: Triangular orthobicupola" << endl;

  return p;
}

BigObject square_orthobicupola()
{
  Matrix<QE> V = square_cupola_impl(false).give("VERTICES");
  V /= (ones_vector<QE>(4) | (-1)*V.minor(sequence(8,4),sequence(1,3)));

  BigObject p=build_from_vertices(V);
  p.set_description() << "Johnson solid J28: Square orthobicupola" << endl;

  return p;
}

BigObject square_gyrobicupola()
{
  QE rot(0,Rational(1,2),2);
  Matrix<QE> R(3,3);
  R(0,0)=R(1,0)=R(1,1)=rot;
  R(0,1)=-rot;
  R(2,2)=1;
  Matrix<QE> V = square_cupola_impl(false).give("VERTICES");
  V /= (ones_vector<QE>(4) | (-1)*(V.minor(sequence(8,4),sequence(1,3))*R));

  BigObject p=build_from_vertices(V);
  p.set_description() << "Johnson solid J29: Square gyrobicupola" << endl;

  return p;
}

BigObject pentagonal_orthobicupola()
{
   BigObject p = pentagonal_cupola();
   p = augment(p, Set<Int>{2,4,5,7,8,10,11,12,13,14});

   IncidenceMatrix<> VIF{{0,2,5},
                         {2,5,15},
                         {0,1,3,6,9},
                         {0,1,2,4},
                         {6,9,13,14},
                         {1,6,8,10},
                         {1,4,8},
                         {8,10,18,19},
                         {6,10,13},
                         {10,13,18},
                         {4,8,19},
                         {13,14,17,18},
                         {12,14,17},
                         {9,12,14},
                         {2,4,15,19},
                         {15,16,17,18,19},
                         {11,12,16,17},
                         {3,9,11,12},
                         {3,7,11},
                         {7,11,16},
                         {5,7,15,16},
                         {0,3,5,7}};

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J30: Pentagonal orthobicupola" << endl;

   return p;
}

BigObject pentagonal_gyrobicupola()
{
   BigObject p = pentagonal_pyramid();
   p = call_function("minkowski_sum", 1, p, -1, p);

   p.set_description() << "Johnson solid J31: Pentagonal gyrobicupola" << endl;

   return p;
}

//FIXME: coordinates #830
BigObject pentagonal_gyrocupolarotunda()
{
   BigObject p = pentagonal_rotunda();
   p = augment(p, Set<Int>{7,9,10,12,13,15,16,17,18,19});

   IncidenceMatrix<> VIF{{4,8,14,16,17},
                          {1,4,8},
                          {16,17,21,22},
                          {0,3,5,7,9},
                          {4,6,14},
                          {0,2,5},
                          {18,19,22,23},
                          {9,13,24},
                          {5,9,13},
                          {11,15,18},
                          {15,18,23},
                          {2,5,11,13,15},
                          {13,15,23,24},
                          {2,6,11},
                          {6,11,14,18,19},
                          {17,19,22},
                          {14,17,19},
                          {7,9,20,24},
                          {20,21,22,23,24},
                          {0,1,3},
                          {0,1,2,4,6},
                          {7,10,20},
                          {3,7,10},
                          {12,16,21},
                          {8,12,16},
                          {10,12,20,21},
                          {1,3,8,10,12}};

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J33: Pentagonal gyrocupolarotunda" << endl;

   return p;
}

//FIXME: coordinates #830
BigObject pentagonal_orthocupolarotunda()
{
   BigObject p = pentagonal_gyrocupolarotunda();
   p = rotate_facet(p, sequence(20,5), M_PI/5);

   IncidenceMatrix<> VIF{ {4,8,14,16,17},
                          {1,4,8},
                          {16,17,22},
                          {0,3,5,7,9},
                          {4,6,14},
                          {17,19,22,23},
                          {14,17,19},
                          {0,2,5},
                          {9,13,20,24},
                          {5,9,13},
                          {15,18,23,24},
                          {11,15,18},
                          {13,15,24},
                          {2,5,11,13,15},
                          {18,19,23},
                          {2,6,11},
                          {6,11,14,18,19},
                          {7,9,20},
                          {20,21,22,23,24},
                          {0,1,3},
                          {0,1,2,4,6},
                          {3,7,10},
                          {7,10,20,21},
                          {10,12,21},
                          {8,12,16},
                          {12,16,21,22},
                          {1,3,8,10,12}};

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J32: Pentagonal orthocupolarotunda" << endl;

   return p;
}

//FIXME: coordinates #830
BigObject pentagonal_orthobirotunda()
{
   BigObject p = pentagonal_rotunda();
   Vector<double> normal{ 0., 0., (1+sqrt(5))/2, 1. };
   p = rotunda(p, Set<Int>{7,9,10,12,13,15,16,17,18,19});

/*   IncidenceMatrix<> VIF{
                       };

   p.take("VERTICES_IN_FACETS") << VIF;*/

   centralize<double>(p);
   p.set_description() << "Johnson solid J34: Pentagonal orthobirotunda" << endl;

   return p;
}

//FIXME: coordinates #830
BigObject elongated_triangular_orthobicupola()
{
   BigObject p = elongated_triangular_cupola();
   p = augment(p, Set<Int>{9,10,11,12,13,14});
   p = rotate_facet(p, Set<Int>{15,16,17}, M_PI/3);

   IncidenceMatrix<> VIF{ {1,2,6,8},
                          {1,5,6},
                          {5,6,11,12},
                          {11,12,16},
                          {4,7,10,13},
                          {10,13,15,17},
                          {7,8,13,14},
                          {15,16,17},
                          {13,14,17},
                          {12,14,16,17},
                          {6,8,12,14},
                          {2,7,8},
                          {9,10,15},
                          {9,11,15,16},
                          {3,4,9,10},
                          {3,5,9,11},
                          {0,3,4},
                          {0,1,2},
                          {0,2,4,7},
                          {0,1,3,5}};

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J35: Elongated triangular orthobicupola" << endl;

   return p;
}

//FIXME: coordinates #830
BigObject elongated_triangular_gyrobicupola()
{
   BigObject p = elongated_triangular_cupola();
   p = augment(p,sequence(9,6));

   IncidenceMatrix<> VIF{ {1,2,6,8},
                          {1,5,6},
                          {5,6,11,12},
                          {11,12,15,16},
                          {4,7,10,13},
                          {10,13,17},
                          {15,16,17},
                          {13,14,16,17},
                          {7,8,13,14},
                          {12,14,16},
                          {6,8,12,14},
                          {2,7,8},
                          {9,11,15},
                          {9,10,15,17},
                          {3,4,9,10},
                          {3,5,9,11},
                          {0,3,4},
                          {0,1,2},
                          {0,2,4,7},
                          {0,1,3,5}};

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J36: Elongated triangular gyrobicupola" << endl;
   return p;
}

BigObject elongated_square_gyrobicupola()
{
  Matrix<QE> V = elongated_square_cupola_impl(false).give("VERTICES");
  Matrix<QE> W = square_gyrobicupola().give("VERTICES");
  V /= W.minor(sequence(12,4),All);
  V(20,3)=V(21,3)=V(22,3)=V(23,3)= V(20,3)-2;

  BigObject p = build_from_vertices(V);
  p.set_description() << "Johnson solid J37: Elongated square gyrobicupola" << endl;
  return p;
}

//FIXME: coordinates #830
BigObject elongated_pentagonal_gyrobicupola()
{
   BigObject p = elongated_pentagonal_cupola();
   p = augment(p,sequence(15,10));
   p = rotate_facet(p, sequence(25,5), M_PI/5);

   IncidenceMatrix<> VIF{{18,21,26,27},
                          {17,18,26},
                          {15,17,25,26},
                          {21,22,27},
                          {25,26,27,28,29},
                          {2,4,15,16},
                          {12,14,22,24},
                          {9,12,14},
                          {16,19,25,29},
                          {4,8,16,19},
                          {13,14,23,24},
                          {20,23,28,29},
                          {19,20,29},
                          {10,13,20,23},
                          {8,10,19,20},
                          {6,10,13},
                          {23,24,28},
                          {1,4,8},
                          {1,6,8,10},
                          {6,9,13,14},
                          {22,24,27,28},
                          {15,16,25},
                          {0,1,2,4},
                          {0,1,3,6,9},
                          {3,9,11,12},
                          {11,12,21,22},
                          {0,2,5},
                          {2,5,15,17},
                          {3,7,11},
                          {0,3,5,7},
                          {7,11,18,21},
                          {5,7,17,18} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J39: Elongated pentagonal gyrobicupola" << endl;

   return p;
}

//FIXME: coordinates #830
BigObject elongated_pentagonal_orthobicupola()
{
   BigObject p = elongated_pentagonal_cupola();
   p = augment(p,sequence(15,10));

   IncidenceMatrix<> VIF{ {17,18,25,26},
                          {18,21,26},
                          {15,17,25},
                          {21,22,26,27},
                          {25,26,27,28,29},
                          {2,4,15,16},
                          {12,14,22,24},
                          {9,12,14},
                          {22,24,27},
                          {23,24,27,28},
                          {4,8,16,19},
                          {13,14,23,24},
                          {20,23,28},
                          {10,13,20,23},
                          {8,10,19,20},
                          {6,10,13},
                          {19,20,28,29},
                          {16,19,29},
                          {1,4,8},
                          {1,6,8,10},
                          {6,9,13,14},
                          {15,16,25,29},
                          {0,1,2,4},
                          {0,1,3,6,9},
                          {3,9,11,12},
                          {11,12,21,22},
                          {0,2,5},
                          {2,5,15,17},
                          {3,7,11},
                          {0,3,5,7},
                          {7,11,18,21},
                          {5,7,17,18} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J38: Elongated pentagonal orthobicupola" << endl;

   return p;
}

//FIXME: coordinates #830
BigObject elongated_pentagonal_orthocupolarotunda()
{
   BigObject p = elongated_pentagonal_rotunda();
   p = augment(p, sequence(20,10));
   p = rotate_facet(p, sequence(30,5), M_PI/5);

   IncidenceMatrix<> VIF{ {23,26,31,32},
                          {22,23,31},
                          {7,10,20,22},
                          {3,7,10},
                          {16,17,26,27},
                          {0,1,2,4,6},
                          {0,1,3},
                          {30,31,32,33,34},
                          {7,9,20,21},
                          {17,19,27,29},
                          {14,17,19},
                          {6,11,14,18,19},
                          {2,6,11},
                          {28,29,33},
                          {2,5,11,13,15},
                          {24,25,34},
                          {13,15,24,25},
                          {15,18,25,28},
                          {11,15,18},
                          {25,28,33,34},
                          {18,19,28,29},
                          {5,9,13},
                          {9,13,21,24},
                          {21,24,30,34},
                          {0,2,5},
                          {27,29,32,33},
                          {20,21,30},
                          {4,6,14},
                          {0,3,5,7,9},
                          {26,27,32},
                          {20,22,30,31},
                          {1,4,8},
                          {4,8,14,16,17},
                          {8,12,16},
                          {1,3,8,10,12},
                          {12,16,23,26},
                          {10,12,22,23} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J40: Elongated pentagonal orthocupolarotunda" << endl;

   return p;
}

      //FIXME: coordinates #830
BigObject elongated_pentagonal_gyrocupolarotunda()
{
   BigObject p = elongated_pentagonal_rotunda();
   p = augment(p, sequence(20,10));

   IncidenceMatrix<> VIF{ {22,23,30,31},
                          {23,26,31},
                          {7,10,20,22},
                          {3,7,10},
                          {16,17,26,27},
                          {0,1,2,4,6},
                          {0,1,3},
                          {30,31,32,33,34},
                          {7,9,20,21},
                          {17,19,27,29},
                          {14,17,19},
                          {27,29,32},
                          {6,11,14,18,19},
                          {2,6,11},
                          {21,24,34},
                          {24,25,33,34},
                          {2,5,11,13,15},
                          {25,28,33},
                          {13,15,24,25},
                          {15,18,25,28},
                          {11,15,18},
                          {18,19,28,29},
                          {5,9,13},
                          {9,13,21,24},
                          {28,29,32,33},
                          {0,2,5},
                          {20,21,30,34},
                          {4,6,14},
                          {0,3,5,7,9},
                          {26,27,31,32},
                          {20,22,30},
                          {1,4,8},
                          {4,8,14,16,17},
                          {8,12,16},
                          {1,3,8,10,12},
                          {12,16,23,26},
                          {10,12,22,23} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J41: Elongated pentagonal gyrocupolarotunda" << endl;

   return p;
}

//FIXME: coordinates #830
BigObject elongated_pentagonal_orthobirotunda()
{
   BigObject p = elongated_pentagonal_rotunda();
   p = rotunda(p, sequence(20,10));

   IncidenceMatrix<> VIF{ {22,23,30,31,35},
                          {23,26,31},
                          {7,10,20,22},
                          {3,7,10},
                          {16,17,26,27},
                          {31,35,36},
                          {0,3,5,7,9},
                          {35,36,37,38,39},
                          {7,9,20,21},
                          {17,19,27,29},
                          {14,17,19},
                          {32,36,37},
                          {6,11,14,18,19},
                          {28,29,32,33,37},
                          {9,13,21,24},
                          {5,9,13},
                          {18,19,28,29},
                          {33,37,38},
                          {13,15,24,25},
                          {15,18,25,28},
                          {25,28,33},
                          {11,15,18},
                          {24,25,33,34,38},
                          {2,5,11,13,15},
                          {21,24,34},
                          {2,6,11},
                          {34,38,39},
                          {0,2,5},
                          {27,29,32},
                          {4,6,14},
                          {30,35,39},
                          {20,21,30,34,39},
                          {0,1,3},
                          {0,1,2,4,6},
                          {26,27,31,32,36},
                          {20,22,30},
                          {1,4,8},
                          {4,8,14,16,17},
                          {8,12,16},
                          {1,3,8,10,12},
                          {12,16,23,26},
                          {10,12,22,23} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J42: Elongated pentagonal orthobirotunda" << endl;

   return p;
}

      //FIXME: coordinates #830
BigObject elongated_pentagonal_gyrobirotunda()
{
   BigObject p = elongated_pentagonal_rotunda();
   p = gyrotunda(p, sequence(20,10));

   IncidenceMatrix<> VIF{ {23,26,30,31,36},
                          {22,23,30},
                          {7,10,20,22},
                          {3,7,10},
                          {16,17,26,27},
                          {30,35,36},
                          {0,3,5,7,9},
                          {35,36,37,38,39},
                          {7,9,20,21},
                          {17,19,27,29},
                          {14,17,19},
                          {34,35,39},
                          {6,11,14,18,19},
                          {32,37,38},
                          {9,13,21,24},
                          {5,9,13},
                          {18,19,28,29},
                          {33,38,39},
                          {13,15,24,25},
                          {15,18,25,28},
                          {24,25,33},
                          {11,15,18},
                          {25,28,32,33,38},
                          {2,5,11,13,15},
                          {28,29,32},
                          {2,6,11},
                          {21,24,33,34,39},
                          {0,2,5},
                          {20,21,34},
                          {4,6,14},
                          {31,36,37},
                          {27,29,31,32,37},
                          {0,1,3},
                          {0,1,2,4,6},
                          {26,27,31},
                          {20,22,30,34,35},
                          {1,4,8},
                          {4,8,14,16,17},
                          {8,12,16},
                          {1,3,8,10,12},
                          {12,16,23,26},
                          {10,12,22,23} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J43: Elongated pentagonal gyrobirotunda" << endl;

   return p;
}

//FIXME: coordinates #830
BigObject gyroelongated_triangular_bicupola()
{
   BigObject p = gyroelongated_triangular_cupola();
   p = augment(p, sequence(9,6));

   IncidenceMatrix<> VIF{  {1,2,6,8},
                          {1,5,6},
                          {5,11,12},
                          {5,6,12},
                          {2,7,8},
                          {6,12,14},
                          {6,8,14},
                          {12,14,16},
                          {7,8,13},
                          {7,10,13},
                          {13,14,16,17},
                          {10,13,17},
                          {8,13,14},
                          {15,16,17},
                          {9,10,15,17},
                          {4,7,10},
                          {4,9,10},
                          {9,11,15},
                          {11,12,15,16},
                          {3,4,9},
                          {3,9,11},
                          {3,5,11},
                          {0,3,4},
                          {0,1,2},
                          {0,2,4,7},
                          {0,1,3,5} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J44: Gyroelongated triangular bicupola" << endl;

   return p;
}

//FIXME: coordinates #830
BigObject gyroelongated_square_bicupola()
{
   BigObject p = gyroelongated_square_cupola();
   p = augment(p,sequence(0,8));

   IncidenceMatrix<> VIF{ {9,10,16,17},
                          {8,9,16},
                          {1,8,9},
                          {1,2,8},
                          {1,2,23},
                          {20,21,22,23},
                          {16,17,18,19},
                          {6,11,12},
                          {2,3,15},
                          {4,5,21,22},
                          {13,14,18,19},
                          {5,12,13},
                          {3,4,14},
                          {4,5,13},
                          {4,13,14},
                          {3,4,22},
                          {12,13,19},
                          {3,14,15},
                          {14,15,18},
                          {5,6,12},
                          {5,6,21},
                          {2,3,22,23},
                          {11,12,17,19},
                          {2,8,15},
                          {8,15,16,18},
                          {6,7,11},
                          {6,7,20,21},
                          {10,11,17},
                          {7,10,11},
                          {0,7,20},
                          {0,7,10},
                          {0,1,9},
                          {0,9,10},
                          {0,1,20,23} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J45: Gyroelongated square bicupola" << endl;

   return p;
}

      //FIXME: coordinates #830
BigObject gyroelongated_pentagonal_bicupola()
{
   BigObject p = gyroelongated_pentagonal_cupola();
   p = augment(p,sequence(15,10));

   IncidenceMatrix<> VIF{ {17,18,25,26},
                          {18,21,26},
                          {5,17,18},
                          {2,5,17},
                          {0,2,5},
                          {15,17,25},
                          {2,15,17},
                          {25,26,27,28,29},
                          {0,1,2,4},
                          {9,12,14},
                          {12,14,24},
                          {23,24,27,28},
                          {6,9,13,14},
                          {1,6,8,10},
                          {1,4,8},
                          {13,14,23},
                          {20,23,28},
                          {8,16,19},
                          {8,10,19},
                          {10,13,20},
                          {10,19,20},
                          {13,20,23},
                          {6,10,13},
                          {16,19,29},
                          {19,20,28,29},
                          {4,8,16},
                          {14,23,24},
                          {4,15,16},
                          {15,16,25,29},
                          {2,4,15},
                          {0,1,3,6,9},
                          {12,22,24},
                          {22,24,27},
                          {11,12,22},
                          {3,9,11,12},
                          {11,21,22},
                          {21,22,26,27},
                          {3,7,11},
                          {7,11,21},
                          {5,7,18},
                          {7,18,21},
                          {0,3,5,7}};

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J46: Gyroelongated pentagonal bicupola" << endl;

   return p;
}

//FIXME: coordinates #830
BigObject gyroelongated_pentagonal_cupolarotunda()
{
   BigObject p = gyroelongated_pentagonal_rotunda();
   p = augment(p, sequence(20,10));

   IncidenceMatrix<> VIF{ {22,23,30,31},
                          {23,26,31},
                          {10,22,23},
                          {4,8,14,16,17},
                          {1,4,8},
                          {16,17,27},
                          {27,29,32},
                          {17,27,29},
                          {0,1,2,4,6},
                          {0,1,3},
                          {4,6,14},
                          {14,17,19},
                          {17,19,29},
                          {28,29,32,33},
                          {9,20,21},
                          {19,28,29},
                          {5,9,13},
                          {9,13,21},
                          {24,25,33,34},
                          {21,24,34},
                          {2,5,11,13,15},
                          {13,21,24},
                          {11,15,18},
                          {15,24,25},
                          {15,18,25},
                          {13,15,24},
                          {18,25,28},
                          {25,28,33},
                          {18,19,28},
                          {2,6,11},
                          {6,11,14,18,19},
                          {0,2,5},
                          {20,21,30,34},
                          {7,9,20},
                          {0,3,5,7,9},
                          {30,31,32,33,34},
                          {7,20,22},
                          {20,22,30},
                          {3,7,10},
                          {7,10,22},
                          {16,26,27},
                          {26,27,31,32},
                          {8,12,16},
                          {12,16,26},
                          {10,12,23},
                          {12,23,26},
                          {1,3,8,10,12} };

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J47: Gyroelongated pentagonal cupolarotunda" << endl;

   return p;
}

//FIXME: coordinates #830
BigObject gyroelongated_pentagonal_birotunda()
{
   BigObject p = gyroelongated_pentagonal_rotunda();
   p = rotunda(p, sequence(20,10));

   IncidenceMatrix<> VIF{  {22,23,30,31,35},
                          {23,26,31},
                          {10,22,23},
                          {31,35,36},
                          {7,10,22},
                          {3,7,10},
                          {7,20,22},
                          {20,22,30},
                          {0,1,2,4,6},
                          {0,1,3},
                          {35,36,37,38,39},
                          {30,35,39},
                          {4,6,14},
                          {14,17,19},
                          {17,19,29},
                          {28,29,32,33,37},
                          {9,20,21},
                          {19,28,29},
                          {33,37,38},
                          {5,9,13},
                          {9,13,21},
                          {2,5,11,13,15},
                          {24,25,33,34,38},
                          {25,28,33},
                          {18,25,28},
                          {13,15,24},
                          {15,18,25},
                          {15,24,25},
                          {11,15,18},
                          {13,21,24},
                          {21,24,34},
                          {18,19,28},
                          {2,6,11},
                          {34,38,39},
                          {6,11,14,18,19},
                          {0,2,5},
                          {20,21,30,34,39},
                          {7,9,20},
                          {32,36,37},
                          {0,3,5,7,9},
                          {27,29,32},
                          {17,27,29},
                          {16,17,27},
                          {1,4,8},
                          {4,8,14,16,17},
                          {16,26,27},
                          {26,27,31,32,36},
                          {8,12,16},
                          {12,16,26},
                          {10,12,23},
                          {12,23,26},
                          {1,3,8,10,12}};

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J48: Gyroelongated pentagonal birotunda" << endl;

   return p;
}

//FIXME: coordinates 830. this could be done exactly if QE was more powerful
BigObject augmented_triangular_prism()
{
  QE height(0,1,3);
  Matrix<QE> ledge(2,4);
  ledge.col(0).fill(1);
  ledge.col(1).fill(1);
  ledge.col(3).fill(-height);
  ledge[1][1]=-1;

  Vector<QE> tip(4);
  tip[0]=1;
  tip[1]=tip[2]=0;
  tip[3]=QE(0,1,2);

  Matrix<QE> V = ((create_square_vertices<QE>() | zero_vector<QE>(4)) / ledge / tip );

  IncidenceMatrix<> VIF{ {0,1,4,5},
                         {0,2,6},
                         {0,2,5},
                         {0,1,6},
                         {1,3,6},
                         {2,3,6},
                         {1,3,4},
                         {2,3,4,5} };

  BigObject p("Polytope<Float>");
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  centralize<double>(p);
  p.set_description() << "Johnson solid J49: augmented triangular prism" << endl;
  return p;
}

BigObject biaugmented_triangular_prism()
{
  QE height(0,1,3);
  Matrix<QE> ledge(2,4);
  ledge.col(0).fill(1);
  ledge.col(1).fill(1);
  ledge.col(3).fill(-height);
  ledge[1][1]=-1;

  Matrix<QE> V = ((create_square_vertices<QE>() | zero_vector<QE>(4)) / ledge );

  BigObject p("Polytope<Float>");
  p.take("VERTICES") << V;

  p = augment(p, Set<Int>{0,1,4,5});

  p = augment(p, sequence(0,4));

  IncidenceMatrix<> VIF{ {1,4,6},
                         {4,5,6},
                         {0,2,7},
                         {0,2,5},
                         {0,5,6},
                         {0,1,6},
                         {0,1,7},
                         {1,3,7},
                         {2,3,7},
                         {1,3,4},
                         {2,3,4,5} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J50: biaugmented triangular prism" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject triaugmented_triangular_prism()
{
  BigObject p = create_prism(3);

  p = augment(p, Set<Int>{1,2,4,5});
  p = augment(p, Set<Int>{0,2,3,5});
  p = augment(p, Set<Int>{0,1,3,4});

  IncidenceMatrix<> VIF{ {0,1,8},
                         {0,2,7},
                         {0,1,2},
                         {2,5,7},
                         {1,2,6},
                         {2,5,6},
                         {4,5,6},
                         {1,4,6},
                         {1,4,8},
                         {3,4,5},
                         {3,5,7},
                         {3,4,8},
                         {0,3,7},
                         {0,3,8} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J51: triaugmented triangular prism" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject augmented_pentagonal_prism()
{
  BigObject p = create_prism(5);

  p = augment(p, Set<Int>{2,3,7,8});

  IncidenceMatrix<> VIF{ {0,1,2,3,4},
                         {2,3,10},
                         {3,8,10},
                         {7,8,10},
                         {2,7,10},
                         {3,4,8,9},
                         {1,2,6,7},
                         {5,6,7,8,9},
                         {0,4,5,9},
                         {0,1,5,6} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J52: augmented pentagonal prism" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject biaugmented_pentagonal_prism()
{
  BigObject p = create_prism(5);

  p = augment(p, Set<Int>{2,3,7,8});
  p = augment(p, Set<Int>{0,4,5,9});

  IncidenceMatrix<> VIF{ {0,1,5,6},
                         {5,6,7,8,9},
                         {1,2,6,7},
                         {3,4,8,9},
                         {2,7,10},
                         {7,8,10},
                         {3,8,10},
                         {2,3,10},
                         {0,1,2,3,4},
                         {4,9,11},
                         {0,4,11},
                         {5,9,11},
                         {0,5,11} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J53: biaugmented pentagonal prism" << endl;

  return p;
}


//FIXME: coordinates #830
BigObject augmented_hexagonal_prism()
{
  BigObject p = create_prism(6);

  p = augment(p, Set<Int>{3,4,9,10});

  IncidenceMatrix<> VIF{ {0,1,2,3,4,5},
                         {3,4,12},
                         {3,9,12},
                         {9,10,12},
                         {4,10,12},
                         {2,3,8,9},
                         {4,5,10,11},
                         {1,2,7,8},
                         {6,7,8,9,10,11},
                         {0,5,6,11},
                         {0,1,6,7} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J54: augmented hexagonal prism" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject parabiaugmented_hexagonal_prism()
{
  BigObject p = augmented_hexagonal_prism();

  p = augment(p, Set<Int>{0,1,6,7});

  IncidenceMatrix<> VIF{ {0,5,6,11},
                         {6,7,8,9,10,11},
                         {1,2,7,8},
                         {4,5,10,11},
                         {2,3,8,9},
                         {4,10,12},
                         {9,10,12},
                         {3,9,12},
                         {3,4,12},
                         {0,1,2,3,4,5},
                         {1,7,13},
                         {0,1,13},
                         {6,7,13},
                         {0,6,13} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J55: parabiaugmented hexagonal prism" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject metabiaugmented_hexagonal_prism()
{
  BigObject p = augmented_hexagonal_prism();

  p = augment(p, Set<Int>{1,2,7,8});

  IncidenceMatrix<> VIF{ {0,1,2,3,4,5},
                         {1,2,13},
                         {2,3,8,9},
                         {4,10,12},
                         {9,10,12},
                         {3,9,12},
                         {3,4,12},
                         {2,8,13},
                         {4,5,10,11},
                         {7,8,13},
                         {1,7,13},
                         {6,7,8,9,10,11},
                         {0,5,6,11},
                         {0,1,6,7} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J56: metabiaugmented hexagonal prism" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject triaugmented_hexagonal_prism()
{
  BigObject p = metabiaugmented_hexagonal_prism();

  p = augment(p, Set<Int>{0,5,6,11});

  IncidenceMatrix<> VIF{ {0,1,6,7},
                         {6,7,8,9,10,11},
                         {1,7,13},
                         {7,8,13},
                         {4,5,10,11},
                         {2,8,13},
                         {3,4,12},
                         {3,9,12},
                         {9,10,12},
                         {4,10,12},
                         {2,3,8,9},
                         {1,2,13},
                         {0,1,2,3,4,5},
                         {5,11,14},
                         {0,5,14},
                         {6,11,14},
                         {0,6,14} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J57: triaugmented hexagonal prism" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject augmented_dodecahedron()
{
  BigObject p = call_function("dodecahedron");
  p = augment(p, Set<Int>{0,2,4,8,9});

  IncidenceMatrix<> VIF{ {8,9,13,16,18},
                         {2,5,8,12,13},
                         {0,1,2,3,5},
                         {12,13,15,18,19},
                         {3,5,10,12,15},
                         {1,3,6,10,11},
                         {10,11,15,17,19},
                         {6,7,11,14,17},
                         {14,16,17,18,19},
                         {0,1,4,6,7},
                         {4,7,9,14,16},
                         {0,4,20},
                         {0,2,20},
                         {4,9,20},
                         {2,8,20},
                         {8,9,20} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J58: augmented dodecahedron" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject parabiaugmented_dodecahedron()
{
  BigObject p = augmented_dodecahedron();

  p = augment(p, Set<Int>{10,11,15,17,19});

  IncidenceMatrix<> VIF{ {8,9,13,16,18},
                         {2,5,8,12,13},
                         {0,1,2,3,5},
                         {12,13,15,18,19},
                         {3,5,10,12,15},
                         {1,3,6,10,11},
                         {10,15,21},
                         {10,11,21},
                         {11,17,21},
                         {17,19,21},
                         {15,19,21},
                         {6,7,11,14,17},
                         {14,16,17,18,19},
                         {0,1,4,6,7},
                         {4,7,9,14,16},
                         {0,4,20},
                         {0,2,20},
                         {4,9,20},
                         {2,8,20},
                         {8,9,20} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J59: parabiaugmented dodecahedron" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject metabiaugmented_dodecahedron()
{
  BigObject p = augmented_dodecahedron();

  p = augment(p, Set<Int>{12,13,15,18,19});

  IncidenceMatrix<> VIF{ {8,9,13,16,18},
                         {2,5,8,12,13},
                         {0,1,2,3,5},
                         {13,18,21},
                         {12,13,21},
                         {3,5,10,12,15},
                         {12,15,21},
                         {15,19,21},
                         {10,11,15,17,19},
                         {1,3,6,10,11},
                         {6,7,11,14,17},
                         {18,19,21},
                         {14,16,17,18,19},
                         {0,1,4,6,7},
                         {4,7,9,14,16},
                         {0,4,20},
                         {0,2,20},
                         {4,9,20},
                         {2,8,20},
                         {8,9,20} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J60: metabiaugmented dodecahedron" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject triaugmented_dodecahedron()
{
  BigObject p = metabiaugmented_dodecahedron();

  p = augment(p, Set<Int>{1,3,6,10,11});

  IncidenceMatrix<> VIF{ {8,9,13,16,18},
                         {2,5,8,12,13},
                         {0,1,2,3,5},
                         {13,18,21},
                         {12,13,21},
                         {3,5,10,12,15},
                         {12,15,21},
                         {15,19,21},
                         {10,11,15,17,19},
                         {6,11,22},
                         {10,11,22},
                         {3,10,22},
                         {1,6,22},
                         {1,3,22},
                         {6,7,11,14,17},
                         {18,19,21},
                         {14,16,17,18,19},
                         {0,1,4,6,7},
                         {4,7,9,14,16},
                         {0,4,20},
                         {0,2,20},
                         {4,9,20},
                         {2,8,20},
                         {8,9,20} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J61: triaugmented dodecahedron" << endl;

  return p;
}

BigObject metabidiminished_icosahedron()
{
  BigObject ico = call_function("icosahedron");

  Matrix<QE> V = ico.give("VERTICES");

  V = ((V.minor(sequence(1,5),All)) / (V.minor(sequence(7,5),All)));

  BigObject p=build_from_vertices(V);

  p.set_description() << "Johnson solid J62: metabidiminished icosahedron" << endl;
  return p;
}

BigObject tridiminished_icosahedron()
{
  BigObject dimico = metabidiminished_icosahedron();

  Matrix<QE> V = dimico.give("VERTICES");
  V = ((V.minor(sequence(0,7),All)) / (V.minor(sequence(8,2),All)));

  BigObject p=build_from_vertices(V);

  p.set_description() << "Johnson solid J63: tridiminished icosahedron" << endl;
  return p;
}

//FIXME: coordinates #830
BigObject augmented_tridiminished_icosahedron()
{
  BigObject p = tridiminished_icosahedron();
  p = augment(p, Set<Int>{0,2,5});

  IncidenceMatrix<> VIF{ {3,6,7},
                         {6,7,8},
                         {0,5,9},
                         {0,2,9},
                         {2,5,9},
                         {2,4,5,7,8},
                         {3,4,7},
                         {1,3,6},
                         {0,1,2,3,4},
                         {0,1,5,6,8} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J64: augmented_tridiminished icosahedron" << endl;

  return p;
}

BigObject augmented_truncated_tetrahedron()
{
  Rational r(1,3);
  Rational s(1,9);
  Matrix<Rational> V(15,4);
  V.col(0).fill(1);
  V(0,1)=V(1,2)=V(2,3)=V(3,1)=V(6,2)=V(10,3)=1;
  V(4,2)=V(5,3)=V(7,3)=V(8,1)=V(9,1)=V(11,2)=-1;
  V(0,2)=V(0,3)=V(1,1)=V(1,3)=V(2,1)=V(2,2)=V(4,1)=V(5,1)=V(7,2)=V(8,2)=V(9,3)=V(11,3)=r;
  V(3,2)=V(3,3)=V(4,3)=V(5,2)=V(6,1)=V(6,3)=V(7,1)=V(8,3)=V(9,2)=V(10,1)=V(10,2)=V(11,1)=-r;
  V(12,1)=-11*s; V(12,2)=V(12,3)=5*s;
  V(13,1)=-5*s; V(13,2)=11*s; V(13,3)=5*s;
  V(14,1)=V(13,1); V(14,2)=V(13,3); V(14,3)=V(13,2);

  BigObject p=build_from_vertices(V);
  p.set_description() << "Johnson solid J65: Augmented truncated tetrahedron" << endl;

  return p;
}

BigObject augmented_truncated_cube()
{
   Matrix<QE> W = square_cupola_impl(false).give("VERTICES");
   W.col(3) += same_element_vector(QE(2,2,2),12);
   Matrix<QE> V = truncated_cube_vertices() / W.minor(sequence(8,4), All);

   BigObject p=build_from_vertices(V);
   p.set_description() << "Johnson solid J66: Augmented truncated cube" << endl;

   return p;
}

BigObject biaugmented_truncated_cube()
{
   Matrix<QE> W = square_cupola_impl(false).give("VERTICES");
   W = W.minor(sequence(8,4),sequence(1,3));
   Matrix<QE> V = truncated_cube_vertices() /
     (ones_vector<QE>() | ((W + repeat_row(unit_vector(3, 2, QE(2,2,2)), 4)) / -W));

   BigObject p=build_from_vertices(V);
   p.set_description() << "Johnson solid J67: Biaugmented truncated cube" << endl;

   return p;
}

BigObject augmented_truncated_dodecahedron()
{
   BigObject p = call_function("truncated_dodecahedron");

   p = augment(p, Set<Int>{0,1,5,7,9,11,13,16,18,21});

   p = rotate_facet(p, sequence(60,5), M_PI/5);

   IncidenceMatrix<> VIF{ {22,27,38},
                          {13,16,19,22,32,35,38,41,43,46},
                          {7,13,19},
                          {41,46,52},
                          {5,7,61},
                          {0,5,60,61},
                          {43,46,48,50,52,54,56,57,58,59},
                          {13,16,62},
                          {7,13,61,62},
                          {0,1,60},
                          {1,9,60,64},
                          {10,15,26},
                          {29,36,42},
                          {12,15,20,23,26,29,37,40,42,45},
                          {1,4,9},
                          {55,57,59},
                          {4,6,9,11,20,23,25,28,30,33},
                          {6,12,20},
                          {40,45,53},
                          {30,33,37,40,44,47,53,55,56,57},
                          {23,30,37},
                          {28,33,44},
                          {47,50,56},
                          {11,18,25},
                          {9,11,64},
                          {11,18,63,64},
                          {18,21,25,28,32,35,44,47,48,50},
                          {18,21,63},
                          {16,21,62,63},
                          {35,43,48},
                          {16,21,32},
                          {60,61,62,63,64},
                          {51,54,58},
                          {0,2,5},
                          {36,39,42,45,49,51,53,55,58,59},
                          {0,1,2,3,4,6,8,10,12,15},
                          {34,39,49},
                          {3,8,14},
                          {8,10,14,17,26,29,31,34,36,39},
                          {17,24,31},
                          {24,27,31,34,38,41,49,51,52,54},
                          {2,3,5,7,14,17,19,22,24,27}};

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);
   p.set_description() << "Johnson solid J68: Augmented truncated dodecahedron" << endl;

   return p;
}


BigObject parabiaugmented_truncated_dodecahedron()
{
   BigObject p = augmented_truncated_dodecahedron();
   p = augment(p, Set<Int>{36,39,42,45,49,51,53,55,58,59});

   p = rotate_facet(p, sequence(65,5), M_PI/5);

   IncidenceMatrix<> VIF{ {22,27,38},
                          {13,16,19,22,32,35,38,41,43,46},
                          {7,13,19},
                          {41,46,52},
                          {5,7,61},
                          {0,5,60,61},
                          {43,46,48,50,52,54,56,57,58,59},
                          {13,16,62},
                          {7,13,61,62},
                          {65,66,67,68,69},
                          {16,21,32},
                          {35,43,48},
                          {12,15,20,23,26,29,37,40,42,45},
                          {42,45,66},
                          {36,42,65,66},
                          {9,11,64},
                          {11,18,63,64},
                          {40,45,53},
                          {47,50,56},
                          {28,33,44},
                          {23,30,37},
                          {30,33,37,40,44,47,53,55,56,57},
                          {6,12,20},
                          {4,6,9,11,20,23,25,28,30,33},
                          {11,18,25},
                          {53,55,67},
                          {45,53,66,67},
                          {55,57,59},
                          {1,4,9},
                          {18,21,25,28,32,35,44,47,48,50},
                          {18,21,63},
                          {16,21,62,63},
                          {58,59,68},
                          {55,59,67,68},
                          {29,36,42},
                          {10,15,26},
                          {0,1,60},
                          {1,9,60,64},
                          {60,61,62,63,64},
                          {51,54,58},
                          {36,39,65},
                          {39,49,65,69},
                          {0,2,5},
                          {49,51,69},
                          {51,58,68,69},
                          {0,1,2,3,4,6,8,10,12,15},
                          {34,39,49},
                          {3,8,14},
                          {8,10,14,17,26,29,31,34,36,39},
                          {17,24,31},
                          {24,27,31,34,38,41,49,51,52,54},
                          {2,3,5,7,14,17,19,22,24,27}};

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);

   p.set_description() << "Johnson solid J69: Parabiaugmented truncated dodecahedron" << endl;
   return p;
}

//FIXME: coordinates #830
BigObject metabiaugmented_truncated_dodecahedron()
{
   BigObject p = augmented_truncated_dodecahedron();
   p = augment(p, Set<Int>{43,46,48,50,52,54,56,57,58,59});

   p = rotate_facet(p, sequence(65,5), M_PI/5);

   IncidenceMatrix<> VIF{ {22,27,38},
                          {13,16,19,22,32,35,38,41,43,46},
                          {7,13,19},
                          {41,46,52},
                          {5,7,61},
                          {0,5,60,61},
                          {52,54,68},
                          {46,52,68,69},
                          {13,16,62},
                          {7,13,61,62},
                          {43,46,69},
                          {43,48,65,69},
                          {65,66,67,68,69},
                          {16,21,32},
                          {35,43,48},
                          {12,15,20,23,26,29,37,40,42,45},
                          {48,50,65},
                          {50,56,65,66},
                          {9,11,64},
                          {11,18,63,64},
                          {56,57,66},
                          {57,59,66,67},
                          {47,50,56},
                          {28,33,44},
                          {23,30,37},
                          {30,33,37,40,44,47,53,55,56,57},
                          {40,45,53},
                          {6,12,20},
                          {4,6,9,11,20,23,25,28,30,33},
                          {11,18,25},
                          {55,57,59},
                          {1,4,9},
                          {18,21,25,28,32,35,44,47,48,50},
                          {18,21,63},
                          {16,21,62,63},
                          {58,59,67},
                          {54,58,67,68},
                          {29,36,42},
                          {10,15,26},
                          {0,1,60},
                          {1,9,60,64},
                          {60,61,62,63,64},
                          {51,54,58},
                          {0,2,5},
                          {36,39,42,45,49,51,53,55,58,59},
                          {0,1,2,3,4,6,8,10,12,15},
                          {34,39,49},
                          {3,8,14},
                          {8,10,14,17,26,29,31,34,36,39},
                          {17,24,31},
                          {24,27,31,34,38,41,49,51,52,54},
                          {2,3,5,7,14,17,19,22,24,27}};

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);

   p.set_description() << "Johnson solid J70: Metabiaugmented truncated dodecahedron" << endl;
   return p;
}

//FIXME: coordinates #830
BigObject triaugmented_truncated_dodecahedron()
{
   BigObject p = metabiaugmented_truncated_dodecahedron();
   p = augment(p, Set<Int>{12,15,20,23,26,29,37,40,42,45});

   p = rotate_facet(p, sequence(70,5), M_PI/5);

   IncidenceMatrix<> VIF{ {22,27,38},
                          {13,16,19,22,32,35,38,41,43,46},
                          {7,13,19},
                          {41,46,52},
                          {5,7,61},
                          {0,5,60,61},
                          {52,54,68},
                          {46,52,68,69},
                          {13,16,62},
                          {7,13,61,62},
                          {43,46,69},
                          {43,48,65,69},
                          {65,66,67,68,69},
                          {16,21,32},
                          {35,43,48},
                          {26,29,74},
                          {29,42,73,74},
                          {48,50,65},
                          {50,56,65,66},
                          {9,11,64},
                          {11,18,63,64},
                          {56,57,66},
                          {57,59,66,67},
                          {47,50,56},
                          {37,40,72},
                          {23,37,71,72},
                          {28,33,44},
                          {23,30,37},
                          {20,23,71},
                          {12,20,70,71},
                          {30,33,37,40,44,47,53,55,56,57},
                          {40,45,53},
                          {6,12,20},
                          {4,6,9,11,20,23,25,28,30,33},
                          {70,71,72,73,74},
                          {11,18,25},
                          {55,57,59},
                          {42,45,73},
                          {40,45,72,73},
                          {12,15,70},
                          {15,26,70,74},
                          {1,4,9},
                          {18,21,25,28,32,35,44,47,48,50},
                          {18,21,63},
                          {16,21,62,63},
                          {58,59,67},
                          {54,58,67,68},
                          {29,36,42},
                          {10,15,26},
                          {0,1,60},
                          {1,9,60,64},
                          {60,61,62,63,64},
                          {51,54,58},
                          {0,2,5},
                          {36,39,42,45,49,51,53,55,58,59},
                          {0,1,2,3,4,6,8,10,12,15},
                          {34,39,49},
                          {3,8,14},
                          {8,10,14,17,26,29,31,34,36,39},
                          {17,24,31},
                          {24,27,31,34,38,41,49,51,52,54},
                          {2,3,5,7,14,17,19,22,24,27}};

   p.take("VERTICES_IN_FACETS") << VIF;

   centralize<double>(p);

   p.set_description() << "Johnson solid J71: Triaugmented truncated dodecahedron" << endl;
   return p;
}

//FIXME: coordinates #830
BigObject gyrate_rhombicosidodecahedron()
{
  BigObject p = call_function("rhombicosidodecahedron");
  p = rotate_facet(p, Set<Int>{5,8,12,16,21}, M_PI/5);

  IncidenceMatrix<> VIF{ {27,32,37,41,45},
                         {18,27,28,37},
                         {3,11,56,58},
                         {11,18,58},
                         {11,14,18,23,28},
                         {37,43,45,50},
                         {28,37,43},
                         {0,2,55,56},
                         {0,3,56},
                         {3,8,11,14},
                         {23,28,39,43},
                         {45,50,53},
                         {2,4,6,9,12},
                         {44,48,51,53,54},
                         {20,26,30,36},
                         {12,20,26},
                         {30,36,44},
                         {9,12,21,26},
                         {36,40,44,51},
                         {4,7,9,13},
                         {21,26,31,36,40},
                         {46,49,51,54},
                         {9,13,21},
                         {40,46,51},
                         {10,15,17,24},
                         {24,33,34,42},
                         {33,38,42,46,49},
                         {17,22,33,38},
                         {22,31,38},
                         {17,24,33},
                         {31,38,40,46},
                         {13,21,22,31},
                         {7,10,13,17,22},
                         {34,42,47},
                         {5,10,15},
                         {49,52,54},
                         {42,47,49,52},
                         {15,19,24,29,34},
                         {1,4,7},
                         {1,5,7,10},
                         {29,34,39,47},
                         {5,8,15,19},
                         {50,52,53,54},
                         {23,29,39},
                         {8,14,19},
                         {14,19,23,29},
                         {0,1,2,4},
                         {39,43,47,50,52},
                         {0,1,3,5,8},
                         {30,35,44,48},
                         {6,12,16,20},
                         {2,6,55},
                         {35,41,48},
                         {41,45,48,53},
                         {16,20,25,30,35},
                         {16,25,57},
                         {6,16,55,57},
                         {25,32,35,41},
                         {27,32,59},
                         {25,32,57,59},
                         {55,56,57,58,59},
                         {18,27,58,59} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);

  p.set_description() << "Johnson solid J72: Gyrate rhombicosidodecahedron" << endl;
  return p;
}

//FIXME: coordinates #830
BigObject parabigyrate_rhombicosidodecahedron()
{
   BigObject p = gyrate_rhombicosidodecahedron();
   p = rotate_facet(p, Set<Int>{33,38,42,46,49}, M_PI/5);

   IncidenceMatrix<> VIF{ {27,32,36,39,42},
                          {18,27,28,36},
                          {3,11,51,53},
                          {11,18,53},
                          {11,14,18,23,28},
                          {36,40,42,45},
                          {28,36,40},
                          {0,2,50,51},
                          {0,3,51},
                          {3,8,11,14},
                          {23,28,37,40},
                          {42,45,48},
                          {2,4,6,9,12},
                          {41,44,46,48,49},
                          {20,26,30,35},
                          {12,20,26},
                          {30,35,41},
                          {9,12,21,26},
                          {35,38,41,46},
                          {4,7,9,13},
                          {21,26,31,35,38},
                          {46,49,59},
                          {9,13,21},
                          {38,46,58,59},
                          {10,15,17,24},
                          {24,33,55},
                          {55,56,57,58,59},
                          {17,24,55,56},
                          {17,22,56},
                          {22,31,56,58},
                          {31,38,58},
                          {13,21,22,31},
                          {7,10,13,17,22},
                          {33,43,55,57},
                          {5,10,15},
                          {43,47,57},
                          {47,49,57,59},
                          {15,19,24,29,33},
                          {1,4,7},
                          {1,5,7,10},
                          {29,33,37,43},
                          {5,8,15,19},
                          {45,47,48,49},
                          {23,29,37},
                          {8,14,19},
                          {14,19,23,29},
                          {0,1,2,4},
                          {37,40,43,45,47},
                          {0,1,3,5,8},
                          {30,34,41,44},
                          {6,12,16,20},
                          {2,6,50},
                          {34,39,44},
                          {39,42,44,48},
                          {16,20,25,30,34},
                          {16,25,52},
                          {6,16,50,52},
                          {25,32,34,39},
                          {27,32,54},
                          {25,32,52,54},
                          {50,51,52,53,54},
                          {18,27,53,54} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);

  p.set_description() << "Johnson solid J73: Parabigyrate rhombicosidodecahedron" << endl;
  return p;
}

//FIXME: coordinates #830
BigObject metabigyrate_rhombicosidodecahedron()
{
   BigObject p = gyrate_rhombicosidodecahedron();
   p = rotate_facet(p, Set<Int>{44,48,51,53,54}, M_PI/5);

   IncidenceMatrix<> VIF{ {27,32,37,41,44},
                          {18,27,28,37},
                          {3,11,51,53},
                          {11,18,53},
                          {11,14,18,23,28},
                          {37,43,44,48},
                          {28,37,43},
                          {0,2,50,51},
                          {0,3,51},
                          {3,8,11,14},
                          {23,28,39,43},
                          {44,48,58,59},
                          {2,4,6,9,12},
                          {55,56,57,58,59},
                          {20,26,30,36},
                          {12,20,26},
                          {30,36,55,56},
                          {48,49,59},
                          {9,12,21,26},
                          {47,49,57,59},
                          {4,7,9,13},
                          {21,26,31,36,40},
                          {36,40,55},
                          {42,46,47,49},
                          {9,13,21},
                          {45,47,57},
                          {10,15,17,24},
                          {24,33,34,42},
                          {33,38,42,45,47},
                          {17,22,33,38},
                          {22,31,38},
                          {17,24,33},
                          {31,38,40,45},
                          {13,21,22,31},
                          {7,10,13,17,22},
                          {34,42,46},
                          {5,10,15},
                          {40,45,55,57},
                          {15,19,24,29,34},
                          {1,4,7},
                          {1,5,7,10},
                          {29,34,39,46},
                          {5,8,15,19},
                          {23,29,39},
                          {8,14,19},
                          {14,19,23,29},
                          {0,1,2,4},
                          {39,43,46,48,49},
                          {0,1,3,5,8},
                          {30,35,56},
                          {6,12,16,20},
                          {2,6,50},
                          {41,44,58},
                          {35,41,56,58},
                          {16,20,25,30,35},
                          {16,25,52},
                          {6,16,50,52},
                          {25,32,35,41},
                          {27,32,54},
                          {25,32,52,54},
                          {50,51,52,53,54},
                          {18,27,53,54} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);

  p.set_description() << "Johnson solid J74: Metabigyrate rhombicosidodecahedron" << endl;
  return p;
}

//FIXME: coordinates #830
BigObject trigyrate_rhombicosidodecahedron()
{
   BigObject p = metabigyrate_rhombicosidodecahedron();
   p = rotate_facet(p, Set<Int>{15,19,24,29,34}, M_PI/5);

   IncidenceMatrix<> VIF{ {24,28,32,36,39},
                          {17,24,25,32},
                          {3,11,46,48},
                          {11,17,48},
                          {11,14,17,21,25},
                          {32,38,39,43},
                          {25,32,38},
                          {0,2,45,46},
                          {0,3,46},
                          {3,8,11,14},
                          {21,25,34,38},
                          {39,43,53,54},
                          {2,4,6,9,12},
                          {50,51,52,53,54},
                          {18,23,26,31},
                          {12,18,23},
                          {26,31,50,51},
                          {43,44,54},
                          {9,12,19,23},
                          {42,44,52,54},
                          {1,5,7,10},
                          {1,4,7},
                          {19,23,27,31,35},
                          {35,40,50,52},
                          {5,10,55,57},
                          {37,41,58,59},
                          {7,10,13,16,20},
                          {27,33,35,40},
                          {10,16,57},
                          {29,37,59},
                          {16,29,57,59},
                          {16,20,29,33},
                          {20,27,33},
                          {29,33,37,40,42},
                          {13,19,20,27},
                          {40,42,52},
                          {9,13,19},
                          {37,41,42,44},
                          {31,35,50},
                          {4,7,9,13},
                          {55,56,57,58,59},
                          {34,41,58},
                          {5,8,55},
                          {21,34,56,58},
                          {14,21,56},
                          {8,14,55,56},
                          {0,1,2,4},
                          {34,38,41,43,44},
                          {0,1,3,5,8},
                          {26,30,51},
                          {6,12,15,18},
                          {2,6,45},
                          {36,39,53},
                          {30,36,51,53},
                          {15,18,22,26,30},
                          {15,22,47},
                          {6,15,45,47},
                          {22,28,30,36},
                          {24,28,49},
                          {22,28,47,49},
                          {45,46,47,48,49},
                          {17,24,48,49} };

  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);

  p.set_description() << "Johnson solid J75: trigyrate rhombicosidodecahedron" << endl;
  return p;
}

BigObject diminished_rhombicosidodecahedron()
{
   BigObject p = call_function("rhombicosidodecahedron");
   p = diminish<QE>(p,  Set<Int>{5,8,12,16,21});
   centralize<QE>(p);

   p.set_description() << "Johnson solid J76: diminished rhombicosidodecahedron" << endl;
   return p;
}

//FIXME: coordinates #830
BigObject paragyrate_diminished_rhombicosidodecahedron()
{
   BigObject p = gyrate_rhombicosidodecahedron();

   p = diminish<double>(p, Set<Int>{33,38,42,46,49});

   IncidenceMatrix<> VIF{ {27,32,36,39,42},
                          {18,27,28,36},
                          {3,11,51,53},
                          {11,18,53},
                          {11,14,18,23,28},
                          {36,40,42,45},
                          {28,36,40},
                          {0,2,50,51},
                          {0,3,51},
                          {3,8,11,14},
                          {23,28,37,40},
                          {42,45,48},
                          {2,4,6,9,12},
                          {41,44,46,48,49},
                          {20,26,30,35},
                          {12,20,26},
                          {30,35,41},
                          {9,12,21,26},
                          {35,38,41,46},
                          {4,7,9,13},
                          {21,26,31,35,38},
                          {9,13,21},
                          {10,15,17,24},
                          {13,21,22,31},
                          {7,10,13,17,22},
                          {5,10,15},
                          {17,22,24,31,33,38,43,46,47,49},
                          {15,19,24,29,33},
                          {1,4,7},
                          {1,5,7,10},
                          {29,33,37,43},
                          {5,8,15,19},
                          {45,47,48,49},
                          {23,29,37},
                          {8,14,19},
                          {14,19,23,29},
                          {0,1,2,4},
                          {37,40,43,45,47},
                          {0,1,3,5,8},
                          {30,34,41,44},
                          {6,12,16,20},
                          {2,6,50},
                          {34,39,44},
                          {39,42,44,48},
                          {16,20,25,30,34},
                          {16,25,52},
                          {6,16,50,52},
                          {25,32,34,39},
                          {27,32,54},
                          {25,32,52,54},
                          {50,51,52,53,54},
                          {18,27,53,54} };

   p.take("VERTICES_IN_FACETS") << VIF;
   centralize<double>(p);
   p.set_description() << "Johnson solid J77: paragyrate diminished rhombicosidodecahedron" << endl;
   return p;
}

//FIXME: coordinates #830
BigObject metagyrate_diminished_rhombicosidodecahedron()
{
   BigObject p = gyrate_rhombicosidodecahedron();

   p = diminish<double>(p, Set<Int>{44,48,51,53,54});

   IncidenceMatrix<> VIF{ {27,32,37,41,44},
                          {18,27,28,37},
                          {3,11,51,53},
                          {11,18,53},
                          {11,14,18,23,28},
                          {37,43,44,48},
                          {28,37,43},
                          {0,2,50,51},
                          {0,3,51},
                          {3,8,11,14},
                          {23,28,39,43},
                          {2,4,6,9,12},
                          {0,1,2,4},
                          {14,19,23,29},
                          {8,14,19},
                          {23,29,39},
                          {9,12,21,26},
                          {1,5,7,10},
                          {1,4,7},
                          {15,19,24,29,34},
                          {42,46,47,49},
                          {9,13,21},
                          {7,10,13,17,22},
                          {13,21,22,31},
                          {31,38,40,45},
                          {17,24,33},
                          {22,31,38},
                          {17,22,33,38},
                          {33,38,42,45,47},
                          {24,33,34,42},
                          {10,15,17,24},
                          {34,42,46},
                          {5,10,15},
                          {21,26,31,36,40},
                          {4,7,9,13},
                          {29,34,39,46},
                          {5,8,15,19},
                          {12,20,26},
                          {20,26,30,36},
                          {39,43,46,48,49},
                          {0,1,3,5,8},
                          {6,12,16,20},
                          {2,6,50},
                          {30,35,36,40,41,44,45,47,48,49},
                          {16,20,25,30,35},
                          {16,25,52},
                          {6,16,50,52},
                          {25,32,35,41},
                          {27,32,54},
                          {25,32,52,54},
                          {50,51,52,53,54},
                          {18,27,53,54} };

   p.take("VERTICES_IN_FACETS") << VIF;
   centralize<double>(p);
   p.set_description() << "Johnson solid J78: metagyrate diminished rhombicosidodecahedron" << endl;
   return p;
}

//FIXME: coordinates #830
BigObject bigyrate_diminished_rhombicosidodecahedron()
{
   BigObject p = metabigyrate_rhombicosidodecahedron();

   p = diminish<double>(p, Set<Int>{15,19,24,29,34});
   IncidenceMatrix<> VIF{ {24,28,32,36,39},
                          {17,24,25,32},
                          {3,11,46,48},
                          {11,17,48},
                          {11,14,17,21,25},
                          {32,38,39,43},
                          {25,32,38},
                          {0,2,45,46},
                          {0,3,46},
                          {3,8,11,14},
                          {21,25,34,38},
                          {39,43,53,54},
                          {2,4,6,9,12},
                          {50,51,52,53,54},
                          {18,23,26,31},
                          {12,18,23},
                          {26,31,50,51},
                          {43,44,54},
                          {9,12,19,23},
                          {42,44,52,54},
                          {4,7,9,13},
                          {31,35,50},
                          {37,41,42,44},
                          {9,13,19},
                          {40,42,52},
                          {13,19,20,27},
                          {29,33,37,40,42},
                          {20,27,33},
                          {16,20,29,33},
                          {27,33,35,40},
                          {7,10,13,16,20},
                          {35,40,50,52},
                          {19,23,27,31,35},
                          {1,4,7},
                          {1,5,7,10},
                          {5,8,10,14,16,21,29,34,37,41},
                          {0,1,2,4},
                          {34,38,41,43,44},
                          {0,1,3,5,8},
                          {26,30,51},
                          {6,12,15,18},
                          {2,6,45},
                          {36,39,53},
                          {30,36,51,53},
                          {15,18,22,26,30},
                          {15,22,47},
                          {6,15,45,47},
                          {22,28,30,36},
                          {24,28,49},
                          {22,28,47,49},
                          {45,46,47,48,49},
                          {17,24,48,49} };

   p.take("VERTICES_IN_FACETS") << VIF;
   centralize<double>(p);
   p.set_description() << "Johnson solid J79: bigyrate diminished rhombicosidodecahedron" << endl;
   return p;
}

BigObject parabidiminished_rhombicosidodecahedron()
{
   BigObject p = diminished_rhombicosidodecahedron();
   p = diminish<QE>(p, Set<Int>{33,38,42,46,49});
   centralize<QE>(p);

   p.set_description() << "Johnson solid J80: parabidiminished rhombicosidodecahedron" << endl;
   return p;
}

BigObject metabidiminished_rhombicosidodecahedron()
{
   BigObject p = diminished_rhombicosidodecahedron();
   p = diminish<QE>(p, Set<Int>{7,10,13,17,22});
   centralize<QE>(p);

   p.set_description() << "Johnson solid J81: metabidiminished rhombicosidodecahedron" << endl;
   return p;
}

//FIXME: coordinates #830
BigObject gyrate_bidiminished_rhombicosidodecahedron()
{
   BigObject p = metabidiminished_rhombicosidodecahedron();
   p = rotate_facet(p, Set<Int>{34,38,42,45,47}, M_PI/5);

   IncidenceMatrix<> VIF{ {20,27,30,35},
                          {13,16,20,25,30},
                          {35,38,40,43},
                          {30,35,40},
                          {6,10,13,16},
                          {25,30,37,40},
                          {0,1,3,5,7},
                          {37,40,42,43,44},
                          {0,1,2,4},
                          {11,15,18,24},
                          {7,11,15},
                          {18,24,45,47},
                          {5,7,12,15},
                          {31,34,37,42},
                          {1,4,5,8,12,17,19,26,28,33},
                          {12,15,19,24,29},
                          {24,29,47},
                          {29,36,47,49},
                          {34,39,42},
                          {19,28,29,36},
                          {28,33,36,39,41},
                          {26,33,34,39},
                          {36,41,49},
                          {39,41,42,44},
                          {17,21,26,31,34},
                          {41,44,48,49},
                          {8,10,17,21},
                          {43,44,48},
                          {25,31,37},
                          {10,16,21},
                          {16,21,25,31},
                          {45,46,47,48,49},
                          {2,4,6,8,10},
                          {38,43,46,48},
                          {18,23,45},
                          {3,7,9,11},
                          {32,38,46},
                          {23,32,45,46},
                          {9,11,14,18,23},
                          {14,22,23,32},
                          {22,27,32,35,38},
                          {0,2,3,6,9,13,14,20,22,27} };

   p.take("VERTICES_IN_FACETS") << VIF;
   centralize<double>(p);
   p.set_description() << "Johnson solid J82: gyrate parabidiminished rhombicosidodecahedron" << endl;
   return p;
}

BigObject tridiminished_rhombicosidodecahedron()
{
   BigObject p = metabidiminished_rhombicosidodecahedron();
   p = diminish<QE>(p, Set<Int>{39,43,46,48,49});
   centralize<QE>(p);

   p.set_description() << "Johnson solid J83: tridiminished rhombicosidodecahedron" << endl;
   return p;
}

//FIXME: coordinates #830
BigObject snub_disphenoid()
{
  //coordinates calculated according to https://de.wikipedia.org/wiki/Trigondodekaeder
  //double r = -sqrt(28.0/3)*cos(acos(-sqrt(27.0/343)/3))+1;
  const double r = 1.289169;
  const double p1 = (sqrt(3.0+2.0*r-pow(r,2))+sqrt(3.0-pow(r,2)))/2;
  const double p2 = (sqrt(3.0+2.0*r-pow(r,2))-sqrt(3.0-pow(r,2)))/2;
  Matrix<double> V(8,4);
  V.col(0).fill(1);
  V(0,2)=1; V(0,3)=p1;
  V(1,2)=-1; V(1,3)=p1;
  V(2,1)=r; V(2,3)=p2;
  V(3,1)=-r; V(3,3)=p2;
  V(4,2)=r; V(4,3)=-p2;
  V(5,2)=-r; V(5,3)=-p2;
  V(6,1)=1; V(6,3)=-p1;
  V(7,1)=-1; V(7,3)=-p1;

  IncidenceMatrix<> VIF{ {4,6,7},
                         {5,6,7},
                         {0,1,3},
                         {1,3,5},
                         {3,5,7},
                         {3,4,7},
                         {0,3,4},
                         {0,2,4},
                         {1,2,5},
                         {0,1,2},
                         {2,5,6},
                         {2,4,6} };

  BigObject p("Polytope<Float>");
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  centralize<double>(p);
  p.set_description() << "Johnson solid J84: snub disphenoid" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject snub_square_antiprism()
{
  //coordinates from:  Weisstein, Eric W. "Snub Square Antiprism." From MathWorld--A Wolfram Web Resource. http://mathworld.wolfram.com/SnubSquareAntiprism.html
  Matrix<double> V(16,4);
  V.col(0).fill(1);
  V(0,1)=-1.09317; V(0,2)=0.431565; V(0,3)=-0.353606;
  V(1,1)=-1.0057; V(1,2)=-0.562189; V(1,3)=-0.422883;
  V(2,1)=-0.859633; V(2,2)=-0.107437; V(2,3)=0.455675;
  V(3,1)=-0.719877; V(3,2)=0.882748; V(3,3)=0.457;
  V(4,1)=-0.363678; V(4,2)=0.0313033; V(4,3)=-0.908245;
  V(5,1)=-0.309622; V(5,2)=-0.905009; V(5,3)=0.207952;
  V(6,1)=-0.197089; V(6,2)=0.873473; V(6,3)=-0.395412;
  V(7,1)=-0.149505; V(7,2)=-0.936855; V(7,3)=-0.778632;
  V(8,1)=-0.0740301; V(8,2)=0.286; V(8,3)=0.933206;
  V(9,1)=0.254708; V(9,2)=1.1066; V(9,3)=0.46571;
  V(10,1)=0.475981; V(10,2)=-0.511572; V(10,3)=0.685483;
  V(11,1)=0.580742; V(11,2)=-0.254465; V(11,3)=-0.745746;
  V(12,1)=0.614674; V(12,2)=-1.05249; V(12,3)=-0.144078;
  V(13,1)=0.747332; V(13,2)=0.587705; V(13,3)=-0.232913;
  V(14,1)=0.900495; V(14,2)=0.392448; V(14,3)=0.735804;
  V(15,1)=1.19838; V(15,2)=-0.261824; V(15,3)=0.0406841;

  IncidenceMatrix<> VIF{ {9,13,14},
                         {8,10,14},
                         {8,9,14},
                         {7,11,12},
                         {5,10,12},
                         {5,7,12},
                         {2,5,8,10},
                         {2,3,8},
                         {0,4,6},
                         {0,3,6},
                         {0,1,4},
                         {0,2,3},
                         {0,1,2},
                         {1,2,5},
                         {1,4,7},
                         {1,5,7},
                         {3,6,9},
                         {3,8,9},
                         {4,7,11},
                         {6,9,13},
                         {4,6,11,13},
                         {11,12,15},
                         {11,13,15},
                         {10,12,15},
                         {10,14,15},
                         {13,14,15} };

  BigObject p("Polytope<Float>");
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  centralize<double>(p);
  p.set_description() << "Johnson solid J85: snub square antiprism" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject sphenocorona()
{
  //coordinates from:  Weisstein, Eric W. "Sphenocorona." From MathWorld--A Wolfram Web Resource. http://mathworld.wolfram.com/Sphenocorona.html

  double a = (-6-sqrt(6)-2*sqrt(213-57*sqrt(6)))/30;
  double b = sqrt(1/6+6*sqrt(6)-sqrt(538+18*sqrt(6))/3)/5;
  double c = sqrt(1/6+6*sqrt(6)+sqrt(538+18*sqrt(6))/3)/5;
  double d = (9-sqrt(6)+2*sqrt(213-57*sqrt(6)))/30;
  double e = sqrt((1+sqrt(6))/2);
  double f = 0.5;

  Matrix<double> V(10,3);
  V(0,1) = V(3,0) = V(4,1) = V(7,1) = -f;
  V(1,1) = V(2,1) = V(5,1) = V(6,0) = f;
  V(2,0) = V(4,0) = a;
  V(2,2) = V(4,2) = V(5,2) = V(7,2) = b;
  V(3,2) = V(6,2) = e;
  V(5,0) = V(7,0) = -a;
  V(8,1) = d;
  V(8,2) = V(9,2) = c;
  V(9,1) = -d;

  // {0, -f, 0},
  // {0, f, 0},
  // {a, f, b},
  // {-f, 0, e},
  // {a, -f, b},
  // {-a, f, b},
  // {f, 0, e},
  // {-a, -f, b},
  // {0, d, c},
  // {0,-d, c}

  V = ones_vector<double>(10) | V;

  IncidenceMatrix<> VIF{ {6,7,9},
                         {0,7,9},
                         {1,2,8},
                         {2,3,8},
                         {0,4,9},
                         {3,4,9},
                         {2,3,4},
                         {0,1,2,4},
                         {3,6,9},
                         {3,6,8},
                         {5,6,8},
                         {1,5,8},
                         {5,6,7},
                         {0,1,5,7} };

  BigObject p("Polytope<Float>");
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;

  centralize<double>(p);
  p.set_description() << "Johnson solid J86: Sphenocorona" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject augmented_sphenocorona()
{
  BigObject p = sphenocorona();
  p = augment(p, Set<Int>{0,1,2,4});

  IncidenceMatrix<> VIF{  {6,7,9},
                          {0,7,9},
                          {1,2,8},
                          {2,3,8},
                          {0,4,9},
                          {0,4,10},
                          {2,4,10},
                          {2,3,4},
                          {3,4,9},
                          {1,2,10},
                          {0,1,10},
                          {3,6,9},
                          {3,6,8},
                          {5,6,8},
                          {1,5,8},
                          {5,6,7},
                          {0,1,5,7}};

  p.take("VERTICES_IN_FACETS") << VIF;
  centralize<double>(p);
  p.set_description() << "Johnson solid J87: Augmented sphenocorona" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject sphenomegacorona()
{
  //coordinates from:  Weisstein, Eric W. "Sphenomegacorona." From MathWorld--A Wolfram Web Resource. http://mathworld.wolfram.com/Sphenomegacorona.html
  Matrix<double> V(12,4);
  V.col(0).fill(1);
  V(0,1)=-0.707414; V(0,2)=-0.299887; V(0,3)=-0.154794;
  V(1,1)=-0.648093; V(1,2)=-0.108705; V(1,3)=0.824966;
  V(2,1)=-0.61836; V(2,2)=0.643014; V(2,3)=0.166154;
  V(3,1)=-0.611207; V(3,2)=-1.05253; V(3,3)=0.496573;
  V(4,1)=-0.164695; V(4,2)=0.357205; V(4,3)=-0.677944;
  V(5,1)=-0.000816218; V(5,2)=-0.993806; V(5,3)=-0.293348;
  V(6,1)=0.166715; V(6,2)=0.46752; V(6,3)=0.760172;
  V(7,1)=0.22644; V(7,2)=-0.509058; V(7,3)=0.551267;
  V(8,1)=0.279678; V(8,2)=1.01458; V(8,3)=-0.0693509;
  V(9,1)=0.541903; V(9,2)=-0.336714; V(9,3)=-0.816498;
  V(10,1)=0.769159; V(10,2)=0.148034; V(10,3)=0.0281172;
  V(11,1)=0.781521; V(11,2)=0.633707; V(11,3)=-0.845936;

  IncidenceMatrix<> VIF{ {5,7,9,10},
                         {6,7,10},
                         {6,8,10},
                         {2,6,8},
                         {2,4,8},
                         {1,2,6},
                         {0,2,4},
                         {0,1,3},
                         {0,1,2},
                         {0,3,5},
                         {0,4,5,9},
                         {1,3,7},
                         {3,5,7},
                         {1,6,7},
                         {4,8,11},
                         {4,9,11},
                         {8,10,11},
                         {9,10,11} };

  BigObject p("Polytope<Float>");
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  centralize<double>(p);
  p.set_description() << "Johnson solid J88: Sphenomegacorona" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject hebesphenomegacorona()
{
  //coordinates from:   Weisstein, Eric W. "Hebesphenomegacorona." From MathWorld--A Wolfram Web Resource. http://mathworld.wolfram.com/Hebesphenomegacorona.html
  Matrix<double> V(14,4);
  V.col(0).fill(1);
  V(0,1)=-0.871506; V(0,2)=0.140534; V(0,3)=-0.0100135;
  V(1,1)=-0.734052; V(1,2)=-0.844455; V(1,3)=-0.114409;
  V(2,1)=-0.661763; V(2,2)=-0.422531; V(2,3)=0.789335;
  V(3,1)=-0.604133; V(3,2)=-0.235754; V(3,3)=-0.897098;
  V(4,1)=-0.468061; V(4,2)=0.558417; V(4,3)=0.803993;
  V(5,1)=-0.181065; V(5,2)=0.857521; V(5,3)=-0.106046;
  V(6,1)=0.086308; V(6,2)=0.481232; V(6,3)=-0.99313;
  V(7,1)=0.110015; V(7,2)=-0.89019; V(7,3)=-0.64869;
  V(8,1)=0.152357; V(8,2)=-0.797719; V(8,3)=0.346125;
  V(9,1)=0.246082; V(9,2)=-0.0960143; V(9,3)=1.0524;
  V(10,1)=0.492183; V(10,2)=0.77578; V(10,3)=0.628835;
  V(11,1)=0.786706; V(11,2)=0.734773; V(11,3)=-0.325928;
  V(12,1)=0.800456; V(12,2)=-0.173203; V(12,3)=-0.744722;
  V(13,1)=0.842797; V(13,2)=-0.0807329; V(13,3)=0.250093;

  IncidenceMatrix<> VIF{ {6,11,12},
                         {4,9,10},
                         {7,8,12,13},
                         {4,5,10},
                         {1,7,8},
                         {1,2,8},
                         {0,2,4},
                         {0,1,3},
                         {0,1,2},
                         {0,4,5},
                         {1,3,7},
                         {2,4,9},
                         {2,8,9},
                         {5,10,11},
                         {10,11,13},
                         {8,9,13},
                         {9,10,13},
                         {11,12,13},
                         {3,6,7,12},
                         {0,3,5,6},
                         {5,6,11} };

  BigObject p("Polytope<Float>");
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  centralize<double>(p);
  p.set_description() << "Johnson solid J89: Hebesphenomegacorona" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject disphenocingulum()
{
  //coordinates from:    Weisstein, Eric W. "Disphenocingulum." From MathWorld--A Wolfram Web Resource. http://mathworld.wolfram.com/Disphenocingulum.html
  Matrix<double> V(16,4);
  V.col(0).fill(1);
  V(0,1)=-1.17114; V(0,2)=0.293688; V(0,3)=0.109312;
  V(1,1)=-0.83877; V(1,2)=-0.457649; V(1,3)=0.679417;
  V(2,1)=-0.814439; V(2,2)=-0.537542; V(2,3)=-0.317089;
  V(3,1)=-0.666585; V(3,2)=0.849234; V(3,3)=-0.551588;
  V(4,1)=-0.46044; V(4,2)=0.461075; V(4,3)=0.792599;
  V(5,1)=-0.30988; V(5,2)=0.0180037; V(5,3)=-0.977988;
  V(6,1)=-0.117267; V(6,2)=-0.991154; V(6,3)=0.238053;
  V(7,1)=0.0104825; V(7,2)=-0.901367; V(7,3)=-0.749679;
  V(8,1)=0.0441194; V(8,2)=1.01662; V(8,3)=0.131699;
  V(9,1)=0.113461; V(9,2)=-0.340277; V(9,3)=0.961325;
  V(10,1)=0.297985; V(10,2)=0.793976; V(10,3)=-0.809566;
  V(11,1)=0.530305; V(11,2)=0.56504; V(11,3)=0.879827;
  V(12,1)=0.656861; V(12,2)=-0.138802; V(12,3)=-0.775936;
  V(13,1)=0.803499; V(13,2)=-0.896901; V(13,3)=-0.140495;
  V(14,1)=0.887588; V(14,2)=0.512075; V(14,3)=-0.0526646;
  V(15,1)=1.03423; V(15,2)=-0.246023; V(15,3)=0.582777;

  IncidenceMatrix<> VIF{ {10,12,14},
                         {8,11,14},
                         {8,10,14},
                         {5,10,12},
                         {5,7,12},
                         {3,8,10},
                         {3,5,10},
                         {0,1,2},
                         {1,2,6},
                         {0,1,4},
                         {0,3,4,8},
                         {0,2,3,5},
                         {2,5,7},
                         {2,6,7},
                         {1,4,9},
                         {1,6,9},
                         {4,8,11},
                         {4,9,11},
                         {6,7,13},
                         {7,12,13},
                         {6,9,13,15},
                         {9,11,15},
                         {12,13,14,15},
                         {11,14,15} };

  BigObject p("Polytope<Float>");
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  centralize<double>(p);
  p.set_description() << "Johnson solid J90: Disphenocingulum" << endl;

  return p;
}

BigObject bilunabirotunda()
{
  //coordiantes from https://en.wikipedia.org/wiki/Bilunabirotunda
  Rational s(1,2);
  QE r(s,s,5);
  Matrix<QE> V(14,4);
  V.col(0).fill(1);
  V(0,3)=s*r;
  V(1,3)=-s*r;
  V(2,1)=s*(r+1); V(2,2)=s;
  V(3,1)=s*(r+1); V(3,2)=-s;
  V(4,1)=-s*(r+1); V(4,2)=s;
  V(5,1)=-s*(r+1); V(5,2)=-s;
  V(6,1)=s; V(6,2)=s*r; V(6,3)=s;
  V(7,1)=s; V(7,2)=s*r; V(7,3)=-s;
  V(8,1)=s; V(8,2)=-s*r; V(8,3)=s;
  V(9,1)=s; V(9,2)=-s*r; V(9,3)=-s;
  V(10,1)=-s; V(10,2)=s*r; V(10,3)=s;
  V(11,1)=-s; V(11,2)=s*r; V(11,3)=-s;
  V(12,1)=-s; V(12,2)=-s*r; V(12,3)=s;
  V(13,1)=-s; V(13,2)=-s*r; V(13,3)=-s;

  BigObject p=build_from_vertices(V);
  p.set_description() << "Johnson solid J91: bilunabirotunda" << endl;

  return p;
}

//FIXME: coordinates #830
BigObject triangular_hebesphenorotunda()
{
  //coordiantes from https://en.wikipedia.org/wiki/Triangular_hebesphenorotunda
  Rational s(1,2);
  const double r(QE(s,s,5));
  const double t(QE(0,1,3));
  Matrix<QE> V(18,4);
  V.col(0).fill(1);
  V(0,1)=1; V(0,2)=t;
  V(1,1)=1; V(1,2)=-t;
  V(2,1)=-1; V(2,2)=t;
  V(3,1)=-1; V(3,2)=-t;
  V(4,1)=2;
  V(5,1)=-2;
  V(6,1)=pow(r,2); V(6,2)=pow(r,2)/t; V(6,3)=2/t;
  V(7,1)=-pow(r,2); V(7,2)=pow(r,2)/t; V(7,3)=2/t;
  V(8,2)=-2*pow(r,2)/t; V(8,3)=2/t;
  V(9,1)=1; V(9,2)=pow(r,3)/t; V(9,3)=2*r/t;
  V(10,1)=-1; V(10,2)=pow(r,3)/t; V(10,3)=2*r/t;
  V(11,1)=pow(r,2); V(11,2)=-1/(r*t); V(11,3)=2*r/t;
  V(12,1)=-pow(r,2); V(12,2)=-1/(r*t); V(12,3)=2*r/t;
  V(13,1)=r; V(13,2)=-(r+2)/t; V(13,3)=2*r/t;
  V(14,1)=-r; V(14,2)=-(r+2)/t; V(14,3)=2*r/t;
  V(15,2)=2/t; V(15,3)=2*pow(r,2)/t;
  V(16,1)=1; V(16,2)=-1/t; V(16,3)=2*pow(r,2)/t;
  V(17,1)=-1; V(17,2)=-1/t; V(17,3)=2*pow(r,2)/t;

  IncidenceMatrix<> VIF{ {11,13,16},
                         {1,8,13},
                         {1,3,8},
                         {3,8,14},
                         {2,5,7},
                         {5,7,12},
                         {12,14,17},
                         {2,7,10},
                         {15,16,17},
                         {9,10,15},
                         {0,6,9},
                         {0,4,6},
                         {4,6,11},
                         {1,4,11,13},
                         {3,5,12,14},
                         {0,2,9,10},
                         {8,13,14,16,17},
                         {7,10,12,15,17},
                         {6,9,11,15,16},
                         {0,1,2,3,4,5} };

  BigObject p("Polytope<Float>");
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  centralize<double>(p);
  p.set_description() << "Johnson solid J92: triangular hebesphenorotunda" << endl;

  return p;
}

namespace {

struct dispatcher_t {
   typedef BigObject (*fptr)();

   const char* name;
   fptr func;

   struct to_map_value {
      typedef dispatcher_t argument_type;
      typedef std::pair<const std::string, fptr> result_type;
      result_type operator() (const argument_type& f) const { return result_type(f.name, f.func); }
   };
};

dispatcher_t dispatcher[]={
   { "square_pyramid",                    &square_pyramid },
   { "pentagonal_pyramid",                &pentagonal_pyramid },
   { "triangular_cupola",                 &triangular_cupola },
   { "square_cupola",                     &square_cupola },
   { "pentagonal_cupola",                 &pentagonal_cupola },
   { "pentagonal_rotunda",                &pentagonal_rotunda },
   { "elongated_triangular_pyramid",      &elongated_triangular_pyramid },
   { "elongated_square_pyramid",          &elongated_square_pyramid },
   { "elongated_pentagonal_pyramid",      &elongated_pentagonal_pyramid },    //inexact
   { "gyroelongated_square_pyramid",      &gyroelongated_square_pyramid },    //inexact
   { "gyroelongated_pentagonal_pyramid",  &gyroelongated_pentagonal_pyramid },
   { "triangular_bipyramid",              &triangular_bipyramid },
   { "pentagonal_bipyramid",              &pentagonal_bipyramid },            //inexact
   { "elongated_triangular_bipyramid",    &elongated_triangular_bipyramid },
   { "elongated_square_bipyramid",        &elongated_square_bipyramid },
   { "elongated_pentagonal_bipyramid",    &elongated_pentagonal_bipyramid },  //inexact
   { "gyroelongated_square_bipyramid",    &gyroelongated_square_bipyramid },  //inexact
   { "elongated_triangular_cupola",       &elongated_triangular_cupola },     //inexact
   { "elongated_square_cupola",           &elongated_square_cupola },
   { "elongated_pentagonal_cupola",       &elongated_pentagonal_cupola },     //inexact
   { "elongated_pentagonal_rotunda",      &elongated_pentagonal_rotunda },    //inexact
   { "gyroelongated_triangular_cupola",   &gyroelongated_triangular_cupola }, //inexact
   { "gyroelongated_square_cupola",       &gyroelongated_square_cupola },     //inexact
   { "gyroelongated_pentagonal_cupola",   &gyroelongated_pentagonal_cupola },  //inexact
   { "gyroelongated_pentagonal_rotunda",  &gyroelongated_pentagonal_rotunda }, //inexact
   { "gyrobifastigium",                   &gyrobifastigium },
   { "triangular_orthobicupola",          &triangular_orthobicupola },
   { "square_orthobicupola",              &square_orthobicupola },
   { "square_gyrobicupola",               &square_gyrobicupola },
   { "pentagonal_orthobicupola",          &pentagonal_orthobicupola },       //inexact
   { "pentagonal_gyrobicupola",           &pentagonal_gyrobicupola },
   { "pentagonal_orthocupolarotunda",     &pentagonal_orthocupolarotunda },  //inexact
   { "pentagonal_gyrocupolarotunda",      &pentagonal_gyrocupolarotunda },   //inexact
   { "pentagonal_orthobirotunda",         &pentagonal_orthobirotunda },      //inexact
   { "elongated_triangular_orthobicupola",      &elongated_triangular_orthobicupola },      //inexact
   { "elongated_triangular_gyrobicupola",       &elongated_triangular_gyrobicupola },       //inexact
   { "elongated_square_gyrobicupola",           &elongated_square_gyrobicupola },
   { "elongated_pentagonal_orthobicupola",      &elongated_pentagonal_orthobicupola },      //inexact
   { "elongated_pentagonal_gyrobicupola",       &elongated_pentagonal_gyrobicupola },       //inexact
   { "elongated_pentagonal_orthocupolarotunda", &elongated_pentagonal_orthocupolarotunda }, //inexact
   { "elongated_pentagonal_gyrocupolarotunda",  &elongated_pentagonal_gyrocupolarotunda },  //inexact
   { "elongated_pentagonal_orthobirotunda",     &elongated_pentagonal_orthobirotunda },     //inexact
   { "elongated_pentagonal_gyrobirotunda",      &elongated_pentagonal_gyrobirotunda },      //inexact
   { "gyroelongated_triangular_bicupola",       &gyroelongated_triangular_bicupola },       //inexact
   { "gyroelongated_square_bicupola",           &gyroelongated_square_bicupola },           //inexact
   { "gyroelongated_pentagonal_bicupola",       &gyroelongated_pentagonal_bicupola },       //inexact
   { "gyroelongated_pentagonal_cupolarotunda",  &gyroelongated_pentagonal_cupolarotunda },  //inexact
   { "gyroelongated_pentagonal_birotunda",      &gyroelongated_pentagonal_birotunda },      //inexact
   { "augmented_triangular_prism",              &augmented_triangular_prism },       //inexact
   { "biaugmented_triangular_prism",            &biaugmented_triangular_prism },     //inexact
   { "triaugmented_triangular_prism",           &triaugmented_triangular_prism },    //inexact
   { "augmented_pentagonal_prism",              &augmented_pentagonal_prism },       //inexact
   { "biaugmented_pentagonal_prism",            &biaugmented_pentagonal_prism },     //inexact
   { "augmented_hexagonal_prism",               &augmented_hexagonal_prism },        //inexact
   { "parabiaugmented_hexagonal_prism",         &parabiaugmented_hexagonal_prism },  //inexact
   { "metabiaugmented_hexagonal_prism",         &metabiaugmented_hexagonal_prism },  //inexact
   { "triaugmented_hexagonal_prism",            &triaugmented_hexagonal_prism },     //inexact
   { "augmented_dodecahedron",                  &augmented_dodecahedron },           //inexact
   { "parabiaugmented_dodecahedron",            &parabiaugmented_dodecahedron },     //inexact
   { "metabiaugmented_dodecahedron",            &metabiaugmented_dodecahedron },     //inexact
   { "triaugmented_dodecahedron",               &triaugmented_dodecahedron },        //inexact
   { "metabidiminished_icosahedron",            &metabidiminished_icosahedron },
   { "tridiminished_icosahedron",               &tridiminished_icosahedron },
   { "augmented_tridiminished_icosahedron",     &augmented_tridiminished_icosahedron },  //inexact
   { "augmented truncated tetrahedron",         &augmented_truncated_tetrahedron },
   { "augmented truncated cube",                &augmented_truncated_cube },
   { "biaugmented truncated cube",              &biaugmented_truncated_cube },
   { "augmented_truncated_dodecahedron",        &augmented_truncated_dodecahedron },       //inexact
   { "parabiaugmented_truncated_dodecahedron",  &parabiaugmented_truncated_dodecahedron }, //inexact
   { "metabiaugmented_truncated_dodecahedron",  &metabiaugmented_truncated_dodecahedron }, //inexact
   { "triaugmented_truncated_dodecahedron",     &triaugmented_truncated_dodecahedron },    //inexact
   { "gyrate_rhombicosidodecahedron",           &gyrate_rhombicosidodecahedron },          //inexact
   { "parabigyrate_rhombicosidodecahedron",     &parabigyrate_rhombicosidodecahedron },    //inexact
   { "metabigyrate_rhombicosidodecahedron",     &metabigyrate_rhombicosidodecahedron },    //inexact
   { "trigyrate_rhombicosidodecahedron",        &trigyrate_rhombicosidodecahedron },       //inexact
   { "diminished_rhombicosidodecahedron",       &diminished_rhombicosidodecahedron },
   { "paragyrate_diminished_rhombicosidodecahedron",  &paragyrate_diminished_rhombicosidodecahedron },  //inexact
   { "metagyrate_diminished_rhombicosidodecahedron",  &metagyrate_diminished_rhombicosidodecahedron },  //inexact
   { "bigyrate_diminished_rhombicosidodecahedron",    &bigyrate_diminished_rhombicosidodecahedron },    //inexact
   { "parabidiminished_rhombicosidodecahedron",       &parabidiminished_rhombicosidodecahedron },
   { "metabidiminished_rhombicosidodecahedron",       &metabidiminished_rhombicosidodecahedron },
   { "gyrate_bidiminished_rhombicosidodecahedron",    &gyrate_bidiminished_rhombicosidodecahedron },    //inexact
   { "tridiminished_rhombicosidodecahedron",          &tridiminished_rhombicosidodecahedron },
   { "snub_disphenoid",              &snub_disphenoid }, //inexact
   { "snub_square_antisprism",       &snub_square_antiprism },  //inexact
   { "sphenocorona",                 &sphenocorona },  //inexact
   { "augmented_sphenocorona",       &augmented_sphenocorona },  //inexact
   { "sphenomegacorona",             &sphenomegacorona },  //inexact
   { "hebesphenomegacorona",         &hebesphenomegacorona }, //inexact
   { "disphenocingulum",             &disphenocingulum },   //inexact
   { "bilunabirotunda",              &bilunabirotunda },
   { "triangular_hebesphenorotunda", &triangular_hebesphenorotunda } //inexact
};

const size_t dispatcher_size = sizeof(dispatcher) / sizeof(dispatcher[0]);

}

BigObject johnson_int(Int n)
{
  const size_t index(n-1);
  if (index < dispatcher_size)
    return dispatcher[index].func();
  else
    throw std::runtime_error("invalid index of Johnson polytope");
}

BigObject johnson_str(std::string s)
{
  using func_map_t = hash_map<std::string, dispatcher_t::fptr> ;
  static const func_map_t func_map(make_unary_transform_iterator(dispatcher+0, dispatcher_t::to_map_value()),
                                   make_unary_transform_iterator(dispatcher+dispatcher_size, dispatcher_t::to_map_value()));
  const auto it=func_map.find(s);
  if (it != func_map.end())
    return (it->second)();
  else
    throw std::runtime_error("unknown name of Johnson polytope");
}

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create Johnson solid number n, where 1 <= n <= 92."
                  "# A Johnson solid is a 3-polytope all of whose facets are regular polygons."
                  "# Some are realized with floating point numbers and thus not exact;"
                  "# yet [[VERTICES_IN_FACETS]] is correct in all cases."
                  "# @param Int n the index of the desired Johnson polytope"
                  "# @return Polytope",
                  &johnson_int, "johnson_solid(Int)");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create Johnson solid with the given name."
                  "# A Johnson solid is a 3-polytope all of whose facets are regular polygons."
                  "# Some are realized with floating point numbers and thus not exact;"
                  "# yet [[VERTICES_IN_FACETS]] is correct in all cases."
                  "# @param String s the name of the desired Johnson polytope"
                  "# @value s 'square_pyramid' Square pyramid with regular facets. Johnson solid J1."
                  "# @value s 'pentagonal_pyramid' Pentagonal pyramid with regular facets. Johnson solid J2."
                  "# @value s 'triangular_cupola' Triangular cupola with regular facets. Johnson solid J3."
                  "# @value s 'square_cupola' Square cupola with regular facets. Johnson solid J4."
                  "# @value s 'pentagonal_cupola' Pentagonal cupola with regular facets. Johnson solid J5."
                  "# @value s 'pentagonal_rotunda' Pentagonal rotunda with regular facets. Johnson solid J6."
                  "# @value s 'elongated_triangular_pyramid' Elongated triangular pyramid with regular facets. Johnson solid J7."
                  "# @value s 'elongated_square_pyramid' Elongated square pyramid with regular facets. Johnson solid J8."
                  "# @value s 'elongated_pentagonal_pyramid' Elongated pentagonal pyramid with regular facets. Johnson solid J9."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyroelongated_square_pyramid' Gyroelongated square pyramid with regular facets. Johnson solid J10."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyroelongated_pentagonal_pyramid' Gyroelongated pentagonal pyramid with regular facets. Johnson solid J11."
                  "# @value s 'triangular_bipyramid' Triangular bipyramid with regular facets. Johnson solid J12."
                  "# @value s 'pentagonal_bipyramid' Pentagonal bipyramid with regular facets. Johnson solid J13."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_triangular_bipyramid' Elongated triangular bipyramid with regular facets. Johnson solid J14."
                  "# @value s 'elongated_square_bipyramid' Elongated square bipyramid with regular facets. Johnson solid J15."
                  "# @value s 'elongated_pentagonal_bipyramid' Elongated pentagonal bipyramid with regular facets. Johnson solid J16."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyroelongated_square_bipyramid' Gyroelongted square bipyramid with regular facets. Johnson solid J17."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_triangular_cupola' Elongted triangular cupola with regular facets. Johnson solid J18."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_square_cupola' Elongted square cupola with regular facets. Johnson solid J19."
                  "# @value s 'elongated_pentagonal_cupola' Elongted pentagonal cupola with regular facets. Johnson solid J20"
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_pentagonal_rotunda' Elongated pentagonal rotunda with regular facets. Johnson solid J21."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyroelongated_triangular_cupola' Gyroelongted triangular cupola with regular facets. Johnson solid J22."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyroelongated_square_cupola' Gyroelongted square cupola with regular facets. Johnson solid J23."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyroelongated_pentagonal_cupola' Gyroelongted pentagonal cupola with regular facets. Johnson solid J24."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyroelongated_pentagonal_rotunda' Gyroelongted pentagonal rotunda with regular facets. Johnson solid J25."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyrobifastigium' Gyrobifastigium with regular facets. Johnson solid J26."
                  "# @value s 'triangular_orthobicupola' Triangular orthobicupola with regular facets. Johnson solid J27."
                  "# @value s 'square_orthobicupola' Square orthobicupola with regular facets. Johnson solid J28."
                  "# @value s 'square_gyrobicupola' Square gyrobicupola with regular facets. Johnson solid J29."
                  "# @value s 'pentagonal_orthobicupola' Pentagonal orthobicupola with regular facets. Johnson solid J30."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'pentagonal_gyrobicupola' Pentagonal gyrobicupola with regular facets. Johnson solid J31."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'pentagonal_orthocupolarotunda' Pentagonal orthocupolarotunda with regular facets. Johnson solid J32."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'pentagonal_gyrocupolarotunda' Pentagonal gyrocupolarotunda with regular facets. Johnson solid J33."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'pentagonal_orthobirotunda' Pentagonal orthobirotunda with regular facets. Johnson solid J34."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_triangular_orthobicupola' Elongated triangular orthobicupola with regular facets. Johnson solid J35."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_triangular_gyrobicupola' Elongated triangular gyrobicupola with regular facets. Johnson solid J36."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_square_gyrobicupola' Elongated square gyrobicupola with regular facets. Johnson solid J37."
                  "# @value s 'elongated_pentagonal_orthobicupola' Elongated pentagonal orthobicupola with regular facets. Johnson solid J38."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_pentagonal_gyrobicupola' Elongated pentagonal gyrobicupola with regular facets. Johnson solid J39."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_pentagonal_orthocupolarotunda' Elongated pentagonal orthocupolarotunda with regular facets. Johnson solid J40."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_pentagonal_gyrocupolarotunda' Elongated pentagonal gyrocupolarotunda with regular facets. Johnson solid J41."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_pentagonal_orthobirotunda' Elongated pentagonal orthobirotunda with regular facets. Johnson solid J42."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_pentagonal_gyrobirotunda' Elongated pentagonal gyrobirotunda with regular facets. Johnson solid J43."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyroelongated_triangular_bicupola' Gyroelongated triangular bicupola with regular facets. Johnson solid J44."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_square_bicupola' Elongated square bicupola with regular facets. Johnson solid J45."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyroelongated_pentagonal_bicupola' Gyroelongated pentagonal bicupola with regular facets. Johnson solid J46."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyroelongated_pentagonal_cupolarotunda' Gyroelongated pentagonal cupolarotunda with regular facets. Johnson solid J47."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyroelongated_pentagonal_birotunda' Gyroelongated pentagonal birotunda with regular facets. Johnson solid J48."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'augmented_triangular_prism' Augmented triangular prism with regular facets. Johnson solid J49."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'biaugmented_triangular_prism' Biaugmented triangular prism with regular facets. Johnson solid J50."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'triaugmented_triangular_prism' Triaugmented triangular prism with regular facets. Johnson solid J51."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'augmented_pentagonal_prism' Augmented prantagonal prism  with regular facets. Johnson solid J52."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'biaugmented_pentagonal_prism' Augmented pentagonal prism with regular facets. Johnson solid J53."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'augmented_hexagonal_prism' Augmented hexagonal prism with regular facets. Johnson solid J54."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'parabiaugmented_hexagonal_prism' Parabiaugmented hexagonal prism with regular facets. Johnson solid J55."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'metabiaugmented_hexagonal_prism' Metabiaugmented hexagonal prism with regular facets. Johnson solid J56."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'triaugmented_hexagonal_prism' triaugmented hexagonal prism with regular facets. Johnson solid J57."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'augmented_dodecahedron' Augmented dodecahedron with regular facets. Johnson solid J58."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'parabiaugmented_dodecahedron' Parabiaugmented dodecahedron with regular facets. Johnson solid J59."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'metabiaugmented_dodecahedron' Metabiaugmented dodecahedron with regular facets. Johnson solid J60."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'triaugmented_dodecahedron' Triaugmented dodecahedron with regular facets. Johnson solid J61."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'metabidiminished_icosahedron' Metabidiminished icosahedron with regular facets. Johnson solid J62."
                  "# @value s 'tridiminished_icosahedron' Tridiminished icosahedron with regular facets. Johnson solid J63."
                  "# @value s 'augmented_tridiminished_icosahedron' Augmented tridiminished icosahedron with regular facets. Johnson solid J64."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'augmented_truncated_tetrahedron' Augmented truncated tetrahedron with regular facets. Johnson solid J65."
                  "# @value s 'augmented_truncated_cube' Augmented truncated cube with regular facets. Johnson solid J66."
                  "# @value s 'biaugmented_truncated_cube' Biaugmented truncated cube with regular facets. Johnson solid J67."
                  "# @value s 'augmented_truncated_dodecahedron' Augmented truncated dodecahedron with regular facets. Johnson solid J68."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'parabiaugmented_truncated_dodecahedron' Parabiaugmented truncated dodecahedron with regular facets. Johnson solid J69."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'metabiaugmented_truncated_dodecahedron' Metabiaugmented truncated dodecahedron with regular facets. Johnson solid J70."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'triaugmented_truncated_dodecahedron' Triaugmented truncated dodecahedron with regular facets. Johnson solid J71."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyrate_rhombicosidodecahedron' Gyrate rhombicosidodecahedron with regular facets. Johnson solid J72."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'parabigyrate_rhombicosidodecahedron' Parabigyrate rhombicosidodecahedron with regular facets. Johnson solid J73."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'metabigyrate_rhombicosidodecahedron' Metabigyrate rhombicosidodecahedron with regular facets. Johnson solid J74."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'trigyrate_rhombicosidodecahedron' Trigyrate rhombicosidodecahedron with regular facets. Johnson solid J75."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'diminished_rhombicosidodecahedron' Diminished rhombicosidodecahedron with regular facets. Johnson solid J76."
                  "# @value s 'paragyrate_diminished_rhombicosidodecahedron' Paragyrate diminished rhombicosidodecahedron with regular facets. Johnson solid J77."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'metagyrate_diminished_rhombicosidodecahedron' Metagyrate diminished rhombicosidodecahedron with regular facets. Johnson solid J78."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'bigyrate_diminished_rhombicosidodecahedron' Bigyrate diminished rhombicosidodecahedron with regular facets. Johnson solid J79."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'parabidiminished_rhombicosidodecahedron' Parabidiminished rhombicosidodecahedron with regular facets. Johnson solid J80."
                  "# @value s 'metabidiminished_rhombicosidodecahedron' Metabidiminished rhombicosidodecahedron with regular facets. Johnson solid J81."
                  "# @value s 'gyrate_bidiminished_rhombicosidodecahedron' Gyrate bidiminished rhombicosidodecahedron with regular facets. Johnson solid J82."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'tridiminished_rhombicosidodecahedron' Tridiminished rhombicosidodecahedron with regular facets. Johnson solid J83."
                  "# @value s 'snub_disphenoid' Snub disphenoid with regular facets. Johnson solid J84."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'snub_square_antisprim' Snub square antiprism with regular facets. Johnson solid J85."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'sphenocorona' Sphenocorona with regular facets. Johnson solid J86."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'augmented_sphenocorona' Augmented sphenocorona with regular facets. Johnson solid J87."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'sphenomegacorona' Sphenomegacorona with regular facets. Johnson solid J88."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'hebesphenomegacorona' Hebesphenomegacorona with regular facets. Johnson solid J89."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'disphenocingulum' Disphenocingulum with regular facets. Johnson solid J90."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'bilunabirotunda' Bilunabirotunda with regular facets. Johnson solid J91."
                  "# @value s 'triangular_hebesphenorotunda' Triangular hebesphenorotunda with regular facets. Johnson solid J92."
                  "# @return Polytope",
                  &johnson_str, "johnson_solid(String)");
} }


// Local Variables:
// mode:C++
// c-basic-offset:2
// indent-tabs-mode:nil
// End:
