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

#ifndef LIBNORMALIZ_COLLECTION_H_
#define LIBNORMALIZ_COLLECTION_H_

#include <vector>
#include <map>
#include <set>
#include <string>
#include <utility>  // for pair

#include <libnormaliz/cone.h>

namespace libnormaliz {

template <typename Integer>
class ConeCollection;

template <typename Integer>
class MiniCone {
    friend class ConeCollection<Integer>;

    vector<key_t> GenKeys;
    bool is_simplex;

    key_t my_place;
    int level;

    list<key_t> Daughters;

    // Matrix <Integer> Genereators;
    Matrix<Integer> SupportHyperplanes;
    // Matrix<Integer> HilbertBasis;
    Integer multiplicity;

    ConeCollection<Integer>* Collection;

    // Cone<Integer> make_cone();
    bool refine(const key_t key, bool& interior, bool only_containement = false);
    bool contains(const key_t key, bool& interior);

    void print() const;

    MiniCone<Integer>(const vector<key_t> GKeys, const Integer& mult, ConeCollection<Integer>& Coll);
};

template <typename Integer>
class ConeCollection {
    friend class MiniCone<Integer>;
    template <typename>
    friend class Cone;

    vector<vector<MiniCone<Integer> > > Members;

    Matrix<Integer> Generators;
    set<vector<Integer> > AllRays;
    vector<pair<vector<key_t>, Integer> > KeysAndMult;

    bool is_initialized;
    bool is_fan;
    bool is_triangulation;
    bool verbose;

    void refine(const key_t key);
    void add_minicone(const int level, const key_t mother, const vector<key_t>& GKeys, const Integer& multiplicity);
    void print() const;

    void addsupport_hyperplanes();
    void insert_vectors(const list<pair<key_t, pair<key_t, key_t> > >& NewRays);
    void locate(const key_t key, list<pair<key_t, pair<key_t, key_t> > >& places);
    void locate(const Matrix<Integer>& NewGens, list<pair<key_t, pair<key_t, key_t> > >& NewRays, bool is_generators = false);

   public:
    void initialize_minicones(const vector<pair<vector<key_t>, Integer> >& Triangulation);
    void set_up(const Matrix<Integer>& Gens, const vector<pair<vector<key_t>, Integer> >& Triangulation);

    void insert_all_gens();
    void make_unimodular();
    void add_extra_generators(const Matrix<Integer>& NewGens);

    void flatten();
    const vector<pair<vector<key_t>, Integer> >& getKeysAndMult() const;
    const Matrix<Integer>& getGenerators() const;
    // ConeCollection(Cone<Integer> C, bool from_triangulation);
    ConeCollection();
};

}  // namespace libnormaliz

#endif /* LIBNORMALIZ_COLLECTION_H_ */
