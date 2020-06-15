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

#include <stdlib.h>
#include <list>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>

#include "libnormaliz/cone.h"
#include "libnormaliz/vector_operations.h"
#include "libnormaliz/project_and_lift.h"
#include "libnormaliz/map_operations.h"
#include "libnormaliz/convert.h"
#include "libnormaliz/full_cone.h"
#include "libnormaliz/descent.h"
#include "libnormaliz/my_omp.h"
#include "libnormaliz/output.h"
#include "libnormaliz/collection.h"

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

template <typename Integer>
template <typename InputNumber>
void Cone<Integer>::check_add_input(const map<InputType, vector<vector<InputNumber> > >& multi_add_data) {
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
}

template <typename Integer>
template <typename InputNumber>
void Cone<Integer>::check_consistency_of_dimension(const map<InputType, vector<vector<InputNumber> > >& multi_input_data) {
    size_t inhom_corr = 0;
    if (inhom_input)
        inhom_corr = 1;
    auto it = multi_input_data.begin();
    size_t test_dim;
    for (; it != multi_input_data.end(); ++it) {
        /* if(type_is_number(it->first))
            continue;*/
        test_dim = it->second.front().size() - type_nr_columns_correction(it->first) + inhom_corr;
        if (test_dim != dim) {
            throw BadInputException("Inconsistent dimensions in input!");
        }
    }
}

