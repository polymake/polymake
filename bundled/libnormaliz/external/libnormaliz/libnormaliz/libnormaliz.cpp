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

#include "libnormaliz/libnormaliz.h"
#include "libnormaliz/general.h"

namespace libnormaliz {

bool verbose = false;

// bool test_arithmetic_overflow = false;
// long overflow_test_modulus = 15401;

size_t GMP_mat=0;
size_t GMP_hyp=0;
size_t GMP_scal_prod=0;
size_t TotDet=0;


namespace {
    std::ostream* verbose_ostream_ptr = &std::cout;
    std::ostream* error_ostream_ptr = &std::cerr;
} // end anonymous namespace, only accessible in this file (and when it is included)

bool setVerboseDefault(bool v) {
    //we want to return the old value
    bool old = verbose;
    verbose = v;
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

InputType to_type(const std::string& type_string) {

    if ( type_string=="0" || type_string=="1" || type_string=="2" || type_string=="3"
      || type_string=="4" || type_string=="5" || type_string=="6"
      || type_string=="hyperplanes"
      || type_string=="10") {
        errorOutput() << "Error: deprecated type \"" << type_string<<"\", please use new type string!" << std::endl;
        throw BadInputException();
    }

    if (type_string=="0"||type_string=="integral_closure") {
        return Type::integral_closure;
    }
    if (type_string=="polyhedron") {
        return Type::polyhedron;
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
    if (type_string=="4"||type_string=="hyperplanes" ||type_string=="inequalities") {
        return Type::inequalities;
    }
    if (type_string=="strict_inequalities") {
         return Type::strict_inequalities;
    }
    if (type_string=="strict_signs") {
        return Type::strict_signs;
    }
    if (type_string=="inhom_inequalities") {
        return Type::inhom_inequalities;
    }
    if (type_string=="dehomogenization") {
         return Type::dehomogenization;
    }
    if (type_string=="5"||type_string=="equations") {
        return Type::equations;
    }
    if (type_string=="inhom_equations") {
        return Type::inhom_equations;
    }
    if (type_string=="6"||type_string=="congruences") {
        return Type::congruences;
    }
    if (type_string=="inhom_congruences") {
        return Type::inhom_congruences;
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
    if (type_string=="excluded_faces") {
        return Type::excluded_faces;
    }
    if (type_string=="lattice") {
        return Type::lattice;
    }
    if (type_string=="saturation") {
        return Type::saturation;
    }
    if (type_string=="cone") {
        return Type::cone;
    }
    if (type_string=="offset") {
        return Type::offset;
    }
    if (type_string=="vertices") {
        return Type::vertices;
    }
    if (type_string=="support_hyperplanes") {
        return Type::support_hyperplanes;
    }
    if (type_string=="cone_and_lattice") {
        return Type::cone_and_lattice;
    }

    errorOutput() << "ERROR: Unknown type \"" << type_string<<"\"!" << std::endl;
    throw BadInputException();
    return Type::integral_closure;
}

long type_nr_columns_correction(InputType t) {
    if (t == Type::polytope || t == Type::rees_algebra)
        return -1;
    if (t == Type::congruences || t == Type::vertices || t == Type::polyhedron
     || t == Type::inhom_inequalities || t == Type::inhom_equations)
        return 1;
    if (t == Type::inhom_congruences)
        return 2;
    return 0;
}

/* returns true if the input of this type is a vector */
bool type_is_vector(InputType type){
    if (type == Type::grading || type == Type::signs || type == Type::strict_signs
            || type == Type::dehomogenization || type == Type::offset) {
        return true;
    }
    return false;
}

} /* end namespace libnormaliz */
