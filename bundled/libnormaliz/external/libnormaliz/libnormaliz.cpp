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

#include "libnormaliz.h"
#include "general.h"

namespace libnormaliz {

bool verbose = false;

bool test_arithmetic_overflow = false;
long overflow_test_modulus = 15401;

namespace {
    std::ostream* verbose_ostream_ptr = &std::cout;
    std::ostream* error_ostream_ptr = &std::cerr;
} // end anonymous namespace, only accessible in this file (and when it is included)

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

InputType to_type(const std::string& type_string) {
    if (type_string=="0"||type_string=="integral_closure") {
        return Type::integral_closure;
    }
    if (type_string=="1"||type_string=="normalization") {
        return Type::normalization;
    }
    if (type_string=="2"||type_string=="polytope") {
        return Type::polytope;
    }
    if (type_string=="3"||type_string=="rees_algebra") {
        return Type::rees_algebra;
    }
    if (type_string=="4"||type_string=="hyperplanes") {
        return Type::hyperplanes;
    }
    if (type_string=="5"||type_string=="equations") {
        return Type::equations;
    }
    if (type_string=="6"||type_string=="congruences") {
        return Type::congruences;
    }
    if (type_string=="signs") {
        return Type::signs;
    }
    if (type_string=="10"||type_string=="lattice_ideal") {
        return Type::lattice_ideal;
    }
    if (type_string=="grading") {
        return Type::grading;
    }
    
    std::cerr<<"ERROR: Unknown type \""<<type_string<<"\"!"<<std::endl;
    throw BadInputException();
    return Type::integral_closure;
}



} /* end namespace libnormaliz */
