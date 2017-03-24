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
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/linalg.h"
#include "polymake/fan/tight_span.h"
#include "polymake/graph/lattice_builder.h"

namespace polymake { namespace tropical {

   using namespace graph;
   using namespace graph::lattice;
   using namespace fan;

   template <typename Addition>
      perl::Object linear_space(perl::Object valuated_matroid) {
         const perl::Object polytope = valuated_matroid.give("POLYTOPE");
         const Matrix<Rational> &vertices = polytope.give("VERTICES");
         const auto no_front_set = sequence(1,vertices.cols()-1);
         auto& vertices_no_front = vertices.minor(All,no_front_set);
         const int n = valuated_matroid.give("N_ELEMENTS");
         int n_facets = valuated_matroid.give("N_BASES");
         const Vector<TropicalNumber<Addition> > &valuation = valuated_matroid.give("VALUATION_ON_BASES");
         const Vector<Rational> rational_valuation(valuation);
         const Array<Set<int> >& subdivision = valuated_matroid.give("SUBDIVISION");
         const Array< Array< Set<int> > >& split_flacets = valuated_matroid.give("SPLIT_FLACETS");
         const int polytope_dim = polytope.call_method("DIM");
         ListMatrix<Vector<Rational> > new_vertices;

         //Check loopfreeness
         const int n_matroid_loops = valuated_matroid.give("N_LOOPS");
         if(n_matroid_loops > 0) {
            perl::Object empty_cycle(perl::ObjectType::construct<Addition>("Cycle"));
            empty_cycle.take("PROJECTIVE_VERTICES") << Matrix<Rational>(0,n+1);
				empty_cycle.take("MAXIMAL_POLYTOPES") << Array<Set<int> >();
				empty_cycle.take("PROJECTIVE_AMBIENT_DIM") << n-1;
				empty_cycle.take("WEIGHTS") << Vector<Integer>();
            return empty_cycle;
         }

         //excluded faces (those with loops):
         Array<Set<int> > including_bases(n);
         std::list<Set<int> > non_including_bases(n);
         const Set<int> total_list = sequence(0,n_facets);
         auto non_bases_it = entire(non_including_bases);
         auto bases_it = entire(including_bases);
         for(auto vcol = entire(cols(vertices_no_front));
               !vcol.at_end(); ++vcol, ++bases_it, ++non_bases_it) {
            (*bases_it) = support(*vcol);
            (*non_bases_it) = total_list - (*bases_it);
         }

         //Vertices of the tight span (for each max. cell in the subdivision):
         const auto& equations = ( - rational_valuation ) | vertices_no_front;
         for(const auto& cell : subdivision) {
            auto nspace = null_space( equations.minor(cell,All));
            auto ncol = entire(nspace.col(0));
            for(auto nrows = entire(rows(nspace)); !nrows.at_end(); ++nrows, ++ncol) {
               if(! (*ncol).is_zero() ) {
                  new_vertices /= ( (*nrows) / (*ncol) );
                  break;
               }
            }
         }

         //stack the 'loop-free' facets and add a ray for each such facet
         RestrictedIncidenceMatrix<> flacets(subdivision.size(), rowwise(), entire(subdivision));
         int i = 1;
         for(auto incl_base  = entire(including_bases); !incl_base.at_end(); ++incl_base, ++i) {
            const int n_loops = attach_selector( cols(vertices_no_front.minor(*incl_base,All)), operations::is_zero()).size();
            if(n_loops > 0 || rank( vertices.minor(*incl_base,All)) != polytope_dim) continue;

            new_vertices /= (Addition::orientation() *unit_vector<Rational>(n+1,i));
            flacets /= ( (*incl_base) + n_facets);
            n_facets++;
         }
         for(auto rk = ensure( split_flacets, (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin();
               !rk.at_end(); ++rk) {
            for(const auto flacet : *rk) {
               Vector<Rational> v(n);
               v.slice(flacet).fill(1);
               v = (- rk.index()) | v;
               auto col_sum_supp = total_list - support( vertices * v );
               if( rank( vertices.minor (col_sum_supp,All)) != polytope_dim) continue;

               v[0] = 0;
               flacets /= col_sum_supp + n_facets;
               new_vertices /= (Addition::orientation() * v);
               n_facets++;
            }
         }

         //Build Hasse diagram
         const IncidenceMatrix<> flacet_inc(std::move(flacets));
         NoBoundaryCut cut( non_including_bases, flacet_inc);
         BasicClosureOperator<> cop(flacet_inc.rows(), T(flacet_inc));
         BasicDecorator<> dec(0, scalar2set(-1));
         Lattice<BasicDecoration> hasse_diagram = lattice_builder::compute_lattice_from_closure<BasicDecoration>(
               cop, cut, dec, 1, lattice_builder::Primal());
         int n_max_polys = hasse_diagram.in_adjacent_nodes(hasse_diagram.top_node()).size();


         //Build lineality space
         const Array<Set<int> >& ccomp = valuated_matroid.give("CONNECTED_COMPONENTS");
         Matrix<Rational> lin_space(ccomp.size()-1, n+1);

         auto lin_no_front = sequence(1, lin_space.cols()-1);
         auto lin_space_no_front = lin_space.minor(All,lin_no_front);
         auto lin_rows = entire(rows(lin_space_no_front));
         auto cc = entire(ccomp);
         if(!cc.at_end()) {
            ++cc; //We take all connected components but the first.
            for(;!cc.at_end(); ++cc, ++lin_rows) {
               ((*lin_rows).slice(*cc)).fill(1);
            }
         }

         perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
            result.take("PROJECTIVE_VERTICES") << new_vertices;
            result.take("HASSE_DIAGRAM") << hasse_diagram.makeObject();
            result.take("LINEALITY_SPACE") << lin_space;
            result.take("WEIGHTS") << ones_vector<Integer>(n_max_polys);

         return result;
      }

      UserFunctionTemplate4perl("# @category Tropical linear spaces"
                                "# This computes the tropical linear space (with the coarsest structure) associated to a valuated matroid."
                                "# If you have a trivial valuation, it is highly recommended, you use"
                                "# [[matroid_fan]] instead."
                                "# @param matroid::ValuatedMatroid<Addition,Rational> A valuated matroid, whose value group must be the rationals."
                                "# @return Cycle<Addition>",
                                "linear_space<Addition>(matroid::ValuatedMatroid<Addition>)");


}}
