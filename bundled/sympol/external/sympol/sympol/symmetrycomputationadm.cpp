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

#include "symmetrycomputationadm.h"
#include "symmetrycomputationadmmemento.h"

#include "configuration.h"
#include "polyhedron.h"
#include "polyhedronio.h"
#include "recursionstrategy.h"

#include <vector>
#include <algorithm>
#include <boost/assert.hpp>

using namespace yal;
using namespace sympol;

LoggerPtr SymmetryComputationADM::logger(Logger::getLogger("SymCompADM"));
ulong SymmetryComputationADM::instanceCounter = 0;

SymmetryComputationADM::SymmetryComputationADM(RecursionStrategy* const recursionStrategy, 
																								const RayComputation* rayCompDefault, const Polyhedron & data, 
																								const PermutationGroup & permGroup, FacesUpToSymmetryList& rays)
	: SymmetryComputation(ADM, recursionStrategy, rayCompDefault, data, permGroup, rays),
	  m_todoList(permGroup, true), m_orbitSize(0),
		m_localId(instanceCounter++), m_currentRay(0)
{ }


bool SymmetryComputationADM::enumerateRaysUpToSymmetry() {
	bool retVal = true;

	logger->logUsageStats();
	YALLOG_DEBUG(logger, "start ADM " << m_localId);

	if (m_data.rows() == 0) {
		YALLOG_WARNING(logger, "encountered empty polyhedron");
		return true;
	}

	// prefill todo-list
	if (m_todoList.empty() && !prepareStart(m_rays)) {
		YALLOG_INFO(logger, "could not find start point");
		return false;
	}

  // our polyhedron is a (possibly homogenized) cone with apex 0.
  // we won't find the apex later, so add it here manually
  QArrayPtr row(new QArray(m_data.dimension()));
  mpq_set_ui((*row)[0], 1, 1);
  Face face(m_data.faceDescription(*row));
  if (face.count() == m_data.rows()) {
    FaceWithDataPtr fdPtr(new FaceWithData(face, row));
    m_rays.add(fdPtr);
  }

	const ulong workingDimension = m_data.workingDimension();
	YALLOG_DEBUG(logger, "working dimension = " << workingDimension);

	uint run = 0;
	while (!m_todoList.empty()) {
		BOOST_ASSERT( !m_todoList.empty() );
		YALLOG_DEBUG3(logger, "todo orbitSize = " << m_todoList.orbitSize());

		// don't apply Balinski criterion if
		//  - we need the complete adjacency graph, or
		//  - at the first vertex
		if (!m_rays.withAdjacencies() && run > 0 && m_todoList.orbitSize() < workingDimension) {
			YALLOG_DEBUG(logger, "leave ADM due to Balinski criterion " << m_todoList.orbitSize() << " <? " << workingDimension << " after " << (run+1) << " runs");
			m_currentRay = 0;
			break;
		}
		++run;

		FaceWithDataPtr faceData = m_todoList.shift();
		m_currentRay = &*faceData;
		
		YALLOG_DEBUG(logger, "ADM[" << m_localId << "]: #todo = " << m_todoList.size() << ", start with face " << faceData->face << " <=> " << *(faceData->ray) );
		BOOST_ASSERT( m_data.checkFace(*(faceData->ray)) );
		
		if (!findNeighborRays(faceData, m_rays)) {
			m_currentRay = 0;
			retVal = false;
			break;
		}
		m_currentRay = 0;
	} // end while todo

	YALLOG_DEBUG(logger, "leaving while[" << m_localId << "]");

	return retVal;
}



/**
 * calculates index of next inequality from f/ray via ray_i
 */
