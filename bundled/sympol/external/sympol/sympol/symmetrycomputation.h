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

#ifndef SYMMETRYCOMPUTATION_H
#define SYMMETRYCOMPUTATION_H

#include "common.h"
#include "polyhedron.h"
#include "raycomputation.h"
#include "facesuptosymmetrylist.h"
#include "yal/logger.h"

#include <list>
#include <set>
#include <string>

#include <permlib/transversal/orbit_set.h>

namespace sympol {

class RecursionStrategy;
class SymmetryComputation;
class SymmetryComputationMemento;


class SymmetryComputation {
	public:
    SymmetryComputation(SymmetryComputationMethod method, RecursionStrategy* const recursionStrategy, const RayComputation* rayCompDefault, const Polyhedron & data, 
            const PermutationGroup & permGroup, FacesUpToSymmetryList & rays);
    
    virtual ~SymmetryComputation() {}
    
    // returns true on success and false on computation error
    virtual bool enumerateRaysUpToSymmetry() = 0;
    
    // tries to compute a success estimation with respect to expected runtime &c.
    virtual double probe(const Polyhedron & data, const PermutationGroup & permGroup, 
            std::list<FaceWithData> & rays) const { return 1.0; }
            
    virtual bool enumerateFacesUpToSymmetry(std::set<Face> & faces) { return false; }
    
    SymmetryComputationMethod method() const { return m_method; }
    
    virtual SymmetryComputationMemento* rememberMe() const;
		void initRememberMe(SymmetryComputationMemento* memo) const;
    virtual void rememberMe(SymmetryComputationMemento* memo);
    
		typedef permlib::OrbitSet<PERM,Face> FaceOrbit2;
		//typedef permlib::OrbitList<PERM,Face> FaceOrbit2;
		static PermutationGroup stabilizer(const PermutationGroup & permGroup, const Face & f);
protected:
    bool equivalentFaces(const PermutationGroup & permGroup, const Face & f1, const Face & f2, const FaceOrbit2 & orbit) const;
    bool equivalentFaces(const PermutationGroup & permGroup, const Face & f1, const Face & f2) const;
    
    RecursionStrategy* const m_recursionStrategy;
    const RayComputation* m_rayCompDefault;
    const Polyhedron & m_data;
    const PermutationGroup & m_permGroup;
    const static FaceOrbit2 ms_setEmpty;
		
    FacesUpToSymmetryList& m_rays;
		bool m_loadedFromMemento;
private:
    static yal::LoggerPtr logger;
		SymmetryComputationMethod m_method;
};

}
#endif
