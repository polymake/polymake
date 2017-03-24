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

#ifndef SYMMETRYCOMPUTATIONIDM_H
#define SYMMETRYCOMPUTATIONIDM_H

#include "symmetrycomputation.h"
#include "yal/logger.h"

namespace sympol {

class SymmetryComputationIDMMemento;
	
class SymmetryComputationIDM : public SymmetryComputation {
	public:
		SymmetryComputationIDM(RecursionStrategy* const recursionStrategy, const RayComputation* rayCompDefault, const Polyhedron & data, 
						const PermutationGroup & permGroup, FacesUpToSymmetryList& rays);
		
		bool enumerateRaysUpToSymmetry();

		double probe(const Polyhedron & data, const PermutationGroup & permGroup, 
				std::list<FaceWithData> & rays) const;

		bool enumerateFacesUpToSymmetry(std::set<Face> & faces);

		virtual SymmetryComputationMemento* rememberMe() const;
		virtual void rememberMe(SymmetryComputationMemento* memo);
	protected:
		// computes description of polyhedron given by face f into poly
		// if some generators are found, these are stored in the respective lists
		bool facePolyhedron(const Face & f, Polyhedron & poly, FacesUpToSymmetryList& myRays) const;
		
		ulong m_lastRowIndex;
		FacesUpToSymmetryList m_thisFaces;
	private:
		static yal::LoggerPtr logger;
		
		friend class SymmetryComputationIDMMemento;
};

}

#endif
