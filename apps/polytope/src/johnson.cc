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

 double norm(Vector<double> v){
  double n = 0;
  int d = v.size();
  int i = 0;
  if(d == 4) i=1; //hom.coords...
  for(;i<v.size(); i++){
    n = n + v[i]*v[i];
  }
  return sqrt(n);
 }

 //rotates vectors in V by angle a about axis u (given in hom. coords) using rodrigues' rotation formula
 Matrix<double> rotate(Matrix<double> V, Vector<double> u, double a){

    u[0]=0; //projection to plane
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
  Matrix<double> create_regular_polygon_vertices(int n, double r, double s){
   if (n<3)
      throw std::runtime_error("At least three vertices required.");
   if (r<=0)
      throw std::runtime_error("Radius must be >0");
   Matrix<double> V(n,3);
   V.col(0).fill(1);

   double phi = 2*M_PI/n;
   for(int i=0; i<n; i++){
     V(i,1)=r*cos(s+i*phi);
     V(i,2)=r*sin(s+i*phi);
   }
   return V;
 }

//creates an exact octagonal prism with z-coordinates z_1 and z_2
perl::Object exact_octagonal_prism(QE z_1, QE z_2)
{
  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  Matrix<QE> V(16,4);
  V.col(0).fill(1);
  for (int i=0; i<8; ++i) {
    V(i,3) = z_1;
    V(i+8,3) = z_2;
  }

  QE q(1,1,2);
  V(0,1)=V(1,2)=V(3,1)=V(6,2)=V(8,1)=V(9,2)=V(11,1)=V(14,2)=1;
  V(2,2)=V(4,1)=V(5,2)=V(7,1)=V(10,2)=V(12,1)=V(13,2)=V(15,1)=-1;
  V(0,2)=V(1,1)=V(2,1)=V(7,2)=V(8,2)=V(9,1)=V(10,1)=V(15,2)=q;
  V(3,2)=V(4,2)=V(5,1)=V(6,1)=V(11,2)=V(12,2)=V(13,1)=V(14,1)=-q;

   p.take("VERTICES") << V;

   return p;
}

//augments the facet given as a vertex index set
//uses sqrt and cos to calculate height, thus not exact
perl::Object augment(perl::Object p, Set<int> f_vert){

   Matrix<double> V = p.give("VERTICES");
   IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");

   int i = 0;
   for(; i<VIF.rows(); i++){
      if(VIF.row(i) == f_vert) break; //find facet index
   }

   Array<Array<int> > c = p.give("VIF_CYCLIC_NORMAL");
   Array<int> neigh = c[i];
   double side_length = norm(V[neigh[0]]-V[neigh[1]]);

   Matrix<double> FV = V.minor(f_vert,All);
   Vector<double> bary = average(rows(FV));

   Matrix<double> F = p.give("FACETS");
   Vector<double> normal = F.row(i);
   normal[0]=0; //project to plane
   normal = normal/norm(normal); //unit facet normal

  int n_vert = f_vert.size();
   if(n_vert == 3 || n_vert == 4 || n_vert == 5){ //pyramid
      double height = side_length*sqrt(1-1/(2-2*cos(2*M_PI/n_vert)));
      normal *= height;
      V /= (bary - normal);
   }else{
      Matrix<double> trans = zero_vector<double>( V.rows()) | repeat_row(-average(rows(FV)), V.rows()).minor(All,sequence(1,3));
      V += trans; //translate barycentre to zero
      double r = (side_length/(2*sin(M_PI/(n_vert/2)))) ; //circumradius of n-gon with side_length
      Matrix<double> H(0,4);
      Vector<double> v;
      for(int i = 0; i<n_vert-1; i=i+2){
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
   perl::Object p_out(perl::ObjectType::construct<double>("Polytope"));
   p_out.take("VERTICES") << V;

   return p_out;
}

//places a rotunda on a facet given as a vertex index set (must be a decagonal facet)
//not exact
perl::Object rotunda(perl::Object p, Set<int> f_vert){

   if(10 != f_vert.size()) throw std::runtime_error("Facet must be decagon.");

   IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");
   int n=0;
   for (; n<VIF.rows(); n++) {
      if(VIF.row(n) == f_vert) break;//find facet index
   }
   Array<Array<int> > c = p.give("VIF_CYCLIC_NORMAL");
   Array<int> neigh = c[n];
   Matrix<double> V = p.give("VERTICES");
   double side_length = norm(V[neigh[0]]-V[neigh[1]]);

   Matrix<double> F = p.give("FACETS");
   Vector<double> normal = F.row(n);
   normal[0]=0; //project to plane
   normal = normal/norm(normal); //unit facet normal

   Matrix<double> FV = V.minor(f_vert,All);
   Vector<double> bary = average(rows(FV));
   Matrix<double> trans = zero_vector<double>( V.rows()) | repeat_row(-average(rows(FV)), V.rows()).minor(All,sequence(1,3));
   V += trans; //translate barycentre to zero

   double r_l = (1+sqrt(5))*sqrt(10*(5+sqrt(5)))*side_length/20;
   double r_s = sqrt((5+sqrt(5))/10)*side_length;
   Matrix<double> H_l(0,4);
   Matrix<double> H_s(0,4);
   Vector<double> v;
   for(int i = 0; i<9; i=i+2){
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

   perl::Object p_out(perl::ObjectType::construct<double>("Polytope"));
   p_out.take("VERTICES") << V;

   return p_out;
}

//places a rotated rotunda on a facet given as vertex index set (must be a decagonal facet)
//inexact
perl::Object gyrotunda(perl::Object p, Set<int> f_vert){
   if(10 != f_vert.size()) throw std::runtime_error("Facet must be decagon.");

   IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");
   int n=0;
   for(; n<VIF.rows(); n++) if(VIF.row(n)==f_vert) break;//find facet index

   Array<Array<int> > c = p.give("VIF_CYCLIC_NORMAL");
   Array<int> neigh = c[n];
   Matrix<double> V = (p.give("VERTICES"));
   double side_length = norm(V[neigh[0]]-V[neigh[1]]);

   Matrix<double> F = p.give("FACETS");
   Vector<double> normal = F.row(n);
   normal[0]=0; //project to plane
   normal = normal/norm(normal); //unit facet normal

   Matrix<double> FV = V.minor(f_vert,All);
   Vector<double> bary = average(rows(FV));
   Matrix<double> trans = zero_vector<double>( V.rows()) | repeat_row(-average(rows(FV)), V.rows()).minor(All,sequence(1,3));
   V += trans; //translate barycentre to zero

   double r_l = (1+sqrt(5))*sqrt(10*(5+sqrt(5)))*side_length/20;
   double r_s = sqrt((5+sqrt(5))/10)*side_length;
   Matrix<double> H_l(0,4);
   Matrix<double> H_s(0,4);
   Vector<double> v;
   for(int i = 0; i<9; i=i+2){
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

   perl::Object p_out(perl::ObjectType::construct<double>("Polytope"));
   p_out.take("VERTICES") << V;

   return p_out;
}

//elongates the facet given as vertex index set
//uses sqrt to calculate side length, thus not exact
perl::Object elongate(perl::Object p, Set<int> f_vert){

   Matrix<double> V = (p.give("VERTICES"));
   IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");
   Matrix<double> F = p.give("FACETS");

   perl::Object facet(perl::ObjectType::construct<double>("Polytope"));

   Matrix<double> FV = V.minor(f_vert,All);
   facet.take("VERTICES")<<FV;

   IncidenceMatrix<> FVIF = facet.give("VERTICES_IN_FACETS");
   Matrix<double> neighbors = FV.minor(FVIF.row(0),All); //two neighbor vertices
   double side_length = norm(neighbors[0]-neighbors[1]);
   int n_vert = facet.give("N_VERTICES");

   perl::Object p_out(perl::ObjectType::construct<double>("Polytope"));

   int n=0;
   for(; n<VIF.rows(); n++){
      if(VIF.row(n)==f_vert) break;
   }

   Vector<double> normal = F.row(n);
   normal[0]=0; //project to plane
   normal = side_length*normal/norm(normal); //facet normal of length side_length
   p_out.take("VERTICES") << (V / (FV - repeat_row(normal, n_vert)));

   return p_out;
}

//rotates the facet given as vertex index set by angle a
//uses sqrt to calculate side length, thus not exact
  perl::Object rotate_facet(perl::Object p, Set<int> f_vert, double a){

  Matrix<double> V = (p.give("VERTICES"));
  IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");
  Matrix<double> F = p.give("FACETS");

  Matrix<double> FV = V.minor(f_vert,All);

  int nv = p.give("N_VERTICES");
  Matrix<double> W(0,4);
  int n=0;
  for(; n<nv; n++) if(VIF.row(n)==f_vert) break; //find facet index

  for(int i=0; i<nv; i++){
     if(VIF(n,i)==0){
        W /= V.row(i); //remove facet vertices
     }
  }

  Vector<double> normal = F.row(n); //facet normal
  normal[0]=0;

  Matrix<double> trans = zero_vector<double>(FV.rows()) | repeat_row(normal - average(rows(FV)), FV.rows()).minor(All,sequence(1,3)); //translate barycentre to axis

  Matrix<double> Rot = rotate(FV+trans, normal, a)-trans;

  perl::Object p_out(perl::ObjectType::construct<double>("Polytope"));
  p_out.take("VERTICES") << (W/Rot);

  return p_out;
}

//gyroelongates the facet given as vertex index set
//uses sqrt to calculate side length, thus not exact
perl::Object gyroelongate(perl::Object p, Set<int> f_vert){

  Matrix<double> V = (p.give("VERTICES"));
  IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");
  Matrix<double> F = p.give("FACETS");
  int n=0;
  for(; n<VIF.rows(); n++) if(VIF.row(n)==f_vert) break; //find facet index
  Vector<double> normal = F.row(n);
  normal[0]=0; //project to plane
  normal /= norm(normal); //unit facet normal

  Matrix<double> FV = V.minor(f_vert,All);
  int n_vert = FV.rows();
  Matrix<double> trans = zero_vector<double>(n_vert) | repeat_row(normal - average(rows(FV)), FV.rows()).minor(All,sequence(1,3)); //translate barycentre to axis
  FV = rotate(FV+trans, normal, M_PI/n_vert)-trans; //rotate the facet by pi/n

  perl::Object facet(perl::ObjectType::construct<double>("Polytope"));
  facet.take("VERTICES")<<FV;
  IncidenceMatrix<> FVIF = facet.give("VERTICES_IN_FACETS");
  Matrix<double> neighbors = FV.minor(FVIF.row(0),All); //two neighbor vertices
  double side_length = norm(neighbors[0]-neighbors[1]);

  // wikipedia seems to be wrong https://en.wikipedia.org/wiki/Antiprism
  //   double height = 2*sqrt((cos(M_PI/n_vert)-cos(2.0*M_PI/n_vert))/2);
  //   the reciprocal might be correct
  // source: http://mathworld.wolfram.com/Antiprism.html
  double height = sqrt(1-1.0/(4.0*cos(M_PI/(2*n_vert))*cos(M_PI/(2*n_vert))));
  normal = (side_length*height)*normal; //facet normal of right length (height of a triangle)

  perl::Object p_out(perl::ObjectType::construct<double>("Polytope"));
  p_out.take("VERTICES") << (V / (FV - repeat_row(normal, n_vert)));

  return p_out;
}

//creates regular n-gonal prism
perl::Object create_prism(int n)
{
  Matrix<double> V = create_regular_polygon_vertices(n, 1, 0);
  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.take("VERTICES") << V;
  IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");
  Matrix<double> F = p.give("FACETS");

  Matrix<double> neighbors = V.minor(VIF.row(0),All); //two neighbor vertices
  double side_length = norm(neighbors[0]-neighbors[1]);

  perl::Object p_out(perl::ObjectType::construct<double>("Polytope"));

  p_out.take("VERTICES") << (V | zero_vector<double>()) / (V | same_element_vector<double>(side_length, n));

  return p_out;
}

//removes the facet given as vertex index set
template <typename T>
perl::Object diminish(perl::Object p, Set<int> f_vert){

  Matrix<T> V = p.give("VERTICES");

  Set<int> v = sequence(0,V.rows());
  v -= f_vert;

  perl::Object p_out(perl::ObjectType::construct<T>("Polytope"));
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

//the following functions are for improved readability of VIF
Set<int> triangle(int v0, int v1, int v2)
{
   Set<int> s;
   s += v0;
   s += v1;
   s += v2;
   return s;
}
Set<int> square(int v0, int v1, int v2, int v3)
{
   Set<int> s;
   s += v0;
   s += v1;
   s += v2;
   s += v3;
   return s;
}
Set<int> pentagon(int v0, int v1, int v2, int v3, int v4)
{
   Set<int> s;
   s += v0;
   s += v1;
   s += v2;
   s += v3;
   s += v4;
   return s;
}
Set<int> hexagon(int v0, int v1, int v2, int v3, int v4, int v5)
{
   Set<int> s;
   s += v0;
   s += v1;
   s += v2;
   s += v3;
   s += v4;
   s += v5;
   return s;
}
Set<int> octagon(int v0, int v1, int v2, int v3, int v4, int v5, int v6, int v7)
{
   Set<int> s;
   s += v0;
   s += v1;
   s += v2;
   s += v3;
   s += v4;
   s += v5;
   s += v6;
   s += v7;
   return s;
}
Set<int> decagon(int v0, int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8, int v9)
{
   Set<int> s;
   s += v0;
   s += v1;
   s += v2;
   s += v3;
   s += v4;
   s += v5;
   s += v6;
   s += v7;
   s += v8;
   s += v9;
   return s;
}

template <typename T>
perl::Object centralize(perl::Object p)
{
  p.take("AFFINE_HULL") << Matrix<T>();
  p = CallPolymakeFunction("center",p);
  return p;
}


} // end anonymous namespace

perl::Object square_pyramid()
{
  Vector<QE> tip(4);
  tip[0]=1;
  tip[1]=tip[2]=0;
  tip[3]=QE(0,1,2);

  Matrix<QE> V((create_square_vertices<QE>() | zero_vector<QE>(4)) / tip);

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);
  p.set_description() << "Johnson solid J1: Square pyramid" << endl;

  return p;
}

perl::Object pentagonal_pyramid()
{
  perl::Object ico = CallPolymakeFunction("icosahedron");
  Matrix<QE> V = ico.give("VERTICES");
  V = V.minor(sequence(0,6),All);

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);
  p.set_description() << "Johnson solid J2: Pentagonal pyramid" << endl;

  return p;
}

perl::Object triangular_cupola()
{
  perl::Object cub = CallPolymakeFunction("cuboctahedron");
  Matrix<QE> V = cub.give("VERTICES");
  V = V.minor(sequence(0,9),All);

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);
  p.set_description() << "Johnson solid J3: Triangular cupola" << endl;

  return p;
}

perl::Object square_cupola_impl(bool centered)
{
  perl::Object base = exact_octagonal_prism(QE(0,0,0),QE(1,0,0));

  Matrix<QE> B =  base.give("VERTICES");
  Matrix<QE> V = B.minor(sequence(0,8),All);

  QE height(0,1,2);

  Matrix<QE> W(4,4);
  W.col(0).fill(1);
  W.col(3).fill(height);
  W(0,1)=W(0,2)=W(1,1)=W(2,2)=1;
  W(1,2)=W(2,1)=W(3,1)=W(3,2)=-1;

  V/= W;

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  if (centered)
    p = centralize<QE>(p);
  p.set_description() << "Johnson solid J4: Square cupola" << endl;

  return p;
}

perl::Object square_cupola()
{
  return square_cupola_impl(true);
}

perl::Object pentagonal_cupola()
{
  perl::Object rico = CallPolymakeFunction("rhombicosidodecahedron");
  Matrix<QE> V = rico.give("VERTICES");
  V= V.minor(sequence(0,7),All) / V.minor(sequence(8,3),All) / V.row(13) / V.row(14) / V.row(18) / V.row(19) / V.row(24);

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);
  p.set_description() << "Johnson solid J5: Pentagonal cupola" << endl;

  return p;
}

perl::Object pentagonal_rotunda()
{
  perl::Object ico = CallPolymakeFunction("icosidodecahedron");
  Matrix<QE> V = ico.give("VERTICES");
  V= V.minor(sequence(0,17),All) / V.row(18) / V.row(19) / V.row(21);

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);
  p.set_description() << "Johnson solid J6: Pentagonal rotunda" << endl;

  return p;
}

perl::Object elongated_triangular_pyramid()
{
  QE c(Rational(-1,3),0,0);

  QE s(0,Rational(1,3),6);

  Matrix<QE> V( ones_vector<QE>(7) | (same_element_vector<QE>(c,3) / unit_matrix<QE>(3) / (unit_matrix<QE>(3) + same_element_matrix<QE>(s , 3, 3))));

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);
  p.set_description() << "Johnson solid J7: Elongated triangular bipyramid" << endl;

  return p;
}

perl::Object elongated_square_pyramid_impl(bool centered)
{
  Matrix<QE> square_vertices =  create_square_vertices<QE>();

  Vector<QE> tip(4);
  tip[0]=1;
  tip[1]=tip[2]=0;
  tip[3]=QE(0,1,2);

  Matrix<QE> V( ((square_vertices | zero_vector<QE>(4)) / (square_vertices | -2*ones_vector<QE>(4))) / tip );

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  if (centered)
    p = centralize<QE>(p);
  p.set_description() << "Johnson solid J8: Elongated square pyramid" << endl;

  return p;
}

