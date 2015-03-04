/* Copyright (c) 1997-2015
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
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/GenericMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

template <typename MatrixTop , typename Triangulation>
void centroid_volume(perl::Object p, const GenericMatrix<MatrixTop>& Points, const Triangulation& tr)
{
   typedef typename MatrixTop::element_type Coord;
   Coord volume(0);
   Vector<Coord> centroid(Points.cols());
   int d=tr.front().size()-1;
     
   for (typename Entire<Triangulation>::const_iterator s=entire(tr); !s.at_end(); ++s) {
      const typename MatrixTop::persistent_type sim=Points.minor(*s,All);
      Coord v=abs(det(sim));
      volume += v;
      Vector<Coord> b(Points.cols());
      for (typename Entire< Rows<MatrixTop> >::const_iterator i=entire(rows(sim));!i.at_end(); ++i)
         b+=*i;
      centroid+=v*b;
   }
   centroid/=volume*(d+1);
   volume /= Integer::fac(d);

   p.take("CENTROID")<<centroid;
   p.take("VOLUME") << volume;
}
  
FunctionTemplate4perl("centroid_volume(Polytope Matrix Array<Set<Int> >) : void");
FunctionTemplate4perl("centroid_volume(Polytope SparseMatrix Array<Set<Int>>) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