long SymmetryComputationADM::calculateMinimalInequality(const Face & f, const QArray & ray, const QArray & ray_i) {
    // no minimal inequality has been found
    long minInequalityIndex = -1;
    
    // seek minimum of
    // (-m_aQIneq[j][0] - <m_aQIneq[j], ray>) / <m_aQIneq[j], ray_i>
    // among all inequalities m_aQIneq[j], which
    //  * are inactive in ray
    //  * and have <m_aQIneq[j], ray_i> < 0
    
    BOOST_FOREACH(const QArray& row, m_data.rowPair()) {
      const ulong j = row.index();
        if (f[j]) {
            YALLOG_DEBUG3(logger, "skipping " << j << " due to " << f[j]);
            continue;
        }
        
        row.scalarProduct(ray_i, m_qScalar, m_qTemp);
        YALLOG_DEBUG3(logger, "scalar[" << j << "] = <" << ray_i << ", " << row << "> = " << m_qScalar);

        // use only rows with negative scalar product
        const int sign = sgn(m_qScalar);
        if (sign >= 0) {
            continue;
        }
        
        row.scalarProduct(ray, m_qScalar2, m_qTemp);
        YALLOG_DEBUG3(logger, "scalar2[" << j << "] = <" << ray << ", " << row << "> = " << m_qScalar2);
        if (!mpq_sgn(ray[0]))
        	m_qScalar2 += mpq_class(row[0]);
        m_qScalar2 /= -m_qScalar;

        YALLOG_DEBUG3(logger, "scalar2 = " << m_qScalar2);

        // check, if new minimum is attained
        // in the first round (minJ == -1), the value is automatically the minimum
        if (m_qMinimum > m_qScalar2 || minInequalityIndex == -1) {
            minInequalityIndex = j;
            m_qMinimum = m_qScalar2;
        }
    }
    
    return minInequalityIndex;
}



/**
 * looks for an appropiate start point for ADM
 */
bool SymmetryComputationADM::prepareStart(FacesUpToSymmetryList& rays) {
	m_todoList.clear();

	for (FacesUpToSymmetryList::FaceIt rit = rays.begin(); rit != rays.end(); ++rit) {
		if (!(*rit)->ray->isRay())
			continue;
		// non-const copy
		FaceWithDataPtr fdPtr(new FaceWithData((*rit)->face, (*rit)->ray, (*rit)->incidenceNumber));
		m_todoList.add(fdPtr);
		return true;
	}

	Face f(m_data.emptyFace());
	QArrayPtr q(new QArray(m_data.dimension()));
	if (!m_rayCompDefault->firstVertex(m_data, f, *q, true)) {
		return false;
	}
	FaceWithDataPtr fdPtr(new FaceWithData(f, q, m_data.incidenceNumber(f)));
	m_todoList.add(fdPtr);
	m_rays.add(fdPtr);
	return true;
}



bool SymmetryComputationADM::findNeighborRays(FaceWithDataPtr& faceData, FacesUpToSymmetryList& rays) {
	const Face& f = faceData->face;
		
	Polyhedron support(m_data.supportCone(f));
	YALLOG_DEBUG3(logger, "Support[" << m_localId << "]\n" << support);
	//BOOST_ASSERT( support.checkFace(ray) );

	if (!faceData->stabilizer)
		faceData->stabilizer.reset(new PermutationGroup(stabilizer(m_permGroup, f)));
	YALLOG_DEBUG2(logger, "order of stabilizer: " << faceData->stabilizer->order());

	const PermutationGroup& stab = *faceData->stabilizer.get();
	FacesUpToSymmetryList localRays(stab);
	const bool retVal = m_recursionStrategy->enumerateRaysUpToSymmetry(m_rayCompDefault, support, stab, localRays);
	YALLOG_DEBUG(logger, "found #localRays = " << localRays.size());
	if (!retVal) {
		return false;
	}
	
	// iterate over all rays of the support cone
	for (FacesUpToSymmetryList::FaceIt rit = localRays.begin(); rit != localRays.end(); ++rit) {
		BOOST_ASSERT( support.checkFace(*((*rit)->ray)) );
		processSupportConeRay(faceData, *((*rit)->ray));
	}
	
	// we assume next round's data is not loaded from a memento
	m_loadedFromMemento = false;
	
	return true;
}

