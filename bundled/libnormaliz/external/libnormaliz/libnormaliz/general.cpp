/*
 * Normaliz
 * Copyright (C) 2007-2022  W. Bruns, B. Ichim, Ch. Soeger, U. v. d. Ohe
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

#include <cstdlib>
#include <csignal>
#ifdef NMZ_DEVELOP
#include <sys/time.h>
#endif
#include "libnormaliz/general.h"

namespace libnormaliz {

bool verbose = false;

volatile sig_atomic_t nmz_interrupted = 0;
const long default_thread_limit = 8;
long thread_limit = default_thread_limit;
bool parallelization_set = false;

// bool test_arithmetic_overflow = false;
// long overflow_test_modulus = 15401;

size_t GMP_mat = 0;
size_t GMP_hyp = 0;
size_t GMP_scal_prod = 0;
size_t TotDet = 0;

long cone_recursion_level = 0;
long full_cone_recursion_level = 0;

bool int_max_value_dual_long_computed = false;
bool int_max_value_dual_long_long_computed = false;
bool int_max_value_primary_long_computed = false;
bool int_max_value_primary_long_long_computed = false;

vector<vector<vector<long> > > CollectedAutoms(default_thread_limit);  // for use in nmz_nauty.cpp

#ifdef NMZ_EXTENDED_TESTS
bool test_arith_overflow_full_cone = false;
bool test_arith_overflow_dual_mode = false;
bool test_arith_overflow_descent = false;
bool test_arith_overflow_proj_and_lift = false;
bool test_simplex_parallel = false;
bool test_small_pyramids = false;
bool test_large_pyramids = false;
bool test_linear_algebra_GMP = false;
#endif

#ifdef NMZ_NAUTY
void kill_nauty();
#endif

void interrupt_signal_handler(int signal) {
    nmz_interrupted = 1;
#ifdef NMZ_NAUTY
    kill_nauty();
#endif
}

namespace {
std::ostream* verbose_ostream_ptr = &std::cout;
std::ostream* error_ostream_ptr = &std::cerr;
}  // namespace

bool setVerboseDefault(bool v) {
    // we want to return the old value
    bool old = verbose;
    verbose = v;
    return old;
}

long set_thread_limit(long t) {
    long old = thread_limit;
    parallelization_set = true;
    thread_limit = t;
    CollectedAutoms.resize(t);
    return old;
}

void setVerboseOutput(std::ostream& v_out) {
    verbose_ostream_ptr = &v_out;
}

void setErrorOutput(std::ostream& e_out) {
    error_ostream_ptr = &e_out;
}

std::ostream& verboseOutput() {
    return *verbose_ostream_ptr;
}

std::ostream& errorOutput() {
    return *error_ostream_ptr;
}

#ifdef NMZ_DEVELOP
struct timeval TIME_begin, TIME_end;

void StartTime() {
    gettimeofday(&TIME_begin, 0);
}

void MeasureTime(bool verbose, const std::string& step) {
    gettimeofday(&TIME_end, 0);
    long seconds = TIME_end.tv_sec - TIME_begin.tv_sec;
    long microseconds = TIME_end.tv_usec - TIME_begin.tv_usec;
    double elapsed = seconds + microseconds * 1e-6;
    if (verbose)
        verboseOutput() << step << ": " << elapsed << " sec" << endl;
    TIME_begin = TIME_end;
}
#else
void StartTime() {
    return;
}
void MeasureTime(bool verbose, const std::string& step) {
    return;
}
#endif

} /* end namespace libnormaliz */
