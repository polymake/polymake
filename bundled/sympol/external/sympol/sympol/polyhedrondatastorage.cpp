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

#include "polyhedrondatastorage.h"

using namespace sympol;

std::list<PolyhedronDataStorage*> PolyhedronDataStorage::ms_storages;

PolyhedronDataStorage::PolyhedronDataStorage(ulong spaceDim, ulong ineq) 
  : m_ulSpaceDim(spaceDim), m_ulIneq(ineq)
{
  m_aQIneq.reserve(m_ulIneq);
}

/**
 * creates storage object with given parameters
 */
PolyhedronDataStorage * PolyhedronDataStorage::createStorage(ulong spaceDim, ulong ineq) {
    PolyhedronDataStorage* stor = new PolyhedronDataStorage(spaceDim, ineq);
    ms_storages.push_back(stor);
    return stor;
}

PolyhedronDataStorage * PolyhedronDataStorage::createStorage(const PolyhedronDataStorage& storage) {
	PolyhedronDataStorage* stor = new PolyhedronDataStorage(storage.m_ulSpaceDim, storage.m_ulIneq);
	for (std::vector<QArray>::const_iterator it = storage.m_aQIneq.begin(); it != storage.m_aQIneq.end(); ++it) {
		stor->m_aQIneq.push_back(*it);
	}
	ms_storages.push_back(stor);
	return stor;
}

void PolyhedronDataStorage::cleanupStorage() {
    std::list<PolyhedronDataStorage*>::iterator it;
    for (it = ms_storages.begin(); it != ms_storages.end(); ++it) {
        delete *it;
    }
    ms_storages.clear();
}
