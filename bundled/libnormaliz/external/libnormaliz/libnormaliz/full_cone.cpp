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
#include <set>
#include <map>
#include <iostream>
#include <string>
#include <algorithm>
#include <time.h>
#include <deque>

#include "libnormaliz/full_cone.h"
#include "libnormaliz/cone_helper.h"
#include "libnormaliz/vector_operations.h"
#include "libnormaliz/list_operations.h"
#include "libnormaliz/map_operations.h"
#include "libnormaliz/my_omp.h"
#include "libnormaliz/integer.h"
// #include "libnormaliz/sublattice_representation.h"
#include "libnormaliz/offload_handler.h"

//---------------------------------------------------------------------------

const size_t RecBoundTriang=1000000;   //  if number(supphyps)*size(triang) > RecBoundTriang
                                       // we pass to (non-recirsive) pyramids

const size_t EvalBoundTriang=2500000; // if more than EvalBoundTriang simplices have been stored
                               // evaluation is started (whenever possible)

const size_t EvalBoundPyr=200000;   // the same for stored pyramids of level > 0

const size_t EvalBoundLevel0Pyr=200000; // 1000000;   // the same for stored level 0 pyramids

// const size_t EvalBoundRecPyr=200000;   // the same for stored RECURSIVE pyramids

// const size_t IntermedRedBoundHB=2000000;  // bound for number of HB elements before 
                                              // intermediate reduction is called
                                              
const int largePyramidFactor=20;  // pyramid is large if largePyramidFactor*Comparisons[Pyramid_key.size()-dim] > old_nr_supp_hyps

const int SuppHypRecursionFactor=100; // pyramids for supphyps formed if Pos*Neg > this factor*dim^4

const size_t RAM_Size=1000000000; // we assume that there is at least 1 GB of RAM

//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------
//private
//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::check_simpliciality_hyperplane(const FACETDATA& hyp) const{
    size_t nr_gen_in_hyp=0;
    for(size_t i=0; i<nr_gen;++i)
        if(in_triang[i]&& hyp.GenInHyp.test(i))
            nr_gen_in_hyp++;
    if((hyp.simplicial &&  nr_gen_in_hyp!=dim-2) || (!hyp.simplicial &&  nr_gen_in_hyp==dim-2)){
        // NOTE: in_triang set at END of main loop in build_cone
        cout << "Simplicial " << hyp.simplicial << " dim " << dim << " gen_in_hyp " << nr_gen_in_hyp << endl;
        assert(false);
    }
}

template<typename Integer>
void Full_Cone<Integer>::set_simplicial(FACETDATA& hyp){
    size_t nr_gen_in_hyp=0;
    for(size_t i=0; i<nr_gen;++i)
        if(in_triang[i]&& hyp.GenInHyp.test(i))
            nr_gen_in_hyp++;
    hyp.simplicial=(nr_gen_in_hyp==dim-2);
}

template<typename Integer>
void Full_Cone<Integer>::number_hyperplane(FACETDATA& hyp, const size_t born_at, const size_t mother){
// add identifying number, the birth day and the number of mother 

    hyp.Mother=mother;
    hyp.BornAt=born_at;
    if(!multithreaded_pyramid){
        hyp.Ident=HypCounter[0];
        HypCounter[0]++;
        return;
    }
    
    int tn;
    if(omp_get_level()==0)
        tn=0;
    else    
        tn = omp_get_ancestor_thread_num(1);
    hyp.Ident=HypCounter[tn];
    HypCounter[tn]+=omp_get_max_threads();
    
}

//---------------------------------------------------------------------------

