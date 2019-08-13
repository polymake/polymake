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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/tropical/thomog.h"

namespace polymake { namespace tropical {

    UserFunctionTemplate4perl("# @Category Affine and projective coordinates"
			      "# Converts tropical affine to tropical projective coordinates."
			      "# It takes a matrix of row vectors in R<sup>n-1</sup> and"
			      "# identifies the latter with R<sup>n</sup> mod (1,..,1) by"
			      "# assuming a certain coordinate has been set to 0."
			      "# I.e. it will return the matrix with a 0 column inserted at"
			      "# the position indicated by chart"
			      "# @param Matrix<Rational> A The matrix. Can also be given as an anonymous perl array reference."
			      "# @param Int chart Optional. Indicates, which coordinate of"
			      "# R<sup>n</sup> mod (1,..,1) should be set to 0 to identify it"
			      "# with R<sup>n-1</sup>. Note that if there is a leading coordinate, "
			      "# the first column is supposed to contain"
			      "# the 1/0-coordinate indicating whether a row is a vertex or a ray and"
			      "# the remaining coordinates are then labelled 0,..,n-1. This option is 0 by default."
			      "# @param Bool has_leading_coordinate Optional. Whether the matrix has a leading 1/0 to indicate"
			      "# whether a row is a vertex or a ray. In that case, this coordinate is not touched."
			      "# This is true by default."
			      "# @return Matrix<Rational>"
                              "# @example Homogenize vectors with leading coordinate by inserting a 0-entry at index 0."
                              "# > print thomog([[1,3,4],[0,5,6]]);"
                              "# | 1 0 3 4"
                              "# | 0 0 5 6"
                              "# @example Homogenize a vector without leading coordinate by inserting a 0-entry at index 2."
                              "# > print thomog([[2,3,4]], 2, 0);"
                              "# | 2 3 0 4",
			      "thomog(Matrix;$=0, $=1)");

   //We need the following rule to allow anonymous arrays as input.

   InsertEmbeddedRule( "function thomog($;$=0,$=1) {\n"
                       "   $_[0] = new Matrix($_[0]);\n"
                       "   return thomog(@_);\n"
                       "}\n");

    UserFunctionTemplate4perl("# @Category Affine and projective coordinates"
			      "# This is the inverse operation of thomog. It assumes a list of"
			      "# rays and vertices is given in tropical projective coordinates and returns"
			      "# a conversion into affine coordinates."
			      "# @param Matrix<Rational> A The matrix. Can also be given as an anonymous array."
			      "# @param Int chart Optional. Indicates which coordinate should be shifted"
			      "# to 0. If there is a leading coordinate, the first column of the matrix "
			      "# will remain untouched and the subsequent"
			      "# ones are numbered from 0. The default value for this is 0."
			      "# @param Bool has_leading_coordinate Whether the matrix has a leading 1/0 to indicate"
			      "# whether a row is a vertex or a ray. In that case, this coordinate is not touched."
			      "# This is true by default."
			      "# @return Matrix<Rational>"
                              "# @example Dehomogenize vector with leading coordinate by shifting entry at index 0 to 0 and forgetting it."
                              "# > print tdehomog([[1,3,5,8]]);"
                              "# | 1 2 5"
                              "# @example Dehomogenize vector without leading coordinate by shifting entry at index 2 to 0 and forgetting it."
                              "# > print tdehomog([[2,3,4,5]], 2, 0);"
                              "# | -2 -1 1",
                              "tdehomog(Matrix;$=0, $=1)");

   //We need the following rule to allow anonymous arrays as input.

   InsertEmbeddedRule( "function tdehomog($;$=0,$=1) {\n"
                       "   $_[0] = new Matrix($_[0]);\n"
                       "   return tdehomog(@_);\n"
                       "}\n");
    
    FunctionTemplate4perl("tdehomog_vec(Vector;$=0,$=1)");

    FunctionTemplate4perl("normalized_first(Matrix)");
}}