perl::Object elongated_square_pyramid()
{
  return elongated_square_pyramid_impl(true);
}

//FIXME: #830 coordinates
perl::Object elongated_pentagonal_pyramid()
{
  perl::Object p = pentagonal_pyramid();
  p = elongate(p,sequence(1,5));

  IncidenceMatrix<> VIF(11,11);
  VIF[0]=pentagon(6,7,8,9,10);
  VIF[1]=square(1,3,6,8);
  VIF[2]=square(3,5,8,10);
  VIF[3]=triangle(0,3,5);
  VIF[4]=triangle(0,1,3);
  VIF[5]=triangle(0,4,5);
  VIF[6]=square(4,5,9,10);
  VIF[7]=triangle(0,2,4);
  VIF[8]=triangle(0,1,2);
  VIF[9]=square(1,2,6,7);
  VIF[10]=square(2,4,7,9);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);
  p.set_description() << "Johnson solid J9: Elongated pentagonal pyramid" << endl;

  return p;
}

//FIXME: coordinates #830
perl::Object gyroelongated_square_pyramid()
{
   perl::Object p = square_pyramid();
   p = gyroelongate(p,sequence(0,4));

   IncidenceMatrix<> VIF(13,9);
   VIF[0] = triangle(1,3,4);
   VIF[1] = triangle(2,3,8);
   VIF[2] = triangle(2,3,4);
   VIF[3] = triangle(2,7,8);
   VIF[4] = triangle(0,2,7);
   VIF[5] = triangle(0,5,7);
   VIF[6] = triangle(0,2,4);
   VIF[7] = triangle(0,1,5);
   VIF[8] = triangle(0,1,4);
   VIF[9] = triangle(1,5,6);
   VIF[10] = triangle(3,6,8);
   VIF[11] = triangle(1,3,6);
   VIF[12] = square(5,6,7,8);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J10: Gyroelongated square pyramid" << endl;

   return p;
}

perl::Object gyroelongated_pentagonal_pyramid()
{
   perl::Object ico = CallPolymakeFunction("icosahedron");
   Matrix<QE> V = ico.give("VERTICES");
   V = V.minor(sequence(0,11),All);

   perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
   p.take("VERTICES") << V;
   p = centralize<QE>(p);
   p.set_description() << "Johnson solid J11: Gyroelongated pentagonal pyramid" << endl;

   return p;
}

perl::Object triangular_bipyramid()
{
  Rational c(-1,3);

  Matrix<Rational> V( ones_vector<Rational>(5) | unit_matrix<Rational>(3) / ones_vector<Rational>(3) / same_element_vector<Rational>(c,3));

  perl::Object p(perl::ObjectType::construct<Rational>("Polytope"));

  p.take("VERTICES") << V;
  p = centralize<Rational>(p);
  p.set_description() << "Johnson solid J12: Triangular bipyramid" << endl;

  return p;
}

//FIXME: coordinates #830
perl::Object pentagonal_bipyramid()
{
  perl::Object p = pentagonal_pyramid();
  p = augment(p,sequence(1,5));

  IncidenceMatrix<> VIF(10,7);
  VIF[0] = triangle(0,4,5);
  VIF[1] = triangle(4,5,6);
  VIF[2] = triangle(3,5,6);
  VIF[3] = triangle(1,3,6);
  VIF[4] = triangle(0,1,3);
  VIF[5] = triangle(0,3,5);
  VIF[6] = triangle(0,1,2);
  VIF[7] = triangle(1,2,6);
  VIF[8] = triangle(2,4,6);
  VIF[9] = triangle(0,2,4);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  p.set_description() << "Johnson solid J13: Pentagonal bipyramid" << endl;
  return p;

}

perl::Object elongated_triangular_bipyramid()
{
  QE c(Rational(-1,3),0,0);

  QE s(0,Rational(1,3),6);

  Matrix<QE> V( ones_vector<QE>(8) | ( same_element_vector<QE>(1+s,3) / same_element_vector<QE>(c,3) / unit_matrix<QE>(3) / (unit_matrix<QE>(3) + same_element_matrix<QE>(s , 3, 3))));

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);
  p.set_description() << "Johnson solid J14: Elongated triangular bipyramid" << endl;

  return p;
}

perl::Object elongated_square_bipyramid()
{
  perl::Object esp = elongated_square_pyramid_impl(false);

  Matrix<QE> esp_vertices =  esp.give("VERTICES");

  Vector<QE> tip(4);
  tip[0]=1;
  tip[1]=tip[2]=0;
  tip[3]=QE(-2,-1,2);

  Matrix<QE> V = ( esp_vertices / tip);

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);
  p.set_description() << "Johnson solid J15: Elongated square bipyramid" << endl;

  return p;

}

//FIXME: coordinates #830
perl::Object elongated_pentagonal_bipyramid()
{
  perl::Object p = elongated_pentagonal_pyramid();

  p = augment(p,sequence(6,5));

  IncidenceMatrix<> VIF(15,12);
  VIF[0] = triangle(7,9,11);
  VIF[1] = triangle(6,7,11);
  VIF[2] = triangle(9,10,11);
  VIF[3] = square(1,3,6,8);
  VIF[4] = square(3,5,8,10);
  VIF[5] = triangle(8,10,11);
  VIF[6] = triangle(6,8,11);
  VIF[7] = triangle(0,3,5);
  VIF[8] = triangle(0,1,3);
  VIF[9] = triangle(0,4,5);
  VIF[10] = square(4,5,9,10);
  VIF[11] = triangle(0,2,4);
  VIF[12] = triangle(0,1,2);
  VIF[13] = square(1,2,6,7);
  VIF[14] = square(2,4,7,9);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<QE>(p);

  p.set_description() << "Johnson solid J16: Elongated pentagonal bipyramid" << endl;
  return p;

}

//FIXME: coordinates #830
perl::Object gyroelongated_square_bipyramid()
{
   perl::Object p = gyroelongated_square_pyramid();
   p = augment(p,sequence(5,4));

   IncidenceMatrix<> VIF(16,10);
   VIF[0] = triangle(1,3,4);
   VIF[1] = triangle(2,3,8);
   VIF[2] = triangle(2,3,4);
   VIF[3] = triangle(7,8,9);
   VIF[4] = triangle(2,7,8);
   VIF[5] = triangle(5,7,9);
   VIF[6] = triangle(0,5,7);
   VIF[7] = triangle(0,2,7);
   VIF[8] = triangle(0,2,4);
   VIF[9] = triangle(0,1,5);
   VIF[10] = triangle(0,1,4);
   VIF[11] = triangle(5,6,9);
   VIF[12] = triangle(1,5,6);
   VIF[13] = triangle(6,8,9);
   VIF[14] = triangle(3,6,8);
   VIF[15] = triangle(1,3,6);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J17: Gyroelongated square bipyramid" << endl;

   return p;
}

//FIXME: coordinates #830
perl::Object elongated_triangular_cupola()
{
   perl::Object p = triangular_cupola();
   p = elongate(p,sequence(3,6));

   IncidenceMatrix<> VIF(14,15);
   VIF[0] = square(1,2,6,8);
   VIF[1] = triangle(1,5,6);
   VIF[2] = square(5,6,11,12);
   VIF[3] = square(4,7,10,13);
   VIF[4] = square(7,8,13,14);
   VIF[5] = square(6,8,12,14);
   VIF[6] = triangle(2,7,8);
   VIF[7] = hexagon(9,10,11,12,13,14);
   VIF[8] = square(3,4,9,10);
   VIF[9] = square(3,5,9,11);
   VIF[10] = triangle(0,3,4);
   VIF[11] = triangle(0,1,2);
   VIF[12] = square(0,2,4,7);
   VIF[13] = square(0,1,3,5);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<QE>(p);
   p.set_description() << "Johnson solid J18: Elongated triangular cupola" << endl;

   return p;
}


perl::Object elongated_square_cupola_impl(bool centered)
{
  perl::Object base = exact_octagonal_prism(QE(-2,0,0),QE(0,0,0));
  Matrix<QE> V =  base.give("VERTICES");
  Matrix<QE> T = square_cupola_impl(false).give("VERTICES");
  V /= T.minor(sequence(8,4),All);

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  if (centered)
     p = centralize<QE>(p);
  p.set_description() << "Johnson solid J19: Elongated square cupola" << endl;

  return p;
}

perl::Object elongated_square_cupola()
{
   return elongated_square_cupola_impl(true);
}

//FIXME: coordinates #830
perl::Object elongated_pentagonal_cupola()
{
   perl::Object p = pentagonal_cupola();
   p = elongate(p,decagon(2,4,5,7,8,10,11,12,13,14));

   IncidenceMatrix<> VIF(22,25);
   VIF[0] = decagon(15,16,17,18,19,20,21,22,23,24);
   VIF[1] = square(0,1,2,4);
   VIF[2] = square(1,6,8,10);
   VIF[3] = triangle(1,4,8);
   VIF[4] = square(8,10,19,20);
   VIF[5] = square(10,13,20,23);
   VIF[6] = triangle(6,10,13);
   VIF[7] = square(13,14,23,24);
   VIF[8] = square(4,8,16,19);
   VIF[9] = square(6,9,13,14);
   VIF[10] = triangle(9,12,14);
   VIF[11] = square(12,14,22,24);
   VIF[12] = square(2,4,15,16);
   VIF[13] = pentagon(0,1,3,6,9);
   VIF[14] = square(3,9,11,12);
   VIF[15] = square(11,12,21,22);
   VIF[16] = triangle(0,2,5);
   VIF[17] = square(2,5,15,17);
   VIF[18] = triangle(3,7,11);
   VIF[19] = square(0,3,5,7);
   VIF[20] = square(7,11,18,21);
   VIF[21] = square(5,7,17,18);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<QE>(p);
   p.set_description() << "Johnson solid J20: Elongated pentagonal cupola" << endl;

  return p;
}

//FIXME: coordinates #830
perl::Object elongated_pentagonal_rotunda()
{
   perl::Object p = pentagonal_rotunda();
   p = elongate(p,decagon(7,9,10,12,13,15,16,17,18,19));

   IncidenceMatrix<> VIF(27,30);
   VIF[0] = decagon(20,21,22,23,24,25,26,27,28,29);
   VIF[1] = square(7,10,20,22);
   VIF[2] = triangle(3,7,10);
   VIF[3] = square(16,17,26,27);
   VIF[4] = pentagon(0,1,2,4,6);
   VIF[5] = triangle(0,1,3);
   VIF[6] = triangle(4,6,14);
   VIF[7] = pentagon(6,11,14,18,19);
   VIF[8] = square(9,13,21,24);
   VIF[9] = triangle(5,9,13);
   VIF[10] = square(18,19,28,29);
   VIF[11] = pentagon(2,5,11,13,15);
   VIF[12] = triangle(11,15,18);
   VIF[13] = square(15,18,25,28);
   VIF[14] = square(13,15,24,25);
   VIF[15] = triangle(2,6,11);
   VIF[16] = triangle(0,2,5);
   VIF[17] = triangle(14,17,19);
   VIF[18] = square(17,19,27,29);
   VIF[19] = square(7,9,20,21);
   VIF[20] = pentagon(0,3,5,7,9);
   VIF[21] = triangle(1,4,8);
   VIF[22] = pentagon(4,8,14,16,17);
   VIF[23] = triangle(8,12,16);
   VIF[24] = pentagon(1,3,8,10,12);
   VIF[25] = square(12,16,23,26);
   VIF[26] = square(10,12,22,23);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J21: Elongated pentagonal rotunda" << endl;

   return p;
}

//FIXME: coordinates #830
perl::Object gyroelongated_triangular_cupola()
{
  perl::Object p = triangular_cupola();

  p = gyroelongate(p,sequence(3,6));

  IncidenceMatrix<> VIF(20,15);
  VIF[0] = square(1,2,6,8);
  VIF[1] = triangle(1,5,6);
  VIF[2] = triangle(5,11,12);
  VIF[3] = triangle(5,6,12);
  VIF[4] = triangle(2,7,8);
  VIF[5] = triangle(6,12,14);
  VIF[6] = triangle(6,8,14);
  VIF[7] = triangle(8,13,14);
  VIF[8] = triangle(7,8,13);
  VIF[9] = triangle(7,10,13);
  VIF[10] = triangle(4,7,10);
  VIF[11] = triangle(4,9,10);
  VIF[12] = hexagon(9,10,11,12,13,14);
  VIF[13] = triangle(3,4,9);
  VIF[14] = triangle(3,9,11);
  VIF[15] = triangle(3,5,11);
  VIF[16] = triangle(0,3,4);
  VIF[17] = triangle(0,1,2);
  VIF[18] = square(0,2,4,7);
  VIF[19] = square(0,1,3,5);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<QE>(p);

  p.set_description() << "Johnson solid J22: Gyroelongated triangular cupola" << endl;
  return p;
}

//FIXME: coordinates #830
perl::Object gyroelongated_square_cupola()
{
  Matrix<double> V = square_cupola_impl(false).give("VERTICES");
  const double height = -2*sqrt(1-1.0/(4.0*cos(M_PI/16)*cos(M_PI/16)));
  Matrix<double> W = create_regular_polygon_vertices(8, sqrt(2)*sqrt(2+sqrt(2)), 0);

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  Matrix<double> X = (W.minor(All, sequence(0,3)) | same_element_vector<double>(height,8)) / V;

  p.set_description() << "Johnson solid J23: Gyroelongated square cupola" << endl;
  IncidenceMatrix<> VIF(26,20);
  VIF[0] = triangle(0,1,9);
  VIF[1] = triangle(1,2,8);
  VIF[2] = triangle(2,3,15);
  VIF[3] = triangle(3,4,14);
  VIF[4] = triangle(4,5,13);
  VIF[5] = triangle(5,6,12);
  VIF[6] = triangle(6,7,11);
  VIF[7] = triangle(0,7,10);
  VIF[8] = triangle(1,8,9);
  VIF[9] = triangle(0,9,10);
  VIF[10] = triangle(7,10,11);
  VIF[11] = triangle(6,11,12);
  VIF[12] = triangle(5,12,13);
  VIF[13] = triangle(4,13,14);
  VIF[14] = triangle(3,14,15);
  VIF[15] = triangle(2,8,15);
  VIF[16] = octagon(0,1,2,3,4,5,6,7);
  VIF[17] = square(9,10,16,17);
  VIF[18] = triangle(8,9,16);
  VIF[19] = square(13,14,18,19);
  VIF[20] = square(8,15,16,18);
  VIF[21] = triangle(10,11,17);
  VIF[22] = square(11,12,17,19);
  VIF[23] = triangle(14,15,18);
  VIF[24] = triangle(12,13,19);
  VIF[25] = square(16,17,18,19);

  p.take("VERTICES") << X;
  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object gyroelongated_pentagonal_cupola()
{
  perl::Object p = pentagonal_cupola();
  p = gyroelongate(p,decagon(2,4,5,7,8,10,11,12,13,14));

  IncidenceMatrix<> VIF(32,25);
  VIF[0] = decagon(15,16,17,18,19,20,21,22,23,24);
  VIF[1] = triangle(5,17,18);
  VIF[2] = triangle(2,5,17);
  VIF[3] = triangle(0,2,5);
  VIF[4] = triangle(2,15,17);
  VIF[5] = pentagon(0,1,3,6,9);
  VIF[6] = triangle(2,4,15);
  VIF[7] = triangle(4,15,16);
  VIF[8] = triangle(14,23,24);
  VIF[9] = triangle(4,8,16);
  VIF[10] = triangle(6,10,13);
  VIF[11] = triangle(13,20,23);
  VIF[12] = triangle(10,19,20);
  VIF[13] = triangle(10,13,20);
  VIF[14] = triangle(8,10,19);
  VIF[15] = triangle(8,16,19);
  VIF[16] = triangle(13,14,23);
  VIF[17] = triangle(1,4,8);
  VIF[18] = square(1,6,8,10);
  VIF[19] = square(6,9,13,14);
  VIF[20] = triangle(12,14,24);
  VIF[21] = triangle(9,12,14);
  VIF[22] = square(0,1,2,4);
  VIF[23] = triangle(12,22,24);
  VIF[24] = triangle(11,12,22);
  VIF[25] = square(3,9,11,12);
  VIF[26] = triangle(11,21,22);
  VIF[27] = triangle(3,7,11);
  VIF[28] = triangle(7,11,21);
  VIF[29] = triangle(5,7,18);
  VIF[30] = triangle(7,18,21);
  VIF[31] = square(0,3,5,7);
  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);
  p.set_description() << "Johnson solid J24: Gyroelongated pentagonal cupola" << endl;

  return p;
}

