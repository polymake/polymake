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

#ifndef RECURSIONSTRATEGY_IDMADM_H_
#define RECURSIONSTRATEGY_IDMADM_H_

#include "recursionstrategy.h"

namespace sympol {

class RecursionStrategyIDMADM : public RecursionStrategy {
public:
	RecursionStrategyIDMADM(double thresholdIDM, double thresholdDirect) 
		: m_estimateThresholdIDM(thresholdIDM),
			m_estimateThresholdDirect(thresholdDirect),
			m_lastFailedIDMLevel(-1)
			{}
protected:
	virtual SymmetryComputation* devise(const RayComputation* rayComp, const Polyhedron & data, 
					const PermutationGroup & permGroup, FacesUpToSymmetryList& rays)
	{
		std::list<FaceWithData> dummy;
		
		if ( -1 == m_lastFailedIDMLevel || 
				(-1 != m_lastFailedIDMLevel && static_cast<int>(this->recursionDepth()) <= m_lastFailedIDMLevel))
		{
			m_lastFailedIDMLevel = -1;
			
			SymmetryComputationIDM* sIdm = new SymmetryComputationIDM(this, rayComp, data, permGroup, rays);
			const double estIdm = sIdm->probe(data, permGroup, dummy);
			YALLOG_INFO(logger, "estimation IDM: " << estIdm << " <? " << m_estimateThresholdIDM);
			if (estIdm < m_estimateThresholdIDM)
				return sIdm;
			delete sIdm;
		}
		
		if (-1 == m_lastFailedIDMLevel)
			m_lastFailedIDMLevel = this->recursionDepth();
		
		SymmetryComputationDirect* sd = new SymmetryComputationDirect(this, rayComp, data, permGroup, rays);
		const double estDirect = sd->probe(data, permGroup, dummy);
		YALLOG_INFO(logger, "estimation dir: " << estDirect << " <? " << m_estimateThresholdDirect);
		if (estDirect < m_estimateThresholdDirect)
			return sd;
		delete sd;
		
		return new SymmetryComputationADM(this, rayComp, data, permGroup, rays);
	}
private:
	const double m_estimateThresholdIDM;
	const double m_estimateThresholdDirect;
	
	int m_lastFailedIDMLevel;
};

}

#endif
