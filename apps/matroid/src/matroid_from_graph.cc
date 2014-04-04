/* Copyright (c) 1997-2014
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
#include "polymake/PowerSet.h"
#include "polymake/linalg.h"
#include "polymake/list"

namespace polymake { namespace matroid {

perl::Object matroid_from_graph(perl::Object g)
{
  const Graph<> graph=g.give("ADJACENCY");
  const int n_elements=g.give("N_EDGES");
  const int n_nodes=g.give("N_NODES");
  const int n_components=g.give("N_CONNECTED_COMPONENTS");
  const int r=n_nodes-n_components;
  std::list< Set<int> > bases;
  int n_bases=0;

  Array<int> start_nodes(n_elements); //the starting nodes of the edges
  Array<int> end_nodes(n_elements); //the ending nodes of the edges
  Array<std::string> labels(n_elements);
  int l=0;
  for (Entire< Edges<Graph<> > >::const_iterator i=entire(edges(graph)); !i.at_end(); ++i) {
    start_nodes[l]=i.from_node();
    end_nodes[l]=i.to_node();
    std::ostringstream label;
    label<<"{"<<i.from_node()<<" "<<i.to_node()<<"}";
    labels[l++]=label.str();
  }
  
  Array<int> in_component(n_nodes); //tells for each node in which component it is
  Array<int> c_n_nodes(n_components); //tells the number of nodes for each component
  PowerSet<int> components=g.give("CONNECTED_COMPONENTS");
  int comp=0;
  for (Entire<PowerSet<int> >::const_iterator i=entire(components);!i.at_end();++i,++comp) 
    for(Entire<Set<int> >::const_iterator j=entire(*i);!j.at_end();++j,++c_n_nodes[comp])
      in_component[*j]=comp;
      
  //for the special case where there exists isolated nodes
  Array<bool> isol(n_nodes);
  for(int i=0;i<n_nodes;++i)
    if (c_n_nodes[in_component[i]]==1) isol[i]=true;
  
  //test for all subsets of size r if they are spanning
  //and if each connected component has the right number of edges
  for (Entire< Subsets_of_k<const sequence&> >::const_iterator i=entire(all_subsets_of_k(sequence(0,n_elements),r)); !i.at_end(); ++i) {
    Array<bool> a(isol);//a[i] <=> node i is reached
    Array<int> e(n_components,1);
    bool is_basis=true;
    const Set<int> basis=*i;
    for (Entire<Set<int> >::const_iterator j=entire(basis); !j.at_end(); ++j) {
      a[start_nodes[*j]]=a[end_nodes[*j]]=true;
      ++e[in_component[start_nodes[*j]]];
    }
    //cerr<<basis<<": "<<endl<<a<<endl<<e<<endl;
    for (int k=0;is_basis&&k<n_components;++k)
      is_basis&=(e[k]==c_n_nodes[k]);
    for (int k=0;is_basis&&k<=r;++k)
      is_basis&=a[k];
    if (is_basis) {
      //we still have to check if basis defines a forest, or, equivalently, that its restriction to each connected
      //component of G is connected
      Graph<> t(n_nodes);
      for (Entire<Set<int> >::const_iterator j=entire(basis); !j.at_end(); ++j)
        t.edge(start_nodes[*j],end_nodes[*j]);
      perl::Object T("Graph<>");
      T.take("ADJACENCY")<<t;
      const int T_n_comp=T.give("N_CONNECTED_COMPONENTS");
      if (T_n_comp==n_components) {
        bases.push_back(basis);
        ++n_bases;
      }
    }
  }

  perl::Object m("Matroid");  
  m.take("BASES") << bases;
  m.take("N_BASES") << n_bases;
  m.take("RANK") << r;
  m.take("N_ELEMENTS") << n_elements;
  m.take("LABELS") << labels;
  m.set_description()<<"Matroid of graph "<<g.name()<<endl;
  return m;
}

UserFunction4perl("# @category Producing from scratch"
                  "# Creates a graphical matroid from a graph //g//."
                  "# @param  graph::Graph g"
                  "# @return Matroid",
                  &matroid_from_graph, "matroid_from_graph(graph::Graph)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