//FIXME: coordinates #830
perl::Object gyroelongated_pentagonal_rotunda()
{
   perl::Object p = pentagonal_rotunda();
   p = gyroelongate(p,decagon(7,9,10,12,13,15,16,17,18,19));

   IncidenceMatrix<> VIF(37,30);
   VIF[0] = decagon(20,21,22,23,24,25,26,27,28,29);
   VIF[1] = triangle(10,22,23);
   VIF[2] = pentagon(4,8,14,16,17);
   VIF[3] = triangle(1,4,8);
   VIF[4] = triangle(16,17,27);
   VIF[5] = triangle(17,27,29);
   VIF[6] = pentagon(0,3,5,7,9);
   VIF[7] = triangle(7,9,20);
   VIF[8] = triangle(0,2,5);
   VIF[9] = pentagon(6,11,14,18,19);
   VIF[10] = triangle(2,6,11);
   VIF[11] = triangle(18,19,28);
   VIF[12] = triangle(18,25,28);
   VIF[13] = triangle(13,15,24);
   VIF[14] = triangle(15,18,25);
   VIF[15] = triangle(15,24,25);
   VIF[16] = triangle(11,15,18);
   VIF[17] = triangle(13,21,24);
   VIF[18] = pentagon(2,5,11,13,15);
   VIF[19] = triangle(9,13,21);
   VIF[20] = triangle(5,9,13);
   VIF[21] = triangle(19,28,29);
   VIF[22] = triangle(9,20,21);
   VIF[23] = triangle(17,19,29);
   VIF[24] = triangle(14,17,19);
   VIF[25] = triangle(4,6,14);
   VIF[26] = triangle(0,1,3);
   VIF[27] = pentagon(0,1,2,4,6);
   VIF[28] = triangle(7,20,22);
   VIF[29] = triangle(3,7,10);
   VIF[30] = triangle(7,10,22);
   VIF[31] = triangle(16,26,27);
   VIF[32] = triangle(8,12,16);
   VIF[33] = triangle(12,16,26);
   VIF[34] = triangle(10,12,23);
   VIF[35] = triangle(12,23,26);
   VIF[36] = pentagon(1,3,8,10,12);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<QE>(p);
   p.set_description() << "Johnson solid J25: Gyroelongated pentagonal rotunda" << endl;

   return p;
}

perl::Object gyrobifastigium(){

  QE height(0,1,3);
  Vector<QE> h(2);
  h[0]=height;
  h[1]=-height;

  Matrix<QE> V = ((create_square_vertices<QE>() | zero_vector<QE>(4)) / (same_element_vector<QE>(QE(1,0,0),4) | ((unit_matrix<QE>(2) | h) / (-unit_matrix<QE>(2) | h))));

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
        p.take("VERTICES") << V;
        p = centralize<QE>(p);
        p.set_description() << "Johnson solid J26: Gyrobifastigium" << endl;

  return p;
}

perl::Object triangular_orthobicupola()
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

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);
  p.set_description() << "Johnson solid J27: Triangular orthobicupola" << endl;

  return p;
}

perl::Object square_orthobicupola()
{
  Matrix<QE> V = square_cupola_impl(false).give("VERTICES");
  V /= (ones_vector<QE>(4) | (-1)*V.minor(sequence(8,4),sequence(1,3)));

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);
  p.set_description() << "Johnson solid J28: Square orthobicupola" << endl;

  return p;
}

perl::Object square_gyrobicupola()
{
  QE rot(0,Rational(1,2),2);
  Matrix<QE> R(3,3);
  R(0,0)=R(1,0)=R(1,1)=rot;
  R(0,1)=-rot;
  R(2,2)=1;
  Matrix<QE> V = square_cupola_impl(false).give("VERTICES");
  V /= (ones_vector<QE>(4) | (-1)*(V.minor(sequence(8,4),sequence(1,3))*R));

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);
  p.set_description() << "Johnson solid J29: Square gyrobicupola" << endl;

  return p;
}

perl::Object pentagonal_orthobicupola() {
   perl::Object p = pentagonal_cupola();
   p = augment(p,decagon(2,4,5,7,8,10,11,12,13,14));
   p = rotate_facet(p,pentagon(0,1,3,6,9),M_PI/5);

   IncidenceMatrix<> VIF(22,20);
   VIF[0] = square(0,2,10,14);
   VIF[1] = square(0,2,15,16);
   VIF[2] = pentagon(15,16,17,18,19);
   VIF[3] = triangle(0,1,16);
   VIF[4] = triangle(0,1,14);
   VIF[5] = square(7,9,11,12);
   VIF[6] = square(1,4,16,18);
   VIF[7] = triangle(8,9,12);
   VIF[8] = triangle(4,5,18);
   VIF[9] = triangle(4,5,13);
   VIF[10] = square(5,8,12,13);
   VIF[11] = triangle(8,9,19);
   VIF[12] = square(5,8,18,19);
   VIF[13] = square(1,4,13,14);
   VIF[14] = square(7,9,17,19);
   VIF[15] = pentagon(10,11,12,13,14);
   VIF[16] = triangle(6,7,11);
   VIF[17] = triangle(6,7,17);
   VIF[18] = triangle(2,3,10);
   VIF[19] = triangle(2,3,15);
   VIF[20] = square(3,6,15,17);
   VIF[21] = square(3,6,10,11);

   p.take("VERTICES_IN_FACETS")<<VIF;
   p = centralize<double>(p);

   p.set_description() << "Johnson solid J30: Pentagonal orthobicupola" << endl;
   return p;
}

perl::Object pentagonal_gyrobicupola(){
   perl::Object p = pentagonal_pyramid();
   p = CallPolymakeFunction("minkowski_sum",1,p,-1,p);

   p.set_description() << "Johnson solid J31: Pentagonal gyrobicupola" << endl;

   return p;
}

//FIXME: coordinates #830
perl::Object pentagonal_orthocupolarotunda()
{
   perl::Object p = pentagonal_rotunda();
   p = augment(p,decagon(7,9,10,12,13,15,16,17,18,19));

   IncidenceMatrix<> VIF(27,25);
   VIF[0] = pentagon(4,8,14,16,17);
   VIF[1] = triangle(1,4,8);
   VIF[2] = triangle(16,17,24);
   VIF[3] = pentagon(0,3,5,7,9);
   VIF[4] = triangle(4,6,14);
   VIF[5] = square(17,19,20,24);
   VIF[6] = triangle(14,17,19);
   VIF[7] = triangle(0,2,5);
   VIF[8] = square(9,13,21,22);
   VIF[9] = triangle(5,9,13);
   VIF[10] = square(15,18,20,21);
   VIF[11] = triangle(11,15,18);
   VIF[12] = triangle(13,15,21);
   VIF[13] = pentagon(2,5,11,13,15);
   VIF[14] = triangle(18,19,20);
   VIF[15] = triangle(2,6,11);
   VIF[16] = pentagon(6,11,14,18,19);
   VIF[17] = triangle(7,9,22);
   VIF[18] = pentagon(20,21,22,23,24);
   VIF[19] = triangle(0,1,3);
   VIF[20] = pentagon(0,1,2,4,6);
   VIF[21] = triangle(3,7,10);
   VIF[22] = square(7,10,22,23);
   VIF[23] = triangle(10,12,23);
   VIF[24] = triangle(8,12,16);
   VIF[25] = square(12,16,23,24);
   VIF[26] = pentagon(1,3,8,10,12);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J32: Pentagonal orthocupolarotunda" << endl;

   return p;
}

//FIXME: coordinates #830
perl::Object pentagonal_gyrocupolarotunda()
{
   perl::Object p = pentagonal_orthocupolarotunda();
   p = rotate_facet(p,sequence(20,5),M_PI/5);

   IncidenceMatrix<> VIF(27,25);
   VIF[0] = pentagon(4,8,14,16,17);
   VIF[1] = triangle(1,4,8);
   VIF[2] = square(16,17,20,24);
   VIF[3] = pentagon(0,3,5,7,9);
   VIF[4] = triangle(4,6,14);
   VIF[5] = triangle(0,2,5);
   VIF[6] = square(18,19,20,21);
   VIF[7] = triangle(9,13,22);
   VIF[8] = triangle(5,9,13);
   VIF[9] = triangle(11,15,18);
   VIF[10] = triangle(15,18,21);
   VIF[11] = pentagon(2,5,11,13,15);
   VIF[12] = square(13,15,21,22);
   VIF[13] = triangle(2,6,11);
   VIF[14] = pentagon(6,11,14,18,19);
   VIF[15] = triangle(17,19,20);
   VIF[16] = triangle(14,17,19);
   VIF[17] = square(7,9,22,23);
   VIF[18] = pentagon(20,21,22,23,24);
   VIF[19] = triangle(0,1,3);
   VIF[20] = pentagon(0,1,2,4,6);
   VIF[21] = triangle(7,10,23);
   VIF[22] = triangle(3,7,10);
   VIF[23] = triangle(12,16,24);
   VIF[24] = triangle(8,12,16);
   VIF[25] = square(10,12,23,24);
   VIF[26] = pentagon(1,3,8,10,12);

   p.take("VERTICES_IN_FACETS")<<VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J33: Pentagonal gyrocupolarotunda" << endl;

   return p;
}

//FIXME: coordinates #830
perl::Object pentagonal_orthobirotunda()
{
   perl::Object p = pentagonal_rotunda();
   Vector<double> normal(4);
   normal[2]=(1+sqrt(5))/2;
   normal[3]=1;
   p = gyrotunda(p,decagon(7,9,10,12,13,15,16,17,18,19));

   IncidenceMatrix<> VIF(32,30);
   VIF[0] = pentagon(4,8,14,16,17);
   VIF[1] = triangle(1,4,8);
   VIF[2] = pentagon(16,17,23,24,29);
   VIF[3] = pentagon(0,1,2,4,6);
   VIF[4] = triangle(0,1,3);
   VIF[5] = pentagon(7,9,21,22,27);
   VIF[6] = triangle(22,27,28);
   VIF[7] = triangle(4,6,14);
   VIF[8] = triangle(14,17,19);
   VIF[9] = triangle(17,19,24);
   VIF[10] = triangle(0,2,5);
   VIF[11] = triangle(21,26,27);
   VIF[12] = triangle(2,6,11);
   VIF[13] = pentagon(2,5,11,13,15);
   VIF[14] = pentagon(13,15,20,21,26);
   VIF[15] = triangle(11,15,18);
   VIF[16] = triangle(15,18,20);
   VIF[17] = triangle(20,25,26);
   VIF[18] = triangle(9,13,21);
   VIF[19] = triangle(5,9,13);
   VIF[20] = pentagon(18,19,20,24,25);
   VIF[21] = pentagon(6,11,14,18,19);
   VIF[22] = triangle(24,25,29);
   VIF[23] = pentagon(25,26,27,28,29);
   VIF[24] = pentagon(0,3,5,7,9);
   VIF[25] = triangle(23,28,29);
   VIF[26] = triangle(7,10,22);
   VIF[27] = triangle(3,7,10);
   VIF[28] = triangle(12,16,23);
   VIF[29] = triangle(8,12,16);
   VIF[30] = pentagon(10,12,22,23,28);
   VIF[31] = pentagon(1,3,8,10,12);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J34: Pentagonal orthobirotunda" << endl;

   return p;
}

//FIXME: coordinates #830
perl::Object elongated_triangular_orthobicupola()
{
   perl::Object p = elongated_triangular_cupola();
   p = augment(p,hexagon(9,10,11,12,13,14));
   p = rotate_facet(p,triangle(15,16,17),M_PI/3);

   IncidenceMatrix<> VIF(20,18);
   VIF[0] = square(1,2,6,8);
   VIF[1] = triangle(1,5,6);
   VIF[2] = square(5,6,11,12);
   VIF[3] = triangle(11,12,15);
   VIF[4] = square(4,7,10,13);
   VIF[5] = square(10,13,16,17);
   VIF[6] = square(7,8,13,14);
   VIF[7] = triangle(15,16,17);
   VIF[8] = triangle(13,14,16);
   VIF[9] = square(12,14,15,16);
   VIF[10] = square(6,8,12,14);
   VIF[11] = triangle(2,7,8);
   VIF[12] = triangle(9,10,17);
   VIF[13] = square(9,11,15,17);
   VIF[14] = square(3,4,9,10);
   VIF[15] = square(3,5,9,11);
   VIF[16] = triangle(0,3,4);
   VIF[17] = triangle(0,1,2);
   VIF[18] = square(0,2,4,7);
   VIF[19] = square(0,1,3,5);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J35: Elongated triangular orthobicupola" << endl;

   return p;
}

//FIXME: coordinates #830
perl::Object elongated_triangular_gyrobicupola()
{
   perl::Object p = elongated_triangular_cupola();
   p = augment(p,sequence(9,6));

   IncidenceMatrix<> VIF(20,18);
   VIF[0] = square(1,2,6,8);
   VIF[1] = triangle(1,5,6);
   VIF[2] = square(5,6,11,12);
   VIF[3] = square(11,12,15,17);
   VIF[4] = square(4,7,10,13);
   VIF[5] = triangle(10,13,16);
   VIF[6] = triangle(15,16,17);
   VIF[7] = square(13,14,15,16);
   VIF[8] = square(7,8,13,14);
   VIF[9] = triangle(12,14,15);
   VIF[10] = square(6,8,12,14);
   VIF[11] = triangle(2,7,8);
   VIF[12] = triangle(9,11,17);
   VIF[13] = square(9,10,16,17);
   VIF[14] = square(3,4,9,10);
   VIF[15] = square(3,5,9,11);
   VIF[16] = triangle(0,3,4);
   VIF[17] = triangle(0,1,2);
   VIF[18] = square(0,2,4,7);
   VIF[19] = square(0,1,3,5);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J36: Elongated triangular gyrobicupola" << endl;

  return p;
}

perl::Object elongated_square_gyrobicupola()
{
  Matrix<QE> V = elongated_square_cupola_impl(false).give("VERTICES");
  Matrix<QE> W = square_gyrobicupola().give("VERTICES");
  V /= W.minor(sequence(12,4),All);
  V(20,3)=V(21,3)=V(22,3)=V(23,3)= V(20,3)-2;

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);
  p.set_description() << "Johnson solid J37: Elongated square gyrobicupola" << endl;

  return p;
}

//FIXME: coordinates #830
perl::Object elongated_pentagonal_orthobicupola()
{
   perl::Object p = elongated_pentagonal_cupola();
   p = augment(p,sequence(15,10));
   p = rotate_facet(p,sequence(25,5),M_PI/5);

   IncidenceMatrix<> VIF(32,30);
   VIF[0] = square(17,18,27,28);
   VIF[1] = triangle(18,21,28);
   VIF[2] = triangle(15,17,27);
   VIF[3] = square(21,22,28,29);
   VIF[4] = pentagon(25,26,27,28,29);
   VIF[5] = square(2,4,15,16);
   VIF[6] = square(12,14,22,24);
   VIF[7] = triangle(9,12,14);
   VIF[8] = triangle(22,24,29);
   VIF[9] = square(23,24,25,29);
   VIF[10] = square(4,8,16,19);
   VIF[11] = square(13,14,23,24);
   VIF[12] = triangle(20,23,25);
   VIF[13] = square(10,13,20,23);
   VIF[14] = square(8,10,19,20);
   VIF[15] = triangle(6,10,13);
   VIF[16] = square(19,20,25,26);
   VIF[17] = triangle(16,19,26);
   VIF[18] = triangle(1,4,8);
   VIF[19] = square(1,6,8,10);
   VIF[20] = square(6,9,13,14);
   VIF[21] = square(15,16,26,27);
   VIF[22] = square(0,1,2,4);
   VIF[23] = pentagon(0,1,3,6,9);
   VIF[24] = square(3,9,11,12);
   VIF[25] = square(11,12,21,22);
   VIF[26] = triangle(0,2,5);
   VIF[27] = square(2,5,15,17);
   VIF[28] = triangle(3,7,11);
   VIF[29] = square(0,3,5,7);
   VIF[30] = square(7,11,18,21);
   VIF[31] = square(5,7,17,18);

    p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J38: Elongated pentagonal orthobicupola" << endl;

   return p;
}

//FIXME: coordinates #830
perl::Object elongated_pentagonal_gyrobicupola()
{
   perl::Object p = elongated_pentagonal_cupola();
   p = augment(p,sequence(15,10));

   IncidenceMatrix<> VIF(32,30);
   VIF[0] = square(18,21,27,28);
   VIF[1] = triangle(17,18,27);
   VIF[2] = square(15,17,26,27);
   VIF[3] = triangle(21,22,28);
   VIF[4] = pentagon(25,26,27,28,29);
   VIF[5] = square(2,4,15,16);
   VIF[6] = square(12,14,22,24);
   VIF[7] = triangle(9,12,14);
   VIF[8] = square(16,19,25,26);
   VIF[9] = square(4,8,16,19);
   VIF[10] = square(13,14,23,24);
   VIF[11] = square(20,23,25,29);
   VIF[12] = triangle(19,20,25);
   VIF[13] = square(10,13,20,23);
   VIF[14] = square(8,10,19,20);
   VIF[15] = triangle(6,10,13);
   VIF[16] = triangle(23,24,29);
   VIF[17] = triangle(1,4,8);
   VIF[18] = square(1,6,8,10);
   VIF[19] = square(6,9,13,14);
   VIF[20] = square(22,24,28,29);
   VIF[21] = triangle(15,16,26);
   VIF[22] = square(0,1,2,4);
   VIF[23] = pentagon(0,1,3,6,9);
   VIF[24] = square(3,9,11,12);
   VIF[25] = square(11,12,21,22);
   VIF[26] = triangle(0,2,5);
   VIF[27] = square(2,5,15,17);
   VIF[28] = triangle(3,7,11);
   VIF[29] = square(0,3,5,7);
   VIF[30] = square(7,11,18,21);
   VIF[31] = square(5,7,17,18);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J39: Elongated pentagonal gyrobicupola" << endl;

   return p;
}

