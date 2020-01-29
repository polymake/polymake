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

namespace libnormaliz {
using std::map;
using std::pair;
using std::vector;

template <typename Integer>
class DescentSystem;

template <typename Integer>
class DescentFace {
   public:
    // size_t dim; // cone dimension of the face
    mpq_class coeff;
    // bool facets_computed;
    // bool multiplicity_computed;
    bool simplicial;
    size_t tree_size;  // the number of paths in the tree from top to to this face
    // dynamic_bitset own_facets; // own_facets[i]==true <==> SuppHyps[i] contains this face

    // libnormaliz::key_t selected_gen; // the generator of C selected for descent
    // vector<dynamic_bitset> opposite_facets; // facets opposite to the selected generator,
    // identified by the SuppsHyps containing them
    // vector<Integer> heights; // over opposite  facets
    // vector<key_t> CuttingFacet; // the facets of C cutting out the opposite facets

    DescentFace();
    // DescentFace(const size_t dim_given, const dynamic_bitset& facets_given);
    void compute(DescentSystem<Integer>& FF,
                 size_t dim,
                 const dynamic_bitset& own_facets,
                 vector<key_t>& mother_key,
                 vector<dynamic_bitset>& opposite_facets,
                 vector<key_t>& CuttingFacet,
                 vector<Integer>& heights,
                 key_t& selected_gen);
};

template <typename Integer>
class DescentSystem {
   public:
    bool verbose;

    Matrix<Integer> Gens;
    Matrix<Integer> SuppHyps;
    vector<Integer> Grading;
    vector<Integer> GradGens;
    vector<mpz_class> GradGens_mpz;

    bool SimplePolytope;

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

    vector<size_t> OldNrFacetsContainingGen;
    vector<size_t> NewNrFacetsContainingGen;

    mpq_class multiplicity;

    DescentSystem(const Matrix<Integer>& Gens, const Matrix<Integer>& SuppHyps, const vector<Integer>& Grading);
    DescentSystem();
    void compute();
    bool set_verbose(bool onoff);
    mpq_class getMultiplicity();
};

}  // namespace libnormaliz

#endif /* DESCENT_H_ */