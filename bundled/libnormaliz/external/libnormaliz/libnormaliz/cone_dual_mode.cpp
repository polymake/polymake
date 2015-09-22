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

#include <stdlib.h>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <string>
#include <algorithm>

#include "libnormaliz/cone_dual_mode.h"
#include "libnormaliz/vector_operations.h"
#include "libnormaliz/list_operations.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------
//private
//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::splice_them_sort(CandidateList< Integer>& Total, vector<CandidateList< Integer> >& Parts){

    CandidateList<Integer> New;
    New.verbose=verbose;
    New.dual=true;
    for(int i=0;i<omp_get_max_threads();i++)
        New.Candidates.splice(New.Candidates.end(),Parts[i].Candidates);
    New.sort_by_val();
    New.unique_vectors();
    Total.merge_by_val(New);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::select_HB(CandidateList<Integer>& Cand, size_t guaranteed_HB_deg, 
                    CandidateList<Integer>& Irred, bool all_irreducible){

    if(all_irreducible){
        Irred.merge_by_val(Cand);
        return;
    }

    typename list<Candidate<Integer> >::iterator h;
    for(h=Cand.Candidates.begin(); h!=Cand.Candidates.end();){
        if(h->old_tot_deg<=guaranteed_HB_deg){
            Irred.Candidates.splice(Irred.Candidates.end(),Cand.Candidates,h++);
        }
        else{
            ++h;
        }
    }
    Irred.auto_reduce_sorted();  // necessary since the guaranteed HB degree only determines 
                          // in which degrees we can already decide whether an element belongs to the HB            
}


//---------------------------------------------------------------------------


//public
//---------------------------------------------------------------------------

