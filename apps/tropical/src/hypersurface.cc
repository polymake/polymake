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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"
#include "polymake/tropical/arithmetic.h"
#include "polymake/polytope/compress_incidence.h"

namespace polymake { namespace tropical {

template <typename Addition>
void hypersurface(perl::Object h, bool dehomogenize)
{
  const Matrix<Rational> monoms=h.give("MONOMIALS");
  const Vector<Rational> coefs=h.give("COEFFICIENTS");
  const int d=monoms.cols();
  const int n=monoms.rows();

  if (monoms.rows()!=coefs.size())
    throw std::runtime_error("Coefficient vector has the wrong dimension.");

  // dual to extended Newton polyhedron
  const Matrix<Rational> ineq(Addition::orientation()*(coefs|monoms|same_element_vector<Rational>(-1,n)));
  Matrix<Rational> eq;
  // set first coordinate to zero (to mod out lineality space), if so desired
  if (dehomogenize)
     eq /= unit_vector<Rational>(d+2,1);

  perl::Object dome(perl::ObjectType::construct<Rational>("polytope::Polytope"));
  dome.take("INEQUALITIES") << ineq;
  dome.take("EQUATIONS") << eq;
  dome.take("FEASIBLE") << true;
  dome.take("BOUNDED") << false;

  const Matrix<Rational> points=dome.give("VERTICES");
  const Set<int> far_face=dome.give("FAR_FACE");
  // here we explicitly want to take redundant MONOMIALS into account ...
  const IncidenceMatrix<> vii=dome.give("VERTICES_IN_INEQUALITIES");
  const std::pair< Set<int>, Set<int> > nonfacets_hiddeneqs(polytope::compress_incidence(vii));

  const Set<int> redundant_monomials(nonfacets_hiddeneqs.first);
  // but we also need the irredudant version
  const IncidenceMatrix<> vif(vii.minor(~redundant_monomials,All)); // should be the same as vif=dome.give("VERTICES_IN_FACETS");
  
  // find the far_face_index
  int f=vif.rows()-1; // should be the last one;
  Graph<> dg=dome.give("DUAL_GRAPH.ADJACENCY");
  dg.out_edges(f).clear(); // deletes the 'cells at infinity'

  const int n_cells=dg.edges();
  Array< Set<int> > cells(n_cells);
  int i=0;
  for (Entire< Edges< Graph<> > >::const_iterator e=entire(edges(dg));  !e.at_end();  ++e, ++i)
        cells[i]=vif.row(e.from_node())*vif.row(e.to_node());

  h.take("DOME") << dome;
  h.take("POINTS") << points.minor(All,~range(d+1,d+1)); // get rid of extra column
  h.take("RAYS") << far_face;
  h.take("MAXIMAL_FACES") << cells;
  h.take("REDUNDANT_MONOMIALS") << redundant_monomials;
  h.take("REGIONS") << IncidenceMatrix<>(vif.minor(~range(f,f),All));
}

FunctionTemplate4perl("hypersurface<Addition>(Hypersurface<Addition> ; $=0) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
