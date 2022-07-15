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

#ifndef LIBNORMALIZ_AUTOMORPHISM_H
#define LIBNORMALIZ_AUTOMORPHISM_H

#include <set>

#include "libnormaliz/general.h"
#include "libnormaliz/dynamic_bitset.h"
#include "libnormaliz/matrix.h"
#include "libnormaliz/nmz_nauty.h"
#include "libnormaliz/descent.h"
// #include "libnormaliz/HilbertSeries.h"

namespace libnormaliz {
using namespace std;

string quality_to_string(AutomParam::Quality quality);

template <typename Integer>
class Cone;
template <typename Integer>
class Full_Cone;
template <typename Integer>
class Isomorphism_Classes;

template <typename Integer>
class AutomorphismGroup {
    template <typename>
    friend class Cone;
    template <typename>
    friend class Full_Cone;
    template <typename>
    friend class Isomorphism_Classes;
    template <typename>
    friend class DescentSystem;

    Matrix<Integer> GensRef, SpecialGensRef, LinFormsRef, SpecialLinFormsRef;
    // "ref" stands for "reference"
    // the data defining the cone. Usially Gens = extreme rays, LinForms = support hyperplanes
    // SpecialGens: vectors to be left fixed, forv example the grading if we compute a dual cone
    // SpecialLinforms: grading, dehomogenization and possibly others

    Matrix<Integer> GensComp, LinFormsComp;  // used for computation
    // gives us the flexibility to use extra generators or linear forms in the computation
    // for example: GensComp = HilbertBasis if the extreme rays are not enough for
    // the computation of integral automorphisms

    bool addedComputationGens, addedComputationLinForms;
    bool makeCanType;

    map<dynamic_bitset, key_t> IncidenceMap;

    vector<vector<key_t> > GenPerms;
    vector<vector<key_t> > LinFormPerms;

    vector<vector<key_t> > ExtRaysPerms;   // used in Cone and computed there !!!!!!!
    vector<vector<key_t> > VerticesPerms;  // ditto
    vector<vector<key_t> > SuppHypsPerms;  // ditto

    vector<vector<key_t> > GenOrbits;
    vector<vector<key_t> > LinFormOrbits;

    vector<vector<key_t> > ExtRaysOrbits;   // used in Cone and computed there !!!!!!!
    vector<vector<key_t> > VerticesOrbits;  // ditto
    vector<vector<key_t> > SuppHypsOrbits;  // ditto

    vector<key_t> CanLabellingGens;

    vector<Matrix<Integer> > LinMaps;
    void compute_incidence_map();

    mpz_class order;

    bool cone_dependent_data_computed;

    size_t nr_special_gens, nr_special_linforms;

    set<AutomParam::Goals> is_Computed;
    set<AutomParam::Quality> Qualities;
    AutomParam::Method method;
    bool is_integral;
    bool integrality_checked;

    bool make_linear_maps_primal(const Matrix<Integer>& GivenGens, const vector<vector<key_t> >& ComputedGenPerms);
    void gen_data_via_lin_maps();
    void linform_data_via_lin_maps();
    void linform_data_via_incidence();
    void reset();

    void set_basic_gens_and_lin_forms(const Matrix<Integer>& ExtRays,
                                      const Matrix<Integer>& SpecialGens,
                                      const Matrix<Integer>& SuppHyps,
                                      const Matrix<Integer>& SpecialLinForms);

    bool compute_inner(const AutomParam::Quality& desired_quality, const bool force_gens_x_linforms = false);
    bool compute_integral();
    bool compute_polytopal(const AutomParam::Quality& desired_quality);
    void dualize();
    void swap_data_from_dual(AutomorphismGroup<Integer> Dual);
    void swap_data_from(AutomorphismGroup<Integer> Copy);
    nauty_result<Integer> prepare_Gns_only_and_apply_nauty(const AutomParam::Quality& desired_quality);
    nauty_result<Integer> prepare_Gns_x_LF_only_and_apply_nauty(const AutomParam::Quality& desired_quality);

