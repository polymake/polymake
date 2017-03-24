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

#include "common.h"
#include "configuration.h"
#include "facesuptosymmetrylist.h"
#include "yal/usagestats.h"
#include "symmetrycomputation.h"

#include <permlib/change/conjugating_base_change.h>
#include <permlib/change/deterministic_base_transpose.h> 
#include <permlib/search/classic/set_stabilizer_search.h>
#include <permlib/search/classic/set_image_search.h>
#include <permlib/search/orbit_lex_min_search.h>

#include <ctime>

using namespace permlib;
using namespace yal;
using namespace std;

namespace sympol {

LoggerPtr FacesUpToSymmetryList::logger(Logger::getLogger("FacesList "));
time_t FacesUpToSymmetryList::ms_lastMemCheck = 0;
uint FacesUpToSymmetryList::ms_lastMem = 0;

FacesUpToSymmetryList::FacesUpToSymmetryList(const PermutationGroup& group, bool sorted, bool withAdjacencies)
	: m_sorted(sorted), m_withAdjacencies(withAdjacencies), m_group(group),
	  m_computeOrbits(Configuration::getInstance().computeOrbitLimit),
	  m_computeCanonicalRepresentative(Configuration::getInstance().computeCanonicalRepresentatives),
	  totalOrbitSize(0)
{
}

bool FacesUpToSymmetryList::equivalentToKnown(FaceWithData& f, FaceWithDataPtr* equiv) const {
	const Face& f2 = f.face;

	if (m_computeCanonicalRepresentative) {
		OrbitLexMinSearch<PermutationGroup> orbitLexMinSearch(m_group);
		YALLOG_DEBUG2(logger, "compute canonical repr (equiv) " << f.face);
		f.canonicalRepresentative.reset(new Face(orbitLexMinSearch.lexMin(f.face)));
		YALLOG_DEBUG2(logger, "computed canonical repr (equiv) " << *f.canonicalRepresentative);
		BOOST_FOREACH(const FaceWithDataPtr& face, m_inequivalentFaces) {
			if (*(face->canonicalRepresentative) == *(f.canonicalRepresentative))
				return true;
		}
		return false;
	}

	BOOST_FOREACH(const FaceWithDataPtr& face, m_inequivalentFaces) {
		const Face& f1 = face->face;

		// we need equal number of equalities
		if (f1.count() != f2.count()) {
			continue;
		}
		if (f1 == f2) {
			if (equiv)
				*equiv = face;
			return true;
		}

		// if we have a complete orbit, we use it
		if (face->orbit) {
			if (face->orbit->contains(f2)) {
				YALLOG_DEBUG2(logger, "face rejected by orbit");
				if (equiv)
					*equiv = const_cast<FaceWithDataPtr&>(face);
				return true;
			} else
				continue;
		}

		unsigned int i;

		std::list<ulong> setF1;
		std::list<ulong> setF2;
		for (i=0; i<f1.size(); ++i) {
			if (f1[i])
				setF1.push_back(i);
			if (f2[i])
				setF2.push_back(i);
		}

		PermutationGroup myGroup(m_group);

		// change the base so that is prefixed by the set
		ConjugatingBaseChange<PERM,TRANSVERSAL,
			DeterministicBaseTranspose<PERM,TRANSVERSAL> > baseChange(myGroup);
		baseChange.change(myGroup, setF1.begin(), setF1.end());

		// prepare search without DCM pruning
		classic::SetImageSearch<PermutationGroup,TRANSVERSAL> backtrackSearch(myGroup, 0);
		backtrackSearch.construct(setF1.begin(), setF1.end(), setF2.begin(), setF2.end());
		boost::shared_ptr<PERM> repr = backtrackSearch.searchCosetRepresentative();

		if (repr) {
			YALLOG_DEBUG2(logger, "face rejected by backtrack search");
			if (equiv)
				*equiv = face;
			return true;
		}
	}

	return false;
}

bool FacesUpToSymmetryList::computeOrbits() {
	if (!m_computeOrbits)
		return false;

#if HAVE_GETRUSAGE_PROTO
	const time_t now = time(NULL);
	// perform memory check every 30 seconds
	if (now - ms_lastMemCheck > 30) {
		// UsageStats::processSize() is in bytes so divide by 2^20
		ms_lastMem = UsageStats::processSize() >> 20;
		ms_lastMemCheck = now;
		YALLOG_DEBUG2(logger, "perform memcheck " << ms_lastMem << " <? " << m_computeOrbits);
	}
	return m_computeOrbits > ms_lastMem;
#else
    return true;
#endif
}

bool FacesUpToSymmetryList::add(FaceWithDataPtr& fd) {
	if (equivalentToKnown(*fd))
		return false;

	forceAdd(fd);
	return true;
}

void FacesUpToSymmetryList::forceAdd(FaceWithDataPtr& fd) {
	YALLOG_DEBUG3(logger, "face " << fd->face << " is new; add as " << (m_inequivalentFaces.size() + 1) << "-th");

	if (computeOrbits()) {
		fd->orbit.reset(new FaceWithData::FaceOrbit());
		fd->orbit->orbit(fd->face, m_group.S, FaceAction());
		fd->orbitSize = fd->orbit->size();
	} else {
		fd->stabilizer.reset(new PermutationGroup(SymmetryComputation::stabilizer(m_group, fd->face)));
		fd->orbitSize = m_group.order() / fd->stabilizer->order();
	}
	if (m_computeCanonicalRepresentative) {
		OrbitLexMinSearch<PermutationGroup> orbitLexMinSearch(m_group);
		YALLOG_DEBUG2(logger, "compute canonical repr " << fd->face);
		fd->canonicalRepresentative.reset(new Face(orbitLexMinSearch.lexMin(fd->face)));
		YALLOG_DEBUG2(logger, "computed canonical repr " << *fd->canonicalRepresentative);
	}

	totalOrbitSize += fd->orbitSize;

	if (m_sorted)
		m_inequivalentFaces.insert(lower_bound(m_inequivalentFaces.begin(), m_inequivalentFaces.end(), fd), fd);
	else
		m_inequivalentFaces.push_back(fd);
}

bool FacesUpToSymmetryList::add(FaceWithDataPtr& f, FaceWithDataPtr& adjacentFace) {
	FaceWithDataPtr equiv;
	const bool knownFace = equivalentToKnown(*f, &equiv);

	if (!knownFace) {
		forceAdd(f);
		f->id = m_inequivalentFaces.size();
		equiv = f;
	}

	if (m_withAdjacencies) {
		YALLOG_DEBUG(logger, "add adjacency " << equiv->face << "(" << equiv->id << ") -- " << adjacentFace->face << "(" << adjacentFace->id << ")");
		// store edge only once
		if (equiv->adjacencies.count(adjacentFace) == 0 && equiv->id != adjacentFace->id)
			adjacentFace->adjacencies.insert(equiv);
	}
	return !knownFace;
}

FaceWithDataPtr FacesUpToSymmetryList::shift() {
	FaceWithDataPtr res = m_inequivalentFaces.front();
	totalOrbitSize -= res->orbitSize;
	m_inequivalentFaces.pop_front();
	return res;
}

long FacesUpToSymmetryList::firstVertexIndex() const {
	long index = 0;
	BOOST_FOREACH(const FaceWithDataPtr& faceData, m_inequivalentFaces) {
		if ( ! faceData->ray->isRay() ) {
			return index;
		}
		++index;
	}
	return -1;
}

}
