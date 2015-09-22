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

//---------------------------------------------------------------------------

// The matrix Map is assumed to be such that the rank of Map equals
// the number of columns of Map and no test is performed in the constructor

//---------------------------------------------------------------------------
#ifndef SIMPLEX_H
#define SIMPLEX_H

//---------------------------------------------------------------------------

#include <vector>
#include <list>

#include <boost/dynamic_bitset.hpp>

#include "libnormaliz/libnormaliz.h"
#include "libnormaliz/cone.h"
#include "libnormaliz/HilbertSeries.h"
#include "libnormaliz/reduction.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using std::list;
using std::vector;

template<typename Integer> class Full_Cone;
template<typename Integer> class Collector;

template<typename Integer>
class SimplexEvaluator {
    Full_Cone<Integer> * C_ptr;
    int tn; //number of associated thread in parallelization of evaluators
            // to be set by Full_Cone
    size_t dim;
    Integer volume;
    mpz_class mpz_volume;
    size_t Deg0_offset; // the degree of 0+offset
    // Integer det_sum; // sum of the determinants of all evaluated simplices --> Collector
    // mpq_class mult_sum; // sum of the multiplicities of all evaluated simplices --> Collector
    vector<key_t> key; 
    // size_t candidates_size;
    // size_t collected_elements_size;
    Matrix<Integer> Generators;
    Matrix<Integer> LinSys;
    Matrix<Integer> GenCopy;
    Matrix<Integer> InvGenSelRows; // selected rows of inverse of Gen
    Matrix<Integer> InvGenSelCols; // selected columns of inverse of Gen
    Matrix<Integer> Sol;
    Matrix<Integer> ProjGen; // generators of projection modulo level 0 sublattice
    vector< Integer > GDiag; // diagonal of generator matrix after trigonalization
    vector< Integer > TDiag; // diagonal of transpose of generaor matrix after trigonalization
    vector< bool > Excluded;
    vector< Integer > Indicator; 
    vector< long > gen_degrees;
    vector< long > level0_gen_degrees; // degrees of the generaors in level 0
    vector< long > gen_levels;
    // vector< num_t > hvector;  //h-vector of the current evaluation
    // vector< num_t > inhom_hvector; // separate vector in the inhomogeneous case in case we want to compute two h-vectors
    // HilbertSeries Hilbert_Series; //this is the summed Hilbert Series
    // list< vector<Integer> > Candidates;
    list< vector<Integer> > Hilbert_Basis;
    // CandidateList<Integer> Collected_HB_Elements;
    // list< vector<Integer> > Collected_Deg1_Elements;
    //temporary objects are kept to prevent repeated alloc and dealloc
    Matrix<Integer> RS; // right hand side to hold order vector
    // Matrix<Integer> RSmult; // for multiple right hand sides
    
    Matrix<Integer>* StanleyMat;
    size_t StanIndex;
    size_t nr_level0_gens; // counts the number of level 0 vectors among the generators
    
    bool sequential_evaluation; // indicates whether the simplex is evaluated by a single thread
    
    bool GMP_transition; 
    
    struct SIMPLINEXDATA{                    // local data of excluded faces
        boost::dynamic_bitset<> GenInFace;   // indicator for generators of simplex in face 
        // vector< num_t > hvector;             // accumulates the h-vector of this face
        long mult;                           // the multiplicity of this face 
        // bool touched;                        // indicates whether hvector != 0 // ALWAYS true, hence superfluous
        vector< long > gen_degrees;          // degrees of generators in this face
    };
    vector<SIMPLINEXDATA> InExSimplData;
    size_t nrInExSimplData;
    // bool InExTouched;                        // indicates whether any hvector!=0 // see above
    
    vector<vector<Integer>* > RS_pointers;
    Matrix<Integer> unit_matrix;
    vector<key_t> id_key;
    Matrix<mpz_class> mpz_Generators;
    

