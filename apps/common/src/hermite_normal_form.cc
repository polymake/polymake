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


#include "polymake/common/hermite_normal_form.h"

namespace polymake { namespace common {

template <typename MatrixTop, typename E>
perl::ListReturn hermite_normal_form_perl(const GenericMatrix<MatrixTop, E>& M, perl::OptionSet options){
   std::pair<Matrix<E>, SparseMatrix<E> > cppresult = hermite_normal_form(M, options["reduced"]);
   perl::ListReturn result;
   result << cppresult.first << cppresult.second;
   return result;
}

FunctionTemplate4perl("hermite_normal_form_perl(Matrix; {reduced=>1})");

InsertEmbeddedRule("# @category Linear Algebra"
                   "# Computes the (column) Hermite normal form of an integer matrix."
                   "# Pivot entries are positive, entries to the left of a pivot are non-negative and strictly smaller than the pivot."
                   "# @param Matrix M Matrix to be transformed."
                   "# @option Bool reduced If this is false, entries to the left of a pivot are left untouched. True by default"
                   "# @return List (Matrix N, SparseMatrix R) such that M*R=N, R quadratic unimodular.\n"
                   "# @example The following stores the result for a small matrix M in H and then prints both N and R:"
                   "# > $M = new Matrix<Integer>([1,2],[2,3]);"
                   "# > @H = hermite_normal_form($M);"
                   "# > print $H[0];"
                   "# | 1 0"
                   "# | 0 1"
                   "# > print $H[1];"
                   "# | -3 2"
                   "# | 2 -1\n"
                   "user_function hermite_normal_form(Matrix; {reduced=>1}) {\n"
                   "   hermite_normal_form_perl(@_);\n"
                   "}\n");

}
}
