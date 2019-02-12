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
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"
#include "polymake/common/incidence_tools.h"
#include "polymake/polytope/compress_incidence.h"
#include "polymake/tropical/arithmetic.h"
#include "polymake/tropical/thomog.h"

namespace polymake { namespace tropical {

template <typename Addition>
void hypersurface(perl::Object h)
{
  const Matrix<int> monoms_int = h.give("MONOMIALS");
  Matrix<Rational> monoms(monoms_int); // cast coefficients Integer -> Rational
  const Vector< TropicalNumber<Addition> > coefs=h.give("COEFFICIENTS");
  const int d=monoms.cols();
  const int n=monoms.rows();

  if (monoms.rows()!=coefs.size())
    throw std::runtime_error("Coefficient vector has the wrong dimension.");

  // We have to make all exponents positive, otherwise the below equations produce
  // a wrong result. We multiply the polynomial with a single monomial, which 
  // does not change the hypersurface.
  Vector<Rational> min_degrees(monoms.cols());
  for (int v = 0; v < monoms.cols(); ++v) {
    min_degrees[v] = accumulate(monoms.col(v),operations::min());
    // If the minimal degree is positive, we're good
    min_degrees[v] = std::min(min_degrees[v],Rational(0));
  }
  for (int m = 0; m < monoms.rows(); ++m) {
    monoms.row(m) -= min_degrees; 
  }

  // dual to extended Newton polyhedron
  ListMatrix< Vector<Rational> > ineq;
  const TropicalNumber<Addition> zero=TropicalNumber<Addition>::zero();
  for (int i=0; i<n; ++i) {
    if (coefs[i]==zero)
      ineq /= unit_vector<Rational>(d+1,0);
    else
      ineq /= Addition::orientation()*(Rational(coefs[i])|monoms[i]);
  }

  perl::Object dome("polytope::Polytope<Rational>");
  dome.take("INEQUALITIES") << ineq;
  dome.take("FEASIBLE") << true;
  dome.take("BOUNDED") << false;

  const Matrix<Rational> vertices=dome.give("VERTICES");
  const Matrix<Rational> lineality=dome.give("LINEALITY_SPACE");
  const Set<int> far_vertices=dome.give("FAR_FACE");

  // here we explicitly want to take redundant MONOMIALS into account ...
  const IncidenceMatrix<> vii=dome.give("VERTICES_IN_INEQUALITIES");
  const Set<int> redundant_monomials = polytope::compress_incidence(vii).first;

  // but we also need the irredundant version
  const IncidenceMatrix<> vif = dome.give("VERTICES_IN_FACETS");
  const int f = common::find_row(vif, far_vertices);
  Graph<> dg = dome.give("DUAL_GRAPH.ADJACENCY");
  if (f >= 0)
    dg.out_edges(f).clear(); // deletes the 'cells at infinity'

  // We construct a map that computes for a row index in vif, what it's original row index in vii was
  // (i.e. what the corresponding monomial is)
  Map<int, int> old_index;
  int count = 0;
  Set<int> non_redundant_monomials = sequence(0,vii.rows()) - redundant_monomials;
  for (auto nrm = entire(non_redundant_monomials); !nrm.at_end(); ++nrm) {
    old_index[count] = *nrm;
    ++count;
  }

  const int n_cells=dg.edges();
  Array< Set<int> > cells(n_cells);
  Vector<Integer> weights(n_cells);
  int i=0;
  for (auto e=entire(edges(dg));  !e.at_end();  ++e, ++i) {
    cells[i]=vif.row(e.from_node())*vif.row(e.to_node());
    // Compute the weight of this cell
    Vector<Integer> lattice_diff(monoms_int.row(old_index[e.from_node()]) - monoms_int.row(old_index[e.to_node()]));
    weights[i] = 0;
    for (int ld = 0; ld < lattice_diff.dim(); ++ld)
      weights[i] = gcd(weights[i],lattice_diff[ld]);
  }

  h.take("DOME") << dome;
  h.take("PROJECTIVE_VERTICES") << vertices;
  h.take("LINEALITY_SPACE") << lineality;
  h.take("FAR_VERTICES") << far_vertices;
  h.take("MAXIMAL_POLYTOPES") << cells;
  h.take("REDUNDANT_MONOMIALS") << redundant_monomials;
  // This takes care of constant polynomials - otherwise the regions would be empty
  IncidenceMatrix<> regions = vif.minor(~range(f,f),All);
  if (regions.rows() == 0) regions /= scalar2set(0);
  h.take("REGIONS") << regions; 
  h.take("WEIGHTS") << weights;
}

FunctionTemplate4perl("hypersurface<Addition>(Hypersurface<Addition>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:2
// indent-tabs-mode:nil
// End:
