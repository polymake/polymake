/* Copyright (c) 1997-2016
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

#include "polymake/polytope/minkowski_sum_fukuda.h"
#include "polymake/linalg.h"

/*
  http://www.uni-frankfurt.de/fb/fb12/mathematik/dm/personen/steffens/Dokumente/MV_Computation1.pdf   [1]
  http://www.uni-frankfurt.de/fb/fb12/mathematik/dm/personen/steffens/Dokumente/Diss1.pdf             [2]
*/

namespace polymake { namespace polytope {

namespace {
  // the following typedefs are deferred until c++11
  // typedef Array<Matrix<E> > matrix_list;
  // typedef Array<Vector<E> > vector_list;
  // typedef std::list<Matrix<E> > clist;     // constraint list
  typedef Array<Graph<Undirected> > graph_list;
}

template<typename E>
Matrix<E> list2matrix(const std::vector<Matrix<E> >& v, const int rows, const int cols)
{
    Matrix<E> A(rows,cols);
    int i=0;
    for (typename std::vector<Matrix<E> >::const_iterator it=v.begin(); it!=v.end(); ++it) {
       for (int j=0; j<(*it).rows(); ++j,++i)
          A.row(i)=(*it).row(j);
    }
    return A;
}

template<typename E>
E solve_lp_mixed_volume(const Matrix<E>& M, const Vector<E>& objective)
{
    typedef typename choose_solver<E>::solver Solver;
    Solver solver;
    Matrix<E> eqs(M.cols()-1,M.cols());
    for (int j=0;j<M.cols()-1;++j)
       eqs.row(j)=unit_vector<E>(M.cols(),j+1);
    typename Solver::lp_solution S=solver.solve_lp(eqs, M, objective, 1);
    return S.first;
}

template<typename E>
Matrix<E> construct_A(const int n, const Array<int>& r, const Array<Matrix<E> >& polytopes, const Array<Vector<E> >& lifted_edges)
{
    std::vector<Matrix<E> > c;
    int R=0;
    for (int i=0; i<n; ++i) {
       Matrix<E> B = zero_matrix<E>(lifted_edges[i].size(),n+1);
       B.col(0)=lifted_edges[i];
       B.col(i+1)=ones_vector<E>(r[i]);
       c.push_back((polytopes[i]| B));
       R+=r[i];
    }
    Matrix<E> A=list2matrix(c,R,n+2+n);
    Vector<E> vec =unit_vector<E>(A.cols(),polytopes[0].cols());
    A = (A / vec);
    return T(A.minor(All,sequence(1,A.cols()-1))); // slice ones and transpose
}

template<typename E>
bool lower_envelope_check(Matrix<E>& A, const int n, const int k, const Array<int>& r, const Vector<E>& m)
{
    int R=0;
    assert(r.size()>=k);
    for (int i=0; i<k; ++i)
       R+=r[i];
    const Vector<E> b = ( m | ones_vector<E>(n));
    const Vector<E> obj = unit_vector<E>(R+2,R+1);
    T(A).row(0)=-b;

    const Matrix<E> M(A.minor(sequence(0,n+k+1),~sequence(R+1,A.cols()-R-2)));
    return solve_lp_mixed_volume(M, obj)==0;
}

template<typename E>
E volumen(const int n, const Array<int>& node, const Array<int>& next, const Array<Matrix<E> >& polytopes, const graph_list& graphs)
{
    Matrix<E> A(1,polytopes[0].cols());
    for (int j=0; j<n; ++j) {
       auto it=entire(graphs[j].adjacent_nodes(node[j]));
       for (int i=0; i<next[j]; ++i)  //reset iterator
          ++it;
       A = ( A / (polytopes[j].row(node[j])-polytopes[j].row(*it)) );
    }
    E d = det(A.minor(sequence(1,A.rows()-1),sequence(1,A.cols()-1)));
    if (d==0)
       throw std::runtime_error("mixed_volume: calculation failed, edge matrix is singular.");
    // check (2.9) in [2]. change the  Lift-functions
    return abs(d);
}

template <typename E>
E mixed_volume(const Array<perl::Object>& summands)
{
   E vol = 0;             // mixedVolume
   const int n = summands.size();      // number of (input)polytopes
   Array<Matrix<E>> polytopes(n);      // stores matrices s.t. the i-th entry is a discribtion of P_j by vertices
   Array<Vector<E>> lifted_edges(n);
   graph_list graphs(n);         // stores all graphs from the input polytopes P_j
   Array<int> node(n);
   Array<int> next(n);
   Array<int> r(n);              //stores the number of input Points

   //initialization:
   int j=0;
   Vector<E> Lift(n+1);
   Lift[0]=0;
   for (const perl::Object& s : summands) {
      const Matrix<E> m=s.give("VERTICES");
      polytopes[j]=m;
      r[j]=m.rows();
      for (int k=1; k<=n; ++k)        //LIFT
         Lift[k] = 1 + j*(1 - j*(1 - k*j)); //1 + j - j*j + k*j*j*j;
      lifted_edges[j] = m*Lift;
      const Graph<Undirected> graph=s.give("GRAPH.ADJACENCY");
      graphs[j]=graph;
      ++j;
   }
   if (n != polytopes[0].cols()-1)
      throw std::runtime_error("mixed_volume: dimension and number of input polytopes mismatch");
   Matrix<E> A = construct_A(n, r, polytopes, lifted_edges);
   A = ones_vector<E>(A.rows()) | A;

   // Algo:
   Vector<E> m=zero_vector<E>(n+1);
   Vector<E> temp;
   int count;

   j=0;
   for (int i=0; i<polytopes[j].rows(); ++i) {
      auto it=entire(graphs[j].adjacent_nodes(i));
      for (count=0; count<polytopes[j].rows()-1; ++count) {
         if (*it>i) {
            temp = (polytopes[j].row(i)+polytopes[j].row(*it))/2;
            temp = (temp | (((lifted_edges[j])[i]+(lifted_edges[j])[*it])/2));
            temp = temp.slice(1,temp.size()-1);
            if (lower_envelope_check(A, n, j+1, r, Vector<E>(m+temp))) {
               node[j]=i;
               next[j]=count;
               if (j==n-1) {
                  vol+=volumen(n,node,next,polytopes,graphs);
               } else {
                  ++j;
                  m+=temp;
                  count=polytopes[j].rows();  //jump out of loop
                  i=-1;
               }
            }
         }
         ++it;
         if (j>0 and it.at_end() and i==polytopes[j].rows()-1) {
            --j;
            i = node[j];
            it = entire(graphs[j].adjacent_nodes(i));
            count = next[j];
            for (int k=0; k<count; ++k) //reset iterator
               ++it;
            temp = (polytopes[j].row(i)+polytopes[j].row(*it))/2;
            temp = (temp | (((lifted_edges[j])[i]+(lifted_edges[j])[*it])/2));
            temp = temp.slice(1,temp.size()-1);
            m = m-temp;
            ++it;
         }
         if (it.at_end())
            break;
      }
   }
   return vol;
}


UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Produces the mixed volume of polytopes P<sub>1</sub>,P<sub>2</sub>,...,P<sub>n</sub>."
                          "# @param Polytope<Scalar> P1 first polytope"
                          "# @param Polytope<Scalar> P2 second polytope"
                          "# @param Polytope<Scalar> Pn last polytope"
                          "# @return Scalar mixed volume"
                          "# @example > print mixed_volume(cube(2),simplex(2));"
                          "# | 4",
                          "mixed_volume<E>(Polytope<E> +)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
