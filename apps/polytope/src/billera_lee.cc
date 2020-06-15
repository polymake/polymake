#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Integer.h"

#include "polymake/hash_map"
#include "polymake/IncidenceMatrix.h"

#include "polymake/Rational.h"
#include "polymake/polytope/inner_point.h"

#include "polymake/Polynomial.h"
#include "polymake/Map.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace polytope {

namespace {

// returns the next monomial in n_vars in revlex order that has degree > min_deg.
// input given in n-expanded notation.
// e.g. n=8, x^2yz^3 = xxyzzz -> monom = [0,0,1,1,2,3,3,3]
Vector<Int> next_monom(const Vector<Int>& prev, Int n_vars, Int min_deg = 0)
{
  // convert to exponent vector
  Vector<Int> exps(n_vars);
  for (Int i : prev)
    if (i != 0)
      exps[i-1]++;

  Int max_deg = prev.size();

  if (exps[n_vars-1] == max_deg)
    throw std::runtime_error("There is no next monomial.");

  for (Int i = 0; i < n_vars; ++i) {
    // revlex increase
    if (exps[i] < max_deg){
      ++exps[i];
      for (Int j = 0; j < i; ++j)
        exps[j] = 0;

      // maybe the increase left the valid deg range. compute degree:
      Int deg = 0;
      for (Int e : exps) deg += e;

      if (deg < min_deg) //degree too small! increase once more.
        i = -1;
      else if(deg <= max_deg)
        break; //found it!
    }
  }

  // convert back to n-expanded notation
  Vector<Int> next(max_deg);
  Int j = max_deg;
  for (Int i = n_vars-1; i >= 0; --i) {
    for (Int k = 0; k < exps[i]; ++k) {
      --j;
      next[j] = i+1;
    }
  }

  return next;
}

// input a monomial in n-expanded notation.
// returns the corresponding facet as vertex index set
// this is the inverse of the alpha map from the paper
// shift is 1 if d is even and 2 if d is odd, those are the vertices present in all facets.
Set<Int> beta(const Vector<Int>& monom, Int dim)
{
  Int shift = 1+dim%2;
  Set<Int> F = sequence(0, shift);
  for (Int i = 0; i < monom.size(); ++i) {
    F += shift + monom[i]+2*i;
    F += shift + monom[i]+2*i+1;
  }
  return F;
}


////////////// computing a realization

// compute a bound on the positive real roots of the uni-polynomial
// this is from a 1767 theorem by langrange, proved here:
// https://hal.archives-ouvertes.fr/hal-00129675
Rational root_bound(const Polynomial<Rational, Int> & p)
{
   Set<double> roots;
   const Int deg = p.deg();
   const Rational l = p.lc();
   for (const auto& t : p.get_terms()) {
      const Rational& a = t.second;
      if (a < 0) {
         const Int n = t.first.empty() ? 0 : *t.first.begin();
         if (n != deg) {
            double ar = std::pow(double(-a/l), 1.0/double(deg-n));
            roots += ar;
         }
      }
   }

   // sum the two largest entries
   double b = 0;
   auto r_it = roots.rbegin();
   if (!r_it.at_end()) {
      b = *r_it;
      ++r_it;
   }
   if (!r_it.at_end()) {
      b += *r_it;
   }
   // round up to prevent huuuge rationals
   b = ceil(b*100.0);
   return Rational(Integer(b), 100);
}


// find a point (t_0...t_k) for which all input polynomials evaluate positive.
// the point satisfies t_0<t_1<...<t_k. The start parameter gives a lower bound on t_0.
// the (k-variate) polynomials must have positive lead coefficient (which means
// coefficient of the highest power of the k-th variable), this function will not
// terminate if they don't so take care.
// Algorithm is from Proposition 3 of the Billera-Lee paper.
Array<Rational> solve_system(const Array<Polynomial<Rational, Int>> & p, Rational start = 0)
{
   // univariate case
   if (p[0].n_vars() <= 1) {
      Array<Rational> t{start};
      // find a large enough number such that all (uni-)polynomials are positive
      for (auto poly : p) {
         assign_max(t[0], root_bound(poly));
      }
      return t;
  } else {
      // multivariate case
      // build the array of lead coefficients (themselves polynomials in one variable less)
      Array<Polynomial<Rational, Int>> lead_coeffs(p.size());
      for (Int i = 0; i < p.size(); ++i) {
         Matrix<Int> exp = p[i].monomials_as_matrix();
         Vector<Int> degs = exp.col(exp.cols()-1);
         Int deg = *std::max_element(degs.begin(), degs.end());
         // collect row indices that correspond to terms in the leading coefficient, that
         // is rows where the last entry is deg.
         Set<Int> leads;
         for (Int j = 0; j < degs.size(); ++j)
            if (degs[j] == deg)
               leads += j;
         lead_coeffs[i] = Polynomial<Rational,Int>(p[i].coefficients_as_vector().slice(leads),
                                                   exp.minor(leads, sequence(0, exp.cols()-1)));
      }
      // solve the k-1-variate system, obtaining a k-1-dim vector
      Array<Rational> tind = solve_system(lead_coeffs,start);

      // partially evaluate the guys in p at that solution, creating a univariate system...
      Array<Polynomial<Rational, Int>> p_eval(p.size());
      Map<Int, Rational> tmap;
      for (Int i = 0; i < tind.size(); ++i)
         tmap[i] = tind[i];
      for (Int i = 0; i < p.size(); ++i) {
         // partially substitute
         p_eval[i] = p[i].substitute(tmap);
         // correct number of variables
         p_eval[i] = p_eval[i].project(Array<Int>{p[i].n_vars()-1});
      }
      //...and solve that.
      Array<Rational> t = solve_system(p_eval, tind[tind.size()-1] + Rational(1,1000));

      // the solutions are the ones from the smaller system together with that last one.
      return tind.append(1,t.begin());
   }
}


// compute the k-th entry of the normal vector of the facet given by indices F in the
// _indeterminate_ cyclic polytope (this hence is a polynomial in t_0...t_{v-1})
Polynomial<Rational, Int> gamma(const Set<Int> & F, Int k, Int v)
{
  Int s = F.size()-k;
  ListMatrix<Vector<Int>> exp;
  for (auto summand = entire(all_subsets_of_k(F, s)); !summand.at_end(); ++summand) {
    Vector<Int> r(v);
    for(auto e : *summand)
      r[e]=1;
    exp /= r;
  }
  Rational c = (s%2==0 ? 1 : -1);
  Array<Rational> coef(exp.rows(), c);
  return Polynomial<Rational, Int>(coef, exp);
}


// compute the k-th entry of the z_i vector from the paper, section 7.
Polynomial<Rational, Int> z(Int i, Int k, Int dim, Int v)
{
  Int n = dim/2;
  Int shift = 1-dim%2;
  Matrix<Int> exp(n-i+1, v);
  for (Int m = 0; m < n-i+1; ++m) {
    exp[m][shift+2*m] = k;
    for (Int j = shift+2*m+1; j < v; ++j)
      exp[m][j] = dim;
  }
  Array<Rational> coef(exp.rows(), 1);
  return Polynomial<Rational, Int>(coef, exp);
}


// input a facet as vertex index set
// returns the degree of the monomial that corresponds via the alpha map from the paper
Int monom_deg(const Set<Int>& F)
{
  // shift is 1 if dimension is even and 2 if it is odd, those are the vertices present in all facets.
  Int shift = 1 + (F.size()-1)%2;

  auto e = F.begin();
  // skip the first vertices
  ++e;
  if (shift == 2)
    ++e;

  Int deg = 0;
  // count the number of vertices that do not get mapped to x_0 by alpha.
  for (Int i = 0; !e.at_end(); i += 2) {
    if (*e-i-shift != 0)
      ++deg;
    // skip to the next pair
    ++e;
    ++e;
  }
  return deg;
}


// make a lex sorted list of all facet index sets in E (= even right-end set)
Array<Set<Int>> compute_E(Int n, Int n_vars, Int d)
{
  // number of monomials in n_vars variables of deg<=n
  Int N(Integer::binom(n+n_vars,n_vars));

  // the facets we're looking for come from the bijective mapping beta
  // from the monomials of degree <= n in n_vars
  Array<Set<Int>> E(N);
  Vector<Int> M(n); // zero monomial
  E[0] = beta(M, d);
  for (Int m = 1; m < N; ++m) {
    M = next_monom(M, n_vars);
    E[m] = beta(M,d);
  }

  return E;
}


// make the system of inequalities described in the paper, section 7, equation (4).
// if the realization satisfies these, the magical z exists.
Array<Polynomial<Rational, Int>> get_inequalities(Int n, Int d, Int v)
{
  // get the set of even-right-end-set-facets
  Array<Set<Int>> E = compute_E(n, v-d-1, d);
  Int N = E.size();

  std::list<Polynomial<Rational, Int>> ineqs;

  // iterate all facets in E (= even right end set)
  for (Int m = 0; m < N-1; ++m) {
    Set<Int> G = E[m];
    Int gdeg = monom_deg(G);
    // iterate all monomials F that are lex bigger than G
    for (Int j = m+1; j < N; ++j) {
      Set<Int> F = E[j];
      Int gfdeg = std::min(gdeg, monom_deg(F));
      // build the sum of polynomials that gives the inequality for this pair F and G
      for (Int i = 0; i < gfdeg; ++i) {
        Polynomial<Rational, Int> p(v); // zero polynomial in v variables
        for (Int k = 0; k < d+1; ++k) {
          p +=  z(i,k,d,v) * (gamma(F,k,v) - gamma(G,k,v));
        }
        ineqs.push_back(p);
      }
    }
  }
  return Array<Polynomial<Rational, Int>>(ineqs);
}


// get a vector 0<t_1<...<t_k for which the magical point exists.
Array<Rational> obtain_realization(Int n, Int d, Int v)
{
   // compute the inequalities that need to be fulfilled in order for the magic point to exist
   Array<Polynomial<Rational, Int>> ineqs = get_inequalities(n, d, v);

   // we want t_0 = 0, so we need to partially evaluate the guys in ineqs at 0
   Array<Polynomial<Rational, Int>> p_eval(ineqs.size());
   Map<Int, Rational> t_0;
   t_0[0] = 0;
   for (Int i = 0; i < ineqs.size(); ++i) {
      // partially substitute
      p_eval[i] = ineqs[i].substitute(t_0);

      // correct number of variables
      p_eval[i] = p_eval[i].project(sequence(1,p_eval[i].n_vars()-1));
   }

   // compute a solution of this inequality system, setting t_0=0.
   Array<Rational> t{0};
   return t.append(solve_system(p_eval,Rational(1,1000)));
}


////////////// finding the collection of good facets

// returns the lex first n_monoms monomials of degree deg in n_vars variables.
// monomials are stored in n-expanded notation (e.g. for n=5, x^2y -> [0,0,1,1,2])
// as the rows of the output matrix.
Matrix<Int> generate_M(Int n, Int n_vars, Int deg, Int n_monoms)
{
  Matrix<Int> M = ones_matrix<Int>(1, deg);
  for (Int i = 1; i < n_monoms; ++i)
    M /= next_monom(M[i-1], n_vars, deg);
  return zero_matrix<Int>(n_monoms, n-deg) | M;
}


// compute the index sets of facets in B, that is, the facets we want to turn around.
Set<Int> compute_facets(const Vector<Integer>& G, const IncidenceMatrix<>& vif, Int d)
{
  //map facet vertex indices set to facet index
  hash_map<Set<Int>, Int> f_map;
  Int ind = 0;
  for (auto r : rows(vif))
    f_map[r] = ind++;

  const Int n_vars(G[1]);
  const Int n = d/2;
  Set<Int> F;
  for (Int i = 0; i < G.size(); ++i) {
    //retrieve lex-first G[i] monomials of degree i with G[1] variables.
    Matrix<Int> M = generate_M(n, n_vars, i, Int(G[i]));
    //map them all to their corresponding facet indices of the cyclic polytope.
    for (auto r = entire(rows(M)); !r.at_end(); ++r) {
      F += f_map[beta(*r,d)];
    }
  }
  return F;
}

} //end anonymous namespace


