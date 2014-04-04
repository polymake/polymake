/*
 * Normaliz
 * Copyright (C) 2007-2013  Winfried Bruns, Bogdan Ichim, Christof Soeger
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef GENERAL_H_
#define GENERAL_H_


#include <iostream>
#include <assert.h>

#ifdef _WIN32 //for 32 and 64 bit windows
    #define NMZ_MPIR //always use MPIR
#endif

#ifdef NMZ_MPIR // use MPIR
    #include <mpirxx.h>
#else         // otherwise use GMP
    #include <gmpxx.h>
#endif


#include "libnormaliz.h"
#include "normaliz_exception.h"
#include "cone_property.h"

namespace libnormaliz {


} /* end namespace libnormaliz */

#endif /* GENERAL_H_ */