template<typename Integer>
bool Full_Cone<Integer>::is_hyperplane_included(FACETDATA& hyp) {
    if (!is_pyramid) { // in the topcone we always have ov_sp > 0
        return true;
    }
    //check if it would be an excluded hyperplane
    Integer ov_sp = v_scalar_product(hyp.Hyp,Order_Vector);
    if (ov_sp > 0) {
        return true;
    } else if (ov_sp == 0) {
        for (size_t i=0; i<dim; i++) {
            if (hyp.Hyp[i]>0) {
                return true;
            } else if (hyp.Hyp[i]<0) {
                return false;
            }
        }
    }
    return false;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::add_hyperplane(const size_t& new_generator, const FACETDATA & positive,const FACETDATA & negative,
                            list<FACETDATA>& NewHyps, bool known_to_be_simplicial){
// adds a new hyperplane found in find_new_facets to this cone (restricted to generators processed)

    size_t k;
    
    FACETDATA NewFacet; NewFacet.Hyp.resize(dim); NewFacet.GenInHyp.resize(nr_gen);        
    
    for (k = 0; k <dim; k++) {
        NewFacet.Hyp[k]=positive.ValNewGen*negative.Hyp[k]-negative.ValNewGen*positive.Hyp[k];
        if(!check_range(NewFacet.Hyp[k]))
            break;    
    }
    
    if(k==dim)
        v_make_prime(NewFacet.Hyp);
    else{
        #pragma omp atomic
        GMP_hyp++;
        vector<mpz_class> mpz_neg(dim), mpz_pos(dim), mpz_sum(dim);
        convert(mpz_neg, negative.Hyp);
        convert(mpz_pos, positive.Hyp);
        for (k = 0; k <dim; k++)
            mpz_sum[k]=convertTo<mpz_class>(positive.ValNewGen)*mpz_neg[k]-convertTo<mpz_class>(negative.ValNewGen)*mpz_pos[k];
        v_make_prime(mpz_sum);
        convert(NewFacet.Hyp, mpz_sum);
    }
    
    NewFacet.ValNewGen=0;    
    NewFacet.GenInHyp=positive.GenInHyp & negative.GenInHyp; // new hyperplane contains old gen iff both pos and neg do
    if(known_to_be_simplicial){
        NewFacet.simplicial=true;
        check_simpliciality_hyperplane(NewFacet);
    }
    else
        set_simplicial(NewFacet);
    NewFacet.GenInHyp.set(new_generator);  // new hyperplane contains new generator
    number_hyperplane(NewFacet,nrGensInCone,positive.Ident);
    
    NewHyps.push_back(NewFacet);
}


//---------------------------------------------------------------------------


template<typename Integer>
void Full_Cone<Integer>::find_new_facets(const size_t& new_generator){
// our Fourier-Motzkin implementation
// the special treatment of simplicial facets was inserted because of line shellings.
// At present these are not computed.

    //to see if possible to replace the function .end with constant iterator since push-back is performed.

    // for dimension 0 and 1 F-M is never necessary and can lead to problems
    // when using dim-2
    if (dim <= 1)
        return;

    // NEW: new_generator is the index of the generator being inserted

    size_t i,k,nr_zero_i;
    size_t subfacet_dim=dim-2; // NEW dimension of subfacet
    size_t facet_dim=dim-1; // NEW dimension of facet
    
    const bool tv_verbose = false; //verbose && !is_pyramid; // && Support_Hyperplanes.nr_of_rows()>10000; //verbose in this method call
    
        
    // preparing the computations, the various types of facets are sorted into the deques
    deque <FACETDATA*> Pos_Simp,Pos_Non_Simp;
    deque <FACETDATA*> Neg_Simp,Neg_Non_Simp;
    deque <FACETDATA*> Neutral_Simp, Neutral_Non_Simp;
    
    boost::dynamic_bitset<> Zero_Positive(nr_gen),Zero_Negative(nr_gen); // here we collect the vertices that lie in a
                                        // postive resp. negative hyperplane

    bool simplex;
    
    if (tv_verbose) verboseOutput()<<"transform_values:"<<flush;
    
    typename list<FACETDATA>::iterator ii = Facets.begin();
    
    for (; ii != Facets.end(); ++ii) {
        // simplex=true;
        // nr_zero_i=0;
        simplex=ii->simplicial; // at present simplicial, will become nonsimplicial if neutral
        /* for (size_t j=0; j<nr_gen; j++){
            if (ii->GenInHyp.test(j)) {
                if (++nr_zero_i > facet_dim) {
                    simplex=false;
                    break;
                }
            }
        }*/
        
        if (ii->ValNewGen==0) {
            ii->GenInHyp.set(new_generator);  // Must be set explicitly !!
            ii->simplicial=false;  // simpliciality definitly gone with the new generator
            if (simplex) {
                Neutral_Simp.push_back(&(*ii)); // simplicial without the new generator
            }   else {
                Neutral_Non_Simp.push_back(&(*ii)); // nonsim¸plicial already without the new generator
            }
        }
        else if (ii->ValNewGen>0) {
            Zero_Positive |= ii->GenInHyp;
            if (simplex) {
                Pos_Simp.push_back(&(*ii));
            } else {
                Pos_Non_Simp.push_back(&(*ii));
            }
        } 
        else if (ii->ValNewGen<0) {
            Zero_Negative |= ii->GenInHyp;
            if (simplex) {
                Neg_Simp.push_back(&(*ii));
            } else {
                Neg_Non_Simp.push_back(&(*ii));
            }
        }
    }
    
    // TO DO: Negativliste mit Zero_Positive verfeinern, also die aussondern, die nicht genug positive Erz enthalten
    // Eventuell sogar Rang-Test einbauen.
    // Letzteres k√∂nnte man auch bei den positiven machen, bevor sie verarbeitet werden
    
    boost::dynamic_bitset<> Zero_PN(nr_gen);
    Zero_PN = Zero_Positive & Zero_Negative;
    
    size_t nr_PosSimp  = Pos_Simp.size();
    size_t nr_PosNonSimp = Pos_Non_Simp.size();
    size_t nr_NegSimp  = Neg_Simp.size();
    size_t nr_NegNonSimp = Neg_Non_Simp.size();
    size_t nr_NeuSimp  = Neutral_Simp.size();
    size_t nr_NeuNonSimp = Neutral_Non_Simp.size();
    
    if (tv_verbose) verboseOutput()<<" PS "<<nr_PosSimp<<", P "<<nr_PosNonSimp<<", NS "<<nr_NegSimp<<", N "<<nr_NegNonSimp<<", ZS "<<nr_NeuSimp<<", Z "<<nr_NeuNonSimp<<endl;

    if (tv_verbose) verboseOutput()<<"transform_values: subfacet of NS: "<<flush;
    
    vector< list<pair < boost::dynamic_bitset<>, int> > > Neg_Subfacet_Multi(omp_get_max_threads()) ;

    boost::dynamic_bitset<> zero_i, subfacet;

    // This parallel region cannot throw a NormalizException
    #pragma omp parallel for private(zero_i,subfacet,k,nr_zero_i)
    for (i=0; i<nr_NegSimp;i++){
        zero_i=Zero_PN & Neg_Simp[i]->GenInHyp;
        
        nr_zero_i=0;
        for(size_t j=0;j<nr_gen;j++){
            if(zero_i.test(j))
                nr_zero_i++;
            if(nr_zero_i>subfacet_dim){
                break;
            }
        }

        if(nr_zero_i==subfacet_dim) // NEW This case treated separately
            Neg_Subfacet_Multi[omp_get_thread_num()].push_back(pair <boost::dynamic_bitset<>, int> (zero_i,i));
            
        if(nr_zero_i==facet_dim){
            for (k =0; k<nr_gen; k++) {  
                if(zero_i.test(k)) {              
                    subfacet=zero_i;
                    subfacet.reset(k);  // remove k-th element from facet to obtain subfacet
                    Neg_Subfacet_Multi[omp_get_thread_num()].push_back(pair <boost::dynamic_bitset<>, int> (subfacet,i));
                }
            }
        }
    }
    
    list < pair < boost::dynamic_bitset<>, int> > Neg_Subfacet_Multi_United;
    for(int i=0;i<omp_get_max_threads();++i)
        Neg_Subfacet_Multi_United.splice(Neg_Subfacet_Multi_United.begin(),Neg_Subfacet_Multi[i]);
    Neg_Subfacet_Multi_United.sort();


    if (tv_verbose) verboseOutput()<<Neg_Subfacet_Multi_United.size() << ", " << flush;

    list< pair < boost::dynamic_bitset<>, int > >::iterator jj;
    list< pair < boost::dynamic_bitset<>, int > >::iterator del;
    jj =Neg_Subfacet_Multi_United.begin();           // remove negative subfacets shared
    while (jj!= Neg_Subfacet_Multi_United.end()) {   // by two neg simpl facets
        del=jj++;
        if (jj!=Neg_Subfacet_Multi_United.end() && (*jj).first==(*del).first) {   //delete since is the intersection of two negative simplicies
            Neg_Subfacet_Multi_United.erase(del);
            del=jj++;
            Neg_Subfacet_Multi_United.erase(del);
        }
    }

    size_t nr_NegSubfMult = Neg_Subfacet_Multi_United.size();
    if (tv_verbose) verboseOutput() << nr_NegSubfMult << ", " << flush;
    
    vector<list<FACETDATA> > NewHypsSimp(nr_PosSimp);
    vector<list<FACETDATA> > NewHypsNonSimp(nr_PosNonSimp);

    map < boost::dynamic_bitset<>, int > Neg_Subfacet;
    size_t nr_NegSubf=0;
    
    // size_t NrMatches=0, NrCSF=0, NrRank=0, NrComp=0, NrNewF=0;
    
    /* deque<bool> Indi(nr_NegNonSimp);
    for(size_t j=0;j<nr_NegNonSimp;++j)
        Indi[j]=false; */
        
    if(multithreaded_pyramid){
        #pragma omp atomic
        nrTotalComparisons+=nr_NegNonSimp*nr_PosNonSimp;
    }
    else{
        nrTotalComparisons+=nr_NegNonSimp*nr_PosNonSimp; 
    } 

    
//=====================================================================
// parallel from here

    bool skip_remaining = false;
#ifndef NCATCH
    std::exception_ptr tmp_exception;
#endif

    #pragma omp parallel private(jj)
    {
    size_t i,j,k,nr_zero_i;
    boost::dynamic_bitset<> subfacet(dim-2);
    jj = Neg_Subfacet_Multi_United.begin();
    size_t jjpos=0;
    int tn = omp_get_ancestor_thread_num(1);

    bool found;
    // This for region cannot throw a NormalizException
    #pragma omp for schedule(dynamic)
    for (size_t j=0; j<nr_NegSubfMult; ++j) {  // remove negative subfacets shared
        for(;j > jjpos; ++jjpos, ++jj) ;       // by non-simpl neg or neutral facets 
        for(;j < jjpos; --jjpos, --jj) ;

        subfacet=(*jj).first;
        found=false; 
        for (i = 0; i <nr_NeuSimp; i++) {
            found=subfacet.is_subset_of(Neutral_Simp[i]->GenInHyp);
            if(found)
                break;
        }
        if (!found) {
            for (i = 0; i <nr_NeuNonSimp; i++) {
                found=subfacet.is_subset_of(Neutral_Non_Simp[i]->GenInHyp);
                if(found)
                    break;                    
            }
            if(!found) {
                for (i = 0; i <nr_NegNonSimp; i++) {
                    found=subfacet.is_subset_of(Neg_Non_Simp[i]->GenInHyp);
                    if(found)
                        break; 
                }
            }
        }
        if (found) {
            jj->second=-1;
        }
    }
    
    #pragma omp single
    { //remove elements that where found in the previous loop
    jj = Neg_Subfacet_Multi_United.begin();
    map < boost::dynamic_bitset<>, int > ::iterator last_inserted=Neg_Subfacet.begin(); // used to speedup insertion into the new map
    for (; jj!= Neg_Subfacet_Multi_United.end(); ++jj) {
        if ((*jj).second != -1) {
            last_inserted = Neg_Subfacet.insert(last_inserted,*jj);
        }
    }
    nr_NegSubf=Neg_Subfacet.size();
    }
    
    #pragma omp single nowait
    {Neg_Subfacet_Multi_United.clear();}

    
    boost::dynamic_bitset<> zero_i(nr_gen);
    map <boost::dynamic_bitset<>, int> ::iterator jj_map;

    
    #pragma omp single nowait
    if (tv_verbose) {
        verboseOutput() << "PS vs NS and PS vs N , " << flush;
    }

    vector<key_t> key(nr_gen);
    size_t nr_missing;
    bool common_subfacet;
    // we cannot use nowait here because of the way we handle exceptions in this loop
    #pragma omp for schedule(dynamic) //nowait
    for (size_t i =0; i<nr_PosSimp; i++){

        if (skip_remaining) continue;
#ifndef NCATCH
        try {
#endif
        zero_i=Zero_PN & Pos_Simp[i]->GenInHyp;
        nr_zero_i=0;
        for(j=0;j<nr_gen && nr_zero_i<=facet_dim;j++)
            if(zero_i.test(j)){
                key[nr_zero_i]=j;
                nr_zero_i++;
            } 
            
        if(nr_zero_i<subfacet_dim)
            continue;
            
        // first PS vs NS
        
        if (nr_zero_i==subfacet_dim) {                 // NEW slight change in logic. Positive simpl facet shared at most
            jj_map=Neg_Subfacet.find(zero_i);           // one subfacet with negative simpl facet
            if (jj_map!=Neg_Subfacet.end()) {
                add_hyperplane(new_generator,*Pos_Simp[i],*Neg_Simp[(*jj_map).second],NewHypsSimp[i],true);
                (*jj_map).second = -1;  // block subfacet in further searches
            }
        }
        if (nr_zero_i==facet_dim){    // now there could be more such subfacets. We make all and search them.      
            for (k =0; k<nr_gen; k++) {  // BOOST ROUTINE
                if(zero_i.test(k)) { 
                    subfacet=zero_i;
                    subfacet.reset(k);  // remove k-th element from facet to obtain subfacet
                    jj_map=Neg_Subfacet.find(subfacet);
                    if (jj_map!=Neg_Subfacet.end()) {
                        add_hyperplane(new_generator,*Pos_Simp[i],*Neg_Simp[(*jj_map).second],NewHypsSimp[i],true);
                        (*jj_map).second = -1;
                        // Indi[j]=true;
                    }
                }
            }
        }

        // now PS vs N

       for (j=0; j<nr_NegNonSimp; j++){ // search negative facet with common subfacet
           nr_missing=0; 
           common_subfacet=true;               
           for(k=0;k<nr_zero_i;k++) {
               if(!Neg_Non_Simp[j]->GenInHyp.test(key[k])) {
                   nr_missing++;
                   if(nr_missing==2 || nr_zero_i==subfacet_dim) {
                       common_subfacet=false;
                       break;
                   }
               }
            }
               
            if(common_subfacet){                 
               add_hyperplane(new_generator,*Pos_Simp[i],*Neg_Non_Simp[j],NewHypsSimp[i],true);
               if(nr_zero_i==subfacet_dim) // only one subfacet can lie in negative hyperplane
                   break;
            }
       }
#ifndef NCATCH
       } catch(const std::exception& ) {
           tmp_exception = std::current_exception();
           skip_remaining = true;
           #pragma omp flush(skip_remaining)
       }
#endif

    } // PS vs NS and PS vs N

    if (!skip_remaining) {
    #pragma omp single nowait
    if (tv_verbose) {
        verboseOutput() << "P vs NS and P vs N" << endl;
    }

    list<FACETDATA*> AllNonSimpHyp;
    typename list<FACETDATA*>::iterator a;

    for(i=0;i<nr_PosNonSimp;++i)
        AllNonSimpHyp.push_back(&(*Pos_Non_Simp[i]));
    for(i=0;i<nr_NegNonSimp;++i)
        AllNonSimpHyp.push_back(&(*Neg_Non_Simp[i]));
    for(i=0;i<nr_NeuNonSimp;++i)
        AllNonSimpHyp.push_back(&(*Neutral_Non_Simp[i])); 
    size_t nr_NonSimp = nr_PosNonSimp+nr_NegNonSimp+nr_NeuNonSimp;
   
    bool ranktest;
    FACETDATA *hp_i, *hp_j, *hp_t; // pointers to current hyperplanes
    
    size_t missing_bound, nr_common_zero;
    boost::dynamic_bitset<> common_zero(nr_gen);
    vector<key_t> common_key;
    common_key.reserve(nr_gen);
    vector<int> key_start(nrGensInCone);
    
    #pragma omp for schedule(dynamic) // nowait
    for (size_t i =0; i<nr_PosNonSimp; i++){ //Positive Non Simp vs.Negative Simp and Non Simp

        if (skip_remaining) continue;

#ifndef NCATCH
        try {
#endif
        jj_map = Neg_Subfacet.begin();       // First the Simp
        for (j=0; j<nr_NegSubf; ++j,++jj_map) {
            if ( (*jj_map).second != -1 ) {  // skip used subfacets
                if(jj_map->first.is_subset_of(Pos_Non_Simp[i]->GenInHyp)){
                    add_hyperplane(new_generator,*Pos_Non_Simp[i],*Neg_Simp[(*jj_map).second],NewHypsNonSimp[i],true);
                    (*jj_map).second = -1; // has now been used
                }
            }
        }
        
        // Now the NonSimp

        hp_i=Pos_Non_Simp[i];
        zero_i=Zero_PN & hp_i->GenInHyp; // these are the potential vertices in an intersection
        nr_zero_i=0;
        int last_existing=-1;
        for(size_t jj=0;jj<nrGensInCone;jj++) // we make a "key" of the potential vertices in the intersection
        {
            j=GensInCone[jj];
            if(zero_i.test(j)){
                key[nr_zero_i]=j;
                for(size_t kk= last_existing+1;kk<=jj;kk++)  // used in the extension test
                    key_start[kk]=nr_zero_i;                 // to find out from which generator on both have existed
                nr_zero_i++;
                last_existing= jj;
            }
        }
        if(last_existing< (int)nrGensInCone-1)
            for(size_t kk=last_existing+1;kk<nrGensInCone;kk++)
                key_start[kk]=nr_zero_i;
                
        if (nr_zero_i<subfacet_dim) 
            continue;
        
        // now nr_zero_i is the number of vertices in hp_i that have a chance to lie in a negative facet
        // and key contains the indices
        
       missing_bound=nr_zero_i-subfacet_dim; // at most this number of generators can be missing
                                             // to have a chance for common subfacet                                            
       
       for (j=0; j<nr_NegNonSimp; j++){
    
        
           hp_j=Neg_Non_Simp[j];
           
           if(hp_i->Ident==hp_j->Mother || hp_j->Ident==hp_i->Mother){   // mother and daughter coming together
               add_hyperplane(new_generator,*hp_i,*hp_j,NewHypsNonSimp[i],false);  // their intersection is a subfacet
               continue;                                                           // simplicial set in add_hyperplane
           } 
           
           
           bool extension_test=hp_i->BornAt==hp_j->BornAt || (hp_i->BornAt<hp_j->BornAt && hp_j->Mother!=0)
                                                          || (hp_j->BornAt<hp_i->BornAt && hp_i->Mother!=0);
                                                          
           // extension_test=false;
                                                          
           size_t both_existing_from=key_start[max(hp_i->BornAt,hp_j->BornAt)];
                      
           nr_missing=0; 
           nr_common_zero=0;
           common_key.clear();
           size_t second_loop_bound=nr_zero_i;
           common_subfacet=true;
           
           // We use the following criterion:
           // if the two facets are not mother and daughter (taken care of already), then
           // they cannot have intersected in a subfacet at the time when the second was born.
           // In other words: they can only intersect in a subfacet now, if at least one common vertex
           // has been added after the birth of the younger one.
           // this is indicated by "extended".
           
           if(extension_test){
               bool extended=false;
               second_loop_bound=both_existing_from;  // fisrt we find the common vertices inserted from the step
                                                      // where both facets existed the first time
               for(k=both_existing_from;k<nr_zero_i;k++){
                   if(!hp_j->GenInHyp.test(key[k])) {
                       nr_missing++;
                       if(nr_missing>missing_bound) {
                           common_subfacet=false;
                           break;
                       }
                   }
                   else {
                       extended=true;  // in this case they have a common vertex added after their common existence
                       common_key.push_back(key[k]);
                       nr_common_zero++;
                   }
               }

               if(!extended || !common_subfacet) // 
                   continue;
           }
                    
           
           for(k=0;k<second_loop_bound;k++) {  // now the remaining 
               if(!hp_j->GenInHyp.test(key[k])) {
                   nr_missing++;
                   if(nr_missing>missing_bound) {
                       common_subfacet=false;
                       break;
                   }
               }
               else {
                   common_key.push_back(key[k]);
                   nr_common_zero++;
               }
            }
            
           if(!common_subfacet)
                continue;
           /* #pragma omp atomic
           NrCSF++;*/
           
           if(using_GMP<Integer>())           
                ranktest = (nr_NonSimp > 10*dim*dim*nr_common_zero/3); // in this case the rank computation takes longer
           else
               ranktest = (nr_NonSimp > dim*dim*nr_common_zero/3);

           if(ranktest) {
           
           /* #pragma omp atomic
            NrRank++; */
            
               Matrix<Integer>& Test = Top_Cone->RankTest[tn];
               if (Test.rank_submatrix(Generators,common_key)<subfacet_dim) {
                   common_subfacet=false;
               }
           } // ranktest
           else{                 // now the comparison test
           
           /* #pragma omp atomic
            NrComp++; */
            
               common_zero = zero_i & hp_j->GenInHyp;
               for (a=AllNonSimpHyp.begin();a!=AllNonSimpHyp.end();++a){
                   hp_t=*a;
                   if ((hp_t!=hp_i) && (hp_t!=hp_j) && common_zero.is_subset_of(hp_t->GenInHyp)) {                                
                       common_subfacet=false;
                       AllNonSimpHyp.splice(AllNonSimpHyp.begin(),AllNonSimpHyp,a); // for the "darwinistic" mewthod
                       break;
                   }
               }                       
           } // else
           if (common_subfacet) {  //intersection of i and j is a subfacet
               add_hyperplane(new_generator,*hp_i,*hp_j,NewHypsNonSimp[i],false); //simplicial set in add_hyperplane
               /* #pragma omp atomic
                NrNewF++; */
                // Indi[j]=true;
           }
        }
#ifndef NCATCH
        } catch(const std::exception& ) {
            tmp_exception = std::current_exception();
            skip_remaining = true;
            #pragma omp flush(skip_remaining)
        }
#endif
    } // end for
    } // end !skip_remaining
    } //END parallel
    
#ifndef NCATCH
    if (!(tmp_exception == 0)) std::rethrow_exception(tmp_exception);
#endif
//=====================================================================
// parallel until here


    /* if(!is_pyramid)
      cout << "Matches " << NrMatches << " pot. common subf " << NrCSF << " rank test " <<  NrRank << " comp test "
        << NrComp << " neww hyps " << NrNewF << endl; */


    for(i=0;i<nr_PosSimp;i++)
        Facets.splice(Facets.end(),NewHypsSimp[i]);

    for(i=0;i<nr_PosNonSimp;i++)
        Facets.splice(Facets.end(),NewHypsNonSimp[i]);

    //removing the negative hyperplanes
    // now done in build_cone

    if (tv_verbose) verboseOutput()<<"transform_values: done"<<endl;
    
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::extend_triangulation(const size_t& new_generator){
// extends the triangulation of this cone by including new_generator
// simplicial facets save us from searching the "brother" in the existing triangulation
// to which the new simplex gets attached

    size_t listsize =old_nr_supp_hyps; // Facets.size();
    vector<typename list<FACETDATA>::iterator> visible;
    visible.reserve(listsize);
    typename list<FACETDATA>::iterator i = Facets.begin();

    listsize=0;
    for (; i!=Facets.end(); ++i) 
        if (i->ValNewGen < 0){ // visible facet
            visible.push_back(i);
            listsize++;
        }

#ifndef NCATCH
    std::exception_ptr tmp_exception;
#endif

    typename list< SHORTSIMPLEX<Integer> >::iterator oldTriBack = --TriangulationBuffer.end();
    #pragma omp parallel private(i)
    {
    size_t k,l;
    bool one_not_in_i, not_in_facet;
    size_t not_in_i=0;
    // size_t facet_dim=dim-1;
    // size_t nr_in_i=0;

    list< SHORTSIMPLEX<Integer> > Triangulation_kk;
    typename list< SHORTSIMPLEX<Integer> >::iterator j;
    
    vector<key_t> key(dim);
    
    // if we only want a partial triangulation but came here because of a deep level
    // mark if this part of the triangulation has not to be evaluated
    bool skip_eval = false;

    #pragma omp for schedule(dynamic)
    for (size_t kk=0; kk<listsize; ++kk) {

#ifndef NCATCH
    try {
#endif
        i=visible[kk];
        
        /* nr_in_i=0;
        for(size_t m=0;m<nr_gen;m++){
            if(i->GenInHyp.test(m))
                nr_in_i++;
            if(nr_in_i>facet_dim){
                break;
            }
        }*/
        
        skip_eval = Top_Cone->do_partial_triangulation && i->ValNewGen == -1
                    && is_hyperplane_included(*i);

        if (i->simplicial){  // simplicial
            l=0;
            for (k = 0; k <nr_gen; k++) {
                if (i->GenInHyp[k]==1) {
                    key[l]=k;
                    l++;
                }
            }
            key[dim-1]=new_generator;
 
           if (skip_eval)
                store_key(key,0,0,Triangulation_kk);
            else
                store_key(key,-i->ValNewGen,0,Triangulation_kk);
            continue;
        } // end simplicial
        
        size_t irrelevant_vertices=0;
        for(size_t vertex=0;vertex<nrGensInCone;++vertex){
        
            if(i->GenInHyp[GensInCone[vertex]]==0) // lead vertex not in hyperplane
                continue;
                
            if(irrelevant_vertices<dim-2){
                ++irrelevant_vertices;
                continue;
            }       
        
            j=TriSectionFirst[vertex];
            bool done=false;
            for(;!done;j++)
            {
              done=(j==TriSectionLast[vertex]);
              key=j->key;
              one_not_in_i=false;  // true indicates that one gen of simplex is not in hyperplane
              not_in_facet=false;  // true indicates that a second gen of simplex is not in hyperplane
              for(k=0;k<dim;k++){
                 if ( !i->GenInHyp.test(key[k])) {
                     if(one_not_in_i){
                         not_in_facet=true;
                         break;
                     }
                     one_not_in_i=true;
                     not_in_i=k;
                  }
              }
              
              if(not_in_facet) // simplex does not share facet with hyperplane
                 continue;
              
              key[not_in_i]=new_generator;              
              if (skip_eval)
                  store_key(key,0,j->vol,Triangulation_kk);
              else
                  store_key(key,-i->ValNewGen,j->vol,Triangulation_kk);
                       
            } // j
            
        } // for vertex

#ifndef NCATCH
        } catch(const std::exception& ) {
            tmp_exception = std::current_exception();
        }
#endif

    } // omp for kk

    if (multithreaded_pyramid) {
        #pragma omp critical(TRIANG)
        TriangulationBuffer.splice(TriangulationBuffer.end(),Triangulation_kk);
    } else
        TriangulationBuffer.splice(TriangulationBuffer.end(),Triangulation_kk);

    } // parallel

#ifndef NCATCH
    if (!(tmp_exception == 0)) std::rethrow_exception(tmp_exception);
#endif

    // GensInCone.push_back(new_generator); // now in extend_cone
    TriSectionFirst.push_back(++oldTriBack);
    TriSectionLast.push_back(--TriangulationBuffer.end());
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::store_key(const vector<key_t>& key, const Integer& height,
            const Integer& mother_vol, list< SHORTSIMPLEX<Integer> >& Triangulation){
// stores a simplex given by key and height in Triangulation
// mother_vol is the volume of the simplex to which the new one is attached

    SHORTSIMPLEX<Integer> newsimplex;
    newsimplex.key=key;
    newsimplex.height=height;
    newsimplex.vol=0;
    
    if(multithreaded_pyramid){
        #pragma omp atomic
        TriangulationBufferSize++;
    }
    else {
        TriangulationBufferSize++;
    }
    int tn;
    if(omp_get_level()==0)
        tn=0;
    else    
        tn = omp_get_ancestor_thread_num(1);
    
    if (do_only_multiplicity) {
        // directly compute the volume
        if (mother_vol==1)
            newsimplex.vol = height;
        // the multiplicity is computed in SimplexEvaluator
        for(size_t i=0; i<dim; i++) // and needs the key in TopCone numbers
            newsimplex.key[i]=Top_Key[newsimplex.key[i]];

        if (keep_triangulation)
            sort(newsimplex.key.begin(),newsimplex.key.end());
        Top_Cone->SimplexEval[tn].evaluate(newsimplex);
        // restore the local generator numbering, needed in extend_triangulation
        newsimplex.key=key;
    }
    if (height == 0) Top_Cone->triangulation_is_partial = true;
    
    if (keep_triangulation){
        Triangulation.push_back(newsimplex);
        return;  
    }
    
    bool Simpl_available=true;

    typename list< SHORTSIMPLEX<Integer> >::iterator F;

    if(Top_Cone->FS[tn].empty()){
        if (Top_Cone->FreeSimpl.empty()) {
            Simpl_available=false;
        } else {
            #pragma omp critical(FREESIMPL)
            {
            if (Top_Cone->FreeSimpl.empty()) {
                Simpl_available=false;
            } else {
                // take 1000 simplices from FreeSimpl or what you can get
                F = Top_Cone->FreeSimpl.begin();
                size_t q;
                for (q = 0; q < 1000; ++q, ++F) {
                    if (F == Top_Cone->FreeSimpl.end())
                        break;
                }

                if(q<1000)
                    Top_Cone->FS[tn].splice(Top_Cone->FS[tn].begin(),
                        Top_Cone->FreeSimpl);
                else
                    Top_Cone->FS[tn].splice(Top_Cone->FS[tn].begin(),
                                  Top_Cone->FreeSimpl,Top_Cone->FreeSimpl.begin(),F);
            } // if empty global (critical)
            } // critical
        } // if empty global
    } // if empty thread

    if (Simpl_available) {
        Triangulation.splice(Triangulation.end(),Top_Cone->FS[tn],
                        Top_Cone->FS[tn].begin());
        Triangulation.back() = newsimplex;
    } else {
        Triangulation.push_back(newsimplex);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::process_pyramids(const size_t new_generator,const bool recursive){

    /*

    We distinguish two types of pyramids:

    (i) recursive pyramids that give their support hyperplanes back to the mother.
    (ii) independent pyramids that are not linked to the mother.

    The parameter "recursive" indicates whether the pyramids that will be created
    in process_pyramid(s) are of type (i) or (ii).

    Every pyramid can create subpyramids of both types (not the case in version 2.8 - 2.10).

    Whether "this" is of type (i) or (ii) is indicated by do_all_hyperplanes.

    The creation of (sub)pyramids of type (i) can be blocked by setting recursion_allowed=false.
    (Not done in this version.)

    is_pyramid==false for the top_cone and ==true else.

    multithreaded_pyramid indicates whether parallelization takes place within the
    computation of a pyramid or whether it is computed in a single thread (defined in build_cone).

    Recursie pyramids are processed immediately after creation (as in 2.8). However, there
    are two exceptions:

    (a) In order to avoid very long waiting times for the computation of the "large" ones,
    these are treated as follows: the support hyperplanes of "this" coming from their bases
    (as negative hyperplanes of "this") are computed by matching them with the
    positive hyperplanes of "this". This Fourier-Motzkin step is much more
    efficient if a pyramid is large. For triangulation a large recursive
    pyramid is then stored as a pyramid of type (ii).

    (b) If "this" is processed in a parallelized loop calling process_pyramids, then
    the loop in process_pyramids cannot be interrupted for the evaluation of simplices. As a
    consequence an extremely long lst of simplices could arise if many small subpyramids of "this"
    are created in process_pyramids. In order to prevent this dangeous effect, small recursive
    subpyramids are stored for later triangulation if the simplex buffer has reached its
    size bound.

    Pyramids of type (ii) are stpred in Pyramids. The store_level of the created pyramids is 0
    for all pyramids created (possibly recursively) from the top cone. Pyramids created
    in evaluate_stored_pyramids get the store level for their subpyramids in that routine and
    transfer it to their recursive daughters. (correction March 4, 2015).

    Note: the top cone has pyr_level=-1. The pyr_level has no algorithmic relevance
    at present, but it shows the depth of the pyramid recursion at which the pyramid has been
    created.

    */


    size_t start_level=omp_get_level(); // allows us to check that we are on level 0
                                        // outside the loop and can therefore call evaluation
                                        // in order to empty the buffers
    vector<key_t> Pyramid_key;
    Pyramid_key.reserve(nr_gen);
    bool skip_triang; // make hyperplanes but skip triangulation (recursive pyramids only)

    deque<bool> done(old_nr_supp_hyps,false);
    bool skip_remaining;
#ifndef NCATCH
    std::exception_ptr tmp_exception;
#endif
    typename list< FACETDATA >::iterator hyp;
    size_t nr_done=0;

    do{  // repeats processing until all hyperplanes have been processed

    hyp=Facets.begin();
    size_t hyppos=0;
    skip_remaining = false;

    #pragma omp parallel for private(skip_triang) firstprivate(hyppos,hyp,Pyramid_key) schedule(dynamic) reduction(+: nr_done)
    for (size_t kk=0; kk<old_nr_supp_hyps; ++kk) {

        if (skip_remaining) continue;
#ifndef NCATCH
        try {
#endif
            for(;kk > hyppos; hyppos++, hyp++) ;
            for(;kk < hyppos; hyppos--, hyp--) ;

            if(done[hyppos])
                continue;

            done[hyppos]=true;

            nr_done++;

            if (hyp->ValNewGen == 0){                   // MUST BE SET HERE
                hyp->GenInHyp.set(new_generator);
                if(recursive) hyp->simplicial=false;                  // in the recursive case
            }

            if (hyp->ValNewGen >= 0) // facet not visible
                continue;

            skip_triang = false;
            if (Top_Cone->do_partial_triangulation && hyp->ValNewGen>=-1) { //ht1 criterion
                skip_triang = is_hyperplane_included(*hyp);
                if (skip_triang) {
                    Top_Cone->triangulation_is_partial = true;
                    if (!recursive) {
                        continue;
                    }
                }
            }

            Pyramid_key.clear(); // make data of new pyramid
            Pyramid_key.push_back(new_generator);
            for(size_t i=0;i<nr_gen;i++){
                if(in_triang[i] && hyp->GenInHyp.test(i)) {
                    Pyramid_key.push_back(i);
                }
            }

            // now we can store the new pyramid at the right place (or finish the simplicial ones)
            if (recursive && skip_triang) { // mark as "do not triangulate"
                process_pyramid(Pyramid_key, new_generator,store_level,0, recursive,hyp,start_level);
            } else { //default
                process_pyramid(Pyramid_key, new_generator,store_level,-hyp->ValNewGen, recursive,hyp,start_level);
            }
            // interrupt parallel execution if it is really parallel
            // to keep the triangulationand pyramid buffers under control
            if (start_level==0) {
                if (check_evaluation_buffer_size() || Top_Cone->check_pyr_buffer(store_level)) {
                    skip_remaining = true;
                }
            }

#ifndef NCATCH
        } catch(const std::exception& ) {
            tmp_exception = std::current_exception();
            skip_remaining = true;
            #pragma omp flush(skip_remaining)
        }
#endif
    } // end parallel loop over hyperplanes

#ifndef NCATCH
    if (!(tmp_exception == 0)) std::rethrow_exception(tmp_exception);
#endif

    if (!omp_in_parallel())
        try_offload(0);
    
    if (start_level==0 && check_evaluation_buffer_size()) {
        Top_Cone->evaluate_triangulation();
    }
    
    if (start_level==0 && Top_Cone->check_pyr_buffer(store_level)) {
        Top_Cone->evaluate_stored_pyramids(store_level);
    }

    } while (nr_done < old_nr_supp_hyps);
    
    evaluate_large_rec_pyramids(new_generator);

}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::process_pyramid(const vector<key_t>& Pyramid_key,
                          const size_t new_generator,const size_t store_level, Integer height, const bool recursive,
                          typename list< FACETDATA >::iterator hyp, size_t start_level){
// processes simplicial pyramids directly, stores other pyramids into their depots

    #pragma omp atomic
    Top_Cone->totalNrPyr++;

    if(Pyramid_key.size()==dim){  // simplicial pyramid completely done here
        #pragma omp atomic        // only for saving memory
        Top_Cone->nrSimplicialPyr++;
        if(recursive){ // the facets may be facets of the mother cone and if recursive==true must be given back
            Matrix<Integer> H(dim,dim);
            Integer dummy_vol;
            Generators.simplex_data(Pyramid_key,H, dummy_vol,false);
            list<FACETDATA> NewFacets;
            FACETDATA NewFacet;
            NewFacet.GenInHyp.resize(nr_gen);
            for (size_t i=0; i<dim;i++) {
                NewFacet.Hyp = H[i];
                NewFacet.GenInHyp.set();
                NewFacet.GenInHyp.reset(i);
                NewFacet.simplicial=true;
                NewFacets.push_back(NewFacet);
            }
            select_supphyps_from(NewFacets,new_generator,Pyramid_key); // takes itself care of multithreaded_pyramid
        }
        if (height != 0 && (do_triangulation || do_partial_triangulation)) {
            if(multithreaded_pyramid) {
#ifndef NCATCH
                std::exception_ptr tmp_exception;
#endif
                #pragma omp critical(TRIANG)
                {
#ifndef NCATCH
                try{
#endif
                    store_key(Pyramid_key,height,0,TriangulationBuffer);
                    nrTotalComparisons+=dim*dim/2;
#ifndef NCATCH
                } catch(const std::exception& ) {
                    tmp_exception = std::current_exception();
                }
#endif
                } // end critical
#ifndef NCATCH
                if (!(tmp_exception == 0)) std::rethrow_exception(tmp_exception);
#endif
            } else {
                store_key(Pyramid_key,height,0,TriangulationBuffer);
                nrTotalComparisons+=dim*dim/2;
            }
        }
    }
    else {  // non-simplicial
    
        bool large=(largePyramidFactor*Comparisons[Pyramid_key.size()-dim] > old_nr_supp_hyps); // Pyramid_key.size()>largePyramidFactor*dim;
        
        if (!recursive || (large && (do_triangulation || do_partial_triangulation) && height!=0) ) {  // must also store for triangulation if recursive and large
            vector<key_t> key_wrt_top(Pyramid_key.size());
            for(size_t i=0;i<Pyramid_key.size();i++)
                key_wrt_top[i]=Top_Key[Pyramid_key[i]];
            #pragma omp critical(STOREPYRAMIDS)
            {
            //      cout << "store_level " << store_level << " large " << large << " pyr level " << pyr_level << endl;
            Top_Cone->Pyramids[store_level].push_back(key_wrt_top);
            Top_Cone->nrPyramids[store_level]++;
            } // critical
            if(!recursive)    // in this case we need only store for future triangulation, and that has been done
                return;
        }
        // now we are in the recursive case and must compute support hyperplanes of the subpyramid
        if(large){  // large recursive pyramid
            if(multithreaded_pyramid){
                #pragma omp critical(LARGERECPYRS)
                LargeRecPyrs.push_back(*hyp);  // LargeRecPyrs are kept and evaluated locally
            }
            else
                LargeRecPyrs.push_back(*hyp);
            return; // done with the large recusive pyramids
        }

        // only recursive small ones left

        Full_Cone<Integer> Pyramid(*this,Pyramid_key);
        Pyramid.Mother = this;
        Pyramid.Mother_Key = Pyramid_key;    // need these data to give back supphyps
        Pyramid.apex=new_generator;
        if (height == 0) { //indicates "do not triangulate"
            Pyramid.do_triangulation = false;
            Pyramid.do_partial_triangulation = false;
            Pyramid.do_Hilbert_basis = false;
            Pyramid.do_deg1_elements=false;
        }

        bool store_for_triangulation=(store_level!=0) //loop in process_pyramids cannot be interrupted
            && (Pyramid.do_triangulation || Pyramid.do_partial_triangulation) // we must (partially) triangulate
            && (start_level!=0 && Top_Cone->TriangulationBufferSize > 2*EvalBoundTriang); // evaluation buffer already full  // EvalBoundTriang

        if (store_for_triangulation) {
            vector<key_t> key_wrt_top(Pyramid_key.size());
            for(size_t i=0;i<Pyramid_key.size();i++)
                key_wrt_top[i]=Top_Key[Pyramid_key[i]];
            #pragma omp critical(STOREPYRAMIDS)
            {
            Top_Cone->Pyramids[store_level].push_back(key_wrt_top);
            Top_Cone->nrPyramids[store_level]++;
            } // critical
            // Now we must suppress immediate triangulation
            Pyramid.do_triangulation = false;
            Pyramid.do_partial_triangulation = false;
            Pyramid.do_Hilbert_basis = false;
            Pyramid.do_deg1_elements=false;
        }

        Pyramid.build_cone();

        if(multithreaded_pyramid){
            #pragma omp atomic
            nrTotalComparisons+=Pyramid.nrTotalComparisons;
        } else
            nrTotalComparisons+=Pyramid.nrTotalComparisons;
    }  // else non-simplicial
}


//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::find_and_evaluate_start_simplex(){

    size_t i,j;
    Integer factor;

    
    /* Simplex<Integer> S = find_start_simplex();
    vector<key_t> key=S.read_key();   // generators indexed from 0 */
    
    vector<key_t> key=find_start_simplex();
    assert(key.size()==dim); // safety heck
    if(verbose){
        verboseOutput() << "Start simplex ";
        for(size_t i=0;i<key.size();++i)
            verboseOutput() <<  key[i]+1 << " ";
        verboseOutput() << endl;
    }
    Matrix<Integer> H(dim,dim);
    Integer vol;
    Generators.simplex_data(key,H,vol,do_partial_triangulation || do_triangulation);
    
    // H.pretty_print(cout);
    
        
    for (i = 0; i < dim; i++) {
        in_triang[key[i]]=true;
        GensInCone.push_back(key[i]);
        if (deg1_triangulation && isComputed(ConeProperty::Grading))
            deg1_triangulation = (gen_degrees[key[i]] == 1);
    }
    
    nrGensInCone=dim;
    
    nrTotalComparisons=dim*dim/2;
    Comparisons.push_back(nrTotalComparisons);
       
    for (i = 0; i <dim; i++) {
        FACETDATA NewFacet; NewFacet.GenInHyp.resize(nr_gen);
        NewFacet.Hyp=H[i];
        NewFacet.simplicial=true; // indeed, the start simplex is simplicial
        for(j=0;j < dim;j++)
            if(j!=i)
                NewFacet.GenInHyp.set(key[j]);
        NewFacet.ValNewGen=-1;         // must be taken negative since opposite facet
        number_hyperplane(NewFacet,0,0); // created with gen 0
        Facets.push_back(NewFacet);    // was visible before adding this vertex
    }
    
    if(!is_pyramid){
        //define Order_Vector, decides which facets of the simplices are excluded
        Order_Vector = vector<Integer>(dim,0);
        // Matrix<Integer> G=S.read_generators();
        for(i=0;i<dim;i++){
            factor=(unsigned long) (1+i%10);  // (2*(rand()%(2*dim))+3);
            for(j=0;j<dim;j++)
                Order_Vector[j]+=factor*Generators[key[i]][j];        
        }
    }

    //the volume is an upper bound for the height
    if(do_triangulation || (do_partial_triangulation && vol>1))
    {
        store_key(key,vol,1,TriangulationBuffer);
        if(do_only_multiplicity) {
            #pragma omp atomic
            TotDet++;
        }
    } else if (do_partial_triangulation) {
        triangulation_is_partial = true;
    }
    
    if(do_triangulation){ // we must prepare the sections of the triangulation
        for(i=0;i<dim;i++)
        {
            // GensInCone.push_back(key[i]); // now done in first loop since always needed
            TriSectionFirst.push_back(TriangulationBuffer.begin());
            TriSectionLast.push_back(TriangulationBuffer.begin());
        }
    }
    
}


//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::select_supphyps_from(const list<FACETDATA>& NewFacets, 
                    const size_t new_generator, const vector<key_t>& Pyramid_key){
// the mother cone (=this) selects supphyps from the list NewFacets supplied by the daughter
// the daughter provides the necessary information via the parameters

    size_t i;
    boost::dynamic_bitset<> in_Pyr(nr_gen);
    for (i=0; i<Pyramid_key.size(); i++) {
        in_Pyr.set(Pyramid_key[i]);
    }
    // the new generator is always the first in the pyramid
    assert(Pyramid_key[0] == new_generator);


    typename list<FACETDATA>::const_iterator pyr_hyp = NewFacets.begin();
    bool new_global_hyp;
    FACETDATA NewFacet;
    NewFacet.GenInHyp.resize(nr_gen);
    Integer test;
    for (; pyr_hyp!=NewFacets.end(); ++pyr_hyp) {
        if(!pyr_hyp->GenInHyp.test(0)) // new gen not in hyp
            continue;
        new_global_hyp=true;
        for (i=0; i<nr_gen; ++i){
            if(in_Pyr.test(i) || !in_triang[i])
                continue;
            test=v_scalar_product(Generators[i],pyr_hyp->Hyp);
            if(test<=0){
                new_global_hyp=false;
                break;
            }

        }
        if(new_global_hyp){
            NewFacet.Hyp=pyr_hyp->Hyp;
            NewFacet.GenInHyp.reset();
            // size_t gens_in_facet=0;
            for (i=0; i<Pyramid_key.size(); ++i) {
                if (pyr_hyp->GenInHyp.test(i) && in_triang[Pyramid_key[i]]) {
                    NewFacet.GenInHyp.set(Pyramid_key[i]);
                    // gens_in_facet++;
                }
            }
            /* for (i=0; i<nr_gen; ++i) {
                if (NewFacet.GenInHyp.test(i) && in_triang[i]) {
                    gens_in_facet++;
                }
            }*/
            // gens_in_facet++; // Note: new generator not yet in in_triang
            NewFacet.GenInHyp.set(new_generator);
            NewFacet.simplicial=pyr_hyp->simplicial; // (gens_in_facet==dim-1); 
            check_simpliciality_hyperplane(NewFacet);
            number_hyperplane(NewFacet,nrGensInCone,0); //mother unknown
            if(multithreaded_pyramid){
                #pragma omp critical(GIVEBACKHYPS) 
                Facets.push_back(NewFacet);
            } else {
                Facets.push_back(NewFacet);
            }
        }
    }
}

//---------------------------------------------------------------------------
template<typename Integer>
void Full_Cone<Integer>::match_neg_hyp_with_pos_hyps(const FACETDATA& hyp, size_t new_generator,list<FACETDATA*>& PosHyps, boost::dynamic_bitset<>& Zero_P){

    size_t missing_bound, nr_common_zero;
    boost::dynamic_bitset<> common_zero(nr_gen);
    vector<key_t> common_key;
    common_key.reserve(nr_gen);
    vector<key_t> key(nr_gen);
    bool common_subfacet;
    list<FACETDATA> NewHyp;
    size_t subfacet_dim=dim-2;
    size_t nr_missing;
    typename list<FACETDATA*>::iterator a;
    list<FACETDATA> NewHyps;
    Matrix<Integer> Test(0,dim);
    
    boost::dynamic_bitset<> zero_hyp=hyp.GenInHyp & Zero_P;  // we intersect with the set of gens in positive hyps
    
    size_t nr_zero_hyp=0;
    vector<int> key_start(nrGensInCone);
    size_t j;
    int last_existing=-1;
    for(size_t jj=0;jj<nrGensInCone;jj++)
    {
        j=GensInCone[jj];
        if(zero_hyp.test(j)){
            key[nr_zero_hyp]=j;
            for(size_t kk= last_existing+1;kk<=jj;kk++)
                key_start[kk]=nr_zero_hyp;
            nr_zero_hyp++;
            last_existing= jj;
        }
    }
    if(last_existing< (int)nrGensInCone-1)
        for(size_t kk=last_existing+1;kk<nrGensInCone;kk++)
            key_start[kk]=nr_zero_hyp;
            
    if (nr_zero_hyp<dim-2) 
        return;
    
    int tn = omp_get_ancestor_thread_num(1);
    missing_bound=nr_zero_hyp-subfacet_dim; // at most this number of generators can be missing
                                          // to have a chance for common subfacet
                                          
    typename list< FACETDATA*>::iterator hp_j_iterator=PosHyps.begin();
    
    FACETDATA* hp_j;

    for (;hp_j_iterator!=PosHyps.end();++hp_j_iterator){ //match hyp with the given Pos
        hp_j=*hp_j_iterator;


       if(hyp.Ident==hp_j->Mother || hp_j->Ident==hyp.Mother){   // mother and daughter coming together
                                            // their intersection is a subfacet
            add_hyperplane(new_generator,*hp_j,hyp,NewHyps,false);    // simplicial set in add_hyperplane
            continue;           
       }
       
       
       bool extension_test=hyp.BornAt==hp_j->BornAt || (hyp.BornAt<hp_j->BornAt && hp_j->Mother!=0)
                                                      || (hp_j->BornAt<hyp.BornAt && hyp.Mother!=0);
                                                      
       size_t both_existing_from=key_start[max(hyp.BornAt,hp_j->BornAt)];
                  
       nr_missing=0; 
       nr_common_zero=0;
       common_key.clear();
       size_t second_loop_bound=nr_zero_hyp;
       common_subfacet=true;  
       
       if(extension_test){
           bool extended=false;
           second_loop_bound=both_existing_from;
           for(size_t k=both_existing_from;k<nr_zero_hyp;k++){
               if(!hp_j->GenInHyp.test(key[k])) {
                   nr_missing++;
                   if(nr_missing>missing_bound) {
                       common_subfacet=false;
                       break;
                   }
               }
               else {
                   extended=true;
                   common_key.push_back(key[k]);
                   nr_common_zero++;
               }
           }

           if(!extended || !common_subfacet) // 
               continue;
       }
                
       for(size_t k=0;k<second_loop_bound;k++) {
           if(!hp_j->GenInHyp.test(key[k])) {
               nr_missing++;
               if(nr_missing>missing_bound) {
                   common_subfacet=false;
                   break;
               }
           }
           else {
               common_key.push_back(key[k]);
               nr_common_zero++;
           }
        }
        
       if(!common_subfacet)
            continue;
       
       assert(nr_common_zero >=subfacet_dim);
            
        // only rank test since we have many supphyps anyway
        if (!hp_j->simplicial){
            Matrix<Integer>& Test = Top_Cone->RankTest[tn];
            if(Test.rank_submatrix(Generators,common_key)<subfacet_dim)
                common_subfacet=false;     // don't make a hyperplane
        }
        
        if(common_subfacet)
            add_hyperplane(new_generator,*hp_j,hyp,NewHyps,false);  // simplicial set in add_hyperplane
    } // for           

    if(multithreaded_pyramid)
        #pragma omp critical(GIVEBACKHYPS)
        Facets.splice(Facets.end(),NewHyps);
    else
        Facets.splice(Facets.end(),NewHyps);

}

//---------------------------------------------------------------------------
template<typename Integer>
void Full_Cone<Integer>::collect_pos_supphyps(list<FACETDATA*>& PosHyps, boost::dynamic_bitset<>& Zero_P, size_t& nr_pos){
           
    // positive facets are collected in a list
    
    typename list<FACETDATA>::iterator ii = Facets.begin();
    nr_pos=0;
    
    for (size_t ij=0; ij< old_nr_supp_hyps; ++ij, ++ii)
        if (ii->ValNewGen>0) {
            Zero_P |= ii->GenInHyp;
            PosHyps.push_back(&(*ii));
            nr_pos++;
        }
}

//---------------------------------------------------------------------------
template<typename Integer>
void Full_Cone<Integer>::evaluate_large_rec_pyramids(size_t new_generator){
    
    size_t nrLargeRecPyrs=LargeRecPyrs.size();
    if(nrLargeRecPyrs==0)
        return;
        
    if(verbose)
        verboseOutput() << "large pyramids " << nrLargeRecPyrs << endl;
    
    list<FACETDATA*> PosHyps;
    boost::dynamic_bitset<> Zero_P(nr_gen);
    size_t nr_pos;
    collect_pos_supphyps(PosHyps,Zero_P,nr_pos);
    
    nrTotalComparisons+=nr_pos*nrLargeRecPyrs;
#ifndef NCATCH
    std::exception_ptr tmp_exception;
#endif
    
    #pragma omp parallel
    {
    size_t ppos=0;
    typename list<FACETDATA>::iterator p=LargeRecPyrs.begin(); 
    
    #pragma omp for schedule(dynamic) 
    for(size_t i=0; i<nrLargeRecPyrs; i++){
        for(; i > ppos; ++ppos, ++p) ;
        for(; i < ppos; --ppos, --p) {};
#ifndef NCATCH
        try {
#endif
            match_neg_hyp_with_pos_hyps(*p,new_generator,PosHyps,Zero_P);
#ifndef NCATCH
        } catch(const std::exception& ) {
            tmp_exception = std::current_exception();
        }
#endif
    }
    } // parallel
#ifndef NCATCH
    if (!(tmp_exception == 0)) std::rethrow_exception(tmp_exception);
#endif

    LargeRecPyrs.clear();
}

//---------------------------------------------------------------------------

template<typename Integer>
bool Full_Cone<Integer>::check_pyr_buffer(const size_t level){
    if(level==0)
        return(nrPyramids[0] > EvalBoundLevel0Pyr);
    else
        return(nrPyramids[level] > EvalBoundPyr);
}

//---------------------------------------------------------------------------

#ifdef NMZ_MIC_OFFLOAD
template<>
void Full_Cone<long long>::try_offload(size_t max_level) {

    if (!is_pyramid && _Offload_get_device_number() < 0) // dynamic check for being on CPU (-1)
    {
        if (max_level >= nrPyramids.size()) max_level = nrPyramids.size()-1;
        for (size_t level = 0; level <= max_level; ++level) {
            if (nrPyramids[level] >= 100) {
                cout << "Try offload of level " << level << " pyramids ..." << endl;
                mic_offloader.offload_pyramids(*this, level);
                break;
            }
        }
    }
}

template<typename Integer>
void Full_Cone<Integer>::try_offload(size_t max_level) {
}
//else it is implemented in the header
#endif // NMZ_MIC_OFFLOAD

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::evaluate_stored_pyramids(const size_t level){
// evaluates the stored non-recursive pyramids

    assert(!omp_in_parallel());

    if(Pyramids[level].empty())
        return;
    if (Pyramids.size() < level+2) {
        Pyramids.resize(level+2);      // provide space for a new generation
        nrPyramids.resize(level+2, 0);
    }

    size_t eval_down_to = 0;

#ifdef NMZ_MIC_OFFLOAD
#ifndef __MIC__
    // only on host and if offload is available
    if (level == 0 && nrPyramids[0] > EvalBoundLevel0Pyr) {
        eval_down_to = EvalBoundLevel0Pyr;
    }
#endif
#endif

    vector<char> Done(nrPyramids[level],0);
    if (verbose) {
        verboseOutput() << "**************************************************" << endl;

        for (size_t l=0; l<=level; ++l) {
            if (nrPyramids[l]>0) {
                verboseOutput() << "level " << l << " pyramids remaining: "
                                << nrPyramids[l] << endl;
            }
        }
        verboseOutput() << "**************************************************" << endl;
    }
    typename list<vector<key_t> >::iterator p;
    size_t ppos;
    bool skip_remaining;
#ifndef NCATCH
    std::exception_ptr tmp_exception;
#endif

    while (nrPyramids[level] > eval_down_to) {

       p = Pyramids[level].begin();
       ppos=0;
       skip_remaining = false;
    
       #pragma omp parallel for firstprivate(p,ppos) schedule(dynamic) 
       for(size_t i=0; i<nrPyramids[level]; i++){
           if (skip_remaining)
                continue;
           for(; i > ppos; ++ppos, ++p) ;
           for(; i < ppos; --ppos, --p) ;
           
           if(Done[i])
               continue;
           Done[i]=1;

#ifndef NCATCH
           try {
#endif
               Full_Cone<Integer> Pyramid(*this,*p);
               // Pyramid.recursion_allowed=false;
               Pyramid.do_all_hyperplanes=false;
               if (level>=2 && do_partial_triangulation){ // limits the descent of do_partial_triangulation
                   Pyramid.do_triangulation=true;
                   Pyramid.do_partial_triangulation=false;
               }
               Pyramid.store_level=level+1;
               Pyramid.build_cone();
               if (check_evaluation_buffer_size() || Top_Cone->check_pyr_buffer(level+1)) {
                   // interrupt parallel execution to keep the buffer under control
                   skip_remaining = true;
               }
#ifndef NCATCH
           } catch(const std::exception& ) {
               tmp_exception = std::current_exception();
               skip_remaining = true;
               #pragma omp flush(skip_remaining)
           }
#endif
        } //end parallel for
#ifndef NCATCH
        if (!(tmp_exception == 0)) std::rethrow_exception(tmp_exception);
#endif

        // remove done pyramids
        p = Pyramids[level].begin();
        for(size_t i=0; p != Pyramids[level].end(); i++){
            if (Done[i]) {
                p=Pyramids[level].erase(p);
                nrPyramids[level]--;
                Done[i]=0;
            } else {
                ++p;
            }
        }

        try_offload(level+1);

        if (check_evaluation_buffer_size()) {
            if (verbose)
                verboseOutput() << nrPyramids[level] <<
                    " pyramids remaining on level " << level << ", ";
            Top_Cone->evaluate_triangulation();
            try_offload(level+1);
        }

        if (Top_Cone->check_pyr_buffer(level+1)) {
            evaluate_stored_pyramids(level+1);
        }
    
    } //end while (nrPyramids[level] > 0)
     
    if (verbose) {
        verboseOutput() << "**************************************************" << endl;
        verboseOutput() << "all pyramids on level "<< level << " done!"<<endl;
        if (nrPyramids[level+1] == 0) {
            for (size_t l=0; l<=level; ++l) {
                if (nrPyramids[l]>0) {
                    verboseOutput() << "level " << l << " pyramids remaining: "
                                    << nrPyramids[l] << endl;
                }
            }
            verboseOutput() << "**************************************************" << endl;
        }
    }
    if(check_evaluation_buffer())
    {
        Top_Cone->evaluate_triangulation();
    }
     
    evaluate_stored_pyramids(level+1);
}
    


//---------------------------------------------------------------------------

/* builds the cone successively by inserting generators */
template<typename Integer>
void Full_Cone<Integer>::build_cone() {
    // if(dim>0){            //correction needed to include the 0 cone;
    
    // cout << "Pyr " << pyr_level << endl;

    long long RecBoundSuppHyp = dim*dim;
    RecBoundSuppHyp *= RecBoundSuppHyp*SuppHypRecursionFactor; //dim^4 * 3000
    
    tri_recursion=false; 
    
    multithreaded_pyramid=(omp_get_level()==0);
    
    if(!use_existing_facets){
        if(multithreaded_pyramid){
            HypCounter.resize(omp_get_max_threads());
            for(size_t i=0;i<HypCounter.size();++i)
                HypCounter[i]=i+1;
        } else{
            HypCounter.resize(1);
            HypCounter[0]=1;    
        }
        
        find_and_evaluate_start_simplex();
    }
    
    size_t last_to_be_inserted; // good to know in case of do_all_hyperplanes==false
    last_to_be_inserted=nr_gen-1;  // because we don't need to compute support hyperplanes in this case 
    for(int j=nr_gen-1;j>=0;--j){
        if(!in_triang[j]){
            last_to_be_inserted=j;
            break;
        }
    } // last_to_be_inserted now determined
    
    bool is_new_generator;
    typename list< FACETDATA >::iterator l;


    for (size_t i=start_from;i<nr_gen;++i) { 
    
        start_from=i;
    
        if (in_triang[i])
            continue;
            
        if(do_triangulation && TriangulationBufferSize > 2*RecBoundTriang) // emermergency brake
            tri_recursion=true;               // to switch off production of simplices in favor
                                              // of non-recursive pyramids
        Integer scalar_product;                                              
        is_new_generator=false;
        l=Facets.begin();
        old_nr_supp_hyps=Facets.size(); // Facets will be xtended in the loop 

        long long nr_pos=0, nr_neg=0;
        long long nr_neg_simp=0, nr_pos_simp=0;
        vector<Integer> L;           
#ifndef NCATCH
        std::exception_ptr tmp_exception;
#endif
        
        size_t lpos=0;
        #pragma omp parallel for private(L,scalar_product) firstprivate(lpos,l) reduction(+: nr_pos, nr_neg)
        for (size_t k=0; k<old_nr_supp_hyps; k++) {
#ifndef NCATCH
            try {
#endif
                for(;k > lpos; lpos++, l++) ;
                for(;k < lpos; lpos--, l--) ;

                L=Generators[i];
                scalar_product=v_scalar_product(L,(*l).Hyp);
                l->ValNewGen=scalar_product;
                if (scalar_product<0) {
                    is_new_generator=true;
                    nr_neg++;
                    if(l->simplicial)
                        #pragma omp atomic
                        nr_neg_simp++;
                }
                if (scalar_product>0) {
                    nr_pos++;
                    if(l->simplicial)
                        #pragma omp atomic
                        nr_pos_simp++;
                }
#ifndef NCATCH
            } catch(const std::exception& ) {
                tmp_exception = std::current_exception();
            }
#endif
        }  //end parallel for
#ifndef NCATCH
        if (!(tmp_exception == 0)) std::rethrow_exception(tmp_exception);
#endif

        if(!is_new_generator)
            continue;

        // the i-th generator is used in the triangulation
        // in_triang[i]=true; // now at end of loop
        if (deg1_triangulation && isComputed(ConeProperty::Grading))
            deg1_triangulation = (gen_degrees[i] == 1);
        
        if (!omp_in_parallel())
            try_offload(0);
        // cout << nr_neg << " " << nr_pos << " " << nr_neg_simp << " " << nr_pos_simp << endl;
        // First we test whether to go to recursive pyramids because of too many supphyps
        if (recursion_allowed && nr_neg*nr_pos-(nr_neg_simp*nr_pos_simp) > RecBoundSuppHyp) {  // use pyramids because of supphyps
            // cout << "In Pyramids" << endl;
            if (do_triangulation)
                tri_recursion = true; // We can not go back to classical triangulation
            if(check_evaluation_buffer()){
                Top_Cone->evaluate_triangulation();
            }

            process_pyramids(i,true); //recursive
            lastGen=i;
            nextGen=i+1; 
        }
        else{ // now we check whether to go to pyramids because of the size of triangulation
              // once we have done so, we must stay with it
            if( tri_recursion || (do_triangulation 
                && (nr_neg*TriangulationBufferSize > RecBoundTriang
                    || 3*omp_get_max_threads()*TriangulationBufferSize>EvalBoundTriang ))){ // go to pyramids because of triangulation
                if(check_evaluation_buffer()){
                    Top_Cone->evaluate_triangulation();
                }
                tri_recursion=true;
                process_pyramids(i,false); //non-recursive
            }
            else{  // no pyramids necesary
                if(do_partial_triangulation)
                    process_pyramids(i,false); // non-recursive
                if(do_triangulation)
                    extend_triangulation(i);
            }

            if(do_all_hyperplanes || i!=last_to_be_inserted) 
                find_new_facets(i);
        }
        
        // removing the negative hyperplanes if necessary
        if(do_all_hyperplanes || i!=last_to_be_inserted){
            l=Facets.begin();
            for (size_t j=0; j<old_nr_supp_hyps;j++){
                if (l->ValNewGen<0) {
                    l=Facets.erase(l);
                }
                else
                    ++l;
            }
        }
        
        GensInCone.push_back(i);
        nrGensInCone++;
        
        Comparisons.push_back(nrTotalComparisons);
        
        if(verbose) {
            verboseOutput() << "gen="<< i+1 <<", ";
            if (do_all_hyperplanes || i!=last_to_be_inserted) {
                verboseOutput() << Facets.size()<<" hyp";
            } else {
                verboseOutput() << Support_Hyperplanes.nr_of_rows()<<" hyp";
            }
            if(nrPyramids[0]>0)
                verboseOutput() << ", " << nrPyramids[0] << " pyr"; 
            if(do_triangulation||do_partial_triangulation)
                verboseOutput() << ", " << TriangulationBufferSize << " simpl";
            verboseOutput()<< endl;
        }
        
        in_triang[i]=true;
        
    }  // loop over i
    
    start_from=nr_gen;
    
    if (is_pyramid && do_all_hyperplanes)  // must give supphyps back to mother
        Mother->select_supphyps_from(Facets, apex, Mother_Key);
    
    // transfer Facets --> SupportHyperplanes
    if (do_all_hyperplanes) {
        nrSupport_Hyperplanes = Facets.size();
        Support_Hyperplanes = Matrix<Integer>(nrSupport_Hyperplanes,0);
        typename list<FACETDATA>::iterator IHV=Facets.begin();
        for (size_t i=0; i<nrSupport_Hyperplanes; ++i, ++IHV) {
            swap(Support_Hyperplanes[i],IHV->Hyp);
        }
        is_Computed.set(ConeProperty::SupportHyperplanes);
    } 
    Support_Hyperplanes.set_nr_of_columns(dim);
   
    
    if(do_extreme_rays && do_all_hyperplanes)
        compute_extreme_rays(true);
    
    transfer_triangulation_to_top(); // transfer remaining simplices to top
    if(check_evaluation_buffer()){
        Top_Cone->evaluate_triangulation();
    }  

    // } // end if (dim>0)
    
    Facets.clear(); 

}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::find_bottom_facets() {

    if(verbose)
        verboseOutput() << "Computing bottom decomposition" << endl;

    vector<key_t> start_simpl=Generators.max_rank_submatrix_lex();
    Order_Vector = vector<Integer>(dim,0);
    for(size_t i=0;i<dim;++i)
        for(size_t j=0;j<dim;++j)
            Order_Vector[j]+=((unsigned long) (1+i%10))*Generators[start_simpl[i]][j];

    // First the generators for the recession cone = our cone
    Matrix<Integer> BottomGen(0,dim+1);
    vector<Integer> help(dim+1);
    for(size_t i=0;i<nr_gen;++i){
        for(size_t j=0;j<dim; ++j)
            help[j]=Generators[i][j];
        help[dim]=0;
        BottomGen.append(help);
    }
    // then the same vectors as generators of the bottom polyhedron
    for(size_t i=0;i<nr_gen;++i){
        for(size_t j=0;j<dim; ++j)
            help[j]=Generators[i][j];
        help[dim]=1;
        BottomGen.append(help);
    }
    
    Full_Cone BottomPolyhedron(BottomGen);
    BottomPolyhedron.verbose=verbose;
    BottomPolyhedron.do_extreme_rays=true;
    BottomPolyhedron.keep_order = true;
    try {     
        BottomPolyhedron.dualize_cone();  // includes finding extreme rays
    } catch(const NonpointedException& ){};

    // transfer pointedness
    assert( BottomPolyhedron.isComputed(ConeProperty::IsPointed) );
    pointed = BottomPolyhedron.pointed;
    is_Computed.set(ConeProperty::IsPointed);

    // BottomPolyhedron.Support_Hyperplanes.pretty_print(cout);

    help.resize(dim);

    // find extreme rays of Bottom among the generators
    vector<key_t> BottomExtRays;
    for(size_t i=0;i<nr_gen;++i)
        if(BottomPolyhedron.Extreme_Rays_Ind[i+nr_gen])
            BottomExtRays.push_back(i);
    /* vector<key_t> BottomExtRays; // can be used if the bool vector should not exist anymore
    size_t start_search=0;
    for(size_t i=0;i<ExtStrahl.nr_of_rows();++i){
        if(BottomPolyhedron.ExtStrahl[i][dim]==1){
            BottomPolyhedron.ExtStrahl[i].resize(dim);
            for(size_t j=0;j<nr_gen;++j){
                size_t k=(j+start_search) % nr_gen;
                if(BottomPolyhedron.ExtStrahl[i]==Generators[k]){
                    BottomExtRays.push_back(k);
                    start_search++;
                }
            }
        }
    }*/

    if(verbose)
        verboseOutput() << "Bottom has " << BottomExtRays.size() << " extreme rays" << endl;
 
    Matrix<Integer> BottomFacets(0,dim);
    vector<Integer> BottomDegs(0,dim);
    if (!isComputed(ConeProperty::SupportHyperplanes)) {
        Support_Hyperplanes = Matrix<Integer>(0,dim);
        nrSupport_Hyperplanes=0;
    }
    for(size_t i=0;i<BottomPolyhedron.nrSupport_Hyperplanes;++i){
        Integer test=BottomPolyhedron.Support_Hyperplanes[i][dim];
        for(size_t j=0;j<dim;++j)
            help[j]=BottomPolyhedron.Support_Hyperplanes[i][j];     
        if(test==0 && !isComputed(ConeProperty::SupportHyperplanes)){
            Support_Hyperplanes.append(help);
            nrSupport_Hyperplanes++;
        }
        if (test < 0){ 
            BottomFacets.append(help);
            BottomDegs.push_back(-test);
        }
    }
    
    is_Computed.set(ConeProperty::SupportHyperplanes);
    
    if (!pointed)
        throw NonpointedException();

    vector<key_t> facet;
    for(size_t i=0;i<BottomFacets.nr_of_rows();++i){
        facet.clear();
        for(size_t k=0;k<BottomExtRays.size();++k)
            if(v_scalar_product(Generators[BottomExtRays[k]],BottomFacets[i])==BottomDegs[i])
                facet.push_back(BottomExtRays[k]);
        Pyramids[0].push_back(facet);
        nrPyramids[0]++;
    }
    if(verbose)
        verboseOutput() << "Bottom decomposition computed, " << nrPyramids[0] << " subcones" << endl;
}

template<typename Integer>
void Full_Cone<Integer>::start_message() {
    
       if (verbose) {
        verboseOutput()<<"************************************************************"<<endl;
        verboseOutput()<<"starting primal algorithm ";
        if (do_partial_triangulation) verboseOutput()<<"with partial triangulation ";
        if (do_triangulation) {
            verboseOutput()<<"with full triangulation ";
        }
        if (!do_triangulation && !do_partial_triangulation) verboseOutput()<<"(only support hyperplanes) ";
        verboseOutput()<<"..."<<endl;
    }   
}

template<typename Integer>
void Full_Cone<Integer>::end_message() {
    
       if (verbose) {
        verboseOutput() << "------------------------------------------------------------"<<endl;
    }   
}


//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::build_top_cone() {

    OldCandidates.verbose=verbose;
    NewCandidates.verbose=verbose;
    
    if(dim==0)
        return;
 
    if( ( !do_bottom_dec || deg1_generated || dim==1 || (!do_triangulation && !do_partial_triangulation))) {        
        build_cone();
    }
    else{
        find_bottom_facets();
        deg1_triangulation=false;
    }   

    try_offload(0);
    evaluate_stored_pyramids(0);  // force evaluation of remaining pyramids

#ifdef NMZ_MIC_OFFLOAD
    if (_Offload_get_device_number() < 0) // dynamic check for being on CPU (-1)
    {
        evaluate_stored_pyramids(0);  // previous run could have left over pyramids
        mic_offloader.evaluate_triangulation();
    }
#endif // NMZ_MIC_OFFLOAD   

}

//---------------------------------------------------------------------------

template<typename Integer>
bool Full_Cone<Integer>::check_evaluation_buffer(){

    return(omp_get_level()==0 && check_evaluation_buffer_size());
}

//---------------------------------------------------------------------------

template<typename Integer>
bool Full_Cone<Integer>::check_evaluation_buffer_size(){

    return(!Top_Cone->keep_triangulation && 
               Top_Cone->TriangulationBufferSize > EvalBoundTriang);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::transfer_triangulation_to_top(){  // NEW EVA

    size_t i;

    // cout << "Pyr level " << pyr_level << endl;
    
    if(!is_pyramid) {  // we are in top cone
        if(check_evaluation_buffer()){
            evaluate_triangulation();
        }
        return;      // no transfer necessary
    }

    // now we are in a pyramid

    // cout << "In pyramid " << endl;
    int tn = 0;
    if (omp_in_parallel())
        tn = omp_get_ancestor_thread_num(1);
  
    typename list< SHORTSIMPLEX<Integer> >::iterator pyr_simp=TriangulationBuffer.begin();
    while (pyr_simp!=TriangulationBuffer.end()) {
        if (pyr_simp->height == 0) { // it was marked to be skipped
            Top_Cone->FS[tn].splice(Top_Cone->FS[tn].end(), TriangulationBuffer, pyr_simp++);
            --TriangulationBufferSize;
        } else {
            for (i=0; i<dim; i++)  // adjust key to topcone generators
                pyr_simp->key[i]=Top_Key[pyr_simp->key[i]];
            ++pyr_simp;
        }
    }

    // cout << "Keys transferred " << endl;
    #pragma omp critical(TRIANG)
    {
        Top_Cone->TriangulationBuffer.splice(Top_Cone->TriangulationBuffer.end(),TriangulationBuffer);
        Top_Cone->TriangulationBufferSize += TriangulationBufferSize;
    }
    TriangulationBufferSize = 0;
  
}

//---------------------------------------------------------------------------
template<typename Integer>
void Full_Cone<Integer>::get_supphyps_from_copy(bool from_scratch){

    if(isComputed(ConeProperty::SupportHyperplanes)) // we have them already
        return;
    
    Full_Cone copy((*this).Generators);
    copy.verbose=verbose;
    if(!from_scratch){
        copy.start_from=start_from;
        copy.use_existing_facets=true;
        copy.keep_order=true;
        copy.HypCounter=HypCounter;
        copy.Extreme_Rays_Ind=Extreme_Rays_Ind;
        copy.in_triang=in_triang;
        copy.old_nr_supp_hyps=old_nr_supp_hyps;
        if(isComputed(ConeProperty::ExtremeRays))
            copy.is_Computed.set(ConeProperty::ExtremeRays);
        copy.GensInCone=GensInCone;
        copy.nrGensInCone=nrGensInCone;
        copy.Comparisons=Comparisons;
        if(!Comparisons.empty())
            copy.nrTotalComparisons=Comparisons[Comparisons.size()-1];
        
        typename list< FACETDATA >::const_iterator l=Facets.begin();
        
        for(size_t i=0;i<old_nr_supp_hyps;++i){
            copy.Facets.push_back(*l);
            ++l;
        }
    }
    
    copy.dualize_cone();
    
    std::swap(Support_Hyperplanes,copy.Support_Hyperplanes);
    nrSupport_Hyperplanes = copy.nrSupport_Hyperplanes;
    is_Computed.set(ConeProperty::SupportHyperplanes);
    do_all_hyperplanes = false;
}


//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::update_reducers(bool forced){
    
    if((!do_Hilbert_basis || do_module_gens_intcl) && !forced)
        return;

    if(NewCandidates.Candidates.empty())
        return;

    if(nr_gen==dim)  // no global reduction in the simplicial case
        NewCandidates.sort_by_deg(); 
    if(nr_gen!=dim || forced){  // global reduction in the nonsimplicial case (or forced)
        NewCandidates.auto_reduce();
        if(verbose){
            verboseOutput() << "reducing " << OldCandidates.Candidates.size() << " candidates by " << NewCandidates.Candidates.size() << " reducers" << endl;
        }
        OldCandidates.reduce_by(NewCandidates);
    }
    OldCandidates.merge(NewCandidates);
    CandidatesSize=OldCandidates.Candidates.size();
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::prepare_old_candidates_and_support_hyperplanes(){

    if(!isComputed(ConeProperty::SupportHyperplanes)){
        if (verbose) {
            verboseOutput() << "**** Computing support hyperplanes for reduction:" << endl;
        }
        get_supphyps_from_copy(false);
    }
    
    check_pointed();
    if(!pointed){
        throw NonpointedException();
    }

    int max_threads = omp_get_max_threads();
    size_t Memory_per_gen=8*nrSupport_Hyperplanes;
    size_t max_nr_gen=RAM_Size/(Memory_per_gen*max_threads);
    // cout << "max_nr_gen " << max_nr_gen << endl;
    AdjustedReductionBound=max_nr_gen;
    if(AdjustedReductionBound < 2000)
        AdjustedReductionBound=2000;


    Sorting=compute_degree_function();
    if (!is_approximation) {
        bool save_do_module_gens_intcl=do_module_gens_intcl;
        do_module_gens_intcl=false; // to avoid multiplying sort_deg by 2 for the original generators
        for (size_t i = 0; i <nr_gen; i++) {               
            // cout << gen_levels[i] << " ** " << Generators[i];
            if(!inhomogeneous || gen_levels[i]==0 || (!save_do_module_gens_intcl && gen_levels[i]<=1)) {
                OldCandidates.Candidates.push_back(Candidate<Integer>(Generators[i],*this));
                OldCandidates.Candidates.back().original_generator=true;
            }
        }
        do_module_gens_intcl=save_do_module_gens_intcl; // restore
        if(!do_module_gens_intcl) // if do_module_gens_intcl we don't want to change the original monoid
            OldCandidates.auto_reduce();
        else
            OldCandidates.sort_by_deg();
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::evaluate_triangulation(){

    assert(omp_get_level()==0);

    // prepare reduction 
    if (do_Hilbert_basis && OldCandidates.Candidates.empty()) {
        prepare_old_candidates_and_support_hyperplanes();
    }
    
    if (TriangulationBufferSize == 0)
        return;

    const long VERBOSE_STEPS = 50;
    long step_x_size = TriangulationBufferSize-VERBOSE_STEPS;
    if (verbose) {
        verboseOutput() << "evaluating "<<TriangulationBufferSize<<" simplices" <<endl;
        /* verboseOutput() << "---------+---------+---------+---------+---------+"
                        << " (one | per 2%)" << endl;*/
    }
    
    totalNrSimplices += TriangulationBufferSize;

    if(do_evaluation && !do_only_multiplicity) {
    
    deque<bool> done(TriangulationBufferSize,false);
    bool skip_remaining;
#ifndef NCATCH
    std::exception_ptr tmp_exception;
#endif

    do{ // allows multiple run of loop below in case of interruption for the update of reducers
    
    skip_remaining=false;
    step_x_size = TriangulationBufferSize-VERBOSE_STEPS;

    #pragma omp parallel
    {
        typename list< SHORTSIMPLEX<Integer> >::iterator s = TriangulationBuffer.begin();
        size_t spos=0;
        int tn = omp_get_thread_num();
        #pragma omp for schedule(dynamic) nowait
        for(size_t i=0; i<TriangulationBufferSize; i++){
#ifndef NCATCH
            try {
#endif
                for(; i > spos; ++spos, ++s) ;
                for(; i < spos; --spos, --s) ;

                if(skip_remaining)
                    continue;
                
                if(done[spos])
                    continue;

                done[spos]=true;

                if(keep_triangulation || do_Stanley_dec)
                    sort(s->key.begin(),s->key.end());
                if(!SimplexEval[tn].evaluate(*s)){
                    #pragma omp critical(LARGESIMPLEX)
                    LargeSimplices.push_back(SimplexEval[tn]);
                }
                if (verbose) {
                    #pragma omp critical(VERBOSE)
                    while ((long)(i*VERBOSE_STEPS) >= step_x_size) {
                        step_x_size += TriangulationBufferSize;
                        verboseOutput() << "|" <<flush;
                    }
                }

                if(do_Hilbert_basis && Results[tn].get_collected_elements_size() > AdjustedReductionBound)
                    skip_remaining=true;
#ifndef NCATCH
            } catch(const std::exception& ) {
                tmp_exception = std::current_exception();
                skip_remaining = true;
                #pragma omp flush(skip_remaining)
            }
#endif
        }
        Results[tn].transfer_candidates();
    } // end parallel
#ifndef NCATCH
    if (!(tmp_exception == 0)) std::rethrow_exception(tmp_exception);
#endif

    if (verbose)
        verboseOutput()  << endl;
        
    update_reducers();
        
    } while(skip_remaining);
        
    } // do_evaluation
            
    if (verbose)
    {
        verboseOutput() << totalNrSimplices << " simplices";
        if(do_Hilbert_basis)
            verboseOutput() << ", " << CandidatesSize << " HB candidates";
        if(do_deg1_elements)
            verboseOutput() << ", " << CandidatesSize << " deg1 vectors";
        verboseOutput() << " accumulated." << endl;
    }
    
    if (keep_triangulation) {
        Triangulation.splice(Triangulation.end(),TriangulationBuffer);
    } else {
        // #pragma omp critical(FREESIMPL)
        FreeSimpl.splice(FreeSimpl.begin(),TriangulationBuffer);
    }
    TriangulationBufferSize=0;

    if (verbose && use_bottom_points) {
        size_t lss=LargeSimplices.size();
        if(lss>0)
            verboseOutput() << lss << " large simplices stored" << endl;
    }

    for(size_t i=0;i<Results.size();++i)
        Results[i].transfer_candidates(); // any remaining ones
    
    update_reducers();
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::evaluate_large_simplices(){

    assert(omp_get_level()==0);

    size_t lss = LargeSimplices.size();
    if (lss == 0)
        return;

    if (verbose) {
        verboseOutput() << "Evaluating " << lss << " large simplices" << endl;
    }
    size_t j;
    for (j = 0; j < lss; ++j) {
        evaluate_large_simplex(j, lss);
    }

    // decomposition might have created new simplices
    evaluate_triangulation();

    // also new large simplices are possible
    if (!LargeSimplices.empty()) {
        use_bottom_points = false;
        lss += LargeSimplices.size();
        if (verbose) {
            verboseOutput() << "Continue evaluation of " << lss << " large simplices without new decompositions of simplicial cones." << endl;
        }
        for (; j < lss; ++j) {
            evaluate_large_simplex(j, lss);
        }
    }
    assert(LargeSimplices.empty());

    for(size_t i=0;i<Results.size();++i)
        Results[i].transfer_candidates(); // any remaining ones

    update_reducers();
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::evaluate_large_simplex(size_t j, size_t lss) {
    if (verbose) {
        verboseOutput() << "Large simplex " << j+1 << " / " << lss << endl;
    }

    if (do_deg1_elements && !do_h_vector && !do_Stanley_dec && !deg1_triangulation) {
        compute_deg1_elements_via_approx_simplicial(LargeSimplices.front().get_key());
    }
    else {
        LargeSimplices.front().Simplex_parallel_evaluation();
        if (do_Hilbert_basis && Results[0].get_collected_elements_size() > AdjustedReductionBound) {
            Results[0].transfer_candidates();
            update_reducers();
        }
    }
    LargeSimplices.pop_front();
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::compute_sub_div_elements(const Matrix<Integer>& gens,list<vector<Integer> >& sub_div_elements){

    if (is_approximation) return; // do not approximate again!

    Full_Cone<Integer> SimplCone(gens);

    vector<Integer> N;
    N = SimplCone.Generators.find_linear_form();
    assert(N.size()==SimplCone.dim);
    // if no grading is computed, we use the normal vector on the simplex
    if (isComputed(ConeProperty::Grading)){
        SimplCone.Grading=Grading;
    } else {

        SimplCone.Grading = N;
    }
    SimplCone.is_Computed.set(ConeProperty::Grading);
    SimplCone.deg1_check();
    if (SimplCone.isDeg1ExtremeRays()) return; // no approx possible

    if (verbose) {
        verboseOutput() << "Computing bottom candidates via approximation... " << flush;
    }
    SimplCone.do_Hilbert_basis=true;  // not srictly true. We only want subdividing points
    SimplCone.do_approximation=true;  // as indicted by do_approximation
    SimplCone.approx_level = approx_level;

    SimplCone.Truncation= N;
    SimplCone.TruncLevel=v_scalar_product(SimplCone.Truncation,SimplCone.Generators[0]);

    SimplCone.compute();
    sub_div_elements.splice(sub_div_elements.begin(),SimplCone.Hilbert_Basis);
    if (verbose) {
        verboseOutput() << "done." << endl;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::compute_deg1_elements_via_approx_simplicial(const vector<key_t>& key){

    Full_Cone<Integer> SimplCone(Generators.submatrix(key));
    SimplCone.verbose=false; // verbose;
    SimplCone.Grading=Grading;
    SimplCone.is_Computed.set(ConeProperty::Grading);
    SimplCone.do_deg1_elements=true;
    SimplCone.do_approximation=true;
    
    SimplCone.compute();
    
    vector<bool> Excluded(dim,false);
    for(size_t i=0;i<dim;++i){
        Integer test=v_scalar_product(SimplCone.Support_Hyperplanes[i],Order_Vector);
        if(test>0)
            continue;
        if(test<0){
            Excluded[i]=true;
            continue;
        }
        size_t j;
        for(j=0;j<dim;++j){
            if(SimplCone.Support_Hyperplanes[i][j]!=0)
                break;        
        }
        if(SimplCone.Support_Hyperplanes[i][j]<0)
            Excluded[i]=true;        
    }
    
    typename list<vector<Integer> >::const_iterator E;
    for(E=SimplCone.Deg1_Elements.begin();E!=SimplCone.Deg1_Elements.end();++E){
        size_t i;
        for(i=0;i<dim;++i)
            if(v_scalar_product(*E,SimplCone.Support_Hyperplanes[i])==0 && Excluded[i])
                break;
        if(i<dim)
            continue;
            
        for(i=0;i<dim;++i)  // exclude original generators
            if(*E==SimplCone.Generators[i])
                 break;
        if(i==dim){    
            Results[0].Deg1_Elements.push_back(*E); // Results[0].Deg1_Elements.push_back(*E);
            Results[0].collected_elements_size++;
        }        
    }
    Results[0].transfer_candidates();
}
    

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::remove_duplicate_ori_gens_from_HB(){
    
return; //TODO reactivate!

    set<vector<Integer> > OriGens;
    typename list<Candidate<Integer> >:: iterator c=OldCandidates.Candidates.begin();
    typename set<vector<Integer> >:: iterator found;
    for(;c!=OldCandidates.Candidates.end();){
        if(!c->original_generator){
            ++c;
            continue;
        }
        found=OriGens.find(c->cand);
        if(found!=OriGens.end()){
            c=OldCandidates.Candidates.erase(c);
        }
        else{
            if(c->original_generator)
                OriGens.insert(c->cand);
            ++c;
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::primal_algorithm(){

    primal_algorithm_initialize();

    /***** Main Work is done in build_top_cone() *****/
    build_top_cone();  // evaluates if keep_triangulation==false
    /***** Main Work is done in build_top_cone() *****/

    check_pointed();
    if(!pointed){
        throw NonpointedException();
    }

    primal_algorithm_finalize();
    primal_algorithm_set_computed();
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::primal_algorithm_initialize() {

    prepare_inclusion_exclusion();

    SimplexEval = vector< SimplexEvaluator<Integer> >(omp_get_max_threads(),SimplexEvaluator<Integer>(*this));
    for(size_t i=0;i<SimplexEval.size();++i)
        SimplexEval[i].set_evaluator_tn(i);
    Results = vector< Collector<Integer> >(omp_get_max_threads(),Collector<Integer>(*this));
    Hilbert_Series.setVerbose(verbose);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::primal_algorithm_finalize() {

    if (isComputed(ConeProperty::Grading) && !deg1_generated) {
        deg1_triangulation = false;
    }
    if (keep_triangulation) {
        is_Computed.set(ConeProperty::Triangulation);
    }
    if (do_cone_dec) {
        is_Computed.set(ConeProperty::ConeDecomposition);
    }

    evaluate_triangulation();
    evaluate_large_simplices();
    FreeSimpl.clear();
    
    compute_class_group();
    
    // collect accumulated data from the SimplexEvaluators
    for (int zi=0; zi<omp_get_max_threads(); zi++) {
        detSum += Results[zi].getDetSum();
        multiplicity += Results[zi].getMultiplicitySum();
        if (do_h_vector) {
            Hilbert_Series += Results[zi].getHilbertSeriesSum();
        }
    }
#ifdef NMZ_MIC_OFFLOAD
    // collect accumulated data from mics
    if (_Offload_get_device_number() < 0) // dynamic check for being on CPU (-1)
    {
        mic_offloader.finalize();
    }
#endif // NMZ_MIC_OFFLOAD
    if (do_h_vector) {
        Hilbert_Series.collectData();
    }
    
    if(verbose) {
        verboseOutput() << "Total number of pyramids = "<< totalNrPyr << ", among them simplicial " << nrSimplicialPyr << endl;
        // cout << "Uni "<< Unimod << " Ht1NonUni " << Ht1NonUni << " NonDecided " << NonDecided << " TotNonDec " << NonDecidedHyp<< endl;
        if(do_only_multiplicity)
            verboseOutput() << "Determinants computed = " << TotDet << endl;
        /* if(NrCompVect>0)
            cout << "Vector comparisons " << NrCompVect << " Value comparisons " << NrCompVal 
                    << " Average " << NrCompVal/NrCompVect+1 << endl; */
    }
    
    if(verbose && GMP_hyp+GMP_scal_prod+GMP_mat>0)
        verboseOutput() << "GMP transitions: matrices " << GMP_mat << " hyperplanes " << GMP_hyp << " vector operations " << GMP_scal_prod << endl; 

}
    
//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::make_module_gens(){

    if(!inhomogeneous){
        NewCandidates.extract(ModuleGeneratorsOverOriginalMonoid);
        vector<Integer> Zero(dim,0);
        ModuleGeneratorsOverOriginalMonoid.push_front(Zero);
        // cout << "Mod " << endl;
        // Matrix<Integer>(ModuleGeneratorsOverOriginalMonoid).pretty_print(cout);
        // cout << "--------" << endl;
        is_Computed.set(ConeProperty::ModuleGeneratorsOverOriginalMonoid,true);
        return;
    }
    
    CandidateList<Integer> Level1OriGens;
    for(size_t i=0;i<nr_gen;++i){
            if(gen_levels[i]==1){
                Level1OriGens.push_back(Candidate<Integer>(Generators[i],*this));    
            }
    }
    CandidateList<Integer> Level1Generators=Level1OriGens;
    Candidate<Integer> new_cand(dim,Support_Hyperplanes.nr_of_rows());
    typename list<Candidate<Integer> >::const_iterator lnew,l1;
    for(lnew=NewCandidates.Candidates.begin();lnew!=NewCandidates.Candidates.end();++lnew){
        Integer level=v_scalar_product(lnew->cand,Truncation);
        if(level==1){
            new_cand=*lnew;
            Level1Generators.reduce_by_and_insert(new_cand,OldCandidates);
        }
        else{
            for(l1=Level1OriGens.Candidates.begin();l1!=Level1OriGens.Candidates.end();++l1){
                new_cand=sum(*l1,*lnew);
                Level1Generators.reduce_by_and_insert(new_cand,OldCandidates);
            }
        }        
    }
    Level1Generators.extract(ModuleGeneratorsOverOriginalMonoid);
    ModuleGeneratorsOverOriginalMonoid.sort();
    ModuleGeneratorsOverOriginalMonoid.unique();
    is_Computed.set(ConeProperty::ModuleGeneratorsOverOriginalMonoid,true);
    
    for (size_t i = 0; i <nr_gen; i++) { // the level 1 input generators have not yet ben inserted into OldCandidates              
        if(gen_levels[i]==1) {          // but they are needed for the truncated Hilbert basis com¸putation
            NewCandidates.Candidates.push_back(Candidate<Integer>(Generators[i],*this));
            NewCandidates.Candidates.back().original_generator=true;
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::make_module_gens_and_extract_HB(){
       
    make_module_gens();
    
    NewCandidates.divide_sortdeg_by2(); // was previously multplied by 2    
    NewCandidates.sort_by_deg();
    
    OldCandidates.merge(NewCandidates);
    OldCandidates.auto_reduce(); 
}


//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::primal_algorithm_set_computed() {

    extreme_rays_and_deg1_check();
    if(!pointed){
        throw NonpointedException();
    }

    if (do_triangulation || do_partial_triangulation) {
        is_Computed.set(ConeProperty::TriangulationSize,true);
        if (do_evaluation) {
            is_Computed.set(ConeProperty::TriangulationDetSum,true);
        }
    }
    if (do_triangulation && do_evaluation && isComputed(ConeProperty::Grading))
        is_Computed.set(ConeProperty::Multiplicity,true);
                
    if (do_Hilbert_basis) {
        if(do_module_gens_intcl){
                make_module_gens_and_extract_HB();
        }
        OldCandidates.sort_by_val();
        OldCandidates.extract(Hilbert_Basis);
        OldCandidates.Candidates.clear();
        Hilbert_Basis.unique();
        is_Computed.set(ConeProperty::HilbertBasis,true);
        if (isComputed(ConeProperty::Grading)) {
            select_deg1_elements();
            check_deg1_hilbert_basis();
        }
    }
    
    if (do_deg1_elements) {
        for(size_t i=0;i<nr_gen;i++)
            if(v_scalar_product(Grading,Generators[i])==1) //TODO in_triang[i] &&
                Deg1_Elements.push_front(Generators[i]);
        is_Computed.set(ConeProperty::Deg1Elements,true);
        Deg1_Elements.sort();
        Deg1_Elements.unique();
    }
    if (do_h_vector) {
        Hilbert_Series.setShift(convertTo<long>(shift));
        Hilbert_Series.adjustShift();
        // now the shift in the HilbertSeries may change and we would have to adjust
        // the shift, the grading and more in the Full_Cone to continue to add data!
            // COMPUTE HSOP here
        if (do_hsop){
            compute_hsop();
            is_Computed.set(ConeProperty::HSOP);
        }
        Hilbert_Series.simplify();
        is_Computed.set(ConeProperty::HilbertSeries);
    }
    if(do_Stanley_dec){
        is_Computed.set(ConeProperty::StanleyDec);
    }
    
}

   
//---------------------------------------------------------------------------
// Normaliz modes (public)
//---------------------------------------------------------------------------

// check the do_* bools, they must be set in advance
// this method (de)activate them according to dependencies between them
template<typename Integer>
void Full_Cone<Integer>::do_vars_check(bool with_default) {

    do_extreme_rays=true; // we always want to do this if compute() is called

    if (do_default_mode && with_default) {
        do_Hilbert_basis = true;
        do_h_vector = true;
        if(!inhomogeneous)
            do_class_group=true;
    }

    // activate implications
    if (do_module_gens_intcl) do_Hilbert_basis= true;
    if (do_module_gens_intcl) use_bottom_points= false;
    //if (do_hsop)            do_Hilbert_basis = true;
    if (do_Stanley_dec)     keep_triangulation = true;
    if (do_cone_dec)        keep_triangulation = true;
    if (keep_triangulation) do_determinants = true;
    if (do_multiplicity)    do_determinants = true;
    if ((do_multiplicity || do_h_vector) && inhomogeneous)    do_module_rank = true;
    if (do_determinants)    do_triangulation = true;
    if (do_h_vector)        do_triangulation = true;
    if (do_deg1_elements)   do_partial_triangulation = true;
    if (do_Hilbert_basis)   do_partial_triangulation = true;
    // activate 
    do_only_multiplicity = do_determinants;
    stop_after_cone_dec = true;
    if(do_cone_dec)          do_only_multiplicity=false;
        
    if (do_Stanley_dec || do_h_vector || do_deg1_elements 
                     || do_Hilbert_basis) {
        do_only_multiplicity = false;
        stop_after_cone_dec = false;
        do_evaluation = true;
    }
    if (do_determinants)    do_evaluation = true;

    // deactivate
    if (do_triangulation)   do_partial_triangulation = false;
    if (do_Hilbert_basis)   do_deg1_elements = false; // they will be extracted later
}


// general purpose compute method
// do_* bools must be set in advance, this method does sanity checks for it
// if no bool is set it does support hyperplanes and extreme rays
template<typename Integer>
void Full_Cone<Integer>::compute() {
    
    if(dim==0){
        set_zero_cone();
        return;
    }
    

    do_vars_check(false);
    explicit_full_triang=do_triangulation; // to distinguish it from do_triangulation via default mode
    if(do_default_mode)
        do_vars_check(true);
    if (do_integrally_closed) {
        if (do_Hilbert_basis) {
            do_integrally_closed = false; // don't interrupt the computation
        } else {
            do_Hilbert_basis = true;
            do_vars_check(false);
        }
    }

    start_message();
    
    if(Support_Hyperplanes.nr_of_rows()==0 && !do_Hilbert_basis && !do_h_vector && !do_multiplicity && !do_deg1_elements
        && !do_Stanley_dec && !do_triangulation && !do_determinants)
        assert(Generators.max_rank_submatrix_lex().size() == dim);

    minimize_support_hyperplanes(); // if they are given
    if (inhomogeneous)
        set_levels();
    
    check_given_grading();

    if ((!do_triangulation && !do_partial_triangulation)
            || (Grading.size()>0 && !isComputed(ConeProperty::Grading))){
            // in the second case there are only two possibilities:
            // either nonpointed or bad grading
        do_triangulation=false;
        do_partial_triangulation=false;
        support_hyperplanes();
    }
    else{
        // look for a grading if it is needed
        find_grading();        
        if(isComputed(ConeProperty::IsPointed) && !pointed){
            end_message();
            return;
        }
        
        if (!isComputed(ConeProperty::Grading))
            disable_grading_dep_comp();

        bool polyhedron_is_polytope=inhomogeneous;
        if(inhomogeneous){
            find_level0_dim();
            for(size_t i=0;i<nr_gen;++i)
                if(gen_levels[i]==0){
                    polyhedron_is_polytope=false;
                    break;
                }                
        }

        set_degrees();
        sort_gens_by_degree(true);

        if(do_approximation && !deg1_generated){
            if(!isComputed(ConeProperty::ExtremeRays) || !isComputed(ConeProperty::SupportHyperplanes)){
                do_extreme_rays=true;
                dualize_cone(false);// no start or end message
            }
            if(verbose)
                verboseOutput() << "Approximating rational by lattice polytope" << endl;
            if(do_deg1_elements){
                compute_deg1_elements_via_approx_global();
                is_Computed.set(ConeProperty::Deg1Elements,true);
                if(do_triangulation){
                    do_deg1_elements=false;
                    do_partial_triangulation=false;
                    do_only_multiplicity = do_determinants;
                    primal_algorithm();            
                }
            } else { // now we want subdividing elements for a simplicial cone
                assert(do_Hilbert_basis);
                compute_elements_via_approx(Hilbert_Basis);            
            }
            
        }
        else{
            if(polyhedron_is_polytope && (do_Hilbert_basis || do_h_vector || do_multiplicity)){ // inthis situation we must find the 
                convert_polyhedron_to_polytope();                  // lattice points in a polytope
            }
            else
                primal_algorithm();            
        }
            
        if(inhomogeneous){
            find_module_rank();
            // cout << "module rank " << module_rank << endl;
        }
        
    }  
    
    end_message();
}

template<typename Integer>
void Full_Cone<Integer>::compute_hsop(){
        vector<long> hsop_deg(dim,1);
        // if all extreme rays are in degree one, there is nothing to compute
        if (!isDeg1ExtremeRays()){
            if(verbose){
            verboseOutput() << "Computing heights ... " << flush;
            }
            
            vector<bool> choice = Extreme_Rays_Ind;
            if (inhomogeneous){
                for (size_t i=0; i<Generators.nr_of_rows(); i++) {
                    if (Extreme_Rays_Ind[i] && v_scalar_product(Generators[i],Truncation) != 0) {
                        choice[i]=false;
                    }
                }
            }
            Matrix<Integer> ER = Generators.submatrix(choice);
            Matrix<Integer> SH = getSupportHyperplanes();
            if (inhomogeneous){
                    Sublattice_Representation<Integer> recession_lattice(ER,true);
                    Matrix<Integer> SH_raw = recession_lattice.to_sublattice_dual(SH);
                    Matrix<Integer> ER_embedded = recession_lattice.to_sublattice(ER);
                    Full_Cone<Integer> recession_cone(ER_embedded);
                    recession_cone.Support_Hyperplanes = SH_raw;
                    recession_cone.dualize_cone();
                    SH = recession_lattice.from_sublattice_dual(recession_cone.getSupportHyperplanes());
            }
            vector<size_t> ideal_heights(ER.nr_of_rows(),1);
            // the heights vector is clear in the simplicial case
            if (is_simplicial){
                    for (size_t j=0;j<ideal_heights.size();j++) ideal_heights[j]=j+1;
            } else {
                list<pair<boost::dynamic_bitset<> , size_t>> facet_list;
                list<vector<key_t>> facet_keys;
                vector<key_t> key;
                size_t d = dim;
                if (inhomogeneous) d = level0_dim;
                for (size_t i=SH.nr_of_rows();i-->0;){
                    boost::dynamic_bitset<> new_facet(ER.nr_of_rows());
                    key.clear();
                    for (size_t j=0;j<ER.nr_of_rows();j++){
                        if (v_scalar_product(SH[i],ER[j])==0){
                            new_facet[new_facet.size()-1-j]=1;
                        } else {
                            key.push_back(j);
                        }
                    }
                    facet_list.push_back(make_pair(new_facet,d-1));
                    facet_keys.push_back(key);
                }
                facet_list.sort(); // should be sorted lex
                //~ cout << "FACETS:" << endl;
                //~ //cout << "size: " << facet_list.size() << " | " << facet_list << endl;
                //~ for (auto jt=facet_list.begin();jt!=facet_list.end();++jt){
                        //~ cout << jt->first << " | " << jt->second << endl;
                //~ }
                //cout << "facet non_keys: " << facet_keys << endl;
                heights(facet_keys,facet_list,ER.nr_of_rows()-1,ideal_heights,d-1);
            }
        if(verbose){
            verboseOutput() << "done." << endl;
            assert(ideal_heights[ER.nr_of_rows()-1]==dim);
            verboseOutput() << "Heights vector: " << ideal_heights << endl;   
        }
        vector<Integer> er_deg = ER.MxV(Grading);
        hsop_deg = convertTo<vector<long> >(degrees_hsop(er_deg,ideal_heights));
        } 
        if(verbose){
            verboseOutput() << "Degrees of HSOP: " << hsop_deg << endl;   
        }
        Hilbert_Series.setHSOPDenom(hsop_deg);
}



// recursive method to compute the heights
// TODO: at the moment: facets are a parameter. global would be better
template<typename Integer>
void Full_Cone<Integer>::heights(list<vector<key_t>>& facet_keys,list<pair<boost::dynamic_bitset<>,size_t>> faces, size_t index,vector<size_t>& ideal_heights,size_t max_dim){
    // since we count the index backwards, this is the actual nr of the extreme ray
    size_t ER_nr = ideal_heights.size()-index-1;
    //~ cout << "starting calculation for extreme ray nr " << ER_nr << endl;
    list<pair<boost::dynamic_bitset<>,size_t>> not_faces;
    auto face_it=faces.begin();
    for (;face_it!=faces.end();++face_it){
        if (face_it->first.test(index)){ // check whether index is set
            break;
        }
        // resize not_faces
        face_it->first.resize(index);
    }
    not_faces.splice(not_faces.begin(),faces,faces.begin(),face_it);
    //~ cout << "faces not containing it:" << endl;
    //~ for (auto jt=not_faces.begin();jt!=not_faces.end();++jt){
                    //~ cout << jt->first << " | " << jt->second << endl;
    //~ }
    //~ cout << "faces containing it:" << endl;
    //~ for (auto jt=faces.begin();jt!=faces.end();++jt){
                    //~ cout << jt->first << " | " << jt->second << endl;
    //~ }
    auto not_faces_it=not_faces.begin();
    // update the heights
    if (ER_nr>0){
        if (!not_faces.empty()){
            ideal_heights[ER_nr] = ideal_heights[ER_nr-1];
            // compute the dimensions of not_faces
            vector<bool> choice = Extreme_Rays_Ind;
            if (inhomogeneous){
                for (size_t i=0; i<Generators.nr_of_rows(); i++) {
                    if (Extreme_Rays_Ind[i] && v_scalar_product(Generators[i],Truncation) != 0) {
                        choice[i]=false;
                    }
                }
            }
            Matrix<Integer> ER = Generators.submatrix(choice);
            int tn;
            if(omp_get_level()==0)
                tn=0;
            else  tn = omp_get_ancestor_thread_num(1);
            Matrix<Integer>& Test = Top_Cone->RankTest[tn];
            vector<key_t> face_key;
            for (;not_faces_it!=not_faces.end();++not_faces_it){
                if (not_faces_it->second==0){ // dimension has not yet been computed
                    // generate the key vector
                    face_key.resize(0);
                    for (size_t i=0;i<not_faces_it->first.size();++i){
                        if (not_faces_it->first.test(i)){
                            face_key.push_back(ER.nr_of_rows()-1-i);
                        }
                    }
                    not_faces_it->second = Test.rank_submatrix(ER,face_key);
                }
                if (not_faces_it->second==max_dim) break;
            }
            if (not_faces_it==not_faces.end()) {
                --max_dim;
                ideal_heights[ER_nr] = ideal_heights[ER_nr-1]+1;
            }
        } else {
            ideal_heights[ER_nr] = ideal_heights[ER_nr-1]+1;
            --max_dim;
        }
    }
    // we computed all the heights
    if (index==0) return;
    // if inner point, we can skip now
    
    // take the union of all faces not containing the current extreme ray
    boost::dynamic_bitset<> union_faces(index);
    not_faces_it = not_faces.begin();
    for (;not_faces_it!=not_faces.end();++not_faces_it){
        union_faces |= not_faces_it->first; // take the union
    }
    //cout << "Their union: " << union_faces << endl;
    // the not_faces now already have a size one smaller
    union_faces.resize(index+1);
    list<pair<boost::dynamic_bitset<>,size_t>> new_faces;
    // delete all facets which only consist of the previous extreme rays
    auto facet_it=facet_keys.begin();
    size_t counter=0;
    while(facet_it!=facet_keys.end()){
        counter=0;
        for (size_t i=0;i<facet_it->size();i++){
            if (facet_it->at(i)<=ER_nr) continue;
            // now we only have new extreme rays
            counter = i;
            break;
        }
        size_t j=ER_nr+1;
        for (;j<ideal_heights.size();j++){
            if (facet_it->at(counter)!=j){ // facet contains the element j
                    break;
            } else if (counter < facet_it->size()-1) counter++;
        }
        if (j==ideal_heights.size()){
            facet_it = facet_keys.erase(facet_it);
        } else ++facet_it;
    }
    facet_it=facet_keys.begin();
    
    // main loop
    for (;facet_it!=facet_keys.end();++facet_it){
        // check whether the facet is contained in the faces not containing the generator
        // and the previous generators
        // and check whether the generator is in the facet    
        // check whether intersection with facet contributes
        bool not_containing_el =false;
        // bool whether the facet contains an element which is NOT in the faces not containing the current extreme ray
        bool containing_critical_el=false; 
        counter=0;
        //cout << "check critical for facet " << *it << endl;
        for (size_t i=0;i<facet_it->size();i++){
            if (facet_it->at(i)==ER_nr){
                not_containing_el = true;
            }
            if (facet_it->at(i)<=ER_nr && i<facet_it->size()-1) continue;
            counter=i; // now we have elements which are bigger than the current extreme ray
            if (not_containing_el){
                for (size_t j=ER_nr+1;j<ideal_heights.size();j++){
                    if (facet_it->at(counter)!=j){ // i.e. j is in the facet
                        if (!union_faces.test(ideal_heights.size()-1-j)){
                            containing_critical_el = true;
                            break;
                        }
                    } else if (counter<facet_it->size()-1) counter++;
                }
            }
            break;
        }
        if(not_containing_el && containing_critical_el){ //facet contributes
            //cout << "Taking intersections with the facet " << *facet_it << endl;
            face_it =faces.begin();
            for (;face_it!=faces.end();++face_it){
                boost::dynamic_bitset<> intersection(face_it->first);
                for (size_t i=0;i<facet_it->size();i++){
                    if (facet_it->at(i)>ER_nr) intersection.set(ideal_heights.size()-1-facet_it->at(i),false);
                }
                intersection.resize(index);
                if (intersection.any()){
                    // check whether the intersection lies in any of the not_faces
                    not_faces_it = not_faces.begin();
                    for (;not_faces_it!=not_faces.end();++not_faces_it){
                            if (intersection.is_subset_of(not_faces_it->first)) break;
                    }
                    if (not_faces_it== not_faces.end()) new_faces.push_back(make_pair(intersection,0)); 
                }
            }
       }
    }
    // the new faces need to be sort in lex order anyway. this can be used to reduce operations
    // for subset checking
    new_faces.sort();
    auto outer_it = new_faces.begin();
    auto inner_it = new_faces.begin();
    for (;outer_it!=new_faces.end();++outer_it){
        // work with a not-key vector
        vector<key_t> face_not_key;
        for (size_t i=0;i<outer_it->first.size();i++){
            if (!outer_it->first.test(i)){
                face_not_key.push_back(i);
            }
        }
        inner_it=new_faces.begin();
        size_t i=0;
        while (inner_it!=outer_it){
            i=0;
            for (;i<face_not_key.size();++i){
                if (inner_it->first.test(face_not_key[i])) break; //inner_it has an element which is not in outer_it
            }
            if (i==face_not_key.size()){
                inner_it = new_faces.erase(inner_it); //inner_it is a subface of outer_it
            } else ++inner_it;
        }
    }
    new_faces.merge(not_faces);
    /*cout << "The new faces: " << endl;
    for (auto jt=new_faces.begin();jt!=new_faces.end();++jt){
                    cout << jt->first << " | " << jt->second << endl;
    }*/
    
    heights(facet_keys,new_faces,index-1,ideal_heights,max_dim);
}



template<typename Integer>
void Full_Cone<Integer>::convert_polyhedron_to_polytope() {
    
    if(verbose){
        verboseOutput() << "Converting polyhedron to polytope" << endl;
        verboseOutput() << "Pointed since cone over polytope" << endl;      
    }
    pointed=true;
    is_Computed.set(ConeProperty::IsPointed);    
    Full_Cone Polytope(Generators);
    Polytope.pointed=true;
    Polytope.is_Computed.set(ConeProperty::IsPointed);    
    Polytope.keep_order=true;
    Polytope.Grading=Truncation;
    Polytope.is_Computed.set(ConeProperty::Grading);
    if(isComputed(ConeProperty::SupportHyperplanes)){
        Polytope.Support_Hyperplanes=Support_Hyperplanes;
        Polytope.nrSupport_Hyperplanes=nrSupport_Hyperplanes;
        Polytope.is_Computed.set(ConeProperty::SupportHyperplanes);     
    }
    if(isComputed(ConeProperty::ExtremeRays)){
        Polytope.Extreme_Rays_Ind=Extreme_Rays_Ind;
        Polytope.is_Computed.set(ConeProperty::ExtremeRays);        
    }
    Polytope.do_deg1_elements=true;
    Polytope.verbose=verbose;
    Polytope.compute();
    if(Polytope.isComputed(ConeProperty::SupportHyperplanes) && 
                    !isComputed(ConeProperty::SupportHyperplanes)){
        Support_Hyperplanes=Polytope.Support_Hyperplanes;
        nrSupport_Hyperplanes=Polytope.nrSupport_Hyperplanes;
        is_Computed.set(ConeProperty::SupportHyperplanes);  
    }
    if(Polytope.isComputed(ConeProperty::ExtremeRays) &&
                    !isComputed(ConeProperty::ExtremeRays)){
        Extreme_Rays_Ind=Polytope.Extreme_Rays_Ind;
        is_Computed.set(ConeProperty::ExtremeRays);     
    }
    if(Polytope.isComputed(ConeProperty::Deg1Elements)){
        module_rank=Polytope.Deg1_Elements.size();
        if(do_Hilbert_basis){
            Hilbert_Basis=Polytope.Deg1_Elements;
            is_Computed.set(ConeProperty::HilbertBasis);
        }
        is_Computed.set(ConeProperty::ModuleRank);
        if(isComputed(ConeProperty::Grading) && module_rank>0){
            multiplicity=1; // of the recession cone;
            is_Computed.set(ConeProperty::Multiplicity);
            if(do_h_vector){
                vector<num_t> hv(1);
                typename list<vector<Integer> >::const_iterator hb=Polytope.Deg1_Elements.begin();
                for(;hb!=Polytope.Deg1_Elements.end();++hb){
                    size_t deg = convertTo<long>(v_scalar_product(Grading,*hb));
                    if(deg+1>hv.size())
                        hv.resize(deg+1);
                    hv[deg]++;                        
                }    
                Hilbert_Series.add(hv,vector<denom_t>());
                Hilbert_Series.setShift(convertTo<long>(shift));
                Hilbert_Series.adjustShift();
                Hilbert_Series.simplify();
                is_Computed.set(ConeProperty::HilbertSeries);
            }
        }  
    }   
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::compute_deg1_elements_via_approx_global() {
    
    compute_elements_via_approx(Deg1_Elements);
    
    typename list<vector<Integer> >::iterator e;
    for(e=Deg1_Elements.begin(); e!=Deg1_Elements.end();)
        if(!contains(*e))
            e=Deg1_Elements.erase(e);
        else
            ++e;
        if(verbose)
            verboseOutput() << Deg1_Elements.size() << " deg 1 elements found" << endl; 
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::compute_elements_via_approx(list<vector<Integer> >& elements_from_approx) {

    if (!isComputed(ConeProperty::Grading)){
        support_hyperplanes(); // the only thing we can do now
        return;
    }
    
    assert(elements_from_approx.empty());

    Full_Cone C_approx(latt_approx()); // latt_approx computes a matrix of generators
    /* cout << "====================" << endl;
    C_approx.Generators.pretty_print(cout);
    cout << "====================" << endl; */
    C_approx.verbose=verbose;
    C_approx.is_approximation=true;
    C_approx.approx_level = approx_level;
    // C_approx.Generators.pretty_print(cout);
    C_approx.do_deg1_elements=do_deg1_elements;  // for supercone C_approx that is generated in degree 1
    C_approx.do_Hilbert_basis=do_Hilbert_basis;
    C_approx.do_all_hyperplanes=false;    // we use the support Hyperplanes of the approximated cone for the selection of elements
    C_approx.Support_Hyperplanes=Support_Hyperplanes;
    C_approx.is_Computed.set(ConeProperty::SupportHyperplanes);
    C_approx.nrSupport_Hyperplanes = nrSupport_Hyperplanes;
    C_approx.Grading = Grading;
    C_approx.is_Computed.set(ConeProperty::Grading);
    C_approx.Truncation=Truncation;
    C_approx.TruncLevel=TruncLevel;

    if(verbose)
        verboseOutput() << "Computing elements in approximating cone with "
                        << C_approx.Generators.nr_of_rows() << " generators." << endl;
    
	
	// TODO different verbosity option!
	bool verbose_tmp = verbose;
	verbose =false;
    C_approx.compute();
    verbose = verbose_tmp;
    if(!C_approx.contains(*this) || Grading!=C_approx.Grading){
        throw FatalException("Wrong approximating cone.");
    }

    if(verbose)
        verboseOutput() << "Sum of dets of simplicial cones evaluated in approximation = " << C_approx.detSum << endl;   
    
    if(verbose)
        verboseOutput() << "Returning to original cone" << endl;
    if(do_deg1_elements)
        elements_from_approx.splice(elements_from_approx.begin(),C_approx.Deg1_Elements);
    if(do_Hilbert_basis)
        elements_from_approx.splice(elements_from_approx.begin(),C_approx.Hilbert_Basis);
}


// -s
template<typename Integer>
void Full_Cone<Integer>::support_hyperplanes() { 
    if(!isComputed(ConeProperty::SupportHyperplanes)){
        sort_gens_by_degree(false); // we do not want to triangulate here
        build_top_cone();           
    }
    extreme_rays_and_deg1_check();
    if(inhomogeneous){
        find_level0_dim();
        if(do_module_rank) 
            find_module_rank();
    }
    compute_class_group();
}

//---------------------------------------------------------------------------
// Checks and auxiliary algorithms
//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::extreme_rays_and_deg1_check() {
    check_pointed();
    if(!pointed){
        throw NonpointedException();
    }
    //cout << "Generators" << endl;
    //Generators.pretty_print(cout);
    //cout << "SupportHyperplanes" << endl;
    //Support_Hyperplanes.pretty_print(cout);
    compute_extreme_rays();
    deg1_check();
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::check_given_grading(){
    
    if(Grading.size()==0)
        return;

    bool positively_graded=true;
    
    if(!isComputed(ConeProperty::Grading)){
        size_t neg_index=0;
        Integer neg_value;
        bool nonnegative=true;
        vector<Integer> degrees = Generators.MxV(Grading);
        for (size_t i=0; i<degrees.size(); ++i) {
            if (degrees[i]<=0 && (!inhomogeneous || gen_levels[i]==0)) { 
                // in the inhomogeneous case: test only generators of tail cone
                positively_graded=false;;
                if(degrees[i]<0){
                    nonnegative=false;
                    neg_index=i;
                    neg_value=degrees[i];
                }
            }
        }

        if(!nonnegative){
            throw BadInputException("Grading gives negative value "
                    + toString(neg_value) + " for generator "
                    + toString(neg_index+1) + "!");
        }
    }
    
    if(positively_graded){
        is_Computed.set(ConeProperty::Grading);    
        if(inhomogeneous)
            find_grading_inhom();
        set_degrees();
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::find_grading(){
    
    if(inhomogeneous) // in the inhomogeneous case we do not allow implicit grading
        return;

    deg1_check(); // trying to find grading under which all generators have the same degree
    if (!isComputed(ConeProperty::Grading) && (do_multiplicity || do_deg1_elements || do_h_vector)) {
        if (!isComputed(ConeProperty::ExtremeRays)) {
            if (verbose) {
                verboseOutput() << "Cannot find grading s.t. all generators have the degree 1! Computing Extreme rays first:" << endl;
            }
            get_supphyps_from_copy(true);
            extreme_rays_and_deg1_check();
            if(!pointed){
                throw NonpointedException();
            };

            // We keep the SupportHyperplanes, so we do not need to recompute them
            // for the last generator, and use them to make a global reduction earlier
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::find_level0_dim(){

    if(!isComputed(ConeProperty::Generators)){
        throw FatalException("Missing Generators.");
    }
    
    Matrix<Integer> Help(nr_gen,dim);
    for(size_t i=0; i<nr_gen;++i)
        if(gen_levels[i]==0)
            Help[i]=Generators[i];
        
    ProjToLevel0Quot=Help.kernel();
    
    level0_dim=dim-ProjToLevel0Quot.nr_of_rows();
    is_Computed.set(ConeProperty::RecessionRank);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::find_module_rank(){
    
    if(isComputed(ConeProperty::ModuleRank))
        return;   

    if(level0_dim==dim){
        module_rank=0;
        is_Computed.set(ConeProperty::ModuleRank);
        return;
    }     
    if(isComputed(ConeProperty::HilbertBasis)){
        find_module_rank_from_HB();
        return;
    }

    // size_t HBrank = module_rank;

    if(do_module_rank)
        find_module_rank_from_proj();
    
    /* if(isComputed(ConeProperty::HilbertBasis))
        assert(HBrank==module_rank);
    */
    
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::find_module_rank_from_proj(){
    
    if(verbose){
        verboseOutput() << "Computing projection to quotient mod level 0" << endl;
    } 

    Matrix<Integer> ProjGen(nr_gen,dim-level0_dim);
    for(size_t i=0;i<nr_gen;++i){
        ProjGen[i]=ProjToLevel0Quot.MxV(Generators[i]);
    }
    
    vector<Integer> GradingProj=ProjToLevel0Quot.transpose().solve_ZZ(Truncation);
    
    Full_Cone<Integer> Cproj(ProjGen);
    Cproj.verbose=false;
    Cproj.Grading=GradingProj;
    Cproj.is_Computed.set(ConeProperty::Grading);
    Cproj.do_deg1_elements=true;
    Cproj.compute();
    
    module_rank=Cproj.Deg1_Elements.size();
    is_Computed.set(ConeProperty::ModuleRank);
    return;
}
        
//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::find_module_rank_from_HB(){
    
    if(level0_dim==0){
        module_rank=Hilbert_Basis.size();
        is_Computed.set(ConeProperty::ModuleRank);
        return;        
    }

    
    set<vector<Integer> > Quotient;
    vector<Integer> v;
    
    // cout << "=======================" << endl;
    // ProjToLevel0Quot.print(cout);
    // cout << "=======================" << endl;
    
    typename list<vector<Integer> >::iterator h;
    
    for(h=Hilbert_Basis.begin();h!=Hilbert_Basis.end();++h){
        v=ProjToLevel0Quot.MxV(*h);
        bool zero=true;
        for(size_t j=0;j<v.size();++j)
            if(v[j]!=0){
                zero=false;
                break;
            }
        if(!zero)
            Quotient.insert(v);
    }
    
    module_rank=Quotient.size();
    is_Computed.set(ConeProperty::ModuleRank);

}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::find_grading_inhom(){

    if(Grading.size()==0 || Truncation.size()==0){
         throw FatalException("Cannot find grading in the inhomogeneous case!");
    }
    
    if(shift!=0)  // to avoid double computation
        return;

    bool first=true;
    Integer level,degree,quot=0,min_quot=0;
    for(size_t i=0;i<nr_gen;++i){
        level=v_scalar_product(Truncation,Generators[i]);
        if(level==0)
            continue;
        degree=v_scalar_product(Grading,Generators[i]);
        quot=degree/level;
        // cout << Generators[i];
        // cout << "*** " << degree << " " << level << " " << quot << endl;
        if(level*quot>=degree)
            quot--;
        if(first){
            min_quot=quot;
            first=false;
        }
        if(quot<min_quot)
            min_quot=quot;
        // cout << "+++ " << min_quot << endl;
    }
    shift = min_quot;
    for(size_t i=0;i<dim;++i) // under this grading all generators have positive degree
        Grading[i] = Grading[i] - shift * Truncation[i];
        
    // shift--;  // NO LONGER correction for the Hilbert series computation to have it start in degree 0
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::set_degrees() {

    // Generators.pretty_print(cout);
    // cout << "Grading " << Grading;
    if (gen_degrees.size() != nr_gen && isComputed(ConeProperty::Grading)) // now we set the degrees
    {
        gen_degrees.resize(nr_gen);
        vector<Integer> gen_degrees_Integer=Generators.MxV(Grading);
        for (size_t i=0; i<nr_gen; i++) {
            if (gen_degrees_Integer[i] < 1) {
                throw BadInputException("Grading gives non-positive value "
                        + toString(gen_degrees_Integer[i])
                        + " for generator " + toString(i+1) + ".");
            }
            convert(gen_degrees[i], gen_degrees_Integer[i]);
        }
    }
    
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::set_levels() {
    if(inhomogeneous && Truncation.size()!=dim){
        throw FatalException("Truncation not defined in inhomogeneous case.");
    }    
    
    // cout <<"trunc " << Truncation;

    if(gen_levels.size()!=nr_gen) // now we compute the levels
    {
        gen_levels.resize(nr_gen);
        vector<Integer> gen_levels_Integer=Generators.MxV(Truncation);
        for (size_t i=0; i<nr_gen; i++) {
            if (gen_levels_Integer[i] < 0) {
                throw FatalException("Truncation gives non-positive value "
                        + toString(gen_levels_Integer[i]) + " for generator "
                        + toString(i+1) + ".");
            }
            convert(gen_levels[i], gen_levels_Integer[i]);
            // cout << "Gen " << Generators[i];
            // cout << "level " << gen_levels[i] << endl << "----------------------" << endl;
        }
    }
    
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::sort_gens_by_degree(bool triangulate) {
    // if(deg1_extreme_rays)  // gen_degrees.size()==0 || 
    // return;
    
    if(keep_order)
        return;
    
    Matrix<Integer> Weights(0,dim);
    vector<bool> absolute;
    if(triangulate){
        if(isComputed(ConeProperty::Grading)){
            Weights.append(Grading);
            absolute.push_back(false);
        }
        else{
            Weights.append(vector<Integer>(dim,1));
            absolute.push_back(true);
        }
    }
    
    vector<key_t> perm=Generators.perm_by_weights(Weights,absolute);
    Generators.order_rows_by_perm(perm);
    order_by_perm(Extreme_Rays_Ind,perm);
    if(isComputed(ConeProperty::Grading))
        order_by_perm(gen_degrees,perm);
    if(inhomogeneous && gen_levels.size()==nr_gen)
        order_by_perm(gen_levels,perm);
    compose_perm_gens(perm);

    if(triangulate){
        Integer roughness;
        if(isComputed(ConeProperty::Grading)){
                roughness=gen_degrees[nr_gen-1]/gen_degrees[0];
        }
        else{
            Integer max_norm=0, min_norm=0;
            for(size_t i=0;i<dim;++i){
            max_norm+=Iabs(Generators[nr_gen-1][i]);
            min_norm+=Iabs(Generators[0][i]); 
            }
            roughness=max_norm/min_norm;
        }
        if(verbose){
            verboseOutput() << "Roughness " << roughness <<  endl;
        }
        
        if(roughness >= 10){
            do_bottom_dec=true;
            if(verbose){
                    verboseOutput() << "Bottom decomposition activated" << endl;
            }
        }
    }
    
    if (verbose) {
        if(triangulate){
            if(isComputed(ConeProperty::Grading)){
                verboseOutput() <<"Generators sorted by degree and lexicographically" << endl;
                verboseOutput() << "Generators per degree:" << endl;
                verboseOutput() << count_in_map<long,long>(gen_degrees);
            }
            else
                verboseOutput() << "Generators sorted by 1-norm and lexicographically" << endl;
        }
        else{
            verboseOutput() << "Generators sorted lexicographically" << endl;
        }
    }
    keep_order=true;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::compose_perm_gens(const vector<key_t>& perm) {
    order_by_perm(PermGens,perm);
}

//---------------------------------------------------------------------------

// an alternative to compute() for the basic tasks that need no triangulation
template<typename Integer>
void Full_Cone<Integer>::dualize_cone(bool print_message){
    
    if(dim==0){
        set_zero_cone();
        return;
    }

    // DO NOT CALL do_vars_check!!

    bool save_tri      = do_triangulation;
    bool save_part_tri = do_partial_triangulation;
    do_triangulation         = false;
    do_partial_triangulation = false;
    
    if(print_message) start_message();
    
    sort_gens_by_degree(false);
    
    if(!isComputed(ConeProperty::SupportHyperplanes))
        build_top_cone();
    
    if(do_pointed)
        check_pointed(); 

    do_triangulation         = save_tri;
    do_partial_triangulation = save_part_tri;
    if(print_message) end_message();
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<key_t> Full_Cone<Integer>::find_start_simplex() const {
        return Generators.max_rank_submatrix_lex();
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Full_Cone<Integer>::select_matrix_from_list(const list<vector<Integer> >& S,
                                   vector<size_t>& selection){

    sort(selection.begin(),selection.end());
    assert(selection.back()<S.size());
    size_t i=0,j=0;
    size_t k=selection.size();
    Matrix<Integer> M(selection.size(),S.front().size());
    typename list<vector<Integer> >::const_iterator ll=S.begin();
    for(;ll!=S.end()&&i<k;++ll){
        if(j==selection[i]){
            M[i]=*ll;
            i++;
        }
        j++;
    }
    return M;
}

//---------------------------------------------------------------------------

template<typename Integer>

void Full_Cone<Integer>::minimize_support_hyperplanes(){
    if(Support_Hyperplanes.nr_of_rows() == 0)
        return;
    if(isComputed(ConeProperty::SupportHyperplanes)){
        nrSupport_Hyperplanes=Support_Hyperplanes.nr_of_rows();
        return;
    }
    if (verbose) {
        verboseOutput() << "Minimize the given set of support hyperplanes by "
                        << "computing the extreme rays of the dual cone" << endl;
    }
    Full_Cone<Integer> Dual(Support_Hyperplanes);
    Dual.verbose=verbose;
    Dual.Support_Hyperplanes = Generators;
    Dual.is_Computed.set(ConeProperty::SupportHyperplanes);
    Dual.compute_extreme_rays();
    Support_Hyperplanes = Dual.Generators.submatrix(Dual.Extreme_Rays_Ind); //only essential hyperplanes
    is_Computed.set(ConeProperty::SupportHyperplanes);
    nrSupport_Hyperplanes=Support_Hyperplanes.nr_of_rows();
    do_all_hyperplanes=false;
}
    

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::compute_extreme_rays(bool use_facets){

    if (isComputed(ConeProperty::ExtremeRays))
        return;
    // when we do approximation, we do not have the correct hyperplanes
    // and cannot compute the extreme rays
    if (is_approximation)
        return;
    assert(isComputed(ConeProperty::SupportHyperplanes));
    
    check_pointed();
    if(!pointed){
        throw NonpointedException();
    }

    if(dim*Support_Hyperplanes.nr_of_rows() < nr_gen) {
         compute_extreme_rays_rank(use_facets);
    } else {
         compute_extreme_rays_compare(use_facets);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::compute_extreme_rays_rank(bool use_facets){

    if (verbose) verboseOutput() << "Select extreme rays via rank ... " << flush;

    size_t i;
    vector<key_t> gen_in_hyperplanes;
    gen_in_hyperplanes.reserve(Support_Hyperplanes.nr_of_rows());
    Matrix<Integer> M(Support_Hyperplanes.nr_of_rows(),dim);

    deque<bool> Ext(nr_gen,false);
    #pragma omp parallel for firstprivate(gen_in_hyperplanes,M)
    for(i=0;i<nr_gen;++i){
//        if (isComputed(ConeProperty::Triangulation) && !in_triang[i])
//            continue;
        gen_in_hyperplanes.clear();
        if(use_facets){
            typename list<FACETDATA>::const_iterator IHV=Facets.begin();            
            for (size_t j=0; j<Support_Hyperplanes.nr_of_rows(); ++j, ++IHV){
                if(IHV->GenInHyp.test(i))
                    gen_in_hyperplanes.push_back(j);
            }            
        }
        else{
            for (size_t j=0; j<Support_Hyperplanes.nr_of_rows(); ++j){
            if(v_scalar_product(Generators[i],Support_Hyperplanes[j])==0)
                gen_in_hyperplanes.push_back(j);
            }
        }
        if (gen_in_hyperplanes.size() < dim-1)
            continue;
        if (M.rank_submatrix(Support_Hyperplanes,gen_in_hyperplanes) >= dim-1)
            Ext[i]=true;   
    }
    for(i=0; i<nr_gen;++i)
        Extreme_Rays_Ind[i]=Ext[i];

    is_Computed.set(ConeProperty::ExtremeRays);
    if (verbose) verboseOutput() << "done." << endl;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::compute_extreme_rays_compare(bool use_facets){

    if (verbose) verboseOutput() << "Select extreme rays via comparison ... " << flush;

    size_t i,j,k;
    // Matrix<Integer> SH=getSupportHyperplanes().transpose();
    // Matrix<Integer> Val=Generators.multiplication(SH);
    size_t nc=Support_Hyperplanes.nr_of_rows();
    
    vector<vector<bool> > Val(nr_gen);
    for (i=0;i<nr_gen;++i)
       Val[i].resize(nc);
        
    // In this routine Val[i][j]==1, i.e. true, indicates that
    // the i-th generator is contained in the j-th support hyperplane
    
    vector<key_t> Zero(nc);
    vector<key_t> nr_ones(nr_gen);

    for (i = 0; i <nr_gen; i++) {
        k=0;
        Extreme_Rays_Ind[i]=true;
        if(use_facets){
            typename list<FACETDATA>::const_iterator IHV=Facets.begin();            
            for (j=0; j<Support_Hyperplanes.nr_of_rows(); ++j, ++IHV){
                if(IHV->GenInHyp.test(i)){
                    k++;
                    Val[i][j]=true;                
                }
                else
                Val[i][j]=false;  
            }          
        }
        else{
            for (j = 0; j <nc; ++j) {
                if (v_scalar_product(Generators[i],Support_Hyperplanes[j])==0) {
                    k++;
                    Val[i][j]=true;                
                }
                else
                    Val[i][j]=false;  
            }
        }
        nr_ones[i]=k;
        if (k<dim-1||k==nc)  // not contained in enough facets or in all (0 as generator)
            Extreme_Rays_Ind[i]=false;
    }
    
    maximal_subsets(Val,Extreme_Rays_Ind);    

    is_Computed.set(ConeProperty::ExtremeRays);
    if (verbose) verboseOutput() << "done." << endl;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::compute_class_group() { // from the support hyperplanes
    if(!do_class_group || !isComputed(ConeProperty::SupportHyperplanes) || isComputed(ConeProperty::ClassGroup))
        return;
    Matrix<Integer> Trans=Support_Hyperplanes; // .transpose();
    size_t rk;
    Trans.SmithNormalForm(rk);
    ClassGroup.push_back(Support_Hyperplanes.nr_of_rows()-rk);
    for(size_t i=0;i<rk;++i)
        if(Trans[i][i]!=1)
            ClassGroup.push_back(Trans[i][i]);
    is_Computed.set(ConeProperty::ClassGroup);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::select_deg1_elements() { // from the Hilbert basis

    if(inhomogeneous)
        return;
    typename list<vector<Integer> >::iterator h = Hilbert_Basis.begin();
    for(;h!=Hilbert_Basis.end();h++){
        if(v_scalar_product(Grading,*h)==1)
            Deg1_Elements.push_back(*h);
    }
    is_Computed.set(ConeProperty::Deg1Elements,true);
}

//---------------------------------------------------------------------------

template<typename Integer>
bool Full_Cone<Integer>::contains(const vector<Integer>& v) {
    for (size_t i=0; i<Support_Hyperplanes.nr_of_rows(); ++i)
        if (v_scalar_product(Support_Hyperplanes[i],v) < 0)
            return false;
    return true;
}
//---------------------------------------------------------------------------

template<typename Integer>
bool Full_Cone<Integer>::contains(const Full_Cone& C) {
    for(size_t i=0;i<C.nr_gen;++i)
        if(!contains(C.Generators[i])){
            cerr << "Missing generator " << C.Generators[i] << endl;
            return(false);
    }
    return(true);
}
//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::select_deg1_elements(const Full_Cone& C) {  // from vectors computed in 
                                                              // the auxiliary cone C
    assert(isComputed(ConeProperty::SupportHyperplanes));
    assert(C.isComputed(ConeProperty::Deg1Elements));
    typename list<vector<Integer> >::const_iterator h = C.Deg1_Elements.begin();
    for(;h!=C.Deg1_Elements.end();++h){
        if(contains(*h))
            Deg1_Elements.push_back(*h);
    }
    is_Computed.set(ConeProperty::Deg1Elements,true);
}

//---------------------------------------------------------------------------


// so far only for experiments
/*
template<typename Integer>
void Full_Cone<Integer>::select_Hilbert_Basis(const Full_Cone& C) {  // from vectors computed in 
                                                              // the auxiliary cone C
    assert(isComputed(ConeProperty::SupportHyperplanes));
    assert(C.isComputed(ConeProperty::Deg1Elements));
    typename list<vector<Integer> >::const_iterator h = C.Hilbert_Basis.begin();
    for(;h!=C.Hilbert_Basis.end();++h){
        if(contains(*h))
            // Deg1_Elements.push_back(*h);
            cout << *h;
    }
    exit(0);
    is_Computed.set(ConeProperty::Deg1Elements,true);
}
*/

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::check_pointed() {
    if (isComputed(ConeProperty::IsPointed))
        return;
    assert(isComputed(ConeProperty::SupportHyperplanes));
    if (isComputed(ConeProperty::Grading)){
        pointed=true;
        if (verbose) verboseOutput() << "Pointed since graded" << endl;
        is_Computed.set(ConeProperty::IsPointed);
        return;
    }
    if (verbose) verboseOutput() << "Checking pointedness ... " << flush;

    pointed = (Support_Hyperplanes.max_rank_submatrix_lex().size() == dim);
    is_Computed.set(ConeProperty::IsPointed);
    if(pointed && Grading.size()>0){
        throw BadInputException("Grading not positive on pointed cone.");
    }
    if (verbose) verboseOutput() << "done." << endl;
}


//---------------------------------------------------------------------------
template<typename Integer>
void Full_Cone<Integer>::disable_grading_dep_comp() {

    if (do_multiplicity || do_deg1_elements || do_h_vector) {
        if (do_default_mode) {
            // if (verbose)
            //    verboseOutput() << "No grading specified and cannot find one. "
            //                    << "Disabling some computations!" << endl;
            do_deg1_elements = false;
            do_h_vector = false;
            if(!explicit_full_triang){
                do_triangulation=false;
                do_partial_triangulation=true;
            }
        } else {
            throw BadInputException("No grading specified and cannot find one. Cannot compute some requested properties!");
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::deg1_check() {

    if(inhomogeneous)  // deg 1 check disabled since it makes no sense in this case
        return;
        
    if (!isComputed(ConeProperty::Grading) && Grading.size()==0          // we still need it and
     && !isComputed(ConeProperty::IsDeg1ExtremeRays)) { // we have not tried it
        if (isComputed(ConeProperty::ExtremeRays)) {
            Matrix<Integer> Extreme=Generators.submatrix(Extreme_Rays_Ind);
            if (has_generator_with_common_divisor) 
                Extreme.make_prime();
            Grading = Extreme.find_linear_form();
            if (Grading.size() == dim && v_scalar_product(Grading,Extreme[0])==1) {
                is_Computed.set(ConeProperty::Grading);
            } else {
                deg1_extreme_rays = false;
                Grading.clear();
                is_Computed.set(ConeProperty::IsDeg1ExtremeRays);
            }
        } else // extreme rays not known
        if (!deg1_generated_computed) {
            Matrix<Integer> GenCopy = Generators;
            if (has_generator_with_common_divisor)
                GenCopy.make_prime();
            Grading = GenCopy.find_linear_form();
            if (Grading.size() == dim && v_scalar_product(Grading,GenCopy[0])==1) {
                is_Computed.set(ConeProperty::Grading);
            } else {
                deg1_generated = false;
                deg1_generated_computed = true;
                Grading.clear();
            }
        }
    }

    //now we hopefully have a grading

    if (!isComputed(ConeProperty::Grading)) {
        if (isComputed(ConeProperty::ExtremeRays)) {
            // there is no hope to find a grading later
            deg1_generated = false;
            deg1_generated_computed = true;
            deg1_extreme_rays = false;
            is_Computed.set(ConeProperty::IsDeg1ExtremeRays);
            disable_grading_dep_comp();
        }
        return; // we are done
    }
    
    set_degrees();

    vector<long> divided_gen_degrees = gen_degrees;
    if (has_generator_with_common_divisor) {
        Matrix<Integer> GenCopy = Generators;
        GenCopy.make_prime();
        convert(divided_gen_degrees, GenCopy.MxV(Grading));
    }

    if (!deg1_generated_computed) {
        deg1_generated = true;
        for (size_t i = 0; i < nr_gen; i++) {
            if (divided_gen_degrees[i] != 1) {
                deg1_generated = false;
                break;
            }
        }
        deg1_generated_computed = true;
        if (deg1_generated) {
            deg1_extreme_rays = true;
            is_Computed.set(ConeProperty::IsDeg1ExtremeRays);
        }
    }
    if (!isComputed(ConeProperty::IsDeg1ExtremeRays)
      && isComputed(ConeProperty::ExtremeRays)) {
        deg1_extreme_rays = true;
        for (size_t i = 0; i < nr_gen; i++) {
            if (Extreme_Rays_Ind[i] && divided_gen_degrees[i] != 1) {
                deg1_extreme_rays = false;
                break;
            }
        }
        is_Computed.set(ConeProperty::IsDeg1ExtremeRays);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::check_deg1_hilbert_basis() {
    if (isComputed(ConeProperty::IsDeg1HilbertBasis) || inhomogeneous)
        return;

    if ( !isComputed(ConeProperty::Grading) || !isComputed(ConeProperty::HilbertBasis)) {
        if (verbose) {
            errorOutput() << "WARNING: unsatisfied preconditions in check_deg1_hilbert_basis()!" <<endl;
        }
        return;
    }

    if (isComputed(ConeProperty::Deg1Elements)) {
        deg1_hilbert_basis = (Deg1_Elements.size() == Hilbert_Basis.size());
    } else {
        deg1_hilbert_basis = true;
        typename list< vector<Integer> >::iterator h;
        for (h = Hilbert_Basis.begin(); h != Hilbert_Basis.end(); ++h) {
            if (v_scalar_product((*h),Grading)!=1) {
                deg1_hilbert_basis = false;
                break;
            }
        }
    }
    is_Computed.set(ConeProperty::IsDeg1HilbertBasis);
}

//---------------------------------------------------------------------------

// Computes the generators of a supercone approximating "this" by a cone over a lattice polytope
template<typename Integer>
Matrix<Integer> Full_Cone<Integer>::latt_approx() {
    assert(isComputed(ConeProperty::Grading));
    assert(isComputed(ConeProperty::ExtremeRays));
    Matrix<Integer> G(1,dim);
    G[0]=Grading;
    Matrix<Integer> G_copy=G;
    
    // Lineare_Transformation<Integer> NewBasis(G); // gives a new basis in which the grading is a coordinate
    size_t dummy;
    Matrix<Integer> U=G_copy.SmithNormalForm(dummy);   // the basis elements are the columns of U

    Integer dummy_denom;                             
    // vector<Integer> dummy_diag(dim); 
    Matrix<Integer> T=U.invert(dummy_denom);       // T is the coordinate transformation
                                                            // to the new basis: v --> Tv (in this case)
                                                    // for which the grading is the FIRST coordinate

    assert(dummy_denom==1);  // for safety 

    // It can happen that -Grading has become the first row of T, but we want Grading. If necessary we replace the
    // first row by its negative, and correspondingly the first column of U by its negative

    if(T[0]!=Grading){
        for(size_t i=0;i<dim;++i){
            U[i][0]*=-1;
            T[0][i]*=-1;
        }
    }
    assert(T[0] == Grading);
    
    list<vector<Integer> > L; // collects the generators of the approximating cone
    for(size_t i=0;i<nr_gen;++i){
        if(Extreme_Rays_Ind[i]){
            list<vector<Integer> > approx;
            //cout << "point before transformation: " << Generators[i];
            approx_simplex(T.MxV(Generators[i]),approx,approx_level);
            L.splice(L.end(),approx);
        }
    }
    
    Matrix<Integer> M=Matrix<Integer>(L);
    
    for(size_t j=0;j<M.nr_of_rows();++j)  // reverse transformation
        M[j]=U.MxV(M[j]);
        
     //cout << "-------" << endl;
     //M.print(cout);
     //cout << "-------" << endl;
    
    return(M);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::prepare_inclusion_exclusion() {

    if (ExcludedFaces.nr_of_rows() == 0)
        return;

    do_excluded_faces = do_h_vector || do_Stanley_dec;

    if (verbose && !do_excluded_faces) {
        errorOutput() << endl << "WARNING: exluded faces, but no h-vector computation or Stanley decomposition"
                      << endl << "Therefore excluded faces will be ignored" << endl;
    }

    if (isComputed(ConeProperty::ExcludedFaces) &&
            (isComputed(ConeProperty::InclusionExclusionData) || !do_excluded_faces) ) {
        return;
    }

    // indicates which generators lie in the excluded faces
    vector<boost::dynamic_bitset<> > GensInExcl(ExcludedFaces.nr_of_rows());

    for(size_t j=0;j<ExcludedFaces.nr_of_rows();++j){ // now we produce these indicators
        bool first_neq_0=true;           // and check whether the linear forms in ExcludedFaces
        bool non_zero=false;             // have the cone on one side
        GensInExcl[j].resize(nr_gen,false);
        for(size_t i=0; i< nr_gen;++i){
            Integer test=v_scalar_product(ExcludedFaces[j],Generators[i]);
            if(test==0){
                GensInExcl[j].set(i);
                continue;
            }
            non_zero=true;
            if(first_neq_0){
                first_neq_0=false;
                if(test<0){
                    for(size_t k=0;k<dim;++k)     // replace linear form by its negative
                        ExcludedFaces[j][k]*=-1;  // to get cone in positive halfspace 
                    test*=-1;                     // (only for error check)
                }    
            }
            if(test<0){
                throw FatalException("Excluded hyperplane does not define a face.");
            }
                
        }
        if(!non_zero){  // not impossible if the hyperplane contains the vector space spanned by the cone
            throw FatalException("Excluded face contains the full cone.");
        }       
    }
    
    vector<bool> essential(ExcludedFaces.nr_of_rows(),true);
    bool remove_one=false;
    for(size_t i=0;i<essential.size();++i)
        for(size_t j=i+1;j<essential.size();++j){
            if(GensInExcl[j].is_subset_of(GensInExcl[i])){
                essential[j]=false;
                remove_one=true;
                continue;
            }
            if(GensInExcl[i].is_subset_of(GensInExcl[j])){
                essential[i]=false;
                remove_one=true;
            }
        }
    if(remove_one){
        Matrix<Integer> Help(0,dim);
        vector<boost::dynamic_bitset<> > HelpGensInExcl;
        for(size_t i=0;i<essential.size();++i)
            if(essential[i]){
                Help.append(ExcludedFaces[i]);
                HelpGensInExcl.push_back(GensInExcl[i]);
            }
        ExcludedFaces=Help;
        GensInExcl=HelpGensInExcl;
    }
    is_Computed.set(ConeProperty::ExcludedFaces);



    if (isComputed(ConeProperty::InclusionExclusionData) || !do_excluded_faces) {
        return;
    }

    vector< pair<boost::dynamic_bitset<> , long> > InExScheme;  // now we produce the formal 
    boost::dynamic_bitset<> all_gens(nr_gen);             // inclusion-exclusion scheme
    all_gens.set();                         // by forming all intersections of
                                           // excluded faces
    InExScheme.push_back(pair<boost::dynamic_bitset<> , long> (all_gens, 1));
    size_t old_size=1;
    
    for(size_t i=0;i<ExcludedFaces.nr_of_rows();++i){
        for(size_t j=0;j<old_size;++j)
            InExScheme.push_back(pair<boost::dynamic_bitset<> , long>
                   (InExScheme[j].first & GensInExcl[i], -InExScheme[j].second));
        old_size*=2;
    }
    
    vector<pair<boost::dynamic_bitset<>, long> >::iterator G;       
    
    InExScheme.erase(InExScheme.begin()); // remove full cone
    
    // map<boost::dynamic_bitset<>, long> InExCollect;
    InExCollect.clear();
    map<boost::dynamic_bitset<>, long>::iterator F;
    
    for(size_t i=0;i<old_size-1;++i){               // we compactify the list of faces
        F=InExCollect.find(InExScheme[i].first);    // obtained as intersections
        if(F!=InExCollect.end())                    // by listing each face only once
            F->second+=InExScheme[i].second;        // but with the right multiplicity
        else
            InExCollect.insert(InExScheme[i]);
    }
     
    for(F=InExCollect.begin();F!=InExCollect.end();){   // faces with multiplicity 0
        if(F->second==0)                                 // can be erased
            InExCollect.erase(F++);
        else{
            ++F;
        }    
    }
     
    if(verbose){
        verboseOutput() << endl;
        verboseOutput() << "InEx complete, " << InExCollect.size() << " faces involved" << endl;
    }
     
    is_Computed.set(ConeProperty::InclusionExclusionData);
} 



//---------------------------------------------------------------------------

/* computes a degree function, s.t. every generator has value >0 */
template<typename Integer>
vector<Integer> Full_Cone<Integer>::compute_degree_function() const {
    size_t i;  
    vector<Integer> degree_function(dim,0);
    if (isComputed(ConeProperty::Grading)) { //use the grading if we have one
        for (i=0; i<dim; i++) {
            degree_function[i] = Grading[i];
        }
    } else { // add hyperplanes to get a degree function
        if(verbose) {
            verboseOutput()<<"computing degree function... "<<flush;
        }
        size_t h;
        for (h=0; h<Support_Hyperplanes.nr_of_rows(); ++h) {
            for (i=0; i<dim; i++) {
                degree_function[i] += Support_Hyperplanes.get_elem(h,i);
            }
        }
        v_make_prime(degree_function);
        if(verbose) {
            verboseOutput()<<"done."<<endl;
        }
    }
    return degree_function;
}

//---------------------------------------------------------------------------

/* adds generators, they have to lie inside the existing cone */
template<typename Integer>
void Full_Cone<Integer>::add_generators(const Matrix<Integer>& new_points) {
    is_simplicial = false;
    int nr_new_points = new_points.nr_of_rows();
    int nr_old_gen = nr_gen;
    Generators.append(new_points);
    nr_gen += nr_new_points;
    set_degrees();
    Top_Key.resize(nr_gen);
    Extreme_Rays_Ind.resize(nr_gen);
    for (size_t i=nr_old_gen; i<nr_gen; ++i) {
        Top_Key[i] = i;
        Extreme_Rays_Ind[i] = false;
    }
    // inhom cones
    if (inhomogeneous) {
        set_levels();
    }
    // excluded faces have to be reinitialized
    is_Computed.set(ConeProperty::ExcludedFaces, false);
    is_Computed.set(ConeProperty::InclusionExclusionData, false);
    prepare_inclusion_exclusion();

    if (do_Hilbert_basis) {
        // add new points to HilbertBasis
        for (size_t i = nr_old_gen; i < nr_gen; ++i) {
            if(!inhomogeneous || gen_levels[i]<=1) {
                OldCandidates.Candidates.push_back(Candidate<Integer>(Generators[i],*this));
                OldCandidates.Candidates.back().original_generator=true;
            }
        }
        OldCandidates.auto_reduce();
    }
}

//---------------------------------------------------------------------------
// Constructors
//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::reset_tasks(){
    do_triangulation = false;
    do_partial_triangulation = false;
    do_determinants = false;
    do_multiplicity=false;
    do_integrally_closed = false;
    do_Hilbert_basis = false;
    do_deg1_elements = false;
    keep_triangulation = false;
    do_Stanley_dec=false;
    do_h_vector=false;
    do_hsop = false;
    do_excluded_faces=false;
    do_approximation=false;
    do_default_mode=false;
    do_class_group = false;
    do_module_gens_intcl = false;
    do_module_rank = false;
    do_cone_dec=false;
    stop_after_cone_dec=false;
    
    do_extreme_rays=false;
    do_pointed=false;
    
    do_evaluation = false;
    do_only_multiplicity=false;

    use_bottom_points = true;

    nrSimplicialPyr=0;
    totalNrPyr=0;
    is_pyramid = false;
    triangulation_is_nested = false;
    triangulation_is_partial = false;
}


//---------------------------------------------------------------------------

template<typename Integer>
Full_Cone<Integer>::Full_Cone(const Matrix<Integer>& M, bool do_make_prime){ // constructor of the top cone
    dim=M.nr_of_columns();
    if(dim>0)
        Generators=M;
    // M.pretty_print(cout);
    // assert(M.row_echelon()== dim); rank check now done later 
    
    /*index=1;                      // not used at present
    for(size_t i=0;i<dim;++i)
        index*=M[i][i];
    index=Iabs(index); */

    //make the generators coprime, remove 0 rows and duplicates
    has_generator_with_common_divisor = false;
    if (do_make_prime) {
        Generators.make_prime();
    } else {
        nr_gen = Generators.nr_of_rows();
        for (size_t i = 0; i < nr_gen; ++i) {
            if (v_gcd(Generators[i]) != 1) {
                has_generator_with_common_divisor = true;
                break;
            }
        }
    }
    Generators.remove_duplicate_and_zero_rows();
    nr_gen = Generators.nr_of_rows();

    if (nr_gen != static_cast<size_t>(static_cast<key_t>(nr_gen))) {
        throw FatalException("Too many generators to fit in range of key_t!");
    }
    
    multiplicity = 0;
    is_Computed = bitset<ConeProperty::EnumSize>();  //initialized to false
    is_Computed.set(ConeProperty::Generators);
    pointed = false;
    is_simplicial = nr_gen == dim;
    deg1_extreme_rays = false;
    deg1_generated = false;
    deg1_generated_computed = false;
    deg1_hilbert_basis = false;
    
    reset_tasks();
    
    Extreme_Rays_Ind = vector<bool>(nr_gen,false);
    in_triang = vector<bool> (nr_gen,false);
    deg1_triangulation = true;
    if(dim==0){            //correction needed to include the 0 cone;
        multiplicity = 1;
        Hilbert_Series.add(vector<num_t>(1,1),vector<denom_t>());
        is_Computed.set(ConeProperty::HilbertSeries);
        is_Computed.set(ConeProperty::Triangulation);
    }
    pyr_level=-1;
    Top_Cone=this;
    Top_Key.resize(nr_gen);
    for(size_t i=0;i<nr_gen;i++)
        Top_Key[i]=i;
    totalNrSimplices=0;
    TriangulationBufferSize=0;
    CandidatesSize=0;
    detSum = 0;
    shift = 0;
    
    FS.resize(omp_get_max_threads());
    
    Pyramids.resize(20);  // prepare storage for pyramids
    nrPyramids.resize(20,0);
      
    recursion_allowed=true;
    
    do_all_hyperplanes=true;
    // multithreaded_pyramid=true; now in build_cone where it is defined dynamically

    
    nextGen=0;
    store_level=0;
    
    Comparisons.reserve(nr_gen);
    nrTotalComparisons=0;

    inhomogeneous=false;
    
    level0_dim=dim; // must always be defined
    
    use_existing_facets=false;
    start_from=0;
    old_nr_supp_hyps=0;
    
    verbose=false;
    OldCandidates.dual=false;
    OldCandidates.verbose=verbose;
    NewCandidates.dual=false;
    NewCandidates.verbose=verbose;
    
    RankTest = vector< Matrix<Integer> >(omp_get_max_threads(), Matrix<Integer>(0,dim));
    
    do_bottom_dec=false;
    keep_order=false;

    approx_level = 1;
    is_approximation=false;
    
    PermGens.resize(nr_gen);
    for(size_t i=0;i<nr_gen;++i)
        PermGens[i]=i;
}

//---------------------------------------------------------------------------

template<typename Integer>
Full_Cone<Integer>::Full_Cone(Cone_Dual_Mode<Integer> &C) {

    is_Computed = bitset<ConeProperty::EnumSize>();  //initialized to false

    dim = C.dim;
    Generators.swap(C.Generators);
    nr_gen = Generators.nr_of_rows();
    if (Generators.nr_of_rows() > 0) 
        is_Computed.set(ConeProperty::Generators);
    has_generator_with_common_divisor = false;
    Extreme_Rays_Ind.swap(C.ExtremeRaysInd);
    if (!Extreme_Rays_Ind.empty()) is_Computed.set(ConeProperty::ExtremeRays);

    multiplicity = 0;
    in_triang = vector<bool>(nr_gen,false);
 
    Basis_Max_Subspace=C.BasisMaxSubspace;
    is_Computed.set(ConeProperty::MaximalSubspace);    
    pointed = (Basis_Max_Subspace.nr_of_rows()==0);
    is_Computed.set(ConeProperty::IsPointed);
    is_simplicial = nr_gen == dim;
    deg1_extreme_rays = false;
    deg1_generated = false;
    deg1_generated_computed = false;
    deg1_triangulation = false;
    deg1_hilbert_basis = false;
    
    reset_tasks();
    
    if (!Extreme_Rays_Ind.empty()) { // only then we can assume that all entries on C.Supp.. are relevant
        Support_Hyperplanes.swap(C.SupportHyperplanes);
        // there may be duplicates in the coordinates of the Full_Cone
        Support_Hyperplanes.remove_duplicate_and_zero_rows();
        is_Computed.set(ConeProperty::SupportHyperplanes);
    }
    if(!C.do_only_Deg1_Elements){
        Hilbert_Basis.swap(C.Hilbert_Basis);
        is_Computed.set(ConeProperty::HilbertBasis);
    }
    else {
        Deg1_Elements.swap(C.Hilbert_Basis);
        is_Computed.set(ConeProperty::Deg1Elements);
    }
    if(dim==0){            //correction needed to include the 0 cone;
        multiplicity = 1;
        Hilbert_Series.add(vector<num_t>(1,1),vector<denom_t>());
        is_Computed.set(ConeProperty::HilbertSeries);
    }
    pyr_level=-1;
    Top_Cone=this;
    Top_Key.resize(nr_gen);
    for(size_t i=0;i<nr_gen;i++)
        Top_Key[i]=i;
    totalNrSimplices=0;
    TriangulationBufferSize=0;
    CandidatesSize=0;
    detSum = 0;
    shift = 0;
    
    do_all_hyperplanes=true;
    
    tri_recursion=false;
    
    nextGen=0;
    
    inhomogeneous=C.inhomogeneous;
    
    level0_dim=dim; // must always be defined
    
    use_existing_facets=false;
    start_from=0;
    old_nr_supp_hyps=0;
    verbose = C.verbose;
    OldCandidates.dual=false;
    OldCandidates.verbose=verbose;
    NewCandidates.dual=false;
    NewCandidates.verbose=verbose;
    
    
    approx_level = 1;
    is_approximation=false;
    
    verbose=C.verbose;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::check_grading_after_dual_mode(){

    if(dim>0 && Grading.size()>0 && !isComputed(ConeProperty::Grading)) {
        if(isComputed(ConeProperty::Generators)){
            vector<Integer> degrees=Generators.MxV(Grading);
            vector<Integer> levels;
            if(inhomogeneous)
                levels=Generators.MxV(Truncation);
            size_t i=0;
            for(;i<degrees.size();++i){
                if(degrees[i]<=0 &&(!inhomogeneous || levels[i]==0))
                    break;
            }
            if(i==degrees.size())
                is_Computed.set(ConeProperty::Grading);
        }
        else if(isComputed(ConeProperty::HilbertBasis)){
            auto hb=Hilbert_Basis.begin();
            for(;hb!=Hilbert_Basis.end();++hb){
                if(v_scalar_product(*hb,Grading)<=0 && (!inhomogeneous || v_scalar_product(*hb,Truncation)==0))
                    break;
            }
            if(hb==Hilbert_Basis.end())
                is_Computed.set(ConeProperty::Grading);
        }   
    }
    if(isComputed(ConeProperty::Deg1Elements)){
        auto hb=Deg1_Elements.begin();
        for(;hb!=Deg1_Elements.end();++hb){
            if(v_scalar_product(*hb,Grading)<=0)
                break;
        }
        if(hb==Deg1_Elements.end())
            is_Computed.set(ConeProperty::Grading);
    }

    if(Grading.size()>0 && !isComputed(ConeProperty::Grading)){
        throw BadInputException("Grading not positive on pointed cone.");
    }
}

template<typename Integer>
void Full_Cone<Integer>::dual_mode() {
    
    if(dim==0){
        set_zero_cone();
        return;
    }

    use_existing_facets=false; // completely irrelevant here
    start_from=0;
    old_nr_supp_hyps=0;
    
    compute_class_group();
    
    check_grading_after_dual_mode();      
        
    if(dim>0 && !inhomogeneous){
        deg1_check();
        if (isComputed(ConeProperty::Grading) && !isComputed(ConeProperty::Deg1Elements)) {
            if (verbose) { 
                verboseOutput() << "Find degree 1 elements" << endl;
            }
            select_deg1_elements();
        }
    }
    
    if(dim==0){
        deg1_extreme_rays = deg1_generated = true;
        Grading=vector<Integer>(dim);
        is_Computed.set(ConeProperty::IsDeg1ExtremeRays);
        deg1_generated_computed = true;
        is_Computed.set(ConeProperty::Grading);
    }
    if(!inhomogeneous && isComputed(ConeProperty::HilbertBasis)){
        if (isComputed(ConeProperty::Grading)) check_deg1_hilbert_basis();
    }

    if (inhomogeneous && isComputed(ConeProperty::Generators)) {
       set_levels();
       find_level0_dim();
       find_module_rank();
    }
    
    use_existing_facets=false;
    start_from=0;
}

//---------------------------------------------------------------------------

/* constructor for pyramids */
template<typename Integer>
Full_Cone<Integer>::Full_Cone(Full_Cone<Integer>& C, const vector<key_t>& Key) {

    Generators = C.Generators.submatrix(Key);
    dim = Generators.nr_of_columns();
    nr_gen = Generators.nr_of_rows();
    has_generator_with_common_divisor = C.has_generator_with_common_divisor;
    is_simplicial = nr_gen == dim;
    
    Top_Cone=C.Top_Cone; // relate to top cone
    Top_Key.resize(nr_gen);
    for(size_t i=0;i<nr_gen;i++)
        Top_Key[i]=C.Top_Key[Key[i]];
  
    multiplicity = 0;
    
    Extreme_Rays_Ind = vector<bool>(nr_gen,false);
    is_Computed.set(ConeProperty::ExtremeRays, C.isComputed(ConeProperty::ExtremeRays));
    if(isComputed(ConeProperty::ExtremeRays))
        for(size_t i=0;i<nr_gen;i++)
            Extreme_Rays_Ind[i]=C.Extreme_Rays_Ind[Key[i]];
    in_triang = vector<bool> (nr_gen,false);
    deg1_triangulation = true;

    // not used in a pyramid, but set precaution
    deg1_extreme_rays = false;
    deg1_generated = false;
    deg1_generated_computed = false;
    deg1_hilbert_basis = false;
    
    Grading=C.Grading;
    is_Computed.set(ConeProperty::Grading, C.isComputed(ConeProperty::Grading));
    Order_Vector=C.Order_Vector;

    do_extreme_rays=false;
    do_triangulation=C.do_triangulation;
    do_partial_triangulation=C.do_partial_triangulation;
    do_determinants=C.do_determinants;
    do_multiplicity=C.do_multiplicity;
    do_deg1_elements=C.do_deg1_elements;
    do_h_vector=C.do_h_vector;
    do_Hilbert_basis=C.do_Hilbert_basis;
    keep_triangulation=C.keep_triangulation;
    do_only_multiplicity=C.do_only_multiplicity;
    do_evaluation=C.do_evaluation;
    do_Stanley_dec=C.do_Stanley_dec;
    inhomogeneous=C.inhomogeneous;   // at present not used in proper pyramids
    is_pyramid=true;
    
    pyr_level=C.pyr_level+1;

    totalNrSimplices=0;
    detSum = 0;
    shift = C.shift;
    if(C.gen_degrees.size()>0){ // now we copy the degrees
        gen_degrees.resize(nr_gen);
        for (size_t i=0; i<nr_gen; i++) {
            gen_degrees[i] = C.gen_degrees[Key[i]];
        }
    }
    if(C.gen_levels.size()>0){ // now we copy the levels
        gen_levels.resize(nr_gen);
        for (size_t i=0; i<nr_gen; i++) {
            gen_levels[i] = C.gen_levels[Key[i]];
        }
    }
    TriangulationBufferSize=0;
    CandidatesSize=0;
    
    recursion_allowed=C.recursion_allowed; // must be reset if necessary 
    do_all_hyperplanes=true; //  must be reset for non-recursive pyramids
    // multithreaded_pyramid=false; // SEE ABOVE
    
    nextGen=0;
    store_level = C.store_level;
    
    Comparisons.reserve(nr_gen);
    nrTotalComparisons=0;
    
    level0_dim = C.level0_dim; // must always be defined
    
    use_existing_facets=false;
    start_from=0;
    old_nr_supp_hyps=0;
    verbose=false;
    OldCandidates.dual=false;
    OldCandidates.verbose=verbose;
    NewCandidates.dual=false;
    NewCandidates.verbose=verbose;
    
    approx_level = C.approx_level;
    is_approximation = C.is_approximation;
	
	do_bottom_dec=false;
	keep_order=true;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::set_zero_cone() {
    
    assert(dim==0);
    
    if (verbose) {
        verboseOutput() << "Zero cone detected!" << endl;
    }
    
    // The basis change already is transforming to zero.
    is_Computed.set(ConeProperty::Sublattice);
    is_Computed.set(ConeProperty::Generators);
    is_Computed.set(ConeProperty::ExtremeRays);
    Support_Hyperplanes=Matrix<Integer> (0);
    is_Computed.set(ConeProperty::SupportHyperplanes);    
    totalNrSimplices = 0;
    is_Computed.set(ConeProperty::TriangulationSize);    
    detSum = 0;
    is_Computed.set(ConeProperty::TriangulationDetSum);
    is_Computed.set(ConeProperty::Triangulation);
    is_Computed.set(ConeProperty::StanleyDec);
    multiplicity = 1;
    is_Computed.set(ConeProperty::Multiplicity);
    is_Computed.set(ConeProperty::HilbertBasis);
    is_Computed.set(ConeProperty::Deg1Elements);
    
    Hilbert_Series = HilbertSeries(vector<num_t>(1,1),vector<denom_t>()); // 1/1
    is_Computed.set(ConeProperty::HilbertSeries);
    
    if (!is_Computed.test(ConeProperty::Grading)) {
        Grading = vector<Integer>(dim);
        // GradingDenom = 1;
        is_Computed.set(ConeProperty::Grading);
    }
    
    pointed = true;
    is_Computed.set(ConeProperty::IsPointed);
    
    deg1_extreme_rays = true;
    is_Computed.set(ConeProperty::IsDeg1ExtremeRays);
    
    deg1_hilbert_basis = true;
    is_Computed.set(ConeProperty::IsDeg1HilbertBasis);
    
    if (inhomogeneous) {  // empty set of solutions
        is_Computed.set(ConeProperty::VerticesOfPolyhedron);        
        module_rank = 0;
        is_Computed.set(ConeProperty::ModuleRank);
        is_Computed.set(ConeProperty::ModuleGenerators);             
        level0_dim=0;
        is_Computed.set(ConeProperty::RecessionRank);
    }
    
    if (!inhomogeneous) {
        ClassGroup.resize(1,0);
        is_Computed.set(ConeProperty::ClassGroup);
    }
    
    if (inhomogeneous || ExcludedFaces.nr_of_rows() != 0) {
        multiplicity = 0;
        is_Computed.set(ConeProperty::Multiplicity);        
        Hilbert_Series.reset(); // 0/1
        is_Computed.set(ConeProperty::HilbertSeries);        
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
bool Full_Cone<Integer>::isComputed(ConeProperty::Enum prop) const{
    return is_Computed.test(prop);
}

//---------------------------------------------------------------------------
// Data access
//---------------------------------------------------------------------------

template<typename Integer>
size_t Full_Cone<Integer>::getDimension()const{
    return dim;
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Full_Cone<Integer>::getNrGenerators()const{
    return nr_gen;
}

//---------------------------------------------------------------------------

template<typename Integer>
bool Full_Cone<Integer>::isPointed()const{
    return pointed;
}

//---------------------------------------------------------------------------

template<typename Integer>
bool Full_Cone<Integer>::isDeg1ExtremeRays() const{
    return deg1_extreme_rays;
}

template<typename Integer>
bool Full_Cone<Integer>::isDeg1HilbertBasis() const{
    return deg1_hilbert_basis;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Full_Cone<Integer>::getGrading() const{
    return Grading;
}

//---------------------------------------------------------------------------

template<typename Integer>
mpq_class Full_Cone<Integer>::getMultiplicity()const{
    return multiplicity;
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer Full_Cone<Integer>::getShift()const{
    return shift;
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Full_Cone<Integer>::getModuleRank()const{
    return module_rank;
}


//---------------------------------------------------------------------------

template<typename Integer>
const Matrix<Integer>& Full_Cone<Integer>::getGenerators()const{
    return Generators;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<bool> Full_Cone<Integer>::getExtremeRays()const{
    return Extreme_Rays_Ind;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Full_Cone<Integer>::getSupportHyperplanes()const{
    return Support_Hyperplanes;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Full_Cone<Integer>::getHilbertBasis()const{
    if(Hilbert_Basis.empty())
        return Matrix<Integer>(0,dim);
    else
        return Matrix<Integer>(Hilbert_Basis);
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Full_Cone<Integer>::getModuleGeneratorsOverOriginalMonoid()const{
    if(ModuleGeneratorsOverOriginalMonoid.empty())
        return Matrix<Integer>(0,dim);
    else
        return Matrix<Integer>(ModuleGeneratorsOverOriginalMonoid);
}


//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Full_Cone<Integer>::getDeg1Elements()const{
    if(Deg1_Elements.empty())
        return Matrix<Integer>(0,dim);
    else
        return Matrix<Integer>(Deg1_Elements);
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Full_Cone<Integer>::getExcludedFaces()const{
    return(ExcludedFaces);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::error_msg(string s) const{
    errorOutput() <<"\nFull Cone "<< s<<"\n";
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::print()const{
    verboseOutput()<<"\ndim="<<dim<<".\n";
    verboseOutput()<<"\nnr_gen="<<nr_gen<<".\n";
    // verboseOutput()<<"\nhyp_size="<<hyp_size<<".\n";
    verboseOutput()<<"\nGrading is:\n";
    verboseOutput()<< Grading;
    verboseOutput()<<"\nMultiplicity is "<<multiplicity<<".\n";
    verboseOutput()<<"\nGenerators are:\n";
    Generators.pretty_print(verboseOutput());
    verboseOutput()<<"\nExtreme_rays are:\n";
    verboseOutput()<< Extreme_Rays_Ind;
    verboseOutput()<<"\nSupport Hyperplanes are:\n";
    Support_Hyperplanes.pretty_print(verboseOutput());
    verboseOutput()<<"\nHilbert basis is:\n";
    verboseOutput()<< Hilbert_Basis;
    verboseOutput()<<"\nDeg1 elements are:\n";
    verboseOutput()<< Deg1_Elements;
    verboseOutput()<<"\nHilbert Series  is:\n";
    verboseOutput()<<Hilbert_Series;
}

} //end namespace
