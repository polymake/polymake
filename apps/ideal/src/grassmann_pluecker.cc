/*  Copyright (c) 1997-2022
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
-------------------------------------------------------------------------------
*/

#include "polymake/client.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/hash_map"
#include "polymake/Vector.h"
#include "polymake/Polynomial.h"


namespace polymake { namespace ideal {
	
	Int term_sign(const Set<Int>& s, const Set<Int>& t){
		
		auto s_it = entire(s);
		auto t_it = entire(t);
		Int count = 0;
		Int ind = 0;
		while (!s_it.at_end() && !t_it.at_end()) {
			if (*s_it == *t_it) return 0;
			if (*s_it <  *t_it) {
				++ind;
				++s_it;
			} else {
				count += s.size() - ind;
				++t_it;
			}
		}
		if (count%2 == 0) return 1;
		return -1;
		
	}
	
	template <typename Scalar>
	void pluecker_ideal_set_varnames_impl(const Array<Set<Int>>& subsets) {
		
		Int m = subsets.size();
		Array<std::string> names(m);
		for (Int i = 0; i < m; ++i) {
			std::stringstream ss;
			ss << "p_(";
			auto t = entire(subsets[i]);
			ss << *t;
			++t;
			while (!t.at_end()) {
				ss << ",";
				ss << *t;
				++t;
			}
			ss << ")";
			names[i] = ss.str();
		}
		Polynomial<Scalar, Int>::set_var_names(names);
		
	}
	
	template <typename Scalar>
	Array<Polynomial<Scalar, Int>> pluecker_ideal_impl(const Array<Set<Int>>& subsets, const Array<Set<Int>>& allsubs, Int d, Int n) {
		
		Int m = subsets.size();
		hash_map<Set<Int>, Int> hm;
		for (Int i = 0; i < m; ++i) {
			hm[subsets[i]] = i;
		}
		Int m2 = static_cast<Int>(Integer::binom(n, d));
		Array<Polynomial<Scalar, Int>> var(m + 1);
		Int a = 0;
		var[m] = Polynomial<Scalar, Int>(m);
		for (Int i = 0; i < m2; ++i) {
			if (hm.find(allsubs[i]) == hm.end()) {
				hm[allsubs[i]] = m;
				++a;
			} else {
				var[i - a] = Polynomial<Scalar, Int>::monomial(i - a, m);
			}
		}
		Int b = static_cast<Int>(Integer::binom(d + 1, (d + 1)/2));
		Array<Polynomial<Scalar, Int>> gb(b * (m2 * m2 - m2)/2);
		Int gb_i(0);
		
		for (Int i = 0; i < m2 - 1; ++i) {
			for (Int j = i + 1; j < m2; ++j) {
				if ((allsubs[i] * allsubs[j]).size() > d - 2) continue;
				Array<Int> array(d + 1);
				Int ind(0);
				Set<Int> x, y;
				auto i_it = entire(allsubs[i]);
				auto j_it = entire(allsubs[j]);
				while (!i_it.at_end() && !j_it.at_end() && *i_it <= *j_it) {
					x += *i_it;
					array[ind++] = *j_it;
					++i_it;
					++j_it;
				}
				if (i_it.at_end() || j_it.at_end()) continue;
				array[ind++] = *j_it;
				array[ind++] = *i_it;
				++i_it;
				++j_it;
				while (!i_it.at_end() && !j_it.at_end()) {
					y += *j_it;
					array[ind++] = *i_it;
					++i_it;
					++j_it;
				}
				Polynomial<Scalar, Int> poly(m);
				auto set = sequence(0, d + 1);
				auto sub = all_subsets_of_k<const Set<Int>>(set, x.size() + 1);
				for (auto s = entire(sub); !s.at_end(); ++s) {
					Set<Int> temp1, temp2;
					for (auto t = entire(*s); !t.at_end(); ++t) {
						temp1 += array[*t];
					}
					for (auto t = entire(set - *s); !t.at_end(); ++t) {
						temp2 += array[*t];
					}
					poly += term_sign(x, temp2) * term_sign(temp1, temp2) * term_sign(temp1, y) * var[hm[x + temp2]] * var[hm[y + temp1]];
				}
				if (poly != var[m]) gb[gb_i++] = std::move(poly);
			}
		}
		
		gb.resize(gb_i);
		return gb;
		
	}
	
	template <typename Scalar>
	Array<Polynomial<Scalar, Int>> pluecker_ideal_generators(Int d, Int n) {
		
		auto all_rows = sequence(0, n);
		auto allsubs = all_subsets_of_k<const Set<Int>>(all_rows, d);
		Array<Set<Int>> subsets(allsubs);
		
		return pluecker_ideal_impl<Scalar>(subsets, subsets, d, n);
	}
	
