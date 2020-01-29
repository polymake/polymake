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

//---------------------------------------------------------------------------
#include <set>

#include "libnormaliz/matrix.h"
#include "libnormaliz/nmz_nauty.h"
#include "libnormaliz/cone.h"
#include "libnormaliz/full_cone.h"
#include "libnormaliz/map_operations.h"

namespace libnormaliz {

using namespace std;

template <typename Integer>
AutomParam::Method AutomorphismGroup<Integer>::getMethod() const {
    return method;
}

template <typename Integer>
bool AutomorphismGroup<Integer>::Is_Computed(AutomParam::Goals goal) const {
    return contains(is_Computed, goal);
}

template <typename Integer>
set<AutomParam::Quality> AutomorphismGroup<Integer>::getQualities() const {
    return Qualities;
}

template <typename Integer>
const Matrix<Integer>& AutomorphismGroup<Integer>::getGens() const {
    return GensRef;
}

template <typename Integer>
const Matrix<Integer>& AutomorphismGroup<Integer>::getLinForms() const {
    return LinFormsRef;
}

template <typename Integer>
const Matrix<Integer>& AutomorphismGroup<Integer>::getSpecialLinForms() const {
    return SpecialLinFormsRef;
}

template <typename Integer>
const vector<vector<key_t> >& AutomorphismGroup<Integer>::getExtremeRaysPerms() const {
    return ExtRaysPerms;
}

template <typename Integer>
const vector<vector<key_t> >& AutomorphismGroup<Integer>::getVerticesPerms() const {
    return VerticesPerms;
}

template <typename Integer>
mpz_class AutomorphismGroup<Integer>::getOrder() const {
    return order;
}

template <typename Integer>
const vector<vector<key_t> >& AutomorphismGroup<Integer>::getSupportHyperplanesPerms() const {
    return SuppHypsPerms;
}

template <typename Integer>
const vector<vector<key_t> >& AutomorphismGroup<Integer>::getExtremeRaysOrbits() const {
    return ExtRaysOrbits;
}

template <typename Integer>
const vector<vector<key_t> >& AutomorphismGroup<Integer>::getVerticesOrbits() const {
    return VerticesOrbits;
}

template <typename Integer>
const vector<vector<key_t> >& AutomorphismGroup<Integer>::getSupportHyperplanesOrbits() const {
    return SuppHypsOrbits;
}

template <typename Integer>
const vector<Matrix<Integer> >& AutomorphismGroup<Integer>::getLinMaps() const {
    return LinMaps;
}

template <typename Integer>
const vector<key_t>& AutomorphismGroup<Integer>::getCanLabellingGens() const {
    return CanLabellingGens;
}

template <typename Integer>
void AutomorphismGroup<Integer>::reset() {
}

template <typename Integer>
AutomorphismGroup<Integer>::AutomorphismGroup() {
    reset();
}

/*
template<typename Integer>
AutomorphismGroup<Integer>::AutomorphismGroup(const Matrix<Integer>& ExtRays, const Matrix<Integer>& SpecialGens,
        const Matrix<Integer>& SupHyps,  const Matrix<Integer>& SpecialLinearForms){

    method=AutomParam::E;

    Gens=ExtRays; // reference for orbits
    LinForms=SuppHyps;  // ditto

    Matrix<Integer> LinFormsComp=GivenLinearForms;
    size_t nr_special_linforms=SpecialLinForms.nr_of_rows();
    LinFormsComp.append(SpecialLinearForms);

    Matrix<Integer> GensComp=GivenGens;
    size_t nr_special_gens=SpecialGens.nr_of_rows();
    GensComp.append(SpecialGens);

}*/

template <typename Integer>
bool AutomorphismGroup<Integer>::make_linear_maps_primal(const Matrix<Integer>& GivenGens,
                                                         const vector<vector<key_t> >& ComputedGenPerms) {
    LinMaps.clear();
    vector<key_t> PreKey = GivenGens.max_rank_submatrix_lex();
    vector<key_t> ImKey(PreKey.size());
    for (const auto& ComputedGenPerm : ComputedGenPerms) {
        for (size_t j = 0; j < ImKey.size(); ++j)
            ImKey[j] = ComputedGenPerm[PreKey[j]];
        Matrix<Integer> Pre = GivenGens.submatrix(PreKey);
        Matrix<Integer> Im = GivenGens.submatrix(ImKey);
        Integer denom, g;
        Matrix<Integer> Map = Pre.solve(Im, denom);
        g = Map.matrix_gcd();
        if (g % denom != 0)
            return false;
        Map.scalar_division(denom);
        if (Map.vol() != 1)
            return false;
        LinMaps.push_back(Map.transpose());
        // Map.pretty_print(cout);
        // cout << "--------------------------------------" << endl;
    }
    return true;
}


#ifdef ENFNORMALIZ
template <>
bool AutomorphismGroup<renf_elem_class>::make_linear_maps_primal(const Matrix<renf_elem_class>& GivenGens,
                                                                 const vector<vector<key_t> >& ComputedGenPerms) {
    LinMaps.clear();
    vector<key_t> PreKey = GivenGens.max_rank_submatrix_lex();
    vector<key_t> ImKey(PreKey.size());
    for (const auto& ComputedGenPerm : ComputedGenPerms) {
        for (size_t j = 0; j < ImKey.size(); ++j)
            ImKey[j] = ComputedGenPerm[PreKey[j]];
        Matrix<renf_elem_class> Pre = GivenGens.submatrix(PreKey);
        Matrix<renf_elem_class> Im = GivenGens.submatrix(ImKey);
        renf_elem_class denom;
        Matrix<renf_elem_class> Map = Pre.solve(Im, denom);
        /*renf_elem_class g=Map.matrix_gcd();
        if(g%denom !=0)
            return false;*/
        Map.scalar_division(denom);
        /*if(Map.vol()!=1)
            return false;*/
        LinMaps.push_back(Map.transpose());
        // Map.pretty_print(cout);
        // cout << "--------------------------------------" << endl;
    }
    return true;
}
#endif

string quality_to_string(AutomParam::Quality quality) {
    if (quality == AutomParam::combinatorial)
        return "combinatorial";
    if (quality == AutomParam::rational)
        return "Rational";
    if (quality == AutomParam::integral)
        return "Integral";
    if (quality == AutomParam::euclidean)
        return "Euclidean";
    if (quality == AutomParam::ambient)
        return "Ambient";
    if (quality == AutomParam::algebraic)
        return "Algebraic";
    if (quality == AutomParam::graded)
        return "Graded";
    assert(false);
}

template <typename Integer>
string AutomorphismGroup<Integer>::getQualitiesString() const {
    string result;
    for (const auto& Q : Qualities)
        result += quality_to_string(Q) + " ";
    return result;
}

template <typename Integer>
AutomorphismGroup<Integer>::AutomorphismGroup(const Matrix<Integer>& ExtRays,
                                              const Matrix<Integer>& SpecialGens,
                                              const Matrix<Integer>& SuppHyps,
                                              const Matrix<Integer>& SpecialLinForms) {
    set_basic_gens_and_lin_forms(ExtRays, SpecialGens, SuppHyps, SpecialLinForms);
}

template <typename Integer>
AutomorphismGroup<Integer>::AutomorphismGroup(const Matrix<Integer>& ExtRays,
                                              const Matrix<Integer>& SuppHyps,
                                              const Matrix<Integer>& SpecialLinForms) {
    size_t dim = ExtRays.nr_of_columns();
    Matrix<Integer> SpecialGens(0, dim);
    set_basic_gens_and_lin_forms(ExtRays, SpecialGens, SuppHyps, SpecialLinForms);
}

template <typename Integer>
void AutomorphismGroup<Integer>::set_basic_gens_and_lin_forms(const Matrix<Integer>& ExtRays,
                                                              const Matrix<Integer>& SpecialGens,
                                                              const Matrix<Integer>& SuppHyps,
                                                              const Matrix<Integer>& SpecialLinForms) {
    GensRef = ExtRays;  // reference data
    LinFormsRef = SuppHyps;
    SpecialLinFormsRef = SpecialLinForms;
    SpecialGensRef = SpecialGens;

    nr_special_linforms = SpecialLinForms.nr_of_rows();
    nr_special_gens = SpecialGens.nr_of_rows();

    GensComp = GensRef;
    GensComp.append(SpecialGensRef);
    LinFormsComp = LinFormsRef;
    LinFormsComp.append(SpecialLinFormsRef);

    addedComputationGens = false;
    addedComputationLinForms = false;
}

template <typename Integer>
void AutomorphismGroup<Integer>::addComputationGens(const Matrix<Integer>& GivenGens) {
    if (GivenGens.nr_of_rows() == 0)
        return;

    GensComp = GivenGens;
    GensComp.append(SpecialGensRef);
    addedComputationGens = true;
}

template <typename Integer>
void AutomorphismGroup<Integer>::addComputationLinForms(const Matrix<Integer>& GivenLinearForms) {
    if (GivenLinearForms.nr_of_rows() == 0)
        return;

    LinFormsComp = GivenLinearForms;
    LinFormsComp.append(SpecialLinFormsRef);
    addedComputationLinForms = true;
}

template <typename Integer>
void AutomorphismGroup<Integer>::dualize() {
    swap(GensRef, LinFormsRef);
    swap(SpecialGensRef, SpecialLinFormsRef);
    swap(GensComp, LinFormsComp);
    swap(addedComputationGens, addedComputationLinForms);
}

template <typename Integer>
void AutomorphismGroup<Integer>::swap_data_from_dual(AutomorphismGroup<Integer> Dual) {
    swap(GenPerms, Dual.LinFormPerms);
    swap(LinFormPerms, Dual.GenPerms);
    swap(GenOrbits, Dual.LinFormOrbits);
    swap(LinFormOrbits, Dual.GenOrbits);

    for (size_t i = 0; i < Dual.LinMaps.size(); ++i) {
        Integer dummy;
        LinMaps.push_back(Dual.LinMaps[i].invert(dummy).transpose());
    }

    order = Dual.order;
    Qualities = Dual.Qualities;
}

template <typename Integer>
void AutomorphismGroup<Integer>::swap_data_from(AutomorphismGroup<Integer> Help) {
    swap(GenPerms, Help.GenPerms);
    swap(LinFormPerms, Help.LinFormPerms);
    swap(GenOrbits, Help.GenOrbits);
    swap(LinFormOrbits, Help.LinFormOrbits);

    for (size_t i = 0; i < Help.LinMaps.size(); ++i) {
        LinMaps.push_back(Help.LinMaps[i]);
    }

    order = Help.order;
    Qualities = Help.Qualities;
}

template <typename Integer>
bool AutomorphismGroup<Integer>::compute_polytopal(const AutomParam::Quality& desired_quality) {
    assert(SpecialLinFormsRef.nr_of_rows() > 0);

    // we "polytopalize" the generators:
    // division by grading/dehomogenization for renf_elem_class
    // scaling to lcm(degrees) else

    vector<Integer> Grad = SpecialLinFormsRef[0];
    Matrix<Integer> NormedGens = GensRef;
    if (using_renf<Integer>()) {
        bool is_polytope = NormedGens.standardize_rows(Grad);
        if (!is_polytope)
            throw NotComputableException("For automorphisms of algebraic polyhedra input must define a polytope");
    }
    else {
        mpz_class LCM_mpz = 1;  // to be on the safe side with this potentially very large number
        for (size_t i = 0; i < NormedGens.nr_of_rows(); ++i) {
            Integer val = v_scalar_product(Grad, NormedGens[i]);
            mpz_class val_mpz = convertTo<mpz_class>(val);
            if (val == 0)
                throw NotComputableException("Euclidean or rational automorphisms only computable for polytopes");
            LCM_mpz = libnormaliz::lcm(LCM_mpz, val_mpz);
        }
        Integer LCM = convertTo<Integer>(LCM_mpz);
        if (LCM != 1) {
            for (size_t i = 0; i < NormedGens.nr_of_rows(); ++i) {
                Integer val = v_scalar_product(Grad, NormedGens[i]);
                Integer quot = LCM / val;
                v_scalar_multiplication(NormedGens[i], quot);
            }
        }
    }

    if (GensRef.nr_of_rows() <= LinFormsRef.nr_of_rows() || LinFormsRef.nr_of_rows() == 0 ||
        desired_quality == AutomParam::euclidean) {
        AutomorphismGroup<Integer> Help(NormedGens, LinFormsRef, SpecialLinFormsRef);
        bool success = Help.compute_inner(desired_quality);
        swap_data_from(Help);
        return success;
    }

    // we make the dual polytope by taking the standard fixed point
    // as the grading on the dual space.
    // in the next round we take the exit above.

    vector<Integer> FixedPoint(Grad.size());
    for (size_t i = 0; i < NormedGens.nr_of_rows(); ++i) {
        FixedPoint = v_add(FixedPoint, NormedGens[i]);
    }
    if (using_renf<Integer>())
        v_standardize(FixedPoint);
    else
        v_make_prime(FixedPoint);

    AutomorphismGroup<Integer> DualPolytope(LinFormsRef, NormedGens, FixedPoint);
    bool success = DualPolytope.compute(desired_quality);
    swap_data_from_dual(DualPolytope);
    return success;
}

template <typename Integer>
bool AutomorphismGroup<Integer>::compute_integral() {
    bool success = false;
    bool gens_tried = false;

    if (addedComputationGens || GensComp.nr_of_rows() <= LinFormsComp.nr_of_rows() || LinFormsRef.nr_of_rows() == 0) {
        success = compute_inner(AutomParam::integral);
        gens_tried = true;
    }

    if (success)
        return true;

    AutomorphismGroup<Integer> Dual(*this);
    Dual.dualize();

    success = Dual.compute_inner(AutomParam::integral);

    if (success) {
        swap_data_from_dual(Dual);
        return true;
    }

    if (!gens_tried)
        success = compute_inner(AutomParam::integral);

    if (success)
        return true;

    success = compute_inner(AutomParam::integral, true);  // true = Gens x LinForms

    return success;
}

template <typename Integer>
bool AutomorphismGroup<Integer>::compute(const AutomParam::Quality& desired_quality, bool force_gens_x_linforms) {
    if (desired_quality == AutomParam::integral)
        return compute_integral();

    if (desired_quality == AutomParam::rational || desired_quality == AutomParam::algebraic ||
        desired_quality == AutomParam::euclidean)
        return compute_polytopal(desired_quality);

    return compute_inner(desired_quality, force_gens_x_linforms);
}

template <typename Integer>
bool AutomorphismGroup<Integer>::compute_inner(const AutomParam::Quality& desired_quality, bool force_gens_x_linforms) {
    bool FromGensOnly = true;
    if (desired_quality == AutomParam::combinatorial || desired_quality == AutomParam::ambient || force_gens_x_linforms)
        FromGensOnly = false;

    assert(desired_quality == AutomParam::integral || !addedComputationGens);

    if (!FromGensOnly) {
        if (!addedComputationGens) {
            if (!addedComputationLinForms) {
                method = AutomParam::E;
            }
            else {
                method = AutomParam::EA;
            }
        }
        else {
            method = AutomParam::G;
        }
    }  // !FromGensOnly
    else {
        if (!addedComputationGens) {
            method = AutomParam::EE;
        }
        else {
            method = AutomParam::GG;
        }
    }

    nauty_result result;

#ifdef NMZ_NAUTY
    if (FromGensOnly) {
        result = compute_automs_by_nauty_FromGensOnly(GensComp, nr_special_gens, SpecialLinFormsRef, desired_quality);
    }
    else {
        result = compute_automs_by_nauty_Gens_LF(GensComp, nr_special_gens, LinFormsComp, nr_special_linforms, desired_quality);
    }
#endif

    order = result.order;
    CanType = result.CanType;

    bool maps_lifted = false;
    if (desired_quality != AutomParam::combinatorial && desired_quality != AutomParam::euclidean) {
        maps_lifted = make_linear_maps_primal(GensComp, result.GenPerms);
    }

    // cout << "LLLL " << maps_lifted << endl;

    if (!maps_lifted && desired_quality == AutomParam::integral)
        return false;

    // cout << quality_to_string(desired_quality) << " " << maps_lifted << endl;

    if (maps_lifted) {
        if (desired_quality == AutomParam::ambient)
            Qualities.insert(AutomParam::ambient);
        if (desired_quality == AutomParam::integral)
            Qualities.insert(AutomParam::integral);
        if (desired_quality == AutomParam::rational)
            Qualities.insert(AutomParam::integral);
        if (desired_quality == AutomParam::algebraic)
            Qualities.insert(AutomParam::algebraic);
    }
    else {
        if (desired_quality == AutomParam::rational)
            Qualities.insert(AutomParam::rational);
        else
            Qualities.insert(desired_quality);
    }

    if (true) {  //(contains(ToCompute,AutomParam::OrbitsPrimal)){
        if (method == AutomParam::E || method == AutomParam::EA || method == AutomParam::EE) {
            GenPerms = result.GenPerms;
            GenOrbits = convert_to_orbits(result.GenOrbits);
        }
        else {
            gen_data_via_lin_maps();
        }
    }

    // cout << "EEE " << given_gens_are_extrays << endl;

    if (LinFormsRef.nr_of_rows() > 0) {
        if ((method == AutomParam::E || method == AutomParam::G) && !using_renf<Integer>()) {
            LinFormPerms = result.LinFormPerms;
            LinFormOrbits = convert_to_orbits(result.LinFormOrbits);
        }
        else {
            // linform_data_via_lin_maps();
            linform_data_via_incidence();
        }
    }

    /* CanLabellingGens.clear();
    if(!addedComputationGens){
        CanLabellingGens=result.CanLabellingGens;
    }*/

    return true;
}

template <typename Integer>
void AutomorphismGroup<Integer>::gen_data_via_lin_maps() {
    GenPerms.clear();
    map<vector<Integer>, key_t> S;
    for (key_t k = 0; k < GensRef.nr_of_rows(); ++k)
        S[GensRef[k]] = k;
    for (size_t i = 0; i < LinMaps.size(); ++i) {
        vector<key_t> Perm(GensRef.nr_of_rows());
        for (key_t j = 0; j < Perm.size(); ++j) {
            vector<Integer> Im = LinMaps[i].MxV(GensRef[j]);
            assert(S.find(Im) != S.end());  // for safety
            if (!using_renf<Integer>())
                v_make_prime(Im);
            Perm[j] = S[Im];
        }
        GenPerms.push_back(Perm);
    }
    GenOrbits = orbits(GenPerms, GensRef.nr_of_rows());
}

template <typename Integer>
void AutomorphismGroup<Integer>::linform_data_via_lin_maps() {
    bool only_rational = contains(Qualities, AutomParam::rational);
    LinFormPerms.clear();
    map<vector<Integer>, key_t> S;
    for (key_t k = 0; k < LinFormsRef.nr_of_rows(); ++k)
        S[LinFormsRef[k]] = k;
    for (size_t i = 0; i < LinMaps.size(); ++i) {
        vector<key_t> Perm(LinFormsRef.nr_of_rows());
        Integer dummy;
        Matrix<Integer> LM = LinMaps[i].invert(dummy).transpose();
        for (key_t j = 0; j < Perm.size(); ++j) {
            vector<Integer> Im = LM.MxV(LinFormsRef[j]);
            if (only_rational)
                v_make_prime(Im);
            assert(S.find(Im) != S.end());  // for safety
            Perm[j] = S[Im];
        }
        LinFormPerms.push_back(Perm);
    }
    LinFormOrbits = orbits(LinFormPerms, LinFormsRef.nr_of_rows());
}

template <typename Integer>
void AutomorphismGroup<Integer>::linform_data_via_incidence() {
    map<dynamic_bitset, int> IncidenceMap;

    for (size_t i = 0; i < LinFormsRef.nr_of_rows(); ++i) {
        dynamic_bitset indicator(GensRef.nr_of_rows());
        for (size_t j = 0; j < GensRef.nr_of_rows(); ++j) {
            if (v_scalar_product(LinFormsRef[i], GensRef[j]) == 0)
                indicator[j] = 1;
        }
        IncidenceMap[indicator] = i;
    }

    LinFormPerms.clear();
    LinFormPerms.resize(GenPerms.size());
    for (size_t i = 0; i < GenPerms.size(); ++i) {
        vector<key_t> linf_perm(LinFormsRef.nr_of_rows());
        for (const auto& L : IncidenceMap) {
            dynamic_bitset permuted_indicator(GensRef.nr_of_rows());
            for (size_t j = 0; j < GensRef.nr_of_rows(); ++j)
                permuted_indicator[GenPerms[i][j]] = L.first[j];
            linf_perm[L.second] = IncidenceMap[permuted_indicator];
        }
        LinFormPerms[i] = linf_perm;
    }

    LinFormOrbits = orbits(LinFormPerms, LinFormsRef.nr_of_rows());
}

template <typename Integer>
void AutomorphismGroup<Integer>::add_images_to_orbit(const vector<Integer>& v, set<vector<Integer> >& orbit) const {
    for (size_t i = 0; i < LinMaps.size(); ++i) {
        vector<Integer> w = LinMaps[i].MxV(v);
        auto f = orbit.find(w);
        if (f != orbit.end())
            continue;
        else {
            orbit.insert(w);
            add_images_to_orbit(w, orbit);
        }
    }
}

template <typename Integer>
list<vector<Integer> > AutomorphismGroup<Integer>::orbit_primal(const vector<Integer>& v) const {
    set<vector<Integer> > orbit;
    add_images_to_orbit(v, orbit);
    list<vector<Integer> > orbit_list;
    for (auto& c : orbit)
        orbit_list.push_back(c);
    return orbit_list;
}

/* MUCH TO DO
template<typename Integer>
IsoType<Integer>::IsoType(Full_Cone<Integer>& C, bool with_Hilbert_basis){

    dim=C.getDim();
    if(dim=0)
        return;

    if(with_Hilbert_basis){
        if(!C.isComputed(ConeProperty::HilbertBasis)){
            C.do_Hilbert_basis=true;
            C.compute();
        }
        HilbertBasis=Matrix<Integer>(C.Hilbert_Basis);
    }

    if(!C.isComputed(ConeProperty::ExtremeRays)){
        C.get_supphyps_from_copy(true);
        C.get_supphyps_from_copy(true,true);
    }

    ExtremeRays=C.Generators.submatrix(C.Extreme_Rays_ind);
    SupportHyperplanes=C.Support_Hyperplanes;

    if(C.isComputed(ConeProperty::Multiplicity))
        Multiplicity=C.multiplicity;

}*/

template <typename Integer>
IsoType<Integer>::IsoType() {  // constructs a dummy object
    rank = 0;
    nrExtremeRays = 1;  // impossible
}

template <typename Integer>
IsoType<Integer>::IsoType(const Full_Cone<Integer>& C, bool& success) {
    success = false;
    assert(C.isComputed(ConeProperty::Automorphisms));

    // we don't want the zero cone here. It should have been filtered out.
    assert(C.dim > 0);
    // We insist that cones arriving here are have their extreme rays as generators
    nrExtremeRays = C.getNrExtremeRays();
    assert(nrExtremeRays == C.nr_gen);

    if (C.isComputed(ConeProperty::Grading))
        Grading = C.Grading;
    if (C.inhomogeneous)
        Truncation = C.Truncation;

    if (C.Automs.getMethod() == AutomParam::G)  // not yet useful
        return;
    CanType = C.Automs.CanType;
    CanLabellingGens = C.Automs.getCanLabellingGens();
    rank = C.dim;
    nrSupportHyperplanes = C.nrSupport_Hyperplanes;
    if (C.isComputed(ConeProperty::Multiplicity))
        Multiplicity = C.multiplicity;

    if (C.isComputed(ConeProperty::HilbertBasis)) {
        HilbertBasis = Matrix<Integer>(0, rank);
        ExtremeRays = C.Generators;
        // we compute the coordinate transformation to the first max linearly indepndent
        // of extreme rays in canonical order
        CanBasisKey = ExtremeRays.max_rank_submatrix_lex(CanLabellingGens);
        CanTransform = ExtremeRays.submatrix(CanBasisKey).invert(CanDenom);

        // now we remove the extreme rays from the stored Hilbert CanBasisKey
        // since the isomorphic copy knows its own extreme rays
        if (C.Hilbert_Basis.size() > nrExtremeRays) {  // otherwise nothing to do
            set<vector<Integer> > ERSet;
            for (size_t i = 0; i < nrExtremeRays; ++i)
                ERSet.insert(ExtremeRays[i]);
            for (const auto& h : C.Hilbert_Basis) {
                if (ERSet.find(h) == ERSet.end())
                    HilbertBasis.append(h);
            }
        }
    }
    success = true;
}

template <typename Integer>
const Matrix<Integer>& IsoType<Integer>::getHilbertBasis() const {
    return HilbertBasis;
}

template <typename Integer>
const Matrix<Integer>& IsoType<Integer>::getCanTransform() const {
    return CanTransform;
}

template <typename Integer>
Integer IsoType<Integer>::getCanDenom() const {
    return CanDenom;
}

template <typename Integer>
bool IsoType<Integer>::isOfType(const Full_Cone<Integer>& C) const {
    if (C.dim != rank || C.nrSupport_Hyperplanes != nrSupportHyperplanes || nrExtremeRays != C.getNrExtremeRays())
        return false;
    if (!CanType.equal(C.Automs.CanType))
        return false;
    return true;
}

template <typename Integer>
mpq_class IsoType<Integer>::getMultiplicity() const {
    return Multiplicity;
}

template <typename Integer>
Isomorphism_Classes<Integer>::Isomorphism_Classes() {
    Classes.push_back(IsoType<Integer>());
}

template <typename Integer>
void Isomorphism_Classes<Integer>::add_type(Full_Cone<Integer>& C, bool& success) {
    Classes.push_back(IsoType<Integer>(C, success));
    if (!success)
        Classes.pop_back();
}

size_t NOT_FOUND = 0;
size_t FOUND = 0;

template <typename Integer>
const IsoType<Integer>& Isomorphism_Classes<Integer>::find_type(Full_Cone<Integer>& C, bool& found) const {
    assert(C.getNrExtremeRays() == C.nr_gen);
    found = false;
    if (C.Automs.method == AutomParam::G)  // cannot be used for automorphism class
        return *Classes.begin();
    auto it = Classes.begin();
    ++it;
    for (; it != Classes.end(); ++it) {
        if (it->isOfType(C)) {
            found = true;
            FOUND++;
            return *it;
        }
    }
    NOT_FOUND++;
    return *Classes.begin();
}

list<dynamic_bitset> partition(size_t n, const vector<vector<key_t> >& Orbits) {
    // produces a list of bitsets, namely the indicator vectors of the key vectors in Orbits

    list<dynamic_bitset> Part;
    for (const auto& Orbit : Orbits) {
        dynamic_bitset p(n);
        for (unsigned int j : Orbit)
            p.set(j, true);
        Part.push_back(p);
    }
    return Part;
}

vector<vector<key_t> > keys(const list<dynamic_bitset>& Partition) {
    // inverse operation of partition
    vector<vector<key_t> > Keys;
    auto p = Partition.begin();
    for (; p != Partition.end(); ++p) {
        vector<key_t> key;
        for (size_t j = 0; j < p->size(); ++j)
            if (p->test(j))
                key.push_back(j);
        Keys.push_back(key);
    }
    return Keys;
}

list<dynamic_bitset> join_partitions(const list<dynamic_bitset>& P1, const list<dynamic_bitset>& P2) {
    // computes the join of two partitions given as lusts of indicator vectors
    list<dynamic_bitset> J = P1;  // work copy pf P1
    auto p2 = P2.begin();
    for (; p2 != P2.end(); ++p2) {
        auto p1 = J.begin();
        for (; p1 != J.end(); ++p1) {  // search the first member of J that intersects p1
            if ((*p2).intersects(*p1))
                break;
        }
        if ((*p2).is_subset_of(*p1))  // is contained in that member, nothing to do
            continue;
        // now we join the members of J that intersect p2
        assert(p1 != J.end());  // to be on the safe side
        auto p3 = p1;
        p3++;
        while (p3 != J.end()) {
            if ((*p2).intersects(*p3)) {
                *p1 |= *p3;  // the union
                p3 = J.erase(p3);
            }
            else
                p3++;
        }
    }
    return J;
}

vector<vector<key_t> > orbits(const vector<vector<key_t> >& Perms, size_t N) {
    // Perms is a list of permutations of 0,...,N-1

    vector<vector<key_t> > Orbits;
    if (Perms.size() == 0) {  // each element is its own orbit
        Orbits.reserve(N);
        for (size_t i = 0; i < N; ++i)
            Orbits.push_back(vector<key_t>(1, i));
        return Orbits;
    }
    vector<bool> InOrbit(N, false);
    for (size_t i = 0; i < N; ++i) {
        if (InOrbit[i])
            continue;
        vector<key_t> NewOrbit;
        NewOrbit.push_back(i);
        InOrbit[i] = true;
        for (size_t j = 0; j < NewOrbit.size(); ++j) {
            for (const auto& Perm : Perms) {
                key_t im = Perm[NewOrbit[j]];
                if (InOrbit[im])
                    continue;
                NewOrbit.push_back(im);
                InOrbit[im] = true;
            }
        }
        sort(NewOrbit.begin(), NewOrbit.end());
        Orbits.push_back(NewOrbit);
    }

    return Orbits;
}

/*
vector<vector<key_t> > orbits(const vector<vector<key_t> >& Perms, size_t N){

    vector<vector<key_t> > Orbits;
    if(Perms.size()==0){  //each element is its own orbit
        Orbits.reserve(N);
        for(size_t i=0;i<N;++i)
            Orbits.push_back(vector<key_t>(1,i));
        return Orbits;
    }
    Orbits=cycle_decomposition(Perms[0],true); // with fixed points!
    list<dynamic_bitset> P1=partition(Perms[0].size(),Orbits);
    for(size_t i=1;i<Perms.size();++i){
        vector<vector<key_t> > Orbits_1=cycle_decomposition(Perms[i]);
        list<dynamic_bitset> P2=partition(Perms[0].size(),Orbits_1);
        P1=join_partitions(P1,P2);
    }
    return keys(P1);
}
*/

vector<vector<key_t> > convert_to_orbits(const vector<key_t>& raw_orbits) {
    vector<key_t> key(raw_orbits.size());
    vector<vector<key_t> > orbits;
    for (key_t i = 0; i < raw_orbits.size(); ++i) {
        if (raw_orbits[i] == i) {
            orbits.push_back(vector<key_t>(1, i));
            key[i] = orbits.size() - 1;
        }
        else {
            orbits[key[raw_orbits[i]]].push_back(i);
        }
    }
    return orbits;
}

vector<vector<key_t> > cycle_decomposition(vector<key_t> perm, bool with_fixed_points) {
    // computes the cacle decomposition of a permutation with or wothout fixed points

    vector<vector<key_t> > dec;
    vector<bool> in_cycle(perm.size(), false);
    for (size_t i = 0; i < perm.size(); ++i) {
        if (in_cycle[i])
            continue;
        if (perm[i] == i) {
            if (!with_fixed_points)
                continue;
            vector<key_t> cycle(1, i);
            in_cycle[i] = true;
            dec.push_back(cycle);
            continue;
        }
        in_cycle[i] = true;
        key_t next = i;
        vector<key_t> cycle(1, i);
        while (true) {
            next = perm[next];
            if (next == i)
                break;
            cycle.push_back(next);
            in_cycle[next] = true;
        }
        dec.push_back(cycle);
    }
    return dec;
}

void pretty_print_cycle_dec(const vector<vector<key_t> >& dec, ostream& out) {
    for (const auto& i : dec) {
        out << "(";
        for (size_t j = 0; j < i.size(); ++j) {
            out << i[j] + 1;
            if (j != i.size() - 1)
                out << " ";
        }
        out << ") ";
    }
    out << "--" << endl;
}

template class AutomorphismGroup<long>;
template class AutomorphismGroup<long long>;
template class AutomorphismGroup<mpz_class>;

template class Isomorphism_Classes<long>;
template class Isomorphism_Classes<long long>;
template class Isomorphism_Classes<mpz_class>;

template class IsoType<long>;
template class IsoType<long long>;
template class IsoType<mpz_class>;

#ifdef ENFNORMALIZ
template class AutomorphismGroup<renf_elem_class>;
template class Isomorphism_Classes<renf_elem_class>;
template class IsoType<renf_elem_class>;
#endif

}  // namespace libnormaliz
