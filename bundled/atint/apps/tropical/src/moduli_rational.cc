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

	Computes the moduli space of rational n-marked curves.
	*/


#include "polymake/client.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/Rational.h"
#include "polymake/Integer.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/specialcycles.h"
//#include "polymake/atint/moduli.h"

namespace polymake { namespace tropical {


//Counts maximal cones by a simple formula
Integer count_maximal_mn_cones(Int n)
{
  if (n == 3) {
    return 1;
  }
  Integer result = 1;
  Integer nint(n);
  for (Int i = 0; i <= n-4; ++i) {
    result *= 2*(nint-i) - 5;
  }
  return result;
}


// Documentation see perl wrapper
Integer count_mn_cones(Int n, Int k)
{
  if (n == 3) {
    return Integer(1);
  }
  if (k == n-3)
    return count_maximal_mn_cones(n);

  Int vertex_count = k+1;
  Int seq_length = n + k-1;

  // We compute the number of ways that a Prüfer sequence of appropriate length and
  // order can be created:
  // We first compute the number of distributions of total valences (i.e. the distribution of numbers
  // of free spaces in the sequence) on the interior vertices p_0,...,p_k as Integer points of a
  // polytope.
  // For each such valence distribution we compute the number of ways to realize it:
  // p_0 has to fill the first position to create an ordered Prüfer sequence and then
  // we have (# of remaining space CHOOSE valence of p_0) other possibilities. Multiplying over all
  // p_i gives all realizations of the valence distribution and summing over all valence distributions
  // gives all possibilities.

  Matrix<Rational> eq(0,vertex_count+1);
  Vector<Rational> eqvec = ones_vector<Rational>(vertex_count);
  eqvec = Rational(-seq_length) | eqvec;
  eq /= eqvec;

  Matrix<Rational> ineq = unit_matrix<Rational>(vertex_count);
  ineq = ( -2 * ones_vector<Rational>(vertex_count)) | ineq;

  BigObject p("polytope::Polytope",
              "INEQUALITIES", ineq,
              "EQUATIONS", eq);
  Matrix<Integer> latt = p.call_method("LATTICE_POINTS");
  latt = latt.minor(All, range_from(1));

  Integer total(0);
  for (Int l = 0; l < latt.rows(); ++l) {
    Integer prod(1);
    Int sum_vi = 0;
    for (Int v = 0; v < vertex_count-1; ++v) {
      Int vi(latt(l, v));
      prod *= Integer::binom(seq_length-sum_vi-1, vi-1);
      sum_vi += vi;
    }
    total += prod;
  }
  return total;
}

///////////////////////////////////////////////////////////////////////////////////////

//Documentation see perl wrapper
Integer count_mn_rays(Int n)
{
  if (n == 3) {
    return Integer(0);
  }
  Integer result(0);
  Integer nint(n);
  for (long i = 1; i <= n-3; ++i) {
    result += Integer::binom(nint-1, i);
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////////////

/**
   @brief Does exactly the same as count_mn_rays, but returns an Int. Only works for n<=12, since larger values produce too large integers
*/
Int count_mn_rays_int(Int n)
{
  if (n == 3) {
    return 0;
  }
  Int result = 0;
  Int nint = n;
  for (Int i = 1; i <= n-3; ++i) {
    result += Int(Integer::binom(nint-1,i));
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////////////

// Documentation see header
Matrix<Int> pair_index_map(Int n)
{
  Matrix<Int> E(n,n);
  Int nextindex = 0;
  for (Int i = 0; i < n-1; ++i) {
    for (Int j = i+1; j < n; ++j) {
      E(i,j) = E(j,i) = nextindex;
      ++nextindex;
    }
  }
  return E;
}

///////////////////////////////////////////////////////////////////////////////////////

// Documentation see header
Vector<Set<Int>> decodePrueferSequence(const Vector<Int>& pseq, Int n)
{
  // Construct vertex set
  if (n < 0) n = pseq[0];  // The first element is always the number of leaves
  // Compute number of bounded edges
  Int no_of_edges = pseq.dim()-n+1;
  Set<Int> V = sequence(0, n+no_of_edges+1);
  Vector<Set<Int>> adjacencies(no_of_edges+1);  // Which leaves lie "behind" which interior vertex?
  Vector<Set<Int>> result;
  const auto allLeafs = sequence(0,n);

  Int firstindex = 0; //We pretend that pseq starts at this index
  // Connect leaves
  for (Int i = 0; i < n; ++i) {
    adjacencies[pseq[firstindex]-n] += i;
    V -= i;
    ++firstindex;
  } //END add leaves


  // Now create edges
  for (Int i = 1; i <= no_of_edges; ++i) {
    Set<Int> rayset;
    // If there are only two vertices left, connect them
    if (i == no_of_edges) {
      Vector<Int> lasttwo(V);
      rayset = adjacencies[lasttwo[0]-n];
    } else {
      // Find the minimal element in V that is not in the sequence (starting at firstindex)
      Set<Int> pset(pseq.slice(range_from(firstindex)));
      Int smallest = -1;
      for (auto vit = entire(V); !vit.at_end(); ++vit) {
        if (!pset.contains(*vit)) {
          smallest = *vit;
          break;
        }
      } //END look for smallest in V\P
      Set<Int> Av = adjacencies[smallest-n];
      rayset = Av;
      adjacencies[pseq[firstindex]-n] += Av;
      V -= smallest;
      ++firstindex;
    }

    // If rayset contains the last leaf, take the complement
    if (rayset.contains(n-1)) {
      rayset = allLeafs - rayset;
    }

    result |= rayset;
  } //END create edges

  return result;
} //END decodePrueferSequence

///////////////////////////////////////////////////////////////////////////////////////

//Documentation see perl wrapper
template <typename Addition>
BigObject m0n(Int n)
{
  if (n == 3) {
    return projective_torus<Addition>(0,1);	
  }
  if (n < 3) {
    throw std::runtime_error("Number of leaves should be at least 3 for M_0,n computation");
  }

  // First we create the edge index matrix E(i,j) that contains at element i,j the edge index of edge (i,j)
  // in the complete graph on n-1 nodes
  Int nextindex = 0;
  Matrix<Int> E(n-1, n-1);
  for (Int i = 0; i < n-2; ++i) {
    for (Int j = i+1; j < n-1; ++j) {
      E(i,j) = nextindex;
      E(j,i) = nextindex;
      ++nextindex;
    }
  }

  // We compute the set of all ordered paired Prüfer sequences on n,...,2n-3
  // (i.e. all sequences of length 2n-4 where each element from n,...,2n-3 occurs twice
  // and where removing every second occurence of an element yields an ascending sequence)
  // From each such Prüfer sequence we then construct a maximal cone

  // Will contain the rays of the moduli space in matroid coordinates
  Int raydim = (n*(n-3))/2 + 1;
  Int raycount = count_mn_rays_int(n);
  Matrix<Rational> rays(raycount, raydim);

  // Will contain value 'true' for each ray that has been computed
  std::vector<bool> raysComputed(count_mn_rays_int(n));
  //Will contain the set of maximal cones
  RestrictedIncidenceMatrix<> cones;

  // Compute the number of sequences = number of maximal cones
  Int noOfMax(count_mn_cones(n, n-3));

  // Things we will need:
  const auto allLeafs = sequence(0,n); //The complete sequence of leaves (for taking complements)
  Vector<Int> rayIndices(n-2); //Entry k contains the sum from i = 1 to k of binomial(n-1,i)
  rayIndices[0] = 0;
  for (Int i = 1; i < rayIndices.dim(); ++i) {
    rayIndices[i] = rayIndices[i-1] + Int(Integer::binom(n-1,i));
  }

  // Iterate through all Prüfer sequences -------------------------------------------------

  Vector<Int> indices = ones_vector<Int>(n-2);
  Vector<Int> baseSequence(2*n-4);
  Vector<Set<Int>> adjacent(n-2);  // These will be the partitions of the edges
  Vector<Rational> newray(raydim);  // Container for new rays
  for (Int iteration = 0; iteration < noOfMax; ++iteration) {

    // Create the sequence currently represented by indices and append it------------------
    baseSequence.resize(2*n-4);
    baseSequence.fill(0);
    for (Int i = 0; i < n-1; ++i) {
      // Go through the non-zero entries of baseSequence. If it is the first or the indices[i]+1-th,
      // insert an n+i
      Int nonzero_count = -1;
      for (Int entry = 0; entry < baseSequence.dim(); ++entry) {
        if (baseSequence[entry] == 0) {
          ++nonzero_count;
          if (nonzero_count == 0) {
            baseSequence[entry] = n+i;
          }
          if (nonzero_count == indices[i]) {
            baseSequence[entry] = n+i;
            break;
          }
        }
      }
    }

    // We now decode the Prüfer sequence to obtain the corresponding cone---------------------
    Set<Int> newcone;

    Set<Int> V = sequence(0,2*n-2);
    adjacent.fill(Set<Int>());
    // First: Connect the leaves
    for (Int i = 0; i < n; ++i) {
      adjacent[baseSequence[0]-n] += i;
      V -= i;
      baseSequence = baseSequence.slice(range_from(1));
    }
    // Now create edges:
    Int enumber = n-3;
    for (Int i = 1; i <= enumber; ++i) {
      // Construct the leaf partition represented by the curve corresponding to the sequence
      Set<Int> rayset;
      if (i == enumber) {  // If V only has two elements left, simply connect these
        rayset = adjacent[V.front() - n];
      } else {
        Set<Int> pset(baseSequence);
        Int smallest = -1;
        // Find the smallest element in V that is not in P
        for (auto vit = entire(V); !vit.at_end(); ++vit) {
          if (!pset.contains(*vit)) {
            smallest = *vit;
            break;
          }
        }
        rayset = adjacent[smallest-n];
        // Add the leaves of this partition to the adjacency of the newly connected p_i
        adjacent[baseSequence[0]-n] += adjacent[smallest-n];
        // Remove v and p_i
        V -= smallest;
        baseSequence = baseSequence.slice(range_from(1));
      }
      // The new edge is: v_{adjacent[smallest]}. If it containst the last leaf, take the complement
      if (rayset.contains(n-1)) {
        rayset = allLeafs - rayset;
      }

      // Now we compute the index of the ray -----------------------------------------
      // Consider ray as a vector of length n filled with a's and b's where entry i is a iff i is in I
      Int k = n - rayset.size();
      Int bsleft = k-1;
      Int l = 1;
      Int rIndex = rayIndices[k-2];
      while (bsleft > 1) {
        if (rayset.contains(n-l-1)) {
          rIndex += Int(Integer::binom(n-l-1,bsleft-1));
        } else {
          --bsleft;
        }
        ++l;
      }
      Int m = 0;
      while (rayset.contains(m)) { ++m; }
      // at last we add the difference of the indices of the second b' and the first b (-1)
      rIndex += (n-1-l)-m;
      newcone += rIndex;


      // If not, create the corresponding matroid coordinates
      if (!raysComputed[rIndex]) {
        raysComputed[rIndex] = true;
        newray.fill(0);
        for (auto raypair = entire(all_subsets_of_k(rayset,2)); !raypair.at_end(); ++raypair) {
          Int newrayindex = E((*raypair).front(),(*raypair).back());
          // If the newrayindex is one higher than the ray dimension,
          // this means it is the last pair. Also, we don't
          // add -e_n but e_1 + ... + e_{n-1} (as we mod out lineality)
          newray[newrayindex] = Addition::orientation();
        }
        rays.row(rIndex) = newray;
      }
    } //END iterate edges
    cones /= newcone;

    // Increase the indices vector by "1"---------------------------------------------------
    if (iteration < noOfMax-1) {
      Int counterindex = n-3;
      while (indices[counterindex] == 2*(n-counterindex)-5) {
        indices[counterindex] = 1;
        --counterindex;
      }
      ++indices[counterindex];
    }
  } //END iterate cones

  // Add the vertex at the origin
  rays = zero_vector<Rational>(rays.rows()) | rays;

  rays /= unit_vector<Rational>(rays.cols(), 0);

  // Add the vertex to all cones
  const Int vertex = rays.rows() - 1;
  for (auto mc = entire(rows(cones)); !mc.at_end(); ++mc) {
    *mc += vertex;
  }

  const IncidenceMatrix<> result_cones{std::move(cones)};

  BigObject result("Cycle", mlist<Addition>(),
                   "WEIGHTS", ones_vector<Int>(result_cones.rows()),
                   "PROJECTIVE_VERTICES", rays,
                   "MAXIMAL_POLYTOPES", result_cones);
  result.set_description() << "Moduli space M_0," << n;
  return result;
}

		
template <typename Addition>
BigObject space_of_stable_maps(Int n, Int d, Int r)
{
  BigObject moduli = m0n<Addition>(n+d);
  BigObject torus = projective_torus<Addition>(r,1);
  BigObject result = call_function("cartesian_product", moduli, torus);
  result.set_description() << "Moduli space of stable rational maps with " << n << " contracted ends, " << d << " non-contracted ends into the torus of dimension " << d;
  return result;
}

// ------------------------- PERL WRAPPERS ---------------------------------------------------

UserFunction4perl("# @category Moduli of rational curves"
                  "# Computes the number of k-dimensional cones of the tropical moduli space M_0,n"
                  "# @param Int n The number of leaves. Should be >= 3"
                  "# @param Int k The number of bounded edges. This argument is optional and n-3 by default"
                  "# @return Integer The number of k-dimensional cones of M_0,n",
                  &count_mn_cones,"count_mn_cones($;$=$_[0]-3)");


UserFunction4perl("# @category Moduli of rational curves"
                  "# Computes the number of rays of the tropical moduli space M_0,n"
                  "# @param Int n The number of leaves. Should be >= 3"
                  "# @return Integer The number of rays",
                  &count_mn_rays,"count_mn_rays($)");

UserFunctionTemplate4perl("# @category Moduli of rational curves"
                          "# Creates the moduli space of abstract rational n-marked curves. Its coordinates are"
                          "# given as the coordinates of the bergman fan of the matroid of the complete graph on "
                          "# n-1 nodes (but not computed as such)"
                          "# The isomorphism to the space of curve metrics is obtained by choosing"
                          "# the last leaf as special leaf"
                          "# @param Int n The number of leaves. Should be at least 3"
                          "# @tparam Addition Min or Max"
                          "# @return Cycle The tropical moduli space M_0,n",
                          "m0n<Addition>($)");

UserFunctionTemplate4perl("# @category Moduli of rational curves"
                          "# Creates the moduli space of stable maps of rational n-marked curves into a "
                          "# projective torus. It is given as the cartesian product of M_{0,n+d} and R^r,"
                          "# where n is the number of contracted leaves, d the number of non-contracted leaves"
                          "# and r is the dimension of the target torus. The R^r - coordinate is interpreted as "
                          "# the image of the last (n-th) contracted leaf."
                          "# Due to the implementation of [[cartesian_product]], the projective coordinates are"
                          "# non-canonical: Both M_{0,n+d} and R^r are dehomogenized after the first coordinate, then"
                          "# the product is taken and homogenized after the first coordinate again."
                          "# Note that functions in a-tint will usually treat this space in such a way that the"
                          "# first d leaves are the non-contracted ones and the remaining n leaves are the "
                          "# contracted ones."
                          "# @param Int n The number of contracted leaves"
                          "# @param Int d The number of non-contracted leaves"
                          "# @param Int r The dimension of the target space for the stable maps."
                          "# @tparam Addition Min or Max. Determines the coordinates."
                          "# @return Cycle The moduli space of rational stable maps.",
                          "space_of_stable_maps<Addition>($,$,$)");

//Function4perl(&decodePrueferSequence,"dcp(Vector<Int>;$=-1)");
//   UserFunction4perl("",&adjacentRays,"adjacentRays(RationalCurve)");

} }