	template <typename Scalar>
	Array<Polynomial<Scalar, Int>> pluecker_ideal_matroid_generators(BigObject matroid) {
		
		Array<Set<Int>> subsets = matroid.give("BASES");
		Int d = matroid.give("RANK");
		Int n = matroid.give("N_ELEMENTS");
		auto all_rows = sequence(0, n);
		auto as = all_subsets_of_k<const Set<Int>>(all_rows, d);
		Array<Set<Int>> allsubs(as);
		
		return pluecker_ideal_impl<Scalar>(subsets, allsubs, d, n);
	}
	
	template <typename Scalar>
	void pluecker_ideal_set_varnames(Int d, Int n) {
		
		auto all_rows = sequence(0, n);
		auto allsubs = all_subsets_of_k<const Set<Int>>(all_rows, d);
		Array<Set<Int>> subsets(allsubs);
		
		return pluecker_ideal_set_varnames_impl<Scalar>(subsets);
	}
	
	template <typename Scalar>
	void pluecker_ideal_matroid_set_varnames(BigObject matroid) {
		
		Array<Set<Int>> subsets = matroid.give("BASES");
		
		return pluecker_ideal_set_varnames_impl<Scalar>(subsets);
	}
	
	Vector<Int> pluecker_ideal_vector(const Array<Set<Int>>& subsets, Int n) {
		
		Int m = subsets.size();
		Vector<Int> weights(m);
		Int p = 1 << n;
		for (Int i = 0; i < m; ++i) {
			Int w = p;
			for (auto t = entire(subsets[i]); !t.at_end(); ++t) {
				w += 1 << *t; // this is not a optimal choice
			}
			weights[i] = w;
		}
		return weights;
		
	}
	
	BigObject pluecker_ideal_matroid(BigObject matroid) {
		
		Array<Set<Int>> subsets = matroid.give("BASES");
		Int d = matroid.give("RANK");
		Int n = matroid.give("N_ELEMENTS");
		auto all_rows = sequence(0, n);
		auto as = all_subsets_of_k<const Set<Int>>(all_rows, d);
		Array<Set<Int>> allsubs(as);
		
		Vector<Int> weights = pluecker_ideal_vector(subsets, n);
		Array<Polynomial<Rational, Int>> gb = pluecker_ideal_impl<Rational>(subsets, allsubs, d, n);
		BigObject I("ideal::Ideal",
						"GROEBNER.ORDER_VECTOR", weights,
						"GROEBNER.BASIS", gb,
						"GENERATORS", gb);
		std::string des = matroid.description();
		if (des != "") I.set_description() << "Grassman Plücker ideal for " << des;
		return I;
		
	}
	
	BigObject pluecker_ideal(Int d, Int n) {
		
		auto all_rows = sequence(0, n);
		auto allsubs = all_subsets_of_k<const Set<Int>>(all_rows, d);
		Array<Set<Int>> subsets(allsubs);
		
		Vector<Int> weights = pluecker_ideal_vector(subsets, n);
		Array<Polynomial<Rational, Int>> gb = pluecker_ideal_impl<Rational>(subsets, subsets, d, n);
		BigObject I("ideal::Ideal",
						"GROEBNER.ORDER_VECTOR", weights,
						"GROEBNER.BASIS", gb,
						"GENERATORS", gb);
		I.set_description() << "Grassman Plücker ideal for " << d << ", " << n;
		return I;
		
	}
	
	UserFunction4perl("# @category Producing an ideal from scratch"
							"# Generates the ideal of all Grassmann-Plücker relations of dxd minors of an dxn matrix."
							"# For the algorithm see Sturmfels: Algorithms in invariant theory, Springer, 2nd ed., 2008"
							"# @param Int d"
							"# @param Int n"
							"# @return Ideal the Grassmann-Plücker ideal",
	&pluecker_ideal, "pluecker_ideal(Int, Int)");
	
	FunctionTemplate4perl("pluecker_ideal_generators<Scalar>(Int, Int)");
	
	FunctionTemplate4perl("pluecker_ideal_set_varnames<Scalar>(Int, Int)");
	
	InsertEmbeddedRule("REQUIRE_APPLICATION matroid\n\n");
	
	
	UserFunction4perl("# @category Producing an ideal from scratch"
							"# Generates the ideal of all Grassmann-Plücker relations of the given matroid."
							"# For the algorithm see Sturmfels: Algorithms in invariant theory, Springer, 2nd ed., 2008"
							"# @param matroid::Matroid m"
							"# @return Ideal the Grassmann-Plücker ideal",
	&pluecker_ideal_matroid, "pluecker_ideal(matroid::Matroid)");
	
	FunctionTemplate4perl("pluecker_ideal_matroid_generators<Scalar>(matroid::Matroid)");
	
	FunctionTemplate4perl("pluecker_ideal_matroid_set_varnames<Scalar>(matroid::Matroid)");
	
}
}


// Local Variables:
// mode: perl
// cperl-indent-level: 3
// indent-tabs-mode:nil
// End:
