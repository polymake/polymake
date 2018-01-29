/* Copyright (c) 1997-2018
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

inline const Vector<Rational> norm(const Vector<Rational> &v) {
  int i=1;
  while (v[i]==0) ++i;
  return Vector<Rational>(v/v[i]);
}

inline const bool operator== (const Array<int> &a,const Array<int> &b)
{ 
  const int s=a.size();
  if (s!=b.size()) return false;
  for (int i=0;i<s; ++i)
    if (a[i]!=b[i]) return false;
  return true;
}

inline const bool operator!= (const Array<int> &a,const Array<int> &b)
{
  return !(a==b);
}


void reverse_search_graph(perl::Object p, const Vector<Rational>& min_vertex, perl::OptionSet options)
{
  const Matrix<Rational> facets=p.give("FACETS|INEQUALITIES");
  const int dim=facets.cols()-1;
  const int n_facets=facets.rows();
  const Matrix<Rational> equations=p.lookup("AFFINE_HULL");
  const int n=n_facets+equations.rows(); //size of matrix A

  const Matrix<Rational> fe=facets/equations;
  
  //we transform to Ax<=b, where the last rows of A are set to equality
  const Matrix<Rational> A= -fe.minor(All, ~scalar2set(0));
  const Vector<Rational> b= fe.col(0);
  
  //used to construct the graph at the end
  hash_map<Vector<Rational>, int> v_map;  
  int n_vertices=0;
  int n_rays=0;
  
  //the output
  hash_set<Vector<Rational> > rays;
  ListMatrix<Vector <Rational> > vertices;


  typedef std::pair<Vector <Rational>, Vector<Rational> > Edge;
  std::list<Edge> edges;



  //select initial basis !

  Array<int> start_B(dim);
  int number=0,index=0;
  for (Entire<Rows <Matrix <Rational> > >::const_iterator i=entire(rows(facets)); !i.at_end();++i,++number)
    if (min_vertex*(*i)==0)  {
      if (index==dim) throw std::runtime_error("Inequalities not in general position.");
      start_B[index++]=number;
    }
  for (int i=n_facets;i<n;++i) start_B[index++]=i;
  //the last rows of A are equations, so they has to be in each basis
  
  
  Vector<Rational> objective;
  if (options.exists("objective"))
    {
      options["objective"] >> objective;
      objective=objective.slice(1);
    }
  else //if no objective function is given we artificially construct one that minimizes min_vertex
    objective=-T(A.minor(start_B,All))*ones_vector<Rational>(dim);


  Array<int> B=start_B; //Basis has to be set to the initial basis
  //reduced cost for the simplex (for maximizing objective)
  Vector<Rational> costs=lin_solve(T(A.minor(B,All)),objective);
  Vector<Rational> vertex=lin_solve(A.minor(B,All),b.slice(B));
  vertices/=(1|vertex);
  v_map[vertex]=n_vertices++;  

  int j=0;
  while (j<dim || B!=start_B) {
    //test for all non-bases variables if we can add them
    while (j<dim) {
      if (costs[j]<0) {
        //compute the reduced cost for the minimizing simplex
        Vector<Rational> dir=-lin_solve(A.minor(B,All),unit_vector<Rational>(dim,j));
        Vector<Rational> Ad=A*dir;
        Vector<Rational> Av=A*vertex;
        bool ray=true;
        Rational lambda;
        int in=0;
        for (int k=0;k<n_facets;++k)
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
          const int old=B[j];
          B[j]=in;
          const Vector<Rational> new_costs=lin_solve(T(A.minor(B,All)),objective);
          const Vector<Rational> new_vertex=vertex+lambda*dir;
          edges.push_back(Edge(vertex,new_vertex));

          int i=0;
          while (new_costs[i]<=0) ++i;
          
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
    int i=0;
    //to go back, we have to make a minimizing simplex step
    while (costs[i]<=0) ++i;
    Vector<Rational> dir=-lin_solve(A.minor(B,All),unit_vector<Rational>(dim,i));
    Vector<Rational> Ad=A*dir;
    Vector<Rational> Av=A*vertex;
    Rational lambda;
    bool ray=true;
    int in=0;
    for (int k=0;k<n_facets;++k)
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

    B[i]=in;
    j=i;


    costs=lin_solve(T(A.minor(B,All)),objective);
    vertex+=lambda*dir;

    ++j;
  }

  // create the graph from the edges
  // it should contain gaps at the rays' positions
  Graph<Directed> graph(sequence(0,n_vertices), n_vertices+n_rays);
  for (Entire< std::list<Edge> >::const_iterator i=entire(edges); !i.at_end(); ++i)
    graph.edge(v_map[i->first],v_map[i->second]);

  if (options.exists("objective")) {
     perl::Object LP=p.add("LP"); 
     LP.take("DIRECTED_BOUNDED_GRAPH.ADJACENCY") << graph;
     LP.take("MINIMAL_VERTEX") << min_vertex;
     LP.take("MINIMAL_FACE") << scalar2set(0);
     LP.take("LINEAR_OBJECTIVE") << (1|objective);
  }

  p.take("BOUNDED_COMPLEX.GRAPH.ADJACENCY") << Graph<>(graph);

  if (rays.empty()) {
    p.take("BOUNDED")<<true;
    p.take("VERTICES")<<vertices;
    const Set<int> e;
    p.take("FAR_FACE")<<e;
  }
  else {
    p.take("BOUNDED")<<false;
    const ListMatrix<Vector <Rational> > v(rays.size()+vertices.rows(), dim+1, entire(concatenate(rows(vertices),rays)));
    p.take("VERTICES")<<v;
    const Set<int> ff=sequence(n_vertices,n_rays);
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
