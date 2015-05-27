#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Matrix.h"
#include "polymake/SparseVector.h"
#include "polymake/IncidenceMatrix.h"
#include <cmath>

namespace polymake { namespace polytope {

typedef QuadraticExtension<Rational> QE;

namespace {

 template<typename T>
 Matrix<T> create_square_vertices()
 {
   Matrix<T> V(4,3);

   V(0,0)=V(1,0)=V(2,0)=V(3,0)=V(1,1)=V(2,2)=V(3,1)=V(3,2)=1;
   V(1,2)=V(2,1)=V(0,1)=V(0,2)=-1;

   return V;
 }

 //vertices of lattice hexagon
 template<typename T>
 Matrix<T> create_hexagon_vertices()
 {
  //vertices: {0,0,0}, {7,-2,1},{12,3,3}, {10,10,4}, {3,12,3},{-2,7,1}
  //barycentre: {5,5,2}
  //side length: 3*sqrt(6)
    Matrix<T> V(6,4);
    V.col(0).fill(1);

    V(1,1)=V(2,2)=7;
    V(1,2)=V(2,1)=-2;
    V(1,3)=V(2,3)=1;
    V(3,2)=V(3,3)=V(4,1)=V(4,3)=3;
    V(3,1)=V(4,2)=12;
    V(5,1)=V(5,2)=10;
    V(5,3)=4;

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

//FIXME: coordinates #830
//creates vertices of regular n-gon with the origin as center and first vertex x
  Matrix<double> create_regular_polygon_vertices(int n, Vector<double> x){
   if(x[0]==0 && x[1]==0)
      throw std::runtime_error("Vertex cannot be in the center (0,0)");
   double r = sqrt(x[0]*x[0]+x[1]*x[1]); //radius of polygon = vector length
   double s = acos(x[0]/r); //angle of first vertex
   return create_regular_polygon_vertices(n, r, s);
 }

 template <typename T>
 perl::Object assign_common_props(perl::Object p)
 {
  p.take("LINEALITY_SPACE") << Matrix<T>();
  p.take("AFFINE_HULL") << Matrix<T>();
  p.take("FEASIBLE") << true;
  p.take("BOUNDED") << true;
  p.take("CONE_DIM") << 4;
  p.take("CONE_AMBIENT_DIM") << 4;
  return p;
 }

//creates a prism over a n-gon with center 0, where n=3,5,6,8,10 and z-coordinates z_1 and z_2
perl::Object n_gonal_prism(int n, QE z_1, QE z_2)
{
  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  Matrix<QE> V(2*n,4);
  V.col(0).fill(1);
  for (int i=0; i<n; ++i)
    V(i,3) = z_1;
  for (int i=8; i<2*n; ++i)
    V(i,3) = z_2;

  switch (n)
  {
  case 3: {

    QE height(0,Rational(1,2),3);

    V(0,2)=V(3,2)=height;
    V(1,1)=V(4,1)=1;
    V(1,2)=V(2,2)=V(4,2)=V(5,2)=-height;
    V(2,1)=V(5,1)=-1;

    break;
  }

  case 5:

    //...
    break;

  case 6:
    
		V(0,2)=V(0,3)=V(1,1)=V(1,2)=V(3,1)=V(5,3)=1;
    V(2,3)=V(3,2)=V(3,3)=V(4,1)=V(4,2)=V(5,1)=-1;

    break;

  case 8: {

    QE q(1,1,2);

    V(0,1)=V(1,2)=V(3,1)=V(6,2)=V(8,1)=V(9,2)=V(11,1)=V(14,2)=1;
    V(2,2)=V(4,1)=V(5,2)=V(7,1)=V(10,2)=V(12,1)=V(13,2)=V(15,1)=-1;
    V(0,2)=V(1,1)=V(2,1)=V(7,2)=V(8,2)=V(9,1)=V(10,1)=V(15,2)=q;
    V(3,2)=V(4,2)=V(5,1)=V(6,1)=V(11,2)=V(12,2)=V(13,1)=V(14,1)=-q;

    break;
  }

  case 10:
    //...
    break;

  default:
    throw std::runtime_error("Invalid number of vertices of the n-gonal prism.");
  }

  IncidenceMatrix<> VIF(2+n,2*n);

  for (int i=0; i<n; ++i) {
    VIF(0,i)=VIF(1,i+8)=1;
    VIF(i+2,i)=VIF(i+2,(i+1)%n)=VIF(i+2,i+8)=VIF(i+2,(i+1)%n+8)=1;
  }

   p.take("VERTICES") << V;

   p.take("VERTICES_IN_FACETS") << VIF;

   return p;
}

double norm(Vector<double> v){
  double n = 0;
  for(int i = 0; i<v.size(); i++){
    n = n + v[i]*v[i];
  }
  return sqrt(n);
}
 
//augments the n-th facet
//uses sqrt and cos to calculate height, thus not exact
perl::Object augment(perl::Object p, int n){

  int d = p.give("CONE_DIM");
  Matrix<double> V = (p.give("VERTICES"));
  IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");
  Matrix<double> F = p.give("FACETS");

  perl::Object facet(perl::ObjectType::construct<double>("Polytope"));

  if(d==4){
    Matrix<double> FV = V.minor(VIF.row(n),All);
    facet.take("VERTICES")<<FV;
  }else{
    facet.take("VERTICES")<<V;
  }

  Matrix<double> FV = facet.give("VERTICES");
  IncidenceMatrix<> FVIF = facet.give("VERTICES_IN_FACETS");
  Matrix<double> neighbors = FV.minor(FVIF.row(0),All); //two neighbor vertices
  double side_length = norm(neighbors[0]-neighbors[1]);
  int n_vert = facet.give("N_VERTICES");
  double height = side_length*sqrt(1-1/(2-2*cos(2*M_PI/n_vert)));

  Vector<double> bary = average(rows(FV));

  perl::Object p_out(perl::ObjectType::construct<double>("Polytope"));

  if(d==4){
    Vector<double> normal = F.row(n);
    if(normal[0]!=0){
          normal = normal/normal[0];
          normal[0]=0;
    }
    normal = height*normal/norm(normal); //facet normal of length height

    p_out.take("VERTICES") << (V / (bary - normal));
  }else{
    p_out.take("VERTICES") << (V | zero_vector<double>()) / (bary | height);
  }
  return p_out;
}

//elongates the n-th facet
//uses sqrt to calculate side length, thus not exact
perl::Object elongate(perl::Object p, int n){

  int d = p.give("CONE_DIM");
  Matrix<double> V = (p.give("VERTICES"));
  IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");
  Matrix<double> F = p.give("FACETS");

  perl::Object facet(perl::ObjectType::construct<double>("Polytope"));

  if(d==4){
    Matrix<double> FV = V.minor(VIF.row(n),All);
    facet.take("VERTICES")<<FV;
  }else{
    facet.take("VERTICES")<<V;
  }

  Matrix<double> FV = facet.give("VERTICES");
  IncidenceMatrix<> FVIF = facet.give("VERTICES_IN_FACETS");
  Matrix<double> neighbors = FV.minor(FVIF.row(0),All); //two neighbor vertices
  double side_length = norm(neighbors[0]-neighbors[1]);
  int n_vert = facet.give("N_VERTICES");

  perl::Object p_out(perl::ObjectType::construct<double>("Polytope"));

  if(d==4){
    Vector<double> normal = F.row(n);
    if(normal[0]!=0){
          normal = normal/normal[0];
          normal[0]=0;
    }
    normal = side_length*normal/norm(normal); //facet normal of length side_length
    p_out.take("VERTICES") << (V / (FV - repeat_row(normal, n_vert)));
  }else{
    p_out.take("VERTICES") << (V | zero_vector<double>()) / (V | same_element_vector<double>(side_length, n_vert));
  }
  return p_out;
}
  
//creates regular n-gonal prism   
perl::Object create_prism(int n)
{
  Matrix<double> V = create_regular_polygon_vertices(n, 1, 0);
  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.take("VERTICES") << V;
  return elongate(p,0);

}
   
Matrix<QE> truncated_cube_vertices()
{
  Matrix<QE> V = n_gonal_prism(8,QE(0,0,0),QE(2,2,2)).give("VERTICES");
  Matrix<QE> W(8,4);
	W.col(0).fill(1);
	W(0,1)=W(1,1)=W(2,2)=W(3,2)=QE(2,1,2);
	W(4,1)=W(5,1)=W(6,2)=W(7,2)=-QE(2,1,2);
	W(0,3)=W(2,3)=W(4,3)=W(6,3)=QE(0,1,2);
	W(1,3)=W(3,3)=W(5,3)=W(7,3)=QE(2,1,2);

  return V / W;
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
  p.set_description() << "Johnson solid J1: Square pyramid" << endl;
  p.take("VERTICES") << V;
  assign_common_props<QE>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object pentagonal_pyramid()
{
  Vector<double> tip(4);
  tip[0]=1;
  tip[1]=tip[2]=0;
  tip[3]=(sqrt(5)-1)/2;

  Matrix<double> V((create_regular_polygon_vertices(5,unit_vector<double>(2,0,1)) | zero_vector<double>(5)) / tip);

  IncidenceMatrix<> VIF(6,6);
  VIF(0,1)=VIF(0,2)=VIF(0,5)=1;
  VIF(1,3)=VIF(1,4)=VIF(1,5)=1;
  VIF(2,2)=VIF(2,3)=VIF(2,5)=1;
  VIF(3,0)=VIF(3,4)=VIF(3,5)=1;
  VIF(4,0)=VIF(4,1)=VIF(4,5)=1;
  VIF(5,0)=VIF(5,1)=VIF(5,2)=VIF(5,3)=VIF(5,4)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J2: Pentagonal pyramid" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS");
  assign_common_props<double>(p);

  return p;
}

perl::Object triangular_cupola()
{
  //height: 6

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

  Matrix<QE> V=( create_hexagon_vertices<QE>() / (triangle_V + repeat_row(6*trans,3)));

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.set_description() << "Johnson solid J3: Triangular cupola" << endl;
  p.take("VERTICES") << V;
  assign_common_props<QE>(p);

  return p;
}

perl::Object square_cupola()
{
  perl::Object base = n_gonal_prism(8,QE(0,0,0),QE(1,0,0));

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
        p.set_description() << "Johnson solid J4: Square cupola" << endl;
        p.take("VERTICES") << V;
        assign_common_props<QE>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object pentagonal_cupola()
{
  double r = sqrt((8+sqrt(5)+2*sqrt(6-2*sqrt(3)))/4); //radius of decagon
  double s = 2*M_PI/4; //angle of decagon vertex
  double h = (sqrt(5)-1)/2;

  Matrix<double> V((create_regular_polygon_vertices(5,1,0) | same_element_vector<double>(h,5)) / (create_regular_polygon_vertices(10,r,s) | same_element_vector<double>(0,10)) );

  IncidenceMatrix<> VIF(12,15);
  VIF(0,0)=VIF(0,4)=VIF(0,11)=VIF(0,12)=1;
  VIF(1,4)=VIF(1,10)=VIF(1,11)=1;
  VIF(2,1)=VIF(2,2)=VIF(2,5)=VIF(2,6)=1;
  VIF(3,2)=VIF(3,3)=VIF(3,7)=VIF(3,8)=1;
  VIF(4,2)=VIF(4,6)=VIF(4,7)=1;
  VIF(5,3)=VIF(5,8)=VIF(5,9)=1;
  VIF(6,3)=VIF(6,4)=VIF(6,9)=VIF(6,10)=1;
  VIF(7,0)=VIF(7,1)=VIF(7,2)=VIF(7,3)=VIF(7,4)=1;
  VIF(8,1)=VIF(8,5)=VIF(8,14)=1;
  VIF(9,0)=VIF(9,12)=VIF(9,13)=1;
  VIF(10,0)=VIF(10,1)=VIF(10,13)=VIF(10,14)=1;
  VIF(11,5)=VIF(11,6)=VIF(11,7)=VIF(11,8)=VIF(11,9)=VIF(11,10)=VIF(11,11)=VIF(11,12)=VIF(11,13)=VIF(11,14)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J5: Pentagonal cupola" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS")<<VIF;
  assign_common_props<double>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object pentagonal_rotunda()
{
  double a = sqrt((5-sqrt(5))/2); //side length
  double r = a*(1+sqrt(5))/2; //radius of decagon
  double h = a*sqrt(1+2/sqrt(5)); //height
  double rp = (1+sqrt(5))/2; //radius of inner pentagon


  Matrix<double> V((create_regular_polygon_vertices(5,1,0) | same_element_vector<double>(h,5))/ (create_regular_polygon_vertices(5,rp,M_PI/5) | same_element_vector<double>(1,5)) / (create_regular_polygon_vertices(10,r,M_PI/10) | same_element_vector<double>(0,10)) );

  IncidenceMatrix<> VIF(17,20);
  VIF(0,9)=VIF(0,18)=VIF(0,19)=1;VIF(1,4)=VIF(1,8)=VIF(1,9)=VIF(1,17)=VIF(1,18)=1;VIF(2,0)=VIF(2,4)=VIF(2,9)=1;VIF(3,1)=VIF(3,2)=VIF(3,6)=1;VIF(4,6)=VIF(4,12)=VIF(4,13)=1;VIF(5,2)=VIF(5,6)=VIF(5,7)=VIF(5,13)=VIF(5,14)=1;VIF(6,2)=VIF(6,3)=VIF(6,7)=1;VIF(7,7)=VIF(7,14)=VIF(7,15)=1;VIF(8,3)=VIF(8,7)=VIF(8,8)=VIF(8,15)=VIF(8,16)=1;VIF(9,8)=VIF(9,16)=VIF(9,17)=1;VIF(10,3)=VIF(10,4)=VIF(10,8)=1;VIF(11,0)=VIF(11,1)=VIF(11,2)=VIF(11,3)=VIF(11,4)=1;VIF(12,0)=VIF(12,1)=VIF(12,5)=1;VIF(13,1)=VIF(13,5)=VIF(13,6)=VIF(13,11)=VIF(13,12)=1;VIF(14,5)=VIF(14,10)=VIF(14,11)=1;VIF(15,10)=VIF(15,11)=VIF(15,12)=VIF(15,13)=VIF(15,14)=VIF(15,15)=VIF(15,16)=VIF(15,17)=VIF(15,18)=VIF(15,19)=1;VIF(16,0)=VIF(16,5)=VIF(16,9)=VIF(16,10)=VIF(16,19)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J6: Pentagonal rotunda" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS")<<VIF;
  assign_common_props<double>(p);

  return p;
}

perl::Object elongated_triangular_pyramid()
{
  QE c(Rational(-1,3),0,0);

  QE s(0,Rational(1,3),6);

  Matrix<QE> V( ones_vector<QE>(7) | (same_element_vector<QE>(c,3) / unit_matrix<QE>(3) / (unit_matrix<QE>(3) + same_element_matrix<QE>(s , 3, 3))));

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.set_description() << "Johnson solid J7: Elongated triangular bipyramid" << endl;
  p.take("VERTICES") << V;
  assign_common_props<QE>(p);

  return p;
}

perl::Object elongated_square_pyramid()
{
  Matrix<QE> square_vertices =  create_square_vertices<QE>();

  Vector<QE> tip(4);
  tip[0]=1;
  tip[1]=tip[2]=0;
  tip[3]=QE(0,1,2);

  Matrix<QE> V( ((square_vertices | zero_vector<QE>(4)) / (square_vertices | -2*ones_vector<QE>(4))) / tip );

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.set_description() << "Johnson solid J8: Elongated square pyramid" << endl;
  p.take("VERTICES") << V;
  assign_common_props<QE>(p);

  return p;
}

//FIXME: #830 coordinates
perl::Object elongated_pentagonal_pyramid()
{
  perl::Object p = pentagonal_pyramid();
  p = elongate(p,5);

  IncidenceMatrix<> VIF(11,11);
  VIF(0,6)=VIF(0,7)=VIF(0,8)=VIF(0,9)=VIF(0,10)=1;
  VIF(1,2)=VIF(1,3)=VIF(1,5)=1;
  VIF(2,2)=VIF(2,3)=VIF(2,8)=VIF(2,9)=1;
  VIF(3,3)=VIF(3,4)=VIF(3,5)=1;
  VIF(4,3)=VIF(4,4)=VIF(4,9)=VIF(4,10)=1;
  VIF(5,1)=VIF(5,2)=VIF(5,5)=1;
  VIF(6,1)=VIF(6,2)=VIF(6,7)=VIF(6,8)=1;
  VIF(7,0)=VIF(7,1)=VIF(7,5)=1;
  VIF(8,0)=VIF(8,4)=VIF(8,5)=1;
  VIF(9,0)=VIF(9,4)=VIF(9,6)=VIF(9,10)=1;
  VIF(10,0)=VIF(10,1)=VIF(10,6)=VIF(10,7)=1;

  p.set_description() << "Johnson solid J9: Elongated pentagonal pyramid" << endl;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<double>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object gyroelongated_square_pyramid()
{
  Matrix<double> square_vertices =  create_square_vertices<double>();

  Matrix<double> rotation(3,2);

  double c = sqrt(2)/2;

  rotation(0,0)=rotation(0,1)=0;
  rotation(1,0)=rotation(2,0)=rotation(2,1)=c;
  rotation(1,1)=-c;

  Matrix<double> rotated_square_vertices = square_vertices * ( unit_vector<double>(3,0,1) | rotation);

  Vector<double> tip(4);
  tip[0]=1;
  tip[1]=tip[2]=0;
  tip[3]=sqrt(2);

  double s = sqrt(sqrt(8));

  Matrix<double> V( ((square_vertices | zero_vector<double>(4)) / (rotated_square_vertices | -same_element_vector<double>(s,4))) / tip );

  IncidenceMatrix<> VIF(13,9);

  VIF(4,0)=VIF(5,0)=VIF(6,0)=VIF(7,0)=VIF(8,0)=1;
  VIF(0,1)=VIF(7,1)=VIF(8,1)=VIF(9,1)=VIF(11,1)=1;
  VIF(1,2)=VIF(2,2)=VIF(3,2)=VIF(4,2)=VIF(6,2)=1;
  VIF(0,3)=VIF(1,3)=VIF(2,3)=VIF(10,3)=VIF(11,3)=1;
  VIF(3,4)=VIF(4,4)=VIF(5,4)=VIF(12,4)=1;
  VIF(5,5)=VIF(7,5)=VIF(9,5)=VIF(12,5)=1;
  VIF(1,6)=VIF(3,6)=VIF(10,6)=VIF(12,6)=1;
  VIF(9,7)=VIF(10,7)=VIF(11,7)=VIF(12,7)=1;
  VIF(0,8)=VIF(2,8)=VIF(6,8)=VIF(8,8)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J10: Gyroelongated square pyramid" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<double>(p);

  return p;
}

perl::Object gyroelongated_pentagonal_pyramid()
{
  perl::Object p = pentagonal_pyramid();

  Matrix<QE> V = p.give("VERTICES");

  double height = -sqrt(6*(5-sqrt(5)))/4;

  V /= (create_regular_polygon_vertices(5, 1, M_PI/5) | same_element_vector<double>(height,5));

  IncidenceMatrix<> VIF(16,11);
  VIF(0,6)=VIF(0,7)=VIF(0,8)=VIF(0,9)=VIF(0,10)=1;
  VIF(1,1)=VIF(1,6)=VIF(1,7)=1;
  VIF(2,1)=VIF(2,2)=VIF(2,5)=1;
  VIF(3,1)=VIF(3,2)=VIF(3,7)=1;
  VIF(4,2)=VIF(4,3)=VIF(4,5)=1;
  VIF(5,2)=VIF(5,3)=VIF(5,8)=1;
  VIF(6,3)=VIF(6,8)=VIF(6,9)=1;
  VIF(7,2)=VIF(7,7)=VIF(7,8)=1;
  VIF(8,3)=VIF(8,4)=VIF(8,9)=1;
  VIF(9,3)=VIF(9,4)=VIF(9,5)=1;
  VIF(10,4)=VIF(10,9)=VIF(10,10)=1;
  VIF(11,0)=VIF(11,4)=VIF(11,5)=1;
  VIF(12,0)=VIF(12,1)=VIF(12,5)=1;
  VIF(13,0)=VIF(13,4)=VIF(13,10)=1;
  VIF(14,0)=VIF(14,1)=VIF(14,6)=1;
  VIF(15,0)=VIF(15,6)=VIF(15,10)=1;

  p.set_description() << "Johnson solid J11: Gyroelongated pentagonal pyramid" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}

perl::Object triangular_bipyramid()
{
  Rational c(-1,3);

  Matrix<Rational> V( ones_vector<Rational>(5) | unit_matrix<Rational>(3) / ones_vector<Rational>(3) / same_element_vector<Rational>(c,3));
  
  perl::Object p(perl::ObjectType::construct<Rational>("Polytope"));

  p.set_description() << "Johnson solid J12: Triangular bipyramid" << endl;
  p.take("VERTICES") << V;
  assign_common_props<Rational>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object pentagonal_bipyramid()
{
  perl::Object p = pentagonal_pyramid();
  p = augment(p,5);

  IncidenceMatrix<> VIF(10,7);
  VIF(0,1)=VIF(0,2)=VIF(0,6)=1;
  VIF(1,1)=VIF(1,2)=VIF(1,5)=1;
  VIF(2,2)=VIF(2,3)=VIF(2,6)=1;
  VIF(3,2)=VIF(3,3)=VIF(3,5)=1;
  VIF(4,3)=VIF(4,4)=VIF(4,5)=1;
  VIF(5,3)=VIF(5,4)=VIF(5,6)=1;
  VIF(6,0)=VIF(6,4)=VIF(6,6)=1;
  VIF(7,0)=VIF(7,4)=VIF(7,5)=1;
  VIF(8,0)=VIF(8,1)=VIF(8,5)=1;
  VIF(9,0)=VIF(9,1)=VIF(9,6)=1;
  p.take("VERTICES_IN_FACETS") << VIF;
  p.set_description() << "Johnson solid J13: Pentagonal bipyramid" << endl;
  assign_common_props<QE>(p);

  return p;

}

perl::Object elongated_triangular_bipyramid()
{
  QE c(Rational(-1,3),0,0);

  QE s(0,Rational(1,3),6);

  Matrix<QE> V( ones_vector<QE>(8) | ( same_element_vector<QE>(1+s,3) / same_element_vector<QE>(c,3) / unit_matrix<QE>(3) / (unit_matrix<QE>(3) + same_element_matrix<QE>(s , 3, 3))));

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.set_description() << "Johnson solid J14: Elongated triangular bipyramid" << endl;
  p.take("VERTICES") << V;
  assign_common_props<QE>(p);

  return p;
}

perl::Object elongated_square_bipyramid()
{
  perl::Object esp = elongated_square_pyramid();

  Matrix<QE> esp_vertices =  esp.give("VERTICES");

  Vector<QE> tip(4);
  tip[0]=1;
  tip[1]=tip[2]=0;
  tip[3]=QE(-2,-1,2);

  Matrix<QE> V = ( esp_vertices / tip);

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.set_description() << "Johnson solid J15: Elongated square bipyramid" << endl;
  p.take("VERTICES") << V;
  assign_common_props<QE>(p);

  return p;

}

//FIXME: coordinates #830
perl::Object elongated_pentagonal_bipyramid()
{
  perl::Object p = elongated_pentagonal_pyramid();
  p = augment(p,0);

  IncidenceMatrix<> VIF(15,12);
  VIF(0,6)=VIF(0,7)=VIF(0,11)=1; //V[0]=Set<int>(6,7,11);
  VIF(1,6)=VIF(1,10)=VIF(1,11)=1;
  VIF(2,7)=VIF(2,8)=VIF(2,11)=1;
  VIF(3,9)=VIF(3,10)=VIF(3,11)=1;
  VIF(4,2)=VIF(4,3)=VIF(4,8)=VIF(4,9)=1;
  VIF(5,8)=VIF(5,9)=VIF(5,11)=1;
  VIF(6,2)=VIF(6,3)=VIF(6,5)=1;
  VIF(7,3)=VIF(7,4)=VIF(7,5)=1;
  VIF(8,3)=VIF(8,4)=VIF(8,9)=VIF(8,10)=1;
  VIF(9,1)=VIF(9,2)=VIF(9,5)=1;
  VIF(10,1)=VIF(10,2)=VIF(10,7)=VIF(10,8)=1;
  VIF(11,0)=VIF(11,1)=VIF(11,5)=1;
  VIF(12,0)=VIF(12,4)=VIF(12,5)=1;
  VIF(13,0)=VIF(13,4)=VIF(13,6)=VIF(13,10)=1;
  VIF(14,0)=VIF(14,1)=VIF(14,6)=VIF(14,7)=1;

  p.take("VERTICES_IN_FACETS") << VIF;
  p.set_description() << "Johnson solid J16: Elongated pentagonal bipyramid" << endl;
  assign_common_props<QE>(p);

  return p;

}

//FIXME: coordinates #830
perl::Object gyroelongated_square_bipyramid()
{
  Matrix<double> square_vertices =  create_square_vertices<double>();

  Matrix<double> rotation(3,2);

  double c = sqrt(2)/2;

  rotation(0,0)=rotation(0,1)=0;
  rotation(1,0)=rotation(2,0)=rotation(2,1)=c;
  rotation(1,1)=-c;

  Matrix<double> rotated_square_vertices = square_vertices * ( unit_vector<double>(3,0,1) | rotation);

  Vector<double> tip(4);
  tip[0]=1;
  tip[1]=tip[2]=0;
  tip[3]=sqrt(2);

  double s = sqrt(sqrt(8));

  Vector<double> tip2(4);
  tip2[0]=1;
  tip2[1]=tip2[2]=0;
  tip2[3]=-s-sqrt(2);

  Matrix<double> V( ((square_vertices | zero_vector<double>(4)) / (rotated_square_vertices | -same_element_vector<double>(s,4))) / tip / tip2 );

  IncidenceMatrix<> VIF(16,10);

  VIF(0,1)=VIF(0,3)=VIF(0,8)=1;
  VIF(1,2)=VIF(1,3)=VIF(1,6)=1;
  VIF(2,2)=VIF(2,3)=VIF(2,8)=1;
  VIF(3,4)=VIF(3,6)=VIF(3,9)=1;
  VIF(4,2)=VIF(4,4)=VIF(4,6)=1;
  VIF(5,4)=VIF(5,5)=VIF(5,9)=1;
  VIF(6,0)=VIF(6,4)=VIF(6,5)=1;
  VIF(7,0)=VIF(7,2)=VIF(7,4)=1;
  VIF(8,0)=VIF(8,2)=VIF(8,8)=1;
  VIF(9,0)=VIF(9,1)=VIF(9,5)=1;
  VIF(10,0)=VIF(10,1)=VIF(10,8)=1;
  VIF(11,5)=VIF(11,7)=VIF(11,9)=1;
  VIF(12,1)=VIF(12,5)=VIF(12,7)=1;
  VIF(13,6)=VIF(13,7)=VIF(13,9)=1;
  VIF(14,3)=VIF(14,6)=VIF(14,7)=1;
  VIF(15,1)=VIF(15,3)=VIF(15,7)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J17: Gyroelongated square bipyramid" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<double>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object elongated_triangular_cupola()
{
  Matrix<QE> hexa_V = create_hexagon_vertices<QE>();

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

  QE t(0,1,2);
  Vector<QE> trans_down(4); // normal vector side length
  trans_down[0]=0;
  trans_down[1]=trans_down[2]=-t;
  trans_down[3]=5*t;

  // this would work if QE was more mighty
  // QE c(0,3,6);
  // Vector<QE> trans_down = -trans * c;

  Matrix<QE> V=( hexa_V / (triangle_V + repeat_row(6*trans,3)) / (hexa_V - repeat_row(trans_down,6)));

  IncidenceMatrix<> VIF(14,15);
  VIF(0,3)=VIF(0,5)=VIF(0,7)=VIF(0,8)=1;
  VIF(1,1)=VIF(1,3)=VIF(1,7)=1;
  VIF(2,4)=VIF(2,5)=VIF(2,8)=1;
  VIF(3,0)=VIF(3,1)=VIF(3,6)=VIF(3,7)=1;
  VIF(4,2)=VIF(4,4)=VIF(4,6)=VIF(4,8)=1;
  VIF(5,0)=VIF(5,2)=VIF(5,6)=1;
  VIF(6,0)=VIF(6,2)=VIF(6,9)=VIF(6,11)=1;
  VIF(7,2)=VIF(7,4)=VIF(7,11)=VIF(7,13)=1;
  VIF(8,6)=VIF(8,7)=VIF(8,8)=1;
  VIF(9,0)=VIF(9,1)=VIF(9,9)=VIF(9,10)=1;
  VIF(10,4)=VIF(10,5)=VIF(10,13)=VIF(10,14)=1;
  VIF(11,9)=VIF(11,10)=VIF(11,11)=VIF(11,12)=VIF(11,13)=VIF(11,14)=1;
  VIF(12,1)=VIF(12,3)=VIF(12,10)=VIF(12,12)=1;
  VIF(13,3)=VIF(13,5)=VIF(13,12)=VIF(13,14)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J18: Elongated triangular cupola" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}


perl::Object elongated_square_cupola()
{
  perl::Object base = n_gonal_prism(8,QE(-2,0,0),QE(0,0,0));
  Matrix<QE> V =  base.give("VERTICES");
        Matrix<QE> T = square_cupola().give("VERTICES");
        V /= T.minor(sequence(8,4),All);

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
        p.set_description() << "Johnson solid J19: Elongated square cupola" << endl;
        p.take("VERTICES") << V;
        assign_common_props<QE>(p);

  return p;
}

//FIXME: coordinates #830
//FIXME: coordinates #830
perl::Object elongated_pentagonal_cupola()
{
  perl::Object p = pentagonal_cupola();
  p = elongate(p,11);

  IncidenceMatrix<> VIF(22,25);
  VIF(0,15)=VIF(0,16)=VIF(0,17)=VIF(0,18)=VIF(0,19)=VIF(0,20)=VIF(0,21)=VIF(0,22)=VIF(0,23)=VIF(0,24)=1;
  VIF(1,1)=VIF(1,2)=VIF(1,5)=VIF(1,6)=1;
  VIF(2,5)=VIF(2,6)=VIF(2,15)=VIF(2,16)=1;
  VIF(3,9)=VIF(3,10)=VIF(3,19)=VIF(3,20)=1;
  VIF(4,2)=VIF(4,3)=VIF(4,7)=VIF(4,8)=1;
  VIF(5,2)=VIF(5,6)=VIF(5,7)=1;
  VIF(6,6)=VIF(6,7)=VIF(6,16)=VIF(6,17)=1;
  VIF(7,8)=VIF(7,9)=VIF(7,18)=VIF(7,19)=1;
  VIF(8,7)=VIF(8,8)=VIF(8,17)=VIF(8,18)=1;
  VIF(9,3)=VIF(9,8)=VIF(9,9)=1;
  VIF(10,3)=VIF(10,4)=VIF(10,9)=VIF(10,10)=1;
  VIF(11,0)=VIF(11,1)=VIF(11,2)=VIF(11,3)=VIF(11,4)=1;
  VIF(12,4)=VIF(12,10)=VIF(12,11)=1;
  VIF(13,10)=VIF(13,11)=VIF(13,20)=VIF(13,21)=1;
  VIF(14,1)=VIF(14,5)=VIF(14,14)=1;
  VIF(15,5)=VIF(15,14)=VIF(15,15)=VIF(15,24)=1;
  VIF(16,0)=VIF(16,4)=VIF(16,11)=VIF(16,12)=1;
  VIF(17,11)=VIF(17,12)=VIF(17,21)=VIF(17,22)=1;
  VIF(18,0)=VIF(18,12)=VIF(18,13)=1;
  VIF(19,0)=VIF(19,1)=VIF(19,13)=VIF(19,14)=1;
  VIF(20,13)=VIF(20,14)=VIF(20,23)=VIF(20,24)=1;
  VIF(21,12)=VIF(21,13)=VIF(21,22)=VIF(21,23)=1;
  p.set_description() << "Johnson solid J20: Elongated pentagonal cupola" << endl;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object elongated_pentagonal_rotunda()
{
  perl::Object p = pentagonal_rotunda();
  p = elongate(p,15);

  IncidenceMatrix<> VIF(27,30);
  VIF(0,20)=VIF(0,21)=VIF(0,22)=VIF(0,23)=VIF(0,24)=VIF(0,25)=VIF(0,26)=VIF(0,27)=VIF(0,28)=VIF(0,29)=1;VIF(1,4)=VIF(1,8)=VIF(1,9)=VIF(1,17)=VIF(1,18)=1;VIF(2,0)=VIF(2,4)=VIF(2,9)=1;VIF(3,1)=VIF(3,2)=VIF(3,6)=1;VIF(4,12)=VIF(4,13)=VIF(4,22)=VIF(4,23)=1;VIF(5,6)=VIF(5,12)=VIF(5,13)=1;VIF(6,16)=VIF(6,17)=VIF(6,26)=VIF(6,27)=1;VIF(7,8)=VIF(7,16)=VIF(7,17)=1;VIF(8,2)=VIF(8,6)=VIF(8,7)=VIF(8,13)=VIF(8,14)=1;VIF(9,2)=VIF(9,3)=VIF(9,7)=1;VIF(10,13)=VIF(10,14)=VIF(10,23)=VIF(10,24)=1;VIF(11,15)=VIF(11,16)=VIF(11,25)=VIF(11,26)=1;VIF(12,14)=VIF(12,15)=VIF(12,24)=VIF(12,25)=1;VIF(13,7)=VIF(13,14)=VIF(13,15)=1;VIF(14,3)=VIF(14,7)=VIF(14,8)=VIF(14,15)=VIF(14,16)=1;VIF(15,3)=VIF(15,4)=VIF(15,8)=1;VIF(16,0)=VIF(16,1)=VIF(16,2)=VIF(16,3)=VIF(16,4)=1;VIF(17,17)=VIF(17,18)=VIF(17,27)=VIF(17,28)=1;VIF(18,11)=VIF(18,12)=VIF(18,21)=VIF(18,22)=1;VIF(19,0)=VIF(19,1)=VIF(19,5)=1;VIF(20,1)=VIF(20,5)=VIF(20,6)=VIF(20,11)=VIF(20,12)=1;VIF(21,9)=VIF(21,18)=VIF(21,19)=1;VIF(22,18)=VIF(22,19)=VIF(22,28)=VIF(22,29)=1;VIF(23,5)=VIF(23,10)=VIF(23,11)=1;VIF(24,0)=VIF(24,5)=VIF(24,9)=VIF(24,10)=VIF(24,19)=1;VIF(25,10)=VIF(25,11)=VIF(25,20)=VIF(25,21)=1;VIF(26,10)=VIF(26,19)=VIF(26,20)=VIF(26,29)=1;

  p.set_description() << "Johnson solid J21: Elongated pentagonal rotunda" << endl;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object gyroelongated_triangular_cupola()
{
  Matrix<double> hexa_V = create_hexagon_vertices<double>();

  double s = -sqrt(3)/9;
  Vector<double> trans(4); //unit normal vector
  trans[0]=0;
  trans[1]=trans[2]=s;
  trans[3]=-5*s;

  Matrix<double> triangle_V(3,4);
  triangle_V.col(0).fill(1);

  triangle_V(0,1)=triangle_V(0,3)=1;
  triangle_V(0,2)=4;
  triangle_V(1,1)=8;
  triangle_V(1,2)=triangle_V(1,3)=2;
  triangle_V(2,1)=6;
  triangle_V(2,2)=9;
  triangle_V(2,3)=3;

  double t = sqrt(6)/2;
  Vector<double> trans_down(4); // normal vector height
  trans_down[0]=0;
  trans_down[1]=trans_down[2]=-t;
  trans_down[3]=5*t;

  Matrix<double> R(3,3);
  R(0,0)=R(1,1)=1+13*sqrt(3);
  R(0,1)=1-8*sqrt(3);
  R(0,2)=R(2,1)=5-4*sqrt(3);
  R(1,0)=1+7*sqrt(3);
  R(1,2)=R(2,0)=-sqrt(3)+5;
  R(2,2)=sqrt(3)+25;
  R=R/27;

  Vector<double> bary(3);
  bary[0]=bary[1]=5;
  bary[2]=2;

  Matrix<double> V = (hexa_V / (triangle_V + repeat_row(6*trans,3)));
  V /= (((ones_vector<double>(6) | ((hexa_V.minor(All,sequence(1,3))-repeat_row(bary,6))*R)+repeat_row(bary,6)) - repeat_row(trans_down,6)));

  IncidenceMatrix<> VIF(20,15);
  VIF(0,3)=VIF(0,5)=VIF(0,7)=VIF(0,8)=1;
  VIF(1,1)=VIF(1,3)=VIF(1,12)=1;
  VIF(2,1)=VIF(2,3)=VIF(2,7)=1;
  VIF(3,4)=VIF(3,5)=VIF(3,13)=1;
  VIF(4,4)=VIF(4,5)=VIF(4,8)=1;
  VIF(5,0)=VIF(5,1)=VIF(5,6)=VIF(5,7)=1;
  VIF(6,0)=VIF(6,1)=VIF(6,10)=1;
  VIF(7,2)=VIF(7,4)=VIF(7,6)=VIF(7,8)=1;
  VIF(8,2)=VIF(8,4)=VIF(8,11)=1;
  VIF(9,0)=VIF(9,2)=VIF(9,9)=1;
  VIF(10,0)=VIF(10,2)=VIF(10,6)=1;
  VIF(11,2)=VIF(11,9)=VIF(11,11)=1;
  VIF(12,0)=VIF(12,9)=VIF(12,10)=1;
  VIF(13,6)=VIF(13,7)=VIF(13,8)=1;
  VIF(14,4)=VIF(14,11)=VIF(14,13)=1;
  VIF(15,1)=VIF(15,10)=VIF(15,12)=1;
  VIF(16,5)=VIF(16,13)=VIF(16,14)=1;
  VIF(17,3)=VIF(17,12)=VIF(17,14)=1;
  VIF(18,3)=VIF(18,5)=VIF(18,14)=1;
  VIF(19,9)=VIF(19,10)=VIF(19,11)=VIF(19,12)=VIF(19,13)=VIF(19,14)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J22: Gyroelongated triangular cupola" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object gyroelongated_square_cupola()
{
        Matrix<double> V = square_cupola().give("VERTICES");
        double a = sqrt((1+sqrt(2)/2)/2);
        double b = sqrt((1-sqrt(2)/2)/2);
        double c = sqrt(1+sqrt(2));
        double height = -sqrt(4-pow(c*a-b-c,2)-pow(c*b-a-1,2));
        Matrix<double> W = create_regular_polygon_vertices(8, sqrt(2)*sqrt(2+sqrt(2)), 0);

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
        p.set_description() << "Johnson solid J23: Gyroelongated square cupola" << endl;
        Matrix<double> X = (W.minor(All, sequence(0,3)) | same_element_vector<double>(height,8)) / V;
        IncidenceMatrix<> VIF(26,20);
        IncidenceMatrix<> VIFofW = square_cupola().give("VERTICES_IN_FACETS");
        for (int i=0; i<8; ++i) {
                VIF(i,i)=VIF(i,(i+1)%8)=VIF(i,8+(9-i)%8)=VIF(i+8,i+8)=VIF(i+8,8+(i+1)%8)=VIF(i+8,(9-i)%8)=1;
        }
        for (int i=0; i<8; ++i) {
                VIF(16,i)=1;
        }
        for (int i=1; i<10; ++i) {
                for (int j=0; j<12; ++j) {
                        if (VIFofW.exists(i,j)) {
                                VIF(i+16,j+8)=1;
                        }
                }
        }

        p.take("VERTICES") << X;
        p.take("VERTICES_IN_FACETS") << VIF;
        assign_common_props<double>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object gyroelongated_pentagonal_cupola()
{
  perl::Object pc = pentagonal_cupola();
  Matrix<double> V = pc.give("VERTICES");
  double height = -sqrt(6*(5-sqrt(5)))/4;
  double r = sqrt((8+sqrt(5)+2*sqrt(6-2*sqrt(3)))/4); //radius of decagon
  V /=  (create_regular_polygon_vertices(10, r, 6*M_PI/10) | same_element_vector<double>(height,10));

  IncidenceMatrix<> VIF(32,25);
  VIF(0,0)=VIF(0,1)=VIF(0,13)=VIF(0,14)=1;VIF(1,0)=VIF(1,12)=VIF(1,13)=1;VIF(2,13)=VIF(2,14)=VIF(2,23)=1;VIF(3,14)=VIF(3,23)=VIF(3,24)=1;VIF(4,1)=VIF(4,5)=VIF(4,14)=1;VIF(5,5)=VIF(5,14)=VIF(5,24)=1;VIF(6,0)=VIF(6,1)=VIF(6,2)=VIF(6,3)=VIF(6,4)=1;VIF(7,10)=VIF(7,19)=VIF(7,20)=1;VIF(8,3)=VIF(8,4)=VIF(8,9)=VIF(8,10)=1;VIF(9,9)=VIF(9,10)=VIF(9,19)=1;VIF(10,9)=VIF(10,18)=VIF(10,19)=1;VIF(11,3)=VIF(11,8)=VIF(11,9)=1;VIF(12,8)=VIF(12,9)=VIF(12,18)=1;VIF(13,7)=VIF(13,8)=VIF(13,17)=1;VIF(14,8)=VIF(14,17)=VIF(14,18)=1;VIF(15,7)=VIF(15,16)=VIF(15,17)=1;VIF(16,6)=VIF(16,7)=VIF(16,16)=1;VIF(17,2)=VIF(17,6)=VIF(17,7)=1;VIF(18,2)=VIF(18,3)=VIF(18,7)=VIF(18,8)=1;VIF(19,6)=VIF(19,15)=VIF(19,16)=1;VIF(20,5)=VIF(20,6)=VIF(20,15)=1;VIF(21,1)=VIF(21,2)=VIF(21,5)=VIF(21,6)=1;VIF(22,5)=VIF(22,15)=VIF(22,24)=1;VIF(23,10)=VIF(23,11)=VIF(23,20)=1;VIF(24,4)=VIF(24,10)=VIF(24,11)=1;VIF(25,11)=VIF(25,20)=VIF(25,21)=1;VIF(26,11)=VIF(26,12)=VIF(26,21)=1;VIF(27,0)=VIF(27,4)=VIF(27,11)=VIF(27,12)=1;VIF(28,12)=VIF(28,21)=VIF(28,22)=1;VIF(29,13)=VIF(29,22)=VIF(29,23)=1;VIF(30,12)=VIF(30,13)=VIF(30,22)=1;VIF(31,15)=VIF(31,16)=VIF(31,17)=VIF(31,18)=VIF(31,19)=VIF(31,20)=VIF(31,21)=VIF(31,22)=VIF(31,23)=VIF(31,24)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J24: Gyroelongated pentagonal cupola" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object gyroelongated_pentagonal_rotunda()
{
  perl::Object pr = pentagonal_rotunda();
  Matrix<double> V = pr.give("VERTICES");

  double a = sqrt((5-sqrt(5))/2); //side length
  double r = a*(1+sqrt(5))/2; //radius of decagon
  V /= (create_regular_polygon_vertices(10,r,0) | same_element_vector<double>(-a*sqrt(3)/2,10));

  IncidenceMatrix<> VIF(37,30);
  VIF(0,0)=VIF(0,5)=VIF(0,9)=VIF(0,10)=VIF(0,19)=1;VIF(1,10)=VIF(1,11)=VIF(1,21)=1;VIF(2,5)=VIF(2,10)=VIF(2,11)=1;VIF(3,11)=VIF(3,21)=VIF(3,22)=1;VIF(4,1)=VIF(4,5)=VIF(4,6)=VIF(4,11)=VIF(4,12)=1;VIF(5,0)=VIF(5,1)=VIF(5,5)=1;VIF(6,11)=VIF(6,12)=VIF(6,22)=1;VIF(7,0)=VIF(7,1)=VIF(7,2)=VIF(7,3)=VIF(7,4)=1;VIF(8,17)=VIF(8,27)=VIF(8,28)=1;VIF(9,3)=VIF(9,4)=VIF(9,8)=1;VIF(10,8)=VIF(10,16)=VIF(10,17)=1;VIF(11,16)=VIF(11,17)=VIF(11,27)=1;VIF(12,3)=VIF(12,7)=VIF(12,8)=VIF(12,15)=VIF(12,16)=1;VIF(13,16)=VIF(13,26)=VIF(13,27)=1;VIF(14,15)=VIF(14,16)=VIF(14,26)=1;VIF(15,15)=VIF(15,25)=VIF(15,26)=1;VIF(16,7)=VIF(16,14)=VIF(16,15)=1;VIF(17,14)=VIF(17,15)=VIF(17,25)=1;VIF(18,14)=VIF(18,24)=VIF(18,25)=1;VIF(19,13)=VIF(19,14)=VIF(19,24)=1;VIF(20,2)=VIF(20,3)=VIF(20,7)=1;VIF(21,13)=VIF(21,23)=VIF(21,24)=1;VIF(22,2)=VIF(22,6)=VIF(22,7)=VIF(22,13)=VIF(22,14)=1;VIF(23,12)=VIF(23,13)=VIF(23,23)=1;VIF(24,6)=VIF(24,12)=VIF(24,13)=1;VIF(25,1)=VIF(25,2)=VIF(25,6)=1;VIF(26,12)=VIF(26,22)=VIF(26,23)=1;VIF(27,17)=VIF(27,18)=VIF(27,28)=1;VIF(28,0)=VIF(28,4)=VIF(28,9)=1;VIF(29,4)=VIF(29,8)=VIF(29,9)=VIF(29,17)=VIF(29,18)=1;VIF(30,18)=VIF(30,28)=VIF(30,29)=1;VIF(31,9)=VIF(31,18)=VIF(31,19)=1;VIF(32,18)=VIF(32,19)=VIF(32,29)=1;VIF(33,19)=VIF(33,20)=VIF(33,29)=1;VIF(34,10)=VIF(34,20)=VIF(34,21)=1;VIF(35,10)=VIF(35,19)=VIF(35,20)=1;VIF(36,20)=VIF(36,21)=VIF(36,22)=VIF(36,23)=VIF(36,24)=VIF(36,25)=VIF(36,26)=VIF(36,27)=VIF(36,28)=VIF(36,29)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J25: Gyroelongated pentagonal rotunda" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}

perl::Object gyrobifastigium(){

  QE height(0,1,3);
  Vector<QE> h(2);
  h[0]=height;
  h[1]=-height;

  Matrix<QE> V = ((create_square_vertices<QE>() | zero_vector<QE>(4)) / (same_element_vector<QE>(QE(1,0,0),4) | ((unit_matrix<QE>(2) | h) / (-unit_matrix<QE>(2) | h))));

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
        p.set_description() << "Johnson solid J26: Gyrobifastigium" << endl;
        p.take("VERTICES") << V;
        assign_common_props<QE>(p);

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

  Matrix<QE> V=( create_hexagon_vertices<QE>() / (triangle_V + repeat_row(6*trans,3)) / (triangle_V - repeat_row(6*trans,3)));

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.set_description() << "Johnson solid J27: Triangular orthobicupola" << endl;
  p.take("VERTICES") << V;
  assign_common_props<QE>(p);

  return p;
}

perl::Object square_orthobicupola()
{
        Matrix<QE> V = square_cupola().give("VERTICES");
        V /= (ones_vector<QE>(4) | (-1)*V.minor(sequence(8,4),sequence(1,3)));

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
        p.set_description() << "Johnson solid J28: Square orthobicupola" << endl;
        p.take("VERTICES") << V;
        assign_common_props<QE>(p);

  return p;
}

perl::Object square_gyrobicupola()
{	
	QE rot(0,Rational(1,2),2);
	Matrix<QE> R(3,3);
	R(0,0)=R(1,0)=R(1,1)=rot;
	R(0,1)=-rot;
	R(2,2)=1;
	Matrix<QE> V = square_cupola().give("VERTICES");
	V /= (ones_vector<QE>(4) | (-1)*(V.minor(sequence(8,4),sequence(1,3))*R));

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
        p.set_description() << "Johnson solid J29: Square gyrobicupola" << endl;
        p.take("VERTICES") << V;
        assign_common_props<QE>(p);

  return p;
}

perl::Object pentagonal_orthobicupola()
{
  double r = sqrt((8+sqrt(5)+2*sqrt(6-2*sqrt(3)))/4); //radius of decagon
  double s = 2*M_PI/4; //angle of decagon vertex
  double h = (sqrt(5)-1)/2;

  Matrix<double> V((create_regular_polygon_vertices(5,1,0) | same_element_vector<double>(h,5)) / (create_regular_polygon_vertices(5,1,0) | same_element_vector<double>(-h,5)) / (create_regular_polygon_vertices(10,r,s) | same_element_vector<double>(0,10)) );

  IncidenceMatrix<> VIF(22,20);
  VIF(0,5)=VIF(0,9)=VIF(0,16)=VIF(0,17)=1;VIF(1,0)=VIF(1,4)=VIF(1,16)=VIF(1,17)=1;VIF(2,4)=VIF(2,15)=VIF(2,16)=1;VIF(3,9)=VIF(3,15)=VIF(3,16)=1;VIF(4,5)=VIF(4,6)=VIF(4,7)=VIF(4,8)=VIF(4,9)=1;VIF(5,6)=VIF(5,7)=VIF(5,10)=VIF(5,11)=1;VIF(6,8)=VIF(6,9)=VIF(6,14)=VIF(6,15)=1;VIF(7,7)=VIF(7,8)=VIF(7,12)=VIF(7,13)=1;VIF(8,7)=VIF(8,11)=VIF(8,12)=1;VIF(9,8)=VIF(9,13)=VIF(9,14)=1;VIF(10,3)=VIF(10,13)=VIF(10,14)=1;VIF(11,2)=VIF(11,11)=VIF(11,12)=1;VIF(12,2)=VIF(12,3)=VIF(12,12)=VIF(12,13)=1;VIF(13,3)=VIF(13,4)=VIF(13,14)=VIF(13,15)=1;VIF(14,1)=VIF(14,2)=VIF(14,10)=VIF(14,11)=1;VIF(15,0)=VIF(15,1)=VIF(15,2)=VIF(15,3)=VIF(15,4)=1;VIF(16,6)=VIF(16,10)=VIF(16,19)=1;VIF(17,1)=VIF(17,10)=VIF(17,19)=1;VIF(18,5)=VIF(18,17)=VIF(18,18)=1;VIF(19,0)=VIF(19,17)=VIF(19,18)=1;VIF(20,0)=VIF(20,1)=VIF(20,18)=VIF(20,19)=1;VIF(21,5)=VIF(21,6)=VIF(21,18)=VIF(21,19)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J30: Pentagonal orthobicupola" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS")<<VIF;
  assign_common_props<double>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object pentagonal_gyrobicupola()
{
  double r = sqrt((8+sqrt(5)+2*sqrt(6-2*sqrt(3)))/4); //radius of decagon
  double s = 2*M_PI/4; //angle of decagon vertex
  double h = (sqrt(5)-1)/2;

  Matrix<double> V((create_regular_polygon_vertices(5,1,0) | same_element_vector<double>(h,5)) / (create_regular_polygon_vertices(5,1,M_PI/5) | same_element_vector<double>(-h,5)) / (create_regular_polygon_vertices(10,r,s) | same_element_vector<double>(0,10)) );

  IncidenceMatrix<> VIF(22,20);
  VIF(0,0)=VIF(0,4)=VIF(0,16)=VIF(0,17)=1;VIF(1,9)=VIF(1,16)=VIF(1,17)=1;VIF(2,8)=VIF(2,9)=VIF(2,15)=VIF(2,16)=1;VIF(3,4)=VIF(3,15)=VIF(3,16)=1;VIF(4,5)=VIF(4,6)=VIF(4,7)=VIF(4,8)=VIF(4,9)=1;VIF(5,1)=VIF(5,2)=VIF(5,10)=VIF(5,11)=1;VIF(6,6)=VIF(6,10)=VIF(6,11)=1;VIF(7,6)=VIF(7,7)=VIF(7,11)=VIF(7,12)=1;VIF(8,2)=VIF(8,3)=VIF(8,12)=VIF(8,13)=1;VIF(9,2)=VIF(9,11)=VIF(9,12)=1;VIF(10,7)=VIF(10,12)=VIF(10,13)=1;VIF(11,3)=VIF(11,13)=VIF(11,14)=1;VIF(12,7)=VIF(12,8)=VIF(12,13)=VIF(12,14)=1;VIF(13,8)=VIF(13,14)=VIF(13,15)=1;VIF(14,3)=VIF(14,4)=VIF(14,14)=VIF(14,15)=1;VIF(15,0)=VIF(15,1)=VIF(15,2)=VIF(15,3)=VIF(15,4)=1;VIF(16,1)=VIF(16,10)=VIF(16,19)=1;VIF(17,5)=VIF(17,6)=VIF(17,10)=VIF(17,19)=1;VIF(18,5)=VIF(18,18)=VIF(18,19)=1;VIF(19,0)=VIF(19,17)=VIF(19,18)=1;VIF(20,0)=VIF(20,1)=VIF(20,18)=VIF(20,19)=1;VIF(21,5)=VIF(21,9)=VIF(21,17)=VIF(21,18)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J31: Pentagonal gyrobicupola" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS")<<VIF;
  assign_common_props<double>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object pentagonal_orthocupolarotunda()
{
  Matrix<double> V = pentagonal_rotunda().give("VERTICES");
  
  double h = (sqrt(5)-1)/2;

  V /= (create_regular_polygon_vertices(5,1,0) | same_element_vector<double>(-h,5));

  IncidenceMatrix<> VIF(27,25);
  VIF(0,18)=VIF(0,19)=VIF(0,20)=VIF(0,24)=1;VIF(1,9)=VIF(1,18)=VIF(1,19)=1;VIF(2,4)=VIF(2,8)=VIF(2,9)=VIF(2,17)=VIF(2,18)=1;VIF(3,0)=VIF(3,4)=VIF(3,9)=1;VIF(4,17)=VIF(4,18)=VIF(4,24)=1;VIF(5,20)=VIF(5,21)=VIF(5,22)=VIF(5,23)=VIF(5,24)=1;VIF(6,12)=VIF(6,13)=VIF(6,21)=VIF(6,22)=1;VIF(7,16)=VIF(7,17)=VIF(7,23)=VIF(7,24)=1;VIF(8,8)=VIF(8,16)=VIF(8,17)=1;VIF(9,3)=VIF(9,7)=VIF(9,8)=VIF(9,15)=VIF(9,16)=1;VIF(10,14)=VIF(10,15)=VIF(10,22)=VIF(10,23)=1;VIF(11,13)=VIF(11,14)=VIF(11,22)=1;VIF(12,15)=VIF(12,16)=VIF(12,23)=1;VIF(13,7)=VIF(13,14)=VIF(13,15)=1;VIF(14,2)=VIF(14,3)=VIF(14,7)=1;VIF(15,2)=VIF(15,6)=VIF(15,7)=VIF(15,13)=VIF(15,14)=1;VIF(16,6)=VIF(16,12)=VIF(16,13)=1;VIF(17,3)=VIF(17,4)=VIF(17,8)=1;VIF(18,1)=VIF(18,2)=VIF(18,6)=1;VIF(19,0)=VIF(19,1)=VIF(19,2)=VIF(19,3)=VIF(19,4)=1;VIF(20,11)=VIF(20,12)=VIF(20,21)=1;VIF(21,0)=VIF(21,1)=VIF(21,5)=1;VIF(22,1)=VIF(22,5)=VIF(22,6)=VIF(22,11)=VIF(22,12)=1;VIF(23,5)=VIF(23,10)=VIF(23,11)=1;VIF(24,10)=VIF(24,19)=VIF(24,20)=1;VIF(25,10)=VIF(25,11)=VIF(25,20)=VIF(25,21)=1;VIF(26,0)=VIF(26,5)=VIF(26,9)=VIF(26,10)=VIF(26,19)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J32: Pentagonal orthocupolarotunda" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS")<<VIF;
  assign_common_props<double>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object pentagonal_gyrocupolarotunda()
{
  Matrix<double> V = pentagonal_rotunda().give("VERTICES");

  double h = (sqrt(5)-1)/2;

  V /= (create_regular_polygon_vertices(5,1,M_PI/5) | same_element_vector<double>(-h,5));

  IncidenceMatrix<> VIF(27,25);
  VIF(0,9)=VIF(0,18)=VIF(0,19)=1;VIF(1,18)=VIF(1,19)=VIF(1,24)=1;VIF(2,4)=VIF(2,8)=VIF(2,9)=VIF(2,17)=VIF(2,18)=1;VIF(3,0)=VIF(3,4)=VIF(3,9)=1;VIF(4,17)=VIF(4,18)=VIF(4,23)=VIF(4,24)=1;VIF(5,20)=VIF(5,21)=VIF(5,22)=VIF(5,23)=VIF(5,24)=1;VIF(6,1)=VIF(6,2)=VIF(6,6)=1;VIF(7,12)=VIF(7,13)=VIF(7,21)=1;VIF(8,6)=VIF(8,12)=VIF(8,13)=1;VIF(9,13)=VIF(9,14)=VIF(9,21)=VIF(9,22)=1;VIF(10,2)=VIF(10,6)=VIF(10,7)=VIF(10,13)=VIF(10,14)=1;VIF(11,2)=VIF(11,3)=VIF(11,7)=1;VIF(12,14)=VIF(12,15)=VIF(12,22)=1;VIF(13,7)=VIF(13,14)=VIF(13,15)=1;VIF(14,3)=VIF(14,7)=VIF(14,8)=VIF(14,15)=VIF(14,16)=1;VIF(15,15)=VIF(15,16)=VIF(15,22)=VIF(15,23)=1;VIF(16,8)=VIF(16,16)=VIF(16,17)=1;VIF(17,16)=VIF(17,17)=VIF(17,23)=1;VIF(18,3)=VIF(18,4)=VIF(18,8)=1;VIF(19,0)=VIF(19,1)=VIF(19,2)=VIF(19,3)=VIF(19,4)=1;VIF(20,11)=VIF(20,12)=VIF(20,20)=VIF(20,21)=1;VIF(21,0)=VIF(21,1)=VIF(21,5)=1;VIF(22,1)=VIF(22,5)=VIF(22,6)=VIF(22,11)=VIF(22,12)=1;VIF(23,5)=VIF(23,10)=VIF(23,11)=1;VIF(24,10)=VIF(24,11)=VIF(24,20)=1;VIF(25,10)=VIF(25,19)=VIF(25,20)=VIF(25,24)=1;VIF(26,0)=VIF(26,5)=VIF(26,9)=VIF(26,10)=VIF(26,19)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J33: Pentagonal gyrocupolarotunda" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<double>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object pentagonal_orthobirotunda()
{
  Matrix<double> V = pentagonal_rotunda().give("VERTICES");
  
  double a = sqrt((5-sqrt(5))/2); //side length
  double h = a*sqrt(1+2/sqrt(5)); //height
  double rp = (1+sqrt(5))/2; //radius of inner pentagon

  V /= (create_regular_polygon_vertices(5,1,0) | same_element_vector<double>(-h,5)) / (create_regular_polygon_vertices(5,rp,M_PI/5) | same_element_vector<double>(-1,5)) ;

  IncidenceMatrix<> VIF(32,30);
  VIF(0,9)=VIF(0,18)=VIF(0,19)=1;VIF(1,18)=VIF(1,19)=VIF(1,29)=1;VIF(2,11)=VIF(2,12)=VIF(2,21)=VIF(2,25)=VIF(2,26)=1;VIF(3,20)=VIF(3,21)=VIF(3,25)=1;VIF(4,17)=VIF(4,18)=VIF(4,24)=VIF(4,28)=VIF(4,29)=1;VIF(5,20)=VIF(5,24)=VIF(5,29)=1;VIF(6,20)=VIF(6,21)=VIF(6,22)=VIF(6,23)=VIF(6,24)=1;VIF(7,21)=VIF(7,22)=VIF(7,26)=1;VIF(8,23)=VIF(8,24)=VIF(8,28)=1;VIF(9,8)=VIF(9,16)=VIF(9,17)=1;VIF(10,16)=VIF(10,17)=VIF(10,28)=1;VIF(11,13)=VIF(11,14)=VIF(11,22)=VIF(11,26)=VIF(11,27)=1;VIF(12,15)=VIF(12,16)=VIF(12,23)=VIF(12,27)=VIF(12,28)=1;VIF(13,22)=VIF(13,23)=VIF(13,27)=1;VIF(14,7)=VIF(14,14)=VIF(14,15)=1;VIF(15,14)=VIF(15,15)=VIF(15,27)=1;VIF(16,2)=VIF(16,3)=VIF(16,7)=1;VIF(17,3)=VIF(17,7)=VIF(17,8)=VIF(17,15)=VIF(17,16)=1;VIF(18,2)=VIF(18,6)=VIF(18,7)=VIF(18,13)=VIF(18,14)=1;VIF(19,12)=VIF(19,13)=VIF(19,26)=1;VIF(20,6)=VIF(20,12)=VIF(20,13)=1;VIF(21,3)=VIF(21,4)=VIF(21,8)=1;VIF(22,1)=VIF(22,2)=VIF(22,6)=1;VIF(23,0)=VIF(23,1)=VIF(23,2)=VIF(23,3)=VIF(23,4)=1;VIF(24,0)=VIF(24,4)=VIF(24,9)=1;VIF(25,4)=VIF(25,8)=VIF(25,9)=VIF(25,17)=VIF(25,18)=1;VIF(26,0)=VIF(26,1)=VIF(26,5)=1;VIF(27,1)=VIF(27,5)=VIF(27,6)=VIF(27,11)=VIF(27,12)=1;VIF(28,10)=VIF(28,11)=VIF(28,25)=1;VIF(29,5)=VIF(29,10)=VIF(29,11)=1;VIF(30,10)=VIF(30,19)=VIF(30,20)=VIF(30,25)=VIF(30,29)=1;VIF(31,0)=VIF(31,5)=VIF(31,9)=VIF(31,10)=VIF(31,19)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J34: Pentagonal orthobirotunda" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<double>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object elongated_triangular_orthobicupola()
{
  Matrix<double> hexa_V = create_hexagon_vertices<double>();

  double s = -sqrt(3)/9;
  Vector<double> trans(4); //unit normal vector
  trans[0]=0;
  trans[1]=trans[2]=s;
  trans[3]=-5*s;

  Matrix<double> triangle_V(3,4);
  triangle_V.col(0).fill(1);

  triangle_V(0,1)=triangle_V(0,3)=1;
  triangle_V(0,2)=4;
  triangle_V(1,1)=8;
  triangle_V(1,2)=triangle_V(1,3)=2;
  triangle_V(2,1)=6;
  triangle_V(2,2)=9;
  triangle_V(2,3)=3;

  double t = sqrt(2);
  Vector<double> trans_down(4); // normal vector side length
  trans_down[0]=0;
  trans_down[1]=trans_down[2]=-t;
  trans_down[3]=5*t;

  Matrix<double> V=( hexa_V / (triangle_V + repeat_row(6*trans,3)) / (hexa_V - repeat_row(trans_down,6)) / (triangle_V - repeat_row(trans_down + 6*trans,3)));

  IncidenceMatrix<> VIF(20,18);
  VIF(0,3)=VIF(0,5)=VIF(0,7)=VIF(0,8)=1;
  VIF(1,1)=VIF(1,3)=VIF(1,7)=1;
  VIF(2,9)=VIF(2,10)=VIF(2,15)=VIF(2,16)=1;
  VIF(3,15)=VIF(3,16)=VIF(3,17)=1;
  VIF(4,11)=VIF(4,13)=VIF(4,15)=VIF(4,17)=1;
  VIF(5,0)=VIF(5,1)=VIF(5,6)=VIF(5,7)=1;
  VIF(6,2)=VIF(6,4)=VIF(6,6)=VIF(6,8)=1;
  VIF(7,9)=VIF(7,11)=VIF(7,15)=1;
  VIF(8,0)=VIF(8,2)=VIF(8,9)=VIF(8,11)=1;
  VIF(9,0)=VIF(9,2)=VIF(9,6)=1;
  VIF(10,2)=VIF(10,4)=VIF(10,11)=VIF(10,13)=1;
  VIF(11,6)=VIF(11,7)=VIF(11,8)=1;
  VIF(12,0)=VIF(12,1)=VIF(12,9)=VIF(12,10)=1;
  VIF(13,4)=VIF(13,5)=VIF(13,8)=1;
  VIF(14,13)=VIF(14,14)=VIF(14,17)=1;
  VIF(15,4)=VIF(15,5)=VIF(15,13)=VIF(15,14)=1;
  VIF(16,10)=VIF(16,12)=VIF(16,16)=1;
  VIF(17,12)=VIF(17,14)=VIF(17,16)=VIF(17,17)=1;
  VIF(18,1)=VIF(18,3)=VIF(18,10)=VIF(18,12)=1;
  VIF(19,3)=VIF(19,5)=VIF(19,12)=VIF(19,14)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J35: Elongated triangular orthobicupola" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}

//FIXME: coordinates #830
perl::Object elongated_triangular_gyrobicupola()
{
  Matrix<double> V = elongated_triangular_orthobicupola().give("VERTICES");

  double t = sqrt(2)+2*sqrt(3)/3;
  Vector<double> bary(3); //barycenter of the lower triangle
  bary[0]=bary[1]=5+t;
  bary[2]=2-5*t;

  Matrix<double> rot_triangle = repeat_row(2*bary, 3) - V.minor(sequence(15,3), sequence(1,3));

  V = (V.minor(sequence(0,15), All) / (ones_vector<double>(3) | rot_triangle));

  IncidenceMatrix<> VIF(20,18);
  VIF(0,3)=VIF(0,5)=VIF(0,7)=VIF(0,8)=1;
  VIF(1,1)=VIF(1,3)=VIF(1,7)=1;
  VIF(2,4)=VIF(2,5)=VIF(2,13)=VIF(2,14)=1;
  VIF(3,0)=VIF(3,1)=VIF(3,9)=VIF(3,10)=1;
  VIF(4,9)=VIF(4,10)=VIF(4,17)=1;
  VIF(5,6)=VIF(5,7)=VIF(5,8)=1;
  VIF(6,2)=VIF(6,4)=VIF(6,6)=VIF(6,8)=1;
  VIF(7,2)=VIF(7,4)=VIF(7,11)=VIF(7,13)=1;
  VIF(8,0)=VIF(8,2)=VIF(8,9)=VIF(8,11)=1;
  VIF(9,0)=VIF(9,2)=VIF(9,6)=1;
  VIF(10,11)=VIF(10,13)=VIF(10,16)=1;
  VIF(11,9)=VIF(11,11)=VIF(11,16)=VIF(11,17)=1;
  VIF(12,0)=VIF(12,1)=VIF(12,6)=VIF(12,7)=1;
  VIF(13,4)=VIF(13,5)=VIF(13,8)=1;
  VIF(14,15)=VIF(14,16)=VIF(14,17)=1;
  VIF(15,13)=VIF(15,14)=VIF(15,15)=VIF(15,16)=1;
  VIF(16,12)=VIF(16,14)=VIF(16,15)=1;
  VIF(17,10)=VIF(17,12)=VIF(17,15)=VIF(17,17)=1;
  VIF(18,1)=VIF(18,3)=VIF(18,10)=VIF(18,12)=1;
  VIF(19,3)=VIF(19,5)=VIF(19,12)=VIF(19,14)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J36: Elongated triangular gyrobicupola" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}
  
perl::Object elongated_square_gyrobicupola()
{
  Matrix<QE> V = elongated_square_cupola().give("VERTICES");
	Matrix<QE> W = square_gyrobicupola().give("VERTICES");
	V /= W.minor(sequence(12,4),All);
	V(20,3)=V(21,3)=V(22,3)=V(23,3)= V(20,3)-2;

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
	p.set_description() << "Johnson solid J37: Elongated square gyrobicupola" << endl;  
	p.take("VERTICES") << V;
	assign_common_props<QE>(p);

  return p;  
}
  
//FIXME: coordinates #830
perl::Object elongated_pentagonal_orthobicupola()
{
  Matrix<double> V = elongated_pentagonal_cupola().give("VERTICES");

  double h = sqrt((5-sqrt(5))/2) + (sqrt(5)-1)/2;
  
  V /= (create_regular_polygon_vertices(5,1,0) | same_element_vector<double>(-h,5));

  IncidenceMatrix<> VIF(32,30);
  VIF(0,23)=VIF(0,24)=VIF(0,25)=VIF(0,26)=1;VIF(1,22)=VIF(1,23)=VIF(1,25)=1;VIF(2,21)=VIF(2,22)=VIF(2,25)=VIF(2,29)=1;VIF(3,15)=VIF(3,24)=VIF(3,26)=1;VIF(4,20)=VIF(4,21)=VIF(4,29)=1;VIF(5,25)=VIF(5,26)=VIF(5,27)=VIF(5,28)=VIF(5,29)=1;VIF(6,15)=VIF(6,16)=VIF(6,26)=VIF(6,27)=1;VIF(7,19)=VIF(7,20)=VIF(7,28)=VIF(7,29)=1;VIF(8,17)=VIF(8,18)=VIF(8,27)=VIF(8,28)=1;VIF(9,16)=VIF(9,17)=VIF(9,27)=1;VIF(10,18)=VIF(10,19)=VIF(10,28)=1;VIF(11,7)=VIF(11,8)=VIF(11,17)=VIF(11,18)=1;VIF(12,8)=VIF(12,9)=VIF(12,18)=VIF(12,19)=1;VIF(13,6)=VIF(13,7)=VIF(13,16)=VIF(13,17)=1;VIF(14,3)=VIF(14,8)=VIF(14,9)=1;VIF(15,2)=VIF(15,6)=VIF(15,7)=1;VIF(16,2)=VIF(16,3)=VIF(16,7)=VIF(16,8)=1;VIF(17,9)=VIF(17,10)=VIF(17,19)=VIF(17,20)=1;VIF(18,5)=VIF(18,6)=VIF(18,15)=VIF(18,16)=1;VIF(19,3)=VIF(19,4)=VIF(19,9)=VIF(19,10)=1;VIF(20,1)=VIF(20,2)=VIF(20,5)=VIF(20,6)=1;VIF(21,0)=VIF(21,1)=VIF(21,2)=VIF(21,3)=VIF(21,4)=1;VIF(22,4)=VIF(22,10)=VIF(22,11)=1;VIF(23,10)=VIF(23,11)=VIF(23,20)=VIF(23,21)=1;VIF(24,1)=VIF(24,5)=VIF(24,14)=1;VIF(25,5)=VIF(25,14)=VIF(25,15)=VIF(25,24)=1;VIF(26,0)=VIF(26,4)=VIF(26,11)=VIF(26,12)=1;VIF(27,11)=VIF(27,12)=VIF(27,21)=VIF(27,22)=1;VIF(28,0)=VIF(28,12)=VIF(28,13)=1;VIF(29,0)=VIF(29,1)=VIF(29,13)=VIF(29,14)=1;VIF(30,13)=VIF(30,14)=VIF(30,23)=VIF(30,24)=1;VIF(31,12)=VIF(31,13)=VIF(31,22)=VIF(31,23)=1;
  
  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J38: Elongated pentagonal orthobicupola" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);
  
  return p;
}

//FIXME: coordinates #830
perl::Object elongated_pentagonal_gyrobicupola()
{
  Matrix<double> V = elongated_pentagonal_cupola().give("VERTICES");

  double h = sqrt((5-sqrt(5))/2) + (sqrt(5)-1)/2;
  
  V /= (create_regular_polygon_vertices(5,1,M_PI/5) | same_element_vector<double>(-h,5));

  IncidenceMatrix<> VIF(32,30);
  VIF(0,22)=VIF(0,23)=VIF(0,25)=VIF(0,29)=1;VIF(1,23)=VIF(1,24)=VIF(1,25)=1;VIF(2,21)=VIF(2,22)=VIF(2,29)=1;VIF(3,15)=VIF(3,24)=VIF(3,25)=VIF(3,26)=1;VIF(4,20)=VIF(4,21)=VIF(4,28)=VIF(4,29)=1;VIF(5,25)=VIF(5,26)=VIF(5,27)=VIF(5,28)=VIF(5,29)=1;VIF(6,1)=VIF(6,2)=VIF(6,5)=VIF(6,6)=1;VIF(7,5)=VIF(7,6)=VIF(7,15)=VIF(7,16)=1;VIF(8,9)=VIF(8,10)=VIF(8,19)=VIF(8,20)=1;VIF(9,16)=VIF(9,17)=VIF(9,26)=VIF(9,27)=1;VIF(10,2)=VIF(10,3)=VIF(10,7)=VIF(10,8)=1;VIF(11,2)=VIF(11,6)=VIF(11,7)=1;VIF(12,17)=VIF(12,18)=VIF(12,27)=1;VIF(13,7)=VIF(13,8)=VIF(13,17)=VIF(13,18)=1;VIF(14,8)=VIF(14,9)=VIF(14,18)=VIF(14,19)=1;VIF(15,6)=VIF(15,7)=VIF(15,16)=VIF(15,17)=1;VIF(16,3)=VIF(16,8)=VIF(16,9)=1;VIF(17,18)=VIF(17,19)=VIF(17,27)=VIF(17,28)=1;VIF(18,19)=VIF(18,20)=VIF(18,28)=1;VIF(19,15)=VIF(19,16)=VIF(19,26)=1;VIF(20,3)=VIF(20,4)=VIF(20,9)=VIF(20,10)=1;VIF(21,0)=VIF(21,1)=VIF(21,2)=VIF(21,3)=VIF(21,4)=1;VIF(22,4)=VIF(22,10)=VIF(22,11)=1;VIF(23,10)=VIF(23,11)=VIF(23,20)=VIF(23,21)=1;VIF(24,1)=VIF(24,5)=VIF(24,14)=1;VIF(25,5)=VIF(25,14)=VIF(25,15)=VIF(25,24)=1;VIF(26,0)=VIF(26,4)=VIF(26,11)=VIF(26,12)=1;VIF(27,11)=VIF(27,12)=VIF(27,21)=VIF(27,22)=1;VIF(28,0)=VIF(28,12)=VIF(28,13)=1;VIF(29,0)=VIF(29,1)=VIF(29,13)=VIF(29,14)=1;VIF(30,13)=VIF(30,14)=VIF(30,23)=VIF(30,24)=1;VIF(31,12)=VIF(31,13)=VIF(31,22)=VIF(31,23)=1;
  
  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J39: Elongated pentagonal gyrobicupola" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);
  
  return p;
}
      
//FIXME: coordinates #830
perl::Object elongated_pentagonal_orthocupolarotunda()
{
  Matrix<double> V = elongated_pentagonal_rotunda().give("VERTICES");

  double h = sqrt((5-sqrt(5))/2) + (sqrt(5)-1)/2;
  
  V /= (create_regular_polygon_vertices(5,1,0) | same_element_vector<double>(-h,5));

  IncidenceMatrix<> VIF(37,35);
  VIF(0,20)=VIF(0,21)=VIF(0,30)=VIF(0,31)=1;VIF(1,20)=VIF(1,29)=VIF(1,30)=1;VIF(2,28)=VIF(2,29)=VIF(2,30)=VIF(2,34)=1;VIF(3,4)=VIF(3,8)=VIF(3,9)=VIF(3,17)=VIF(3,18)=1;VIF(4,0)=VIF(4,4)=VIF(4,9)=1;VIF(5,21)=VIF(5,22)=VIF(5,31)=1;VIF(6,27)=VIF(6,28)=VIF(6,34)=1;VIF(7,30)=VIF(7,31)=VIF(7,32)=VIF(7,33)=VIF(7,34)=1;VIF(8,22)=VIF(8,23)=VIF(8,31)=VIF(8,32)=1;VIF(9,26)=VIF(9,27)=VIF(9,33)=VIF(9,34)=1;VIF(10,3)=VIF(10,7)=VIF(10,8)=VIF(10,15)=VIF(10,16)=1;VIF(11,24)=VIF(11,25)=VIF(11,32)=VIF(11,33)=1;VIF(12,23)=VIF(12,24)=VIF(12,32)=1;VIF(13,25)=VIF(13,26)=VIF(13,33)=1;VIF(14,7)=VIF(14,14)=VIF(14,15)=1;VIF(15,14)=VIF(15,15)=VIF(15,24)=VIF(15,25)=1;VIF(16,15)=VIF(16,16)=VIF(16,25)=VIF(16,26)=1;VIF(17,13)=VIF(17,14)=VIF(17,23)=VIF(17,24)=1;VIF(18,2)=VIF(18,3)=VIF(18,7)=1;VIF(19,2)=VIF(19,6)=VIF(19,7)=VIF(19,13)=VIF(19,14)=1;VIF(20,8)=VIF(20,16)=VIF(20,17)=1;VIF(21,16)=VIF(21,17)=VIF(21,26)=VIF(21,27)=1;VIF(22,6)=VIF(22,12)=VIF(22,13)=1;VIF(23,12)=VIF(23,13)=VIF(23,22)=VIF(23,23)=1;VIF(24,3)=VIF(24,4)=VIF(24,8)=1;VIF(25,1)=VIF(25,2)=VIF(25,6)=1;VIF(26,0)=VIF(26,1)=VIF(26,2)=VIF(26,3)=VIF(26,4)=1;VIF(27,17)=VIF(27,18)=VIF(27,27)=VIF(27,28)=1;VIF(28,11)=VIF(28,12)=VIF(28,21)=VIF(28,22)=1;VIF(29,0)=VIF(29,1)=VIF(29,5)=1;VIF(30,1)=VIF(30,5)=VIF(30,6)=VIF(30,11)=VIF(30,12)=1;VIF(31,9)=VIF(31,18)=VIF(31,19)=1;VIF(32,18)=VIF(32,19)=VIF(32,28)=VIF(32,29)=1;VIF(33,5)=VIF(33,10)=VIF(33,11)=1;VIF(34,0)=VIF(34,5)=VIF(34,9)=VIF(34,10)=VIF(34,19)=1;VIF(35,10)=VIF(35,11)=VIF(35,20)=VIF(35,21)=1;VIF(36,10)=VIF(36,19)=VIF(36,20)=VIF(36,29)=1;
    
  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J40: Elongated pentagonal orthocupolarotunda" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);
  
  return p;
}
    
//FIXME: coordinates #830
perl::Object elongated_pentagonal_gyrocupolarotunda()
{
  Matrix<double> V = elongated_pentagonal_rotunda().give("VERTICES");

  double h = sqrt((5-sqrt(5))/2) + (sqrt(5)-1)/2;
  
  V /= (create_regular_polygon_vertices(5,1,M_PI/5) | same_element_vector<double>(-h,5));

  IncidenceMatrix<> VIF(37,35);
  VIF(0,20)=VIF(0,29)=VIF(0,30)=VIF(0,34)=1;VIF(1,20)=VIF(1,21)=VIF(1,30)=1;VIF(2,28)=VIF(2,29)=VIF(2,34)=1;VIF(3,4)=VIF(3,8)=VIF(3,9)=VIF(3,17)=VIF(3,18)=1;VIF(4,0)=VIF(4,4)=VIF(4,9)=1;VIF(5,21)=VIF(5,22)=VIF(5,30)=VIF(5,31)=1;VIF(6,27)=VIF(6,28)=VIF(6,33)=VIF(6,34)=1;VIF(7,30)=VIF(7,31)=VIF(7,32)=VIF(7,33)=VIF(7,34)=1;VIF(8,1)=VIF(8,2)=VIF(8,6)=1;VIF(9,12)=VIF(9,13)=VIF(9,22)=VIF(9,23)=1;VIF(10,6)=VIF(10,12)=VIF(10,13)=1;VIF(11,16)=VIF(11,17)=VIF(11,26)=VIF(11,27)=1;VIF(12,8)=VIF(12,16)=VIF(12,17)=1;VIF(13,23)=VIF(13,24)=VIF(13,31)=VIF(13,32)=1;VIF(14,2)=VIF(14,6)=VIF(14,7)=VIF(14,13)=VIF(14,14)=1;VIF(15,2)=VIF(15,3)=VIF(15,7)=1;VIF(16,24)=VIF(16,25)=VIF(16,32)=1;VIF(17,7)=VIF(17,14)=VIF(17,15)=1;VIF(18,14)=VIF(18,15)=VIF(18,24)=VIF(18,25)=1;VIF(19,15)=VIF(19,16)=VIF(19,25)=VIF(19,26)=1;VIF(20,13)=VIF(20,14)=VIF(20,23)=VIF(20,24)=1;VIF(21,3)=VIF(21,7)=VIF(21,8)=VIF(21,15)=VIF(21,16)=1;VIF(22,25)=VIF(22,26)=VIF(22,32)=VIF(22,33)=1;VIF(23,26)=VIF(23,27)=VIF(23,33)=1;VIF(24,22)=VIF(24,23)=VIF(24,31)=1;VIF(25,3)=VIF(25,4)=VIF(25,8)=1;VIF(26,0)=VIF(26,1)=VIF(26,2)=VIF(26,3)=VIF(26,4)=1;VIF(27,17)=VIF(27,18)=VIF(27,27)=VIF(27,28)=1;VIF(28,11)=VIF(28,12)=VIF(28,21)=VIF(28,22)=1;VIF(29,0)=VIF(29,1)=VIF(29,5)=1;VIF(30,1)=VIF(30,5)=VIF(30,6)=VIF(30,11)=VIF(30,12)=1;VIF(31,9)=VIF(31,18)=VIF(31,19)=1;VIF(32,18)=VIF(32,19)=VIF(32,28)=VIF(32,29)=1;VIF(33,5)=VIF(33,10)=VIF(33,11)=1;VIF(34,0)=VIF(34,5)=VIF(34,9)=VIF(34,10)=VIF(34,19)=1;VIF(35,10)=VIF(35,11)=VIF(35,20)=VIF(35,21)=1;VIF(36,10)=VIF(36,19)=VIF(36,20)=VIF(36,29)=1;
  
  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J41: Elongated pentagonal gyrocupolarotunda" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);
  
  return p;
}
    
//FIXME: coordinates #830
perl::Object elongated_pentagonal_orthobirotunda()
{
  double a = sqrt((5-sqrt(5))/2); //side length
  
  Matrix<double> V = pentagonal_rotunda().give("VERTICES");
  
  Matrix<double> U = unit_matrix<double>(4);
  U(3,3)=-1;
  V /=  ((V*U) - (zero_matrix<double>(20,3) | same_element_vector(a,20))); 
  
  IncidenceMatrix<> VIF(42,40);
  VIF(0,20)=VIF(0,25)=VIF(0,29)=VIF(0,30)=VIF(0,39)=1;VIF(1,25)=VIF(1,30)=VIF(1,31)=1;VIF(2,29)=VIF(2,38)=VIF(2,39)=1;VIF(3,21)=VIF(3,25)=VIF(3,26)=VIF(3,31)=VIF(3,32)=1;VIF(4,20)=VIF(4,21)=VIF(4,25)=1;VIF(5,24)=VIF(5,28)=VIF(5,29)=VIF(5,37)=VIF(5,38)=1;VIF(6,20)=VIF(6,24)=VIF(6,29)=1;VIF(7,20)=VIF(7,21)=VIF(7,22)=VIF(7,23)=VIF(7,24)=1;VIF(8,21)=VIF(8,22)=VIF(8,26)=1;VIF(9,23)=VIF(9,24)=VIF(9,28)=1;VIF(10,26)=VIF(10,32)=VIF(10,33)=1;VIF(11,28)=VIF(11,36)=VIF(11,37)=1;VIF(12,22)=VIF(12,26)=VIF(12,27)=VIF(12,33)=VIF(12,34)=1;VIF(13,23)=VIF(13,27)=VIF(13,28)=VIF(13,35)=VIF(13,36)=1;VIF(14,22)=VIF(14,23)=VIF(14,27)=1;VIF(15,14)=VIF(15,15)=VIF(15,34)=VIF(15,35)=1;VIF(16,27)=VIF(16,34)=VIF(16,35)=1;VIF(17,7)=VIF(17,14)=VIF(17,15)=1;VIF(18,15)=VIF(18,16)=VIF(18,35)=VIF(18,36)=1;VIF(19,13)=VIF(19,14)=VIF(19,33)=VIF(19,34)=1;VIF(20,2)=VIF(20,3)=VIF(20,7)=1;VIF(21,3)=VIF(21,7)=VIF(21,8)=VIF(21,15)=VIF(21,16)=1;VIF(22,2)=VIF(22,6)=VIF(22,7)=VIF(22,13)=VIF(22,14)=1;VIF(23,8)=VIF(23,16)=VIF(23,17)=1;VIF(24,16)=VIF(24,17)=VIF(24,36)=VIF(24,37)=1;VIF(25,6)=VIF(25,12)=VIF(25,13)=1;VIF(26,12)=VIF(26,13)=VIF(26,32)=VIF(26,33)=1;VIF(27,3)=VIF(27,4)=VIF(27,8)=1;VIF(28,1)=VIF(28,2)=VIF(28,6)=1;VIF(29,0)=VIF(29,1)=VIF(29,2)=VIF(29,3)=VIF(29,4)=1;VIF(30,17)=VIF(30,18)=VIF(30,37)=VIF(30,38)=1;VIF(31,11)=VIF(31,12)=VIF(31,31)=VIF(31,32)=1;VIF(32,0)=VIF(32,4)=VIF(32,9)=1;VIF(33,4)=VIF(33,8)=VIF(33,9)=VIF(33,17)=VIF(33,18)=1;VIF(34,0)=VIF(34,1)=VIF(34,5)=1;VIF(35,1)=VIF(35,5)=VIF(35,6)=VIF(35,11)=VIF(35,12)=1;VIF(36,9)=VIF(36,18)=VIF(36,19)=1;VIF(37,18)=VIF(37,19)=VIF(37,38)=VIF(37,39)=1;VIF(38,5)=VIF(38,10)=VIF(38,11)=1;VIF(39,0)=VIF(39,5)=VIF(39,9)=VIF(39,10)=VIF(39,19)=1;VIF(40,10)=VIF(40,11)=VIF(40,30)=VIF(40,31)=1;VIF(41,10)=VIF(41,19)=VIF(41,30)=VIF(41,39)=1;
  
  
  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J42: Elongated pentagonal orthobirotunda" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);
  
  return p;
}
    
//FIXME: coordinates #830
perl::Object elongated_pentagonal_gyrobirotunda()
{
  double a = sqrt((5-sqrt(5))/2); //side length
  double h = a*sqrt(1+2/sqrt(5)); //height
  double rp = (1+sqrt(5))/2; //radius of inner pentagon
  
  Matrix<double> V = elongated_pentagonal_rotunda().give("VERTICES");
  Matrix<double> rV = (create_regular_polygon_vertices(5,1,M_PI/5) | same_element_vector<double>(-h-a,5))/ (create_regular_polygon_vertices(5,rp,0) | same_element_vector<double>(-1-a,5));
  
  V = V / rV;
    
  IncidenceMatrix<> VIF(42,40);
  VIF(0,20)=VIF(0,21)=VIF(0,30)=VIF(0,35)=VIF(0,36)=1;VIF(1,20)=VIF(1,29)=VIF(1,35)=1;VIF(2,28)=VIF(2,29)=VIF(2,34)=VIF(2,35)=VIF(2,39)=1;VIF(3,1)=VIF(3,5)=VIF(3,6)=VIF(3,11)=VIF(3,12)=1;VIF(4,0)=VIF(4,1)=VIF(4,5)=1;VIF(5,11)=VIF(5,12)=VIF(5,21)=VIF(5,22)=1;VIF(6,17)=VIF(6,18)=VIF(6,27)=VIF(6,28)=1;VIF(7,0)=VIF(7,1)=VIF(7,2)=VIF(7,3)=VIF(7,4)=1;VIF(8,33)=VIF(8,34)=VIF(8,39)=1;VIF(9,26)=VIF(9,27)=VIF(9,33)=VIF(9,38)=VIF(9,39)=1;VIF(10,3)=VIF(10,4)=VIF(10,8)=1;VIF(11,32)=VIF(11,33)=VIF(11,38)=1;VIF(12,3)=VIF(12,7)=VIF(12,8)=VIF(12,15)=VIF(12,16)=1;VIF(13,13)=VIF(13,14)=VIF(13,23)=VIF(13,24)=1;VIF(14,15)=VIF(14,16)=VIF(14,25)=VIF(14,26)=1;VIF(15,14)=VIF(15,15)=VIF(15,24)=VIF(15,25)=1;VIF(16,7)=VIF(16,14)=VIF(16,15)=1;VIF(17,25)=VIF(17,26)=VIF(17,38)=1;VIF(18,23)=VIF(18,24)=VIF(18,37)=1;VIF(19,24)=VIF(19,25)=VIF(19,32)=VIF(19,37)=VIF(19,38)=1;VIF(20,2)=VIF(20,3)=VIF(20,7)=1;VIF(21,2)=VIF(21,6)=VIF(21,7)=VIF(21,13)=VIF(21,14)=1;VIF(22,31)=VIF(22,32)=VIF(22,37)=1;VIF(23,8)=VIF(23,16)=VIF(23,17)=1;VIF(24,16)=VIF(24,17)=VIF(24,26)=VIF(24,27)=1;VIF(25,6)=VIF(25,12)=VIF(25,13)=1;VIF(26,12)=VIF(26,13)=VIF(26,22)=VIF(26,23)=1;VIF(27,1)=VIF(27,2)=VIF(27,6)=1;VIF(28,22)=VIF(28,23)=VIF(28,31)=VIF(28,36)=VIF(28,37)=1;VIF(29,30)=VIF(29,31)=VIF(29,36)=1;VIF(30,30)=VIF(30,31)=VIF(30,32)=VIF(30,33)=VIF(30,34)=1;VIF(31,27)=VIF(31,28)=VIF(31,39)=1;VIF(32,21)=VIF(32,22)=VIF(32,36)=1;VIF(33,0)=VIF(33,4)=VIF(33,9)=1;VIF(34,4)=VIF(34,8)=VIF(34,9)=VIF(34,17)=VIF(34,18)=1;VIF(35,30)=VIF(35,34)=VIF(35,35)=1;VIF(36,9)=VIF(36,18)=VIF(36,19)=1;VIF(37,18)=VIF(37,19)=VIF(37,28)=VIF(37,29)=1;VIF(38,5)=VIF(38,10)=VIF(38,11)=1;VIF(39,0)=VIF(39,5)=VIF(39,9)=VIF(39,10)=VIF(39,19)=1;VIF(40,10)=VIF(40,11)=VIF(40,20)=VIF(40,21)=1;VIF(41,10)=VIF(41,19)=VIF(41,20)=VIF(41,29)=1;
  
  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
  p.set_description() << "Johnson solid J43: Elongated pentagonal gyrobirotunda" << endl;
  p.take("VERTICES") << V;
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);
  
  return p;
}

//FIXME: coordinates #830
perl::Object gyroelongated_square_bicupola()
{
	perl::Object q = gyroelongated_square_cupola();
	Matrix<double> V = q.give("VERTICES");	
	Matrix<double> W = create_regular_polygon_vertices(4, sqrt(2), M_PI/8);
	Matrix<double> X = (W.minor(All, sequence(0,3)) | same_element_vector<double>(V(0,3)-sqrt(2),4)) / V;

	IncidenceMatrix<> VIFofq = q.give("VERTICES_IN_FACETS");
	IncidenceMatrix<> VIF(34,24);
	VIF(0,0)=VIF(0,1)=VIF(0,2)=VIF(0,3)=1;
  VIF(1,2)=VIF(1,3)=VIF(1,9)=VIF(1,10)=1;
	VIF(2,1)=VIF(2,2)=VIF(2,7)=VIF(2,8)=1;
	VIF(3,0)=VIF(3,1)=VIF(3,5)=VIF(3,6)=1;
	VIF(4,0)=VIF(4,3)=VIF(4,4)=VIF(4,11)=1;
	VIF(5,3)=VIF(5,10)=VIF(5,11)=1;
	VIF(6,2)=VIF(6,8)=VIF(6,9)=1;
	VIF(7,1)=VIF(7,6)=VIF(7,7)=1;
	VIF(8,0)=VIF(8,4)=VIF(8,5)=1;

	for (int i=0; i<26; ++i) {
		for (int j=0; j<20; ++j) {
			if (VIFofq.exists(i,j)) {
				if (i<16)				
					VIF(i+9,j+4)=1;
				if (i>16)
					VIF(i+8,j+4)=1;
			} 	
		}
	}

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
	p.set_description() << "Johnson solid J45: Gyroelongated square bicupola" << endl;  
	
	p.take("VERTICES") << X;
	p.take("VERTICES_IN_FACETS") << VIF;
	assign_common_props<double>(p);

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
  VIF(0,0)=VIF(0,1)=VIF(0,4)=VIF(0,5)=1;
  VIF(1,0)=VIF(1,2)=VIF(1,6)=1;
  VIF(2,0)=VIF(2,2)=VIF(2,5)=1;
  VIF(3,0)=VIF(3,1)=VIF(3,6)=1;
  VIF(4,1)=VIF(4,3)=VIF(4,6)=1;
  VIF(5,2)=VIF(5,3)=VIF(5,6)=1;
  VIF(6,1)=VIF(6,3)=VIF(6,4)=1;
  VIF(7,2)=VIF(7,3)=VIF(7,4)=VIF(7,5)=1;

  perl::Object p(perl::ObjectType::construct<double>("Polytope"));
        p.set_description() << "Johnson solid J49: augmented triangular prism" << endl;
        p.take("VERTICES") << V;
        p.take("VERTICES_IN_FACETS") << VIF;
        assign_common_props<QE>(p);

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
        
  p = augment(p,0);
  p = augment(p,6);
  
  p.set_description() << "Johnson solid J50: biaugmented triangular prism" << endl;

  IncidenceMatrix<> VIF(11,8);
  VIF(0,1)=VIF(0,4)=VIF(0,6)=1;VIF(1,4)=VIF(1,5)=VIF(1,6)=1;VIF(2,0)=VIF(2,2)=VIF(2,7)=1;VIF(3,0)=VIF(3,2)=VIF(3,5)=1;VIF(4,0)=VIF(4,5)=VIF(4,6)=1;VIF(5,0)=VIF(5,1)=VIF(5,6)=1;VIF(6,0)=VIF(6,1)=VIF(6,7)=1;VIF(7,1)=VIF(7,3)=VIF(7,7)=1;VIF(8,2)=VIF(8,3)=VIF(8,7)=1;VIF(9,1)=VIF(9,3)=VIF(9,4)=1;VIF(10,2)=VIF(10,3)=VIF(10,4)=VIF(10,5)=1;

  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}
      
perl::Object triaugmented_triangular_prism(){

  perl::Object p = create_prism(3);
        
  p = augment(p,0);
  p = augment(p,6);
  p = augment(p,10);
  
  p.set_description() << "Johnson solid J51: triaugmented triangular prism" << endl;

  IncidenceMatrix<> VIF(14,9);
  VIF(0,0)=VIF(0,1)=VIF(0,8)=1;VIF(1,0)=VIF(1,2)=VIF(1,7)=1;VIF(2,0)=VIF(2,1)=VIF(2,2)=1;VIF(3,2)=VIF(3,5)=VIF(3,7)=1;VIF(4,1)=VIF(4,2)=VIF(4,6)=1;VIF(5,2)=VIF(5,5)=VIF(5,6)=1;VIF(6,4)=VIF(6,5)=VIF(6,6)=1;VIF(7,1)=VIF(7,4)=VIF(7,6)=1;VIF(8,1)=VIF(8,4)=VIF(8,8)=1;VIF(9,3)=VIF(9,4)=VIF(9,5)=1;VIF(10,3)=VIF(10,5)=VIF(10,7)=1;VIF(11,3)=VIF(11,4)=VIF(11,8)=1;VIF(12,0)=VIF(12,3)=VIF(12,7)=1;VIF(13,0)=VIF(13,3)=VIF(13,8)=1;

  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}
      
perl::Object augmented_pentagonal_prism(){

  perl::Object p = create_prism(5);
        
  p = augment(p,1);
  
  p.set_description() << "Johnson solid J52: augmented pentagonal prism" << endl;

  IncidenceMatrix<> VIF(10,11);
  VIF(0,0)=VIF(0,1)=VIF(0,2)=VIF(0,3)=VIF(0,4)=1;VIF(1,2)=VIF(1,3)=VIF(1,10)=1;VIF(2,3)=VIF(2,8)=VIF(2,10)=1;VIF(3,7)=VIF(3,8)=VIF(3,10)=1;VIF(4,2)=VIF(4,7)=VIF(4,10)=1;VIF(5,3)=VIF(5,4)=VIF(5,8)=VIF(5,9)=1;VIF(6,1)=VIF(6,2)=VIF(6,6)=VIF(6,7)=1;VIF(7,5)=VIF(7,6)=VIF(7,7)=VIF(7,8)=VIF(7,9)=1;VIF(8,0)=VIF(8,4)=VIF(8,5)=VIF(8,9)=1;VIF(9,0)=VIF(9,1)=VIF(9,5)=VIF(9,6)=1;

  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}
      
perl::Object biaugmented_pentagonal_prism(){

  perl::Object p = create_prism(5);
        
  p = augment(p,1);
  p = augment(p,8);
  
  p.set_description() << "Johnson solid J53: biaugmented pentagonal prism" << endl;

  IncidenceMatrix<> VIF(13,12);
VIF(0,0)=VIF(0,1)=VIF(0,5)=VIF(0,6)=1;VIF(1,5)=VIF(1,6)=VIF(1,7)=VIF(1,8)=VIF(1,9)=1;VIF(2,1)=VIF(2,2)=VIF(2,6)=VIF(2,7)=1;VIF(3,3)=VIF(3,4)=VIF(3,8)=VIF(3,9)=1;VIF(4,2)=VIF(4,7)=VIF(4,10)=1;VIF(5,7)=VIF(5,8)=VIF(5,10)=1;VIF(6,3)=VIF(6,8)=VIF(6,10)=1;VIF(7,2)=VIF(7,3)=VIF(7,10)=1;VIF(8,0)=VIF(8,1)=VIF(8,2)=VIF(8,3)=VIF(8,4)=1;VIF(9,4)=VIF(9,9)=VIF(9,11)=1;VIF(10,0)=VIF(10,4)=VIF(10,11)=1;VIF(11,5)=VIF(11,9)=VIF(11,11)=1;VIF(12,0)=VIF(12,5)=VIF(12,11)=1;
 
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}
      
      
perl::Object augmented_hexagonal_prism(){

  perl::Object p = create_prism(6);
        
  p = augment(p,1);
  
  p.set_description() << "Johnson solid J54: augmented hexagonal prism" << endl;

  IncidenceMatrix<> VIF(11,13);
  VIF(0,0)=VIF(0,1)=VIF(0,2)=VIF(0,3)=VIF(0,4)=VIF(0,5)=1;VIF(1,3)=VIF(1,4)=VIF(1,12)=1;VIF(2,3)=VIF(2,9)=VIF(2,12)=1;VIF(3,9)=VIF(3,10)=VIF(3,12)=1;VIF(4,4)=VIF(4,10)=VIF(4,12)=1;VIF(5,2)=VIF(5,3)=VIF(5,8)=VIF(5,9)=1;VIF(6,4)=VIF(6,5)=VIF(6,10)=VIF(6,11)=1;VIF(7,1)=VIF(7,2)=VIF(7,7)=VIF(7,8)=1;VIF(8,6)=VIF(8,7)=VIF(8,8)=VIF(8,9)=VIF(8,10)=VIF(8,11)=1;VIF(9,0)=VIF(9,5)=VIF(9,6)=VIF(9,11)=1;VIF(10,0)=VIF(10,1)=VIF(10,6)=VIF(10,7)=1;
  
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}
      
perl::Object parabiaugmented_hexagonal_prism(){

  perl::Object p = create_prism(6);
        
  p = augment(p,1);
  p = augment(p,10);
  
  p.set_description() << "Johnson solid J55: parabiaugmented hexagonal prism" << endl;

  IncidenceMatrix<> VIF(14,14);
  VIF(0,0)=VIF(0,5)=VIF(0,6)=VIF(0,11)=1;VIF(1,6)=VIF(1,7)=VIF(1,8)=VIF(1,9)=VIF(1,10)=VIF(1,11)=1;VIF(2,1)=VIF(2,2)=VIF(2,7)=VIF(2,8)=1;VIF(3,4)=VIF(3,5)=VIF(3,10)=VIF(3,11)=1;VIF(4,2)=VIF(4,3)=VIF(4,8)=VIF(4,9)=1;VIF(5,4)=VIF(5,10)=VIF(5,12)=1;VIF(6,9)=VIF(6,10)=VIF(6,12)=1;VIF(7,3)=VIF(7,9)=VIF(7,12)=1;VIF(8,3)=VIF(8,4)=VIF(8,12)=1;VIF(9,0)=VIF(9,1)=VIF(9,2)=VIF(9,3)=VIF(9,4)=VIF(9,5)=1;VIF(10,1)=VIF(10,7)=VIF(10,13)=1;VIF(11,0)=VIF(11,1)=VIF(11,13)=1;VIF(12,6)=VIF(12,7)=VIF(12,13)=1;VIF(13,0)=VIF(13,6)=VIF(13,13)=1;
  
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
  
}

perl::Object metabiaugmented_hexagonal_prism(){

  perl::Object p = create_prism(6);
        
  p = augment(p,1);
  p = augment(p,7);
  
  p.set_description() << "Johnson solid J56: metabiaugmented hexagonal prism" << endl;

  IncidenceMatrix<> VIF(14,14);
  VIF(0,0)=VIF(0,1)=VIF(0,2)=VIF(0,3)=VIF(0,4)=VIF(0,5)=1;VIF(1,1)=VIF(1,2)=VIF(1,13)=1;VIF(2,2)=VIF(2,3)=VIF(2,8)=VIF(2,9)=1;VIF(3,4)=VIF(3,10)=VIF(3,12)=1;VIF(4,9)=VIF(4,10)=VIF(4,12)=1;VIF(5,3)=VIF(5,9)=VIF(5,12)=1;VIF(6,3)=VIF(6,4)=VIF(6,12)=1;VIF(7,2)=VIF(7,8)=VIF(7,13)=1;VIF(8,4)=VIF(8,5)=VIF(8,10)=VIF(8,11)=1;VIF(9,7)=VIF(9,8)=VIF(9,13)=1;VIF(10,1)=VIF(10,7)=VIF(10,13)=1;VIF(11,6)=VIF(11,7)=VIF(11,8)=VIF(11,9)=VIF(11,10)=VIF(11,11)=1;VIF(12,0)=VIF(12,5)=VIF(12,6)=VIF(12,11)=1;VIF(13,0)=VIF(13,1)=VIF(13,6)=VIF(13,7)=1;
  
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}

perl::Object triaugmented_hexagonal_prism(){

  perl::Object p = create_prism(6);
        
  p = augment(p,1);
  p = augment(p,7);
  p = augment(p,12);
  
  p.set_description() << "Johnson solid J57: triaugmented hexagonal prism" << endl;

  IncidenceMatrix<> VIF(18,15);
  VIF(0,0)=VIF(0,1)=VIF(0,6)=VIF(0,7)=1;VIF(1,6)=VIF(1,7)=VIF(1,8)=VIF(1,9)=VIF(1,10)=VIF(1,11)=1;VIF(2,1)=VIF(2,7)=VIF(2,13)=1;VIF(3,7)=VIF(3,8)=VIF(3,13)=1;VIF(4,4)=VIF(4,5)=VIF(4,10)=VIF(4,11)=1;VIF(5,2)=VIF(5,8)=VIF(5,13)=1;VIF(6,3)=VIF(6,4)=VIF(6,12)=1;VIF(7,3)=VIF(7,9)=VIF(7,12)=1;VIF(8,9)=VIF(8,10)=VIF(8,12)=1;VIF(9,4)=VIF(9,10)=VIF(9,12)=1;VIF(10,2)=VIF(10,3)=VIF(10,8)=VIF(10,9)=1;VIF(11,1)=VIF(11,2)=VIF(11,13)=1;VIF(12,0)=VIF(12,1)=VIF(12,2)=VIF(12,3)=VIF(12,4)=VIF(12,5)=1;VIF(13,5)=VIF(13,11)=VIF(13,14)=1;VIF(14,0)=VIF(14,5)=VIF(14,14)=1;VIF(15,6)=VIF(15,11)=VIF(15,14)=1;VIF(16,0)=VIF(16,6)=VIF(16,14)=1;
  
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}
     
perl::Object augmented_dodecahedron(){

   //TODO import wythoff instead
  perl::Object p = CallPolymakeFunction("dodecahedron");
        
  p = augment(p,0);
  
  p.set_description() << "Johnson solid J58: augmented dodecahedron" << endl;

  IncidenceMatrix<> VIF(16,21);
  VIF(0,8)=VIF(0,9)=VIF(0,13)=VIF(0,16)=VIF(0,18)=1;VIF(1,2)=VIF(1,5)=VIF(1,8)=VIF(1,12)=VIF(1,13)=1;VIF(2,0)=VIF(2,1)=VIF(2,2)=VIF(2,3)=VIF(2,5)=1;VIF(3,12)=VIF(3,13)=VIF(3,15)=VIF(3,18)=VIF(3,19)=1;VIF(4,3)=VIF(4,5)=VIF(4,10)=VIF(4,12)=VIF(4,15)=1;VIF(5,1)=VIF(5,3)=VIF(5,6)=VIF(5,10)=VIF(5,11)=1;VIF(6,10)=VIF(6,11)=VIF(6,15)=VIF(6,17)=VIF(6,19)=1;VIF(7,6)=VIF(7,7)=VIF(7,11)=VIF(7,14)=VIF(7,17)=1;VIF(8,14)=VIF(8,16)=VIF(8,17)=VIF(8,18)=VIF(8,19)=1;VIF(9,0)=VIF(9,1)=VIF(9,4)=VIF(9,6)=VIF(9,7)=1;VIF(10,4)=VIF(10,7)=VIF(10,9)=VIF(10,14)=VIF(10,16)=1;VIF(11,0)=VIF(11,4)=VIF(11,20)=1;VIF(12,0)=VIF(12,2)=VIF(12,20)=1;VIF(13,4)=VIF(13,9)=VIF(13,20)=1;VIF(14,2)=VIF(14,8)=VIF(14,20)=1;VIF(15,8)=VIF(15,9)=VIF(15,20)=1;
  
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}
      
perl::Object parabiaugmented_dodecahedron(){

  perl::Object p = augmented_dodecahedron();
        
  p = augment(p,6);
  
  p.set_description() << "Johnson solid J59: parabiaugmented dodecahedron" << endl;

  IncidenceMatrix<> VIF(20,22);
  VIF(0,8)=VIF(0,9)=VIF(0,13)=VIF(0,16)=VIF(0,18)=1;VIF(1,2)=VIF(1,5)=VIF(1,8)=VIF(1,12)=VIF(1,13)=1;VIF(2,0)=VIF(2,1)=VIF(2,2)=VIF(2,3)=VIF(2,5)=1;VIF(3,12)=VIF(3,13)=VIF(3,15)=VIF(3,18)=VIF(3,19)=1;VIF(4,3)=VIF(4,5)=VIF(4,10)=VIF(4,12)=VIF(4,15)=1;VIF(5,1)=VIF(5,3)=VIF(5,6)=VIF(5,10)=VIF(5,11)=1;VIF(6,10)=VIF(6,15)=VIF(6,21)=1;VIF(7,10)=VIF(7,11)=VIF(7,21)=1;VIF(8,11)=VIF(8,17)=VIF(8,21)=1;VIF(9,17)=VIF(9,19)=VIF(9,21)=1;VIF(10,15)=VIF(10,19)=VIF(10,21)=1;VIF(11,6)=VIF(11,7)=VIF(11,11)=VIF(11,14)=VIF(11,17)=1;VIF(12,14)=VIF(12,16)=VIF(12,17)=VIF(12,18)=VIF(12,19)=1;VIF(13,0)=VIF(13,1)=VIF(13,4)=VIF(13,6)=VIF(13,7)=1;VIF(14,4)=VIF(14,7)=VIF(14,9)=VIF(14,14)=VIF(14,16)=1;VIF(15,0)=VIF(15,4)=VIF(15,20)=1;VIF(16,0)=VIF(16,2)=VIF(16,20)=1;VIF(17,4)=VIF(17,9)=VIF(17,20)=1;VIF(18,2)=VIF(18,8)=VIF(18,20)=1;VIF(19,8)=VIF(19,9)=VIF(19,20)=1;
  
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}
      
perl::Object metabiaugmented_dodecahedron(){

  perl::Object p = augmented_dodecahedron();
        
  p = augment(p,3);
  
  p.set_description() << "Johnson solid J60: metabiaugmented dodecahedron" << endl;

  IncidenceMatrix<> VIF(20,22);
  VIF(0,8)=VIF(0,9)=VIF(0,13)=VIF(0,16)=VIF(0,18)=1;VIF(1,2)=VIF(1,5)=VIF(1,8)=VIF(1,12)=VIF(1,13)=1;VIF(2,0)=VIF(2,1)=VIF(2,2)=VIF(2,3)=VIF(2,5)=1;VIF(3,13)=VIF(3,18)=VIF(3,21)=1;VIF(4,12)=VIF(4,13)=VIF(4,21)=1;VIF(5,3)=VIF(5,5)=VIF(5,10)=VIF(5,12)=VIF(5,15)=1;VIF(6,12)=VIF(6,15)=VIF(6,21)=1;VIF(7,15)=VIF(7,19)=VIF(7,21)=1;VIF(8,10)=VIF(8,11)=VIF(8,15)=VIF(8,17)=VIF(8,19)=1;VIF(9,1)=VIF(9,3)=VIF(9,6)=VIF(9,10)=VIF(9,11)=1;VIF(10,6)=VIF(10,7)=VIF(10,11)=VIF(10,14)=VIF(10,17)=1;VIF(11,18)=VIF(11,19)=VIF(11,21)=1;VIF(12,14)=VIF(12,16)=VIF(12,17)=VIF(12,18)=VIF(12,19)=1;VIF(13,0)=VIF(13,1)=VIF(13,4)=VIF(13,6)=VIF(13,7)=1;VIF(14,4)=VIF(14,7)=VIF(14,9)=VIF(14,14)=VIF(14,16)=1;VIF(15,0)=VIF(15,4)=VIF(15,20)=1;VIF(16,0)=VIF(16,2)=VIF(16,20)=1;VIF(17,4)=VIF(17,9)=VIF(17,20)=1;VIF(18,2)=VIF(18,8)=VIF(18,20)=1;VIF(19,8)=VIF(19,9)=VIF(19,20)=1;

  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}
      
perl::Object triaugmented_dodecahedron(){

  perl::Object p = metabiaugmented_dodecahedron();

  p = augment(p,9);
  
  p.set_description() << "Johnson solid J60: metabiaugmented dodecahedron" << endl;

  IncidenceMatrix<> VIF(24,23);
  VIF(0,8)=VIF(0,9)=VIF(0,13)=VIF(0,16)=VIF(0,18)=1;VIF(1,2)=VIF(1,5)=VIF(1,8)=VIF(1,12)=VIF(1,13)=1;VIF(2,0)=VIF(2,1)=VIF(2,2)=VIF(2,3)=VIF(2,5)=1;VIF(3,13)=VIF(3,18)=VIF(3,21)=1;VIF(4,12)=VIF(4,13)=VIF(4,21)=1;VIF(5,3)=VIF(5,5)=VIF(5,10)=VIF(5,12)=VIF(5,15)=1;VIF(6,12)=VIF(6,15)=VIF(6,21)=1;VIF(7,15)=VIF(7,19)=VIF(7,21)=1;VIF(8,10)=VIF(8,11)=VIF(8,15)=VIF(8,17)=VIF(8,19)=1;VIF(9,6)=VIF(9,11)=VIF(9,22)=1;VIF(10,10)=VIF(10,11)=VIF(10,22)=1;VIF(11,3)=VIF(11,10)=VIF(11,22)=1;VIF(12,1)=VIF(12,6)=VIF(12,22)=1;VIF(13,1)=VIF(13,3)=VIF(13,22)=1;VIF(14,6)=VIF(14,7)=VIF(14,11)=VIF(14,14)=VIF(14,17)=1;VIF(15,18)=VIF(15,19)=VIF(15,21)=1;VIF(16,14)=VIF(16,16)=VIF(16,17)=VIF(16,18)=VIF(16,19)=1;VIF(17,0)=VIF(17,1)=VIF(17,4)=VIF(17,6)=VIF(17,7)=1;VIF(18,4)=VIF(18,7)=VIF(18,9)=VIF(18,14)=VIF(18,16)=1;VIF(19,0)=VIF(19,4)=VIF(19,20)=1;VIF(20,0)=VIF(20,2)=VIF(20,20)=1;VIF(21,4)=VIF(21,9)=VIF(21,20)=1;VIF(22,2)=VIF(22,8)=VIF(22,20)=1;VIF(23,8)=VIF(23,9)=VIF(23,20)=1;
  
  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}
    
perl::Object metabidiminished_icosahedron(){

   //TODO import wythoff instead
  perl::Object ico = CallPolymakeFunction("icosahedron");

  Matrix<QE> V = ico.give("VERTICES");

  V = ((V.minor(sequence(1,5),All)) / (V.minor(sequence(7,5),All)));
  
  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p.set_description() << "Johnson solid J62: metabidiminished icosahedron" << endl;
  assign_common_props<QE>(p);

  return p;
}
perl::Object tridiminished_icosahedron(){

   perl::Object dimico = metabidiminished_icosahedron();

  Matrix<QE> V = dimico.give("VERTICES");
  V = ((V.minor(sequence(0,7),All)) / (V.minor(sequence(8,2),All)));
  
  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.take("VERTICES") << V;
  p.set_description() << "Johnson solid J63: tridiminished icosahedron" << endl;
  assign_common_props<QE>(p);

  return p;
}
      
perl::Object augmented_tridiminished_icosahedron(){

  perl::Object p = tridiminished_icosahedron();
  p = augment(p,0);
  
  p.set_description() << "Johnson solid J64: augmented_tridiminished icosahedron" << endl;

  IncidenceMatrix<> VIF(10,10);
  VIF(0,3)=VIF(0,6)=VIF(0,7)=1;VIF(1,6)=VIF(1,7)=VIF(1,8)=1;VIF(2,0)=VIF(2,5)=VIF(2,9)=1;VIF(3,0)=VIF(3,2)=VIF(3,9)=1;VIF(4,2)=VIF(4,5)=VIF(4,9)=1;VIF(5,2)=VIF(5,4)=VIF(5,5)=VIF(5,7)=VIF(5,8)=1;VIF(6,3)=VIF(6,4)=VIF(6,7)=1;VIF(7,1)=VIF(7,3)=VIF(7,6)=1;VIF(8,0)=VIF(8,1)=VIF(8,2)=VIF(8,3)=VIF(8,4)=1;VIF(9,0)=VIF(9,1)=VIF(9,5)=VIF(9,6)=VIF(9,8)=1;

  p.take("VERTICES_IN_FACETS") << VIF;
  assign_common_props<QE>(p);

  return p;
}
     
     
perl::Object augmented_truncated_tetrahedron()
{
	Rational r(1,3);
  Matrix<QE> V(12,4);
	V.col(0).fill(1);
	V(0,1)=V(1,2)=V(2,3)=V(3,1)=V(6,2)=V(10,3)=1;  
	V(4,2)=V(5,3)=V(7,3)=V(8,1)=V(9,1)=V(11,2)=-1;
	V(0,2)=V(0,3)=V(1,1)=V(1,3)=V(2,1)=V(2,2)=V(4,1)=V(5,1)=V(7,2)=V(8,2)=V(9,3)=V(11,3)=r;
	V(3,2)=V(3,3)=V(4,3)=V(5,2)=V(6,1)=V(6,3)=V(7,1)=V(8,3)=V(9,2)=V(10,1)=V(10,2)=V(11,1)=-r;

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.set_description() << "Johnson solid J65: Augmented truncated tetrahedron" << endl;
  p.take("VERTICES") << V;
  assign_common_props<QE>(p);

  return p;
}

perl::Object augmented_truncated_cube()
{
  Matrix<QE> V = truncated_cube_vertices();
  Matrix<QE> W = square_cupola().give("VERTICES");
  W.col(3) += same_element_vector(QE(2,2,2),12);

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
  p.set_description() << "Johnson solid J66: Augmented truncated cube" << endl;
  p.take("VERTICES") << V / W.minor(sequence(8,4),All);
  assign_common_props<QE>(p);

  return p;
}
      
perl::Object biaugmented_truncated_cube()
{
  Matrix<QE> V = augmented_truncated_cube().give("VERTICES");
	Matrix<QE> W = square_cupola().give("VERTICES");

  perl::Object p(perl::ObjectType::construct<QE>("Polytope"));
	p.set_description() << "Johnson solid J67: Biaugmented truncated cube" << endl;  
	p.take("VERTICES") << V / (ones_vector<QE>(4) | (-1)*W.minor(sequence(8,4),sequence(1,3)));
	assign_common_props<QE>(p);

  return p;  
}
      
perl::Object johnson_dispatcher(std::string s, int n){
  if(s=="square_pyramid" || n==1)
    return square_pyramid();
  if(s=="pentagonal_pyramid" || n==2) //inexact
    return pentagonal_pyramid();
  if(s=="triangular_cupola" || n==3)
    return triangular_cupola();
  if(s=="square_cupola" || n==4)
    return square_cupola();
  if(s=="pentagonal_cupola" || n==5) //inexact
    return pentagonal_cupola();
  if(s=="pentagonal_rotunda" || n==6) //inexact
    return pentagonal_rotunda();
  if(s=="elongated_triangular_pyramid" || n==7)
    return elongated_triangular_pyramid();
  if(s=="elongated_square_pyramid" || n==8)
    return elongated_square_pyramid();
  if(s=="elongated_pentagonal_pyramid" || n==9) //inexact
    return elongated_pentagonal_pyramid();
  if(s=="gyroelongated_square_pyramid" || n==10) //inexact
    return gyroelongated_square_pyramid();
  if(s=="gyroelongated_pentagonal_pyramid" || n==11) //inexact
    return gyroelongated_pentagonal_pyramid();
  if(s=="triangular_bipyramid" || n==12)
    return triangular_bipyramid();
  if(s=="pentagonal_bipyramid()" || n==13) //inexact
    return pentagonal_bipyramid();
  if(s=="elongated_triangular_bipyramid" || n==14)
    return elongated_triangular_bipyramid();
  if(s=="elongated_square_bipyramid()" || n==15)
    return elongated_square_bipyramid();
  if(s=="elongated_pentagonal_bipyramid()" || n==16) //inexact
    return elongated_pentagonal_bipyramid();
  if(s=="gyroelongated_square_bipyramid()" || n==17) //inexact
    return gyroelongated_square_bipyramid();
  if(s=="elongated_triangular_cupola()" || n==18) //inexact
    return elongated_triangular_cupola();
  if(s=="elongated_square_cupola" || n==19)
    return elongated_square_cupola();
  if(s=="elongated_pentagonal_cupola" || n==20) //inexact
    return elongated_pentagonal_cupola();
  if(s=="elongated_pentagonal_rotunda" || n==21) //inexact
    return elongated_pentagonal_rotunda();
  if(s=="gyroelongated_triangular_cupola" || n==22) //inexact
    return gyroelongated_triangular_cupola();
  if(s=="gyroelongated_square_cupola" || n==23) //inexact
    return gyroelongated_square_cupola();
  if(s=="gyroelongated_pentagonal_cupola" || n==24) //inexact
    return gyroelongated_pentagonal_cupola();
  if(s=="gyroelongated_pentagonal_rotunda" || n==25) //inexact
    return gyroelongated_pentagonal_rotunda();
  if(s=="gyrobifastigium" || n==26)
    return gyrobifastigium();
  if(s=="triangular_orthobicupola" || n==27)
    return triangular_orthobicupola();
  if(s=="square_orthobicupola" || n==28)
    return square_orthobicupola();
  if(s=="square_gyrobicupola" || n==29)
    return square_gyrobicupola();
  if(s=="pentagonal_orthobicupola" || n==30) //inexact
    return pentagonal_orthobicupola();
  if(s=="pentagonal_gyrobicupola" || n==31) //inexact
    return pentagonal_gyrobicupola();
  if(s=="pentagonal_orthocupolarotunda" || n==32) //inexact
    return pentagonal_orthocupolarotunda();
  if(s=="pentagonal_gyrocupolarotunda" || n==33) //inexact
    return pentagonal_gyrocupolarotunda();
  if(s=="pentagonal_orthobirotunda" || n==34) //inexact
    return pentagonal_orthobirotunda();
  if(s=="elongated_triangular_orthobicupola" || n==35) //inexact
    return elongated_triangular_orthobicupola();
  if(s=="elongated_triangular_gyrobicupola" || n==36) //inexact
    return elongated_triangular_gyrobicupola();
  if(s=="elongated_square_gyrobicupola" | n==37) 
    return elongated_square_gyrobicupola();
  if(s=="elongated_pentagonal_orthobicupola" || n==38) //inexact
    return elongated_pentagonal_orthobicupola();
  if(s=="elongated_pentagonal_gyrobicupola" || n==39) //inexact
    return elongated_pentagonal_gyrobicupola();
  if(s=="elongated_pentagonal_orthocupolarotunda" || n==40) //inexact
    return elongated_pentagonal_orthocupolarotunda();
  if(s=="elongated_pentagonal_gyrocupolarotunda" || n==41) //inexact
    return elongated_pentagonal_gyrocupolarotunda();
  if(s=="elongated_pentagonal_orthobirotunda" || n==42) //inexact
    return elongated_pentagonal_orthobirotunda();
  if(s=="elongated_pentagonal_gyrobirotunda" || n==43) //inexact
    return elongated_pentagonal_gyrobirotunda();
  //todo 44 
  if(s=="gyroelongated_square_bicupola" || n==45) //inexact
    return gyroelongated_square_bicupola();
  //todo 46-48
  if(s=="augmented_triangular_prism" || n==49) //inexact
    return augmented_triangular_prism();
  if(s=="biaugmented_triangular_prism" || n==50) //inexact
    return biaugmented_triangular_prism();
  if(s=="triaugmented_triangular_prism" || n==51) //inexact
    return triaugmented_triangular_prism();
  if(s=="augmented_pentagonal_prism" || n==52) //inexact
    return augmented_pentagonal_prism();
  if(s=="biaugmented_pentagonal_prism" || n==53) //inexact
    return biaugmented_pentagonal_prism();
  if(s=="augmented_hexagonal_prism" || n==54) //inexact
    return augmented_hexagonal_prism();
  if(s=="parabiaugmented_hexagonal_prism" || n==55) //inexact
    return parabiaugmented_hexagonal_prism();
  if(s=="metabiaugmented_hexagonal_prism" || n==56) //inexact
    return metabiaugmented_hexagonal_prism();
  if(s=="triaugmented_hexagonal_prism" || n==57) //inexact
    return triaugmented_hexagonal_prism();
  if(s=="augmented_dodecahedron" || n==58) //inexact
    return augmented_dodecahedron();
  if(s=="parabiaugmented_dodecahedron" || n==59) //inexact
    return parabiaugmented_dodecahedron();
  if(s=="metabiaugmented_dodecahedron" || n==60) //inexact
    return metabiaugmented_dodecahedron();
  if(s=="triaugmented_dodecahedron" || n==61) //inexact
    return triaugmented_dodecahedron();
  if(s=="metabidiminished_icosahedron" || n==62)
     return metabidiminished_icosahedron();
  if(s=="tridiminished_icosahedron" || n==63)
     return tridiminished_icosahedron();
  if(s=="augmented_tridiminished_icosahedron" || n==64) //inexact
     return augmented_tridiminished_icosahedron();
  if(s=="augmented truncated tetrahedron" | n==65) //inexact
    return augmented_truncated_tetrahedron();
  if(s=="augmented truncated cube" | n==66) 
    return augmented_truncated_cube();
  if(s=="biaugmented truncated cube" || n==67) 
    return biaugmented_truncated_cube();  

//TODO: to be continued.
  
  else
    throw std::runtime_error("No Johnson polytope of given name/index found.");
}

perl::Object johnson_int(int n){
  return johnson_dispatcher("",n);
}

perl::Object johnson_str(std::string s){
  return johnson_dispatcher(s,0);
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create elongated square gyrobicupola with regular facets. Johnson solid J37."
                  "# @return Polytope",
                  &elongated_square_gyrobicupola, "elongated_square_gyrobicupola()");
    
UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create elongated pentagonal orthobicupola with regular facets. Johnson solid J38."
		  "# The vertices are realized as floating point numbers and thus not exact. Vertex-facet-incidences are correct."
                  "# @return Polytope",
                  &elongated_pentagonal_orthobicupola, "elongated_pentagonal_orthobicupola()");
    
UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create elongated pentagonal gyrobicupola with regular facets. Johnson solid J39."
		  "# The vertices are realized as floating point numbers and thus not exact. Vertex-facet-incidences are correct."
                  "# @return Polytope",
                  &elongated_pentagonal_gyrobicupola, "elongated_pentagonal_gyrobicupola()");
    
UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create elongated pentagonal orthocupolarotunda with regular facets. Johnson solid J40."
		  "# The vertices are realized as floating point numbers and thus not exact. Vertex-facet-incidences are correct."
                  "# @return Polytope",
                  &elongated_pentagonal_orthocupolarotunda, "elongated_pentagonal_orthocupolarotunda()");
    
UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create elongated pentagonal gyrocupolarotunda with regular facets. Johnson solid J41."
		  "# The vertices are realized as floating point numbers and thus not exact. Vertex-facet-incidences are correct."
                  "# @return Polytope",
                  &elongated_pentagonal_gyrocupolarotunda, "elongated_pentagonal_gyrocupolarotunda()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create gyroelongated square bicupola with regular facets. Johnson solid J45."
		  "# The vertices are realized as floating point numbers and thus not exact. Vertex-facet-incidences are correct."
                  "# @return Polytope",
                  &gyroelongated_square_bicupola, "gyroelongated_square_bicupola()");
    
UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create elongated pentagonal orthobirotunda with regular facets. Johnson solid J42."
		  "# The vertices are realized as floating point numbers and thus not exact. Vertex-facet-incidences are correct."
                  "# @return Polytope",
                  &elongated_pentagonal_orthobirotunda, "elongated_pentagonal_orthobirotunda()");
    
UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create elongated pentagonal gyrobirotunda with regular facets. Johnson solid J43."
		  "# The vertices are realized as floating point numbers and thus not exact. Vertex-facet-incidences are correct."
                  "# @return Polytope",
                  &elongated_pentagonal_gyrobirotunda, "elongated_pentagonal_gyrobirotunda()");
    
UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create augmented triangular prism with regular facets. Johnson solid J49."
                  "# The vertices are realized as floating point numbers and thus not exact. Vertex-facet-incidences are correct."
                  "# @return Polytope",
                  &augmented_triangular_prism, "augmented_triangular_prism()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create augmented truncated cube with regular facets. Johnson solid J66."
                  "# @return Polytope",
                  &augmented_truncated_cube, "augmented_truncated_cube()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create biaugmented truncated cube with regular facets. Johnson solid J67."
                  "# @return Polytope",
                  &biaugmented_truncated_cube, "biaugmented_truncated_cube()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create Johnson solid number n."
                  "# @param Int n the index of the desired Johnson polytope"
                  "# @return Polytope",
                  &johnson_int, "johnson(Int)");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create Johnson solid with the given name."
                  "# Some polytopes are realized with floating point numbers and thus not exact;"
                  "# Vertex-facet-incidences are correct in all cases."
                  "# @param String s the name of the desired Johnson polytope"
                  "# @value s 'square_pyramid' Square pyramid with regular facets. Johnson solid J1."
                  "# @value s 'pentagonal_pyramid' Pentagonal pyramid with regular facets. Johnson solid J2."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'triangular_cupola' Triangular cupola with regular facets. Johnson solid J3."
                  "# @value s 'square_cupola' Square cupola with regular facets. Johnson solid J4."
                  "# @value s 'pentagonal_cupola' Pentagonal cupola with regular facets. Johnson solid J5."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'pentagonal_rotunda' Pentagonal rotunda with regular facets. Johnson solid J6."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'elongated_triangular_pyramid' Elongated triangular pyramid with regular facets. Johnson solid J7."
                  "# @value s 'elongated_square_pyramid' Elongated square pyramid with regular facets. Johnson solid J8."
                  "# @value s 'elongated_pentagonal_pyramid' Elongated pentagonal pyramid with regular facets. Johnson solid J9."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyroelingated_square_pyramid' Gyroelongated square pyramid with regular facets. Johnson solid J10."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'gyroelingated_pentagonal_pyramid' Gyroelongated pentagonal pyramid with regular facets. Johnson solid J11."
                  "#          The vertices are realized as floating point numbers."
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
                  //todo 44
                  "# @value s 'elongated_square_bicupola' Elongated square bicupola with regular facets. Johnson solid J45."
                  "#          The vertices are realized as floating point numbers."
                  //todo 46-48
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
                  "# @value s 'metabidiminished_icosahedron' Tridiminished icosahedron with regular facets. Johnson solid J63."
                  "# @value s 'augmented_tridiminished_icosahedron' Augmented tridiminished icosahedron with regular facets. Johnson solid J64."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'augmented_truncated_tetrahedron' Augmented truncated tetrahedron with regular facets. Johnson solid J65."
                  "#          The vertices are realized as floating point numbers."
                  "# @value s 'augmented_truncated_cube' Augmented truncated cube with regular facets. Johnson solid J66."
                  "# @value s 'biaugmented_truncated_cube' Biaugmented truncated cube with regular facets. Johnson solid J67."
                  "# @return Polytope",
                  &johnson_str, "johnson(String)");
} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
