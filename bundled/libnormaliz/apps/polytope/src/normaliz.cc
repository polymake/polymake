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

#include <cstddef> // needed for gcc 4.9, see http://gcc.gnu.org/gcc-4.9/porting_to.html
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

   inline bool try_convert(long& a, const pm::Integer& b) {
      if (!mpz_fits_slong_p(b.get_rep()) || !isfinite(b))
         return false;
      a = b.to_long();
      return true;
   }

   inline bool try_convert(long long& a, const pm::Integer& b) {
      if (!mpz_fits_slong_p(b.get_rep()) || !isfinite(b))
         return false;
      a = b.to_long();
      return true;
   }

   inline bool try_convert(mpz_class& a, const pm::Integer& b) {
      a = mpz_class(b.get_rep());
      return true;
   }

   inline bool try_convert(pm::Integer& a, const mpz_class& b) {
      a = pm::Integer(b);
      return true;
   }

   inline bool try_convert(pm::Integer& a, const long& b) {
      a = pm::Integer(b);
      return true;
   }
   inline bool try_convert(pm::Integer& a, const long long& b) {
      a = pm::Integer(b);
      return true;
   }

   inline double convert_to_double(const pm::Integer& a) {
      return a.to_double();
   }

   pm::Integer operator%(size_t a, const pm::Integer& b) {
      return pm::Integer((unsigned long int) a) % b;
   }

   pm::Integer operator*(size_t a, const pm::Integer& b) {
      return pm::Integer((unsigned long int) a) * b;
   }
}

#include "libnormaliz/libnormaliz-all.cpp"

namespace libnormaliz {
   template<>
      pm::Integer int_max_value_dual<pm::Integer>(){
         assert(false);
         return 0;
      }

   template<>
      pm::Integer int_max_value_primary<pm::Integer>(){
         assert(false);
         return 0;
      }

   template<>
      inline bool using_GMP<pm::Integer>() {
         return true;
      }

