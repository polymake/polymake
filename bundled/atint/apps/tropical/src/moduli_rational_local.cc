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

	Functions to compute local versions of M_0,n and to analyse such moduli spaces
	locally.
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/moduli_rational.h"

namespace polymake { namespace tropical {

/**
   @brief Computes all paired ordered Prüfer sequences of order n, i.e. all sequences of length 2n-4 where each element from n,...,2n-3 occurs twice and where removing every second occurence of an element yields an ascending sequence)
   @param Int n Should be greater equal 3
   @return Vector<Vector<Int>> A list of index sequences
*/
Vector<Vector<Int>> computePrueferSequences(Int n)
{
  if (n < 3) throw std::runtime_error("Cannot compute M_n cones for n < 3");

  // Compute number of sequences
  Int noOfCones(count_mn_cones(n, n-3));

  Vector<Vector<Int>> result;

  // The idea of the algorithm is the following: Indices describes the Prüfer sequence in the 
  // following way:
  // If the entry at position i is j, then the second occurence of i is placed at the (j+1)-st
  // *possible* (i.e. not yet filled) position. Note that the first occurence of each i is alredy
  // determined by the placement of all entries lower than i.
  Vector<Int> indices = ones_vector<Int>(n-2);
  for (Int iteration = 0; iteration < noOfCones; ++iteration) {
    Vector<Int> baseSequence = zero_vector<Int>(2*n -4);
    for (Int i = 0; i < n-1; ++i) {
      // Go through the zero entries of baseSequence. If it is the first or the indices[i]+1-th, 
      // insert an n+i
      Int zero_count = -1;
      for (Int entry = 0; entry < baseSequence.dim(); ++entry) {
        if (baseSequence[entry] == 0) {
          ++zero_count;
          if (zero_count == 0) {
            baseSequence[entry] = n+i;
          }
          if (zero_count == indices[i]) {
            baseSequence[entry] = n+i;
            break;
          }
        }
      }
    }
    result |= baseSequence;
    // Increase the indices vector by "1"---------------------------------------------------    
    if (iteration < noOfCones-1) {
      Int counterindex = n-3;
      while (indices[counterindex] == 2*(n-counterindex)-5) {
        indices[counterindex] = 1;
        --counterindex;
      }
      ++indices[counterindex];
    }
  }

  return result;
}

// Documentation see perl wrapper
template <typename Addition>
BigObject local_m0n(const Array<BigObject>& curves)
{
  // Compute the set of all vertex valences > 3
  Set<Int> valences;
  for (Int c = 0; c < curves.size(); ++c) {
    Vector<Int> degrees = curves[c].give("NODE_DEGREES");
    valences += (Set<Int>(degrees) - 3);
  }

  // Now we compute combinatorially all M_0,v for all possible valences v
  Map<Int, Vector<Vector<Set<Int>>>> combinatorial_mns;
  for (auto v = entire(valences); !v.at_end(); ++v) {
    Vector<Vector<Int>> pseq = computePrueferSequences(*v);
    Vector<Vector<Set<Int>>> cmns;
    for (Int p = 0; p < pseq.dim(); p++)
      cmns |= decodePrueferSequence(pseq[p]);
    combinatorial_mns[*v] = cmns;
  }

  // Now iterate through all curves and compute their adjacent maximal cones
  Vector<Set<Int>> rays; // Ray list in terms of partitions
  Vector<Set<Int>> cones; // Cone list in terms of ray indices
  Vector<Set<Int>> local_cones; 

  Int n_leaves = curves[0].give("N_LEAVES");
  const auto all_leaves = sequence(1,n_leaves);

  // First we add all the curve rays and construct the corresponding local cones
  for (Int cu = 0; cu < curves.size(); ++cu) {
    IncidenceMatrix<> set_list = curves[cu].give("SETS");
    Set<Int> l_cone;
    // For each ray, check if it exists already
    for (Int cu_ray = 0; cu_ray < set_list.rows(); ++cu_ray) {
      // Normalize
      Set<Int> cu_set = set_list.row(cu_ray);
      if (cu_set.contains(n_leaves)) cu_set = all_leaves - cu_set;
      Int ray_index = -1;
      for (Int oray = 0; oray < rays.dim(); ++oray) {
        if (rays[oray] == cu_set) {
          ray_index = oray; break;
        }
      }
      if (ray_index == -1) {
        rays |= cu_set;
        ray_index = rays.dim()-1;
      }
      l_cone += ray_index;
    }
    local_cones |= l_cone;
  } // END create curve rays and local cones

  // Then we construct the actual cones
  for (Int cu = 0; cu < curves.size(); ++cu) {
    // We iteratively compute the cartesian product of the M_0,ns at the different vertices
    Vector<Set<Int>> cones_so_far; 
    cones_so_far |= local_cones[cu];
    IncidenceMatrix<> nodes_by_sets = curves[cu].give("NODES_BY_SETS");
    IncidenceMatrix<> nodes_by_leaves = curves[cu].give("NODES_BY_LEAVES");
    IncidenceMatrix<> set_list = curves[cu].give("SETS");
    Vector<Int> degrees = curves[cu].give("NODE_DEGREES");
    for (Int node = 0; node < nodes_by_sets.rows(); ++node) {
      if (degrees[node] > 3) {
        // We have to translate the cones of the M_0,val(node) first
        Vector<Set<Int>> translated_cones;
        Vector<Vector<Set<Int>>> valence_cones = combinatorial_mns[degrees[node]];
        // The semantics are the following: The edges adjacent to node are numbered
        // in this order: First the leaves ordered from lowest to highest number, then the 
        // bounded edges, ordered according to the order of their appearance in NODES_BY_SETS
        Vector<Set<Int>> adjacent_edges;
        for (const Int l : nodes_by_leaves.row(node)) {
          Set<Int> singleset = scalar2set(l);
          adjacent_edges |= singleset;
        }
        Vector<Int> sets(nodes_by_sets.row(node));
        for (Int s = 0; s < sets.dim(); ++s) {
          // We normalize the sets to 'point away' from the node, i.e. they should have
          // empty intersection with the first leaf (or already normalized set)
          // If there is no leaf or normalized set, the intersection with the next set 
          // must be either emtpy or this set. Otherwise we take the complement.
          Set<Int> compare_set;
          if (adjacent_edges.dim() > 0)
            compare_set = adjacent_edges[0];
          else
            compare_set = set_list.row(sets[1]);
          Int isize = (set_list.row(sets[s]) * compare_set).size();
          if (isize == 0 || isize == set_list.row(sets[s]).size()) {
            adjacent_edges |= set_list.row(sets[s]);
          } else {
            adjacent_edges |= all_leaves - set_list.row(sets[s]);
          }
        }
        // Now go through the cones and translate
        Vector<Set<Int>> new_cones;
        for (Int vc = 0; vc < valence_cones.dim(); ++vc) {
          Set<Int> ray_indices;
          // Go through all rays of the cone
          for (Int r = 0; r < valence_cones[vc].dim(); ++r) {
            // Translate the ray
            Set<Int> ray_partition = accumulate(adjacent_edges.slice(valence_cones[vc][r]),operations::add());
            // To avoid doubles via complements, we make sure that N_LEAVES is always in the complement
            if (ray_partition.contains(n_leaves))
              ray_partition = all_leaves - ray_partition;
            // Check if this ray already exists
            Int ray_index = -1;
            for (Int oray = 0; oray < rays.dim(); ++oray) {
              if (rays[oray] == ray_partition) {
                ray_index = oray; break;
              }
            }
            if (ray_index == -1) {
              rays |= ray_partition;
              ray_index = rays.dim()-1;
            }
            ray_indices += ray_index;
          } // END iterate cone rays
          new_cones |= ray_indices;
        } // END iterate local mn cones
        // Finally take the cartesian products of the new cones with all the old ones
        Vector<Set<Int>> replace_cones_so_far;
        for (Int nc = 0; nc < new_cones.dim(); ++nc) {
          for (Int oc = 0; oc < cones_so_far.dim(); ++oc) {
            replace_cones_so_far |= (new_cones[nc] + cones_so_far[oc]);
          }
        }
        cones_so_far = replace_cones_so_far;
      } // END if deg(v) > 3
    } // END iterate nodes
    // Now we have to check for cones that already exist
    Set<Int> double_cones;
    for (Int nc = 0; nc < cones_so_far.dim(); ++nc) {
      for (Int oc = 0; oc < cones.dim(); ++oc) {
        if (cones[oc] == cones_so_far[nc])
          double_cones += nc;
      }
    }
    cones |= cones_so_far.slice(~double_cones);
  } // END iterate curves

  // Finally we convert the rays to matroid coordinates
  Int nextindex = 0;
  Matrix<Int> E(n_leaves-1,n_leaves-1);
  for (Int i = 0; i < n_leaves-2; ++i) {
    for (Int j = i+1; j < n_leaves-1; ++j) {
      E(i,j) = nextindex;
      E(j,i) = nextindex;
      ++nextindex;
    }
  }
  Matrix<Rational> bergman_rays(rays.dim(),(n_leaves*(n_leaves-3))/2 + 1);
  for (Int r = 0; r < rays.dim(); ++r) {
    Vector<Int> raylist(rays[r]);
    Vector<Rational> newray(bergman_rays.cols());
    for (Int k = 0; k < raylist.dim()-1; ++k) {
      for (Int l = k+1; l < raylist.dim(); ++l) {
        Int newrayindex = E(raylist[k]-1,raylist[l]-1);
        newray[newrayindex] = Addition::orientation();
      }
    }
    bergman_rays.row(r) = newray; 
  }

  // Add vertex 
  bergman_rays = zero_vector<Rational>() | bergman_rays;
  bergman_rays /= unit_vector<Rational>(bergman_rays.cols(),0);
  for (Int c = 0; c < cones.dim(); ++c) {
    cones[c] += scalar2set(bergman_rays.rows()-1);
  }
  for (Int lc = 0; lc < local_cones.dim(); ++lc) {
    local_cones[lc] += scalar2set(bergman_rays.rows()-1);
  }

  Vector<Integer> weights = ones_vector<Integer>(cones.dim());

  return BigObject("Cycle", mlist<Addition>(),
                   "PROJECTIVE_VERTICES", bergman_rays,
                   "MAXIMAL_POLYTOPES", cones,
                   "WEIGHTS", weights,
                   "LOCAL_RESTRICTION", local_cones);
}

UserFunctionTemplate4perl("# @category Moduli of rational curves" 
                          "# Computes the moduli space M_0,n locally around a given list of combinatorial"
                          "# types. More precisely: It computes the weighted complex consisting of all"
                          "# maximal cones containing any of the given combinatorial types and localizes "
                          "# at these types "
                          "# This should only be used for curves of small codimension. What the function "
                          "# actually does, is that it combinatorially computes the cartesian products "
                          "# of M_0,v's, where v runs over the possible valences of vertices in the curves"
                          "# For max(v) <= 8 this should terminate in a reasonable time (depending on the "
                          "# number of curves)"
                          "# The coordinates are the same that would be produced by the function "
                          "# [[m0n]]"
                          "# @param RationalCurve R ... A list of rational curves (preferrably in the same M_0,n)"
                          "# @tparam Addition Min or Max, determines the coordinates"
                          "# @return Cycle<Addition> The local complex",
                          "local_m0n<Addition>(RationalCurve+)");
} }