//FIXME: coordinates #830
perl::Object elongated_pentagonal_orthocupolarotunda()
{
   perl::Object p = elongated_pentagonal_rotunda();
   p = augment(p,sequence(20,10));
   p = rotate_facet(p,sequence(30,5),M_PI/5);

   IncidenceMatrix<> VIF(37,35);
   VIF[0] = square(23,26,31,32);
   VIF[1] = triangle(22,23,31);
   VIF[2] = square(7,10,20,22);
   VIF[3] = triangle(3,7,10);
   VIF[4] = square(16,17,26,27);
   VIF[5] = pentagon(0,1,2,4,6);
   VIF[6] = triangle(0,1,3);
   VIF[7] = pentagon(30,31,32,33,34);
   VIF[8] = square(7,9,20,21);
   VIF[9] = square(17,19,27,29);
   VIF[10] = triangle(14,17,19);
   VIF[11] = pentagon(6,11,14,18,19);
   VIF[12] = triangle(2,6,11);
   VIF[13] = triangle(28,29,33);
   VIF[14] = pentagon(2,5,11,13,15);
   VIF[15] = triangle(24,25,34);
   VIF[16] = square(13,15,24,25);
   VIF[17] = square(15,18,25,28);
   VIF[18] = triangle(11,15,18);
   VIF[19] = square(25,28,33,34);
   VIF[20] = square(18,19,28,29);
   VIF[21] = triangle(5,9,13);
   VIF[22] = square(9,13,21,24);
   VIF[23] = square(21,24,30,34);
   VIF[24] = triangle(0,2,5);
   VIF[25] = square(27,29,32,33);
   VIF[26] = triangle(20,21,30);
   VIF[27] = triangle(4,6,14);
   VIF[28] = pentagon(0,3,5,7,9);
   VIF[29] = triangle(26,27,32);
   VIF[30] = square(20,22,30,31);
   VIF[31] = triangle(1,4,8);
   VIF[32] = pentagon(4,8,14,16,17);
   VIF[33] = triangle(8,12,16);
   VIF[34] = pentagon(1,3,8,10,12);
   VIF[35] = square(12,16,23,26);
   VIF[36] = square(10,12,22,23);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<QE>(p);
   p.set_description() << "Johnson solid J40: Elongated pentagonal orthocupolarotunda" << endl;

   return p;
}

      //FIXME: coordinates #830
perl::Object elongated_pentagonal_gyrocupolarotunda()
{
   perl::Object p = elongated_pentagonal_rotunda();
   p = augment(p,sequence(20,10));

   IncidenceMatrix<> VIF(37,35);
   VIF[0] = square(22,23,30,31);
   VIF[1] = triangle(23,26,31);
   VIF[2] = square(7,10,20,22);
   VIF[3] = triangle(3,7,10);
   VIF[4] = square(16,17,26,27);
   VIF[5] = pentagon(0,1,2,4,6);
   VIF[6] = triangle(0,1,3);
   VIF[7] = pentagon(30,31,32,33,34);
   VIF[8] = square(7,9,20,21);
   VIF[9] = square(17,19,27,29);
   VIF[10] = triangle(14,17,19);
   VIF[11] = triangle(27,29,32);
   VIF[12] = pentagon(6,11,14,18,19);
   VIF[13] = triangle(2,6,11);
   VIF[14] = triangle(21,24,34);
   VIF[15] = square(24,25,33,34);
   VIF[16] = pentagon(2,5,11,13,15);
   VIF[17] = triangle(25,28,33);
   VIF[18] = square(13,15,24,25);
   VIF[19] = square(15,18,25,28);
   VIF[20] = triangle(11,15,18);
   VIF[21] = square(18,19,28,29);
   VIF[22] = triangle(5,9,13);
   VIF[23] = square(9,13,21,24);
   VIF[24] = square(28,29,32,33);
   VIF[25] = triangle(0,2,5);
   VIF[26] = square(20,21,30,34);
   VIF[27] = triangle(4,6,14);
   VIF[28] = pentagon(0,3,5,7,9);
   VIF[29] = square(26,27,31,32);
   VIF[30] = triangle(20,22,30);
   VIF[31] = triangle(1,4,8);
   VIF[32] = pentagon(4,8,14,16,17);
   VIF[33] = triangle(8,12,16);
   VIF[34] = pentagon(1,3,8,10,12);
   VIF[35] = square(12,16,23,26);
   VIF[36] = square(10,12,22,23);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J41: Elongated pentagonal gyrocupolarotunda" << endl;

   return p;
}

//FIXME: coordinates #830
perl::Object elongated_pentagonal_orthobirotunda()
{
   perl::Object p = elongated_pentagonal_rotunda();
   p = rotunda(p,sequence(20,10));

   IncidenceMatrix<> VIF(42,40);
   VIF[0] = pentagon(22,23,30,31,35);
   VIF[1] = triangle(23,26,31);
   VIF[2] = square(7,10,20,22);
   VIF[3] = triangle(3,7,10);
   VIF[4] = square(16,17,26,27);
   VIF[5] = triangle(31,35,36);
   VIF[6] = pentagon(0,3,5,7,9);
   VIF[7] = pentagon(35,36,37,38,39);
   VIF[8] = square(7,9,20,21);
   VIF[9] = square(17,19,27,29);
   VIF[10] = triangle(14,17,19);
   VIF[11] = triangle(32,36,37);
   VIF[12] = pentagon(6,11,14,18,19);
   VIF[13] = pentagon(28,29,32,33,37);
   VIF[14] = square(9,13,21,24);
   VIF[15] = triangle(5,9,13);
   VIF[16] = square(18,19,28,29);
   VIF[17] = triangle(33,37,38);
   VIF[18] = square(13,15,24,25);
   VIF[19] = square(15,18,25,28);
   VIF[20] = triangle(25,28,33);
   VIF[21] = triangle(11,15,18);
   VIF[22] = pentagon(24,25,33,34,38);
   VIF[23] = pentagon(2,5,11,13,15);
   VIF[24] = triangle(21,24,34);
   VIF[25] = triangle(2,6,11);
   VIF[26] = triangle(34,38,39);
   VIF[27] = triangle(0,2,5);
   VIF[28] = triangle(27,29,32);
   VIF[29] = triangle(4,6,14);
   VIF[30] = triangle(30,35,39);
   VIF[31] = pentagon(20,21,30,34,39);
   VIF[32] = triangle(0,1,3);
   VIF[33] = pentagon(0,1,2,4,6);
   VIF[34] = pentagon(26,27,31,32,36);
   VIF[35] = triangle(20,22,30);
   VIF[36] = triangle(1,4,8);
   VIF[37] = pentagon(4,8,14,16,17);
   VIF[38] = triangle(8,12,16);
   VIF[39] = pentagon(1,3,8,10,12);
   VIF[40] = square(12,16,23,26);
   VIF[41] = square(10,12,22,23);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J42: Elongated pentagonal orthobirotunda" << endl;

   return p;
}

      //FIXME: coordinates #830
perl::Object elongated_pentagonal_gyrobirotunda()
{
   perl::Object p = elongated_pentagonal_rotunda();
   p = gyrotunda(p,sequence(20,10));

   IncidenceMatrix<> VIF(42,40);
   VIF[0] = pentagon(23,26,30,31,36);
   VIF[1] = triangle(22,23,30);
   VIF[2] = square(7,10,20,22);
   VIF[3] = triangle(3,7,10);
   VIF[4] = square(16,17,26,27);
   VIF[5] = triangle(30,35,36);
   VIF[6] = pentagon(0,3,5,7,9);
   VIF[7] = pentagon(35,36,37,38,39);
   VIF[8] = square(7,9,20,21);
   VIF[9] = square(17,19,27,29);
   VIF[10] = triangle(14,17,19);
   VIF[11] = triangle(34,35,39);
   VIF[12] = pentagon(6,11,14,18,19);
   VIF[13] = triangle(32,37,38);
   VIF[14] = square(9,13,21,24);
   VIF[15] = triangle(5,9,13);
   VIF[16] = square(18,19,28,29);
   VIF[17] = triangle(33,38,39);
   VIF[18] = square(13,15,24,25);
   VIF[19] = square(15,18,25,28);
   VIF[20] = triangle(24,25,33);
   VIF[21] = triangle(11,15,18);
   VIF[22] = pentagon(25,28,32,33,38);
   VIF[23] = pentagon(2,5,11,13,15);
   VIF[24] = triangle(28,29,32);
   VIF[25] = triangle(2,6,11);
   VIF[26] = pentagon(21,24,33,34,39);
   VIF[27] = triangle(0,2,5);
   VIF[28] = triangle(20,21,34);
   VIF[29] = triangle(4,6,14);
   VIF[30] = triangle(31,36,37);
   VIF[31] = pentagon(27,29,31,32,37);
   VIF[32] = triangle(0,1,3);
   VIF[33] = pentagon(0,1,2,4,6);
   VIF[34] = triangle(26,27,31);
   VIF[35] = pentagon(20,22,30,34,35);
   VIF[36] = triangle(1,4,8);
   VIF[37] = pentagon(4,8,14,16,17);
   VIF[38] = triangle(8,12,16);
   VIF[39] = pentagon(1,3,8,10,12);
   VIF[40] = square(12,16,23,26);
   VIF[41] = square(10,12,22,23);

   p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);
   p.set_description() << "Johnson solid J43: Elongated pentagonal gyrobirotunda" << endl;

  return p;
}

      //FIXME: coordinates #830
perl::Object gyroelongated_triangular_bicupola()
{
   perl::Object p = gyroelongated_triangular_cupola();
   p = augment(p,sequence(9,6));

   IncidenceMatrix<> VIF(26,18);
   VIF[0] = square(1,2,6,8);
   VIF[1] = triangle(1,5,6);
   VIF[2] = triangle(5,11,12);
   VIF[3] = triangle(5,6,12);
   VIF[4] = triangle(2,7,8);
   VIF[5] = triangle(6,12,14);
   VIF[6] = triangle(6,8,14);
   VIF[7] = square(12,14,15,17);
   VIF[8] = triangle(7,8,13);
   VIF[9] = triangle(7,10,13);
   VIF[10] = square(10,13,15,16);
   VIF[11] = triangle(13,14,15);
   VIF[12] = triangle(8,13,14);
   VIF[13] = triangle(15,16,17);
   VIF[14] = triangle(9,10,16);
   VIF[15] = triangle(4,7,10);
   VIF[16] = triangle(4,9,10);
   VIF[17] = triangle(11,12,17);
   VIF[18] = square(9,11,16,17);
   VIF[19] = triangle(3,4,9);
   VIF[20] = triangle(3,9,11);
   VIF[21] = triangle(3,5,11);
   VIF[22] = triangle(0,3,4);
   VIF[23] = triangle(0,1,2);
   VIF[24] = square(0,2,4,7);
   VIF[25] = square(0,1,3,5);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J44: Gyroelongated triangular bicupola" << endl;

   return p;
}

//FIXME: coordinates #830
perl::Object gyroelongated_square_bicupola()
{
   perl::Object p = gyroelongated_square_cupola();
   p = augment(p,sequence(0,8));

   IncidenceMatrix<> VIF(34,24);
   VIF[0] = square(9,10,16,17);
   VIF[1] = triangle(1,8,9);
   VIF[2] = triangle(8,9,16);
   VIF[3] = square(1,2,20,23);
   VIF[4] = triangle(1,2,8);
   VIF[5] = square(20,21,22,23);
   VIF[6] = square(16,17,18,19);
   VIF[7] = triangle(6,11,12);
   VIF[8] = triangle(2,3,23);
   VIF[9] = triangle(2,3,15);
   VIF[10] = square(3,4,22,23);
   VIF[11] = square(13,14,18,19);
   VIF[12] = triangle(5,12,13);
   VIF[13] = triangle(3,4,14);
   VIF[14] = triangle(4,5,13);
   VIF[15] = triangle(4,13,14);
   VIF[16] = triangle(4,5,22);
   VIF[17] = triangle(12,13,19);
   VIF[18] = triangle(3,14,15);
   VIF[19] = triangle(14,15,18);
   VIF[20] = triangle(5,6,12);
   VIF[21] = square(5,6,21,22);
   VIF[22] = square(11,12,17,19);
   VIF[23] = triangle(2,8,15);
   VIF[24] = square(8,15,16,18);
   VIF[25] = triangle(6,7,21);
   VIF[26] = triangle(6,7,11);
   VIF[27] = triangle(10,11,17);
   VIF[28] = triangle(7,10,11);
   VIF[29] = triangle(0,7,10);
   VIF[30] = triangle(0,1,20);
   VIF[31] = triangle(0,1,9);
   VIF[32] = triangle(0,9,10);
   VIF[33] = square(0,7,20,21);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J45: Gyroelongated square bicupola" << endl;

   return p;
}

      //FIXME: coordinates #830
perl::Object gyroelongated_pentagonal_bicupola()
{
   perl::Object p = gyroelongated_pentagonal_cupola();
   p = augment(p,sequence(15,10));

   IncidenceMatrix<> VIF(42,30);
   VIF[0] = square(18,21,25,26);
   VIF[1] = triangle(17,18,25);
   VIF[2] = triangle(5,17,18);
   VIF[3] = triangle(2,5,17);
   VIF[4] = triangle(0,2,5);
   VIF[5] = square(15,17,25,29);
   VIF[6] = triangle(2,15,17);
   VIF[7] = pentagon(0,1,3,6,9);
   VIF[8] = triangle(2,4,15);
   VIF[9] = triangle(15,16,29);
   VIF[10] = triangle(4,15,16);
   VIF[11] = triangle(23,24,27);
   VIF[12] = triangle(14,23,24);
   VIF[13] = square(20,23,27,28);
   VIF[14] = triangle(4,8,16);
   VIF[15] = triangle(6,10,13);
   VIF[16] = triangle(8,16,19);
   VIF[17] = triangle(8,10,19);
   VIF[18] = triangle(10,13,20);
   VIF[19] = triangle(10,19,20);
   VIF[20] = triangle(13,20,23);
   VIF[21] = triangle(19,20,28);
   VIF[22] = triangle(13,14,23);
   VIF[23] = triangle(1,4,8);
   VIF[24] = square(1,6,8,10);
   VIF[25] = square(16,19,28,29);
   VIF[26] = square(6,9,13,14);
   VIF[27] = triangle(12,14,24);
   VIF[28] = triangle(9,12,14);
   VIF[29] = square(0,1,2,4);
   VIF[30] = pentagon(25,26,27,28,29);
   VIF[31] = triangle(12,22,24);
   VIF[32] = square(22,24,26,27);
   VIF[33] = triangle(11,12,22);
   VIF[34] = square(3,9,11,12);
   VIF[35] = triangle(21,22,26);
   VIF[36] = triangle(11,21,22);
   VIF[37] = triangle(3,7,11);
   VIF[38] = triangle(7,11,21);
   VIF[39] = triangle(5,7,18);
   VIF[40] = triangle(7,18,21);
   VIF[41] = square(0,3,5,7);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
    p.set_description() << "Johnson solid J46: Gyroelongated pentagonal bicupola" << endl;

   return p;
}
      //FIXME: coordinates #830
perl::Object gyroelongated_pentagonal_cupolarotunda()
{
   perl::Object p = gyroelongated_pentagonal_rotunda();
   p = augment(p,sequence(20,10));

   IncidenceMatrix<> VIF(47,35);
   VIF[0] = square(23,26,30,31);
   VIF[1] = triangle(22,23,30);
   VIF[2] = triangle(10,22,23);
   VIF[3] = pentagon(4,8,14,16,17);
   VIF[4] = triangle(1,4,8);
   VIF[5] = triangle(16,17,27);
   VIF[6] = square(27,29,31,32);
   VIF[7] = triangle(17,27,29);
   VIF[8] = pentagon(0,3,5,7,9);
   VIF[9] = triangle(7,9,20);
   VIF[10] = triangle(0,2,5);
   VIF[11] = pentagon(6,11,14,18,19);
   VIF[12] = square(21,24,33,34);
   VIF[13] = triangle(2,6,11);
   VIF[14] = triangle(18,19,28);
   VIF[15] = triangle(13,21,24);
   VIF[16] = triangle(11,15,18);
   VIF[17] = triangle(15,24,25);
   VIF[18] = triangle(15,18,25);
   VIF[19] = triangle(13,15,24);
   VIF[20] = triangle(18,25,28);
   VIF[21] = triangle(24,25,33);
   VIF[22] = pentagon(2,5,11,13,15);
   VIF[23] = triangle(9,13,21);
   VIF[24] = triangle(5,9,13);
   VIF[25] = square(25,28,32,33);
   VIF[26] = triangle(19,28,29);
   VIF[27] = triangle(28,29,32);
   VIF[28] = triangle(9,20,21);
   VIF[29] = triangle(20,21,34);
   VIF[30] = triangle(17,19,29);
   VIF[31] = triangle(14,17,19);
   VIF[32] = triangle(4,6,14);
   VIF[33] = pentagon(30,31,32,33,34);
   VIF[34] = triangle(0,1,3);
   VIF[35] = pentagon(0,1,2,4,6);
   VIF[36] = triangle(7,20,22);
   VIF[37] = square(20,22,30,34);
   VIF[38] = triangle(3,7,10);
   VIF[39] = triangle(7,10,22);
   VIF[40] = triangle(26,27,31);
   VIF[41] = triangle(16,26,27);
   VIF[42] = triangle(8,12,16);
   VIF[43] = triangle(12,16,26);
   VIF[44] = triangle(10,12,23);
   VIF[45] = triangle(12,23,26);
   VIF[46] = pentagon(1,3,8,10,12);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);

   p.set_description() << "Johnson solid J47: Gyroelongated pentagonal cupolarotunda" << endl;
   return p;
}