   template<>
      inline bool check_range<pm::Integer>(const pm::Integer& m) {
         return true;
      }
}

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

      template <typename Scalar>
      std::vector< std::vector< Scalar > > pmMatrix_to_stdvectorvector(const Matrix<Rational>& m)
      {
         ListMatrix< Vector<Integer> > pmdata = common::primitive(m);
         std::vector< std::vector< Scalar > > data;
         std::transform(rows(pmdata).begin(),rows(pmdata).end(),
               std::back_inserter(data),&pmVector_to_stdvector<Scalar>);
         return data;
      }

      // create libnormaliz cone object based on given type
      //  from rays or inequalities
      template <typename Scalar>
      libnormaliz::Cone<Scalar> libnormaliz_create_cone(perl::Object c, bool from_ineq, bool compute_facets, bool with_grading)
      {
         std::map< libnormaliz::InputType , std::vector< std::vector<Scalar> > > inputmap;
         Matrix<Rational> data;
         if (from_ineq) {
            const Matrix<Rational>& f = c.give("FACETS | INEQUALITIES");
            inputmap[libnormaliz::Type::inequalities]=pmMatrix_to_stdvectorvector<Scalar>(f);
            if (c.lookup("LINEAR_SPAN | EQUATIONS") >> data)
               inputmap[libnormaliz::Type::equations]=pmMatrix_to_stdvectorvector<Scalar>(data);
         } else {
            const Matrix<Rational>& r = c.give("RAYS | INPUT_RAYS");
            inputmap[libnormaliz::Type::integral_closure] = pmMatrix_to_stdvectorvector<Scalar>(r);
            // lookup dual description if we do not want to compute it
            if (!compute_facets) {
               if (c.lookup("FACETS | INEQUALITIES") >> data)
                  inputmap[libnormaliz::Type::inequalities]=pmMatrix_to_stdvectorvector<Scalar>(data);
               if (c.lookup("LINEAR_SPAN | EQUATIONS") >> data)
                  inputmap[libnormaliz::Type::equations]=pmMatrix_to_stdvectorvector<Scalar>(data);
            }
         }
         if (with_grading)
            inputmap[libnormaliz::Type::grading] = std::vector< std::vector<Scalar> >(1, pmVector_to_stdvector<Scalar>(c.lookup("MONOID_GRADING")));
         return libnormaliz::Cone<Scalar>(inputmap);
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
      bool with_grading = c.exists("MONOID_GRADING");
      libnormaliz::verbose=options["verbose"];

      libnormaliz::ConeProperties todo;
      if (options["degree_one_generators"])
         todo.set(libnormaliz::ConeProperty::Deg1Elements);
      if (options["hilbert_basis"])
         todo.set(libnormaliz::ConeProperty::HilbertBasis);
      if (options["hilbert_series"] || options["h_star_vector"])
         todo.set(libnormaliz::ConeProperty::HilbertSeries);
      if (options["dual_algorithm"])
         todo.set(libnormaliz::ConeProperty::DualMode);
      if (options["facets"])
         todo.set(libnormaliz::ConeProperty::SupportHyperplanes);
      if (options["rays"])
         todo.set(libnormaliz::ConeProperty::ExtremeRays);
      perl::ListReturn result;
      if (!options["skip_long"])
      {
         try
         {
            // try with long first
            libnormaliz::Cone<long> nmzCone = libnormaliz_create_cone<long>(c, options["from_facets"] , options["facets"], with_grading );
            nmzCone.compute(todo);
            if (options["degree_one_generators"])
               result << Matrix<Integer>(stdvectorvector_to_pmListMatrix(nmzCone.getDeg1Elements()));
            if (options["hilbert_basis"])
               result << Matrix<Integer>(stdvectorvector_to_pmListMatrix(nmzCone.getHilbertBasis()));
            if (options["h_star_vector"])
               // adjust to correct length, especially for non-full-dimensional polytopes
               result << (convert_to<Integer>(Vector<mpz_class>(nmzCone.getHilbertSeries().getNum())) | zero_vector<Integer>(-1-nmzCone.getHilbertSeries().getDegreeAsRationalFunction()));
            if (options["hilbert_series"])
               result << nmz_convert_HS(nmzCone.getHilbertSeries());
            if (options["facets"]) {
               result << Matrix<Rational>(stdvectorvector_to_pmListMatrix(nmzCone.getSupportHyperplanes()));
               result << Matrix<Rational>(stdvectorvector_to_pmListMatrix(nmzCone.getSublattice().getEquations()));
            }
            if (options["rays"])
               result << Matrix<Rational>(stdvectorvector_to_pmListMatrix(nmzCone.getExtremeRays()));
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

      libnormaliz::Cone<Integer> nmzCone = libnormaliz_create_cone<Integer>(c, options["from_facets"] , options["facets"], with_grading );
      nmzCone.compute(todo);
      if (options["degree_one_generators"])
         result << Matrix<Integer>(stdvectorvector_to_pmListMatrix(nmzCone.getDeg1Elements()));
      if (options["hilbert_basis"])
         result << Matrix<Integer>(stdvectorvector_to_pmListMatrix(nmzCone.getHilbertBasis()));
      if (options["h_star_vector"])
         result << (convert_to<Integer>(Vector<mpz_class>(nmzCone.getHilbertSeries().getNum())) | zero_vector<Integer>(-1-nmzCone.getHilbertSeries().getDegreeAsRationalFunction()));
      if (options["hilbert_series"])
         result << nmz_convert_HS(nmzCone.getHilbertSeries());
      if (options["facets"]) {
         result << Matrix<Rational>(stdvectorvector_to_pmListMatrix(nmzCone.getSupportHyperplanes()));
         result << Matrix<Rational>(stdvectorvector_to_pmListMatrix(nmzCone.getSublattice().getEquations()));
      }
      if (options["rays"])
         result << Matrix<Rational>(stdvectorvector_to_pmListMatrix(nmzCone.getExtremeRays()));
      return result;
   }

UserFunction4perl("# @category Geometry"
		  "# Compute degree one elements, Hilbert basis or Hilbert series of a cone C with libnormaliz"
                  "# Hilbert series and Hilbert h-vector depend on the given grading"
                  "# and will not work unless C is [[HOMOGENEOUS]] or a [[MONOID_GRADING]] is set"
                  "# @param Cone C"
                  "# @option Bool from_facets supply facets instead of rays to normaliz"
                  "# @option Bool degree_one_generators compute the generators of degree one, i.e. lattice points of the polytope"
                  "# @option Bool hilbert_basis compute Hilbert basis of the cone C"
                  "# @option Bool h_star_vector compute Hilbert h-vector of the cone C"
                  "# @option Bool hilbert_series compute Hilbert series of the monoid"
                  "# @option Bool facets compute support hyperplanes (=FACETS,LINEAR_SPAN)"
                  "# @option Bool rays compute extreme rays (=RAYS)"
                  "# @option Bool dual_algorithm use the dual algorithm by Pottier"
                  "# @option Bool skip_long do not try to use long coordinates first"
                  "# @option Bool verbose libnormaliz debug output"
                  "# @return List (Matrix<Integer> degree one generators, Matrix<Integer> Hilbert basis, Vector<Integer> Hilbert h-vector, RationalFunction Hilbert series, Matrix<Rational> facets, Matrix<Rational> linear_span, Matrix<Rational> rays) (only requested items)",
                  &normaliz_compute, "normaliz_compute(Cone { from_facets => 0, degree_one_generators=>0, hilbert_basis=>0, h_star_vector=>0, hilbert_series=>0, facets=>0, rays=>0, dual_algorithm=>0, skip_long=>0, verbose => 0 })");

}
}


