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

	This file provides functionality to compute certain special tropical varieties
	*/

#ifndef POLYMAKE_ATINT_SPECIALCYCLES_H
#define POLYMAKE_ATINT_SPECIALCYCLES_H

#include "polymake/client.h"
#include "polymake/PowerSet.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"
#include "polymake/linalg.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/misc_tools.h"

namespace polymake { namespace tropical {


	template <typename Addition>
		perl::Object empty_cycle(int ambient_dim) {
			perl::Object cycle(perl::ObjectType::construct<Addition>("Cycle"));
				cycle.take("VERTICES") << Matrix<Rational>(0,ambient_dim+2);
				cycle.take("MAXIMAL_POLYTOPES") << Array<Set<int> >();
				cycle.take("WEIGHTS") << Vector<Integer>();
				cycle.take("PROJECTIVE_AMBIENT_DIM") << ambient_dim;
				cycle.set_description() << "Empty cycle in dimension " << ambient_dim;

			return cycle;
		}//END empty_cycle

	template <typename Addition>
		perl::Object point_collection(Matrix<Rational> m, Vector<Integer> weights) {
			//Sanity check
			if(m.rows() == 0)
				throw std::runtime_error("No points given.");
			if(m.rows() != weights.dim()) 
				throw std::runtime_error("Number of points does not match number of weights");
			
			//Create vertices
			m = ones_vector<Rational>(m.rows()) | m;

			//Create polytopes
			Array<Set<int> > polytopes(m.rows());
			for(int i = 0; i < polytopes.size(); i++)
				polytopes[i] = scalar2set(i);

			perl::Object cycle(perl::ObjectType::construct<Addition>("Cycle"));
				cycle.take("PROJECTIVE_VERTICES") << m;
				cycle.take("MAXIMAL_POLYTOPES") << polytopes;
				cycle.take("WEIGHTS") << weights;
		

			return cycle;
		}//END point_collection


	template <typename Addition> 
		perl::Object uniform_linear_space(const int n, const int k, Integer weight = 1) {

			//Ensure that dimensions match
			if(k > n) 
				throw std::runtime_error("Cannot create uniform linear space. Fan dimension is larger than ambient dimension.");
			if(k < 0 || n < 0) 
				throw std::runtime_error("Cannot create uniform linear space. Negative dimension provided.");
			if(k == 0) {
				return point_collection<Addition>( Matrix<Rational>(1,n+1), ones_vector<Integer>(1));	
			}

			//Create rays
			Matrix<Rational> vertices(unit_matrix<Rational>(n+1));
			vertices = zero_vector<Rational>(n+1) | vertices;
			vertices *= Addition::orientation();
			vertices = unit_vector<Rational>(n+2,0) / vertices;

			//Create cones
			Array<Set<int>> polytopes{ all_subsets_of_k(sequence(1,n+1), k) };
			for (int i = 0; i < polytopes.size(); i++) 
				polytopes[i] += 0;

			//Create weights
			Vector<Integer> weights = weight * ones_vector<Integer>(polytopes.size());

			//Create final object
			perl::Object fan(perl::ObjectType::construct<Addition>("Cycle"));
			fan.take("PROJECTIVE_VERTICES") << vertices;
			fan.take("MAXIMAL_POLYTOPES") << polytopes;
			fan.take("WEIGHTS") << weights;
			fan.set_description() << "Uniform linear space of dimension " << k << " in dimension " << n;
			return fan;

		}//END uniform_linear_space

	template <typename Addition>
		perl::Object halfspace_subdivision(const Rational& a, const Vector<Rational>& g, const Integer& weight) {
			//Sanity check
                        if (is_zero(g))
                           throw std::runtime_error("Zero vector does not define a hyperplane.");
			if (!is_zero(accumulate(g, operations::add())))
                           throw std::runtime_error("Normal vector must be homogenous, i.e. sum of entries must be zero");

			//Create vertices
			Matrix<Rational> vertices(0,g.dim());
			vertices /= g;
			vertices /= (-g);
			vertices = zero_vector<Rational>(2) | vertices;

			//Create lineality
			Matrix<Rational> lineality = zero_vector<Rational>() | null_space(g).minor(~scalar2set(0), All);

			//Compute apex
			Rational sum = accumulate(attach_operation(g,operations::square()),operations::add());
			const Vector<Rational> apex = Rational(1) | (a/sum)*g;
			vertices = apex / vertices;

			Array<Set<int> > polytopes(2);
				polytopes[0] = (sequence(0,3)-1);
				polytopes[1] = (sequence(0,3)-2);

			perl::Object cycle(perl::ObjectType::construct<Addition>("Cycle"));
				cycle.take("PROJECTIVE_VERTICES") << vertices;
				cycle.take("MAXIMAL_POLYTOPES") << polytopes;
				if(lineality.rows() > 0)
					cycle.take("LINEALITY_SPACE") << lineality;
				cycle.take("WEIGHTS") << weight * ones_vector<Integer>(2);

			return cycle;
		}//END halfspace_subdivision

