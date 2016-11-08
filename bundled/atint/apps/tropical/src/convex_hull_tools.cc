
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

	Implements convex_hull_tools.h
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/FacetList.h"
#include "polymake/polytope/canonicalize.h"
#include "polymake/polytope/cdd_interface.h"
#include "polymake/tropical/convex_hull_tools.h"
#include "polymake/tropical/solver_def.h"
#include "polymake/tropical/thomog.h"

namespace polymake { namespace tropical {



	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see header
	Vector<int> insert_rays(Matrix<Rational> &rays, Matrix<Rational> nrays, bool is_normalized) {
		//Normalize new rays, if necessary
		if(!is_normalized) {
			cdd_normalize_rays(nrays);
		}

		//Insert rays
      std::vector<int> new_ray_indices;
		for(auto nr = entire(rows(nrays)); !nr.at_end(); ++nr) {
			int new_rayindex = -1;
			for(auto oray = ensure(rows(rays), (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin(); 
               !oray.at_end(); ++oray) {
				if(*oray == *nr) {
					new_rayindex = oray.index(); break;
				}
			}
			if(new_rayindex == -1) {
				rays /= *nr; 
				new_rayindex = rays.rows()-1;
			}
			new_ray_indices.push_back(new_rayindex);
		}

		return Vector<int>(new_ray_indices);
	}//END insert_rays

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see header
   template <typename MType> inline
      void cdd_normalize_rays(GenericMatrix<MType> &rays) {
         for (auto r = entire(rows(rays)); !r.at_end(); ++r) {
            polytope::canonicalize_oriented( find_in_range_if(entire(r->top()), operations::non_zero()));
         }
      }

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see header
	std::pair<Matrix<Rational>, Matrix<Rational> > cdd_cone_intersection(
			const Matrix<Rational> &xrays, const Matrix<Rational> &xlin, 
			const Matrix<Rational> &yrays, const Matrix<Rational> &ylin) {

		solver<Rational> sv;


		//Compute facets
		std::pair<Matrix<Rational>, Matrix<Rational> > x_eq =
			sv.enumerate_facets(xrays, xlin,false,false);
		std::pair<Matrix<Rational>, Matrix<Rational> > y_eq =
			sv.enumerate_facets(yrays, ylin,false,false);

		//Compute intersection rays
		std::pair<Matrix<Rational>, Matrix<Rational> > inter;
		try {
			inter = sv.enumerate_vertices(x_eq.first / y_eq.first, x_eq.second / y_eq.second, false,true);
		}
		catch(const infeasible&) {
			inter.first = Matrix<Rational>(0,std::max(xrays.cols(), xlin.cols()));
			inter.second = Matrix<Rational>(0,std::max(xrays.cols(), xlin.cols()));
		}

		//normalize
		cdd_normalize_rays(inter.first);

		return inter;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see header
	fan_intersection_result cdd_fan_intersection(	
			const Matrix<Rational> &xrays, const Matrix<Rational> &xlin, const IncidenceMatrix<> &xcones,
			const Matrix<Rational> &yrays, const Matrix<Rational> &ylin, const IncidenceMatrix<> &ycones) {

		solver<Rational> sv;

		//Precompute h-representations of the x-cones and y-cones
      std::vector<std::pair<Matrix<Rational>, Matrix<Rational> > > xequations;
		for(auto xc = entire(rows(xcones)); !xc.at_end(); ++xc) {
			xequations.push_back(sv.enumerate_facets(xrays.minor(*xc,All),xlin,false,false));
		}
      std::vector<std::pair<Matrix<Rational>, Matrix<Rational> > > yequations;
		for(auto yc = entire(rows(ycones)); !yc.at_end(); ++yc) {
			yequations.push_back( sv.enumerate_facets(yrays.minor(*yc,All),
					ylin,false,false));
		}

		//Now compute intersections
		Matrix<Rational> interrays;
		Matrix<Rational> interlineality;
		bool lineality_computed = false;
      std::vector<Set<int> > intercones;

      std::vector<Set<int> > xcontainers;
      std::vector<Set<int> > ycontainers;

		for(auto xc = ensure(xequations, (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin(); 
            !xc.at_end();++xc) {
         for(auto yc = ensure(yequations, (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin(); 
            !yc.at_end();++yc) {
				//Compute intersection
				std::pair<Matrix<Rational>, Matrix<Rational> > inter;
				try {
					inter = sv.enumerate_vertices( (*xc).first / (*yc).first,
							(*xc).second / (*yc).second,
							false,true);
				}
				catch(const infeasible&) {
					inter.first = Matrix<Rational>(0,std::max(xrays.cols(), xlin.cols()));
					inter.second = Matrix<Rational>(0,std::max(xrays.cols(), xlin.cols()));
				}

				if (!lineality_computed) {
					interlineality = inter.second.rows() > 0 ? 
						inter.second :
						Matrix<Rational>();
					lineality_computed = true;
				}

				//The empty cone will not be included 
				if (inter.first.rows() == 0) continue;

				//If cone contains no vertices (i.e. the intersection is actually 
				//empty), we leave it out
				if (is_zero(inter.first.col(0))) continue;

				cdd_normalize_rays(inter.first);

				//Insert rays into ray list and create cone
				Set<int> new_cone_set ( insert_rays( interrays, inter.first, true));

				//Make sure we don't add a cone twice
				//Also: Remember intersections that are contained in this one or contain this one
				Set<int> containedCones;
				Set<int> containerCones;
				int new_cone_index = -1;
				for ( auto ic = ensure( intercones, (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin();
                     !ic.at_end(); ic++) {
					int cmp_set = incl( *ic , new_cone_set);
               if (cmp_set == 0) new_cone_index = ic.index(); 
					if (cmp_set == -1) containedCones += ic.index();
				   if (cmp_set == 1) containerCones += ic.index();
				}
				if (new_cone_index == -1) {
					intercones.push_back(new_cone_set);
					new_cone_index = intercones.size()-1;
					xcontainers.push_back(Set<int>());
					ycontainers.push_back(Set<int>());
				}

				//First add all containers from the containing cones
				for (auto sup = entire(containerCones); !sup.at_end(); sup++) {
					xcontainers[new_cone_index] += xcontainers[*sup];
					ycontainers[new_cone_index] += ycontainers[*sup];
				}
				//Add xc and yc as containers
				xcontainers[new_cone_index] += xc.index();
				ycontainers[new_cone_index] += yc.index();
				//Add all current containers to the contained cones
				for (auto sub = entire(containedCones); !sub.at_end(); sub++) {
					xcontainers[*sub] += xcontainers[new_cone_index];
					ycontainers[*sub] += ycontainers[new_cone_index];
				}



			}//END iterate ycones
		}//END iterate xcones

		//Create result:
		fan_intersection_result f;
		f.rays = interrays;
		if(interlineality.rows() == 0) interlineality = Matrix<Rational>(0,interrays.cols());
		f.lineality_space = interlineality;
		f.cones = IncidenceMatrix<>(intercones);
		f.xcontainers = IncidenceMatrix<>(xcontainers);
		f.ycontainers = IncidenceMatrix<>(ycontainers);
		return f;

	}

	/*
	 * @brief Computes the set-theoretic intersection of two Cycles and returns it as a 
	 * PolyhedralComplex
	 * @param Cycle A
	 * @param Cycle B
	 * @return PolyhedralComplex in non-tropical-homogeneous coordinates
	 */
	perl::Object set_theoretic_intersection(perl::Object A, perl::Object B) {
		//Extract results
		const Matrix<Rational> &arays = A.give("VERTICES");
		const IncidenceMatrix<> &acones = A.give("MAXIMAL_POLYTOPES");
		const Matrix<Rational> &alineality = A.give("LINEALITY_SPACE");

		const Matrix<Rational> &brays = B.give("VERTICES");
		const IncidenceMatrix<> &bcones = B.give("MAXIMAL_POLYTOPES");
		const Matrix<Rational> &blineality = B.give("LINEALITY_SPACE");

		fan_intersection_result result = cdd_fan_intersection(arays,alineality,acones, brays,blineality,bcones);

		//Check for contained cones
      FacetList flist;
      for(auto c = entire(rows(result.cones)); !c.at_end(); ++c) flist.insertMax(*c);

		perl::Object p("fan::PolyhedralComplex");
			p.take("VERTICES") << tdehomog(result.rays);
			p.take("MAXIMAL_POLYTOPES") << flist;
			p.take("LINEALITY_SPACE") << tdehomog(result.lineality_space);
		return p;
	}


	// ------------------------- PERL WRAPPERS ---------------------------------------------------

	Function4perl(&cdd_cone_intersection, "cdd_cone_intersection(Matrix<Rational>,Matrix<Rational>,Matrix<Rational>,Matrix<Rational>,$)");

	FunctionTemplate4perl("cdd_normalize_rays(Matrix<Rational>)");

	UserFunction4perl("# @category Basic polyhedral operations"
			"# Computes the set-theoretic intersection of two cycles and returns it as a polyhedral complex."
			"# The cycles need not use the same tropical addition"
			"# @param Cycle A"
			"# @param Cycle B"
			"# @return fan::PolyhedralComplex The set-theoretic intersection of the supports of A and B",
			&set_theoretic_intersection, "set_theoretic_intersection(Cycle,Cycle)");

}}
