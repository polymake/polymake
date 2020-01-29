#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/Graph.h"
#include "polymake/Rational.h"
#include <cmath>

namespace polymake { namespace graph {

// the matrix entry (i,j) contains the distance between points i and j (can also be lower triangle matrix.)
BigObject neighborhood_graph(const Matrix<Rational>& D, const Rational& delta)
{
  Int n = D.rows();
  Graph<Undirected> G(n);
  for (Int i = 0; i < n; ++i) {
    for (Int j = i+1; j < n; j++){
      if (D(i, j) < delta)
        G.edge(i, j);
    }
  }
  BigObject GG("Graph");
  GG.take("ADJACENCY") << G;
  GG.set_description() << "Neighborhood graph of the input point set. Two vertices are adjacent if the distance of the corresponding points is less than " << delta << "." << endl;
  return GG;
}

UserFunction4perl("# @category Producing a graph"
                  "# Constructs the __neighborhood graph__ of a point set //S// given a parameter //delta//. The set is passed as its so-called \"distance matrix\", whose (i,j)-entry is the distance between point i and j. This matrix can e.g. be computed using the distance_matrix function. Two vertices will be adjacent if the distance of the corresponding points is less than //delta//."
                  "# @param Matrix<Rational> D input point cloud distance matrix (can be upper triangular)"
                  "# @param Rational delta the maximal distance of neighbored vertices"
                  "# @return Graph"
                  "# @example The following prints the neighborhood graph of a distance matrix with a limit of 3.3, producing the graph of a square:"
                  "# > $D = new Matrix<Rational>([[0,17/10,21/10,42/10],[0,0,79/10,31/10],[0,0,0,6/10],[0,0,0,0]]);"
                  "# > print neighborhood_graph($D,3.3)->ADJACENCY;"
                  "# | {1 2}"
                  "# | {0 3}"
                  "# | {0 3}"
                  "# | {1 2}",
                  &neighborhood_graph, "neighborhood_graph($$)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
