/* Copyright (c) 1997-2020
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
#include "polymake/Graph.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/linalg.h"
#include "polymake/hash_set"
#include "polymake/hash_map"
#include "polymake/list"

namespace polymake { namespace polytope {

const Vector<Rational> norm(const Vector<Rational>& v)
{
  Int i = 1;
  while (v[i]==0) ++i;
  return Vector<Rational>(v/v[i]);
}

const bool operator== (const Array<Int>& a,const Array<Int>& b)
{ 
  const Int s = a.size();
  if (s != b.size()) return false;
  for (Int i = 0; i < s; ++i)
    if (a[i] != b[i]) return false;
  return true;
}

const bool operator!= (const Array<Int>& a, const Array<Int>& b)
{
  return !(a==b);
}


void reverse_search_graph(BigObject p, const Vector<Rational>& min_vertex, OptionSet options)
{
  const Matrix<Rational> facets=p.give("FACETS|INEQUALITIES");
  const Int dim = facets.cols()-1;
  const Int n_facets = facets.rows();
  const Matrix<Rational> equations=p.lookup("AFFINE_HULL");
  const Int n = n_facets+equations.rows(); //size of matrix A

  const Matrix<Rational> fe=facets/equations;
  
  //we transform to Ax<=b, where the last rows of A are set to equality
  const Matrix<Rational> A= -fe.minor(All, range_from(1));
  const Vector<Rational> b= fe.col(0);
  
  //used to construct the graph at the end
  hash_map<Vector<Rational>, Int> v_map;  
  Int n_vertices = 0;
  Int n_rays = 0;
  
  //the output
  hash_set<Vector<Rational> > rays;
  ListMatrix<Vector <Rational> > vertices;


  typedef std::pair<Vector <Rational>, Vector<Rational> > Edge;
  std::list<Edge> edges;



  //select initial basis !

  Array<Int> start_B(dim);
  Int number = 0, index = 0;
  for (auto i=entire(rows(facets)); !i.at_end();++i,++number)
    if (min_vertex*(*i)==0)  {
      if (index==dim) throw std::runtime_error("Inequalities not in general position.");
      start_B[index++]=number;
    }
  for (Int i = n_facets; i < n; ++i) start_B[index++]=i;
  //the last rows of A are equations, so they has to be in each basis
  
  
  Vector<Rational> objective;
  if (options.exists("objective")) {
     options["objective"] >> objective;
     objective=objective.slice(range_from(1));
  } else {
     // if no objective function is given we artificially construct one that minimizes min_vertex
     objective = -T(A.minor(start_B, All)) * ones_vector<Rational>(dim);
  }

  Array<Int> B=start_B; //Basis has to be set to the initial basis
  //reduced cost for the simplex (for maximizing objective)
  Vector<Rational> costs=lin_solve(T(A.minor(B, All)), objective);
  Vector<Rational> vertex=lin_solve(A.minor(B, All), b.slice(B));
  vertices/=(1|vertex);
  v_map[vertex]=n_vertices++;  

  Int j = 0;
  while (j < dim || B != start_B) {
    //test for all non-bases variables if we can add them
    while (j<dim) {
      if (costs[j]<0) {
        //compute the reduced cost for the minimizing simplex
        Vector<Rational> dir=-lin_solve(A.minor(B,All),unit_vector<Rational>(dim,j));
        Vector<Rational> Ad=A*dir;
        Vector<Rational> Av=A*vertex;
        bool ray=true;
        Rational lambda;
        Int in = 0;
        for (Int k = 0; k < n_facets; ++k)
           if (Ad[k]>0) {
              if (ray) {
                 ray=false;
                 lambda=(b[k]-Av[k])/Ad[k];
                 in=k;
              } else {
                 const Rational lambda_neu=(b[k]-Av[k])/Ad[k];
                 if (lambda_neu<lambda) {
                    in=k;
                    lambda=lambda_neu;
                 }
              }
           }

        if (!ray) {    
          if (lambda==0) throw std::runtime_error("Inequalities not in general position.");
          //compute the vertex
          const Int old = B[j];
          B[j] = in;
          const Vector<Rational> new_costs = lin_solve(T(A.minor(B,All)),objective);
          const Vector<Rational> new_vertex = vertex+lambda*dir;
          edges.push_back(Edge(vertex,new_vertex));

          Int i = 0;
          while (new_costs[i] <= 0) ++i;
          
          //only if the minimizing simplex would come from the new vertex, we go to it.
          if (in==B[i]) {
            costs=new_costs;
            vertex=new_vertex;
            vertices/=(1|vertex);
            v_map[vertex]=n_vertices++;
            j=0;
          }
          else {
            B[j]=old;
            ++j;
          }
        }
        else {
          //ray
          if ((rays.insert(norm(0|dir))).second) ++n_rays;
          ++j;
          //end ray
        }
      }
      else ++j;
    }
    if (B==start_B) break; //only needed in the case where there is only one (real) vertex
    Int i = 0;
    //to go back, we have to make a minimizing simplex step
    while (costs[i]<=0) ++i;
    Vector<Rational> dir=-lin_solve(A.minor(B,All),unit_vector<Rational>(dim,i));
    Vector<Rational> Ad=A*dir;
    Vector<Rational> Av=A*vertex;
    Rational lambda;
    bool ray=true;
    Int in = 0;
    for (Int k = 0; k < n_facets; ++k)
       if (Ad[k] > 0) {
          if (ray) {
             ray = false;
             lambda = (b[k]-Av[k])/Ad[k];
             in = k;
          } else {
             const Rational lambda_neu = (b[k]-Av[k])/Ad[k];
             if (lambda_neu<lambda) {
                in = k;
                lambda = lambda_neu;
             }
          }
       }

    B[i] = in;
    j = i;

    costs = lin_solve(T(A.minor(B,All)),objective);
    vertex += lambda*dir;

    ++j;
  }

  // create the graph from the edges
  // it should contain gaps at the rays' positions
  Graph<Directed> graph(sequence(0,n_vertices), n_vertices+n_rays);
  for (auto i=entire(edges); !i.at_end(); ++i)
    graph.edge(v_map[i->first],v_map[i->second]);

  if (options.exists("objective")) {
     BigObject LP=p.add("LP"); 
     LP.take("DIRECTED_BOUNDED_GRAPH.ADJACENCY") << graph;
     LP.take("MINIMAL_VERTEX") << min_vertex;
     LP.take("MINIMAL_FACE") << scalar2set(0);
     LP.take("LINEAR_OBJECTIVE") << (1|objective);
  }

  p.take("BOUNDED_COMPLEX.GRAPH.ADJACENCY") << Graph<>(graph);

  if (rays.empty()) {
    p.take("BOUNDED")<<true;
    p.take("VERTICES")<<vertices;
    const Set<Int> e;
    p.take("FAR_FACE")<<e;
  }
  else {
    p.take("BOUNDED")<<false;
    // TODO: replace with variadic constructor similar to Array
    const ListMatrix<Vector<Rational>> v(rays.size()+vertices.rows(), dim+1, concatenate(rows(vertices),rays).begin());
    p.take("VERTICES")<<v;
    const Set<Int> ff = sequence(n_vertices,n_rays);
    p.take("FAR_FACE")<<ff;
  }
Matrix<Rational> empty_lin_space(0,facets.cols());
p.take("LINEALITY_SPACE") << empty_lin_space;
}

Function4perl(&reverse_search_graph,"reverse_search_graph(Polytope<Rational>,$,{ objective => undef })"); 

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