   public:
    BinaryMatrix<Integer> CanType;  // see nauty

    const Matrix<Integer>& getGens() const;
    const Matrix<Integer>& getLinForms() const;
    const Matrix<Integer>& getSpecialLinForms() const;

    mpz_class getOrder() const;

    const vector<vector<key_t> >& getGensPerms() const;
    const vector<vector<key_t> >& getGensOrbits() const;
    const vector<vector<key_t> >& getLinFormsPerms() const;
    const vector<vector<key_t> >& getLinFormsOrbits() const;

    const vector<vector<key_t> >& getExtremeRaysPerms() const;          // as mentioned above, these data
    const vector<vector<key_t> >& getVerticesPerms() const;             // are defined w.r.t. to a calling cone
    const vector<vector<key_t> >& getSupportHyperplanesPerms() const;   // ...
    const vector<vector<key_t> >& getExtremeRaysOrbits() const;         // ...
    const vector<vector<key_t> >& getVerticesOrbits() const;            // ...
    const vector<vector<key_t> >& getSupportHyperplanesOrbits() const;  // ...

    const vector<Matrix<Integer> >& getLinMaps() const;
    const vector<key_t>& getCanLabellingGens() const;

    void setGensRef(const Matrix<Integer>& GivenGensRef);  // if GensRef are set later

    void setIncidenceMap(const map<dynamic_bitset, key_t>& Incidence);
    void activateCanType(bool onoff = true);
    set<AutomParam::Quality> getQualities() const;
    AutomParam::Method getMethod() const;
    bool Is_Computed(AutomParam::Goals goal) const;
    string getQualitiesString() const;
    bool HasQuality(AutomParam::Quality quality) const;
    bool IsIntegral() const;
    bool IsIntegralityChecked() const;
    bool IsAmbient() const;
    bool IsInput() const;

    list<vector<Integer> > orbit_primal(const vector<Integer>& v) const;
    void add_images_to_orbit(const vector<Integer>& v, set<vector<Integer> >& orbit) const;

    const BinaryMatrix<Integer>& getCanType() const;

    // bool compute(const AutomParam::Quality& desired_quality, const set<AutomParam::Goals>& ToCompute); // not yet implemented

    bool compute(const AutomParam::Quality& desired_quality, const bool force_gens_x_linforms = false);

    AutomorphismGroup();

    AutomorphismGroup(const Matrix<Integer>& ExtRays, const Matrix<Integer>& SupHyps, const Matrix<Integer>& SpecialLinForms);

    AutomorphismGroup(const Matrix<Integer>& ExtRays,
                      const Matrix<Integer>& SpecialGens,
                      const Matrix<Integer>& SuppHyps,
                      const Matrix<Integer>& SpecialLinForms);

    void addComputationGens(const Matrix<Integer>& GivenGens);
    void addComputationLinForms(const Matrix<Integer>& GivenLinearForms);

};  // end class

template <typename Integer>
class Isomorphism_Classes;

/*
template <typename Integer>
class IsoType {
    template <typename>
    friend class Isomorphism_Classes;

    AutomParam::Quality quality;

    size_t rank;
    Matrix<Integer> ExtremeRays;
    size_t nrExtremeRays;
    // Matrix<Integer> SupportHyperplanes;
    size_t nrSupportHyperplanes;
    Matrix<Integer> HilbertBasis;  // without extreme rays
    // size_t nrHilbertBasis; // with extreme rays, but not used
    vector<Integer> Grading;
    vector<Integer> Truncation;
    // HilbertSeries HilbertSer;
    mpq_class Multiplicity;
    bool needs_Hilbert_basis;

    // For the coordinate transformation to the canonical basis
    vector<key_t> CanLabellingGens;
    Matrix<Integer> CanTransform;
    Integer CanDenom;
    vector<key_t> CanBasisKey;

    BinaryMatrix<Integer> CanType;
    const BinaryMatrix<Integer>& getCanType() const;
    IsoType();  // constructs a dummy object

   public:
    bool isOfType(const Full_Cone<Integer>& C) const;
    // bool isOfType(Cone<Integer>& C) const;

    IsoType(const Full_Cone<Integer>& C, bool& success);  // success indicates whether a class could be created
    IsoType(Cone<Integer>& C);

    // size_t getRank();
    // Matrix<Integer> getExtremeRays() const;
    // Matrix<Integer> getSupportHyperplanes() const;
    const Matrix<Integer>& getHilbertBasis() const;
    // vector<Integer> getGrading() const;
    // vector<Integer> getTruncation() const;
    // HilbertSeries getHilbertSeries() const;
    mpq_class getMultiplicity() const;
    const Matrix<Integer>& getCanTransform() const;
    Integer getCanDenom() const;
    const BinaryMatrix<Integer>& getCanType() const;
};
*/

template <typename Integer>
class IsoType {
    template <typename>
    friend class Isomorphism_Classes;

