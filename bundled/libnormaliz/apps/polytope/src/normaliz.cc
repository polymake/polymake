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

#include <gmpxx.h>

#include "polymake/client.h"
#include "polymake/vector"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/common/lattice_tools.h"
#include "polymake/Polynomial.h"
#include "polymake/RationalFunction.h"


namespace libnormaliz {
   template<typename Integer> inline long explicit_cast_to_long(const Integer& a);

   template<> inline long explicit_cast_to_long(const pm::Integer& a) {
      return a.to_long();
   }

   mpz_class to_mpz(const pm::Integer& a) {
      return mpz_class(a.get_rep());
   }

}

#include "libnormaliz-all.cpp"

namespace polymake { namespace polytope {

   namespace {
      template <typename Scalar>
      std::vector<Scalar> pmVector_to_stdvector(const Vector<Integer>& v) 
      {
         return std::vector<Scalar>(attach_converter<Scalar>(v).begin(),
               attach_converter<Scalar>(v).end());
      }

      template <typename Scalar>
      ListMatrix<Vector<Integer> > stdvectorvector_to_pmListMatrix(const std::vector<std::vector<Scalar> > vec) 
      {
         ListMatrix<Vector<Integer> > matrix;
         for(typename std::vector<std::vector<Scalar> >::const_iterator row = vec.begin();row!=vec.end();++row)
         {
            matrix /= Vector<Integer>(*row);
         }
         return matrix;
      }

      // create libnormaliz cone object based on given type 
      //  from rays or inequalities
      template <typename Scalar>
      libnormaliz::Cone<Scalar> libnormaliz_create_cone(const Matrix<Rational>& input, bool from_ineq)
      {
         ListMatrix< Vector<Integer> > pmdata = common::primitive(input);
         std::vector< std::vector< Scalar > > data;
         std::transform(rows(pmdata).begin(),rows(pmdata).end(),
               std::back_inserter(data),&pmVector_to_stdvector<Scalar>);
         return libnormaliz::Cone<Scalar>( data, 
               from_ineq ? libnormaliz::Type::hyperplanes
               : libnormaliz::Type::integral_closure );
      }

      // hilbert series conversion: 
      // nmzHilb.getNum() vector<mpz_class>
      // nmzHilb.getDenom() map<long,denom_t> (exponents of (1-t^i)^e) denom_t = long
      RationalFunction<> nmz_convert_HS(const libnormaliz::HilbertSeries& nmzHilb)
      {
         Ring<> r(1);
         UniPolynomial<> HSnum(convert_to<Integer>(Vector<mpz_class>(nmzHilb.getNum())),
               Vector<int>(sequence(0,nmzHilb.getNum().size())),r);
         const std::map<long,long>& HSdenomMap(nmzHilb.getDenom());
         UniPolynomial<> HSdenom(1,r);
         for(std::map<long,long>::const_iterator mapit = HSdenomMap.begin(); 
               mapit != HSdenomMap.end(); ++mapit)
         {
            for(long i=0; i<mapit->second; ++i)
               HSdenom *= (UniTerm<>(1,r) - UniTerm<>(UniMonomial<>(mapit->first,r),1));
         }
         return RationalFunction<>(HSnum,HSdenom);
      }
   }

