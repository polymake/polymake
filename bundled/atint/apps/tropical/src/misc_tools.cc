/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

	Implementations of miscelleaneous tools
	*/

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/tropical/LoggingPrinter.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/morphism_values.h"
#include "polymake/tropical/homogeneous_convex_hull.h"
#include "polymake/tropical/solver_def.h"
#include "polymake/tropical/misc_tools.h"


namespace polymake { namespace tropical { 

	typedef std::pair<Matrix<Rational>, Matrix<Rational> > matrix_pair;


	std::pair<Set<int>, Set<int> > far_and_nonfar_vertices(const Matrix<Rational> &m) {
		Set<int> nonfar;
		Set<int> far;
		for(int r = 0; r < m.rows(); r++) {
			(m(r,0) == 0 ? far : nonfar) += r;
		}
		return std::pair<Set<int>,Set<int> >(far,nonfar);
	}


	IncidenceMatrix<> all_cones_as_incidence(perl::Object complex) {
		Array<IncidenceMatrix<> > all_cones = complex.give("CONES");
		if(all_cones.size() == 0) return IncidenceMatrix<>();
		IncidenceMatrix<> result(0,all_cones[0].cols());
		for(int i = 0; i < all_cones.size(); i++) {
			result /= all_cones[i];
		}
		return result;
	}


	Vector<Set<int> > incMatrixToVector(const IncidenceMatrix<> &i) {
		Vector<Set<int> > result;
		for(int r = 0; r < i.rows(); r++) {
			result |= i.row(r);
		}
		return result;
	}


	Array<Integer> randomInteger(const int& max_arg, const int &n) {
		static Integer upperBound = 0;
		static UniformlyRandomRanged<Integer> rg(max_arg);
		if(max_arg != upperBound)  {
			rg = UniformlyRandomRanged<Integer>(max_arg);
			upperBound = max_arg;
		}
		Array<Integer> result(n);
		for(int i = 0; i < n; i++) {
			result[i] = rg.get();
		}
		return result;
	}

	Matrix<Rational> binaryMatrix(int n) {
		Matrix<Rational> result(0,n);
		result /= (- ones_vector<Rational>(n));
		//Now increase the last row of result by "one" in each iteration and append the new row to result
		Integer iterations = pow(2,n)-1;
		int i = 1;
		while(i <= iterations) {
			//Find the first -1-entry
			int index = 0;
			while(result(i-1,index) == 1) { index++;}
			//Now toggle this and all preceding entries
			Vector<Rational> newrow(result.row(i-1));
			newrow[index] = 1;
			for(int j = 0; j < index; j++) newrow[j] = -1;
			result /= newrow;
			i++;
		}
		return result;
	}

	int binaryIndex(Vector<Rational> v) {
		int result = 0;
		for(int i = 0; i < v.dim(); i++) {
			if(v[i] == 1) result += pow(2,i);
		}
		return result;
	}

	/**
	  @brief Computes the labels for each cell of a function domain, given its ray and lineality values
	  @param perl::Object domain A Cycle object, representing the domain of the function
	  @param Matrix<Rationl> ray_values Values of the function on the rays, given as row vectors in non-homog. coordinates
	  @param Matrix<Rational> lin_values Values of the function on the lineality space, given as row vectors in non-homog. coordinates.
	  @param bool values_are_homogeneous Whether vertex and lineality values are given in tropical projective
	  coordinates.
	  @return A list of std::strings
	  */
	perl::ListReturn computeFunctionLabels(perl::Object domain, Matrix<Rational> ray_values, Matrix<Rational> lin_values, bool values_are_homogeneous) {
		//Extract values
		Matrix<Rational> rays = domain.give("SEPARATED_VERTICES");
		rays = tdehomog(rays,0);
		IncidenceMatrix<> cones = domain.give("SEPARATED_MAXIMAL_POLYTOPES");
		Matrix<Rational> lineality = domain.give("LINEALITY_SPACE");
		lineality = tdehomog(lineality,0);

		if(values_are_homogeneous) {
			ray_values = tdehomog(ray_values,0);
			lin_values = tdehomog(lin_values,0);
		}

		perl::ListReturn result;

		for(int mc = 0; mc < cones.rows(); mc++) {
			//dbgtrace << "Computing representation of cone " << mc << endl;
			Matrix<Rational> matrix;
			Vector<Rational> translate;
			computeConeFunction(rays.minor(cones.row(mc),All), lineality, ray_values.minor(cones.row(mc),All), lin_values, translate, matrix);

			matrix = thomog(matrix,0,false);
			translate = thomog_vec(translate,0,false);

			std::ostringstream sstream;
			pm::PlainPrinter<> rep(sstream);
			if(matrix.rows() > 1) {
				rep << "(" << translate << ")" << " + ";
				for(int i = 0; i < matrix.rows(); i++) {
					rep << "[" << matrix.row(i) << "]";
				}
			}
			//We have a special representation format for functions to R
			else {
				bool hadnonzeroterm = false;
				for(int i = 0; i < matrix.cols(); i++) {
					if(matrix(0,i) != 0) {
						if(hadnonzeroterm) rep << " + ";
						hadnonzeroterm = true;
						if(matrix(0,i) < 0) rep << "(" << matrix(0,i) << ")";
						else rep << matrix(0,i);
						rep << "*x_" << (i+1);
					}
				}
				if(translate[0] < 0 && hadnonzeroterm) rep << " - " << (-translate[0]);
				if(translate[0] > 0 && hadnonzeroterm) rep << " + " << translate[0];
				if(!hadnonzeroterm) rep << translate[0];
			}
			result << sstream.str();

		}

		return result;

	}


	//Documentation see perl wrapper
	bool contains_point(perl::Object complex, Vector<Rational> point) {

		//Special case: Empty cycle
		if(CallPolymakeFunction("is_empty",complex))
			return false;

		//Extract values
		Matrix<Rational> rays = complex.give("VERTICES");
		Matrix<Rational> linspace = complex.give("LINEALITY_SPACE");
		IncidenceMatrix<> cones = complex.give("MAXIMAL_POLYTOPES");

		if(point.dim() != rays.cols() && point.dim() != linspace.cols()) {
			throw std::runtime_error("Point does not have the same ambient dimension as the complex.");
		}

		solver<Rational> sv;
		for(int mc = 0; mc < cones.rows(); mc++) {
			if(is_ray_in_cone(rays.minor(cones.row(mc),All),linspace,point,true,sv)) return true;
		}

		return false;
	}

	UserFunction4perl("# @category Lattices"
			"# Returns n random integers in the range 0.. (max_arg-1),inclusive"
			"# Note that this algorithm is not optimal for real randomness:"
			"# If you change the range parameter and then change it back, you will"
			"# usually get the exact same sequence as the first time"
			"# @param int max_arg The upper bound for the random integers"
			"# @param int n The number of integers to be created"
			"# @return Array<Integer>",
			&randomInteger,"randomInteger($, $)");

	UserFunction4perl("# @category Basic polyhedral operations"
			"# Takes a weighted complex and a point and computed whether that point lies in "
			"# the complex"
			"# @param Cycle A weighted complex"
			"# @param Vector<Rational> point An arbitrary vector in the same ambient"
			"# dimension as complex. Given in tropical projective coordinates with leading coordinate."
			"# @return bool Whether the point lies in the support of complex",
			&contains_point,"contains_point(Cycle,$)");

	Function4perl(&computeFunctionLabels, "computeFunctionLabels(Cycle, Matrix,Matrix,$)");
}}
