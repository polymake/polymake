/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

	---
	Copyright (c) 2016-2021
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Implements the addition of two morphisms.
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/tropical/refine.h"

namespace polymake { namespace tropical {

/*
 * @brief Takes a morphism and a Cycle whose support is equal to f's [[DOMAIN]]
 * and returns f as a morphism on that cycle.
 * @param Morphism f
 * @param Cycle X
 * @param bool need_to_refine Whether the cycle needs to be refined or it is
 * already fine in f's [[DOMAIN]].
 * @tparam Addition Min or Max
 * @return Morphism The "refined" morphism
 */
template <typename Addition>
BigObject refine_domain(BigObject f, BigObject cycle, bool need_to_refine)
{
  BigObject oldDomain = f.give("DOMAIN");
  bool has_matrix = f.exists("MATRIX");
  RefinementResult r = refinement(cycle, oldDomain,  false, !has_matrix,false,need_to_refine,false);

  BigObject nDomain = r.complex;
  if (has_matrix) {
    Matrix<Rational> matrix = f.give("MATRIX");
    Vector<Rational> translate = f.give("TRANSLATE");
    return BigObject("Morphism", mlist<Addition>(),
                     "DOMAIN", nDomain,
                     "MATRIX", matrix,
                     "TRANSLATE", translate);
  }

  Matrix<Rational> rayRep = r.rayRepFromY;
  Matrix<Rational> linRep = r.linRepFromY;
		
  Matrix<Rational> vertex_values = f.give("VERTEX_VALUES");
  Matrix<Rational> lineality_values = f.give("LINEALITY_VALUES");
  Matrix<Rational> total_values = T( vertex_values / lineality_values);
  Int target_dim = std::max(vertex_values.cols(), lineality_values.cols());

  Matrix<Rational> ndom_vertices = nDomain.give("SEPARATED_VERTICES");
  Matrix<Rational> ndom_lineality = nDomain.give("LINEALITY_SPACE");

  Matrix<Rational> rValues(0,target_dim);
  Matrix<Rational> lValues(0,target_dim);

  for (Int v = 0; v < ndom_vertices.rows(); ++v) {
    rValues /= total_values * rayRep.row(v);
  }

  for (Int l = 0; l < ndom_lineality.rows(); ++l) {
    lValues /= total_values * linRep.row(l);
  }

  return BigObject("Morphism", mlist<Addition>(),
                   "DOMAIN", nDomain,
                   "VERTEX_VALUES", rValues,
                   "LINEALITY_VALUES", lValues);
}

/**
   @brief Computes the sum of two morphisms (which should be defined on the same support)
   @param BigObject f A Morphism object
   @param BigObject g A Morphism object, whose DOMAIN has the same support as f's DOMAIN and whose image has the same ambient dimension as f's image
   @return BigObject A Morphism object representing f+g
*/
template <typename Addition>
BigObject add_morphisms(BigObject f, BigObject g)
{
  // First we treat the special case where both are global
  bool f_global = f.exists("MATRIX");
  bool g_global = g.exists("MATRIX");
  Matrix<Rational> sum_matrix;
  Vector<Rational> sum_translate;
  if (f_global && g_global) {
    Matrix<Rational> fmatrix = f.give("MATRIX");
    Vector<Rational> ftranslate = f.give("TRANSLATE");
    Matrix<Rational> gmatrix = g.give("MATRIX");
    Vector<Rational> gtranslate = g.give("TRANSLATE");

    sum_matrix = fmatrix + gmatrix;
    sum_translate = ftranslate + gtranslate;
  }

  // First we homogenize where necessary
  BigObject fDomain = f.give("DOMAIN");
  BigObject gDomain = g.give("DOMAIN");

  // Then compute the common refinement of the domains
  RefinementResult r = refinement(fDomain,gDomain,false,false,false,true,false);
  BigObject nDomain = r.complex;
	
  // If the map is given by a matrix, we're done
  if (f_global && g_global) {
    return BigObject("Morphism", mlist<Addition>(),
                     "MATRIX", sum_matrix,
                     "TRANSLATE", sum_translate,
                     "DOMAIN", nDomain);
  }

  // Otherwise, compute values on the new domain
  BigObject f_refined = refine_domain<Addition>(f, nDomain, false);
  BigObject g_refined = refine_domain<Addition>(g, nDomain, false);

  Matrix<Rational> fref_vert = f_refined.give("VERTEX_VALUES");
  Matrix<Rational> gref_vert = g_refined.give("VERTEX_VALUES");
  Matrix<Rational> fref_lin  = f_refined.give("LINEALITY_VALUES");
  Matrix<Rational> gref_lin  = g_refined.give("LINEALITY_VALUES");

  return BigObject("Morphism", mlist<Addition>(),
                   "DOMAIN", nDomain,
                   "VERTEX_VALUES", fref_vert + gref_vert,
                   "LINEALITY_VALUES", fref_lin + gref_lin);
}

UserFunctionTemplate4perl("# @category Morphisms"
                          "# Computes the sum of two morphisms. Both [[DOMAIN]]s should have the same support"
                          "# and the target spaces should have the same ambient dimension"
                          "# The domain of the result will be the common refinement of the two domains."
                          "# @param Morphism f"
                          "# @param Morphism g"
                          "# @return Morphism",
                          "add_morphisms<Addition>(Morphism<Addition>, Morphism<Addition>)");
} }
