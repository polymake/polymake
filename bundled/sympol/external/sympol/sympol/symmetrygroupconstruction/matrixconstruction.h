// ---------------------------------------------------------------------------
//
// This file is part of SymPol
//
// Copyright (C) 2006-2012  Thomas Rehn <thomas@carmen76.de>
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

#include "../common.h"
#include "../polyhedron.h"

#ifndef SYMPOL_MATRIXCONSTRUCTION_H_
#define SYMPOL_MATRIXCONSTRUCTION_H_

namespace sympol {

class MatrixConstruction {
public:
	MatrixConstruction() : m_dimension(0), m_numberOfWeights(0) { }
	virtual ~MatrixConstruction() {}
	virtual bool construct(const Polyhedron& poly) = 0;
	virtual bool checkSymmetries(boost::shared_ptr<PermutationGroup>& group) const = 0;
	
	unsigned int k() const { return m_numberOfWeights; }
	unsigned int dimension() const { return m_dimension; }
	unsigned int at(unsigned int i, unsigned int j) const { return this->weightAt(i,j); }
	bool isEquation(unsigned int i) const { return m_linearities.count(i) > 0; }
	const std::set<unsigned int>& linearities() const { return m_linearities; }
protected:
	virtual unsigned int weightAt(unsigned int i, unsigned int j) const = 0;
	void initData(const Polyhedron& poly, unsigned int numberOfWeights);
	
	std::set<unsigned int> m_linearities;
	unsigned int m_dimension;
	unsigned int m_numberOfWeights;
};

} // end NS

#endif // SYMPOL_MATRIXCONSTRUCTION_H_
