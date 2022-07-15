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
    rational_lattice,
    //
    // inhomogeneous generators
    //
    vertices,
    offset,
    rational_offset,
    //
    // homogeneous constraints
    //
    inequalities,
    signs,
    equations,
    congruences,
    excluded_faces,
    //
    // inhomogeneous constraints
    //
    inhom_equations,
    inhom_inequalities,
    strict_inequalities,
    strict_signs,
    inhom_congruences,
    inhom_excluded_faces,
    //
    // linearforms
    //
    grading,
    dehomogenization,
    //
    // special
    open_facets,
    projection_coordinates,
    lattice_ideal,
    //
    // precomputed data
    //
    support_hyperplanes,
    extreme_rays,
    maximal_subspace,
    generated_lattice,
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
    add_subspace,
    add_vertices,
    add_inequalities,
    add_inhom_inequalities,
    add_equations,
    add_inhom_equations

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
    block_size_hollow_tri,
    decimal_digits,
    not_a_num_param
};
}  // end namespace NumParam

bool isNumParam(NumParam::Param& numpar, const std::string& type_string);
NumParam::Param to_numpar(const std::string& type_string);
std::string numpar_to_string(const NumParam::Param& numpar);

inline InputType to_type(const std::string& type_string) {
    if (type_string == "0" || type_string == "1" || type_string == "2" || type_string == "3" || type_string == "4" ||
        type_string == "5" || type_string == "6" || type_string == "hyperplanes" || type_string == "10") {
        throw BadInputException("Error: deprecated type \"" + type_string + "\", please use new type string!");
    }

    if (type_string == "0" || type_string == "integral_closure") {
        return Type::integral_closure;
    }
    if (type_string == "polyhedron") {
        return Type::polyhedron;
    }
    if (type_string == "1" || type_string == "normalization") {
        return Type::normalization;
    }
    if (type_string == "2" || type_string == "polytope") {
        return Type::polytope;
    }
    if (type_string == "3" || type_string == "rees_algebra") {
        return Type::rees_algebra;
    }
    if (type_string == "4" || type_string == "hyperplanes" || type_string == "inequalities") {
        return Type::inequalities;
    }
    if (type_string == "strict_inequalities") {
        return Type::strict_inequalities;
    }
    if (type_string == "strict_signs") {
        return Type::strict_signs;
    }
    if (type_string == "inhom_inequalities") {
        return Type::inhom_inequalities;
    }
    if (type_string == "dehomogenization") {
        return Type::dehomogenization;
    }
    if (type_string == "5" || type_string == "equations") {
        return Type::equations;
    }
    if (type_string == "inhom_equations") {
        return Type::inhom_equations;
    }
    if (type_string == "6" || type_string == "congruences") {
        return Type::congruences;
    }
    if (type_string == "inhom_congruences") {
        return Type::inhom_congruences;
    }
    if (type_string == "signs") {
        return Type::signs;
    }
    if (type_string == "10" || type_string == "lattice_ideal") {
        return Type::lattice_ideal;
    }
    if (type_string == "grading") {
        return Type::grading;
    }
    if (type_string == "excluded_faces") {
        return Type::excluded_faces;
    }
    if (type_string == "inhom_excluded_faces") {
        return Type::inhom_excluded_faces;
    }
    if (type_string == "lattice") {
        return Type::lattice;
    }
    if (type_string == "rational_lattice") {
        return Type::rational_lattice;
    }
    if (type_string == "saturation") {
        return Type::saturation;
    }
    if (type_string == "cone") {
        return Type::cone;
    }
    if (type_string == "offset") {
        return Type::offset;
    }
    if (type_string == "rational_offset") {
        return Type::rational_offset;
    }
    if (type_string == "vertices") {
        return Type::vertices;
    }
    if (type_string == "support_hyperplanes") {
        return Type::support_hyperplanes;
    }
    if (type_string == "cone_and_lattice") {
        return Type::cone_and_lattice;
    }
    if (type_string == "subspace") {
        return Type::subspace;
    }
    if (type_string == "open_facets") {
        return Type::open_facets;
    }
    if (type_string == "projection_coordinates") {
        return Type::projection_coordinates;
    }

    if (type_string == "hilbert_basis_rec_cone") {
        return Type::hilbert_basis_rec_cone;
    }

    if (type_string == "extreme_rays") {
        return Type::extreme_rays;
    }

    if (type_string == "maximal_subspace") {
        return Type::maximal_subspace;
    }

    if (type_string == "generated_lattice") {
        return Type::generated_lattice;
    }

    if (type_string == "scale") {
        return Type::scale;
    }

    if (type_string == "add_cone") {
        return Type::add_cone;
    }

    if (type_string == "add_subspace") {
        return Type::add_subspace;
    }

    if (type_string == "add_vertices") {
        return Type::add_vertices;
    }

    if (type_string == "add_inequalities") {
        return Type::add_inequalities;
    }

    if (type_string == "add_equations") {
        return Type::add_equations;
    }

    if (type_string == "add_inhom_inequalities") {
        return Type::add_inhom_inequalities;
    }

    if (type_string == "add_inhom_equations") {
        return Type::add_inhom_equations;
    }

    throw BadInputException("Unknown type \"" + type_string + "\"!");
    return Type::integral_closure;
}

