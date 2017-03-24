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

#include "symmetrycomputationidm.h"

#include "polyhedron.h"
#include <vector>
#include <boost/foreach.hpp>
#include "recursionstrategy.h"
#include "symmetrycomputationidmmemento.h"

using namespace yal;
using namespace sympol;

LoggerPtr SymmetryComputationIDM::logger(Logger::getLogger("SymCompIDM"));

SymmetryComputationIDM::SymmetryComputationIDM(RecursionStrategy* const recursionStrategy, const RayComputation* rayCompDefault, const Polyhedron & data, 
            const PermutationGroup & permGroup, FacesUpToSymmetryList& rays)
        : SymmetryComputation(IDM, recursionStrategy, rayCompDefault, data, permGroup, rays), m_lastRowIndex(0),
          m_thisFaces(permGroup)
{ }

// TODO: merge code with enumerateRaysUpToSymmetry
double SymmetryComputationIDM::probe(const Polyhedron & data, const PermutationGroup & permGroup, 
            std::list<FaceWithData> & rays) const {
  double retVal = 0.0;
  unsigned long facesChecked = 0;
  unsigned long facesTotal = 0;
  Face test(data.toFace());
  FacesUpToSymmetryList thisFaces(permGroup);
  
  BOOST_FOREACH(const QArray& row, data.rowPair()) {
    test[row.index()] = 1;
    ++facesTotal;
    
    FaceWithDataPtr testPtr(new FaceWithData(test));
    if (!thisFaces.add(testPtr)) {
			test[row.index()] = 0;
			continue;
		}
    
    Polyhedron pdat(data);
    pdat.addLinearity(row);
    if (!m_rayCompDefault->determineRedundancies(pdat, rays)) {
      return 0.0;
    }
    YALLOG_DEBUG2(logger, "pdat has #row = " << pdat.rows() << " from " << data.rows());
    
    ++facesChecked;
    retVal += pdat.rows();
    //retVal += (double) lstRedundancies.size() / (data.m_ulIneq - data.m_ulEq - data.m_setEqualitiesSubproblem.size());
    
    test[row.index()] = 0;
  }
  
  YALLOG_DEBUG(logger, "orbit contains " << facesChecked << " of " << facesTotal << " faces");
  
  return facesChecked * retVal / static_cast<double>(facesTotal);
}


bool SymmetryComputationIDM::enumerateFacesUpToSymmetry(std::set< Face >& faces)
{
    return false;
}


bool SymmetryComputationIDM::enumerateRaysUpToSymmetry() {
  logger->logUsageStats();
  YALLOG_DEBUG(logger, "start IDM");
    
  Face test(m_data.toFace());
  
  BOOST_FOREACH(const QArray& row, m_data.rowPair()) {
		if (row.index() < m_lastRowIndex)
			continue;
    if (m_data.isLinearity(row))
      continue;
    test[row.index()] = 1;
		m_lastRowIndex = row.index();
    
		FaceWithDataPtr testPtr(new FaceWithData(test));
		if (!m_loadedFromMemento && !m_thisFaces.add(testPtr)) {
			test[row.index()] = 0;
			continue;
		}
    
    Polyhedron pdat(m_data);
    pdat.addLinearity(row);
    std::list<FaceWithData> dummyList;
    if (!m_rayCompDefault->determineRedundancies(pdat, dummyList)) {
      return false;
    }
    //TODO: re-activate line if necessary
    /*BOOST_FOREACH(const FaceWithData& fd, dummyList) {
    	m_rays.add(fd);
    }*/

    
    YALLOG_DEBUG3(logger, "IDM polyhedron " << pdat);

    PermutationGroup stab = stabilizer(m_permGroup, test);
    YALLOG_DEBUG2(logger, "order of stabilizer: " << stab.order());
    FacesUpToSymmetryList localRays(stab);

    if (!m_recursionStrategy->enumerateRaysUpToSymmetry(m_rayCompDefault, pdat, stab, localRays))
      return false;
    
    YALLOG_DEBUG2(logger, "enumerateRaysUpToSymmetry found " << localRays.size() << " rays");

    // and exploit symmetries
    for (FacesUpToSymmetryList::FaceIt rayIt = localRays.begin(); rayIt != localRays.end(); ++rayIt) {
    	// non-const copy
    	FaceWithDataPtr fdPtr(new FaceWithData((*rayIt)->face, (*rayIt)->ray, (*rayIt)->incidenceNumber));
    	bool inequivalentRay = m_rays.add(fdPtr);
    	if (inequivalentRay) {
    		YALLOG_DEBUG3(logger, "no equivalency found");
    	} else {
    		YALLOG_DEBUG3(logger, "equivalency found");
    	}
    } //endfor localRays
    
    test[row.index()] = 0;
    
    // we assume next round's data is not loaded from a memento
    m_loadedFromMemento = false;
  }
  
  return true;
}

SymmetryComputationMemento* SymmetryComputationIDM::rememberMe() const { 
	SymmetryComputationIDMMemento* idm = new SymmetryComputationIDMMemento(this);
	initRememberMe(idm);
	return idm;
}

void SymmetryComputationIDM::rememberMe(SymmetryComputationMemento* memo) {
	this->SymmetryComputation::rememberMe(memo);
	
	//TODO
	/*
	SymmetryComputationIDMMemento* idm = dynamic_cast<SymmetryComputationIDMMemento*>(memo);
	m_lastRowIndex = idm->lastRowIndex;
	
	m_thisFaces.insert(idm->thisFaces.begin(), idm->thisFaces.end());
	YALLOG_DEBUG(logger, "restored " << idm->thisFaces.size() << " faces");
	
	idm->idm2 = this;
	*/
}

