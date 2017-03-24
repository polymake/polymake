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

#ifndef RAYCOMPUTATIONLRS_H
#define RAYCOMPUTATIONLRS_H

#include "common.h"
#include "raycomputation.h"
#include "yal/logger.h"

struct lrs_dic_struct;
typedef struct lrs_dic_struct lrs_dic;

struct lrs_dat;
typedef mpz_t **lrs_mp_matrix;

namespace sympol {
  
class RayComputationLRS : public RayComputation {
public:
    bool initialize();
    bool finish();
    
    bool dualDescription(const Polyhedron & data, std::vector<FaceWithDataPtr> & rays) const;
    bool firstVertex(const Polyhedron & data, Face & f, QArray & q, bool requireRay = true) const;
    bool determineRedundancies(Polyhedron & data, std::list<FaceWithData> & myRays) const;

    const char* name() const { return RayComputationLRS::ms_chName; }
    
    double estimate(const Polyhedron & data, std::list<FaceWithData> & rays) const;
    
    bool determineRedundantColumns(const Polyhedron & data, std::set<ulong> & redundantColumns) const;

    bool getLinearities(const Polyhedron & data, std::list<QArrayPtr>& linearities) const;
private:
    static bool ms_bInitialized;
    static const char* ms_chName;
    
    // files for LRS
    static FILE* ms_fIn;
    static FILE* ms_fOut;
    
    bool initLRS(const Polyhedron & data, lrs_dic* & P, lrs_dat* & Q) const;
    bool initLRS(const Polyhedron & data, lrs_dic* & P, lrs_dat* & Q, lrs_mp_matrix& Lin, int estimates, int maxDepth) const;
    
    void fillModelLRS(const Polyhedron & data, lrs_dic *P, lrs_dat *Q) const;
    
    static yal::LoggerPtr logger;
};

}

#endif