   perl::ListReturn normaliz_compute(perl::Object c, perl::OptionSet options) {
      bool withgrading = c.exists("MONOID_GRADING");
      libnormaliz::verbose=options["verbose"];
      libnormaliz::test_arithmetic_overflow=true;
      libnormaliz::ConeProperties todo;
      if (options["degree_one_generators"])
         todo.set(libnormaliz::ConeProperty::Deg1Elements);
      if (options["hilbert_basis"])
         todo.set(libnormaliz::ConeProperty::HilbertBasis);
      if (options["hilbert_series"] || options["h_star_vector"])
         todo.set(libnormaliz::ConeProperty::HilbertSeries);
      if (options["dual_algorithm"])
         todo.set(libnormaliz::ConeProperty::DualMode);
      perl::ListReturn result;
      if (!options["skip_long"])
      {
         try 
         {
            // try with long first
            libnormaliz::Cone<long> nmzCone = libnormaliz_create_cone<long>(
                   options["from_facets"] ? c.give("FACETS") : c.give("RAYS"),
                   options["from_facets"] );
            if (withgrading)
               nmzCone.setGrading(pmVector_to_stdvector<long>(c.lookup("MONOID_GRADING")));
            nmzCone.compute(todo);
            if (options["degree_one_generators"])
               result << Matrix<Integer>(stdvectorvector_to_pmListMatrix(nmzCone.getDeg1Elements()));
            if (options["hilbert_basis"])
               result << Matrix<Integer>(stdvectorvector_to_pmListMatrix(nmzCone.getHilbertBasis()));
            if (options["h_star_vector"])
               result << convert_to<Integer>(Vector<mpz_class>(nmzCone.getHilbertSeries().getNum()));
            if (options["hilbert_series"])
               result << nmz_convert_HS(nmzCone.getHilbertSeries());
            return result;
         } 
         catch(const pm::GMP::error& ex)
         {
            if (libnormaliz::verbose)
               cerr << "libnormaliz: error converting coordinates to long, retrying with pm::Integer" << endl;
         }
         catch(const libnormaliz::ArithmeticException& ex) 
         {
            if (libnormaliz::verbose)
               cerr << "libnormaliz: arithmetic error detected, retrying with pm::Integer" << endl;
         }
      }
      libnormaliz::Cone<Integer> nmzCone = libnormaliz_create_cone<Integer>(
             options["from_facets"] ? c.give("FACETS") : c.give("RAYS"),
             options["from_facets"] );
      if (withgrading)
         nmzCone.setGrading(pmVector_to_stdvector<Integer>(c.lookup("MONOID_GRADING")));
      nmzCone.compute(todo);
      if (options["degree_one_generators"])
         result << Matrix<Integer>(stdvectorvector_to_pmListMatrix(nmzCone.getDeg1Elements()));
      if (options["hilbert_basis"])
         result << Matrix<Integer>(stdvectorvector_to_pmListMatrix(nmzCone.getHilbertBasis()));
      if (options["h_star_vector"])
         result << convert_to<Integer>(Vector<mpz_class>(nmzCone.getHilbertSeries().getNum()));
      if (options["hilbert_series"])
         result << nmz_convert_HS(nmzCone.getHilbertSeries());
      return result;
   }

UserFunction4perl("# @category Geometric properties"
		  "# Compute degree one elements, Hilbert basis or Hilbert series of a cone C with libnormaliz"
                  "# Hilbert series and Hilbert h-vector depend on the given grading"
                  "# and will not work unless C is [[HOMOGENEOUS]] or a [[MONOID_GRADING]] is set"
                  "# @param Cone C"
                  "# @option Bool from_facets supply facets instead of rays to normaliz"
                  "# @option Bool degree_one_generators compute the generators of degree one, i.e. lattice points of the polytope"
                  "# @option Bool hilbert_basis compute Hilbert basis of the cone C"
                  "# @option Bool h_star_vector compute Hilbert h-vector of the cone C"
                  "# @option Bool hilbert_series compute Hilbert series of the monoid"
                  "# @option Bool dual_algorithm use the dual algorithm by Pottier"
                  "# @option Bool skip_long do not try to use long coordinates first"
                  "# @option Bool verbose libnormaliz debug output"
                  "# @return perl::ListReturn (degree one generators, Hilbert basis, Hilbert h-vector, Hilbert series) (if they are requested)",
                  &normaliz_compute, "normaliz_compute(Cone { from_facets => 0, degree_one_generators=>0, hilbert_basis=>0, h_star_vector=>0, hilbert_series=>0, dual_algorithm=>0, skip_long=>0, verbose => 0 })");

}
}


