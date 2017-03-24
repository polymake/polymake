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

#ifndef SYMPOL_FACESUPTOSYMMETRYLIST_H_
#define SYMPOL_FACESUPTOSYMMETRYLIST_H_

#include "configuration.h"
#include "qarray.h"
#include "facewithdata.h"
#include "yal/logger.h"

#include <list>
#include <permlib/transversal/orbit_set.h>

namespace sympol {

class FacesUpToSymmetryList {
public:
	explicit FacesUpToSymmetryList(const PermutationGroup& group, bool sorted = false, bool withAdjacencies = false);
	virtual ~FacesUpToSymmetryList() {}

	bool equivalentToKnown(FaceWithData& f, FaceWithDataPtr* equiv = 0) const;
	bool add(FaceWithDataPtr& f);
	bool add(FaceWithDataPtr& f, FaceWithDataPtr& adjacentFace);

	/*
	template<class ForwardIterator>
	void insert(ForwardIterator begin, ForwardIterator end) {
		m_inequivalentFaces.insert(m_inequivalentFaces.end(), begin, end);
	}
	void insert(const FaceWithData& f) {
		m_inequivalentFaces.push_back(f);
	}
*/

	typedef std::list<FaceWithDataPtr>::const_iterator FaceIt;
	FaceIt begin() const { return m_inequivalentFaces.begin(); }
	FaceIt end() const { return m_inequivalentFaces.end(); }
	long firstVertexIndex() const;

	FaceWithDataPtr shift();

	bool empty() const { return m_inequivalentFaces.empty(); }
	size_t size() const { return m_inequivalentFaces.size(); }
	void clear() { m_inequivalentFaces.clear(); }

	ulong orbitSize() const { return totalOrbitSize; }

	bool sorted() const { return m_sorted; }
	bool withAdjacencies() const { return m_withAdjacencies; }
private:
	bool m_sorted;
	bool m_withAdjacencies;
	std::list<FaceWithDataPtr> m_inequivalentFaces;
	const PermutationGroup& m_group;

	typedef FaceWithData::FingerprintPtr FingerprintPtr;
	std::set<FingerprintPtr,FaceWithData::CompareFingerprint> m_fingerprints;

	uint m_computeInvariants;
	uint m_computeOrbits;
	bool m_computeCanonicalRepresentative;
	//WARNING: not thread-safe
	static time_t ms_lastMemCheck;
	static uint ms_lastMem;
	ulong totalOrbitSize;

	void forceAdd(FaceWithDataPtr& face);
	bool computeOrbits();

	void computeInvariants();
	void evaluateInvariants(const Face& face, FingerprintPtr& fingerprint) const;

	static yal::LoggerPtr logger;
};

}

#endif /* SYMPOL_FACESUPTOSYMMETRYLIST_H_ */