    void local_reduction(Collector<Integer>& Coll);

    //checks whether new_element is reducible by the Reducers list
    bool is_reducible(const vector< Integer >& new_element, list< vector<Integer> >& Reducers);
    // removes elements from Candi which are reducible by Reducers, Reducers must be sorted by compare_last!
    void reduce(list< vector<Integer> >& Candi, list< vector<Integer> >& Reducers, size_t& Candi_size);
    void count_and_reduce(list< vector<Integer> >& Candi, list< vector<Integer> >& Reducers);

    bool isDuplicate(const vector<Integer>& cand) const;

	void addMult( Integer multiplicity, Collector<Integer>& Coll);

    void prepare_inclusion_exclusion_simpl(size_t Deg, Collector<Integer>& Coll);
    void add_to_inex_faces(const vector<Integer> offset, size_t Deg, Collector<Integer>& Coll);
    void update_inhom_hvector(long level_offset, size_t Deg, Collector<Integer>& Coll);
    void update_mult_inhom(Integer& multiplicity);
    
    Integer start_evaluation(SHORTSIMPLEX<Integer>& s, Collector<Integer>& Coll);
    void take_care_of_0vector(Collector<Integer>& Coll);
    // void evaluation_loop_sequential(Collector<Integer>& Coll);
    void evaluate_element(const vector<Integer>& element, Collector<Integer>& Coll);
    void conclude_evaluation(Collector<Integer>& Coll);
    void evaluation_loop_parallel();
    void evaluate_block(long block_start, long block_end, Collector<Integer>& Coll);
    void collect_vectors();
    
    // void insert_gens();
    // void insert_gens_transpose();
    // void insert_unit_vectors(vector<key_t> RHS_key);
    
    void transform_to_global(const vector<Integer>& element, vector<Integer>& help);


//---------------------------------------------------------------------------

public:

    SimplexEvaluator(Full_Cone<Integer>& fc); 
    
        // sets the thread number of the evaluator (needed to associate a collector)
    void set_evaluator_tn(int threadnum);

    // full evaluation of the simplex in a single thread, delivers results to to a collector
    bool evaluate(SHORTSIMPLEX<Integer>& s);
    
    // evaluation in parallel threads
    void Simplex_parallel_evaluation();  

        vector<key_t> get_key();

    
    void print_all();
};
//class SimplexEvaluator end

template<typename Integer>
class Collector {
    
    template<typename> friend class SimplexEvaluator;
    template<typename> friend class Full_Cone;
    
    Full_Cone<Integer> * C_ptr;
    size_t dim;

    Integer det_sum; // sum of the determinants of all evaluated simplices
    mpq_class mult_sum; // sum of the multiplicities of all evaluated simplices
    size_t candidates_size;
    size_t collected_elements_size;
    vector< num_t > hvector;  //h-vector of the current evaluation
    vector< num_t > inhom_hvector; // separate vector in the inhomogeneous case in case we want to compute two h-vectors
    HilbertSeries Hilbert_Series; //this is the summed Hilbert Series
    list< vector<Integer> > Candidates;
    CandidateList<Integer> HB_Elements;
    list< vector<Integer> > Deg1_Elements;
    vector<vector< num_t> > InEx_hvector;

    Matrix<Integer> elements;
    
    public:

    Collector(Full_Cone<Integer>& fc);
    
    // returns sum of the determinants of all evaluated simplices
    Integer getDetSum() const;
    
    // returns sum of the multiplicities of all evaluated simplices
    mpq_class getMultiplicitySum() const;
    
    // returns sum of the Hilbert Series of all evaluated simplices
    const HilbertSeries& getHilbertSeriesSum() const;
    
    // moves the union of Hilbert basis / deg1 elements to the cone
    // for partial triangulation it merges the sorted list
    void transfer_candidates();
    
    size_t get_collected_elements_size();

};
// class end Collector

}

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