map<InputType, vector<vector<mpq_class> > > nmzfloat_input_to_mpqclass(
    const map<InputType, vector<vector<nmz_float> > >& multi_input_data) {
    map<InputType, vector<vector<mpq_class> > > multi_input_data_QQ;
    auto it = multi_input_data.begin();
    for (; it != multi_input_data.end(); ++it) {
        vector<vector<mpq_class> > Transfer;
        for (const auto& j : it->second) {
            vector<mpq_class> vt;
            for (double k : j){
                vt.push_back(mpq_class(k));
            }
            Transfer.push_back(vt);
        }
        multi_input_data_QQ[it->first] = Transfer;
    }
    return multi_input_data_QQ;
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
map<InputType, vector<vector<Integer> > > Cone<Integer>::mpqclass_input_to_integer(
    const map<InputType, vector<vector<mpq_class> > >& multi_input_data_const) {
    // The input type polytope is replaced by cone+grading in this routine.
    // Nevertheless it appears in the subsequent routines.
    // But any implications of its appearance must be handled here already.
    // However, polytope can still be used without conversion to cone via libnormaliz !!!!!

    map<InputType, vector<vector<mpq_class> > > multi_input_data(
        multi_input_data_const);  // since we want to change it internally

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

    map<InputType, vector<vector<Integer> > > multi_input_data_ZZ;

    // special treatment of polytope. We convert it o cone
    // and define the grading
    if (contains(multi_input_data, Type::polytope)) {
        size_t dim;
        if (multi_input_data[Type::polytope].size() > 0) {
            dim = multi_input_data[Type::polytope][0].size() + 1;
            vector<vector<Integer> > grading;
            grading.push_back(vector<Integer>(dim));
            grading[0][dim - 1] = 1;
            multi_input_data_ZZ[Type::grading] = grading;
        }
        multi_input_data[Type::cone] = multi_input_data[Type::polytope];
        multi_input_data.erase(Type::polytope);
        for (size_t i = 0; i < multi_input_data[Type::cone].size(); ++i) {
            multi_input_data[Type::cone][i].resize(dim);
            multi_input_data[Type::cone][i][dim - 1] = 1;
        }
    }

    // now we clear denominators
    auto it = multi_input_data.begin();
    for (; it != multi_input_data.end(); ++it) {
        for (size_t i = 0; i < it->second.size(); ++i) {
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
            multi_input_data_ZZ[it->first].push_back(transfer);
        }
    }

    return multi_input_data_ZZ;
}

// adds the signs inequalities given by Signs to Inequalities
template <typename Integer>
Matrix<Integer> sign_inequalities(const vector<vector<Integer> >& Signs) {
    if (Signs.size() != 1) {
        throw BadInputException("ERROR: Bad signs matrix, has " + toString(Signs.size()) + " rows (should be 1)!");
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
Matrix<Integer> strict_sign_inequalities(const vector<vector<Integer> >& Signs) {
    if (Signs.size() != 1) {
        throw BadInputException("ERROR: Bad signs matrix, has " + toString(Signs.size()) + " rows (should be 1)!");
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
vector<vector<Integer> > find_input_matrix(const map<InputType, vector<vector<Integer> > >& multi_input_data,
                                           const InputType type) {
    typename map<InputType, vector<vector<Integer> > >::const_iterator it;
    it = multi_input_data.find(type);
    if (it != multi_input_data.end())
        return (it->second);

    vector<vector<Integer> > dummy;
    return (dummy);
}

template <typename Integer>
void insert_column(vector<vector<Integer> >& mat, size_t col, Integer entry) {
    if (mat.size() == 0)
        return;
    vector<Integer> help(mat[0].size() + 1);
    for (size_t i = 0; i < mat.size(); ++i) {
        for (size_t j = 0; j < col; ++j)
            help[j] = mat[i][j];
        help[col] = entry;
        for (size_t j = col; j < mat[i].size(); ++j)
            help[j + 1] = mat[i][j];
        mat[i] = help;
    }
}

template <typename Integer>
void insert_zero_column(vector<vector<Integer> >& mat, size_t col) {
    // Integer entry=0;
    insert_column<Integer>(mat, col, 0);
}

template <typename Integer>
template <typename InputNumber>
void Cone<Integer>::homogenize_input(map<InputType, vector<vector<InputNumber> > >& multi_input_data) {
    auto it = multi_input_data.begin();
    for (; it != multi_input_data.end(); ++it) {
        switch (it->first) {
            case Type::dehomogenization:
            case Type::support_hyperplanes:
            case Type::extreme_rays:
                throw BadInputException("Types dehomogenization, extreme_rays, support_hyperplanes not allowed with inhomogeneous input!");
                break;
            case Type::inhom_inequalities:  // nothing to do
            case Type::inhom_equations:
            case Type::inhom_congruences:
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
/*
template <typename Integer>
template <typename T>
void Cone<Integer>::modifyCone(InputType input_type, const vector<vector<T> >& Input) {
    // convert to a map
    map<InputType, vector<vector<T> > > multi_add_input;
    multi_add_input[input_type] = Input;
    modifyCone(multi_add_input);
}
//---------------------------------------------------------------------------

template <typename Integer>
template <typename T>
void Cone<Integer>::modifyCone(InputType input_type, const Matrix<T>& Input) {
    // convert to a map
    map<InputType, vector<vector<T> > > multi_add_input;
    multi_add_input[input_type] = Input.get_elements();
    modifyCone(multi_add_input);
}
*/
//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::modifyCone(const map<InputType, vector<vector<Integer> > >& multi_add_input_const) {
    precomputed_extreme_rays=false;
    precomputed_support_hyperplanes=false;
    map<InputType, vector<vector<Integer> > > multi_add_input(multi_add_input_const);
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

    if (!(AddInequalities.nr_of_rows() == 0 || AddGenerators.nr_of_rows() == 0))
        throw BadInputException("Only one category of additional input allowed between two compute(...)");

    bool save_dehom = isComputed(ConeProperty::Dehomogenization);

    if (AddGenerators.nr_of_rows() > 0) {
        if (!isComputed(ConeProperty::ExtremeRays))
            throw BadInputException("Generators can only be added after the first computation of extreme rays");
        if (inhomogeneous)
            Generators = ExtremeRays;
        Generators.append(AddGenerators);
        bool dummy;
        SupportHyperplanes.resize(0, dim);
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
        is_Computed = ConeProperties();
        setComputed(ConeProperty::Generators);
        if (Grading.size() > 0)
            setComputed(ConeProperty::Grading);
    }

    if (AddInequalities.nr_of_rows() > 0) {
        if (!isComputed(ConeProperty::SupportHyperplanes))
            throw BadInputException("Inequalities can only be added after the first computation of esupport hyperplanes");
        
        bool max_subspace_preserved=true;
        for (size_t i = 0; i < BasisMaxSubspace.nr_of_rows(); ++i) {
            for (size_t j = 0; j < AddInequalities.nr_of_rows(); ++j)
                if (v_scalar_product(AddInequalities[j], BasisMaxSubspace[i]) != 0){
                    max_subspace_preserved=false;
                    break;
                }
                    
                    // throw BadInputException("Additional inequalities do not vanish on maximal subspace");
        }
        SupportHyperplanes.append(AddInequalities);
        is_Computed = ConeProperties();
        if(max_subspace_preserved){
            setComputed(ConeProperty::MaximalSubspace);  // cannot change since inequalities vanish on max subspace
            setComputed(ConeProperty::IsPointed);
        }
    }

    setComputed(ConeProperty::Dehomogenization, save_dehom);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::modifyCone(const map<InputType, vector<vector<mpq_class> > >& multi_add_input_const) {
    map<InputType, vector<vector<Integer> > > multi_add_input_ZZ = mpqclass_input_to_integer(multi_add_input_const);
    modifyCone(multi_add_input_ZZ);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::modifyCone(const map<InputType, vector<vector<nmz_float> > >& multi_add_input_const) {
    map<InputType, vector<vector<mpq_class> > > multi_add_input_QQ = nmzfloat_input_to_mpqclass(multi_add_input_const);
    modifyCone(multi_add_input_QQ);
}

//---------------------------------------------------------------------------

template <typename Integer>
Cone<Integer>::~Cone() {
    if (IntHullCone != NULL)
        delete IntHullCone;
    if (SymmCone != NULL)
        delete SymmCone;
    if (ProjCone != NULL)
        delete ProjCone;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::process_multi_input(const map<InputType, vector<vector<mpq_class> > >& multi_input_data_const) {
    initialize();
    map<InputType, vector<vector<Integer> > > multi_input_data_ZZ = mpqclass_input_to_integer(multi_input_data_const);
    process_multi_input_inner(multi_input_data_ZZ);
}

template <typename Integer>
void Cone<Integer>::process_multi_input(const map<InputType, vector<vector<nmz_float> > >& multi_input_data) {
    initialize();
    map<InputType, vector<vector<mpq_class> > > multi_input_data_QQ = nmzfloat_input_to_mpqclass(multi_input_data);
    process_multi_input(multi_input_data_QQ);
}

template <typename Integer>
void scale_matrix(vector<vector<Integer> >& mat, const vector<Integer>& scale_axes, bool dual) {
    for (size_t j = 0; j < scale_axes.size(); ++j) {
        if (scale_axes[j] == 0)
            continue;
        for (size_t i = 0; i < mat.size(); ++i) {
            if (dual)
                mat[i][j] /= scale_axes[j];
            else
                mat[i][j] *= scale_axes[j];
        }
    }
}

template <typename Integer>
void scale_input(map<InputType, vector<vector<Integer> > >& multi_input_data) {
    vector<vector<Integer> > scale_mat = find_input_matrix(multi_input_data, Type::scale);
    vector<Integer> scale_axes = scale_mat[0];

    auto it = multi_input_data.begin();
    for (; it != multi_input_data.end(); ++it) {
        switch (it->first) {
            case Type::inhom_inequalities:
            case Type::inhom_equations:
            case Type::inequalities:
            case Type::equations:
            case Type::dehomogenization:
            case Type::grading:
                scale_matrix(it->second, scale_axes, true);  // true = dual space
                break;
            case Type::polytope:
            case Type::cone:
            case Type::subspace:
            case Type::saturation:
            case Type::vertices:
            case Type::offset:
                scale_matrix(it->second, scale_axes, false);  // false = primal space
                break;
            case Type::signs:
                throw BadInputException("signs not allowed with scale");
                break;
            default:
                break;
        }
    }
}

template <typename Integer>
void check_types_precomputed(map<InputType, vector<vector<Integer> > >& multi_input_data) {

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
void Cone<Integer>::process_multi_input(const map<InputType, vector<vector<Integer> > >& multi_input_data_const) {
    initialize();
    map<InputType, vector<vector<Integer> > > multi_input_data(multi_input_data_const);
    if (contains(multi_input_data, Type::scale)) {
        if (!using_renf<Integer>())
            throw BadInputException("scale only allowed for field coefficients");
        else
            scale_input(multi_input_data);
    }
    process_multi_input_inner(multi_input_data);
}

template <typename Integer>
void Cone<Integer>::process_multi_input_inner(map<InputType, vector<vector<Integer> > >& multi_input_data) {
    // find basic input type
    lattice_ideal_input = false;
    nr_latt_gen = 0, nr_cone_gen = 0;
    inhom_input = false;

    if (using_renf<Integer>()) { //better in a table
        if (contains(multi_input_data, Type::lattice_ideal) || contains(multi_input_data, Type::lattice) ||
            contains(multi_input_data, Type::cone_and_lattice) || contains(multi_input_data, Type::congruences) ||
            contains(multi_input_data, Type::inhom_congruences)
            // || contains(multi_input_data,Type::dehomogenization)
            || contains(multi_input_data, Type::offset) || contains(multi_input_data, Type::excluded_faces) ||
            contains(multi_input_data, Type::open_facets) || contains(multi_input_data, Type::hilbert_basis_rec_cone) ||
            contains(multi_input_data, Type::strict_inequalities) || contains(multi_input_data, Type::strict_signs))
        throw BadInputException("Input type not allowed for field coefficients");
    }

    // inequalities_present=false; //control choice of positive orthant ?? Done differently

    // NEW: Empty matrices have syntactical influence
    auto it = multi_input_data.begin();
    for (; it != multi_input_data.end(); ++it) {
        switch (it->first) {
            case Type::inhom_inequalities:
            case Type::inhom_equations:
            case Type::inhom_congruences:
            case Type::strict_inequalities:
            case Type::strict_signs:
            case Type::open_facets:
                inhom_input = true;
                break;
            case Type::signs:
            case Type::inequalities:
            case Type::equations:
            case Type::congruences:
            case Type::support_hyperplanes:
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
                nr_latt_gen++;
                break;
            case Type::vertices:
            case Type::offset:
                inhom_input = true;
                break;
            default:
                break;
        }

        /* switch (it->first) {  // chceck existence of inrqualities
            case Type::inhom_inequalities:
            case Type::strict_inequalities:
            case Type::strict_signs:
            case Type::signs:
            case Type::inequalities:
            case Type::excluded_faces:
            case Type::support_hyperplanes:
                inequalities_present=true;
            default:
                break;
        }*/
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    bool gen_error = false;
    if (nr_cone_gen > 2)
        gen_error = true;
    
    precomputed_extreme_rays=contains(multi_input_data,Type::extreme_rays);
    precomputed_support_hyperplanes = contains(multi_input_data,Type::support_hyperplanes);
    if(precomputed_extreme_rays != precomputed_support_hyperplanes)
        throw BadInputException("Precomputwed extreme rays and support hyperplanes can only be used together");
    
    if(precomputed_extreme_rays)
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
            if (multi_input_data[Type::vertices].size() > 1)
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
        if (contains(multi_input_data, Type::excluded_faces)) {
            throw BadInputException("Type excluded_faces not allowed with inhomogeneous input or dehomogenization!");
        }
    }
    /*if(contains(multi_input_data,Type::grading) && contains(multi_input_data,Type::polytope)){ // now superfluous
           throw BadInputException("No explicit grading allowed with polytope!");
    }*/

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    // remove empty matrices
    it = multi_input_data.begin();
    for (; it != multi_input_data.end();) {
        if (it->second.size() == 0)
            multi_input_data.erase(it++);
        else
            ++it;
    }

    if (multi_input_data.size() == 0) {
        throw BadInputException("All input matrices empty!");
    }

    // determine dimension
    it = multi_input_data.begin();
    size_t inhom_corr = 0;  // correction in the inhom_input case
    if (inhom_input)
        inhom_corr = 1;
    dim = it->second.front().size() - type_nr_columns_correction(it->first) + inhom_corr;

    // We now process input types that are independent of generators, constraints, lattice_ideal
    // check for excluded faces

    ExcludedFaces = find_input_matrix(multi_input_data, Type::excluded_faces);
    if (ExcludedFaces.nr_of_rows() == 0)
        ExcludedFaces = Matrix<Integer>(0, dim);  // we may need the correct number of columns

    // check for a grading
    vector<vector<Integer> > lf = find_input_matrix(multi_input_data, Type::grading);
    if (lf.size() > 1) {
        throw BadInputException("Bad grading, has " + toString(lf.size()) + " rows (should be 1)!");
    }
    if (lf.size() == 1) {
        if (inhom_input)
            lf[0].push_back(0);  // first we extend grading trivially to have the right dimension
        setGrading(lf[0]);       // will eventually be set in full_cone.cpp
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    // cout << "Dim " << dim <<endl;

    check_consistency_of_dimension(multi_input_data);

    if (inhom_input)
        homogenize_input(multi_input_data);

    // check for codim_bound
    /*lf = find_input_matrix(multi_input_data,Type::codim_bound_vectors);
    if (lf.size() > 0) {
        autom_codim_vectors=convertTo<long>(lf[0][0]);
        autom_codim_vectors_set=true;
    }
    lf = find_input_matrix(multi_input_data,Type::codim_bound_mult);
    if (lf.size() > 0) {
        autom_codim_mult=convertTo<long>(lf[0][0]);
        autom_codim_mult_set=true;
    }*/

    if (contains(multi_input_data, Type::projection_coordinates)) {
        projection_coord_indicator.resize(dim);
        for (size_t i = 0; i < dim; ++i)
            if (multi_input_data[Type::projection_coordinates][0][i] != 0)
                projection_coord_indicator[i] = true;
    }

    // check for dehomogenization
    lf = find_input_matrix(multi_input_data, Type::dehomogenization);
    if (lf.size() > 1) {
        throw BadInputException("Bad dehomogenization, has " + toString(lf.size()) + " rows (should be 1)!");
    }
    if (lf.size() == 1) {
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

    if(!precomputed_extreme_rays)
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
    
    if(Generators.nr_of_rows() >0 && LatticeGenerators.nr_of_rows()>0 && 
            !(contains(multi_input_data, Type::cone_and_lattice) || contains(multi_input_data, Type::normalization)))
        convert_lattice_generators_to_constraints(LatticeGenerators); // necessary in order that we can perform the intersection
                                                     // of the cone with the subspce generated by LatticeGenerators
    // cout << "Ineq " << Inequalities.nr_of_rows() << endl;
    
    if(precomputed_extreme_rays)
            LatticeGenerators = find_input_matrix(multi_input_data,Type::generated_lattice);
 
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
    
    remove_superfluous_equations(); // equations satisfied by all cone generators if any
    
     //if(Generators.nr_of_rows() >0 && Equations.nr_of_rows() > 0)
      //   convert_equations_to_inequalties(); // allows us to intersect the cone(Generators) with the subspace(Equations)

    remove_superfluous_inequalities(); // namely those satisfied by the cone generators if there are any
    
    remove_superfluous_congruences(); // ditto

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    bool cone_sat_eq = true;
    bool cone_sat_cong = true;
    bool cone_sat_ineq = true;
    if(Generators.nr_of_rows() > 0){
        cone_sat_eq = (Equations.nr_of_rows()==0);
        cone_sat_cong = (Congruences.nr_of_rows()==0);
        cone_sat_ineq = (Inequalities.nr_of_rows()==0);
    }
    
    // cout << "Cond " << cone_sat_eq << cone_sat_ineq << cone_sat_cong << endl;
    
    if(precomputed_extreme_rays && !(cone_sat_eq && cone_sat_ineq && cone_sat_cong))
        throw BadInputException("Precomputed extreme rays violate constraints");
    
    if(precomputed_support_hyperplanes && !cone_sat_ineq)
        throw BadInputException("Precomputed support hyperplanes do not support the cone");
        
    if(cone_sat_eq && cone_sat_cong && cone_sat_ineq && Generators.nr_of_rows()!=0) 
        set_original_monoid_generators(Generators);

    if(!cone_sat_cong){
        for (size_t i = 0; i < Generators.nr_of_rows(); ++i)
            v_scalar_multiplication(Generators[i], BasisChange.getAnnihilator()); // the saled generators satisfy the congruences
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
    }

    BasisChangePointed = BasisChange;
    setWeights();  // make matrix of weights for sorting
    
    if(!cone_sat_eq){ // in this case we must compute a copy without the already found BasisChange
        if (verbose)  // since the generators are NOT in the sublattice
            verboseOutput() << "Converting generators to inequalities avoiding coordinate transformation" << endl;
        Cone<Integer> Copy(Type::cone, Generators);
        Copy.compute(ConeProperty::SupportHyperplanes);
        Inequalities.append(Copy.getSupportHyperplanesMatrix());
        if(Copy.getSublattice().getEquationsMatrix().nr_of_rows()>0){
            Inequalities.append(Copy.getSublattice().getEquationsMatrix()); // must convert equations to inequalities            
            vector<Integer> neg_sum_subspace(dim, 0);
            for (size_t i = 0; i < Copy.getSublattice().getEquationsMatrix().nr_of_rows(); ++i)
                neg_sum_subspace = v_add(neg_sum_subspace, Copy.getSublattice().getEquationsMatrix()[i]);
            v_scalar_multiplication<Integer>(neg_sum_subspace, -1);
            Inequalities.append(neg_sum_subspace);
        }
        if (verbose)
            verboseOutput() << "Conversion finished" << endl;
        Generators = Matrix<Integer>(0, dim);
    }
    else{ // in this case the generators are in the sublattice and we can go via modifyCone
        if (Inequalities.nr_of_rows() != 0 && Generators.nr_of_rows() != 0) {
            if (verbose)
                verboseOutput() << "Converting generators to inequalities" << endl;
            keep_convex_hull_data = true;
            // verbose=true;
            setComputed(ConeProperty::Generators);
            compute(ConeProperty::SupportHyperplanes);
            if (verbose)
                verboseOutput() << "Conversion finished" << endl;
            if (inhomogeneous) {
                Inequalities.append(Dehomogenization);
                modifyCone(Type::inhom_inequalities, Inequalities);
            }
            else
                modifyCone(Type::inequalities, Inequalities);
            // compute(ConeProperty::SupportHyperplanes);
            Generators = Matrix<Integer>(0, dim);  // are contained in the ConvexHullData
            conversion_done = true;
            keep_convex_hull_data = false;
        }
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    assert(Inequalities.nr_of_rows() == 0 || Generators.nr_of_rows() == 0);

    if (Generators.nr_of_rows() != 0) {
        setComputed(ConeProperty::Generators);
        setComputed(ConeProperty::Sublattice);
    }
    
    

    if (Inequalities.nr_of_rows() != 0 && !conversion_done) {
        if (inhomogeneous)
            SupportHyperplanes.append(Dehomogenization);  // dehomogenization is first!
        SupportHyperplanes.append(Inequalities);
        if (inhomogeneous)
            Inequalities.append(Dehomogenization);  // needed in check of symmetrization for Ehrhart series
    }

    checkGrading(false); // do not compute frading denom
    checkDehomogenization();

    if (!precomputed_extreme_rays && SupportHyperplanes.nr_of_rows() > 0) {
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
    // (2) by inequalities stored in SupportHyperplanes.
    // Exceptions: precomputed support hyperplanes (see below) or simultaneous
    // input of generators and inequalities (conversion above)
    //
    // The lattice defining information in the input has been
    // processed and sits in BasisChange.
    //
    // Note that the processing of the inequalities can
    // later on change the lattice.
    //
    // TODO Keep the inequalities in Inequalities,
    // and put only the final support hyperplanes into
    // SupportHyperplanes.

    // read precomputed data

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    if (precomputed_extreme_rays) {
        Generators= find_input_matrix(multi_input_data, Type::extreme_rays);
        setComputed(ConeProperty::Generators);
        setComputed(ConeProperty::ExtremeRays);
        ExtremeRays.sort_by_weights(WeightsGrad, GradAbs);
        set_extreme_rays(vector<bool>(Generators.nr_of_rows(), true));
        BasisMaxSubspace=Matrix<Integer>(0,dim);
        if(contains(multi_input_data,Type::maximal_subspace))
            BasisMaxSubspace=find_input_matrix(multi_input_data,Type::maximal_subspace);
        if(BasisMaxSubspace.nr_of_rows()>0){
            Matrix<Integer> Help = BasisMaxSubspace; // for protection
            Matrix<Integer> Dummy(0,dim);                
            BasisChangePointed.compose_with_passage_to_quotient(Help,Dummy); // now modulo Help, was not yet pointed
        }
        pointed = (BasisMaxSubspace.nr_of_rows() == 0);
        setComputed(ConeProperty::IsPointed);
        setComputed(ConeProperty::MaximalSubspace);
        setComputed(ConeProperty::Sublattice);
        SupportHyperplanes = find_input_matrix(multi_input_data, Type::support_hyperplanes);
        SupportHyperplanes.sort_lex();
        setComputed(ConeProperty::SupportHyperplanes);
        
        size_t test_rank=BasisChangePointed.getRank();
        if( test_rank != BasisChangePointed.to_sublattice(Generators).rank() ||
                test_rank != BasisChangePointed.to_sublattice_dual(SupportHyperplanes).rank())
            throw BadInputException("Precomputed data do not define pointed cone modulo maximal subspace");        
        create_convex_hull_data();
        keep_convex_hull_data=true;
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION
    
    // for integer hull cones the maximal subsapce is known since it is the same
    // as for the original cone
    if(is_inthull_cone){
        if(contains(multi_input_data, Type::subspace)){
            BasisMaxSubspace = find_input_matrix(multi_input_data,Type::subspace);
            setComputed(ConeProperty::MaximalSubspace);
            if(BasisMaxSubspace.nr_of_rows() > 0){
                Matrix<Integer> Help = BasisMaxSubspace; // for protection
                Matrix<Integer> Dummy(0,dim);                
                BasisChangePointed.compose_with_passage_to_quotient(Help,Dummy); // now modulo Help, was not yet pointed
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
        if (isComputed(ConeProperty::Dehomogenization) && isComputed(ConeProperty::Grading))
            throw BadInputException("Grading not allowed for inhomogeneous computations over number fields");
    }

    AddInequalities.resize(0, dim);
    AddGenerators.resize(0, dim);
    
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
void Cone<Integer>::remove_superfluous_inequalities(){
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
void Cone<Integer>::remove_superfluous_equations(){
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
void Cone<Integer>::remove_superfluous_congruences(){
    if (Congruences.nr_of_rows() > 0 && Generators.nr_of_rows() > 0) {
        vector<key_t> essential;        
        size_t cc=Congruences[0].size();
        for (size_t k = 0; k < Congruences.nr_of_rows(); ++k) {
            for(size_t i=0;i< Generators.nr_of_rows(); ++i){
                if (v_scalar_product_vectors_unequal_lungth(Generators[i], Congruences[k]) % Congruences[k][cc - 1] != 0) {  // congruence not satisfied
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
void  Cone<renf_elem_class>::remove_superfluous_congruences(){
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
void Cone<Integer>::prepare_input_constraints(const map<InputType, vector<vector<Integer> > >& multi_input_data) {
    Matrix<Integer> Signs(0, dim), StrictSigns(0, dim);

    SupportHyperplanes = Matrix<Integer>(0, dim); // only initialize here 
    
    Inequalities = Matrix<Integer>(0, dim); //these rhree will be set here
    Equations = Matrix<Integer>(0, dim);
    Congruences = Matrix<Integer>(0, dim + 1);
    
    if(precomputed_extreme_rays)
        return;

    for (const auto& it : multi_input_data) {
        switch (it.first) {
            case Type::strict_inequalities:
            case Type::inequalities:
            case Type::inhom_inequalities:
            case Type::excluded_faces:
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
    bool nonnegative = true;
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
            nonnegative = false;
            break;
        }
    }

    if (!nonnegative)
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
void Cone<Integer>::prepare_input_generators(map<InputType, vector<vector<Integer> > >& multi_input_data,
                                             Matrix<Integer>& LatticeGenerators) {

    if (contains(multi_input_data, Type::vertices)) {
        for (size_t i = 0; i < multi_input_data[Type::vertices].size(); ++i)
            if (multi_input_data[Type::vertices][i][dim - 1] <= 0) {
                throw BadInputException("Vertex has non-positive denominator!");
            }
    }

    if (contains(multi_input_data, Type::polyhedron)) {
        for (size_t i = 0; i < multi_input_data[Type::polyhedron].size(); ++i)
            if (multi_input_data[Type::polyhedron][i][dim - 1] < 0) {
                throw BadInputException("Polyhedron vertex has negative denominator!");
            }
    }

    typename map<InputType, vector<vector<Integer> > >::const_iterator it = multi_input_data.begin();
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
                if (it->second.size() > 1) {
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
void Cone<Integer>::convert_lattice_generators_to_constraints(Matrix<Integer>& LatticeGenerators){

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
    if (Generators.nr_of_rows() == 0 && Inequalities.nr_of_rows() == 0) {
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
Matrix<Integer> Cone<Integer>::prepare_input_type_2(const vector<vector<Integer> >& Input) {
    size_t j;
    size_t nr = Input.size();
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
Matrix<Integer> Cone<Integer>::prepare_input_type_3(const vector<vector<Integer> >& InputV) {
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
Matrix<renf_elem_class> Cone<renf_elem_class>::prepare_input_type_3(const vector<vector<renf_elem_class> >& InputV) {
    assert(false);
    return {};
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::prepare_input_lattice_ideal(map<InputType, vector<vector<Integer> > >& multi_input_data) {
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
    multi_input_data.insert(make_pair(Type::normalization,
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
void Cone<renf_elem_class>::prepare_input_lattice_ideal(map<InputType, vector<vector<renf_elem_class> > >& multi_input_data) {
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
    face_codim_bound = -1;

    keep_convex_hull_data = false;
    conversion_done = false;
    ConvHullData.is_primal = false;  // to i9nitialize it
    
    precomputed_extreme_rays=false;
    precomputed_support_hyperplanes=false;
    
    is_inthull_cone = false;

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

#ifdef ENFNORMALIZ
template <typename Number>
void Cone<Number>::setRenf(renf_class* renf) {
}

template <>
void Cone<renf_elem_class>::setRenf(renf_class* renf) {
    Renf = renf;
    renf_degree = fmpq_poly_degree(renf->get_renf()->nf->pol);
}

#endif
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
                // in the inhomogeneous case: test only generators of tail cone
                positively_graded = false;
                ;
                if (degrees[i] < 0) {
                    nonnegative = false;
                    neg_index = i;
                    neg_value = degrees[i];
                }
            }
        }
        if(compute_grading_denom){
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
    checkGrading(false); // no computation of GradingDenom
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

template <typename Integer>  // computation depends on OriginalMonoidGenerators
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
    return OriginalMonoidGenerators;
}
template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getOriginalMonoidGenerators() {
    compute(ConeProperty::OriginalMonoidGenerators);
    return OriginalMonoidGenerators.get_elements();
}
template <typename Integer>
size_t Cone<Integer>::getNrOriginalMonoidGenerators() {
    compute(ConeProperty::OriginalMonoidGenerators);
    return OriginalMonoidGenerators.nr_of_rows();
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
const Matrix<Integer>& Cone<Integer>::getGeneratorsMatrix() {
    compute(ConeProperty::Generators);
    return Generators;
}

template <typename Integer>
const vector<vector<Integer> >& Cone<Integer>::getGenerators() {
    compute(ConeProperty::Generators);
    return Generators.get_elements();
}

template <typename Integer>
size_t Cone<Integer>::getNrGenerators() {
    compute(ConeProperty::Generators);
    return Generators.nr_of_rows();
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
const vector<pair<vector<key_t>, Integer> >& Cone<Integer>::getTriangulation() {
    compute(ConeProperty::Triangulation);
    return Triangulation;
}

template <typename Integer>
const vector<pair<vector<key_t>, Integer> >& Cone<Integer>::getTriangulation(ConeProperty::Enum quality) {
    if(! (quality == ConeProperty::LatticePointTriangulation || quality == ConeProperty::AllGeneratorsTriangulation
        || quality == ConeProperty::UnimodularTriangulation) ){
        throw BadInputException("Illegal parameter in getTriangulation(ConeProperty::Enum quality)");
    }
    if(isComputed(quality)) // we have already what we want
        return Triangulation;
    if( ! (isComputed(ConeProperty::LatticePointTriangulation) || isComputed(ConeProperty::AllGeneratorsTriangulation)
        || isComputed(ConeProperty::UnimodularTriangulation) ) ){ // ==> none of the refined computed
        compute(quality); // compute the desired one
        return Triangulation;
    }
    // remaining case: the computed refined triangulation is not the wanted one ==> start from scratch
    is_Computed.reset(ConeProperty::Triangulation);
    compute(quality);
    return Triangulation;
}

template <typename Integer>
const vector<vector<bool> >& Cone<Integer>::getOpenFacets() {
    compute(ConeProperty::ConeDecomposition);
    return OpenFacets;
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
void Cone<Integer>::make_StanleyDec_export() {
    if (!StanleyDec_export.empty())
        return;
    assert(isComputed(ConeProperty::StanleyDec));
    auto SD = StanleyDec.begin();
    for (; SD != StanleyDec.end(); ++SD) {
        STANLEYDATA<Integer> NewSt;
        NewSt.key = SD->key;
        convert(NewSt.offsets, SD->offsets);
        sort(NewSt.offsets.access_elements().begin(), NewSt.offsets.access_elements().end());
        StanleyDec_export.push_back(NewSt);
    }
    StanleyDec_export.sort(compareStDec<Integer>);
}

template <typename Integer>
const list<STANLEYDATA<Integer> >& Cone<Integer>::getStanleyDec() {
    compute(ConeProperty::StanleyDec);
    make_StanleyDec_export();
    return StanleyDec_export;
}

template <typename Integer>
list<STANLEYDATA_int>& Cone<Integer>::getStanleyDec_mutable() {
    assert(isComputed(ConeProperty::StanleyDec));
    return StanleyDec;
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

#ifdef ENFNORMALIZ
template <typename Integer>
renf_elem_class Cone<Integer>::getRenfVolume() {
    throw NotComputableException("For the volume of rational polytopes use getVolume()");
    return {};
}

template <>
mpq_class Cone<renf_elem_class>::getVolume() {
    throw NotComputableException("For the volume of algebraic polytopes use getRenfVolume()");
    return 0;
}

template <>
renf_elem_class Cone<renf_elem_class>::getRenfVolume() {
    compute(ConeProperty::RenfVolume);
    return renf_volume;
}
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
    if(!isComputed(ConeProperty::IsTriangulationNested))
        throw NotComputableException("isTriangulationNested() only defined if a triangulation has been computed");
    return triangulation_is_nested;
}
template <typename Integer>
bool Cone<Integer>::isTriangulationPartial() {
    if(!isComputed(ConeProperty::IsTriangulationPartial))
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
    if (!(quality == ConeProperty::Automorphisms || quality == ConeProperty::RationalAutomorphisms ||
          quality == ConeProperty::AmbientAutomorphisms || quality == ConeProperty::CombinatorialAutomorphisms ||
          quality == ConeProperty::EuclideanAutomorphisms)) {
        throw BadInputException("Illegal parameter in getAutomorphismGroup(ConeProperty::Enum quality)");
    }
    compute(quality);
    is_Computed.reset(ConeProperty::Automorphisms);
    is_Computed.reset(ConeProperty::RationalAutomorphisms);
    is_Computed.reset(ConeProperty::AmbientAutomorphisms);
    is_Computed.reset(ConeProperty::CombinatorialAutomorphisms);
    is_Computed.reset(ConeProperty::EuclideanAutomorphisms);
    setComputed(quality);
    return Automs;
}

template <typename Integer>
const AutomorphismGroup<Integer>& Cone<Integer>::getAutomorphismGroup() {
    if (!(isComputed(ConeProperty::Automorphisms) || isComputed(ConeProperty::RationalAutomorphisms) ||
          isComputed(ConeProperty::AmbientAutomorphisms) || isComputed(ConeProperty::CombinatorialAutomorphisms) ||
          isComputed(ConeProperty::EuclideanAutomorphisms))) {
        throw BadInputException("No automorphism group computed. Use getAutomorphismGroup(ConeProperty::Enum quality)");
    }

    return Automs;
}



template <typename Integer>
const map<dynamic_bitset, int>& Cone<Integer>::getFaceLattice() {
    compute(ConeProperty::FaceLattice);
    return FaceLattice;
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

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_lattice_points_in_polytope(ConeProperties& ToCompute) {
    assert(false);
}

#ifdef ENFNORMALIZ
template <>
void Cone<renf_elem_class>::project_and_lift(const ConeProperties& ToCompute,
                                             Matrix<renf_elem_class>& Deg1,
                                             const Matrix<renf_elem_class>& Gens,
                                             const Matrix<renf_elem_class>& Supps,
                                             const Matrix<renf_elem_class>& Congs,
                                             const vector<renf_elem_class> GradingOnPolytope) {
    // if(verbose)
    //    verboseOutput() << "Starting projection" << endl;

    // vector<dynamic_bitset> Pair;
    //  vector<dynamic_bitset> ParaInPair;

    vector<dynamic_bitset> Ind;
    //
    // if(!is_parallelotope){
    Ind = vector<dynamic_bitset>(Supps.nr_of_rows(), dynamic_bitset(Gens.nr_of_rows()));
    for (size_t i = 0; i < Supps.nr_of_rows(); ++i)
        for (size_t j = 0; j < Gens.nr_of_rows(); ++j)
            if (v_scalar_product(Supps[i], Gens[j]) == 0)
                Ind[i][j] = true;
    //}

    size_t rank = BasisChangePointed.getRank();

    Matrix<renf_elem_class> Verts;
    if (isComputed(ConeProperty::Generators)) {
        vector<key_t> choice = identity_key(Gens.nr_of_rows());  // Gens.max_rank_submatrix_lex();
        if (choice.size() >= dim)
            Verts = Gens.submatrix(choice);
    }

    Matrix<mpz_class> Raw(0, Gens.nr_of_columns());

    vector<renf_elem_class> Dummy;
    // project_and_lift_inner<renf_elem_class>(Deg1,Supps,Ind,GradingDenom,rank,verbose,true,Dummy);
    ProjectAndLift<renf_elem_class, mpz_class> PL;
    // if(!is_parallelotope)
    PL = ProjectAndLift<renf_elem_class, mpz_class>(Supps, Ind, rank);
    // else
    //    PL=ProjectAndLift<renf_elem_class,renf_elem_class>(Supps,Pair,ParaInPair,rank);
    PL.set_grading_denom(1);
    PL.set_verbose(verbose);
    PL.set_no_relax(ToCompute.test(ConeProperty::NoRelax));
    PL.set_LLL(false);
    PL.set_vertices(Verts);
    PL.compute();
    PL.put_eg1Points_into(Raw);

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

template <>
void Cone<renf_elem_class>::compute_lattice_points_in_polytope(ConeProperties& ToCompute) {
    if (isComputed(ConeProperty::ModuleGenerators) || isComputed(ConeProperty::Deg1Elements))
        return;
    if (!ToCompute.test(ConeProperty::ModuleGenerators) && !ToCompute.test(ConeProperty::Deg1Elements))
        return;

    if (!isComputed(ConeProperty::Grading) && !isComputed(ConeProperty::Dehomogenization))
        throw BadInputException("Lattice points not computable without grading in the homogeneous case");

    compute(ConeProperty::SupportHyperplanes);
    if (!isComputed(ConeProperty::SupportHyperplanes))
        throw FatalException("Could not compute SupportHyperplanes");

    if (inhomogeneous && ExtremeRaysRecCone.nr_of_rows() > 0) {
        throw BadInputException("Lattice points not computable for unbounded poyhedra");
    }

    // The same procedure as in cone.cpp, but no approximation, and grading always extra first coordinate

    renf_elem_class MinusOne = -1;

    vector<vector<renf_elem_class> > SuppsHelp = SupportHyperplanes.get_elements();
    Matrix<renf_elem_class> Equs = BasisChange.getEquationsMatrix();
    for (size_t i = 0; i < Equs.nr_of_rows(); ++i) {  // add equations as inequalities
        SuppsHelp.push_back(Equs[i]);
        SuppsHelp.push_back(Equs[i]);
        v_scalar_multiplication(SuppsHelp.back(), MinusOne);
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
    SuppsHelp.push_back(ExtraEqu);
    v_scalar_multiplication(ExtraEqu, MinusOne);
    SuppsHelp.push_back(ExtraEqu);

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

    Deg1Elements.resize(0, dim);
    ModuleGenerators.resize(0, dim);
    Matrix<renf_elem_class> DummyCongs(0, 0);
    Matrix<renf_elem_class> DummyResult(0, 0);
    vector<renf_elem_class> dummy_grad(0);

    if (inhomogeneous)
        project_and_lift(ToCompute, DummyResult, GradGen, Supps, DummyCongs, dummy_grad);
    else
        project_and_lift(ToCompute, DummyResult, GradGen, Supps, DummyCongs, dummy_grad);

    // In this version, the lattice points are transferresd into the cone
    // in project_and_lift below.

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

    if (!inhomogeneous && !isComputed(ConeProperty::Grading))
        throw NotComputableException("Volume needs a grading in the homogeneous case");
    if (getRank() != dim)
        throw NotComputableException("Normaliz requires full dimension for volume");
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
        Grad_mpz.push_back(Grad[i].get_num());
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
template <typename IntegerFC>
void Cone<Integer>::compute_full_cone(ConeProperties& ToCompute) {
    
#ifdef NMZ_EXTENDED_TESTS
    if(!using_GMP<IntegerFC>() && !using_renf<IntegerFC>() && test_arith_overflow_full_cone)
        throw ArithmeticException(0);    
#endif
    
    if (ToCompute.test(ConeProperty::IsPointed) && Grading.size() == 0) {
        if (verbose) {
            verboseOutput() << "Checking pointedness first" << endl;
        }
        ConeProperties Dualize;
        Dualize.set(ConeProperty::SupportHyperplanes);
        Dualize.set(ConeProperty::ExtremeRays);
        compute(Dualize);
    }

    Matrix<IntegerFC> FC_Gens;

    BasisChangePointed.convert_to_sublattice(FC_Gens, Generators);
    Full_Cone<IntegerFC> FC(FC_Gens, !(ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid) || ToCompute.test(ConeProperty::AllGeneratorsTriangulation)));
    // !ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid) blocks make_prime in full_cone.cpp

    /* activate bools in FC */
    
    if(ToCompute.test(ConeProperty::FullConeDynamic)){
        FC.do_supphyps_dynamic=true;
        if(IntHullNorm.size() > 0)
            BasisChangePointed.convert_to_sublattice_dual(FC.IntHullNorm,IntHullNorm);
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
    if (ToCompute.test(ConeProperty::Triangulation)) {
        FC.keep_triangulation = true;
    }

    if (ToCompute.test(ConeProperty::ConeDecomposition)) {
        FC.do_cone_dec = true;
    }
    if (ToCompute.test(ConeProperty::Multiplicity)) {
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
    if (ToCompute.test(ConeProperty::Deg1Elements)) {
        FC.do_deg1_elements = true;
    }
    if (ToCompute.test(ConeProperty::StanleyDec)) {
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
    if (ToCompute.test(ConeProperty::KeepOrder) && isComputed(ConeProperty::OriginalMonoidGenerators)) {
        FC.keep_order = true;
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

    if (ToCompute.test(ConeProperty::Automorphisms)){
        FC.do_automorphisms = true;
        FC.quality_of_automorphisms = AutomParam::integral;
    }
    
    if (ToCompute.test(ConeProperty::RationalAutomorphisms)){
        FC.do_automorphisms = true;
        FC.quality_of_automorphisms = AutomParam::rational;
    }

    /*
        if (ToCompute.test(ConeProperty::ExploitAutomsMult)) {
            FC.exploit_automs_mult = true;
        }
        if (ToCompute.test(ConeProperty::ExploitAutomsVectors)) {
            FC.exploit_automs_vectors = true;
        }
        FC.autom_codim_vectors = autom_codim_vectors;
        FC.autom_codim_mult = autom_codim_mult;
    }*/

    if (SupportHyperplanes.nr_of_rows() != 0) {
        BasisChangePointed.convert_to_sublattice_dual(FC.Support_Hyperplanes, SupportHyperplanes);
    }
    if (isComputed(ConeProperty::SupportHyperplanes)) {
        FC.is_Computed.set(ConeProperty::SupportHyperplanes);
        FC.do_all_hyperplanes = false;
    }

    if (is_approximation)
        give_data_of_approximated_cone_to(FC);

    bool must_triangulate = FC.do_h_vector || FC.do_Hilbert_basis || FC.do_multiplicity || FC.do_Stanley_dec ||
                            FC.do_module_rank || FC.do_module_gens_intcl || FC.do_bottom_dec || FC.do_hsop ||
                            FC.do_integrally_closed || FC.keep_triangulation || FC.do_integrally_closed || FC.do_cone_dec ||
                            FC.do_determinants || FC.do_triangulation_size || FC.do_deg1_elements || FC.do_default_mode;

    // Do we really need the Full_Cone?

    if (!must_triangulate && !FC.do_automorphisms && isComputed(ConeProperty::SupportHyperplanes) &&
        isComputed(ConeProperty::ExtremeRays) && !ToCompute.test(ConeProperty::Grading) &&
        !ToCompute.test(ConeProperty::IsPointed) && !ToCompute.test(ConeProperty::ClassGroup))
        return;
    
    // restore if usaeful
    if (!must_triangulate && keep_convex_hull_data && ConvHullData.SLR.equal(BasisChangePointed) &&
        ConvHullData.nr_threads == omp_get_max_threads() && ConvHullData.Generators.nr_of_rows() > 0) {
        FC.keep_order = true;
        FC.restore_previous_vcomputation(ConvHullData, true);  // true = primal
    }

    /* do the computation */

    try {
        try {
            FC.compute();
        } catch (const NotIntegrallyClosedException&) {
        }
        setComputed(ConeProperty::Sublattice);
        // make sure we minimize the excluded faces if requested
        if (ToCompute.test(ConeProperty::ExcludedFaces) || ToCompute.test(ConeProperty::SupportHyperplanes)) {
            FC.prepare_inclusion_exclusion();
        }
        extract_data(FC, ToCompute);
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
        compute_full_cone<IntegerFC>(ToCompute);
    }
}

#ifdef ENFNORMALIZ
template <>
template <typename IntegerFC>
void Cone<renf_elem_class>::compute_full_cone(ConeProperties& ToCompute) {
    if (ToCompute.test(ConeProperty::IsPointed) && Grading.size() == 0) {
        if (verbose) {
            verboseOutput() << "Checking pointedness first" << endl;
        }
        ConeProperties Dualize;
        Dualize.set(ConeProperty::SupportHyperplanes);
        Dualize.set(ConeProperty::ExtremeRays);
        compute(Dualize);
    }

    Matrix<renf_elem_class> FC_Gens;

    BasisChangePointed.convert_to_sublattice(FC_Gens, Generators);
    Full_Cone<renf_elem_class> FC(FC_Gens, !ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid));
    // !ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid) blocks make_prime in full_cone.cpp

    /* activate bools in FC */
    
    if(ToCompute.test(ConeProperty::FullConeDynamic)){
        FC.do_supphyps_dynamic=true;
        if(IntHullNorm.size() > 0)
            BasisChangePointed.convert_to_sublattice_dual(FC.IntHullNorm,IntHullNorm);
    }

    FC.verbose = verbose;
    FC.renf_degree = renf_degree;

    FC.inhomogeneous = inhomogeneous;

    if (ToCompute.test(ConeProperty::Triangulation)) {
        FC.keep_triangulation = true;
    }

    if (ToCompute.test(ConeProperty::Multiplicity) || ToCompute.test(ConeProperty::Volume)) {
        FC.do_multiplicity = true;
    }

    if (ToCompute.test(ConeProperty::ConeDecomposition)) {
        FC.do_cone_dec = true;
    }

    if (ToCompute.test(ConeProperty::TriangulationDetSum)) {
        FC.do_determinants = true;
    }
    if (ToCompute.test(ConeProperty::TriangulationSize)) {
        FC.do_triangulation = true;
    }
    if (ToCompute.test(ConeProperty::KeepOrder)) {
        FC.keep_order = true;
    }

    /* Give extra data to FC */
    if (isComputed(ConeProperty::ExtremeRays)) {
        FC.Extreme_Rays_Ind = ExtremeRaysIndicator;
        FC.is_Computed.set(ConeProperty::ExtremeRays);
    }

    if (inhomogeneous) {
        BasisChangePointed.convert_to_sublattice_dual_no_div(FC.Truncation, Dehomogenization);
    }

    if (SupportHyperplanes.nr_of_rows() != 0) {
        BasisChangePointed.convert_to_sublattice_dual(FC.Support_Hyperplanes, SupportHyperplanes);
    }
    if (isComputed(ConeProperty::SupportHyperplanes)) {
        FC.is_Computed.set(ConeProperty::SupportHyperplanes);
        FC.do_all_hyperplanes = false;
    }

    if (isComputed(ConeProperty::Grading)) {
        BasisChangePointed.convert_to_sublattice_dual(FC.Grading, Grading);
        FC.is_Computed.set(ConeProperty::Grading);
    }

    if(ToCompute.test(ConeProperty::Automorphisms)){
        FC.do_automorphisms = true;
        FC.quality_of_automorphisms = AutomParam::algebraic;
    }

    bool must_triangulate = FC.do_h_vector || FC.do_Hilbert_basis || FC.do_multiplicity || FC.do_Stanley_dec ||
                            FC.do_module_rank || FC.do_module_gens_intcl || FC.do_bottom_dec || FC.do_hsop ||
                            FC.do_integrally_closed || FC.keep_triangulation || FC.do_integrally_closed || FC.do_cone_dec ||
                            FC.do_determinants || FC.do_triangulation_size || FC.do_deg1_elements || FC.do_default_mode;

    FC.keep_convex_hull_data = keep_convex_hull_data;

    if (!must_triangulate && keep_convex_hull_data && ConvHullData.SLR.equal(BasisChangePointed) &&
        ConvHullData.nr_threads == omp_get_max_threads() && ConvHullData.Generators.nr_of_rows() > 0) {
        FC.keep_order = true;
        FC.restore_previous_vcomputation(ConvHullData, true);  // true=primal
    }

    /* do the computation */

    try {
        try {
            FC.compute();
        } catch (const NotIntegrallyClosedException&) {
        }
        setComputed(ConeProperty::Sublattice);
        // make sure we minimize the excluded faces if requested

        extract_data(FC, ToCompute);
        if (isComputed(ConeProperty::IsPointed) && pointed)
            setComputed(ConeProperty::MaximalSubspace);
    } catch (const NonpointedException&) {
        if(precomputed_extreme_rays)
            throw BadInputException("Cone not pointed for precomputed data");
        setComputed(ConeProperty::Sublattice);
        extract_data(FC, ToCompute);
        if (ToCompute.test(ConeProperty::Deg1Elements) || ToCompute.test(ConeProperty::ModuleGenerators) ||
            ToCompute.test(ConeProperty::Volume))
            throw NotComputableException("Normaliz requires pointedness for lattice points or volume");

        if (verbose) {
            verboseOutput() << "Cone not pointed. Restarting computation." << endl;
        }
        FC = Full_Cone<renf_elem_class>(Matrix<renf_elem_class>(1));  // to kill the old FC (almost)
        pass_to_pointed_quotient();
        compute_full_cone<renf_elem_class>(ToCompute);
    }
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_integer_hull() {
    if (isComputed(ConeProperty::IntegerHull))
        return;

    if (verbose) {
        verboseOutput() << "Computing integer hull" << endl;
    }

    Matrix<Integer> IntHullGen;
    bool IntHullComputable = true;
    // size_t nr_extr = 0;
    if (inhomogeneous) {
        if ((!using_renf<Integer>() && !isComputed(ConeProperty::HilbertBasis)) ||
            (using_renf<Integer>() && !isComputed(ConeProperty::ModuleGenerators)))
            IntHullComputable = false;
        if (using_renf<Integer>())
            IntHullGen = ModuleGenerators;
        else {
            IntHullGen = HilbertBasis;  // not defined in case of renf_elem_class
            IntHullGen.append(ModuleGenerators);
        }
    }
    else {
        if (!isComputed(ConeProperty::Deg1Elements))
            IntHullComputable = false;
        IntHullGen = Deg1Elements;
    }
    ConeProperties IntHullCompute;
    IntHullCompute.set(ConeProperty::SupportHyperplanes);
    if (!IntHullComputable) {
        errorOutput() << "Integer hull not computable: no integer points available." << endl;
        throw NotComputableException(IntHullCompute);
    }

    if (IntHullGen.nr_of_rows() == 0) {
        IntHullGen.append(vector<Integer>(dim, 0));  // we need a non-empty input matrix
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    if (!inhomogeneous || HilbertBasis.nr_of_rows() == 0) {  // polytoe since homogeneous or recession coe = 0
        // nr_extr = IntHullGen.extreme_points_first(verbose);         // don't need a norm here since all points have degree or level 1
    }
    else {  // now an unbounded polyhedron
        if (isComputed(ConeProperty::Grading)) {
            //nr_extr = IntHullGen.extreme_points_first(verbose,Grading);
            IntHullNorm  =Grading;
        }
        else {
            if (isComputed(ConeProperty::SupportHyperplanes)) {
                IntHullNorm = SupportHyperplanes.find_inner_point();
                // nr_extr = IntHullGen.extreme_points_first(verbose,aux_grading);
            }
        }
    }

    /* if (verbose) {
        verboseOutput() << nr_extr << " extreme points found" << endl;
    }*/

    // IntHullGen.pretty_print(cout);
    
    if (IntHullCone != NULL)
        delete IntHullCone;
    
    if (!using_renf<Integer>())
        IntHullCone = new Cone<Integer>(InputType::cone_and_lattice, IntHullGen, Type::subspace, BasisMaxSubspace);
    else
        IntHullCone = new Cone<Integer>(InputType::cone, IntHullGen, Type::subspace, BasisMaxSubspace);
    /* if (nr_extr != 0)  // we suppress the ordering in full_cone only if we have found few extreme rays
        IntHullCompute.set(ConeProperty::KeepOrder);*/

    IntHullCone->inhomogeneous = true;  // inhomogeneous;
    IntHullCone->is_inthull_cone = true;
    IntHullCone->HilbertBasis = HilbertBasis;
    IntHullCone->ModuleGenerators = ModuleGenerators;
    IntHullCone->setComputed(ConeProperty::HilbertBasis);
    IntHullCone->setComputed(ConeProperty::ModuleGenerators);
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
        ToCompute.test(ConeProperty::Projection) || nr_cone_gen > 0 || nr_latt_gen > 0 ||
        SupportHyperplanes.nr_of_rows() > 2 * dim ||
        SupportHyperplanes.nr_of_rows() <= BasisChangePointed.getRank() + 50 / (BasisChangePointed.getRank() + 1))
        return;
    if (ToCompute.test(ConeProperty::HilbertBasis))
        ToCompute.set(ConeProperty::DualMode);
    if (ToCompute.test(ConeProperty::Deg1Elements) &&
        !(ToCompute.test(ConeProperty::HilbertSeries) || ToCompute.test(ConeProperty::Multiplicity)))
        ToCompute.set(ConeProperty::DualMode);
    return;
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
void Cone<Integer>::set_extended_tests(ConeProperties& ToCompute){
    if(ToCompute.test(ConeProperty::TestArithOverflowFullCone))
        test_arith_overflow_full_cone=true;
    if(ToCompute.test(ConeProperty::TestArithOverflowDualMode))
        test_arith_overflow_dual_mode=true;
    if(ToCompute.test(ConeProperty::TestArithOverflowDescent))
        test_arith_overflow_descent=true;
    if(ToCompute.test(ConeProperty::TestArithOverflowProjAndLift))
        test_arith_overflow_proj_and_lift=true;
    if(ToCompute.test(ConeProperty::TestSmallPyramids))
        test_small_pyramids=true;
    if(ToCompute.test(ConeProperty::TestLargePyramids)){
        test_large_pyramids=true;
        test_small_pyramids=true;
    }
    if(ToCompute.test(ConeProperty::TestLinearAlgebraGMP)){
        test_linear_algebra_GMP=true;
        if(isComputed(ConeProperty::OriginalMonoidGenerators)){
            Matrix<MachineInteger> GenLL;
            convert(GenLL,Generators);
            if(GenLL.rank()==dim){
                MachineInteger test=GenLL.full_rank_index(); // to test full_rank_index with "overflow"
                assert(convertTo<Integer>(test)==internal_index);
            }
            MachineInteger test_rank;
            test_rank=GenLL.row_echelon_reduce(); // same reason as above
            assert(convertTo<Integer>(test_rank)==Generators.rank());
        }
    
    }
    if(ToCompute.test(ConeProperty::TestSimplexParallel)){
        test_simplex_parallel=true;
        ToCompute.set(ConeProperty::NoSubdivision);
        SimplexParallelEvaluationBound =0;
    }
    
    if(ToCompute.test(ConeProperty::TestLibNormaliz)){
        run_additional_tests_libnormaliz();        
    }
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
ConeProperties Cone<Integer>::compute(ConeProperties ToCompute) {
    
    // cout << "AAAA " << ToCompute << endl;
    
    size_t nr_computed_at_start = is_Computed.count();

    handle_dynamic(ToCompute);

    set_parallelization();
    nmz_interrupted = 0;
    
    ToCompute.reset(is_Computed);
    if (ToCompute.none()) {
        return ConeProperties();
    }
    
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
    
    if(ToCompute.test(ConeProperty::NoGradingDenom)){
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
     if ( ToCompute.test(ConeProperty::Automorphisms) || ToCompute.test(ConeProperty::RationalAutomorphisms) ||
          ToCompute.test(ConeProperty::AmbientAutomorphisms) || ToCompute.test(ConeProperty::CombinatorialAutomorphisms) ||
          ToCompute.test(ConeProperty::EuclideanAutomorphisms))
        throw BadInputException("automorphism groups only computable with nauty");
#endif

    // default_mode=ToCompute.test(ConeProperty::DefaultMode);

    if (ToCompute.test(ConeProperty::BigInt)) {
        if (!using_GMP<Integer>())
            throw BadInputException("BigInt can only be set for cones of Integer type GMP");
        change_integer_type = false;
    }

    if (ToCompute.test(ConeProperty::KeepOrder)) {
        if (!isComputed(ConeProperty::OriginalMonoidGenerators) && !dual_original_generators)
            throw BadInputException("KeepOrder can only be set if the cone or the dual has original generators");
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    if (BasisMaxSubspace.nr_of_rows() > 0 && !isComputed(ConeProperty::MaximalSubspace)) {
        BasisMaxSubspace = Matrix<Integer>(0, dim);
        if(ToCompute.test(ConeProperty::FullConeDynamic)) // if integer hull is computed
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

    ToCompute.check_conflicting_variants();
    ToCompute.set_preconditions(inhomogeneous, using_renf<Integer>());
    if(ToCompute.test(ConeProperty::Sublattice) && !isComputed(ConeProperty::Generators))
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
    
    
    
    ToCompute.reset(is_Computed);
    if (ToCompute.none()) {
        return ConeProperties();
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

    /* if(!inhomogeneous && ToCompute.test(ConeProperty::NoGradingDenom) && Grading.size()==0)
        throw BadInputException("Options require an explicit grading."); */

    if (conversion_done)
        compute_generators(ToCompute);

    ToCompute.reset(is_Computed);
    if (ToCompute.none()) {     // IMPORTANT: do not use goals() at this point because it would prevent
        return ConeProperties();  // HSOP if HSOP is applied to an already computed Hilbert series
    }                             // Alternatively one could do complete_HilbertSeries_comp(ToCompute)
                                  // at the very beginning of this function
    
    check_integrally_closed(ToCompute); // check cheap necessary conditions

    try_multiplicity_of_para(ToCompute);
    ToCompute.reset(is_Computed);

    try_multiplicity_by_descent(ToCompute);
    ToCompute.reset(is_Computed);

    try_symmetrization(ToCompute);
    ToCompute.reset(is_Computed);

    complete_HilbertSeries_comp(ToCompute);

    complete_sublattice_comp(ToCompute);
    if (ToCompute.goals().none()) {
        return ConeProperties();
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    compute_projection(ToCompute);

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    treat_polytope_as_being_hom_defined(ToCompute);  // if necessary

    ToCompute.reset(is_Computed);  // already computed

    complete_HilbertSeries_comp(ToCompute);

    complete_sublattice_comp(ToCompute);
    if (ToCompute.goals().none()) {
        return ConeProperties();
    }

    try_approximation_or_projection(ToCompute);

    ToCompute.reset(is_Computed);
    if (ToCompute.goals().none()) {
        return ConeProperties();
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
        return ConeProperties();
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    /* preparation: get generators if necessary */
    
    // cout << "SSSS " << ToCompute.full_cone_goals() << endl;

    if (ToCompute.full_cone_goals().any()) {
        compute_generators(ToCompute);
        if (!isComputed(ConeProperty::Generators)) {
            throw FatalException("Could not get Generators.");
        }
    }
    
    /* cout << "TTTT " << ToCompute.full_cone_goals() << endl;
    cout << "TTTT IIIII  " << ToCompute.full_cone_goals() << endl;*/
    
    if (rees_primary && (ToCompute.test(ConeProperty::ReesPrimaryMultiplicity) || ToCompute.test(ConeProperty::Multiplicity) ||
                         ToCompute.test(ConeProperty::HilbertSeries) || ToCompute.test(ConeProperty::DefaultMode))) {
        ReesPrimaryMultiplicity = compute_primary_multiplicity();
        setComputed(ConeProperty::ReesPrimaryMultiplicity);
    }

    ToCompute.reset(is_Computed);
    complete_HilbertSeries_comp(ToCompute);
    complete_sublattice_comp(ToCompute);
    if (ToCompute.goals().none()) {
        return ConeProperties();
    }

    try_Hilbert_Series_from_lattice_points(ToCompute);
    ToCompute.reset(is_Computed);
    complete_HilbertSeries_comp(ToCompute);
    complete_sublattice_comp(ToCompute);
    if (ToCompute.goals().none()) {
        return ConeProperties();
    }
    
    /* cout << "UUUU " << ToCompute.full_cone_goals() << endl;
    cout << "UUUU All  " << ToCompute << endl;
    cout << "UUUU  IIIII  " << ToCompute.full_cone_goals() << endl;*/

    // the computation of the full cone
    if (ToCompute.full_cone_goals().any()) {
        if (change_integer_type) {
            try {
                compute_full_cone<MachineInteger>(ToCompute);
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
                compute_full_cone<Integer>(ToCompute);
            }
            else {
                try {
                    compute_full_cone<Integer>(ToCompute);
                } catch (const ArithmeticException& e) {  // the nonly reason for failure is an overflow in a degree computation
                    if (verbose) {                        // so we can relax in default mode
                        verboseOutput() << e.what() << endl;
                        verboseOutput() << "Reducing computation goals." << endl;
                    }
                    ToCompute.reset(ConeProperty::HilbertBasis);
                    ToCompute.reset(ConeProperty::HilbertSeries);
                    compute_full_cone<Integer>(ToCompute);
                }
            }
        }
    }
    
    // cout << " VVVV " << ToCompute.full_cone_goals() << endl;

     if (ToCompute.test(ConeProperty::WitnessNotIntegrallyClosed)) {
        find_witness(ToCompute);
    }
    
    if(precomputed_extreme_rays && inhomogeneous)
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

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    complete_HilbertSeries_comp(ToCompute);
    complete_sublattice_comp(ToCompute);

    compute_vertices_float(ToCompute);
    compute_supp_hyps_float(ToCompute);

    if (ToCompute.test(ConeProperty::WeightedEhrhartSeries))
        compute_weighted_Ehrhart(ToCompute);
    ToCompute.reset(is_Computed);

    if (ToCompute.test(ConeProperty::Integral))
        compute_integral(ToCompute);
    ToCompute.reset(is_Computed);

    if (ToCompute.test(ConeProperty::VirtualMultiplicity))
        compute_virt_mult(ToCompute);
    ToCompute.reset(is_Computed);

    /* check if everything is computed */
    ToCompute.reset(is_Computed);  // remove what is now computed
    if (ToCompute.test(ConeProperty::Deg1Elements) && isComputed(ConeProperty::Grading)) {
        // can happen when we were looking for a witness earlier
        if(nr_computed_at_start == is_Computed.count()){ // Prevention of infinte loop
            throw FatalException("FATAL: Compute without effect would be repeated. Inform the authors !");
        }
        compute(ToCompute);
    }    

    if (!ToCompute.test(ConeProperty::DefaultMode) && ToCompute.goals().any()) {
        throw NotComputableException(ToCompute.goals());
    }

    ToCompute.reset_compute_options();

    return ToCompute;
}

#ifdef ENFNORMALIZ
template <>
ConeProperties Cone<renf_elem_class>::compute(ConeProperties ToCompute) {
    
    handle_dynamic(ToCompute);
    
#ifndef NMZ_NAUTY
     if ( ToCompute.test(ConeProperty::Automorphisms) || ToCompute.test(ConeProperty::RationalAutomorphisms) ||
          ToCompute.test(ConeProperty::AmbientAutomorphisms) || ToCompute.test(ConeProperty::CombinatorialAutomorphisms) ||
          ToCompute.test(ConeProperty::EuclideanAutomorphisms))
        throw BadInputException("automorphism groups only computable with nauty");
#endif

    ToCompute.reset(is_Computed);
    if (ToCompute.none()) {
        return ConeProperties();
    }

    set_parallelization();

    if (ToCompute.test(ConeProperty::GradingIsPositive)) {
        if (Grading.size() == 0)
            throw BadInputException("No grading declared that could be positive.");
        else
            setComputed(ConeProperty::Grading);
    }
    
    if(ToCompute.test(ConeProperty::NoGradingDenom)){
        GradingDenom = 1;
        setComputed(ConeProperty::GradingDenom);
    }

    change_integer_type = false;

    if (BasisMaxSubspace.nr_of_rows() > 0 && !isComputed(ConeProperty::MaximalSubspace)) {
        BasisMaxSubspace = Matrix<renf_elem_class>(0, dim);
        compute(ConeProperty::MaximalSubspace);
    }

    ToCompute.check_Q_permissible(false);  // before implications!
    ToCompute.reset(is_Computed);

    ToCompute.set_preconditions(inhomogeneous, using_renf<renf_elem_class>());

    ToCompute.check_Q_permissible(true);  // after implications!

    // ToCompute.prepare_compute_options(inhomogeneous, using_renf<renf_elem_class>());

    // ToCompute.set_default_goals(inhomogeneous,using_renf<renf_elem_class>());
    ToCompute.check_sanity(inhomogeneous);

    /* preparation: get generators if necessary */
    compute_generators(ToCompute);

    if (!isComputed(ConeProperty::Generators)) {
        throw FatalException("Could not get Generators.");
    }

    ToCompute.reset(is_Computed);  // already computed
    if (ToCompute.goals().none()) {
        return ConeProperties();
    }

    prepare_volume_computation(ToCompute);

    // the actual computation

    if (isComputed(ConeProperty::SupportHyperplanes))
        ToCompute.reset(ConeProperty::DefaultMode);

    if (ToCompute.full_cone_goals().any()  || ToCompute.test(ConeProperty::Volume)) {
        compute_full_cone<renf_elem_class>(ToCompute);
    }
    compute_projection(ToCompute);
    
    if(precomputed_extreme_rays && inhomogeneous)
        compute_affine_dim_and_recession_rank();

    compute_lattice_points_in_polytope(ToCompute);

    make_face_lattice(ToCompute);

    compute_combinatorial_automorphisms(ToCompute);
    compute_euclidean_automorphisms(ToCompute);

    if (ToCompute.test(ConeProperty::IntegerHull)) {
        compute_integer_hull();
    }
    
    compute_refined_triangulation(ToCompute);

    complete_sublattice_comp(ToCompute);

    /* check if everything is computed */
    ToCompute.reset(is_Computed);  // remove what is now computed

    compute_vertices_float(ToCompute);
    compute_supp_hyps_float(ToCompute);
    ToCompute.reset(is_Computed);

    if (!ToCompute.test(ConeProperty::DefaultMode) && ToCompute.goals().any()) {
        throw NotComputableException(ToCompute.goals());
    }
    ToCompute.reset_compute_options();
    return ToCompute;
}
#endif

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
            throw BadInputException("Dehomogenization does not vanish on maximal subspace.");
        }
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_generators(ConeProperties& ToCompute) {
    // create Generators from SupportHyperplanes

    if (!isComputed(ConeProperty::Generators) && (SupportHyperplanes.nr_of_rows() != 0 || inhomogeneous)) {
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
void Cone<Integer>::pass_to_pointed_quotient(){
    
    if(isComputed(ConeProperty::MaximalSubspace))
        return;
    
    BasisChangePointed = BasisChange;
    Matrix<Integer> DualGen = SupportHyperplanes; // must priotect SupportHyperplanes!
    BasisChangePointed.compose_with_passage_to_quotient(BasisMaxSubspace,DualGen);
    
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
    if(!using_GMP<IntegerFC>() && !using_renf<IntegerFC>() && test_arith_overflow_full_cone)
        throw ArithmeticException(0);    
#endif

    pass_to_pointed_quotient();

    // restrict the supphyps to efficient sublattice and push to quotient mod subspace
    Matrix<IntegerFC> Dual_Gen_Pointed;
    BasisChangePointed.convert_to_sublattice_dual(Dual_Gen_Pointed, SupportHyperplanes);
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
        Dual_Cone.restore_previous_vcomputation(ConvHullData, false);  // false=dual
    }

    Dual_Cone.keep_convex_hull_data = keep_convex_hull_data;

    try {
        Dual_Cone.dualize_cone();
    } catch (const NonpointedException&) {
    };  // we don't mind if the dual cone is not pointed

    if (Dual_Cone.isComputed(ConeProperty::SupportHyperplanes)) {
        if (keep_convex_hull_data) {
            extract_convex_hull_data(Dual_Cone, false);  // false means: dual
        }
        // get the extreme rays of the primal cone
        // BasisChangePointed.convert_from_sublattice(Generators,
        //                 Dual_Cone.getSupportHyperplanes());
        extract_supphyps(Dual_Cone, Generators, false);  // false means: no dualization
        setComputed(ConeProperty::Generators);
        check_gens_vs_reference();

        // get minmal set of support_hyperplanes if possible
        if (Dual_Cone.isComputed(ConeProperty::ExtremeRays)) {
            Matrix<IntegerFC> Supp_Hyp = Dual_Cone.getGenerators().submatrix(Dual_Cone.getExtremeRays());
            BasisChangePointed.convert_from_sublattice_dual(SupportHyperplanes, Supp_Hyp);
            if (using_renf<Integer>())
                SupportHyperplanes.standardize_rows();
            norm_dehomogenization(BasisChangePointed.getRank());
            SupportHyperplanes.sort_lex();
            setComputed(ConeProperty::SupportHyperplanes);
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
        
        checkGrading(! ToCompute.test(ConeProperty::NoGradingDenom));
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
        set_extreme_rays(vector<bool>(Generators.nr_of_rows(), true));  // here since they get sorted
        setComputed(ConeProperty::ExtremeRays);
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
    if(!using_GMP<IntegerFC>() && test_arith_overflow_dual_mode)
        throw ArithmeticException(0);    
#endif

    bool do_only_Deg1_Elements = ToCompute.test(ConeProperty::Deg1Elements) && !ToCompute.test(ConeProperty::HilbertBasis);

    if (isComputed(ConeProperty::Generators) && SupportHyperplanes.nr_of_rows() == 0) {
        if (verbose) {
            verboseOutput() << "Computing support hyperplanes for the dual mode:" << endl;
        }
        ConeProperties Dualize;
        Dualize.set(ConeProperty::SupportHyperplanes);
        Dualize.set(ConeProperty::ExtremeRays);
        if (ToCompute.test(ConeProperty::KeepOrder))
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

    if (do_only_Deg1_Elements && Grading.size()==0){
        if(Generators.nr_of_rows()>0){
            throw BadInputException("Need grading to compute degree 1 elements and cannot find one.");
        }
        else
            Grading=vector<Integer>(dim,0);
    }


    if (SupportHyperplanes.nr_of_rows() == 0 && !isComputed(ConeProperty::SupportHyperplanes)) {
        throw FatalException("Could not get SupportHyperplanes.");
    }

    Matrix<IntegerFC> Inequ_on_Ker;
    BasisChangePointed.convert_to_sublattice_dual(Inequ_on_Ker, SupportHyperplanes);

    vector<IntegerFC> Truncation;
    if (inhomogeneous) {
        BasisChangePointed.convert_to_sublattice_dual_no_div(Truncation, Dehomogenization);
    }
    if (do_only_Deg1_Elements) {
        // in this case the grading acts as truncation and it is a NEW inequality
        if(ToCompute.test(ConeProperty::NoGradingDenom))
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
template <typename Integer>
void Cone<Integer>::check_gens_vs_reference() {
    if (ReferenceGenerators.nr_of_rows() > 0) {
        if (!Generators.equal(ReferenceGenerators)) {
            Triangulation.clear();
            StanleyDec.clear();
            is_Computed.reset(ConeProperty::Triangulation);
            is_Computed.reset(ConeProperty::StanleyDec);
            is_Computed.reset(ConeProperty::TriangulationSize);
            is_Computed.reset(ConeProperty::TriangulationDetSum);
            is_Computed.reset(ConeProperty::IsTriangulationPartial);
            is_Computed.reset(ConeProperty::IsTriangulationNested);
            is_Computed.reset(ConeProperty::ConeDecomposition);
        }
    }
}

//---------------------------------------------------------------------------

// This function creates convex hull data from precomputed support hyperplanes and extreme rays
// so that these can be used in interactive mode for the modification of the originally constructed cone
// In principle it works like extract_convex_hull_data below.
template <typename Integer>
void Cone<Integer>::create_convex_hull_data() {
    
    ConvHullData.is_primal=true;
    
    ConvHullData.SLR = BasisChangePointed;
    ConvHullData.nr_threads = omp_get_max_threads();
    ConvHullData.HypCounter=vector<size_t>(ConvHullData.nr_threads);
    for(int i=0;i<ConvHullData.nr_threads;++i)
        ConvHullData.HypCounter[i]=i+1;
    ConvHullData.old_nr_supp_hyps=SupportHyperplanes.nr_of_rows();
    
    size_t nr_extreme_rays = ExtremeRays.nr_of_rows();
    // no better idea
    ConvHullData.Comparisons.resize(nr_extreme_rays);
    ConvHullData.nrTotalComparisons = 0;

    ConvHullData.in_triang = vector<bool>(nr_extreme_rays, true);
    ConvHullData.GensInCone = identity_key(nr_extreme_rays);
    ConvHullData.nrGensInCone = nr_extreme_rays;
    
    ConvHullData.Generators=ExtremeRays;
    
    ConvHullData.Facets.clear();
    
    size_t rank=ExtremeRays.rank();

    for (auto& Fac : SupportHyperplanes.get_elements()) {
        FACETDATA<Integer> Ret;
        Ret.Hyp=Fac;
        Ret.GenInHyp.resize(nr_extreme_rays);
        size_t nr_gens_in_hyp=0;
        for (size_t i = 0; i < nr_extreme_rays; ++i) {
            Integer p=v_scalar_product(Fac,ConvHullData.Generators[i]);
            if(p<0)
                throw BadInputException("Incompatible precomputed data: wextreme rays and support hyperplanes inconsitent");
            Ret.GenInHyp[i]=0;
            if(p==0){
                Ret.GenInHyp[i]=1;
                nr_gens_in_hyp++;
            }
        }

        Ret.BornAt = 0;  // no better choice
        Ret.Mother = 0;  // ditto
        Ret.Ident = ConvHullData.HypCounter[0];
        ConvHullData.HypCounter[0]+=ConvHullData.nr_threads; // we use only residue class 0 mod nr_threads
        Ret.simplicial = (nr_gens_in_hyp==rank-1);
        
        ConvHullData.Facets.push_back(Ret);        
    }
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
    if(isComputed(ConeProperty::RecessionRank) || !inhomogeneous)
        return;
    compute(ConeProperty::ExtremeRays);
    vector<key_t> level0key;
    Matrix<Integer> Help=BasisChangePointed.to_sublattice(ExtremeRays);
    vector<Integer> HelpDehom=BasisChangePointed.to_sublattice_dual(Dehomogenization);
    for(size_t i=0; i < Help.nr_of_rows(); ++i){
        if(v_scalar_product(Help[i],HelpDehom)==0)
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
    
    if((isComputed(ConeProperty::AffineDim) && isComputed(ConeProperty::RecessionRank)) || !inhomogeneous)
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

    if (FC.isComputed(ConeProperty::Generators)) {
        BasisChangePointed.convert_from_sublattice(Generators, FC.getGenerators());
        setComputed(ConeProperty::Generators);
        check_gens_vs_reference();
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

    Integer local_grading_denom;
    if(is_Computed.goals_using_grading(inhomogeneous).any()) // in this case we do not pull
        local_grading_denom = GradingDenom;                  // the grading from FC
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

    if (FC.isComputed(ConeProperty::ExtremeRays)) {
        set_extreme_rays(FC.getExtremeRays());
    }
    if (FC.isComputed(ConeProperty::SupportHyperplanes)) {
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
        setComputed(ConeProperty::SupportHyperplanes);
    }
    if (FC.isComputed(ConeProperty::TriangulationSize)) {
        TriangulationSize = FC.totalNrSimplices;
        triangulation_is_nested = FC.triangulation_is_nested;
        triangulation_is_partial = FC.triangulation_is_partial;
        setComputed(ConeProperty::TriangulationSize);
        setComputed(ConeProperty::IsTriangulationPartial);
        setComputed(ConeProperty::IsTriangulationNested);
        is_Computed.reset(ConeProperty::Triangulation);
        Triangulation.clear();  // to get rid of a previously computed triangulation
    }
    if (FC.isComputed(ConeProperty::TriangulationDetSum)) {
        convert(TriangulationDetSum, FC.detSum);
        setComputed(ConeProperty::TriangulationDetSum);
    }

    if (FC.isComputed(ConeProperty::Triangulation)) {
        
        is_Computed.reset(ConeProperty::LatticePointTriangulation); // must reset these friends
        is_Computed.reset(ConeProperty::AllGeneratorsTriangulation); // when the basic triangulation 
        is_Computed.reset(ConeProperty::UnimodularTriangulation);  // is recomputed
        
        size_t tri_size = FC.Triangulation.size();
        FC.Triangulation.sort(compareKeys<IntegerFC>);  // necessary to make triangulation unique
        Triangulation = vector<pair<vector<key_t>, Integer> >(tri_size);
        if (FC.isComputed(ConeProperty::ConeDecomposition))
            OpenFacets.resize(tri_size);
        SHORTSIMPLEX<IntegerFC> simp;
        for (size_t i = 0; i < tri_size; ++i) {
            simp = FC.Triangulation.front();
            Triangulation[i].first.swap(simp.key);
            /* sort(Triangulation[i].first.begin(), Triangulation[i].first.end()); -- no longer allowed here because of
             * ConeDecomposition. Done in full_cone.cpp, transfer_triangulation_to top */
            if (FC.isComputed(ConeProperty::TriangulationDetSum))
                convert(Triangulation[i].second, simp.vol);
            else
                Triangulation[i].second = 0;
            if (FC.isComputed(ConeProperty::ConeDecomposition))
                OpenFacets[i].swap(simp.Excluded);
            FC.Triangulation.pop_front();
        }
        if (FC.isComputed(ConeProperty::ConeDecomposition))
            setComputed(ConeProperty::ConeDecomposition);
        setComputed(ConeProperty::Triangulation);
    }

    if (FC.isComputed(ConeProperty::StanleyDec)) {
        StanleyDec.clear();
        StanleyDec.splice(StanleyDec.begin(), FC.StanleyDec);
        setComputed(ConeProperty::StanleyDec);
    }

    if (isComputed(ConeProperty::Triangulation) || isComputed(ConeProperty::TriangulationSize) ||
        isComputed(ConeProperty::TriangulationDetSum) || isComputed(ConeProperty::StanleyDec))
        ReferenceGenerators = Generators;

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
        if(isComputed(ConeProperty::Sublattice))
            compute_affine_dim_and_recession_rank();
    }
    
    if (FC.isComputed(ConeProperty::ModuleRank)) {
        module_rank = FC.getModuleRank();
        setComputed(ConeProperty::ModuleRank);
    }

    if (FC.isComputed(ConeProperty::Multiplicity) && !using_renf<Integer>()) {
        if (!inhomogeneous) {
            multiplicity = FC.getMultiplicity();
            setComputed(ConeProperty::Multiplicity);
        }
        else if (FC.isComputed(ConeProperty::ModuleRank)) {
            multiplicity = FC.getMultiplicity() * module_rank;
            setComputed(ConeProperty::Multiplicity);
        }
    }

#ifdef ENFNORMALIZ
    if (FC.isComputed(ConeProperty::Multiplicity) && using_renf<Integer>()) {
        renf_volume = FC.renf_multiplicity;
        // setComputed(ConeProperty::Multiplicity);
        setComputed(ConeProperty::Volume);
        setComputed(ConeProperty::RenfVolume);
        euclidean_volume = renf_volume.get_d();
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
        Automs.order = FC.Automs.order;
        Automs.Qualities = FC.Automs.Qualities;

        vector<key_t> SuppHypsKey, ExtRaysKey, VerticesKey, GensKey;

        Automs.GenPerms = extract_permutations(FC.Automs.GenPerms, FC.Automs.GensRef, ExtremeRays, true, GensKey);
        if (inhomogeneous) {
            Automs.ExtRaysPerms =
                extract_permutations(FC.Automs.GenPerms, FC.Automs.GensRef, ExtremeRaysRecCone, true, ExtRaysKey);
            Automs.VerticesPerms =
                extract_permutations(FC.Automs.GenPerms, FC.Automs.GensRef, VerticesOfPolyhedron, true, VerticesKey);
        }
        else {
            Automs.ExtRaysPerms = Automs.GenPerms;
            ExtRaysKey = GensKey;
        }

        Automs.LinFormPerms =
            extract_permutations(FC.Automs.LinFormPerms, FC.Automs.LinFormsRef, SupportHyperplanes, false, SuppHypsKey);
        Automs.SuppHypsPerms = Automs.LinFormPerms;

        Automs.GenOrbits = extract_subsets(FC.Automs.GenOrbits, FC.Automs.GensRef.nr_of_rows(), GensKey);
        sort_individual_vectors(Automs.GenOrbits);
        if (inhomogeneous) {
            Automs.VerticesOrbits = extract_subsets(FC.Automs.GenOrbits, FC.Automs.GensRef.nr_of_rows(), VerticesKey);
            sort_individual_vectors(Automs.VerticesOrbits);

            Automs.ExtRaysOrbits = extract_subsets(FC.Automs.GenOrbits, FC.Automs.GensRef.nr_of_rows(), ExtRaysKey);
            sort_individual_vectors(Automs.ExtRaysOrbits);
        }
        else {
            Automs.ExtRaysOrbits = Automs.GenOrbits;
        }

        Automs.LinFormOrbits = extract_subsets(FC.Automs.LinFormOrbits, FC.Automs.LinFormsRef.nr_of_rows(), SuppHypsKey);
        sort_individual_vectors(Automs.LinFormOrbits);
        Automs.SuppHypsOrbits = Automs.LinFormOrbits;

        if (ToCompute.test(ConeProperty::Automorphisms))
            setComputed(ConeProperty::Automorphisms);
        /*if(ToCompute.test(ConeProperty::AmbientAutomorphisms))
            setComputed(ConeProperty::AmbientAutomorphisms);*/
        if (ToCompute.test(ConeProperty::RationalAutomorphisms))
            setComputed(ConeProperty::RationalAutomorphisms);
        if (FC.isComputed(ConeProperty::ExploitAutomsVectors))
            setComputed(ConeProperty::ExploitAutomsVectors);
        if (FC.isComputed(ConeProperty::ExploitAutomsMult))
            setComputed(ConeProperty::ExploitAutomsMult);
    }

    /* if (FC.isComputed(ConeProperty::MaximalSubspace) &&
                                   !isComputed(ConeProperty::MaximalSubspace)) {
        BasisChangePointed.convert_from_sublattice(BasisMaxSubspace, FC.Basis_Max_Subspace);
        check_vanishing_of_grading_and_dehom();
        setComputed(ConeProperty::MaximalSubspace);
    }*/

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
                                                           vector<key_t>& Key) {
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
    for (size_t i = 0; i < ConeVectors.nr_of_rows(); ++i) {
        vector<IntegerFC> search;
        if (primal)
            BasisChangePointed.convert_to_sublattice(search, ConeVectors[i]);
        else
            BasisChangePointed.convert_to_sublattice_dual(search, ConeVectors[i]);
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
    
    if(isComputed(ConeProperty::IsIntegrallyClosed) && !ToCompute.test(ConeProperty::WitnessNotIntegrallyClosed))
        return;
    
    if(!ToCompute.test(ConeProperty::IsIntegrallyClosed) && !isComputed(ConeProperty::HilbertBasis)){
        return;
    }
    
    if(!isComputed(ConeProperty::IsIntegrallyClosed)){
        unit_group_index = 1;
        if (BasisMaxSubspace.nr_of_rows() > 0)
            compute_unit_group_index();
        setComputed(ConeProperty::UnitGroupIndex);
        
        if (internal_index != 1 || unit_group_index != 1){
            integrally_closed = false;
            setComputed(ConeProperty::IsIntegrallyClosed);
            return;
        }    
    }

    if (!isComputed(ConeProperty::HilbertBasis))
        return;

    if (HilbertBasis.nr_of_rows() > OriginalMonoidGenerators.nr_of_rows()) {
        integrally_closed = false;
        setComputed(ConeProperty::IsIntegrallyClosed);
        if(!ToCompute.test(ConeProperty::WitnessNotIntegrallyClosed))
            return;
    }
    find_witness(ToCompute);
    setComputed(ConeProperty::IsIntegrallyClosed);
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_unit_group_index() {
    assert(isComputed(ConeProperty::MaximalSubspace));
    // we want to compute in the maximal linear subspace
    Sublattice_Representation<Integer> Sub(BasisMaxSubspace, true);
    Matrix<Integer> origens_in_subspace(0, dim);

    // we must collect all original generators that lie in the maximal subspace

    for (size_t i = 0; i < OriginalMonoidGenerators.nr_of_rows(); ++i) {
        size_t j;
        for (j = 0; j < SupportHyperplanes.nr_of_rows(); ++j) {
            if (v_scalar_product(OriginalMonoidGenerators[i], SupportHyperplanes[j]) != 0)
                break;
        }
        if (j == SupportHyperplanes.nr_of_rows())
            origens_in_subspace.append(OriginalMonoidGenerators[i]);
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
        gens_quot = BasisChangePointed.to_sublattice(OriginalMonoidGenerators);
        hilb_quot = BasisChangePointed.to_sublattice(HilbertBasis);
    }
    Matrix<Integer>& gens = pointed ? OriginalMonoidGenerators : gens_quot;
    Matrix<Integer>& hilb = pointed ? HilbertBasis : hilb_quot;
    integrally_closed = true;
    
    set<vector<Integer> > gens_set;
    gens_set.insert(gens.get_elements().begin(),gens.get_elements().end());
    integrally_closed=true;
    for (long h = 0; h < nr_hilb; ++h) {
        if(gens_set.find(hilb[h])==gens_set.end()){
            integrally_closed = false;
            if(ToCompute.test(ConeProperty::WitnessNotIntegrallyClosed)){
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
    
    if(using_renf<Integer>())
        return;
    if (!isComputed(ConeProperty::OriginalMonoidGenerators)) {
        OriginalMonoidGenerators = Input;
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
    ExtremeRays = Generators.submatrix(ext);  // extreme rays of the homogenized cone
    ExtremeRaysIndicator = ext;
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
            nlp = convertTo<long long>(expansion[1]);
        }
        number_lattice_points = nlp;
        setComputed(ConeProperty::NumberLatticePoints);
    }

    // we want to be able to convert HS ohr EhrS to hsop denom if
    // they have been computed without

    if (! (ToCompute.test(ConeProperty::HSOP) && !isComputed(ConeProperty::HSOP) &&
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
    np = num_params.find(NumParam::autom_codim_bound_mult);
    if (np != num_params.end())
        setAutomCodimBoundMult(np->second);
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
    FaceLattice.clear();
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

//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::try_symmetrization(ConeProperties& ToCompute) {
    if (dim <= 1)
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
            polynomial += "(x[" + to_string((unsigned long long)i + 1) + "]+" + to_string((unsigned long long)j) + ")*";
    }
    polynomial += "1";
    mpz_class fact = 1;
    for (unsigned long multiplicitie : multiplicities) {
        for (size_t j = 1; j < multiplicitie; ++j)
            fact *= j;
    }
    polynomial += "/" + fact.get_str() + ";";

#ifdef NMZ_COCOA

    map<InputType, Matrix<Integer> > SymmInput;
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
    SymmCone->setNrCoeffQuasiPol(HSeries.get_nr_coeff_quasipol());
    SymmCone->HSeries.set_period_bounded(HSeries.get_period_bounded());
    SymmCone->setVerbose(verbose);
    ConeProperties SymmToCompute;
    SymmToCompute.set(ConeProperty::SupportHyperplanes);
    SymmToCompute.set(ConeProperty::WeightedEhrhartSeries, ToCompute.test(ConeProperty::HilbertSeries));
    SymmToCompute.set(ConeProperty::VirtualMultiplicity, ToCompute.test(ConeProperty::Multiplicity));
    SymmToCompute.set(ConeProperty::BottomDecomposition, ToCompute.test(ConeProperty::BottomDecomposition));
    SymmToCompute.set(ConeProperty::NoGradingDenom, ToCompute.test(ConeProperty::NoGradingDenom));
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
    if (BasisMaxSubspace.nr_of_rows() > 0)
        throw NotComputableException("Integral not computable for polyhedra containing an affine space of dim > 0");
    if (isComputed(ConeProperty::Integral) || !ToCompute.test(ConeProperty::Integral))
        return;
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
    if (!isComputed(ConeProperty::SupportHyperplanes))
        compute(ConeProperty::SupportHyperplanes);
    if (!isComputed(ConeProperty::MaximalSubspace))
        compute(ConeProperty::MaximalSubspace);

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

    if (inhomogeneous && (!ToCompute.test(ConeProperty::HilbertBasis) && !ToCompute.test(ConeProperty::NumberLatticePoints)))
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
        SupportHyperplanes.remove_row(Dehomogenization);
        setComputed(ConeProperty::SupportHyperplanes);
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
    if (inhomogeneous)
        NeededHere.set(ConeProperty::AffineDim);
    if (!inhomogeneous){
        NeededHere.set(ConeProperty::Grading);
        if(ToCompute.test(ConeProperty::NoGradingDenom))
            NeededHere.set(ConeProperty::NoGradingDenom);
    }
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
            SupportHyperplanes.remove_row(Dehomogenization);
            setComputed(ConeProperty::SupportHyperplanes);
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
            multiplicity = module_rank;  // of the recession cone;
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
    
    SupportHyperplanes.sort_lex();

    Matrix<Integer> Supps(SupportHyperplanes);
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
        euclidean_volume = mpq_to_nmz_float(volume) * euclidean_corr_factor();
        setComputed(ConeProperty::EuclideanVolume);
        setComputed(ConeProperty::Volume);
        return;
    }

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
    map<InputType, Matrix<Integer> > DefVolCone;
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
    return;
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
    if (raw_degrees[non_zero] < 0){
        v_scalar_multiplication(Simplex[non_zero], MinusOne);  // makes this degree > 0
        raw_degrees[non_zero]*=-1;
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
    VolCone.compute(ConeProperty::Multiplicity, ConeProperty::NoBottomDec, ConeProperty::NoGradingDenom);
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

    if (isComputed(ConeProperty::Generators))
        compute_projection_from_gens(GradOrDehomProj);
    else
        compute_projection_from_constraints(GradOrDehomProj, ToCompute);

    setComputed(ConeProperty::ProjectCone);
}
//---------------------------------------------------------------------------
template <typename Integer>
void Cone<Integer>::compute_projection_from_gens(const vector<Integer>& GradOrDehomProj) {
    Matrix<Integer> GensProj = Generators.select_columns(projection_coord_indicator);
    map<InputType, Matrix<Integer> > ProjInput;
    ProjInput[Type::cone] = GensProj;
    if (GradOrDehomProj.size() > 0) {
        if (inhomogeneous)
            ProjInput[Type::dehomogenization] = GradOrDehomProj;
        else
            ProjInput[Type::grading] = GradOrDehomProj;
    }
    
    if(ProjCone != NULL)
        delete ProjCone;
    
    ProjCone = new Cone<Integer>(ProjInput);
    if (verbose)
        verboseOutput() << "Computing projection from generators" << endl;
    ProjCone->compute(ConeProperty::SupportHyperplanes);
}

//---------------------------------------------------------------------------
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

    map<InputType, Matrix<Integer> > ProjInput;
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
    ProjCone->compute(ConeProperty::SupportHyperplanes, ConeProperty::ExtremeRays);
}

#ifdef ENFNORMALIZ
template <>
void Cone<renf_elem_class>::compute_projection_from_constraints(const vector<renf_elem_class>& GradOrDehomProj,
                                                                ConeProperties& ToCompute) {
    assert(false);
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::try_multiplicity_by_descent(ConeProperties& ToCompute) {
    
    if(inhomogeneous) // in this case multiplicity defined algebraically, not as the volume of a polytope
        return;

    if (!ToCompute.test(ConeProperty::Multiplicity) || ToCompute.test(ConeProperty::NoDescent) ||
        ToCompute.test(ConeProperty::ExploitAutomsMult))
        return;

    if (ToCompute.test(ConeProperty::HilbertSeries) || ToCompute.test(ConeProperty::WeightedEhrhartSeries) ||
        ToCompute.test(ConeProperty::VirtualMultiplicity) || ToCompute.test(ConeProperty::Integral) ||
        ToCompute.test(ConeProperty::Triangulation) || ToCompute.test(ConeProperty::StanleyDec) ||
        ToCompute.test(ConeProperty::TriangulationDetSum) || ToCompute.test(ConeProperty::TriangulationSize) ||
        ToCompute.test(ConeProperty::Symmetrize))
        return;

    if (!ToCompute.test(ConeProperty::Descent)) {  // we use Descent gy default if there are not too many facets
        if (SupportHyperplanes.nr_of_rows() > 10 * dim + 1 ||
            SupportHyperplanes.nr_of_rows() <= BasisChangePointed.getRank())
            return;
    }
    
    if(ToCompute.test(ConeProperty::NoGradingDenom))
        compute(ConeProperty::ExtremeRays, ConeProperty::Grading, ConeProperty::NoGradingDenom); 
    else
        compute(ConeProperty::ExtremeRays, ConeProperty::Grading);
    
    if(isComputed(ConeProperty::Multiplicity)) // can happen !!
        return;

    try_multiplicity_of_para(ToCompute);  // we try this first, even if Descent is set
    if (isComputed(ConeProperty::Multiplicity))
        return;

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
            FF = DescentSystem<Integer>(ExtremeRays, SupportHyperplanes, GradingEmb);
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

#ifdef ENFNORMALIZ
template <>
void Cone<renf_elem_class>::try_multiplicity_by_descent(ConeProperties& ToCompute) {
    assert(false);
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::try_multiplicity_of_para(ConeProperties& ToCompute) {
    if (((!inhomogeneous && !ToCompute.test(ConeProperty::Multiplicity)) ||
         (inhomogeneous && !ToCompute.test(ConeProperty::Volume))) ||
        !check_parallelotope())
        return;

    SupportHyperplanes.remove_row(Dehomogenization);
    setComputed(ConeProperty::SupportHyperplanes);
    setComputed(ConeProperty::MaximalSubspace);
    setComputed(ConeProperty::Sublattice);
    pointed = true;
    setComputed(ConeProperty::IsPointed);
    if(inhomogeneous){
        affine_dim=dim-1;
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
    gen = SupportHyperplanes.submatrix(CornerKey).kernel(false)[0];
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

    if (!ToCompute.test(ConeProperty::EhrhartSeries) && !ToCompute.test(ConeProperty::Triangulation) &&
        !ToCompute.test(ConeProperty::ConeDecomposition) && !ToCompute.test(ConeProperty::StanleyDec))
        return;  // homogeneous treatment not necessary

    compute(ConeProperty::Generators, ConeProperty::AffineDim);
    ToCompute.reset(is_Computed);

    if (affine_dim == -1 && Generators.nr_of_rows() > 0) {
        throw NotComputableException(
            "Ehrhart series, triangulation, cone decomposition, Stanley decomposition  not computable for empty polytope with "
            "non-subspace recession cone.");
    }

    for (size_t i = 0; i < Generators.nr_of_rows(); ++i)
        if (v_scalar_product(Dehomogenization, Generators[i]) <= 0)
            throw NotComputableException(
                "Ehrhart series, triangulation, cone decomposition, Stanley decomposition  not computable for unbounded "
                "polyhedra.");

    // swap(VerticesOfPolyhedron,ExtremeRays);

    vector<Integer> SaveGrading;
    swap(Grading, SaveGrading);
    bool save_grad_computed = isComputed(ConeProperty::Grading);
    Integer SaveDenom = GradingDenom;
    bool save_denom_computed = isComputed(ConeProperty::GradingDenom);

    bool saveFaceLattice = ToCompute.test(ConeProperty::FaceLattice);  // better to do this in the
    bool saveFVector = ToCompute.test(ConeProperty::FVector);          // original inhomogeneous settimg
    ToCompute.reset(ConeProperty::FaceLattice);
    ToCompute.reset(ConeProperty::FVector);

    bool save_Hilbert_series_to_comp =
        ToCompute.test(ConeProperty::HilbertSeries);  // on the homogenous cone EhrhartSeries is used
    // bool save_Explicit_Hilbert_series_to_comp=ToCompute.test(ConeProperty::ExplicitHilbertSeries);
    bool save_Hilbert_series_is_comp = isComputed(ConeProperty::HilbertSeries);
    // bool save_Explicit_Hilbert_series_is_comp=isComputed(ConeProperty::ExplicitHilbertSeries);
    ToCompute.reset(ConeProperty::HilbertSeries);
    HilbertSeries SaveHSeries;
    swap(HSeries, SaveHSeries);

    mpq_class save_mult = multiplicity;
    bool save_Multiplicity_is_comp = isComputed(ConeProperty::Multiplicity);
    bool save_Multiplicity_to_comp = ToCompute.test(ConeProperty::Multiplicity);

    assert(isComputed(ConeProperty::Dehomogenization));
    vector<Integer> SaveDehomogenization;
    swap(Dehomogenization, SaveDehomogenization);
    bool save_dehom_computed = isComputed(ConeProperty::Dehomogenization);

    bool save_hilb_bas = ToCompute.test(ConeProperty::HilbertBasis);

    bool save_module_rank = ToCompute.test(ConeProperty::ModuleRank);

    ToCompute.reset(ConeProperty::VerticesOfPolyhedron);  //
    ToCompute.reset(ConeProperty::ModuleRank);            //
    ToCompute.reset(ConeProperty::RecessionRank);         //  these 5 will be computed below
    // ToCompute.reset(ConeProperty::AffineDim);             //  <--------- already done
    ToCompute.reset(ConeProperty::VerticesOfPolyhedron);  //

    bool save_mod_gen_over_ori = ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid);
    ToCompute.reset(ConeProperty::ModuleGeneratorsOverOriginalMonoid);

    inhomogeneous = false;
    Grading = SaveDehomogenization;
    setComputed(ConeProperty::Grading);
    if (save_hilb_bas || save_module_rank || save_mod_gen_over_ori)
        ToCompute.set(ConeProperty::Deg1Elements);
    ToCompute.reset(ConeProperty::HilbertBasis);

    compute(ToCompute);  // <--------------------------------------------------- Here we compute
    // cout << "IS "<< is_Computed << endl;

    VerticesOfPolyhedron = ExtremeRays;
    ExtremeRaysRecCone.resize(0, dim);  // in the homogeneous case ExtremeRays=ExtremeEaysRecCone
    setComputed(ConeProperty::VerticesOfPolyhedron);

    is_Computed.reset(ConeProperty::IsDeg1ExtremeRays);  // makes no sense in the inhomogeneous case
    deg1_extreme_rays = false;

    compute(ConeProperty::Sublattice);
    if (!isComputed(ConeProperty::Sublattice))
        throw FatalException("Could not compute sublattice");

    if (isComputed(ConeProperty::Deg1Elements)) {
        swap(ModuleGenerators, Deg1Elements);
        is_Computed.reset(ConeProperty::Deg1Elements);
        setComputed(ConeProperty::HilbertBasis);
        setComputed(ConeProperty::ModuleGenerators);
        module_rank = ModuleGenerators.nr_of_rows();
        setComputed(ConeProperty::ModuleRank);
        if (save_mod_gen_over_ori) {
            ModuleGeneratorsOverOriginalMonoid = ModuleGenerators;
            setComputed(ConeProperty::ModuleGeneratorsOverOriginalMonoid);
        }
    }

    if (isComputed(ConeProperty::HilbertSeries)) {
        setComputed(ConeProperty::EhrhartSeries);
        swap(EhrSeries, HSeries);
        swap(HSeries, SaveHSeries);
    }
    ToCompute.set(ConeProperty::HilbertSeries, save_Hilbert_series_to_comp);
    setComputed(ConeProperty::HilbertSeries, save_Hilbert_series_is_comp);
    // ToCompute.set(ConeProperty::ExplicitHilbertSeries,save_Explicit_Hilbert_series_to_comp);
    // setComputed(ConeProperty::ExplicitHilbertSeries,save_Explicit_Hilbert_series_is_comp);

    multiplicity = save_mult;
    setComputed(ConeProperty::Multiplicity, save_Multiplicity_is_comp);
    ToCompute.set(ConeProperty::Multiplicity, save_Multiplicity_to_comp);

    ToCompute.set(ConeProperty::HilbertBasis, save_hilb_bas);
    setComputed(ConeProperty::Dehomogenization, save_dehom_computed);
    swap(SaveDehomogenization, Dehomogenization);
    setComputed(ConeProperty::Grading, save_grad_computed);
    setComputed(ConeProperty::GradingDenom, save_denom_computed);
    swap(SaveGrading, Grading);
    GradingDenom = SaveDenom;

    ToCompute.set(ConeProperty::FaceLattice, saveFaceLattice);
    ToCompute.set(ConeProperty::FVector, saveFVector);

    inhomogeneous = true;

    recession_rank = BasisMaxSubspace.nr_of_rows();
    setComputed(ConeProperty::RecessionRank);

    if (affine_dim == -1) {
        volume = 0;
        euclidean_volume = 0;
    }

    /*
    if(isComputed(ConeProperty::Sublattice)){
        if (get_rank_internal() == recession_rank) {
            affine_dim = -1;
        } else {
            affine_dim = get_rank_internal()-1;
        }
        setComputed(ConeProperty::AffineDim);
    }*/
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::try_Hilbert_Series_from_lattice_points(const ConeProperties& ToCompute) {
    if (!inhomogeneous || !isComputed(ConeProperty::ModuleGenerators) ||
        !(isComputed(ConeProperty::RecessionRank) && recession_rank == 0) || !isComputed(ConeProperty::Grading))
        return;

    multiplicity = ModuleGenerators.nr_of_rows();
    setComputed(ConeProperty::Multiplicity);

    if (!ToCompute.test(ConeProperty::HilbertSeries))
        return;

    if (verbose)
        verboseOutput() << "Computing Hilbert series from lattice points" << endl;

    vector<num_t> h_vec_pos(1), h_vec_neg;

    for (size_t i = 0; i < ModuleGenerators.nr_of_rows(); ++i) {
        long deg = convertTo<long>(v_scalar_product(Grading, ModuleGenerators[i]));
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

#ifdef ENFNORMALIZ
template <>
void Cone<renf_elem_class>::try_Hilbert_Series_from_lattice_points(const ConeProperties& ToCompute) {
    assert(false);
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::make_Hilbert_series_from_pos_and_neg(const vector<num_t>& h_vec_pos, const vector<num_t>& h_vec_neg) {
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

struct FaceInfo {
    // dynamic_bitset ExtremeRays;
    dynamic_bitset HypsContaining;
    int max_cutting_out;
    bool max_subset;
    // bool max_prec;
    bool simple;
};

bool face_compare(const pair<dynamic_bitset, FaceInfo>& a, const pair<dynamic_bitset, FaceInfo>& b) {
    return (a.first < b.first);
}

template <typename Integer>
void Cone<Integer>::make_face_lattice(const ConeProperties& ToCompute) {
    bool something_to_do = (ToCompute.test(ConeProperty::FaceLattice) && !isComputed(ConeProperty::FaceLattice)) ||
                           (ToCompute.test(ConeProperty::FVector) && !isComputed(ConeProperty::FVector)) ||
                           (ToCompute.test(ConeProperty::Incidence) && !isComputed(ConeProperty::Incidence));

    if (!something_to_do)
        return;

    if (verbose)
        verboseOutput() << "Computing incidence/face lattice/f-vector ... " << endl;

    FaceLattice.clear();
    f_vector.clear();

    compute(ConeProperty::ExtremeRays, ConeProperty::SupportHyperplanes); // both necessary
                                         // since ExtremeRays can be comuted without SupportHyperplanes
                                         // if the cone is not full dimensional

    bool bound_codim = false;
    if (face_codim_bound >= 0)
        bound_codim = true;

    size_t nr_supphyps = SupportHyperplanes.nr_of_rows();
    size_t nr_extr_rec_cone = ExtremeRaysRecCone.nr_of_rows();
    size_t nr_gens = ExtremeRays.nr_of_rows();
    size_t nr_vert = nr_gens - nr_extr_rec_cone;

    SuppHypInd.clear();
    SuppHypInd.resize(nr_supphyps);

    // order of the extreme rays:
    //
    // first the vertices of polyhedron (in the inhomogeneous case)
    // then the extreme rays of the (recession) cone
    //

    // order of the extreme rays:
    //
    // first the vertices of polyhedron (in the inhomogeneous case)
    // then the extreme rays of the (recession) cone
    //

    bool skip_remaining = false;
    std::exception_ptr tmp_exception;

    int nr_simplial_facets = 0;

#pragma omp parallel for
    for (size_t i = 0; i < nr_supphyps; ++i) {
        if (skip_remaining)
            continue;

        int nr_gens_in_hyp = 0;

        SuppHypInd[i].resize(nr_gens);

        try {
            INTERRUPT_COMPUTATION_BY_EXCEPTION

            if (inhomogeneous) {
                for (size_t j = 0; j < nr_vert; ++j) {
                    if (v_scalar_product(SupportHyperplanes[i], VerticesOfPolyhedron[j]) == 0) {
                        nr_gens_in_hyp++;
                        SuppHypInd[i][j] = true;
                    }
                }
            }

            for (size_t j = 0; j < nr_extr_rec_cone; ++j) {
                if (v_scalar_product(SupportHyperplanes[i], ExtremeRaysRecCone[j]) == 0) {
                    nr_gens_in_hyp++;
                    SuppHypInd[i][j + nr_vert] = true;
                }
            }

            if (nr_gens_in_hyp == (int)(getRank() - 1))
#pragma omp atomic
                nr_simplial_facets++;

        } catch (const std::exception&) {
            tmp_exception = std::current_exception();
            skip_remaining = true;
#pragma omp flush(skip_remaining)
        }
    }
    if (!(tmp_exception == 0))
        std::rethrow_exception(tmp_exception);

    if (verbose)
        verboseOutput() << "Simplicial facets " << nr_simplial_facets << " of " << nr_supphyps << endl;

    if (ToCompute.test(ConeProperty::Incidence))
        setComputed(ConeProperty::Incidence);

    if (!ToCompute.test(ConeProperty::FVector) && !ToCompute.test(ConeProperty::FaceLattice)) {
        if (verbose)
            verboseOutput() << "done" << endl;
        return;
    }

    dynamic_bitset SimpleVert(nr_gens);
    size_t nr_simpl = 0;
    for (size_t j = 0; j < nr_gens; ++j) {
        size_t nr_cont = 0;
        for (size_t i = 0; i < nr_supphyps; ++i)
            if (SuppHypInd[i][j])
                nr_cont++;
        if (nr_cont == getRank() - 1) {
            SimpleVert[j] = 1;
            nr_simpl++;
        }
    }
    if (verbose)
        verboseOutput() << "Cosimplicial gens " << nr_simpl << " of " << nr_gens << endl;

    bool use_simple_vert = (10 * nr_simpl > nr_gens);

    vector<size_t> prel_f_vector(dim + 1, 0);

    dynamic_bitset the_cone(nr_gens);
    the_cone.set();
    dynamic_bitset empty(nr_supphyps);
    dynamic_bitset AllFacets(nr_supphyps);
    AllFacets.set();

    map<dynamic_bitset, pair<dynamic_bitset, dynamic_bitset> > NewFaces;
    map<dynamic_bitset, pair<dynamic_bitset, dynamic_bitset> > WorkFaces;

    WorkFaces[empty] = make_pair(empty, AllFacets);  // start with the full cone
    dynamic_bitset ExtrRecCone(nr_gens);             // in the inhomogeneous case
    if (inhomogeneous) {                             // we exclude the faces of the recession cone
        for (size_t j = 0; j < nr_extr_rec_cone; ++j)
            ExtrRecCone[j + nr_vert] = 1;
        ;
    }

    Matrix<Integer> EmbeddedSuppHyps = BasisChange.to_sublattice_dual(SupportHyperplanes);
    Matrix<MachineInteger> EmbeddedSuppHyps_MI;
    if (change_integer_type)
        BasisChange.convert_to_sublattice_dual(EmbeddedSuppHyps_MI, SupportHyperplanes);

    /*for(int i=0;i< 10000;++i){ // for pertubation of order of supphyps
        int j=rand()%nr_supphyps;
        int k=rand()%nr_supphyps;
        swap(SuppHypInd[j],SuppHypInd[k]);
        swap(EmbeddedSuppHyps[j],EmbeddedSuppHyps[k]);
        if(change_integer_type)
            swap(EmbeddedSuppHyps_MI[j],EmbeddedSuppHyps_MI[k]);
    }*/

    vector<dynamic_bitset> Unit_bitset(nr_supphyps);
    for (size_t i = 0; i < nr_supphyps; ++i) {
        Unit_bitset[i].resize(nr_supphyps);
        Unit_bitset[i][i] = 1;
    }

    long codimension_so_far = 0;  // the lower bound for the codimension so far

    const long VERBOSE_STEPS = 50;
    const size_t RepBound = 1000;
    bool report_written = false;

    size_t total_inter = 0;
    size_t avoided_inter = 0;
    size_t total_new = 0;
    size_t total_simple = 1;  // the full cone is cosimplicial
    size_t total_max_subset = 0;

    while (true) {
        codimension_so_far++;  // codimension of faces put into NewFaces
        bool CCC = false;
        if (codimension_so_far == 1)
            CCC = true;

        if (bound_codim && codimension_so_far > face_codim_bound + 1)
            break;
        size_t nr_faces = WorkFaces.size();
        if (verbose) {
            if (report_written)
                verboseOutput() << endl;
            verboseOutput() << "codim " << codimension_so_far - 1 << " faces to process " << nr_faces << endl;
            report_written = false;
        }

        long step_x_size = nr_faces - VERBOSE_STEPS;

#pragma omp parallel
        {
            size_t Fpos = 0;
            auto F = WorkFaces.begin();
            list<pair<dynamic_bitset, FaceInfo> > FreeFaces, Faces;
            pair<dynamic_bitset, FaceInfo> fr;
            fr.first.resize(nr_gens);
            fr.second.HypsContaining.resize(nr_supphyps);
            for (size_t i = 0; i < nr_supphyps; ++i) {
                FreeFaces.push_back(fr);
            }

#pragma omp for schedule(dynamic)
            for (size_t kkk = 0; kkk < nr_faces; ++kkk) {
                if (skip_remaining)
                    continue;

                for (; kkk > Fpos; ++Fpos, ++F)
                    ;
                for (; kkk < Fpos; --Fpos, --F)
                    ;

                if (verbose && nr_faces >= RepBound) {
#pragma omp critical(VERBOSE)
                    while ((long)(kkk * VERBOSE_STEPS) >= step_x_size) {
                        step_x_size += nr_faces;
                        verboseOutput() << "." << flush;
                        report_written = true;
                    }
                }

                Faces.clear();

                try {
                    INTERRUPT_COMPUTATION_BY_EXCEPTION

                    dynamic_bitset beta_F = F->second.first;

                    bool F_simple = ((long)F->first.count() == codimension_so_far - 1);

#pragma omp atomic
                    prel_f_vector[codimension_so_far - 1]++;

                    dynamic_bitset Gens = the_cone;  // make indicator vector of *F
                    for (int i = 0; i < (int)nr_supphyps; ++i) {
                        if (F->second.first[nr_supphyps - 1 - i] == 0)  // does not define F
                            continue;
                        // beta_F=i;
                        Gens = Gens & SuppHypInd[i];
                    }

                    dynamic_bitset MM_mother = F->second.second;

                    // now we produce the intersections with facets
                    dynamic_bitset Intersect(nr_gens);

                    int start;
                    if (CCC)
                        start = 0;
                    else {
                        start = F->second.first.find_first();
                        start = nr_supphyps - start;
                    }

                    for (size_t i = start; i < nr_supphyps; ++i) {
                        if (F->first[i] == 1) {  // contains *F
                            continue;
                        }
#pragma omp atomic
                        total_inter++;
                        if (MM_mother[i] == 0) {  // using restriction criteria of the paper
#pragma omp atomic
                            avoided_inter++;
                            continue;
                        }
                        Intersect = Gens & SuppHypInd[i];
                        if (inhomogeneous && Intersect.is_subset_of(ExtrRecCone))
                            continue;

                        Faces.splice(Faces.end(), FreeFaces, FreeFaces.begin());
                        Faces.back().first = Intersect;
                        Faces.back().second.max_cutting_out = i;
                        Faces.back().second.max_subset = true;
                        // Faces.back().second.HypsContaining.reset();
                        // Faces.push_back(make_pair(Intersect,fr));
                    }

                    Faces.sort(face_compare);
                    for (auto Fac = Faces.begin(); Fac != Faces.end(); ++Fac) {
                        if (Fac != Faces.begin()) {
                            auto Gac = Fac;
                            --Gac;
                            if (Fac->first == Gac->first) {
                                Fac->second.max_subset = false;
                                Gac->second.max_subset = false;
                            }
                        }
                    }

                    for (auto Fac = Faces.end(); Fac != Faces.begin();) {  // first we check for inclusion

                        --Fac;

                        if (!Fac->second.max_subset)
                            continue;

                        auto Gac = Fac;
                        Gac++;
                        for (; Gac != Faces.end(); Gac++) {
                            if (!Gac->second.max_subset)
                                continue;
                            if (Fac->first.is_subset_of(Gac->first)) {
                                Fac->second.max_subset = false;
                                break;
                            }
                        }
                    }

                    dynamic_bitset MM_F(nr_supphyps);

                    for (auto Fac = Faces.end(); Fac != Faces.begin();) {
                        --Fac;

                        if (!Fac->second.max_subset)
                            continue;

#pragma omp atomic
                        total_max_subset++;

                        INTERRUPT_COMPUTATION_BY_EXCEPTION

                        dynamic_bitset Containing = F->first;
                        Containing[Fac->second.max_cutting_out] = 1;

                        bool simple = false;
                        if (F_simple && use_simple_vert) {
                            if ((Fac->first & SimpleVert).any()) {
                                simple = true;
                            }
                        }

                        if (!simple) {
                            bool extra_hyp = false;
                            for (size_t j = 0; j < nr_supphyps; ++j) {  // beta_F
                                if (Containing[j] == 0 && Fac->first.is_subset_of(SuppHypInd[j])) {
                                    Containing[j] = 1;
                                    extra_hyp = true;
                                }
                            }
                            simple = F_simple && !extra_hyp;
                        }

                        int codim_of_face = 0;  // to make gcc happy
                        if (simple)
                            codim_of_face = codimension_so_far;
                        else {
                            dynamic_bitset Containing(nr_supphyps);
                            for (size_t j = 0; j < nr_supphyps; ++j) {  // beta_F
                                if (Containing[j] == 0 && Fac->first.is_subset_of(SuppHypInd[j])) {
                                    Containing[j] = 1;
                                }
                            }
                            vector<bool> selection = bitset_to_bool(Containing);
                            if (change_integer_type) {
                                try {
                                    codim_of_face = EmbeddedSuppHyps_MI.submatrix(selection).rank();
                                } catch (const ArithmeticException& e) {
                                    change_integer_type = false;
                                }
                            }
                            if (!change_integer_type)
                                codim_of_face = EmbeddedSuppHyps.submatrix(selection).rank();

                            if (codim_of_face > codimension_so_far) {
                                Fac->second.max_subset = false;
                                continue;
                            }
                        }

                        MM_F[Fac->second.max_cutting_out] = 1;
                        Fac->second.simple = simple;
                        Fac->second.HypsContaining = Containing;
                    }

                    for (auto Fac = Faces.end(); Fac != Faces.begin();) {  // why backwards??

                        --Fac;

                        if (!Fac->second.max_subset)
                            continue;

                        bool simple = Fac->second.simple;

                        beta_F[nr_supphyps - 1 - Fac->second.max_cutting_out] =
                            1;  // we must go to revlex, beta_F reconstituted below

#pragma omp critical(INSERT_NEW)
                        {
                            total_new++;

                            if (simple) {
                                NewFaces[Fac->second.HypsContaining] = make_pair(beta_F, MM_F);
                                total_simple++;
                            }
                            else {
                                auto G = NewFaces.find(Fac->second.HypsContaining);
                                if (G == NewFaces.end()) {
                                    NewFaces[Fac->second.HypsContaining] = make_pair(beta_F, MM_F);
                                }
                                else {
                                    if (G->second.first < beta_F) {  // because of revlex < instead of >
                                        G->second.first = beta_F;
                                        G->second.second = MM_F;
                                    }
                                }
                            }
                        }  // critical

                        beta_F[nr_supphyps - 1 - Fac->second.max_cutting_out] = 0;
                    }
                } catch (const std::exception&) {
                    tmp_exception = std::current_exception();
                    skip_remaining = true;
#pragma omp flush(skip_remaining)
                }

                FreeFaces.splice(FreeFaces.end(), Faces);
            }  // omp for
        }      // parallel
        if (!(tmp_exception == 0))
            std::rethrow_exception(tmp_exception);

        if (ToCompute.test(ConeProperty::FaceLattice))
            for (auto H = WorkFaces.begin(); H != WorkFaces.end(); ++H)
                FaceLattice[H->first] = codimension_so_far - 1;
        WorkFaces.clear();
        if (NewFaces.empty())
            break;
        swap(WorkFaces, NewFaces);
    }

    if (inhomogeneous && nr_vert != 1) {  // we want the empty face in the face lattice
                                          // (never the case in homogeneous computations)
        dynamic_bitset NoGens(nr_gens);
        size_t codim_max_subspace = EmbeddedSuppHyps.rank();
        FaceLattice[AllFacets] = codim_max_subspace;
        if (!(bound_codim && (int)codim_max_subspace > face_codim_bound))
            prel_f_vector[codim_max_subspace]++;
    }

    size_t total_nr_faces = 0;
    for (int i = prel_f_vector.size() - 1; i >= 0; --i) {
        if (prel_f_vector[i] != 0) {
            f_vector.push_back(prel_f_vector[i]);
            total_nr_faces += prel_f_vector[i];
        }
    }

    // cout << " Total " << FaceLattice.size() << endl;

    if (verbose) {
        verboseOutput() << endl << "Total number of faces computed " << total_nr_faces << endl;
        verboseOutput() << "f-vector " << f_vector;
    }

    if (ToCompute.test(ConeProperty::FaceLattice))
        setComputed(ConeProperty::FaceLattice);
    setComputed(ConeProperty::FVector);

    /*
    if(verbose){
        verboseOutput() << "done" << endl;

    cout << "total " << total_inter << " avoided " << avoided_inter << " computed " << total_inter-avoided_inter <<  endl;

    cout << "faces sent to NewFaces " << total_new << " cosimplicial " << total_simple << " degenerate " << total_nr_faces -
    total_simple << endl;

    cout << "total max subset " << total_max_subset <<endl;;

    if(total_nr_faces - total_simple!=0)
        cout << "average number of computations degenerate " <<  (float) (total_new+1 - total_simple) /(float) (total_nr_faces -
    total_simple) << endl; else cout << "all faces cosimpliocial" << endl;
    }
    */
}

//---------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_combinatorial_automorphisms(const ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::CombinatorialAutomorphisms) || isComputed(ConeProperty::CombinatorialAutomorphisms))
        return;

    if (verbose)
        verboseOutput() << "Computing combinatorial automorphism group" << endl;

    compute(ConeProperty::SupportHyperplanes);

    Matrix<Integer> SpecialLinFoprms(0, dim);

    if (inhomogeneous) {
        SpecialLinFoprms.append(Dehomogenization);
    }

    /* set<AutomParam::Goals> AutomToCompute;
    AutomToCompute.insert(AutomParam::OrbitsPrimal);
    AutomToCompute.insert(AutomParam::OrbitsDual);*/

    Automs = AutomorphismGroup<Integer>(ExtremeRays, SupportHyperplanes, SpecialLinFoprms);
    
    if(ExtremeRays.nr_of_rows()==0){
        setComputed(ConeProperty::CombinatorialAutomorphisms);
        return;
    }
    
    if(ExtremeRays.nr_of_rows()==0){
        setComputed(ConeProperty::CombinatorialAutomorphisms);
        return;
    }
    
    Automs.compute(AutomParam::combinatorial);

    if (verbose)
        verboseOutput() << Automs.getQualitiesString() << "automorphism group of order " << Automs.getOrder() << "  done" << endl;

    vector<key_t> ExtRaysKey, VerticesKey;

    if (inhomogeneous) {
        Automs.ExtRaysPerms = extract_permutations(Automs.GenPerms, Automs.GensRef, ExtremeRaysRecCone, true, ExtRaysKey);
        Automs.VerticesPerms = extract_permutations(Automs.GenPerms, Automs.GensRef, VerticesOfPolyhedron, true, VerticesKey);
    }
    else {
        Automs.ExtRaysPerms = Automs.GenPerms;
    }

    Automs.SuppHypsPerms = Automs.LinFormPerms;

    sort_individual_vectors(Automs.GenOrbits);
    if (inhomogeneous) {
        Automs.VerticesOrbits = extract_subsets(Automs.GenOrbits, Automs.GensRef.nr_of_rows(), VerticesKey);
        sort_individual_vectors(Automs.VerticesOrbits);

        Automs.ExtRaysOrbits = extract_subsets(Automs.GenOrbits, Automs.GensRef.nr_of_rows(), ExtRaysKey);
        sort_individual_vectors(Automs.ExtRaysOrbits);
    }
    else {
        Automs.ExtRaysOrbits = Automs.GenOrbits;
    }

    sort_individual_vectors(Automs.LinFormOrbits);
    Automs.SuppHypsOrbits = Automs.LinFormOrbits;

    setComputed(ConeProperty::CombinatorialAutomorphisms);
}

template <typename Integer>
void Cone<Integer>::compute_euclidean_automorphisms(const ConeProperties& ToCompute) {
    if (!ToCompute.test(ConeProperty::EuclideanAutomorphisms) || isComputed(ConeProperty::EuclideanAutomorphisms))
        return;

    compute(ConeProperty::SupportHyperplanes);

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
    
    if(ExtremeRays.nr_of_rows()==0){
        setComputed(ConeProperty::EuclideanAutomorphisms);
        return;
    }
    
    Automs.compute(AutomParam::euclidean);

    if (verbose)
        verboseOutput() << Automs.getQualitiesString() << "automorphism group of order " << Automs.getOrder() << "  done" << endl;

    vector<key_t> VerticesKey;

    if (inhomogeneous) {
        Automs.VerticesPerms = extract_permutations(Automs.GenPerms, Automs.GensRef, VerticesOfPolyhedron, true, VerticesKey);
    }
    else {
        Automs.ExtRaysPerms = Automs.GenPerms;
    }

    Automs.SuppHypsPerms = Automs.LinFormPerms;

    sort_individual_vectors(Automs.GenOrbits);
    if (inhomogeneous) {
        Automs.VerticesOrbits = extract_subsets(Automs.GenOrbits, Automs.GensRef.nr_of_rows(), VerticesKey);
        sort_individual_vectors(Automs.VerticesOrbits);
    }
    else {
        Automs.ExtRaysOrbits = Automs.GenOrbits;
    }

    sort_individual_vectors(Automs.LinFormOrbits);
    Automs.SuppHypsOrbits = Automs.LinFormOrbits;

    setComputed(ConeProperty::EuclideanAutomorphisms);
}

//----------------------------------------------------------------------------

template <typename Integer>
void Cone<Integer>::compute_refined_triangulation(ConeProperties& ToCompute){
    
    
    if (change_integer_type) {
        try {
#ifdef NMZ_EXTENDED_TESTS
            if(!using_GMP<Integer>() && !using_renf<Integer>() && test_arith_overflow_descent)
                throw ArithmeticException(0);    
#endif
            compute_unimodular_triangulation<MachineInteger>(ToCompute); // only one can be actiated
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
    if (! change_integer_type) {
        compute_unimodular_triangulation<Integer>(ToCompute); // only one can be actiated
        compute_lattice_point_triangulation<Integer>(ToCompute);
        compute_all_generators_triangulation<Integer>(ToCompute);
    }    
}
    
template <typename Integer>
template <typename IntegerColl>
void Cone<Integer>::prepare_collection(ConeCollection<IntegerColl>& Coll){
    
    check_gens_vs_reference();
    compute(ConeProperty::Triangulation);
    
    BasisChangePointed.convert_to_sublattice(Coll.Generators,Generators);
    vector<pair<vector<key_t>, IntegerColl> > CollTriangulation;
    for(auto& T: Triangulation){
        IntegerColl CollMult = convertTo<IntegerColl>(T.second);
        CollTriangulation.push_back(make_pair(T.first, CollMult));        
    }
    Coll.verbose = verbose;
    Coll.initialize_minicones(CollTriangulation);
}

template <typename Integer>
template <typename IntegerColl>
void Cone<Integer>::extract_data(ConeCollection<IntegerColl>& Coll){
    
    BasisChangePointed.convert_from_sublattice(Generators, Coll.Generators);
    ReferenceGenerators = Generators;
    Triangulation.clear();
    Coll.flatten();
    for(auto& T: Coll.getKeysAndMult()){
        
        INTERRUPT_COMPUTATION_BY_EXCEPTION
        
        Integer CollMult = convertTo<Integer>(T.second);
        Triangulation.push_back(make_pair(T.first, CollMult));        
    }
#ifdef NMZ_EXTENDED_TESTS
    if(isComputed(ConeProperty::Volume)  && !using_renf<Integer>()){
        mpq_class test_vol;
        vector<Integer> TestGrad;
        if(inhomogeneous)
            TestGrad = Dehomogenization;
        else
            TestGrad = Grading;
        for(auto& T: Triangulation){
            Integer grad_prod = 1;
            for(auto& k: T.first)
                grad_prod *= v_scalar_product(Generators[k], TestGrad);
            mpz_class gp_mpz = convertTo<mpz_class>(grad_prod);
            mpz_class vol_mpz = convertTo<mpz_class>(T.second);
            mpq_class quot = vol_mpz;
            quot /= gp_mpz;
            test_vol+=quot;
        }
        assert(test_vol == getVolume());
    }
#endif
}

template <typename Integer>
void Cone<Integer>::extract_data(ConeCollection<Integer>& Coll){
 
    if(BasisChangePointed.IsIdentity())
        swap(Generators,Coll.Generators);
    else
        Generators = BasisChangePointed.from_sublattice(Coll.Generators);
    ReferenceGenerators = Generators;
    Triangulation.clear();
    Coll.flatten();
    Triangulation.clear();
    swap(Triangulation, Coll.KeysAndMult);
#ifdef NMZ_EXTENDED_TESTS
    if(isComputed(ConeProperty::Volume) && !using_renf<Integer>()){
        vector<Integer> TestGrad;
        if(inhomogeneous)
            TestGrad = Dehomogenization;
        else
            TestGrad = Grading;
        mpq_class test_vol;
        for(auto& T: Triangulation){
            Integer grad_prod = 1;
            for(auto& k: T.first)
                grad_prod *= v_scalar_product(Generators[k], TestGrad);
            mpz_class gp_mpz = convertTo<mpz_class>(grad_prod);
            mpz_class vol_mpz = convertTo<mpz_class>(T.second);
            mpq_class quot = vol_mpz;
            quot /= gp_mpz;
            test_vol+=quot;
        }
        assert(test_vol == getVolume());
    }
#endif
}

template <typename Integer>
template <typename IntegerColl>
void Cone<Integer>::compute_unimodular_triangulation(ConeProperties& ToCompute){
    
    if(!ToCompute.test(ConeProperty::UnimodularTriangulation) || isComputed(ConeProperty::UnimodularTriangulation))
        return;
    
    if(verbose)
        verboseOutput() << "Computing unimimodular triangulation" << endl; 

    ConeCollection<IntegerColl> UMT;
    prepare_collection<IntegerColl>(UMT);
    if(isComputed(ConeProperty::HilbertBasis)){
        Matrix<IntegerColl> HBPointed;        
        BasisChangePointed.convert_to_sublattice(HBPointed,HilbertBasis);
        UMT.add_extra_generators(HBPointed);
    }
    
    UMT.make_unimodular();
    extract_data<IntegerColl>(UMT);
    setComputed(ConeProperty::UnimodularTriangulation);
    setComputed(ConeProperty::Triangulation );
}

#ifdef ENFNORMALIZ
template<>
template <typename IntegerColl>
void Cone<renf_elem_class>::compute_unimodular_triangulation(ConeProperties& ToCompute){
    
    if(!ToCompute.test(ConeProperty::UnimodularTriangulation) || isComputed(ConeProperty::UnimodularTriangulation))
        return;

    assert(false);
}
#endif

template <typename Integer>
template <typename IntegerColl>
void Cone<Integer>::compute_lattice_point_triangulation(ConeProperties& ToCompute){
    
    if(!ToCompute.test(ConeProperty::LatticePointTriangulation) || isComputed(ConeProperty::LatticePointTriangulation))
        return;
    
    if(verbose)
        verboseOutput() << "Computing lattice points triangulation" << endl;
       
    ConeCollection<IntegerColl> LPT;
    prepare_collection<IntegerColl>(LPT);
    Matrix<IntegerColl> LPPointed;
    if(inhomogeneous){
        assert(isComputed(ConeProperty::ModuleGenerators));         
        BasisChangePointed.convert_to_sublattice(LPPointed,ModuleGenerators);
    }
    else{
        assert(isComputed(ConeProperty::Deg1Elements)); 
        BasisChangePointed.convert_to_sublattice(LPPointed,Deg1Elements);
    }
    LPT.add_extra_generators(LPPointed);
    extract_data<IntegerColl>(LPT);
    setComputed(ConeProperty::LatticePointTriangulation);
    setComputed(ConeProperty::Triangulation);    
}

template <typename Integer>
template <typename IntegerColl>
void Cone<Integer>::compute_all_generators_triangulation(ConeProperties& ToCompute){
    
    if(!ToCompute.test(ConeProperty::AllGeneratorsTriangulation) || isComputed(ConeProperty::AllGeneratorsTriangulation))
        return;
    
    if(verbose)
        verboseOutput() << "Computing all generators triangulation" << endl;
       
    ConeCollection<IntegerColl> OMT;
    prepare_collection<IntegerColl>(OMT);
    Matrix<IntegerColl> OMPointed;
    BasisChangePointed.convert_to_sublattice(OMPointed,OriginalMonoidGenerators);
    OMT.insert_all_gens();
    extract_data<IntegerColl>(OMT);
    setComputed(ConeProperty::AllGeneratorsTriangulation);
    setComputed(ConeProperty::Triangulation );    
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
        case ConeProperty::Generators:
            return this->getGeneratorsMatrix();
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
            throw FatalException("Intehger property without output");;
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
            throw FatalException("Rational property without output");;
    }
}

#ifdef ENFNORMALIZ
template <typename Integer>
renf_elem_class Cone<Integer>::getFieldElemConeProperty(ConeProperty::Enum property) {
    if (output_type(property) != OutputType::FieldElem) {
        throw FatalException("property has no field element output");
    }
    switch (property) {
        case ConeProperty::RenfVolume:
            return this->getRenfVolume();
        default:
            throw FatalException("Field element property without output");;
    }
}
#endif

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
            throw FatalException("Float property without output");;
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
    Out.set_renf(Renf);
#endif

    Out.write_files();
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
void run_additional_tests_libnormaliz(){
    
    vector<nmz_float> ext_1 ={ 1.0, 1.5};
    vector<nmz_float> ext_2 ={ 3.1e1, 1.7e1};
    vector<vector<nmz_float> > test_ext = {ext_1, ext_2};
    Matrix<nmz_float> test_input(test_ext);
    
    Cone<mpz_class> C(Type::polytope, test_input);
    // C.getHilbertBasisMatrix().pretty_print(cout);
    
    vector<mpz_class> new_grading = {1,2,3};    
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
    
    vector<vector<nmz_float> > eq = {{-1,1,-1}};    
    
    C.modifyCone(Type::equations,eq);
    
    C = Cone<mpz_class>(Type::vertices, test_input);
    
    C.getVerticesFloat();  
    C.getNrVerticesFloat(); 
    C.getVerticesOfPolyhedron(); 
    C.getModuleGenerators();
    
    vector<vector<mpz_class> > trivial = {{-1,1},{1,1}};
    vector<vector<mpz_class> > excl = {{-1,1}};
    C = Cone<mpz_class>(Type::cone, trivial, Type::excluded_faces,excl);
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
