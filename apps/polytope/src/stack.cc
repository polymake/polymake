/* Copyright (c) 1997-2015
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
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/PowerSet.h"
#include "polymake/Graph.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/polytope/bisector.h"

namespace polymake { namespace polytope {
namespace {

template <typename VectorTop>
void nearest_vertex(const GenericVector<VectorTop, Rational>& F, const Vector<Rational>& F_normal,
                    const Vector<Rational>& Fb, Rational& min_ratio)
{
   Rational r=F*F_normal;
   if (r>0) {
      r=F*Fb / r;
      if (r<min_ratio) min_ratio=r;
   }
}

template <typename SetTop>
Vector<Rational>
compute_new_vertex(const Matrix<Rational>& Facets, const Matrix<Rational>& Vertices,
                   const Vector<Rational>& Vb, const IncidenceMatrix<>& VIF,
                   const Graph<>& DG, const GenericSet<SetTop>& stack_facets, int sf, const Rational& lift_factor)
{
   const Vector<Rational> Fb=average(rows(Vertices.minor(VIF[sf],All)));
   Vector<Rational> F_normal=Facets[sf];
   F_normal[0] = -(F_normal.slice(1)*Fb.slice(1));           // facet normal thru facet barycenter
   Rational min_ratio=(Vb-Fb)*F_normal/sqr(F_normal);        // furthest position: mirror opposite of VERTEX_BARYCENTER

   for (Entire<Graph<>::adjacent_node_list>::const_iterator nb=entire(DG.adjacent_nodes(sf)); !nb.at_end();  ++nb) {
      if (stack_facets.top().contains(*nb))
         nearest_vertex(bisector(Facets[sf], Facets[*nb], Vertices[ (VIF[sf] * VIF[*nb]).front() ]),
                        F_normal, Fb, min_ratio);
      else
         nearest_vertex(Facets[*nb], F_normal, Fb, min_ratio);
   }
   return Fb-min_ratio*lift_factor*F_normal;
}

} // end unnamed namespace

template <typename SetTop>
perl::Object stack(perl::Object p_in, const GenericSet<SetTop>& stack_facets, perl::OptionSet options)
{
   const bool bounded = p_in.give("BOUNDED");
   if (!bounded)
      throw std::runtime_error("polytope must be bounded");
     
   Rational lift_factor(1,2);
   if (options.exists("lift")) {
      if (options.exists("no_coordinates")) 
         throw std::runtime_error("stack: cannot specify lift and no_coordinates options simultaneously");

      options["lift"] >> lift_factor;
      if (lift_factor<=0 || lift_factor>=1)
         throw std::runtime_error("lift factor must be between 0 and 1");
   }
   const bool relabel=options["relabel"],
      noc=options["no_coordinates"];
   const int dim=p_in.give("COMBINATORIAL_DIM");
   if (dim<=2)
      throw std::runtime_error("dimension too low to distinguish between simpliciality and cubicality");

   const bool simplicial=p_in.give("SIMPLICIAL"),
      cubical=p_in.give("CUBICAL");
   if (!simplicial && !cubical)
      throw std::runtime_error("polytope neither simplicial nor cubical");

   const IncidenceMatrix<> VIF=p_in.give("VERTICES_IN_FACETS");
   const int n_vertices=VIF.cols(), n_facets=VIF.rows();

   if (stack_facets.top().empty())
      throw std::runtime_error("stack: no facets to stack specified");
   if (stack_facets.top().front() < 0 || stack_facets.top().back() >= n_facets)
      throw std::runtime_error("facet numbers out of range");

   perl::Object p_out("Polytope<Rational>");
   if (pm::identical<SetTop, Set<int> >::value)
      p_out.set_description() << p_in.name() << " with facets " << stack_facets << " stacked" << endl;

   const int n_stack_facets=stack_facets.top().size();

   const int n_vertices_out= n_vertices + n_stack_facets * (simplicial ?     1 : 1 << dim-1),
      n_facets_out  = n_facets   + n_stack_facets * (simplicial ? dim-1 : 2*(dim-1));
   p_out.take("COMBINATORIAL_DIM") << dim;
   p_out.take("N_VERTICES") << n_vertices_out;

   IncidenceMatrix<> VIF_out(n_facets_out, n_vertices_out);
   // copy untouched facets
   Rows< IncidenceMatrix<> >::iterator new_facet=rows(VIF_out).begin();
   new_facet=copy(entire(rows(VIF.minor(~stack_facets,All))), new_facet);

   std::vector<std::string> labels, facet_labels;
   if (relabel) {
      labels.resize(n_vertices_out);  facet_labels.resize(n_facets);
      read_labels(p_in, "VERTEX_LABELS", labels);
      read_labels(p_in, "FACET_LABELS", facet_labels);
   }

   Matrix<Rational> Vertices, Facets, Vertices_out;
   Vector<Rational> Vb;
   if (!noc) {
      p_in.give("VERTICES") >> Vertices;
      p_in.give("FACETS") >> Facets;
      p_in.give("VERTEX_BARYCENTER") >> Vb;
      Vertices_out=Vertices / zero_matrix<Rational>(n_vertices_out-n_vertices, Vertices.cols());
   }

   if (simplicial) {
      int nv=n_vertices;     // new vertex
      for (typename Entire<SetTop>::const_iterator sf=entire(stack_facets.top());  !sf.at_end();  ++sf, ++nv) {
         // new facet = old facet - one of its vertices + new vertex
         for (Subsets_less_1<const IncidenceMatrix<>::const_row_type&>::const_iterator
                 ridges=entire(all_subsets_less_1(VIF[*sf]));  !ridges.at_end();  ++ridges, ++new_facet) {
            *new_facet=*ridges;
            new_facet->push_back(nv);
         }
      }

      if (!noc) {
         const Graph<> DG=p_in.give("DUAL_GRAPH.ADJACENCY");
         Rows< Matrix<Rational> >::iterator nv=rows(Vertices_out).begin()+n_vertices;
         for (typename Entire<SetTop>::const_iterator sf=entire(stack_facets.top());  !sf.at_end();  ++sf, ++nv) {
            *nv=compute_new_vertex(Facets,Vertices,Vb,VIF,DG,stack_facets,*sf,lift_factor);
         }
      }

      if (relabel) {
         std::vector<std::string>::iterator nl=labels.begin()+n_vertices;
         for (typename Entire<SetTop>::const_iterator sf=entire(stack_facets.top());  !sf.at_end();  ++sf, ++nl)
            *nl = "f(" + facet_labels[*sf] + ')';
      }

   } else {
      // cubical
      const Graph<> DG=p_in.give("DUAL_GRAPH.ADJACENCY");

      int first_new_vertex=n_vertices;
      const int vertices_per_facet=1<<dim-1;
      std::vector<int> new_neighbors(n_vertices);

      for (typename Entire<SetTop>::const_iterator sf=entire(stack_facets.top());  !sf.at_end();  ++sf) {
         *new_facet=sequence(first_new_vertex, vertices_per_facet);
         ++new_facet;
         copy(entire(sequence(first_new_vertex, vertices_per_facet)), select(new_neighbors, VIF[*sf]).begin());

         for (Graph<>::adjacent_node_list::const_iterator nb=DG.adjacent_nodes(*sf).begin();
              !nb.at_end();  ++nb, ++new_facet) {
            const Set<int> ridge=VIF[*sf] * VIF[*nb];
            *new_facet = ridge;
            *new_facet += assure_ordered(select(new_neighbors,ridge));
         }

         if (!noc) {
            const Vector<Rational> Apex=compute_new_vertex(Facets,Vertices,Vb,VIF,DG,stack_facets,*sf,lift_factor);
            for (IncidenceMatrix<>::const_row_type::const_iterator nb=VIF[*sf].begin(); !nb.at_end();  ++nb)
               Vertices_out[new_neighbors[*nb]] = (Apex + Vertices[*nb])/2;
         }

         if (relabel) {
            for (IncidenceMatrix<>::const_row_type::const_iterator v=VIF[*sf].begin(); !v.at_end();  ++v)
               labels[new_neighbors[*v]] = "f(" + facet_labels[*sf] + ")-" + labels[*v];
         }
         first_new_vertex += vertices_per_facet;
      }
   }

   p_out.take("VERTICES_IN_FACETS") << VIF_out;
   p_out.take("SIMPLICIAL") << simplicial;
   p_out.take("CUBICAL") << cubical;

   if (!noc) {
      p_out.take("VERTICES") << Vertices_out;
      const Matrix<Rational> empty;
      p_out.take("LINEALITY_SPACE") << empty;
   }
   if (relabel)
      p_out.take("VERTEX_LABELS") << labels;

   return p_out;
}

perl::Object stack(perl::Object p_in, const pm::all_selector&, perl::OptionSet options)
{
   const int n_facets=p_in.give("N_FACETS");
   perl::Object p_out=stack(p_in, sequence(0,n_facets), options);
   p_out.set_description() << p_in.name() << " with all facets stacked" << endl;
   return p_out;
}

perl::Object stack(perl::Object p_in, int facet, perl::OptionSet options)
{
   perl::Object p_out=stack(p_in, scalar2set(facet), options);
   p_out.set_description() << p_in.name() << " with facet " << facet << " stacked" << endl;
   return p_out;
}

perl::Object stack(perl::Object p_in, const Array<int>& facets, perl::OptionSet options)
{
   Set<int> stack_facets;
   for (Entire< Array<int> >::const_iterator fi = entire(facets); !fi.at_end(); ++fi)
      stack_facets += *fi;

   if (stack_facets.size() != facets.size())
      throw std::runtime_error("stack: repeating facet numbers in the list");

   return stack(p_in, stack_facets, options);
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Stack a (simplicial or cubical) polytope over one or more of its facets."
                          "# "
                          "# For each facet of the polytope //P// specified in //stack_facets//, the barycenter of its vertices"
                          "# is lifted along the normal vector of the facet."
                          "# In the simplicial case, this point is directly added to the vertex set, thus building a pyramid over"
                          "# the facet. In the cubical case, this pyramid is truncated by a hyperplane parallel to the original facet"
                          "# at its half height. This way, the property of being simplicial or cubical is preserved in both cases."
                          "# "
                          "# The option //lift// controls the exact coordinates of the new vertices."
                          "# It should be a rational number between 0 and 1, which expresses the ratio of the distance between the"
                          "# new vertex and the stacked facet, to the maximal possible distance. When //lift//=0, the new vertex would lie"
                          "# on the original facet. //lift//=1 corresponds to the opposite extremal case, where the new vertex"
                          "# hit the hyperplane of some neighbor facet. As an additional restriction, the new vertex can't"
                          "# lie further from the facet as the vertex barycenter of the whole polytope."
                          "# Alternatively, the option //noc// (no coordinates) can be specified to produce a"
                          "# pure combinatorial description of the resulting polytope."
                          "# @param Polytope P"
                          "# @param Set<Int> stack_facets the facets to be stacked;"
                          "#   A single facet to be stacked is specified by its number."
                          "#   Several facets can be passed in a Set or in an anonymous array of indices: [n1,n2,...]"
                          "#   Special keyword __All__ means that all factes are to be stacked."
                          "# @option Rational lift controls the exact coordinates of the new vertices;"
                          "#   rational number between 0 and 1; default value: 1/2"
                          "# @option Bool no_coordinates  produces a pure combinatorial description (in contrast to //lift//)"
                          "# @option Bool relabel creates an additional section [[VERTEX_LABELS]];"
                          "#   New vertices get labels 'f(FACET_LABEL)' in the simplicial case,"
                          "#   and 'f(FACET_LABEL)-NEIGHBOR_VERTEX_LABEL' in the cubical case."
                          "# @return Polytope",
                          "stack(Polytope * {lift=>undef, no_coordinates=>undef, relabel=>undef})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
