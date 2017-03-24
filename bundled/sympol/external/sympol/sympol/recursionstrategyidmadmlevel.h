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

#ifndef RECURSIONSTRATEGY_IDMADM_LEVEL_H_
#define RECURSIONSTRATEGY_IDMADM_LEVEL_H_

#include "recursionstrategy.h"

namespace sympol {

class RecursionStrategyIDMADMLevel : public RecursionStrategy {
public:
	RecursionStrategyIDMADMLevel(uint levelIDM, uint levelADM)
		 : m_levelIDM(levelIDM), m_levelADM(levelADM)
			{}
protected:
	virtual SymmetryComputation* devise(const RayComputation* rayComp, const Polyhedron & data,
					const PermutationGroup & permGroup, FacesUpToSymmetryList& rays)
	{
		if (this->recursionDepth() < m_levelIDM) {
			YALLOG_INFO(logger, this->recursionDepth() << " < " << m_levelIDM << " IDM level");
			return new SymmetryComputationIDM(this, rayComp, data, permGroup, rays);
		} else if (this->recursionDepth() < m_levelADM) {
			YALLOG_INFO(logger, this->recursionDepth() << " < " << m_levelADM << " ADM level");
			return new SymmetryComputationADM(this, rayComp, data, permGroup, rays);
		} else {
			YALLOG_INFO(logger, this->recursionDepth() << " direct level");
			return new SymmetryComputationDirect(this, rayComp, data, permGroup, rays);
		}
	}
private:
	const uint m_levelIDM;
	const uint m_levelADM;
};

}

#endif
