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

#ifndef POLYHEDRONDATASTORAGE_H
#define POLYHEDRONDATASTORAGE_H

#include "common.h"
#include "qarray.h"

#include <vector>
#include <list>

namespace sympol {

class PolyhedronDataStorage {
public:
    ulong ineq() const { return m_ulIneq; }
    
    ulong                   m_ulSpaceDim;
    // number of original inequalities
    ulong                   m_ulIneq;
    
    //TODO: vector of QArrayPtr
    std::vector<QArray>      m_aQIneq;
    
    // creates a new storage object
    static PolyhedronDataStorage * createStorage(ulong spaceDim = 0, ulong ineq = 0);
    static PolyhedronDataStorage * createStorage(const PolyhedronDataStorage& storage);
    // clean up all instanciated storage objects
    static void cleanupStorage();
protected:
    static std::list<PolyhedronDataStorage*> ms_storages;
    
    // constructor is protected
    // use createStorage instead
    PolyhedronDataStorage(ulong spaceDim = 0, ulong ineq = 0);
};

}

#endif
