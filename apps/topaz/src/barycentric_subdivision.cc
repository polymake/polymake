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
#include "polymake/topaz/barycentric_subdivision.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/list"
#include <sstream>
#include <vector>

namespace polymake { namespace topaz {

Array<Set<int>> bs(const graph::HasseDiagram& HD)
{
   const bool has_top_node (HD.faces()[HD.top_node()].size() > 0); // it comes from a polytope
   const bool built_dually (HD.built_dually()); // the Hasse diagram has the empty set on top, i.e., in last position
   const int dim (has_top_node ? HD.dim() : HD.dim()-1);

   // each old facet is divided into at least (dim+1)! cells, with equality iff the object is simplicial.
   // since we don't know the size beforehand, we use a std::vector instead of an Array.
   // each facet of the barycentric subdivision is a flag in the input face lattice HD,
   // stored as the set of node indices of the constituent faces in HD
   std::vector< Set<int> > facets; 
   facets.reserve(HD.nodes_of_dim(-1).size() * int(Integer::fac(dim+1)));

   typedef Graph<Directed>::out_edge_list::const_iterator out_edge;
   typedef std::vector<out_edge> stack_type;  // vector is more efficient than list
   stack_type flag;
   flag.reserve(dim+1);

   // start with the "empty set" node - just for convenience
   flag.push_back(HD.out_edges(HD.bottom_node()).begin());

   int d=0; // dimension of the face accumulated in the flag so far
   do {
      // complete the facet
      while (d<dim) {
         const int n = flag.back().to_node();
         flag.push_back(HD.out_edges(n).begin()); // the index of the next face in the flag
         ++d;
      }
        
      // copy the facet
      Set<int> facet;
      for (auto s=entire(flag);  !s.at_end();  ++s)
         facet += s->to_node() - 1 + (built_dually && has_top_node); // disregard bottom node to start at index 0, unless two things happen        
      facets.push_back(facet);

      // depth-first search to the next facet
      do {
         if (!(++flag.back()).at_end()) break;
         flag.pop_back();
      } while (--d>=0);
   } while (d>=0);

   return Array<Set<int>>(facets);
}

Array<Set<int>> bs(const Array<Set<int> >& old_bs) 
{
   return bs(pure_hasse_diagram(old_bs));
}

template <typename T>
T iterated_bs(int i, const T& old_bs)
{
   return i<=0 ? old_bs : iterated_bs(i-1, bs(old_bs));
}


  
Array<std::string> bs_labels(const graph::HasseDiagram& HD, const Array<std::string>& old_labels)
{
   const bool has_top_node (HD.faces()[HD.top_node()].size() > 0); // it comes from a polytope
   const bool built_dually (HD.built_dually()); // the Hasse diagram has the empty set on top, i.e., in last position
   Array<std::string> L(HD.nodes() - 1 - (built_dually && !has_top_node));
   graph::HasseDiagram::faces_map_type::const_iterator f=HD.faces().begin(); 
   if (!(*f).size())
      ++f;  // skip the top(bottom) node corresponding to the empty set
   std::ostringstream label;
   const bool convert_old_labels(old_labels.size() > 0);
   for (auto l=entire(L); !l.at_end(); ++l, ++f) {
      if (!convert_old_labels)
         wrap(label) << *f;
      else {
         wrap(label) << "{";
         bool first(true);
         const Set<int> fset(*f);
         for (auto fsit = entire(fset); !fsit.at_end(); ++fsit) {
            if (first) 
               first = false;
            else 
               wrap(label) << " ";
            wrap(label) << old_labels[*fsit];
         }
         wrap(label) << "}";
      } 
      *l=label.str();
      label.str("");
   }
   return L;
}
  
template <typename Scalar>  
Matrix<Scalar> bs_geom_real(const Matrix<Scalar>& old_coord, const graph::HasseDiagram& HD)
{
   const bool has_top_node (HD.faces()[HD.top_node()].size() > 0); // it comes from a polytope
   const bool built_dually (HD.built_dually()); // the Hasse diagram has the empty set on top, i.e., in last position
   const int ambient_dim = old_coord.cols();
   const int n_nodes = HD.nodes() - 1 - (built_dually && !has_top_node); 
   Matrix<Scalar> new_coord(n_nodes, ambient_dim);
  
   graph::HasseDiagram::faces_map_type::const_iterator f=HD.faces().begin(); 
   if (!(*f).size())
      ++f;  // skip the top(bottom) node corresponding to the empty set
   for (typename Entire<Rows<Matrix<Scalar> > >::iterator r=entire(rows(new_coord));  !r.at_end();  ++r, ++f) {
      accumulate_in( entire(select(rows(old_coord), *f)), operations::add(), *r );
      if (f->size()) {
         *r /= f->size();
      } else (*r)[0] = pm::choose_generic_object_traits<Scalar>::one();
   }
   return new_coord;
}

template <typename Scalar>
perl::Object barycentric_subdivision_impl(perl::Object p_in, perl::OptionSet options)
{
   const bool isComplex = p_in.isa("topaz::SimplicialComplex");
   const bool realize = options["geometric_realization"];

   bool isPC = false;
   try {
      isPC = p_in.isa("PointConfiguration");
   } catch (const std::exception &e) {
      // PointConfiguration isn't defined, maybe because we're called from topaz;
      // just leave isPC as false
   }

   perl::ObjectType result_type =
                     realize ? perl::ObjectType::construct<Scalar>("topaz::GeometricSimplicialComplex")
                             : perl::ObjectType("topaz::SimplicialComplex");
   perl::Object p_out(result_type);
   p_out.set_description() << "Barycentric subdivision of " << p_in.description() << endl;

   graph::HasseDiagram HD;
   std::string hasse_section = options["pin_hasse_section"];
   if (isComplex)
      hasse_section = "HASSE_DIAGRAM";
   p_in.give(hasse_section) >> HD;
   
   p_out.take("FACETS") << bs(HD);
   p_out.take("PURE") << 1;
   p_out.take("DIM") << HD.dim() - (isComplex || isPC);

   if (options["relabel"]) {
      Array<std::string> old_labels;
      std::string label_section = options["label_section"];
      if (isComplex)
         label_section = "VERTEX_LABELS";
      p_in.lookup(label_section) >> old_labels;
      p_out.take("VERTEX_LABELS") << bs_labels(HD, old_labels);
   }

   if (realize) {
      std::string coord_section = options["coord_section"];
      if (isComplex)
         coord_section = "COORDINATES";
      Matrix<Scalar> old_coord = p_in.give(coord_section);
      p_out.take("COORDINATES") << bs_geom_real<Scalar>(old_coord, HD);
   }

   return p_out;
}

template <typename Scalar>  
perl::Object iterated_barycentric_subdivision_impl(perl::Object p_in, int k, perl::OptionSet options)
{
   if (k<=0) return p_in;
   const perl::Object subd1 = barycentric_subdivision_impl<Scalar>(p_in, options);
   perl::Object subd = iterated_barycentric_subdivision_impl<Scalar>(subd1, k-1, options);

   const char num[][5] = { "1st ", "2nd ", "3rd " };
   std::ostringstream desc;
   if (k <= 3) desc << num[k-1];
   else desc << k << "th ";
   desc << "barycentric subdivision of " << p_in.description();
   subd.set_description() << desc.str();
   return subd;
}

FunctionTemplate4perl("barycentric_subdivision_impl<Scalar=Rational>($ { relabel => 1, geometric_realization => 0 })");

FunctionTemplate4perl("iterated_barycentric_subdivision_impl<Scalar=Rational>($ $ { relabel => 1, geometric_realization => 0 })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
