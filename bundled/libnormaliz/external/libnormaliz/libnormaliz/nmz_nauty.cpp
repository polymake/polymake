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

#ifdef NMZ_NAUTYNAUTY
#include <nauty/nauty.h>
#else
#include <nauty.h>
#endif

namespace libnormaliz {

void kill_nauty() {
    nauty_kill_request = 1;
}

extern vector<vector<vector<long> > > CollectedAutoms;

void getmyautoms(int count, int* perm, int* orbits, int numorbits, int stabvertex, int n) {
    int i;

    int tn = 0;
    if (omp_in_parallel())
        tn = omp_get_ancestor_thread_num(omp_get_level());
    vector<long> this_perm(n);
    for (i = 0; i < n; ++i)
        this_perm[i] = perm[i];
    CollectedAutoms[tn].push_back(this_perm);
}

/* The computation of automorphism groups and isomorphism types uses nauty.
 * We start from a matrix that defines our polyhedron up to isomorphism:
 * two such matrices have the same isomorphism type if they differ only by
 * a permutation of the rows followed by a permutation of the colimns.
 *
 * What matrixis taken, depends on the type of automorphism group (or isomorphism
 * classes) that are computed.
 *
 * The given matrices are transformed as follows: we replace the tru eentries by
 * indices in a vector listing the values of the entries. In this way the pattern
 * of equality is preserved.
 *
 * From this matrix of indices we produce a BinaryMatrix (= layer5s of 0-1-matrices
 * representing the indices vertically) that can be directly transformed into a graph
 * whose automorphuism group is then computed by nauty. (For isomorphism types we just
 * need the canonical type.). See the nauty manual for this trick.
 *
 * Taking the entries of the matrix themselves instead of their indices in the value vector
 * can create a problem: they can by very large (especially in makeMMFromGensOnly(..)) and this
 * slows down nauty considerably. By taking the indices we keep the BinaryMatrix as small
 * as possible.
 *
 * There is one crucial point for isomorphism classes. See the comment in makeMM(...): we must take
 * care that the canonical type does not depend on the order in which the values in our matrix
 * are produced (or procesed).
 *
 */

template <typename Integer>
void makeMM_euclidean(BinaryMatrix<Integer>& MM, const Matrix<Integer>& Generators, const Matrix<Integer>& SpecialLinForms) {
    key_t i, j;
    size_t mm = Generators.nr_of_rows();
    size_t nn = mm + SpecialLinForms.nr_of_rows();
    Matrix<long> MVal(mm, nn);

    long new_val = 0;
    Integer val;
    std::map<Integer, long> Values;
    vector<Integer> VV;
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
                VV.push_back(val);
            }
        }
    }

    // for the following see the comment in makeMM

    sort(VV.begin(), VV.end());
    vector<long> new_index(VV.size());

    for (size_t j = 0; j < VV.size(); ++j) {
        long old_index = Values[VV[j]];
        new_index[old_index] = j;
    }

    for (i = 0; i < mm; ++i) {
        for (j = 0; j < nn; ++j) {
            MM.insert(new_index[MVal[i][j]], i, j);
        }
    }

    MM.set_values(VV);
}

template <typename Integer>
void makeMM(BinaryMatrix<Integer>& MM,
            const Matrix<Integer>& Generators,
            const Matrix<Integer>& LinForms,
            AutomParam::Quality quality) {
    /* The matrix  determining the automorphism group (or isomorphism class)
     * is given by the scalar products of the generators and the linear forms.
     *
     * For the combinatorial automorph group we replace the non-zero values of the scalar products by 1. This gives
     * thwe 0-1 complement of the inciodence matrix -- does not matter.
     */

    key_t i, j;
    size_t mm = Generators.nr_of_rows();
    size_t nn = LinForms.nr_of_rows();
    Matrix<long> MVal(mm, nn);

    bool zero_one = false;
    if (quality == AutomParam::combinatorial)
        zero_one = true;

    long new_val = 0;
    Integer val;
    std::map<Integer, long> Values;
    vector<Integer> VV;

    for (i = 0; i < mm; ++i) {
        INTERRUPT_COMPUTATION_BY_EXCEPTION

        for (j = 0; j < nn; ++j) {
            val = v_scalar_product(Generators[i], LinForms[j]);
            // cout << "SSSS " << val << endl;
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
                VV.push_back(val);
            }
        }
    }

    // At this point the order of the values stored in VV depends on the order in
    // which they are computed. This is no problem in the computatio of automorphism groups,
    // but for isomorphism types we must make sure that two matrices Val that differ
    // only by row and column transformations produce binary matrices MVal that again differ only
    // by such permutations. Therefore we must order the values and replace the entries of MVal
    // accordingly: the smallest entry of Val is represented by 0 in MVal etc.

    sort(VV.begin(), VV.end());
    vector<long> new_index(VV.size());

    for (size_t j = 0; j < VV.size(); ++j) {
        long old_index = Values[VV[j]];
        new_index[old_index] = j;
    }

    for (i = 0; i < mm; ++i) {
        for (j = 0; j < nn; ++j) {
            MM.insert(new_index[MVal[i][j]], i, j);
            // cout << "MM " << i << " " << j << " " << MVal[i][j] << endl;
        }
    }

    MM.set_values(VV);
}