	template <typename Addition>
		perl::Object projective_torus(int n, Integer weight) {
			//Sanity check
			if(n < 0) throw std::runtime_error("Negative ambient dimension is not allowed.");

			Matrix<Rational> vertex(0,n+2);
				vertex /= unit_vector<Rational>(n+2,0);
			Matrix<Rational> lineality = unit_matrix<Rational>(n);
				lineality = Matrix<Rational>(n,2) | lineality;

			Array<Set<int> > polytopes(1);
				polytopes[0] = scalar2set(0);

			perl::Object cycle(perl::ObjectType::construct<Addition>("Cycle"));
				cycle.take("PROJECTIVE_VERTICES") << vertex;
				cycle.take("MAXIMAL_POLYTOPES") << polytopes;
				cycle.take("LINEALITY_SPACE") << lineality;
				cycle.take("WEIGHTS") << (weight* ones_vector<Integer>(1));
			return cycle;

		}//END projective_torus

	template <typename Addition>
		perl::Object orthant_subdivision(Vector<Rational> point, int chart = 0, Integer weight = 1) {
			if(point.dim() <= 2) {
				throw std::runtime_error("Cannot create orthant subdivision. Vector dimension too small");
			}

			//Dehomogenize
			point = tdehomog_vec(point,chart);
			int dim = point.dim() -1;
			//Create ray matrix - first positive rays, then negative rays
			Matrix<Rational> rays = unit_matrix<Rational>(dim);
			rays /= (-unit_matrix<Rational>(dim));
			//Prepend a zero and set the vertex as last ray
			rays = zero_vector<Rational>() | rays; 
			rays = rays / point;

			//Create cones
			Set<int> seq = sequence(0,dim);
			Array<Set<int>> all_sets( pm::all_subsets(seq) ); //All possible sign choices
			Vector<Set<int>> cones;
			for(int s = 0; s < all_sets.size(); s++) {
				Set<int> rayset;
				Set<int> complement = seq - all_sets[s];
				//Add all rays from the current set with positive sign and all the others with negative sign
				rayset += all_sets[s];
				for(Entire<Set<int> >::iterator c = entire(complement); !c.at_end(); c++) {
					rayset += (*c + dim);
				}
				//Finally add the vertex
				rayset += (rays.rows()-1);
				cones |= rayset;
			}//END create cones

			Vector<Integer> weights = weight * ones_vector<Integer>(cones.dim());

			//Create result
			perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
			result.take("PROJECTIVE_VERTICES") << thomog(rays,chart);
			result.take("MAXIMAL_POLYTOPES") << cones;
			result.take("WEIGHTS") << weights;

			return result;

		}//END orthant_subdivision

	template <typename Addition>
		perl::Object affine_linear_space(const Matrix<Rational> &generators, Vector<Rational> translate = Vector<Rational>(), Integer weight = 1) {
			//Sanity check 
			if(translate.dim() > 0 && translate.dim() != generators.cols()) {
				throw std::runtime_error("affine_linear_space: Dimension mismatch.");
			}
			if(translate.dim() == 0) translate = Vector<Rational>(generators.cols());

			Matrix<Rational> vertices(1,generators.cols()+1);
				vertices(0,0) = 1;
				vertices.row(0).slice(~scalar2set(0)) = translate;
			Vector<Set<int> > polytopes;
				polytopes |= scalar2set(0);
			Vector<Integer> weights(1);
				weights[0] = weight;

			perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
				result.take("PROJECTIVE_VERTICES") << vertices;
				result.take("MAXIMAL_POLYTOPES") << polytopes;
				result.take("LINEALITY_SPACE") << (zero_vector<Rational>() | generators);
				result.take("WEIGHTS") << weights;
				
			return result;
		}

	///////////////////////////////////////////////////////////////////////////////////////
	
