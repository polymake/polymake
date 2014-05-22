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

#ifndef RAYCOMPUTATIONCDD_H
#define RAYCOMPUTATIONCDD_H

#include "common.h"
#include "raycomputation.h"
#include "raycomputationlrs.h"
#include "yal/logger.h"

struct dd_matrixdata;
typedef struct  dd_matrixdata *dd_MatrixPtr;

namespace sympol {
  
class RayComputationCDD : public RayComputation {
	public:
		RayComputationCDD();
		
		bool initialize();
		bool finish();
		
		bool dualDescription(const Polyhedron & data, std::vector<FaceWithDataPtr> & rays) const;
		bool firstVertex(const Polyhedron & data, Face & f, QArray & q, bool requireRay = true) const;
		bool determineRedundancies(Polyhedron & data, std::list<FaceWithData> & myRays) const;
		double estimate(const Polyhedron & data, std::list<FaceWithData> & rays) const;
		bool getLinearities(const Polyhedron & data, std::list<QArrayPtr>& linearities) const;
		
		const char* name() const { return RayComputationCDD::ms_chName; }    
	private:
		static bool ms_bInitialized;
		static const char* ms_chName;
		
		/// lrs instance to give an estimation (don't know how to do that in cdd)
		const boost::shared_ptr<RayComputationLRS> m_lrs;
		
		bool fillModelCDD(const Polyhedron & data, dd_MatrixPtr& matrix) const;

		static yal::LoggerPtr logger;
};

}

#endif
