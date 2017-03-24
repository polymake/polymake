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

#ifndef SYMPOL_FACEWITHDATA_H_
#define SYMPOL_FACEWITHDATA_H_

#include "common.h"
#include <permlib/transversal/orbit_set.h>
#include <permlib/search/orbit_lex_min_search.h>

namespace sympol {

typedef boost::shared_ptr<QArray> QArrayPtr;

struct FaceWithData {
	typedef permlib::OrbitSet<PERM,Face> FaceOrbit;
	typedef std::vector<boost::uint64_t> Fingerprint;
	typedef boost::shared_ptr<Fingerprint> FingerprintPtr;
	typedef boost::shared_ptr<Face> CanonicalRepresentativePtr;

	Face face;
	QArrayPtr ray;
	uint incidenceNumber;
	boost::shared_ptr<FaceOrbit> orbit;
	FingerprintPtr fingerprint;
	CanonicalRepresentativePtr canonicalRepresentative;
	ulong orbitSize;
	ulong id;
	std::set<boost::shared_ptr<FaceWithData> > adjacencies;
	boost::shared_ptr<PermutationGroup> stabilizer;

	FaceWithData(const Face& f, const QArrayPtr& ray, uint incidenceNumber = 0) : face(f), ray(ray),
			incidenceNumber(incidenceNumber), orbitSize(0), id(1)
	{ }

	explicit FaceWithData(const Face& f) : face(f), incidenceNumber(0), orbitSize(0), id(1)
	{ }

	FaceWithData() : incidenceNumber(0), orbitSize(0), id(1) { }

	bool operator<(const FaceWithData& f) const {
		if (this->incidenceNumber < f.incidenceNumber)
			return true;
		return this->face < f.face;
	}

	struct CompareFingerprint {
		bool operator()(const FingerprintPtr& fp1, const FingerprintPtr& fp2) const {
			if (!fp1)
				return true;
			if (!fp2)
				return false;
			return *fp1 < *fp2;
		}
	};
};

typedef boost::shared_ptr<FaceWithData> FaceWithDataPtr;

}

#endif // SYMPOL_FACEWITHDATA_H_
