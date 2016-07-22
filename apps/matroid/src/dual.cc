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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/Rational.h"

namespace polymake { namespace matroid {

perl::Object dual(perl::Object m)
{
  perl::Object m_new("Matroid");
  const int n=m.give("N_ELEMENTS");
  m_new.set_description()<<"Dual matroid of "<<m.name()<<"."<<endl;
  m_new.take("N_ELEMENTS")<<n;

  bool found_defined = false;

  int rank;
  if (m.lookup("RANK")>>rank) {
     m_new.take("RANK")<<n-rank;
  }

  Array<std::string> labels;
  if ((m.lookup("LABELS")>>labels)) {
     m_new.take("LABELS")<<labels;
  }


  Array<Set<int> > circuits;
  if (m.lookup("COCIRCUITS")>>circuits) m_new.take("CIRCUITS")<<circuits;
  if (m.lookup("CIRCUITS")>>circuits) {
     m_new.take("COCIRCUITS")<<circuits;
     found_defined = true;
  }

  Matrix<Rational> points;
  if (m.lookup("VECTORS")>>points) {
    m_new.take("VECTORS")<<T(null_space(T(points)));
    found_defined = true;
  }

  //Bases are also a fallback which we always compute if neither CIRCUITS nor VECTORS are present.
  Array<Set<int> > bases;
  bool has_bases = (m.lookup("BASES")>>bases);
  if (!found_defined || has_bases) {
    if(!found_defined) bases = m.give("BASES");
    Array<Set<int> > new_bases(bases.size());
    for (int i=0;i<bases.size();++i)
      new_bases[i]=sequence(0,n)-bases[i];
    m_new.take("BASES")<<new_bases;
  }

  if (m.lookup("NON_BASES")>>bases) {
    Array<Set<int> > new_bases(bases.size());
    for (int i=0;i<bases.size();++i)
      new_bases[i]=sequence(0,n)-bases[i];
    m_new.take("NON_BASES")<<new_bases;
  }

  return m_new;
}

UserFunction4perl("# @category Producing a matroid from matroids"
                  "# Produces the __dual__ of a given matroid //m//."
                  "# @param Matroid m"
                  "# @return Matroid",
                  &dual, "dual(Matroid)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
