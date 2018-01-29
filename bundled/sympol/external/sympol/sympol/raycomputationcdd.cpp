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


#include "raycomputationcdd.h"
#include "polyhedron.h"

extern "C" {
	#include <setoper.h>
	#include <cdd.h>
}

#include <ctime>
#include <cstdio>

using namespace yal;
using namespace sympol;

//
// static members
//
bool RayComputationCDD::ms_bInitialized = false;
const char* RayComputationCDD::ms_chName = "CDD";
LoggerPtr RayComputationCDD::logger(Logger::getLogger("RayCompCDD"));

RayComputationCDD::RayComputationCDD() 
	: m_lrs(new RayComputationLRS())
{
}

bool RayComputationCDD::initialize() {
	if (!RayComputationCDD::ms_bInitialized) {
		m_lrs->initialize();
		dd_set_global_constants();
		
		RayComputationCDD::ms_bInitialized = true;       
		return true;
	}
	
	return true;
}

bool RayComputationCDD::finish() {
	if (!RayComputationCDD::ms_bInitialized) {
		return true;
	}
	m_lrs->finish();
	dd_free_global_constants();
	
	RayComputationCDD::ms_bInitialized = false;
	
	return true;
}


/**
 * computes dual description of data into rays
 */
bool RayComputationCDD::dualDescription(const Polyhedron & data, std::vector<FaceWithDataPtr> & rays) const {
	dd_MatrixPtr matrix;
	if (!fillModelCDD(data, matrix))
		return false;
	
	dd_ErrorType err;
	dd_PolyhedraPtr poly = dd_DDMatrix2Poly(matrix, &err);
	if (err != dd_NoError) {
		dd_FreeMatrix(matrix);
		return false;
	}
	dd_MatrixPtr G = dd_CopyGenerators(poly);
	dd_Amatrix A = G->matrix;

  // we assume here that the polyhedron is a (possibly homogenized) cone
  // cdd doesn't return the cone apex, so add it manually
  QArrayPtr row(new QArray(data.dimension()));
  mpq_set_ui((*row)[0], 1, 1);
  Face face(data.faceDescription(*row));
  if (face.count() == data.rows()) {
    FaceWithDataPtr fdPtr(new FaceWithData(face,row));
    rays.push_back(fdPtr);
  }

	for (uint i = 0; i < static_cast<uint>(G->rowsize); ++i) {
		QArrayPtr row(new QArray(data.dimension()));
		row->initFromArray(A[i]);
		const Face f = data.faceDescription(*row);
		FaceWithDataPtr fdPtr(new FaceWithData(f, row, data.incidenceNumber(f)));
		rays.push_back(fdPtr);
	}
	
	dd_FreePolyhedra(poly);
	dd_FreeMatrix(matrix);
	dd_FreeMatrix(G);

	return true;
}

bool RayComputationCDD::firstVertex(const Polyhedron & data, Face & f, QArray & q, bool requireRay) const {
   return m_lrs->firstVertex(data, f, q, requireRay);
}

bool RayComputationCDD::determineRedundancies(Polyhedron & data, std::list<FaceWithData>& myRays) const {
	dd_MatrixPtr matrix;
	if (!fillModelCDD(data, matrix))
		return false;
	
	std::list<ulong> redundancies;
	dd_ErrorType err;
	dd_rowset redundantRows = dd_RedundantRows(matrix, &err);
	if (err != dd_NoError) {
		dd_FreeMatrix(matrix);
		return false;
	}
	
	for (uint i = 0; i < static_cast<uint>(set_card(redundantRows)); ++i)
		if (set_member(i+1, redundantRows))
			redundancies.push_back(i);
	
	data.addRedundancies(redundancies);
	
	set_free(redundantRows);
	dd_FreeMatrix(matrix);
	
	return true;
}

double RayComputationCDD::estimate(const sympol::Polyhedron& data, std::list<FaceWithData>& rays) const
{
    return m_lrs->estimate(data, rays);
}

bool RayComputationCDD::getLinearities(const Polyhedron & data, std::list<QArrayPtr>& linearities) const {
	return m_lrs->getLinearities(data, linearities);
}

bool RayComputationCDD::fillModelCDD(const Polyhedron & data, dd_MatrixPtr& matrix) const {
	matrix = dd_CreateMatrix(data.rows(), data.dimension());
	if (!matrix)
		return false;
	
	matrix->representation = dd_Inequality;
	matrix->numbtype = dd_GetNumberType("rational");
	
	uint i = 0;
	BOOST_FOREACH(const QArray& row, data.rowPair()) {
		for (uint j = 0; j < data.dimension(); ++j) {
			dd_set(matrix->matrix[i][j], row[j]);
		}
		if (data.isLinearity(row))
			set_addelem(matrix->linset, i+1);
		++i;
	}
	
	return true;
}