    AutomParam::Type type;

   public:
    BinaryMatrix<Integer> CanType;
    vector<unsigned char> HashValue;
    Integer index;
    // vector<dynamic_bitset> FacetOrbits;

    IsoType();  // constructs a dummy object
    IsoType(Cone<Integer>& C);
    IsoType(const Matrix<Integer>& M);
    IsoType(const Matrix<Integer>& Inequalities,
            const Matrix<Integer> Equations,
            const vector<Integer> Grading,
            const bool strict_type_check);
    IsoType(const Matrix<Integer>& ExtremeRays, const vector<Integer> Grading, const bool strict_type_check);
    const BinaryMatrix<Integer>& getCanType() const;
};

template <typename Integer>
class IsoType_compare {
   public:
    bool operator()(const IsoType<Integer>& A, const IsoType<Integer>& B) const {
#ifdef NMZ_HASHLIBRARY
        if (A.HashValue.size() > 0) {
            if (A.HashValue < B.HashValue)
                return true;
            return false;
        }
#endif
        return BM_compare(A.getCanType(), B.getCanType());
    }
};

template <typename Integer>
class Isomorphism_Classes {
    template <typename>
    friend class Cone;
    template <typename>
    friend class Full_Cone;

    set<IsoType<Integer>, IsoType_compare<Integer> > Classes;

    AutomParam::Type type;

   public:
    Isomorphism_Classes();
    Isomorphism_Classes(AutomParam::Type given_type);

    const IsoType<Integer>& find_type(const IsoType<Integer>& IT, bool& found) const;
    const IsoType<Integer>& add_type(const IsoType<Integer>& IT, bool& found);
    size_t erase_type(const IsoType<Integer>& IT);
    const IsoType<Integer>& find_type(Cone<Integer>& C, bool& found) const;
    const IsoType<Integer>& add_type(Cone<Integer>& C, bool& found);
    size_t erase_type(Cone<Integer>& C);

    const set<IsoType<Integer>, IsoType_compare<Integer> >& getClasses() const;

    size_t size() const;
};

vector<vector<key_t> > convert_to_orbits(const vector<key_t>& raw_orbits);

vector<vector<key_t> > cycle_decomposition(vector<key_t> perm, bool with_fixed_points = false);

void pretty_print_cycle_dec(const vector<vector<key_t> >& dec, ostream& out);

vector<vector<key_t> > keys(const list<dynamic_bitset>& Partition);

list<dynamic_bitset> partition(size_t n, const vector<vector<key_t> >& Orbits);

list<dynamic_bitset> join_partitions(const list<dynamic_bitset>& P1, const list<dynamic_bitset>& P2);

vector<vector<key_t> > orbits(const vector<vector<key_t> >& Perms, size_t N);

vector<vector<key_t> > PermGroup(const vector<vector<key_t> >& Perms, size_t N);

}  // namespace libnormaliz

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