//FIXME: coordinates #830
perl::Object gyroelongated_pentagonal_birotunda()
{
   perl::Object p = gyroelongated_pentagonal_rotunda();
   p = rotunda(p,sequence(20,10));

   IncidenceMatrix<> VIF(52,40);
   VIF[0] = pentagon(23,26,30,31,35);
   VIF[1] = triangle(10,22,23);
   VIF[2] = triangle(22,23,30);
   VIF[3] = pentagon(4,8,14,16,17);
   VIF[4] = triangle(1,4,8);
   VIF[5] = triangle(16,17,27);
   VIF[6] = pentagon(27,29,31,32,36);
   VIF[7] = triangle(31,35,36);
   VIF[8] = triangle(17,27,29);
   VIF[9] = pentagon(0,1,2,4,6);
   VIF[10] = triangle(0,1,3);
   VIF[11] = triangle(4,6,14);
   VIF[12] = triangle(14,17,19);
   VIF[13] = triangle(17,19,29);
   VIF[14] = triangle(32,36,37);
   VIF[15] = triangle(20,21,34);
   VIF[16] = triangle(9,20,21);
   VIF[17] = triangle(28,29,32);
   VIF[18] = triangle(19,28,29);
   VIF[19] = pentagon(25,28,32,33,37);
   VIF[20] = triangle(5,9,13);
   VIF[21] = triangle(9,13,21);
   VIF[22] = triangle(33,37,38);
   VIF[23] = pentagon(2,5,11,13,15);
   VIF[24] = triangle(13,21,24);
   VIF[25] = triangle(11,15,18);
   VIF[26] = triangle(15,18,25);
   VIF[27] = triangle(24,25,33);
   VIF[28] = triangle(15,24,25);
   VIF[29] = triangle(13,15,24);
   VIF[30] = triangle(18,25,28);
   VIF[31] = triangle(18,19,28);
   VIF[32] = triangle(2,6,11);
   VIF[33] = pentagon(21,24,33,34,38);
   VIF[34] = pentagon(6,11,14,18,19);
   VIF[35] = triangle(0,2,5);
   VIF[36] = triangle(34,38,39);
   VIF[37] = triangle(7,9,20);
   VIF[38] = pentagon(0,3,5,7,9);
   VIF[39] = pentagon(35,36,37,38,39);
   VIF[40] = triangle(7,20,22);
   VIF[41] = triangle(30,35,39);
   VIF[42] = pentagon(20,22,30,34,39);
   VIF[43] = triangle(3,7,10);
   VIF[44] = triangle(7,10,22);
   VIF[45] = triangle(26,27,31);
   VIF[46] = triangle(16,26,27);
   VIF[47] = triangle(8,12,16);
   VIF[48] = triangle(12,16,26);
   VIF[49] = triangle(10,12,23);
   VIF[50] = triangle(12,23,26);
   VIF[51] = pentagon(1,3,8,10,12);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J48: Gyroelongated pentagonal birotunda" << endl;

   return p;
}

//FIXME: coordinates 830. this could be done exactly if QE was more powerful
perl::Object augmented_triangular_prism(){

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

  IncidenceMatrix<> VIF(8,7);
  VIF[0] = square(0,1,4,5);
  VIF[1] = triangle(0,2,6);
  VIF[2] = triangle(0,2,5);
  VIF[3] = triangle(0,1,6);
  VIF[4] = triangle(1,3,6);
  VIF[5] = triangle(2,3,6);
  VIF[6] = triangle(1,3,4);
  VIF[7] = square(2,3,4,5);


  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
        p.take("VERTICES") << V;
        p.take("VERTICES_IN_FACETS") << VIF;
        p.set_description() << "Johnson solid J49: augmented triangular prism" << endl;
        p = centralize<QE>(p);

  return p;
}

perl::Object biaugmented_triangular_prism(){

  QE height(0,1,3);
  Matrix<QE> ledge(2,4);
  ledge.col(0).fill(1);
  ledge.col(1).fill(1);
  ledge.col(3).fill(-height);
  ledge[1][1]=-1;

  Matrix<QE> V = ((create_square_vertices<QE>() | zero_vector<QE>(4)) / ledge );

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.take("VERTICES") << V;

  p = augment(p,square(0,1,4,5));

  p = augment(p,sequence(0,4));


  IncidenceMatrix<> VIF(11,8);
  p.set_description() << "Johnson solid J50: biaugmented triangular prism" << endl;
  VIF[0] = triangle(1,4,6);
  VIF[1] = triangle(4,5,6);
  VIF[2] = triangle(0,2,7);
  VIF[3] = triangle(0,2,5);
  VIF[4] = triangle(0,5,6);
  VIF[5] = triangle(0,1,6);
  VIF[6] = triangle(0,1,7);
  VIF[7] = triangle(1,3,7);
  VIF[8] = triangle(2,3,7);
  VIF[9] = triangle(1,3,4);
  VIF[10] = square(2,3,4,5);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  return p;
}

perl::Object triaugmented_triangular_prism(){

  perl::Object p = create_prism(3);

  p = augment(p,square(1,2,4,5));
  p = augment(p,square(0,2,3,5));
  p = augment(p,square(0,1,3,4));


  IncidenceMatrix<> VIF(14,9);
  p.set_description() << "Johnson solid J51: triaugmented triangular prism" << endl;
  VIF[0] = triangle(0,1,8);
  VIF[1] = triangle(0,2,7);
  VIF[2] = triangle(0,1,2);
  VIF[3] = triangle(2,5,7);
  VIF[4] = triangle(1,2,6);
  VIF[5] = triangle(2,5,6);
  VIF[6] = triangle(4,5,6);
  VIF[7] = triangle(1,4,6);
  VIF[8] = triangle(1,4,8);
  VIF[9] = triangle(3,4,5);
  VIF[10] = triangle(3,5,7);
  VIF[11] = triangle(3,4,8);
  VIF[12] = triangle(0,3,7);
  VIF[13] = triangle(0,3,8);


  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<QE>(p);

  return p;
}

perl::Object augmented_pentagonal_prism(){

  perl::Object p = create_prism(5);

  p = augment(p,square(2,3,7,8));


  IncidenceMatrix<> VIF(10,11);
  p.set_description() << "Johnson solid J52: augmented pentagonal prism" << endl;
  VIF[0] = pentagon(0,1,2,3,4);
  VIF[1] = triangle(2,3,10);
  VIF[2] = triangle(3,8,10);
  VIF[3] = triangle(7,8,10);
  VIF[4] = triangle(2,7,10);
  VIF[5] = square(3,4,8,9);
  VIF[6] = square(1,2,6,7);
  VIF[7] = pentagon(5,6,7,8,9);
  VIF[8] = square(0,4,5,9);
  VIF[9] = square(0,1,5,6);


  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  return p;
}

perl::Object biaugmented_pentagonal_prism(){

  perl::Object p = create_prism(5);

  p = augment(p,square(2,3,7,8));
  p = augment(p,square(0,4,5,9));


  IncidenceMatrix<> VIF(13,12);
  p.set_description() << "Johnson solid J53: biaugmented pentagonal prism" << endl;
  VIF[0] = square(0,1,5,6);
  VIF[1] = pentagon(5,6,7,8,9);
  VIF[2] = square(1,2,6,7);
  VIF[3] = square(3,4,8,9);
  VIF[4] = triangle(2,7,10);
  VIF[5] = triangle(7,8,10);
  VIF[6] = triangle(3,8,10);
  VIF[7] = triangle(2,3,10);
  VIF[8] = pentagon(0,1,2,3,4);
  VIF[9] = triangle(4,9,11);
  VIF[10] = triangle(0,4,11);
  VIF[11] = triangle(5,9,11);
  VIF[12] = triangle(0,5,11);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  return p;
}


perl::Object augmented_hexagonal_prism(){

  perl::Object p = create_prism(6);

  p = augment(p,square(3,4,9,10));


  IncidenceMatrix<> VIF(11,13);
  p.set_description() << "Johnson solid J54: augmented hexagonal prism" << endl;
  VIF[0] = hexagon(0,1,2,3,4,5);
  VIF[1] = triangle(3,4,12);
  VIF[2] = triangle(3,9,12);
  VIF[3] = triangle(9,10,12);
  VIF[4] = triangle(4,10,12);
  VIF[5] = square(2,3,8,9);
  VIF[6] = square(4,5,10,11);
  VIF[7] = square(1,2,7,8);
  VIF[8] = hexagon(6,7,8,9,10,11);
  VIF[9] = square(0,5,6,11);
  VIF[10] = square(0,1,6,7);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  return p;
}

perl::Object parabiaugmented_hexagonal_prism(){

  perl::Object p = augmented_hexagonal_prism();

  p = augment(p,square(0,1,6,7));


  IncidenceMatrix<> VIF(14,14);
  p.set_description() << "Johnson solid J55: parabiaugmented hexagonal prism" << endl;
  VIF[0] = square(0,5,6,11);
  VIF[1] = hexagon(6,7,8,9,10,11);
  VIF[2] = square(1,2,7,8);
  VIF[3] = square(4,5,10,11);
  VIF[4] = square(2,3,8,9);
  VIF[5] = triangle(4,10,12);
  VIF[6] = triangle(9,10,12);
  VIF[7] = triangle(3,9,12);
  VIF[8] = triangle(3,4,12);
  VIF[9] = hexagon(0,1,2,3,4,5);
  VIF[10] = triangle(1,7,13);
  VIF[11] = triangle(0,1,13);
  VIF[12] = triangle(6,7,13);
  VIF[13] = triangle(0,6,13);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  return p;

}

perl::Object metabiaugmented_hexagonal_prism(){

  perl::Object p = augmented_hexagonal_prism();

  p = augment(p,square(1,2,7,8));


  IncidenceMatrix<> VIF(14,14);
  p.set_description() << "Johnson solid J56: metabiaugmented hexagonal prism" << endl;
  VIF[0] = hexagon(0,1,2,3,4,5);
  VIF[1] = triangle(1,2,13);
  VIF[2] = square(2,3,8,9);
  VIF[3] = triangle(4,10,12);
  VIF[4] = triangle(9,10,12);
  VIF[5] = triangle(3,9,12);
  VIF[6] = triangle(3,4,12);
  VIF[7] = triangle(2,8,13);
  VIF[8] = square(4,5,10,11);
  VIF[9] = triangle(7,8,13);
  VIF[10] = triangle(1,7,13);
  VIF[11] = hexagon(6,7,8,9,10,11);
  VIF[12] = square(0,5,6,11);
  VIF[13] = square(0,1,6,7);


  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  return p;
}

perl::Object triaugmented_hexagonal_prism(){

  perl::Object p = metabiaugmented_hexagonal_prism();

  p = augment(p,square(0,5,6,11));


  IncidenceMatrix<> VIF(17,15);
  p.set_description() << "Johnson solid J57: triaugmented hexagonal prism" << endl;
  VIF[0] = square(0,1,6,7);
  VIF[1] = hexagon(6,7,8,9,10,11);
  VIF[2] = triangle(1,7,13);
  VIF[3] = triangle(7,8,13);
  VIF[4] = square(4,5,10,11);
  VIF[5] = triangle(2,8,13);
  VIF[6] = triangle(3,4,12);
  VIF[7] = triangle(3,9,12);
  VIF[8] = triangle(9,10,12);
  VIF[9] = triangle(4,10,12);
  VIF[10] = square(2,3,8,9);
  VIF[11] = triangle(1,2,13);
  VIF[12] = hexagon(0,1,2,3,4,5);
  VIF[13] = triangle(5,11,14);
  VIF[14] = triangle(0,5,14);
  VIF[15] = triangle(6,11,14);
  VIF[16] = triangle(0,6,14);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  return p;
}

perl::Object augmented_dodecahedron(){

  perl::Object p = CallPolymakeFunction("dodecahedron");
  p = augment(p,pentagon(0,2,4,8,9));


  IncidenceMatrix<> VIF(16,21);
  p.set_description() << "Johnson solid J58: augmented dodecahedron" << endl;
  VIF[0] = pentagon(8,9,13,16,18);
  VIF[1] = pentagon(2,5,8,12,13);
  VIF[2] = pentagon(0,1,2,3,5);
  VIF[3] = pentagon(12,13,15,18,19);
  VIF[4] = pentagon(3,5,10,12,15);
  VIF[5] = pentagon(1,3,6,10,11);
  VIF[6] = pentagon(10,11,15,17,19);
  VIF[7] = pentagon(6,7,11,14,17);
  VIF[8] = pentagon(14,16,17,18,19);
  VIF[9] = pentagon(0,1,4,6,7);
  VIF[10] = pentagon(4,7,9,14,16);
  VIF[11] = triangle(0,4,20);
  VIF[12] = triangle(0,2,20);
  VIF[13] = triangle(4,9,20);
  VIF[14] = triangle(2,8,20);
  VIF[15] = triangle(8,9,20);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  return p;
}

perl::Object parabiaugmented_dodecahedron(){

  perl::Object p = augmented_dodecahedron();

  p = augment(p,pentagon(10,11,15,17,19));


  IncidenceMatrix<> VIF(20,22);
  p.set_description() << "Johnson solid J59: parabiaugmented dodecahedron" << endl;
  VIF[0] = pentagon(8,9,13,16,18);
  VIF[1] = pentagon(2,5,8,12,13);
  VIF[2] = pentagon(0,1,2,3,5);
  VIF[3] = pentagon(12,13,15,18,19);
  VIF[4] = pentagon(3,5,10,12,15);
  VIF[5] = pentagon(1,3,6,10,11);
  VIF[6] = triangle(10,15,21);
  VIF[7] = triangle(10,11,21);
  VIF[8] = triangle(11,17,21);
  VIF[9] = triangle(17,19,21);
  VIF[10] = triangle(15,19,21);
  VIF[11] = pentagon(6,7,11,14,17);
  VIF[12] = pentagon(14,16,17,18,19);
  VIF[13] = pentagon(0,1,4,6,7);
  VIF[14] = pentagon(4,7,9,14,16);
  VIF[15] = triangle(0,4,20);
  VIF[16] = triangle(0,2,20);
  VIF[17] = triangle(4,9,20);
  VIF[18] = triangle(2,8,20);
  VIF[19] = triangle(8,9,20);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  return p;
}

perl::Object metabiaugmented_dodecahedron(){

  perl::Object p = augmented_dodecahedron();

  p = augment(p,pentagon(12,13,15,18,19));


  IncidenceMatrix<> VIF(20,22);
  p.set_description() << "Johnson solid J60: metabiaugmented dodecahedron" << endl;
  VIF[0] = pentagon(8,9,13,16,18);
  VIF[1] = pentagon(2,5,8,12,13);
  VIF[2] = pentagon(0,1,2,3,5);
  VIF[3] = triangle(13,18,21);
  VIF[4] = triangle(12,13,21);
  VIF[5] = pentagon(3,5,10,12,15);
  VIF[6] = triangle(12,15,21);
  VIF[7] = triangle(15,19,21);
  VIF[8] = pentagon(10,11,15,17,19);
  VIF[9] = pentagon(1,3,6,10,11);
  VIF[10] = pentagon(6,7,11,14,17);
  VIF[11] = triangle(18,19,21);
  VIF[12] = pentagon(14,16,17,18,19);
  VIF[13] = pentagon(0,1,4,6,7);
  VIF[14] = pentagon(4,7,9,14,16);
  VIF[15] = triangle(0,4,20);
  VIF[16] = triangle(0,2,20);
  VIF[17] = triangle(4,9,20);
  VIF[18] = triangle(2,8,20);
  VIF[19] = triangle(8,9,20);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  return p;
}

perl::Object triaugmented_dodecahedron(){

  perl::Object p = metabiaugmented_dodecahedron();

  p = augment(p,pentagon(1,3,6,10,11));


  IncidenceMatrix<> VIF(24,23);
  p.set_description() << "Johnson solid J61: triaugmented dodecahedron" << endl;
  VIF[0] = pentagon(8,9,13,16,18);
  VIF[1] = pentagon(2,5,8,12,13);
  VIF[2] = pentagon(0,1,2,3,5);
  VIF[3] = triangle(13,18,21);
  VIF[4] = triangle(12,13,21);
  VIF[5] = pentagon(3,5,10,12,15);
  VIF[6] = triangle(12,15,21);
  VIF[7] = triangle(15,19,21);
  VIF[8] = pentagon(10,11,15,17,19);
  VIF[9] = triangle(6,11,22);
  VIF[10] = triangle(10,11,22);
  VIF[11] = triangle(3,10,22);
  VIF[12] = triangle(1,6,22);
  VIF[13] = triangle(1,3,22);
  VIF[14] = pentagon(6,7,11,14,17);
  VIF[15] = triangle(18,19,21);
  VIF[16] = pentagon(14,16,17,18,19);
  VIF[17] = pentagon(0,1,4,6,7);
  VIF[18] = pentagon(4,7,9,14,16);
  VIF[19] = triangle(0,4,20);
  VIF[20] = triangle(0,2,20);
  VIF[21] = triangle(4,9,20);
  VIF[22] = triangle(2,8,20);
  VIF[23] = triangle(8,9,20);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  return p;
}

perl::Object metabidiminished_icosahedron(){

  perl::Object ico = CallPolymakeFunction("icosahedron");

  Matrix<QE> V = ico.give("VERTICES");

  V = ((V.minor(sequence(1,5),All)) / (V.minor(sequence(7,5),All)));

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);

  p.set_description() << "Johnson solid J62: metabidiminished icosahedron" << endl;
  return p;
}

perl::Object tridiminished_icosahedron(){

   perl::Object dimico = metabidiminished_icosahedron();

  Matrix<QE> V = dimico.give("VERTICES");
  V = ((V.minor(sequence(0,7),All)) / (V.minor(sequence(8,2),All)));

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);

  p.set_description() << "Johnson solid J63: tridiminished icosahedron" << endl;
  return p;
}

