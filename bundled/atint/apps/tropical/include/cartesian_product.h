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
	Copyright (c) 2016-2020
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Contains a function to compute the cartesian product of cycles.
	*/

#ifndef POLYMAKE_ATINT_CARTESIAN_PRODUCT_H
#define POLYMAKE_ATINT_CARTESIAN_PRODUCT_H

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/specialcycles.h"

namespace polymake { namespace tropical {

/**
   @brief Takes a list of Cycle objects (that may be weighted but need not be) and computes the cartesian product of these.
   If any complex has weights, all non-weighted complexes will be treated as having constant weight 1.
   The [[LOCAL_RESTRICTION]] of the result will be the cartesian product of the [[LOCAL_RESTRICTION]]s of each complex.
   (If a complex does not have any restrictions, the new local restriction is the (pairwise) product of all
   local restriction cones with ALL cones (including faces) of the next complex)
   @param Array<BigObject> complexes A list of Cycle objects
   @return Cycle The cartesian product of the complexes. Note that the representation is noncanonical, as it identifies
   the product of two projective tori of dimensions d and e with a projective torus of dimension d+e
   by dehomogenizing and then later rehomogenizing after the first coordinate.
*/
template <typename Addition>
BigObject cartesian_product(const Array<BigObject>& complexes)
{
  //** EXTRACT FIRST COMPLEX ********************************************

  BigObject firstComplex = complexes[0];
  // This will contain the sets describing the maximal cones
  IncidenceMatrix<> maximalCones = firstComplex.give("MAXIMAL_POLYTOPES");
  // Will contain the rays
  Matrix<Rational> rayMatrix = firstComplex.give("VERTICES");
  rayMatrix = tdehomog(rayMatrix);
  // Will contain the lineality space
  Matrix<Rational> linMatrix = firstComplex.give("LINEALITY_SPACE");
  linMatrix = tdehomog(linMatrix);
  // Will contain the weights (if any)
  Vector<Integer> weights;
  bool product_has_weights = false;
  if (firstComplex.lookup("WEIGHTS") >> weights) {
    product_has_weights = true;
  }
  IncidenceMatrix<> local_restriction;
  if (firstComplex.exists("LOCAL_RESTRICTION")) {
    firstComplex.give("LOCAL_RESTRICTION") >> local_restriction;
  }
  bool product_has_lattice = false;
  Matrix<Integer> product_l_generators;
  IncidenceMatrix<> product_l_bases;
  if (firstComplex.lookup("LATTICE_BASES") >> product_l_bases) {
    Matrix<Integer> lg = firstComplex.give("LATTICE_GENERATORS");
    product_l_generators = tdehomog(lg);
    product_has_lattice = true;
  }

  Int product_dim = rayMatrix.rows() > 0 ? rayMatrix.cols() : linMatrix.cols();
  // Sort rays by affine and directional
  std::pair<Set<Int>, Set<Int>> product_vertex_pair = far_and_nonfar_vertices(rayMatrix);
  Set<Int> product_affine = product_vertex_pair.second;
  Set<Int> product_directional = product_vertex_pair.first;

  //** ITERATE OTHER COMPLEXES ********************************************

  for (Int ci = 1; ci < complexes.size(); ++ci) {
    // Extract properties
    if (call_function("is_empty", complexes[ci])) {
      Int projective_amb = std::max(rayMatrix.cols(), linMatrix.cols()) - 1;
      for (Int cj = ci; cj < complexes.size(); ++cj) {
        const Int jth_projective_amb = complexes[cj].give("PROJECTIVE_AMBIENT_DIM");
        projective_amb += jth_projective_amb;
      }
      return empty_cycle<Addition>(projective_amb);
    }

    bool uses_weights = false;
    Matrix<Rational> prerays = complexes[ci].give("VERTICES");
    prerays = tdehomog(prerays);
    Matrix<Rational> prelin = complexes[ci].give("LINEALITY_SPACE");
    prelin = tdehomog(prelin);
    IncidenceMatrix<> premax = complexes[ci].give("MAXIMAL_POLYTOPES");
    IncidenceMatrix<> pre_local_restriction;
    complexes[ci].lookup("LOCAL_RESTRICTION") >> pre_local_restriction;

    Vector<Integer> preweights;
    if (complexes[ci].lookup("WEIGHTS") >> preweights) {
      uses_weights = true;
    }

    // ** RECOMPUTE RAY DATA ***********************************************

    // Sort rays
    std::pair<Set<Int>, Set<Int>> complex_vertex_pair = far_and_nonfar_vertices(prerays);
    Set<Int> complex_affine = complex_vertex_pair.second;
    Set<Int> complex_directional = complex_vertex_pair.first;
    // If this fan uses homog. coordinates, strip away the first column of rays and linear space
    if (prerays.rows() > 0) prerays = prerays.minor(All, range_from(1));
    if (prelin.rows() > 0) prelin = prelin.minor(All, range_from(1));
    const Int dim = prerays.rows() > 0 ? prerays.cols() : prelin.cols();

    // Create new ray matrix
    ListMatrix<Vector<Rational>> newRays(0, product_dim + dim);
    // First create affine rays
    Map<Int, Map<Int, Int>> affineIndices;
    for (auto prays = entire(product_affine); !prays.at_end(); ++prays) {
      affineIndices[*prays] = Map<Int, Int>();
      Vector<Rational> pRay;
      if (*prays >= 0)
        pRay = rayMatrix.row(*prays);
      else
        pRay = zero_vector<Rational>(product_dim);
      for (auto crays = entire(complex_affine); !crays.at_end(); ++crays) {
        Vector<Rational> cRay;
        if (*crays >= 0)
          cRay = prerays.row(*crays);
        else
          cRay = zero_vector<Rational>(dim);
        newRays /= pRay | cRay;
        affineIndices[*prays][*crays] = newRays.rows()-1;
      }
    }
    const auto newAffine = sequence(0, newRays.rows());

    // Now add the directional rays of both cones
    Map<Int, Int> pdirIndices;
    Map<Int, Int> cdirIndices; //For index conversion
    Vector<Rational> product_zero = zero_vector<Rational>(product_dim);
    Vector<Rational> complex_zero = zero_vector<Rational>(dim);
    for (auto prays = entire(product_directional); !prays.at_end(); ++prays)  {
      newRays /= rayMatrix.row(*prays) | complex_zero;
      pdirIndices[*prays] = newRays.rows()-1;
    }
    for (auto crays = entire(complex_directional); !crays.at_end(); ++crays) {
      newRays /= product_zero | prerays.row(*crays);
      cdirIndices[*crays] = newRays.rows()-1;
    }
    Set<Int> newDirectional = sequence(newAffine.size(), product_directional.size() + complex_directional.size());

    // Create new lineality matrix
    if (prelin.rows() > 0) {
      prelin = Matrix<Rational>(prelin.rows(),product_dim) | prelin;
    }
    if (linMatrix.rows() > 0) {
      linMatrix = linMatrix | Matrix<Rational>(linMatrix.rows(), dim);
    }

    // ** RECOMPUTE LATTICE DATA ***************************************

    // Compute lattice data
    const bool complex_has_lattice = complexes[ci].exists("LATTICE_BASES");
    product_has_lattice = product_has_lattice && complex_has_lattice;
    Int lattice_index_translation = 0; //Number of row where new lattice gens. begin

    IncidenceMatrix<> new_lattice_bases;

    Matrix<Integer> complex_lg;
    IncidenceMatrix<> complex_lb;
    if (product_has_lattice) {
      Matrix<Integer> clg = complexes[ci].give("LATTICE_GENERATORS");
      clg = tdehomog(clg);
      complex_lg = clg;
      IncidenceMatrix<> clb = complexes[ci].give("LATTICE_BASES");
      complex_lb = clb;
      // Compute cartesian product of lattice matrices:
      // Adjust dimension, then concatenate
      if (complex_lg.rows() > 0)
        complex_lg = complex_lg.minor(All, range_from(1));
      cols(product_l_generators).resize(product_l_generators.cols() + dim);
      complex_lg = Matrix<Integer>(complex_lg.rows(), product_dim) | complex_lg;
      lattice_index_translation = product_l_generators.rows();
      product_l_generators /= complex_lg;
    }

    // ** RECOMPUTE CONES *******************************************

    // Now create the new cones and weights:
    IncidenceMatrix<> newMaxCones(0, newRays.rows());

    // Make sure, we have at least one "cone" in each fan, even if it is empty
    if (premax.rows() == 0)
      premax = premax / Set<Int>{};

    std::vector<Integer> newWeights;
    if (product_has_weights || uses_weights)
      newWeights.reserve(maximalCones.rows() * premax.rows());

    for (Int pmax = 0; pmax < maximalCones.rows(); ++pmax) {
      Set<Int> product_cone = maximalCones.row(pmax);
      for (Int cmax = 0; cmax < premax.rows(); ++cmax) {
        Set<Int> complex_cone = premax.row(cmax);
        Set<Int> newcone;
        Set<Int> pAffine = product_cone * product_affine;
        Set<Int> pDirectional = product_cone * product_directional;
        Set<Int> cAffine = complex_cone * complex_affine;
        Set<Int> cDirectional = complex_cone * complex_directional;

        // First add the affine rays: For each pair of affine rays add the corresponding index from
        // affineIndices
        for (auto pa = entire(pAffine); !pa.at_end(); ++pa) {
          for (auto ca = entire(cAffine); !ca.at_end(); ++ca) {
            newcone += affineIndices[*pa][*ca];
          }
        }
        // Now add the directional indices
        for (auto pd = entire(pDirectional); !pd.at_end(); ++pd) {
          newcone += pdirIndices[*pd];
        }
        for (auto cd = entire(cDirectional); !cd.at_end(); ++cd) {
          newcone += cdirIndices[*cd];
        }
        newMaxCones /= newcone;
        // Compute weight
        if (product_has_weights || uses_weights) {
          newWeights.push_back((product_has_weights ? weights[pmax] : Integer(1)) * (uses_weights ? preweights[cmax] : Integer(1)));
        }
        // Compute lattice data
        if (product_has_lattice) {
          Set<Int> cone_l_basis = product_l_bases.row(pmax) + Set<Int>(translate(complex_lb.row(cmax), lattice_index_translation));
          new_lattice_bases /= cone_l_basis;
        }
      }
    }

    // Compute the cross product of the local_restrictions
    RestrictedIncidenceMatrix<only_cols> new_local_restriction(0, newRays.rows());
    if (local_restriction.rows() > 0 || pre_local_restriction.rows() > 0) {
      // If one variety is not local, we take all its cones for the product
      IncidenceMatrix<> product_locality(local_restriction);
      if (product_locality.rows() == 0) {
        BigObject current_product("fan::PolyhedralComplex",
                                  "VERTICES", rayMatrix,
                                  "MAXIMAL_POLYTOPES", maximalCones);
        product_locality = all_cones_as_incidence(current_product);
      }
      IncidenceMatrix<> pre_locality(pre_local_restriction);
      if (pre_locality.rows() == 0) {
        pre_locality = all_cones_as_incidence(complexes[ci]);
      }

      for (Int i = 0; i < product_locality.rows(); ++i) {
        Set<Int> pAffine = product_locality.row(i) * product_affine;
        Set<Int> pDirectional = product_locality.row(i) * product_directional;
        for (Int j = 0; j < pre_locality.rows(); ++j) {
          Set<Int> local_cone;
          Set<Int> cAffine = pre_locality.row(j) * complex_affine;
          Set<Int> cDirectional = pre_locality.row(j) * complex_directional;
          // First add the affine rays: For each pair of affine rays add the corresponding index from
          // affineIndices
          for (auto pa = entire(pAffine); !pa.at_end(); ++pa) {
            for (auto ca = entire(cAffine); !ca.at_end(); ++ca) {
              local_cone += affineIndices[*pa][*ca];
            }
          }
          // Now add the directional indices
          for (auto pd = entire(pDirectional); !pd.at_end(); ++pd) {
            local_cone += pdirIndices[*pd];
          }
          for (auto cd = entire(cDirectional); !cd.at_end(); ++cd) {
            local_cone += cdirIndices[*cd];
          }
          new_local_restriction /= local_cone;
        }
      }
    }

    // ** COPY VALUES ONTO NEW PRODUCT ***************************************

    // Copy values
    rayMatrix = newRays;
    if (linMatrix.rows() != 0 || prelin.rows() != 0) {
       linMatrix = linMatrix.rows() == 0 ? prelin : (prelin.rows() == 0 ? linMatrix : linMatrix / prelin);
    } else {
       linMatrix.resize(0, rayMatrix.cols());
    }
    product_dim = rayMatrix.cols() > linMatrix.cols() ? rayMatrix.cols() : linMatrix.cols();
    product_affine = newAffine;
    product_directional = newDirectional;
    maximalCones = newMaxCones;
    weights = Vector<Integer>(newWeights);
    product_has_weights = product_has_weights ||  uses_weights;
    local_restriction = std::move(new_local_restriction);
    product_l_bases = new_lattice_bases;
  }

  // Fill fan with result
  BigObject result("Cycle", mlist<Addition>());
  result.take("VERTICES") << thomog(rayMatrix);
  result.take("LINEALITY_SPACE") << thomog(linMatrix);
  result.take("MAXIMAL_POLYTOPES") << maximalCones;
  if (product_has_weights)
    result.take("WEIGHTS") << weights;
  if (local_restriction.rows() > 0)
    result.take("LOCAL_RESTRICTION") << local_restriction;
  if (product_has_lattice) {
    result.take("LATTICE_BASES") << product_l_bases;
    result.take("LATTICE_GENERATORS") << thomog(product_l_generators);
  }

  return result;
}

} }

#endif
