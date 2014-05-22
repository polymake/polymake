/* Copyright (c) 1997-2014
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
#include "polymake/group/permlib.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/linalg.h"
#include "polymake/common/lattice_tools.h"

namespace polymake { namespace polytope {
namespace {


  bool are_equiv(const Vector<Rational>& vec1, const Vector<Rational>& vec2, const Matrix<Rational>& nullspace) {
    bool answer=false;
    if(nullspace.rows()==0){ //full-dim
      answer=(vec1==vec2);
    } else {
      Vector<Rational> diff=vec1-vec2;
      if(nullspace*diff==zero_vector<Rational>(nullspace.rows())){
	answer=true;
      }
    }
    return answer;
  }
}

  typedef std::pair< Matrix<Rational> , Array< Set<int> > > MatrixOrbitPair;

    //works for both facets/linear span and rays/lineality space
  MatrixOrbitPair symmetrize_poly_reps(const Matrix<Rational>& facets_in, const Matrix<Rational>& linspan, perl::Object group) {
    using namespace polymake::group;
    Matrix<Rational> facets(common::primitive(facets_in));
    Matrix<Rational> nullspace(null_space(linspan));
    Matrix<Rational> symmetric_facets(facets.rows(),facets.cols());
    std::list< Set<int> > facet_orbit_list;
    Set<int> not_checked(range(0,facets.rows()-1));
    
    for(int i=0;i<facets.rows();++i) {
      if(not_checked.contains(i)) {
	symmetric_facets.row(i)=facets.row(i);
	not_checked-=i;
	Set<int> cur_facet_orbit;
	cur_facet_orbit+=i;
	std::pair< ListMatrix< Vector<Rational> >,Array < Set<int> > > orbit = orbits_coord_action_complete_sub(group,vector2row(facets.row(i)));
	for(Entire< Rows< ListMatrix< Vector<Rational> > > >::const_iterator row=entire(rows(orbit.first)); !row.at_end(); ++row) {
	  for(Entire< Set <int> >::iterator index=entire(not_checked);!index.at_end(); ++index) {
	    if(are_equiv(*row,facets.row(*index),nullspace)) {
	      cur_facet_orbit+=*index;
	      symmetric_facets.row(*index)=*row;
	      not_checked.erase(index);
	      break;
	    }	      
	  }
	}
	facet_orbit_list.push_back(cur_facet_orbit);
      }
    }
    Array< Set<int> > facet_orbits(facet_orbit_list);
    MatrixOrbitPair result(symmetric_facets,facet_orbits);
    return result;
  }


    Function4perl(&symmetrize_poly_reps,"symmetrize_poly_reps(Matrix, Matrix, group::GroupOfCone)");

}}