perl::Object augmented_tridiminished_icosahedron(){

  perl::Object p = tridiminished_icosahedron();
  p = augment(p,triangle(0,2,5));


  IncidenceMatrix<> VIF(10,10);
  p.set_description() << "Johnson solid J64: augmented_tridiminished icosahedron" << endl;
  VIF[0] = triangle(3,6,7);
  VIF[1] = triangle(6,7,8);
  VIF[2] = triangle(0,5,9);
  VIF[3] = triangle(0,2,9);
  VIF[4] = triangle(2,5,9);
  VIF[5] = pentagon(2,4,5,7,8);
  VIF[6] = triangle(3,4,7);
  VIF[7] = triangle(1,3,6);
  VIF[8] = pentagon(0,1,2,3,4);
  VIF[9] = pentagon(0,1,5,6,8);


  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  return p;
}


perl::Object augmented_truncated_tetrahedron()
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

  perl::Object p(perl::ObjectType::construct<Rational>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<Rational>(p);
  p.set_description() << "Johnson solid J65: Augmented truncated tetrahedron" << endl;

  return p;
}

perl::Object augmented_truncated_cube()
{
   Matrix<QE> V = truncated_cube_vertices();
   Matrix<QE> W = square_cupola_impl(false).give("VERTICES");
   W.col(3) += same_element_vector(QE(2,2,2),12);

   perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
   p.take("VERTICES") << V / W.minor(sequence(8,4),All);
   p = centralize<QE>(p);
   p.set_description() << "Johnson solid J66: Augmented truncated cube" << endl;

   return p;
}

perl::Object biaugmented_truncated_cube()
{
   Matrix<QE> V = truncated_cube_vertices();
   Matrix<QE> W = square_cupola_impl(false).give("VERTICES");
   W = W.minor(sequence(8,4),sequence(1,3));

   perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
   p.take("VERTICES") << V / (ones_vector<QE>(8) |
                              ((W + repeat_row(QE(2,2,2)*unit_vector<QE>(3,2),4)) / -W));
   p = centralize<QE>(p);
   p.set_description() << "Johnson solid J67: Biaugmented truncated cube" << endl;

   return p;
}

perl::Object augmented_truncated_dodecahedron()
{
   perl::Object p = CallPolymakeFunction("truncated_dodecahedron");

   p = augment(p,decagon(0,1,5,7,9,11,13,16,18,21));

   IncidenceMatrix<> VIF(42,65);
   VIF[0] = triangle(22,27,38);
   VIF[1] = decagon(13,16,19,22,32,35,38,41,43,46);
   VIF[2] = triangle(7,13,19);
   VIF[3] = triangle(41,46,52);
   VIF[4] = square(7,13,61,62);
   VIF[5] = triangle(5,7,61);
   VIF[6] = decagon(43,46,48,50,52,54,56,57,58,59);
   VIF[7] = square(0,5,60,61);
   VIF[8] = triangle(13,16,62);
   VIF[9] = triangle(0,1,60);
   VIF[10] = triangle(10,15,26);
   VIF[11] = triangle(29,36,42);
   VIF[12] = decagon(12,15,20,23,26,29,37,40,42,45);
   VIF[13] = triangle(1,4,9);
   VIF[14] = triangle(55,57,59);
   VIF[15] = decagon(4,6,9,11,20,23,25,28,30,33);
   VIF[16] = triangle(6,12,20);
   VIF[17] = triangle(40,45,53);
   VIF[18] = decagon(30,33,37,40,44,47,53,55,56,57);
   VIF[19] = triangle(23,30,37);
   VIF[20] = triangle(28,33,44);
   VIF[21] = triangle(47,50,56);
   VIF[22] = triangle(11,18,25);
   VIF[23] = triangle(9,11,64);
   VIF[24] = triangle(18,21,63);
   VIF[25] = square(11,18,63,64);
   VIF[26] = decagon(18,21,25,28,32,35,44,47,48,50);
   VIF[27] = square(1,9,60,64);
   VIF[28] = triangle(35,43,48);
   VIF[29] = triangle(16,21,32);
   VIF[30] = square(16,21,62,63);
   VIF[31] = pentagon(60,61,62,63,64);
   VIF[32] = triangle(51,54,58);
   VIF[33] = triangle(0,2,5);
   VIF[34] = decagon(36,39,42,45,49,51,53,55,58,59);
   VIF[35] = decagon(0,1,2,3,4,6,8,10,12,15);
   VIF[36] = triangle(34,39,49);
   VIF[37] = triangle(3,8,14);
   VIF[38] = decagon(8,10,14,17,26,29,31,34,36,39);
   VIF[39] = triangle(17,24,31);
   VIF[40] = decagon(24,27,31,34,38,41,49,51,52,54);
   VIF[41] = decagon(2,3,5,7,14,17,19,22,24,27);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J68: Augmented truncated dodecahedron" << endl;

  return p;
}


perl::Object parabiaugmented_truncated_dodecahedron()
{
   perl::Object p = augmented_truncated_dodecahedron();
   p = augment(p,decagon(36,39,42,45,49,51,53,55,58,59));

   IncidenceMatrix<> VIF(52,70);
   VIF[0] = triangle(22,27,38);
   VIF[1] = decagon(13,16,19,22,32,35,38,41,43,46);
   VIF[2] = triangle(7,13,19);
   VIF[3] = triangle(41,46,52);
   VIF[4] = square(7,13,61,62);
   VIF[5] = triangle(5,7,61);
   VIF[6] = decagon(43,46,48,50,52,54,56,57,58,59);
   VIF[7] = square(0,5,60,61);
   VIF[8] = triangle(13,16,62);
   VIF[9] = pentagon(65,66,67,68,69);
   VIF[10] = square(16,21,62,63);
   VIF[11] = triangle(16,21,32);
   VIF[12] = triangle(35,43,48);
   VIF[13] = triangle(58,59,69);
   VIF[14] = decagon(12,15,20,23,26,29,37,40,42,45);
   VIF[15] = square(55,59,68,69);
   VIF[16] = square(11,18,63,64);
   VIF[17] = triangle(18,21,63);
   VIF[18] = triangle(55,57,59);
   VIF[19] = triangle(53,55,68);
   VIF[20] = triangle(11,18,25);
   VIF[21] = triangle(47,50,56);
   VIF[22] = triangle(28,33,44);
   VIF[23] = triangle(23,30,37);
   VIF[24] = decagon(30,33,37,40,44,47,53,55,56,57);
   VIF[25] = triangle(40,45,53);
   VIF[26] = triangle(6,12,20);
   VIF[27] = decagon(4,6,9,11,20,23,25,28,30,33);
   VIF[28] = triangle(9,11,64);
   VIF[29] = square(45,53,67,68);
   VIF[30] = triangle(42,45,67);
   VIF[31] = triangle(1,4,9);
   VIF[32] = decagon(18,21,25,28,32,35,44,47,48,50);
   VIF[33] = square(1,9,60,64);
   VIF[34] = triangle(29,36,42);
   VIF[35] = square(36,42,66,67);
   VIF[36] = triangle(10,15,26);
   VIF[37] = triangle(0,1,60);
   VIF[38] = pentagon(60,61,62,63,64);
   VIF[39] = triangle(51,54,58);
   VIF[40] = square(51,58,65,69);
   VIF[41] = triangle(36,39,66);
   VIF[42] = triangle(0,2,5);
   VIF[43] = triangle(49,51,65);
   VIF[44] = square(39,49,65,66);
   VIF[45] = decagon(0,1,2,3,4,6,8,10,12,15);
   VIF[46] = triangle(34,39,49);
   VIF[47] = triangle(3,8,14);
   VIF[48] = decagon(8,10,14,17,26,29,31,34,36,39);
   VIF[49] = triangle(17,24,31);
   VIF[50] = decagon(24,27,31,34,38,41,49,51,52,54);
   VIF[51] = decagon(2,3,5,7,14,17,19,22,24,27);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);

   p.set_description() << "Johnson solid J69: Parabiaugmented truncated dodecahedron" << endl;
   return p;
}

perl::Object metabiaugmented_truncated_dodecahedron()
{
   perl::Object p = augmented_truncated_dodecahedron();
   p = augment(p,decagon(43,46,48,50,52,54,56,57,58,59));

   IncidenceMatrix<> VIF(52,70);
   VIF[0] = triangle(22,27,38);
   VIF[1] = decagon(13,16,19,22,32,35,38,41,43,46);
   VIF[2] = triangle(7,13,19);
   VIF[3] = triangle(41,46,52);
   VIF[4] = square(7,13,61,62);
   VIF[5] = triangle(5,7,61);
   VIF[6] = square(46,52,65,69);
   VIF[7] = triangle(52,54,65);
   VIF[8] = square(0,5,60,61);
   VIF[9] = triangle(13,16,62);
   VIF[10] = triangle(43,46,69);
   VIF[11] = square(54,58,65,66);
   VIF[12] = pentagon(65,66,67,68,69);
   VIF[13] = square(16,21,62,63);
   VIF[14] = triangle(16,21,32);
   VIF[15] = square(43,48,68,69);
   VIF[16] = triangle(35,43,48);
   VIF[17] = triangle(58,59,66);
   VIF[18] = decagon(12,15,20,23,26,29,37,40,42,45);
   VIF[19] = square(57,59,66,67);
   VIF[20] = square(11,18,63,64);
   VIF[21] = triangle(18,21,63);
   VIF[22] = triangle(48,50,68);
   VIF[23] = triangle(55,57,59);
   VIF[24] = triangle(56,57,67);
   VIF[25] = triangle(11,18,25);
   VIF[26] = triangle(47,50,56);
   VIF[27] = triangle(28,33,44);
   VIF[28] = triangle(23,30,37);
   VIF[29] = decagon(30,33,37,40,44,47,53,55,56,57);
   VIF[30] = triangle(40,45,53);
   VIF[31] = triangle(6,12,20);
   VIF[32] = decagon(4,6,9,11,20,23,25,28,30,33);
   VIF[33] = triangle(9,11,64);
   VIF[34] = square(50,56,67,68);
   VIF[35] = triangle(1,4,9);
   VIF[36] = decagon(18,21,25,28,32,35,44,47,48,50);
   VIF[37] = square(1,9,60,64);
   VIF[38] = triangle(29,36,42);
   VIF[39] = triangle(10,15,26);
   VIF[40] = triangle(0,1,60);
   VIF[41] = pentagon(60,61,62,63,64);
   VIF[42] = triangle(51,54,58);
   VIF[43] = triangle(0,2,5);
   VIF[44] = decagon(36,39,42,45,49,51,53,55,58,59);
   VIF[45] = decagon(0,1,2,3,4,6,8,10,12,15);
   VIF[46] = triangle(34,39,49);
   VIF[47] = triangle(3,8,14);
   VIF[48] = decagon(8,10,14,17,26,29,31,34,36,39);
   VIF[49] = triangle(17,24,31);
   VIF[50] = decagon(24,27,31,34,38,41,49,51,52,54);
   VIF[51] = decagon(2,3,5,7,14,17,19,22,24,27);

   p.take("VERTICES_IN_FACETS") << VIF;

   p = centralize<double>(p);

   p.set_description() << "Johnson solid J70: Metabiaugmented truncated dodecahedron" << endl;
  return p;
}

perl::Object triaugmented_truncated_dodecahedron()
{
   perl::Object p = metabiaugmented_truncated_dodecahedron();
   p = augment(p,decagon(12,15,20,23,26,29,37,40,42,45));

   IncidenceMatrix<> VIF(62,75);
   VIF[0] = triangle(22,27,38);
   VIF[1] = decagon(13,16,19,22,32,35,38,41,43,46);
   VIF[2] = triangle(7,13,19);
   VIF[3] = triangle(41,46,52);
   VIF[4] = square(7,13,61,62);
   VIF[5] = triangle(5,7,61);
   VIF[6] = square(46,52,65,69);
   VIF[7] = triangle(52,54,65);
   VIF[8] = square(0,5,60,61);
   VIF[9] = triangle(13,16,62);
   VIF[10] = triangle(43,46,69);
   VIF[11] = square(54,58,65,66);
   VIF[12] = pentagon(65,66,67,68,69);
   VIF[13] = square(16,21,62,63);
   VIF[14] = triangle(16,21,32);
   VIF[15] = square(43,48,68,69);
   VIF[16] = triangle(35,43,48);
   VIF[17] = triangle(58,59,66);
   VIF[18] = square(15,26,70,74);
   VIF[19] = triangle(26,29,74);
   VIF[20] = square(29,42,73,74);
   VIF[21] = square(57,59,66,67);
   VIF[22] = square(11,18,63,64);
   VIF[23] = triangle(18,21,63);
   VIF[24] = triangle(48,50,68);
   VIF[25] = triangle(55,57,59);
   VIF[26] = triangle(56,57,67);
   VIF[27] = decagon(4,6,9,11,20,23,25,28,30,33);
   VIF[28] = square(12,20,70,71);
   VIF[29] = triangle(6,12,20);
   VIF[30] = square(40,45,72,73);
   VIF[31] = triangle(40,45,53);
   VIF[32] = decagon(30,33,37,40,44,47,53,55,56,57);
   VIF[33] = triangle(20,23,71);
   VIF[34] = triangle(37,40,72);
   VIF[35] = square(23,37,71,72);
   VIF[36] = triangle(23,30,37);
   VIF[37] = triangle(28,33,44);
   VIF[38] = triangle(47,50,56);
   VIF[39] = triangle(11,18,25);
   VIF[40] = pentagon(70,71,72,73,74);
   VIF[41] = triangle(9,11,64);
   VIF[42] = square(50,56,67,68);
   VIF[43] = triangle(42,45,73);
   VIF[44] = triangle(12,15,70);
   VIF[45] = triangle(1,4,9);
   VIF[46] = decagon(18,21,25,28,32,35,44,47,48,50);
   VIF[47] = square(1,9,60,64);
   VIF[48] = triangle(29,36,42);
   VIF[49] = triangle(10,15,26);
   VIF[50] = triangle(0,1,60);
   VIF[51] = pentagon(60,61,62,63,64);
   VIF[52] = triangle(51,54,58);
   VIF[53] = triangle(0,2,5);
   VIF[54] = decagon(36,39,42,45,49,51,53,55,58,59);
   VIF[55] = decagon(0,1,2,3,4,6,8,10,12,15);
   VIF[56] = triangle(34,39,49);
   VIF[57] = triangle(3,8,14);
   VIF[58] = decagon(8,10,14,17,26,29,31,34,36,39);
   VIF[59] = triangle(17,24,31);
   VIF[60] = decagon(24,27,31,34,38,41,49,51,52,54);
   VIF[61] = decagon(2,3,5,7,14,17,19,22,24,27);

   p.take("VERTICES_IN_FACETS") << VIF;

   p = centralize<double>(p);

   p.set_description() << "Johnson solid J71: Triaugmented truncated dodecahedron" << endl;
   return p;
}

perl::Object gyrate_rhombicosidodecahedron()
{
  perl::Object p = CallPolymakeFunction("rhombicosidodecahedron");
  p = rotate_facet(p,pentagon(5,8,12,16,21),M_PI/5);

  IncidenceMatrix<> VIF(62,60);
  VIF[0] = pentagon(27,32,37,41,45);
  VIF[1] = square(18,27,28,37);
  VIF[2] = square(3,11,56,58);
  VIF[3] = triangle(11,18,58);
  VIF[4] = pentagon(11,14,18,23,28);
  VIF[5] = square(37,43,45,50);
  VIF[6] = triangle(28,37,43);
  VIF[7] = square(0,2,55,56);
  VIF[8] = triangle(0,3,56);
  VIF[9] = square(3,8,11,14);
  VIF[10] = square(23,28,39,43);
  VIF[11] = triangle(45,50,53);
  VIF[12] = pentagon(2,4,6,9,12);
  VIF[13] = pentagon(44,48,51,53,54);
  VIF[14] = square(20,26,30,36);
  VIF[15] = triangle(12,20,26);
  VIF[16] = triangle(30,36,44);
  VIF[17] = square(9,12,21,26);
  VIF[18] = square(36,40,44,51);
  VIF[19] = square(4,7,9,13);
  VIF[20] = pentagon(21,26,31,36,40);
  VIF[21] = square(46,49,51,54);
  VIF[22] = triangle(9,13,21);
  VIF[23] = triangle(40,46,51);
  VIF[24] = square(10,15,17,24);
  VIF[25] = square(24,33,34,42);
  VIF[26] = pentagon(33,38,42,46,49);
  VIF[27] = square(17,22,33,38);
  VIF[28] = triangle(22,31,38);
  VIF[29] = triangle(17,24,33);
  VIF[30] = square(31,38,40,46);
  VIF[31] = square(13,21,22,31);
  VIF[32] = pentagon(7,10,13,17,22);
  VIF[33] = triangle(34,42,47);
  VIF[34] = triangle(5,10,15);
  VIF[35] = triangle(49,52,54);
  VIF[36] = square(42,47,49,52);
  VIF[37] = pentagon(15,19,24,29,34);
  VIF[38] = triangle(1,4,7);
  VIF[39] = square(1,5,7,10);
  VIF[40] = square(29,34,39,47);
  VIF[41] = square(5,8,15,19);
  VIF[42] = square(50,52,53,54);
  VIF[43] = triangle(23,29,39);
  VIF[44] = triangle(8,14,19);
  VIF[45] = square(14,19,23,29);
  VIF[46] = square(0,1,2,4);
  VIF[47] = pentagon(39,43,47,50,52);
  VIF[48] = pentagon(0,1,3,5,8);
  VIF[49] = square(30,35,44,48);
  VIF[50] = square(6,12,16,20);
  VIF[51] = triangle(2,6,55);
  VIF[52] = triangle(35,41,48);
  VIF[53] = square(41,45,48,53);
  VIF[54] = pentagon(16,20,25,30,35);
  VIF[55] = triangle(16,25,57);
  VIF[56] = square(6,16,55,57);
  VIF[57] = square(25,32,35,41);
  VIF[58] = triangle(27,32,59);
  VIF[59] = square(25,32,57,59);
  VIF[60] = pentagon(55,56,57,58,59);
  VIF[61] = square(18,27,58,59);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);
  p.set_description() << "Johnson solid J72: Gyrate rhombicosidodecahedron" << endl;
  return p;
}

