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

#ifndef SYMMETRYCOMPUTATIONADM_H
#define SYMMETRYCOMPUTATIONADM_H

#include "symmetrycomputation.h"
#include "facesuptosymmetrylist.h"
#include "yal/logger.h"

namespace sympol {

class SymmetryComputationADMMemento;

class SymmetryComputationADM : public SymmetryComputation {
public:
    SymmetryComputationADM(RecursionStrategy* const recursionStrategy, const RayComputation* rayCompDefault, 
                           const Polyhedron & data, const PermutationGroup & permGroup, FacesUpToSymmetryList& rays);

    bool enumerateRaysUpToSymmetry();
		static ulong instanceNumber();
protected:
    // some helper variables
    // make them class members for performance
    // (avoid costly mpq_init)
    mpq_class m_qScalar;
    mpq_class m_qScalar2;
    mpq_class m_qTemp;
    mpq_class m_qMinimum;
	
    // helper variables for ADM
    FacesUpToSymmetryList m_todoList;
    ulong m_orbitSize;
		ulong m_localId;
		/// ray which is currently used in enumerateRaysUpToSymmetry
		const FaceWithData* m_currentRay;
    
    // calculates index of next inequality from f/ray via ray_i
    long calculateMinimalInequality(const Face & f, const QArray & ray, const QArray & ray_i);
    
    // looks for an appropriate start point for ADM
    bool prepareStart(FacesUpToSymmetryList& rays);
    
		bool findNeighborRays(FaceWithDataPtr& faceData, FacesUpToSymmetryList& rays);
		void processSupportConeRay(FaceWithDataPtr& faceData, QArray& ray_i);

		virtual SymmetryComputationMemento* rememberMe() const;
		virtual void rememberMe(SymmetryComputationMemento* memo);
private:
    static yal::LoggerPtr logger;
		
		static ulong instanceCounter;
		
		friend class SymmetryComputationADMMemento;
};

}

#endif
