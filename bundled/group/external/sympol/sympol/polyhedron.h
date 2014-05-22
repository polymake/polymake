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


#ifndef POLYHEDRON_H
#define POLYHEDRON_H

#include "common.h"
#include "polyhedrondatastorage.h"
#include "yal/logger.h"

#include <set>
#include <boost/iterator/filter_iterator.hpp>

namespace sympol {
  
class Polyhedron {
  public:
    enum Representation {
      H, V
    };
    
    Polyhedron (PolyhedronDataStorage * stor, Representation representation, const std::set<ulong>& lin, const std::set<ulong>& red);
    ~Polyhedron();
    
    void addRow(const QArray& row);
    void setHomogenized() { m_homogenized = true; }
    bool homogenized() const { return m_homogenized; }
    // indices with respect to rowsBegin/rowsEnd, i.e. all previous non redundant rows
    // list is expected to be sorted ascendingly
    void addRedundancies(const std::list<ulong>& filteredIndices);
    void setRedundancies(const std::set<ulong>& red);
    void setLinearities(const std::set<ulong>& lin);
    void addLinearity(const QArray& row);
    void removeLinearity(const QArray& row);
    std::list<ulong> linearities() const { return std::list<ulong>(m_setLinearities.begin(), m_setLinearities.end()); }
    
    ulong dimension() const { return m_polyData->m_ulSpaceDim; }
    /// number of rows, not counting redundant ones
    ulong rows() const { return m_polyData->m_aQIneq.size() - m_setRedundancies.size(); }
    /// number of rows, even redundant ones
    ulong realRowNumber() const { return m_polyData->m_aQIneq.size(); }
    ulong realLinearitiesNumber() const { return m_setLinearities.size(); }
    Representation representation() const { return this->m_representation; }
    
    const QArray& row(ulong i) const { return m_polyData->m_aQIneq[i]; }
    const mpq_t& element(ulong i, ulong j) const { return m_polyData->m_aQIneq[i][j]; }
    
    Face emptyFace() const;
    Face toFace() const;
    Face faceDescription(const QArray& ray) const;
    Polyhedron supportCone(const Face& f) const;
    ulong incidenceNumber(const Face& f) const;
    bool checkFace(const QArray& ray) const;
    
    /// WARNING: value may be cached
    const QArray & axis() const;

		/// WARNING: value may be cached
    ulong workingDimension() const;
  private:
    std::set<unsigned long>  m_setLinearities;
    std::set<unsigned long>  m_setRedundancies;
    PolyhedronDataStorage*   m_polyData;
    bool                     m_homogenized;
    Representation           m_representation;
    
    // store "axis" of polyhedron,
    // sum of all normal vectors of defining hyperplanes
    mutable boost::shared_ptr<QArray> m_qAxis;
    mutable ulong m_dimension;

  public:
    bool isLinearity(const QArray& row) const { return m_setLinearities.count(row.index()) > 0; }
    bool isRedundancy(const QArray& row) const { return m_setRedundancies.count(row.index()) > 0; }
    
    struct is_non_redundant {
      is_non_redundant(const Polyhedron& poly) : m_poly(poly) {}
      bool operator()(const QArray& row) const {
        return !m_poly.isRedundancy(row);
      }
      private:
        const Polyhedron& m_poly;
    };
    typedef boost::filter_iterator<is_non_redundant, std::vector<QArray>::const_iterator> RowIterator;
    
    RowIterator rowsBegin() const { 
      return RowIterator(is_non_redundant(*this), m_polyData->m_aQIneq.begin(), m_polyData->m_aQIneq.end());
    }
    RowIterator rowsEnd() const { 
      return RowIterator(is_non_redundant(*this), m_polyData->m_aQIneq.end(), m_polyData->m_aQIneq.end());
    }
    std::pair<RowIterator,RowIterator> rowPair() const { return std::make_pair(rowsBegin(), rowsEnd()); }
    
    friend class PolyhedronIO;
    friend std::ostream& operator<<(std::ostream& out, const Polyhedron& pc);
    
  private:
    static yal::LoggerPtr logger;
};

}

#endif
