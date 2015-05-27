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
#include "polymake/Rational.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Matrix.h"
#include "polymake/group/group_domain.h"
#include "polymake/Array.h"
#include "polymake/SparseMatrix.h"

namespace polymake { namespace polytope {

namespace {

perl::Object simplex_group(int d) {
   perl::Object g("group::GroupOfPolytope");
   g.set_description() << "full combinatorial group on vertices of " << d << "-dim simplex" << endl;
   g.set_name("fullCombinatorialGroupOnRays");
   g.take("DOMAIN") << polymake::group::OnRays;
   int n_gens=2;
   if (d==1){
      n_gens=1;
   }
   Array< Array< int > > gens(n_gens);
   if ( d==1 ) {
      Array< int > gen(2);
      gen[0]=1;
      gen[1]=0;
      gens[0]=gen;
   } else {
      Array< int > gen=sequence(0,d+1);
      gen[0]=1;
      gen[1]=0;

      gens[0]=gen;

      
      gen[0]=d;
      for ( int j=1; j<=d; ++j ) {
         gen[j]=j-1; 
      }
      gens[1]=gen;
   }

   g.take("GENERATORS") << gens;
   return g;
}


void add_simplex_data(perl::Object& p, const int d, const bool group_flag) {
   p.take("CONE_DIM") << d+1;   
   p.take("N_VERTICES") << d+1;
   p.take("SIMPLICIALITY") << d;
   p.take("BOUNDED") << true;
   p.take("FEASIBLE") << true;
   p.take("POINTED") << true;
   if ( group_flag ) {
      p.take("GROUP") << simplex_group(d);
   }
}

} // end anonymous namespace

template<typename Scalar>
perl::Object simplex(int d, const Scalar& s, perl::OptionSet options)
{
   if (d < 0)
      throw std::runtime_error("dimension must be non-negative");
   if (s==0)
      throw std::runtime_error("scale must be non-zero");

   perl::Object p(perl::ObjectType::construct<Scalar>("Polytope"));
   p.set_description() << "standard simplex of dimension " << d << endl;

   SparseMatrix<Scalar> V( ones_vector<Scalar>(d+1) | (zero_vector<Scalar>(d) / (s*unit_matrix<Scalar>(d))));

   p.take("VERTICES") << V;
   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("LINEALITY_SPACE") << Matrix<Scalar>();
   if (d == 0)
     p.take("CENTERED") << true;
   else    
     p.take("CENTERED") << false;
   add_simplex_data(p,d,options["group"]);

   return p;
}

typedef QuadraticExtension<Rational> QE;

perl::Object regular_simplex(const int d, perl::OptionSet options)
{
   if (d < 0)
      throw std::runtime_error("dimension must be non-negative");
   if (d==0)
      return simplex< QE >(0,QE(1,0,0),options);

   perl::Object p("Polytope<QuadraticExtension>");
   p.set_description() << "regular simplex of dimension " << d << endl;

   QE c(Rational(1,d),Rational(-1,d),d+1);
   SparseMatrix<QE> V( ones_vector<QE>(d+1) | (unit_matrix<QE>(d) / same_element_vector<QE>(c,d)));

   p.take("VERTICES") << V;
   p.take("LINEALITY_SPACE") << Matrix<QE>();
   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("CENTERED") << true;
   add_simplex_data(p,d,options["group"]);

   return p;
   
}

perl::Object fano_simplex(int d, perl::OptionSet options)
{
   if (d <= 0)
      throw std::runtime_error("fano_simplex : dimension must be postive");

   perl::Object p(perl::ObjectType::construct<Rational>("Polytope"));
   p.set_description() << "Fano simplex of dimension " << d << endl;

   SparseMatrix<Rational> V( ones_vector<Rational>(d+1) | (unit_matrix<Rational>(d) / same_element_vector<Rational>(-1,d)) );

   p.take("VERTICES") << V;
   p.take("LINEALITY_SPACE") << Matrix<Rational>();
   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("CENTERED") << true;
   p.take("REFLEXIVE") << true;
   add_simplex_data(p,d,options["group"]);

   return p;
}

perl::Object lecture_hall_simplex(int d, perl::OptionSet options)
{
   if (d <= 0)
      throw std::runtime_error("lecture_hall_simplex : dimension must be postive");

   perl::Object p(perl::ObjectType::construct<Rational>("Polytope"));
   p.set_description() << "lecture hall simplex of dimension " << d << endl;

   Matrix<Rational> V(d+1,d+1);
   for (int i=0; i<=d; ++i) {
      V(i,0)=1;
      for (int k=d; k>d-i; --k) V(i,k)=k;
   }

   p.take("VERTICES") << V;
   p.take("LINEALITY_SPACE") << Matrix<Rational>();
   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("CENTERED") << false;
   add_simplex_data(p,d,options["group"]);

   return p;
}


UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a regular //d//-simplex embedded in R^d with edge length sqrt(2)."
                  "# @param Int d the dimension"
                  "# @option Bool group"
                  "# @return Polytope",
                  &regular_simplex, "regular_simplex(Int; { group => undef } )");

UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                          "# Produce the standard //d//-simplex."
                          "# Combinatorially equivalent to a regular polytope corresponding to the Coxeter group of type A<sub>//d//-1</sub>."
                          "# Optionally, the simplex can be scaled by the parameter //scale//."
                          "# @param Int d the dimension"
                          "# @param Scalar scale default value: 1"
                          "# @option Bool group"
                          "# @return Polytope",
                          "simplex<Scalar> [ is_ordered_field(type_upgrade<Scalar, Rational>) ] (Int; type_upgrade<Scalar>=1, { group => undef } )");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a Fano //d//-simplex."
                  "# @param Int d the dimension"
                  "# @option Bool group"
                  "# @return Polytope",
                  &fano_simplex, "fano_simplex(Int; { group => undef } )");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a lecture hall //d//-simplex."
                  "# @param Int d the dimension"
                  "# @option Bool group"
                  "# @return Polytope",
                  &lecture_hall_simplex, "lecture_hall_simplex(Int; { group => undef } )");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
