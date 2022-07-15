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
#include <list>
#include <sys/stat.h>
#include <sys/types.h>
#include <cmath>
#include <fstream>
#include <cctype>

#include "libnormaliz/cone.h"
#include "libnormaliz/vector_operations.h"
#include "libnormaliz/project_and_lift.h"
// #include "libnormaliz/map_operations.h"
// #include "libnormaliz/convert.h"
#include "libnormaliz/full_cone.h"
#include "libnormaliz/descent.h"
#include "libnormaliz/my_omp.h"
#include "libnormaliz/output.h"
#include "libnormaliz/collection.h"
#include "libnormaliz/face_lattice.h"
#include "libnormaliz/input.h"

namespace libnormaliz {
using namespace std;

/*
template class FACETDATA<long>;
template class FACETDATA<long long>;
template class FACETDATA<mpz_class>;
#ifdef ENFNORMALIZ
template class FACETDATA<renf_elem_class>;
#endif
*/

template <typename Number>
void Cone<Number>::setRenf(const renf_class_shared renf) {
}

#ifdef ENFNORMALIZ
template <>
void Cone<renf_elem_class>::setRenf(const renf_class_shared renf) {
    Renf = &*renf;
    renf_degree = fmpq_poly_degree(renf->renf_t()->nf->pol);
    RenfSharedPtr = renf;
}
#endif

template <typename Integer>
Cone<Integer>::Cone(const string project) {
    OptionsHandler options;
    string polynomial;
    map<NumParam::Param, long> num_param_input;
    renf_class_shared number_field_ref;

    string name_in = project + ".in";
    const char* file_in = name_in.c_str();
    ifstream in;
    in.open(file_in, ifstream::in);
    if (!in.is_open()) {
        string message = "error: Failed to open file " + name_in;
        throw BadInputException(message);
    }

    bool number_field_in_input = false;
    string test;
    while (in.good()) {
        in >> test;
        if (test == "number_field") {
            number_field_in_input = true;
            break;
        }
    }
    in.close();
    if (number_field_in_input)
        throw BadInputException("number_field input only allowed for Cone<renf_elem_class>");

    in.open(file_in, ifstream::in);
    map<Type::InputType, Matrix<mpq_class> > input;
    input = readNormalizInput<mpq_class>(in, options, num_param_input, polynomial, number_field_ref);

    const renf_class_shared number_field = number_field_ref;
    process_multi_input(input);
    setPolynomial(polynomial);
    setRenf(number_field);
    setProjectName(project);
}

#ifdef ENFNORMALIZ
template <>
Cone<renf_elem_class>::Cone(const string project) {
    OptionsHandler options;
    string polynomial;
    map<NumParam::Param, long> num_param_input;
    renf_class_shared number_field_ref;

    string name_in = project + ".in";
    const char* file_in = name_in.c_str();
    ifstream in;
    in.open(file_in, ifstream::in);
    if (!in.is_open()) {
        string message = "error: Failed to open file " + name_in;
        throw BadInputException(message);
    }

    bool number_field_in_input = false;
    string test;
    while (in.good()) {
        in >> test;
        if (test == "number_field") {
            number_field_in_input = true;
            break;
        }
    }
    in.close();
    if (!number_field_in_input)
        throw BadInputException("Missing number field in input for Cone<renf_elem_class>");

    in.open(file_in, ifstream::in);
    map<Type::InputType, Matrix<renf_elem_class> > renf_input;
    renf_input = readNormalizInput<renf_elem_class>(in, options, num_param_input, polynomial, number_field_ref);
    const renf_class_shared number_field = number_field_ref.get();

    process_multi_input(renf_input);
    setPolynomial(polynomial);
    setRenf(number_field);
    setProjectName(project);
}
#endif

template <typename Integer>
void check_length_of_vectors_in_input(const InputMap<Integer>& multi_input_data, size_t dim) {
    for (auto& it : multi_input_data) {
        size_t prescribed_length = dim + type_nr_columns_correction(it.first);
        for (auto& v : it.second.get_elements()) {
            if (v.size() == 0)
                throw BadInputException("Vectors of length 0 not allowed in input");
            if (v.size() != prescribed_length)
                throw BadInputException("Inconsistent length of vectors in input");
        }
    }
}

template <typename Integer>
template <typename InputNumber>
void Cone<Integer>::check_add_input(const InputMap<InputNumber>& multi_add_data) {
    // if(!keep_convex_hull_data)
    //    throw BadInputException("Additional input only possible if cone is set Dynamic");

    auto M = multi_add_data.begin();

    if (multi_add_data.size() > 1)
        throw BadInputException("Additional input has too many matrices");

    auto T = M->first;
    if (!(T == Type::inequalities || T == Type::inhom_inequalities || T == Type::inhom_equations || T == Type::equations ||
          T == Type::cone || T == Type::vertices || T == Type::subspace))
        throw BadInputException("Additional input of illegal type");

    if (!inhomogeneous) {
        if (T == Type::inhom_inequalities || T == Type::vertices || T == Type::inhom_equations)
            throw BadInputException("Additional inhomogeneous input only with inhomogeneous original input");
    }
    check_consistency_of_dimension(multi_add_data);
    int inhom_corr = 0;
    if (inhomogeneous)
        inhom_corr = 1;
    check_length_of_vectors_in_input(multi_add_data, dim - inhom_corr);
}

template <typename Integer>
template <typename InputNumber>
void Cone<Integer>::check_consistency_of_dimension(const InputMap<InputNumber>& multi_input_data) {
    size_t inhom_corr = 0;
    if (inhom_input)
        inhom_corr = 1;
    auto it = multi_input_data.begin();
    size_t test_dim;
    for (; it != multi_input_data.end(); ++it) {
        /* if(type_is_number(it->first))
            continue;*/
        test_dim = it->second[0].size() - type_nr_columns_correction(it->first) + inhom_corr;
        if (test_dim != dim) {
            throw BadInputException("Inconsistent dimensions in input!");
        }
    }
}

InputMap<mpq_class> nmzfloat_input_to_mpqclass(
    const InputMap<nmz_float>& multi_input_data) {
    InputMap<mpq_class> multi_input_data_QQ;
    auto it = multi_input_data.begin();
    for (; it != multi_input_data.end(); ++it) {
        Matrix<mpq_class> Transfer;
        for (const auto& j : it->second.get_elements() ) {
            vector<mpq_class> vt;
            for (double k : j) {
                vt.push_back(mpq_class(k));
            }
            if(Transfer.nr_of_columns() != vt.size())
                Transfer.resize_columns(vt.size());
            Transfer.append(vt);
        }
        multi_input_data_QQ[it->first] = Transfer;
    }
    return multi_input_data_QQ;
}

bool renf_allowed(InputType input_type) {
    switch (input_type) {
        case Type::congruences:
        case Type::inhom_congruences:
        case Type::lattice:
        case Type::cone_and_lattice:
        case Type::rational_lattice:
        case Type::normalization:
        case Type::integral_closure:
        case Type::offset:
        case Type::rational_offset:
        case Type::rees_algebra:
        case Type::lattice_ideal:
        case Type::strict_signs:
        case Type::strict_inequalities:
        case Type::hilbert_basis_rec_cone:
        case Type::open_facets:
            return false;
            break;
        default:
            return true;
            break;
    }
}

bool denominator_allowed(InputType input_type) {
    switch (input_type) {
        case Type::congruences:
        case Type::inhom_congruences:
        case Type::grading:
        case Type::dehomogenization:
        case Type::lattice:
        case Type::normalization:
        case Type::cone_and_lattice:
        case Type::offset:
        case Type::rees_algebra:
        case Type::lattice_ideal:
        case Type::signs:
        case Type::strict_signs:
        case Type::scale:
        case Type::strict_inequalities:
        case Type::projection_coordinates:
        case Type::hilbert_basis_rec_cone:
        case Type::open_facets:
            return false;
            break;
        default:
            return true;
            break;
    }
}

template <typename Integer>
Matrix<Integer> find_input_matrix(const InputMap<Integer>& multi_input_data,
                                           const InputType type) {
    typename InputMap<Integer>::const_iterator it;
    it = multi_input_data.find(type);
    if (it != multi_input_data.end())
        return (it->second);

    Matrix<Integer> dummy;
    return (dummy);
}

// finds input matrix of given type. If not present, sets nr of cols
template <typename Integer>
Matrix<Integer> find_input_matrix(const InputMap<Integer>& multi_input_data,
                                           const InputType type, size_t nr_cols) {
    typename InputMap<Integer>::const_iterator it;
    it = multi_input_data.find(type);
    if (it != multi_input_data.end())
        return (it->second);

    Matrix<Integer> dummy(0, nr_cols);
    return (dummy);
}

template <typename Integer>
void scale_matrix(Matrix<Integer>& mat, const vector<Integer>& scale_axes, bool dual) {
    for (size_t j = 0; j < scale_axes.size(); ++j) {
        if (scale_axes[j] == 0)
            continue;
        for (size_t i = 0; i < mat.nr_of_rows(); ++i) {
            if (dual)
                mat[i][j] /= scale_axes[j];
            else
                mat[i][j] *= scale_axes[j];
        }
    }
}

template <typename Integer>
void scale_input(InputMap<Integer>& multi_input_data, const vector<Integer> scale_axes) {
    vector<Integer> ScaleHelp = scale_axes;
    ScaleHelp.resize(scale_axes.size() - 1);

    auto it = multi_input_data.begin();
    for (; it != multi_input_data.end(); ++it) {
        switch (it->first) {
            case Type::inhom_inequalities:
            case Type::inequalities:
            case Type::inhom_equations:
            case Type::equations:
            case Type::inhom_excluded_faces:
            case Type::excluded_faces:
            case Type::dehomogenization:
            case Type::grading:
                scale_matrix(it->second, scale_axes, true);  // true = dual space
                break;
            case Type::polytope:
                scale_matrix(it->second, ScaleHelp, false);
                break;
            case Type::cone:
            case Type::subspace:
            case Type::lattice:
            case Type::saturation:
            case Type::vertices:
            case Type::offset:
                scale_matrix(it->second, scale_axes, false);  // false = primal space
                break;
            default:
                break;
        }
    }
}

template <typename Integer>
void apply_cale(InputMap<Integer>& multi_input_data) {
    Matrix<Integer> scale_mat = find_input_matrix(multi_input_data, Type::scale);
    vector<Integer> scale_axes = scale_mat[0];
    scale_input(multi_input_data, scale_axes);
}

void process_rational_lattice(InputMap<mpq_class>& multi_input_data) {
    Matrix<mpq_class> RatLat = find_input_matrix(multi_input_data, Type::rational_lattice);
    Matrix<mpq_class> RatOff = find_input_matrix(multi_input_data, Type::rational_offset);

    if (RatLat.nr_of_rows() == 0 && RatOff.nr_of_rows() == 0)
        return;

    size_t dim;
    if (RatLat.nr_of_rows() > 0)
        dim = RatLat.nr_of_columns();
    else
        dim = RatOff.nr_of_columns();

    vector<mpq_class> Den(dim, 1);
    for (size_t i = 0; i < RatLat.nr_of_rows(); ++i) {
        for (size_t j = 0; j < dim; ++j) {
            Den[j] = libnormaliz::lcm(Den[j].get_num(), RatLat[i][j].get_den());
        }
    }
    if (RatOff.nr_of_rows() > 0) {
        for (size_t j = 0; j < dim; ++j) {
            Den[j] = libnormaliz::lcm(Den[j].get_num(), RatOff[0][j].get_den());
        }
    }

    multi_input_data.erase(Type::rational_lattice);
    multi_input_data.erase(Type::rational_offset);
    auto it = multi_input_data.begin();
    for (; it != multi_input_data.end(); ++it) {
        if (!renf_allowed(it->first))
            throw BadInputException("Some input type not allowed together with rational_lattice or offset");
    }

    if (RatLat.nr_of_rows() > 0)
        multi_input_data[Type::lattice] = RatLat.get_elements();
    if (RatOff.nr_of_rows() > 0)
        multi_input_data[Type::offset] = RatOff.get_elements();
    scale_input(multi_input_data, Den);

    if (contains(multi_input_data, Type::scale))
        throw BadInputException("Explicit input type scale only allowed for field coefficients");
    Matrix<mpq_class> DenMat(0,Den.size());
    DenMat.append(Den);
    multi_input_data[Type::scale] = DenMat;  // we use scale to ship Den
}

template <typename Integer>
InputMap<Integer> Cone<Integer>::mpqclass_input_to_integer(
    const InputMap<mpq_class>& multi_input_data_const) {
    /*cout << "---------------" << endl;
    for(auto& jt: multi_input_data_const){
            cout << jt.second;
            cout << "---------------" << endl;
    }*/

    InputMap<mpq_class> multi_input_data(
        multi_input_data_const);  // since we want to change it internally

    if (contains(multi_input_data, Type::rational_lattice) || contains(multi_input_data, Type::rational_offset))
        process_rational_lattice(multi_input_data);

    // The input type polytope is replaced by cone+grading in this routine.
    // Nevertheless it appears in the subsequent routines.
    // But any implications of its appearance must be handled here already.
    // However, polytope can still be used without conversion to cone via libnormaliz !!!!!

    // since polytope will be converted to cone, we must do some checks here
    if (contains(multi_input_data, Type::polytope)) {
        polytope_in_input = true;
    }
    if (contains(multi_input_data, Type::grading) && polytope_in_input) {
        throw BadInputException("No explicit grading allowed with polytope!");
    }
    if (contains(multi_input_data, Type::cone) && polytope_in_input) {
        throw BadInputException("Illegal combination of cone generator types!");
    }

    if (contains(multi_input_data, Type::polytope)) {
        general_no_grading_denom = true;
    }

    InputMap<Integer> multi_input_data_ZZ;

    // special treatment of polytope. We convert it o cone
    // and define the grading
    if (contains(multi_input_data, Type::polytope)) {
        size_t dim;
        if (multi_input_data[Type::polytope].nr_of_rows() > 0) {
            dim = multi_input_data[Type::polytope][0].size() + 1;
            Matrix<Integer> grading(0,dim);
            grading.append(vector<Integer>(dim));
            grading[0][dim - 1] = 1;
            multi_input_data_ZZ[Type::grading] = grading;
        }
        vector<vector<mpq_class> >  Help = multi_input_data[Type::polytope].get_elements();
        multi_input_data.erase(Type::polytope);
        for (size_t i = 0; i < Help.size(); ++i) {
            Help[i].resize(dim);
            Help[i][dim - 1] = 1;
        }
        multi_input_data[Type::cone]=Matrix<mpq_class>(Help);
    }

    // now we clear denominators
    auto it = multi_input_data.begin();
    for (; it != multi_input_data.end(); ++it) {
        multi_input_data_ZZ[it->first] =
                Matrix<Integer>(0,it->second.nr_of_columns());
        for (size_t i = 0; i < it->second.nr_of_rows(); ++i) {
            mpz_class common_denom = 1;
            for (auto& j : it->second[i]) {
                j.canonicalize();
                common_denom = libnormaliz::lcm(common_denom, j.get_den());
            }
            if (common_denom > 1 && !denominator_allowed(it->first))
                throw BadInputException("Proper fraction not allowed in certain input types");
            vector<Integer> transfer(it->second[i].size());
            for (size_t j = 0; j < it->second[i].size(); ++j) {
                it->second[i][j] *= common_denom;
                convert(transfer[j], it->second[i][j].get_num());
            }
            multi_input_data_ZZ[it->first].append(transfer);
        }
    }

    return multi_input_data_ZZ;
}

// adds the signs inequalities given by Signs to Inequalities
template <typename Integer>
Matrix<Integer> sign_inequalities(const Matrix<Integer>& Signs) {
    if (Signs.nr_of_rows() != 1) {
        throw BadInputException("ERROR: Bad signs matrix, has " + toString(Signs.nr_of_rows()) + " rows (should be 1)!");
    }
    size_t dim = Signs[0].size();
    Matrix<Integer> Inequ(0, dim);
    vector<Integer> ineq(dim, 0);
    for (size_t i = 0; i < dim; i++) {
        Integer sign = Signs[0][i];
        if (sign == 1 || sign == -1) {
            ineq[i] = sign;
            Inequ.append(ineq);
            ineq[i] = 0;
        }
        else if (sign != 0) {
            throw BadInputException("Bad signs matrix, has entry " + toString(sign) + " (should be -1, 1 or 0)!");
        }
    }
    return Inequ;
}

template <typename Integer>
Matrix<Integer> strict_sign_inequalities(const Matrix<Integer>& Signs) {
    if (Signs.nr_of_rows() != 1) {
        throw BadInputException("ERROR: Bad signs matrix, has " + toString(Signs.nr_of_rows()) + " rows (should be 1)!");
    }
    size_t dim = Signs[0].size();
    Matrix<Integer> Inequ(0, dim);
    vector<Integer> ineq(dim, 0);
    ineq[dim - 1] = -1;
    for (size_t i = 0; i < dim - 1; i++) {  // last component of strict_signs always 0
        Integer sign = Signs[0][i];
        if (sign == 1 || sign == -1) {
            ineq[i] = sign;
            Inequ.append(ineq);
            ineq[i] = 0;
        }
        else if (sign != 0) {
            throw BadInputException("Bad signs matrix, has entry " + toString(sign) + " (should be -1, 1 or 0)!");
        }
    }
    return Inequ;
}

template <typename Integer>
void insert_column(Matrix<Integer>& mat, size_t col, Integer entry) {
/*    mat.resize_columns(mat.nr_of_columns()+1);
    if (mat.nr_of_rows() == 0){
        return;
    }
    vector<Integer> help(mat.nr_of_columns() + 1);
    for (size_t i = 0; i < mat.nr_of_rows(); ++i) {
        for (size_t j = 0; j < col; ++j)
            help[j] = mat[i][j];
        help[col] = entry;
        for (size_t j = col; j < mat[i].size(); ++j)
            help[j + 1] = mat[i][j];
        mat[i] = help;
    }*/
    vector<Integer> new_column(mat.nr_of_rows(),entry);
    mat.insert_column(col, new_column);
}

template <typename Integer>
void insert_zero_column(Matrix<Integer>& mat, size_t col) {
    // Integer entry=0;
    insert_column<Integer>(mat, col, 0);
}

template <typename Integer>
template <typename InputNumber>
void Cone<Integer>::homogenize_input(InputMap<InputNumber>& multi_input_data) {
    auto it = multi_input_data.begin();
    for (; it != multi_input_data.end(); ++it) {
        switch (it->first) {
            case Type::dehomogenization:
            case Type::support_hyperplanes:
            case Type::extreme_rays:
                throw BadInputException(
                    "Types dehomogenization, extreme_rays, support_hyperplanes not allowed with inhomogeneous input!");
                break;
            case Type::inhom_inequalities:  // nothing to do
            case Type::inhom_equations:
            case Type::inhom_congruences:
            case Type::inhom_excluded_faces:
            case Type::polyhedron:
            case Type::vertices:
            case Type::open_facets:
            case Type::hilbert_basis_rec_cone:
            case Type::grading:  // already taken care of
                break;
            case Type::strict_inequalities:
                insert_column<InputNumber>(it->second, dim - 1, -1);
                break;
            case Type::offset:
            case Type::projection_coordinates:
                insert_column<InputNumber>(it->second, dim - 1, 1);
                break;
            default:  // is correct for signs and strict_signs !
                insert_zero_column<InputNumber>(it->second, dim - 1);
                break;
        }
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::modifyCone(const InputMap<Integer>& multi_add_input_const) {
    if (rational_lattice_in_input)
        throw BadInputException("Modification of cone not possible with rational_lattice in construction");

    compute(ConeProperty::SupportHyperplanes, ConeProperty::ExtremeRays);

    precomputed_extreme_rays = false;
    precomputed_support_hyperplanes = false;
    InputMap<Integer> multi_add_input(multi_add_input_const);
    check_add_input(multi_add_input);
    if (inhomogeneous)
        homogenize_input(multi_add_input);

    auto T = multi_add_input.begin()->first;
    if (T == InputType::inequalities || T == InputType::inhom_inequalities || T == InputType::equations ||
        T == InputType::inhom_equations)
        AddInequalities.append(Matrix<Integer>(multi_add_input.begin()->second));
    if (T == InputType::equations || T == InputType::inhom_equations) {
        Matrix<Integer> Help(multi_add_input.begin()->second);
        Integer MinusOne = -1;
        Help.scalar_multiplication(MinusOne);
        AddInequalities.append(Help);
    }
    if (T == InputType::vertices || T == InputType::cone || T == InputType::subspace)
        AddGenerators.append(Matrix<Integer>(multi_add_input.begin()->second));
    if (T == InputType::subspace) {
        Matrix<Integer> Help(multi_add_input.begin()->second);
        Integer MinusOne = -1;
        Help.scalar_multiplication(MinusOne);
        AddGenerators.append(Help);
    }

    if (AddInequalities.nr_of_rows() == 0 && AddGenerators.nr_of_rows() == 0)
        return;

    if (AddInequalities.nr_of_rows() > 0)
        addition_generators_allowed = false;
    if (AddGenerators.nr_of_rows() > 0)
        addition_constraints_allowed = false;

    if ((AddInequalities.nr_of_rows() > 0 && !addition_constraints_allowed) ||
        (AddGenerators.nr_of_rows() > 0 && !addition_generators_allowed))
        throw BadInputException("Illgeal modification of cone!");

    bool save_dehom = isComputed(ConeProperty::Dehomogenization);

    if (AddGenerators.nr_of_rows() > 0) {
        Generators = ExtremeRays;
        Generators.append(AddGenerators);

        bool dummy;
        SupportHyperplanes.resize(0, dim);
        Inequalities.resize(0, dim);
        if (!check_lattice_restrictions_on_generators(dummy))
            throw BadInputException("Additional generators violate equations of sublattice");
        if (inhomogeneous)
            checkDehomogenization();
        if (Grading.size() > 0) {  // disable grading if it has nonpositive value somewhere
            for (size_t i = 0; i < Generators.nr_of_rows(); ++i) {
                if (v_scalar_product(Grading, Generators[i]) <= 0) {
                    Grading.resize(0);
                    break;
                }
            }
        }
        delete_aux_cones();
        is_Computed = ConeProperties();
        setComputed(ConeProperty::Generators);
        if (Grading.size() > 0)
            setComputed(ConeProperty::Grading);
    }

    if (AddInequalities.nr_of_rows() > 0) {
        if (!AddInequalities.zero_product_with_transpose_of(BasisMaxSubspace))
            throw BadInputException("Additional inequalities do not vanish on maximal subspace");
        Inequalities = SupportHyperplanes;
        Inequalities.append(AddInequalities);
        is_Computed = ConeProperties();
        setComputed(ConeProperty::MaximalSubspace);  // cannot change since inequalities vanish on max subspace
        setComputed(ConeProperty::IsPointed);
    }

    setComputed(ConeProperty::Dehomogenization, save_dehom);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::modifyCone(const InputMap<mpq_class>& multi_add_input_const) {
    InputMap<Integer> multi_add_input_ZZ = mpqclass_input_to_integer(multi_add_input_const);
    modifyCone(multi_add_input_ZZ);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::modifyCone(const InputMap<nmz_float>& multi_add_input_const) {
    InputMap<mpq_class> multi_add_input_QQ = nmzfloat_input_to_mpqclass(multi_add_input_const);
    modifyCone(multi_add_input_QQ);
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::delete_aux_cones() {
    if (IntHullCone != NULL)
        delete IntHullCone;
    if (SymmCone != NULL)
        delete SymmCone;
    if (ProjCone != NULL)
        delete ProjCone;
}

template <typename Integer>
Cone<Integer>::~Cone() {
    delete_aux_cones();
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::process_multi_input(const InputMap<mpq_class>& multi_input_data_const) {
    initialize();
    InputMap<Integer> multi_input_data_ZZ = mpqclass_input_to_integer(multi_input_data_const);
    process_multi_input_inner(multi_input_data_ZZ);
}

template <typename Integer>
void Cone<Integer>::process_multi_input(const InputMap<nmz_float>& multi_input_data) {
    initialize();
    InputMap<mpq_class> multi_input_data_QQ = nmzfloat_input_to_mpqclass(multi_input_data);
    process_multi_input(multi_input_data_QQ);
}

template <typename Integer>
void check_types_precomputed(InputMap<Integer>& multi_input_data) {
    auto it = multi_input_data.begin();
    for (; it != multi_input_data.end(); ++it) {
        switch (it->first) {
            case Type::maximal_subspace:
            case Type::generated_lattice:
            case Type::dehomogenization:
            case Type::extreme_rays:
            case Type::support_hyperplanes:
            case Type::grading:
                break;
            default:
                throw BadInputException("Input type not allowed with precomputed data");
                break;
        }
    }
}

template <typename Integer>
void Cone<Integer>::process_multi_input(const InputMap<Integer>& multi_input_data_const) {
    initialize();
    InputMap<Integer> multi_input_data(multi_input_data_const);
    if (contains(multi_input_data, Type::scale)) {
        if (using_renf<Integer>()) {
            apply_cale(multi_input_data);
        }
        else
            throw BadInputException("Explicit nput type scale only allowed for field coefficients");
    }
    process_multi_input_inner(multi_input_data);
}

template <typename Integer>
void Cone<Integer>::process_multi_input_inner(InputMap<Integer>& multi_input_data) {
    StartTime();

    // find basic input type
    lattice_ideal_input = false;
    nr_latt_gen = 0, nr_cone_gen = 0;
    inhom_input = false;

    auto it = multi_input_data.begin();
    if (using_renf<Integer>()) {
        for (; it != multi_input_data.end(); ++it) {
            if (!renf_allowed(it->first))
                throw BadInputException("Some onput type not allowed for field coefficients");
        }
    }

    if (!using_renf<Integer>() && contains(multi_input_data, Type::scale)) {
        AxesScaling = multi_input_data[Type::scale][0];  // only possible with rational_lattice
        setComputed(ConeProperty::AxesScaling);
        rational_lattice_in_input = true;
    }

    // NEW: Empty matrices have syntactical influence
    it = multi_input_data.begin();
    for (; it != multi_input_data.end(); ++it) {
        switch (it->first) {
            case Type::inhom_inequalities:
            case Type::strict_inequalities:
                inequalities_in_input = true;
            case Type::inhom_excluded_faces:
            case Type::inhom_equations:
            case Type::inhom_congruences:
            case Type::strict_signs:
            case Type::open_facets:
                inhom_input = true;
                break;
            case Type::signs:
            case Type::equations:
            case Type::congruences:
            case Type::support_hyperplanes:
                break;
            case Type::inequalities:
                inequalities_in_input = true;
                break;
            case Type::lattice_ideal:
                lattice_ideal_input = true;
                break;
            case Type::polyhedron:
                inhom_input = true;
                nr_cone_gen++;
                break;
            case Type::integral_closure:
            case Type::rees_algebra:
            case Type::polytope:
            case Type::cone:
            case Type::subspace:
            case Type::extreme_rays:
                nr_cone_gen++;
                break;
            case Type::normalization:
            case Type::cone_and_lattice:
                nr_cone_gen++;
                nr_latt_gen++;
                break;
            case Type::lattice:
            case Type::saturation:
            case Type::rational_lattice:
                nr_latt_gen++;
                break;
            case Type::vertices:
            case Type::offset:
                inhom_input = true;
                break;
            default:
                break;
        }
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    bool gen_error = false;
    if (nr_cone_gen > 2)
        gen_error = true;

    precomputed_extreme_rays = contains(multi_input_data, Type::extreme_rays);
    precomputed_support_hyperplanes = contains(multi_input_data, Type::support_hyperplanes);
    if (precomputed_extreme_rays != precomputed_support_hyperplanes)
        throw BadInputException("Precomputwed extreme rays and support hyperplanes can only be used together");

    if (precomputed_extreme_rays)
        check_types_precomputed(multi_input_data);

    if (!precomputed_extreme_rays && nr_cone_gen == 2 &&
        (!contains(multi_input_data, Type::subspace) ||
         !((contains(multi_input_data, Type::cone) && !polytope_in_input) || contains(multi_input_data, Type::cone_and_lattice) ||
           contains(multi_input_data, Type::integral_closure) || contains(multi_input_data, Type::normalization))))
        gen_error = true;

    if (gen_error) {
        throw BadInputException("Illegal combination of cone generator types");
    }

    if (nr_latt_gen > 1) {
        throw BadInputException("Only one matrix of lattice generators allowed!");
    }

    if (lattice_ideal_input) {
        if (multi_input_data.size() > 2 || (multi_input_data.size() == 2 && !contains(multi_input_data, Type::grading))) {
            throw BadInputException("Only grading allowed with lattice_ideal!");
        }
    }
    if (contains(multi_input_data, Type::open_facets)) {
        size_t allowed = 0;
        auto it = multi_input_data.begin();
        for (; it != multi_input_data.end(); ++it) {
            switch (it->first) {
                case Type::open_facets:
                case Type::cone:
                case Type::grading:
                case Type::vertices:
                    allowed++;
                    break;
                default:
                    break;
            }
        }
        if (allowed != multi_input_data.size())
            throw BadInputException("Illegal combination of input types with open_facets!");
        if (contains(multi_input_data, Type::vertices)) {
            if (multi_input_data[Type::vertices].nr_of_rows() > 1)
                throw BadInputException("At most one vertex allowed with open_facets!");
        }
    }

    /* if (inhom_input) { // checked in homogenize_input
        if (contains(multi_input_data, Type::dehomogenization) || contains(multi_input_data, Type::support_hyperplanes) ||
            contains(multi_input_data, Type::extreme_rays)) {
            throw BadInputException("Some types not allowed in combination with inhomogeneous input!");
        }
    }*/

    if (!inhom_input) {
        if (contains(multi_input_data, Type::hilbert_basis_rec_cone))
            throw BadInputException("Type hilbert_basis_rec_cone only allowed with inhomogeneous input!");
    }

    if (inhom_input || contains(multi_input_data, Type::dehomogenization)) {
        if (contains(multi_input_data, Type::rees_algebra) || contains(multi_input_data, Type::polytope) || polytope_in_input) {
            throw BadInputException("Types polytope and rees_algebra not allowed with inhomogeneous input or dehomogenization!");
        }
        // if (contains(multi_input_data, Type::excluded_faces)) {
        //    throw BadInputException("Type excluded_faces not allowed with inhomogeneous input or dehomogenization!");
        // }
    }
    /*if(contains(multi_input_data,Type::grading) && contains(multi_input_data,Type::polytope)){ // now superfluous
           throw BadInputException("No explicit grading allowed with polytope!");
    }*/

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    // remove empty matrices
    it = multi_input_data.begin();
    for (; it != multi_input_data.end();) {
        if (it->second.nr_of_rows() == 0)
            multi_input_data.erase(it++);
        else
            ++it;
    }

    if (multi_input_data.size() == 0) {
        throw BadInputException("All empty matrices empty. Cannot find dimension!");
    }

    // determine dimension
    it = multi_input_data.begin();
    size_t inhom_corr = 0;  // correction in the inhom_input case
    if (inhom_input)
        inhom_corr = 1;
    if (it->second[0].size() == 0)
        throw BadInputException("Ambient space of dimension 0 not allowed");
    dim = it->second[0].size() - type_nr_columns_correction(it->first) + inhom_corr;

    // We now process input types that are independent of generators, constraints, lattice_ideal
    // check for excluded faces

    ExcludedFaces = find_input_matrix(multi_input_data, Type::excluded_faces);
    if (ExcludedFaces.nr_of_rows() == 0)
        ExcludedFaces = Matrix<Integer>(0, dim);  // we may need the correct number of columns
    Matrix<Integer> InhomExcludedFaces = find_input_matrix(multi_input_data, Type::inhom_excluded_faces);
    if (InhomExcludedFaces.nr_of_rows() != 0)
        ExcludedFaces.append(InhomExcludedFaces);

    // check for a grading
    Matrix<Integer> lf = find_input_matrix(multi_input_data, Type::grading);
    if (lf.nr_of_rows() > 1) {
        throw BadInputException("Bad grading, has " + toString(lf.nr_of_rows()) + " rows (should be 1)!");
    }
    if (lf.nr_of_rows() == 1) {
        if (inhom_input)
            lf[0].push_back(0);  // first we extend grading trivially to have the right dimension
        setGrading(lf[0]);       // will eventually be set in full_cone.cpp
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    // cout << "Dim " << dim <<endl;

    check_consistency_of_dimension(multi_input_data);

    check_length_of_vectors_in_input(multi_input_data, dim - inhom_corr);

    if (inhom_input)
        homogenize_input(multi_input_data);

    if (contains(multi_input_data, Type::projection_coordinates)) {
        projection_coord_indicator.resize(dim);
        for (size_t i = 0; i < dim; ++i)
            if (multi_input_data[Type::projection_coordinates][0][i] != 0)
                projection_coord_indicator[i] = true;
    }

    // check for dehomogenization
    lf = find_input_matrix(multi_input_data, Type::dehomogenization);
    if (lf.nr_of_rows() > 1) {
        throw BadInputException("Bad dehomogenization, has " + toString(lf.nr_of_rows()) + " rows (should be 1)!");
    }
    if (lf.nr_of_rows() == 1) {
        setDehomogenization(lf[0]);
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    // now we can unify implicit and explicit truncation
    // Note: implicit and explicit truncation have already been excluded
    if (inhom_input) {
        Dehomogenization.resize(dim, 0), Dehomogenization[dim - 1] = 1;
        setComputed(ConeProperty::Dehomogenization);
    }
    if (isComputed(ConeProperty::Dehomogenization))
        inhomogeneous = true;

    if (lattice_ideal_input) {
        prepare_input_lattice_ideal(multi_input_data);
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    Matrix<Integer> LatticeGenerators(0, dim);
    prepare_input_generators(multi_input_data, LatticeGenerators);

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    if (!precomputed_extreme_rays)
        prepare_input_constraints(multi_input_data);  // sets Equations,Congruences,Inequalities

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    // set default values if necessary
    if (inhom_input && LatticeGenerators.nr_of_rows() != 0 && !contains(multi_input_data, Type::offset)) {
        vector<Integer> offset(dim);
        offset[dim - 1] = 1;
        LatticeGenerators.append(offset);
    }
    if (inhom_input && Generators.nr_of_rows() != 0 && !contains(multi_input_data, Type::vertices) &&
        !contains(multi_input_data, Type::polyhedron)) {
        vector<Integer> vertex(dim);
        vertex[dim - 1] = 1;
        Generators.append(vertex);
    }

    if (Generators.nr_of_rows() > 0 && LatticeGenerators.nr_of_rows() > 0 &&
        !(contains(multi_input_data, Type::cone_and_lattice) || contains(multi_input_data, Type::normalization)))
        convert_lattice_generators_to_constraints(
            LatticeGenerators);  // necessary in order that we can perform the intersection
                                 // of the cone with the subspce generated by LatticeGenerators
    // cout << "Ineq " << Inequalities.nr_of_rows() << endl;

    if (precomputed_extreme_rays)
        LatticeGenerators = find_input_matrix(multi_input_data, Type::generated_lattice, dim);

    process_lattice_data(LatticeGenerators, Congruences, Equations);

    // At this point BasisChange has absorbed all input of inequalities coming from cone,
    // the sublatticd defined by lattice generators,
    // the sublattice defined by constraints
    //
    // Inequalities can be restricted to this sublattice. They may later
    // restrict the sublattice further.
    //
    // BUT: cone generators are not necessarily contained in it.
    // If they violate equations, we convert the equations to inequalities
    // If they only violate congruences, we can pass to multiples.

    remove_superfluous_equations();  // equations satisfied by all cone generators if any

    // if(Generators.nr_of_rows() >0 && Equations.nr_of_rows() > 0)
    //   convert_equations_to_inequalties(); // allows us to intersect the cone(Generators) with the subspace(Equations)

    remove_superfluous_inequalities();  // namely those satisfied by the cone generators if there are any

    remove_superfluous_congruences();  // ditto

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    bool cone_sat_eq = true;
    bool cone_sat_cong = true;
    bool cone_sat_ineq = true;
    if (Generators.nr_of_rows() > 0) {
        cone_sat_eq = (Equations.nr_of_rows() == 0);
        cone_sat_cong = (Congruences.nr_of_rows() == 0);
        cone_sat_ineq = (Inequalities.nr_of_rows() == 0);
    }

    // cout << "Cond " << cone_sat_eq << cone_sat_ineq << cone_sat_cong << endl;

    if (precomputed_extreme_rays && !(cone_sat_eq && cone_sat_ineq && cone_sat_cong))
        throw BadInputException("Precomputed extreme rays violate constraints");

    if (precomputed_support_hyperplanes && !cone_sat_ineq)
        throw BadInputException("Precomputed support hyperplanes do not support the cone");

    // Note: in the inhomogeneous case the original monoid generators as set hetre contain
    // the verices. So the name is mathematically incorrect, but the different types will be separated
    // in Full_Cone for the computation of generators OVER original monoid.
    if (cone_sat_eq && cone_sat_cong && cone_sat_ineq && Generators.nr_of_rows() != 0)
        set_original_monoid_generators(Generators);

    if (!cone_sat_cong) {
        for (size_t i = 0; i < Generators.nr_of_rows(); ++i)
            v_scalar_multiplication(Generators[i], BasisChange.getAnnihilator());  // the saled generators satisfy the congruences
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    if (Inequalities.nr_of_rows() != 0 && Generators.nr_of_rows() == 0) {
        dual_original_generators = true;
    }

    if (contains(multi_input_data, Type::open_facets)) {
        // read manual for the computation that follows
        if (!isComputed(ConeProperty::OriginalMonoidGenerators))  // practically impossible, but better to check
            throw BadInputException("Error in connection with open_facets");
        if (Generators.nr_of_rows() != BasisChange.getRank())
            throw BadInputException("Cone for open_facets not simplicial!");
        Matrix<Integer> TransformedGen = BasisChange.to_sublattice(Generators);
        vector<key_t> key(TransformedGen.nr_of_rows());
        for (size_t j = 0; j < TransformedGen.nr_of_rows(); ++j)
            key[j] = j;
        Matrix<Integer> TransformedSupps;
        Integer dummy;
        TransformedGen.simplex_data(key, TransformedSupps, dummy, false);
        Matrix<Integer> NewSupps = BasisChange.from_sublattice_dual(TransformedSupps);
        NewSupps.remove_row(NewSupps.nr_of_rows() - 1);  // must remove the inequality for the homogenizing variable
        for (size_t j = 0; j < NewSupps.nr_of_rows(); ++j) {
            if (!(multi_input_data[Type::open_facets][0][j] == 0 || multi_input_data[Type::open_facets][0][j] == 1))
                throw BadInputException("Illegal entry in open_facets");
            NewSupps[j][dim - 1] -= multi_input_data[Type::open_facets][0][j];
        }
        NewSupps.append(BasisChange.getEquationsMatrix());
        Matrix<Integer> Ker = NewSupps.kernel(false);  // gives the new verterx
        // Ker.pretty_print(cout);
        assert(Ker.nr_of_rows() == 1);
        Generators[Generators.nr_of_rows() - 1] = Ker[0];
        InputGenerators[Generators.nr_of_rows() - 1] = Ker[0];
    }

    BasisChangePointed = BasisChange;
    setWeights();  // make matrix of weights for sorting

    // Next we must convert generators to constraints if we have mixed input
    //
    bool must_convert = !cone_sat_eq;
    if (Inequalities.nr_of_rows() != 0 && Generators.nr_of_rows() != 0)
        must_convert = true;
    if (must_convert && verbose)
        verboseOutput() << "Converting generators to inequalities" << endl;

    bool inequalities_vanish = Inequalities.zero_product_with_transpose_of(BasisMaxSubspace);

    if (must_convert && cone_sat_eq && inequalities_vanish) {  // in this case we can use the already
        // computed coordinate transformation and modifyCone
        Cone<Integer> RestoreIfNecessary(*this);
        keep_convex_hull_data = true;
        setComputed(ConeProperty::Generators);
        Matrix<Integer> SaveInequalities = Inequalities; // the input inequalities are added later
        Inequalities = Matrix<Integer>(0,dim);
        compute(ConeProperty::SupportHyperplanes);
        Inequalities = SaveInequalities;
        if (Inequalities.zero_product_with_transpose_of(BasisMaxSubspace)) {
            if (verbose)
                verboseOutput() << "Conversion finished" << endl;

            if (inhomogeneous) {
                Inequalities.append(Dehomogenization);
                modifyCone(Type::inhom_inequalities, Inequalities);
            }
            else
                modifyCone(Type::inequalities, Inequalities);
            Generators = Matrix<Integer>(0, dim);  // are contained in the ConvexHullData
            conversion_done = true;
            must_convert = false;
            keep_convex_hull_data = false;
        }
        else
            *this = RestoreIfNecessary;  // we prefer the method following now.
                                         // One could think of a less drastic method
    }

    if (must_convert) {  // in the remaining case we must use a copy
        if (verbose)
            verboseOutput() << "Converting generators using a copy" << endl;
        Cone<Integer> Copy(Type::cone, Generators);
        Copy.compute(ConeProperty::SupportHyperplanes);
        Inequalities.append(Copy.getSupportHyperplanesMatrix());
        if (Copy.getSublattice().getEquationsMatrix().nr_of_rows() > 0) {
            Inequalities.append(Copy.getSublattice().getEquationsMatrix());  // must convert equations to inequalities
            vector<Integer> neg_sum_subspace(dim, 0);
            for (size_t i = 0; i < Copy.getSublattice().getEquationsMatrix().nr_of_rows(); ++i)
                neg_sum_subspace = v_add(neg_sum_subspace, Copy.getSublattice().getEquationsMatrix()[i]);
            v_scalar_multiplication<Integer>(neg_sum_subspace, -1);
            Inequalities.append(neg_sum_subspace);
        }
        if (verbose)
            verboseOutput() << "Conversion finished" << endl;
        Generators = Matrix<Integer>(0, dim);
        BasisMaxSubspace = Matrix<Integer>(0, dim);
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    assert(Inequalities.nr_of_rows() == 0 || Generators.nr_of_rows() == 0);

    if (Generators.nr_of_rows() != 0) {
        setComputed(ConeProperty::Generators);
        setComputed(ConeProperty::Sublattice);
    }

    if (!conversion_done && inhomogeneous) {  // the inhomogeneous case is more tricky since we must decide
                                              // whether to append the dehomogenization
        bool append_dehomogenization = false;
        if (Inequalities.nr_of_rows() != 0)
            append_dehomogenization = true;
        else {
            if (inequalities_in_input && Generators.nr_of_rows() == 0)
                append_dehomogenization = true;  // this takes care of empty inequalities to break default
                                                 // positive orthant.
        }
        if (append_dehomogenization)
            Inequalities.append(Dehomogenization);
    }

    checkGrading(false);  // do not compute grading denom
    checkDehomogenization();

    if (positive_orthant && Grading.size() > 0) {
        size_t hom_dim = dim;
        if (inhom_input)
            hom_dim--;
        bool grading_is_positive = true;
        for (size_t i = 0; i < hom_dim; ++i) {
            if (Grading[i] <= 0) {
                grading_is_positive = false;
                break;
            }
        }
        if (grading_is_positive) {
            setComputed(ConeProperty::Grading);
            GradingDenom = 1;
            setComputed(ConeProperty::GradingDenom);
        }
    }

    if (!precomputed_extreme_rays && Inequalities.nr_of_rows() > 0) {
        pass_to_pointed_quotient();
        check_vanishing_of_grading_and_dehom();
    }

    if (isComputed(ConeProperty::Grading)) {  // cone known to be pointed
        setComputed(ConeProperty::MaximalSubspace);
        pointed = true;
        setComputed(ConeProperty::IsPointed);
    }

    // At the end of the construction of the cone we have either
    // (1) the cone defined by generators in Generators or
    // (2) by inequalities stored in Inequalities.
    // Exception: precomputed support hyperplanes (see below).
    //
    // The lattice defining information in the input has been
    // processed and sits in BasisChange.
    //
    // Note that the processing of the inequalities can
    // later on change the lattice.
    //

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    // read precomputed data
    if (precomputed_extreme_rays) {
        Generators = Matrix<Integer>(0, dim);
        Generators = find_input_matrix(multi_input_data, Type::extreme_rays,dim);
        setComputed(ConeProperty::Generators);
        addition_generators_allowed = true;
        // ExtremeRays.sort_by_weights(WeightsGrad, GradAbs); -- disabled to allow KeepOrder
        set_extreme_rays(vector<bool>(Generators.nr_of_rows(), true));
        BasisMaxSubspace = find_input_matrix(multi_input_data, Type::maximal_subspace, dim);
        if (BasisMaxSubspace.nr_of_rows() > 0) {
            Matrix<Integer> Help = BasisMaxSubspace;  // for protection
            Matrix<Integer> Dummy(0, dim);
            BasisChangePointed.compose_with_passage_to_quotient(Help, Dummy);  // now modulo Help, was not yet pointed
        }
        pointed = (BasisMaxSubspace.nr_of_rows() == 0);
        setComputed(ConeProperty::IsPointed);
        setComputed(ConeProperty::MaximalSubspace);
        setComputed(ConeProperty::Sublattice);
        SupportHyperplanes = find_input_matrix(multi_input_data, Type::support_hyperplanes, dim);
        SupportHyperplanes.sort_lex();
        setComputed(ConeProperty::SupportHyperplanes);
        Inequalities = SupportHyperplanes;
        addition_constraints_allowed = true;

        size_t test_rank = BasisChangePointed.getRank();
        if (test_rank != BasisChangePointed.to_sublattice(Generators).rank() ||
            test_rank != BasisChangePointed.to_sublattice_dual(SupportHyperplanes).rank())
            throw BadInputException("Precomputed data do not define pointed cone modulo maximal subspace");
        create_convex_hull_data();
        keep_convex_hull_data = true;
        checkGrading(true);
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    // for integer hull cones the maximal subsapce is known since it is the same
    // as for the original cone
    if (is_inthull_cone) {
        if (contains(multi_input_data, Type::subspace)) {
            BasisMaxSubspace = find_input_matrix(multi_input_data, Type::subspace);
            setComputed(ConeProperty::MaximalSubspace);
            if (BasisMaxSubspace.nr_of_rows() > 0) {
                Matrix<Integer> Help = BasisMaxSubspace;  // for protection
                Matrix<Integer> Dummy(0, dim);
                BasisChangePointed.compose_with_passage_to_quotient(Help, Dummy);  // now modulo Help, was not yet pointed
            }
        }
    }

    HilbertBasisRecCone = find_input_matrix(multi_input_data, Type::hilbert_basis_rec_cone);

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    setComputed(ConeProperty::IsInhomogeneous);
    setComputed(ConeProperty::EmbeddingDim);

    if (inhomogeneous)
        Norm = Dehomogenization;
    else
        Norm = Grading;

    // standardization for renf>_elem_class

    if (using_renf<Integer>()) {
        if (isComputed(ConeProperty::Generators))
            Generators.standardize_rows(Norm);
        if (Dehomogenization.size() > 0 && Grading.size() > 0)
            throw BadInputException("Grading not allowed for inhomogeneous computations over number fields");
    }

    AddInequalities.resize(0, dim);
    AddGenerators.resize(0, dim);

    assert(Generators.nr_of_rows() == 0 || Inequalities.nr_of_rows() == 0 || precomputed_extreme_rays);

    if(Generators.nr_of_rows() == 0 && Inequalities.nr_of_rows() == 0 && !precomputed_extreme_rays)
        Inequalities.append(vector<Integer>(dim,0));


    /* cout << "Supps " << endl;
    SupportHyperplanes.pretty_print(cout);
    cout << "Excl " << endl;
    ExcludedFaces.pretty_print(cout);
    cout << "===========" << endl;
    cout << is_Computed << endl;
    cout << "===========" << endl; */

    /* if(ExcludedFaces.nr_of_rows()>0){ // Nothing to check anymore
        check_excluded_faces();
    } */

    /*
    cout <<"-----------------------" << endl;
    cout << "Gen " << endl;
    Generators.pretty_print(cout);
    cout << "Supp " << endl;
    SupportHyperplanes.pretty_print(cout);
    cout << "A" << endl;
    BasisChange.get_A().pretty_print(cout);
    cout << "B" << endl;
    BasisChange.get_B().pretty_print(cout);
    cout <<"-----------------------" << endl;
    */
}

//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::remove_superfluous_inequalities() {
    if (Inequalities.nr_of_rows() > 0 && Generators.nr_of_rows() > 0) {
        vector<key_t> essential;
        for (size_t i = 0; i < Inequalities.nr_of_rows(); ++i) {
            for (size_t j = 0; j < Generators.nr_of_rows(); ++j) {
                if (v_scalar_product(Inequalities[i], Generators[j]) < 0) {
                    essential.push_back(i);
                    break;
                }
            }
        }
        if (essential.size() < Inequalities.nr_of_rows())
            Inequalities = Inequalities.submatrix(essential);
    }
}
//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::remove_superfluous_equations() {
    if (Equations.nr_of_rows() > 0 && Generators.nr_of_rows() > 0) {
        vector<key_t> essential;
        for (size_t i = 0; i < Equations.nr_of_rows(); ++i) {
            for (size_t j = 0; j < Generators.nr_of_rows(); ++j) {
                if (v_scalar_product(Equations[i], Generators[j]) != 0) {
                    essential.push_back(i);
                    break;
                }
            }
        }
        if (essential.size() < Equations.nr_of_rows())
            Equations = Equations.submatrix(essential);
    }
}

//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::remove_superfluous_congruences() {
    if (Congruences.nr_of_rows() > 0 && Generators.nr_of_rows() > 0) {
        vector<key_t> essential;
        size_t cc = Congruences[0].size();
        for (size_t k = 0; k < Congruences.nr_of_rows(); ++k) {
            for (size_t i = 0; i < Generators.nr_of_rows(); ++i) {
                if (v_scalar_product_vectors_unequal_lungth(Generators[i], Congruences[k]) % Congruences[k][cc - 1] !=
                    0) {  // congruence not satisfied
                    essential.push_back(k);
                    break;
                }
            }
        }
        if (essential.size() < Congruences.nr_of_rows())
            Congruences = Congruences.submatrix(essential);
    }
}

#ifdef ENFNORMALIZ
template <>
void Cone<renf_elem_class>::remove_superfluous_congruences() {
    return;
}
#endif
//---------------------------------------------------------------------------

// ONLY USED FOR ADDITIONAL GENERATORS
// We check whether the given generators satisfy the lattice restrictions by
// congruences and equations.
// If both are satisfied, we return true.
// If the equations are not satisfied, we return false.
// if only the congruences are violated, the generators are replaced by multiples
// that satisfy the congruences, and return true.
// We need cone_sat_cong to control original generators
template <typename Integer>
bool Cone<Integer>::check_lattice_restrictions_on_generators(bool& cone_sat_cong) {
    if (BasisChange.IsIdentity())
        return true;

    for (size_t i = 0; i < Generators.nr_of_rows(); ++i) {
        for (size_t j = 0; j < BasisChange.getEquationsMatrix().nr_of_rows(); ++j) {
            if (v_scalar_product(Generators[i], BasisChange.getEquationsMatrix()[j]) != 0) {
                return false;
            }
        }
    }

    cone_sat_cong = true;

    if (using_renf<Integer>())  // no congruences to check
        return true;

    if (Congruences.nr_of_rows() == 0)
        return true;

    for (size_t i = 0; i < Generators.nr_of_rows(); ++i) {
        cone_sat_cong = BasisChange.getCongruencesMatrix().check_congruences(Generators[i]);
        if (!cone_sat_cong)
            break;
    }

    if (cone_sat_cong)
        return true;

    // multiply generators by anniullator mod sublattice
    for (size_t i = 0; i < Generators.nr_of_rows(); ++i)
        v_scalar_multiplication(Generators[i], BasisChange.getAnnihilator());

    return true;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::prepare_input_constraints(const InputMap<Integer>& multi_input_data) {
    Matrix<Integer> Signs(0, dim), StrictSigns(0, dim);

    SupportHyperplanes = Matrix<Integer>(0, dim);  // only initialize here

    Inequalities = Matrix<Integer>(0, dim);  // these three will be set here
    Equations = Matrix<Integer>(0, dim);
    Congruences = Matrix<Integer>(0, dim + 1);

    if (precomputed_extreme_rays)
        return;

    for (const auto& it : multi_input_data) {
        switch (it.first) {
            case Type::strict_inequalities:
            case Type::inequalities:
            case Type::inhom_inequalities:
            case Type::excluded_faces:
            case Type::inhom_excluded_faces:
                Inequalities.append(it.second);
                break;
            case Type::equations:
            case Type::inhom_equations:
                Equations.append(it.second);
                break;
            case Type::congruences:
            case Type::inhom_congruences:
                Congruences.append(it.second);
                break;
            case Type::signs:
                Signs = sign_inequalities(it.second);
                break;
            case Type::strict_signs:
                StrictSigns = strict_sign_inequalities(it.second);
                break;
            default:
                break;
        }
    }
    if (!BC_set)
        compose_basis_change(Sublattice_Representation<Integer>(dim));
    Matrix<Integer> Help(Signs);  // signs first !!
    Help.append(StrictSigns);     // then strict signs
    Help.append(Inequalities);
    Inequalities = Help;

    insert_default_inequalities(Inequalities);

    vector<Integer> test(dim);
    test[dim - 1] = 1;

    if (inhomogeneous && Dehomogenization != test)
        return;

    size_t hom_dim = dim;
    if (inhomogeneous)
        hom_dim--;
    positive_orthant = true;
    for (size_t i = 0; i < hom_dim; ++i) {
        bool found = false;
        vector<Integer> gt0(dim);
        gt0[i] = 1;
        for (size_t j = 0; j < Inequalities.nr_of_rows(); ++j) {
            if (Inequalities[j] == gt0) {
                found = true;
                break;
            }
        }
        if (!found) {
            positive_orthant = false;
            break;
        }
    }

    if (!positive_orthant)
        return;

    Matrix<Integer> HelpEquations(0, dim);

    for (size_t i = 0; i < Equations.nr_of_rows(); ++i) {
        if (inhomogeneous && Equations[i][dim - 1] < 0)
            continue;
        vector<key_t> positive_coord;
        for (size_t j = 0; j < hom_dim; ++j) {
            if (Equations[i][j] < 0) {
                positive_coord.clear();
                break;
            }
            if (Equations[i][j] > 0)
                positive_coord.push_back(j);
        }
        for (unsigned int& k : positive_coord) {
            vector<Integer> CoordZero(dim);
            CoordZero[k] = 1;
            HelpEquations.append(CoordZero);
        }
    }
    Equations.append(HelpEquations);
    /* cout << "Help " << HelpEquations.nr_of_rows() <<  endl;
    HelpEquations.pretty_print(cout);
    cout << "====================================" << endl;
    Equations.pretty_print(cout);
    cout << "====================================" << endl;*/
}

//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::prepare_input_generators(InputMap<Integer>& multi_input_data,
                                             Matrix<Integer>& LatticeGenerators) {
    if (contains(multi_input_data, Type::vertices)) {
        for (size_t i = 0; i < multi_input_data[Type::vertices].nr_of_rows(); ++i)
            if (multi_input_data[Type::vertices][i][dim - 1] <= 0) {
                throw BadInputException("Vertex has non-positive denominator!");
            }
    }

    if (contains(multi_input_data, Type::polyhedron)) {
        for (size_t i = 0; i < multi_input_data[Type::polyhedron].nr_of_rows(); ++i)
            if (multi_input_data[Type::polyhedron][i][dim - 1] < 0) {
                throw BadInputException("Polyhedron vertex has negative denominator!");
            }
    }

    typename InputMap<Integer>::const_iterator it = multi_input_data.begin();
    // find specific generator type -- there is only one, as checked already

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    normalization = false;

    // check for subspace
    BasisMaxSubspace = find_input_matrix(multi_input_data, Type::subspace);
    if (BasisMaxSubspace.nr_of_rows() == 0)
        BasisMaxSubspace = Matrix<Integer>(0, dim);

    vector<Integer> neg_sum_subspace(dim, 0);
    for (size_t i = 0; i < BasisMaxSubspace.nr_of_rows(); ++i)
        neg_sum_subspace = v_add(neg_sum_subspace, BasisMaxSubspace[i]);
    v_scalar_multiplication<Integer>(neg_sum_subspace, -1);

    Generators = Matrix<Integer>(0, dim);
    for (; it != multi_input_data.end(); ++it) {
        INTERRUPT_COMPUTATION_BY_EXCEPTION

        switch (it->first) {
            case Type::normalization:
            case Type::cone_and_lattice:
                normalization = true;
                LatticeGenerators.append(it->second);
                if (BasisMaxSubspace.nr_of_rows() > 0)
                    LatticeGenerators.append(BasisMaxSubspace);
                Generators.append(it->second);
                break;
            case Type::vertices:
            case Type::polyhedron:
            case Type::cone:
            case Type::integral_closure:
                Generators.append(it->second);
                break;
            case Type::subspace:
                Generators.append(it->second);
                Generators.append(neg_sum_subspace);
                break;
            case Type::polytope:
                Generators.append(prepare_input_type_2(it->second));
                break;
            case Type::rees_algebra:
                Generators.append(prepare_input_type_3(it->second));
                break;
            case Type::lattice:
                LatticeGenerators.append(it->second);
                break;
            case Type::saturation:
                LatticeGenerators.append(it->second);
                LatticeGenerators.saturate();
                break;
            case Type::offset:
                if (it->second.nr_of_rows() > 1) {
                    throw BadInputException("Only one offset allowed!");
                }
                LatticeGenerators.append(it->second);
                break;
            default:
                break;
        }
    }
}

//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::convert_lattice_generators_to_constraints(Matrix<Integer>& LatticeGenerators) {
    Sublattice_Representation<Integer> GenSublattice(LatticeGenerators, false);
    Congruences.append(GenSublattice.getCongruencesMatrix());
    Equations.append(GenSublattice.getEquationsMatrix());
    LatticeGenerators.resize(0);
}

//---------------------------------------------------------------------------

/*
template <typename Integer>
void Cone<Integer>::convert_equations_to_inequalties(){

    assert(Equations.nr_of_rows()>0);

   vector<Integer> neg_sum_subspace(dim, 0);
    for (size_t i = 0; i < Equations.nr_of_rows(); ++i)
        neg_sum_subspace = v_add(neg_sum_subspace, Equations[i]);
    v_scalar_multiplication<Integer>(neg_sum_subspace, -1);

    Inequalities.append(Equations);
    Inequalities.append(neg_sum_subspace);
    Equations.resize(0);

}
*/

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::process_lattice_data(const Matrix<Integer>& LatticeGenerators,
                                         Matrix<Integer>& Congruences,
                                         Matrix<Integer>& Equations) {
    if (!BC_set)
        compose_basis_change(Sublattice_Representation<Integer>(dim));

    bool no_constraints = (Congruences.nr_of_rows() == 0) && (Equations.nr_of_rows() == 0);
    bool only_cone_gen = (Generators.nr_of_rows() != 0) && no_constraints && (LatticeGenerators.nr_of_rows() == 0);

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    if (only_cone_gen) {
        Sublattice_Representation<Integer> Basis_Change(Generators, true);
        compose_basis_change(Basis_Change);
        return;
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    if (normalization && no_constraints && !inhomogeneous) {
        Sublattice_Representation<Integer> Basis_Change(Generators, false);
        compose_basis_change(Basis_Change);
        return;
    }

    if (Generators.nr_of_rows() != 0) {
        Equations.append(Generators.kernel(!using_renf<Integer>()));
    }

    if (LatticeGenerators.nr_of_rows() != 0) {
        Sublattice_Representation<Integer> GenSublattice(LatticeGenerators, false);
        if ((Equations.nr_of_rows() == 0) && (Congruences.nr_of_rows() == 0)) {
            compose_basis_change(GenSublattice);
            return;
        }
        Congruences.append(GenSublattice.getCongruencesMatrix());
        Equations.append(GenSublattice.getEquationsMatrix());
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    if (Congruences.nr_of_rows() > 0) {
        bool zero_modulus;
        Matrix<Integer> Ker_Basis = Congruences.solve_congruences(zero_modulus);
        if (zero_modulus) {
            throw BadInputException("Modulus 0 in congruence!");
        }
        Sublattice_Representation<Integer> Basis_Change(Ker_Basis, false);
        compose_basis_change(Basis_Change);
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    if (Equations.nr_of_rows() > 0) {
        Matrix<Integer> Ker_Basis = BasisChange.to_sublattice_dual(Equations).kernel(!using_renf<Integer>());
        Sublattice_Representation<Integer> Basis_Change(Ker_Basis, true);
        compose_basis_change(Basis_Change);
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::insert_default_inequalities(Matrix<Integer>& Inequalities) {
    if (Generators.nr_of_rows() == 0 && Inequalities.nr_of_rows() == 0 && !inequalities_in_input) {
        if (verbose) {
            verboseOutput() << "No inequalities specified in constraint mode, using non-negative orthant." << endl;
        }
        if (inhomogeneous) {
            vector<Integer> test(dim);
            test[dim - 1] = 1;
            size_t matsize = dim;
            if (test == Dehomogenization)  // in this case "last coordinate >= 0" will come in through the dehomogenization
                matsize = dim - 1;         // we don't check for any other coincidence
            Inequalities = Matrix<Integer>(matsize, dim);
            for (size_t j = 0; j < matsize; ++j)
                Inequalities[j][j] = 1;
        }
        else
            Inequalities = Matrix<Integer>(dim);
    }
}

//---------------------------------------------------------------------------

/* polytope input */
template <typename Integer>
Matrix<Integer> Cone<Integer>::prepare_input_type_2(const Matrix<Integer>& Input) {
    size_t j;
    size_t nr = Input.nr_of_rows();
    // append a column of 1
    Matrix<Integer> Generators(nr, dim);
    for (size_t i = 0; i < nr; i++) {
        for (j = 0; j < dim - 1; j++)
            Generators[i][j] = Input[i][j];
        Generators[i][dim - 1] = 1;
    }
    // use the added last component as grading
    Grading = vector<Integer>(dim, 0);
    Grading[dim - 1] = 1;
    setComputed(ConeProperty::Grading);
    GradingDenom = 1;
    setComputed(ConeProperty::GradingDenom);
    return Generators;
}

//---------------------------------------------------------------------------

/* rees input */
template <typename Integer>
Matrix<Integer> Cone<Integer>::prepare_input_type_3(const Matrix<Integer>& InputV) {
    Matrix<Integer> Input(InputV);
    int i, j, k, nr_rows = Input.nr_of_rows(), nr_columns = Input.nr_of_columns();
    // create cone generator matrix
    Matrix<Integer> Full_Cone_Generators(nr_rows + nr_columns, nr_columns + 1, 0);
    for (i = 0; i < nr_columns; i++) {
        Full_Cone_Generators[i][i] = 1;
    }
    for (i = 0; i < nr_rows; i++) {
        Full_Cone_Generators[i + nr_columns][nr_columns] = 1;
        for (j = 0; j < nr_columns; j++) {
            Full_Cone_Generators[i + nr_columns][j] = Input[i][j];
        }
    }
    // primarity test
    vector<bool> Prim_Test(nr_columns, false);
    for (i = 0; i < nr_rows; i++) {
        k = 0;
        size_t v = 0;
        for (j = 0; j < nr_columns; j++)
            if (Input[i][j] != 0) {
                k++;
                v = j;
            }
        if (k == 1)
            Prim_Test[v] = true;
    }
    rees_primary = true;
    for (i = 0; i < nr_columns; i++)
        if (!Prim_Test[i])
            rees_primary = false;

    setComputed(ConeProperty::IsReesPrimary);
    return Full_Cone_Generators;
}

#ifdef ENFNORMALIZ
template <>
Matrix<renf_elem_class> Cone<renf_elem_class>::prepare_input_type_3(const Matrix<renf_elem_class>& InputV) {
    assert(false);
    return {};
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::prepare_input_lattice_ideal(InputMap<Integer>& multi_input_data) {
    Matrix<Integer> Binomials(find_input_matrix(multi_input_data, Type::lattice_ideal));

    if (Grading.size() > 0) {
        // check if binomials are homogeneous
        vector<Integer> degrees = Binomials.MxV(Grading);
        for (size_t i = 0; i < degrees.size(); ++i) {
            if (degrees[i] != 0) {
                throw BadInputException("Grading gives non-zero value " + toString(degrees[i]) + " for binomial " +
                                        toString(i + 1) + "!");
            }
            if (Grading[i] < 0) {
                throw BadInputException("Grading gives negative value " + toString(Grading[i]) + " for generator " +
                                        toString(i + 1) + "!");
            }
        }
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    Matrix<Integer> Gens = Binomials.kernel().transpose();
    Full_Cone<Integer> FC(Gens);
    FC.verbose = verbose;
    if (verbose)
        verboseOutput() << "Computing a positive embedding..." << endl;

    FC.dualize_cone();
    Matrix<Integer> Supp_Hyp = FC.getSupportHyperplanes().sort_lex();
    Matrix<Integer> Selected_Supp_Hyp_Trans = (Supp_Hyp.submatrix(Supp_Hyp.max_rank_submatrix_lex())).transpose();
    Matrix<Integer> Positive_Embedded_Generators = Gens.multiplication(Selected_Supp_Hyp_Trans);
    // GeneratorsOfToricRing = Positive_Embedded_Generators;
    // setComputed(ConeProperty::GeneratorsOfToricRing);
    dim = Positive_Embedded_Generators.nr_of_columns();
    multi_input_data.insert(make_pair(Type::cone_and_lattice,
                                      Positive_Embedded_Generators.get_elements()));  // this is the cone defined by the binomials

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    if (Grading.size() > 0) {
        // solve GeneratorsOfToricRing * grading = old_grading
        Integer dummyDenom;
        // Grading must be set directly since map entry has been processed already
        Grading = Positive_Embedded_Generators.solve_rectangular(Grading, dummyDenom);
        if (Grading.size() != dim) {
            errorOutput() << "Grading could not be transferred!" << endl;
            setComputed(ConeProperty::Grading, false);
        }
    }
}

#ifdef ENFNORMALIZ
template <>
void Cone<renf_elem_class>::prepare_input_lattice_ideal(map<InputType, Matrix<renf_elem_class> >& multi_input_data) {
    assert(false);
}
#endif

/* only used by the constructors */
template <typename Integer>
void Cone<Integer>::initialize() {
    BC_set = false;
    is_Computed = bitset<ConeProperty::EnumSize>();  // initialized to false
    dim = 0;
    unit_group_index = 1;
    inhomogeneous = false;
    input_automorphisms = false;
    rees_primary = false;
    triangulation_is_nested = false;
    triangulation_is_partial = false;
    is_approximation = false;
    verbose = libnormaliz::verbose;  // take the global default
    if (using_GMP<Integer>()) {
        change_integer_type = true;
    }
    else {
        change_integer_type = false;
    }
    autom_codim_vectors = -1;
    autom_codim_mult = -1;
    IntHullCone = NULL;
    SymmCone = NULL;
    ProjCone = NULL;

    set_parallelization();
    nmz_interrupted = 0;
    is_parallelotope = false;
    dual_original_generators = false;
    general_no_grading_denom = false;
    polytope_in_input = false;
    inequalities_in_input = false;
    rational_lattice_in_input = false;
    face_codim_bound = -1;
    positive_orthant = false;
    decimal_digits = -1;
    block_size_hollow_tri = -1;

    keep_convex_hull_data = false;
    conversion_done = false;
    ConvHullData.is_primal = false;  // to i9nitialize it

    precomputed_extreme_rays = false;
    precomputed_support_hyperplanes = false;

    is_inthull_cone = false;

    addition_constraints_allowed = false;
    addition_generators_allowed = false;

    renf_degree = 2;  // to give it a value
}

template <typename Integer>
void Cone<Integer>::set_parallelization() {
    omp_set_max_active_levels(1);

    if (thread_limit < 0)
        throw BadInputException("Invalid thread limit");

    if (parallelization_set) {
        if (thread_limit != 0)
            omp_set_num_threads(thread_limit);
    }
    else {
        if (std::getenv("OMP_NUM_THREADS") == NULL) {
            long old = omp_get_max_threads();
            if (old > default_thread_limit)
                set_thread_limit(default_thread_limit);
            omp_set_num_threads(thread_limit);
        }
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compose_basis_change(const Sublattice_Representation<Integer>& BC) {
    if (BC_set) {
        BasisChange.compose(BC);
    }
    else {
        BasisChange = BC;
        BC_set = true;
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
bool Cone<Integer>::setVerbose(bool v) {
    // we want to return the old value
    bool old = verbose;
    verbose = v;
    return old;
}
//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::deactivateChangeOfPrecision() {
    change_integer_type = false;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::checkGrading(bool compute_grading_denom) {
    if (isComputed(ConeProperty::Grading) || Grading.size() == 0) {
        GradingDenom = 1;
        return;
    }

    bool positively_graded = true;
    bool nonnegative = true;
    size_t neg_index = 0;
    Integer neg_value;
    if (Generators.nr_of_rows() > 0) {
        vector<Integer> degrees = Generators.MxV(Grading);
        for (size_t i = 0; i < degrees.size(); ++i) {
            if (degrees[i] <= 0 && (!inhomogeneous || v_scalar_product(Generators[i], Dehomogenization) == 0)) {
                // in the inhomogeneous case: test only generators of recession cone
                positively_graded = false;
                ;
                if (degrees[i] < 0) {
                    nonnegative = false;
                    neg_index = i;
                    neg_value = degrees[i];
                }
            }
        }
        if (compute_grading_denom) {
            if (positively_graded) {
                vector<Integer> test_grading = BasisChangePointed.to_sublattice_dual_no_div(Grading);
                GradingDenom = v_make_prime(test_grading);
            }
            else
                GradingDenom = 1;
        }
    }
    else {
        GradingDenom = 1;
    }

    if (isComputed(ConeProperty::Generators)) {
        if (!nonnegative) {
            throw BadInputException("Grading gives negative value " + toString(neg_value) + " for generator " +
                                    toString(neg_index + 1) + "!");
        }
        if (positively_graded) {
            setComputed(ConeProperty::Grading);
            setComputed(ConeProperty::GradingDenom);
        }
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::checkDehomogenization() {
    if (Dehomogenization.size() > 0) {
        vector<Integer> test = Generators.MxV(Dehomogenization);
        for (size_t i = 0; i < test.size(); ++i)
            if (test[i] < 0) {
                throw BadInputException("Dehomogenization has has negative value on generator " + toString(Generators[i]));
            }
    }
}
//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::setGrading(const vector<Integer>& lf) {
    if (isComputed(ConeProperty::Grading) && Grading == lf) {
        return;
    }

    if (lf.size() != dim) {
        throw BadInputException("Grading linear form has wrong dimension " + toString(lf.size()) + " (should be " +
                                toString(dim) + ")");
    }

    Grading = lf;
    checkGrading(false);  // no computation of GradingDenom
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::setWeights() {
    if (WeightsGrad.nr_of_columns() != dim) {
        WeightsGrad = Matrix<Integer>(0, dim);  // weight matrix for ordering
    }
    if (Grading.size() > 0 && WeightsGrad.nr_of_rows() == 0)
        WeightsGrad.append(Grading);
    GradAbs = vector<bool>(WeightsGrad.nr_of_rows(), false);
}
//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::setDehomogenization(const vector<Integer>& lf) {
    if (lf.size() != dim) {
        throw BadInputException("Dehomogenizing linear form has wrong dimension " + toString(lf.size()) + " (should be " +
                                toString(dim) + ")");
    }
    Dehomogenization = lf;
    setComputed(ConeProperty::Dehomogenization);
}

//---------------------------------------------------------------------------

/* check what is computed */
template <typename Integer>
bool Cone<Integer>::isComputed(ConeProperty::Enum prop) const {
    return is_Computed.test(prop);
}

template <typename Integer>
const ConeProperties& Cone<Integer>::getIsComputed() const {
    return is_Computed;
}

template <typename Integer>
void Cone<Integer>::setComputed(ConeProperty::Enum prop) {
    is_Computed.set(prop);
}

template <typename Integer>
void Cone<Integer>::setComputed(ConeProperty::Enum prop, bool value) {
    is_Computed.set(prop, value);
}

/*
template <typename Integer>
bool Cone<Integer>::isComputed(ConeProperties CheckComputed) const {
    return CheckComputed.reset(is_Computed).any();
}
*/

/*

template <typename Integer>
void Cone<Integer>::resetComputed(ConeProperty::Enum prop) {
    is_Computed.reset(prop);
}
*/

/* getter */

template <typename Integer>
Cone<Integer>& Cone<Integer>::getIntegerHullCone() const {
    return *IntHullCone;
}

template <typename Integer>
Cone<Integer>& Cone<Integer>::getProjectCone() const {
    return *ProjCone;
}

template <typename Integer>
Cone<Integer>& Cone<Integer>::getSymmetrizedCone() const {
    return *SymmCone;
}

template <typename Integer>
size_t Cone<Integer>::getRank() {
    compute(ConeProperty::Sublattice);
    return BasisChange.getRank();
}

template <typename Integer>
size_t Cone<Integer>::get_rank_internal() {  // introduced at a time when "internal"
                                             // external calls of compute were distinguished
                                             // most likely supefluous now
    if (!isComputed(ConeProperty::Sublattice))
        compute(ConeProperty::Sublattice);
    return BasisChange.getRank();
}

template <typename Integer>  // computation depends on InputGenerators
Integer Cone<Integer>::getInternalIndex() {
    compute(ConeProperty::OriginalMonoidGenerators);
    return internal_index;
}

template <typename Integer>
Integer Cone<Integer>::getUnitGroupIndex() {
    compute(ConeProperty::OriginalMonoidGenerators, ConeProperty::IsIntegrallyClosed);
    return unit_group_index;
}

template <typename Integer>
size_t Cone<Integer>::getRecessionRank() {
    compute(ConeProperty::RecessionRank);
    return recession_rank;
}

template <typename Integer>
long Cone<Integer>::getAffineDim() {
    compute(ConeProperty::AffineDim);
    return affine_dim;
}

template <typename Integer>
const Sublattice_Representation<Integer>& Cone<Integer>::getSublattice() {
    compute(ConeProperty::Sublattice);
    return BasisChange;
}

template <typename Integer>
const Sublattice_Representation<Integer>& Cone<Integer>::get_sublattice_internal() {
    if (!isComputed(ConeProperty::Sublattice))
        compute(ConeProperty::Sublattice);
    return BasisChange;
}

template <typename Integer>
const Matrix<Integer>& Cone<Integer>::getOriginalMonoidGeneratorsMatrix() {
    compute(ConeProperty::OriginalMonoidGenerators);
    return InputGenerators;
}
template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getOriginalMonoidGenerators() {
    compute(ConeProperty::OriginalMonoidGenerators);
    return InputGenerators.get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrOriginalMonoidGenerators() {
    compute(ConeProperty::OriginalMonoidGenerators);
    return InputGenerators.nr_of_rows();
}

template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getMaximalSubspace() {
    compute(ConeProperty::MaximalSubspace);
    return BasisMaxSubspace.get_elements();
}
template <typename Integer>
const Matrix<Integer>& Cone<Integer>::getMaximalSubspaceMatrix() {
    compute(ConeProperty::MaximalSubspace);
    return BasisMaxSubspace;
}
template <typename Integer>
size_t Cone<Integer>::getDimMaximalSubspace() {
    compute(ConeProperty::MaximalSubspace);
    return BasisMaxSubspace.nr_of_rows();
}

template <typename Integer>
const Matrix<Integer>& Cone<Integer>::getExtremeRaysMatrix() {
    compute(ConeProperty::ExtremeRays);
    return ExtremeRaysRecCone;
}
template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getExtremeRays() {
    compute(ConeProperty::ExtremeRays);
    return ExtremeRaysRecCone.get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrExtremeRays() {
    compute(ConeProperty::ExtremeRays);
    return ExtremeRaysRecCone.nr_of_rows();
}

template <typename Integer>
const Matrix<nmz_float>& Cone<Integer>::getVerticesFloatMatrix() {
    compute(ConeProperty::VerticesFloat);
    return VerticesFloat;
}
template <typename Integer>
const vector<vector<nmz_float> >& Cone<Integer>::getVerticesFloat() {
    compute(ConeProperty::VerticesFloat);
    return VerticesFloat.get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrVerticesFloat() {
    compute(ConeProperty::VerticesFloat);
    return VerticesFloat.nr_of_rows();
}

template <typename Integer>
const Matrix<nmz_float>& Cone<Integer>::getExtremeRaysFloatMatrix() {
    compute(ConeProperty::ExtremeRaysFloat);
    return ExtremeRaysFloat;
}
template <typename Integer>
const vector<vector<nmz_float> >& Cone<Integer>::getExtremeRaysFloat() {
    compute(ConeProperty::ExtremeRaysFloat);
    return ExtremeRaysFloat.get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrExtremeRaysFloat() {
    compute(ConeProperty::ExtremeRaysFloat);
    return ExtremeRaysFloat.nr_of_rows();
}

template <typename Integer>
const Matrix<nmz_float>& Cone<Integer>::getSuppHypsFloatMatrix() {
    compute(ConeProperty::SuppHypsFloat);
    return SuppHypsFloat;
}
template <typename Integer>
const vector<vector<nmz_float> >& Cone<Integer>::getSuppHypsFloat() {
    compute(ConeProperty::SuppHypsFloat);
    return SuppHypsFloat.get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrSuppHypsFloat() {
    compute(ConeProperty::SuppHypsFloat);
    return SuppHypsFloat.nr_of_rows();
}

template <typename Integer>
const Matrix<Integer>& Cone<Integer>::getVerticesOfPolyhedronMatrix() {
    compute(ConeProperty::VerticesOfPolyhedron);
    return VerticesOfPolyhedron;
}
template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getVerticesOfPolyhedron() {
    compute(ConeProperty::VerticesOfPolyhedron);
    return VerticesOfPolyhedron.get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrVerticesOfPolyhedron() {
    compute(ConeProperty::VerticesOfPolyhedron);
    return VerticesOfPolyhedron.nr_of_rows();
}

template <typename Integer>
const Matrix<Integer>& Cone<Integer>::getEquationsMatrix() {
    compute(ConeProperty::Equations);
    return BasisChange.getEquationsMatrix();
}
template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getEquations() {
    compute(ConeProperty::Equations);
    return getEquationsMatrix().get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrEquations() {
    compute(ConeProperty::Equations);
    return getEquationsMatrix().nr_of_rows();
}

template <typename Integer>
const Matrix<Integer>& Cone<Integer>::getCongruencesMatrix() {
    compute(ConeProperty::Congruences);
    return BasisChange.getCongruencesMatrix();
}
template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getCongruences() {
    compute(ConeProperty::Congruences);
    return getCongruencesMatrix().get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrCongruences() {
    compute(ConeProperty::Congruences);
    return getCongruencesMatrix().nr_of_rows();
}

template <typename Integer>
const Matrix<Integer>& Cone<Integer>::getSupportHyperplanesMatrix() {
    compute(ConeProperty::SupportHyperplanes);
    return SupportHyperplanes;
}
template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getSupportHyperplanes() {
    compute(ConeProperty::SupportHyperplanes);
    return SupportHyperplanes.get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrSupportHyperplanes() {
    compute(ConeProperty::SupportHyperplanes);
    return SupportHyperplanes.nr_of_rows();
}

/*
template <typename Integer>
map<InputType, vector<vector<Integer> > > Cone<Integer>::getConstraints() {
    compute(ConeProperty::Sublattice, ConeProperty::SupportHyperplanes);
    map<InputType, vector<vector<Integer> > > c;
    c[Type::inequalities] = SupportHyperplanes.get_elements();
    c[Type::equations] = BasisChange.getEquations();
    c[Type::congruences] = BasisChange.getCongruences();
    return c;
}
*/

template <typename Integer>
const Matrix<Integer>& Cone<Integer>::getExcludedFacesMatrix() {
    compute(ConeProperty::ExcludedFaces);
    return ExcludedFaces;
}
template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getExcludedFaces() {
    compute(ConeProperty::ExcludedFaces);
    return ExcludedFaces.get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrExcludedFaces() {
    compute(ConeProperty::ExcludedFaces);
    return ExcludedFaces.nr_of_rows();
}

template <typename Integer>
const pair<vector<SHORTSIMPLEX<Integer> >, Matrix<Integer> >& Cone<Integer>::getBasicTriangulation() {
    if (!isComputed(ConeProperty::BasicTriangulation))  // no triangulation for output computed
        compute(ConeProperty::BasicTriangulation);      // we compute the basic triangulation if necessarily
    return BasicTriangulation;
}

template <typename Integer>
const pair<vector<SHORTSIMPLEX<Integer> >, Matrix<Integer> >& Cone<Integer>::getTriangulation() {
    if (is_Computed.intersection_with(all_triangulations()).none())  // no triangulation for output computed
        compute(ConeProperty::Triangulation);                        // we compute the basic triangulation
    return Triangulation;
}

template <typename Integer>
const pair<vector<SHORTSIMPLEX<Integer> >, Matrix<Integer> >& Cone<Integer>::getTriangulation(ConeProperty::Enum quality) {
    if (!all_triangulations().test(quality)) {
        throw BadInputException("Illegal parameter in getTriangulation(ConeProperty::Enum quality)");
    }
    compute(quality);
    return Triangulation;
}

template <typename Integer>
const pair<vector<SHORTSIMPLEX<Integer> >, Matrix<Integer> >& Cone<Integer>::getConeDecomposition() {
    compute(ConeProperty::ConeDecomposition);
    return getTriangulation();
}

template <typename Integer>
const vector<pair<vector<key_t>, long> >& Cone<Integer>::getInclusionExclusionData() {
    compute(ConeProperty::InclusionExclusionData);
    return InExData;
}

template <typename Integer>
bool compareStDec(const STANLEYDATA<Integer>& A, const STANLEYDATA<Integer>& B) {
    return A.key < B.key;
}

template <typename Integer>
void Cone<Integer>::make_StanleyDec_export(const ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::StanleyDec) || isComputed(ConeProperty::StanleyDec))
        return;
    assert(isComputed(ConeProperty::BasicStanleyDec));
    auto SD = BasicStanleyDec.first.begin();
    for (; SD != BasicStanleyDec.first.end(); ++SD) {
        STANLEYDATA<Integer> NewSt;
        NewSt.key = SD->key;
        convert(NewSt.offsets, SD->offsets);
        sort(NewSt.offsets.access_elements().begin(), NewSt.offsets.access_elements().end());
        StanleyDec.first.push_back(NewSt);
    }
    StanleyDec.first.sort(compareStDec<Integer>);
    StanleyDec.second = BasicStanleyDec.second;
    setComputed(ConeProperty::StanleyDec);
}

template <typename Integer>
const pair<list<STANLEYDATA<Integer> >, Matrix<Integer> >& Cone<Integer>::getStanleyDec() {
    compute(ConeProperty::StanleyDec);
    return StanleyDec;
}

template <typename Integer>
pair<list<STANLEYDATA_int>, Matrix<Integer> >& Cone<Integer>::getStanleyDec_mutable() {
    assert(isComputed(ConeProperty::BasicStanleyDec));
    return BasicStanleyDec;
}

template <typename Integer>
size_t Cone<Integer>::getTriangulationSize() {
    compute(ConeProperty::TriangulationSize);
    return TriangulationSize;
}

template <typename Integer>
Integer Cone<Integer>::getTriangulationDetSum() {
    compute(ConeProperty::TriangulationDetSum);
    return TriangulationDetSum;
}

template <typename Integer>
vector<Integer> Cone<Integer>::getWitnessNotIntegrallyClosed() {
    compute(ConeProperty::WitnessNotIntegrallyClosed);
    return WitnessNotIntegrallyClosed;
}

template <typename Integer>
vector<Integer> Cone<Integer>::getGeneratorOfInterior() {
    compute(ConeProperty::GeneratorOfInterior);
    return GeneratorOfInterior;
}

template <typename Integer>
vector<Integer> Cone<Integer>::getAxesScaling() {
    if (!isComputed(ConeProperty::AxesScaling))
        throw NotComputableException("AxesScaling is not a computation goal");
    return AxesScaling;
}

template <typename Integer>
vector<Integer> Cone<Integer>::getCoveringFace() {
    compute(ConeProperty::CoveringFace);
    return CoveringFace;
}

template <typename Integer>
const Matrix<Integer>& Cone<Integer>::getHilbertBasisMatrix() {
    compute(ConeProperty::HilbertBasis);
    return HilbertBasis;
}
template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getHilbertBasis() {
    compute(ConeProperty::HilbertBasis);
    return HilbertBasis.get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrHilbertBasis() {
    compute(ConeProperty::HilbertBasis);
    return HilbertBasis.nr_of_rows();
}

template <typename Integer>
const Matrix<Integer>& Cone<Integer>::getModuleGeneratorsOverOriginalMonoidMatrix() {
    compute(ConeProperty::ModuleGeneratorsOverOriginalMonoid);
    return ModuleGeneratorsOverOriginalMonoid;
}
template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getModuleGeneratorsOverOriginalMonoid() {
    compute(ConeProperty::ModuleGeneratorsOverOriginalMonoid);
    return ModuleGeneratorsOverOriginalMonoid.get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrModuleGeneratorsOverOriginalMonoid() {
    compute(ConeProperty::ModuleGeneratorsOverOriginalMonoid);
    return ModuleGeneratorsOverOriginalMonoid.nr_of_rows();
}

template <typename Integer>
const Matrix<Integer>& Cone<Integer>::getModuleGeneratorsMatrix() {
    compute(ConeProperty::ModuleGenerators);
    return ModuleGenerators;
}
template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getModuleGenerators() {
    compute(ConeProperty::ModuleGenerators);
    return ModuleGenerators.get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrModuleGenerators() {
    compute(ConeProperty::ModuleGenerators);
    return ModuleGenerators.nr_of_rows();
}

template <typename Integer>
const Matrix<Integer>& Cone<Integer>::getDeg1ElementsMatrix() {
    compute(ConeProperty::Deg1Elements);
    return Deg1Elements;
}
template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getDeg1Elements() {
    compute(ConeProperty::Deg1Elements);
    return Deg1Elements.get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrDeg1Elements() {
    compute(ConeProperty::Deg1Elements);
    return Deg1Elements.nr_of_rows();
}

template <typename Integer>
size_t Cone<Integer>::getNumberLatticePoints() {
    compute(ConeProperty::NumberLatticePoints);
    return number_lattice_points;
}

template <typename Integer>
const Matrix<Integer>& Cone<Integer>::getLatticePointsMatrix() {
    compute(ConeProperty::LatticePoints);
    if (!inhomogeneous)
        return Deg1Elements;
    else
        return ModuleGenerators;
}

template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getLatticePoints() {
    compute(ConeProperty::LatticePoints);
    return getLatticePointsMatrix().get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrLatticePoints() {
    compute(ConeProperty::LatticePoints);
    return getLatticePointsMatrix().nr_of_rows();
}

template <typename Integer>
const HilbertSeries& Cone<Integer>::getHilbertSeries() {
    compute(ConeProperty::HilbertSeries);
    return HSeries;
}

template <typename Integer>
const HilbertSeries& Cone<Integer>::getEhrhartSeries() {
    compute(ConeProperty::EhrhartSeries);
    if (inhomogeneous)
        return EhrSeries;
    else
        return HSeries;
}

template <typename Integer>
vector<Integer> Cone<Integer>::getGrading() {
    compute(ConeProperty::Grading);
    return Grading;
}

template <typename Integer>
Integer Cone<Integer>::getGradingDenom() {
    compute(ConeProperty::Grading);
    return GradingDenom;
}

template <typename Integer>
vector<Integer> Cone<Integer>::getDehomogenization() {
    compute(ConeProperty::Dehomogenization);
    return Dehomogenization;
}

template <typename Integer>
mpq_class Cone<Integer>::getMultiplicity() {
    compute(ConeProperty::Multiplicity);
    return multiplicity;
}

template <typename Integer>
mpq_class Cone<Integer>::getVolume() {
    compute(ConeProperty::Volume);
    return volume;
}

template <typename Integer>
renf_elem_class Cone<Integer>::getRenfVolume() {
    throw NotComputableException("For the volume of rational polytopes use getVolume()");
}

template <typename Integer>
vector<string> Cone<Integer>::getRenfData() {
    throw NotComputableException("Renf data only available for Cone<renf_elem_class>");
}

template <typename Integer>
string Cone<Integer>::getRenfGenerator() {
    return "";
}

template <typename Integer>
string Cone<Integer>::getRenfGenerator(const renf_class*) {
    return "";
}

template <typename Integer>
vector<string> Cone<Integer>::getRenfData(const renf_class* renf) {
    throw NotComputableException("Renf data only available for Cone<renf_elem_class>");
}

template <typename Integer>
const renf_class* Cone<Integer>::getRenf() {
    throw NotComputableException("Renf only available for Cone<renf_elem_class>");
}

template <typename Integer>
renf_class_shared Cone<Integer>::getRenfSharedPtr() {
    if (using_renf<Integer>())
        throw NotComputableException("RenfSharedPtr only available for Cone<renf_elem_class>");
    else
        return RenfSharedPtr;
}

#ifdef ENFNORMALIZ
template <>
mpq_class Cone<renf_elem_class>::getVolume() {
    throw NotComputableException("For the volume of algebraic polytopes use getRenfVolume()");
}

template <>
renf_elem_class Cone<renf_elem_class>::getRenfVolume() {
    compute(ConeProperty::RenfVolume);
    return renf_volume;
}

template <>
vector<string> Cone<renf_elem_class>::getRenfData(const renf_class* renf) {
    std::string s = renf->to_string();

    static const char* prefix = "NumberField(";
    static const char* split = ", ";
    static const char* suffix = ")";

    assert(s.find(prefix) == 0);
    assert(s.find(split) > 0);
    assert(s.rfind(suffix) == s.size() - strlen(suffix));

    s = s.substr(strlen(prefix), s.length() - strlen(prefix) - strlen(suffix));

    const int at = s.find(", ");

    return vector<string>{
        s.substr(0, at),
        s.substr(at + strlen(split)),
    };
}

template <>
vector<string> Cone<renf_elem_class>::getRenfData() {
    return Cone<renf_elem_class>::getRenfData(Renf);
}

template <>
string Cone<renf_elem_class>::getRenfGenerator(const renf_class* renf) {
    string GenName;
    vector<string> RenfData = Cone<renf_elem_class>::getRenfData(renf);
    string min_poly = RenfData[0];
    for (size_t i = 0; i < min_poly.size(); ++i) {
        if (isalpha(min_poly[i])) {
            GenName = min_poly[i];
            break;
        }
    }
    return GenName;
}

template <>
string Cone<renf_elem_class>::getRenfGenerator() {
    return Cone<renf_elem_class>::getRenfGenerator(Renf);
}

template <>
const renf_class* Cone<renf_elem_class>::getRenf() {
    return Renf;
}

/*template<>
const std::shared_ptr<const renf_class>  getRenfSharedPtr(){
    return RenfSharedPtr;
}*/
#endif

template <typename Integer>
nmz_float Cone<Integer>::getEuclideanVolume() {
    compute(ConeProperty::Volume);
    return euclidean_volume;
}

template <typename Integer>
mpq_class Cone<Integer>::getVirtualMultiplicity() {
    if (!isComputed(ConeProperty::VirtualMultiplicity))  // in order not to compute the triangulation
        compute(ConeProperty::VirtualMultiplicity);      // which is deleted if not asked for explicitly
    return IntData.getVirtualMultiplicity();
}

template <typename Integer>
const pair<HilbertSeries, mpz_class>& Cone<Integer>::getWeightedEhrhartSeries() {
    if (!isComputed(ConeProperty::WeightedEhrhartSeries))  // see above
        compute(ConeProperty::WeightedEhrhartSeries);
    return getIntData().getWeightedEhrhartSeries();
}

template <typename Integer>
IntegrationData& Cone<Integer>::getIntData() {  // cannot be made const
    return IntData;
}

template <typename Integer>
mpq_class Cone<Integer>::getIntegral() {
    if (!isComputed(ConeProperty::Integral))  // see above
        compute(ConeProperty::Integral);
    return IntData.getIntegral();
}

template <typename Integer>
nmz_float Cone<Integer>::getEuclideanIntegral() {
    if (!isComputed(ConeProperty::Integral))  // see above
        compute(ConeProperty::EuclideanIntegral);
    return IntData.getEuclideanIntegral();
}

template <typename Integer>
bool Cone<Integer>::isPointed() {
    compute(ConeProperty::IsPointed);
    return pointed;
}

template <typename Integer>
bool Cone<Integer>::isEmptySemiOpen() {
    compute(ConeProperty::IsEmptySemiOpen);
    return empty_semiopen;
}

template <typename Integer>
bool Cone<Integer>::isInhomogeneous() {
    return inhomogeneous;
}

template <typename Integer>
bool Cone<Integer>::isIntHullCone() {
    return is_inthull_cone;
}

template <typename Integer>
bool Cone<Integer>::isDeg1ExtremeRays() {
    compute(ConeProperty::IsDeg1ExtremeRays);
    return deg1_extreme_rays;
}

template <typename Integer>
bool Cone<Integer>::isGorenstein() {
    compute(ConeProperty::IsGorenstein);
    return Gorenstein;
}

template <typename Integer>
bool Cone<Integer>::isDeg1HilbertBasis() {
    compute(ConeProperty::IsDeg1HilbertBasis);
    return deg1_hilbert_basis;
}

template <typename Integer>
bool Cone<Integer>::isIntegrallyClosed() {
    compute(ConeProperty::IsIntegrallyClosed);
    return integrally_closed;
}

template <typename Integer>
bool Cone<Integer>::isReesPrimary() {
    compute(ConeProperty::IsReesPrimary);
    return rees_primary;
}

template <typename Integer>
Integer Cone<Integer>::getReesPrimaryMultiplicity() {
    compute(ConeProperty::ReesPrimaryMultiplicity);
    return ReesPrimaryMultiplicity;
}

// the information about the triangulation will just be returned
// if no triangulation was computed so far they return false
template <typename Integer>
bool Cone<Integer>::isTriangulationNested() {
    if (!isComputed(ConeProperty::IsTriangulationNested))
        throw NotComputableException("isTriangulationNested() only defined if a triangulation has been computed");
    return triangulation_is_nested;
}
template <typename Integer>
bool Cone<Integer>::isTriangulationPartial() {
    if (!isComputed(ConeProperty::IsTriangulationPartial))
        throw NotComputableException("isTriangulationPartial() only defined if a triangulation has been computed");
    return triangulation_is_partial;
}

template <typename Integer>
size_t Cone<Integer>::getModuleRank() {
    compute(ConeProperty::ModuleRank);
    return module_rank;
}

template <typename Integer>
vector<Integer> Cone<Integer>::getClassGroup() {
    compute(ConeProperty::ClassGroup);
    return ClassGroup;
}

template <typename Integer>
const AutomorphismGroup<Integer>& Cone<Integer>::getAutomorphismGroup(ConeProperty::Enum quality) {
    if (!all_automorphisms().test(quality)) {
        throw BadInputException("Illegal parameter in getAutomorphismGroup(ConeProperty::Enum quality)");
    }
    compute(quality);
    return Automs;
}

template <typename Integer>
const AutomorphismGroup<Integer>& Cone<Integer>::getAutomorphismGroup() {
    if (is_Computed.intersection_with(all_automorphisms()).none()) {
        throw BadInputException("No automorphism group computed. Use getAutomorphismGroup(ConeProperty::Enum quality)");
    }

    return Automs;
}

template <typename Integer>
const map<dynamic_bitset, int>& Cone<Integer>::getFaceLattice() {
    compute(ConeProperty::FaceLattice);
    return FaceLat;
}

template <typename Integer>
const vector<dynamic_bitset>& Cone<Integer>::getIncidence() {
    compute(ConeProperty::Incidence);
    return SuppHypInd;
}

template <typename Integer>
vector<size_t> Cone<Integer>::getFVector() {
    compute(ConeProperty::FVector);
    return f_vector;
}

template <typename Integer>
const map<dynamic_bitset, int>& Cone<Integer>::getDualFaceLattice() {
    compute(ConeProperty::DualFaceLattice);
    return DualFaceLat;
}

template <typename Integer>
const vector<dynamic_bitset>& Cone<Integer>::getDualIncidence() {
    compute(ConeProperty::DualIncidence);
    return DualSuppHypInd;
}

template <typename Integer>
vector<size_t> Cone<Integer>::getDualFVector() {
    compute(ConeProperty::DualFVector);
    return dual_f_vector;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_lattice_points_in_polytope(ConeProperties& ToCompute) {
    assert(false);
}

#ifdef ENFNORMALIZ

// special version to avoid problems with machine integer etc.
template <>
void Cone<renf_elem_class>::project_and_lift(const ConeProperties& ToCompute,
                                             Matrix<renf_elem_class>& Deg1,
                                             const Matrix<renf_elem_class>& Gens,
                                             const Matrix<renf_elem_class>& Supps,
                                             const Matrix<renf_elem_class>& Congs,
                                             const vector<renf_elem_class> GradingOnPolytope) {
    vector<dynamic_bitset> Ind;

    Ind = vector<dynamic_bitset>(Supps.nr_of_rows(), dynamic_bitset(Gens.nr_of_rows()));
    for (size_t i = 0; i < Supps.nr_of_rows(); ++i)
        for (size_t j = 0; j < Gens.nr_of_rows(); ++j)
            if (v_scalar_product(Supps[i], Gens[j]) == 0)
                Ind[i][j] = true;

    size_t rank = BasisChangePointed.getRank();

    Matrix<renf_elem_class> Verts;
    if (isComputed(ConeProperty::Generators)) {
        vector<key_t> choice = identity_key(Gens.nr_of_rows());  // Gens.max_rank_submatrix_lex();
        if (choice.size() >= dim)
            Verts = Gens.submatrix(choice);
    }

    Matrix<mpz_class> Raw(0, Gens.nr_of_columns());

    vector<renf_elem_class> Dummy;
    ProjectAndLift<renf_elem_class, mpz_class> PL;
    PL = ProjectAndLift<renf_elem_class, mpz_class>(Supps, Ind, rank);
    PL.set_grading_denom(1);
    PL.set_verbose(verbose);
    PL.set_no_relax(ToCompute.test(ConeProperty::NoRelax));
    PL.set_LLL(false);
    PL.set_vertices(Verts);
    PL.compute();
    PL.put_eg1Points_into(Raw);

    Deg1Elements.resize(0, dim);      // done here because they may not be defined earlier
    ModuleGenerators.resize(0, dim);  // in the renf case module generators appear only
                                      // as lattice points in polytopes
    for (size_t i = 0; i < Raw.nr_of_rows(); ++i) {
        vector<renf_elem_class> point(dim);
        for (size_t j = 0; j < dim; ++j) {
            point[j] = Raw[i][j + 1];
        }
        if (inhomogeneous) {
            ModuleGenerators.append(point);
        }
        else {
            Deg1Elements.append(point);
        }
    }
    if (inhomogeneous)
        ModuleGenerators.sort_by_weights(WeightsGrad, GradAbs);
    else
        Deg1Elements.sort_by_weights(WeightsGrad, GradAbs);

    number_lattice_points = PL.getNumberLatticePoints();
    setComputed(ConeProperty::NumberLatticePoints);

    if (verbose)
        verboseOutput() << "Project-and-lift complete" << endl
                        << "------------------------------------------------------------" << endl;
}

// replacement of try_approximation_or_projection for renf_elem_class
// in connection with project_and_lift above
// unification perhaps not impossible, but not hassle free
template <>
void Cone<renf_elem_class>::compute_lattice_points_in_polytope(ConeProperties& ToCompute) {
    if (isComputed(ConeProperty::ModuleGenerators) || isComputed(ConeProperty::Deg1Elements))
        return;
    if (!ToCompute.test(ConeProperty::ModuleGenerators) && !ToCompute.test(ConeProperty::Deg1Elements))
        return;

    if (!isComputed(ConeProperty::Grading) && !isComputed(ConeProperty::Dehomogenization))
        throw BadInputException("Lattice points not computable without grading in the homogeneous case");

    if (ToCompute.test(ConeProperty::KeepOrder))
        compute(ConeProperty::ExtremeRays, ConeProperty::SupportHyperplanes, ConeProperty::KeepOrder);
    else
        compute(ConeProperty::ExtremeRays, ConeProperty::SupportHyperplanes);
    if (!isComputed(ConeProperty::SupportHyperplanes))
        throw FatalException("Could not compute SupportHyperplanes");

    if (inhomogeneous && (ExtremeRaysRecCone.nr_of_rows() > 0 || BasisMaxSubspace.nr_of_rows() > 0 )) {
        throw BadInputException("Lattice points not computable for unbounded poyhedra over number fields");
    }

    // The same procedure as in cone.cpp, but no approximation, and grading always extra first coordinate

    renf_elem_class MinusOne = -1;

    Matrix<renf_elem_class> SuppsHelp = SupportHyperplanes;
    Matrix<renf_elem_class> Equs = BasisChange.getEquationsMatrix();
    for (size_t i = 0; i < Equs.nr_of_rows(); ++i) {  // add equations as inequalities
        SuppsHelp.append(Equs[i]);
        SuppsHelp.append(Equs[i]);
        v_scalar_multiplication(SuppsHelp[SuppsHelp.nr_of_rows()-1], MinusOne);
    }
    renf_elem_class Zero = 0;
    insert_column(SuppsHelp, 0, Zero);

    // we insert the degree/level into the 0th column
    vector<renf_elem_class> ExtraEqu(1, -1);
    for (size_t j = 0; j < dim; ++j) {
        if (inhomogeneous)
            ExtraEqu.push_back(Dehomogenization[j]);
        else
            ExtraEqu.push_back(Grading[j]);
    }
    SuppsHelp.append(ExtraEqu);
    v_scalar_multiplication(ExtraEqu, MinusOne);
    SuppsHelp.append(ExtraEqu);

    Matrix<renf_elem_class> Supps(SuppsHelp);

    Matrix<renf_elem_class> Gens;
    if (inhomogeneous)
        Gens = VerticesOfPolyhedron;
    else
        Gens = ExtremeRays;

    Matrix<renf_elem_class> GradGen(0, dim + 1);
    for (size_t i = 0; i < Gens.nr_of_rows(); ++i) {
        vector<renf_elem_class> gg(dim + 1);
        for (size_t j = 0; j < dim; ++j)
            gg[j + 1] = Gens[i][j];
        if (inhomogeneous)
            gg[0] = v_scalar_product(Gens[i], Dehomogenization);
        else
            gg[0] = v_scalar_product(Gens[i], Grading);
        GradGen.append(gg);
    }

    Matrix<renf_elem_class> DummyCongs(0, 0);
    Matrix<renf_elem_class> DummyResult(0, 0);
    vector<renf_elem_class> dummy_grad(0);

    if (inhomogeneous)
        project_and_lift(ToCompute, DummyResult, GradGen, Supps, DummyCongs, dummy_grad);
    else
        project_and_lift(ToCompute, DummyResult, GradGen, Supps, DummyCongs, dummy_grad);

    // In this version, the lattice points are transferresd into the cone
    // in project_and_lift above.

    if (inhomogeneous)
        setComputed(ConeProperty::ModuleGenerators);
    else
        setComputed(ConeProperty::Deg1Elements);
}

#endif

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::prepare_volume_computation(ConeProperties& ToCompute) {
    assert(false);
}

#ifdef ENFNORMALIZ
template <>
void Cone<renf_elem_class>::prepare_volume_computation(ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::Volume))
        return;

    if (!inhomogeneous && Grading.size() != dim)  // we cannot check the cone property Grading yet
                                                  // will be done later anyway
        throw NotComputableException("Volume needs an expicit grading for algebraic polytopes in the homogeneous case");
    if (getRank() != dim)
        throw NotComputableException("Normaliz requires full dimension for volume of algebraic polytope");
    vector<renf_elem_class> Grad;
    if (inhomogeneous)
        Grad = Dehomogenization;
    else
        Grad = Grading;

    /* for(size_t i=0;i<dim;++i)
        if(!Grad[i].is_integer())
            throw NotComputableException("Entries of grading or dehomogenization must be mpzegers for volume");*/

    vector<mpz_class> Grad_mpz;
    for (size_t i = 0; i < dim; ++i)
        Grad_mpz.push_back(Grad[i].num());
    for (size_t i = 0; i < dim; ++i) {
        if (Grad[i] != Grad_mpz[i])
            throw BadInputException("Entries of grading or dehomogenization must be coprime integers for volume");
    }
    // if(libnormaliz::v_make_prime(Grad_mpz)!=1)
    //    throw NotComputableException("Entries of grading or dehomogenization must be coprime integers for volume");

    vector<double> Grad_double(dim);
    for (size_t i = 0; i < dim; ++i)
        // libnormaliz::convert(Grad_double[i],Grad_mpz[i]);
        Grad_double[i] = Grad_mpz[i].get_d();

    double norm = v_scalar_product(Grad_double, Grad_double);
    euclidean_height = sqrt(norm);
}
#endif
//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_full_cone(ConeProperties& ToCompute) {
    if (ToCompute.test(ConeProperty::PullingTriangulationInternal))
        assert(ToCompute.count() == 1);

    if (change_integer_type) {
        try {
            compute_full_cone_inner<MachineInteger>(ToCompute);
        } catch (const ArithmeticException& e) {
            if (verbose) {
                verboseOutput() << e.what() << endl;
                verboseOutput() << "Restarting with a bigger type." << endl;
            }
            change_integer_type = false;
        }
    }

    if (!change_integer_type) {
        if (!using_GMP<Integer>() && !ToCompute.test(ConeProperty::DefaultMode)) {
            compute_full_cone_inner<Integer>(ToCompute);
        }
        else {
            try {
                compute_full_cone_inner<Integer>(ToCompute);
            } catch (const ArithmeticException& e) {  // the nonly reason for failure is an overflow in a degree computation
                if (verbose) {                        // so we can relax in default mode
                    verboseOutput() << e.what() << endl;
                    verboseOutput() << "Reducing computation goals." << endl;
                }
                ToCompute.reset(ConeProperty::HilbertBasis);
                ToCompute.reset(ConeProperty::HilbertSeries);
                compute_full_cone_inner<Integer>(ToCompute);
            }
        }
    }
}

template <typename Integer>
template <typename IntegerFC>
void Cone<Integer>::compute_full_cone_inner(ConeProperties& ToCompute) {
#ifdef NMZ_EXTENDED_TESTS
    if (!using_GMP<IntegerFC>() && !using_renf<IntegerFC>() && test_arith_overflow_full_cone)
        throw ArithmeticException(0);
#endif

    if (ToCompute.test(ConeProperty::IsPointed) && Grading.size() == 0) {
        if (verbose) {
            verboseOutput() << "Checking pointedness first" << endl;
        }
        ConeProperties Dualize;
        Dualize.set(ConeProperty::SupportHyperplanes);
        Dualize.set(ConeProperty::ExtremeRays);
        Dualize.set(ConeProperty::KeepOrder, ToCompute.test(ConeProperty::KeepOrder));
        compute(Dualize);
    }

    Matrix<IntegerFC> FC_Gens;

    BasisChangePointed.convert_to_sublattice(FC_Gens, Generators);
    Full_Cone<IntegerFC> FC(FC_Gens, !(ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid) ||
                                       ToCompute.test(ConeProperty::AllGeneratorsTriangulation)));
    // !ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid) blocks make_prime in full_cone.cpp

    /* activate bools in FC */

    if (ToCompute.test(ConeProperty::IsEmptySemiOpen) && !isComputed(ConeProperty::IsEmptySemiOpen))
        FC.check_semiopen_empty = true;

    if (ToCompute.test(ConeProperty::FullConeDynamic)) { // want to compute integer hull
        FC.do_supphyps_dynamic = true;
        BasisChangePointed.convert_to_sublattice(FC.Basis_Max_Subspace, RationalBasisMaxSubspace);
        BasisChangePointed.convert_to_sublattice(FC.RationalExtremeRays, RationalExtremeRays);
        if (IntHullNorm.size() > 0)
            BasisChangePointed.convert_to_sublattice_dual(FC.IntHullNorm, IntHullNorm);
    }

    FC.keep_convex_hull_data = keep_convex_hull_data;

    FC.verbose = verbose;
    FC.renf_degree = renf_degree;  // even if it is not defined without renf

    FC.inhomogeneous = inhomogeneous;
    // FC.explicit_h_vector=(ToCompute.test(ConeProperty::ExplicitHilbertSeries) && !isComputed(ConeProperty::HilbertSeries));

    if (ToCompute.test(ConeProperty::HilbertSeries)) {
        FC.do_h_vector = true;
        FC.Hilbert_Series.set_period_bounded(HSeries.get_period_bounded());
    }
    if (ToCompute.test(ConeProperty::HilbertBasis)) {
        FC.do_Hilbert_basis = true;
    }
    if (ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid)) {
        FC.do_module_gens_intcl = true;
    }

    if (ToCompute.test(ConeProperty::IsIntegrallyClosed) || ToCompute.test(ConeProperty::WitnessNotIntegrallyClosed)) {
        FC.do_integrally_closed = true;
    }

    if (ToCompute.test(ConeProperty::BasicTriangulation)) {
        FC.keep_triangulation = true;
    }

    if (ToCompute.test(ConeProperty::PullingTriangulationInternal)) {
        FC.pulling_triangulation = true;
    }

    if (ToCompute.test(ConeProperty::ConeDecomposition)) {
        FC.do_cone_dec = true;
    }
    if (ToCompute.test(ConeProperty::Multiplicity) || (using_renf<Integer>() && ToCompute.test(ConeProperty::Volume))) {
        FC.do_multiplicity = true;
    }

    if (ToCompute.test(ConeProperty::TriangulationDetSum)) {
        FC.do_determinants = true;
    }
    if (ToCompute.test(ConeProperty::TriangulationSize)) {
        FC.do_triangulation_size = true;
    }
    if (ToCompute.test(ConeProperty::NoSubdivision)) {
        FC.use_bottom_points = false;
    }
    if (ToCompute.test(ConeProperty::Deg1Elements) && !using_renf<Integer>()) {
        FC.do_deg1_elements = true;
    }
    if (ToCompute.test(ConeProperty::BasicStanleyDec)) {
        FC.do_Stanley_dec = true;
    }

    if (ToCompute.test(ConeProperty::Approximate) && ToCompute.test(ConeProperty::Deg1Elements)) {
        FC.do_approximation = true;
        FC.do_deg1_elements = true;
    }

    if (ToCompute.test(ConeProperty::DefaultMode)) {
        FC.do_default_mode = true;
    }
    if (ToCompute.test(ConeProperty::BottomDecomposition)) {
        FC.do_bottom_dec = true;
    }
    if (ToCompute.test(ConeProperty::NoBottomDec)) {
        FC.suppress_bottom_dec = true;
    }
    if (ToCompute.test(ConeProperty::KeepOrder) && !dual_original_generators) {
        FC.keep_order = true;  // The second condition restricts KeepOrder to the dual cone if the input
                               // was inequalities
    }
    if (ToCompute.test(ConeProperty::ClassGroup)) {
        FC.do_class_group = true;
    }
    if (ToCompute.test(ConeProperty::ModuleRank)) {
        FC.do_module_rank = true;
    }

    if (ToCompute.test(ConeProperty::HSOP)) {
        FC.do_hsop = true;
    }

    /* Give extra data to FC */
    if (isComputed(ConeProperty::ExtremeRays)) {
        FC.Extreme_Rays_Ind = ExtremeRaysIndicator;
        FC.is_Computed.set(ConeProperty::ExtremeRays);
    }

    /* if(isComputed(ConeProperty::Deg1Elements)){
        Matrix<IntegerFC> Deg1Converted;
        BasisChangePointed.convert_to_sublattice(Deg1Converted, Deg1Elements);
        for(size_t i=0;i<Deg1Elements.nr_of_rows();++i)
            FC.Deg1_Elements.push_back(Deg1Converted[i]);
        FC.is_Computed.set(ConeProperty::Deg1Elements);
    }*/

    if (HilbertBasisRecCone.nr_of_rows() > 0) {
        BasisChangePointed.convert_to_sublattice(FC.HilbertBasisRecCone, HilbertBasisRecCone);
    }

    if (ExcludedFaces.nr_of_rows() != 0) {
        BasisChangePointed.convert_to_sublattice_dual(FC.ExcludedFaces, ExcludedFaces);
    }
    if (isComputed(ConeProperty::ExcludedFaces)) {
        FC.is_Computed.set(ConeProperty::ExcludedFaces);
    }

    if (inhomogeneous) {
        BasisChangePointed.convert_to_sublattice_dual_no_div(FC.Truncation, Dehomogenization);
    }
    if (Grading.size() > 0) {  // IMPORTANT: Truncation must be set before Grading
        if (ToCompute.test(ConeProperty::NoGradingDenom))
            BasisChangePointed.convert_to_sublattice_dual_no_div(FC.Grading, Grading);
        else
            BasisChangePointed.convert_to_sublattice_dual(FC.Grading, Grading);
        if (isComputed(ConeProperty::Grading)) {  // is grading positive?
            FC.is_Computed.set(ConeProperty::Grading);
        }
    }

    if (ToCompute.test(ConeProperty::Automorphisms)) {
        FC.do_automorphisms = true;
        FC.quality_of_automorphisms = AutomParam::integral;  // if necessary changed into
                                                             // algebtraic in full_cone.cpp
    }

    if (ToCompute.test(ConeProperty::RationalAutomorphisms)) {
        FC.do_automorphisms = true;
        FC.quality_of_automorphisms = AutomParam::rational;
    }

    /*
        // if (ToCompute.test(ConeProperty::ExploitIsosMult)) { DONE VIA DESCENT
        //    FC.exploit_automs_mult = true;
        // }
        if (ToCompute.test(ConeProperty::ExploitAutomsVectors)) {
            FC.exploit_automs_vectors = true;
        }
        FC.autom_codim_vectors = autom_codim_vectors;
        FC.autom_codim_mult = autom_codim_mult;
    }*/

    if (Inequalities.nr_of_rows() != 0 && !isComputed(ConeProperty::SupportHyperplanes)) {
        BasisChangePointed.convert_to_sublattice_dual(FC.Support_Hyperplanes, Inequalities);
    }
    if (isComputed(ConeProperty::SupportHyperplanes)) {
        BasisChangePointed.convert_to_sublattice_dual(FC.Support_Hyperplanes, SupportHyperplanes);
        FC.is_Computed.set(ConeProperty::SupportHyperplanes);
        FC.do_all_hyperplanes = false;
    }

    if (is_approximation)
        give_data_of_approximated_cone_to(FC);

    bool must_triangulate = FC.do_h_vector || FC.do_Hilbert_basis || FC.do_multiplicity || FC.do_Stanley_dec ||
                            FC.do_module_rank || FC.do_module_gens_intcl || FC.do_bottom_dec || FC.do_hsop ||
                            FC.do_integrally_closed || FC.keep_triangulation || FC.do_integrally_closed || FC.do_cone_dec ||
                            FC.do_determinants || FC.do_triangulation_size || FC.do_deg1_elements || FC.do_default_mode;

    // Do we really need the Full_Cone? ALREADY CHECKED

    /* if (!must_triangulate && !FC.do_automorphisms && isComputed(ConeProperty::SupportHyperplanes) &&
        isComputed(ConeProperty::ExtremeRays) && !ToCompute.test(ConeProperty::Grading) &&
        !ToCompute.test(ConeProperty::IsPointed) && !ToCompute.test(ConeProperty::ClassGroup) &&
        !ToCompute.test(ConeProperty::IsEmptySemiOpen) )
        return; */

    // restore if usaeful
    if (!must_triangulate && keep_convex_hull_data && ConvHullData.SLR.equal(BasisChangePointed) &&
        ConvHullData.nr_threads == omp_get_max_threads() && ConvHullData.Generators.nr_of_rows() > 0) {
        FC.keep_order = true;
        FC.restore_previous_computation(ConvHullData, true);  // true = primal
    }

    /* do the computation */

    try {
        try {
            FC.compute();
        } catch (const NotIntegrallyClosedException&) {
        }
        setComputed(ConeProperty::Sublattice);
        extract_data(FC, ToCompute);
        ToCompute.reset(is_Computed);
        // make sure we minimize the excluded faces if requested
        if (ToCompute.test(ConeProperty::ExcludedFaces) || ToCompute.test(ConeProperty::SupportHyperplanes)) {
            FC.prepare_inclusion_exclusion();  // WHY THIS ??????
        }
        if (isComputed(ConeProperty::IsPointed) && pointed)
            setComputed(ConeProperty::MaximalSubspace);
    } catch (const NonpointedException&) {
        setComputed(ConeProperty::Sublattice);
        extract_data(FC, ToCompute);
        if (verbose) {
            verboseOutput() << "Cone not pointed. Restarting computation." << endl;
        }
        FC = Full_Cone<IntegerFC>(Matrix<IntegerFC>(1));  // to kill the old FC (almost)
        pass_to_pointed_quotient();
        compute_full_cone_inner<IntegerFC>(ToCompute);
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_integer_hull() {
    if (isComputed(ConeProperty::IntegerHull))
        return;

    if (verbose) {
        verboseOutput() << "Computing integer hull" << endl;
    }

    compute(ConeProperty::SupportHyperplanes, ConeProperty::MaximalSubspace);

    Matrix<Integer> IntHullGen;
    vector<Integer> IntHullDehom;
    ConeProperties IntHullCompute;
    IntHullCompute.set(ConeProperty::SupportHyperplanes);
    IntHullCompute.set(ConeProperty::AffineDim);
    IntHullCompute.set(ConeProperty::RecessionRank);
    bool IntHullComputable = true;
    // size_t nr_extr = 0;
    if (inhomogeneous) {
        if ((!using_renf<Integer>() && !isComputed(ConeProperty::HilbertBasis)) ||
            (using_renf<Integer>() && !isComputed(ConeProperty::ModuleGenerators)))
            IntHullComputable = false;
        IntHullDehom = Dehomogenization;
        if (using_renf<Integer>())
            IntHullGen = ModuleGenerators;
        else {
            IntHullGen = ExtremeRaysRecCone;  // not defined in case of renf_elem_class
            IntHullGen.append(ModuleGenerators);
        }
    }
    else {
        if (!isComputed(ConeProperty::Deg1Elements))
            IntHullComputable = false;
        IntHullGen = Deg1Elements;
        IntHullDehom = Grading;
    }

    if (!IntHullComputable) {
        errorOutput() << "Integer hull not computable: no integer points available." << endl;
        throw NotComputableException(IntHullCompute);
    }

    Matrix<Integer> RationalExtremeRays_Integral(0,dim); // we collect the integral rational exteme rays
    bool all_extreme_rays_integral = false;
    if(!using_renf<Integer>()){
        all_extreme_rays_integral = true;
        for(size_t i = 0; i < ExtremeRays.nr_of_rows(); ++ i){
            Integer test = v_scalar_product(IntHullDehom, ExtremeRays[i]);
            if(test == 0 || test == 1)
                RationalExtremeRays_Integral.append(ExtremeRays[i]);
            else
                all_extreme_rays_integral = false;
        }
    }

    if(all_extreme_rays_integral){
        if(verbose)
            verboseOutput() << "Polyhedron has integral vertices ==> is its own intehger hull" << endl;
        InputMap<Integer> PrecomputedForIntegerHull;
        PrecomputedForIntegerHull[Type::support_hyperplanes] = SupportHyperplanes;
        PrecomputedForIntegerHull[Type::extreme_rays] = ExtremeRays;
        PrecomputedForIntegerHull[Type::generated_lattice] = BasisChange.getEmbeddingMatrix();
        PrecomputedForIntegerHull[Type::maximal_subspace] = BasisMaxSubspace;
        PrecomputedForIntegerHull[Type::dehomogenization] = IntHullDehom;
        IntHullCone = new Cone<Integer>(PrecomputedForIntegerHull);
        IntHullCone->compute(IntHullCompute);
        setComputed(ConeProperty::IntegerHull);
        if (verbose)
            verboseOutput() << "Integer hull finished" << endl;
        return;
    }

    if (IntHullGen.nr_of_rows() == 0) {
        IntHullGen.append(vector<Integer>(dim, 0));  // we need a non-empty input matrix
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    IntHullNorm.resize(0); // disable it since the cone may change in an interactive environmemt

    if (inhomogeneous && HilbertBasis.nr_of_rows() != 0) { // not needed in the case of polytopes
        assert(isComputed(ConeProperty::SupportHyperplanes));
        IntHullNorm = SupportHyperplanes.find_inner_point();
    }

    if (IntHullCone != NULL)
        delete IntHullCone;

    if (!using_renf<Integer>())
        IntHullCone = new Cone<Integer>(InputType::cone_and_lattice, IntHullGen, Type::subspace, BasisMaxSubspace);
    else
        IntHullCone = new Cone<Integer>(InputType::cone, IntHullGen, Type::subspace, BasisMaxSubspace);
    /* if (nr_extr != 0)  // we suppress the ordering in full_cone only if we have found few extreme rays
        IntHullCompute.set(ConeProperty::KeepOrder);*/

    IntHullCone->setRenf(RenfSharedPtr);

    IntHullCone->inhomogeneous = true;  // inhomogeneous;
    IntHullCone->is_inthull_cone = true;
    // IntHullCone->HilbertBasis = HilbertBasis;
    IntHullCone->IntHullNorm = IntHullNorm;
    // IntHullCone->ModuleGenerators = ModuleGenerators;
    if(!using_renf<Integer>())
        IntHullCone->RationalExtremeRays = RationalExtremeRays_Integral;
    IntHullCone->RationalBasisMaxSubspace = BasisMaxSubspace;
    // IntHullCone->setComputed(ConeProperty::HilbertBasis);
    // IntHullCone->setComputed(ConeProperty::ModuleGenerators);
    if (inhomogeneous)
        IntHullCone->Dehomogenization = Dehomogenization;
    else
        IntHullCone->Dehomogenization = Grading;
    IntHullCone->verbose = verbose;
    IntHullCompute.set(ConeProperty::FullConeDynamic);
    try {
        IntHullCone->compute(IntHullCompute);
        if (IntHullCone->isComputed(ConeProperty::SupportHyperplanes))
            setComputed(ConeProperty::IntegerHull);
        if (verbose) {
            verboseOutput() << "Integer hull finished" << endl;
        }
    } catch (const NotComputableException&) {
        errorOutput() << "Error in computation of integer hull" << endl;
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
ConeProperties Cone<Integer>::compute(ConeProperty::Enum cp) {
    if (isComputed(cp))
        return ConeProperties();
    return compute(ConeProperties(cp));
}

template <typename Integer>
ConeProperties Cone<Integer>::compute(ConeProperty::Enum cp1, ConeProperty::Enum cp2) {
    if (isComputed(cp1) && isComputed(cp2))
        return ConeProperties();
    return compute(ConeProperties(cp1, cp2));
}

template <typename Integer>
ConeProperties Cone<Integer>::compute(ConeProperty::Enum cp1, ConeProperty::Enum cp2, ConeProperty::Enum cp3) {
    if (isComputed(cp1) && isComputed(cp2) && isComputed(cp3))
        return ConeProperties();
    return compute(ConeProperties(cp1, cp2, cp3));
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::set_implicit_dual_mode(ConeProperties& ToCompute) {
    if (ToCompute.test(ConeProperty::DualMode) || ToCompute.test(ConeProperty::PrimalMode) ||
        ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid) || ToCompute.test(ConeProperty::Approximate) ||
        ToCompute.test(ConeProperty::Projection) || nr_cone_gen > 0 || Inequalities.nr_of_rows() > 2 * dim ||
        Inequalities.nr_of_rows() <= BasisChangePointed.getRank() + 50 / (BasisChangePointed.getRank() + 1))
        return;
    if (ToCompute.test(ConeProperty::HilbertBasis))
        ToCompute.set(ConeProperty::DualMode);
    if (ToCompute.test(ConeProperty::Deg1Elements) &&
        !(ToCompute.test(ConeProperty::HilbertSeries) || ToCompute.test(ConeProperty::Multiplicity)))
        ToCompute.set(ConeProperty::DualMode);
    return;
}

// If this function is called, either no type of automorphisms has been computed
// or the computed one is different than the one asked for.
// So we can reset all of them.
template <typename Integer>
void Cone<Integer>::prepare_automorphisms(const ConeProperties& ToCompute) {
    ConeProperties ToCompute_Auto = ToCompute.intersection_with(all_automorphisms());
    if (ToCompute_Auto.none())
        return;
    is_Computed.reset(all_automorphisms());
}

// Similarly for triangulations
template <typename Integer>
void Cone<Integer>::prepare_refined_triangulation(const ConeProperties& ToCompute) {
    ConeProperties ToCompute_Tri = ToCompute.intersection_with(all_triangulations());
    if (ToCompute_Tri.none())
        return;
    is_Computed.reset(all_triangulations());
}

template <typename Integer>
void Cone<Integer>::handle_dynamic(const ConeProperties& ToCompute) {
    if (ToCompute.test(ConeProperty::Dynamic))
        keep_convex_hull_data = true;
    if (ToCompute.test(ConeProperty::Static))
        keep_convex_hull_data = false;

    AddGenerators.resize(0, dim);
    AddInequalities.resize(0, dim);
}

#ifdef NMZ_EXTENDED_TESTS

extern long SimplexParallelEvaluationBound;

template <typename Integer>
void Cone<Integer>::set_extended_tests(ConeProperties& ToCompute) {
    if (ToCompute.test(ConeProperty::TestArithOverflowFullCone))
        test_arith_overflow_full_cone = true;
    if (ToCompute.test(ConeProperty::TestArithOverflowDualMode))
        test_arith_overflow_dual_mode = true;
    if (ToCompute.test(ConeProperty::TestArithOverflowDescent))
        test_arith_overflow_descent = true;
    if (ToCompute.test(ConeProperty::TestArithOverflowProjAndLift))
        test_arith_overflow_proj_and_lift = true;
    if (ToCompute.test(ConeProperty::TestSmallPyramids))
        test_small_pyramids = true;
    if (ToCompute.test(ConeProperty::TestLargePyramids)) {
        test_large_pyramids = true;
        test_small_pyramids = true;
    }
    if (ToCompute.test(ConeProperty::TestLinearAlgebraGMP)) {
        test_linear_algebra_GMP = true;
        if (isComputed(ConeProperty::OriginalMonoidGenerators)) {
            Matrix<MachineInteger> GenLL;
            convert(GenLL, Generators);
            if (GenLL.rank() == dim) {
                MachineInteger test = GenLL.full_rank_index();  // to test full_rank_index with "overflow"
                assert(convertTo<Integer>(test) == internal_index);
            }
            MachineInteger test_rank;
            test_rank = GenLL.row_echelon_reduce();  // same reason as above
            assert(convertTo<Integer>(test_rank) == Generators.rank());
        }
    }
    if (ToCompute.test(ConeProperty::TestSimplexParallel)) {
        test_simplex_parallel = true;
        ToCompute.set(ConeProperty::NoSubdivision);
        SimplexParallelEvaluationBound = 0;
    }

    if (ToCompute.test(ConeProperty::TestLibNormaliz)) {
        run_additional_tests_libnormaliz();
    }
}
#endif

//---------------------------------------------------------------------------

#ifdef NMZ_DEBUG
#define LEAVE_CONE if(verbose) cout << "Leaving cone level " << --cone_recursion_level << endl; \
if(verbose) cout << "+++++++++++++++++++++++++++++++++++++++++++++" << endl;
#else
#define LEAVE_CONE
#endif

template <typename Integer>
ConeProperties Cone<Integer>::compute(ConeProperties ToCompute) {


#ifdef NMZ_DEBUG
    if(verbose){
        cout << "+++++++++++++++++++++++++++++++++++++++++++++" << endl;
        cout << "Level " << cone_recursion_level << endl;
        cout << "is_Computed " << is_Computed << endl;
        cout << "ToCompute   " << ToCompute << endl;
        cout << "+++++++++++++++++++++++++++++++++++++++++++++" << endl;
            cone_recursion_level++;
    }
#endif

    size_t nr_computed_at_start = is_Computed.count();

    handle_dynamic(ToCompute);

    set_parallelization();
    nmz_interrupted = 0;

    ToCompute.reset(is_Computed);
    if (ToCompute.none()) {
        LEAVE_CONE return ConeProperties();
    }

    // We want to make sure that the exckuded faces are shown in the output/can be  returned
    if (!isComputed(ConeProperty::ExcludedFaces) && ExcludedFaces.nr_of_rows() > 0)
        ToCompute.set(ConeProperty::ExcludedFaces);

#ifdef NMZ_EXTENDED_TESTS
    set_extended_tests(ToCompute);
#endif

    if (general_no_grading_denom || inhomogeneous)
        ToCompute.set(ConeProperty::NoGradingDenom);

    if (ToCompute.test(ConeProperty::GradingIsPositive)) {
        if (Grading.size() == 0)
            throw BadInputException("No grading declared that could be positive.");
        else
            setComputed(ConeProperty::Grading);
    }

    if (ToCompute.test(ConeProperty::NoGradingDenom)) {
        GradingDenom = 1;
        setComputed(ConeProperty::GradingDenom);
    }

    if (ToCompute.test(ConeProperty::NoPeriodBound)) {
        HSeries.set_period_bounded(false);
        IntData.getWeightedEhrhartSeries().first.set_period_bounded(false);
    }

#ifndef NMZ_COCOA
    if (ToCompute.test(ConeProperty::VirtualMultiplicity) || ToCompute.test(ConeProperty::Integral) ||
        ToCompute.test(ConeProperty::WeightedEhrhartSeries))
        throw BadInputException("Integral, VirtualMultiplicity, WeightedEhrhartSeries only computable with CoCoALib");
#endif

#ifndef NMZ_NAUTY
    if (ToCompute.intersection_with(all_automorphisms()).any())
        throw BadInputException("automorphism groups only computable with nauty");
    if (ToCompute.test(ConeProperty::ExploitIsosMult))
        throw BadInputException("ExploitIsosMult only computable with nauty");
#endif

    // default_mode=ToCompute.test(ConeProperty::DefaultMode);

    if (ToCompute.test(ConeProperty::BigInt)) {
        if (!using_GMP<Integer>())
            throw BadInputException("BigInt can only be set for cones of Integer type GMP");
        change_integer_type = false;
    }

    // we don't want a different order of the generators if an order sensitive goal
    // has already been computed
    if (isComputed(ConeProperty::BasicTriangulation) || isComputed(ConeProperty::PullingTriangulation)) {
        Generators = BasicTriangulation.second;
        ToCompute.set(ConeProperty::KeepOrder);
        is_Computed.reset(ConeProperty::ExtremeRays);  // we may have lost ExtremeRaysIndicator
    }

    if ((ToCompute.test(ConeProperty::PullingTriangulation) || ToCompute.test(ConeProperty::PlacingTriangulation)) &&
        !isComputed(ConeProperty::Generators))
        throw BadInputException("Placing/pulling tiangulation only computable with generator input");

    if (ToCompute.test(ConeProperty::IsEmptySemiOpen) && ExcludedFaces.nr_of_rows() == 0)
        throw BadInputException("IsEmptySemiOpen can only be computed with excluded faces");

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    if (BasisMaxSubspace.nr_of_rows() > 0 && !isComputed(ConeProperty::MaximalSubspace)) {
        BasisMaxSubspace = Matrix<Integer>(0, dim);
        if (ToCompute.test(ConeProperty::FullConeDynamic))  // if integer hull is to be computed
            compute(ConeProperty::MaximalSubspace, ConeProperty::FullConeDynamic);
        else
            compute(ConeProperty::MaximalSubspace);
    }

    /*
    // must distiguish it from being set through DefaultMode; -- DONE VIA FDefaultMode

    if(ToCompute.test(ConeProperty::HilbertSeries) || ToCompute.test(ConeProperty::HSOP)
               || ToCompute.test(ConeProperty::EhrhartSeries) || ToCompute.test(ConeProperty::HilbertQuasiPolynomial)
               || ToCompute.test(ConeProperty::EhrhartQuasiPolynomial))
        ToCompute.set(ConeProperty::ExplicitHilbertSeries);
    */

    // to control the computation of rational solutions in the inhomogeneous case
    if (ToCompute.test(ConeProperty::DualMode) &&
        !(ToCompute.test(ConeProperty::HilbertBasis) || ToCompute.test(ConeProperty::Deg1Elements) ||
          ToCompute.test(ConeProperty::ModuleGenerators) || ToCompute.test(ConeProperty::LatticePoints))) {
        ToCompute.set(ConeProperty::NakedDual);
    }
    // to control the computation of rational solutions in the inhomogeneous case

    if (using_renf<Integer>())
        ToCompute.check_Q_permissible(false);  // before implications!

    ToCompute.check_conflicting_variants();
    ToCompute.set_preconditions(inhomogeneous, using_renf<Integer>());

    if (using_renf<Integer>())
        ToCompute.check_Q_permissible(true);  // after implications!

    if (ToCompute.test(ConeProperty::Sublattice) && !isComputed(ConeProperty::Generators))
        ToCompute.set(ConeProperty::ExtremeRays, ConeProperty::SupportHyperplanes);

    ToCompute.check_sanity(inhomogeneous);
    if (inhomogeneous) {
        if (Grading.size() == 0) {
            if (ToCompute.test(ConeProperty::DefaultMode)) {
                ToCompute.reset(ConeProperty::HilbertSeries);
            }
            ToCompute.reset(ConeProperty::NoGradingDenom);
        }
    }

    prepare_refined_triangulation(ToCompute);
    prepare_automorphisms(ToCompute);

    // ToCompute.set_default_goals(inhomogeneous,using_renf<renf_elem_class>());
    ToCompute.check_sanity(inhomogeneous);

    ToCompute.reset(is_Computed);
    if (ToCompute.none()) {
        LEAVE_CONE return ConeProperties();
    }
    if (!isComputed(ConeProperty::OriginalMonoidGenerators)) {
        if (ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid)) {
            errorOutput() << "ERROR: Module generators over original monoid only computable if original monoid is defined!"
                          << endl;
            throw NotComputableException(ConeProperty::ModuleGeneratorsOverOriginalMonoid);
        }
        if (ToCompute.test(ConeProperty::IsIntegrallyClosed)) {
            errorOutput() << "ERROR: Original monoid is not defined, cannot check it for being integrally closed." << endl;
            throw NotComputableException(ConeProperty::IsIntegrallyClosed);
        }
    }

    // to protect against intermediate computaions of generators in interactive use
    if (ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid) || ToCompute.test(ConeProperty::IsIntegrallyClosed)) {
        Generators = InputGenerators;
        is_Computed.reset(ConeProperty::ExtremeRays);
    }

    //***********************************************
    // Checking and setting of computation goals done
    //***********************************************

    /*
    cout << "ToCompute   " << ToCompute << endl;
    cout << "is_Computed "<< is_Computed << endl;
    cout << "inhomogen   " << inhomogeneous << endl;
    */

    compute_ambient_automorphisms(ToCompute);
    compute_input_automorphisms(ToCompute);
    ToCompute.reset(is_Computed);
    if (ToCompute.none()) {
        LEAVE_CONE return ConeProperties();
    }

    if (ToCompute.test(ConeProperty::PullingTriangulation))
        compute_refined_triangulation(ToCompute);

    ToCompute.reset(is_Computed);
    if (ToCompute.none()) {
        LEAVE_CONE return ConeProperties();
    }

    if (conversion_done)
        compute_generators(ToCompute);

    ToCompute.reset(is_Computed);
    if (ToCompute.none()) {       // IMPORTANT: do not use goals() at this point because it would prevent
        LEAVE_CONE return ConeProperties();  // HSOP if HSOP is applied to an already computed Hilbert series
    }                             // Alternatively one could do complete_HilbertSeries_comp(ToCompute)
                                  // at the very beginning of this function

    /*if(ToCompute.test(ConeProperty::IsEmptySemiOpen) && !isComputed(ConeProperty::IsEmptySemiOpen)){
        compute_generators(ToCompute);
        ConeProperties ToComputeFirst;
        ToComputeFirst.set(ConeProperty::Generators);
        ToComputeFirst.set(ConeProperty::SupportHyperplanes);
        ToComputeFirst.set(ConeProperty::ExtremeRays);
        ToComputeFirst.set(ConeProperty::IsEmptySemiOpen);
        compute_full_cone(ToComputeFirst);
        ToCompute.reset(is_Computed);
    }*/

    check_integrally_closed(ToCompute);  // check cheap necessary conditions

    if (rees_primary && ToCompute.test(ConeProperty::Multiplicity))  // for backward compatibility
        ToCompute.set(ConeProperty::ReesPrimaryMultiplicity);

    try_multiplicity_of_para(ToCompute);
    ToCompute.reset(is_Computed);

    try_signed_dec(ToCompute);
    ToCompute.reset(is_Computed);

    if (ToCompute.test(ConeProperty::Integral) || ToCompute.test(ConeProperty::VirtualMultiplicity))
        ToCompute.set(ConeProperty::BasicTriangulation);  // Hasn't been done by signed dec.

    try_multiplicity_by_descent(ToCompute);
    ToCompute.reset(is_Computed);

    try_symmetrization(ToCompute);
    ToCompute.reset(is_Computed);

    complete_HilbertSeries_comp(ToCompute);

    /*    Disabled since the extra coordinate transformation may need much memory
     * for large examples. Perhaps not worth the savintg in time
    compute_rational_data(ToCompute); // computes multiplicity
    ToCompute.reset(is_Computed);    // if change to smaller lattice is possible
    if (ToCompute.goals().none()) {
        LEAVE_CONE return ConeProperties();
    }
    */

    complete_sublattice_comp(ToCompute);
    if (ToCompute.goals().none()) {
        LEAVE_CONE return ConeProperties();
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    compute_projection(ToCompute);

    INTERRUPT_COMPUTATION_BY_EXCEPTION

#ifdef ENFNORMALIZ
    if (using_renf<Integer>())
        prepare_volume_computation(ToCompute);
#endif

    treat_polytope_as_being_hom_defined(ToCompute);  // if necessary

    ToCompute.reset(is_Computed);  // already computed

    complete_HilbertSeries_comp(ToCompute);

    complete_sublattice_comp(ToCompute);
    if (ToCompute.goals().none()) {
        LEAVE_CONE return ConeProperties();
    }

    if (!using_renf<Integer>())                      // lattice points in algebraic polytopes will be computed later
        try_approximation_or_projection(ToCompute);  // by compute_lattice_points_in_polytope

    ToCompute.reset(is_Computed);
    if (ToCompute.goals().none()) {
        LEAVE_CONE return ConeProperties();
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    set_implicit_dual_mode(ToCompute);

    if (ToCompute.test(ConeProperty::DualMode)) {
        compute_dual(ToCompute);
    }

    if (ToCompute.test(ConeProperty::WitnessNotIntegrallyClosed)) {
        find_witness(ToCompute);
    }

    ToCompute.reset(is_Computed);
    complete_HilbertSeries_comp(ToCompute);
    complete_sublattice_comp(ToCompute);
    if (ToCompute.goals().none()) {
        LEAVE_CONE return ConeProperties();
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    /* preparation: get generators if necessary */

    // cout << "FFFF " << ToCompute.full_cone_goals(using_renf<Integer>()) << endl;

    if (ToCompute.full_cone_goals(using_renf<Integer>()).any()) {
        compute_generators(ToCompute);
        if (!isComputed(ConeProperty::Generators)) {
            throw FatalException("Could not get Generators.");
        }
    }
    ToCompute.reset(is_Computed);

    try_multiplicity_of_para(ToCompute);
    ToCompute.reset(is_Computed);

    /* cout << "TTTT " << ToCompute.full_cone_goals(using_renf<Integer>()) << endl;*/
    /* cout << "TTTT IIIII  " << ToCompute.full_cone_goals() << endl;*/

    if (rees_primary && (ToCompute.test(ConeProperty::ReesPrimaryMultiplicity) || ToCompute.test(ConeProperty::HilbertSeries) ||
                         ToCompute.test(ConeProperty::DefaultMode))) {
        ReesPrimaryMultiplicity = compute_primary_multiplicity();
        setComputed(ConeProperty::ReesPrimaryMultiplicity);
    }

    ToCompute.reset(is_Computed);
    complete_HilbertSeries_comp(ToCompute);
    complete_sublattice_comp(ToCompute);
    if (ToCompute.goals().none()) {
        LEAVE_CONE return ConeProperties();
    }

    if (!using_renf<Integer>())
        try_Hilbert_Series_from_lattice_points(ToCompute);
    ToCompute.reset(is_Computed);
    complete_HilbertSeries_comp(ToCompute);
    complete_sublattice_comp(ToCompute);
    if (ToCompute.goals().none()) {
        LEAVE_CONE return ConeProperties();
    }

    // the actual computation

    if (isComputed(ConeProperty::SupportHyperplanes) && using_renf<Integer>())
        ToCompute.reset(ConeProperty::DefaultMode);  // in this case Default = SupportHyperplanes

    // cout << "UUUU " << ToCompute.full_cone_goals(using_renf<Integer>()) << endl;
    /* cout << "UUUU All  " << ToCompute << endl;
    cout << "UUUU  IIIII  " << ToCompute.full_cone_goals() << endl;*/

    // the computation of the full cone
    if (ToCompute.full_cone_goals(using_renf<Integer>()).any()) {
        compute_full_cone(ToCompute);
    }

    // cout << " VVVV " << ToCompute.full_cone_goals() << endl;

    if (ToCompute.test(ConeProperty::WitnessNotIntegrallyClosed)) {
        find_witness(ToCompute);
    }

    if (using_renf<Integer>())  // done here because not computed in full_cone
        compute_lattice_points_in_polytope(ToCompute);
    ToCompute.reset(is_Computed);  // already computed

    if (precomputed_extreme_rays && inhomogeneous)
        compute_affine_dim_and_recession_rank();

    compute_combinatorial_automorphisms(ToCompute);
    compute_euclidean_automorphisms(ToCompute);

    make_face_lattice(ToCompute);

    compute_volume(ToCompute);

    check_Gorenstein(ToCompute);

    if (ToCompute.test(ConeProperty::IntegerHull)) {
        compute_integer_hull();
    }

    compute_refined_triangulation(ToCompute);
    make_StanleyDec_export(ToCompute);

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    complete_HilbertSeries_comp(ToCompute);
    complete_sublattice_comp(ToCompute);

    compute_vertices_float(ToCompute);
    compute_supp_hyps_float(ToCompute);
    compute_extreme_rays_float(ToCompute);

    if (ToCompute.test(ConeProperty::WeightedEhrhartSeries))
        compute_weighted_Ehrhart(ToCompute);
    ToCompute.reset(is_Computed);

    if (ToCompute.test(ConeProperty::Integral))
        compute_integral(ToCompute);
    ToCompute.reset(is_Computed);

    if (ToCompute.test(ConeProperty::VirtualMultiplicity))
        compute_virt_mult(ToCompute);
    ToCompute.reset(is_Computed);

    // back up solution for excluded faces which are not set if symmetrization has been used
    if (!isComputed(ConeProperty::ExcludedFaces) && ExcludedFaces.nr_of_rows() > 0) {
        ExcludedFaces.sort_lex();
        setComputed(ConeProperty::ExcludedFaces);
    }

    /* check if everything is computed */
    ToCompute.reset(is_Computed);  // remove what is now computed
    if (ToCompute.test(ConeProperty::Deg1Elements) && isComputed(ConeProperty::Grading)) {
        // can happen when we were looking for a witness earlier
        if (nr_computed_at_start == is_Computed.count()) {  // Prevention of infinte loop
            throw FatalException("FATAL: Compute without effect would be repeated. Inform the authors !");
        }
        compute(ToCompute);
    }

    if (!ToCompute.test(ConeProperty::DefaultMode) && ToCompute.goals().any()) {
        throw NotComputableException(ToCompute.goals());
    }

    ToCompute.reset_compute_options();

    LEAVE_CONE return ToCompute;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::check_vanishing_of_grading_and_dehom() {
    if (Grading.size() > 0) {
        vector<Integer> test = BasisMaxSubspace.MxV(Grading);
        if (test != vector<Integer>(test.size())) {
            throw BadInputException("Grading does not vanish on maximal subspace.");
        }
    }
    if (Dehomogenization.size() > 0) {
        vector<Integer> test = BasisMaxSubspace.MxV(Dehomogenization);
        if (test != vector<Integer>(test.size())) {
            assert(false);
            throw BadInputException("Dehomogenization does not vanish on maximal subspace.");
        }
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_generators(ConeProperties& ToCompute) {
    // create Generators from Inequalities

    if (!isComputed(ConeProperty::Generators) && (Inequalities.nr_of_rows() != 0 || inhomogeneous)) {
        if (verbose) {
            verboseOutput() << "Computing extreme rays as support hyperplanes of the dual cone:" << endl;
        }
        if (change_integer_type) {
            try {
                compute_generators_inner<MachineInteger>(ToCompute);
            } catch (const ArithmeticException& e) {
                if (verbose) {
                    verboseOutput() << e.what() << endl;
                    verboseOutput() << "Restarting with a bigger type." << endl;
                }
                compute_generators_inner<Integer>(ToCompute);
            }
        }
        else {
            compute_generators_inner<Integer>(ToCompute);
        }
    }
    /*for(size_t i=0;i<Generators.nr_of_rows();++i){
        for(size_t j=0;j<SupportHyperplanes.nr_of_rows();++j)
            cout << v_scalar_product(Generators[i],SupportHyperplanes[j]) << endl;
    }*/
    assert(isComputed(ConeProperty::Generators));
}

//---------------------------------------------------------------------------

// computes the basis change to the pointed quotient
template <typename Integer>
void Cone<Integer>::pass_to_pointed_quotient() {
    if (isComputed(ConeProperty::MaximalSubspace))
        return;

    BasisChangePointed = BasisChange;
    Matrix<Integer> DualGen;  // must protect Inequalities/SupportHyperplanes!
    if(isComputed(ConeProperty::SupportHyperplanes))
        DualGen = SupportHyperplanes;
    else
        DualGen = Inequalities;

    BasisChangePointed.compose_with_passage_to_quotient(BasisMaxSubspace, DualGen);

    check_vanishing_of_grading_and_dehom();
    setComputed(ConeProperty::MaximalSubspace);

    if (!isComputed(ConeProperty::IsPointed)) {
        pointed = (BasisMaxSubspace.nr_of_rows() == 0);
        setComputed(ConeProperty::IsPointed);
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
template <typename IntegerFC>
void Cone<Integer>::compute_generators_inner(ConeProperties& ToCompute) {
#ifdef NMZ_EXTENDED_TESTS
    if (!using_GMP<IntegerFC>() && !using_renf<IntegerFC>() && test_arith_overflow_full_cone)
        throw ArithmeticException(0);
#endif

    pass_to_pointed_quotient();

    // restrict the supphyps to efficient sublattice and push to quotient mod subspace
    Matrix<IntegerFC> Dual_Gen_Pointed;
    BasisChangePointed.convert_to_sublattice_dual(Dual_Gen_Pointed, Inequalities);
    Full_Cone<IntegerFC> Dual_Cone(Dual_Gen_Pointed);
    Dual_Cone.verbose = verbose;
    Dual_Cone.renf_degree = renf_degree;
    Dual_Cone.do_extreme_rays = true;  // we try to find them, need not exist
    if (ToCompute.test(ConeProperty::KeepOrder) && dual_original_generators)
        Dual_Cone.keep_order = true;

    if ((keep_convex_hull_data || conversion_done) && ConvHullData.SLR.equal(BasisChangePointed) &&
        ConvHullData.nr_threads == omp_get_max_threads() && ConvHullData.Generators.nr_of_rows() > 0) {
        conversion_done = false;
        Dual_Cone.keep_order = true;
        Dual_Cone.restore_previous_computation(ConvHullData, false);  // false=dual
    }

    Dual_Cone.keep_convex_hull_data = keep_convex_hull_data;
    Dual_Cone.do_pointed = true;  // Dual may very well be non-pointed.
                                  // In this case the support hyperplanes
                                  // can contain duplictes. There are erased in full_cone.cpp
    try {                         // most likely superfluous, but doesn't harm
        Dual_Cone.dualize_cone();
    } catch (const NonpointedException&) {
    };  // we don't mind if the dual cone is not pointed

    // cout << "GGGGG " << Dual_Cone.is_Computed << endl;
    extract_data_dual(Dual_Cone, ToCompute);
}

//---------------------------------------------------------------------------

template <typename Integer>
template <typename IntegerFC>
void Cone<Integer>::extract_data_dual(Full_Cone<IntegerFC>& Dual_Cone, ConeProperties& ToCompute) {
    if (Dual_Cone.isComputed(ConeProperty::SupportHyperplanes)) {
        if (keep_convex_hull_data) {
            extract_convex_hull_data(Dual_Cone, false);  // false means: dual
        }
        // get the extreme rays of the primal cone
        // BasisChangePointed.convert_from_sublattice(Generators,
        //                 Dual_Cone.getSupportHyperplanes());
        extract_supphyps(Dual_Cone, Generators, false);  // false means: no dualization
        ExtremeRaysIndicator.resize(0);
        setComputed(ConeProperty::Generators);

        // get minmal set of support_hyperplanes if possible
        if (Dual_Cone.isComputed(ConeProperty::ExtremeRays)) {
            Matrix<IntegerFC> Supp_Hyp = Dual_Cone.getGenerators().submatrix(Dual_Cone.getExtremeRays());
            BasisChangePointed.convert_from_sublattice_dual(SupportHyperplanes, Supp_Hyp);
            if (using_renf<Integer>())
                SupportHyperplanes.standardize_rows();
            norm_dehomogenization(BasisChangePointed.getRank());
            SupportHyperplanes.sort_lex();
            setComputed(ConeProperty::SupportHyperplanes);
            Inequalities = SupportHyperplanes;
            addition_constraints_allowed = true;
        }

        // now the final transformations
        // only necessary if the basis changes computed so far do not make the cone full-dimensional
        // the latter is equaivalent to the dual cone bot being pointed
        if (!(Dual_Cone.isComputed(ConeProperty::IsPointed) && Dual_Cone.isPointed())) {
            // first to full-dimensional pointed
            Matrix<Integer> Help;
            Help = BasisChangePointed.to_sublattice(Generators);  // sublattice of the primal space
            Sublattice_Representation<Integer> PointedHelp(Help, true);
            BasisChangePointed.compose(PointedHelp);
            // second to efficient sublattice
            if (BasisMaxSubspace.nr_of_rows() == 0) {  // primal cone is pointed and we can copy
                BasisChange = BasisChangePointed;
            }
            else {
                Help = BasisChange.to_sublattice(Generators);
                Help.append(BasisChange.to_sublattice(BasisMaxSubspace));
                Sublattice_Representation<Integer> EmbHelp(Help, true);  // sublattice of the primal space
                compose_basis_change(EmbHelp);
            }
        }
        setComputed(ConeProperty::Sublattice);  // will not be changed anymore

        checkGrading(!ToCompute.test(ConeProperty::NoGradingDenom));
        // compute grading, so that it is also known if nothing else is done afterwards
        // it is only done if the denominator is 1, like in full_cone.cpp
        if (!isComputed(ConeProperty::Grading) && !inhomogeneous && !using_renf<Integer>()) {
            // Generators = ExtremeRays
            // we only do it if the cone is pointed
            vector<Integer> lf = BasisChangePointed.to_sublattice(Generators).find_linear_form();
            if (lf.size() == BasisChange.getRank()) {
                vector<Integer> test_lf = BasisChange.from_sublattice_dual(lf);
                if (Generators.nr_of_rows() == 0 || v_scalar_product(Generators[0], test_lf) == 1) {
                    setGrading(test_lf);
                    deg1_extreme_rays = true;
                    setComputed(ConeProperty::IsDeg1ExtremeRays);
                }
            }
        }
        setWeights();
        set_extreme_rays(vector<bool>(Generators.nr_of_rows(), true));
        addition_generators_allowed = true;
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_dual(ConeProperties& ToCompute) {
    ToCompute.reset(is_Computed);
    if (ToCompute.goals().none() || !(ToCompute.test(ConeProperty::Deg1Elements) || ToCompute.test(ConeProperty::HilbertBasis))) {
        return;
    }

    if (change_integer_type) {
        try {
            compute_dual_inner<MachineInteger>(ToCompute);
        } catch (const ArithmeticException& e) {
            if (verbose) {
                verboseOutput() << e.what() << endl;
                verboseOutput() << "Restarting with a bigger type." << endl;
            }
            change_integer_type = false;
        }
    }

    if (!change_integer_type) {
        if (!using_GMP<Integer>() && !ToCompute.test(ConeProperty::DefaultMode)) {
            compute_dual_inner<Integer>(ToCompute);
        }
        else {
            try {
                compute_dual_inner<Integer>(ToCompute);
            } catch (const ArithmeticException& e) {  // the nonly reason for failure is an overflow in a degree computation
                if (verbose) {                        // so we can relax in default mode
                    verboseOutput() << e.what() << endl;
                    verboseOutput() << "Reducing computation goals." << endl;
                }
                ToCompute.reset(ConeProperty::HilbertBasis);
                ToCompute.reset(ConeProperty::HilbertSeries);
                // we cannot do more here
            }
        }
    }

    ToCompute.reset(ConeProperty::DualMode);
    ToCompute.reset(is_Computed);
    // if (ToCompute.test(ConeProperty::DefaultMode) && ToCompute.goals().none()) {
    //    ToCompute.reset(ConeProperty::DefaultMode);
    // }
}

//---------------------------------------------------------------------------

template <typename Integer>
vector<Sublattice_Representation<Integer> > MakeSubAndQuot(const Matrix<Integer>& Gen, const Matrix<Integer>& Ker) {
    vector<Sublattice_Representation<Integer> > Result;
    Matrix<Integer> Help = Gen;
    Help.append(Ker);
    Sublattice_Representation<Integer> Sub(Help, true);
    Sublattice_Representation<Integer> Quot = Sub;
    if (Ker.nr_of_rows() > 0) {
        Matrix<Integer> HelpQuot = Sub.to_sublattice(Ker).kernel(false);  // kernel here to be interpreted as subspace of the dual
                                                                          // namely the linear forms vanishing on Ker
        Sublattice_Representation<Integer> SubToQuot(HelpQuot, true);     // sublattice of the dual
        Quot.compose_dual(SubToQuot);
    }
    Result.push_back(Sub);
    Result.push_back(Quot);

    return Result;
}

//---------------------------------------------------------------------------

template <typename Integer>
template <typename IntegerFC>
void Cone<Integer>::compute_dual_inner(ConeProperties& ToCompute) {
#ifdef NMZ_EXTENDED_TESTS
    if (!using_GMP<IntegerFC>() && test_arith_overflow_dual_mode)
        throw ArithmeticException(0);
#endif

    bool do_only_Deg1_Elements = ToCompute.test(ConeProperty::Deg1Elements) && !ToCompute.test(ConeProperty::HilbertBasis);

    if (isComputed(ConeProperty::Generators) && Inequalities.nr_of_rows() == 0) {
        if (verbose) {
            verboseOutput() << "Computing support hyperplanes for the dual mode:" << endl;
        }
        ConeProperties Dualize;
        Dualize.set(ConeProperty::SupportHyperplanes);
        Dualize.set(ConeProperty::ExtremeRays);
        if (ToCompute.test(ConeProperty::KeepOrder) && dual_original_generators)
            Dualize.set(ConeProperty::KeepOrder);
        compute(Dualize);
    }

    bool do_extreme_rays_first = false;
    if (!isComputed(ConeProperty::ExtremeRays)) {
        if (do_only_Deg1_Elements && Grading.size() == 0)
            do_extreme_rays_first = true;
        else if ((do_only_Deg1_Elements || inhomogeneous) &&
                 (ToCompute.test(ConeProperty::NakedDual) || ToCompute.test(ConeProperty::ExtremeRays) ||
                  ToCompute.test(ConeProperty::SupportHyperplanes) || ToCompute.test(ConeProperty::Sublattice)))
            do_extreme_rays_first = true;
    }

    if (do_extreme_rays_first) {
        if (verbose) {
            verboseOutput() << "Computing extreme rays for the dual mode:" << endl;
        }
        compute_generators(ToCompute);  // computes extreme rays, but does not find grading !
    }

    if (do_only_Deg1_Elements && Grading.size() == 0) {
        if (Generators.nr_of_rows() > 0) {
            throw BadInputException("Need grading to compute degree 1 elements and cannot find one.");
        }
        else
            Grading = vector<Integer>(dim, 0);
    }

    if (Inequalities.nr_of_rows() == 0 && !isComputed(ConeProperty::SupportHyperplanes)) {
        throw FatalException("Could not get SupportHyperplanes.");
    }

    Matrix<IntegerFC> Inequ_on_Ker;
    BasisChangePointed.convert_to_sublattice_dual(Inequ_on_Ker, Inequalities);

    vector<IntegerFC> Truncation;
    if (inhomogeneous) {
        BasisChangePointed.convert_to_sublattice_dual_no_div(Truncation, Dehomogenization);
    }
    if (do_only_Deg1_Elements) {
        // in this case the grading acts as truncation and it is a NEW inequality
        if (ToCompute.test(ConeProperty::NoGradingDenom))
            BasisChangePointed.convert_to_sublattice_dual_no_div(Truncation, Grading);
        else
            BasisChangePointed.convert_to_sublattice_dual(Truncation, Grading);
    }

    Cone_Dual_Mode<IntegerFC> ConeDM(Inequ_on_Ker, Truncation,
                                     ToCompute.test(ConeProperty::KeepOrder) && dual_original_generators);
    // Inequ_on_Ker is NOT const
    Inequ_on_Ker = Matrix<IntegerFC>(0, 0);  // destroy it
    ConeDM.verbose = verbose;
    ConeDM.inhomogeneous = inhomogeneous;
    ConeDM.do_only_Deg1_Elements = do_only_Deg1_Elements;
    if (isComputed(ConeProperty::Generators))
        BasisChangePointed.convert_to_sublattice(ConeDM.Generators, Generators);
    if (isComputed(ConeProperty::ExtremeRays))
        ConeDM.ExtremeRaysInd = ExtremeRaysIndicator;
    ConeDM.hilbert_basis_dual();

    if (!isComputed(ConeProperty::MaximalSubspace)) {
        BasisChangePointed.convert_from_sublattice(BasisMaxSubspace, ConeDM.BasisMaxSubspace);
        BasisMaxSubspace.standardize_basis();
        check_vanishing_of_grading_and_dehom();  // all this must be done here because to_sublattice may kill it
    }

    if (!isComputed(ConeProperty::Sublattice) || !isComputed(ConeProperty::MaximalSubspace)) {
        if (!(do_only_Deg1_Elements || inhomogeneous)) {
            // At this point we still have BasisChange==BasisChangePointed
            // now we can pass to a pointed full-dimensional cone

            vector<Sublattice_Representation<IntegerFC> > BothRepFC = MakeSubAndQuot(ConeDM.Generators, ConeDM.BasisMaxSubspace);
            if (!BothRepFC[0].IsIdentity())
                BasisChange.compose(Sublattice_Representation<Integer>(BothRepFC[0]));
            setComputed(ConeProperty::Sublattice);
            if (!BothRepFC[1].IsIdentity())
                BasisChangePointed.compose(Sublattice_Representation<Integer>(BothRepFC[1]));
            ConeDM.to_sublattice(BothRepFC[1]);
        }
    }

    setComputed(ConeProperty::MaximalSubspace);  // NOT EARLIER !!!!

    // create a Full_Cone out of ConeDM
    Full_Cone<IntegerFC> FC(ConeDM);
    FC.verbose = verbose;
    // Give extra data to FC
    if (Grading.size() > 0) {
        BasisChangePointed.convert_to_sublattice_dual(FC.Grading, Grading);
        if (isComputed(ConeProperty::Grading))
            FC.is_Computed.set(ConeProperty::Grading);
    }
    if (inhomogeneous)
        BasisChangePointed.convert_to_sublattice_dual_no_div(FC.Truncation, Dehomogenization);
    FC.do_class_group = ToCompute.test(ConeProperty::ClassGroup);
    FC.dual_mode();
    extract_data(FC, ToCompute);

    /* if(verbose){
            cout << "Emb" << endl;
            BasisChangePointed.getEmbeddingMatrix().pretty_print(cout);
            cout << "Proj" << endl;
            BasisChangePointed.getProjectionMatrix().pretty_print(cout);
            cout << "ProjKey" << endl;
            cout << BasisChangePointed.getProjectionKey();
            cout << "Hilb" << endl;
            HilbertBasis.pretty_print(cout);
            cout << "Supps " << endl;
            SupportHyperplanes.pretty_print(cout);
    }*/
}

#ifdef ENFNORMALIZ
template <>
template <typename IntegerFC>
void Cone<renf_elem_class>::compute_dual_inner(ConeProperties& ToCompute) {
    assert(false);
}
#endif
//---------------------------------------------------------------------------

template <typename Integer>
Integer Cone<Integer>::compute_primary_multiplicity() {
    if (change_integer_type) {
        try {
            return compute_primary_multiplicity_inner<MachineInteger>();
        } catch (const ArithmeticException& e) {
            if (verbose) {
                verboseOutput() << e.what() << endl;
                verboseOutput() << "Restarting with a bigger type." << endl;
            }
            change_integer_type = false;
        }
    }
    return compute_primary_multiplicity_inner<Integer>();
}

//---------------------------------------------------------------------------

template <typename Integer>
template <typename IntegerFC>
Integer Cone<Integer>::compute_primary_multiplicity_inner() {
    Matrix<IntegerFC> Ideal(0, dim - 1);
    vector<IntegerFC> help(dim - 1);
    for (size_t i = 0; i < Generators.nr_of_rows(); ++i) {  // select ideal generators
        if (Generators[i][dim - 1] == 1) {
            for (size_t j = 0; j < dim - 1; ++j)
                convert(help[j], Generators[i][j]);
            Ideal.append(help);
        }
    }
    Full_Cone<IntegerFC> IdCone(Ideal, false);
    IdCone.do_bottom_dec = true;
    IdCone.do_determinants = true;
    IdCone.compute();
    return convertTo<Integer>(IdCone.detSum);
}

//---------------------------------------------------------------------------

// This function creates convex hull data from precomputed support hyperplanes and extreme rays
// so that these can be used in interactive mode for the modification of the originally constructed cone
// In principle it works like extract_convex_hull_data below.
template <typename Integer>
void Cone<Integer>::create_convex_hull_data() {
    ConvHullData.is_primal = true;

    ConvHullData.SLR = BasisChangePointed;
    ConvHullData.nr_threads = omp_get_max_threads();
    ConvHullData.HypCounter = vector<size_t>(ConvHullData.nr_threads);
    for (int i = 0; i < ConvHullData.nr_threads; ++i)
        ConvHullData.HypCounter[i] = i + 1;
    ConvHullData.old_nr_supp_hyps = SupportHyperplanes.nr_of_rows();

    size_t nr_extreme_rays = ExtremeRays.nr_of_rows();
    // no better idea
    ConvHullData.Comparisons.resize(nr_extreme_rays);
    ConvHullData.nrTotalComparisons = 0;

    ConvHullData.in_triang = vector<bool>(nr_extreme_rays, true);
    ConvHullData.GensInCone = identity_key(nr_extreme_rays);
    ConvHullData.nrGensInCone = nr_extreme_rays;

    ConvHullData.Generators = ExtremeRays;

    ConvHullData.Facets.clear();

    size_t rank = ExtremeRays.rank();

    bool skip_remaining = false;
    std::exception_ptr tmp_exception;
    size_t fac_counter = 0;
    vector<FACETDATA<Integer> > VecFacets(SupportHyperplanes.nr_of_rows());

#pragma omp parallel for
    for (fac_counter = 0; fac_counter < SupportHyperplanes.nr_of_rows(); ++fac_counter) {
        if (skip_remaining)
            continue;
        try {
            INTERRUPT_COMPUTATION_BY_EXCEPTION

            FACETDATA<Integer> Ret;
            Ret.Hyp = SupportHyperplanes[fac_counter];
            Ret.GenInHyp.resize(nr_extreme_rays);
            size_t nr_gens_in_hyp = 0;
            for (size_t i = 0; i < nr_extreme_rays; ++i) {
                Integer p = v_scalar_product(SupportHyperplanes[fac_counter], ConvHullData.Generators[i]);
                if (p < 0)
                    throw BadInputException("Incompatible precomputed data: wextreme rays and support hyperplanes inconsitent");
                Ret.GenInHyp[i] = 0;
                if (p == 0) {
                    Ret.GenInHyp[i] = 1;
                    nr_gens_in_hyp++;
                }
            }

            Ret.BornAt = 0;  // no better choice
            Ret.Mother = 0;  // ditto
            Ret.Ident = ConvHullData.HypCounter[0];
            ConvHullData.HypCounter[0] += ConvHullData.nr_threads;  // we use only residue class 0 mod nr_threads
            Ret.simplicial = (nr_gens_in_hyp == rank - 1);
            VecFacets[fac_counter] = Ret;
        } catch (const std::exception&) {
            tmp_exception = std::current_exception();
            skip_remaining = true;
#pragma omp flush(skip_remaining)
        }
    }
    if (!(tmp_exception == 0))
        std::rethrow_exception(tmp_exception);

    ConvHullData.Facets.insert(ConvHullData.Facets.begin(), VecFacets.begin(), VecFacets.end());
}

//---------------------------------------------------------------------------

template <typename Integer>
template <typename IntegerFC>
void Cone<Integer>::extract_convex_hull_data(Full_Cone<IntegerFC>& FC, bool primal) {
    ConvHullData.SLR = BasisChangePointed;
    ConvHullData.nr_threads = omp_get_max_threads();

    ConvHullData.is_primal = primal;
    // ConvHullData.Generators=Generators;
    swap(ConvHullData.HypCounter, FC.HypCounter);
    // swap(ConvHullData.in_triang,FC.in_triang);
    // swap(ConvHullData.GensInCone,FC.GensInCone);
    // ConvHullData.nrGensInCone=FC.nrGensInCone;
    swap(ConvHullData.Comparisons, FC.Comparisons);
    ConvHullData.nrTotalComparisons = FC.nrTotalComparisons;
    ConvHullData.old_nr_supp_hyps = FC.old_nr_supp_hyps;

    ConvHullData.Generators = Matrix<Integer>(0, dim);
    for (size_t i = 0; i < FC.nr_gen; ++i) {
        if (FC.Extreme_Rays_Ind[i]) {
            vector<Integer> v;
            if (primal)
                BasisChangePointed.convert_from_sublattice(v, FC.getGenerators()[i]);
            else
                BasisChangePointed.convert_from_sublattice_dual(v, FC.getGenerators()[i]);
            ConvHullData.Generators.append(v);
        }
    }

    size_t nr_extreme_rays = ConvHullData.Generators.nr_of_rows();

    ConvHullData.in_triang = vector<bool>(nr_extreme_rays, true);
    ConvHullData.GensInCone = identity_key(nr_extreme_rays);
    ConvHullData.nrGensInCone = nr_extreme_rays;

    ConvHullData.Facets.clear();

    for (const auto& Fac : FC.Facets) {
        FACETDATA<Integer> Ret;
        if (primal)
            BasisChangePointed.convert_from_sublattice_dual(Ret.Hyp, Fac.Hyp);
        else
            BasisChangePointed.convert_from_sublattice(Ret.Hyp, Fac.Hyp);

        // swap(Ret.GenInHyp,Fac.GenInHyp);
        // convert(Ret.ValNewGen,Fac.ValNewGen);
        Ret.GenInHyp.resize(nr_extreme_rays);
        size_t j = 0;
        for (size_t i = 0; i < FC.nr_gen; ++i) {
            if (FC.Extreme_Rays_Ind[i]) {
                Ret.GenInHyp[j] = Fac.GenInHyp[i];
                j++;
            }
        }

        Ret.BornAt = 0;  // no better choice
        Ret.Mother = 0;  // ditto
        Ret.Ident = Fac.Ident;
        // Ret.is_positive_on_all_original_gens = Fac.is_positive_on_all_original_gens;
        // Ret.is_negative_on_some_original_gen = Fac.is_negative_on_some_original_gen;
        Ret.simplicial = Fac.simplicial;

        ConvHullData.Facets.push_back(Ret);
    }

    /* FC.getGenerators().pretty_print(cout);
    cout << "-----------" << endl;
    ConvHullData.Generators.pretty_print(cout);
    cout << "-----------" << endl;*/
}

template <typename Integer>
void Cone<Integer>::compute_recession_rank() {
    if (isComputed(ConeProperty::RecessionRank) || !inhomogeneous)
        return;
    compute(ConeProperty::ExtremeRays);
    vector<key_t> level0key;
    Matrix<Integer> Help = BasisChangePointed.to_sublattice(ExtremeRays);
    vector<Integer> HelpDehom = BasisChangePointed.to_sublattice_dual(Dehomogenization);
    for (size_t i = 0; i < Help.nr_of_rows(); ++i) {
        if (v_scalar_product(Help[i], HelpDehom) == 0)
            level0key.push_back(i);
    }
    size_t pointed_recession_rank = Help.submatrix(level0key).rank();
    if (!isComputed(ConeProperty::MaximalSubspace))
        compute(ConeProperty::MaximalSubspace);
    recession_rank = pointed_recession_rank + BasisMaxSubspace.nr_of_rows();
    setComputed(ConeProperty::RecessionRank);
}

template <typename Integer>
void Cone<Integer>::compute_affine_dim_and_recession_rank() {
    if ((isComputed(ConeProperty::AffineDim) && isComputed(ConeProperty::RecessionRank)) || !inhomogeneous)
        return;

    if (!isComputed(ConeProperty::RecessionRank))
        compute_recession_rank();

    if (get_rank_internal() == recession_rank) {
        affine_dim = -1;
    }
    else {
        affine_dim = get_rank_internal() - 1;
    }
    setComputed(ConeProperty::AffineDim);
}

//---------------------------------------------------------------------------

template <typename Integer>
template <typename IntegerFC>
void Cone<Integer>::extract_data(Full_Cone<IntegerFC>& FC, ConeProperties& ToCompute) {
    // this function extracts ALL available data from the Full_Cone
    // even if it was in Cone already <- this may change
    // it is possible to delete the data in Full_Cone after extracting it

    if (verbose) {
        verboseOutput() << "transforming data..." << flush;
    }

    // It is important to extract the generators from the full cone.
    // The generators extracted from the full cone are the "purified" versions of
    // the generators with which the full cone was constructed. Since the order
    // can change, ExtremeRays is reset. Will be set again below.
    if (FC.isComputed(ConeProperty::Generators)) {
        BasisChangePointed.convert_from_sublattice(Generators, FC.getGenerators());
        setComputed(ConeProperty::Generators);
        ExtremeRaysIndicator.resize(0);
        is_Computed.reset(ConeProperty::ExtremeRays);
    }

    if (keep_convex_hull_data) {
        extract_convex_hull_data(FC, true);
    }

    if (FC.isComputed(ConeProperty::IsPointed) && !isComputed(ConeProperty::IsPointed)) {
        pointed = FC.isPointed();
        if (pointed)
            setComputed(ConeProperty::MaximalSubspace);
        setComputed(ConeProperty::IsPointed);
    }

    if (FC.isComputed(ConeProperty::IsEmptySemiOpen)) {
        empty_semiopen = false;
        if (FC.index_covering_face < ExcludedFaces.nr_of_rows()) {
            empty_semiopen = true;
            CoveringFace = ExcludedFaces[FC.index_covering_face];
            setComputed(ConeProperty::CoveringFace);
        }
        setComputed(ConeProperty::IsEmptySemiOpen);
    }

    Integer local_grading_denom;
    if (is_Computed.goals_using_grading(inhomogeneous).any())  // in this case we do not pull
        local_grading_denom = GradingDenom;                    // the grading from FC
    else
        local_grading_denom = 1;

    if (FC.isComputed(ConeProperty::Grading) && !using_renf<Integer>() && is_Computed.goals_using_grading(inhomogeneous).none()) {
        if (BasisChangePointed.getRank() != 0) {
            vector<Integer> test_grading_1, test_grading_2;
            if (Grading.size() == 0)  // grading is implicit, get it from FC
                BasisChangePointed.convert_from_sublattice_dual(test_grading_1, FC.getGrading());
            else
                test_grading_1 = Grading;
            test_grading_2 = BasisChangePointed.to_sublattice_dual_no_div(test_grading_1);
            local_grading_denom = v_gcd(test_grading_2);
        }

        if (Grading.size() == 0) {
            BasisChangePointed.convert_from_sublattice_dual(Grading, FC.getGrading());
            if (local_grading_denom > 1 && ToCompute.test(ConeProperty::NoGradingDenom))
                throw BadInputException("Grading denominator of implicit grading > 1 not allowed with NoGradingDenom.");
        }

        setComputed(ConeProperty::Grading);
        setWeights();

        // set denominator of Grading
        GradingDenom = 1;  // should have this value already, but to be on the safe sisde
        if (!ToCompute.test(ConeProperty::NoGradingDenom))
            GradingDenom = local_grading_denom;
        setComputed(ConeProperty::GradingDenom);
    }

    if (FC.isComputed(ConeProperty::ModuleGeneratorsOverOriginalMonoid)) {  // must precede extreme rays
        BasisChangePointed.convert_from_sublattice(ModuleGeneratorsOverOriginalMonoid,
                                                   FC.getModuleGeneratorsOverOriginalMonoid());
        ModuleGeneratorsOverOriginalMonoid.sort_by_weights(WeightsGrad, GradAbs);
        setComputed(ConeProperty::ModuleGeneratorsOverOriginalMonoid);
    }

    // Important: must be done after ModuleGeneratorsOverOriginalMonoid because in this case
    // generators may not be primitive or can contain duplicates genereating the same ray
    if (FC.isComputed(ConeProperty::ExtremeRays)) {
        set_extreme_rays(FC.getExtremeRays());
    }

    if (FC.isComputed(ConeProperty::SupportHyperplanes) && !isComputed(ConeProperty::SupportHyperplanes)) {
        /* if (inhomogeneous) {
            // remove irrelevant support hyperplane 0 ... 0 1
            vector<IntegerFC> irr_hyp_subl;
            BasisChangePointed.convert_to_sublattice_dual(irr_hyp_subl, Dehomogenization);
            FC.Support_Hyperplanes.remove_row(irr_hyp_subl);
        } */
        // BasisChangePointed.convert_from_sublattice_dual(SupportHyperplanes, FC.getSupportHyperplanes());
        extract_supphyps(FC, SupportHyperplanes);
        if (using_renf<Integer>())
            SupportHyperplanes.standardize_rows();
        norm_dehomogenization(FC.dim);
        SupportHyperplanes.sort_lex();
        Inequalities = SupportHyperplanes;
        setComputed(ConeProperty::SupportHyperplanes);
        addition_constraints_allowed = true;
    }
    if (FC.isComputed(ConeProperty::TriangulationSize) && !FC.isComputed(ConeProperty::PullingTriangulation)) {
        TriangulationSize = FC.totalNrSimplices;
        triangulation_is_nested = FC.triangulation_is_nested;
        triangulation_is_partial = FC.triangulation_is_partial;
        setComputed(ConeProperty::TriangulationSize);
        setComputed(ConeProperty::IsTriangulationPartial);
        setComputed(ConeProperty::IsTriangulationNested);
    }
    if (FC.isComputed(ConeProperty::TriangulationDetSum) && !FC.isComputed(ConeProperty::PullingTriangulation)) {
        convert(TriangulationDetSum, FC.detSum);
        setComputed(ConeProperty::TriangulationDetSum);
    }

    if (FC.isComputed(ConeProperty::Triangulation)) {
        is_Computed.reset(all_triangulations());                   // must reset these friends
                                                                   // when the basic triangulation
        is_Computed.reset(ConeProperty::UnimodularTriangulation);  // is recomputed
        Triangulation.first.clear();

        BasisChangePointed.convert_from_sublattice(BasicTriangulation.second, FC.getGenerators());

        size_t tri_size = FC.Triangulation.size();
        FC.Triangulation.sort(compareKeys<IntegerFC>);  // necessary to make triangulation unique
        BasicTriangulation.first.resize(tri_size);
        SHORTSIMPLEX<IntegerFC> simp;
        for (size_t i = 0; i < tri_size; ++i) {
            simp = FC.Triangulation.front();
            BasicTriangulation.first[i].key.swap(simp.key);
            if (FC.isComputed(ConeProperty::TriangulationDetSum))
                convert(BasicTriangulation.first[i].vol, simp.vol);
            else
                BasicTriangulation.first[i].vol = 0;
            if (FC.isComputed(ConeProperty::ConeDecomposition))
                BasicTriangulation.first[i].Excluded.swap(simp.Excluded);
            FC.Triangulation.pop_front();
        }
        if (FC.isComputed(ConeProperty::ConeDecomposition))
            setComputed(ConeProperty::ConeDecomposition);

        setComputed(ConeProperty::BasicTriangulation);

        if (ToCompute.test(ConeProperty::PlacingTriangulation)) {
            setComputed(ConeProperty::PlacingTriangulation);
            Triangulation = BasicTriangulation;
        }
    }

    if (FC.isComputed(ConeProperty::StanleyDec)) {
        BasicStanleyDec.first.clear();
        BasicStanleyDec.first.splice(BasicStanleyDec.first.begin(), FC.StanleyDec);
        setComputed(ConeProperty::BasicStanleyDec);
        BasisChangePointed.convert_from_sublattice(BasicStanleyDec.second, FC.getGenerators());
    }

    if (FC.isComputed(ConeProperty::InclusionExclusionData)) {
        InExData.clear();
        InExData.reserve(FC.InExCollect.size());
        vector<key_t> key;
        for (const auto& F : FC.InExCollect) {
            key.clear();
            for (size_t i = 0; i < FC.nr_gen; ++i) {
                if (F.first.test(i)) {
                    key.push_back(i);
                }
            }
            InExData.push_back(make_pair(key, F.second));
        }
        setComputed(ConeProperty::InclusionExclusionData);
    }
    if (FC.isComputed(ConeProperty::RecessionRank) && isComputed(ConeProperty::MaximalSubspace)) {
        recession_rank = FC.level0_dim + BasisMaxSubspace.nr_of_rows();
        setComputed(ConeProperty::RecessionRank);
        if (isComputed(ConeProperty::Sublattice))
            compute_affine_dim_and_recession_rank();
    }

    if (FC.isComputed(ConeProperty::ModuleRank)) {
        module_rank = FC.getModuleRank();
        setComputed(ConeProperty::ModuleRank);
    }

    // It would be correct to extract the multiülicity also from FC with the pulling triangulation.
    // However, we insist that no additional information is computed from it to avoid confusion.
    if (FC.isComputed(ConeProperty::Multiplicity) && !using_renf<Integer>() &&
        !FC.isComputed(ConeProperty::PullingTriangulation)) {
        if (!inhomogeneous) {
            multiplicity = FC.getMultiplicity();
            setComputed(ConeProperty::Multiplicity);
        }
        else if (FC.isComputed(ConeProperty::ModuleRank)) {
            multiplicity = FC.getMultiplicity() * static_cast<unsigned long>(module_rank);
            setComputed(ConeProperty::Multiplicity);
        }
    }

#ifdef ENFNORMALIZ
    if (FC.isComputed(ConeProperty::Multiplicity) && using_renf<Integer>()) {
        renf_volume = FC.renf_multiplicity;
        // setComputed(ConeProperty::Multiplicity);
        setComputed(ConeProperty::Volume);
        setComputed(ConeProperty::RenfVolume);
        euclidean_volume = static_cast<double>(renf_volume);
        for (size_t i = 1; i < dim; ++i)
            euclidean_volume /= i;
        euclidean_volume *= euclidean_height;

        setComputed(ConeProperty::EuclideanVolume);
    }
#endif

    if (FC.isComputed(ConeProperty::WitnessNotIntegrallyClosed)) {
        BasisChangePointed.convert_from_sublattice(WitnessNotIntegrallyClosed, FC.Witness);
        setComputed(ConeProperty::WitnessNotIntegrallyClosed);
        integrally_closed = false;
        setComputed(ConeProperty::IsIntegrallyClosed);
    }
    if (FC.isComputed(ConeProperty::HilbertBasis)) {
        if (inhomogeneous) {
            // separate (capped) Hilbert basis to the Hilbert basis of the level 0 cone
            // and the module generators in level 1
            HilbertBasis = Matrix<Integer>(0, dim);
            ModuleGenerators = Matrix<Integer>(0, dim);
            typename list<vector<IntegerFC> >::const_iterator FCHB(FC.Hilbert_Basis.begin());
            vector<Integer> tmp;
            for (; FCHB != FC.Hilbert_Basis.end(); ++FCHB) {
                INTERRUPT_COMPUTATION_BY_EXCEPTION

                BasisChangePointed.convert_from_sublattice(tmp, *FCHB);
                if (v_scalar_product(tmp, Dehomogenization) == 0) {  // Hilbert basis element of the cone at level 0
                    HilbertBasis.append(tmp);
                }
                else {  // module generator
                    ModuleGenerators.append(tmp);
                }
            }
            ModuleGenerators.sort_by_weights(WeightsGrad, GradAbs);
            setComputed(ConeProperty::ModuleGenerators);
            number_lattice_points = ModuleGenerators.nr_of_rows();
            setComputed(ConeProperty::NumberLatticePoints);
        }
        else {  // homogeneous
            HilbertBasis = Matrix<Integer>(0, dim);
            typename list<vector<IntegerFC> >::const_iterator FCHB(FC.Hilbert_Basis.begin());
            vector<Integer> tmp;
            for (; FCHB != FC.Hilbert_Basis.end(); ++FCHB) {
                BasisChangePointed.convert_from_sublattice(tmp, *FCHB);
                HilbertBasis.append(tmp);
            }
        }
        HilbertBasis.sort_by_weights(WeightsGrad, GradAbs);
        setComputed(ConeProperty::HilbertBasis);
    }

    if (FC.isComputed(ConeProperty::Deg1Elements)) {
        Deg1Elements = Matrix<Integer>(0, dim);
        if (local_grading_denom == GradingDenom) {
            typename list<vector<IntegerFC> >::const_iterator DFC(FC.Deg1_Elements.begin());
            vector<Integer> tmp;
            for (; DFC != FC.Deg1_Elements.end(); ++DFC) {
                INTERRUPT_COMPUTATION_BY_EXCEPTION

                BasisChangePointed.convert_from_sublattice(tmp, *DFC);
                Deg1Elements.append(tmp);
            }
            Deg1Elements.sort_by_weights(WeightsGrad, GradAbs);
        }
        setComputed(ConeProperty::Deg1Elements);
        number_lattice_points = Deg1Elements.nr_of_rows();
        setComputed(ConeProperty::NumberLatticePoints);
    }

    if (FC.isComputed(ConeProperty::HilbertSeries)) {
        long save_nr_coeff_quasipol = HSeries.get_nr_coeff_quasipol();  // Full_Cone does not compute the quasipolynomial
        long save_expansion_degree = HSeries.get_expansion_degree();    // or the expansion
        HSeries = FC.Hilbert_Series;
        HSeries.set_nr_coeff_quasipol(save_nr_coeff_quasipol);
        HSeries.set_expansion_degree(save_expansion_degree);
        setComputed(ConeProperty::HilbertSeries);
        // setComputed(ConeProperty::ExplicitHilbertSeries);
    }
    if (FC.isComputed(ConeProperty::HSOP)) {
        setComputed(ConeProperty::HSOP);
    }
    if (FC.isComputed(ConeProperty::IsDeg1ExtremeRays)) {
        deg1_extreme_rays = FC.isDeg1ExtremeRays();
        setComputed(ConeProperty::IsDeg1ExtremeRays);
    }
    if (FC.isComputed(ConeProperty::ExcludedFaces)) {
        BasisChangePointed.convert_from_sublattice_dual(ExcludedFaces, FC.getExcludedFaces());
        ExcludedFaces.sort_lex();
        setComputed(ConeProperty::ExcludedFaces);
    }

    if (FC.isComputed(ConeProperty::IsDeg1HilbertBasis)) {
        deg1_hilbert_basis = FC.isDeg1HilbertBasis();
        setComputed(ConeProperty::IsDeg1HilbertBasis);
    }
    if (FC.isComputed(ConeProperty::ClassGroup)) {
        convert(ClassGroup, FC.ClassGroup);
        setComputed(ConeProperty::ClassGroup);
    }

    if (FC.isComputed(ConeProperty::Automorphisms)) {
        extract_automorphisms(FC.Automs, true);  // true = must transform

        if (ToCompute.test(ConeProperty::Automorphisms))
            setComputed(ConeProperty::Automorphisms);
        if (ToCompute.test(ConeProperty::RationalAutomorphisms))
            setComputed(ConeProperty::RationalAutomorphisms);
        /* if (FC.isComputed(ConeProperty::ExploitAutomsVectors))
            setComputed(ConeProperty::ExploitAutomsVectors); */
    }

    check_integrally_closed(ToCompute);

    if (verbose) {
        verboseOutput() << " done." << endl;
    }
}

//---------------------------------------------------------------------------
template <typename Integer>
vector<vector<key_t> > Cone<Integer>::extract_subsets(const vector<vector<key_t> >& FC_Subsets,
                                                      size_t max_index,
                                                      const vector<key_t>& Key) {
    // Key is an injective map from [0,m-1] to [0,max_index-1];
    // Inv is its partial inverse, defined on Image(Key)
    // We assume that an indifidual subset of  [0,max_index-1] that has nonempty intersection
    // with Image(Key) is contained in Image(Key)
    // The nonempty intersections are mapped to subsets of [0,m-1] by Inv
    //
    // This funnction does not use the class Cone; could be in vector_operations
    //

    vector<vector<key_t> > C_Subsets;
    if (Key.empty())
        return C_Subsets;

    vector<key_t> Inv(max_index);
    for (size_t i = 0; i < Key.size(); ++i)
        Inv[Key[i]] = i;

    for (const auto& FC_Subset : FC_Subsets) {
        bool nonempty = false;
        for (unsigned int j : Key) {  // testing nonempty intersection = containment by assumption
            if (j == FC_Subset[0]) {
                nonempty = true;
                break;
            }
        }
        if (!nonempty)
            continue;
        vector<key_t> transf_subset(FC_Subset.size());
        for (size_t j = 0; j < FC_Subset.size(); ++j) {
            transf_subset[j] = Inv[FC_Subset[j]];
        }
        C_Subsets.push_back(transf_subset);
    }
    return C_Subsets;
}

//---------------------------------------------------------------------------
template <typename Integer>
template <typename IntegerFC>
vector<vector<key_t> > Cone<Integer>::extract_permutations(const vector<vector<key_t> >& FC_Permutations,
                                                           Matrix<IntegerFC>& FC_Vectors,
                                                           const Matrix<Integer>& ConeVectors,
                                                           bool primal,
                                                           vector<key_t>& Key,
                                                           const bool must_transform) {
    // Key has the same meaning as in extract_subsets,
    // but is computed by searching the properly transformed vectors of ConeVectors in FC_Vectors: ConeVector[i] =
    // FC_Vector[Key[i]] It is assumed that each permutation in FC_Permutations can be restricted to Image(Key) The induced
    // permutations on [0,m-1] (m=size(Key)) are computed.

    if (using_renf<Integer>()) {
        for (size_t i = 0; i < FC_Vectors.nr_of_rows(); ++i)
            v_standardize(FC_Vectors[i]);
    }

    map<vector<IntegerFC>, key_t> VectorsRef;
    for (size_t i = 0; i < FC_Vectors.nr_of_rows(); ++i) {
        VectorsRef[FC_Vectors[i]] = i;
    }
    Key.resize(ConeVectors.nr_of_rows());

    /*cout << "--------------" << endl;
    FC_Vectors.pretty_print(cout);
    cout << "--------------" << endl;
    ConeVectors.pretty_print(cout);
    cout << "=============" << endl;*/

    for (size_t i = 0; i < ConeVectors.nr_of_rows(); ++i) {
        vector<IntegerFC> search;
        if (must_transform) {
            if (primal)
                BasisChangePointed.convert_to_sublattice(search, ConeVectors[i]);
            else
                BasisChangePointed.convert_to_sublattice_dual(search, ConeVectors[i]);
        }
        else {
            if (primal)
                convert(search, ConeVectors[i]);
            else
                convert(search, ConeVectors[i]);
        }
        if (using_renf<Integer>()) {
            v_standardize(search);
        }
        auto E = VectorsRef.find(search);
        assert(E != VectorsRef.end());
        Key[i] = E->second;
    }

    vector<vector<key_t> > ConePermutations;
    for (const auto& FC_Permutation : FC_Permutations) {
        ConePermutations.push_back(conjugate_perm(FC_Permutation, Key));
    }
    return ConePermutations;
}

//---------------------------------------------------------------------------
template <typename Integer>
template <typename IntegerFC>
void Cone<Integer>::extract_supphyps(Full_Cone<IntegerFC>& FC, Matrix<Integer>& ret, bool dual) {
    if (dual)
        BasisChangePointed.convert_from_sublattice_dual(ret, FC.getSupportHyperplanes());
    else
        BasisChangePointed.convert_from_sublattice(ret, FC.getSupportHyperplanes());
}

template <typename Integer>
void Cone<Integer>::extract_supphyps(Full_Cone<Integer>& FC, Matrix<Integer>& ret, bool dual) {
    if (dual) {
        if (BasisChangePointed.IsIdentity())
            swap(ret, FC.Support_Hyperplanes);
        else
            ret = BasisChangePointed.from_sublattice_dual(FC.getSupportHyperplanes());
    }
    else {
        if (BasisChangePointed.IsIdentity())
            swap(ret, FC.Support_Hyperplanes);
        else
            ret = BasisChangePointed.from_sublattice(FC.getSupportHyperplanes());
    }
}

template <typename Integer>
void Cone<Integer>::norm_dehomogenization(size_t FC_dim) {
    if (inhomogeneous && FC_dim < dim) {  // make inequality for the inhomogeneous variable appear as dehomogenization
        vector<Integer> dehom_restricted = BasisChangePointed.to_sublattice_dual(Dehomogenization);
        if (using_renf<Integer>())
            v_standardize(dehom_restricted);
        for (size_t i = 0; i < SupportHyperplanes.nr_of_rows(); ++i) {
            vector<Integer> test = BasisChangePointed.to_sublattice_dual(SupportHyperplanes[i]);
            if (using_renf<Integer>())
                v_standardize(test);
            if (dehom_restricted == test) {
                SupportHyperplanes[i] = Dehomogenization;
                break;
            }
        }
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::check_integrally_closed(const ConeProperties& ToCompute) {
    if (!isComputed(ConeProperty::OriginalMonoidGenerators) || inhomogeneous)
        return;

    if (isComputed(ConeProperty::IsIntegrallyClosed) && !ToCompute.test(ConeProperty::WitnessNotIntegrallyClosed))
        return;

    if (!ToCompute.test(ConeProperty::IsIntegrallyClosed) && !isComputed(ConeProperty::HilbertBasis)) {
        return;
    }

    if (!isComputed(ConeProperty::IsIntegrallyClosed)) {
        unit_group_index = 1;
        if (BasisMaxSubspace.nr_of_rows() > 0)
            compute_unit_group_index();
        setComputed(ConeProperty::UnitGroupIndex);

        if (internal_index != 1 || unit_group_index != 1) {
            integrally_closed = false;
            setComputed(ConeProperty::IsIntegrallyClosed);
            return;
        }
    }

    if (!isComputed(ConeProperty::HilbertBasis))
        return;

    if (HilbertBasis.nr_of_rows() > InputGenerators.nr_of_rows()) {
        integrally_closed = false;
        setComputed(ConeProperty::IsIntegrallyClosed);
        if (!ToCompute.test(ConeProperty::WitnessNotIntegrallyClosed))
            return;
    }
    find_witness(ToCompute);
    setComputed(ConeProperty::IsIntegrallyClosed);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_unit_group_index() {
    assert(isComputed(ConeProperty::MaximalSubspace));
    compute(ConeProperty::SupportHyperplanes);
    // we want to compute in the maximal linear subspace
    Sublattice_Representation<Integer> Sub(BasisMaxSubspace, true);
    Matrix<Integer> origens_in_subspace(0, dim);

    // we must collect all original generators that lie in the maximal subspace

    for (size_t i = 0; i < InputGenerators.nr_of_rows(); ++i) {
        size_t j;
        for (j = 0; j < SupportHyperplanes.nr_of_rows(); ++j) {
            if (v_scalar_product(InputGenerators[i], SupportHyperplanes[j]) != 0)
                break;
        }
        if (j == SupportHyperplanes.nr_of_rows())
            origens_in_subspace.append(InputGenerators[i]);
    }
    Matrix<Integer> M = Sub.to_sublattice(origens_in_subspace);
    unit_group_index = M.full_rank_index();
    // cout << "Unit group index " << unit_group_index;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::find_witness(const ConeProperties& ToCompute) {
    if (!isComputed(ConeProperty::OriginalMonoidGenerators) || inhomogeneous) {
        // no original monoid defined
        throw NotComputableException(ConeProperties(ConeProperty::WitnessNotIntegrallyClosed));
    }
    if (isComputed(ConeProperty::IsIntegrallyClosed) && integrally_closed) {
        // original monoid is integrally closed
        throw NotComputableException(ConeProperties(ConeProperty::WitnessNotIntegrallyClosed));
    }
    if (isComputed(ConeProperty::WitnessNotIntegrallyClosed) || !isComputed(ConeProperty::HilbertBasis))
        return;

    long nr_hilb = HilbertBasis.nr_of_rows();
    // if the cone is not pointed, we have to check it on the quotion
    Matrix<Integer> gens_quot;
    Matrix<Integer> hilb_quot;
    if (!pointed) {
        gens_quot = BasisChangePointed.to_sublattice(InputGenerators);
        hilb_quot = BasisChangePointed.to_sublattice(HilbertBasis);
    }
    Matrix<Integer>& gens = pointed ? InputGenerators : gens_quot;
    Matrix<Integer>& hilb = pointed ? HilbertBasis : hilb_quot;
    integrally_closed = true;

    set<vector<Integer> > gens_set;
    gens_set.insert(gens.get_elements().begin(), gens.get_elements().end());
    integrally_closed = true;
    for (long h = 0; h < nr_hilb; ++h) {
        if (gens_set.find(hilb[h]) == gens_set.end()) {
            integrally_closed = false;
            if (ToCompute.test(ConeProperty::WitnessNotIntegrallyClosed)) {
                WitnessNotIntegrallyClosed = HilbertBasis[h];
                setComputed(ConeProperty::WitnessNotIntegrallyClosed);
            }
            break;
        }
    }
    setComputed(ConeProperty::IsIntegrallyClosed);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::set_original_monoid_generators(const Matrix<Integer>& Input) {
    InputGenerators = Input;
    if (using_renf<Integer>())
        return;

    if (!isComputed(ConeProperty::OriginalMonoidGenerators)) {
        setComputed(ConeProperty::OriginalMonoidGenerators);
    }
    // Generators = Input;
    // setComputed(ConeProperty::Generators);
    Matrix<Integer> M = BasisChange.to_sublattice(Input);
    internal_index = M.full_rank_index();
    setComputed(ConeProperty::InternalIndex);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::set_extreme_rays(const vector<bool>& ext) {
    assert(ext.size() == Generators.nr_of_rows());
    ExtremeRaysIndicator = ext;

    if (isComputed(ConeProperty::ExtremeRays))
        return;

    ExtremeRays = Generators.submatrix(ext);  // extreme rays of the homogenized cone
    vector<bool> choice = ext;
    if (inhomogeneous) {
        // separate extreme rays to rays of the level 0 cone
        // and the verticies of the polyhedron, which are in level >=1
        size_t nr_gen = Generators.nr_of_rows();
        vector<bool> VOP(nr_gen);
        for (size_t i = 0; i < nr_gen; i++) {
            if (ext[i] && v_scalar_product(Generators[i], Dehomogenization) != 0) {
                VOP[i] = true;
                choice[i] = false;
            }
        }
        VerticesOfPolyhedron = Generators.submatrix(VOP);
        if (using_renf<Integer>())
            VerticesOfPolyhedron.standardize_rows(Norm);
        VerticesOfPolyhedron.sort_by_weights(WeightsGrad, GradAbs);
        setComputed(ConeProperty::VerticesOfPolyhedron);
    }
    ExtremeRaysRecCone = Generators.submatrix(choice);

    if (inhomogeneous && !isComputed(ConeProperty::AffineDim) && isComputed(ConeProperty::MaximalSubspace)) {
        size_t level0_dim = ExtremeRaysRecCone.max_rank_submatrix_lex().size();
        recession_rank = level0_dim + BasisMaxSubspace.nr_of_rows();
        setComputed(ConeProperty::RecessionRank);
        if (get_rank_internal() == recession_rank) {
            affine_dim = -1;
        }
        else {
            affine_dim = get_rank_internal() - 1;
        }
        setComputed(ConeProperty::AffineDim);
    }
    if (isComputed(ConeProperty::ModuleGeneratorsOverOriginalMonoid)) {
        Matrix<Integer> ExteEmbedded = BasisChangePointed.to_sublattice(ExtremeRaysRecCone);  // done to extract gcd
        for (size_t i = 0; i < ExteEmbedded.nr_of_rows(); ++i)  // not always done for original monoid generators
            v_make_prime(ExteEmbedded[i]);                      // Moreover, several generators can give the same xtreme ray
        ExteEmbedded.remove_duplicate_and_zero_rows();
        ExtremeRaysRecCone = BasisChangePointed.from_sublattice(ExteEmbedded);
    }

    if (using_renf<Integer>()) {
        ExtremeRays.standardize_rows(Norm);
        ExtremeRaysRecCone.standardize_rows(Norm);
    }
    ExtremeRays.sort_by_weights(WeightsGrad, GradAbs);
    ExtremeRaysRecCone.sort_by_weights(WeightsGrad, GradAbs);
    setComputed(ConeProperty::ExtremeRays);
    addition_generators_allowed = true;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_vertices_float(ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::VerticesFloat) || isComputed(ConeProperty::VerticesFloat))
        return;
    if (!isComputed(ConeProperty::ExtremeRays))
        throw NotComputableException("VerticesFloat not computable without extreme rays");
    if (inhomogeneous && !isComputed(ConeProperty::VerticesOfPolyhedron))
        throw NotComputableException("VerticesFloat not computable in the inhomogeneous case without vertices");
    if (!inhomogeneous && !isComputed(ConeProperty::Grading))
        throw NotComputableException("VerticesFloat not computable in the homogeneous case without a grading");
    if (inhomogeneous)
        convert(VerticesFloat, VerticesOfPolyhedron);
    else
        convert(VerticesFloat, ExtremeRays);
    vector<nmz_float> norm;
    if (inhomogeneous)
        convert(norm, Dehomogenization);
    else {
        convert(norm, Grading);
        nmz_float GD = 1.0 / convertTo<double>(GradingDenom);
        v_scalar_multiplication(norm, GD);
    }
    VerticesFloat.standardize_rows(norm);
    setComputed(ConeProperty::VerticesFloat);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_extreme_rays_float(ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::ExtremeRaysFloat) || isComputed(ConeProperty::ExtremeRaysFloat))
        return;
    if (!isComputed(ConeProperty::ExtremeRays))
        throw NotComputableException("ExtremeRaysFloat not computable without extreme rays");
    if (inhomogeneous)
        convert(ExtremeRaysFloat, ExtremeRaysRecCone);
    else
        convert(ExtremeRaysFloat, ExtremeRays);
    vector<nmz_float> norm;
    if (!inhomogeneous) {
        if (isComputed(ConeProperty::Grading)) {
            convert(norm, Grading);
            nmz_float GD = 1.0 / convertTo<double>(GradingDenom);
            v_scalar_multiplication(norm, GD);
        }
    }
    ExtremeRaysFloat.standardize_rows(norm);
    setComputed(ConeProperty::ExtremeRaysFloat);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_supp_hyps_float(ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::SuppHypsFloat) || isComputed(ConeProperty::SuppHypsFloat))
        return;
    if (!isComputed(ConeProperty::SupportHyperplanes))
        throw NotComputableException("SuppHypsFloat not computable without support hyperplanes");

    convert(SuppHypsFloat, SupportHyperplanes);
    SuppHypsFloat.standardize_rows();
    setComputed(ConeProperty::SuppHypsFloat);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::complete_sublattice_comp(ConeProperties& ToCompute) {
    if (!isComputed(ConeProperty::Sublattice))
        return;
    setComputed(ConeProperty::Rank);
    if (ToCompute.test(ConeProperty::Equations)) {
        BasisChange.getEquationsMatrix();  // just to force computation, ditto below
        setComputed(ConeProperty::Equations);
    }
    if (ToCompute.test(ConeProperty::Congruences) || ToCompute.test(ConeProperty::ExternalIndex)) {
        BasisChange.getCongruencesMatrix();
        BasisChange.getExternalIndex();
        setComputed(ConeProperty::Congruences);
        setComputed(ConeProperty::ExternalIndex);
    }
}

template <typename Integer>
void Cone<Integer>::complete_HilbertSeries_comp(ConeProperties& ToCompute) {
    if (!isComputed(ConeProperty::HilbertSeries) && !isComputed(ConeProperty::EhrhartSeries))
        return;
    if (ToCompute.test(ConeProperty::HilbertQuasiPolynomial) || ToCompute.test(ConeProperty::EhrhartQuasiPolynomial))
        HSeries.computeHilbertQuasiPolynomial();
    if (HSeries.isHilbertQuasiPolynomialComputed()) {
        setComputed(ConeProperty::HilbertQuasiPolynomial);
        setComputed(ConeProperty::EhrhartQuasiPolynomial);
    }

    if (!inhomogeneous && !isComputed(ConeProperty::NumberLatticePoints) && ExcludedFaces.nr_of_rows() == 0) {
        // note: ConeProperty::ExcludedFaces not necessarily set TODO
        long save_expansion_degree = HSeries.get_expansion_degree();
        HSeries.set_expansion_degree(1);
        vector<mpz_class> expansion = HSeries.getExpansion();
        HSeries.set_expansion_degree(save_expansion_degree);
        long long nlp = 0;
        if (expansion.size() > 1) {
            nlp = convertToLongLong(expansion[1]);
        }
        number_lattice_points = nlp;
        setComputed(ConeProperty::NumberLatticePoints);
    }

    // we want to be able to convert HS ohr EhrS to hsop denom if
    // they have been computed without

    if (!(ToCompute.test(ConeProperty::HSOP) && !isComputed(ConeProperty::HSOP) &&
          (isComputed(ConeProperty::HilbertSeries) || isComputed(ConeProperty::EhrhartSeries))))  // everything done already
        return;

    compute(ConeProperty::ExtremeRays);
    if (inhomogeneous && !isComputed(ConeProperty::EhrhartSeries) && ExtremeRaysRecCone.nr_of_rows() == 0)
        return;  // in this case the Hilbert series is a polynomial and the Ehrhart series is not available

    Matrix<Integer> FC_gens;
    FC_gens = BasisChangePointed.to_sublattice(ExtremeRays);
    /* if(inhomogeneous){
        FC_gens.append(BasisChangePointed.to_sublattice(VerticesOfPolyhedron));
    }*/
    Full_Cone<Integer> FC(FC_gens);

    FC.inhomogeneous = inhomogeneous && !isComputed(ConeProperty::EhrhartSeries);

    FC.Support_Hyperplanes = BasisChangePointed.to_sublattice_dual(SupportHyperplanes);
    FC.dualize_cone();  // minimizes support hyperplanes

    if (!inhomogeneous || !isComputed(ConeProperty::EhrhartSeries)) {
        if (ToCompute.test(ConeProperty::NoGradingDenom))
            BasisChangePointed.convert_to_sublattice_dual_no_div(FC.Grading, Grading);
        else
            BasisChangePointed.convert_to_sublattice_dual(FC.Grading, Grading);
        FC.is_Computed.set(ConeProperty::Grading);
    }
    else {
        FC.Grading = BasisChangePointed.to_sublattice_dual_no_div(Dehomogenization);
    }
    if (FC.inhomogeneous)
        FC.Truncation = BasisChangePointed.to_sublattice_dual_no_div(Dehomogenization);
    FC.Extreme_Rays_Ind = vector<bool>(FC_gens.nr_of_rows(), true);
    FC.is_Computed.set(ConeProperty::ExtremeRays);

    FC.compute_hsop();

    if (isComputed(ConeProperty::EhrhartSeries)) {
        EhrSeries.setHSOPDenom(FC.Hilbert_Series.getHSOPDenom());
        EhrSeries.compute_hsop_num();
    }
    else {  // we have a proper Hilbert series which is not a polynomial
        if (isComputed(ConeProperty::HilbertSeries)) {
            HSeries.setHSOPDenom(FC.Hilbert_Series.getHSOPDenom());
            HSeries.compute_hsop_num();
        }
    }
    setComputed(ConeProperty::HSOP);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::setNumericalParams(const map<NumParam::Param, long>& num_params) {
    auto np = num_params.find(NumParam::expansion_degree);
    if (np != num_params.end())
        setExpansionDegree(np->second);
    np = num_params.find(NumParam::nr_coeff_quasipol);
    if (np != num_params.end())
        setNrCoeffQuasiPol(np->second);
    np = num_params.find(NumParam::face_codim_bound);
    if (np != num_params.end())
        setFaceCodimBound(np->second);
    np = num_params.find(NumParam::autom_codim_bound_vectors);
    if (np != num_params.end())
        setAutomCodimBoundVectors(np->second);
    np = num_params.find(NumParam::decimal_digits);
    if (np != num_params.end())
        setDecimalDigits(np->second);
    np = num_params.find(NumParam::block_size_hollow_tri);
    if (np != num_params.end())
        setBlocksizeHollowTri(np->second);
}

template <typename Integer>
void Cone<Integer>::setPolynomial(string poly) {
    IntData = IntegrationData(poly);
    is_Computed.reset(ConeProperty::WeightedEhrhartSeries);
    is_Computed.reset(ConeProperty::WeightedEhrhartQuasiPolynomial);
    is_Computed.reset(ConeProperty::Integral);
    is_Computed.reset(ConeProperty::EuclideanIntegral);
    is_Computed.reset(ConeProperty::VirtualMultiplicity);
}

template <typename Integer>
void Cone<Integer>::setNrCoeffQuasiPol(long nr_coeff) {
    HSeries.resetHilbertQuasiPolynomial();
    IntData.set_nr_coeff_quasipol(nr_coeff);
    is_Computed.reset(ConeProperty::WeightedEhrhartQuasiPolynomial);
    IntData.resetHilbertQuasiPolynomial();
    HSeries.set_nr_coeff_quasipol(nr_coeff);
    is_Computed.reset(ConeProperty::HilbertQuasiPolynomial);
}

template <typename Integer>
void Cone<Integer>::setExpansionDegree(long degree) {
    IntData.set_expansion_degree(degree);
    HSeries.set_expansion_degree(degree);
    EhrSeries.set_expansion_degree(degree);
}

template <typename Integer>
void Cone<Integer>::setFaceCodimBound(long bound) {
    face_codim_bound = bound;
    is_Computed.reset(ConeProperty::FaceLattice);
    is_Computed.reset(ConeProperty::FVector);
    is_Computed.reset(ConeProperty::DualFaceLattice);
    is_Computed.reset(ConeProperty::DualFVector);
    FaceLat.clear();
    DualFaceLat.clear();
    dual_f_vector.clear();
    f_vector.clear();
}

template <typename Integer>
void Cone<Integer>::setAutomCodimBoundMult(long bound) {
    autom_codim_mult = bound;
}

template <typename Integer>
void Cone<Integer>::setAutomCodimBoundVectors(long bound) {
    autom_codim_vectors = bound;
}

template <typename Integer>
void Cone<Integer>::setDecimalDigits(long digits) {
    decimal_digits = digits;
}

template <typename Integer>
void Cone<Integer>::setBlocksizeHollowTri(long block_size) {
    block_size_hollow_tri = block_size;
}

template <typename Integer>
void Cone<Integer>::setProjectName(const string& my_project) {
    project_name = my_project;
}

template <typename Integer>
string Cone<Integer>::getProjectName() const {
    return project_name;
}

//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::try_symmetrization(ConeProperties& ToCompute) {
    if (dim <= 1 || using_renf<Integer>())
        return;

    if (ToCompute.test(ConeProperty::NoSymmetrization) || ToCompute.test(ConeProperty::Descent))
        return;

    if (!(ToCompute.test(ConeProperty::Symmetrize) || ToCompute.test(ConeProperty::HilbertSeries) ||
          ToCompute.test(ConeProperty::Multiplicity)))
        return;

    if (inhomogeneous || nr_latt_gen > 0 || nr_cone_gen > 0 || lattice_ideal_input || Grading.size() < dim) {
        if (ToCompute.test(ConeProperty::Symmetrize))
            throw BadInputException("Symmetrization not possible with the given input");
        else
            return;
    }

#ifndef NMZ_COCOA
    if (ToCompute.test(ConeProperty::Symmetrize)) {
        throw BadInputException("Symmetrization not possible without CoCoALib");
    }
    else
        return;
#endif

    Matrix<Integer> AllConst = ExcludedFaces;
    size_t nr_excl = AllConst.nr_of_rows();
    AllConst.append(Equations);
    size_t nr_equ = AllConst.nr_of_rows() - nr_excl;
    vector<bool> unit_vector(dim, false);
    for (size_t i = 0; i < Inequalities.nr_of_rows(); ++i) {
        size_t nr_nonzero = 0;
        size_t nonzero_coord;
        bool is_unit_vector = true;
        bool is_zero = true;
        for (size_t j = 0; j < dim; ++j) {
            if (Inequalities[i][j] == 0)
                continue;
            is_zero = false;
            if (nr_nonzero > 0 || Inequalities[i][j] != 1) {  // not a sign inequality
                is_unit_vector = false;
                break;
            }
            nr_nonzero++;
            nonzero_coord = j;
        }
        if (is_zero)  // tatological inequality superfluous
            continue;
        if (!is_unit_vector)
            AllConst.append(Inequalities[i]);
        else
            unit_vector[nonzero_coord] = true;
    }

    size_t nr_inequ = AllConst.nr_of_rows() - nr_equ - nr_excl;

    for (size_t i = 0; i < dim; ++i)
        if (!unit_vector[i]) {
            if (ToCompute.test(ConeProperty::Symmetrize))
                throw BadInputException("Symmetrization not possible: Not all sign inequalities in input");
            else
                return;
        }

    for (size_t i = 0; i < Congruences.nr_of_rows(); ++i) {
        vector<Integer> help = Congruences[i];
        help.resize(dim);
        AllConst.append(help);
    }
    // now we have collected all constraints and checked the existence of the sign inequalities

    AllConst.append(Grading);

    /* AllConst.pretty_print(cout);
    cout << "----------------------" << endl;
    cout << nr_excl << " " << nr_equ << " " << nr_inequ << endl; */

    AllConst = AllConst.transpose();

    map<vector<Integer>, size_t> classes;

    for (size_t j = 0; j < AllConst.nr_of_rows(); ++j) {
        auto C = classes.find(AllConst[j]);
        if (C != classes.end())
            C->second++;
        else
            classes.insert(pair<vector<Integer>, size_t>(AllConst[j], 1));
    }

    vector<size_t> multiplicities;
    Matrix<Integer> SymmConst(0, AllConst.nr_of_columns());

    for (const auto& C : classes) {
        multiplicities.push_back(C.second);
        SymmConst.append(C.first);
    }
    SymmConst = SymmConst.transpose();

    vector<Integer> SymmGrad = SymmConst[SymmConst.nr_of_rows() - 1];

    if (verbose) {
        verboseOutput() << "Embedding dimension of symmetrized cone = " << SymmGrad.size() << endl;
    }

    if (SymmGrad.size() > 2 * dim / 3) {
        if (!ToCompute.test(ConeProperty::Symmetrize)) {
            return;
        }
    }

    /* compute_generators(); // we must protect against the zero cone
    if(get_rank_internal()==0)
        return; */

    Matrix<Integer> SymmInequ(0, SymmConst.nr_of_columns());
    Matrix<Integer> SymmEqu(0, SymmConst.nr_of_columns());
    Matrix<Integer> SymmCong(0, SymmConst.nr_of_columns());
    Matrix<Integer> SymmExcl(0, SymmConst.nr_of_columns());

    for (size_t i = 0; i < nr_excl; ++i)
        SymmExcl.append(SymmConst[i]);
    for (size_t i = nr_excl; i < nr_excl + nr_equ; ++i)
        SymmEqu.append(SymmConst[i]);
    for (size_t i = nr_excl + nr_equ; i < nr_excl + nr_equ + nr_inequ; ++i)
        SymmInequ.append(SymmConst[i]);
    for (size_t i = nr_excl + nr_equ + nr_inequ; i < SymmConst.nr_of_rows() - 1; ++i) {
        SymmCong.append(SymmConst[i]);
        SymmCong[SymmCong.nr_of_rows() - 1].push_back(Congruences[i - (nr_inequ + nr_equ)][dim]);  // restore modulus
    }

    string polynomial;

    for (size_t i = 0; i < multiplicities.size(); ++i) {
        for (size_t j = 1; j < multiplicities[i]; ++j)
            polynomial += "(x[" + to_string((unsigned long long)i + 1) + "]+" + to_string((unsigned long long)j) + ")|";
    }
    polynomial += "1";
    mpz_class fact = 1;
    for (unsigned long multiplicitie : multiplicities) {
        for (size_t j = 1; j < multiplicitie; ++j)
            fact *= static_cast<unsigned long>(j);
    }
    polynomial += "/" + fact.get_str() + ";";

#ifdef NMZ_COCOA

    InputMap<Integer> SymmInput;
    SymmInput[InputType::inequalities] = SymmInequ;
    SymmInput[InputType::equations] = SymmEqu;
    SymmInput[InputType::congruences] = SymmCong;
    SymmInput[InputType::excluded_faces] = SymmExcl;
    SymmInput[InputType::grading] = SymmGrad;
    vector<Integer> NonNeg(SymmGrad.size(), 1);
    SymmInput[InputType::signs] = NonNeg;

    if (SymmCone != NULL)
        delete SymmCone;

    SymmCone = new Cone<Integer>(SymmInput);
    SymmCone->setPolynomial(polynomial);
    SymmCone->setDecimalDigits(decimal_digits);
    SymmCone->setNrCoeffQuasiPol(HSeries.get_nr_coeff_quasipol());
    SymmCone->HSeries.set_period_bounded(HSeries.get_period_bounded());
    SymmCone->setVerbose(verbose);
    ConeProperties SymmToCompute;
    SymmToCompute.set(ConeProperty::SupportHyperplanes);
    SymmToCompute.set(ConeProperty::WeightedEhrhartSeries, ToCompute.test(ConeProperty::HilbertSeries));
    SymmToCompute.set(ConeProperty::VirtualMultiplicity, ToCompute.test(ConeProperty::Multiplicity));
    SymmToCompute.set(ConeProperty::BottomDecomposition, ToCompute.test(ConeProperty::BottomDecomposition));
    SymmToCompute.set(ConeProperty::NoGradingDenom, ToCompute.test(ConeProperty::NoGradingDenom));
    SymmToCompute.set(ConeProperty::SignedDec, ToCompute.test(ConeProperty::SignedDec));
    SymmToCompute.set(ConeProperty::NoSignedDec, ToCompute.test(ConeProperty::NoSignedDec));
    SymmToCompute.set(ConeProperty::GradingIsPositive, ToCompute.test(ConeProperty::GradingIsPositive));
    SymmToCompute.set(ConeProperty::FixedPrecision, ToCompute.test(ConeProperty::FixedPrecision));
    SymmCone->compute(SymmToCompute);
    if (SymmCone->isComputed(ConeProperty::WeightedEhrhartSeries)) {
        long save_expansion_degree = HSeries.get_expansion_degree();  // not given to the symmetrization
        HSeries = SymmCone->getWeightedEhrhartSeries().first;
        HSeries.set_expansion_degree(save_expansion_degree);
        setComputed(ConeProperty::HilbertSeries);
        // setComputed(ConeProperty::ExplicitHilbertSeries);
    }
    if (SymmCone->isComputed(ConeProperty::VirtualMultiplicity)) {
        multiplicity = SymmCone->getVirtualMultiplicity();
        setComputed(ConeProperty::Multiplicity);
    }
    if (SymmCone->isComputed(ConeProperty::FixedPrecision)) {
        setComputed(ConeProperty::FixedPrecision);
    }
    setComputed(ConeProperty::Symmetrize);
    ToCompute.set(ConeProperty::NoGradingDenom);
    return;

#endif
}

template <typename Integer>
void integrate(Cone<Integer>& C, const bool do_virt_mult);

template <typename Integer>
void generalizedEhrhartSeries(Cone<Integer>& C);

template <typename Integer>
void Cone<Integer>::compute_integral(ConeProperties& ToCompute) {
    if (isComputed(ConeProperty::Integral) || !ToCompute.test(ConeProperty::Integral))
        return;
    if (BasisMaxSubspace.nr_of_rows() > 0)
        throw NotComputableException("Integral not computable for polyhedra containing an affine space of dim > 0");
    if (IntData.getPolynomial() == "")
        throw BadInputException("Polynomial weight missing");
#ifdef NMZ_COCOA
    if (get_rank_internal() == 0) {
        getIntData().setIntegral(0);
        getIntData().setEuclideanIntegral(0);
    }
    else {
        integrate<Integer>(*this, false);
    }
    setComputed(ConeProperty::Integral);
    setComputed(ConeProperty::EuclideanIntegral);
#endif
}

template <typename Integer>
void Cone<Integer>::compute_virt_mult(ConeProperties& ToCompute) {
    if (isComputed(ConeProperty::VirtualMultiplicity) || !ToCompute.test(ConeProperty::VirtualMultiplicity))
        return;
    if (BasisMaxSubspace.nr_of_rows() > 0)
        throw NotComputableException("Virtual multiplicity not computable for polyhedra containing an affine space of dim > 0");
    if (IntData.getPolynomial() == "")
        throw BadInputException("Polynomial weight missing");
#ifdef NMZ_COCOA
    if (get_rank_internal() == 0)
        getIntData().setVirtualMultiplicity(0);
    else
        integrate<Integer>(*this, true);
    setComputed(ConeProperty::VirtualMultiplicity);
#endif
}

template <typename Integer>
void Cone<Integer>::compute_weighted_Ehrhart(ConeProperties& ToCompute) {
    if (isComputed(ConeProperty::WeightedEhrhartSeries) || !ToCompute.test(ConeProperty::WeightedEhrhartSeries))
        return;
    if (BasisMaxSubspace.nr_of_rows() > 0)
        throw NotComputableException(
            "Weighted Ehrhart series not computable for polyhedra containing an affine space of dim > 0");
    if (IntData.getPolynomial() == "")
        throw BadInputException("Polynomial weight missing");
        /* if(get_rank_internal()==0)
            throw NotComputableException("WeightedEhrhartSeries not computed in dimenison 0");*/
#ifdef NMZ_COCOA
    generalizedEhrhartSeries(*this);
    setComputed(ConeProperty::WeightedEhrhartSeries);
    if (getIntData().isWeightedEhrhartQuasiPolynomialComputed()) {
        setComputed(ConeProperty::WeightedEhrhartQuasiPolynomial);
        setComputed(ConeProperty::VirtualMultiplicity);
    }
#endif
}

#ifdef ENFNORMALIZ
template <>
void Cone<renf_elem_class>::compute_weighted_Ehrhart(ConeProperties& ToCompute) {
    assert(false);
}

template <>
void Cone<renf_elem_class>::compute_virt_mult(ConeProperties& ToCompute) {
    assert(false);
}

template <>
void Cone<renf_elem_class>::compute_integral(ConeProperties& ToCompute) {
    assert(false);
}
#endif
//---------------------------------------------------------------------------
template <typename Integer>
bool Cone<Integer>::get_verbose() {
    return verbose;
}

//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::check_Gorenstein(ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::IsGorenstein) || isComputed(ConeProperty::IsGorenstein))
        return;
    if (ToCompute.test(ConeProperty::KeepOrder))
        compute(ConeProperty::SupportHyperplanes, ConeProperty::KeepOrder, ConeProperty::MaximalSubspace);
    else
        compute(ConeProperty::SupportHyperplanes, ConeProperty::MaximalSubspace);

    if (dim == 0) {
        Gorenstein = true;
        setComputed(ConeProperty::IsGorenstein);
        GeneratorOfInterior = vector<Integer>(dim, 0);
        setComputed(ConeProperty::GeneratorOfInterior);
        return;
    }
    Matrix<Integer> TransfSupps = BasisChangePointed.to_sublattice_dual(SupportHyperplanes);
    assert(TransfSupps.nr_of_rows() > 0);
    Gorenstein = false;
    vector<Integer> TransfIntGen = TransfSupps.find_linear_form();
    if (TransfIntGen.size() != 0 && v_scalar_product(TransfIntGen, TransfSupps[0]) == 1) {
        Gorenstein = true;
        GeneratorOfInterior = BasisChangePointed.from_sublattice(TransfIntGen);
        setComputed(ConeProperty::GeneratorOfInterior);
    }
    setComputed(ConeProperty::IsGorenstein);
}

//---------------------------------------------------------------------------
template <typename Integer>
template <typename IntegerFC>
void Cone<Integer>::give_data_of_approximated_cone_to(Full_Cone<IntegerFC>& FC) {
    // *this is the approximatING cone. The support hyperplanes and equations of the approximatED
    // cone are given to the Full_Cone produced from *this so that the superfluous points can
    // bre sorted out as early as possible.

    assert(is_approximation);
    assert(ApproximatedCone->inhomogeneous || ApproximatedCone->getGradingDenom() == 1);  // in case we generalize later

    FC.is_global_approximation = true;
    // FC.is_approximation=true; At present not allowed. Only used for approximation within Full_Cone

    // We must distinguish zwo cases: Approximated->Grading_Is_Coordinate or it is not

    // If it is not:
    // The first coordinate in *this is the degree given by the grading
    // in ApproximatedCone. We disregard it by setting the first coordinate
    // of the grading, inequalities and equations to 0, and then have 0 followed
    // by the grading, equations and inequalities resp. of ApproximatedCone.

    vector<Integer> help_g;
    if (ApproximatedCone->inhomogeneous)
        help_g = ApproximatedCone->Dehomogenization;
    else
        help_g = ApproximatedCone->Grading;

    if (ApproximatedCone->Grading_Is_Coordinate) {
        swap(help_g[0], help_g[ApproximatedCone->GradingCoordinate]);
        BasisChangePointed.convert_to_sublattice_dual_no_div(FC.Subcone_Grading, help_g);
    }
    else {
        vector<Integer> help(help_g.size() + 1);
        help[0] = 0;
        for (size_t j = 0; j < help_g.size(); ++j)
            help[j + 1] = help_g[j];
        BasisChangePointed.convert_to_sublattice_dual_no_div(FC.Subcone_Grading, help);
    }

    Matrix<Integer> Eq = ApproximatedCone->BasisChangePointed.getEquationsMatrix();
    FC.Subcone_Equations = Matrix<IntegerFC>(Eq.nr_of_rows(), BasisChangePointed.getRank());
    if (ApproximatedCone->Grading_Is_Coordinate) {
        Eq.exchange_columns(0, ApproximatedCone->GradingCoordinate);
        BasisChangePointed.convert_to_sublattice_dual(FC.Subcone_Equations, Eq);
    }
    else {
        for (size_t i = 0; i < Eq.nr_of_rows(); ++i) {
            vector<Integer> help(Eq.nr_of_columns() + 1, 0);
            for (size_t j = 0; j < Eq.nr_of_columns(); ++j)
                help[j + 1] = Eq[i][j];
            BasisChangePointed.convert_to_sublattice_dual(FC.Subcone_Equations[i], help);
        }
    }

    ApproximatedCone->compute(ConeProperty::SupportHyperplanes);
    Matrix<Integer> Supp = ApproximatedCone->SupportHyperplanes;
    FC.Subcone_Support_Hyperplanes = Matrix<IntegerFC>(Supp.nr_of_rows(), BasisChangePointed.getRank());

    if (ApproximatedCone->Grading_Is_Coordinate) {
        Supp.exchange_columns(0, ApproximatedCone->GradingCoordinate);
        BasisChangePointed.convert_to_sublattice_dual(FC.Subcone_Support_Hyperplanes, Supp);
    }
    else {
        for (size_t i = 0; i < Supp.nr_of_rows(); ++i) {
            vector<Integer> help(Supp.nr_of_columns() + 1, 0);
            for (size_t j = 0; j < Supp.nr_of_columns(); ++j)
                help[j + 1] = Supp[i][j];
            BasisChangePointed.convert_to_sublattice_dual(FC.Subcone_Support_Hyperplanes[i], help);
        }
    }
}

//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::try_approximation_or_projection(ConeProperties& ToCompute) {
    if ((ToCompute.test(ConeProperty::NoProjection) && !ToCompute.test(ConeProperty::Approximate)) ||
        ToCompute.test(ConeProperty::DualMode) || ToCompute.test(ConeProperty::PrimalMode) ||
        ToCompute.test(ConeProperty::ExploitAutomsVectors))
        return;

    if (ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid))
        return;

    if (!inhomogeneous && (!(ToCompute.test(ConeProperty::Deg1Elements) || ToCompute.test(ConeProperty::NumberLatticePoints)) ||
                           ToCompute.test(ConeProperty::HilbertBasis) || ToCompute.test(ConeProperty::HilbertSeries)))
        return;

    if (inhomogeneous && (!ToCompute.test(ConeProperty::ModuleGenerators) && !ToCompute.test(ConeProperty::HilbertBasis) &&
                          !ToCompute.test(ConeProperty::NumberLatticePoints)))
        return;

    bool polytope_check_done = false;
    if (inhomogeneous && isComputed(ConeProperty::Generators)) {  // try to catch unbounded polyhedra as early as possible
        polytope_check_done = true;
        for (size_t i = 0; i < Generators.nr_of_rows(); ++i) {
            if (v_scalar_product(Generators[i], Dehomogenization) == 0) {
                if (ToCompute.test(ConeProperty::Approximate) || ToCompute.test(ConeProperty::Projection) ||
                    ToCompute.test(ConeProperty::NumberLatticePoints))
                    throw NotComputableException(
                        "Approximation, Projection or NumberLatticePoints not applicable to unbounded polyhedra");
                else
                    return;
            }
        }
    }

    if (!ToCompute.test(ConeProperty::Approximate))
        is_parallelotope = check_parallelotope();

    if (verbose && is_parallelotope)
        verboseOutput() << "Polyhedron is parallelotope" << endl;

    if (is_parallelotope) {
        SupportHyperplanes = Inequalities;
        SupportHyperplanes.remove_row(Dehomogenization);
        setComputed(ConeProperty::SupportHyperplanes);
        addition_constraints_allowed = true;
        setComputed(ConeProperty::MaximalSubspace);
        setComputed(ConeProperty::Sublattice);
        pointed = true;
        setComputed(ConeProperty::IsPointed);
        if (inhomogeneous) {
            affine_dim = dim - 1;
            setComputed(ConeProperty::AffineDim);
            recession_rank = 0;
            setComputed(ConeProperty::RecessionRank);
        }
    }

    ConeProperties NeededHere;
    NeededHere.set(ConeProperty::SupportHyperplanes);
    NeededHere.set(ConeProperty::Sublattice);
    NeededHere.set(ConeProperty::MaximalSubspace);
    NeededHere.set(ConeProperty::KeepOrder, ToCompute.test(ConeProperty::KeepOrder));
    if (inhomogeneous)
        NeededHere.set(ConeProperty::AffineDim);
    if (!inhomogeneous) {
        NeededHere.set(ConeProperty::Grading);
        if (ToCompute.test(ConeProperty::NoGradingDenom))
            NeededHere.set(ConeProperty::NoGradingDenom);
    }
    NeededHere.reset(is_Computed);
    try {
        compute(NeededHere);
    } catch (const NotComputableException& e)  // in case the grading does not exist -- will be found later
    {
    }

    if (!is_parallelotope && !ToCompute.test(ConeProperty ::Projection) && !ToCompute.test(ConeProperty::Approximate) &&
        SupportHyperplanes.nr_of_rows() > 100 * ExtremeRays.nr_of_rows())
        return;

    if (!is_parallelotope && !ToCompute.test(ConeProperty::Approximate)) {  // we try again
        is_parallelotope = check_parallelotope();
        if (is_parallelotope) {
            if (verbose)
                verboseOutput() << "Polyhedron is parallelotope" << endl;
            SupportHyperplanes = Inequalities;
            SupportHyperplanes.remove_row(Dehomogenization);
            setComputed(ConeProperty::SupportHyperplanes);
            addition_constraints_allowed = true;
            setComputed(ConeProperty::MaximalSubspace);
            setComputed(ConeProperty::Sublattice);
            pointed = true;
            setComputed(ConeProperty::IsPointed);
            if (inhomogeneous) {
                affine_dim = dim - 1;
                setComputed(ConeProperty::AffineDim);
            }
        }
    }

    if (!is_parallelotope) {  // don't need them anymore
        Pair.clear();
        ParaInPair.clear();
    }

    if (inhomogeneous && affine_dim <= 0)
        return;

    if (!inhomogeneous && !isComputed(ConeProperty::Grading))
        return;

    if (!inhomogeneous && ToCompute.test(ConeProperty::Approximate) && GradingDenom != 1)
        return;

    if (!pointed || BasisChangePointed.getRank() == 0)
        return;

    if (inhomogeneous && !polytope_check_done) {
        for (size_t i = 0; i < Generators.nr_of_rows(); ++i) {
            if (v_scalar_product(Generators[i], Dehomogenization) == 0) {
                if (ToCompute.test(ConeProperty::Approximate) || ToCompute.test(ConeProperty::Projection) ||
                    ToCompute.test(ConeProperty::NumberLatticePoints))
                    throw NotComputableException(
                        "Approximation, Projection or NumberLatticePoints not applicable to unbounded polyhedra");
                else
                    return;
            }
        }
    }

    if (inhomogeneous) {  // exclude that dehomogenization has a gcd > 1
        vector<Integer> test_dehom = BasisChange.to_sublattice_dual_no_div(Dehomogenization);
        if (v_make_prime(test_dehom) != 1)
            return;
    }

    // ****************************************************************
    //
    // NOTE: THE FIRST COORDINATE IS (OR WILL BE MADE) THE GRADING !!!!
    //
    // ****************************************************************

    vector<Integer> GradForApprox;
    if (!inhomogeneous)
        GradForApprox = Grading;
    else {
        GradForApprox = Dehomogenization;
        GradingDenom = 1;
    }

    Grading_Is_Coordinate = false;
    size_t nr_nonzero = 0;
    for (size_t i = 0; i < dim; ++i) {
        if (GradForApprox[i] != 0) {
            nr_nonzero++;
            GradingCoordinate = i;
        }
    }
    if (nr_nonzero == 1) {
        if (GradForApprox[GradingCoordinate] == 1)
            Grading_Is_Coordinate = true;
    }

    Matrix<Integer> GradGen;
    if (Grading_Is_Coordinate) {
        if (!ToCompute.test(ConeProperty::Approximate)) {
            GradGen = Generators;
            GradGen.exchange_columns(0, GradingCoordinate);  // we swap it into the first coordinate
        }
        else {  // we swap the grading into the first coordinate and approximate
            GradGen.resize(0, dim);
            for (size_t i = 0; i < Generators.nr_of_rows(); ++i) {
                vector<Integer> gg = Generators[i];
                swap(gg[0], gg[GradingCoordinate]);
                list<vector<Integer> > approx;
                approx_simplex(gg, approx, 1);
                GradGen.append(Matrix<Integer>(approx));
            }
        }
    }
    else {  // to avoid coordinate transformations, we prepend the degree as the first coordinate
        GradGen.resize(0, dim + 1);
        for (size_t i = 0; i < Generators.nr_of_rows(); ++i) {
            vector<Integer> gg(dim + 1);
            for (size_t j = 0; j < dim; ++j)
                gg[j + 1] = Generators[i][j];
            gg[0] = v_scalar_product(Generators[i], GradForApprox);
            // cout << gg;
            if (ToCompute.test(ConeProperty::Approximate)) {
                list<vector<Integer> > approx;
                approx_simplex(gg, approx, 1);
                GradGen.append(Matrix<Integer>(approx));
            }
            else
                GradGen.append(gg);
        }
    }

    // data prepared, bow nthe computation

    Matrix<Integer> CongOri = BasisChange.getCongruencesMatrix();
    vector<Integer> GradingOnPolytope;  // used in the inhomogeneous case for Hilbert function
    if (inhomogeneous && isComputed(ConeProperty::Grading) && ToCompute.test(ConeProperty::HilbertSeries))
        GradingOnPolytope = Grading;

    Matrix<Integer> Raw(0, GradGen.nr_of_columns());  // result is returned in this matrix

    if (ToCompute.test(ConeProperty::Approximate)) {
        if (verbose)
            verboseOutput() << "Computing lattice points by approximation" << endl;
        Cone<Integer> HelperCone(InputType::cone, GradGen);
        HelperCone.ApproximatedCone =
            &(*this);                        // we will pass this information to the Full_Cone that computes the lattice points.
        HelperCone.is_approximation = true;  // It allows us to discard points outside *this as quickly as possible
        HelperCone.compute(ConeProperty::Deg1Elements, ConeProperty::PrimalMode);
        Raw = HelperCone.getDeg1ElementsMatrix();
    }
    else {
        if (verbose) {
            string activity = "Computing ";
            if (ToCompute.test(ConeProperty::NumberLatticePoints))
                activity = "counting ";
            verboseOutput() << activity + "lattice points by project-and-lift" << endl;
        }
        Matrix<Integer> Supps, Equs, Congs;
        if (Grading_Is_Coordinate) {
            Supps = SupportHyperplanes;
            Supps.exchange_columns(0, GradingCoordinate);
            Equs = BasisChange.getEquationsMatrix();
            Equs.exchange_columns(0, GradingCoordinate);
            Congs = CongOri;
            Congs.exchange_columns(0, GradingCoordinate);
            if (GradingOnPolytope.size() > 0)
                swap(GradingOnPolytope[0], GradingOnPolytope[GradingCoordinate]);
        }
        else {
            Supps = SupportHyperplanes;
            Supps.insert_column(0, 0);
            Equs = BasisChange.getEquationsMatrix();
            Equs.insert_column(0, 0);
            vector<Integer> ExtraEqu(Equs.nr_of_columns());
            ExtraEqu[0] = -1;
            for (size_t i = 0; i < Grading.size(); ++i)
                ExtraEqu[i + 1] = Grading[i];
            Equs.append(ExtraEqu);
            Congs = CongOri;
            Congs.insert_column(0, 0);
            if (GradingOnPolytope.size() > 0) {
                GradingOnPolytope.insert(GradingOnPolytope.begin(), 0);
            }
        }
        Supps.append(Equs);  // we must add the equations as pairs of inequalities
        Equs.scalar_multiplication(-1);
        Supps.append(Equs);
        project_and_lift(ToCompute, Raw, GradGen, Supps, Congs, GradingOnPolytope);
    }

    // computation done. It remains to restore the old coordinates

    HilbertBasis = Matrix<Integer>(0, dim);
    Deg1Elements = Matrix<Integer>(0, dim);
    ModuleGenerators = Matrix<Integer>(0, dim);

    if (Grading_Is_Coordinate)
        Raw.exchange_columns(0, GradingCoordinate);

    if (Grading_Is_Coordinate && CongOri.nr_of_rows() == 0) {
        if (inhomogeneous)
            ModuleGenerators.swap(Raw);
        else
            Deg1Elements.swap(Raw);
    }
    else {
        if (CongOri.nr_of_rows() > 0 && verbose && ToCompute.test(ConeProperty::Approximate))
            verboseOutput() << "Sieving lattice points by congruences" << endl;
        for (size_t i = 0; i < Raw.nr_of_rows(); ++i) {
            vector<Integer> rr;
            if (Grading_Is_Coordinate) {
                swap(rr, Raw[i]);
            }
            else {
                rr.resize(dim);  // remove the prepended grading
                for (size_t j = 0; j < dim; ++j)
                    rr[j] = Raw[i][j + 1];
            }
            if (ToCompute.test(ConeProperty::Approximate) &&
                !CongOri.check_congruences(rr))  // already checked with project_and_lift
                continue;
            if (inhomogeneous) {
                ModuleGenerators.append(rr);
            }
            else
                Deg1Elements.append(rr);
        }
    }

    setWeights();
    if (inhomogeneous)
        ModuleGenerators.sort_by_weights(WeightsGrad, GradAbs);
    else
        Deg1Elements.sort_by_weights(WeightsGrad, GradAbs);

    if (!ToCompute.test(ConeProperty::NumberLatticePoints)) {
        if (!inhomogeneous)
            number_lattice_points = Deg1Elements.nr_of_rows();
        else
            number_lattice_points = ModuleGenerators.nr_of_rows();
    }

    setComputed(ConeProperty::NumberLatticePoints);  // always computed
    if (!inhomogeneous &&
        !ToCompute.test(ConeProperty::Deg1Elements))  // we have only counted, nothing more possible in the hom case
        return;

    if (inhomogeneous) {  // as in convert_polyhedron_to polytope of full_cone.cpp

        module_rank = number_lattice_points;
        setComputed(ConeProperty::ModuleRank);
        recession_rank = 0;
        setComputed(ConeProperty::RecessionRank);

        if (ToCompute.test(ConeProperty::HilbertBasis)) {  // we have computed the lattice points and not only counted them
            setComputed(ConeProperty::HilbertBasis);
            setComputed(ConeProperty::ModuleGenerators);
        }

        if (isComputed(ConeProperty::Grading)) {
            multiplicity = static_cast<unsigned long>(module_rank);  // of the recession cone;
            setComputed(ConeProperty::Multiplicity);
            if (ToCompute.test(ConeProperty::HilbertSeries) &&
                ToCompute.test(ConeProperty::Approximate)) {  // already done with project_and_lift
                try_Hilbert_Series_from_lattice_points(ToCompute);
            }
        }
    }
    else
        setComputed(ConeProperty::Deg1Elements);

    // setComputed(ConeProperty::Approximate);

    return;
}

//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::project_and_lift(const ConeProperties& ToCompute,
                                     Matrix<Integer>& Deg1,
                                     const Matrix<Integer>& Gens,
                                     const Matrix<Integer>& Supps,
                                     const Matrix<Integer>& Congs,
                                     const vector<Integer> GradingOnPolytope) {
    bool float_projection = ToCompute.test(ConeProperty::ProjectionFloat);
    bool count_only = ToCompute.test(ConeProperty::NumberLatticePoints);

    vector<dynamic_bitset> Ind;

    if (!is_parallelotope) {
        Ind = vector<dynamic_bitset>(Supps.nr_of_rows(), dynamic_bitset(Gens.nr_of_rows()));
        for (size_t i = 0; i < Supps.nr_of_rows(); ++i)
            for (size_t j = 0; j < Gens.nr_of_rows(); ++j)
                if (v_scalar_product(Supps[i], Gens[j]) == 0)
                    Ind[i][j] = true;
    }

    size_t rank = BasisChangePointed.getRank();

    Matrix<Integer> Verts;
    if (isComputed(ConeProperty::Generators)) {
        vector<key_t> choice = identity_key(Gens.nr_of_rows());  // Gens.max_rank_submatrix_lex();
        if (choice.size() >= dim)
            Verts = Gens.submatrix(choice);
    }

    vector<num_t> h_vec_pos, h_vec_neg;

    if (float_projection) {  // conversion to float inside project-and-lift
        // vector<Integer> Dummy;
        ProjectAndLift<Integer, MachineInteger> PL;
        if (!is_parallelotope)
            PL = ProjectAndLift<Integer, MachineInteger>(Supps, Ind, rank);
        else
            PL = ProjectAndLift<Integer, MachineInteger>(Supps, Pair, ParaInPair, rank);
        Matrix<MachineInteger> CongsMI;
        convert(CongsMI, Congs);
        PL.set_congruences(CongsMI);
        PL.set_grading_denom(convertTo<MachineInteger>(GradingDenom));
        vector<MachineInteger> GOPMI;
        convert(GOPMI, GradingOnPolytope);
        PL.set_grading(GOPMI);
        PL.set_verbose(verbose);
        PL.set_LLL(!ToCompute.test(ConeProperty::NoLLL));
        PL.set_no_relax(ToCompute.test(ConeProperty::NoRelax));
        PL.set_vertices(Verts);
        PL.compute(true, true, count_only);  // the first true for all_points, the second for float
        Matrix<MachineInteger> Deg1MI(0, Deg1.nr_of_columns());
        PL.put_eg1Points_into(Deg1MI);
        convert(Deg1, Deg1MI);
        number_lattice_points = PL.getNumberLatticePoints();
        PL.get_h_vectors(h_vec_pos, h_vec_neg);
    }
    else {
        if (change_integer_type) {
            Matrix<MachineInteger> Deg1MI(0, Deg1.nr_of_columns());
            // Matrix<MachineInteger> GensMI;
            Matrix<MachineInteger> SuppsMI;
            try {
                // convert(GensMI,Gens);
                convert(SuppsMI, Supps);
                MachineInteger GDMI = convertTo<MachineInteger>(GradingDenom);
                ProjectAndLift<MachineInteger, MachineInteger> PL;
                if (!is_parallelotope)
                    PL = ProjectAndLift<MachineInteger, MachineInteger>(SuppsMI, Ind, rank);
                else
                    PL = ProjectAndLift<MachineInteger, MachineInteger>(SuppsMI, Pair, ParaInPair, rank);
                Matrix<MachineInteger> CongsMI;
                convert(CongsMI, Congs);
                PL.set_congruences(CongsMI);
                PL.set_grading_denom(GDMI);
                vector<MachineInteger> GOPMI;
                convert(GOPMI, GradingOnPolytope);
                PL.set_grading(GOPMI);
                PL.set_verbose(verbose);
                PL.set_no_relax(ToCompute.test(ConeProperty::NoRelax));
                PL.set_LLL(!ToCompute.test(ConeProperty::NoLLL));
                Matrix<MachineInteger> VertsMI;
                convert(VertsMI, Verts);
                PL.set_vertices(VertsMI);
                PL.compute(true, false, count_only);
                PL.put_eg1Points_into(Deg1MI);
                number_lattice_points = PL.getNumberLatticePoints();
                PL.get_h_vectors(h_vec_pos, h_vec_neg);
            } catch (const ArithmeticException& e) {
                if (verbose) {
                    verboseOutput() << e.what() << endl;
                    verboseOutput() << "Restarting with a bigger type." << endl;
                }
                change_integer_type = false;
            }
            if (change_integer_type) {
                convert(Deg1, Deg1MI);
            }
        }

        if (!change_integer_type) {
            ProjectAndLift<Integer, Integer> PL;
            if (!is_parallelotope)
                PL = ProjectAndLift<Integer, Integer>(Supps, Ind, rank);
            else
                PL = ProjectAndLift<Integer, Integer>(Supps, Pair, ParaInPair, rank);
            PL.set_congruences(Congs);
            PL.set_grading_denom(GradingDenom);
            PL.set_grading(GradingOnPolytope);
            PL.set_verbose(verbose);
            PL.set_no_relax(ToCompute.test(ConeProperty::NoRelax));
            PL.set_LLL(!ToCompute.test(ConeProperty::NoLLL));
            PL.set_vertices(Verts);
            PL.compute(true, false, count_only);
            PL.put_eg1Points_into(Deg1);
            number_lattice_points = PL.getNumberLatticePoints();
            PL.get_h_vectors(h_vec_pos, h_vec_neg);
        }
    }

    if (ToCompute.test(ConeProperty::HilbertSeries) && isComputed(ConeProperty::Grading)) {
        make_Hilbert_series_from_pos_and_neg(h_vec_pos, h_vec_neg);
    }

    /* setComputed(ConeProperty::Projection);
    if(ToCompute.test(ConeProperty::NoRelax))
        setComputed(ConeProperty::NoRelax);
    if(ToCompute.test(ConeProperty::NoLLL))
        setComputed(ConeProperty::NoLLL);
    if(float_projection)
        setComputed(ConeProperty::ProjectionFloat);*/

    if (verbose)
        verboseOutput() << "Project-and-lift complete" << endl
                        << "------------------------------------------------------------" << endl;
}

//---------------------------------------------------------------------------
template <typename Integer>
bool Cone<Integer>::check_parallelotope() {
    if (dim <= 1)
        return false;

    vector<Integer> Grad;  // copy of Grading or Dehomogenization

    if (inhomogeneous) {
        Grad = Dehomogenization;
    }
    else {
        if (!isComputed(ConeProperty::Grading))
            return false;
        Grad = Grading;
    }

    Grading_Is_Coordinate = false;
    size_t nr_nonzero = 0;
    for (size_t i = 0; i < Grad.size(); ++i) {
        if (Grad[i] != 0) {
            nr_nonzero++;
            GradingCoordinate = i;
        }
    }
    if (nr_nonzero == 1) {
        if (Grad[GradingCoordinate] == 1)
            Grading_Is_Coordinate = true;
    }
    if (!Grading_Is_Coordinate)
        return false;
    if (Equations.nr_of_rows() > 0)
        return false;

    Inequalities.sort_lex();

    Matrix<Integer> Supps(Inequalities);
    if (inhomogeneous)
        Supps.remove_row(Grad);

    size_t dim = Supps.nr_of_columns() - 1;  // affine dimension
    if (Supps.nr_of_rows() != 2 * dim)
        return false;
    Pair.resize(2 * dim);
    ParaInPair.resize(2 * dim);
    for (size_t i = 0; i < 2 * dim; ++i) {
        Pair[i].resize(dim);
        Pair[i].reset();
        ParaInPair[i].resize(dim);
        ParaInPair[i].reset();
    }

    vector<bool> done(2 * dim);
    Matrix<Integer> M2(2, dim + 1), M3(3, dim + 1);
    M3[2] = Grad;
    size_t pair_counter = 0;

    vector<key_t> Supp_1;  // to find antipodal vertices
    vector<key_t> Supp_2;

    for (size_t i = 0; i < 2 * dim; ++i) {
        if (done[i])
            continue;
        bool parallel_found = false;
        M2[0] = Supps[i];
        M3[0] = Supps[i];
        size_t j = i + 1;
        for (; j < 2 * dim; ++j) {
            if (done[j])
                continue;
            M2[1] = Supps[j];
            if (M2.rank() < 2)
                continue;
            M3[1] = Supps[j];
            if (M3.rank() == 3)
                continue;
            else {
                parallel_found = true;
                done[j] = true;
                break;
            }
        }
        if (!parallel_found)
            return false;
        Supp_1.push_back(i);
        Supp_2.push_back(j);
        Pair[i][pair_counter] = true;        // Pair[i] indicates to which pair of parallel facets rge facet i belongs
        Pair[j][pair_counter] = true;        // ditto for face j
        ParaInPair[j][pair_counter] = true;  // face i is "distinguished" and gace j is its parallel (and marked as such)
        pair_counter++;
    }

    Matrix<Integer> v1 = Supps.submatrix(Supp_1).kernel(false);  // opposite vertices
    Matrix<Integer> v2 = Supps.submatrix(Supp_2).kernel(false);
    Integer MinusOne = -1;
    if (v_scalar_product(v1[0], Grad) == 0)
        return false;
    if (v_scalar_product(v2[0], Grad) == 0)
        return false;
    if (v_scalar_product(v1[0], Grad) < 0)
        v_scalar_multiplication(v1[0], MinusOne);
    if (v_scalar_product(v2[0], Grad) < 0)
        v_scalar_multiplication(v2[0], MinusOne);
    if (v1.nr_of_rows() != 1 || v2.nr_of_rows() != 1)
        return false;
    for (unsigned int& i : Supp_1) {
        if (!(v_scalar_product(Supps[i], v2[0]) > 0))
            return false;
    }
    for (unsigned int& i : Supp_2) {
        if (!(v_scalar_product(Supps[i], v1[0]) > 0))
            return false;
    }

    // we have found opposite vertices

    return true;
}

//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::compute_volume(ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::Volume))
        return;
    if (!inhomogeneous) {
        if (BasisMaxSubspace.nr_of_rows() > 0)
            throw NotComputableException("Volume not computable for polyhedra containing an affine space of dim > 0");
        volume = multiplicity;
        setComputed(ConeProperty::Volume);
        euclidean_volume = mpq_to_nmz_float(volume) * euclidean_corr_factor();
        setComputed(ConeProperty::EuclideanVolume);
        return;
    }

    /*
    compute(ConeProperty::Generators);
    compute(ConeProperty::AffineDim);

    if (affine_dim <= 0) {
        if (affine_dim == -1) {
            volume = 0;
            euclidean_volume = 0;
        }
        else {
            volume = 1;
            euclidean_volume = 1.0;
        }
        setComputed(ConeProperty::Volume);
        setComputed(ConeProperty::EuclideanVolume);
        return;
    }

    if (BasisMaxSubspace.nr_of_rows() > 0)
        throw NotComputableException("Volume not computable for polyhedra containing an affine space of dim > 0");

    for (size_t i = 0; i < Generators.nr_of_rows(); ++i) {
        if (v_scalar_product(Generators[i], Dehomogenization) == 0)
            throw NotComputableException("Volume not computable for unbounded polyhedra");
    }
    InputMap<Integer> DefVolCone;
    if (!BasisChangePointed.IsIdentity())
        DefVolCone[Type::lattice] = get_sublattice_internal().getEmbeddingMatrix();
    DefVolCone[Type::grading] = Dehomogenization;
    if (isComputed(ConeProperty::SupportHyperplanes))
        DefVolCone[Type::inequalities] = SupportHyperplanes;
    if (isComputed(ConeProperty::ExtremeRays))
        DefVolCone[Type::cone] = VerticesOfPolyhedron;
    else
        DefVolCone[Type::cone] = Generators;
    Cone<Integer> VolCone(DefVolCone);
    if (ToCompute.test(ConeProperty::Descent))
        VolCone.compute(ConeProperty::Volume, ConeProperty::Descent);
    if (ToCompute.test(ConeProperty::SignedDec))
        VolCone.compute(ConeProperty::Volume, ConeProperty::SignedDec);
    else {
        if (ToCompute.test(ConeProperty::NoDescent))
            VolCone.compute(ConeProperty::Volume, ConeProperty::NoDescent);
        else
            VolCone.compute(ConeProperty::Volume);
    }
    volume = VolCone.getVolume();
    euclidean_volume = VolCone.getEuclideanVolume();
    setComputed(ConeProperty::Volume);
    setComputed(ConeProperty::EuclideanVolume);
    return; */
}

//---------------------------------------------------------------------------
template <typename Integer>
nmz_float Cone<Integer>::euclidean_corr_factor() {
    // Though this function can now only be called with GradingDenom=1
    // but variable not yet removed
    // In the inhomogeneous case we may have to set it:

    if (get_rank_internal() - BasisMaxSubspace.nr_of_rows() == 0)
        return 1.0;

    Integer GradingDenom = 1;

    vector<Integer> Grad;
    if (inhomogeneous)
        Grad = Dehomogenization;
    else
        Grad = Grading;

    // First we find a simplex in our space as quickly as possible

    Matrix<Integer> Simplex = BasisChangePointed.getEmbeddingMatrix();
    // Matrix<Integer> Simplex=Generators.submatrix(Generators.max_rank_submatrix_lex()); -- numerically bad !!!!
    size_t n = Simplex.nr_of_rows();
    vector<Integer> raw_degrees = Simplex.MxV(Grad);

    size_t non_zero = 0;
    for (size_t i = 0; i < raw_degrees.size(); ++i)
        if (raw_degrees[i] != 0) {
            non_zero = i;
            break;
        }

    Integer MinusOne = -1;
    if (raw_degrees[non_zero] < 0) {
        v_scalar_multiplication(Simplex[non_zero], MinusOne);  // makes this degree > 0
        raw_degrees[non_zero] *= -1;
    }
    for (size_t i = 0; i < n; ++i) {
        if (raw_degrees[i] == 0)
            Simplex[i] = v_add(Simplex[i], Simplex[non_zero]);  // makes this degree > 0
        if (raw_degrees[i] < 0)
            v_scalar_multiplication(Simplex[i], MinusOne);  // ditto
    }

    vector<Integer> degrees = Simplex.MxV(Grad);

    // we compute the lattice normalized volume and later the euclidean volume
    // of the simplex defined by Simplex to get the correction factor
    Cone<Integer> VolCone(Type::cone, Simplex, Type::lattice, get_sublattice_internal().getEmbeddingMatrix(), Type::grading,
                          Matrix<Integer>(Grad));
    VolCone.setVerbose(false);
    ConeProperties VolComp;
    VolComp.set(ConeProperty::Multiplicity);
    VolComp.set(ConeProperty::NoBottomDec);
    VolComp.set(ConeProperty::NoGradingDenom);
    VolComp.set(ConeProperty::NoDescent);
    VolComp.set(ConeProperty::NoSignedDec);
    VolCone.compute(VolComp);
    mpq_class norm_vol_simpl = VolCone.getMultiplicity();
    // lattice normalized volume of our Simplex

    // now the euclidean volume
    Matrix<nmz_float> Bas;
    convert(Bas, Simplex);
    for (size_t i = 0; i < n; ++i) {
        v_scalar_division(Bas[i], convertTo<nmz_float>(degrees[i]));
        v_scalar_multiplication(Bas[i], convertTo<nmz_float>(GradingDenom));
    }
    // choose an origin, namely Bas[0]
    Matrix<nmz_float> Bas1(n - 1, dim);
    for (size_t i = 1; i < n; ++i)
        for (size_t j = 0; j < dim; ++j)
            Bas1[i - 1][j] = Bas[i][j] - Bas[0][j];

    // orthogonalize Bas1
    Matrix<double> G(n, dim);
    Matrix<double> M(n, n);
    Bas1.GramSchmidt(G, M, 0, n - 1);
    // compute euclidean volume
    nmz_float eucl_vol_simpl = 1;
    for (size_t i = 0; i < n - 1; ++i)
        eucl_vol_simpl *= sqrt(v_scalar_product(G[i], G[i]));
    // so far the euclidean volume of the paralleotope
    nmz_float fact;
    convert(fact, nmz_factorial((long)n - 1));
    // now the volume of the simplex
    eucl_vol_simpl /= fact;

    // now the correction
    nmz_float corr_factor = eucl_vol_simpl / mpq_to_nmz_float(norm_vol_simpl);
    return corr_factor;
}

//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::compute_projection(ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::ProjectCone))
        return;

    if (projection_coord_indicator.size() == 0)
        throw BadInputException("input projection_coordinates not set");

    if (projection_coord_indicator == vector<bool>(dim))
        throw BadInputException("Projection to zero coordinates make no sense");

    if (projection_coord_indicator == vector<bool>(dim, true))
        throw BadInputException("Projection to all coordinates make no sense");

    vector<Integer> GradOrDehom, GradOrDehomProj;
    if (inhomogeneous)
        GradOrDehom = Dehomogenization;
    else if (isComputed(ConeProperty::Grading))
        GradOrDehom = Grading;
    for (size_t i = 0; i < GradOrDehom.size(); ++i) {
        if (!projection_coord_indicator[i]) {
            if (GradOrDehom[i] != 0)
                throw BadInputException("Grading or Dehomogenization not compatible with projection");
        }
        else
            GradOrDehomProj.push_back(GradOrDehom[i]);
    }

    // if (isComputed(ConeProperty::Generators))
    compute_projection_from_gens(GradOrDehomProj, ToCompute);
    // else
    //    compute_projection_from_constraints(GradOrDehomProj, ToCompute); // out of use

    setComputed(ConeProperty::ProjectCone);
}
//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::compute_projection_from_gens(const vector<Integer>& GradOrDehomProj, ConeProperties& ToCompute) {
    compute_generators(ToCompute);
    Matrix<Integer> GensProj = Generators.select_columns(projection_coord_indicator);
    Matrix<Integer> Help = BasisMaxSubspace.select_columns(projection_coord_indicator);
    GensProj.append(Help);
    Integer MinusOne = -1;
    Help.scalar_multiplication(MinusOne);
    GensProj.append(Help);

    InputMap<Integer> ProjInput;
    ProjInput[Type::cone] = GensProj;
    if (GradOrDehomProj.size() > 0) {
        if (inhomogeneous)
            ProjInput[Type::dehomogenization] = GradOrDehomProj;
        else
            ProjInput[Type::grading] = GradOrDehomProj;
    }

    if (ProjCone != NULL)
        delete ProjCone;

    ProjCone = new Cone<Integer>(ProjInput);
    if (verbose)
        verboseOutput() << "Computing projection from generators" << endl;
    ProjCone->compute(ConeProperty::SupportHyperplanes, ConeProperty::ExtremeRays);
}

// The next routine is too complicated
// it would only make sense without the computation of the generators
//---------------------------------------------------------------------------
/*
template <typename Integer>
void Cone<Integer>::compute_projection_from_constraints(const vector<Integer>& GradOrDehomProj, ConeProperties& ToCompute) {
    compute_generators(ToCompute);
    Matrix<Integer> Gens = Generators.selected_columns_first(projection_coord_indicator);
    Matrix<Integer> ReorderedBasis = BasisMaxSubspace.selected_columns_first(projection_coord_indicator);
    Gens.append(ReorderedBasis);

    Matrix<Integer> Supps = SupportHyperplanes.selected_columns_first(projection_coord_indicator);
    Matrix<Integer> ReorderedEquations = BasisChange.getEquationsMatrix().selected_columns_first(projection_coord_indicator);
    Supps.append(ReorderedEquations);
    Integer MinusOne = -1;
    ReorderedEquations.scalar_multiplication(MinusOne);
    Supps.append(ReorderedEquations);

    vector<dynamic_bitset> Ind;

    Ind = vector<dynamic_bitset>(Supps.nr_of_rows(), dynamic_bitset(Gens.nr_of_rows()));
    for (size_t i = 0; i < Supps.nr_of_rows(); ++i)
        for (size_t j = 0; j < Gens.nr_of_rows(); ++j)
            if (v_scalar_product(Supps[i], Gens[j]) == 0)
                Ind[i][j] = true;

    size_t proj_dim = 0;
    for (size_t i = 0; i < projection_coord_indicator.size(); ++i)
        if (projection_coord_indicator[i])
            proj_dim++;

    ProjectAndLift<Integer, Integer> PL;
    PL = ProjectAndLift<Integer, Integer>(Supps, Ind, BasisChangePointed.getRank());
    if (verbose)
        verboseOutput() << "Computing constraints of projection" << endl;
    PL.set_verbose(verbose);
    PL.compute_only_projection(proj_dim);
    Matrix<Integer> SuppsProj, EqusProj;
    PL.putSuppsAndEqus(SuppsProj, EqusProj, proj_dim);
    if (SuppsProj.nr_of_rows() == 0)
        SuppsProj.append(vector<Integer>(SuppsProj.nr_of_columns(), 0));  // to avoid completely empty input matrices
    if (EqusProj.nr_of_rows() == 0)
        EqusProj.append(vector<Integer>(EqusProj.nr_of_columns(), 0));

    InputMap<Integer> ProjInput;
    if (GradOrDehomProj.size() > 0) {
        if (inhomogeneous)
            ProjInput[Type::dehomogenization] = GradOrDehomProj;
        else
            ProjInput[Type::grading] = GradOrDehomProj;
    }
    ProjInput[Type::inequalities] = SuppsProj;
    ProjInput[Type::equations] = EqusProj;

    Matrix<Integer> GensProj = Generators.select_columns(projection_coord_indicator);
    Matrix<Integer> BasHelp = BasisMaxSubspace.select_columns(projection_coord_indicator);
    GensProj.append(BasHelp);
    BasHelp.scalar_multiplication(MinusOne);
    GensProj.append(BasHelp);
    ProjInput[Type::cone] = GensProj;

    ProjCone = new Cone<Integer>(ProjInput);
    ProjCone->setRenf(RenfSharedPtr);
    ProjCone->compute(ConeProperty::SupportHyperplanes, ConeProperty::ExtremeRays);
}

#ifdef ENFNORMALIZ
template <>
void Cone<renf_elem_class>::compute_projection_from_constraints(const vector<renf_elem_class>& GradOrDehomProj,
                                                                ConeProperties& ToCompute) {
    assert(false);
}
#endif

*/

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::try_signed_dec(ConeProperties& ToCompute) {
    if (inhomogeneous)  // in this case multiplicity defined algebraically, not as the volume of a polytope
        return;

    if (using_renf<Integer>())  // not yet implemented for renf
        return;

    bool something_to_do = (!isComputed(ConeProperty::Multiplicity) && ToCompute.test(ConeProperty::Multiplicity)) ||
                           (!isComputed(ConeProperty::Integral) && ToCompute.test(ConeProperty::Integral)) ||
                           (!isComputed(ConeProperty::VirtualMultiplicity) && ToCompute.test(ConeProperty::VirtualMultiplicity));

    if (!something_to_do)
        return;

    bool do_integral = ToCompute.test(ConeProperty::Integral) || ToCompute.test(ConeProperty::VirtualMultiplicity);

    if (ToCompute.test(ConeProperty::NoSignedDec) || ToCompute.test(ConeProperty::Descent) ||
        ToCompute.test(ConeProperty::Symmetrize))  // user wants something different
        return;

    bool do_multiplicity_differently = false;

    if ((ToCompute.test(ConeProperty::HilbertSeries) || ToCompute.test(ConeProperty::WeightedEhrhartSeries) ||
         ToCompute.test(ConeProperty::VirtualMultiplicity) || ToCompute.test(ConeProperty::Integral) ||
         ToCompute.test(ConeProperty::Triangulation) || ToCompute.test(ConeProperty::StanleyDec) ||
         ToCompute.test(ConeProperty::TriangulationDetSum) || ToCompute.test(ConeProperty::TriangulationSize)) &&
        !do_integral)
        do_multiplicity_differently = true;  // casws in which we must use an ordinary triangulation

    if (do_multiplicity_differently && !do_integral)
        return;

    if (!ToCompute.test(ConeProperty::SignedDec)) {  // we use Descent by default if there are not too many facets
        if (Inequalities.nr_of_rows() > 2 * dim + 1 || Inequalities.nr_of_rows() <= BasisChangePointed.getRank())
            return;
    }

    if (Inequalities.nr_of_rows() == 0) {
        compute(ConeProperty::SupportHyperplanes);
        Inequalities = SupportHyperplanes;
        ToCompute.reset(is_Computed);
    }

    if (!ToCompute.test(ConeProperty::SignedDec) && Generators.nr_of_rows() > 0 &&
        Generators.nr_of_rows() < dim * Inequalities.nr_of_rows() / 3)
        return;  // ordinary triangulation can be expected to be faster

    if (BasisChangePointed.getRank() == 0) {  // we want to go through full_cone
        return;
    }

    if (ToCompute.test(ConeProperty::NoGradingDenom))
        compute(ConeProperty::Grading, ConeProperty::NoGradingDenom);
    else
        compute(ConeProperty::Grading);
    ToCompute.reset(is_Computed);

    if (ToCompute.test(ConeProperty::SupportHyperplanes) || ToCompute.test(ConeProperty::ExtremeRays) || do_integral) {
        compute_generators(ToCompute);
        ToCompute.reset(is_Computed);
    }
    if (!ToCompute.test(ConeProperty::SignedDec) && Generators.nr_of_rows() > 0 &&
        Generators.nr_of_rows() < dim * Inequalities.nr_of_rows() / 3)
        return;  // ordinary triangulation can be expected to be faster

    if (do_integral) {
        if (BasisMaxSubspace.nr_of_rows() > 0)
            throw NotComputableException("Integral not computable for polyhedra containing an affine space of dim > 0");
        if (IntData.getPolynomial() == "")
            throw BadInputException("Polynomial weight missing");
    }

    if (verbose)
        verboseOutput() << "Working with dual cone" << endl;

    if (change_integer_type) {
        try {
            try_signed_dec_inner<MachineInteger>(ToCompute);
        } catch (const ArithmeticException& e) {
            if (verbose) {
                verboseOutput() << e.what() << endl;
                verboseOutput() << "Restarting with a bigger type." << endl;
            }
            change_integer_type = false;
        }
    }

    if (!change_integer_type)
        try_signed_dec_inner<Integer>(ToCompute);
}

template <typename Integer>
template <typename IntegerFC>
void Cone<Integer>::try_signed_dec_inner(ConeProperties& ToCompute) {
    Matrix<IntegerFC> SupphypEmb;
    BasisChangePointed.convert_to_sublattice_dual(SupphypEmb, Inequalities);
    Full_Cone<IntegerFC> Dual(SupphypEmb);
    Dual.verbose = verbose;
    if (ToCompute.test(ConeProperty::FixedPrecision)) {
        if (decimal_digits > 0)
            Dual.decimal_digits = decimal_digits;
        else
            Dual.decimal_digits = 100;
        setComputed(ConeProperty::FixedPrecision);
    }
    if (ToCompute.test(ConeProperty::DistributedComp)) {
        block_size_hollow_tri = 500000;
    }
    Dual.block_size_hollow_tri = block_size_hollow_tri;  // for backward compatibility it is enough
    Dual.project_name = project_name;                    // to set block_size_hollow_tri;
    if (ToCompute.test(ConeProperty::NoGradingDenom))
        BasisChangePointed.convert_to_sublattice_dual_no_div(Dual.GradingOnPrimal, Grading);
    else
        BasisChangePointed.convert_to_sublattice_dual(Dual.GradingOnPrimal, Grading);
    if (ToCompute.test(ConeProperty::Multiplicity))
        Dual.do_multiplicity_by_signed_dec = true;
    if (ToCompute.test(ConeProperty::Integral))
        Dual.do_integral_by_signed_dec = true;
    if (ToCompute.test(ConeProperty::VirtualMultiplicity))
        Dual.do_virtual_multiplicity_by_signed_dec = true;
    if (ToCompute.test(ConeProperty::Integral) || ToCompute.test(ConeProperty::VirtualMultiplicity)) {
        Dual.Polynomial = getIntData().getPolynomial();
        if (!BasisChangePointed.IsIdentity())
            convert(Dual.Embedding, BasisChangePointed.getEmbeddingMatrix());
    }

    if (ToCompute.test(ConeProperty::SupportHyperplanes))
        Dual.include_dualization = true;

    Dual.compute();

    if (Dual.isComputed(ConeProperty::Multiplicity)) {
        if (Dual.multiplicity == 0) {
            if (verbose) {
                verboseOutput() << "SignedDec applied to polytope embedded into higher dimensional space." << endl;
                verboseOutput() << "Will be repeated after re-embdiing of polytope." << endl;
            }
            compute_generators(ToCompute);
            try_signed_dec_inner<IntegerFC>(ToCompute);
            return;
        }

        multiplicity = Dual.multiplicity;
        setComputed(ConeProperty::Multiplicity);
    }
    else {
        if (ToCompute.test(ConeProperty::Multiplicity))
            throw NotComputableException("Multiplicty not computable by signed decomposition");
    }

    if (Dual.isComputed(ConeProperty::Integral)) {
        Integral = Dual.Integral;
        getIntData().setIntegral(Dual.Integral);
        nmz_float help = Dual.RawEuclideanIntegral;
        help *= euclidean_corr_factor();
        getIntData().setEuclideanIntegral(help);
        setComputed(ConeProperty::Integral);
        setComputed(ConeProperty::EuclideanIntegral);
    }
    if (Dual.isComputed(ConeProperty::VirtualMultiplicity)) {
        VirtualMultiplicity = Dual.VirtualMultiplicity;
        getIntData().setVirtualMultiplicity(Dual.VirtualMultiplicity);
        setComputed(ConeProperty::VirtualMultiplicity);
    }

    ToCompute.reset(is_Computed);
    extract_data_dual(Dual, ToCompute);
}

//---------------------------------------------------------------------------
// This routine aims at the computation of multiplicities by better exploitation
// of unimodularity


template <typename Integer>
void Cone<Integer>::compute_rational_data(ConeProperties& ToCompute) {
    if (inhomogeneous || using_renf<Integer>())
        return;
    if (!ToCompute.test(ConeProperty::Multiplicity))
        return;
    if (!isComputed(ConeProperty::OriginalMonoidGenerators))
        return;
    if (internal_index == 1)  // This is the critical point: the external index
        return;               // divides all determinants computed.
                              // So no unimodularity if it is > 1.

    if (!isComputed(ConeProperty::Grading))  // The coordinate change below
        return;                              // could produce an implicit grading
                                             // that is not liftable

    if (BasisMaxSubspace.nr_of_rows() > 0)
        return;

    // We have generators and they span a proper sublattice
    // of the lattice in which we must compute.

    size_t nr_goals = ToCompute.goals().count();
    size_t nr_vol_goals = 1;  //  We have Multiplicity already
    if (ToCompute.test(ConeProperty::Volume))
        nr_vol_goals++;
    if (ToCompute.test(ConeProperty::SupportHyperplanes))  // they can be computed alongside
        nr_vol_goals++;
    if (ToCompute.test(ConeProperty::ExtremeRays))  // ditto
        nr_vol_goals++;
    if (nr_goals != nr_vol_goals)  // There is something else asked for that does not allow the
        return;                    // reduction of the lattice

    // So we want to compute only things for which we can pass to
    // a sublattice.

    if (verbose)
        verboseOutput() << "Computing copy of cone with lattice spanned by generators" << endl;

    Matrix<Integer> GradMat(Grading);
    Cone<Integer> D(Type::cone_and_lattice, Generators, Type::grading, GradMat, Type::inequalities, SupportHyperplanes);
    if (!isComputed(ConeProperty::SupportHyperplanes) && ToCompute.test(ConeProperty::SupportHyperplanes))
        D.compute(ConeProperty::Multiplicity, ConeProperty::SupportHyperplanes);
    else
        D.compute(ConeProperty::Multiplicity);

    if (D.isComputed(ConeProperty::SupportHyperplanes) && !isComputed(ConeProperty::SupportHyperplanes)) {
        swap(SupportHyperplanes, D.SupportHyperplanes);
        setComputed(ConeProperty::SupportHyperplanes);
    }
    if (D.isComputed(ConeProperty::ExtremeRays) && !isComputed(ConeProperty::ExtremeRays)) {
        Generators = D.Generators;  // to take reordering into account
        swap(D.ExtremeRays, ExtremeRays);
        ExtremeRaysRecCone = ExtremeRays;
        ExtremeRaysIndicator = D.ExtremeRaysIndicator;
        setComputed(ConeProperty::ExtremeRays);
    }
    if (!D.isComputed(ConeProperty::Multiplicity))
        return;
    mpq_class raw_mult;
    raw_mult = D.multiplicity;
    // cout << "MMM " << raw_mult << " " << raw_mult *convertTo<mpz_class>(internal_index) << endl;
    raw_mult *= convertTo<mpz_class>(internal_index);

    mpz_class large_grading_denom = convertTo<mpz_class>(D.GradingDenom);  // grading denom on small lattice --> large denom
    // cout << "Large " << large_grading_denom << endl;
    vector<Integer> test_grading = BasisChangePointed.to_sublattice_dual_no_div(Grading);
    mpz_class small_grading_denom = convertTo<mpz_class>(v_gcd(test_grading));  // the grading denom of the given grading
    if (ToCompute.test(ConeProperty::NoGradingDenom))                           // we make the official GradingDenom
        GradingDenom = 1;
    else
        GradingDenom = convertTo<Integer>(small_grading_denom);
    setComputed(ConeProperty::GradingDenom);
    // cout << "Small " << small_grading_denom << endl;
    for (size_t i = 0; i < D.getRank(); ++i)
        raw_mult /= large_grading_denom;
    raw_mult *= small_grading_denom;  // the usual correction, see comment in full_cone.cpp
    // At this point we have computed the multiplicity for the given grading
    if (!ToCompute.test(ConeProperty::NoGradingDenom)) {
        for (size_t i = 1; i < D.getRank(); ++i)  // now we must take care of the official grading denomionator
            raw_mult *= small_grading_denom;      // it comes in with the exponent = polytope dimension
    }
    multiplicity = raw_mult;
    setComputed(ConeProperty::Multiplicity);  // see comment in full_cone.cpp
    if (verbose)
        verboseOutput() << "Returning to original cone" << endl;
}

template <>
void Cone<renf_elem_class>::compute_rational_data(ConeProperties& ToCompute) {
    return;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::try_multiplicity_by_descent(ConeProperties& ToCompute) {
    if (inhomogeneous)  // in this case multiplicity defined algebraically, not as the volume of a polytope
        return;

    if (using_renf<Integer>())  // descent not usable for renf
        return;

    if (isComputed(ConeProperty::Multiplicity) || !ToCompute.test(ConeProperty::Multiplicity))  // nothing to do here
        return;

    if (ToCompute.test(ConeProperty::NoDescent) || ToCompute.test(ConeProperty::SignedDec) ||
        ToCompute.test(ConeProperty::Symmetrize))  // user wants something different
        return;

    if (ToCompute.test(ConeProperty::HilbertSeries) || ToCompute.test(ConeProperty::WeightedEhrhartSeries) ||
        ToCompute.test(ConeProperty::VirtualMultiplicity) || ToCompute.test(ConeProperty::Integral) ||
        ToCompute.test(ConeProperty::Triangulation) || ToCompute.test(ConeProperty::StanleyDec) ||
        ToCompute.test(ConeProperty::TriangulationDetSum) || ToCompute.test(ConeProperty::TriangulationSize) ||
        ToCompute.test(ConeProperty::Symmetrize))  // needs triangulation
        return;

    if (!ToCompute.test(ConeProperty::Descent)) {  // we use Descent by default if there are not too many facets
        if ((Generators.nr_of_rows() > 0 && Inequalities.nr_of_rows() > 3 * Generators.nr_of_rows()) ||
            Inequalities.nr_of_rows() <=
                BasisChangePointed.getRank())  // if we start from generators, descent is not used by default
            return;
    }

    if (ToCompute.test(ConeProperty::NoGradingDenom))
        compute(ConeProperty::SupportHyperplanes, ConeProperty::Grading, ConeProperty::NoGradingDenom);
    else
        compute(ConeProperty::SupportHyperplanes, ConeProperty::Grading);

    if (isComputed(ConeProperty::Multiplicity))  // can happen !!
        return;

    try_multiplicity_of_para(ToCompute);  // we try this first, even if Descent is set
    if (isComputed(ConeProperty::Multiplicity))
        return;

    if (BasisChangePointed.getRank() == 0) {  // we want to go through full_cone
        return;
    }

    if (verbose)
        verboseOutput() << "Multiplicity by descent in the face lattice" << endl;

    if (change_integer_type) {
        try {
            Matrix<MachineInteger> ExtremeRaysMI, SupportHyperplanesMI;
            vector<MachineInteger> GradingMI;
            BasisChangePointed.convert_to_sublattice(ExtremeRaysMI, ExtremeRays);
            BasisChangePointed.convert_to_sublattice_dual(SupportHyperplanesMI, SupportHyperplanes);
            if (ToCompute.test(ConeProperty::NoGradingDenom))
                BasisChangePointed.convert_to_sublattice_dual_no_div(GradingMI, Grading);
            else
                BasisChangePointed.convert_to_sublattice_dual(GradingMI, Grading);
            DescentSystem<MachineInteger> FF(ExtremeRaysMI, SupportHyperplanesMI, GradingMI);
            FF.set_verbose(verbose);
            FF.setExploitAutoms(ToCompute.test(ConeProperty::ExploitIsosMult));
            FF.compute();
            multiplicity = FF.getMultiplicity();
        } catch (const ArithmeticException& e) {
            if (verbose) {
                verboseOutput() << e.what() << endl;
                verboseOutput() << "Restarting with a bigger type." << endl;
            }
            change_integer_type = false;
        }
    }

    if (!change_integer_type) {
        DescentSystem<Integer> FF;
        if (BasisChangePointed.IsIdentity()) {
            vector<Integer> GradingEmb;
            if (ToCompute.test(ConeProperty::NoGradingDenom))
                GradingEmb = BasisChangePointed.to_sublattice_dual_no_div(Grading);
            else
                GradingEmb = BasisChangePointed.to_sublattice_dual(Grading);
            FF = DescentSystem<Integer>(ExtremeRays, SupportHyperplanes, GradingEmb, false);  // no swapping
        }
        else {
            Matrix<Integer> ExtremeRaysEmb, SupportHyperplanesEmb;
            vector<Integer> GradingEmb;
            ExtremeRaysEmb = BasisChangePointed.to_sublattice(ExtremeRays);
            SupportHyperplanesEmb = BasisChangePointed.to_sublattice_dual(SupportHyperplanes);
            if (ToCompute.test(ConeProperty::NoGradingDenom))
                GradingEmb = BasisChangePointed.to_sublattice_dual_no_div(Grading);
            else
                GradingEmb = BasisChangePointed.to_sublattice_dual(Grading);
            FF = DescentSystem<Integer>(ExtremeRaysEmb, SupportHyperplanesEmb, GradingEmb);
        }
        FF.set_verbose(verbose);
        FF.setExploitAutoms(ToCompute.test(ConeProperty::ExploitIsosMult));
        FF.compute();
        multiplicity = FF.getMultiplicity();
    }

    // now me must correct the multiplicity if NoGradingDenom is set,
    // namely multiply it by the GradingDenom
    // as in full_cone.cpp (see comment there)
    if (ToCompute.test(ConeProperty::NoGradingDenom)) {
        vector<Integer> test_grading = BasisChangePointed.to_sublattice_dual_no_div(Grading);
        Integer corr_factor = v_gcd(test_grading);
        multiplicity *= convertTo<mpz_class>(corr_factor);
    }

    setComputed(ConeProperty::Multiplicity);
    setComputed(ConeProperty::Descent);
    if (verbose)
        verboseOutput() << "Multiplicity by descent done" << endl;
}

template <>
void Cone<renf_elem_class>::try_multiplicity_by_descent(ConeProperties& ToCompute) {
    return;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::try_multiplicity_of_para(ConeProperties& ToCompute) {
    if ((using_renf<Integer>() || (!inhomogeneous && !ToCompute.test(ConeProperty::Multiplicity)) ||
         (inhomogeneous && !ToCompute.test(ConeProperty::Volume))) ||
        !check_parallelotope())
        return;

    // if(ToCompute.test(ConeProperty::Descent) || ToCompute.test(ConeProperty::SignedDec))
    //   return; // temporily for benchmarks

    Inequalities.remove_row(Dehomogenization);
    SupportHyperplanes = Inequalities;
    setComputed(ConeProperty::SupportHyperplanes);
    addition_constraints_allowed = true;
    setComputed(ConeProperty::MaximalSubspace);
    setComputed(ConeProperty::Sublattice);
    pointed = true;
    setComputed(ConeProperty::IsPointed);
    if (inhomogeneous) {
        affine_dim = dim - 1;
        setComputed(ConeProperty::AffineDim);
        recession_rank = 0;
        setComputed(ConeProperty::RecessionRank);
    }

    if (verbose)
        verboseOutput() << "Multiplicity/Volume of parallelotope ...";

    vector<Integer> Grad;

    if (inhomogeneous) {
        Grad = Dehomogenization;
    }
    else {
        Grad = Grading;
    }

    size_t polytope_dim = dim - 1;

    // find a corner
    // CornerKey lists the supphyps that meet in the corner
    // OppositeKey lists the respective parallels
    vector<key_t> CornerKey, OppositeKey;
    for (size_t pc = 0; pc < polytope_dim; ++pc) {
        for (size_t i = 0; i < 2 * polytope_dim; ++i) {
            if (Pair[i][pc] == true) {
                if (ParaInPair[i][pc] == false)
                    CornerKey.push_back(i);
                else
                    OppositeKey.push_back(i);
            }
        }
    }

    Matrix<Integer> Simplex(0, dim);
    vector<Integer> gen;
    gen = Inequalities.submatrix(CornerKey).kernel(false)[0];
    if (v_scalar_product(gen, Grad) < 0)
        v_scalar_multiplication<Integer>(gen, -1);
    Simplex.append(gen);
    for (size_t i = 0; i < polytope_dim; ++i) {
        vector<key_t> ThisKey = CornerKey;
        ThisKey[i] = OppositeKey[i];
        gen = SupportHyperplanes.submatrix(ThisKey).kernel(false)[0];
        if (v_scalar_product(gen, Grad) < 0)
            v_scalar_multiplication<Integer>(gen, -1);
        Simplex.append(gen);
    }

    if (Simplex.nr_of_rows() <= 1)
        return;

    Cone<Integer> VolCone(Type::cone, Simplex, Type::grading, Matrix<Integer>(Grad));
    VolCone.setVerbose(false);
    if (inhomogeneous || ToCompute.test(ConeProperty::NoGradingDenom))
        VolCone.compute(ConeProperty::Multiplicity, ConeProperty::NoGradingDenom);
    else
        VolCone.compute(ConeProperty::Multiplicity);
    mpq_class mult_or_vol = VolCone.getMultiplicity();
    mult_or_vol *= nmz_factorial((long)polytope_dim);
    if (!inhomogeneous) {
        multiplicity = mult_or_vol;
        setComputed(ConeProperty::Multiplicity);
        if (ToCompute.test(ConeProperty::Volume))
            volume = mult_or_vol;
    }
    else {
        volume = mult_or_vol;
    }

    if (ToCompute.test(ConeProperty::Volume)) {
        euclidean_volume = mpq_to_nmz_float(volume) * euclidean_corr_factor();
        setComputed(ConeProperty::Volume);
        setComputed(ConeProperty::EuclideanVolume);
    }

    if (verbose)
        verboseOutput() << "done" << endl;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::treat_polytope_as_being_hom_defined(ConeProperties ToCompute) {
    if (!inhomogeneous)
        return;

    if (using_renf<Integer>())
        return;

    if (ToCompute.intersection_with(treated_as_hom_props()).none())
        return;  // homogeneous treatment not necessary

    ConeProperties ToComputeFirst;
    ToComputeFirst.set(ConeProperty::Generators);
    ToComputeFirst.set(ConeProperty::SupportHyperplanes);
    ToComputeFirst.set(ConeProperty::ExtremeRays);
    ToComputeFirst.set(ConeProperty::KeepOrder, ToCompute.test(ConeProperty::KeepOrder));
    compute(ToComputeFirst);
    ToCompute.reset(is_Computed);

    bool empty_polytope = true;
    for (size_t i = 0; i < Generators.nr_of_rows(); ++i) {
        Integer test = v_scalar_product(Dehomogenization, Generators[i]);
        if (test <= 0)
            throw NotComputableException("At least one goal not computable for unbounded polyhedra.");
        if (test > 0)
            empty_polytope = false;
    }

    if (empty_polytope && Generators.nr_of_rows() > 0) {
        throw NotComputableException("At least obe goal  not computable for empty polytope with non-subspace recession cone.");
    }

    if (empty_polytope) {
        affine_dim = -1;
        setComputed(ConeProperty::AffineDim);
        volume = 0;
        euclidean_volume = 0;
        setComputed(ConeProperty::Volume);
        setComputed(ConeProperty::EuclideanVolume);
        ToCompute.reset(is_Computed);
    }

    Cone Hom(*this);  // make a copy and make it homogeneous
    Hom.Grading = Dehomogenization;
    Hom.setComputed(ConeProperty::Grading);
    Hom.Dehomogenization.resize(0);
    Hom.inhomogeneous = false;
    ConeProperties HomToCompute = ToCompute;
    HomToCompute.reset(ConeProperty::FaceLattice);  // better to do this in the
    HomToCompute.reset(ConeProperty::FVector);      // original inhomogeneous settimg
    HomToCompute.reset(ConeProperty::Incidence);    //

    HomToCompute.reset(ConeProperty::VerticesOfPolyhedron);                //
    HomToCompute.reset(ConeProperty::ModuleRank);                          //
    HomToCompute.reset(ConeProperty::RecessionRank);                       //  these 6 will be computed below
    HomToCompute.reset(ConeProperty::AffineDim);                           //             //
    HomToCompute.reset(ConeProperty::VerticesOfPolyhedron);                //
    HomToCompute.reset(ConeProperty::ModuleGenerators);                    //
    HomToCompute.reset(ConeProperty::ModuleGeneratorsOverOriginalMonoid);  //
    HomToCompute.reset(ConeProperty::HilbertBasis);                        // we definitely don't want this

    if (ToCompute.test(ConeProperty::HilbertBasis) || ToCompute.test(ConeProperty::ModuleRank) ||
        ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid))
        HomToCompute.set(ConeProperty::LatticePoints);  // ==> NoGradingDenom

    Hom.compute(HomToCompute);  // <--------------------------- Here we compute

    /* compute(ConeProperty::Sublattice);
    if (!isComputed(ConeProperty::Sublattice))
        throw FatalException("Could not compute sublattice");

    pass_to_pointed_quotient();*/

    if (Hom.isComputed(ConeProperty::Deg1Elements)) {
        swap(ModuleGenerators, Hom.Deg1Elements);
        setComputed(ConeProperty::HilbertBasis);
        setComputed(ConeProperty::ModuleGenerators);
        module_rank = ModuleGenerators.nr_of_rows();
        setComputed(ConeProperty::ModuleRank);
        number_lattice_points = module_rank;
        setComputed(ConeProperty::NumberLatticePoints);

        if (ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid)) {
            ModuleGeneratorsOverOriginalMonoid = ModuleGenerators;
            setComputed(ConeProperty::ModuleGeneratorsOverOriginalMonoid);
        }
    }

    if (Hom.isComputed(ConeProperty::NumberLatticePoints)) {  // sometimes computed from the Hilbert series
        number_lattice_points = Hom.number_lattice_points;
        setComputed(ConeProperty::NumberLatticePoints);
    }

    IntData = Hom.IntData;
    if (Hom.isComputed(ConeProperty::WeightedEhrhartSeries))
        setComputed(ConeProperty::WeightedEhrhartSeries);
    if (Hom.isComputed(ConeProperty::WeightedEhrhartQuasiPolynomial))
        setComputed(ConeProperty::WeightedEhrhartQuasiPolynomial);
    if (Hom.isComputed(ConeProperty::Integral))
        setComputed(ConeProperty::Integral);
    if (Hom.isComputed(ConeProperty::EuclideanIntegral))
        setComputed(ConeProperty::EuclideanIntegral);
    if (Hom.isComputed(ConeProperty::VirtualMultiplicity))
        setComputed(ConeProperty::VirtualMultiplicity);

    if (Hom.isComputed(ConeProperty::HilbertSeries)) {
        setComputed(ConeProperty::EhrhartSeries);
        swap(EhrSeries, Hom.HSeries);
    }

    if (Hom.isComputed(ConeProperty::HSOP))
        setComputed(ConeProperty::HSOP);

    if (Hom.isComputed(ConeProperty::Volume)) {
        volume = Hom.volume;
        setComputed(ConeProperty::Volume);
    }
    if (Hom.isComputed(ConeProperty::EuclideanVolume)) {
        euclidean_volume = Hom.euclidean_volume;
        setComputed(ConeProperty::EuclideanVolume);
    }

    if (!isComputed(ConeProperty::BasicTriangulation) && Hom.isComputed(ConeProperty::BasicTriangulation)) {
        swap(BasicTriangulation, Hom.BasicTriangulation);
        setComputed(ConeProperty::BasicTriangulation);
    }

    bool triang_computed = false;
    if (Hom.isComputed(ConeProperty::Triangulation)) {
        setComputed(ConeProperty::Triangulation);
        triang_computed = true;
    }
    if (Hom.isComputed(ConeProperty::PlacingTriangulation)) {
        setComputed(ConeProperty::Triangulation);
        triang_computed = true;
    }
    if (Hom.isComputed(ConeProperty::PullingTriangulation)) {
        setComputed(ConeProperty::Triangulation);
        triang_computed = true;
    }
    if (Hom.isComputed(ConeProperty::UnimodularTriangulation)) {
        setComputed(ConeProperty::UnimodularTriangulation);
        triang_computed = true;
    }
    if (Hom.isComputed(ConeProperty::LatticePointTriangulation)) {
        setComputed(ConeProperty::LatticePointTriangulation);
        triang_computed = true;
    }
    if (Hom.isComputed(ConeProperty::AllGeneratorsTriangulation)) {
        setComputed(ConeProperty::AllGeneratorsTriangulation);
        triang_computed = true;
    }
    if (triang_computed)
        swap(Triangulation, Hom.Triangulation);

    if (Hom.isComputed(ConeProperty::TriangulationSize)) {
        TriangulationSize = Hom.TriangulationSize;
        setComputed(ConeProperty::TriangulationSize);
    }
    if (Hom.isComputed(ConeProperty::TriangulationDetSum)) {
        TriangulationDetSum = Hom.TriangulationDetSum;
        setComputed(ConeProperty::TriangulationDetSum);
    }

    if (Hom.isComputed(ConeProperty::BasicTriangulation) || Hom.isComputed(ConeProperty::TriangulationSize) ||
        Hom.isComputed(ConeProperty::TriangulationDetSum)) {
        triangulation_is_nested = Hom.triangulation_is_nested;
        triangulation_is_partial = Hom.triangulation_is_partial;
        setComputed(ConeProperty::IsTriangulationPartial);
        setComputed(ConeProperty::IsTriangulationNested);
    }

    if (Hom.isComputed(ConeProperty::ConeDecomposition)) {
        setComputed(ConeProperty::ConeDecomposition);
    }

    if (Hom.isComputed(ConeProperty::StanleyDec)) {
        swap(StanleyDec, Hom.StanleyDec);
        setComputed(ConeProperty::StanleyDec);
    }

    if (Hom.isComputed(ConeProperty::ExcludedFaces)) {
        swap(ExcludedFaces, Hom.ExcludedFaces);
        setComputed(ConeProperty::ExcludedFaces);
    }

    if (Hom.isComputed(ConeProperty::CoveringFace)) {
        swap(CoveringFace, Hom.CoveringFace);
        setComputed(ConeProperty::CoveringFace);
    }

    if (Hom.isComputed(ConeProperty::IsEmptySemiOpen)) {
        empty_semiopen = Hom.empty_semiopen;
        setComputed(ConeProperty::IsEmptySemiOpen);
    }

    bool automs_computed = false;
    if (Hom.isComputed(ConeProperty::Automorphisms)) {
        setComputed(ConeProperty::Automorphisms);
        automs_computed = true;
    }
    if (Hom.isComputed(ConeProperty::RationalAutomorphisms)) {
        setComputed(ConeProperty::RationalAutomorphisms);
        automs_computed = true;
    }
    if (Hom.isComputed(ConeProperty::EuclideanAutomorphisms)) {
        setComputed(ConeProperty::EuclideanAutomorphisms);
        automs_computed = true;
    }
    if (Hom.isComputed(ConeProperty::CombinatorialAutomorphisms)) {
        setComputed(ConeProperty::CombinatorialAutomorphisms);
        automs_computed = true;
    }
    if (automs_computed) {
        Automs = Hom.Automs;
        Automs.VerticesPerms = Automs.ExtRaysPerms;  // make things inhomogeneous
        Automs.VerticesOrbits = Automs.ExtRaysOrbits;
        Automs.ExtRaysPerms.clear();
        Automs.ExtRaysOrbits.clear();
    }
    if (Hom.isComputed(ConeProperty::DualIncidence)) {
        swap(Hom.DualSuppHypInd, DualSuppHypInd);
        setComputed(ConeProperty::DualIncidence);
    }
    if (Hom.isComputed(ConeProperty::DualFaceLattice)) {
        swap(Hom.DualFaceLat, DualFaceLat);
        setComputed(ConeProperty::DualFaceLattice);
    }
    if (Hom.isComputed(ConeProperty::DualFVector)) {
        dual_f_vector = Hom.dual_f_vector;
        setComputed(ConeProperty::DualFVector);
    }
    if (Hom.isComputed(ConeProperty::FixedPrecision))
        setComputed(ConeProperty::FixedPrecision);

    recession_rank = Hom.BasisMaxSubspace.nr_of_rows();  // in our polytope case
    setComputed(ConeProperty::RecessionRank);
    if (!empty_polytope) {
        affine_dim = getRank() - 1;
        setComputed(ConeProperty::AffineDim);
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::try_Hilbert_Series_from_lattice_points(const ConeProperties& ToCompute) {
    if (!inhomogeneous || !isComputed(ConeProperty::ModuleGenerators) ||
        !(isComputed(ConeProperty::RecessionRank) && recession_rank == 0) || !isComputed(ConeProperty::Grading))
        return;

    multiplicity = static_cast<unsigned long>(ModuleGenerators.nr_of_rows());
    setComputed(ConeProperty::Multiplicity);

    if (!ToCompute.test(ConeProperty::HilbertSeries))
        return;

    vector<num_t> h_vec_pos(1), h_vec_neg;

    for (size_t i = 0; i < ModuleGenerators.nr_of_rows(); ++i) {
        long deg = convertToLong(v_scalar_product(Grading, ModuleGenerators[i]));
        if (deg >= 0) {
            if (deg >= (long)h_vec_pos.size())
                h_vec_pos.resize(deg + 1);
            h_vec_pos[deg]++;
        }
        else {
            deg *= -1;
            if (deg >= (long)h_vec_neg.size())
                h_vec_neg.resize(deg + 1);
            h_vec_neg[deg]++;
        }
    }

    /*cout << "Pos " << h_vec_pos;
    cout << "Neg " << h_vec_neg;*/

    make_Hilbert_series_from_pos_and_neg(h_vec_pos, h_vec_neg);
}

template <>
void Cone<renf_elem_class>::try_Hilbert_Series_from_lattice_points(const ConeProperties& ToCompute) {
    assert(false);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::make_Hilbert_series_from_pos_and_neg(const vector<num_t>& h_vec_pos, const vector<num_t>& h_vec_neg) {
    if (verbose)
        verboseOutput() << "Computing Hilbert series from lattice points" << endl;

    vector<num_t> hv = h_vec_pos;
    long raw_shift = 0;
    if (h_vec_neg.size() > 0) {  // insert negative degrees
        raw_shift = -(h_vec_neg.size() - 1);
        for (size_t j = 1; j < h_vec_neg.size(); ++j)
            hv.insert(hv.begin(), h_vec_neg[j]);
    }

    HSeries.add(hv, vector<denom_t>());
    HSeries.setShift(raw_shift);
    HSeries.adjustShift();
    HSeries.simplify();
    setComputed(ConeProperty::HilbertSeries);
    // setComputed(ConeProperty::ExplicitHilbertSeries);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::make_face_lattice(const ConeProperties& ToCompute) {
    bool something_to_do_primal = (ToCompute.test(ConeProperty::FaceLattice) && !isComputed(ConeProperty::FaceLattice)) ||
                                  (ToCompute.test(ConeProperty::FVector) && !isComputed(ConeProperty::FVector)) ||
                                  (ToCompute.test(ConeProperty::Incidence) && !isComputed(ConeProperty::Incidence));

    bool something_to_do_dual = (ToCompute.test(ConeProperty::DualFaceLattice) && !isComputed(ConeProperty::DualFaceLattice)) ||
                                (ToCompute.test(ConeProperty::DualFVector) && !isComputed(ConeProperty::DualFVector)) ||
                                (ToCompute.test(ConeProperty::DualIncidence) && !isComputed(ConeProperty::DualIncidence));

    if (!something_to_do_dual && !something_to_do_primal)
        return;

    if (something_to_do_dual && something_to_do_primal)
        throw BadInputException("Only one of primal or dual face lattice/f-vector/incidence allowed");

    if (something_to_do_dual && inhomogeneous)
        throw BadInputException("Dual face lattice/f-vector/incidence not computable for inhomogeneous input");

    if (ToCompute.test(ConeProperty::KeepOrder))
        compute(ConeProperty::ExtremeRays, ConeProperty::SupportHyperplanes, ConeProperty::KeepOrder);
    else
        compute(ConeProperty::ExtremeRays,
                ConeProperty::SupportHyperplanes);  // both necessary
                                                    // since ExtremeRays can be comuted without SupportHyperplanes
                                                    // if the cone is not full dimensional

    bool only_f_vector =
        (something_to_do_primal && !ToCompute.test(ConeProperty::FaceLattice) && !ToCompute.test(ConeProperty::Incidence)) ||
        (something_to_do_dual && !ToCompute.test(ConeProperty::DualFaceLattice) && !ToCompute.test(ConeProperty::DualIncidence));

    bool dualize = only_f_vector &&
                   ((something_to_do_primal && ExtremeRays.nr_of_rows() < SupportHyperplanes.nr_of_rows()) ||
                    (something_to_do_dual && ExtremeRays.nr_of_rows() > SupportHyperplanes.nr_of_rows())) &&
                   face_codim_bound < 0;

    if ((something_to_do_primal && !dualize) || (something_to_do_dual && dualize) || inhomogeneous) {
        make_face_lattice_primal(ToCompute);
    }
    else {
        make_face_lattice_dual(ToCompute);
    }
}
//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::make_face_lattice_primal(const ConeProperties& ToCompute) {
    if (verbose && ToCompute.test(ConeProperty::DualFVector))
        verboseOutput() << "Going to the primal side for the dual f-vector" << endl;
    if (verbose)
        verboseOutput() << "Computing incidence/face lattice/f-vector ... " << endl;

    Matrix<Integer> SuppHypPointed;
    BasisChangePointed.convert_to_sublattice_dual(SuppHypPointed, SupportHyperplanes);
    Matrix<Integer> VertOfPolPointed;
    BasisChangePointed.convert_to_sublattice(VertOfPolPointed, VerticesOfPolyhedron);
    Matrix<Integer> ExtrRCPointed;
    BasisChangePointed.convert_to_sublattice(ExtrRCPointed, ExtremeRaysRecCone);
    FaceLattice<Integer> FL(SuppHypPointed, VertOfPolPointed, ExtrRCPointed, inhomogeneous);

    if (ToCompute.test(ConeProperty::FaceLattice) || ToCompute.test(ConeProperty::FVector) ||
        ToCompute.test(ConeProperty::DualFVector))
        FL.compute(face_codim_bound, verbose, change_integer_type);

    if (ToCompute.test(ConeProperty::Incidence)) {
        FL.get(SuppHypInd);
        setComputed(ConeProperty::Incidence);
    }
    if (ToCompute.test(ConeProperty::FaceLattice)) {
        FL.get(FaceLat);
        setComputed(ConeProperty::FaceLattice);
    }
    if (ToCompute.test(ConeProperty::FaceLattice) || ToCompute.test(ConeProperty::FVector) ||
        ToCompute.test(ConeProperty::DualFVector)) {
        vector<size_t> prel_f_vector = FL.getFVector();
        if (!ToCompute.test(ConeProperty::DualFVector)) {
            f_vector = prel_f_vector;
            setComputed(ConeProperty::FVector);
        }
        else {
            dual_f_vector.resize(prel_f_vector.size());
            for (size_t i = 0; i < prel_f_vector.size(); ++i)
                dual_f_vector[i] = prel_f_vector[prel_f_vector.size() - 1 - i];
            setComputed(ConeProperty::DualFVector);
        }
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::make_face_lattice_dual(const ConeProperties& ToCompute) {
    if (verbose && ToCompute.test(ConeProperty::FVector))
        verboseOutput() << "Going to the dual side for the primal f-vector" << endl;
    if (verbose)
        verboseOutput() << "Computing dual incidence/face lattice/f-vector ... " << endl;

    // Note for the coordinate transformation:
    // On the dual space we must use the dual coordinate transformation
    // Since the primal extreme rays are the support hyperplanes on the dual space
    // they must be transformed by the dual of the dual = primal
    // The support hyperplanes are extreme rays on the dual.
    // They are transformed by the primal of the dual = dual.

    Matrix<Integer> SuppHypPointed;
    BasisChangePointed.convert_to_sublattice(SuppHypPointed, ExtremeRays);  // We dualize !!!!
    Matrix<Integer> VertOfPolPointed;                                       // empty matrix in the dual case
    Matrix<Integer> ExtrRCPointed;
    BasisChangePointed.convert_to_sublattice_dual(ExtrRCPointed, SupportHyperplanes);  // We dualize !!!!

    FaceLattice<Integer> FL(SuppHypPointed, VertOfPolPointed, ExtrRCPointed, inhomogeneous);

    if (ToCompute.test(ConeProperty::DualFaceLattice) || ToCompute.test(ConeProperty::DualFVector) ||
        ToCompute.test(ConeProperty::FVector))
        FL.compute(face_codim_bound, verbose, change_integer_type);

    if (ToCompute.test(ConeProperty::DualIncidence)) {
        FL.get(DualSuppHypInd);
        setComputed(ConeProperty::DualIncidence);
    }
    if (ToCompute.test(ConeProperty::DualFaceLattice)) {
        FL.get(DualFaceLat);
        setComputed(ConeProperty::DualFaceLattice);
    }
    if (ToCompute.test(ConeProperty::DualFaceLattice) || ToCompute.test(ConeProperty::DualFVector) ||
        ToCompute.test(ConeProperty::FVector)) {
        vector<size_t> prel_f_vector = FL.getFVector();
        if (!ToCompute.test(ConeProperty::FVector)) {
            dual_f_vector = prel_f_vector;
            setComputed(ConeProperty::DualFVector);
        }
        else {
            dual_f_vector.resize(prel_f_vector.size());
            for (size_t i = 0; i < prel_f_vector.size(); ++i)
                f_vector[i] = prel_f_vector[prel_f_vector.size() - 1 - i];
            setComputed(ConeProperty::FVector);
        }
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_combinatorial_automorphisms(const ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::CombinatorialAutomorphisms) || isComputed(ConeProperty::CombinatorialAutomorphisms))
        return;

    if (verbose)
        verboseOutput() << "Computing combinatorial automorphism group" << endl;

    if (ToCompute.test(ConeProperty::KeepOrder))
        compute(ConeProperty::ExtremeRays, ConeProperty::SupportHyperplanes, ConeProperty::KeepOrder);
    else
        compute(ConeProperty::ExtremeRays, ConeProperty::SupportHyperplanes);

    Matrix<Integer> SpecialLinFoprms(0, dim);

    if (inhomogeneous) {
        SpecialLinFoprms.append(Dehomogenization);
    }

    /* set<AutomParam::Goals> AutomToCompute;
    AutomToCompute.insert(AutomParam::OrbitsPrimal);
    AutomToCompute.insert(AutomParam::OrbitsDual);*/

    Automs = AutomorphismGroup<Integer>(ExtremeRays, SupportHyperplanes, SpecialLinFoprms);

    Automs.compute(AutomParam::combinatorial);

    if (verbose)
        verboseOutput() << Automs.getQualitiesString() << "automorphism group of order " << Automs.getOrder() << "  done" << endl;

    extract_automorphisms(Automs);

    setComputed(ConeProperty::CombinatorialAutomorphisms);
}

//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::compute_input_automorphisms(const ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::InputAutomorphisms) || isComputed(ConeProperty::InputAutomorphisms))
        return;

    if (Generators.nr_of_rows() > 0)
        compute_input_automorphisms_gen(ToCompute);
    if (Generators.nr_of_rows() == 0) {
        compute_input_automorphisms_ineq(ToCompute);
    }
    setComputed(ConeProperty::InputAutomorphisms);

    if (verbose)
        verboseOutput() << Automs.getQualitiesString() << "automorphism group of order " << Automs.getOrder() << "  done" << endl;
}
//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::compute_input_automorphisms_gen(const ConeProperties& ToCompute) {
    if (verbose)
        verboseOutput() << "Computing automorphisms from input generators" << endl;

    Matrix<Integer> GeneratorsHere = BasisChangePointed.to_sublattice(Generators);
    Matrix<Integer> SpecialLinForms(0, BasisChangePointed.getRank());
    if (Grading.size() == dim)
        SpecialLinForms.append(BasisChangePointed.to_sublattice_dual(Grading));
    if (Dehomogenization.size() == dim)
        SpecialLinForms.append(BasisChangePointed.to_sublattice_dual_no_div(Dehomogenization));

    Matrix<Integer> Empty(0, BasisChangePointed.getRank());
    Automs = AutomorphismGroup<Integer>(GeneratorsHere, Empty, SpecialLinForms);
    Automs.compute(AutomParam::input_gen);

    Automs.setGensRef(Generators);
}
//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::compute_input_automorphisms_ineq(const ConeProperties& ToCompute) {
    if (verbose)
        verboseOutput() << "Computing automorphisms from input inequalities" << endl;

    Matrix<Integer> SpecialGens(0, BasisChangePointed.getRank());
    Matrix<Integer> Empty(0, BasisChangePointed.getRank());
    if (Grading.size() == dim)
        SpecialGens.append(BasisChangePointed.to_sublattice_dual(Grading));
    Matrix<Integer> InequalitiesHere = BasisChangePointed.to_sublattice_dual(Inequalities);
    if (inhomogeneous) {
        SpecialGens.append(BasisChangePointed.to_sublattice_dual_no_div(Dehomogenization));
        InequalitiesHere.remove_row(BasisChangePointed.to_sublattice_dual(Dehomogenization));
    }

    Automs = AutomorphismGroup<Integer>(InequalitiesHere, SpecialGens, Empty, Empty);
    Automs.compute(AutomParam::input_ineq);

    InequalitiesHere = Inequalities;
    if (inhomogeneous) {
        InequalitiesHere.remove_row(Dehomogenization);
    }
    Automs.setGensRef(InequalitiesHere);
}

//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::compute_ambient_automorphisms(const ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::AmbientAutomorphisms) || isComputed(ConeProperty::AmbientAutomorphisms))
        return;
    if (Generators.nr_of_rows() > 0)
        compute_ambient_automorphisms_gen(ToCompute);
    if (Generators.nr_of_rows() == 0 && Inequalities.nr_of_rows() > 0) {
        if (BasisChange.IsIdentity())
            compute_ambient_automorphisms_ineq(ToCompute);
        else
            throw BadInputException("Ambient automorphisms not computable from input automorphisms");
    }
    setComputed(ConeProperty::AmbientAutomorphisms);

    if (verbose)
        verboseOutput() << Automs.getQualitiesString() << "automorphism group of order " << Automs.getOrder() << "  done" << endl;
}
//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::compute_ambient_automorphisms_gen(const ConeProperties& ToCompute) {
    if (verbose)
        verboseOutput() << "Computing ambient automorphisms from input generators" << endl;

    Matrix<Integer> UnitMatrix(dim);
    Matrix<Integer> SpecialLinForms(0, dim);
    if (Grading.size() == dim)
        SpecialLinForms.append(Grading);
    if (Dehomogenization.size() == dim)
        SpecialLinForms.append(Dehomogenization);

    Automs = AutomorphismGroup<Integer>(Generators, UnitMatrix, SpecialLinForms);
    Automs.compute(AutomParam::ambient_gen);
}
//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::compute_ambient_automorphisms_ineq(const ConeProperties& ToCompute) {
    if (verbose)
        verboseOutput() << "Computing ambient automorphisms from input inequalities" << endl;

    Matrix<Integer> UnitMatrix(dim);
    Matrix<Integer> SpecialGens(0, dim);
    Matrix<Integer> Empty(0, dim);
    if (Grading.size() == dim)
        SpecialGens.append(Grading);
    Matrix<Integer> InequalitiesHere = Inequalities;
    if (inhomogeneous) {
        SpecialGens.append(Dehomogenization);
        InequalitiesHere.remove_row(Dehomogenization);
    }

    Automs = AutomorphismGroup<Integer>(InequalitiesHere, SpecialGens, UnitMatrix, Empty);
    Automs.compute(AutomParam::ambient_ineq);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_euclidean_automorphisms(const ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::EuclideanAutomorphisms) || isComputed(ConeProperty::EuclideanAutomorphisms))
        return;

    if (ToCompute.test(ConeProperty::KeepOrder))
        compute(ConeProperty::ExtremeRays, ConeProperty::SupportHyperplanes, ConeProperty::KeepOrder);
    else
        compute(ConeProperty::ExtremeRays, ConeProperty::SupportHyperplanes);

    if (getDimMaximalSubspace() > 0)
        throw NotComputableException("Euclidean automorphisms not computable if maximal subspace is nonzero");
    if (inhomogeneous && getRecessionRank() > 0)
        throw NotComputableException("Unbounded polyhedron. Euclidean automorphisms only computable for polytopes");
    if (!inhomogeneous && !isComputed(ConeProperty::Grading))
        throw NotComputableException("No Grading. Euclidean automorphisms only computable for polytopes");

    if (verbose)
        verboseOutput() << "Computing euclidean automorphism group" << endl;

    Matrix<Integer> SpecialLinFoprms(0, dim);
    if (!inhomogeneous) {
        SpecialLinFoprms.append(Grading);
    }
    if (inhomogeneous) {
        SpecialLinFoprms.append(Dehomogenization);
    }

    /* set<AutomParam::Goals> AutomToCompute;
    AutomToCompute.insert(AutomParam::OrbitsPrimal);
    AutomToCompute.insert(AutomParam::OrbitsDual);*/

    Automs = AutomorphismGroup<Integer>(ExtremeRays, SupportHyperplanes, SpecialLinFoprms);

    Automs.compute(AutomParam::euclidean);

    if (verbose)
        verboseOutput() << Automs.getQualitiesString() << "automorphism group of order " << Automs.getOrder() << "  done" << endl;

    extract_automorphisms(Automs);

    setComputed(ConeProperty::EuclideanAutomorphisms);
}

//---------------------------------------------------------------------------
template <typename Integer>
template <typename IntegerFC>
void Cone<Integer>::extract_automorphisms(AutomorphismGroup<IntegerFC>& AutomsComputed, const bool must_transform) {
    Automs.order = AutomsComputed.order;
    Automs.is_integral = AutomsComputed.is_integral;
    Automs.integrality_checked = AutomsComputed.integrality_checked;
    Automs.Qualities = AutomsComputed.Qualities;

    vector<key_t> SuppHypsKey, ExtRaysKey, VerticesKey, GensKey;

    Automs.GenPerms =
        extract_permutations(AutomsComputed.GenPerms, AutomsComputed.GensRef, ExtremeRays, true, GensKey, must_transform);

    Automs.ExtRaysPerms.clear();  // not necessarily set below
    if (inhomogeneous) {
        if (ExtremeRaysRecCone.nr_of_rows() > 0) {
            Automs.ExtRaysPerms = extract_permutations(AutomsComputed.GenPerms, AutomsComputed.GensRef, ExtremeRaysRecCone, true,
                                                       ExtRaysKey, must_transform);
        }
        Automs.VerticesPerms = extract_permutations(AutomsComputed.GenPerms, AutomsComputed.GensRef, VerticesOfPolyhedron, true,
                                                    VerticesKey, must_transform);
    }
    else {
        Automs.ExtRaysPerms = Automs.GenPerms;
        ExtRaysKey = GensKey;
    }

    Automs.LinFormPerms = extract_permutations(AutomsComputed.LinFormPerms, AutomsComputed.LinFormsRef, SupportHyperplanes, false,
                                               SuppHypsKey, must_transform);
    Automs.SuppHypsPerms = Automs.LinFormPerms;

    Automs.GenOrbits = extract_subsets(AutomsComputed.GenOrbits, AutomsComputed.GensRef.nr_of_rows(), GensKey);

    sort_individual_vectors(Automs.GenOrbits);
    if (inhomogeneous) {
        Automs.VerticesOrbits = extract_subsets(AutomsComputed.GenOrbits, AutomsComputed.GensRef.nr_of_rows(), VerticesKey);
        sort_individual_vectors(Automs.VerticesOrbits);
        Automs.ExtRaysOrbits.clear();  // not necessarily set below
        if (ExtremeRaysRecCone.nr_of_rows() > 0) {
            Automs.ExtRaysOrbits = extract_subsets(AutomsComputed.GenOrbits, AutomsComputed.GensRef.nr_of_rows(), ExtRaysKey);
            sort_individual_vectors(Automs.ExtRaysOrbits);
        }
    }
    else {
        Automs.ExtRaysOrbits = Automs.GenOrbits;
    }

    Automs.LinFormOrbits = extract_subsets(AutomsComputed.LinFormOrbits, AutomsComputed.LinFormsRef.nr_of_rows(), SuppHypsKey);
    sort_individual_vectors(Automs.LinFormOrbits);
    Automs.SuppHypsOrbits = Automs.LinFormOrbits;

    Automs.cone_dependent_data_computed = true;
}
//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::compute_pulling_triangulation(ConeProperties& ToCompute) {
    if (isComputed(ConeProperty::PullingTriangulation))
        return;

    if (verbose)
        verboseOutput() << "Computing pulling triangulation" << endl;

    // BasicTriangulation is the only data field to get a triangulation from the full_cone.
    // This will be the pulling triangulation.
    // Therefore an existing basic triangulation must be saved and restired.
    pair<vector<SHORTSIMPLEX<Integer> >, Matrix<Integer> > SaveBasicTriangulation;
    bool save_is_computed_BasicTriangulation = isComputed(ConeProperty::BasicTriangulation);
    if (isComputed(ConeProperty::BasicTriangulation))
        swap(BasicTriangulation, SaveBasicTriangulation);

    ConeProperties PullTri;
    PullTri.set(ConeProperty::PullingTriangulationInternal);
    compute_full_cone(PullTri);
    Triangulation = BasicTriangulation;
    setComputed(ConeProperty::Triangulation);
    setComputed(ConeProperty::PullingTriangulationInternal);
    setComputed(ConeProperty::PullingTriangulation);

    is_Computed.set(ConeProperty::BasicTriangulation, save_is_computed_BasicTriangulation);
    if (isComputed(ConeProperty::BasicTriangulation))
        swap(BasicTriangulation, SaveBasicTriangulation);
}

//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::compute_refined_triangulation(ConeProperties& ToCompute) {
    if (ToCompute.intersection_with(all_triangulations()).none())
        return;

    if (ToCompute.test(ConeProperty::PullingTriangulation)) {
        compute_pulling_triangulation(ToCompute);
        return;
    }

    compute(ConeProperty::BasicTriangulation);  // we need it here

    // First we deal with the ordinary triangulation
    if (ToCompute.test(ConeProperty::Triangulation)) {
        Triangulation = BasicTriangulation;
        setComputed(ConeProperty::Triangulation);
        return;
    }

    is_Computed.reset(ConeProperty::ConeDecomposition);

    if (change_integer_type) {
        try {
#ifdef NMZ_EXTENDED_TESTS
            if (!using_GMP<Integer>() && !using_renf<Integer>() && test_arith_overflow_descent)
                throw ArithmeticException(0);
#endif
            compute_unimodular_triangulation<MachineInteger>(ToCompute);  // only one can be actiated
            compute_lattice_point_triangulation<MachineInteger>(ToCompute);
            compute_all_generators_triangulation<MachineInteger>(ToCompute);
        } catch (const ArithmeticException& e) {
            if (verbose) {
                verboseOutput() << e.what() << endl;
                verboseOutput() << "Restarting with a bigger type." << endl;
            }
            change_integer_type = false;
        }
    }
    if (!change_integer_type) {
        compute_unimodular_triangulation<Integer>(ToCompute);  // only one can be actiated
        compute_lattice_point_triangulation<Integer>(ToCompute);
        compute_all_generators_triangulation<Integer>(ToCompute);
    }
}

template <typename Integer>
template <typename IntegerColl>
void Cone<Integer>::prepare_collection(ConeCollection<IntegerColl>& Coll) {
    compute(ConeProperty::BasicTriangulation);

    BasisChangePointed.convert_to_sublattice(Coll.Generators, BasicTriangulation.second);
    vector<pair<vector<key_t>, IntegerColl> > CollTriangulation;
    for (auto& T : BasicTriangulation.first) {
        IntegerColl CollMult = convertTo<IntegerColl>(T.vol);
        CollTriangulation.push_back(make_pair(T.key, CollMult));
    }
    Coll.verbose = verbose;
    Coll.initialize_minicones(CollTriangulation);
}

template <typename Integer>
template <typename IntegerColl>
void Cone<Integer>::extract_data(ConeCollection<IntegerColl>& Coll) {
    BasisChangePointed.convert_from_sublattice(Triangulation.second, Coll.Generators);
    Triangulation.first.clear();
    Coll.flatten();
    for (auto& T : Coll.getKeysAndMult()) {
        INTERRUPT_COMPUTATION_BY_EXCEPTION

        Integer CollMult = convertTo<Integer>(T.second);
        SHORTSIMPLEX<Integer> Simp;
        Simp.key = T.first;
        Simp.vol = CollMult;
        Triangulation.first.push_back(Simp);
    }
#ifdef NMZ_EXTENDED_TESTS
    if (isComputed(ConeProperty::Volume) && !using_renf<Integer>()) {
        mpq_class test_vol;
        vector<Integer> TestGrad;
        if (inhomogeneous)
            TestGrad = Dehomogenization;
        else
            TestGrad = Grading;
        for (auto& T : Triangulation.first) {
            Integer grad_prod = 1;
            for (auto& k : T.key)
                grad_prod *= v_scalar_product(Triangulation.second[k], TestGrad);
            mpz_class gp_mpz = convertTo<mpz_class>(grad_prod);
            mpz_class vol_mpz = convertTo<mpz_class>(T.vol);
            mpq_class quot = vol_mpz;
            quot /= gp_mpz;
            test_vol += quot;
        }
        assert(test_vol == getVolume());
    }
#endif
}

template <typename Integer>
template <typename IntegerColl>
void Cone<Integer>::compute_unimodular_triangulation(ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::UnimodularTriangulation) || isComputed(ConeProperty::UnimodularTriangulation))
        return;

    if (verbose)
        verboseOutput() << "Computing unimimodular triangulation" << endl;

    ConeCollection<IntegerColl> UMT;
    prepare_collection<IntegerColl>(UMT);
    if (isComputed(ConeProperty::HilbertBasis)) {
        Matrix<IntegerColl> HBPointed;
        BasisChangePointed.convert_to_sublattice(HBPointed, HilbertBasis);
        UMT.add_extra_generators(HBPointed);
    }

    UMT.make_unimodular();
    extract_data<IntegerColl>(UMT);
    setComputed(ConeProperty::UnimodularTriangulation);
    setComputed(ConeProperty::Triangulation);
}

template <>
template <typename IntegerColl>
void Cone<renf_elem_class>::compute_unimodular_triangulation(ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::UnimodularTriangulation) || isComputed(ConeProperty::UnimodularTriangulation))
        return;

    assert(false);
}

template <typename Integer>
template <typename IntegerColl>
void Cone<Integer>::compute_lattice_point_triangulation(ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::LatticePointTriangulation) || isComputed(ConeProperty::LatticePointTriangulation))
        return;

    if (inhomogeneous && getNrExtremeRays() > 0)
        throw BadInputException("LatticePointTriangulation not defined for unbounded polyhedra");

    if (verbose)
        verboseOutput() << "Computing lattice points triangulation" << endl;

    ConeCollection<IntegerColl> LPT;
    prepare_collection<IntegerColl>(LPT);
    Matrix<IntegerColl> LPPointed;
    if (inhomogeneous) {
        assert(isComputed(ConeProperty::ModuleGenerators));
        BasisChangePointed.convert_to_sublattice(LPPointed, ModuleGenerators);
    }
    else {
        assert(isComputed(ConeProperty::Deg1Elements));
        BasisChangePointed.convert_to_sublattice(LPPointed, Deg1Elements);
    }
    LPT.add_extra_generators(LPPointed);
    extract_data<IntegerColl>(LPT);
    setComputed(ConeProperty::LatticePointTriangulation);
    setComputed(ConeProperty::Triangulation);
}

template <typename Integer>
template <typename IntegerColl>
void Cone<Integer>::compute_all_generators_triangulation(ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::AllGeneratorsTriangulation) || isComputed(ConeProperty::AllGeneratorsTriangulation))
        return;

    if (verbose)
        verboseOutput() << "Computing all generators triangulation" << endl;

    ConeCollection<IntegerColl> OMT;
    prepare_collection<IntegerColl>(OMT);
    Matrix<IntegerColl> OMPointed;
    BasisChangePointed.convert_to_sublattice(OMPointed, InputGenerators);
    OMT.insert_all_gens();
    extract_data<IntegerColl>(OMT);
    setComputed(ConeProperty::AllGeneratorsTriangulation);
    setComputed(ConeProperty::Triangulation);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::resetProjectionCoords(const vector<Integer>& lf) {
    if (ProjCone != NULL)
        delete ProjCone;

    if (lf.size() > dim)
        throw BadInputException("Too many projection coordinates");
    projection_coord_indicator.resize(dim);
    for (size_t i = 0; i < lf.size(); ++i)
        if (lf[i] != 0)
            projection_coord_indicator[i] = true;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::resetGrading(vector<Integer> lf) {
    is_Computed.reset(ConeProperty::HilbertSeries);
    is_Computed.reset(ConeProperty::HSOP);
    is_Computed.reset(ConeProperty::HilbertQuasiPolynomial);
    is_Computed.reset(ConeProperty::EhrhartSeries);
    is_Computed.reset(ConeProperty::EhrhartQuasiPolynomial);
    is_Computed.reset(ConeProperty::WeightedEhrhartSeries);
    is_Computed.reset(ConeProperty::WeightedEhrhartQuasiPolynomial);
    is_Computed.reset(ConeProperty::Integral);
    is_Computed.reset(ConeProperty::EuclideanIntegral);
    is_Computed.reset(ConeProperty::Multiplicity);
    is_Computed.reset(ConeProperty::VirtualMultiplicity);
    is_Computed.reset(ConeProperty::Grading);
    is_Computed.reset(ConeProperty::GradingDenom);
    is_Computed.reset(ConeProperty::IsDeg1ExtremeRays);
    // is_Computed.reset(ConeProperty::ExplicitHilbertSeries);
    is_Computed.reset(ConeProperty::IsDeg1HilbertBasis);
    is_Computed.reset(ConeProperty::Deg1Elements);
    if (!inhomogeneous) {
        is_Computed.reset(ConeProperty::Volume);
        is_Computed.reset(ConeProperty::EuclideanVolume);
        if (isComputed(ConeProperty::IntegerHull))
            delete IntHullCone;
        is_Computed.reset(ConeProperty::IntegerHull);
    }

    if (inhom_input) {
        lf.push_back(0);
    }
    setGrading(lf);
}

// Multi-getter methods
template <typename Integer>
const Matrix<Integer>& Cone<Integer>::getMatrixConePropertyMatrix(ConeProperty::Enum property) {
    if (output_type(property) != OutputType::Matrix) {
        throw FatalException("property has no matrix output");
    }
    switch (property) {
        case ConeProperty::ExtremeRays:
            return this->getExtremeRaysMatrix();
        case ConeProperty::VerticesOfPolyhedron:
            return this->getVerticesOfPolyhedronMatrix();
        case ConeProperty::SupportHyperplanes:
            return this->getSupportHyperplanesMatrix();
        case ConeProperty::HilbertBasis:
            return this->getHilbertBasisMatrix();
        case ConeProperty::ModuleGenerators:
            return this->getModuleGeneratorsMatrix();
        case ConeProperty::Deg1Elements:
            return this->getDeg1ElementsMatrix();
        case ConeProperty::LatticePoints:
            return this->getLatticePointsMatrix();
        case ConeProperty::ModuleGeneratorsOverOriginalMonoid:
            return this->getModuleGeneratorsOverOriginalMonoidMatrix();
        case ConeProperty::ExcludedFaces:
            return this->getExcludedFacesMatrix();
        case ConeProperty::OriginalMonoidGenerators:
            return this->getOriginalMonoidGeneratorsMatrix();
        case ConeProperty::MaximalSubspace:
            return this->getMaximalSubspaceMatrix();
        // The following point to the sublattice
        case ConeProperty::Equations:
            return this->getSublattice().getEquationsMatrix();
        case ConeProperty::Congruences:
            return this->getSublattice().getCongruencesMatrix();
        default:
            throw FatalException("Matrix property without output");
    }
}

template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getMatrixConeProperty(ConeProperty::Enum property) {
    return getMatrixConePropertyMatrix(property).get_elements();
}

template <typename Integer>
const Matrix<nmz_float>& Cone<Integer>::getFloatMatrixConePropertyMatrix(ConeProperty::Enum property) {
    if (output_type(property) != OutputType::MatrixFloat) {
        throw FatalException("property has no float matrix output");
    }
    switch (property) {
        case ConeProperty::SuppHypsFloat:
            return this->getSuppHypsFloatMatrix();
        case ConeProperty::ExtremeRaysFloat:
            return this->getSuppHypsFloatMatrix();
        case ConeProperty::VerticesFloat:
            return this->getVerticesFloatMatrix();
        default:
            throw FatalException("Flaot Matrix property without output");
    }
}

template <typename Integer>
const vector<vector<nmz_float> >& Cone<Integer>::getFloatMatrixConeProperty(ConeProperty::Enum property) {
    return getFloatMatrixConePropertyMatrix(property).get_elements();
}

template <typename Integer>
vector<Integer> Cone<Integer>::getVectorConeProperty(ConeProperty::Enum property) {
    if (output_type(property) != OutputType::Vector) {
        throw FatalException("property has no vector output");
    }
    switch (property) {
        case ConeProperty::Grading:
            return this->getGrading();
        case ConeProperty::Dehomogenization:
            return this->getDehomogenization();
        case ConeProperty::WitnessNotIntegrallyClosed:
            return this->getWitnessNotIntegrallyClosed();
        case ConeProperty::GeneratorOfInterior:
            return this->getGeneratorOfInterior();
        case ConeProperty::CoveringFace:
            return this->getCoveringFace();
        case ConeProperty::AxesScaling:
            return this->getAxesScaling();
        default:
            throw FatalException("Vector property without output");
    }
}

template <typename Integer>
Integer Cone<Integer>::getIntegerConeProperty(ConeProperty::Enum property) {
    if (output_type(property) != OutputType::Integer) {
        throw FatalException("property has no integer output");
    }
    switch (property) {
        case ConeProperty::TriangulationDetSum:
            return this->getTriangulationDetSum();
        case ConeProperty::ReesPrimaryMultiplicity:
            return this->getReesPrimaryMultiplicity();
        case ConeProperty::GradingDenom:
            return this->getGradingDenom();
        case ConeProperty::UnitGroupIndex:
            return this->getUnitGroupIndex();
        case ConeProperty::InternalIndex:
            return this->getInternalIndex();
        default:
            throw FatalException("Intehger property without output");
            ;
    }
}

template <typename Integer>
mpz_class Cone<Integer>::getGMPIntegerConeProperty(ConeProperty::Enum property) {
    if (output_type(property) != OutputType::GMPInteger) {
        throw FatalException("property has no GMP integer output");
    }
    switch (property) {
        case ConeProperty::ExternalIndex:
            return this->getSublattice().getExternalIndex();
        default:
            throw FatalException("GMP integer property without output");
    }
}

template <typename Integer>
mpq_class Cone<Integer>::getRationalConeProperty(ConeProperty::Enum property) {
    if (output_type(property) != OutputType::Rational) {
        throw FatalException("property has no rational output");
    }
    switch (property) {
        case ConeProperty::Multiplicity:
            return this->getMultiplicity();
        case ConeProperty::Volume:
            return this->getVolume();
        case ConeProperty::Integral:
            return this->getIntegral();
        case ConeProperty::VirtualMultiplicity:
            return this->getVirtualMultiplicity();
        default:
            throw FatalException("Rational property without output");
            ;
    }
}

template <typename Integer>
renf_elem_class Cone<Integer>::getFieldElemConeProperty(ConeProperty::Enum property) {
    if (output_type(property) != OutputType::FieldElem) {
        throw FatalException("property has no field element output");
    }
    switch (property) {
        case ConeProperty::RenfVolume:
            return this->getRenfVolume();
        default:
            throw FatalException("Field element property without output");
            ;
    }
}

template <typename Integer>
nmz_float Cone<Integer>::getFloatConeProperty(ConeProperty::Enum property) {
    if (output_type(property) != OutputType::Float) {
        throw FatalException("property has no float output");
    }
    switch (property) {
        case ConeProperty::EuclideanVolume:
            return this->getEuclideanVolume();
        case ConeProperty::EuclideanIntegral:
            return this->getEuclideanIntegral();
        default:
            throw FatalException("Float property without output");
            ;
    }
}

template <typename Integer>
long Cone<Integer>::getMachineIntegerConeProperty(ConeProperty::Enum property) {
    if (output_type(property) != OutputType::MachineInteger) {
        throw FatalException("property has no machine integer output");
    }
    switch (property) {
        case ConeProperty::TriangulationSize:
            return this->getTriangulationSize();
        case ConeProperty::RecessionRank:
            return this->getRecessionRank();
        case ConeProperty::AffineDim:
            return this->getAffineDim();
        case ConeProperty::ModuleRank:
            return this->getModuleRank();
        case ConeProperty::Rank:
            return this->getRank();
        case ConeProperty::EmbeddingDim:
            return this->getEmbeddingDim();
        case ConeProperty::NumberLatticePoints:
            return this->getNumberLatticePoints();
        default:
            throw FatalException("Machine integer property without output");
    }
}

template <typename Integer>
bool Cone<Integer>::getBooleanConeProperty(ConeProperty::Enum property) {
    if (output_type(property) != OutputType::Bool) {
        throw FatalException("property has no boolean output");
    }
    switch (property) {
        case ConeProperty::IsPointed:
            return this->isPointed();
        case ConeProperty::IsDeg1ExtremeRays:
            return this->isDeg1ExtremeRays();
        case ConeProperty::IsDeg1HilbertBasis:
            return this->isDeg1HilbertBasis();
        case ConeProperty::IsIntegrallyClosed:
            return this->isIntegrallyClosed();
        case ConeProperty::IsReesPrimary:
            return this->isReesPrimary();
        case ConeProperty::IsInhomogeneous:
            return this->isInhomogeneous();
        case ConeProperty::IsGorenstein:
            return this->isGorenstein();
        case ConeProperty::IsEmptySemiOpen:
            return this->isEmptySemiOpen();
        case ConeProperty::IsTriangulationNested:
            return this->isTriangulationNested();
        case ConeProperty::IsTriangulationPartial:
            return this->isTriangulationPartial();
        default:
            throw FatalException("Boolean property without output");
    }
}

template <typename Integer>
void Cone<Integer>::write_cone_output(const string& output_file) {
    Output<Integer> Out;

    Out.set_name(output_file);

    // Out.set_lattice_ideal_input(input.count(Type::lattice_ideal)>0);

    Out.setCone(*this);
#ifdef ENFNORMALIZ
    if (using_renf<Integer>())
        Out.set_renf(Renf);
#endif

    Out.write_files();
}

template <typename Integer>
void Cone<Integer>::write_precomp_for_input(const string& output_file) {
    ConeProperties NeededHere;
    NeededHere.set(ConeProperty::SupportHyperplanes);
    NeededHere.set(ConeProperty::ExtremeRays);
    NeededHere.set(ConeProperty::Sublattice);
    NeededHere.set(ConeProperty::MaximalSubspace);
    compute(NeededHere);

    Output<Integer> Out;

    Out.set_name(output_file);  // one could take it from the cone in output.cpp

    // Out.set_lattice_ideal_input(input.count(Type::lattice_ideal)>0);

    Out.setCone(*this);
#ifdef ENFNORMALIZ
    if (using_renf<Integer>())
        Out.set_renf(Renf);
#endif

    Out.set_write_precomp(true);
    Out.write_precomp();
}

#ifndef NMZ_MIC_OFFLOAD  // offload with long is not supported
template class Cone<long>;
#endif
template class Cone<long long>;
template class Cone<mpz_class>;

#ifdef ENFNORMALIZ
template class Cone<renf_elem_class>;
#endif

#ifdef NMZ_EXTENDED_TESTS
// additional tests for libnormaliz that are not accessoble from Normaliz
void run_additional_tests_libnormaliz() {
    vector<nmz_float> ext_1 = {1.0, 1.5};
    vector<nmz_float> ext_2 = {3.1e1, 1.7e1};
    vector<vector<nmz_float> > test_ext = {ext_1, ext_2};
    Matrix<nmz_float> test_input(test_ext);

    Cone<mpz_class> C(Type::polytope, test_input);
    // C.getHilbertBasisMatrix().pretty_print(cout);

    vector<mpz_class> new_grading = {1, 2, 3};
    C.resetGrading(new_grading);
    C.compute(ConeProperty::HilbertSeries);

    C.getAutomorphismGroup(ConeProperty::CombinatorialAutomorphisms);

    C.getOriginalMonoidGenerators();

    C.getMaximalSubspace();

    C.getExtremeRays();

    C.getSuppHypsFloat();

    C.getNrSuppHypsFloat();

    C.getSupportHyperplanes();
    C.getNrSupportHyperplanes();

    C.getEquations();
    C.getNrEquations();

    C.getCongruences();
    C.getNrCongruences();

    C.getHilbertBasis();

    C.getModuleGeneratorsOverOriginalMonoid();

    C.getDeg1Elements();

    C.getLatticePointsMatrix();

    C.getLatticePoints();

    C.getNrLatticePoints();

    C.isPointed();

    C.getTriangulation(ConeProperty::UnimodularTriangulation);
    C.getTriangulation(ConeProperty::LatticePointTriangulation);
    C.getTriangulation(ConeProperty::AllGeneratorsTriangulation);

    vector<vector<nmz_float> > eq = {{-1, 1, -1}};

    C.modifyCone(Type::equations, eq);

    C = Cone<mpz_class>(Type::vertices, test_input);

    C.getVerticesFloat();
    C.getNrVerticesFloat();
    C.getVerticesOfPolyhedron();
    C.getModuleGenerators();
    C.getExtremeRaysFloat();
    C.getExtremeRaysFloatMatrix();
    C.getNrExtremeRaysFloat();
    C.write_precomp_for_input("blabla12345");
    C.write_cone_output("blabla12345");

    vector<vector<mpz_class> > trivial = {{-1, 1}, {1, 1}};
    vector<vector<mpz_class> > excl = {{-1, 1}};
    C = Cone<mpz_class>(Type::cone, trivial, Type::excluded_faces, excl);
    C.getHilbertSeries();
    C.compute(ConeProperty::HSOP);

    C.getExcludedFaces();

    C = Cone<mpz_class>(Type::polytope, trivial);
    C.getLatticePoints();

    Isomorphism_Classes<mpz_class> IsoC;
    bool found;
    IsoC.add_type(C, found);
    IsoC.find_type(C, found);
}
#endif

}  // end namespace libnormaliz
