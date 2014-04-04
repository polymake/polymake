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

#ifndef LIBNORMALIZ_H_
#define LIBNORMALIZ_H_

#include <iostream>
#include <string>

namespace libnormaliz {

namespace Type {
enum InputType {
    integral_closure,
    normalization,
    polytope,
    rees_algebra,
    hyperplanes,
    signs,
    equations,
    congruences,
    inhomogeneous_hyperplanes,
    inhomogeneous_equations,
    inhomogeneous_congruences,
    lattice_ideal,
    grading
};
} //end namespace Type

namespace Mode {
enum ComputationMode {
    supportHyperplanes,
    triangulationSize,
    triangulation,
    volumeTriangulation,
    volumeLarge,
    degree1Elements,
    hilbertBasisTriangulation,
    hilbertBasisMultiplicity,
    hilbertBasisLarge,
    hilbertSeries,
    hilbertSeriesLarge,
    hilbertBasisSeries,
    hilbertBasisSeriesLarge,
    dual
};
} //end namespace Mode

using Type::InputType;
using Mode::ComputationMode;

/* converts a string to an InputType
 * throws an BadInputException if the string cannot be converted */
InputType to_type(const std::string& type_string);

/* this type is used in the entries of keys
 * it has to be able to hold number of generators */
typedef unsigned int key_t;

extern bool verbose;

/* if test_arithmetic_overflow is true, many operations are also done
 * modulo overflow_test_modulus to ensure the correctness of the calculations */
extern bool test_arithmetic_overflow;
extern long overflow_test_modulus;

/* methods to set and use the output streams */
void setVerboseOutput(std::ostream&);
void setErrorOutput(std::ostream&);

std::ostream& verboseOutput();
std::ostream& errorOutput();

} /* end namespace libnormaliz */

#endif /* LIBNORMALIZ_H_ */