void SymmetryComputationADM::processSupportConeRay(FaceWithDataPtr& faceData, QArray& ray_i) {
	// we only want rays here
	if (!ray_i.isRay()) {
		return;
	}
	
	const QArray& ray = *faceData->ray;
	const QArray& axis = m_data.axis();
	
	// project ray_i in the plane spanned by ray and ray_i
	// so that is orthogonal to the "axis" of our cone
	//
	// projecting guarantees that our new ray_i hits
	// another hyperplane to find a new vertex/ray
	//
	// let axis = \sum_i m_aQIneq[i]. then:
	// ray_i := ray_i - <axis, ray_i> / <axis, ray> * ray
	//
	YALLOG_DEBUG2(logger, "projecting " << ray_i);
	axis.scalarProduct(ray_i, m_qScalar, m_qTemp);
	axis.scalarProduct(ray, m_qScalar2, m_qTemp);
	m_qScalar /= -m_qScalar2;
	for (ulong k = 0; k < ray_i.size(); ++k) {
		mpq_mul(m_qTemp.get_mpq_t(), m_qScalar.get_mpq_t(), ray[k]);
		mpq_add(ray_i[k], ray_i[k], m_qTemp.get_mpq_t());
	}
	YALLOG_DEBUG2(logger, "        to " << ray_i);

	
	// get the next inequality/plane that ray_i penetrates
	long minInequalityIndex = calculateMinimalInequality(faceData->face, ray, ray_i);
	
	QArrayPtr nextVertex(new QArray(ray));
	Face nextFace = m_data.faceDescription(*nextVertex);
	YALLOG_DEBUG2(logger, *nextVertex << " ~~~ " << nextFace);
	
	// if we have found a suitable inequality
	if (minInequalityIndex >= 0) {
		// we can calculate the neighborly "vertex"
		for (ulong k = 0; k < ray_i.size(); ++k) {
			mpq_mul(m_qTemp.get_mpq_t(), m_qMinimum.get_mpq_t(), ray_i[k]);
			mpq_add((*nextVertex)[k], (*nextVertex)[k], m_qTemp.get_mpq_t());
		}

		YALLOG_DEBUG2(logger, "minimum found (" << m_qMinimum << ")- new vertex");
	} else if (faceData->face == nextFace) {
		YALLOG_DEBUG2(logger, "found original ray; skipped");
	} else {
		YALLOG_WARNING(logger, "could not determine minimum for " << nextFace << " // " << *nextVertex << "; skipped");
		return;
	}
				
	nextFace = m_data.faceDescription(*nextVertex);
	YALLOG_DEBUG(logger, "ADM[" << m_localId << "] calculated " << nextFace << " // " << *nextVertex);
	BOOST_ASSERT( m_data.checkFace(*nextVertex) );
	
	FaceWithDataPtr fdPtr(new FaceWithData(nextFace, nextVertex, m_data.incidenceNumber(nextFace)));
	bool inequivalentRay = m_rays.add(fdPtr, faceData);
	if (inequivalentRay)
		m_todoList.add(fdPtr);
}

SymmetryComputationMemento* SymmetryComputationADM::rememberMe() const { 
	SymmetryComputationADMMemento* adm = new SymmetryComputationADMMemento(this);
	initRememberMe(adm);
	return adm;
}

void SymmetryComputationADM::rememberMe(SymmetryComputationMemento* memo) {
	this->SymmetryComputation::rememberMe(memo);
	
	SymmetryComputationADMMemento* adm = dynamic_cast<SymmetryComputationADMMemento*>(memo);
	m_localId = adm->localId;

	if (adm->currentRay) {
		YALLOG_DEBUG2(logger, "restored current ray" << adm->currentRay->face << " / " << adm->currentRay->ray);
		//TODO: re-active line if necessary
		//m_todoList.insert(*adm->currentRay);
		//TODO: delete here?
		//delete adm->currentRay;
	} else {
		YALLOG_DEBUG2(logger, "restored current ray NULL");
	}
	
	//TODO: re-active line if necessary
	//m_todoList.insert(adm->todoRays.begin(), adm->todoRays.end());
	YALLOG_DEBUG2(logger, "restored " << adm->todoRays.size() << " todoRays");

	//TODO: re-active line if necessary
	//m_rays.insert(adm->rays.begin(), adm->rays.end());
	
	YALLOG_DEBUG2(logger, "restored " << adm->rays.size() << " rays");
	
	adm->adm2 = this;
}

ulong SymmetryComputationADM::instanceNumber() {
	return instanceCounter;
}
