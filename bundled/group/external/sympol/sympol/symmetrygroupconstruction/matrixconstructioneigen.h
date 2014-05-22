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

#include "matrixconstruction.h"

#ifndef SYMPOL_MATRIXCONSTRUCTIONEIGEN_H_
#define SYMPOL_MATRIXCONSTRUCTIONEIGEN_H_

#if HAVE_EIGEN

#include <Eigen/Dense>

namespace sympol {

static const long double zeroTolerance = 1e-9;

/// tolerant comparison for double floating points
template<typename T>
struct DoubleCmp {
	bool operator()(T a, T b) const {
		const T eps = zeroTolerance;
		if (a - b < -eps)
			return true;
		return false;
	}
};

class MatrixConstructionEigen : public MatrixConstruction {
public:
	MatrixConstructionEigen() {}
	virtual ~MatrixConstructionEigen() { }
	virtual bool construct(const Polyhedron& poly);
	virtual bool checkSymmetries(boost::shared_ptr<PermutationGroup>& group) const;
protected:
	virtual unsigned int weightAt(unsigned int i, unsigned int j) const;
	
	typedef long double FloatType;
	typedef Eigen::Matrix<FloatType, Eigen::Dynamic, Eigen::Dynamic> FloatMatrix;
	typedef std::map<FloatType, unsigned int, DoubleCmp<FloatType> > WeightMap;
	
	FloatMatrix m_W;
	FloatMatrix m_Acopy;
	WeightMap m_weights;
	std::vector<unsigned int> m_rowBasis;
};

} // end NS

#endif // HAVE_EIGEN
#endif // SYMPOL_MATRIXCONSTRUCTIONEIGEN_H_