// This is an implementation of the algorithm described in the paper
// "A Proof of the Sufficiency of McMullen’s Conditions of Simplicial Convex Polytopes" by
// by Louis Billera and Carl Lee, DOI: 10.1016/0097-3165(81)90058-3
BigObject billera_lee(const Vector<Integer>& H)
{
  const Int d = H.size()-1; // dimension of the facets
  const Int n = d/2;

  bool valid = d > 0 && H[0] == 1;
  for (Int i = 0; valid && i < n; ++i)
    valid = H[i] == H[d-i] && H[i+1] >= H[i];

  if (!(valid && H[n] == H[d-n]))
    throw std::runtime_error("Given vector does not satisfy McMullen's Conditions");

  if (H[1]==1)
    return call_function("simplex", d);

  // compute G-vector from H-vector.
  Vector<Integer> G(n+1);
  G[0] = H[0];
  for (Int i = 1; i < G.size(); ++i)
    G[i] = H[i] - H[i-1];

  if (!call_function("m_sequence", G))
    throw std::runtime_error("The G-vector is not an M-sequence.");

  Int v = Int(G[1])+d+1; // number of vertices our cyclic polytope shall have

  // get a realization of the cyclic polytope such that the magic point exists
  Array<Rational> t = obtain_realization(n, d, v);

  // compute vertices of the realization of the cyclic polytope coming from t...
  Matrix<Rational> cyclic_V(v, d+2);
  auto vert = concat_rows(cyclic_V).begin();
  for (auto ti : t){
    *vert++ = 1; // homogenize!
    Rational pow = 1;
    for (Int j = 1; j <= d+1; ++j) {
      pow *= ti;
      *vert++ = pow;
    }
  }

  //...and construct a polytope from them.
  BigObject cyclic("Polytope<Rational>", "VERTICES", cyclic_V);

  const IncidenceMatrix<>& vif = cyclic.give("VERTICES_IN_FACETS");
  // get the indices of the good facets in vif
  Set<Int> f_good_indices = compute_facets(G, vif, d);

  // compute a magic point z beneath the good and beyond the bad facets
  // it exists because the realization was chosen carefully
  const Matrix<Rational>& F = cyclic.give("FACETS");

  // append the unit vector (far face) as the polyhedron could be unbounded
  Vector<Rational> z = inner_point_from_facets( - F.minor(f_good_indices, All) /
                       F.minor(sequence(0,F.rows()) - f_good_indices, All) /
                       unit_vector<Rational>(F.cols(),0));

  // construct a polytope from z and the points on the good facets
  const Matrix<Rational> V = z / cyclic_V;
  BigObject p("Polytope<Rational>", "VERTICES", V);
  // take the vertex figure of z
  BigObject out = call_function("vertex_figure", p, 0, OptionSet("no_labels", true));
  out.set_description() << "Billera-Lee construction for given H-vector "<< H << endl;

  return out;
}

UserFunction4perl("# @category Producing a polytope from other objects"
                  "# Produces a simplicial polytope whose H-vector is the given input vector."
                  "# The G-vector coming from the given vector must be an M-sequence."
                  "# This is an implementation of the algorithm described in the paper"
                  "# \"A Proof of the Sufficiency of McMullen’s Conditions of Simplicial Convex Polytopes\""
                  "# by Louis Billera and Carl Lee, DOI: 10.1016/0097-3165(81)90058-3"
                  "# @param Vector<Integer> H"
                  "# @return Polytope"
                  "# @example"
                  "# > $p = billera_lee([1,5,15,15,5,1]);"
                  "# > print $p->H_VECTOR;"
                  "# | 1 5 15 15 5 1",
                  &billera_lee, "billera_lee($)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