perl::Object parabigyrate_rhombicosidodecahedron()
{
   perl::Object p = gyrate_rhombicosidodecahedron();
   p = rotate_facet(p,pentagon(33,38,42,46,49),M_PI/5);
  IncidenceMatrix<> VIF(62,60);
  VIF[0] = pentagon(27,32,36,39,42);
  VIF[1] = square(18,27,28,36);
  VIF[2] = square(3,11,51,53);
  VIF[3] = triangle(11,18,53);
  VIF[4] = pentagon(11,14,18,23,28);
  VIF[5] = square(36,40,42,45);
  VIF[6] = triangle(28,36,40);
  VIF[7] = square(0,2,50,51);
  VIF[8] = triangle(0,3,51);
  VIF[9] = square(3,8,11,14);
  VIF[10] = square(23,28,37,40);
  VIF[11] = triangle(42,45,48);
  VIF[12] = pentagon(2,4,6,9,12);
  VIF[13] = pentagon(41,44,46,48,49);
  VIF[14] = square(20,26,30,35);
  VIF[15] = triangle(12,20,26);
  VIF[16] = triangle(30,35,41);
  VIF[17] = square(9,12,21,26);
  VIF[18] = square(35,38,41,46);
  VIF[19] = square(4,7,9,13);
  VIF[20] = pentagon(21,26,31,35,38);
  VIF[21] = triangle(46,49,59);
  VIF[22] = triangle(9,13,21);
  VIF[23] = square(38,46,58,59);
  VIF[24] = square(10,15,17,24);
  VIF[25] = triangle(24,33,55);
  VIF[26] = pentagon(55,56,57,58,59);
  VIF[27] = square(17,24,55,56);
  VIF[28] = triangle(17,22,56);
  VIF[29] = square(22,31,56,58);
  VIF[30] = triangle(31,38,58);
  VIF[31] = square(13,21,22,31);
  VIF[32] = pentagon(7,10,13,17,22);
  VIF[33] = square(33,43,55,57);
  VIF[34] = triangle(5,10,15);
  VIF[35] = triangle(43,47,57);
  VIF[36] = square(47,49,57,59);
  VIF[37] = pentagon(15,19,24,29,33);
  VIF[38] = triangle(1,4,7);
  VIF[39] = square(1,5,7,10);
  VIF[40] = square(29,33,37,43);
  VIF[41] = square(5,8,15,19);
  VIF[42] = square(45,47,48,49);
  VIF[43] = triangle(23,29,37);
  VIF[44] = triangle(8,14,19);
  VIF[45] = square(14,19,23,29);
  VIF[46] = square(0,1,2,4);
  VIF[47] = pentagon(37,40,43,45,47);
  VIF[48] = pentagon(0,1,3,5,8);
  VIF[49] = square(30,34,41,44);
  VIF[50] = square(6,12,16,20);
  VIF[51] = triangle(2,6,50);
  VIF[52] = triangle(34,39,44);
  VIF[53] = square(39,42,44,48);
  VIF[54] = pentagon(16,20,25,30,34);
  VIF[55] = triangle(16,25,52);
  VIF[56] = square(6,16,50,52);
  VIF[57] = square(25,32,34,39);
  VIF[58] = triangle(27,32,54);
  VIF[59] = square(25,32,52,54);
  VIF[60] = pentagon(50,51,52,53,54);
  VIF[61] = square(18,27,53,54);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);

  p.set_description() << "Johnson solid J73: Parabigyrate rhombicosidodecahedron" << endl;
  return p;
}

perl::Object metabigyrate_rhombicosidodecahedron()
{
   perl::Object p = gyrate_rhombicosidodecahedron();
  p = rotate_facet(p,pentagon(44,48,51,53,54),M_PI/5);
  IncidenceMatrix<> VIF(62,60);
  VIF[0] = pentagon(27,32,37,41,44);
  VIF[1] = square(18,27,28,37);
  VIF[2] = square(3,11,51,53);
  VIF[3] = triangle(11,18,53);
  VIF[4] = pentagon(11,14,18,23,28);
  VIF[5] = square(37,43,44,48);
  VIF[6] = triangle(28,37,43);
  VIF[7] = square(0,2,50,51);
  VIF[8] = triangle(0,3,51);
  VIF[9] = square(3,8,11,14);
  VIF[10] = square(23,28,39,43);
  VIF[11] = square(44,48,58,59);
  VIF[12] = pentagon(2,4,6,9,12);
  VIF[13] = pentagon(55,56,57,58,59);
  VIF[14] = square(20,26,30,36);
  VIF[15] = triangle(12,20,26);
  VIF[16] = square(30,36,55,56);
  VIF[17] = triangle(48,49,59);
  VIF[18] = square(9,12,21,26);
  VIF[19] = square(47,49,57,59);
  VIF[20] = square(4,7,9,13);
  VIF[21] = pentagon(21,26,31,36,40);
  VIF[22] = triangle(36,40,55);
  VIF[23] = square(42,46,47,49);
  VIF[24] = triangle(9,13,21);
  VIF[25] = triangle(45,47,57);
  VIF[26] = square(10,15,17,24);
  VIF[27] = square(24,33,34,42);
  VIF[28] = pentagon(33,38,42,45,47);
  VIF[29] = square(17,22,33,38);
  VIF[30] = triangle(22,31,38);
  VIF[31] = triangle(17,24,33);
  VIF[32] = square(31,38,40,45);
  VIF[33] = square(13,21,22,31);
  VIF[34] = pentagon(7,10,13,17,22);
  VIF[35] = triangle(34,42,46);
  VIF[36] = triangle(5,10,15);
  VIF[37] = square(40,45,55,57);
  VIF[38] = pentagon(15,19,24,29,34);
  VIF[39] = triangle(1,4,7);
  VIF[40] = square(1,5,7,10);
  VIF[41] = square(29,34,39,46);
  VIF[42] = square(5,8,15,19);
  VIF[43] = triangle(23,29,39);
  VIF[44] = triangle(8,14,19);
  VIF[45] = square(14,19,23,29);
  VIF[46] = square(0,1,2,4);
  VIF[47] = pentagon(39,43,46,48,49);
  VIF[48] = pentagon(0,1,3,5,8);
  VIF[49] = triangle(30,35,56);
  VIF[50] = square(6,12,16,20);
  VIF[51] = triangle(2,6,50);
  VIF[52] = triangle(41,44,58);
  VIF[53] = square(35,41,56,58);
  VIF[54] = pentagon(16,20,25,30,35);
  VIF[55] = triangle(16,25,52);
  VIF[56] = square(6,16,50,52);
  VIF[57] = square(25,32,35,41);
  VIF[58] = triangle(27,32,54);
  VIF[59] = square(25,32,52,54);
  VIF[60] = pentagon(50,51,52,53,54);
  VIF[61] = square(18,27,53,54);


  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);
  p.set_description() << "Johnson solid J74: Metabigyrate rhombicosidodecahedron" << endl;
  return p;
}

perl::Object trigyrate_rhombicosidodecahedron()
{
   perl::Object p = metabigyrate_rhombicosidodecahedron();
  p = rotate_facet(p,pentagon(15,19,24,29,34),M_PI/5);
  IncidenceMatrix<> VIF(62,60);
  VIF[0] = pentagon(24,28,32,36,39);
  VIF[1] = square(17,24,25,32);
  VIF[2] = square(3,11,46,48);
  VIF[3] = triangle(11,17,48);
  VIF[4] = pentagon(11,14,17,21,25);
  VIF[5] = square(32,38,39,43);
  VIF[6] = triangle(25,32,38);
  VIF[7] = square(0,2,45,46);
  VIF[8] = triangle(0,3,46);
  VIF[9] = square(3,8,11,14);
  VIF[10] = square(21,25,34,38);
  VIF[11] = square(39,43,53,54);
  VIF[12] = pentagon(2,4,6,9,12);
  VIF[13] = pentagon(50,51,52,53,54);
  VIF[14] = square(18,23,26,31);
  VIF[15] = triangle(12,18,23);
  VIF[16] = square(26,31,50,51);
  VIF[17] = triangle(43,44,54);
  VIF[18] = square(9,12,19,23);
  VIF[19] = square(42,44,52,54);
  VIF[20] = square(1,5,7,10);
  VIF[21] = triangle(1,4,7);
  VIF[22] = pentagon(19,23,27,31,35);
  VIF[23] = square(35,40,50,52);
  VIF[24] = square(5,10,55,57);
  VIF[25] = square(37,41,58,59);
  VIF[26] = pentagon(7,10,13,16,20);
  VIF[27] = square(27,33,35,40);
  VIF[28] = triangle(10,16,57);
  VIF[29] = triangle(29,37,59);
  VIF[30] = square(16,29,57,59);
  VIF[31] = square(16,20,29,33);
  VIF[32] = triangle(20,27,33);
  VIF[33] = pentagon(29,33,37,40,42);
  VIF[34] = square(13,19,20,27);
  VIF[35] = triangle(40,42,52);
  VIF[36] = triangle(9,13,19);
  VIF[37] = square(37,41,42,44);
  VIF[38] = triangle(31,35,50);
  VIF[39] = square(4,7,9,13);
  VIF[40] = pentagon(55,56,57,58,59);
  VIF[41] = triangle(34,41,58);
  VIF[42] = triangle(5,8,55);
  VIF[43] = square(21,34,56,58);
  VIF[44] = triangle(14,21,56);
  VIF[45] = square(8,14,55,56);
  VIF[46] = square(0,1,2,4);
  VIF[47] = pentagon(34,38,41,43,44);
  VIF[48] = pentagon(0,1,3,5,8);
  VIF[49] = triangle(26,30,51);
  VIF[50] = square(6,12,15,18);
  VIF[51] = triangle(2,6,45);
  VIF[52] = triangle(36,39,53);
  VIF[53] = square(30,36,51,53);
  VIF[54] = pentagon(15,18,22,26,30);
  VIF[55] = triangle(15,22,47);
  VIF[56] = square(6,15,45,47);
  VIF[57] = square(22,28,30,36);
  VIF[58] = triangle(24,28,49);
  VIF[59] = square(22,28,47,49);
  VIF[60] = pentagon(45,46,47,48,49);
  VIF[61] = square(17,24,48,49);


  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);
  p.set_description() << "Johnson solid J75: trigyrate rhombicosidodecahedron" << endl;
  return p;
}

perl::Object diminished_rhombicosidodecahedron()
{
   perl::Object p = CallPolymakeFunction("rhombicosidodecahedron");
   p = diminish<QE>(p,pentagon(5,8,12,16,21));
   p = centralize<QE>(p);

   p.set_description() << "Johnson solid J76: diminished rhombicosidodecahedron" << endl;
   return p;
}

  perl::Object paragyrate_diminished_rhombicosidodecahedron()
{
   perl::Object p = gyrate_rhombicosidodecahedron();

   p = diminish<double>(p,pentagon(33,38,42,46,49));
   IncidenceMatrix<> VIF(52,55);
   VIF[0] = pentagon(27,32,36,39,42);
   VIF[1] = square(18,27,28,36);
   VIF[2] = square(3,11,51,53);
   VIF[3] = triangle(11,18,53);
   VIF[4] = pentagon(11,14,18,23,28);
   VIF[5] = square(36,40,42,45);
   VIF[6] = triangle(28,36,40);
   VIF[7] = square(0,2,50,51);
   VIF[8] = triangle(0,3,51);
   VIF[9] = square(3,8,11,14);
   VIF[10] = square(23,28,37,40);
   VIF[11] = triangle(42,45,48);
   VIF[12] = pentagon(2,4,6,9,12);
   VIF[13] = pentagon(41,44,46,48,49);
   VIF[14] = square(20,26,30,35);
   VIF[15] = triangle(12,20,26);
   VIF[16] = triangle(30,35,41);
   VIF[17] = square(9,12,21,26);
   VIF[18] = square(35,38,41,46);
   VIF[19] = square(4,7,9,13);
   VIF[20] = pentagon(21,26,31,35,38);
   VIF[21] = triangle(9,13,21);
   VIF[22] = square(10,15,17,24);
   VIF[23] = square(13,21,22,31);
   VIF[24] = pentagon(7,10,13,17,22);
   VIF[25] = triangle(5,10,15);
   VIF[26] = decagon(17,22,24,31,33,38,43,46,47,49);
   VIF[27] = pentagon(15,19,24,29,33);
   VIF[28] = triangle(1,4,7);
   VIF[29] = square(1,5,7,10);
   VIF[30] = square(29,33,37,43);
   VIF[31] = square(5,8,15,19);
   VIF[32] = square(45,47,48,49);
   VIF[33] = triangle(23,29,37);
   VIF[34] = triangle(8,14,19);
   VIF[35] = square(14,19,23,29);
   VIF[36] = square(0,1,2,4);
   VIF[37] = pentagon(37,40,43,45,47);
   VIF[38] = pentagon(0,1,3,5,8);
   VIF[39] = square(30,34,41,44);
   VIF[40] = square(6,12,16,20);
   VIF[41] = triangle(2,6,50);
   VIF[42] = triangle(34,39,44);
   VIF[43] = square(39,42,44,48);
   VIF[44] = pentagon(16,20,25,30,34);
   VIF[45] = triangle(16,25,52);
   VIF[46] = square(6,16,50,52);
   VIF[47] = square(25,32,34,39);
   VIF[48] = triangle(27,32,54);
   VIF[49] = square(25,32,52,54);
   VIF[50] = pentagon(50,51,52,53,54);
   VIF[51] = square(18,27,53,54);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J77: paragyrate diminished rhombicosidodecahedron" << endl;
   return p;
}

  perl::Object metagyrate_diminished_rhombicosidodecahedron()
{
   perl::Object p = gyrate_rhombicosidodecahedron();

   p = diminish<double>(p,pentagon(44,48,51,53,54));
   IncidenceMatrix<> VIF(52,55);
   VIF[0] = pentagon(27,32,37,41,44);
   VIF[1] = square(18,27,28,37);
   VIF[2] = square(3,11,51,53);
   VIF[3] = triangle(11,18,53);
   VIF[4] = pentagon(11,14,18,23,28);
   VIF[5] = square(37,43,44,48);
   VIF[6] = triangle(28,37,43);
   VIF[7] = square(0,2,50,51);
   VIF[8] = triangle(0,3,51);
   VIF[9] = square(3,8,11,14);
   VIF[10] = square(23,28,39,43);
   VIF[11] = pentagon(2,4,6,9,12);
   VIF[12] = square(0,1,2,4);
   VIF[13] = square(14,19,23,29);
   VIF[14] = triangle(8,14,19);
   VIF[15] = triangle(23,29,39);
   VIF[16] = square(9,12,21,26);
   VIF[17] = square(1,5,7,10);
   VIF[18] = triangle(1,4,7);
   VIF[19] = pentagon(15,19,24,29,34);
   VIF[20] = square(42,46,47,49);
   VIF[21] = triangle(9,13,21);
   VIF[22] = pentagon(7,10,13,17,22);
   VIF[23] = square(13,21,22,31);
   VIF[24] = square(31,38,40,45);
   VIF[25] = triangle(17,24,33);
   VIF[26] = triangle(22,31,38);
   VIF[27] = square(17,22,33,38);
   VIF[28] = pentagon(33,38,42,45,47);
   VIF[29] = square(24,33,34,42);
   VIF[30] = square(10,15,17,24);
   VIF[31] = triangle(34,42,46);
   VIF[32] = triangle(5,10,15);
   VIF[33] = pentagon(21,26,31,36,40);
   VIF[34] = square(4,7,9,13);
   VIF[35] = square(29,34,39,46);
   VIF[36] = square(5,8,15,19);
   VIF[37] = triangle(12,20,26);
   VIF[38] = square(20,26,30,36);
   VIF[39] = pentagon(39,43,46,48,49);
   VIF[40] = pentagon(0,1,3,5,8);
   VIF[41] = square(6,12,16,20);
   VIF[42] = triangle(2,6,50);
   VIF[43] = decagon(30,35,36,40,41,44,45,47,48,49);
   VIF[44] = pentagon(16,20,25,30,35);
   VIF[45] = triangle(16,25,52);
   VIF[46] = square(6,16,50,52);
   VIF[47] = square(25,32,35,41);
   VIF[48] = triangle(27,32,54);
   VIF[49] = square(25,32,52,54);
   VIF[50] = pentagon(50,51,52,53,54);
   VIF[51] = square(18,27,53,54);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J78: metagyrate diminished rhombicosidodecahedron" << endl;
   return p;
}

  perl::Object bigyrate_diminished_rhombicosidodecahedron()
{
   perl::Object p = metabigyrate_rhombicosidodecahedron();

   p = diminish<double>(p,pentagon(15,19,24,29,34));
   IncidenceMatrix<> VIF(52,55);
   VIF[0] = pentagon(24,28,32,36,39);
   VIF[1] = square(17,24,25,32);
   VIF[2] = square(3,11,46,48);
   VIF[3] = triangle(11,17,48);
   VIF[4] = pentagon(11,14,17,21,25);
   VIF[5] = square(32,38,39,43);
   VIF[6] = triangle(25,32,38);
   VIF[7] = square(0,2,45,46);
   VIF[8] = triangle(0,3,46);
   VIF[9] = square(3,8,11,14);
   VIF[10] = square(21,25,34,38);
   VIF[11] = square(39,43,53,54);
   VIF[12] = pentagon(2,4,6,9,12);
   VIF[13] = pentagon(50,51,52,53,54);
   VIF[14] = square(18,23,26,31);
   VIF[15] = triangle(12,18,23);
   VIF[16] = square(26,31,50,51);
   VIF[17] = triangle(43,44,54);
   VIF[18] = square(9,12,19,23);
   VIF[19] = square(42,44,52,54);
   VIF[20] = square(4,7,9,13);
   VIF[21] = triangle(31,35,50);
   VIF[22] = square(37,41,42,44);
   VIF[23] = triangle(9,13,19);
   VIF[24] = triangle(40,42,52);
   VIF[25] = square(13,19,20,27);
   VIF[26] = pentagon(29,33,37,40,42);
   VIF[27] = triangle(20,27,33);
   VIF[28] = square(16,20,29,33);
   VIF[29] = square(27,33,35,40);
   VIF[30] = pentagon(7,10,13,16,20);
   VIF[31] = square(35,40,50,52);
   VIF[32] = pentagon(19,23,27,31,35);
   VIF[33] = triangle(1,4,7);
   VIF[34] = square(1,5,7,10);
   VIF[35] = decagon(5,8,10,14,16,21,29,34,37,41);
   VIF[36] = square(0,1,2,4);
   VIF[37] = pentagon(34,38,41,43,44);
   VIF[38] = pentagon(0,1,3,5,8);
   VIF[39] = triangle(26,30,51);
   VIF[40] = square(6,12,15,18);
   VIF[41] = triangle(2,6,45);
   VIF[42] = triangle(36,39,53);
   VIF[43] = square(30,36,51,53);
   VIF[44] = pentagon(15,18,22,26,30);
   VIF[45] = triangle(15,22,47);
   VIF[46] = square(6,15,45,47);
   VIF[47] = square(22,28,30,36);
   VIF[48] = triangle(24,28,49);
   VIF[49] = square(22,28,47,49);
   VIF[50] = pentagon(45,46,47,48,49);
   VIF[51] = square(17,24,48,49);

   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J79: bigyrate diminished rhombicosidodecahedron" << endl;
   return p;
}

  perl::Object parabidiminished_rhombicosidodecahedron()
{
   perl::Object p = diminished_rhombicosidodecahedron();
   p = diminish<QE>(p,pentagon(33,38,42,46,49));
   p = centralize<QE>(p);

   p.set_description() << "Johnson solid J80: parabidiminished rhombicosidodecahedron" << endl;
   return p;
}

  perl::Object metabidiminished_rhombicosidodecahedron()
{
   perl::Object p = diminished_rhombicosidodecahedron();
   p = diminish<QE>(p,pentagon(7,10,13,17,22));
   p = centralize<QE>(p);

   p.set_description() << "Johnson solid J81: metabidiminished rhombicosidodecahedron" << endl;
   return p;
}

