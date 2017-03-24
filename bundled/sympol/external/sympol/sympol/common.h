// ---------------------------------------------------------------------------
//
// This file is part of SymPol
//
// Copyright (C) 2006-2010  Thomas Rehn <thomas@carmen76.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// ---------------------------------------------------------------------------

#ifndef SYMPOL_COMMON_H_
#define SYMPOL_COMMON_H_

#include <boost/dynamic_bitset.hpp>
#include <set>
#include <map>
#include <iostream>

#include "config.h"
#include "types.h"

#include <permlib/permutation.h>
//#include <permlib/transversal/explicit_transversal.h>
#include <permlib/transversal/schreier_tree_transversal.h>
#include <permlib/bsgs.h>

namespace sympol {

typedef boost::dynamic_bitset<> Face;
typedef permlib::Permutation PERM;
//typedef permlib::ExplicitTransversal<PERM> TRANSVERSAL;
typedef permlib::SchreierTreeTransversal<PERM> TRANSVERSAL;
typedef permlib::BSGS<PERM,TRANSVERSAL> PermutationGroup;

struct FaceAction {
	Face operator()(const PERM &p, const Face &f) const {
		Face ret(f.size());
		for (uint i = 0; i < f.size(); ++i) {
			if (f[i])
				ret.set(p / i, 1);
		}
		return ret;
	}
};

enum SymmetryComputationMethod { DIRECT, ADM, IDM };

}

#endif // COMMON_H_
