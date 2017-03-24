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


#include "polyhedron.h"
#include "polyhedronio.h"
#include "qarray.h"
#include "facesuptosymmetrylist.h"
#include <string>
#include <sstream>

#include <boost/algorithm/string/trim.hpp>

using namespace sympol;
using namespace yal;

LoggerPtr PolyhedronIO::logger(Logger::getLogger("PolyhedrIO"));

Polyhedron* sympol::PolyhedronIO::read (std::istream& is, std::list<boost::shared_ptr<PERM> >& groupGenerators, std::vector<ulong>& groupBase) {
  Polyhedron* poly = 0;
  
  std::string line;
  /*
   * the modes are:
   * 0 - do nothing
   * h - we expect an H-representation (or V-representation)
   * D - we expect dimensions of input
   * H - we expect inequalities for H-description (or vertices/rays for V-representation)
   * p - we expect the number of generators for a permutation group
   * P - we expect the generating cycles for a permutation group
   * b - we expect the number of base points (or 0 if no (partial) base is known)
   * B - we expect a permutation group base
   */
  char inputMode = '0';
  unsigned long ulQOffset = 0;
  unsigned long ulGenerators;
  unsigned long ulBase = 0;
  
  std::set<ulong> setLinearities;
  std::set<ulong> setRedundancies;
  ulong expectedInequalities = 0;
  ulong dimension = 0;
  std::list<std::string> inequalityRows;
	std::list<std::string> generatorRows;
	Polyhedron::Representation representation = Polyhedron::H;
  
  do {
    std::getline(is, line);
    boost::trim_left(line);
		
		// comments
		if (line.length() > 0 && (line.at(0) == '#' || line.at(0) == '*'))
			continue;
    
    if (line.compare("H-representation") == 0) {
      inputMode = 'h';
      representation = Polyhedron::H;
      continue;
    }
    if (line.compare("V-representation") == 0) {
      inputMode = 'h';
      representation = Polyhedron::V;
      continue;
    }
    if (line.compare("begin") == 0 && inputMode == 'h') {
      inputMode = 'D';
      continue;
    }
    if (line.compare("permutation group") == 0) {
      inputMode = 'p';
      continue;
    }
    if (line.find("linearity") != std::string::npos) {
      std::stringstream ss(line.substr(9));
      unsigned long k;
      unsigned long temp;
      ss >> k;
      for (unsigned long i=0; i<k && !ss.eof(); ++i) {
        ss >> temp;
        // the number corresponds to an inequality number stating at 1
        // our counting starts at 0
        setLinearities.insert(temp - 1);
      }
    }
    if (line.find("redundant") != std::string::npos) {
      std::stringstream ss(line.substr(9));
      unsigned long k;
      unsigned long temp;
      ss >> k;
      for (unsigned long i=0; i<k && !ss.eof(); ++i) {
        ss >> temp;
        // the number corresponds to an inequality number stating at 1
        // our counting starts at 0
        setRedundancies.insert(temp - 1);
      }
    }
    if (line.compare("end") == 0) {
      inputMode = '0';
    }

    if (inputMode == 'D') {
      std::stringstream ss(line);
      
      ss >> expectedInequalities;
      ss >> dimension;
      
      ulQOffset = 0;
      
      inputMode = 'H';
      continue;
    }
    
    if (inputMode == 'H') {
      inequalityRows.push_back(line);
    }
    
    if (inputMode == 'p') {
      std::stringstream ss(line);
      
      ss >> ulGenerators;
      
      if (ulGenerators == 0)
      	inputMode = '0';
      else
      	inputMode = 'P';
      continue;
    }
    
    if (inputMode == 'P') {
			generatorRows.push_back(line);
      
      // if all generators have been read, switch to default mode
      if (--ulGenerators == 0) {
        inputMode = 'b';
      }
      continue;
    }
    
    if (inputMode == 'b') {
			std::stringstream ss(line);

			ss >> ulBase;
			if (ulBase > 0) {
				groupBase.resize(ulBase);
				inputMode = 'B';
			} else
				inputMode = '0';
			continue;
		}

		if (inputMode == 'B') {
			std::stringstream ss(line);
			for (uint i = 0; i < ulBase; ++i) {
				ss >> groupBase[i];
				--groupBase[i];
			}
			inputMode = '0';
			continue;
		}

  } while (line.length() > 0);
  
  if (!dimension) {
    YALLOG_WARNING(logger, "no information about problem dimension found");
    return 0;
  }
  
  if (inequalityRows.size() != expectedInequalities)
    YALLOG_WARNING(logger, "input file specified " << expectedInequalities << " inequalities/rays/vertices but " << inequalityRows.size() << " were found");
  
  bool homogeneous = true;
  BOOST_FOREACH(const std::string& row, inequalityRows) {
    if (row.at(0) != '0') {
    	homogeneous = false;
      break;
    }
  }
  
  if (!homogeneous)
    ++dimension;
  
  BOOST_ASSERT(poly == 0);
  poly = new Polyhedron(PolyhedronDataStorage::createStorage(dimension, expectedInequalities), 
                        representation,
                        setLinearities, setRedundancies);
      
  
	typedef std::map<QArray, unsigned int> RowMap;
	RowMap uniqueRows;
	unsigned int skipped = 0;
	BOOST_FOREACH(const std::string& row, inequalityRows) {
		std::stringstream ss(row);
		// don't read directly into p.m_aQIneq[ulQOffset]
		// because STL-vector seems to get confused
		QArray dummy(poly->dimension(), ulQOffset, !homogeneous);
		ss >> dummy;
		unsigned int origOffset = ulQOffset;
		RowMap::iterator mapIt = uniqueRows.find(dummy);
		if (mapIt == uniqueRows.end()) {
			uniqueRows.insert(std::make_pair(dummy, ulQOffset));
			poly->addRow(dummy);
			++ulQOffset;
		} else {
			origOffset = (*mapIt).second;
			YALLOG_DEBUG(logger, "skipping duplicate inequality " << row);
			++skipped;
		}
		
		// correct indices after deleting duplicate rows
		std::set<unsigned long>::iterator setIt;
		setIt = setLinearities.find(ulQOffset + skipped - 1);
		if (setIt != setLinearities.end()) {
			setLinearities.erase(setIt);
			setLinearities.insert(origOffset);
		}
		setIt = setRedundancies.find(ulQOffset + skipped - 1);
		if (setIt != setRedundancies.end()) {
			setRedundancies.erase(setIt);
			setRedundancies.insert(origOffset);
		}
	}
	
	if (skipped) {
		YALLOG_INFO(logger, "skipped " << skipped << " duplicate inequalities");
		poly->setRedundancies(setRedundancies);
		poly->setLinearities(setLinearities);
	}

	if (!homogeneous)
		poly->setHomogenized();

  
  BOOST_FOREACH(const std::string& row, generatorRows) {
    boost::shared_ptr<PERM> gen(new PERM(poly->realRowNumber(), row));
    groupGenerators.push_back(gen);
  }
  
  return poly;
}

