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

#ifndef SYMMETRY_COMPUTATION_ADM_MEMENTO_H_
#define SYMMETRY_COMPUTATION_ADM_MEMENTO_H_

#include "symmetrycomputationmemento.h"

namespace sympol {

class SymmetryComputationADMMemento : public SymmetryComputationMemento {
	friend class SymmetryComputationADM;
	
	ulong localId;
	FaceWithData* currentRay;
	std::list<FaceWithData> rays;
	std::list<FaceWithData> todoRays;
	const SymmetryComputationADM* adm;
	SymmetryComputationADM* adm2;
	
	SymmetryComputationADMMemento() : currentRay(0), adm(0) {}
	SymmetryComputationADMMemento(const SymmetryComputationADM* adm) : currentRay(0), adm(adm) {}
	virtual ~SymmetryComputationADMMemento() { delete currentRay; }
};

}

#endif
