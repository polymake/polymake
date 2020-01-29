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

#ifndef LIBNORMALIZ_INPUT_TYPE_H_
#define LIBNORMALIZ_INPUT_TYPE_H_

#include <iostream>
#include <string>

#include "libnormaliz/general.h"

namespace libnormaliz {

namespace Type {
enum InputType {
    //
    // homogeneous generators
    //
    polytope,
    rees_algebra,
    subspace,
    cone,
    cone_and_lattice,
    lattice,
    saturation,
    //
    // inhomogeneous generators
    //
    vertices,
    offset,
    //
    // homogeneous constraints
    //
    inequalities,
    signs,
    equations,
    congruences,
    //
    // inhomogeneous constraints
    //
    inhom_equations,
    inhom_inequalities,
    strict_inequalities,
    strict_signs,
    inhom_congruences,
    //
    // linearforms
    //
    grading,
    dehomogenization,
    //
    // special
    open_facets,
    projection_coordinates,
    excluded_faces,
    lattice_ideal,
    //
    // prwecomputed data
    //
    support_hyperplanes,
    extreme_rays,
    hilbert_basis_rec_cone,
    //
    // deprecated
    //
    integral_closure,
    normalization,
    polyhedron,
    // internal
    scale,
    //
    add_cone,
    add_vertices,
    add_inequalities,
    add_inhom_inequalities
};
}  // end namespace Type

using Type::InputType;

/* converts a string to an InputType
 * throws an BadInputException if the string cannot be converted */
InputType to_type(const std::string& type_string);

/* gives the difference of the number of columns to the dimension */
long type_nr_columns_correction(InputType type);

/* returns true if the input of this type is a vector */
bool type_is_vector(InputType type);

/* returns true if the input of this type is a number */
bool type_is_number(InputType type);

namespace NumParam {
enum Param {
    expansion_degree,
    nr_coeff_quasipol,
    face_codim_bound,
    autom_codim_bound_vectors,
    autom_codim_bound_mult,
    not_a_num_param
};
}  // end namespace NumParam

bool isNumParam(NumParam::Param& numpar, const std::string& type_string);
NumParam::Param to_numpar(const std::string& type_string);
std::string numpar_to_string(const NumParam::Param& numpar);

} /* end namespace libnormaliz */

#endif /* LIBNORMALIZ_H_ */
