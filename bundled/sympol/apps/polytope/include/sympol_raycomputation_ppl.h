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

#ifndef RAYCOMPUTATION_PPL_H
#define RAYCOMPUTATION_PPL_H

#include "sympol/raycomputation.h"

namespace sympol {
	class Polyhedron;
	class QArray;
	// to match the definition in sympol's facewithdata.h, this might be better defined as a class ?
	struct FaceWithData;
	class RayComputationLRS;
}

namespace polymake { namespace polytope { namespace sympol_interface {

class RayComputationPPL : public sympol::RayComputation {
	public:
		RayComputationPPL();
		
		bool initialize();
		bool finish();
		
		bool dualDescription(const sympol::Polyhedron & data, std::vector<sympol::FaceWithDataPtr> & rays) const;
		bool firstVertex(const sympol::Polyhedron & data, sympol::Face & f, sympol::QArray & q, bool requireRay = true) const;
		bool determineRedundancies(sympol::Polyhedron & data, std::list<sympol::FaceWithData> & myRays) const;
		double estimate(const sympol::Polyhedron & data, std::list<sympol::FaceWithData> & rays) const;
		bool getLinearities(const sympol::Polyhedron & data, std::list<sympol::QArrayPtr>& linearities) const;
		
		const char* name() const { return RayComputationPPL::ms_chName; }
	private:
		static const char* ms_chName;
		
		/// lrs instance for computations that beneath_beyond cannot do easily
		const boost::shared_ptr<sympol::RayComputationLRS> m_lrs;
};

} } } // end NS

#endif
