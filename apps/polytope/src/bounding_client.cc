/* Copyright (c) 1997-2019
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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/AccurateFloat.h"
#include "polymake/linalg.h"
#include "polymake/common/bounding_box.h"
#include "polymake/polytope/solve_LP.h"
#include "polymake/polytope/convex_hull.h"

namespace polymake { namespace polytope {

template <typename Scalar>
Matrix<Scalar>
bounding_box_facets(const Matrix<Scalar>& V, perl::OptionSet options)
{
	const Scalar offset = options["offset"];
   if (offset<0) throw std::runtime_error("bounding_box_facets: offset must be non-negative");
   const Scalar surplus_k = options["surplus_k"];
   if (surplus_k<0) throw std::runtime_error("bounding_box_facets: surplus value must be non-negative");
   const bool fulldim = options["fulldim"];
   const bool make_cube = options["make_cube"];
   
   const int d = V.cols();
	const Set<int> boundedV = indices(attach_selector(V.col(0), operations::non_zero()));
   if (!boundedV.size()) throw std::runtime_error("bounding_box_facets: no bounded vertices");
   Matrix<Scalar> MM = common::bounding_box(V.minor(boundedV,All));

   Vector<Scalar> total_offsets = surplus_k*(MM.row(1)-MM.row(0)) + offset*(0|ones_vector<Scalar>(d-1));
   // the following is useful for visualization if vertices do not span space
   if (fulldim) 
      for (int j=1; j<d; ++j)
         if (total_offsets[j]==0) total_offsets[j]=1;
   
   Matrix<Scalar> SP(0,d);
   SP /= -total_offsets;
   SP /= total_offsets;
   MM += SP;
	
	if (make_cube) {
		Scalar total_min = accumulate( MM.row(0).slice(range_from(1)), operations::min());
		Scalar total_max = accumulate( MM.row(1).slice(range_from(1)), operations::max());
		MM[0]=(MM[0][0] | ones_vector<Scalar>(d-1)*total_min);
		MM[1]=(MM[1][0] | ones_vector<Scalar>(d-1)*total_max);
	}
   
   ListMatrix< Vector<Scalar> > af(0,d);
   for (int j=1; j<d; ++j) {
      af /= ( MM[1][j] | -unit_vector<Scalar>(d-1,j-1));
      af /= (-MM[0][j] |  unit_vector<Scalar>(d-1,j-1));
   }

   return af;
} //bounding_box_facets

UserFunctionTemplate4perl("# @category Visualization"
                  "# Introduce artificial boundary facets (which are always vertical,"
                  "# i.e., the last coordinate is zero) to allow for bounded images of "
                  "# unbounded polyhedra (e.g. Voronoi polyhedra)."
                  "# @param Scalar offset the minimum offset between a bounding box facet and its nearest bounded vertex"
                  "# @param Matrix V vertices that should be in the box"
                  "# @param Scalar surplus_k size of the bounding box relative to the box spanned by //V// (added to offset)"
                  "# @param Bool fulldim keeps the bounding box full dimensional even if the bounded vertices do not span the whole space and offset is zero. Useful for visualizations of Voronoi diagrams that do not have enough vertices. Default value is 0."
                  "# @param Bool make_cube"
                  "# @return Matrix",
                  "bounding_box_facets<Scalar>(Matrix<Scalar>; { offset => 0, surplus_k => 0, fulldim => 0, make_cube => 0 })");

template <typename Scalar>
Scalar 
l2norm(const Vector<Scalar>& v) 
{	
	//is this really the right way to handle a possible conversion in order to take the sqrt?
	return convert_to<Scalar>(sqrt(convert_to<AccurateFloat>(sqr(v))));
}

template <typename Scalar, typename T1, typename T2>
Matrix<Scalar>
bounding_facets(const GenericMatrix<T1>& F, const GenericMatrix<T2>& V, perl::OptionSet options)
{	
	const Scalar offset = options["offset"];
   if (offset<0) throw std::runtime_error("bounding_facets: offset must be non-negative");
   const Scalar surplus_k = options["surplus_k"];
   if (surplus_k<0) throw std::runtime_error("bounding_facets: surplus value must be non-negative");
   const bool transform = options["transform"];
   const bool fulldim = options["fulldim"];
   const bool ret_nonrd = options["return_nonredundant"];
	const Set<int> bounded = indices(attach_selector(V.col(0), operations::non_zero()));
   if (!bounded.size()) throw std::runtime_error("bounding_facets: no bounded vertices");
   const int d = V.cols();
   const int nrF = F.rows();
	
	auto Fs = convert_to<Scalar>(F);
	auto Vs_bd = convert_to<Scalar>(V.minor(bounded,All));
	//std::transform doesn't like this to be auto
	const Matrix<Scalar> Fs_normals(Fs.minor(All,range_from(1)));
	if (offset==0 && rank(Vs_bd)<2) throw std::runtime_error("bounding_facets: 0 vertices or 1 vertex and no offset is a bad request");
	const Matrix<Scalar> prod = Vs_bd.minor(All,range_from(1))*T(Fs_normals);
   const Matrix<Scalar> fb = common::bounding_box(prod);
	Vector<Scalar> extends = fb.row(1)-fb.row(0);
	
	if (fulldim) { 
      for (int j=0; j<nrF; ++j) {
         if (extends[j]==0) extends[j]=1;
		}
	}
	Vector<Scalar> total_offsets = fb.row(0)-surplus_k*extends;
	
	if (offset!=0) {
		Vector<Scalar> norms(nrF);
		Vector<Scalar> rnorms(nrF);
		std::transform(rows(Fs_normals).begin(), rows(Fs_normals).end(), norms.begin(), l2norm<Scalar>);
		total_offsets-=offset*norms;
	}

	if (transform) {	
		perl::Object lpp("Polytope", mlist<Scalar>());

		const Vector<Scalar> obj(unit_vector<Scalar>(d+1, d));
		const Matrix<Scalar> ineqs = Fs|total_offsets;
		const Matrix<Scalar> eqs(0,d+1);
		
		lpp.take("INEQUALITIES") << ineqs;
		lpp.take("EQUATIONS") << eqs;
		perl::Object lp = lpp.add("LP");
		lp.take("LINEAR_OBJECTIVE") << obj;
		const Matrix<Scalar> LPV = lpp.give("VERTICES");

		Set<int> max_face = lp.give("MAXIMAL_FACE");
		Vector<Scalar> bc = barycenter(LPV.minor(max_face,All));
		const Matrix<Scalar> trafo = bc.slice(range(0,d-1))|(zero_vector<Scalar>()/unit_matrix<Scalar>(d-1)*bc[d]);
		Matrix<Scalar> BF = Fs*trafo;
		return BF;
	} else {
		Matrix<Scalar> BF = -total_offsets|Fs_normals;
		const Matrix<Scalar> eqs(0,d);
	   if (ret_nonrd) {
	   	std::pair<Bitset, Set<int>> BF_nonrd = get_non_redundant_inequalities(BF, eqs, true);
			BF = BF.minor(BF_nonrd.first,All);
		}
		return BF;
	}
} //bounding_facets

UserFunctionTemplate4perl("# @category Visualization"
                  "# A function to construct the H-description of a bounding polytope BP for a given set of vertices."
                  "# @param Matrix H H-description of some bounded polytope P"
                  "# @param Matrix V vertices of which the bounded ones will be contained in BP"
                  "# @param Scalar offset the minimum euclidean distance between a hyperplane and a bounded vertex. Default is 0"
                  "# @param Scalar surplus_k factor multiplied with $ max(<f,v> | v in V) - min(<f,v> | v in V) $ to describe the minimum offset relative to the extents of //V// in f direction (added to offset)"
                  "# @param Bool transform instead of simply shifting the facets. For P simplicial/(and simple?) this should produce the same as the LP and can be turned off. Default is true"
                  "# @param Bool fulldim keep BP full dimensional. Default is false"
                  "# @param Bool return_nonredundant (shifted) hyperplanes only. If transform is true there will be no check. Regardless of this variable. Default is true"
                  "# @return Matrix BF bounding facets containing all bounded points of V",
                  "bounding_facets<Scalar>(Matrix<type_upgrade<Scalar>> Matrix<type_upgrade<Scalar>>; { offset => 0, surplus_k => 0, transform => 1, fulldim => 0, return_nonredundant => 1 })");

} //namespace polytope 
} //namespace polymake 

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
