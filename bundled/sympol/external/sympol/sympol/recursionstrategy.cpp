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

#include "recursionstrategy.h"
#include "polyhedronio.h"

#include <iostream>
#include <fstream>
#include <sstream>

using namespace sympol;
using namespace std;

yal::LoggerPtr RecursionStrategy::logger(yal::Logger::getLogger("RecrStrat "));
uint RecursionStrategy::ms_instanceCounter = 0;

RecursionStrategy::RecursionStrategy() 
	: m_dumpFilename(0), m_usedComputations(), m_compIt(m_usedComputations.begin()), m_recursionDepth(0) 
{ }

RecursionStrategy::~RecursionStrategy() {
	delete m_dumpFilename;
}

bool RecursionStrategy::enumerateRaysUpToSymmetry(const RayComputation* rayComp, const Polyhedron & data, 
						const PermutationGroup & permGroup, FacesUpToSymmetryList& rays)
{
	SymmetryComputation* sd = NULL;
	if (m_dumpFilename && m_compIt != m_usedComputations.end()) {
		sd = this->symmetryComputationFactory((*m_compIt)->method(), rayComp, data, permGroup, rays);
		BOOST_ASSERT( sd != NULL );
		sd->rememberMe(*m_compIt);
		++m_compIt;
		YALLOG_INFO(logger, "load computation " << sd->method() << " from list // " << data.rows());
	} else {
		YALLOG_INFO(logger, "enter rec depth " << m_usedComputations.size() << " // " << data.rows());
		sd = this->devise(rayComp, data, permGroup, rays);
		m_usedComputations.push_back(sd->rememberMe());
		
		if (!Configuration::getInstance().intermediatePolyFilePrefix.empty()) {
			stringstream ss;
			ss << Configuration::getInstance().intermediatePolyFilePrefix << "-" << setw(7) << setfill('0') << ms_instanceCounter << ".ine";
			
			ofstream ofile(ss.str().c_str());
			PolyhedronIO::writeRedundanciesFiltered(data, ofile);
			ofile.close();
		}
		++ms_instanceCounter;
	}
	++m_recursionDepth;
	const bool ret = sd->enumerateRaysUpToSymmetry();

	SymmetryComputationMemento* mem = m_usedComputations.back();
	delete mem;
	m_usedComputations.pop_back();
	--m_recursionDepth;
	
	delete sd;
	return ret;
}

bool RecursionStrategy::resumeComputation(const RayComputation* rayComp, const Polyhedron & data, 
						const PermutationGroup & permGroup, FacesUpToSymmetryList& rays)
{
	m_compIt = m_usedComputations.begin();
	
	return this->enumerateRaysUpToSymmetry(rayComp, data, permGroup, rays);
}

void RecursionStrategy::setDumpfile(const std::string& dumpFile) {
	delete m_dumpFilename;
	m_dumpFilename = new char[dumpFile.size()+1];
	dumpFile.copy(m_dumpFilename, dumpFile.size(), 0);
	m_dumpFilename[dumpFile.size()] = 0;
}
