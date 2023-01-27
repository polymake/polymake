/* Copyright (c) 1997-2023
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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
#include "polymake/tropical/double_description.h"
#include "polymake/polytope/hasse_diagram.h"


namespace polymake { namespace tropical {

template<typename Scalar>
BigObject monomial_cone_lattice(const Matrix<Scalar>& m)
{
  BigObject H("Lattice<BasicDecoration, Nonsequential>");
  H = polytope::hasse_diagram(tropical::monomial_dual_description(m).second, m.cols());
  return H;
}

template <typename Addition, typename Scalar>
Matrix<TropicalNumber<Addition, Scalar>> V_trop_input(BigObject p)
{
  const std::pair<Matrix<TropicalNumber<Addition,Scalar>>,Matrix<TropicalNumber<Addition,Scalar>>> Ineq = p.lookup("INEQUALITIES");
  Matrix<TropicalNumber<Addition, Scalar> > extremals(extremals_from_halfspaces(Ineq.first,Ineq.second));
  if (extremals.rows() == 0)
    throw std::runtime_error("the inequalities form an infeasible system");
  return extremals;
}

FunctionTemplate4perl("V_trop_input<Addition,Scalar>(Polytope<Addition,Scalar>)");
    
UserFunctionTemplate4perl("# @category Tropical operations"
                          "# computes the VIF of a monomial tropical cone "
                          "# given by generators "
                          "# @param Matrix M the exponent vectors of the generators. "
                          "# @return Lattice<BasicDecoration, Nonsequential>",
                          "monomial_cone_lattice(Matrix)");
    
FunctionTemplate4perl("monoextremals(Matrix, Matrix, Vector)");

FunctionTemplate4perl("extremals_from_generators(Matrix)");

FunctionTemplate4perl("extremals_from_halfspaces(Matrix,Matrix)");

FunctionTemplate4perl("matrixPair2splitApices(Matrix,Matrix)");

UserFunctionTemplate4perl("# @category Tropical operations"
                          "# This computes the __extremal generators__ of a tropical cone "
                          "# given by generators //G// intersected with one inequality //a//x ~ //b//x."
                          "# Here, ~ is <= for min and >= for max."
                          "# @param Matrix<TropicalNumber> G"
                          "# @param Vector<TropicalNumber> a"
                          "# @param Vector<TropicalNumber> b"
                          "# @return Matrix<TropicalNumber> extrls"
                          "# @example"
                          "# > $G = new Matrix<TropicalNumber<Min>>([[0,0,2],[0,4,0],[0,3,1]]);"
                          "# > $a = new Vector<TropicalNumber<Min>>(['inf','inf',-2]);"
                          "# > $b = new Vector<TropicalNumber<Min>>([0,-1,'inf']);"
                          "# > print intersection_extremals($G,$a,$b);"
                          "# | 0 0 1"
                          "# | 0 4 0"
                          "# | 0 3 1",
                          "intersection_extremals(Matrix, Vector, Vector)");

UserFunctionTemplate4perl("# @category Tropical operations"
                          "# compute the dual description of "
                          "# a monomial tropical cone. "
                          "# @param Matrix monomial_generators"
                          "# @return Pair<Matrix, IncidenceMatrix<>>",
                          "monomial_dual_description(Matrix)");

UserFunctionTemplate4perl("# @category Tropical operations"
                          "# Reformulate the description of an "
                          "# inequality system given by two matrices"
                          "# to the description by apices and infeasible sectors " 
                          "# @param Matrix<TropicalNumber> G"
                          "# @param Matrix<TropicalNumber> A"
                          "# @return Pair<Matrix<TropicalNumber>, Array<Set<Int>>> signed_apices",
                          "matrixPair2apexSet(Matrix, Matrix)");

UserFunctionTemplate4perl("# @category Tropical operations"
                          "# Check if a point is contained in "
                          "# all tropical halfspaces given by "
                          "# their apices and the infeasible sectors " 
                          "# @param Matrix<TropicalNumber> apices"
                          "# @param Array<Set<Int>> sectors"
                          "# @return Bool",
                          "is_contained(Vector, Matrix, Array)");
} }
