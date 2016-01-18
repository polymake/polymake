/*
 * Normaliz
 * Copyright (C) 2007-2014  Winfried Bruns, Bogdan Ichim, Christof Soeger
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
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

#ifndef GENERAL_H_
#define GENERAL_H_


#include <iostream>
#include <assert.h>
#include <cstddef>

#ifdef _WIN32 //for 32 and 64 bit windows
    #define NMZ_MPIR //always use MPIR
#endif

#ifdef NMZ_MPIR // use MPIR
    #include <mpirxx.h>
#else         // otherwise use GMP
    #include <gmpxx.h>
#endif

// in the serial version there is no need to catch-rethrow
#ifndef _OPENMP
    #define NCATCH
#endif

namespace libnormaliz {

typedef long long MachineInteger;

} /* end namespace libnormaliz */

#include <libnormaliz/libnormaliz.h>
#include <libnormaliz/normaliz_exception.h>
#include <libnormaliz/cone_property.h>

#endif /* GENERAL_H_ */
