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


#include "polymake/polytope/sympol_config.h"
#include "polymake/polytope/sympol_raycomputation_beneathbeyond.h"
#include "polymake/polytope/sympol_interface.h"
#include "polymake/polytope/beneath_beyond.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "sympol/common.h"
#include "sympol/polyhedron.h"
#include "sympol/raycomputationlrs.h"
#include <boost/foreach.hpp>

using namespace sympol;
using namespace polymake::polytope::sympol_interface;

//
// static members
//
const char* RayComputationBeneathBeyond::ms_chName = "BeneathBeyond";

RayComputationBeneathBeyond::RayComputationBeneathBeyond() 
	: m_lrs(new RayComputationLRS())
{
}

bool RayComputationBeneathBeyond::initialize() {
	return m_lrs->initialize();
}

bool RayComputationBeneathBeyond::finish() {
	return m_lrs->finish();
}

/**
 * computes dual description of data into rays
 */
bool RayComputationBeneathBeyond::dualDescription(const Polyhedron & data, std::vector<FaceWithDataPtr> & rays) const {
	Matrix<Rational> points(data.rows(), data.dimension());
	
	int i = 0;
	BOOST_FOREACH(const QArray& r, data.rowPair()) {
		for (unsigned int j = 0; j < data.dimension(); j++) {
			points[i][j].copy_from(r[j]);
		}
		++i;
	}
	
	beneath_beyond_algo<Rational> bb_algo(points, true);
	bb_algo.compute(entire(sequence(0,points.rows())));
	
	Matrix<Rational> facets = bb_algo.getFacets();
	bool is_homogenized = true;
	std::list<QArray> facetList = sympol_wrapper::matrix2QArray(facets, is_homogenized);
	BOOST_FOREACH(const QArray& r, facetList) {
		QArrayPtr qRay(new QArray(r));
		qRay->normalizeArray();

		const Face f = data.faceDescription(*qRay);
		FaceWithDataPtr fdPtr(new FaceWithData(f, qRay, data.incidenceNumber(f)));
		rays.push_back(fdPtr);
	}
	
	return true;
}

bool RayComputationBeneathBeyond::firstVertex(const Polyhedron & data, Face & f, QArray & q, bool requireRay) const {
	return m_lrs->firstVertex(data, f, q, requireRay);
}

bool RayComputationBeneathBeyond::determineRedundancies(Polyhedron & data, std::list<FaceWithData>& myRays) const {
	return m_lrs->determineRedundancies(data, myRays);
}

double RayComputationBeneathBeyond::estimate(const sympol::Polyhedron& data, std::list<FaceWithData>& rays) const
{
	return m_lrs->estimate(data, rays);
}

bool RayComputationBeneathBeyond::getLinearities(const Polyhedron & data, std::list<QArrayPtr>& linearities) const {
	return m_lrs->getLinearities(data, linearities);
}

