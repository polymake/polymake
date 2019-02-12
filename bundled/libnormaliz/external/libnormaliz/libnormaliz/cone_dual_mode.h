/*
 * Normaliz
 * Copyright (C) 2007-2014  Winfried Bruns, Bogdan Ichim, Christof Soeger
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

#ifndef CONE_DUAL_MODE_H
#define CONE_DUAL_MODE_H

#include <list>
#include <vector>

#include "libnormaliz/libnormaliz.h"
#include "libnormaliz/matrix.h"
#include "libnormaliz/sublattice_representation.h"
#include "libnormaliz/reduction.h"

namespace libnormaliz {
using std::list;
using std::vector;

template<typename Integer> class CandidateList;
template<typename Integer> class Candidate;

template<typename Integer>
class Cone_Dual_Mode {
public:
    size_t dim;
    size_t nr_sh;

    bool verbose;
    
    bool inhomogeneous;
    bool do_only_Deg1_Elements;
    bool truncate;  // = inhomogeneous || do_only_Deg1_Elements
    
    Matrix<Integer> SupportHyperplanes;
    Matrix<Integer> Generators;
    vector<bool> ExtremeRaysInd;
    list<Candidate<Integer>* > ExtremeRayList; //only temporarily used
    CandidateList<Integer> Intermediate_HB; // intermediate Hilbert basis
    list<vector<Integer> > Hilbert_Basis; //the final result
    Matrix<Integer> BasisMaxSubspace; // a basis of the maximal linear subspace of the cone

/* ---------------------------------------------------------------------------
 *              Private routines, used in the public routines
 * ---------------------------------------------------------------------------
 */
    /* splices a vector of lists into a total list*/
    void splice_them_sort(CandidateList< Integer>& Total, vector<CandidateList< Integer> >& Parts);

    /* computes the Hilbert basis after adding a support hyperplane with the dual algorithm */
    void cut_with_halfspace_hilbert_basis(const size_t & hyp_counter, const bool  lifting, 
            vector<Integer> & halfspace, bool pointed);
    
    /* computes the Hilbert basis after adding a support hyperplane with the dual algorithm , general case */
    Matrix<Integer> cut_with_halfspace(const size_t & hyp_counter, const Matrix<Integer>& Basis_Max_Subspace);

    /* computes the extreme rays using rank test */
    void extreme_rays_rank();

    void relevant_support_hyperplanes();
    
    // move candidates of old_tot_deg <= guaranteed_HB_deg to Irred
    void select_HB(CandidateList<Integer>& Cand, size_t guaranteed_HB_deg, 
                                CandidateList<Integer>& Irred, bool all_irreducible);

    Cone_Dual_Mode(Matrix<Integer>& M, const vector<Integer>& Truncation, bool keep_order);            //main constructor

/*---------------------------------------------------------------------------
 *                      Data access
 *---------------------------------------------------------------------------
 */
 
    Matrix<Integer> get_support_hyperplanes() const;
    Matrix<Integer> get_generators() const;
    vector<bool> get_extreme_rays() const;


/*---------------------------------------------------------------------------
 *              Computation Methods
 *---------------------------------------------------------------------------
 */
    void hilbert_basis_dual();

    /* transforms all data to the sublattice */
    void to_sublattice(const Sublattice_Representation<Integer>& SR);

};
//class end *****************************************************************

}

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
