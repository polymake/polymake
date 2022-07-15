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

#ifndef LIBNORMALIZ_DESCENT_H_
#define LIBNORMALIZ_DESCENT_H_

#include <vector>
#include <set>
#include <list>
#include <map>

#include <libnormaliz/general.h>
#include <libnormaliz/matrix.h>
#include <libnormaliz/sublattice_representation.h>
#include "libnormaliz/dynamic_bitset.h"
#include "libnormaliz/automorph.h"

namespace libnormaliz {
using std::map;
using std::pair;
using std::vector;

template <typename Integer>
class IsoType;

template <typename Integer>
class OrbitInfo {
   public:
    vector<key_t> FacetInOrbit;  // the selected facet in the orbit
    vector<size_t> SizeOfOrbit;
    vector<Integer> HeightFixPointOverFacet;  // the height of the fix_point
    vector<Integer> fix_point;
    mpz_class deg_fix_point;
};

template <typename Integer>
class DescentSystem;

template <typename Integer>
class DescentFace {
    template <typename>
    friend class DescentSystem;

    bool dead;  // to be skipped in next round.
    // size_t dim; // cone dimension of the face
    mpq_class coeff;
    // OrbitInfo<Integer>* Orbits;
    // bool simplicial;
    size_t tree_size;  // the number of paths in the tree from top to to this face
    // dynamic_bitset own_facets; // own_facets[i]==true <==> SuppHyps[i] contains this face

    dynamic_bitset FacetsOfFace;  // an indicator picking for each facet F of *this a facet of the cone
                                  // cutting out F from *this
#ifdef NMZ_HASHLIBRARY
    vector<unsigned char> ERC_Hash;
#else
    vector<long> ERC_Hash;
#endif
   public:
    DescentFace();
    // DescentFace(const size_t dim_given, const dynamic_bitset& facets_given);

    void compute(DescentSystem<Integer>& FF,  // comments see cpp
                 const size_t dim,
                 const dynamic_bitset& own_facets,
                 vector<key_t>& mother_key,
                 vector<key_t>& CuttingFacet,
                 list<pair<dynamic_bitset, DescentFace<Integer> > >& Children);

    void compute_with_orbits(DescentSystem<Integer>& FF,
                             const size_t dim,
                             const dynamic_bitset& signature,
                             list<pair<dynamic_bitset, DescentFace<Integer> > >& Children);
};

template <typename Integer>
class DescentSystem {
    template <typename>
    friend class DescentFace;

    bool verbose;
    bool facet_based;

    Matrix<Integer> Gens;
    Matrix<Integer> SuppHyps;
    vector<Integer> Grading;
    vector<Integer> GradGens;
    vector<mpz_class> GradGens_mpz;

    bool SimplePolytope;
    bool exploit_automorphisms;
    bool strict_type_check;

    size_t dim;
    size_t nr_supphyps;
    size_t nr_gens;

    size_t descent_steps;
    size_t nr_simplicial;
    size_t tree_size;
    size_t system_size;

    vector<dynamic_bitset> SuppHypInd;

    map<dynamic_bitset, DescentFace<Integer> > OldFaces;
    map<dynamic_bitset, DescentFace<Integer> > NewFaces;

    list<OrbitInfo<Integer> > OldFacesOrbitInfos;

    vector<size_t> OldNrFacetsContainingGen;
    vector<size_t> NewNrFacetsContainingGen;

    mpq_class multiplicity;

    /* mpq_class mult_simp( const dynamic_bitset&  SimpInds, const vector<key_t>& SimpKeys,
                            const Sublattice_Representation<Integer>& sub_latt,
                            const vector<Integer>&  selected_apex, const mpz_class& deg_selected_apex) const;
    void find_iso_type_and_orbit_data(IsoType<Integer>& IT, const dynamic_bitset& GensInd,
                                      DescentFace<Integer>& F, OrbitInfo<Integer>& MyOrbits);*/

    void collect_old_faces_in_iso_classes(size_t& nr_iso_classes);
    void make_orbits_global();

   public:
    DescentSystem(Matrix<Integer>& Gens, Matrix<Integer>& SuppHyps, vector<Integer>& Grading, bool swap_allowed = true);
    DescentSystem();
    void compute();
    bool set_verbose(bool onoff);
    void setExploitAutoms(bool exploit);
    void setStrictIsoTypeCheck(bool check);
    mpq_class getMultiplicity();
};

}  // namespace libnormaliz

#endif /* DESCENT_H_ */