template <typename Integer>
void makeMMFromGensOnly_inner(BinaryMatrix<Integer>& MM,
                              const Matrix<Integer>& Generators,
                              const Matrix<Integer>& SpecialLinForms,
                              AutomParam::Quality quality) {
    /* Here we use only generators, following
     *
     * D. Bremner , M. D. Sikiri\'c , D. V. Pasechnik, Th. Rehn and A. Schürmann,
          \emph{Computing symmetry groups of polyhedra.}
        LMS J. Comp. Math. 17 (2014), 565--581.
     *
     * In the euclidean case (branched off makeMMFromGensOnly(...)) , we must preserve the norms of the difference vectors
     * of the vertices of the polytope.
     *
     */

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
void makeMMFromGensOnly(BinaryMatrix<Integer>& MM,
                        const Matrix<Integer>& Generators,
                        const Matrix<Integer>& SpecialLinForms,
                        AutomParam::Quality quality) {
    if (quality == AutomParam::euclidean) {
        makeMMFromGensOnly_inner(MM, Generators, SpecialLinForms, quality);
        return;
    }

    Matrix<mpz_class> Generators_mpz;       // we go through mpz_class since taking inverse matrices
    convert(Generators_mpz, Generators);    // is extremely critical, and we don't want to risk
    Matrix<mpz_class> SpecialLinForms_mpz;  // an overflow exception at this point
    convert(SpecialLinForms_mpz, SpecialLinForms);
    BinaryMatrix<mpz_class> MM_mpz(MM.get_nr_rows(), MM.get_nr_columns());
    makeMMFromGensOnly_inner(MM_mpz, Generators_mpz, SpecialLinForms_mpz, quality);
    MM.get_data_mpz(MM_mpz);
}

template <>
void makeMMFromGensOnly(BinaryMatrix<renf_elem_class>& MM,
                        const Matrix<renf_elem_class>& Generators,
                        const Matrix<renf_elem_class>& SpecialLinForms,
                        AutomParam::Quality quality) {
    makeMMFromGensOnly_inner(MM, Generators, SpecialLinForms, quality);
}

// This routine starts from generators x and linear forms f. They define a rectangular
// matrix with entries f(x), with x corresponding to a row and f to a column
// This rectangular matrix is then interpreted as the weight pattern on a complete
// bipartite graph.
// Via a binary matrix the weights are tranlated into a grpah with "layers"
// where each layer corresponds to a place in the binary expansion of the entries.
//
// But the function can be used also for 0-1-matrices where the entries
// of the rectangular matrix are somplified to 0 or 1.
template <typename Integer>
nauty_result<Integer> compute_automs_by_nauty_Gens_LF(const Matrix<Integer>& Generators,
                                                      size_t nr_special_gens,
                                                      const Matrix<Integer>& LinForms,
                                                      const size_t nr_special_linforms,
                                                      AutomParam::Quality quality) {
    int tn = 0;
    if (omp_in_parallel())
        tn = omp_get_ancestor_thread_num(omp_get_level());
    CollectedAutoms[tn].clear();

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

    BinaryMatrix<Integer> MM(mm, nn);
    makeMM(MM, Generators, LinForms, quality);

    size_t ll = MM.get_nr_layers();

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

    // vector<vector<long> > AutomsAndOrbits(2*CollectedAutoms[tn].size());
    // AutomsAndOrbits.reserve(2*CollectedAutoms[tn].size()+3);

    nauty_result<Integer> result;

    for (k = 0; k < CollectedAutoms[tn].size(); ++k) {
        vector<key_t> GenPerm(mm_pure);
        for (i = 0; i < mm_pure; ++i)
            GenPerm[i] = CollectedAutoms[tn][k][i];
        result.GenPerms.push_back(GenPerm);
        vector<key_t> LFPerm(nn_pure);  // we remove the special linear forms here
        for (i = mm; i < mm + nn_pure; ++i)
            LFPerm[i - mm] = CollectedAutoms[tn][k][i] - mm;
        result.LinFormPerms.push_back(LFPerm);
    }

    vector<key_t> GenOrbits(mm_pure);
    for (i = 0; i < mm_pure; ++i)
        GenOrbits[i] = orbits[i];
    result.GenOrbits = GenOrbits;

    vector<key_t> LFOrbits(nn_pure);  // we remove the special linear forms here
    for (i = 0; i < nn_pure; ++i)
        LFOrbits[i] = orbits[i + mm] - mm;
    result.LinFormOrbits = LFOrbits;

    result.order = mpz_class(stats.grpsize1);

    if (stats.grpsize2 != 0) {
        mpz_class power_mpz = mpz_class(stats.grpsize2);
        long power = convertToLong(power_mpz);
        for (long i = 0; i < power; ++i)
            result.order *= 10;
    }

    vector<key_t> row_order(mm), col_order(nn);  // the special gens and linforms go into
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

// The following routine uses only "generators" x and special linear forms f
// Together they correspond to the vertices of a graph.
// The weights on the graph come from a SYMMETRIC matrix of "values" form
// where each entry corresponds to val(x,y) = val(y,x).
// The numbers f(x) are attached as an extra column.
// It would be possible to apply the precerding function to this situation,
// but the graph is more compact here.
// The layers of the binary matrix have the same meaning as above.
template <typename Integer>
nauty_result<Integer> compute_automs_by_nauty_FromGensOnly(const Matrix<Integer>& Generators,
                                                           size_t nr_special_gens,
                                                           const Matrix<Integer>& SpecialLinForms,
                                                           AutomParam::Quality quality) {
    size_t mm = Generators.nr_of_rows();
    size_t mm_pure = mm - nr_special_gens;

    size_t nr_special_linforms = SpecialLinForms.nr_of_rows();

    /*cout << "--------------------" << endl;
    Generators.pretty_print(cout);
    cout << "--------------------" << endl;
    SpecialLinForms.pretty_print(cout);
    cout << "--------------------" << endl;*/

    // LinForms.append(SpecialLinForms);

    int tn = 0;
    if (omp_in_parallel())
        tn = omp_get_ancestor_thread_num(omp_get_level());

    CollectedAutoms[tn].clear();

    static DEFAULTOPTIONS_GRAPH(options);
    statsblk stats;

    options.userautomproc = getmyautoms;
    options.getcanon = TRUE;

    int n, m;

    options.writeautoms = FALSE;
    options.defaultptn = FALSE;

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    BinaryMatrix<Integer> MM(mm, mm + nr_special_linforms);
    makeMMFromGensOnly(MM, Generators, SpecialLinForms, quality);

    size_t ll = MM.get_nr_layers();

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
        for (j = 0; j <= i; ++j) {  // take lower triangular matrix inclcudung diagonal
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

    /*cout << "+++++++++++++" << endl;
    cout << ptn;
    cout << "+++++++++++++" << endl;*/

    INTERRUPT_COMPUTATION_BY_EXCEPTION

    densenauty(g.data(), lab.data(), ptn.data(), orbits.data(), &options, &stats, m, n, cg.data());
    if (stats.errstatus == NAUKILLED) {
        INTERRUPT_COMPUTATION_BY_EXCEPTION
    }

    nauty_result<Integer> result;

    for (k = 0; k < CollectedAutoms[tn].size(); ++k) {
        vector<key_t> GenPerm(mm_pure);
        for (i = 0; i < mm_pure; ++i)  // remove special gens and lion forms
            GenPerm[i] = CollectedAutoms[tn][k][i];
        result.GenPerms.push_back(GenPerm);
    }

    vector<key_t> GenOrbits(mm_pure);
    for (i = 0; i < mm_pure; ++i)
        GenOrbits[i] = orbits[i];
    result.GenOrbits = GenOrbits;

    result.order = mpz_class(stats.grpsize1);
    if (stats.grpsize2 != 0) {
        mpz_class power_mpz = mpz_class(stats.grpsize2);
        long power = convertToLong(power_mpz);
        for (long i = 0; i < power; ++i)
            result.order *= 10;
    }

    nauty_freedyn();

    // cout << "::::::::::::::" << endl;
    // cout << lab;
    // cout << "::::::::::::::" << endl;

    vector<key_t> row_order;  //
    for (key_t i = 0; i < layer_size; ++i)
        if (lab[i] < (int)mm)  // we suppoess the clumn of the special linear form
            row_order.push_back(lab[i]);

    /*vector<key_t> col_order(layer_size); // this includes the column of the special linear form
    for(size_t i=0; i< col_order.size();++i)
        col_order[i] = lab[i+mm] - mm; */

    vector<key_t> col_order = row_order;
    col_order.resize(layer_size);  // this includes the column of the special linear form
    for (size_t i = mm; i < col_order.size(); ++i)
        col_order[i] = i;

    result.CanLabellingGens = row_order;

    /*cout << "********" << endl;
    cout << row_order;
    cout << endl;
    cout << col_order;
    cout << "=======" << endl;*/

    // MM.pretty_print(cout);
    // cout << "--------" << endl;

    result.CanType = MM.reordered(row_order, col_order);

    // result.CanType.pretty_print(cout);

    // cout << "ORDER " << result.order << endl;

    return result;
}

#ifndef NMZ_MIC_OFFLOAD  // offload with long is not supported
template nauty_result<long> compute_automs_by_nauty_Gens_LF(const Matrix<long>& Generators,
                                                            size_t nr_special_gens,
                                                            const Matrix<long>& LinForms,
                                                            const size_t nr_special_linforms,
                                                            AutomParam::Quality quality);

template nauty_result<long> compute_automs_by_nauty_FromGensOnly(const Matrix<long>& Generators,
                                                                 size_t nr_special_gens,
                                                                 const Matrix<long>& SpecialLinForms,
                                                                 AutomParam::Quality quality);
#endif  // NMZ_MIC_OFFLOAD
template nauty_result<long long> compute_automs_by_nauty_Gens_LF(const Matrix<long long>& Generators,
                                                                 size_t nr_special_gens,
                                                                 const Matrix<long long>& LinForms,
                                                                 const size_t nr_special_linforms,
                                                                 AutomParam::Quality quality);

template nauty_result<long long> compute_automs_by_nauty_FromGensOnly(const Matrix<long long>& Generators,
                                                                      size_t nr_special_gens,
                                                                      const Matrix<long long>& SpecialLinForms,
                                                                      AutomParam::Quality quality);

template nauty_result<mpz_class> compute_automs_by_nauty_Gens_LF(const Matrix<mpz_class>& Generators,
                                                                 size_t nr_special_gens,
                                                                 const Matrix<mpz_class>& LinForms,
                                                                 const size_t nr_special_linforms,
                                                                 AutomParam::Quality quality);

template nauty_result<mpz_class> compute_automs_by_nauty_FromGensOnly(const Matrix<mpz_class>& Generators,
                                                                      size_t nr_special_gens,
                                                                      const Matrix<mpz_class>& SpecialLinForms,
                                                                      AutomParam::Quality quality);
#ifdef ENFNORMALIZ
template nauty_result<renf_elem_class> compute_automs_by_nauty_Gens_LF(const Matrix<renf_elem_class>& Generators,
                                                                       size_t nr_special_gens,
                                                                       const Matrix<renf_elem_class>& LinForms,
                                                                       const size_t nr_special_linforms,
                                                                       AutomParam::Quality quality);

template nauty_result<renf_elem_class> compute_automs_by_nauty_FromGensOnly(const Matrix<renf_elem_class>& Generators,
                                                                            size_t nr_special_gens,
                                                                            const Matrix<renf_elem_class>& SpecialLinForms,
                                                                            AutomParam::Quality quality);
#endif

}  // namespace libnormaliz

#endif  // NMZ_NAUTY
