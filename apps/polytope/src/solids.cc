/* Copyright (c) 1997-2023
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
#include "polymake/QuadraticExtension.h"
#include "polymake/Matrix.h"
#include "polymake/permutations.h"
#include "polymake/IncidenceMatrix.h"
#include <vector>

namespace polymake { namespace polytope {

namespace {

const Array<std::string> platonic_names{ std::string("tetrahedron"),
														std::string("cube"),
														std::string("octahedron"),
														std::string("icosahedron"),
														std::string("dodecahedron") };

const Array<std::string> archimedean_names{ std::string("truncated_tetrahedron"),
															std::string("cuboctahedron"),
															std::string("truncated_cube"),
															std::string("truncated_octahedron"),
															std::string("snub_cube"),
															std::string("rhombicuboctahedron"),
															std::string("truncated_cuboctahedron"),
															std::string("icosidodecahedron"),
															std::string("truncated_dodecahedron"),
															std::string("truncated_icosahedron"),
															std::string("rhombicosidodecahedron"),
															std::string("truncated_icosidodecahedron"),
															std::string("snub_dodecahedron") };

const Array<std::string> catalan_names{	std::string("triakis_tetrahedron"),
														std::string("triakis_octahedron"),
														std::string("rhombic_dodecahedron"),
														std::string("tetrakis_hexahedron"),
														std::string("disdyakis_dodecahedron"),
														std::string("pentagonal_icositetrahedron"),
														std::string("pentagonal_hexecontahedron"),
														std::string("rhombic_triacontahedron"),
														std::string("triakis_icosahedron"),
														std::string("deltoidal_icositetrahedron"),
														std::string("pentakis_dodecahedron"),
														std::string("deltoidal_hexecontahedron"),
														std::string("disdyakis_triacontahedron") };
														
}

BigObject platonic_int(Int n) {
	switch(n) {
	case 1: {
		std::vector<std::string> type_params{ "Rational" };
		auto function = polymake::prepare_call_function("tetrahedron", type_params);
		BigObject P = function();
		P.set_description() << "Tetrahedron. A Platonic solid.";
		return P;
	}
	case 2: {
		std::vector<std::string> type_params{ "Rational" };
		auto function = polymake::prepare_call_function("cube", type_params);
		function << 3 << 1 << -1 << OptionSet("character_table", 1);
		BigObject P = function();
		P.set_description() << "Cube. A Platonic solid.";
		return P;
	}
	case 3: {
		std::vector<std::string> type_params{ "Rational" };
		auto function = polymake::prepare_call_function("cross", type_params);
		function << 3 << 1 << OptionSet("character_table", 1);
		BigObject P = function();
		P.set_description() << "Octahedron. A Platonic solid.";
		return P;
	}
	case 4: {
		BigObject P = call_function("icosahedron");
		P.set_description() << "Icosahedron. A Platonic solid.";
		return P;
	}
	case 5: {
		BigObject P = call_function("dodecahedron");
		P.set_description() << "Dodecahedron. A Platonic solid.";
		return P;
	}
	default: {
		throw std::runtime_error("Invalid index of Platonic solid.");
	}
	}
}

BigObject platonic_str(std::string name) {
	static hash_map<std::string,Int> platonic2Int(attach_operation(platonic_names, sequence(1, platonic_names.size()), operations::pair_maker()).begin(),
                                                	attach_operation(platonic_names, sequence(1, platonic_names.size()), operations::pair_maker()).end());
	Int n = platonic2Int[name];
	if (n == 0) throw std::runtime_error("No Platonic solid of given name found.");
	return platonic_int(n);
}

BigObject archimedean_int(Int n) {
	switch(n) {
	case 1: {
		BigObject P = call_function("truncation", call_function("tetrahedron"), sequence(0, 4), OptionSet("cutoff", Rational(2, 3)));
		P.set_description() << "Truncated tetrahedron. An Archimedean solid.";
		return P;
	}
	case 2: {
		BigObject P = call_function("cuboctahedron");
		P.set_description() << "Cuboctahedron. An Archimedean solid.";
		return P;
	}
	case 3: {
		BigObject P = call_function("truncated_cube");
		P.set_description() << "Truncated cube. An Archimedean solid.";
		return P;
	}
	case 4: {
		BigObject P = call_function("truncated_octahedron");
		P.set_description() << "Truncated octahedron. An Archimedean solid.";
		return P;
	}
	case 5: {
		// coordinates from wikipedia
		Matrix<double> M(0, 3);
		double t = (1 + std::pow(19 - 3*sqrt(33), 1/3.) + std::pow(19 + 3*sqrt(33), 1/3.))/3; // FIXME: #830 - cube root in tribonacci constant is inexact
		Matrix<double> N_even{	{-1., 1/t, t},
										{1., -1/t, t},
										{1., 1/t, -t},
										{-1., -1/t, -t}	};
		Matrix<double> N_odd{	{-1., -1/t, t},
										{1., -1/t, -t},
										{-1., 1/t, -t},
										{1., 1/t, t}	};
		auto ap = all_permutations(3);
		for (auto i = ap.begin(); !i.at_end(); ++i) {
			Matrix<double> P = permutation_matrix<double>(*i);
			if (permutation_sign(*i) == 1) {
				M = M / (N_even * P);
			} else {
				M = M / (N_odd * P);
			}
		}
		M = Vector<double>(M.rows(), 1) | M;
		IncidenceMatrix<> VIF{	{16, 17, 20, 23},
										{18, 19, 21, 22},
										{10, 11, 13, 14},
										{8, 9, 12, 15},
										{6, 13, 20},
										{3, 19, 22},
										{12, 18, 22},
										{0, 18, 21},
										{3, 11, 19},
										{3, 5, 22},
										{11, 14, 19},
										{4, 14, 21},
										{0, 9, 18},
										{0, 4, 21},
										{5, 12, 22},
										{0, 1, 4, 7},
										{3, 6, 11},
										{4, 10, 14},
										{5, 8, 12},
										{1, 4, 10},
										{9, 12, 18},
										{7, 9, 15},
										{0, 7, 9},
										{2, 5, 8},
										{2, 3, 5, 6},
										{6, 11, 13},
										{10, 13, 16},
										{14, 19, 21}, 
										{7, 15, 23},
										{1, 10, 16},
									 	{2, 6, 20},
										{13, 16, 20},
										{1, 7, 23},
										{1, 16, 23},
										{8, 15, 17},
										{2, 17, 20},
										{15, 17, 23},
										{2, 8, 17}	};
		BigObject P("Polytope<Float>");
		P.take("VERTICES") << M;
		P.take("VERTICES_IN_FACETS") << VIF;
		P.set_description() << "Snub cube. An Archimedean solid.";
		return P;
	}
	case 6: {
		BigObject P = call_function("rhombicuboctahedron");
		P.set_description() << "Rhombicuboctahedron. An Archimedean solid.";
		return P;
	}
	case 7: {
		BigObject P = call_function("truncated_cuboctahedron");
		P.set_description() << "Truncated cuboctahedron. An Archimedean solid.";
		return P;
	}
	case 8: {
		BigObject P = call_function("icosidodecahedron");
		P.set_description() << "Icosidodecahedron. An Archimedean solid.";
		return P;
	}
	case 9: {
		BigObject P = call_function("truncated_dodecahedron");
		P.set_description() << "Truncated dodecahedron. An Archimedean solid.";
		return P;
	}
	case 10: {
		BigObject P = call_function("truncated_icosahedron");
		P.set_description() << "Truncated icosahedron. An Archimedean solid.";
		return P;
	}
	case 11: {
		BigObject P = call_function("rhombicosidodecahedron");
		P.set_description() << "Rhombicosidodecahedron. An Archimedean solid.";
		return P;
	}
	case 12: {
		BigObject P = call_function("truncated_icosidodecahedron");
		P.set_description() << "Truncated icosidodecahedron. An Archimedean solid.";
		return P;
	}
	case 13: {
		// coordinates from wikipedia
		Matrix<double> M(0,3);
		double phi = (1 + sqrt(5))/2;
		double cet = std::pow((phi + sqrt(phi - 5/27.))/2, 1/3.) + std::pow((phi - sqrt(phi - 5/27.))/2, 1/3.); // FIXME: #830 - cube root is inexact
		double a = cet - 1/cet;
		double b = cet*phi + std::pow(phi, 2) + phi/cet;
		Matrix<double> N{	{-2*a, -2., -2*b},
								{-2*a, 2., 2*b},
								{2*a, -2., 2*b},
								{2*a, 2., -2*b},
								{-(a + b/phi + phi), -(-a*phi + b + 1/phi), -(a/phi + b*phi - 1)},
								{-(a + b/phi + phi), (-a*phi + b + 1/phi), (a/phi + b*phi - 1)},
								{(a + b/phi + phi), -(-a*phi + b + 1/phi), (a/phi + b*phi - 1)},
								{(a + b/phi + phi), (-a*phi + b + 1/phi), -(a/phi + b*phi - 1)},
								{-(a + b/phi - phi), -(a*phi - b + 1/phi), -(a/phi + b*phi + 1)},
								{-(a + b/phi - phi), (a*phi - b + 1/phi), (a/phi + b*phi + 1)},
								{(a + b/phi - phi), -(a*phi - b + 1/phi), (a/phi + b*phi + 1)},
								{(a + b/phi - phi), (a*phi - b + 1/phi), -(a/phi + b*phi + 1)},
								{-(-a/phi + b*phi + 1), -(-a + b/phi - phi), -(a*phi + b - 1/phi)},
								{-(-a/phi + b*phi + 1), (-a + b/phi - phi), (a*phi + b - 1/phi)},
								{(-a/phi + b*phi + 1), -(-a + b/phi - phi), (a*phi + b - 1/phi)},
								{(-a/phi + b*phi + 1), (-a + b/phi - phi), -(a*phi + b - 1/phi)},
								{-(-a/phi + b*phi - 1), -(a - b/phi - phi), -(a*phi + b + 1/phi)},
								{-(-a/phi + b*phi - 1), (a - b/phi - phi), (a*phi + b + 1/phi)},
								{(-a/phi + b*phi - 1), -(a - b/phi - phi), (a*phi + b + 1/phi)},
								{(-a/phi + b*phi - 1), (a - b/phi - phi), -(a*phi + b + 1/phi)}	};
		auto ap = all_permutations(3);
		for (auto i = ap.begin(); !i.at_end(); ++i) {
			Matrix<double> P = permutation_matrix<double>(*i);
			if (permutation_sign(*i) == 1) {
				M = M / (N * P);
			}
		}
		M = Vector<double>(M.rows(), 1) | M;
		IncidenceMatrix<> VIF{	{1, 5, 9, 13, 17},
										{40, 44, 48, 52, 56},
										{0, 4, 8, 12, 16},
										{3, 7, 11, 15, 19},
										{42, 46, 50, 54, 58},
										{2, 6, 10, 14, 18},
										{23, 27, 31, 35, 39},
										{41, 45, 49, 53, 57},
										{22, 26, 30, 34, 38},
										{43, 47, 51, 55, 59},
										{21, 25, 29, 33, 37},
										{20, 24, 28, 32, 36},
										{40, 43, 51},
										{12, 40, 51},
										{17, 48, 56},
										{13, 17, 48},
										{12, 16, 51},
										{16, 51, 59},
										{4, 12, 44},
										{5, 13, 47},
										{17, 39, 56},
										{4, 24, 44},
										{24, 44, 52},
										{26, 47, 55},
										{16, 37, 59},
										{4, 24, 32},
										{5, 26, 47},
										{29, 55, 59},
										{31, 39, 56},
										{31, 52, 56},
										{1, 5, 34},
										{9, 17, 39},
										{5, 26, 34},
										{29, 37, 59},
										{22, 29, 55},
										{40, 43, 48},
										{1, 10, 34},
										{13, 43, 48},
										{8, 16, 37},
										{12, 40, 44},
										{13, 43, 47},
										{0, 4, 32}, {20, 24, 52}, {20, 31, 52},
										{22, 26, 55},
										{1, 2, 10},
										{21, 22, 29},
										{9, 35, 39},
										{10, 34, 38},
										{20, 23, 31},
										{0, 11, 32},
										{0, 3, 8},
										{20, 23, 28},
										{1, 2, 9},
										{8, 33, 37},
										{3, 8, 33},
										{2, 9, 35},
										{19, 36, 57},
										{0, 3, 11},
										{6, 27, 35},
										{11, 32, 36},
										{21, 22, 30},
										{23, 27, 53},
										{21, 25, 54},
										{3, 7, 33},
										{11, 19, 36},
										{21, 30, 54},
										{23, 28, 53},
										{6, 14, 45},
										{2, 6, 35},
										{30, 54, 58},
										{10, 18, 38},
										{27, 45, 53},
										{7, 25, 33},
										{30, 38, 58},
										{6, 27, 45},
										{28, 36, 57},
										{28, 53, 57},
										{18, 38, 58},
										{25, 46, 54},
										{19, 49, 57},
										{41, 42, 49},
										{7, 25, 46},
										{15, 19, 49},
										{14, 41, 45},
										{7, 15, 46},
										{15, 42, 46},
										{15, 42, 49},
										{14, 41, 50},
										{41, 42, 50},
										{18, 50, 58},
										{14, 18, 50}	};
		BigObject P("Polytope<Float>");
		P.take("VERTICES") << M;
		P.take("VERTICES_IN_FACETS") << VIF;
		P.set_description() << "Snub dodecahedron. An Archimedean solid.";
		return P;
	}
	default: {
		throw std::runtime_error("Invalid index of Archimedean solid.");
	}
	}
}

BigObject archimedean_str(std::string name) {
	static hash_map<std::string,Int> archimedean2Int(attach_operation(archimedean_names, sequence(1, archimedean_names.size()), operations::pair_maker()).begin(),
                                                	attach_operation(archimedean_names, sequence(1, archimedean_names.size()), operations::pair_maker()).end());
	Int n = archimedean2Int[name];
	if (n == 0) throw std::runtime_error("No Archimedean solid of given name found.");
	return archimedean_int(n);
}

BigObject catalan_int(Int n) {
	switch(n) {
	case 1: {
		BigObject p = call_function("polarize", archimedean_str("truncated_tetrahedron"));
		p.set_description() << "Triakis Tetrahedron. A Catalan solid.";
		return p;
	}
	case 2: {
		BigObject p = call_function("polarize", archimedean_str("truncated_cube"));
		p.set_description() << "Triakis Octahedron. A Catalan solid.";
		return p;
	}
	case 3: {
		BigObject p = call_function("polarize", call_function("cuboctahedron"));
		p.set_description() << "Rhombic dodecahedron. A Catalan solid.";
		return p;
	}
	case 4: {
		BigObject p = call_function("polarize", call_function("truncated_octahedron"));
		p.set_description() << "Tetrakis Hexahedron . A Catalan solid.";
		return p;
	}
	case 5: {
		BigObject p = call_function("polarize", call_function("truncated_cuboctahedron"));
		p.set_description() << "Disdyakis dodecahedron. A Catalan solid.";
		return p;
	}
	case 6: {
		BigObject p = call_function("polarize", archimedean_str("snub_cube"));
		p.set_description() << "Pentagonal Icositetrahedron. A Catalan solid.";
		return p;
	}
	case 7: {
		BigObject p = call_function("polarize", archimedean_str("snub_dodecahedron"));
		p.set_description() << "Pentagonal Hexecontahedron. A Catalan solid.";
		return p;
	}
	case 8: {
		BigObject p = call_function("polarize", call_function("icosidodecahedron"));
		p.set_description() << "Rhombic triacontahedron. A Catalan solid.";
		return p;
	}
	case 9: {
		BigObject p = call_function("polarize", call_function("truncated_dodecahedron"));
		p.set_description() << "Triakis icosahedron. A Catalan solid.";
		return p;
	}
	case 10: {
		BigObject p = call_function("polarize", call_function("rhombicuboctahedron"));
		p.set_description() << "Deltoidal icositetrahedron. A Catalan solid.";
		return p;
	}
	case 11: {
		BigObject p = call_function("polarize", call_function("truncated_icosahedron"));
		p.set_description() << "Pentakis dodecahedron. A Catalan solid.";
		return p;
	}
	case 12: {
		BigObject p = call_function("polarize", call_function("rhombicosidodecahedron"));
		p.set_description() << "Deltoidal hexecontahedron. A Catalan solid.";
		return p;
	}
	case 13: {
		BigObject p = call_function("polarize", call_function("truncated_icosidodecahedron"));
		p.set_description() << "Disdyakis triacontahedron. A Catalan solid.";
		return p;
	}
	default: {
		throw std::runtime_error("No Catalan solid of given name found.");
	}
	}
}

BigObject catalan_str(std::string name) {
	static hash_map<std::string,Int> catalan2Int(attach_operation(catalan_names, sequence(1, catalan_names.size()), operations::pair_maker()).begin(),
                                                	attach_operation(catalan_names, sequence(1, catalan_names.size()), operations::pair_maker()).end());
	Int n = catalan2Int[name];
	if (n == 0) throw std::runtime_error("No Catalan solid of given name found.");
	return catalan_int(n);
}


UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create Platonic solid of the given name."
						"# @param String s the name of the desired Platonic solid"
						"# @value s 'tetrahedron' Tetrahedron."
						"#          Regular polytope with four triangular facets."
						"# @value s 'cube' Cube."
						"#          Regular polytope with six square facets."
						"# @value s 'octahedron' Octahedron."
						"#          Regular polytope with eight triangular facets."
						"# @value s 'dodecahedron' Dodecahedron."
						"#          Regular polytope with 12 pentagonal facets."
						"# @value s 'icosahedron' Icosahedron."
						"#          Regular polytope with 20 triangular facets."
                  "# @return Polytope",
                  &platonic_str, "platonic_solid(String)");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Create Platonic solid number n, where 1 <= n <= 5."
						"# @param Int n the index of the desired Platonic solid"
                  "# @return Polytope",
                  &platonic_int, "platonic_solid(Int)");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
						"# Create Archimedean solid of the given name."
						"# Some polytopes are realized with floating point numbers and thus not exact;"
						"# Vertex-facet-incidences are correct in all cases."
						"# @param String s the name of the desired Archimedean solid"
						"# @value s 'truncated_tetrahedron' Truncated tetrahedron."
						"#          Regular polytope with four triangular and four hexagonal facets."
						"# @value s 'cuboctahedron' Cuboctahedron."
						"#          Regular polytope with eight triangular and six square facets."
						"# @value s 'truncated_cube' Truncated cube."
						"#          Regular polytope with eight triangular and six octagonal facets."
						"# @value s 'truncated_octahedron' Truncated Octahedron."
						"#          Regular polytope with six square and eight hexagonal facets."
						"# @value s 'rhombicuboctahedron' Rhombicuboctahedron."
						"#          Regular polytope with eight triangular and 18 square facets."
						"# @value s 'truncated_cuboctahedron' Truncated Cuboctahedron."
						"#          Regular polytope with 12 square, eight hexagonal and six octagonal facets."
						"# @value s 'snub_cube' Snub Cube."
						"#          Regular polytope with 32 triangular and six square facets."
						"#          The vertices are realized as floating point numbers."
						"#          This is a chiral polytope."
						"# @value s 'icosidodecahedron' Icosidodecahedon."
						"#          Regular polytope with 20 triangular and 12 pentagonal facets."
						"# @value s 'truncated_dodecahedron' Truncated Dodecahedron."
						"#          Regular polytope with 20 triangular and 12 decagonal facets."
						"# @value s 'truncated_icosahedron' Truncated Icosahedron."
						"#          Regular polytope with 12 pentagonal and 20 hexagonal facets."
						"# @value s 'rhombicosidodecahedron' Rhombicosidodecahedron."
						"#          Regular polytope with 20 triangular, 30 square and 12 pentagonal facets."
						"# @value s 'truncated_icosidodecahedron' Truncated Icosidodecahedron."
						"#          Regular polytope with 30 square, 20 hexagonal and 12 decagonal facets."
						"# @value s 'snub_dodecahedron' Snub Dodecahedron."
						"#          Regular polytope with 80 triangular and 12 pentagonal facets."
						"#          The vertices are realized as floating point numbers."
						"#          This is a chiral polytope."
						"# @return Polytope"
						"# @example To show the mirror image of the snub cube use:"
						"# > scale(archimedean_solid('snub_cube'),-1)->VISUAL;",
                  &archimedean_str, "archimedean_solid(String)");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
						"# Create Archimedean solid number n, where 1 <= n <= 13."
						"# Some polytopes are realized with floating point numbers and thus not exact;"
						"# Vertex-facet-incidences are correct in all cases."
						"# @param Int n the index of the desired Archimedean solid"
						"# @return Polytope",
                  &archimedean_int, "archimedean_solid(Int)");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
						"# Create Catalan solid of the given name."
						"# Some polytopes are realized with floating point numbers and thus not exact;"
						"# Vertex-facet-incidences are correct in all cases."
						"# @param String s the name of the desired Catalan solid"
						"# @value s 'triakis_tetrahedron' Triakis Tetrahedron."
						"#          Dual polytope to the Truncated Tetrahedron, made of 12 isosceles triangular facets."
						"# @value s 'triakis_octahedron' Triakis Octahedron."
						"#          Dual polytope to the Truncated Cube, made of 24 isosceles triangular facets."
						"# @value s 'rhombic_dodecahedron' Rhombic dodecahedron."
						"#          Dual polytope to the cuboctahedron, made of 12 rhombic facets."
						"# @value s 'tetrakis_hexahedron' Tetrakis hexahedron."
						"#          Dual polytope to the truncated octahedron, made of 24 isosceles triangluar facets."
						"# @value s 'disdyakis_dodecahedron' Disdyakis dodecahedron."
						"#          Dual polytope to the truncated cuboctahedron, made of 48 scalene triangular facets."
						"# @value s 'pentagonal_icositetrahedron' Pentagonal Icositetrahedron."
						"#          Dual polytope to the snub cube, made of 24 irregular pentagonal facets."
						"#          The vertices are realized as floating point numbers."
						"# @value s 'pentagonal_hexecontahedron' Pentagonal Hexecontahedron."
						"#          Dual polytope to the snub dodecahedron, made of 60 irregular pentagonal facets."
						"#          The vertices are realized as floating point numbers."
						"# @value s 'rhombic_triacontahedron' Rhombic triacontahedron."
						"#          Dual polytope to the icosidodecahedron, made of 30 rhombic facets."
						"# @value s 'triakis_icosahedron' Triakis icosahedron."
						"#          Dual polytope to the icosidodecahedron, made of 30 rhombic facets."
						"# @value s 'deltoidal_icositetrahedron' Deltoidal Icositetrahedron."
						"#          Dual polytope to the rhombicubaoctahedron, made of 24 kite facets."
						"# @value s 'pentakis_dodecahedron' Pentakis dodecahedron."
						"#          Dual polytope to the truncated icosahedron, made of 60 isosceles triangular facets."
						"# @value s 'deltoidal_hexecontahedron' Deltoidal hexecontahedron."
						"#          Dual polytope to the rhombicosidodecahedron, made of 60 kite facets."
						"# @value s 'disdyakis_triacontahedron' Disdyakis triacontahedron."
						"#          Dual polytope to the truncated icosidodecahedron, made of 120 scalene triangular facets."
						"# @return Polytope",
                  &catalan_str, "catalan_solid(String)");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
						"# Create Catalan solid number n, where 1 <= n <= 13."
						"# Some polytopes are realized with floating point numbers and thus not exact;"
						"# Vertex-facet-incidences are correct in all cases."
						"# @param Int n the index of the desired Catalan solid"
						"# @return Polytope",
                  &catalan_int, "catalan_solid(Int)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
