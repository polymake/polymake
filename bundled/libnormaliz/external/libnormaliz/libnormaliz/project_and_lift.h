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

#ifndef LIBNORMALIZ_PROJECT_AND_LIFT_H_
#define LIBNORMALIZ_PROJECT_AND_LIFT_H_

#include <vector>
#include <list>

#include "libnormaliz/general.h"
#include "libnormaliz/matrix.h"
#include "libnormaliz/sublattice_representation.h"
#include "libnormaliz/HilbertSeries.h"
#include "libnormaliz/dynamic_bitset.h"

namespace libnormaliz {
using std::vector;

// the project-and-lift algorithm for lattice points in a polytope

template <typename IntegerPL, typename IntegerRet>
class ProjectAndLift {
    template <typename, typename>
    friend class ProjectAndLift;

    vector<Matrix<IntegerPL> > AllSupps;
    vector<vector<size_t> > AllOrders;
    vector<size_t> AllNrEqus;  // the numbers of equations --- well defined
                               // in dimensions < start dimension !!!!

    Matrix<IntegerRet> Congs;  // congruences used in pure counting (so far)

    Matrix<IntegerPL> Vertices;  // only used for LLL coordinates

    Sublattice_Representation<IntegerRet> LLL_Coordinates;

    vector<dynamic_bitset> StartInd;
    vector<dynamic_bitset> StartPair;
    vector<dynamic_bitset> StartParaInPair;

    size_t StartRank;

    list<vector<IntegerRet> > Deg1Points;
    vector<IntegerRet> SingleDeg1Point;
    vector<IntegerRet> excluded_point;
    IntegerRet GD;

    vector<IntegerRet> Grading;
    size_t TotalNrLP;
    vector<size_t> NrLP;  // number of lattice points by dimension

    vector<num_t> h_vec_pos;
    vector<num_t> h_vec_neg;

    size_t EmbDim;
    bool verbose;

    bool is_parallelotope;
    bool no_crunch;  // indicates that the projection vector is nevere parallel to a facet of
                     // the parallelotope (in all dimensions)
    bool use_LLL;
    bool no_relax;

    bool count_only;

    vector<size_t> order_supps(const Matrix<IntegerPL>& Supps);
    bool fiber_interval(IntegerRet& MinInterval, IntegerRet& MaxInterval, const vector<IntegerRet>& base_point);

    void lift_point_recursively(vector<IntegerRet>& final_latt_point, const vector<IntegerRet>& latt_point_proj);

    void lift_points_to_this_dim(list<vector<IntegerRet> >& Deg1Proj);  // for counting of lattice points

    void find_single_point();

    void compute_latt_points();
    void compute_latt_points_float();

    void compute_projections(size_t dim,
                             size_t down_to,
                             vector<dynamic_bitset>& Ind,
                             vector<dynamic_bitset>& Pair,
                             vector<dynamic_bitset>& ParaInPair,
                             size_t rank,
                             bool only_projections = false);

    void initialize(const Matrix<IntegerPL>& Supps, size_t rank);

    // void make_LLL_coordinates();

   public:
    ProjectAndLift();
    ProjectAndLift(const Matrix<IntegerPL>& Supps, const vector<dynamic_bitset>& Ind, size_t rank);
    ProjectAndLift(const Matrix<IntegerPL>& Supps,
                   const vector<dynamic_bitset>& Pair,
                   const vector<dynamic_bitset>& ParaInPair,
                   size_t rank);
    template <typename IntegerPLOri, typename IntegerRetOri>
    ProjectAndLift(const ProjectAndLift<IntegerPLOri, IntegerRetOri>& Original);

    void set_excluded_point(const vector<IntegerRet>& excl_point);
    void set_grading_denom(const IntegerRet GradingDenom);
    void set_verbose(bool on_off);
    void set_LLL(bool on_off);
    void set_no_relax(bool on_off);
    void set_vertices(const Matrix<IntegerPL>& Verts);
    void set_congruences(const Matrix<IntegerRet>& congruences);
    void set_grading(const vector<IntegerRet>& grad);

    void compute(bool do_all_points = true, bool lifting_float = false, bool count_only = false);
    void compute_only_projection(size_t down_to);

    void putSuppsAndEqus(Matrix<IntegerPL>& SuppsRet, Matrix<IntegerPL>& EqusRet, size_t in_dim);
    void put_eg1Points_into(Matrix<IntegerRet>& LattPoints);
    void put_single_point_into(vector<IntegerRet>& LattPoint);
    void get_h_vectors(vector<num_t>& pos, vector<num_t>& neg) const;

    size_t getNumberLatticePoints() const;
};

// constructor by conversion

template <typename IntegerPL, typename IntegerRet>
template <typename IntegerPLOri, typename IntegerRetOri>
ProjectAndLift<IntegerPL, IntegerRet>::ProjectAndLift(const ProjectAndLift<IntegerPLOri, IntegerRetOri>& Original) {
    // The constructed PL is only good for lifting!!
    // Don't apply initialize to it

    EmbDim = Original.EmbDim;
    AllOrders = Original.AllOrders;
    verbose = Original.verbose;
    no_relax = Original.no_relax;
    convert(GD, Original.GD);
    AllSupps.resize(EmbDim + 1);
    for (size_t i = 0; i < AllSupps.size(); ++i)
        convert(AllSupps[i], Original.AllSupps[i]);
    convert(Congs, Original.Congs);
    TotalNrLP = 0;
    Grading = Original.Grading;
    count_only = Original.count_only;
    NrLP.resize(EmbDim + 1);
}

// computes c1*v1-c2*v2
template <typename Integer>
vector<Integer> FM_comb(Integer c1, const vector<Integer>& v1, Integer c2, const vector<Integer>& v2, bool& is_zero);

}  // end namespace libnormaliz

#endif /* PROJECT_AND_LIFT_H_ */