template<typename Integer>
Cone_Dual_Mode<Integer>::Cone_Dual_Mode(const Matrix<Integer>& M){
    dim=M.nr_of_columns();
    if (dim!=M.rank()) {
        errorOutput()<<"Cone_Dual_Mode error: constraints do not define pointed cone!"<<endl;
        // M.pretty_print(errorOutput());
        throw BadInputException();
    }
    SupportHyperplanes = M;
    // support hyperplanes are already coprime (except for truncation/grading)
    // so just remove 0 rows
    SupportHyperplanes.remove_zero_rows();
    nr_sh = SupportHyperplanes.nr_of_rows();
    // hyp_size = dim + nr_sh;
    first_pointed = true;
    Intermediate_HB.dual=true;

    if (nr_sh != static_cast<size_t>(static_cast<key_t>(nr_sh))) {
        errorOutput()<<"Too many support hyperplanes to fit in range of key_t!"<<endl;
        throw FatalException();
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Cone_Dual_Mode<Integer>::get_support_hyperplanes() const {
    return SupportHyperplanes;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Cone_Dual_Mode<Integer>::get_generators()const{
    return Generators;
}

template<typename Integer>
vector<bool> Cone_Dual_Mode<Integer>::get_extreme_rays() const{
    return ExtremeRays;

}


size_t counter=0,counter1=0, counter2=0;

const size_t ReportBound=100000;

//---------------------------------------------------------------------------

// In the inhomogeneous case or when only degree 1 elements are to be found,
// we truncate the Hilbert basis at level 1. The level is the ordinaryl degree in
// for degree 1 elements and the degree of the homogenizing variable 
// in the inhomogeneous case.
//
// As soon as there are no positive or neutral (with respect to the current hyperplane)
// elements in the current Hilbert basis and truncate==true, new elements can only 
// be produced as sums of positive irreds of level 1 and negative irreds of level 0.
// In particular no new negative elements can be produced, and the only type of
// reduction on the positive side is the elimination of duplicates.
//
// If there are no elements on level 0 at all, then new elements cannot be produced anymore,
// and the production of new elements can be skipped.

template<typename Integer>
void Cone_Dual_Mode<Integer>::cut_with_halfspace_hilbert_basis(const size_t& hyp_counter, 
         const bool lifting, vector<Integer>& old_lin_subspace_half, bool pointed){
    if (verbose==true) {
        verboseOutput()<<"==================================================" << endl;
        verboseOutput()<<"cut with halfspace "<<hyp_counter+1 <<" ..."<<endl;
    }
    
    truncate=inhomogeneous || do_only_Deg1_Elements;
    
    size_t i;
    int sign;

    CandidateList<Integer> Positive_Irred(true),Negative_Irred(true),Neutral_Irred(true); // for the Hilbert basis elements
    Positive_Irred.verbose=Negative_Irred.verbose=Neutral_Irred.verbose=verbose;
    list<Candidate<Integer>* > Pos_Gen0, Pos_Gen1, Neg_Gen0, Neg_Gen1;  // pointer lists for generation control
    size_t pos_gen0_size=0, pos_gen1_size=0, neg_gen0_size=0, neg_gen1_size=0;
        
    Integer orientation, scalar_product,diff,factor;
    vector <Integer> hyperplane=SupportHyperplanes[hyp_counter]; // the current hyperplane dividing the old cone
    typename list<Candidate<Integer> >::iterator h;

    if (lifting==true) {
        orientation=v_scalar_product<Integer>(hyperplane,old_lin_subspace_half);
        if(orientation<0){
            orientation=-orientation;
            v_scalar_multiplication<Integer>(old_lin_subspace_half,-1); //transforming into the generator of the positive half of the old max lin subsapce
        }
        // from now on orientation > 0 
        
        for (h = Intermediate_HB.Candidates.begin(); h != Intermediate_HB.Candidates.end(); ++h) { //reduction  modulo  the generators of the two halves of the old max lin subspace
            scalar_product=v_scalar_product(hyperplane,h->cand); //  allows us to declare "old" HB candiadtes as irreducible
            sign=1;                                                               
            if (scalar_product<0) {
                scalar_product=-scalar_product;
                sign=-1;
            }
            factor=scalar_product/orientation;  // we reduce all elements by the generator of the halfspace
            for (i = 0; i < dim; i++) {
                h->cand[i]=h->cand[i]-sign*factor*old_lin_subspace_half[i];
            }
        }
        
        //adding the generators of the halves of the old max lin subspaces to the the "positive" and the "negative" generators
        // ABSOLUTELY NECESSARY since we need a monoid system of generators of the full "old" cone

        Candidate<Integer> halfspace_gen_as_cand(old_lin_subspace_half,nr_sh);
        halfspace_gen_as_cand.mother=0;
        // halfspace_gen_as_cand.father=0;
        halfspace_gen_as_cand.old_tot_deg=0;
        (halfspace_gen_as_cand.values)[hyp_counter]=orientation; // value under the new linear form
        halfspace_gen_as_cand.sort_deg=convertTo<long>(orientation);
        assert(orientation!=0);
        Positive_Irred.push_back(halfspace_gen_as_cand);
        Pos_Gen0.push_back(&Positive_Irred.Candidates.front());
        pos_gen0_size=1;
        v_scalar_multiplication<Integer>(halfspace_gen_as_cand.cand,-1);    
        Negative_Irred.push_back(halfspace_gen_as_cand);
        Neg_Gen0.push_back(&Negative_Irred.Candidates.front());
        neg_gen0_size=1;
    } //end lifting
    
    long gen0_mindeg;  // minimal degree of a generator
    if(lifting)
        gen0_mindeg=0;  // sort_deg has already been set > 0 for half_space_gen
    else
        gen0_mindeg=Intermediate_HB.Candidates.begin()->sort_deg;
    typename list<Candidate<Integer> >::const_iterator hh;
    for(hh=Intermediate_HB.Candidates.begin();hh!=Intermediate_HB.Candidates.end();++hh)
        if(hh->sort_deg < gen0_mindeg)
            gen0_mindeg=hh->sort_deg;
        
    bool gen1_pos=false, gen1_neg=false;    
    bool no_pos_in_level0=pointed;
    bool all_positice_level=pointed;
    for (h = Intermediate_HB.Candidates.begin(); h != Intermediate_HB.Candidates.end(); ++h) { //dividing into negative and positive
        Integer new_val=v_scalar_product<Integer>(hyperplane,h->cand);
        long new_val_long=convertTo<long>(new_val);
        h->reducible=false;
        h->mother=0;
        // h->father=0;
        h->old_tot_deg=h->sort_deg;
        if (new_val>0) {
            gen1_pos=true;
            h->values[hyp_counter]=new_val;
            h->sort_deg+=new_val_long;
            Positive_Irred.Candidates.push_back(*h); // could be spliced
            Pos_Gen1.push_back(&Positive_Irred.Candidates.back());
            pos_gen1_size++;
            if(h->values[0]==0){
                no_pos_in_level0=false;
                all_positice_level=false;
            }
        }
        if (new_val<0) {
            gen1_neg=true;
            h->values[hyp_counter]=-new_val;
            h->sort_deg+=-new_val_long;
            Negative_Irred.Candidates.push_back(*h);
            Neg_Gen1.push_back(&Negative_Irred.Candidates.back());
            neg_gen1_size++;
            if(h->values[0]==0){
                all_positice_level=false;
            }
        }
        if (new_val==0) {
            Neutral_Irred.Candidates.push_back(*h);
            if(h->values[0]==0){
                no_pos_in_level0=false;
                all_positice_level=false;
            }
        }       
    }
   

    if((truncate && (no_pos_in_level0 && !all_positice_level))){
        if(verbose){
            verboseOutput() << "Eliminating negative generators of level > 0" << endl;
        }
        Neg_Gen1.clear();
        neg_gen1_size=0;
        for (h = Negative_Irred.Candidates.begin(); h != Negative_Irred.Candidates.end();){
            if(h->values[0]>0)
                h=Negative_Irred.Candidates.erase(h);
            else{
                Neg_Gen1.push_back(&(*h));
                neg_gen1_size++;   
                ++h;
            }
        }
    }
    
    std::exception_ptr tmp_exception;

    #pragma omp parallel num_threads(3)
    {

        #pragma omp single nowait
        {
#ifndef NCATCH
        try {
#endif
        check_range_list(Negative_Irred);
        Negative_Irred.sort_by_val();
        Negative_Irred.last_hyp=hyp_counter;
#ifndef NCATCH
        } catch(const std::exception& ) {
            tmp_exception = std::current_exception();
        }
#endif
        }

        #pragma omp single nowait
        {
#ifndef NCATCH
        try {
#endif
        check_range_list(Positive_Irred);
        Positive_Irred.sort_by_val();
        Positive_Irred.last_hyp=hyp_counter;
#ifndef NCATCH
        } catch(const std::exception& ) {
            tmp_exception = std::current_exception();
        }
#endif
        }

        #pragma omp single nowait
        {
        Neutral_Irred.sort_by_val();
        Neutral_Irred.last_hyp=hyp_counter;
        }
    }
    if (!(tmp_exception == 0)) std::rethrow_exception(tmp_exception);
    
    CandidateList<Integer> New_Positive_Irred(true),New_Negative_Irred(true),New_Neutral_Irred(true);
    New_Positive_Irred.verbose=New_Negative_Irred.verbose=New_Neutral_Irred.verbose=verbose;
    New_Negative_Irred.last_hyp=hyp_counter;  // for the newly generated vector in each thread
    New_Positive_Irred.last_hyp=hyp_counter;
    New_Neutral_Irred.last_hyp=hyp_counter;
    
    CandidateList<Integer> Positive_Depot(true),Negative_Depot(true),Neutral_Depot(true); // to store the new vectors after generation
    Positive_Depot.verbose=Negative_Depot.verbose=Neutral_Depot.verbose=verbose;
    
    vector<CandidateList<Integer> > New_Positive_thread(omp_get_max_threads()),
                      New_Negative_thread(omp_get_max_threads()),
                      New_Neutral_thread(omp_get_max_threads());
                      
     vector<CandidateTable<Integer> > Pos_Table, Neg_Table, Neutr_Table; // for reduction in each thread                   
                      
     for(long i=0;i<omp_get_max_threads();++i){
        New_Positive_thread[i].dual=true;
        New_Positive_thread[i].verbose=verbose;
        New_Negative_thread[i].dual=true;
        New_Negative_thread[i].verbose=verbose;
        New_Neutral_thread[i].dual=true;
        New_Neutral_thread[i].verbose=verbose;
    }
    
    for(int k=0;k<omp_get_max_threads();++k){
        Pos_Table.push_back(CandidateTable<Integer>(Positive_Irred));
        Neg_Table.push_back(CandidateTable<Integer>(Negative_Irred));
        Neutr_Table.push_back(CandidateTable<Integer>(Neutral_Irred));
    }
    
    typename list<Candidate<Integer>* >::iterator n,p;
    Candidate<Integer> *p_cand, *n_cand;
    // typename list<Candidate<Integer> >::iterator c;
    
    bool not_done;
    if(lifting)
        not_done=gen1_pos || gen1_neg;
    else
        not_done=gen1_pos && gen1_neg;
        
    bool do_reduction=!(truncate && no_pos_in_level0);
    
    bool do_only_selection=truncate && all_positice_level;
    
    size_t round=0;        
    
    while(not_done && !do_only_selection) {

        //generating new elements
        round++;
        
        typename list<Candidate<Integer>* >::iterator pos_begin, pos_end, neg_begin, neg_end;
        size_t pos_size, neg_size;

        // Steps are:
        // 0: old pos vs. new neg
        // 1: new pos vs. old neg
        // 2: new pos vs. new neg
        for(size_t step=0;step<=2;step++)
        {
        
        if(step==0){
            pos_begin=Pos_Gen0.begin();
            pos_end=Pos_Gen0.end();
            neg_begin=Neg_Gen1.begin();
            neg_end=Neg_Gen1.end();
            pos_size=pos_gen0_size;
            neg_size=neg_gen1_size;      
        }
        
        if(step==1){
            pos_begin=Pos_Gen1.begin();
            pos_end=Pos_Gen1.end();
            neg_begin=Neg_Gen0.begin();
            neg_end=Neg_Gen0.end();
            pos_size=pos_gen1_size;
            neg_size=neg_gen0_size;;      
        }
        
        if(step==2){
            pos_begin=Pos_Gen1.begin();
            pos_end=Pos_Gen1.end();
            neg_begin=Neg_Gen1.begin();
            neg_end=Neg_Gen1.end();
            pos_size=pos_gen1_size;
            neg_size=neg_gen1_size;      
        }
        
        // cout << "Step " << step << " pos " << pos_size << " neg " << neg_size << endl;
        
        if(pos_size==0 || neg_size==0)
            continue;

        if (verbose) {
            // size_t neg_size=Negative_Irred.size();
            // size_t zsize=Neutral_Irred.size();
            if (pos_size*neg_size>=ReportBound)
                verboseOutput()<<"Positive: "<<pos_size<<"  Negative: "<<neg_size<< endl;
            else{
                if(round%100==0)
                    verboseOutput() << "Round " << round << endl;
            }
        }
        
        bool skip_remaining = false;

        const long VERBOSE_STEPS = 50;
        long step_x_size = pos_size-VERBOSE_STEPS;
        
        #pragma omp parallel private(p,n,diff,p_cand,n_cand)
        {
        Candidate<Integer> new_candidate(dim,nr_sh);
                
        size_t ppos=0;
        p = pos_begin;
        #pragma omp for schedule(dynamic)
        for(i = 0; i<pos_size; ++i){
            for(;i > ppos; ++ppos, ++p) ;
            for(;i < ppos; --ppos, --p) ;

            if (skip_remaining) continue;

#ifndef NCATCH
            try {
#endif
            
            if(verbose && pos_size*neg_size>=ReportBound){
                #pragma omp critical(VERBOSE)
                while ((long)(i*VERBOSE_STEPS) >= step_x_size) {
                    step_x_size += pos_size;
                    verboseOutput() << "." <<flush;
                }

            }
            
            p_cand=*p;
            
            Integer pos_val=p_cand->values[hyp_counter];

            for (n= neg_begin; n!= neg_end; ++n){
            
                n_cand=*n;
            
                if(truncate && p_cand->values[0]+n_cand->values[0] >=2) // in the inhomogeneous case we truncate at level 1
                    continue;

                Integer neg_val=n_cand->values[hyp_counter];
                diff=pos_val-neg_val;
                
                // prediction of reducibility
                
                if (diff >0 && n_cand->mother!=0 && 
                    ( 
                    n_cand->mother<=pos_val // sum of p_cand and n_cand would be irreducible by mother + the vector on the opposite side
                    || (p_cand->mother >= n_cand->mother && p_cand->mother-n_cand->mother <=diff) // sum would reducible ny mother + mother
                    ) 
                    ){  
                    // #pragma omp atomic     
                    // counter1++;
                    continue;
                }
                
                                
                if ( diff <0 && p_cand->mother!=0 && 
                    (
                    p_cand->mother<=neg_val
                    || (n_cand->mother >= p_cand->mother && n_cand->mother-p_cand->mother <= -diff)
                    )
                    ){  
                    // #pragma omp atomic     // sum would be irreducible by mother + the vector on the opposite side
                    // counter1++;
                    continue;
                }
                
                if(diff==0 && p_cand->mother!=0 && n_cand->mother == p_cand->mother){
                    // #pragma omp atomic
                    // counter1++;
                    continue;
                }
                
                // #pragma omp atomic
                // counter++;
                
                new_candidate.old_tot_deg=p_cand->old_tot_deg+n_cand->old_tot_deg;
                v_add_result(new_candidate.values,hyp_counter,p_cand->values,n_cand->values);   // new_candidate=v_add

                if (diff>0) {
                    new_candidate.values[hyp_counter]=diff;
                    new_candidate.sort_deg=p_cand->sort_deg+n_cand->sort_deg-2*convertTo<long>(neg_val);
                    if(do_reduction && (Pos_Table[omp_get_thread_num()].is_reducible_unordered(new_candidate) ||
                                Neutr_Table[omp_get_thread_num()].is_reducible_unordered(new_candidate)))
                        continue;
                    v_add_result(new_candidate.cand,dim,p_cand->cand,n_cand->cand);
                    new_candidate.mother=pos_val;
                    // new_candidate.father=neg_val;                    
                    New_Positive_thread[omp_get_thread_num()].push_back(new_candidate);
                }
                if (diff<0) {                    
                    if(!do_reduction) // don't need new negative elements anymore
                        continue;
                    new_candidate.values[hyp_counter]=-diff;
                    new_candidate.sort_deg=p_cand->sort_deg+n_cand->sort_deg-2*convertTo<long>(pos_val);
                    if(Neg_Table[omp_get_thread_num()].is_reducible_unordered(new_candidate)) {
                        continue;
                    }
                    if(Neutr_Table[omp_get_thread_num()].is_reducible_unordered(new_candidate)) {
                        continue;
                    }
                    v_add_result(new_candidate.cand,dim,p_cand->cand,n_cand->cand);
                    new_candidate.mother=neg_val;
                    // new_candidate.father=pos_val;
                    New_Negative_thread[omp_get_thread_num()].push_back(new_candidate);
                }
                if (diff==0) {                  
                    new_candidate.values[hyp_counter]=0;
                    new_candidate.sort_deg=p_cand->sort_deg+n_cand->sort_deg-2*convertTo<long>(pos_val);  //pos_val==neg_val
                    if(do_reduction && Neutr_Table[omp_get_thread_num()].is_reducible_unordered(new_candidate)) {
                        continue;
                    }
                    v_add_result(new_candidate.cand,dim,p_cand->cand,n_cand->cand);
                    // new_candidate.mother=0; // irrelevant
                    New_Neutral_thread[omp_get_thread_num()].push_back(new_candidate);
                }
            }
#ifndef NCATCH
        } catch(const std::exception& ) {
            tmp_exception = std::current_exception();
            skip_remaining = true;
            #pragma omp flush(skip_remaining)
        }
#endif
        } //end generation of new elements
        
        #pragma omp single
        {
        if(verbose && pos_size*neg_size>=ReportBound)
            verboseOutput() << endl;
        }

        } //END PARALLEL

        if (!(tmp_exception == 0)) std::rethrow_exception(tmp_exception);

        } // steps

        Pos_Gen0.splice(Pos_Gen0.end(),Pos_Gen1); // the new generation has becomeold
        pos_gen0_size+=pos_gen1_size;
        pos_gen1_size=0;
        Neg_Gen0.splice(Neg_Gen0.end(),Neg_Gen1);
        neg_gen0_size+=neg_gen1_size;
        neg_gen1_size=0;

        splice_them_sort(Neutral_Depot,New_Neutral_thread); // sort by sort_deg and values   
        
        splice_them_sort(Positive_Depot,New_Positive_thread);

        splice_them_sort(Negative_Depot,New_Negative_thread);
        
        if(Positive_Depot.empty() && Negative_Depot.empty())
            not_done=false;
            
        // Attention: the element with smallest old_tot_deg need not be the first in the list which is ordered by sort_deg
        size_t gen1_mindeg=0;  // minimal old_tot_deg of a new element used for generation
        bool first=true;
        typename list<Candidate<Integer> >::iterator c;
        for(c = Positive_Depot.Candidates.begin();c!=Positive_Depot.Candidates.end();++c){
            if(first){
                first=false;
                gen1_mindeg=c->old_tot_deg;
            }
            if(c->old_tot_deg<gen1_mindeg)
                gen1_mindeg=c->old_tot_deg;
        }
        
        for(c = Negative_Depot.Candidates.begin();c!=Negative_Depot.Candidates.end();++c){
            if(first){
                first=false;
                gen1_mindeg=c->old_tot_deg;
            }
            if(c->old_tot_deg<gen1_mindeg)
                gen1_mindeg=c->old_tot_deg;

        }
        
        size_t min_deg_new=gen0_mindeg+gen1_mindeg;
        if(not_done)
            assert(min_deg_new>0);

        size_t all_known_deg=min_deg_new-1;
        size_t guaranteed_HB_deg=2*all_known_deg+1;  // the degree up to which we can decide whether an element belongs to the HB
        
        if(not_done){
            select_HB(Neutral_Depot,guaranteed_HB_deg,New_Neutral_Irred,!do_reduction);         
        }
        else{
            Neutral_Depot.auto_reduce_sorted();                    // in this case new elements will not be produced anymore
            Neutral_Irred.merge_by_val(Neutral_Depot);      // and there is nothing to do for positive or negative elements
                                                        // but the remaining neutral elements must be auto-reduced.             
       }
       
       CandidateTable<Integer> New_Pos_Table(true,hyp_counter), New_Neg_Table(true,hyp_counter), New_Neutr_Table(true,hyp_counter); 
                 // for new elements

       if (!New_Neutral_Irred.empty()) {
            if(do_reduction){
                Positive_Depot.reduce_by(New_Neutral_Irred);
                Neutral_Depot.reduce_by(New_Neutral_Irred);
            }
            Negative_Depot.reduce_by(New_Neutral_Irred);
            list<Candidate<Integer>* > New_Elements;
            Neutral_Irred.merge_by_val(New_Neutral_Irred,New_Elements); 
            typename list<Candidate<Integer>* >::iterator c;
            for(c=New_Elements.begin(); c!=New_Elements.end(); ++c){
                New_Neutr_Table.ValPointers.push_back(pair< size_t, vector<Integer>* >((*c)->sort_deg,&((*c)->values)));
            }
            New_Elements.clear();
        } 
       
        select_HB(Positive_Depot,guaranteed_HB_deg,New_Positive_Irred,!do_reduction);

        select_HB(Negative_Depot,guaranteed_HB_deg,New_Negative_Irred,!do_reduction);
                                                                 
        if (!New_Positive_Irred.empty()) {
            if(do_reduction)
                Positive_Depot.reduce_by(New_Positive_Irred);
            check_range_list(New_Positive_Irred);  // check for danger of overflow
            Positive_Irred.merge_by_val(New_Positive_Irred,Pos_Gen1);
            typename list<Candidate<Integer>* >::iterator c;
            for(c=Pos_Gen1.begin(); c!=Pos_Gen1.end(); ++c){
                New_Pos_Table.ValPointers.push_back(pair< size_t, vector<Integer>* >((*c)->sort_deg,&((*c)->values)));
                pos_gen1_size++;
            }
        }

        if (!New_Negative_Irred.empty()) {
            Negative_Depot.reduce_by(New_Negative_Irred);
            check_range_list(New_Negative_Irred);
            Negative_Irred.merge_by_val(New_Negative_Irred,Neg_Gen1);
            typename list<Candidate<Integer>* >::iterator c;
            for(c=Neg_Gen1.begin(); c!=Neg_Gen1.end(); ++c){
                New_Neg_Table.ValPointers.push_back(pair< size_t, vector<Integer>* >((*c)->sort_deg,&((*c)->values)));
                neg_gen1_size++;
            }
        }
    
        CandidateTable<Integer> Help(true,hyp_counter);
        
        for(int k=0;k<omp_get_max_threads();++k){
            Help=New_Pos_Table;
            Pos_Table[k].ValPointers.splice(Pos_Table[k].ValPointers.end(),Help.ValPointers);
            Help=New_Neg_Table;
            Neg_Table[k].ValPointers.splice(Neg_Table[k].ValPointers.end(),Help.ValPointers);
            Help=New_Neutr_Table;
            Neutr_Table[k].ValPointers.splice(Neutr_Table[k].ValPointers.end(),Help.ValPointers);
        }

    }  // while(not_done)
    
    if (verbose) {
        verboseOutput()<<"Final sizes: Pos " << pos_gen0_size << " Neg " << neg_gen0_size  << " Neutral " << Neutral_Irred.size() <<endl;
    }
    
    

    Intermediate_HB.clear();
    Intermediate_HB.Candidates.splice(Intermediate_HB.Candidates.begin(),Positive_Irred.Candidates);
    Intermediate_HB.Candidates.splice(Intermediate_HB.Candidates.end(),Neutral_Irred.Candidates);    
    Intermediate_HB.sort_by_val();       
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Cone_Dual_Mode<Integer>::cut_with_halfspace(const size_t& hyp_counter, const Matrix<Integer>& Basis_Max_Subspace){
    size_t i,rank_subspace=Basis_Max_Subspace.nr_of_rows();

    vector <Integer> restriction,lin_form=SupportHyperplanes[hyp_counter],old_lin_subspace_half;
    bool lifting=false;
    Matrix<Integer> New_Basis_Max_Subspace=Basis_Max_Subspace; // the new maximal subspace is the intersection of the old with the new haperplane
    if (rank_subspace!=0) {
        restriction=Basis_Max_Subspace.MxV(lin_form);  // the restriction of the new linear form to Max_Subspace
        for (i = 0; i <rank_subspace; i++)
            if (restriction[i]!=0)
                break;
        if (i!=rank_subspace) {    // the new hyperplane does not contain the intersection of the previous hyperplanes
                                   // so we must intersect the new hyperplane and Max_Subspace
            lifting=true;

            Matrix<Integer> M(1,rank_subspace); // this is the restriction of the new linear form to Max_Subspace
            M[0]=restriction;                   // encoded as a matrix
            
            size_t dummy_rank;
            Matrix<Integer> NewBasisOldMaxSubspace=M.AlmostHermite(dummy_rank).transpose(); // compute kernel of restriction and complementary subspace            
            
            Matrix<Integer> NewBasisOldMaxSubspaceAmbient=NewBasisOldMaxSubspace.multiplication(Basis_Max_Subspace); 
            // in coordinates of the ambient space

            old_lin_subspace_half=NewBasisOldMaxSubspaceAmbient[0];            
            
            // old_lin_subspace_half refers to the fact that the complementary space is subdivided into
            // two halfspaces generated by old_lin_subspace_half and -old_lin_subspace_half (taken care of in cut_with_halfspace_hilbert_basis)
            
            Matrix<Integer> temp(rank_subspace-1,dim);
            for(size_t k=1;k<rank_subspace;++k)            
                temp[k-1]=NewBasisOldMaxSubspaceAmbient[k];
            New_Basis_Max_Subspace=temp;
        }
    }
    bool pointed=(Basis_Max_Subspace.nr_of_rows()==0);

    cut_with_halfspace_hilbert_basis(hyp_counter, lifting,old_lin_subspace_half,pointed);

    return New_Basis_Max_Subspace;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::hilbert_basis_dual(){

    assert(dim>0);         
    if (verbose==true) {
        verboseOutput()<<"************************************************************\n";
        verboseOutput()<<"computing Hilbert basis";
        if(truncate)
            verboseOutput() << " (truncated)";
        verboseOutput() << " ..." << endl;
    }
    
    if(Generators.nr_of_rows()!=ExtremeRays.size()){
        errorOutput() << "Mismatch of extreme rays and generators in cone dual mode. THIS SHOULD NOT HAPPEN." << endl;
        throw FatalException(); 
    }
    
    size_t hyp_counter;      // current hyperplane
    Matrix<Integer> Basis_Max_Subspace(dim);      //identity matrix
    for (hyp_counter = 0; hyp_counter < nr_sh; hyp_counter++) {
        Basis_Max_Subspace=cut_with_halfspace(hyp_counter,Basis_Max_Subspace);
    }
    
    if(ExtremeRays.size()==0){  // no precomputed generators
        extreme_rays_rank();
        relevant_support_hyperplanes();
        ExtremeRayList.clear();
        
    }
    else{  // must produce the relevant support hyperplanes from the generators
           // since the Hilbert basis may have been truncated
        vector<Integer> test(SupportHyperplanes.nr_of_rows());
        vector<key_t> key;
        vector <key_t> relevant_sh;
        size_t realdim=Generators.rank();
        for(key_t h=0;h<SupportHyperplanes.nr_of_rows();++h){
            key.clear();
            vector<Integer> test=Generators.MxV(SupportHyperplanes[h]);
            for(key_t i=0;i<test.size();++i)
                if(test[i]==0)
                    key.push_back(i);
            if (key.size() >= realdim-1 && Generators.submatrix(key).rank() >= realdim-1)
                relevant_sh.push_back(h);
        }    
        SupportHyperplanes = SupportHyperplanes.submatrix(relevant_sh);
    }
        
    /* if(verbose)
       verboseOutput() << "matches = " << counter << endl << "avoided = " << counter1 << endl << 
            "comparisons = " << redcounter << endl << "comp/match " << (float) redcounter/(float) counter << endl;
       // verboseOutput() << "matches = " << counter << endl << "avoided = " << counter1 << endl; //  << "add avoided " << counter2 << endl;
    */
       
    Intermediate_HB.extract(Hilbert_Basis);
    
    if(verbose) {
        verboseOutput() << "Hilbert basis ";
        if(truncate)
            verboseOutput() << "(truncated) ";
        verboseOutput() << Hilbert_Basis.size() << endl;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::extreme_rays_rank(){
    if (verbose) {
        verboseOutput() << "Find extreme rays" << endl;
    }
    
    typename list < Candidate <Integer> >::iterator c;
    vector <key_t> zero_list;
    size_t i,k;
    for (c=Intermediate_HB.Candidates.begin(); c!=Intermediate_HB.Candidates.end(); ++c){
        zero_list.clear();
        for (i = 0; i < nr_sh; i++) {
            if(c->values[i]==0) {
                zero_list.push_back(i);
            }
        }
        k=zero_list.size();
        if (k>=dim-1) {

            // Matrix<Integer> Test=SupportHyperplanes.submatrix(zero_list);
            if (SupportHyperplanes.rank_submatrix(zero_list)>=dim-1) {
                ExtremeRayList.push_back(&(*c));
            }
        }
    }
    size_t s = ExtremeRayList.size();
    Generators = Matrix<Integer>(s,dim);
   
    typename  list< Candidate<Integer>* >::const_iterator l;
    for (i=0, l=ExtremeRayList.begin(); l != ExtremeRayList.end(); ++l, ++i) {
        Generators[i]= (*l)->cand;
    }
    ExtremeRays=vector<bool>(s,true);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::relevant_support_hyperplanes(){
    if (verbose) {
        verboseOutput() << "Find relevant support hyperplanes" << endl;
    }
    list <key_t> zero_list;
    typename list<Candidate<Integer>* >::iterator gen_it;
    vector <key_t> relevant_sh;
    relevant_sh.reserve(nr_sh);
    size_t i,k;
    
    size_t realdim = Generators.rank();

    for (i = 0; i < nr_sh; ++i) {
        Matrix<Integer> Test(0,dim);
        k = 0;
        for (gen_it = ExtremeRayList.begin(); gen_it != ExtremeRayList.end(); ++gen_it) {
            if ((*gen_it)->values[i]==0) {
                Test.append((*gen_it)->cand);
                k++;
            }
        }
        if (k >= realdim-1 && Test.rank()>=realdim-1) {
            relevant_sh.push_back(i);
        }
    }
    SupportHyperplanes = SupportHyperplanes.submatrix(relevant_sh);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::to_sublattice(const Sublattice_Representation<Integer>& SR) {
    assert(SR.getDim() == dim);

    dim = SR.getRank();
    // hyp_size = dim+nr_sh;
    SupportHyperplanes = SR.to_sublattice_dual(SupportHyperplanes);
    typename list<vector<Integer> >::iterator it;
    vector<Integer> tmp;
    
    Generators = SR.to_sublattice(Generators);

    for (it = Hilbert_Basis.begin(); it != Hilbert_Basis.end(); ) {
        tmp = SR.to_sublattice(*it);
        it = Hilbert_Basis.erase(it);
        Hilbert_Basis.insert(it,tmp);
    }
}

} //end namespace libnormaliz
