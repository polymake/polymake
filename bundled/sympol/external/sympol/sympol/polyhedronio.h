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

#ifndef POLYHEDRONIO_H
#define POLYHEDRONIO_H

#include "yal/logger.h"

#include <iostream>
#include <list>

namespace sympol {

class Polyhedron;
class FacesUpToSymmetryList;
class QArray;
  
class PolyhedronIO {
	public:
		static Polyhedron* read (std::istream& is, std::list<boost::shared_ptr<PERM> >& groupGenerators, std::vector<ulong>& groupBase);
		static void write(const FacesUpToSymmetryList& rays, bool homogenized, std::ostream& os);
		static void write(const boost::shared_ptr<QArray>& row, bool homogenized, std::ostream& os);
		/**
		 * writes a polyhedron to the given output stream, all redundancies are printed and marked as such
		 */
		static void write(const Polyhedron& poly, std::ostream& os);
		/**
		 * writes a polyhedron to the given output stream, all redundancies are filtered and not printed
		 */
		static void writeRedundanciesFiltered(const sympol::Polyhedron& poly, std::ostream& os);
	private:
		static yal::LoggerPtr logger;
};

}

#endif // POLYHEDRONIO_H
