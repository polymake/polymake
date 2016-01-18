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

	Functions to create special morphisms.
	*/

#ifndef POLYMAKE_ATINT_MORPHISM_SPECIAL_H
#define POLYMAKE_ATINT_MORPHISM_SPECIAL_H

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/Map.h"

namespace polymake { namespace tropical {


	//Documentation see perl wrapper
	template <typename Addition>
		perl::Object evaluation_map(int n,  Matrix<Rational> delta, int i) {
			if(n <= 0 || delta.rows() <= 0 || i <= 0 || i > n) {
				throw std::runtime_error("Cannot create evaluation map: Invalid parameters");
			}

			//Dimension of the target space
			int r = delta.cols()-1;

			//We create the map on the 


			//The matrix of ev_i is pr(R^r) + G*pr(M_0,N), where
			//pr denotes the projection (to homogeneous coordinates!) 
			//and G is the matrix evaluating the differences between the marked points
			//i and n (which is the special leaf).
			//Since we take ev_n as the special evaluation, the map reads under moduli coordinates:
			// R^((n-1) over 2) -> R^r, x |-> - sum_(k=1)^|delta| x_{ik} v_k (where v_k is the k-th
			// row of delta). 

			//Projection matrices
			int N = n + delta.rows();
			int modulidim = (N*(N-3))/2+1; 

			//The projection to R^r needs the first coordinate as homogenizing coordinate.
			Matrix<Rational> projR = Matrix<Rational>(r,modulidim) | unit_matrix<Rational>(r);
			projR = unit_vector<Rational>(projR.cols(),0) / projR;


			Matrix<Rational> projM = unit_matrix<Rational>(modulidim) | Matrix<Rational>(modulidim,r);

			Matrix<Rational> G(r+1,modulidim);
			if(i < n) {    //If i is the special leaf, G is the zero map
				//First we create the edge index matrix E(i,j) that contains at element i,j 
				//the edge index of edge (i,j)
				//in the complete graph on N-1 nodes (in lexicographic order).
				int nextindex = 0;
				Matrix<int> E(N-1,N-1);
				for(int k = 0; k < N-2; k++) {
					for(int j = k+1; j < N-1; j++) {
						E(k,j) = nextindex;
						E(j,k) = nextindex;
						nextindex++;
					}
				}	
				//Create index map (i,j) -> index(i,j)
				int evalIndex = delta.rows() + i-1;
				//Set all rows corr. to coordinate (n+i,k) to -  v_k
				for(int k = 0; k < delta.rows(); k++) {
					G.col(E(evalIndex,k)) = Addition::orientation() * delta.row(k);
				}
			}

			Matrix<Rational> map_matrix = projR + G*projM;

			perl::Object morphism(perl::ObjectType::construct<Addition>("Morphism"));
			morphism.take("MATRIX") << map_matrix;

			return morphism;
		}


	//Documentation see perl wrapper
	template <typename Addition>
		perl::Object evaluation_map_d(int n, int r, int d, int i) {
			if(n <= 0 || r <= 0 || d <= 0 || i <= 0 || i > n) {
				throw std::runtime_error("Cannot create evaluation map: Invalid parameters");
			}

			//Create standard d-fold direction matrix
			Matrix<Rational> delta(0,r+1);
			for(int k = 0; k <= r; k++) {
				for(int j = 1; j <= d; j++) {
					delta /= unit_vector<Rational>(r+1,k);
				}
			}

			//dbgtrace << "Delta: " << delta << endl;

			return evaluation_map<Addition>(n,delta,i);
		}


	//Documentation see perl wrapper
	template <typename Addition>
		perl::Object projection_map(int n, Set<int> coords) {

			//Create matrix
			Matrix<Rational> proj_matrix(coords.size(), n+1);
			int image_index = 0;
			for(Entire<Set<int> >::iterator c = entire(coords); !c.at_end(); c++) {
				if(*c > n) {
					throw std::runtime_error("Cannot create projection: Image dimension larger than domain dimension");
				}
				proj_matrix.col(*c) = unit_vector<Rational>(coords.size(),image_index);
				image_index++;
			}

			perl::Object result(perl::ObjectType::construct<Addition>("Morphism"));
			result.take("MATRIX") << proj_matrix;

			return result;
		}

