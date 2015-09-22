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
    // size_t hyp_size;
    
    bool verbose;
    
    bool inhomogeneous;
    bool do_only_Deg1_Elements;
    bool truncate;  // = inhomogeneous || do_only_Deg1_Elements
    bool first_pointed; // indicates in the course of the algorithm the first ppointed cone
    
    Matrix<Integer> SupportHyperplanes;
    Matrix<Integer> Generators;
    vector<bool> ExtremeRays;
    list<Candidate<Integer>* > ExtremeRayList; //only temporarily used
    CandidateList<Integer> Intermediate_HB; // intermediate Hilbert basis
    list<vector<Integer> > Hilbert_Basis; //the final result

/* ---------------------------------------------------------------------------
 *              Private routines, used in the public routines
 * ---------------------------------------------------------------------------
 */
    /* splices a vector of lists into a total list*/
    void splice_them_sort(CandidateList< Integer>& Total, vector<CandidateList< Integer> >& Parts);
    void set_generation_0(CandidateList< Integer>& L);

    /* Returns true if new_element is reducible versus the elements in Irred used for dual algorithm
     *  ATTENTION: this is "random access" for new_element if ordered==false. 
     * Otherrwise it is assumed that the new elements tested come in ascending total degree 
     * after the list underlying Irred has been ordered the last time */
    bool reducible(list<vector<Integer> *> & Irred, const vector<Integer> & new_element, 
                            const size_t & size, const bool ordered);

    /* reduce Red versus Irred ATTENTION: both lists must be ordered by total degree 
     * Irred will not be changed, Red is returned without the reducible elements, but no other
     * change 
     * ATTENTION: not suitable for autoreduction */
    void reduce(list<vector<Integer> > & Irred, list<vector<Integer> > & Red, const size_t & size);

    /* adds a new element that is irreducible w.r.t. Irred to Irred
     * the new elements must come from a structure sorted by total degree
     * used for dual algorithm */
    void reduce_and_insert(const vector<Integer> & new_element, list<vector<Integer> >& Irred, const size_t & size);
    
    /* reduces a list against itself
     * the list must be sorted  sorted by total degree as used for dual algorithm
     * The irreducible elements are reurned in ascendingorder */
    void auto_reduce(list< vector< Integer> >& To_Reduce, const size_t& size,bool no_pos_in_deg0);


    /* computes the Hilbert basis after adding a support hyperplane with the dual algorithm */
    void cut_with_halfspace_hilbert_basis(const size_t & hyp_counter, const bool  lifting, 
            vector<Integer> & halfspace, bool pointed);
    
    /* computes the Hilbert basis after adding a support hyperplane with the dual algorithm , general case */
    Matrix<Integer> cut_with_halfspace(const size_t & hyp_counter, const Matrix<Integer>& Basis_Max_Subspace);

    /* computes the extreme rays using reduction, used for the dual algorithm */
    void extreme_rays_reduction();
    
    /* computes the extreme rays using rank test, used for the dual algorithm */
    void extreme_rays_rank();

    void relevant_support_hyperplanes();
    
    void unique_vectors(list< vector< Integer > >& HB);
    
    // move canmdidates of old_tot_deg <= guaranteed_HB_deg to Irred
    void select_HB(CandidateList<Integer>& Cand, size_t guaranteed_HB_deg, 
                                CandidateList<Integer>& Irred, bool all_irreducible);

    Cone_Dual_Mode(const Matrix<Integer>& M);            //main constructor

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
