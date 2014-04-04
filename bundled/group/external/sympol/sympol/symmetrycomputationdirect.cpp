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

#include "symmetrycomputationdirect.h"
#include "facesuptosymmetrylist.h"

#include <vector>

using namespace yal;
using namespace sympol;

LoggerPtr SymmetryComputationDirect::logger(Logger::getLogger("SymCompDir"));

SymmetryComputationDirect::SymmetryComputationDirect(RecursionStrategy* const recursionStrategy, 
                                                     const RayComputation* rayCompDefault, const Polyhedron & data, 
                                                     const PermutationGroup & permGroup, FacesUpToSymmetryList& rays)
        : SymmetryComputation(DIRECT, recursionStrategy, rayCompDefault, data, permGroup, rays) 
{ }

double SymmetryComputationDirect::probe(const Polyhedron & data, const PermutationGroup & permGroup, 
            std::list<FaceWithData> & rays) const {
	return m_rayCompDefault->estimate(m_data, rays);
}

bool SymmetryComputationDirect::enumerateRaysUpToSymmetry() {
	logger->logUsageStats();
	YALLOG_DEBUG(logger, "start Direct");
	//
	// compute dual description
	//
	std::vector<FaceWithDataPtr> localRays;
	if (!m_rayCompDefault->dualDescription(m_data, localRays)) {
		return false;
	}

	for (ulong m = 0; m < localRays.size(); ++m) {
		bool inequivalentRay = m_rays.add(localRays[m]);
		if (inequivalentRay) {
			YALLOG_DEBUG2(logger, m_rayCompDefault->name() << " found new " << localRays[m]->face << " ~~ " << *localRays[m]->ray);
		} else {
			YALLOG_DEBUG2(logger, m_rayCompDefault->name() << " rejected " << localRays[m]->face);
		}
	}

	return true;

}
