/* Copyright (c) 1997-2018
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
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/topaz/hasse_diagram.h"
#include "polymake/Rational.h"
#include "polymake/list"

namespace polymake { namespace topaz {

perl::Object unknot(const int m, const int n, perl::OptionSet options)
{
  if (m<2 || n<1)
    throw std::runtime_error("unknot: m>=2 and n>=1 required\n");

  Rational eps;
  if (!(options["eps"] >> eps))  eps=Rational(1,200*(n+m+2));

  perl::Object p("GeometricSimplicialComplex<Rational>");
  p.set_description() << "Vicious embedding of the unknot in the 1-skeleton of the 3-sphere.\n";

  std::list< Set<int> > C;
  const int k1=2*(m+1)-1;
  const int k2=2*(n+m+2)-1;
  const int k3=2*(2*n+m+3)-1;
  const int k4=2*(2*n+2*m+3)-1;
  const int a=k4+1;

  // add "crossing" simplices
  Set<int> f;
  f+=0; f+=1;
  for (int i=3; i<=k1; i+=2) {
    f-=i-5; f-=i-4; f+=i-1; f+=i;
    C.push_back(f);
  }

  f.clear();
  f+=k1+1; f+=k1+2;
  for (int i=k1+4; i<=k2; i+=2) {
    f-=i-5; f-=i-4; f+=i-1; f+=i;
    C.push_back(f);
  }

  f.clear();
  f+=k2+1; f+=k2+2;
  for (int i=k2+4; i<=k3; i+=2) {
    f-=i-5; f-=i-4; f+=i-1; f+=i;
    C.push_back(f);
  }

  f.clear();
  f+=k3+1; f+=k3+2;
  for (int i=k3+4; i<=k4; i+=2) {
    f-=i-5; f-=i-4; f+=i-1; f+=i;
    C.push_back(f);
  }

  // add "cone" simplices of the knot
  f.clear();
  f+=a; f+=0; f+=1;
  for (int i=2; i<=k1; ++i) {
    f-=i-3; f+=i;
    C.push_back(f);
  }

  f.clear();
  f+=a; f+=k1+1; f+=k1+2;
  for (int i=k1+3; i<=k2; ++i) {
    f-=i-3; f+=i;
    C.push_back(f);
  }

  f.clear();
  f+=a; f+=k2+1; f+=k2+2;
  for (int i=k2+3; i<=k3; ++i) {
    f-=i-3; f+=i;
    C.push_back(f);
  }

  f.clear();
  f+=a; f+=k3+1; f+=k3+2;
  for (int i=k3+3; i<=k4; ++i) {
    f-=i-3; f+=i;
    C.push_back(f);
  }


  // add fill in "cone" simplices
  f.clear();
  f+=a; f+=k1+1; f+=1;
  for (int i=3; i<=k1+2; i+=2) {
    f-=i-4; f+=i;
    C.push_back(f);
  }

  f.clear();
  f+=a; f+=k2+1; f+=0;
  for (int i=2; i<=k1-1; i+=2) {
    f-=i-4; f+=i;
    C.push_back(f);
  }
  f-=k1-3; f+=k2+2;
  C.push_back(f);

  f.clear();
  f+=a; f+=k2-1; f+=k4-1;
  for (int i=k4-3; i>=k3+1; i-=2) {
    if (i+4!=a) f-=i+4;
    f+=i;
    C.push_back(f);
  }
  f-=k3+3; f+=k2;
  C.push_back(f);

  f.clear();
  f+=a; f+=k3-1; f+=k4;
  for (int i=k4-2; i>=k3; i-=2) {
    f-=i+4; f+=i;
    C.push_back(f);
  }

  f.clear();
  f+=a; f+=k1-1; f+=k1;
  for (int i=k1+2; i<=k2; i+=2) {
    f-=i-4; f+=i;
    C.push_back(f);
  }
  f-=k2-2; f+=k3+1;
  C.push_back(f);

  f.clear();
  f+=a; f+=k3+1; f+=k3+2;
  for (int i=k3; i>=k2+2; i-=2) {
    f-=i+4; f+=i;
    C.push_back(f);
  }
  f-=k2+4; f+=k1-1;
  C.push_back(f);

  // add cone over boundary
  const Lattice<BasicDecoration> HD = hasse_diagram_from_facets(Array<Set<int> >(C));
  const auto B = boundary_of_pseudo_manifold(HD);
  for (auto b=entire(B); !b.at_end(); ++b)
    C.push_back(b->face + scalar2set(a+1));

  // compute knot
  std::list< Set<int> > K;

  for (int i=3; i<=k1; i+=2) {
    Set<int> e;
    e+=i-3; e+=i;
    K.push_back(e);
    e.clear();
    e+=i-2; e+=i-1;
    K.push_back(e);
  }

  for (int i=k1+4; i<=k2; i+=2) {
    Set<int> e;
    e+=i-3; e+=i;
    K.push_back(e);
    e.clear();
    e+=i-2; e+=i-1;
    K.push_back(e);
  }

  for (int i=k2+4; i<=k3; i+=2) {
    Set<int> e;
    e+=i-3; e+=i;
    K.push_back(e);
    e.clear();
    e+=i-2; e+=i-1;
    K.push_back(e);
  }

  for (int i=k3+4; i<=k4; i+=2) {
    Set<int> e;
    e+=i-3; e+=i;
    K.push_back(e);
    e.clear();
    e+=i-2; e+=i-1;
    K.push_back(e);
  }

  Set<int> e;
  e+=k1; e+=k1+2;
  K.push_back(e);

  e.clear();
  e+=k1-1; e+=k2+2;
  K.push_back(e);

  e.clear();
  e+=k2; e+=k3+1;
  K.push_back(e);

  e.clear();
  e+=k3; e+=k3+2;
  K.push_back(e);

  e.clear();
  e+=1; e+=k1+1;
  K.push_back(e);

  e.clear();
  e+=0; e+=k2+1;
  K.push_back(e);

  e.clear();
  e+=k2-1; e+=k4-1;
  K.push_back(e);

  e.clear();
  e+=k3-1; e+=k4;
  K.push_back(e);

  // geometric realization
  Matrix<Rational> Coordinates(a+2,3);
  for (int i=0; i<=k1; ++i){
    Coordinates(i,0) = Rational(2-(i%2));
    Coordinates(i,1) = Rational(i/2);
    Coordinates(i,2) = i%2==0 ? -Rational(i/2)*eps  : Rational(i/2)*eps;
  }

  for (int i=k1+1; i<=k2; ++i){
    Coordinates(i,0)=Rational(i%2);
    Coordinates(i,1)=Rational(i/2);
    Coordinates(i,2) = i%2==0 ? -Rational((i-k1-1)/2)*eps : Rational((i-k1-1)/2)*eps;
  }

  for (int i=k2+1; i<=k3; ++i){
    Coordinates(i,0)=Rational(3-(i%2));
    Coordinates(i,1)=Rational((i-k2+k1)/2);
    Coordinates(i,2) = i%2==0 ? -Rational((i-k2-1)/2)*eps : Rational((i-k2-1)/2)*eps;
  }

  for (int i=k3+1; i<=k4; ++i){
    Coordinates(i,0)=Rational(1+(i%2));
    Coordinates(i,1)=Rational((i-k3+k2)/2);
    Coordinates(i,2) = i%2==0 ? -Rational((i-k3-1)/2)*eps : Rational((i-k3-1)/2)*eps;
  }

  Coordinates(a,0)=Rational(3,2);
  Coordinates(a,1)=Rational((k1+k2)/4);
  Coordinates(a,2)=Rational(-2);
  Coordinates(a+1,0)=Rational(3,2);
  Coordinates(a+1,1)=Rational((k1+k2)/4);
  Coordinates(a+1,2)=Rational(2);

  p.take("FACETS") << C;
  p.take("COORDINATES") << Coordinates;
  p.take("KNOT") << K;
  return p;
}

UserFunction4perl("# @category Producing from scratch\n"
                  "# Produces a triangulated 3-sphere with the particularly NASTY embedding\n"
                  "# of the unknot in its 1-skeleton. The parameters //m// >= 2 and //n// >= 1\n"
                  "# determine how entangled the unknot is. //eps// determines the [[COORDINATES]].\n"
                  "# @param Int m"
                  "# @param Int n"
                  "# @option Rational eps"
                  "# @return GeometricSimplicialComplex",
                  &unknot, "unknot($$ { eps => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
