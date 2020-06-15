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

#ifndef LIBNORMALIZ_GENERAL_H_
#define LIBNORMALIZ_GENERAL_H_

#include <iostream>
#include <assert.h>
#include <signal.h>
#include <cstddef>

#ifndef NMZ_MAKEFILE_CLASSIC
#include <libnormaliz/nmz_config.h>
#endif

#ifdef _WIN32
#if defined(DLL_EXPORT)
#define NORMALIZ_DLL_EXPORT __declspec(dllexport)
#elif defined(NORMALIZ_USE_DLL) && !defined(NORMALIZ_USE_STATIC)
#define NORMALIZ_DLL_EXPORT __declspec(dllimport)
#else
#define NORMALIZ_DLL_EXPORT
#endif
#else
#define NORMALIZ_DLL_EXPORT
#endif

#ifndef NMZ_DEVELOP
#include "libnormaliz/version.h"
#endif

#include "libnormaliz/my_omp.h"


#ifdef _WIN32     // for 32 and 64 bit windows
#define NMZ_MPIR  // always use MPIR
#endif

#ifdef NMZ_MPIR  // use MPIR
#include <mpirxx.h>
#else  // otherwise use GMP 
#include <gmpxx.h>
#endif

// in the serial version there is no need to catch-rethrow
#ifndef _OPENMP
#define NCATCH
#endif

#ifdef ENFNORMALIZ
#include <e-antic/renfxx.h>
//#else
//typedef long renf_elem_class;
//typedef long renf_class;
#endif

namespace libnormaliz {

typedef long long MachineInteger;
typedef double nmz_float;
const nmz_float nmz_epsilon = 1.0e-12;

/* this type is used in the entries of keys
 * it has to be able to hold number of generators */
typedef unsigned int key_t;

NORMALIZ_DLL_EXPORT extern bool verbose;
NORMALIZ_DLL_EXPORT extern size_t GMP_mat, GMP_hyp, GMP_scal_prod;
NORMALIZ_DLL_EXPORT extern size_t TotDet;

#ifdef NMZ_EXTENDED_TESTS
NORMALIZ_DLL_EXPORT extern bool test_arith_overflow_full_cone, test_arith_overflow_dual_mode;
NORMALIZ_DLL_EXPORT extern bool test_arith_overflow_descent, test_arith_overflow_proj_and_lift;
NORMALIZ_DLL_EXPORT extern bool test_small_pyramids, test_large_pyramids;
NORMALIZ_DLL_EXPORT extern bool test_linear_algebra_GMP, test_simplex_parallel;
#endif

/*
 * If this variable is set to true, the current computation is interrupted and
 * an InterruptException is raised.
 */
NORMALIZ_DLL_EXPORT extern volatile sig_atomic_t nmz_interrupted;

// NORMALIZ_DLL_EXPORT extern bool nmz_scip; // controls the use of Scip

#define INTERRUPT_COMPUTATION_BY_EXCEPTION              \
    if (nmz_interrupted) {                              \
        throw InterruptException("external interrupt"); \
    }

/* if test_arithmetic_overflow is true, many operations are also done
 * modulo overflow_test_modulus to ensure the correctness of the calculations */
// extern bool test_arithmetic_overflow;
// extern long overflow_test_modulus;

NORMALIZ_DLL_EXPORT extern long default_thread_limit;
NORMALIZ_DLL_EXPORT extern long thread_limit;
NORMALIZ_DLL_EXPORT extern bool parallelization_set;
long set_thread_limit(long t);

/* set the verbose default value */
bool setVerboseDefault(bool v);
/* methods to set and use the output streams */
void setVerboseOutput(std::ostream&);
void setErrorOutput(std::ostream&);

std::ostream& verboseOutput();
std::ostream& errorOutput();

void interrupt_signal_handler(int signal);

} /* end namespace libnormaliz */

#include <libnormaliz/input_type.h>
#include <libnormaliz/normaliz_exception.h>
#include <libnormaliz/cone_property.h>
#include <libnormaliz/integer.h>

#endif /* GENERAL_H_ */