	template <typename Addition>
		perl::Object cross_variety(int n, int k, Rational h = 1, Integer weight = 1) {
			//Create the cube vertices
			Matrix<Rational> rays = binaryMatrix(n);
			Vector<Set<int> > cones;

			//Sanity check
			if(n < k || k < 0 || h < 0) {
				throw std::runtime_error("cross_variety: Invalid input parameters.");
			}

			//First we treat the special case of k = 0
			if(k == 0) {
				perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
					if(h == 0) {
						rays = Matrix<Rational>(1,n+1);
						rays(0,0) = 1;
					}
					else 
						rays = ones_vector<Rational>(rays.rows()) | (weight *rays);
					Vector<Set<int> > polytopes;
					for(int r = 0; r < rays.rows(); r++) 
						polytopes |= scalar2set(r);
					result.take("VERTICES") << thomog(rays);
					result.take("MAXIMAL_POLYTOPES") << polytopes;
					result.take("WEIGHTS") << weight * ones_vector<Integer>(polytopes.dim());
				return result;
			}

			//Now create the k-skeleton of the n-cube: For each n-k-set S of 0,..,n-1 and for each vertex
			// v of the n-k-dimensional cube: Insert the entries of v in S and then insert all possible 
			//vertices of the k-dimensional cube in S^c to obtain a k-dimensional face of the cube
			Array<Set<int>> nmkSets{ all_subsets_of_k(sequence(0,n), n-k) };
			Matrix<Rational> nmkVertices = binaryMatrix(n-k);
			Matrix<Rational> kVertices = binaryMatrix(k);

			for(int s = 0; s < nmkSets.size(); s++) {
				for(int v = 0; v < nmkVertices.rows(); v++) {
					Set<int> S = nmkSets[s];
					Set<int> newface;
					Vector<Rational> vertex(n);
					vertex.slice(S) = nmkVertices.row(v);
					for(int w = 0; w < kVertices.rows(); w++) {
						vertex.slice(~S) = kVertices.row(w);
						newface += binaryIndex(vertex);
					}
					cones |= newface;
				}
			}//End create k-skeleton

			int vertexnumber = rays.rows();

			//Now we also create the k-1-skeleton of the cube to compute the ray faces
			Array<Set<int>> nmlSets{ all_subsets_of_k(sequence(0,n), n-k+1) };
			Matrix<Rational> nmlVertices = binaryMatrix(n-k+1);
			Matrix<Rational> lVertices = binaryMatrix(k-1);
			Vector<Set<int> > raycones;

			for(int s = 0; s < nmlSets.size(); s++) {
				for(int v = 0; v < nmlVertices.rows(); v++) {
					Set<int> S = nmlSets[s];
					Set<int> newface;
					Vector<Rational> vertex(n);
					vertex.slice(S) = nmlVertices.row(v);
					for(int w = 0; w < lVertices.rows(); w++) {
						vertex.slice(~S) = lVertices.row(w);
						newface += binaryIndex(vertex);
					}
					raycones |= newface;
				}
			}//End create k-1-skeleton


			//We add a copy of each vertex and consider it a ray.
			//Now, for each face S of the k-1-skeleton, we add a cone that contains for each i in S:
			//The vertex i and its corresponding ray
			if(k > 0) rays = rays / rays;
			int iter = raycones.size();
			for(int c = 0; c < iter; c++) {
				Set<int> newface; 
				Set<int> cubeface = raycones[c];
				for(Entire<Set<int> >::iterator v = entire(cubeface); !v.at_end(); v++) {
					newface += *v;
					int rayindex = *v + vertexnumber;
					newface += rayindex;
				}
				cones |= newface;
			}

			//In the degenerate case, where h = 0, we replace all nonfar vertices by a single one.
			if(h == 0) {
				rays = zero_vector<Rational>(rays.cols()) / rays.minor(~sequence(0,vertexnumber),All);
				rays = unit_vector<Rational>(rays.rows(),0) | rays;
				//Re-index cones and remove bounded ones 
				Set<int> bad_indices = sequence(0,vertexnumber);
				Set<int> bounded_cones;
				for(int c = 0; c <  cones.dim(); c++) {
					Set<int> bad_of_c = cones[c] * bad_indices;
					if(bad_of_c.size() == cones[c].size()) {
						bounded_cones += c;
					}
					else {
						Set<int> rest_of_c = cones[c] - bad_indices;
						cones[c] = Set<int>();
						for(Entire<Set<int> >::iterator shiftindex = entire(rest_of_c); !shiftindex.at_end(); shiftindex++) {
							cones[c] += (*shiftindex - vertexnumber + 1);
						}
						cones[c] += 0;
					}
				}
				cones = cones.slice(~bounded_cones);
			}
			//Otherwise scale vertices appropriately
			else {
				rays.minor(sequence(0,vertexnumber),All) *= h;
				Vector<Rational> leading_coordinate = ones_vector<Rational>(vertexnumber) | zero_vector<Rational>(vertexnumber);
				rays = leading_coordinate | rays;
			}


			perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
				result.take("VERTICES") << thomog(rays);
				result.take("MAXIMAL_POLYTOPES") << cones;
				result.take("WEIGHTS") << weight * ones_vector<Integer>(cones.dim());
			return result;
		}

}}

#endif
