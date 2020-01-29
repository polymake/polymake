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

#include <map>

#include "libnormaliz/integer.h"
#include "libnormaliz/matrix.h"
#include "libnormaliz/nmz_nauty.h"
#include "libnormaliz/normaliz_exception.h"
#include "libnormaliz/vector_operations.h"
#include "libnormaliz/dynamic_bitset.h"

#ifdef NMZ_NAUTY

// #define MAXN 5000    /* Define this before including nauty.h */
// we use dynamic allocation

extern "C" {
extern volatile int nauty_kill_request;
}

#include <nauty/nauty.h>

namespace libnormaliz {
using namespace std;

void kill_nauty() {
    nauty_kill_request = 1;
}

vector<vector<long> > CollectedAutoms;

void getmyautoms(int count, int* perm, int* orbits, int numorbits, int stabvertex, int n) {
    int i;
    vector<long> this_perm(n);
    for (i = 0; i < n; ++i)
        this_perm[i] = perm[i];
    CollectedAutoms.push_back(this_perm);
}

template <typename Integer>
void makeMM_euclidean(BinaryMatrix& MM, const Matrix<Integer>& Generators, const Matrix<Integer>& SpecialLinForms) {
    key_t i, j;
    size_t mm = Generators.nr_of_rows();
    size_t nn = mm + SpecialLinForms.nr_of_rows();
    Matrix<long> MVal(mm, nn);

    long new_val = 0;
    Integer val;
    map<Integer, long> Values;
    for (i = 0; i < mm; ++i) {
        vector<Integer> minus = Generators[i];
        Integer MinusOne = -1;
        v_scalar_multiplication(minus, MinusOne);

        INTERRUPT_COMPUTATION_BY_EXCEPTION

        for (j = 0; j < nn; ++j) {
            if (j < mm) {
                vector<Integer> diff = v_add(minus, Generators[j]);
                val = v_scalar_product(diff, diff);
            }
            else {
                val = v_scalar_product(Generators[i], SpecialLinForms[j - mm]);
            }
            auto v = Values.find(val);
            if (v != Values.end()) {
                MVal[i][j] = v->second;
            }
            else {
                Values[val] = new_val;
                MVal[i][j] = new_val;
                new_val++;
            }
        }
    }

    MM.set_offset((long)0);
    for (i = 0; i < mm; ++i) {
        for (j = 0; j < nn; ++j)
            MM.insert(MVal[i][j], i, j);
    }
}

template <typename Integer>
void makeMM(BinaryMatrix& MM, const Matrix<Integer>& Generators, const Matrix<Integer>& LinForms, AutomParam::Quality quality) {
    key_t i, j;
    size_t mm = Generators.nr_of_rows();
    size_t nn = LinForms.nr_of_rows();
    Matrix<long> MVal(mm, nn);

    bool zero_one = false;
    if (quality == AutomParam::combinatorial)
        zero_one = true;

    long new_val = 0;
    Integer val;
    map<Integer, long> Values;
    for (i = 0; i < mm; ++i) {
        INTERRUPT_COMPUTATION_BY_EXCEPTION

        for (j = 0; j < nn; ++j) {
            val = v_scalar_product(Generators[i], LinForms[j]);
            if (zero_one && val != 0)
                val = 1;
            auto v = Values.find(val);
            if (v != Values.end()) {
                MVal[i][j] = v->second;
            }
            else {
                Values[val] = new_val;
                MVal[i][j] = new_val;
                new_val++;
            }
        }
    }

    MM.set_offset((long)0);
    for (i = 0; i < mm; ++i) {
        for (j = 0; j < nn; ++j)
            MM.insert(MVal[i][j], i, j);
    }
}

template <typename Integer>
void makeMMFromGensOnly_inner(BinaryMatrix& MM,
                              const Matrix<Integer>& Generators,
                              const Matrix<Integer>& SpecialLinForms,
                              AutomParam::Quality quality) {
    if (quality == AutomParam::euclidean) {
        makeMM_euclidean(MM, Generators, SpecialLinForms);
        return;
    }

    size_t mm = Generators.nr_of_rows();
    size_t dim = Generators.nr_of_columns();

    Matrix<Integer> ScalarProd(dim, dim);

    for (size_t i = 0; i < mm; ++i) {
        for (size_t j = 0; j < dim; ++j) {
            for (size_t k = 0; k < dim; ++k) {
                ScalarProd[j][k] += Generators[i][j] * Generators[i][k];
            }
        }
    }

    Integer dummy;
    Matrix<Integer> SPInv = ScalarProd.invert(dummy);
    Matrix<Integer> LinForms = Generators.multiplication(SPInv);
    LinForms.append(SpecialLinForms);

    makeMM(MM, Generators, LinForms, quality);
}

template <typename Integer>
void makeMMFromGensOnly(BinaryMatrix& MM,
                        const Matrix<Integer>& Generators,
                        const Matrix<Integer>& SpecialLinForms,
                        AutomParam::Quality quality) {
    if (quality == AutomParam::euclidean) {
        makeMMFromGensOnly_inner(MM, Generators, SpecialLinForms, quality);
        return;
    }

    Matrix<mpz_class> Generators_mpz;     // we go through mpz_class since taking inverse matrices
    convert(Generators_mpz, Generators);  // is extremely critical
    Matrix<mpz_class> SpecialLinForms_mpz;
    convert(SpecialLinForms_mpz, SpecialLinForms);
    makeMMFromGensOnly_inner(MM, Generators_mpz, SpecialLinForms_mpz, quality);
}

#ifdef ENFNORMALIZ
template <>
void makeMMFromGensOnly(BinaryMatrix& MM,
                        const Matrix<renf_elem_class>& Generators,
                        const Matrix<renf_elem_class>& SpecialLinForms,
                        AutomParam::Quality quality) {
    makeMMFromGensOnly_inner(MM, Generators, SpecialLinForms, quality);
}
#endif

template <typename Integer>
nauty_result compute_automs_by_nauty_Gens_LF(const Matrix<Integer>& Generators,
                                             size_t nr_special_gens,
                                             const Matrix<Integer>& LinForms,
                                             const size_t nr_special_linforms,
                                             AutomParam::Quality quality) {
    CollectedAutoms.clear();

    static DEFAULTOPTIONS_GRAPH(options);
    statsblk stats;

    options.userautomproc = getmyautoms;
    options.getcanon = TRUE;

    int n, m;

    options.writeautoms = FALSE;
    options.defaultptn = FALSE;

    size_t mm = Generators.nr_of_rows();
    size_t mm_pure = mm - nr_special_gens;
    size_t nn = LinForms.nr_of_rows();
    size_t nn_pure = nn - nr_special_linforms;

    BinaryMatrix MM(mm, nn);
    makeMM(MM, Generators, LinForms, quality);

    size_t ll = MM.nr_layers();

    size_t layer_size = mm + nn;
    n = ll * layer_size;
    m = SETWORDSNEEDED(n);

    nauty_check(WORDSIZE, m, n, NAUTYVERSIONID);

    std::vector<graph> g(m * n);
    std::vector<graph> cg(m * n);
    std::vector<int> lab(n);
    std::vector<int> ptn(n);
    std::vector<int> orbits(n);

    EMPTYGRAPH(g.data(), m, n);

    key_t i, j, k;

    for (i = 0; i < layer_size; ++i) {  // make vertical edges over all layers
        for (k = 1; k < ll; ++k)
            ADDONEEDGE(g.data(), (k - 1) * layer_size + i, k * layer_size + i, m);
    }

    for (i = 0; i < mm; ++i) {  // make horizontal edges layer by layer
        for (j = 0; j < nn; ++j) {
            for (k = 0; k < ll; ++k) {
                if (MM.test(i, j, k))  // k is the number of layers below the current one
                    ADDONEEDGE(g.data(), k * layer_size + i, k * layer_size + mm + j, m);
            }
        }
    }

    for (int ii = 0; ii < n; ++ii) {  // prepare labelling and partitions
        lab[ii] = ii;
        ptn[ii] = 1;
    }

    for (k = 0; k < ll; ++k) {                        // make partitions layer by layer
        ptn[k * layer_size + mm_pure - 1] = 0;        // row vertices in one partition
        for (size_t s = 0; s < nr_special_gens; ++s)  // speciall generators in extra partitions (makes them fixed points)
            ptn[k * layer_size + mm_pure + s] = 0;
        ptn[(k + 1) * layer_size - 1] = 0;                // column indices in the next
        for (size_t s = 0; s < nr_special_linforms; ++s)  // special linear forms in extra partitions
            ptn[(k + 1) * layer_size - 2 - s] = 0;
    }

    densenauty(g.data(), lab.data(), ptn.data(), orbits.data(), &options, &stats, m, n, cg.data());
    if (stats.errstatus == NAUKILLED) {
        INTERRUPT_COMPUTATION_BY_EXCEPTION
    }

    // vector<vector<long> > AutomsAndOrbits(2*CollectedAutoms.size());
    // AutomsAndOrbits.reserve(2*CollectedAutoms.size()+3);

    nauty_result result;

    for (k = 0; k < CollectedAutoms.size(); ++k) {
        vector<key_t> GenPerm(mm_pure);
        for (i = 0; i < mm_pure; ++i)
            GenPerm[i] = CollectedAutoms[k][i];
        result.GenPerms.push_back(GenPerm);
        vector<key_t> LFPerm(nn_pure);  // we remove the special linear forms here
        for (i = mm; i < mm + nn_pure; ++i)
            LFPerm[i - mm] = CollectedAutoms[k][i] - mm;
        result.LinFormPerms.push_back(LFPerm);
    }

    vector<key_t> GenOrbits(mm);
    for (i = 0; i < mm_pure; ++i)
        GenOrbits[i] = orbits[i];
    result.GenOrbits = GenOrbits;

    vector<key_t> LFOrbits(nn_pure);  // we remove the special linear forms here
    for (i = 0; i < nn_pure; ++i)
        LFOrbits[i] = orbits[i + mm] - mm;
    result.LinFormOrbits = LFOrbits;

    result.order = mpz_class(stats.grpsize1);

    vector<key_t> row_order(mm), col_order(nn);  // the spßecial gens and linforms go into
    for (key_t i = 0; i < mm; ++i)               // these data
        row_order[i] = lab[i];
    for (key_t i = 0; i < nn; ++i)
        col_order[i] = lab[mm + i] - mm;

    result.CanLabellingGens = row_order;

    result.CanType = MM.reordered(row_order, col_order);

    nauty_freedyn();

    return result;
}

//====================================================================

template <typename Integer>
nauty_result compute_automs_by_nauty_FromGensOnly(const Matrix<Integer>& Generators,
                                                  size_t nr_special_gens,
                                                  const Matrix<Integer>& SpecialLinForms,
                                                  AutomParam::Quality quality) {
    size_t mm = Generators.nr_of_rows();
    size_t mm_pure = mm - nr_special_gens;

    size_t nr_special_linforms = SpecialLinForms.nr_of_rows();

    // LinForms.append(SpecialLinForms);

    CollectedAutoms.clear();

    static DEFAULTOPTIONS_GRAPH(options);
    statsblk stats;

    options.userautomproc = getmyautoms;
    options.getcanon = TRUE;

    int n, m;

    options.writeautoms = FALSE;
    options.defaultptn = FALSE;

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    BinaryMatrix MM(mm, mm + nr_special_linforms);
    makeMMFromGensOnly(MM, Generators, SpecialLinForms, quality);

    size_t ll = MM.nr_layers();

    size_t layer_size = mm + nr_special_linforms;
    n = ll * layer_size;  // total number of vertices
    m = SETWORDSNEEDED(n);

    nauty_check(WORDSIZE, m, n, NAUTYVERSIONID);

    std::vector<graph> g(m * n);
    std::vector<graph> cg(m * n);
    std::vector<int> lab(n);
    std::vector<int> ptn(n);
    std::vector<int> orbits(n);

    EMPTYGRAPH(g.data(), m, n);

    key_t i, j, k;

    for (i = 0; i < layer_size; ++i) {  // make vertical edges over all layers
        for (k = 1; k < ll; ++k)
            ADDONEEDGE(g.data(), (k - 1) * layer_size + i, k * layer_size + i, m);
    }

    for (i = 0; i < mm; ++i) {      // make horizontal edges layer by layer
        for (j = 0; j <= i; ++j) {  // take lower triangularr matrix inclcudung diagonal
            for (k = 0; k < ll; ++k) {
                if (MM.test(i, j, k))  // k is the number of layers below the current one
                    ADDONEEDGE(g.data(), k * layer_size + i, k * layer_size + j, m);
            }
        }
    }

    // we add the edges that connect generators and special linear forms
    for (i = mm; i < mm + nr_special_linforms; ++i) {
        for (j = 0; j < mm; ++j) {
            for (k = 0; k < ll; ++k) {
                if (MM.test(j, i, k)) {  // here we use that the special linear forms appear in columns: i <--> j
                    ADDONEEDGE(g.data(), k * layer_size + i, k * layer_size + j, m);
                }
            }
        }
    }

    for (int ii = 0; ii < n; ++ii) {  // prepare partitions
        lab[ii] = ii;                 // label of vertex
        ptn[ii] = 1;                  // indicatorvector for partitions: 0 indicates end of partition
    }

    for (k = 0; k < ll; ++k) {                        // make partitions layer by layer
        ptn[k * layer_size + mm_pure - 1] = 0;        // row vertices in one partition
        for (size_t s = 0; s < nr_special_gens; ++s)  // speciall generators in extra partitions (makes them fixed points)
            ptn[k * layer_size + mm_pure + s] = 0;
        for (size_t s = 0; s < nr_special_linforms; ++s)  // special linear forms in extra partitions
            ptn[(k + 1) * layer_size - 1 - s] = 0;
    }

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    densenauty(g.data(), lab.data(), ptn.data(), orbits.data(), &options, &stats, m, n, cg.data());
    if (stats.errstatus == NAUKILLED) {
        INTERRUPT_COMPUTATION_BY_EXCEPTION
    }

    nauty_result result;

    for (k = 0; k < CollectedAutoms.size(); ++k) {
        vector<key_t> GenPerm(mm);
        for (i = 0; i < mm_pure; ++i)  // remove special gens and lion forms
            GenPerm[i] = CollectedAutoms[k][i];
        result.GenPerms.push_back(GenPerm);
    }

    vector<key_t> GenOrbits(mm);
    for (i = 0; i < mm_pure; ++i)
        GenOrbits[i] = orbits[i];  // remove special lin forms
    result.GenOrbits = GenOrbits;

    result.order = mpz_class(stats.grpsize1);

    vector<key_t> row_order(mm);
    for (key_t i = 0; i < mm; ++i)
        row_order[i] = lab[i];

    result.CanLabellingGens = row_order;

    nauty_freedyn();

    // CanType=MM.reordered(row_order,col_order);

    // cout << "ORDER " << result.order << endl;

    return result;
}

#ifndef NMZ_MIC_OFFLOAD  // offload with long is not supported
template nauty_result compute_automs_by_nauty_Gens_LF(const Matrix<long>& Generators,
                                                      size_t nr_special_gens,
                                                      const Matrix<long>& LinForms,
                                                      const size_t nr_special_linforms,
                                                      AutomParam::Quality quality);

template nauty_result compute_automs_by_nauty_FromGensOnly(const Matrix<long>& Generators,
                                                           size_t nr_special_gens,
                                                           const Matrix<long>& SpecialLinForms,
                                                           AutomParam::Quality quality);
#endif  // NMZ_MIC_OFFLOAD
template nauty_result compute_automs_by_nauty_Gens_LF(const Matrix<long long>& Generators,
                                                      size_t nr_special_gens,
                                                      const Matrix<long long>& LinForms,
                                                      const size_t nr_special_linforms,
                                                      AutomParam::Quality quality);

template nauty_result compute_automs_by_nauty_FromGensOnly(const Matrix<long long>& Generators,
                                                           size_t nr_special_gens,
                                                           const Matrix<long long>& SpecialLinForms,
                                                           AutomParam::Quality quality);

template nauty_result compute_automs_by_nauty_Gens_LF(const Matrix<mpz_class>& Generators,
                                                      size_t nr_special_gens,
                                                      const Matrix<mpz_class>& LinForms,
                                                      const size_t nr_special_linforms,
                                                      AutomParam::Quality quality);

template nauty_result compute_automs_by_nauty_FromGensOnly(const Matrix<mpz_class>& Generators,
                                                           size_t nr_special_gens,
                                                           const Matrix<mpz_class>& SpecialLinForms,
                                                           AutomParam::Quality quality);
#ifdef ENFNORMALIZ
template nauty_result compute_automs_by_nauty_Gens_LF(const Matrix<renf_elem_class>& Generators,
                                                      size_t nr_special_gens,
                                                      const Matrix<renf_elem_class>& LinForms,
                                                      const size_t nr_special_linforms,
                                                      AutomParam::Quality quality);

template nauty_result compute_automs_by_nauty_FromGensOnly(const Matrix<renf_elem_class>& Generators,
                                                           size_t nr_special_gens,
                                                           const Matrix<renf_elem_class>& SpecialLinForms,
                                                           AutomParam::Quality quality);
#endif

}  // namespace libnormaliz

#endif  // NMZ_NAUTY
