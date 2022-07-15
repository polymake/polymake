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

#include <iomanip>

#include "libnormaliz/cone.h"
#include "libnormaliz/descent.h"
#include "libnormaliz/vector_operations.h"
#include "libnormaliz/sublattice_representation.h"
#include "libnormaliz/list_and_map_operations.h"
#include "libnormaliz/nmz_hash.h"

namespace libnormaliz {

template <typename Integer>
DescentFace<Integer>::DescentFace() {
    // simplicial = false;
    coeff = 0;
    tree_size = 0;
    dead = false;
}

template <typename Integer>
DescentSystem<Integer>::DescentSystem() {
    descent_steps = 0;
    tree_size = 0;
    nr_simplicial = 0;
    system_size = 0;
    exploit_automorphisms = false;
    facet_based = true;  // the standard case
#ifdef NMZ_HASHLIBRARY
    strict_type_check = false;
#else
    strict_type_check = true;
#endif
}

template <typename Integer>
DescentSystem<Integer>::DescentSystem(Matrix<Integer>& Gens_given,
                                      Matrix<Integer>& SuppHyps_given,
                                      vector<Integer>& Grading_given,
                                      bool swap_allowed) {
    descent_steps = 0;
    tree_size = 0;
    nr_simplicial = 0;
    system_size = 0;
    exploit_automorphisms = false;
#ifndef NMZ_HASHLIBRARY
    strict_type_check = true;
#endif

    if (swap_allowed) {
        swap(Gens, Gens_given);
        swap(SuppHyps, SuppHyps_given);
        swap(Grading, Grading_given);
    }
    else {
        Gens = Gens_given;
        SuppHyps = SuppHyps_given;
        Grading = Grading_given;
    }

    nr_gens = Gens.nr_of_rows();
    nr_supphyps = SuppHyps.nr_of_rows();
    dim = Gens.nr_of_columns();

    facet_based = true;
    if (nr_gens < nr_supphyps)
        facet_based = false;

    GradGens.resize(nr_gens);
    GradGens_mpz.resize(nr_gens);
    for (size_t i = 0; i < nr_gens; ++i) {
        GradGens[i] = v_scalar_product(Grading, Gens[i]);
        convert(GradGens_mpz[i], GradGens[i]);
    }

    multiplicity = 0;

    makeIncidenceMatrix(SuppHypInd, Gens, SuppHyps);

    SimplePolytope = true;

    for (size_t j = 0; j < nr_gens; ++j) {
        size_t NrFacetsContainingGen = 0;
        for (size_t i = 0; i < nr_supphyps; ++i) {
            INTERRUPT_COMPUTATION_BY_EXCEPTION
            if (SuppHypInd[i][j]) {
                NrFacetsContainingGen++;
            }
        }
        if (NrFacetsContainingGen > dim - 1) {
            SimplePolytope = false;
            break;
        }
    }

    OldNrFacetsContainingGen.resize(nr_gens, 1);
    NewNrFacetsContainingGen.resize(nr_gens, 0);

    OldNrFacetsContainingGen.resize(nr_gens, 1);
    NewNrFacetsContainingGen.resize(nr_gens, 0);
}

/*
#ifdef ENFNORMALIZ
template <>
DescentSystem<renf_elem_class>::DescentSystem() {
    assert(false);
}

template <>
DescentSystem<renf_elem_class>::DescentSystem(Matrix<renf_elem_class>& Gens_given,
                                      Matrix<renf_elem_class>& SuppHyps_given,
                                      vector<renf_elem_class>& Grading_given,
                                      bool swap_allowed ) {
    assert(false);
}
#endif
*/

template <typename Integer>
void DescentFace<Integer>::compute(
    DescentSystem<Integer>& FF,       // not const since we change multiplicity
    const size_t dim,                 //  dim of *this
    const dynamic_bitset& signature,  // indicates
    // (i) in the facet based case the supphyps of which *this is the intersection
    // (ii) in the generator based case the extreme rays of Üthis
    //
    // return values
    //
    vector<key_t>& extrays_of_this,  // will indicate the extreme rays of *this
                                     // used after return from this function to count the number of total  faces containing
                                     // the selected extreme rays
                                     // these data are used in this function for the choice of the optimal vertex // used as a
                                     // signature in the descent system
    vector<key_t>& opposite_facets,  // the indices of facets opposite to selected extreme ray (not unique),
                                     // also used for optmization
    list<pair<dynamic_bitset, DescentFace<Integer> > >& Children  // the children of *this
                                                                  // that are sent into the next lower codimension
) {
    long omp_start_level = omp_get_level();

    extrays_of_this.clear();
    opposite_facets.clear();
    Children.clear();

    size_t nr_supphyps = FF.nr_supphyps;
    size_t nr_gens = FF.nr_gens;
    size_t d = dim;

    dynamic_bitset cone_facets_cutting_this_out(nr_supphyps);  // facets of cone cutting *this out
    dynamic_bitset GensInd(nr_gens);                           // extreme rays of *this, indicated by GensInd

    if (FF.facet_based) {
        cone_facets_cutting_this_out = signature;
        GensInd.set();
        for (size_t i = 0; i < nr_supphyps; ++i) {  // find Gens in this
            if (cone_facets_cutting_this_out[i] == true) {
                GensInd = GensInd & FF.SuppHypInd[i];
            }
        }
    }
    else {
        GensInd = signature;
        for (size_t i = 0; i < nr_supphyps; ++i) {
            if (GensInd.is_subset_of(FF.SuppHypInd[i]))
                cone_facets_cutting_this_out[i] = true;
        }
    }

    for (size_t i = 0; i < nr_gens; ++i)
        if (GensInd[i])
            extrays_of_this.push_back(i);

    Matrix<Integer> Gens_this;

    if (extrays_of_this.size() > 3 * dim) {  //
        try {
            size_t nr_selected = 3 * dim;
            vector<key_t> selection;
            key_t j;
            size_t rk = 0;
            while (rk < dim && nr_selected <= extrays_of_this.size()) {
                selection.resize(nr_selected);
                for (size_t i = 0; i < nr_selected; ++i) {
                    j = rand() % extrays_of_this.size();
                    selection[i] = extrays_of_this[j];
                }
                Gens_this = FF.Gens.submatrix(selection);
                rk = Gens_this.row_echelon();
                nr_selected *= 2;
            }
            if (rk < dim) {
                Gens_this = FF.Gens.submatrix(extrays_of_this);
                Gens_this.row_echelon();
            }
        } catch (const ArithmeticException& e) {
            Gens_this = FF.Gens.submatrix(extrays_of_this);
            Gens_this.row_echelon();
        }
    }
    else {
        Gens_this = FF.Gens.submatrix(extrays_of_this);
        Gens_this.row_echelon();
    }

    bool must_saturate = false;

    for (size_t i = 0; i < Gens_this.nr_of_rows(); ++i) {
        for (size_t j = i; j < FF.dim; ++j) {
            if (Gens_this[i][j] == 0)
                continue;
            if (Gens_this[i][j] != 1 && Gens_this[i][j] != -1) {
                must_saturate = true;
            }
            break;
        }
        if (must_saturate)
            break;
    }

    Sublattice_Representation<Integer> Sublatt_this;
    if (must_saturate)
        Sublatt_this = Sublattice_Representation<Integer>(Gens_this, true, false);  //  take saturation, no LLL

    // Now we find the potential facets of *this.

    dynamic_bitset facet_ind(extrays_of_this.size());  // lists Gens, local variable for work
    map<dynamic_bitset, dynamic_bitset> FacetInds;     // potential facets, map from gens(potential facet)
                                                       //  to set of supphyps(C) containing these gens
                                                       // reference for gens(potential facet) is the selection via extrays_of_this
    map<dynamic_bitset, key_t> CutOutBy;               // the facet citting it out (we must choose one)

    map<dynamic_bitset, vector<key_t> > SimpKeys;  // generator keys for simplicial facets
    map<dynamic_bitset, vector<bool> > SimpInds;   // alternative: generator indices for simplicial facets (if less memory needed)
    bool ind_better_than_keys = (dim * 64 > FF.nr_gens);  // decision between the alternatives

    for (size_t i = 0; i < nr_supphyps; ++i) {
        if (cone_facets_cutting_this_out[i] == true)  // contains *this
            continue;

        // we can identify the facet(*this) uniquely only via the Gens in it
        vector<libnormaliz::key_t> facet_key;  // keys of extreme rays in current supphyp of cone
        for (size_t k = 0; k < extrays_of_this.size(); ++k) {
            if (FF.SuppHypInd[i][extrays_of_this[k]] == true)
                facet_key.push_back(k);
        }
        if (facet_key.size() < d - 1)  // can't be a facet(*this)
            continue;

        // now we make facet_ind out of facet_key: key for gens in potatntial facet
        facet_ind.reset();
        for (unsigned int jj : facet_key)
            facet_ind[jj] = true;

        // next we check whether we have the intersection already
        // not necessary for simple polytopes and in top dimension
        // Note: if P is simple, F is a face of P and H a support hyperplave of P,
        // then F\cap H is either empty or a facet of F. Moreover H is eniquely determined
        // by F\cap H. This will again be used below.
        if (d < FF.dim && !FF.SimplePolytope) {
            if (FacetInds.find(facet_ind) != FacetInds.end()) {  // already found, we need it only once
                if (facet_key.size() > d - 1)
                    FacetInds[facet_ind][i] = true;
                // but in the nonsimplicial case we must add SuppHyps[i] to the facets(C) containing
                // the current facet(*this)
                continue;
            }
        }

        // now we have a new potential facet
        if (facet_key.size() == d - 1) {               // simplicial or not a facet
            FacetInds[facet_ind] = dynamic_bitset(0);  // don't need support hyperplanes
            CutOutBy[facet_ind] = FF.nr_supphyps + 1;  // signalizes "simplicial facet"
            if (ind_better_than_keys) {                // choose shorter representation
                vector<bool> gen_ind(FF.nr_gens);
                for (unsigned int k : facet_key)
                    gen_ind[extrays_of_this[k]] = 1;
                SimpInds[facet_ind] = gen_ind;
            }
            else {
                vector<key_t> trans_key;  // translate back to FF
                for (unsigned int k : facet_key)
                    trans_key.push_back(extrays_of_this[k]);
                SimpKeys[facet_ind] = trans_key;  // helps to pick the submatrix of its generators
            }
        }
        else {
            FacetInds[facet_ind] = cone_facets_cutting_this_out;
            FacetInds[facet_ind][i] = true;  // plus the facet cutting out facet_ind
            CutOutBy[facet_ind] = i;         // memorize the facet that cuts it out
        }
    }

    // if we don't have the coordinate transformation and there is a simplicial facet, we must make it
    if (!must_saturate && (SimpKeys.size() > 0 || SimpInds.size() > 0))
        Sublatt_this = Sublattice_Representation<Integer>(Gens_this, true, false);  //  take saturation, no LLL

    if (d < FF.dim && !FF.SimplePolytope) {  // now we select the true facets of *this
        if (FacetInds.size() < d * d) {
            auto G = FacetInds.end();  // by taking those with a maximal set of gens
            for (--G; G != FacetInds.begin(); --G) {
                for (auto F = FacetInds.begin(); F != G;) {
                    if (F->first.is_subset_of(G->first))
                        F = FacetInds.erase(F);
                    else
                        ++F;
                }
            }
        }
        else {
            auto G = FacetInds.begin();
            for (; G != FacetInds.end();) {
                vector<key_t> trans_key;  // translate back to FF
                vector<key_t> local_key = bitset_to_key(G->first);
                for (unsigned int k : local_key)
                    trans_key.push_back(extrays_of_this[k]);
                Matrix<Integer> RankTest = FF.Gens.submatrix(trans_key);
                if (RankTest.rank() < d - 1)
                    G = FacetInds.erase(G);
                else
                    ++G;
            }
        }
    }

    // At this point we know the facets of *this.
    // The map FacetInds assigns the set of containing SuppHyps(cone) to the facet_ind(Gens).
    // The set of containing SuppHyps is a unique signature as well.

    // Now we want to find the generator with the lrast number opf opposite facets(*this)
    vector<size_t> count_in_facets(extrays_of_this.size());
#pragma omp parallel for
    for (size_t i = 0; i < extrays_of_this.size(); ++i) {
        size_t k = i;
        for (auto& FacetInd : FacetInds)
            if ((FacetInd.first)[k] == true)
                count_in_facets[k]++;
    }

    size_t m = count_in_facets[0];  // we must have at least one facet (actually 3, since dim 2 is simplicial)
    libnormaliz::key_t m_ind = 0;

    for (size_t i = 1; i < count_in_facets.size(); ++i) {
        if (count_in_facets[i] > m) {
            m = count_in_facets[i];
            m_ind = i;
            continue;
        }
        if (count_in_facets[i] == m &&
            FF.OldNrFacetsContainingGen[extrays_of_this[i]] < FF.OldNrFacetsContainingGen[extrays_of_this[m_ind]]) {
            m_ind = i;
        }
    }

    key_t selected_gen = extrays_of_this[m_ind];  // this is the selected generator
    vector<Integer> embedded_selected_gen;
    if (must_saturate)
        embedded_selected_gen = Sublatt_this.to_sublattice(FF.Gens[selected_gen]);

    // now we must find the facets opposite to thge selected generator

    vector<Integer> embedded_supphyp;
    Integer ht;

    mpq_class divided_coeff = coeff / FF.GradGens_mpz[selected_gen];

    auto G = FacetInds.begin();
    for (; G != FacetInds.end(); ++G) {
        INTERRUPT_COMPUTATION_BY_EXCEPTION

        if ((G->first)[m_ind] == false && CutOutBy[G->first] != FF.nr_supphyps + 1) {  // is opposite and not simplicial

            dynamic_bitset the_name_of_the_child;
            if (FF.facet_based)
                the_name_of_the_child = G->second;  // supphyps are the signature
            else {
                // extrays are the signature
                the_name_of_the_child.resize(nr_gens);             // in the first step we must tranlate
                for (size_t kk = 0; kk < G->first.size(); ++kk) {  // G->first into an indictaor relative to the global
                    if ((G->first)[kk])                            // list of extreme rays
                        the_name_of_the_child[extrays_of_this[kk]] = 1;
                }
            }

            auto H = Children.insert(Children.begin(), make_pair(the_name_of_the_child, DescentFace<Integer>()));
            if (must_saturate) {
                embedded_supphyp = Sublatt_this.to_sublattice_dual(FF.SuppHyps[CutOutBy[G->first]]);
                ht = v_scalar_product(embedded_selected_gen, embedded_supphyp);
            }
            else {
                embedded_supphyp = Gens_this.MxV(FF.SuppHyps[CutOutBy[G->first]]);
                Integer den = v_make_prime(embedded_supphyp);
                ht = v_scalar_product(FF.Gens[selected_gen], FF.SuppHyps[CutOutBy[G->first]]) / den;
            }

            H->second.coeff = divided_coeff * convertTo<mpz_class>(ht);
            opposite_facets.push_back(CutOutBy[G->first]);

            if (FF.exploit_automorphisms && FF.facet_based) {      // Absolutely necessary, used in definitiomn of IsoType
                dynamic_bitset ExtRaysFacet(FF.nr_gens);           // in the first step we must tranlate
                for (size_t kk = 0; kk < G->first.size(); ++kk) {  // G->first into an indictaor relative to the global
                    if ((G->first)[kk])                            // list of extreme rays
                        ExtRaysFacet[extrays_of_this[kk]] = 1;
                }
                dynamic_bitset FacetCandidates = ~G->second;  // indicates the gobal support bhyperplanes
                                                              // intersecting this facet in a proper subset
                vector<dynamic_bitset> Intersections(FF.nr_supphyps, dynamic_bitset(nr_gens));
                vector<long> NrExtRays(FF.nr_supphyps);
                for (size_t i = 0; i < FF.nr_supphyps; ++i) {
                    if (FacetCandidates[i] == 0)
                        continue;
                    Intersections[i] = ExtRaysFacet & FF.SuppHypInd[i];
                    NrExtRays[i] = Intersections[i].count();
                }

                dynamic_bitset TheFacets;
                maximal_subsets(Intersections, TheFacets);
                H->second.FacetsOfFace = TheFacets;
                map<long, long> Counter;
                for (size_t i = 0; i < FF.nr_supphyps; ++i) {
                    if (!TheFacets[i])
                        continue;
                    Counter[NrExtRays[i]]++;
                }
                vector<long> ERC;
                for (auto& C : Counter) {
                    ERC.push_back(C.first);
                    ERC.push_back(C.second);
                }
#ifdef NMZ_HASHLIBRARY
                ostringstream VecString;
                VecString << ERC;
                H->second.ERC_Hash = sha256hexvec(VecString.str());
#else
                H->second.ERC_Hash = ERC;
#endif
            }
        }
    }

    if (SimpKeys.size() > 0 || SimpInds.size() > 0) {
        G = FacetInds.begin();
        size_t loop_length = FacetInds.size();
        size_t fpos = 0;
        bool skip_remaining = false;
        vector<mpq_class> thread_mult(omp_get_max_threads(), 0);
        Matrix<Integer> Embedded_Gens(d, d);
        Matrix<Integer> Gens_this(d, FF.dim);

        std::exception_ptr tmp_exception;

#pragma omp parallel for firstprivate(G, fpos, Embedded_Gens, Gens_this)
        for (size_t ff = 0; ff < loop_length; ++ff) {
            if (skip_remaining)
                continue;
            for (; ff > fpos; ++fpos, ++G)
                ;
            for (; ff < fpos; --fpos, --G)
                ;

            int tn;
            if (omp_get_level() == omp_start_level)
                tn = 0;
            else
                tn = omp_get_ancestor_thread_num(omp_start_level + 1);

            try {
                INTERRUPT_COMPUTATION_BY_EXCEPTION

                if ((G->first)[m_ind] == false && CutOutBy[G->first] == FF.nr_supphyps + 1) {  // is opposite and simplicial
                    if (ind_better_than_keys)
                        Gens_this = FF.Gens.submatrix(SimpInds[G->first]);
                    else
                        Gens_this = FF.Gens.submatrix(SimpKeys[G->first]);
                    Gens_this.append(FF.Gens[selected_gen]);
                    Integer det;
                    if (Sublatt_this.IsIdentity())
                        det = Gens_this.vol();
                    else {
                        Embedded_Gens = Sublatt_this.to_sublattice(Gens_this);
                        det = Embedded_Gens.vol();
                    }
                    mpz_class mpz_det = convertTo<mpz_class>(det);
                    mpq_class multiplicity = mpz_det;
                    if (ind_better_than_keys) {
                        for (size_t i = 0; i < FF.nr_gens; ++i)
                            if (SimpInds[G->first][i] && FF.GradGens[i] > 1)
                                multiplicity /= FF.GradGens_mpz[i];
                    }
                    else {
                        for (size_t i = 0; i < Gens_this.nr_of_rows() - 1; ++i)
                            if (FF.GradGens[SimpKeys[G->first][i]] > 1)
                                multiplicity /= FF.GradGens_mpz[SimpKeys[G->first][i]];
                    }
                    if (FF.GradGens[selected_gen] > 1)
                        multiplicity /= FF.GradGens_mpz[selected_gen];
                    // #pragma omp critical(ADD_MULT)
                    // FF.multiplicity+=multiplicity*coeff;
                    thread_mult[tn] += multiplicity;
#pragma omp atomic
                    FF.nr_simplicial++;
#pragma omp atomic
                    FF.tree_size += tree_size;
                }

            } catch (const std::exception&) {
                tmp_exception = std::current_exception();
                skip_remaining = true;
#pragma omp flush(skip_remaining)
            }
        }

        if (!(tmp_exception == 0))
            std::rethrow_exception(tmp_exception);

        mpq_class local_multiplicity = 0;
        for (const auto& j : thread_mult)
            local_multiplicity += j;

            /* if(!has_non_simp && test_mult != 0){
                cout << local_multiplicity << "    " << test_mult << endl;
                assert(test_mult == local_multiplicity);
            }*/
#pragma omp critical(ADD_MULT)
        FF.multiplicity += local_multiplicity * coeff;
    }
}

template <typename Integer>
void DescentSystem<Integer>::collect_old_faces_in_iso_classes(size_t& nr_iso_classes) {
    if (OldFaces.size() <= 1)  // nothingt to do here
        return;

    // Isomorphism_Classes<Integer> Isos(AutomParam::rational_dual);
    map<IsoType<Integer>, DescentFace<Integer>*, IsoType_compare<Integer> > Isos;

    size_t nr_F = OldFaces.size();
    auto F = OldFaces.begin();
    size_t kkpos = 0;
    std::exception_ptr tmp_exception;
    bool skip_remaining = false;

    const long VERBOSE_STEPS = 50;
    const size_t ReportBound = 200;
    long step_x_size = nr_F - VERBOSE_STEPS;
    size_t total = nr_F;

    if (verbose)
        verboseOutput() << "Collecting isomorphism classes" << endl;

#ifdef NMZ_HASHLIBRARY
    map<vector<unsigned char>, long> CountHashs;
#else
    map<vector<long>, long> CountHashs;
#endif
    if (facet_based) {
        for (auto& X : OldFaces) {
            CountHashs[X.second.ERC_Hash]++;
        }
    }

    if (verbose && facet_based)
        verboseOutput() << "Coarse classes " << CountHashs.size() << endl;

    size_t isolanis = 0;

#ifndef NMZ_NAUTY_TLS
    int save_nr_threads = omp_get_max_threads();
    omp_set_num_threads(1);
#endif

#pragma omp parallel for firstprivate(F, kkpos) schedule(dynamic)
    for (size_t kk = 0; kk < nr_F; ++kk) {
        if (skip_remaining)
            continue;
        try {
            INTERRUPT_COMPUTATION_BY_EXCEPTION

            for (; kk > kkpos; kkpos++, F++)
                ;
            for (; kk < kkpos; kkpos--, F--)
                ;

            if (verbose && nr_F >= ReportBound) {
#pragma omp critical(VERBOSE)
                while ((long)(kk * VERBOSE_STEPS) >= step_x_size) {
                    step_x_size += total;
                    verboseOutput() << "." << flush;
                }
            }
            if (facet_based && CountHashs[F->second.ERC_Hash] == 1) {
#pragma omp atomic
                isolanis++;
                continue;
            }

            IsoType<Integer> IT;
            if (facet_based) {
                Matrix<Integer> Equations = SuppHyps.submatrix(bitset_to_key(F->first));
                Matrix<Integer> Inequalities = SuppHyps.submatrix(bitset_to_key(F->second.FacetsOfFace));
                IT = IsoType<Integer>(Inequalities, Equations, Grading, strict_type_check);
            }
            else {
                Matrix<Integer> ExtRays = Gens.submatrix(bitset_to_key(F->first));
                IT = IsoType<Integer>(ExtRays, Grading, strict_type_check);
            }
#pragma omp critical(INSERT_ISOTYPE)
            {
                auto G = Isos.find(IT);
                if (G != Isos.end()) {
                    mpz_class index_source = convertTo<mpz_class>(IT.index);
                    mpz_class index_taaget = convertTo<mpz_class>(G->first.index);
                    // At this point one could allow non-equality. Then omne needs a correction factor
                    if (index_source == index_taaget) {
                        F->second.dead = true;  // to be skipped in descent
                        G->second->coeff += F->second.coeff;
                    }
                    // cout << "--------------------------" << endl;
                    // cout << "Index Source " << index_source << " Index Target " << index_traget << " CORR " << corr << endl;
                    // cout << "coeff Source " << F->second.coeff << " Coeff Target " << G->second->coeff << endl;
                    // cout << "--------------------------" << endl; */
                }
                if (!F->second.dead) {
                    // cout << "========================" << endl;
                    Isos[IT] = &(F->second);
                    // cout << "New New New " << "Index " << IT.index << " Coeff " << F->second.coeff << endl;
                    // IT.getCanType().pretty_print(cout);
                    // cout << "========================" << endl;
                }
            }

        } catch (const std::exception&) {
            tmp_exception = std::current_exception();
            skip_remaining = true;
#pragma omp flush(skip_remaining)
        }
    }  // parallel for kk

    if (!(tmp_exception == 0))
        std::rethrow_exception(tmp_exception);

    if (verbose && nr_F >= ReportBound)
        verboseOutput() << endl;

#ifndef NMZ_NAUTY_TLS
    omp_set_num_threads(save_nr_threads);
#endif

    nr_iso_classes = Isos.size();
    if (verbose) {
        if (facet_based)
            verboseOutput() << "Coarse classes of 1 element " << isolanis << ", iso types " << nr_iso_classes + isolanis << endl;
        else
            verboseOutput() << "Iso types " << nr_iso_classes << endl;
    }
    /*for(auto& F: OldFaces){
        cout << "DDDD " << F.second.dead << " CCCC " << F.second.coeff << endl;
    }*/
}

//----------------------------------------------------------------------

template <typename Integer>
void DescentSystem<Integer>::make_orbits_global() {
    /* Cone<Integer> C(Type::extreme_rays, Gens, Type::support_hyperplanes, SuppHyps, Type::grading, Matrix<Integer>(Grading));
    C.compute(ConeProperty::Automorphisms);
    vector<vector<key_t> > GenOrbits = C.getAutomorphismGroup().getExtremeRaysOrbits();*/

    AutomorphismGroup<Integer> Aut(Gens, SuppHyps, Grading);
    Aut.compute(AutomParam::integral);
    vector<vector<key_t> > GenOrbits = Aut.getGensOrbits();

    size_t min_at, min_size;
    for (size_t i = 0; i < GenOrbits.size(); ++i) {
        if (i == 0 || GenOrbits[i].size() < min_size) {
            min_size = GenOrbits[i].size();
            min_at = i;
        }
    }
    vector<Integer> fix_point(dim);
    for (size_t i = 0; i < GenOrbits[min_at].size(); ++i) {
        fix_point = v_add(fix_point, Gens[GenOrbits[min_at][i]]);
    }
    v_make_prime(fix_point);
    Integer deg_fix_point = v_scalar_product(fix_point, Grading);

    OldFaces.clear();

    vector<vector<key_t> > SuppOrbits = Aut.getLinFormsOrbits();
    for (auto& Orb : SuppOrbits) {
        dynamic_bitset orb_indicator(nr_gens);
        for (size_t i = 0; i < nr_gens; ++i) {
            if (v_scalar_product(SuppHyps[Orb[0]], Gens[i]) == 0)
                orb_indicator[i] = 1;
        }
        Integer ht_fix_point = v_scalar_product(SuppHyps[Orb[0]], fix_point);
        mpq_class coeff = convertTo<mpz_class>(ht_fix_point);
        coeff *= convertTo<mpz_class>((long long)Orb.size());
        coeff /= convertTo<mpz_class>(deg_fix_point);
        OldFaces[orb_indicator] = DescentFace<Integer>();
        OldFaces[orb_indicator].coeff = coeff;
    }
}

//----------------------------------------------------------------------
/*
template <typename Integer>
void DescentSystem<Integer>::make_orbits_global() {

    Cone<Integer> C(Type::extreme_rays, Gens, Type::support_hyperplanes, SuppHyps, Type::grading, Matrix<Integer>(Grading));
    C.compute(ConeProperty::Automorphisms);
    vector<vector<key_t> > GenOrbits = C.getAutomorphismGroup().getExtremeRaysOrbits();

    size_t min_at, min_size;
    for(size_t i = 0; i< GenOrbits.size(); ++i){
        if(i == 0 || GenOrbits[i].size() < min_size){
            min_size = GenOrbits[i].size();
            min_at = i;
        }
    }
    vector<Integer> fix_point(dim);
    for(size_t i = 0; i< GenOrbits[min_at].size(); ++i){
        fix_point = v_add(fix_point, C.getExtremeRaysMatrix()[GenOrbits[min_at][i]]);
    }
    v_make_prime(fix_point);
    Integer deg_fix_point = v_scalar_product(fix_point, C.getGrading());

    OldFaces.clear();

    vector<vector<key_t> > SuppOrbits = C.getAutomorphismGroup().getSupportHyperplanesOrbits();
    for(auto& Orb: SuppOrbits){
        dynamic_bitset orb_indicator(nr_gens);
        for(size_t i=0; i< nr_gens; ++i){
            if( v_scalar_product(C.getSupportHyperplanes()[Orb[0]],Gens[i]) == 0)
                orb_indicator[i] = 1;
        }
        Integer ht_fix_point = v_scalar_product(C.getSupportHyperplanes()[Orb[0]], fix_point);
        mpq_class coeff = convertTo<mpz_class>(ht_fix_point);
        coeff *= convertTo<mpz_class>((long long) Orb.size());
        coeff /= convertTo<mpz_class>(deg_fix_point);
        OldFaces[orb_indicator] = DescentFace<Integer>();
        OldFaces[orb_indicator].coeff = coeff;
    }
}
*/
//----------------------------------------------------------------------

template <typename Integer>
void DescentSystem<Integer>::compute() {
#ifdef NMZ_EXTENDED_TESTS
    if (!using_GMP<Integer>() && !using_renf<Integer>() && test_arith_overflow_descent)
        throw ArithmeticException(0);
#endif

    if (verbose) {
        if (SimplePolytope)
            verboseOutput() << "Polytope is simple" << endl;
        else
            verboseOutput() << "Polytope is not simple" << endl;
    }

    const size_t ReportBound = 400;
    const size_t MaxBlocksize = 1000000;

    DescentFace<Integer> top;
    top.coeff = 1;
    top.tree_size = 1;
    if (facet_based) {
        dynamic_bitset empty(nr_supphyps);
        OldFaces[empty] = top;
    }
    else {
        dynamic_bitset full(nr_gens);
        full = ~full;
        OldFaces[full] = top;
    }
    long d = (long)dim;

    Integer global_corr_factor = 1;

    if (!facet_based && exploit_automorphisms) {
        // make_orbits_global can potentially compute the Hilbert basis
        // if the extreme rays do not generate the full lattice
        // In this case we make a transformation to the smaller lattice and
        // correct the mutliplicity at the ned
        global_corr_factor = Gens.full_rank_index();
        if (global_corr_factor != 1) {
            Sublattice_Representation<Integer> ExtRaysLattice(Gens, false, false);  // NO SATURATION, no LLL
            Gens = ExtRaysLattice.to_sublattice(Gens);
            SuppHyps = ExtRaysLattice.to_sublattice_dual(SuppHyps);
            Grading = ExtRaysLattice.to_sublattice_dual_no_div(Grading);  // just the restriction, degrees don't change
                                                                          // no need to recompute new degrees
        }
        make_orbits_global();
        d--;
    }

    bool start = true;

    while (!OldFaces.empty()) {
        size_t nr_F = OldFaces.size();
        if (verbose)
            verboseOutput() << "Descent from dim " << d << ", size " << nr_F << endl;

        if (exploit_automorphisms && !start) {
            size_t nr_iso_classes;
            collect_old_faces_in_iso_classes(nr_iso_classes);
            system_size += nr_iso_classes;
        }
        else
            system_size += nr_F;

        start = false;

        bool in_blocks = false;
        if (nr_F > MaxBlocksize)
            in_blocks = true;
        if (in_blocks && verbose)
            verboseOutput() << "processing in blocks" << endl;

        size_t nr_remaining = nr_F;

        size_t nr_block = 0;

        while (nr_remaining > 0) {
            nr_block++;

            size_t block_size = min((long)MaxBlocksize, (long)nr_remaining);

            auto F = OldFaces.begin();

            size_t kkpos = 0;
            bool skip_remaining = false;

            const long VERBOSE_STEPS = 50;
            long step_x_size = block_size - VERBOSE_STEPS;
            size_t total = block_size;

            if (in_blocks && verbose)
                verboseOutput() << nr_block << ": " << flush;

            vector<key_t> mother_key;
            mother_key.reserve(nr_gens);
            vector<key_t> opposite_facets;
            opposite_facets.reserve(nr_supphyps);
            list<pair<dynamic_bitset, DescentFace<Integer> > > Children;

            std::exception_ptr tmp_exception;
#pragma omp parallel for firstprivate(kkpos, F, mother_key, opposite_facets, Children) schedule(dynamic) if (block_size > 1)
            for (size_t kk = 0; kk < block_size; ++kk) {
                if (skip_remaining)
                    continue;

                if (verbose && block_size >= ReportBound) {
#pragma omp critical(VERBOSE)
                    while ((long)(kk * VERBOSE_STEPS) >= step_x_size) {
                        step_x_size += total;
                        verboseOutput() << "." << flush;
                    }
                }

                try {
                    INTERRUPT_COMPUTATION_BY_EXCEPTION

                    for (; kk > kkpos; kkpos++, F++)
                        ;
                    for (; kk < kkpos; kkpos--, F--)
                        ;
#pragma omp atomic
                    descent_steps++;

                    if (F->second.dead)
                        continue;

                    // cout << "FIRST" << F->first << endl;

                    F->second.compute(*this, d, F->first, mother_key, opposite_facets, Children);
                    // if (F->second.simplicial)
                    //    continue;

                    size_t j = 0;
                    for (auto& G : Children) {
                        auto H = NewFaces.begin();
                        bool inserted = false;
#pragma omp critical(INSERT)
                        {
                            H = NewFaces.find(G.first);
                            if (H == NewFaces.end()) {
                                H = NewFaces.insert(NewFaces.begin(), G);
                                inserted = true;
                            }
                        }
                        if (inserted) {
                            for (unsigned int& i : mother_key)
                                if (SuppHypInd[opposite_facets[j]][i])
#pragma omp atomic
                                    NewNrFacetsContainingGen[i]++;
                        }
#pragma omp critical(ADD_COEFF)
                        {
                            if (!inserted)
                                (H->second).coeff += G.second.coeff;
                            (H->second).tree_size += (F->second).tree_size;
                        }
                        ++j;
                    }

                } catch (const std::exception&) {
                    tmp_exception = std::current_exception();
                    skip_remaining = true;
#pragma omp flush(skip_remaining)
                }
            }  // parallel for kk

            if (!(tmp_exception == 0))
                std::rethrow_exception(tmp_exception);

            if (verbose && block_size >= ReportBound)
                verboseOutput() << endl;

            for (size_t i = 0; i < block_size; ++i)
                OldFaces.erase(OldFaces.begin());

            nr_remaining -= block_size;

        }  // while nr_remaining >0

        OldFaces.swap(NewFaces);
        NewFaces.clear();

        OldNrFacetsContainingGen.swap(NewNrFacetsContainingGen);
        for (size_t i = 0; i < nr_gens; ++i)
            NewNrFacetsContainingGen[i] = 0;

        d--;

    }  // while

    multiplicity *= convertTo<mpz_class>(global_corr_factor);

    if (verbose) {
        if (global_corr_factor != 1)
            verboseOutput() << "Global correction factor used " << global_corr_factor << endl;
        verboseOutput() << "Mult (before NoGradingDenom correction) " << multiplicity << endl;
        verboseOutput() << "Mult (float) " << std::setprecision(12) << mpq_to_nmz_float(multiplicity) << endl;
        verboseOutput() << "Determinants computed " << nr_simplicial << endl;
        verboseOutput() << "Number of descent steps " << descent_steps << endl;
        verboseOutput() << "Number of ";
        if (exploit_automorphisms)
            verboseOutput() << "isomorphism classes of ";
        verboseOutput() << "faces in descent system " << system_size << endl;
        if (!exploit_automorphisms) {
            verboseOutput() << "Full tree size (modulo 2^64)" << tree_size << endl;
        }
    }
}

template <typename Integer>
bool DescentSystem<Integer>::set_verbose(bool onoff) {
    bool old_verbose = verbose;
    verbose = onoff;
    return old_verbose;
}

template <typename Integer>
void DescentSystem<Integer>::setExploitAutoms(bool exploit) {
    exploit_automorphisms = exploit;
}

template <typename Integer>
void DescentSystem<Integer>::setStrictIsoTypeCheck(bool check) {
#ifdef NMZ_HASHLIBRARY
    strict_type_check = check;
#else
    assert(strict_type_check);
    if (!check)
        if (verbose)
            verboseOutput() << "Attempt to disable StrictIsoTypeCheck without Hashing-Library; "
                               "leaving it enabled."
                            << endl;
#endif
}

template <typename Integer>
mpq_class DescentSystem<Integer>::getMultiplicity() {
    return multiplicity;
}

template class DescentFace<long>;
template class DescentFace<long long>;
template class DescentFace<mpz_class>;

template class DescentSystem<long>;
template class DescentSystem<long long>;
template class DescentSystem<mpz_class>;

}  // namespace libnormaliz
