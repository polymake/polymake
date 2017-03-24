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

#include "configuration.h"

namespace sympol {

Configuration::Configuration() 
	: lrsEstimates(8),
		lrsEstimateMaxDepth(3),
		computeOrbitLimit(1024),
		computeCanonicalRepresentatives(false)
{ }

Configuration& Configuration::getInstance() {
	// This local static variable is only instantiated if and when getInstance() is called.
	// Although the C++ Standard (still ISO/IEC 14882:2003 as of May 2010) does not
	// describe behavior in a multithreaded environment, a commonly used C++ ABI provides
	// a lock around its initialization (check for __cxa_guard_acquire, __cxa_guard_release),
	// but it is not clear how this synchronizes with object destruction (are the correct
	// memory fences in place?), and whether objects are correctly destroyed in reverse
	// order of construction.
	static Configuration instance;
	return instance;
}

}
