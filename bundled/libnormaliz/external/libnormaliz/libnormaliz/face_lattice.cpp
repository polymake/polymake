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

#include "libnormaliz/cone.h"
#include "libnormaliz/face_lattice.h"
#include "libnormaliz/vector_operations.h"

namespace libnormaliz {

using namespace std;

template <typename Integer>
FaceLattice<Integer>::FaceLattice() {
}

// It is assumed that the matrices in the constructor are for the pointed quotient,
// even if the names of the parameters don't indicate that.

template <typename Integer>
FaceLattice<Integer>::FaceLattice(Matrix<Integer>& SupportHyperplanes,
                                  const Matrix<Integer>& VerticesOfPolyhedron,
                                  const Matrix<Integer>& ExtremeRaysRecCone,
                                  const bool cone_inhomogeneous,
                                  bool swap_allowed) {
    inhomogeneous = cone_inhomogeneous;

    nr_supphyps = SupportHyperplanes.nr_of_rows();
    nr_extr_rec_cone = ExtremeRaysRecCone.nr_of_rows();
    nr_vert = VerticesOfPolyhedron.nr_of_rows();
    nr_gens = nr_extr_rec_cone + nr_vert;

    if (swap_allowed)
        swap(SuppHyps, SupportHyperplanes);
    else
        SuppHyps = SupportHyperplanes;
    dim = SuppHyps[0].size();

    SuppHypInd.clear();
    SuppHypInd.resize(nr_supphyps);

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
                    if (v_scalar_product(SuppHyps[i], VerticesOfPolyhedron[j]) == 0) {
                        nr_gens_in_hyp++;
                        SuppHypInd[i][j] = true;
                    }
                }
            }

            for (size_t j = 0; j < nr_extr_rec_cone; ++j) {
                if (v_scalar_product(SuppHyps[i], ExtremeRaysRecCone[j]) == 0) {
                    nr_gens_in_hyp++;
                    SuppHypInd[i][j + nr_vert] = true;
                }
            }

            if (nr_gens_in_hyp == (int)(dim - 1))
                //#pragma omp atomic
                nr_simplial_facets++;

        } catch (const std::exception&) {
            tmp_exception = std::current_exception();
            skip_remaining = true;
#pragma omp flush(skip_remaining)
        }
    }
    if (!(tmp_exception == 0))
        std::rethrow_exception(tmp_exception);

    // if (verbose)
    //    verboseOutput() << "Simplicial facets " << nr_simplial_facets << " of " << nr_supphyps << endl;
}

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
void FaceLattice<Integer>::compute(const long face_codim_bound, const bool verbose, bool change_integer_type) {
    bool bound_codim = false;
    if (face_codim_bound >= 0)
        bound_codim = true;

    dynamic_bitset SimpleVert(nr_gens);
    size_t nr_simpl = 0;
    for (size_t j = 0; j < nr_gens; ++j) {
        size_t nr_cont = 0;
        for (size_t i = 0; i < nr_supphyps; ++i)
            if (SuppHypInd[i][j])
                nr_cont++;
        if (nr_cont == dim - 1) {
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

    Matrix<MachineInteger> SuppHyps_MI;
    if (change_integer_type)
        convert(SuppHyps_MI, SuppHyps);

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

        bool skip_remaining = false;
        std::exception_ptr tmp_exception;

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
                                    codim_of_face = SuppHyps_MI.submatrix(selection).rank();
                                } catch (const ArithmeticException& e) {
                                    change_integer_type = false;
                                }
                            }
                            if (!change_integer_type)
                                codim_of_face = SuppHyps.submatrix(selection).rank();

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

        // if (ToCompute.test(ConeProperty::FaceLattice))
        for (auto H = WorkFaces.begin(); H != WorkFaces.end(); ++H)
            FaceLat[H->first] = codimension_so_far - 1;
        WorkFaces.clear();
        if (NewFaces.empty())
            break;
        swap(WorkFaces, NewFaces);
    }

    if (inhomogeneous && nr_vert != 1) {  // we want the empty face in the face lattice
                                          // (never the case in homogeneous computations)
        dynamic_bitset NoGens(nr_gens);
        size_t codim_max_subspace = SuppHyps.rank();
        FaceLat[AllFacets] = codim_max_subspace;
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
}

template <typename Integer>
vector<size_t> FaceLattice<Integer>::getFVector() {
    return f_vector;
}

template <typename Integer>
void FaceLattice<Integer>::get(map<dynamic_bitset, int>& FaceLatticeOutput) {
    swap(FaceLat, FaceLatticeOutput);
}

template <typename Integer>
void FaceLattice<Integer>::get(vector<dynamic_bitset>& SuppHypIndOutput) {
    swap(SuppHypInd, SuppHypIndOutput);
}

#ifndef NMZ_MIC_OFFLOAD  // offload with long is not supported
template class FaceLattice<long>;
#endif
template class FaceLattice<long long>;
template class FaceLattice<mpz_class>;

#ifdef ENFNORMALIZ
template class FaceLattice<renf_elem_class>;
#endif

}  // namespace libnormaliz
