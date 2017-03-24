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

#ifndef SYMPOL_COMPUTESYMMETRIES_H_
#define SYMPOL_COMPUTESYMMETRIES_H_

#include "matrixconstructioneigen.h"
#include "matrixconstructiondefault.h"
#include "graphconstructiondefault.h"
#include "graphconstructionbliss.h"

#include <boost/scoped_ptr.hpp>

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

namespace sympol {

/// class to compute restricted symmetries of a polyhedron
class ComputeSymmetries {
public:
	/**
	 * @param useBliss true if graph automorphisms should be computed with bliss
	 * @param useEigen true if graph should be constructed with floating-point arithmetic
	 */
	ComputeSymmetries(bool useBliss, bool useEigen) : m_useBliss(useBliss), m_useEigen(useEigen) { }
	
	/// computes the restricted symmetries of a polyhedron
	boost::shared_ptr<PermutationGroup> compute(const Polyhedron& poly) const {
		boost::scoped_ptr<MatrixConstruction> matCons;
		boost::scoped_ptr<GraphConstruction> graphCons;

#if HAVE_EIGEN
		if (m_useEigen)
			matCons.reset(new MatrixConstructionEigen());
		else
#endif // HAVE_EIGEN
			matCons.reset(new MatrixConstructionDefault());

#if HAVE_BLISS
		if (m_useBliss)
			graphCons.reset(new GraphConstructionBliss());
		else
#endif // HAVE_BLISS
			graphCons.reset(new GraphConstructionDefault());
		
		BOOST_ASSERT(matCons);
		BOOST_ASSERT(graphCons);
		
		if (!matCons->construct(poly))
			return boost::shared_ptr<PermutationGroup>();
		
		boost::shared_ptr<PermutationGroup> group = graphCons->compute(matCons.get());
		
		if (!matCons->checkSymmetries(group))
			return boost::shared_ptr<PermutationGroup>();
		
		return group;
	}
private:
	const bool m_useBliss;
	const bool m_useEigen;
};

}

#endif // SYMPOL_COMPUTESYMMETRIES_H_
