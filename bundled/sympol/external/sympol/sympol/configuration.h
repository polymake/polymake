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


#ifndef SYMPOL_CONFIGURATION_H_
#define SYMPOL_CONFIGURATION_H_

#include "common.h"

namespace sympol {

class Configuration {
private:
	Configuration();
	~Configuration() {}
	Configuration(const Configuration &);             // intentionally undefined
	Configuration & operator=(const Configuration &); // intentionally undefined
public:
	static Configuration& getInstance();

	uint lrsEstimates;
	uint lrsEstimateMaxDepth;
	uint computeOrbitLimit;
	bool computeCanonicalRepresentatives;
	std::string intermediatePolyFilePrefix;
};

}

#endif /* SYMPOL_CONFIGURATION_H_ */
