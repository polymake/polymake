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

#ifndef RAYCOMPUTATION_H
#define RAYCOMPUTATION_H

#include "common.h"
#include "qarray.h"
#include "facesuptosymmetrylist.h"

#include <list>
#include <vector>

namespace sympol {

class Polyhedron;
  
class RayComputation {
public:
    virtual ~RayComputation() {}
    
    // prepare and clean up computational instruments
    virtual bool initialize() = 0;
    virtual bool finish() = 0;
    
    // compute dual description of given polyhedron
    virtual bool dualDescription(const Polyhedron & data, std::vector<FaceWithDataPtr> & rays) const = 0;
    
    // compute a first vertex/ray of given polyhedron
    virtual bool firstVertex(const Polyhedron & data, Face & f, QArray & q, bool requireRay = false) const = 0;

    // determine redundant inequalities
    // this procedure might find some vertices/rays that are stored in the respective lists
    virtual bool determineRedundancies(Polyhedron & data, std::list<FaceWithData> & myRays) const = 0;
    
    // name of computation method
    virtual const char* name() const = 0;

    // estimate problem
    virtual double estimate(const Polyhedron & data, std::list<FaceWithData> & rays) const { return 0.0; }

    virtual bool determineRedundantColumns(const Polyhedron & data, std::set<ulong> & redundantColumns) const { return false; }
    
    virtual bool getLinearities(const Polyhedron & data, std::list<QArrayPtr>& linearities) const { return false; }
};
}

#endif
