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

	Computes all [[LATTICE...]- related properties
	*/

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Map.h"
#include "polymake/linalg.h"
#include "polymake/common/hermite_normal_form.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/lattice.h"
#include "polymake/tropical/linear_algebra_tools.h"
#include "polymake/tropical/LoggingPrinter.h"
#include "polymake/tropical/misc_tools.h"


namespace polymake { namespace tropical {


	using namespace atintlog::donotlog;
	//   using namespace atintlog::dolog;
	//using namespace atintlog::dotrace;
	
	typedef Map< std::pair<int,int>, Vector<Integer> > LatticeMap;


	Matrix<Integer> makePrimitiveInteger(const Matrix<Rational> &m) {
		Matrix<Integer> result(m.rows(), m.cols());
		for(int r = 0; r < m.rows(); r++) { 
			Integer lc = 1;
			for(int c = 0; c < m.cols(); c++) {
				lc = lcm(lc,denominator(m(r,c)));
			}
			result.row(r) = lc* m.row(r);
			Integer gc = result(r,0);
			for(int c = 1; c < m.cols(); c++) {
				gc = gcd(gc,result(r,c));
			}
			result.row(r) = (1/Rational(gc)) * result.row(r);
		}
		return result;
	}

	Vector<Integer> makePrimitiveInteger(const Vector<Rational> &v) {
		Vector<Integer> result(v.dim());
		Integer lc = 1;
		for(int c = 0; c < v.dim(); c++) {
			lc = lcm(lc,denominator(v[c]));
		}
		result = lc* v;
		Integer gc = result[0];
		for(int c = 1; c < result.dim(); c++) {
			gc = gcd(gc,result[c]);
		}
		result = (1/Rational(gc)) * result;

		return result;
	}


	Matrix<Integer> make_rowwise_integer(const Matrix<Rational> &m) {
		Matrix<Integer> result(m.rows(), m.cols());
		for(int r = 0; r < m.rows(); r++) { 
			Integer mult = 1;
			for(int c = 0; c < m.cols(); c++) {
				mult *= denominator(m(r,c));
			}
			result.row(r) = mult* m.row(r);
		}
		return result;
	}//END make_rowwise_integer

	/*
	 * @brief Computes [[LATTICE_NORMAL_SUM]]
	 */
	void computeLatticeNormalSum(perl::Object cycle) {
		LatticeMap latticeNormals = cycle.give("LATTICE_NORMALS");
		int ambient_dim = cycle.give("FAN_AMBIENT_DIM");
		IncidenceMatrix<> codimOneCones = cycle.give("CODIMENSION_ONE_POLYTOPES");
		Vector<Integer> weights = cycle.give("WEIGHTS");
		IncidenceMatrix<> codimInc = cycle.give("MAXIMAL_AT_CODIM_ONE");

		//This will contain the result
		Matrix<Integer> summatrix(0,ambient_dim);

		//Iterate over all codim one faces
		for(int facet = 0; facet < codimOneCones.rows(); facet++) {
			//This will contain the weighted sum of the lattice normals
			Vector<Integer> result = zero_vector<Integer>(ambient_dim);
			Set<int> adjacentCones = codimInc.row(facet);
			//Go through all adjacent cones
			for(Entire<Set<int> >::iterator e=entire(adjacentCones); !e.at_end(); ++e) {
				result = result +latticeNormals[std::make_pair(facet,*e)] * weights[*e];
			}
			summatrix = summatrix / result;
		}

		cycle.take("LATTICE_NORMAL_SUM") << summatrix;

	}//END computeLatticeNormalSum

	/*
	 * @brief Computes properties [[LATTICE_NORMAL_FCT_VECTOR]], [[LATTICE_NORMAL_SUM_FCT_VECTOR]]
	 */
	void computeLatticeFunctionData(perl::Object cycle) {
		//Extract properties from the cycle
		Matrix<Rational> linealitySpace = cycle.give("LINEALITY_SPACE");
		linealitySpace = tdehomog(linealitySpace);	
		int lineality_dim = linealitySpace.rows();

		Matrix<Rational> rays = cycle.give("SEPARATED_VERTICES");
		rays = tdehomog(rays);
		LatticeMap latticeNormals = cycle.give("LATTICE_NORMALS");
		Matrix<Rational> normalsums = cycle.give("LATTICE_NORMAL_SUM");
		normalsums = tdehomog(normalsums);
		IncidenceMatrix<> codimOneCones = cycle.give("SEPARATED_CODIMENSION_ONE_POLYTOPES");
		IncidenceMatrix<> maximalCones = cycle.give("SEPARATED_MAXIMAL_POLYTOPES");
		IncidenceMatrix<> coneIncidences = cycle.give("MAXIMAL_AT_CODIM_ONE");


		//Result variables
		Map<std::pair<int,int>, Vector<Rational> > summap;
		Matrix<Rational> summatrix;
		Vector<bool> balancedFaces(codimOneCones.rows());

		//Iterate over all codim 1 faces
		for(int fct = 0; fct < codimOneCones.rows(); fct++) {
			//dbgtrace << "Facet: " << fct << endl;

			Set<int> adjacentCones = coneIncidences.row(fct);
			for(Entire<Set<int> >::iterator mc = entire(adjacentCones); !mc.at_end(); ++mc) {
				//dbgtrace << "Maxcone " << *mc << endl;
				Vector<Rational> normalvector(latticeNormals[std::make_pair(fct,*mc)]);
				normalvector = tdehomog_vec(normalvector);
				//Compute the representation of the normal vector
				summap[std::make_pair(fct,*mc)]= functionRepresentationVector(
						maximalCones.row(*mc),
						normalvector,
						rays, linealitySpace);
			}

			//Now compute the representation of the sum of the normals
			try {
				summatrix = summatrix / functionRepresentationVector(
						codimOneCones.row(fct),
						normalsums.row(fct),
						rays, linealitySpace);
			}
			catch(std::runtime_error &e) { //This goes wrong, if X is not balanced at a given codim 1 face
				summatrix /= zero_vector<Rational>(rays.rows() + lineality_dim);
			}
		}

		//Set fan properties
		cycle.take("LATTICE_NORMAL_FCT_VECTOR") << summap;
		cycle.take("LATTICE_NORMAL_SUM_FCT_VECTOR") << summatrix; 
	}//END computeLatticeFunctionData


	Matrix<Integer> lattice_basis_of_cone(const Matrix<Rational> &rays, const Matrix<Rational> &lineality, 
			int dim, bool has_leading_coordinate) {
		//Special case: If the cone is full-dimensional, return the standard basis
		int ambient_dim = std::max(rays.cols(), lineality.cols()) - (has_leading_coordinate? 1 : 0);
		if(dim == ambient_dim)
			return unit_matrix<Integer>(ambient_dim);
		//Compute span matrix if there is a leading coordinate 
		Matrix<Rational> span_matrix(0, ambient_dim);
		if(has_leading_coordinate) {
			std::pair<Set<int>, Set<int> > sortedVertices = far_and_nonfar_vertices(rays);
			span_matrix = rays.minor(sortedVertices.first,~scalar2set(0)); 
			if(lineality.rows() > 0) span_matrix /= lineality.minor(All,~scalar2set(0));
			int first = *(sortedVertices.second.begin());
			sortedVertices.second -= first;
			for(Entire<Set<int> >::iterator vtx = entire(sortedVertices.second); !vtx.at_end(); vtx++) {
				span_matrix /= (rays.row(*vtx) - rays.row(first)).slice(~scalar2set(0));
			}
		}
		else {
			span_matrix = rays / lineality;
		}
		//Compute span of cone
		Matrix<Rational> linspan = null_space(span_matrix);
		SparseMatrix<Integer> transformation =
			polymake::common::hermite_normal_form( make_rowwise_integer(linspan),false).second;
		//The last dim columns are a Z-basis for the cone
		return Matrix<Integer>(T(transformation.minor(All,sequence(transformation.cols()-dim,dim))));
	}//END lattice_basis_of_cone

	/*
	 * @brief Computes properties [[LATTICE_BASES]] and [[LATTICE_GENERATORS]]
	 */
	void computeLatticeBases(perl::Object cycle) {
		//dbgtrace << "Computing lattice " << endl;
		//Extract properties
		Matrix<Rational> rays = cycle.give("VERTICES");
		rays = tdehomog(rays);
		rays = rays.minor(All,~scalar2set(0));
		Matrix<Rational> linspace = cycle.give("LINEALITY_SPACE");
		linspace = tdehomog(linspace);
		linspace = linspace.minor(All,~scalar2set(0));
		IncidenceMatrix<> cones = cycle.give("MAXIMAL_POLYTOPES");
		//FIXME Use unimodularity?
		Set<int> directional = cycle.give("FAR_VERTICES");
		Set<int> vertices = sequence(0,rays.rows()) - directional; 
		int dim = cycle.give("PROJECTIVE_DIM");

		Matrix<Integer> generators;
		Vector<Set<int> > bases;

		//Iterate all cones
		for(int mc = 0; mc < cones.rows(); mc++) {
			//Compute a lattice basis for the cone:
			//Construct ray matrix of cone:
			Matrix<Rational> mc_rays = rays.minor(cones.row(mc) * directional, All);      
			Matrix<Rational> mc_vert = rays.minor(cones.row(mc) * vertices,All);
			for(int v = 1; v < mc_vert.rows(); v++) {
				mc_rays /= (mc_vert.row(v) - mc_vert.row(0));
			}

			Matrix<Integer> basis;
			//FIXME Use unimodularity?
			basis = lattice_basis_of_cone(mc_rays, linspace,dim,false);
			basis = zero_vector<Integer>(basis.rows()) | basis;

			Set<int> basis_set;
			//Add rays, looking for doubles
			for(int b = 0; b < basis.rows(); b++) {
				//We normalize s.t. the first non-zero entry is > 0
				for(int c = 0; c < basis.cols(); c++) {
					if(basis(b,c) != 0) {
						if(basis(b,c) < 0) {
							basis.row(b) *= -1;
						}
						break;
					}
				}
				int ray_index = -1;
				for(int i = 0; i < generators.rows(); i++) {
					if(generators.row(i) == basis.row(b)) {
						ray_index = i; break;
					}
				}
				if(ray_index == -1) {
					generators /= basis.row(b);
					ray_index = generators.rows()-1;
				}
				basis_set += ray_index;
			}//END go through basis elements
			bases |= basis_set;
		}//END iterate all maximal cones

		//Set properties
		cycle.take("LATTICE_GENERATORS") << thomog(generators);
		cycle.take("LATTICE_BASES") << bases;
	}

	Function4perl(&computeLatticeNormalSum,"computeLatticeNormalSum(Cycle)");
	Function4perl(&computeLatticeFunctionData,"computeLatticeFunctionData(Cycle)");
	Function4perl(&computeLatticeBases, "computeLatticeBases(Cycle)");
	Function4perl(&lattice_basis_of_cone,"lattice_basis_of_cone(Matrix,Matrix,$,$)");

}}
