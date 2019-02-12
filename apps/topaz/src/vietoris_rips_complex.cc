#include "polymake/Graph.h"
#include "polymake/PowerSet.h"
#include "polymake/topaz/Filtration.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Rational.h"

namespace polymake { namespace topaz {

perl::Object vietoris_rips_complex(Matrix<Rational> dist, Rational step)
{
  perl::Object NG = call_function("neighborhood_graph", dist, step);
  perl::Object vr_complex = call_function("clique_complex", NG);
  vr_complex.set_description() << "Vietoris Rips complex of the input point set." << endl;
  return vr_complex;
}

// this does not use the existing clique/flag complex functionality, as we need to calculate degrees for each face.
template <typename Coeff>
Filtration<SparseMatrix<Coeff>> vietoris_rips_filtration(const Matrix<float>& dist, const Array<int>& point_degs, float step, int k)
{
  using pair = std::pair<int, int>;

  int n = dist.rows();

  if (k>=n) k=n-1; //TODO this is ugly

  Map<Set<int>, pair> deg_idx; //first pair entry is degree in filtration, second is index in bd matrix

  int size = 0;
  for (int i=1; i<=k+1; ++i)
    size += int(Integer::binom(n,i)); //number of all subsets of [n] with <=k members
  Array<Cell> F(size); //TODO: IntType thingy

  int cell_index = 0; //index in filtration

  Array<SparseMatrix<Coeff>> bd(k+1);
  bd[0] = SparseMatrix<Coeff>(n,1);

  for (int index = 0; index < n; ++index) {
    bd[0][index][0] = 1;
    deg_idx[Set<int>{index}] = pair(point_degs[index], index); //get point degrees from input array
    Cell c(point_degs[index], 0, index);
    F[cell_index++] = c;
  }
  Set<int> facet = sequence(0,n); //the full-dimensional simplex

  for (int d = 1; d<=k; ++d) { //iterate over dimensions to max_dim
    bd[d]=SparseMatrix<Coeff>(int(Integer::binom(n,d+1)),int(Integer::binom(n,d)));

    int index = 0; //index in bd matrix

    for (auto i=entire(all_subsets_of_k(facet,d+1)); !i.at_end(); ++i) { //iterate over all simplices of dimension d

      int sgn = 1;  //entry in bd matrix simply alternates as simplices are enumerated lexicographically
      int max = 0; //maximal degree of boundary simplices
      Set<int> simplex(*i);

      for (auto s=entire(all_subsets_of_k(simplex,d)); !s.at_end(); ++s){ //iterate over boundary and find maximal degree

        pair p = deg_idx[*s];

        int s_deg = p.first;
        if (s_deg > max) max = s_deg;

        bd[d][index][p.second] = sgn; // set bd matrix entry
        sgn *= -1;
      }
      if (d==1) { //for edges, calculate degree from distance matrix
        float l = dist(simplex.front(),simplex.back());
        int edge_deg = ceil(l/step);
        if(edge_deg > max) max = edge_deg;
      }

      deg_idx[simplex] =  pair(max,index);
      Cell c(max,d,index);
      F[cell_index++] = c;
      ++index;
    }
  }
  return Filtration<SparseMatrix<Coeff> >(F,bd);
}


UserFunction4perl("# @category Producing a simplicial complex from other objects"
                  "# Computes the __Vietoris Rips complex__ of a point set. The set is passed as its so-called \"distance matrix\", whose (i,j)-entry is the distance between point i and j. This matrix can e.g. be computed using the distance_matrix function. The points corresponding to vertices of a common simplex will all have a distance less than //delta// from each other."
                  "# @param Matrix D the \"distance matrix\" of the point set (can be upper triangular)"
                  "# @param Rational delta"
                  "# @return SimplicialComplex"
                  ,&vietoris_rips_complex, "vietoris_rips_complex($$)");


UserFunctionTemplate4perl("# @category Other"
                          "# Constructs the k-skeleton of the Vietrois Rips filtration of a point set. The set is passed as its so-called \"distance matrix\", whose (i,j)-entry is the distance between point i and j. This matrix can e.g. be computed using the distance_matrix function. The other inputs are an integer array containing the degree of each point, the desired distance step size between frames, and the dimension up to which to compute the skeleton. Redundant points will appear as seperate vertices of the complex. Setting k to |S| will compute the entire VR-Complex for each frame."
                          "# @param Matrix D the \"distance matrix\" of the point set (can be upper triangular)"
                          "# @param Array<Int> deg the degrees of input points"
                          "# @param Float step_size"
                          "# @param Int k dimension of the resulting filtration"
                          "# @tparam Coeff desired coefficient type of the filtration"
                          "# @return Filtration<SparseMatrix<Coeff, NonSymmetric> >",
                          "vietoris_rips_filtration<Coeff>($$$$)");
} }