inline long type_nr_columns_correction(InputType t) {
    if (t == Type::polytope || t == Type::rees_algebra)
        return -1;
    if (t == Type::congruences || t == Type::vertices || t == Type::polyhedron || t == Type::inhom_inequalities ||
        t == Type::inhom_equations || t == Type::hilbert_basis_rec_cone || t == Type::add_inhom_inequalities ||
        t == Type::add_vertices || t == Type::add_inhom_equations || t == Type::inhom_excluded_faces)
        return 1;
    if (t == Type::inhom_congruences)
        return 2;
    return 0;
}

/* returns true if the input of this type is a vector */
inline bool type_is_vector(InputType type) {
    if (type == Type::grading || type == Type::signs || type == Type::strict_signs || type == Type::dehomogenization ||
        type == Type::offset || type == Type::open_facets || type == Type::projection_coordinates || type == Type::scale ||
        type == Type::rational_offset) {
        return true;
    }
    return false;
}

inline NumParam::Param to_numpar(const std::string& type_string) {
    if (type_string == "expansion_degree")
        return NumParam::expansion_degree;
    if (type_string == "nr_coeff_quasipol")
        return NumParam::nr_coeff_quasipol;
    if (type_string == "face_codim_bound")
        return NumParam::face_codim_bound;
    if (type_string == "autom_codim_bound_vectors")
        return NumParam::autom_codim_bound_vectors;
    if (type_string == "block_size_hollow_tri")
        return NumParam::block_size_hollow_tri;
    if (type_string == "decimal_digits")
        return NumParam::decimal_digits;

    return NumParam::not_a_num_param;
}

inline std::string numpar_to_string(const NumParam::Param& numpar) {
    if (numpar == NumParam::expansion_degree)
        return "expansion_degree";
    if (numpar == NumParam::nr_coeff_quasipol)
        return "nr_coeff_quasipol";
    if (numpar == NumParam::face_codim_bound)
        return "face_codim_bound";
    if (numpar == NumParam::autom_codim_bound_vectors)
        return "autom_codim_bound_vectors";
    if (numpar == NumParam::autom_codim_bound_vectors)
        return "autom_codim_bound_vectors";
    if (numpar == NumParam::block_size_hollow_tri)
        return "block_size_hollow_tri";
    if (numpar == NumParam::decimal_digits)
        return "decimal_digits";
    if (numpar == NumParam::not_a_num_param)
        return "not_a_num_param";
    assert(false);
    return std::string();  // silence compiler warning
}

inline bool isNumParam(NumParam::Param& numpar, const std::string& type_string) {
    numpar = to_numpar(type_string);
    if (numpar == NumParam::not_a_num_param)
        return false;
    return true;
}

} /* end namespace libnormaliz */

#endif /* LIBNORMALIZ_H_ */
