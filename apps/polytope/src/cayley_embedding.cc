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
#include "polymake/polytope/cayley_embedding.h"

namespace polymake { namespace polytope {

template<typename Scalar>
perl::Object cayley_embedding(const perl::Object& p_in1, const perl::Object& p_in2,
                              const Scalar& t, const Scalar& t_prime, 
                              perl::OptionSet options)
{
   const Array<perl::Object> p_array{ p_in1, p_in2 };
   const Vector<Scalar> t_vec{ t, t_prime };

   return cayley_embedding(p_array, t_vec, options);
}

template<typename Scalar>
perl::Object cayley_embedding(const Array<perl::Object>& p_array,
                              perl::OptionSet options)
{
   Vector<Scalar> t_vec;
   options["factors"] >> t_vec;

   return cayley_embedding(p_array, t_vec, options);
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Create a Cayley embedding of two polytopes (one of them must be pointed)."
                          "# The vertices of the first polytope //P_0// get embedded to //(t_0,0)//"
                          "# and the vertices of the second polytope //P_1// to //(0,t_1)//."
                          "# "
                          "# Default values are //t_0//=//t_1//=1."
                          "# @param Polytope P_0 the first polytope"
                          "# @param Polytope P_1 the second polytope"
                          "# @param Scalar t_0 the extra coordinate for the vertices of //P_0//"
                          "# @param Scalar t_1 the extra coordinate for the vertices of //P_1//"
                  "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytope. default: 0"
                          "# @return Polytope",
                          "cayley_embedding<Scalar>(Polytope<type_upgrade<Scalar>>, Polytope<type_upgrade<Scalar>>; type_upgrade<Scalar>=1, type_upgrade<Scalar>=($_[-1]),"
                          "                         { no_labels => 0 })");


UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Create a Cayley embedding of an array (P1,...,Pm) of polytopes. "
                          "# All polytopes must have the same dimension, at least one of them must be pointed, "
                          "# and all must be defined over the same number type. "
                          "# Each vertex //v// of the //i//-th polytope is embedded to //v//|//t_i e_i//, "
                          "# where //t_i// is the //i//-th entry of the optional array //t//. "
                          "# @param Array<Polytope> A the input polytopes"
                          "# @option Array<Scalar> factors array of scaling factors for the Cayley embedding; defaults to the all-1 vector"
                  "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytope. default: 0"
                          "# @return Polytope",
                          "cayley_embedding<Scalar>(Polytope<type_upgrade<Scalar>>+; { factors => [], no_labels => 0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
