/*
 * Normaliz
 * Copyright (C) 2007-2019  Winfried Bruns, Bogdan Ichim, Christof Soeger
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

#ifdef NMZ_MIC_OFFLOAD
#pragma offload_attribute(push, target(mic))
#endif

#include "libnormaliz/HilbertSeries.cpp"

#ifdef NMZ_COCOA
#include "libnormaliz/nmz_integrate.h"

namespace libnormaliz {
bool verbose_INT;
}

#include "libnormaliz/nmz_polynomial.cpp"
#include "libnormaliz/nmz_integral.cpp"

#endif  // NMZ_COCOA

#ifdef NMZ_MIC_OFFLOAD
#pragma offload_attribute(pop)
#endif
