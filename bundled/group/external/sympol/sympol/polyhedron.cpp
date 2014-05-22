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
#include "matrix/matrix.h"
#include "matrix/rank.h"

using namespace permlib;
using namespace sympol;
using namespace sympol::matrix;

yal::LoggerPtr Polyhedron::logger = yal::Logger::getLogger("Polyhedron");



Polyhedron::Polyhedron(PolyhedronDataStorage * stor, Representation representation, const std::set<ulong>& lin, 
                       const std::set<ulong>& red) 
  : m_setLinearities(lin), m_setRedundancies(red), m_polyData(stor), m_homogenized(false),
    m_representation(representation), m_dimension(0)
{ }

Polyhedron::~Polyhedron() {
  YALLOG_DEBUG3(logger, "~Polyhedron");
}

void Polyhedron::addRow(const QArray& row) {
  BOOST_ASSERT(m_polyData != 0);
  
  m_polyData->m_aQIneq.push_back(row);
}

void Polyhedron::addRedundancies(const std::list<ulong>& filteredIndices) {
  if (filteredIndices.empty())
    return;
  std::list<ulong>::const_iterator indexIt = filteredIndices.begin();
  ulong iFiltered = 0;
  
  for (ulong i = 0; i < m_polyData->m_aQIneq.size(); ++i) {
    if (!m_setRedundancies.count(i)) {
      if (*indexIt == iFiltered) {
        m_setRedundancies.insert(i);
        ++indexIt;
        if (indexIt == filteredIndices.end())
          return;
      }
      ++iFiltered;
    }
  }

}

void Polyhedron::setRedundancies(const std::set<ulong>& red) { 
  m_setRedundancies = red;
}

void Polyhedron::setLinearities(const std::set<ulong>& lin) {
  m_setLinearities = lin;
}

void Polyhedron::addLinearity(const sympol::QArray& row)
{
  m_setLinearities.insert(row.index());
}

void Polyhedron::removeLinearity(const sympol::QArray& row)
{
  m_setLinearities.erase(row.index());
}


Face sympol::Polyhedron::faceDescription ( const sympol::QArray& ray ) const {
  Face f(m_polyData->m_aQIneq.size());
  mpq_class sum, temp;
  
  ulong i = 0;
  BOOST_FOREACH(const QArray& row, m_polyData->m_aQIneq) {
    row.scalarProduct(ray, sum, temp);
    
    // if inequality is satisfied with equality
    // the hyperplane is a defining one for the ray
    if (sgn(sum) == 0) {
      f[i] = true;
    }
    
    ++i;
  }
  
  return f;
}

sympol::Polyhedron sympol::Polyhedron::supportCone(const Face& f) const {
  Polyhedron supportCone(m_polyData, H, m_setLinearities, m_setRedundancies);
  for (Face::size_type i = 0; i < f.size(); ++i) {
    if (!f[i] && !m_setLinearities.count(i))
      supportCone.m_setRedundancies.insert(i);
  }
  
  return supportCone;
}

ulong sympol::Polyhedron::incidenceNumber(const Face& f) const {
  ulong incNumber = 0;
    
  for (Face::size_type i = 0; i < f.size(); ++i) {
    // count only active, non-redundant inequalities
    if (f[i] && m_setRedundancies.count(i) == 0) {
      ++incNumber;
    }
  }
  
  return incNumber;
}

Face sympol::Polyhedron::emptyFace() const {
  return Face(m_polyData->m_aQIneq.size());
}

Face Polyhedron::toFace() const {
  Face f(m_polyData->m_aQIneq.size());
  BOOST_FOREACH(ulong e, m_setLinearities) {
    f[e] = 1;
  }
  return f;
}


bool Polyhedron::checkFace(const QArray & ray) const {
    mpq_class sum, temp;

    // check in which hyperplanes the ray lies 
    BOOST_FOREACH(const QArray& row, rowPair()) {
        row.scalarProduct(ray, sum, temp);
        YALLOG_DEBUG4(logger, "sum " << (row.index()) << " : " << sum << "  @ " << row);
    
        const int sign = sgn(sum);
        if (sign < 0) {
            std::cerr << "non-redund inequality " << (row.index()) << " is violated" << std::endl;
            return false;
        } else if (sign > 0 && m_setLinearities.count(row.index())) {
            std::cerr << "equality constraint " << (row.index()) << " is violated" << std::endl;
            return false;
        }
    }
    
    return true;
}

/**
 * computes the axis (sum of all normal vectors of defining hyperplanes) of this polyhedron
 */
const QArray & Polyhedron::axis() const {
  if (!m_qAxis) {
    m_qAxis = boost::shared_ptr<QArray>(new QArray(dimension()));
    for (RowIterator it = rowsBegin(); it != rowsEnd(); ++it) {
      *m_qAxis += *it;
    }
  }
   
  return *m_qAxis;
}

ulong Polyhedron::workingDimension() const {
	if (!m_dimension) {
		typedef Matrix<mpq_class> QMatrix;
		QMatrix qmat(rows(), dimension());
		uint i = 0;
		BOOST_FOREACH(const QArray& row, rowPair()) {
			for (uint j = 0; j < dimension(); ++j) {
				qmat.at(i,j) = mpq_class(row[j]);
			}
			++i;
		}

		Rank<QMatrix> rank(&qmat);
		m_dimension = rank.rank();
	}
	return m_dimension;
}

namespace sympol {
  std::ostream& operator<<(std::ostream& out, const Polyhedron& p) {
    PolyhedronIO::write(p, out);
    return out;
  }
}

