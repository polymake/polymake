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

	This contains functions for computing refinements
	*/

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/separated_data.h"
#include "polymake/tropical/linear_algebra_tools.h"
#include "polymake/tropical/minimal_interior.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/refine.h"
#include "polymake/polytope/convex_hull.h"


namespace polymake { namespace tropical {

///////////////////////////////////////////////////////////////////////////////////////

RefinementResult refinement(perl::Object X, perl::Object Y,
                            bool repFromX, bool repFromY,bool computeAssoc,bool refine, bool forceLatticeComputation)
{
  // Sanity check
  if (call_function("is_empty", X)) {
    int ambient_dim=X.give("PROJECTIVE_AMBIENT_DIM");
    ambient_dim+=2;
    RefinementResult r;
    r.complex = X;
    r.rayRepFromX = Matrix<Rational>(0, ambient_dim);
    r.linRepFromX = Matrix<Rational>(0, ambient_dim);
    r.rayRepFromY = Matrix<Rational>(0, ambient_dim);
    r.linRepFromY = Matrix<Rational>(0, ambient_dim);
    r.associatedRep = Vector<int>();
    return r;
  }

  // All computations will be done on affine charts of X and Y, the result will then be lifted
  // at the end.
  // This is to reduce the amount of work we have to do in convex hull computations

  // Extract values of the variety

  const Matrix<Rational> &x_rays_ref = X.give("VERTICES");
  const Matrix<Rational> x_rays = tdehomog(x_rays_ref);
  const IncidenceMatrix<> &x_cones = X.give("MAXIMAL_POLYTOPES");
  Matrix<Rational> x_cmplx_rays(0, x_rays.cols());
  if (repFromX) {
    X.give("SEPARATED_VERTICES") >> x_cmplx_rays;
    x_cmplx_rays = tdehomog(x_cmplx_rays);
  }
  const IncidenceMatrix<> &x_cmplx_cones = repFromX? X.give("SEPARATED_MAXIMAL_POLYTOPES") : IncidenceMatrix<>();
  const Matrix<Rational> &x_lineality_ref = X.give("LINEALITY_SPACE");
  const Matrix<Rational> x_lineality = tdehomog(x_lineality_ref);
  const int x_lineality_dim = X.give("LINEALITY_DIM");
  const int ambient_dim = x_rays.cols();
  const int x_dimension = X.give("PROJECTIVE_DIM");
  Vector<Integer> weights; bool weightsExist = false;
  if (X.exists("WEIGHTS")) {
    X.give("WEIGHTS") >> weights;
    weightsExist = true;
  }
  IncidenceMatrix<> local_restriction;
  if (X.exists("LOCAL_RESTRICTION")) {
    X.give("LOCAL_RESTRICTION") >> local_restriction;
  }
  bool lattice_exists = false;
  Matrix<Integer> lattice_generators;
  IncidenceMatrix<> lattice_bases;
  if (X.exists("LATTICE_BASES") || forceLatticeComputation) {
    X.give("LATTICE_GENERATORS") >> lattice_generators;
    lattice_generators = tdehomog(Matrix<Rational>(lattice_generators));
    X.give("LATTICE_BASES") >> lattice_bases;
  }

  // Extract values of the container
  const Matrix<Rational>& y_rays_ref = Y.give("VERTICES");
  const Matrix<Rational> y_rays = tdehomog(y_rays_ref);
  Matrix<Rational> y_cmplx_rays(0, y_rays.cols());
  if (repFromY) {
    Y.give("SEPARATED_VERTICES") >> y_cmplx_rays;
    y_cmplx_rays = tdehomog(y_cmplx_rays);
  }
  const IncidenceMatrix<>& y_cmplx_cones = repFromY ? Y.give("SEPARATED_MAXIMAL_POLYTOPES") : IncidenceMatrix<>();
  const IncidenceMatrix<>& y_cones = Y.give("MAXIMAL_POLYTOPES");
  const Matrix<Rational> &y_lineality_ref = Y.give("LINEALITY_SPACE");
  const Matrix<Rational> y_lineality  = tdehomog(y_lineality_ref);


  // Prepare result variables
  perl::Object complex(X.type());
  Matrix<Rational> c_rays(0,ambient_dim);
  Matrix<Rational> c_lineality(0,ambient_dim);
  int c_lineality_dim = 0;
  Vector<Set<int>> c_cones;
  Vector<Integer> c_weights;
  Matrix<Integer> c_lattice_g = lattice_generators;
  Vector<Set<int>> c_lattice_b;
  Matrix<Rational> rayRepFromX;
  Matrix<Rational> rayRepFromY;
  Matrix<Rational> linRepFromX;
  Matrix<Rational> linRepFromY;
  Vector<int> associatedRep;


  // If we don't refine, we already know most of the results concerning X
  if (!refine) {
    c_rays = x_rays;
    for (int xr = 0; xr < x_cones.rows(); ++xr) c_cones |= x_cones.row(xr);
    c_weights = weights;
    // if(computeAssoc) associatedRep = Vector<int>(x_cones.rows());
    if (repFromX) rayRepFromX = unit_matrix<Rational>(x_cmplx_rays.rows()) |
                                zero_matrix<Rational>(x_cmplx_rays.rows(),x_lineality.rows());
  }


  // Step 1: Compute the lineality space ----------------------------------
  if (x_lineality.rows() != 0 && y_lineality.rows() != 0) {
    if (refine) {
      // Compute the intersection of the two spaces
      // We compute the kernel of (x_lineality | -y_lineality)
      Matrix<Rational> i_lineality = T(x_lineality  / (-y_lineality));
      Matrix<Rational> dependence =  null_space(i_lineality);
      c_lineality = dependence.minor(All,sequence(0,x_lineality.rows())) * x_lineality;
      c_lineality_dim = rank(c_lineality);
      //Compute X-rep if necessary
      if (repFromX) {
        linRepFromX = dependence.minor(All,sequence(0,x_lineality.rows()));
      }
      if (repFromY) {
        linRepFromY = dependence.minor(All,sequence(x_lineality.rows(),y_lineality.rows()));
      }
    } else {
      c_lineality = x_lineality;
      c_lineality_dim = x_lineality_dim;
      if (repFromX) {
        linRepFromX = unit_matrix<Rational>(x_lineality.rows());
      }
      if (repFromY) {
        // We compute the kernel of (x_lineality | -y_lineality) and invert the first
        // half to obtain a description of x_lineality in terms of y_lineality
        linRepFromY = null_space(T(x_lineality / (-y_lineality)));
        linRepFromY = (inv(linRepFromY.minor(All,sequence(0,x_lineality.rows()))) * linRepFromY).minor(All,sequence(x_lineality.rows(),y_lineality.rows()));
      }
    }
  } //END compute lineality space


  // Step 2: Compute cone refinement and ray representations. -----------------

  // If any of the two is just a lineality space, we still have to consider this as a single
  // maximal cone for refinement / representation
  const bool x_onlylineality = x_cones.rows() == 0;
  const bool y_onlylineality = y_cones.rows() == 0;

  // Compute a facet representation for the Y-cones
  std::vector<polytope::convex_hull_result<Rational>> y_equations;
  if (y_onlylineality) {
    y_equations.reserve(1);
    y_equations.push_back(polytope::enumerate_facets(Matrix<Rational>(0,y_lineality.cols()), y_lineality, false));
  } else {
    const int n_y_equations = y_cones.rows();
    y_equations.reserve(n_y_equations);
    for (int yc = 0; yc < n_y_equations; ++yc) {
      y_equations.push_back(polytope::enumerate_facets(y_rays.minor(y_cones.row(yc), All), y_lineality, false));
    }
  }

  // This saves for each x-cone (index in x_cones) the cones that have been created as refinements.
  // Since doubles can only occur when the same x-cone is contained in the intersection of several y-cones,
  // we only have to check the cones in xrefinements[xc]
  Vector<Set<Set<int>>> xrefinements(x_onlylineality ? 1 : x_cones.rows());

  // These variables save for each cone of the intersection the indices of the cones in X and Y containing it
  Vector<int> xcontainers;
  Vector<int> ycontainers;

  // Data on local restriction
  // This variable declares whether local cone i has been subdivided yet
  Array<bool> local_subdivided(local_restriction.rows());
  // The list of new local cones.
  IncidenceMatrix<> new_local_restriction;

  // -----------------------------------------------------------------------------------
  if (refine || repFromX || repFromY) {
    // Iterate all cones of X
    for (int xc = 0; xc < (x_onlylineality ? 1 : x_cones.rows()); ++xc) {
      // If we have local restriction, we have to find all not-yet-subdivided local cones
      // contained in xc
      Vector<int> xc_local_cones;  // Saves position indices in array local_restriction
      if (local_restriction.rows() > 0) {
        for (int lc = 0; lc < local_restriction.rows(); ++lc) {
          if (!local_subdivided[lc]) {
            if ((local_restriction.row(lc) * x_cones.row(xc)).size() == local_restriction.row(lc).size()) {
              xc_local_cones |= lc;
            }
          }
        }
      }
      // Initalize refinement cone set
      xrefinements[xc] = Set<Set<int> >();
      // Compute a facet representation for the X-cone
      const auto x_equations = polytope::enumerate_facets( x_onlylineality ? Matrix<Rational>(0,x_lineality.cols()) :
                                                           x_rays.minor(x_cones.row(xc),All), x_lineality, false );

      // Iterate all cones of Y
      for (int yc = 0; yc < (y_onlylineality? 1 : y_cones.rows()); ++yc) {
        // Compute a V-representation of the intersection
        Matrix<Rational> interrays = polytope::try_enumerate_vertices(x_equations.first / y_equations[yc].first,
                                                                      x_equations.second / y_equations[yc].second, false).first;

        // Check if it is full-dimensional (and has at least one ray - lin.spaces are not interesting)
        if (interrays.rows() > 0 && rank(interrays) + c_lineality_dim - 1 == x_dimension) {
          // If we refine, add the cone. Otherwise just remember the indices
          Set<int> interIndices;
          if (!refine) {
            // Copy indices
            interIndices = x_cones.row(xc);
            ycontainers |= yc;
          } else {
            // Now we canonicalize the rays and assign ray indices
            Set<int> newRays;
            for (int rw = 0; rw < interrays.rows(); ++rw) {
              if (interrays(rw,0) != 0) {
                // Vertices start with a 1
                interrays.row(rw) /= interrays(rw,0);
              } else {
                // The first non-zero entry in a directional ray is +-1
                for (int cl = 0; cl < interrays.cols(); ++cl) {
                  if (interrays(rw,cl) != 0) {
                    interrays.row(rw) /= abs(interrays(rw,cl));
                    break;
                  }
                }
              }


              // Go through the existing rays and compare
              int nrays = c_rays.rows();
              int newrayindex = -1;
              for (int oray = 0; oray < nrays; ++oray) {
                if (interrays.row(rw) == c_rays.row(oray)) {
                  newrayindex = oray;
                  break;
                }
              }
              if (newrayindex == -1) {
                c_rays /= interrays.row(rw);
                newrayindex = c_rays.rows()-1;
                newRays += newrayindex;
              }
              interIndices += newrayindex;
            } // END canonicalize rays


            // Check if the cone exists - if there are new rays, then the cone must be new as well
            bool addCone = newRays.size() > 0;
            if (!addCone) addCone = !xrefinements[xc].contains(interIndices);
            // If the cone is new, add it
            if (addCone) {
              c_cones |= interIndices;
              if (weightsExist) c_weights |= weights[xc];
              if (lattice_exists) {
                c_lattice_b |= lattice_bases.row(xc);
              }
              xrefinements[xc] += interIndices;
              xcontainers |= xc;
              ycontainers |= yc;
            }

          } // END canonicalize intersection cone and add it

          // If we do not refine, we only need to find one y-cone containing the x-cone
          if (!refine) break;
        } // END if full-dimensional
      }// END iterate y-cones


      // Now compute new local cones: Go through all subdivison cones of xc,
      // go through all local cones in xc. Check which ray of the subdivision cone
      // lies in the local cone. If the cone spanned by these has the right dimension
      // add it as a local cone
      if (local_restriction.rows() > 0 && refine) {
        for (int t = 0; t < xc_local_cones.dim(); ++t) {
          // Will contain the subdivision cones of the local cone we currently study
          Set<Set<int>> local_subdivision_cones;
          Matrix<Rational> lrays = x_rays.minor(local_restriction[xc_local_cones[t]],All);
          int local_cone_dim = rank(lrays) + x_lineality_dim;
          for (auto s = entire(xrefinements[xc]); !s.at_end(); ++s) {
            // Check which rays of refinement cone lie in local cone
            Set<int> cone_subset;
            for (auto cs = entire(*s); !cs.at_end(); ++cs) {
              if (is_ray_in_cone(lrays, x_lineality, c_rays.row(*cs), false)) {
                cone_subset += *cs;
              }
            }
            // If the dimension is correct, add the new local cone
            if (rank(c_rays.minor(cone_subset,All)) + c_lineality_dim == local_cone_dim) {
              local_subdivision_cones += cone_subset;
            }
            local_subdivided[xc_local_cones[t]] = true;
          } // END iterate all refinement cones of xc
          // Finally we add the minimal interior faces of the subdivision as new local cones
          new_local_restriction = minimal_interior(c_rays, IncidenceMatrix<>(local_subdivision_cones));
        } // END iterate all remaining local cones in xc
      } // END refine local cones and remove non compatible maximal cones

    } // END iterate x-cones
  } // END if intersection is necessary?

  IncidenceMatrix<> c_cones_result(c_cones); //Copy result cones for local restriction clean-up
  IncidenceMatrix<> local_restriction_result(new_local_restriction.rows(), c_rays.rows());
  local_restriction_result.minor(All, sequence(0, new_local_restriction.cols())) = new_local_restriction;

  // At the end we still have to check if all maximal cones are still compatible
  // and remove those that aren't
  if (local_restriction.rows() > 0 && refine) {
    Set<int> removableCones;
    for (int c = 0; c < c_cones.dim(); ++c) {
      if (!is_coneset_compatible(c_cones[c],local_restriction_result)) {
        removableCones += c;
      }
    }
    // Remove cones
    c_cones = c_cones.slice(~removableCones);
    c_weights = c_weights.slice(~removableCones);
    xcontainers = xcontainers.slice(~removableCones);
    ycontainers = ycontainers.slice(~removableCones);
    // Remove unused rays
    Set<int> used_rays = accumulate(c_cones, operations::add());
    c_rays = c_rays.minor(used_rays,All);
    if (lattice_exists) {
      c_lattice_b = c_lattice_b.slice(~removableCones);
    }
    c_cones_result = c_cones_result.minor(~removableCones,used_rays);
    local_restriction_result = local_restriction_result.minor(All,used_rays);
  } //END finish up locality computation

  // Copy return values into the fan
  if (refine) {
    complex.take("VERTICES") << thomog(c_rays);
    complex.take("MAXIMAL_POLYTOPES") << c_cones_result;
    complex.take("LINEALITY_SPACE") << thomog(c_lineality);
    if (weightsExist) complex.take("WEIGHTS") << c_weights;
    if (lattice_exists) {
      complex.take("LATTICE_BASES") << c_lattice_b;
      complex.take("LATTICE_GENERATORS") << thomog(Matrix<Rational>(lattice_generators));
    }
    if (local_restriction.rows() > 0)
      complex.take("LOCAL_RESTRICTION") << local_restriction_result;
  } else {
    complex = X;
  }

  // To compute representations of SEPARATED_VERTICES, we naturally
  // have to compute the SEPARATED_VERTICES first
  if ((repFromX && refine) || repFromY || computeAssoc) {
    Matrix<Rational> c_cmplx_rays = complex.give("SEPARATED_VERTICES");
    c_cmplx_rays = tdehomog(c_cmplx_rays);
    IncidenceMatrix<> c_cmplx_cones = complex.give("SEPARATED_MAXIMAL_POLYTOPES");
    // Initialize rep matrices to proper size
    rayRepFromX = Matrix<Rational>(c_cmplx_rays.rows(),x_cmplx_rays.rows() + x_lineality.rows());
    rayRepFromY = Matrix<Rational>(c_cmplx_rays.rows(),y_cmplx_rays.rows() + y_lineality.rows());
    // Compute representations for X (mode 0) and/or Y (mode 1)
    for (int mode = 0; mode <= 1; ++mode) {
      if ((mode == 0 && repFromX) || (mode == 1 && repFromY)) {
        // Recalls for which ray we already computed a representation
        Vector<bool> repComputed(c_cmplx_rays.rows());
        Matrix<Rational> raysForComputation = (mode == 0? x_cmplx_rays : y_cmplx_rays);
        Matrix<Rational> linForComputation = (mode == 0? x_lineality : y_lineality);
        // Go through all complex cones
        for (int cone = 0; cone < c_cmplx_cones.rows(); ++cone) {
          // Go through all rays for which we have not yet computed a representation
          Set<int> raysOfCone = c_cmplx_cones.row(cone);
          for (auto r = entire(raysOfCone); !r.at_end(); ++r) {
            if (!repComputed[*r]) {
              repComputed[*r] = true;
              // The ray used for computing the representation are the rays of the containing
              // cone (or none, if the corr. fan is only a lineality space)
              Set<int> rfc;
              if (mode == 0 && !x_onlylineality)
                rfc = x_cmplx_cones.row(xcontainers[cone]);
              if (mode == 1 && !y_onlylineality)
                rfc = y_cmplx_cones.row(ycontainers[cone]);
              // Compute representation vector
              Vector<Rational> repv = linearRepresentation(c_cmplx_rays.row(*r),
                                                           raysForComputation.minor(rfc,All)/ linForComputation);
              if (repv.dim() == 0)
                throw std::runtime_error("Error computing representations during refinement. Rays is not in span of refined cone.");
              (mode == 0? rayRepFromX : rayRepFromY).row(*r).slice(rfc) = repv;
            }
          }
        } // END iterate all cones
      } // END if mode
    } // END go through X- and Y-mode

    if (computeAssoc) {
      // For each cmplx_ray, in which cone does it lie?
      IncidenceMatrix<> c_cmplx_cones_t = T(c_cmplx_cones);
      for (int cr = 0; cr < c_cmplx_rays.rows(); ++cr) {
        if (c_cmplx_rays(cr,0) == 1) {
          associatedRep |= cr;
        } else {
          // Take a cone containing ray cr
          int mc = *(c_cmplx_cones_t.row(cr).begin());
          // Find an affine ray in this cone
          Set<int> mcrays = c_cmplx_cones.row(mc);
          for (auto mcr = entire(mcrays); !mcr.at_end(); ++mcr) {
            if (c_cmplx_rays(*mcr,0) == 1) {
              associatedRep |= *mcr;
              break;
            }
          }
        }
      }
    } // END if computeAssoc
  } // END compute nontrivial representations

  // Insert values

  RefinementResult result;
  result.complex = complex;
  result.rayRepFromX = rayRepFromX;
  result.rayRepFromY = rayRepFromY;
  result.linRepFromX = linRepFromX;
  result.linRepFromY = linRepFromY;
  result.associatedRep = associatedRep;
  return result;

} // END refinement


perl::Object intersect_container(perl::Object cycle, perl::Object container,
                                 bool forceLatticeComputation)
{
  RefinementResult r = refinement(cycle, container, false, false, false, true, forceLatticeComputation);
  return r.complex;
} // END intersect_container


UserFunction4perl("# @category Basic polyhedral operations"
                  "# Takes two Cycles and computes the intersection of both. The function"
                  "# relies on the fact that the second cycle contains the first cycle to "
                  "# compute the refinement correctly"
                  "# The function copies [[WEIGHTS]], [[LATTICE_BASES]] and [[LATTICE_GENERATORS]]"
                  "# in the obvious manner if they exist."
                  "# @param Cycle cycle An arbitrary Cycle"
                  "# @param Cycle container A cycle containing the first one (as a set)"
                  "# Doesn't need to have any weights and its tropical addition is irrelevant."
                  "# @param Bool forceLatticeComputation Whether the properties"
                  "# [[LATTICE_BASES]] and [[LATTICE_GENERATORS]] of cycle should be computed"
                  "# before refining. False by default."
                  "# @return Cycle The intersection of both complexes"
                  "# (whose support is equal to the support of cycle)."
                  "# It uses the same tropical addition as cycle.",
                  &intersect_container,"intersect_container(Cycle,Cycle;$=0)");

} }