	//Documentation see perl wrapper
	template <typename Addition>
		perl::Object projection_map_default(int n, int m) {
			if(m > n) {
				throw std::runtime_error("Cannot create projection: Image dimension larger than domain dimension");
			}
			return projection_map<Addition>(n, sequence(0,m+1));
		}



	//Documentation see perl wrapper
	template <typename Addition>
		perl::Object forgetful_map(int n, Set<int> leaves_to_forget) {

			//First check, that the leaves are in (1,..,n)
			if((leaves_to_forget * sequence(1,n)).size() < leaves_to_forget.size()) {
				throw std::runtime_error("Cannot compute forgetful map: The forgotten leaves should be in {1,..,n}");
			}

			//Compute domain and image dimension
			int domain_dim = (n*(n-3))/2 +1;
			int small_n = n - leaves_to_forget.size();
			int image_dim = (small_n*(small_n - 3))/2 + 1;



			//Check if we forget so many leaves that we get the zero map
			if(small_n <= 3) {
				perl::Object result(perl::ObjectType::construct<Addition>("Morphism"));
				result.take("MATRIX") << Matrix<Rational>(small_n == 3? 1 : 0,domain_dim);
				return result;
			}
			//Check if we don't forget anything at all
			if(leaves_to_forget.size() == 0) {
				perl::Object result(perl::ObjectType::construct<Addition>("Morphism"));
				Matrix<Rational> um = unit_matrix<Rational>(domain_dim);
				result.take("MATRIX") << um;
				return result;
			}

			//Prepare map mapping remaining leaves to {1,..,n-|leaves_to_forget|}
			Map<int,int> remaining_map;
			int next_index = 1;
			for(int i = 1; i <= n; i++) {
				if(!leaves_to_forget.contains(i)) {
					remaining_map[i] = next_index;
					next_index++;
				}
			}

			//This maps each edge (i,j) in the complete graph on small_n-1 vertices to an
			//index in 0 .. N. Each such edge is a flat, whose vector is (minus) a unit vector
			Map<std::pair<int,int>, int> unit_edges;
			int unit_index = 0;
			for(int i = 1; i < small_n-1; i++) {
				for(int j = i+1; j < small_n; j++) {
					unit_edges[std::make_pair(i,j)] = unit_index;
					unit_index++;
				}
			}

			//Prepare matrix representing forgetful map
			Matrix<Rational> ffm(image_dim,0);

			//Compute image of each unit vector e_i, correpsonding to all v_{k,l} (- v_kl for Max)
			//with 1 <= k < l < n
			for(int k = 1; k < n-1; k++) {
				for(int l = k+1; l < n; l++) {
					Set<int> klset; klset += k; klset +=l;
					//If klset contains forgotten leaves, the ray is mapped to zero
					if( (leaves_to_forget * klset).size() > 0) {
						ffm |= zero_vector<Rational>(image_dim);
					}
					else {
						//First we compute the new number of the leaves in the image
						int newk = remaining_map[k];
						int newl = remaining_map[l];

						//The easy case is that newl is not the new maximal leaf
						if(newl < small_n) {
							//The image is just the corr. unit vector.
							ffm |= unit_vector<Rational>(image_dim, unit_edges[std::make_pair(newk,newl)]);
						}//END if newl < small_n
						else {
							//If the new set contains the maximal leaf, we have to take the complement to 
							//compute the moduli coordinates
							Set<int> complement = (sequence(1,small_n) - newk) - newl;
							//Compute moduli coordinates as usual: Sum up over all flat vectors of the 
							//1-edge flats, i.e. all pairs contained in complement
							Vector<Rational> ray_image(image_dim);
							Vector<int> slist(complement);
							for(int i = 0; i < slist.dim(); i++) {
								for(int j = i+1; j < slist.dim(); j++) {
									//If its the last edge (small_n-2, small_n-1), we add the ones_vector, otherwise, 
									//we  substract the corr. unit_vector
									ray_image[ unit_edges[std::make_pair(slist[i], slist[j])] ] += 1;
								}
							}//END compute ray image

							ffm |= ray_image;

						}//END if newl == small_n

					}//END compute image of ray
				}//END iterate l
			}//END iterate k


			//Return result
			perl::Object result(perl::ObjectType::construct<Addition>("Morphism"));
			result.take("MATRIX") << ffm;

			return result;

		}//END function forgetful_map


}}

#endif
