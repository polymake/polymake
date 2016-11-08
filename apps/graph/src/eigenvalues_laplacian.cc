
#include "polymake/client.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Graph.h"
#include "polymake/linalg.h"

namespace polymake { 
   namespace graph {
      
      SparseMatrix<int> signed_incidence_matrix(perl::Object p);      

      Matrix<Rational> laplacian(perl::Object p)
      {
         SparseMatrix<Rational> IncidenceM= convert_to<Rational>(signed_incidence_matrix(p));
         return (IncidenceM*T(IncidenceM));
      }
      
      Vector<double> eigenvalues_laplacian(perl::Object p)
      {
         SparseMatrix<double> laplacianM=convert_to<double>(laplacian(p));
         return eigenvalues(laplacianM);
      }

UserFunction4perl("# @category Combinatorics"
                  "# Compute the unsigned vertex-edge incidence matrix of the graph."
                  "# @param Graph G"
                  "# @return SparseMatrix<Rational>"
                  "# @example > $I = laplacian(cycle_graph(4));"
                  "# > print $I;"
                  "# | 2 -1 1 0"
                  "# | -1 2 0 1"
                  "# | 1 0 2 1"
                  "# | 0 -1 1 2",
                  &laplacian, "laplacian(Graph)");
   
UserFunction4perl("# @category Combinatorics"
                  "# Compute the eigenvalues of the discrete laplacian a graph."
                  "# @param Graph G"
                  "# @return Vector<Float>"
                  "# @example > $v = eigenvalues_laplacian(cycle_graph(4));"
                  "# > print $v;"
                  "# | 4 2 2 0",
                  &eigenvalues_laplacian, "eigenvalues_laplacian(Graph)");
   } 
}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