void sympol::PolyhedronIO::write(const FacesUpToSymmetryList& rays, bool homogenized, std::ostream& os) {
	BOOST_FOREACH(const FaceWithDataPtr& r, std::make_pair(rays.begin(), rays.end())) {
		write(r->ray, homogenized, os);
	}
}

void sympol::PolyhedronIO::write(const QArrayPtr& row, bool homogenized, std::ostream& os) {
	if (!homogenized) {
		QArray copy(*row);
		copy.normalizeArray(0);
		os << " " << copy << std::endl;
  } else {
		if (mpq_sgn((*row)[0]) != 0)
			// skip the origin (apex of the homogenized polyhedral cone)
			return;
		
		QArray copy(*row);
		// compute intersection with hyperplane {x_1 = 1}
		copy.normalizeArray(1);
		for (uint i = 1; i < copy.size(); ++i)
			os << " " << copy[i];
		os << std::endl;
	}
}

void sympol::PolyhedronIO::write(const sympol::Polyhedron& poly, std::ostream& os) {
  if (poly.m_representation == Polyhedron::H)
    os << "H-representation" << std::endl;
  else if (poly.m_representation == Polyhedron::V)
    os << "V-representation" << std::endl;
  if (!poly.m_setLinearities.empty()) {
    os << "linearity " << poly.m_setLinearities.size() << " ";
    BOOST_FOREACH(ulong l, poly.m_setLinearities) {
      os << (l+1) << " ";
    }
    os << std::endl;
  } 
  if (!poly.m_setRedundancies.empty()) {
    os << "redundant " << poly.m_setRedundancies.size() << " ";
    std::set<ulong>::const_iterator it;
    BOOST_FOREACH(ulong l, poly.m_setRedundancies) {
      os << (l+1) << " ";
    }
    os << std::endl;
  }
  os << "begin" << std::endl;
  os << poly.m_polyData->m_ulIneq << " " << poly.m_polyData->m_ulSpaceDim << " rational" << std::endl;
  for (unsigned long j = 0; j < poly.m_polyData->m_ulIneq; ++j) {     
    for (unsigned long i = 0; i < poly.m_polyData->m_ulSpaceDim; ++i) {
      os << mpq_class(poly.m_polyData->m_aQIneq[j][i]);
      if (i < poly.m_polyData->m_ulSpaceDim - 1) {
        os << ' ';
      } else {
        os << std::endl;
      }
    }
  }
  os << "end" << std::endl;
}

void sympol::PolyhedronIO::writeRedundanciesFiltered(const sympol::Polyhedron& poly, std::ostream& os) {
  if (poly.m_representation == Polyhedron::H)
    os << "H-representation" << std::endl;
  else if (poly.m_representation == Polyhedron::V)
    os << "V-representation" << std::endl;
  
  std::list<ulong> correctedLinearities;
  uint index = 1;
  BOOST_FOREACH(const QArray& row, poly.rowPair()) {
		if (poly.isLinearity(row)) {
			correctedLinearities.push_back(index);
		}
		++index;
	}
	if (!correctedLinearities.empty()) {
		os << "linearity " << correctedLinearities.size() << " ";
    BOOST_FOREACH(ulong l, correctedLinearities) {
      os << l << " ";
    }
    os << std::endl;
	}
	
  os << "begin" << std::endl;
  os << poly.rows() << " " << poly.m_polyData->m_ulSpaceDim << " rational" << std::endl;
  BOOST_FOREACH(const QArray& row, poly.rowPair()) {
		os << row << std::endl;
  }
  os << "end" << std::endl;
}
