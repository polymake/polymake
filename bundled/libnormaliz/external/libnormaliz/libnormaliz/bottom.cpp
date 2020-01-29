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

#ifdef NMZ_MIC_OFFLOAD
#pragma offload_attribute(push, target(mic))
#endif

#include <stdlib.h>
#include <math.h>

#include <iostream>
//#include <sstream>
#include <algorithm>
#include <queue>

#include "libnormaliz/general.h"
#include "libnormaliz/bottom.h"
#include "libnormaliz/vector_operations.h"
#include "libnormaliz/integer.h"
// #include "libnormaliz/full_cone.h"

namespace libnormaliz {
using namespace std;

long SubDivBound = 1000000;

template <typename Integer>
bool bottom_points_inner(Matrix<Integer>& gens,
                         list<vector<Integer> >& local_new_points,
                         vector<Matrix<Integer> >& local_q_gens,
                         size_t& stellar_det_sum);

// kept here for simplicity:

double convert_to_double(mpz_class a) {
    return a.get_d();
}

double convert_to_double(long a) {
    return a;
}

double convert_to_double(long long a) {
    return a;
}

template <typename Integer>
void bottom_points(list<vector<Integer> >& new_points, const Matrix<Integer>& given_gens, Integer VolumeBound) {
    /* gens.pretty_print(cout);
    cout << "=======================" << endl;

    gens.transpose().pretty_print(cout);
    cout << "=======================" << endl;*/

    Matrix<Integer> gens, Trans, Trans_inv;
    // given_gens.LLL_transform_transpose(gens,Trans,Trans_inv);  // now in optimal_subdivision_point()
    gens = given_gens;

    Integer volume;
    // int dim = gens[0].size();
    Matrix<Integer> Support_Hyperplanes = gens.invert(volume);

    vector<Integer> grading;  // = grading_;
    if (grading.empty())
        grading = gens.find_linear_form();
    // cout << grading;

    list<vector<Integer> > bottom_candidates;
    bottom_candidates.splice(bottom_candidates.begin(), new_points);
    // Matrix<Integer>(bottom_candidates).pretty_print(cout);

    if (verbose) {
        verboseOutput() << "Computing bbottom points using projection " << endl;
    }

    if (verbose) {
        verboseOutput() << "simplex volume " << volume << endl;
    }

    //---------------------------- begin stellar subdivision -------------------

    size_t stellar_det_sum = 0;
    vector<Matrix<Integer> > q_gens;  // for successive stellar subdivision
    q_gens.push_back(gens);
    int level = 0;  // level of subdivision

    std::exception_ptr tmp_exception;
    bool skip_remaining = false;
#pragma omp parallel  // reduction(+:stellar_det_sum)
    {
        try {
            vector<Matrix<Integer> > local_q_gens;
            list<vector<Integer> > local_new_points;

            while (!q_gens.empty()) {
                if (skip_remaining)
                    break;
                if (verbose) {
#pragma omp single
                    verboseOutput() << q_gens.size() << " simplices on level " << level++ << endl;
                }

#pragma omp for schedule(static)
                for (size_t i = 0; i < q_gens.size(); ++i) {
                    if (skip_remaining)
                        continue;

                    try {
                        bottom_points_inner(q_gens[i], local_new_points, local_q_gens, stellar_det_sum);
                    } catch (const std::exception&) {
                        tmp_exception = std::current_exception();
                        skip_remaining = true;
#pragma omp flush(skip_remaining)
                    }
                }

#pragma omp single
                { q_gens.clear(); }
#pragma omp critical(LOCALQGENS)
                { q_gens.insert(q_gens.end(), local_q_gens.begin(), local_q_gens.end()); }
                local_q_gens.clear();
#pragma omp barrier
            }

#pragma omp critical(LOCALNEWPOINTS)
            { new_points.splice(new_points.end(), local_new_points, local_new_points.begin(), local_new_points.end()); }

        } catch (const std::exception&) {
            tmp_exception = std::current_exception();
            skip_remaining = true;
#pragma omp flush(skip_remaining)
        }

    }  // end parallel

    //---------------------------- end stellar subdivision -----------------------

    if (!(tmp_exception == 0))
        std::rethrow_exception(tmp_exception);

    // cout  << new_points.size() << " new points accumulated" << endl;
    new_points.sort();
    new_points.unique();
    if (verbose) {
        verboseOutput() << new_points.size() << " bottom points accumulated in total." << endl;
        verboseOutput() << "The sum of determinants of the stellar subdivision is " << stellar_det_sum << endl;
    }

    /* for(auto& it : new_points)
        it=Trans_inv.VxM(it); */
}

//-----------------------------------------------------------------------------------------

template <typename Integer>
bool bottom_points_inner(Matrix<Integer>& gens,
                         list<vector<Integer> >& local_new_points,
                         vector<Matrix<Integer> >& local_q_gens,
                         size_t& stellar_det_sum) {
    INTERRUPT_COMPUTATION_BY_EXCEPTION

    vector<Integer> grading = gens.find_linear_form();
    Integer volume;
    int dim = gens[0].size();
    Matrix<Integer> Support_Hyperplanes = gens.invert(volume);

    if (volume < SubDivBound) {
#pragma omp atomic
        stellar_det_sum += convertTo<long long>(volume);
        return false;  // not subdivided
    }

    // try st4ellar subdivision
    Support_Hyperplanes = Support_Hyperplanes.transpose();
    Support_Hyperplanes.make_prime();
    vector<Integer> new_point;

    if (new_point.empty()) {
        list<vector<Integer> > Dummy;
        new_point = gens.optimal_subdivision_point();  // projection method
    }

    if (!new_point.empty()) {
        // if (find(local_new_points.begin(), local_new_points.end(),new_point) == local_new_points.end())
        local_new_points.push_back(new_point);
        Matrix<Integer> stellar_gens(gens);

        int nr_hyps = 0;
        for (int i = 0; i < dim; ++i) {
            if (v_scalar_product(Support_Hyperplanes[i], new_point) != 0) {
                stellar_gens[i] = new_point;
                local_q_gens.push_back(stellar_gens);

                stellar_gens[i] = gens[i];
            }
            else
                nr_hyps++;
        }
        //#pragma omp critical(VERBOSE)
        // cout << new_point << " liegt in " << nr_hyps <<" hyperebenen" << endl;
        return true;  // subdivided
    }
    else {  // could not subdivided
#pragma omp atomic
        stellar_det_sum += convertTo<long long>(volume);
        return false;
    }
}

// returns -1 if maximum is negative
template <typename Integer>
double max_in_col(const Matrix<Integer>& M, size_t j) {
    Integer max = -1;
    for (size_t i = 0; i < M.nr_of_rows(); ++i) {
        if (M[i][j] > max)
            max = M[i][j];
    }
    return convert_to_double(max);
}

// returns 1 if minimum is positive
template <typename Integer>
double min_in_col(const Matrix<Integer>& M, size_t j) {
    Integer min = 1;
    for (size_t i = 0; i < M.nr_of_rows(); ++i) {
        if (M[i][j] < min)
            min = M[i][j];
    }
    return convert_to_double(min);
}

#ifndef NMZ_MIC_OFFLOAD  // offload with long is not supported
template void bottom_points(list<vector<long> >& new_points, const Matrix<long>& gens, long VolumeBound);
#endif  // NMZ_MIC_OFFLOAD
template void bottom_points(list<vector<long long> >& new_points, const Matrix<long long>& gens, long long VolumeBound);
template void bottom_points(list<vector<mpz_class> >& new_points, const Matrix<mpz_class>& gens, mpz_class VolumeBound);

}  // namespace libnormaliz

#ifdef NMZ_MIC_OFFLOAD
#pragma offload_attribute(pop)
#endif
