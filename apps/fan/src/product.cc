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
#include "polymake/Matrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace fan {

    perl::Object product(perl::Object f_in1, perl::Object f_in2, perl::OptionSet options)
    {
      const bool noc=options["no_coordinates"];

      perl::Object f_out("PolyhedralFan<Rational>");
      f_out.set_description() << "Product of " << f_in1.name() << " and " << f_in2.name() << endl;

      const IncidenceMatrix<> MaxCones1=f_in1.give("MAXIMAL_CONES"),
	MaxCones2=f_in2.give("MAXIMAL_CONES");

      IncidenceMatrix<> MaxCones_out(MaxCones1.rows() * MaxCones2.rows(), MaxCones1.cols() + MaxCones2.cols(),
                                     polymake::product(rows(MaxCones1), rows(MaxCones2), operations::concat()).begin());

      f_out.take("N_RAYS") << MaxCones_out.cols();
      f_out.take("MAXIMAL_CONES") << MaxCones_out;


      if (noc && f_in1.exists("COMBINATORIAL_DIM") && f_in2.exists("COMBINATORIAL_DIM")) {
	const int dim1=f_in1.give("COMBINATORIAL_DIM"),
	  dim2=f_in2.give("COMBINATORIAL_DIM");
	f_out.take("COMBINATORIAL_DIM") << dim1+dim2;
      }

      if (!noc) {
	const bool pointed=f_in1.give("POINTED") && f_in2.give("POINTED");
	if (!pointed)
	  throw std::runtime_error("product: input fan not pointed");

	const Matrix<Rational> R1=f_in1.give("RAYS"),
	  R2=f_in2.give("RAYS");

        f_out.take("RAYS") << SparseMatrix<Rational>(diag(R1,R2));
      }

      return f_out;
    }

    UserFunction4perl("# @category Producing a fan"
		      "# Construct a new polyhedral fan as the __product__ of two given polyhedral fans //F1// and //F2//."
		      "# @param PolyhedralFan F1"
		      "# @param PolyhedralFan F2"
		      "# @option Bool no_coordinates only combinatorial information is handled"
		      "# @return PolyhedralFan",
		      &product, "product(PolyhedralFan PolyhedralFan { no_coordinates => 0, relabel => 0 })");
  } }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
