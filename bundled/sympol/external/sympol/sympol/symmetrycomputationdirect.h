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

#ifndef SYMMETRYCOMPUTATIONDIRECT_H
#define SYMMETRYCOMPUTATIONDIRECT_H

#include "symmetrycomputation.h"
#include "yal/logger.h"

namespace sympol {

class SymmetryComputationDirect : public SymmetryComputation {
public:
    SymmetryComputationDirect(RecursionStrategy* const recursionStrategy, const RayComputation* rayCompDefault, const Polyhedron & data, 
            const PermutationGroup & permGroup, FacesUpToSymmetryList& rays);
    
    bool enumerateRaysUpToSymmetry();
    
    double probe(const Polyhedron & data, const PermutationGroup & permGroup, 
            std::list<FaceWithData> & rays) const;
	private:
		static yal::LoggerPtr logger;
};

}

#endif