perl::Object gyrate_bidiminished_rhombicosidodecahedron()
{
   perl::Object p = metabidiminished_rhombicosidodecahedron();
   p = rotate_facet(p,pentagon(34,38,42,45,47),M_PI/5);
   IncidenceMatrix<> VIF(42,50);
   VIF[0] = square(20,27,30,35);
   VIF[1] = pentagon(13,16,20,25,30);
   VIF[2] = square(35,38,40,43);
   VIF[3] = triangle(30,35,40);
   VIF[4] = square(6,10,13,16);
   VIF[5] = square(25,30,37,40);
   VIF[6] = pentagon(0,1,3,5,7);
   VIF[7] = pentagon(37,40,42,43,44);
   VIF[8] = square(0,1,2,4);
   VIF[9] = square(11,15,18,24);
   VIF[10] = triangle(7,11,15);
   VIF[11] = square(18,24,45,47);
   VIF[12] = square(5,7,12,15);
   VIF[13] = square(31,34,37,42);
   VIF[14] = decagon(1,4,5,8,12,17,19,26,28,33);
   VIF[15] = pentagon(12,15,19,24,29);
   VIF[16] = triangle(24,29,47);
   VIF[17] = square(29,36,47,49);
   VIF[18] = triangle(34,39,42);
   VIF[19] = square(19,28,29,36);
   VIF[20] = pentagon(28,33,36,39,41);
   VIF[21] = square(26,33,34,39);
   VIF[22] = triangle(36,41,49);
   VIF[23] = square(39,41,42,44);
   VIF[24] = pentagon(17,21,26,31,34);
   VIF[25] = square(41,44,48,49);
   VIF[26] = square(8,10,17,21);
   VIF[27] = triangle(43,44,48);
   VIF[28] = triangle(25,31,37);
   VIF[29] = triangle(10,16,21);
   VIF[30] = square(16,21,25,31);
   VIF[31] = pentagon(45,46,47,48,49);
   VIF[32] = pentagon(2,4,6,8,10);
   VIF[33] = square(38,43,46,48);
   VIF[34] = triangle(18,23,45);
   VIF[35] = square(3,7,9,11);
   VIF[36] = triangle(32,38,46);
   VIF[37] = square(23,32,45,46);
   VIF[38] = pentagon(9,11,14,18,23);
   VIF[39] = square(14,22,23,32);
   VIF[40] = pentagon(22,27,32,35,38);
   VIF[41] = decagon(0,2,3,6,9,13,14,20,22,27);


   p.take("VERTICES_IN_FACETS") << VIF;
   p = centralize<double>(p);
   p.set_description() << "Johnson solid J82: gyrate parabidiminished rhombicosidodecahedron" << endl;
   return p;
}

  perl::Object tridiminished_rhombicosidodecahedron()
{
   perl::Object p = metabidiminished_rhombicosidodecahedron();
   p = diminish<QE>(p,pentagon(39,43,46,48,49));
   p = centralize<QE>(p);

   p.set_description() << "Johnson solid J83: tridiminished rhombicosidodecahedron" << endl;
   return p;
}


perl::Object snub_disphenoid()
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

  IncidenceMatrix<> VIF(12,8);
  VIF[0] = triangle(4,6,7);
  VIF[1] = triangle(5,6,7);
  VIF[2] = triangle(0,1,3);
  VIF[3] = triangle(1,3,5);
  VIF[4] = triangle(3,5,7);
  VIF[5] = triangle(3,4,7);
  VIF[6] = triangle(0,3,4);
  VIF[7] = triangle(0,2,4);
  VIF[8] = triangle(1,2,5);
  VIF[9] = triangle(0,1,2);
  VIF[10] = triangle(2,5,6);
  VIF[11] = triangle(2,4,6);

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);
  p.set_description() << "Johnson solid J84: snub disphenoid" << endl;

  return p;
}

perl::Object snub_square_antiprism()
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

  IncidenceMatrix<> VIF(26,16);
  VIF[0] = triangle(9,13,14);
  VIF[1] = triangle(8,10,14);
  VIF[2] = triangle(8,9,14);
  VIF[3] = triangle(7,11,12);
  VIF[4] = triangle(5,10,12);
  VIF[5] = triangle(5,7,12);
  VIF[6] = square(2,5,8,10);
  VIF[7] = triangle(2,3,8);
  VIF[8] = triangle(0,4,6);
  VIF[9] = triangle(0,3,6);
  VIF[10] = triangle(0,1,4);
  VIF[11] = triangle(0,2,3);
  VIF[12] = triangle(0,1,2);
  VIF[13] = triangle(1,2,5);
  VIF[14] = triangle(1,4,7);
  VIF[15] = triangle(1,5,7);
  VIF[16] = triangle(3,6,9);
  VIF[17] = triangle(3,8,9);
  VIF[18] = triangle(4,7,11);
  VIF[19] = triangle(6,9,13);
  VIF[20] = square(4,6,11,13);
  VIF[21] = triangle(11,12,15);
  VIF[22] = triangle(11,13,15);
  VIF[23] = triangle(10,12,15);
  VIF[24] = triangle(10,14,15);
  VIF[25] = triangle(13,14,15);

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);
  p.set_description() << "Johnson solid J85: snub square antiprism" << endl;

  return p;
}

//FIXME: coordinates #830
perl::Object sphenocorona()
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

  IncidenceMatrix<> VIF(14,10);
  VIF[0] = triangle(6,7,9);
  VIF[1] = triangle(0,7,9);
  VIF[2] = triangle(1,2,8);
  VIF[3] = triangle(2,3,8);
  VIF[4] = triangle(0,4,9);
  VIF[5] = triangle(3,4,9);
  VIF[6] = triangle(2,3,4);
  VIF[7] = square(0,1,2,4);
  VIF[8] = triangle(3,6,9);
  VIF[9] = triangle(3,6,8);
  VIF[10] = triangle(5,6,8);
  VIF[11] = triangle(1,5,8);
  VIF[12] = triangle(5,6,7);
  VIF[13] = square(0,1,5,7);

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;

  p = centralize<double>(p);
  p.set_description() << "Johnson solid J86: Sphenocorona" << endl;

  return p;
}

//FIXME: coordinates #830
perl::Object augmented_sphenocorona()
{
  perl::Object p = sphenocorona();
  p = augment(p,square(0,1,2,4));

  IncidenceMatrix<> VIF(17,11);
  VIF[0] = triangle(6,7,9);
  VIF[1] = triangle(0,7,9);
  VIF[2] = triangle(1,2,8);
  VIF[3] = triangle(2,3,8);
  VIF[4] = triangle(0,4,9);
  VIF[5] = triangle(0,4,10);
  VIF[6] = triangle(2,4,10);
  VIF[7] = triangle(2,3,4);
  VIF[8] = triangle(3,4,9);
  VIF[9] = triangle(1,2,10);
  VIF[10] = triangle(0,1,10);
  VIF[11] = triangle(3,6,9);
  VIF[12] = triangle(3,6,8);
  VIF[13] = triangle(5,6,8);
  VIF[14] = triangle(1,5,8);
  VIF[15] = triangle(5,6,7);
  VIF[16] = square(0,1,5,7);

  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);
  p.set_description() << "Johnson solid J87: Augmented sphenocorona" << endl;

  return p;
}

perl::Object sphenomegacorona()
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

  IncidenceMatrix<> VIF(18,12);
  VIF[0] = square(5,7,9,10);
  VIF[1] = triangle(6,7,10);
  VIF[2] = triangle(6,8,10);
  VIF[3] = triangle(2,6,8);
  VIF[4] = triangle(2,4,8);
  VIF[5] = triangle(1,2,6);
  VIF[6] = triangle(0,2,4);
  VIF[7] = triangle(0,1,3);
  VIF[8] = triangle(0,1,2);
  VIF[9] = triangle(0,3,5);
  VIF[10] = square(0,4,5,9);
  VIF[11] = triangle(1,3,7);
  VIF[12] = triangle(3,5,7);
  VIF[13] = triangle(1,6,7);
  VIF[14] = triangle(4,8,11);
  VIF[15] = triangle(4,9,11);
  VIF[16] = triangle(8,10,11);
  VIF[17] = triangle(9,10,11);

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);
  p.set_description() << "Johnson solid J88: Sphenomegacorona" << endl;

  return p;
}

perl::Object hebesphenomegacorona()
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

  IncidenceMatrix<> VIF(21,14);
  VIF[0] = triangle(6,11,12);
  VIF[1] = triangle(4,9,10);
  VIF[2] = square(7,8,12,13);
  VIF[3] = triangle(4,5,10);
  VIF[4] = triangle(1,7,8);
  VIF[5] = triangle(1,2,8);
  VIF[6] = triangle(0,2,4);
  VIF[7] = triangle(0,1,3);
  VIF[8] = triangle(0,1,2);
  VIF[9] = triangle(0,4,5);
  VIF[10] = triangle(1,3,7);
  VIF[11] = triangle(2,4,9);
  VIF[12] = triangle(2,8,9);
  VIF[13] = triangle(5,10,11);
  VIF[14] = triangle(10,11,13);
  VIF[15] = triangle(8,9,13);
  VIF[16] = triangle(9,10,13);
  VIF[17] = triangle(11,12,13);
  VIF[18] = square(3,6,7,12);
  VIF[19] = square(0,3,5,6);
  VIF[20] = triangle(5,6,11);

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);
  p.set_description() << "Johnson solid J89: Hebesphenomegacorona" << endl;

  return p;
}

perl::Object disphenocingulum()
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

  IncidenceMatrix<> VIF(24,16);
  VIF[0] = triangle(10,12,14);
  VIF[1] = triangle(8,11,14);
  VIF[2] = triangle(8,10,14);
  VIF[3] = triangle(5,10,12);
  VIF[4] = triangle(5,7,12);
  VIF[5] = triangle(3,8,10);
  VIF[6] = triangle(3,5,10);
  VIF[7] = triangle(0,1,2);
  VIF[8] = triangle(1,2,6);
  VIF[9] = triangle(0,1,4);
  VIF[10] = square(0,3,4,8);
  VIF[11] = square(0,2,3,5);
  VIF[12] = triangle(2,5,7);
  VIF[13] = triangle(2,6,7);
  VIF[14] = triangle(1,4,9);
  VIF[15] = triangle(1,6,9);
  VIF[16] = triangle(4,8,11);
  VIF[17] = triangle(4,9,11);
  VIF[18] = triangle(6,7,13);
  VIF[19] = triangle(7,12,13);
  VIF[20] = square(6,9,13,15);
  VIF[21] = triangle(9,11,15);
  VIF[22] = square(12,13,14,15);
  VIF[23] = triangle(11,14,15);

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);
  p.set_description() << "Johnson solid J90: Disphenocingulum" << endl;

  return p;
}


perl::Object bilunabirotunda()
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

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p = centralize<QE>(p);
  p.set_description() << "Johnson solid J91: bilunabirotunda" << endl;

  return p;
}

//FIXME: coordinates #830
perl::Object triangular_hebesphenorotunda()
{
  //coordiantes from https://en.wikipedia.org/wiki/Triangular_hebesphenorotunda
  Rational s(1,2);
  const double r = QE(s,s,5).to_double();
  const double t = QE(0,1,3).to_double();
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

  IncidenceMatrix<> VIF(20,18);
  VIF[0] = triangle(11,13,16);
  VIF[1] = triangle(1,8,13);
  VIF[2] = triangle(1,3,8);
  VIF[3] = triangle(3,8,14);
  VIF[4] = triangle(2,5,7);
  VIF[5] = triangle(5,7,12);
  VIF[6] = triangle(12,14,17);
  VIF[7] = triangle(2,7,10);
  VIF[8] = triangle(15,16,17);
  VIF[9] = triangle(9,10,15);
  VIF[10] = triangle(0,6,9);
  VIF[11] = triangle(0,4,6);
  VIF[12] = triangle(4,6,11);
  VIF[13] = square(1,4,11,13);
  VIF[14] = square(3,5,12,14);
  VIF[15] = square(0,2,9,10);
  VIF[16] = pentagon(8,13,14,16,17);
  VIF[17] = pentagon(7,10,12,15,17);
  VIF[18] = pentagon(6,9,11,15,16);
  VIF[19] = hexagon(0,1,2,3,4,5);

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  p = centralize<double>(p);
  p.set_description() << "Johnson solid J92: triangular hebesphenorotunda" << endl;

  return p;
}

namespace {

struct dispatcher_t {
   typedef perl::Object (*fptr)();

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

}

perl::Object johnson_int(int n)
{
  const size_t index(n-1);
  if (index < sizeof(dispatcher)/sizeof(dispatcher[0]))
    return dispatcher[index].func();
  else
    throw std::runtime_error("invalid index of Johnson polytope");
}

perl::Object johnson_str(std::string s)
{
  typedef hash_map<std::string, dispatcher_t::fptr> func_map_t;
  static const func_map_t func_map(attach_operation(array2container(dispatcher), dispatcher_t::to_map_value()).begin(),
                                   attach_operation(array2container(dispatcher), dispatcher_t::to_map_value()).end());
  const func_map_t::const_iterator it=func_map.find(s);
  if (it != func_map.end())
    return (it->second)();
  else
    throw std::runtime_error("unknown name of Johnson polytope");
}

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create Johnson solid number n."
                  "# @param Int n the index of the desired Johnson polytope"
                  "# @return Polytope",
                  &johnson_int, "johnson_solid(Int)");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create Johnson solid with the given name."
                  "# Some polytopes are realized with floating point numbers and thus not exact;"
                  "# Vertex-facet-incidences are correct in all cases."
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
                  "# @value s 'pentagonal_orthobirotunda' Pentagonal orthobirotunda with regular facets. Johnson solid J32."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_triangular_orthbicupola' Elongated triangular orthobicupola with regular facets. Johnson solid J35."
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
                  "# @value s 'triminished_rhombicosidodecahedron' Tridiminished rhombicosidodecahedron with regular facets. Johnson solid J83."
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
