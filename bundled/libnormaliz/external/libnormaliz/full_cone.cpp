/*
 * Normaliz
 * Copyright (C) 2007-2013  Winfried Bruns, Bogdan Ichim, Christof Soeger
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

#include "full_cone.h"
#include "vector_operations.h"
#include "lineare_transformation.h"
#include "list_operations.h"
#include "map_operations.h"
#include "my_omp.h"
#include "integer.h"

//---------------------------------------------------------------------------

const size_t RecBoundTriang=1000000;   //  if number(supphyps)*size(triang) > RecBoundTriang
                                       // we pass to (non-recirsive) pyramids

const size_t EvalBoundTriang=2500000; // if more than EvalBoundTriang simplices have been stored
                               // evaluation is started (whenever possible)

const size_t EvalBoundPyr=200000;   // the same for stored pyramids

const size_t EvalBoundRecPyr=20000;   // the same for stored RECURSIVE pyramids


//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------
//private
//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::add_hyperplane(const size_t& new_generator, const FACETDATA & positive,const FACETDATA & negative,
                            list<FACETDATA>& NewHyps){
// adds a new hyperplane found in find_new_facets to this cone (restricted to generators processed)

    size_t k;
    
    FACETDATA NewFacet; NewFacet.Hyp.resize(dim); NewFacet.GenInHyp.resize(nr_gen);
    
    Integer used_for_tests;
    if (test_arithmetic_overflow) {  // does arithmetic tests
        for (k = 0; k <dim; k++) {
            NewFacet.Hyp[k]=positive.ValNewGen*negative.Hyp[k]-negative.ValNewGen*positive.Hyp[k];
            used_for_tests =(positive.ValNewGen%overflow_test_modulus)*(negative.Hyp[k]%overflow_test_modulus)-(negative.ValNewGen%overflow_test_modulus)*(positive.Hyp[k]%overflow_test_modulus);
            if (((NewFacet.Hyp[k]-used_for_tests) % overflow_test_modulus)!=0) {
                errorOutput()<<"Arithmetic failure in Full_Cone::add_hyperplane. Possible arithmetic overflow.\n";
                throw ArithmeticException();
            }
        }
    }
    else  {                      // no arithmetic tests
        for (k = 0; k <dim; k++) {
            NewFacet.Hyp[k]=positive.ValNewGen*negative.Hyp[k]-negative.ValNewGen*positive.Hyp[k];
        }
    }
    v_make_prime(NewFacet.Hyp);
    NewFacet.ValNewGen=0; 
    
    NewFacet.GenInHyp=positive.GenInHyp & negative.GenInHyp; // new hyperplane contains old gen iff both pos and neg do
    NewFacet.GenInHyp.set(new_generator);  // new hyperplane contains new generator
    
    NewHyps.push_back(NewFacet);
}


//---------------------------------------------------------------------------


template<typename Integer>
void Full_Cone<Integer>::find_new_facets(const size_t& new_generator){
// our Fourier-Motzkin implementation
// the special treatment of simplicial facets was inserted because of line shellings.
// At present these are not computed.

    //to see if possible to replace the function .end with constant iterator since push-back is performed.

    // NEW: new_generator is the index of the generator being inserted

    size_t i,k,nr_zero_i;
    size_t subfacet_dim=dim-2; // NEW dimension of subfacet
    size_t facet_dim=dim-1; // NEW dimension of facet
    
    const bool tv_verbose = false; //verbose && !is_pyramid; // && Support_Hyperplanes.size()>10000; //verbose in this method call
    
        
    // preparing the computations, the various types of facets are sorted into the deques
    deque <FACETDATA*> Pos_Simp,Pos_Non_Simp;
    deque <FACETDATA*> Neg_Simp,Neg_Non_Simp;
    deque <FACETDATA*> Neutral_Simp, Neutral_Non_Simp;
    
    boost::dynamic_bitset<> Zero_Positive(nr_gen),Zero_Negative(nr_gen);

    bool simplex;
    
    if (tv_verbose) verboseOutput()<<"transform_values:"<<flush;
    
    typename list<FACETDATA>::iterator ii = Facets.begin();
    
    for (; ii != Facets.end(); ++ii) {
        simplex=true;
        nr_zero_i=0;
        for(size_t j=0;j<nr_gen;j++){
            if(ii->GenInHyp.test(j))
                nr_zero_i++;
            if(nr_zero_i>facet_dim){
                simplex=false;
                break;
            }
        }
        
        if(ii->ValNewGen>0)
            Zero_Positive|=ii->GenInHyp;
        else if(ii->ValNewGen<0)
            Zero_Negative|=ii->GenInHyp;       
            
        if (ii->ValNewGen==0) {
            ii->GenInHyp.set(new_generator);  // Must be set explicitly !!
            if (simplex) {
                Neutral_Simp.push_back(&(*ii));
            }   else {
                Neutral_Non_Simp.push_back(&(*ii));
            }
        }
        else if (ii->ValNewGen>0) {
            if (simplex) {
                Pos_Simp.push_back(&(*ii));
            } else {
                Pos_Non_Simp.push_back(&(*ii));
            }
        } 
        else if (ii->ValNewGen<0) {
            if (simplex) {
                Neg_Simp.push_back(&(*ii));
            } else {
                Neg_Non_Simp.push_back(&(*ii));
            }
        }
    }
    
    boost::dynamic_bitset<> Zero_PN(nr_gen);
    Zero_PN=Zero_Positive & Zero_Negative;
    
    size_t nr_PosSimp  = Pos_Simp.size();
    size_t nr_PosNonSimp = Pos_Non_Simp.size();
    size_t nr_NegSimp  = Neg_Simp.size();
    size_t nr_NegNonSimp = Neg_Non_Simp.size();
    size_t nr_NeuSimp  = Neutral_Simp.size();
    size_t nr_NeuNonSimp = Neutral_Non_Simp.size();
    
    if (tv_verbose) verboseOutput()<<" PS "<<nr_PosSimp<<", P "<<nr_PosNonSimp<<", NS "<<nr_NegSimp<<", N "<<nr_NegNonSimp<<", ZS "<<nr_NeuSimp<<", Z "<<nr_NeuNonSimp<<endl;

    if (tv_verbose) verboseOutput()<<"transform_values: subfacet of NS: "<<flush;
    
    vector< list<pair < boost::dynamic_bitset<>, int> > > Neg_Subfacet_Multi(omp_get_max_threads()) ;

    boost::dynamic_bitset<> zero_i(nr_gen);
    boost::dynamic_bitset<> subfacet(nr_gen);

    #pragma omp parallel for firstprivate(zero_i,subfacet) private(k,nr_zero_i) schedule(dynamic)
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
            
        else{       
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
    
    #pragma omp parallel private(jj) //if(nr_NegNonSimp+nr_NegSimp>1000)
    {
    size_t i,j,k,nr_zero_i;
    boost::dynamic_bitset<> subfacet(dim-2);
    jj = Neg_Subfacet_Multi_United.begin();
    size_t jjpos=0;

    bool found;
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

    #pragma omp single nowait
    if (tv_verbose) {
        verboseOutput()<< nr_NegSubf <<endl;
        verboseOutput()<<"transform_values: PS vs NS, "<<flush;
    }
    
    boost::dynamic_bitset<> zero_i(nr_gen);
    map <boost::dynamic_bitset<>, int> ::iterator jj_map;

    #pragma omp for schedule(dynamic) // nowait   // Now matching positive and negative (sub)facets
    for (i =0; i<nr_PosSimp; i++){ //Positive Simp vs.Negative Simp
        zero_i=Pos_Simp[i]->GenInHyp & Zero_PN;
        nr_zero_i=0;
        for(size_t m=0;m<nr_gen;m++){
            if(zero_i.test(m))
                nr_zero_i++;
            if(nr_zero_i>subfacet_dim){
                break;
            }
        }
        
        if (nr_zero_i==subfacet_dim) {                 // NEW slight change in logic. Positive simpl facet shared at most
            jj_map=Neg_Subfacet.find(zero_i);           // one subfacet with negative simpl facet
            if (jj_map!=Neg_Subfacet.end()) {
                add_hyperplane(new_generator,*Pos_Simp[i],*Neg_Simp[(*jj_map).second],NewHypsSimp[i]);
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
                        add_hyperplane(new_generator,*Pos_Simp[i],*Neg_Simp[(*jj_map).second],NewHypsSimp[i]);
                        (*jj_map).second = -1;
                    }
                }
            }
        }
    }

    #pragma omp single
    if (tv_verbose) {
        verboseOutput() << "P vs NS, " << flush;
    }


    #pragma omp for schedule(dynamic) // nowait
    for (i = 0; i <nr_PosNonSimp; i++) {
        jjpos=0;
        jj_map = Neg_Subfacet.begin();
        for (size_t j=0; j<nr_NegSubf; ++j) {
            for( ; j > jjpos; ++jjpos, ++jj_map) ;
            for( ; j < jjpos; --jjpos, --jj_map) ;

            if ( (*jj_map).second != -1 ) {  // skip used subfacets
                if(jj_map->first.is_subset_of(Pos_Non_Simp[i]->GenInHyp)){
                    add_hyperplane(new_generator,*Pos_Non_Simp[i],*Neg_Simp[(*jj_map).second],NewHypsNonSimp[i]);
                    (*jj_map).second = -1; // has now been used
                }
            }
        }
    } // P vs NS
    
    #pragma omp single nowait
    if (tv_verbose) {
        verboseOutput() << "PS vs N, " << flush;
    }

    vector<key_t> key(nr_gen);
    size_t nr_missing;
    bool common_subfacet;
    #pragma omp for schedule(dynamic) nowait
    for (size_t i =0; i<nr_PosSimp; i++){ //Positive Simp vs.Negative Non Simp
        zero_i=Zero_PN & Pos_Simp[i]->GenInHyp;
        nr_zero_i=0;
        for(j=0;j<nr_gen && nr_zero_i<=facet_dim;j++)
            if(zero_i.test(j)){
                key[nr_zero_i]=j;
                nr_zero_i++;
            }        
        if(nr_zero_i>=subfacet_dim) {
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
                    add_hyperplane(new_generator,*Pos_Simp[i],*Neg_Non_Simp[j],NewHypsSimp[i]);
                    if(nr_zero_i==subfacet_dim) // only one subfacet can lie in negative hyperplane
                        break;
                 }
            }           
        }            
    } // PS vs N

    #pragma omp single nowait
    if (tv_verbose) {
        verboseOutput() << "P vs N" << endl;
    }

    list<FACETDATA*> AllNonSimpHyp;
    typename list<FACETDATA*>::iterator a;

    for(i=0;i<nr_PosNonSimp;++i)
        AllNonSimpHyp.push_back(&(*Pos_Non_Simp[i]));
    for(i=0;i<nr_NegNonSimp;++i)
        AllNonSimpHyp.push_back(&(*Neg_Non_Simp[i]));
    for(i=0;i<nr_NeuNonSimp;++i)
        AllNonSimpHyp.push_back(&(*Neutral_Non_Simp[i])); 
   
    bool exactly_two, ranktest;
    FACETDATA *hp_i, *hp_j, *hp_t; // pointers to current hyperplanes
    
    size_t missing_bound, nr_common_zero;
    boost::dynamic_bitset<> common_zero(nr_gen);
    vector<key_t> common_key(nr_gen);
    
    #pragma omp for schedule(dynamic) // nowait
    for (size_t i =0; i<nr_PosNonSimp; i++){ //Positive Non Simp vs.Negative Non Simp

        hp_i=Pos_Non_Simp[i];
        zero_i=Zero_PN & hp_i->GenInHyp;
        nr_zero_i=0;
        for(j=0;j<nr_gen;j++)
            if(zero_i.test(j)){
                key[nr_zero_i]=j;
                nr_zero_i++;
            }
            
        if (nr_zero_i>=subfacet_dim) {
        
            missing_bound=nr_zero_i-subfacet_dim; // at most this number of generators can be missing
                                                  // to have a chance for common subfacet
                for (j=0; j<nr_NegNonSimp; j++){
                hp_j=Neg_Non_Simp[j];
                
                nr_missing=0; 
                nr_common_zero=0;
                common_subfacet=true;               
                for(k=0;k<nr_zero_i;k++) {
                    if(!hp_j->GenInHyp.test(key[k])) {
                        nr_missing++;
                        if(nr_missing>missing_bound || nr_zero_i==subfacet_dim) {
                            common_subfacet=false;
                            break;
                        }
                    }
                    else {
                        common_key[nr_common_zero]=key[k];
                        nr_common_zero++;
                    }
                 }
                 
                if(common_subfacet){//intersection of *i and *j may be a subfacet
                    exactly_two=true;
                    
                    ranktest=((nr_PosNonSimp+nr_NegNonSimp+nr_NeuNonSimp>dim*dim*nr_common_zero/3)); 

                    if (ranktest) {
                        Matrix<Integer> Test(nr_common_zero,dim);
                        for (k = 0; k < nr_common_zero; k++)
                            Test.write(k,Generators[common_key[k]]);

                        if (Test.rank_destructive()<subfacet_dim) {
                            exactly_two=false;
                        }
                    } // ranktest
                    else{                 // now the comparison test
                        common_zero = zero_i & hp_j->GenInHyp;
                        for (a=AllNonSimpHyp.begin();a!=AllNonSimpHyp.end();++a){
                            hp_t=*a;
                            if ((hp_t!=hp_i) && (hp_t!=hp_j) && common_zero.is_subset_of(hp_t->GenInHyp)) {                                
                                exactly_two=false;
                                AllNonSimpHyp.splice(AllNonSimpHyp.begin(),AllNonSimpHyp,a);
                                break;
                            }
                        }                       
                    } // else
                    if (exactly_two) {  //intersection of i and j is a subfacet
                        add_hyperplane(new_generator,*hp_i,*hp_j,NewHypsNonSimp[i]);
                    }
                }
            }
        }
    }
    } //END parallel

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

    size_t listsize = Facets.size();
    vector<typename list<FACETDATA>::iterator> visible;
    visible.reserve(listsize);
    typename list<FACETDATA>::iterator i = Facets.begin();

    // #pragma omp critical(VERBOSE)
    // verboseOutput() << "L " << pyr_level << " H " << listsize << " T " << TriangulationSize << endl;
    
    for (; i!=Facets.end(); ++i) 
        if (i->ValNewGen < 0) // visible facet
            visible.push_back(i);

    listsize = visible.size();
    // cout << "Pyr Level " << pyr_level << " Visible " << listsize <<  " Triang " << TriangulationSize << endl;


    typename list< SHORTSIMPLEX<Integer> >::iterator oldTriBack = --Triangulation.end();
    #pragma omp parallel private(i)  if(TriangulationSize>1000)
    {
    size_t k,l;
    bool one_not_in_i, not_in_facet;
    size_t not_in_i=0;
    size_t facet_dim=dim-1;
    size_t nr_in_i=0;
    list< SHORTSIMPLEX<Integer> > Triangulation_kk;
    
    typename list< SHORTSIMPLEX<Integer> >::iterator j;
    
    vector<key_t> key(dim);
    
    #pragma omp for schedule(dynamic)
    for (size_t kk=0; kk<listsize; ++kk) {
        i=visible[kk];
        
        nr_in_i=0;
        for(size_t m=0;m<nr_gen;m++){
            if(i->GenInHyp.test(m))
                nr_in_i++;
            if(nr_in_i>facet_dim){
                break;
            }
        }

        if (nr_in_i==facet_dim){  // simplicial
            l=0;
            for (k = 0; k <nr_gen; k++) {
                if (i->GenInHyp[k]==1) {
                    key[l]=k;
                    l++;
                }
            }
            key[dim-1]=new_generator;
 
            if(parallel_inside_pyramid) {
                #pragma omp critical(TRIANG) // critical only on top level
                store_key(key,-i->ValNewGen,0,Triangulation);
            } else {
                store_key(key,-i->ValNewGen,0,Triangulation);
            }
            continue;
        }
        
        size_t irrelevant_vertices=0;
        for(size_t vertex=0;vertex<VertInTri.size();++vertex){
        
            if(i->GenInHyp[VertInTri[vertex]]==0) // lead vertex not in hyperplane
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
              store_key(key,-i->ValNewGen,j->vol,Triangulation_kk);
                       
            } // j
            
        } // for vertex

        if(parallel_inside_pyramid) { 
            #pragma omp critical(TRIANG)
                Triangulation.splice(Triangulation.end(),Triangulation_kk);
            }
        else 
            Triangulation.splice(Triangulation.end(),Triangulation_kk);
    } // for kk

    } // parallel

    VertInTri.push_back(new_generator);
    TriSectionFirst.push_back(++oldTriBack);
    TriSectionLast.push_back(--Triangulation.end());    
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
    
    #pragma omp atomic
    TriangulationSize++;
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
    
    if (keep_triangulation){
        Triangulation.push_back(newsimplex);
        return;  
    }
    
    bool Simpl_available=true;

    typename list< SHORTSIMPLEX<Integer> >::iterator F;

    if(Top_Cone->FS[tn].empty()){
        #pragma omp critical(FREESIMPL)
        {
        if(Top_Cone->FreeSimpl.empty())
            Simpl_available=false;
        else{
            F=Top_Cone->FreeSimpl.begin();  // take 100 simplices from FreeSimpl
            size_t q; for(q=0;q<1000;++q){            // or what you can get
                if(F==Top_Cone->FreeSimpl.end())
                    break;
                ++F;
            }
        
            if(q<1000)
                Top_Cone->FS[tn].splice(Top_Cone->FS[tn].begin(),
                    Top_Cone->FreeSimpl);
            else
                Top_Cone->FS[tn].splice(Top_Cone->FS[tn].begin(),
                              Top_Cone->FreeSimpl,Top_Cone->FreeSimpl.begin(),++F);
        } // else
        } // critical
    } // if empty
          

    if(Simpl_available){
        Triangulation.splice(Triangulation.end(),Top_Cone->FS[tn],
                        Top_Cone->FS[tn].begin());
        Triangulation.back()=newsimplex;
    }
    else
        Triangulation.push_back(newsimplex);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::process_pyramids(const size_t new_generator,const bool recursive){

    /*
      
    We distinguish two types of pyramids:
    (i) recursive pyramids that give their support hyperplanes back to the mother.
    (ii) independent pyramids that are not linked to the mother.
    Pyramids of type (i) are
    
    The parameter recursive indicates whether the pyramids that will be created 
    in process_pyramid(s) are of type (i) or (ii).
    
    We must also know whether "this" is of type (i) or (ii). This is indicted by
    do_all_hyperplanes.
    
    recursion_allowed indicates whether recursive pyramids can be created from "this". 
    It has the same value as do_all_hyperplanes, except possibly for the top cone.
    By setting recursion_allowed=false in the constructor of the top cone,
    one can suppress recursive pyramids completely, but do_all_hyperplanes
    must be set to true for the top cone.
    
    Pyramids of type (ii) set recursion_allowed=false.
    
    Pyramids of type (i) are stored in RecPyramids, those of type (ii) in Pyramids. 
    The store_level of the created pyramids is the one of the mother +1, 
    EXCEPT that all pyramids of type (ii) created from pyramids of type (i) 
    are stored at level 0.

    Note: the top cone has pyr_level=-1.

    */

    long store_level;
    if(recursion_allowed && !recursive)
        store_level=0;
    else
        store_level=pyr_level+1;
        
    size_t start_level=omp_get_level(); // allows us to check that we are on level 0
                                        // outside the loop and can therefore call evaluation
                                        // NOW the level doesn't go up in the loop
                                        // because parallelization has been dropped        
    vector<key_t> Pyramid_key;
    Pyramid_key.reserve(nr_gen);
    boost::dynamic_bitset<> in_Pyramid(nr_gen);

    typename list< FACETDATA >::iterator l=Facets.begin();
    size_t listsize=Facets.size();
    Integer ov_sp; // Order_Vector scalar product
    bool skip_triang; // make hyperplanes but skip triangulation (recursive pyramids only)

    // no need to parallelize the followqing loop since ALL pyramids
    // are now processed in parallel (see version 2.8 for parallel version)
    // BUT: new hyperplanes can be added before the loop has been finished.
    // Therefore we must work with listsize.
    for (size_t kk=0; kk<listsize; ++l, ++kk) {

        if (l->ValNewGen>=0) // facet not visible
            continue;

        skip_triang = false;
        if (Top_Cone->do_partial_triangulation && l->ValNewGen>=-1) { //ht1 criterion
            if (!is_pyramid) { // in the topcone we always have ov_sp > 0
                skip_triang = true;
            } else {
                //check if it would be an excluded hyperplane
                ov_sp = v_scalar_product(l->Hyp,Order_Vector);
                if (ov_sp > 0) {
                    skip_triang = true;
                } else if (ov_sp == 0) {
                    for (size_t i=0; i<dim; i++) {
                        if (l->Hyp[i]>0) {
                            skip_triang = true;
                            break;
                        } else if (l->Hyp[i]<0) {
                            break;
                        }
                    }
                }
            }
            if (skip_triang && !recursive) {
                continue;
            }
        }

        Pyramid_key.clear(); // make data of new pyramid
        Pyramid_key.push_back(new_generator);
        for(size_t i=0;i<nr_gen;i++){
            in_Pyramid.reset(i);
            if(in_triang[i] && v_scalar_product(l->Hyp,Generators[i])==0){ //  incidence data may no longer exist
                Pyramid_key.push_back(i);                                      
                in_Pyramid.set(i);
            }
        }
        in_Pyramid.set(new_generator);

        // now we can store the new pyramid at the right place (or finish the simplicial ones)
        if (recursive && skip_triang) { // mark as "do not triangulate"
            process_pyramid(Pyramid_key, in_Pyramid, new_generator,store_level,0, recursive);
        } else { //default
            process_pyramid(Pyramid_key, in_Pyramid, new_generator,store_level,-l->ValNewGen, recursive);
        }

        if(Top_Cone->nrRecPyrs[store_level]>EvalBoundRecPyr && start_level==0){
            Top_Cone->evaluate_rec_pyramids(store_level);
        }
        if(Top_Cone->nrPyramids[store_level] > EvalBoundPyr && start_level==0){  
            Top_Cone->evaluate_stored_pyramids(store_level);
        }
    }

} 

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::process_pyramid(const vector<key_t>& Pyramid_key, const boost::dynamic_bitset<>& in_Pyramid, 
                          const size_t new_generator,const size_t store_level, Integer height, const bool recursive){
// processes simplicial pyramids directly, stores other pyramids into their depots
    
    #pragma omp atomic
    Top_Cone->totalNrPyr++;
    
    if(Pyramid_key.size()==dim){  // simplicial pyramid completely done here  
        #pragma omp atomic        // only for saving memory 
        Top_Cone->nrSimplicialPyr++;
        if(recursive){ // the facets may be facets of the mother cone and if recursive==true must be given back
            Simplex<Integer> S(Pyramid_key, Generators);
            height = S.read_volume(); //update our lower bound for the volume
            Matrix<Integer> H=S.read_support_hyperplanes();
            list<vector<Integer> > NewFacets;
            for (size_t i=0; i<dim;i++)
                NewFacets.push_back(H[i]);
            select_supphyps_from(NewFacets,new_generator,in_Pyramid);  // SEE BELOW
        }
        if (height != 0 && (do_triangulation || do_partial_triangulation)) {
            //if(recursion_allowed) {                                   // AT PRESENT
            //    #pragma omp critical(TRIANG)                          // NO PARALLELIZATION
            //     store_key(Pyramid_key,height,0,Triangulation);       // HERE
            // } else {
                store_key(Pyramid_key,height,0,Triangulation);
            // }
        }
    }
    else {  // non-simplicial
        if(recursive){
            Full_Cone<Integer> Pyramid(*this,Pyramid_key);
            Pyramid.Mother=this;
            Pyramid.in_Pyramid=in_Pyramid;    // need these data to give back supphyps
            Pyramid.new_generator=new_generator;
            if (height == 0) { //indicates "do not triangulate"
                Pyramid.do_partial_triangulation = false;
                Pyramid.do_Hilbert_basis = false;
            }
            nrRecPyramidsDue++;
            #pragma omp critical(RECPYRAMIDS)
            {
            Top_Cone->RecPyrs[store_level].push_back(Pyramid); 
            Top_Cone->nrRecPyrs[store_level]++;            
            } // critical
       } else { //not recursive
           vector<key_t> key_wrt_top(Pyramid_key.size());
           for(size_t i=0;i<Pyramid_key.size();i++)
                key_wrt_top[i]=Top_Key[Pyramid_key[i]];
           #pragma omp critical(STOREPYRAMIDS)
           {
           Top_Cone->Pyramids[store_level].push_back(key_wrt_top);
           Top_Cone->nrPyramids[store_level]++;           
           } // critical
       }
    }   

}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::find_and_evaluate_start_simplex(){

    size_t i,j;
    Integer factor;

    
    Simplex<Integer> S = find_start_simplex();
    vector<key_t> key=S.read_key();   // generators indexed from 0
        
    for (i = 0; i < dim; i++) {
        in_triang[key[i]]=true;
        if (deg1_triangulation && isComputed(ConeProperty::Grading))
            deg1_triangulation = (gen_degrees[i] == 1);
    }
       
    Matrix<Integer> H=S.read_support_hyperplanes();
    for (i = 0; i <dim; i++) {
        FACETDATA NewFacet; NewFacet.Hyp.resize(dim); NewFacet.GenInHyp.resize(nr_gen);
        NewFacet.Hyp=H.read(i);
        for(j=0;j < dim;j++)
            if(j!=i)
                NewFacet.GenInHyp.set(key[j]);
        NewFacet.ValNewGen=-1;         // must be taken negative since opposite facet
        Facets.push_back(NewFacet);    // was visible before adding this vertex
    }
    
    if(!is_pyramid){
        //define Order_Vector, decides which facets of the simplices are excluded
        Order_Vector = vector<Integer>(dim,0);
        Matrix<Integer> G=S.read_generators();
        //srand(12345);
        for(i=0;i<dim;i++){
            factor=(unsigned long)(2*(rand()%(2*dim))+3);
            for(j=0;j<dim;j++)
                Order_Vector[j]+=factor*G[i][j];        
        }
    }

    //the volume is an upper bound for the height
    if(do_triangulation || (do_partial_triangulation && S.read_volume()>1))
    {
        store_key(key,S.read_volume(),1,Triangulation); 
        if(do_only_multiplicity) {
            #pragma omp atomic
            TotDet++;
        }
    }
    
    if(do_triangulation){ // we must prepare the sections of the triangulation
        for(i=0;i<dim;i++)
        {
            VertInTri.push_back(key[i]);
            TriSectionFirst.push_back(Triangulation.begin());
            TriSectionLast.push_back(Triangulation.begin());
        }
    }
    
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::select_supphyps_from(list<vector<Integer> >& NewFacets, 
                                 const size_t new_generator, const boost::dynamic_bitset<>& in_Pyr){
// the mother cone (=this) selects supphyps from the list NewFacets supplied by the daughter
// the daughter provides the necessary information via the parameters

    size_t i;                        
    typename list<vector<Integer> >::iterator pyr_hyp = NewFacets.begin();
    bool new_global_hyp;
    FACETDATA NewFacet;
    Integer test;   
    for(;pyr_hyp!= NewFacets.end();pyr_hyp++){
        if(v_scalar_product(Generators[new_generator],*pyr_hyp)>0)
            continue;
        new_global_hyp=true;
        for(i=0;i<nr_gen;i++){
            if(in_Pyr.test(i) || !in_triang[i])
                continue;
            test=v_scalar_product(Generators[i],*pyr_hyp);
            if(test<=0){
                new_global_hyp=false;
                break;
            }
        }
        if(new_global_hyp){
            NewFacet.Hyp=*pyr_hyp;                
            if(recursion_allowed){
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
void Full_Cone<Integer>::evaluate_rec_pyramids(const size_t level){
// evaluates the stored recursive pyramids
// Note that we must call extend_cone for every new_generator added to a given pyramid,
// once we have left extend_cone to make recursive pyramids.
// extend_cone is locked until all subpyramids have been finished.
// Therefore we need not take care of this question here.
// The "skip_remaining" technique is applied here and later on since we cannot interrupt
// parallelized loops to do paralleized ecaluation. (No nested parallelization)

    assert(omp_get_level()==0);

    if(RecPyrs[level].empty())
        return;
        
    if (verbose){
        verboseOutput() << "++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;

        for (size_t l=0; l<level; ++l) {
            if (nrRecPyrs[l]>0) {
                verboseOutput() << "level " << l << " recursive pyramids remaining: "
                                << nrRecPyrs[l] << endl;
            }
        }
        if (nrRecPyrs[level]>0) {
            verboseOutput() << "Computing support hyperplanes of " << nrRecPyrs[level]
                            << " level " << level << " recursive pyramids." << endl;
        }
        verboseOutput() << "++++++++++++++++++++++++++++++++++++++++++++++++++" << endl; 
    }
    RecPyrs.resize(level+2); // provide space for a new generation
    nrRecPyrs.resize(level+2);
    nrRecPyrs[level+1]=0;

    size_t nr_pyramids=nrRecPyrs[level];

    typename list<Full_Cone<Integer> >::iterator p;
    size_t ppos;
    bool skip_remaining_tri,skip_remaining_pyr,skip_remaining_rec_pyr;

    do
    {
       p = RecPyrs[level].begin();
       ppos=0;
       skip_remaining_tri=false;
       skip_remaining_pyr=false;
       skip_remaining_rec_pyr=false;
    
       #pragma omp parallel for firstprivate(p,ppos) schedule(dynamic) 
       for(size_t i=0; i<nr_pyramids; i++){
       
           if(skip_remaining_tri || skip_remaining_pyr || skip_remaining_rec_pyr)
                continue;
                
           for(; i > ppos; ++ppos, ++p) ;
           for(; i < ppos; --ppos, --p) ;
           
           p->pyr_level=level;

           p->extend_cone();
           if(check_evaluation_buffer_size())  // we interrupt parallel execution if it is really parallel
                skip_remaining_tri=true;       //  to keep the triangulation buffer under control
                
            if(nrRecPyrs[level+1]>EvalBoundRecPyr) 
                 skip_remaining_rec_pyr=true;
            
            if(nrPyramids[0]>EvalBoundPyr) 
                 skip_remaining_pyr=true;
        }
        
        if (!skip_remaining_tri && !skip_remaining_pyr) 
            evaluate_rec_pyramids(level+1);
       
        // remove done pyramids
        p = RecPyrs[level].begin();
        for(size_t i=0; i<nr_pyramids; i++){
            if (p->Done) {
                p=RecPyrs[level].erase(p);
                nrRecPyrs[level]--;
            } else {
                ++p;
            }
        }
        nr_pyramids = nrRecPyrs[level];

        if (skip_remaining_tri) {
            if (verbose)
                verboseOutput() << nr_pyramids <<
                    " recursive pyramids remaining on level " << level << ", ";
            Top_Cone->evaluate_triangulation();
        }
        
        if(skip_remaining_pyr){
            if (verbose)
                verboseOutput() << nr_pyramids <<
                    " recursive pyramids remaining on level " << level << endl;
            evaluate_stored_pyramids(0);
        }
    
    } while(nr_pyramids>0); // indicates: not all pyramids done
     
/*    if (verbose) {
        verboseOutput() << "++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
        verboseOutput() << "all recursive pyramids on level "<< level << " done!"<<endl;
        for (size_t l=0; l<=level; ++l) {
            if (nrRecPyrs[l]>0) {
                verboseOutput() << "level " << l << " recursive pyramids remaining: "
                                << nrRecPyrs[l] << endl;
            }
        }
        verboseOutput() << "++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
    } */
    if(check_evaluation_buffer())
    {
        Top_Cone->evaluate_triangulation();
    }
}


//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::evaluate_stored_pyramids(const size_t level){
// evaluates the stored non-recursive pyramids
// In contrast to the the recusrive pyramids, extend_cone is called
// only once for every stored pyramid since we set recursion_allowed=false.

    assert(omp_get_level()==0);

    if(Pyramids[level].empty())
        return;
    Pyramids.resize(level+2); // provide space for a new generation
    nrPyramids.resize(level+2);
    nrPyramids[level+1]=0;

    size_t nr_done=0;
    size_t nr_pyramids=nrPyramids[level];
    vector<char> Done(nr_pyramids,0);
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
    bool skip_remaining_tri,skip_remaining_pyr;

    do
    {

       p = Pyramids[level].begin();
       ppos=0;
       skip_remaining_tri=false;
       skip_remaining_pyr=false;
    
       #pragma omp parallel for firstprivate(p,ppos) schedule(dynamic) 
       for(size_t i=0; i<nr_pyramids; i++){
       
           if(skip_remaining_tri || skip_remaining_pyr)
                continue;
                
           for(; i > ppos; ++ppos, ++p) ;
           for(; i < ppos; --ppos, --p) ;
           
           if(Done[i])
               continue;
           Done[i]=1;
           
           #pragma omp atomic
           nr_done++;
           
           Full_Cone<Integer> Pyramid(*this,*p);
           Pyramid.recursion_allowed=false; // ABSOLUTELY NECESSARY HERE
           Pyramid.pyr_level=level;
           Pyramid.do_all_hyperplanes=false;
           if(level>=2 && do_partial_triangulation){ // limits the descent of do_partial_triangulation
               Pyramid.do_triangulation=true;
               Pyramid.do_partial_triangulation=false;
           }
           Pyramid.extend_cone();
           if(check_evaluation_buffer_size() && nr_done < nr_pyramids)  // we interrupt parallel execution if it is really parallel
                skip_remaining_tri=true;                         //  to keep the triangulation buffer under control
                
            if(nrPyramids[level+1]>EvalBoundPyr && nr_done < nr_pyramids) 
                 skip_remaining_pyr=true;
        }
       
        // remove done pyramids
        p = Pyramids[level].begin();
        for(size_t i=0; i<nr_pyramids; i++){
            if (Done[i]) {
                p=Pyramids[level].erase(p);
                nrPyramids[level]--;
                Done[i]=0;
            } else {
                ++p;
            }
        }
        nr_done=0;
        nr_pyramids = nrPyramids[level];

        if (skip_remaining_tri) {
            if (verbose)
                verboseOutput() << nr_pyramids <<
                    " pyramids remaining on level " << level << ", ";
            Top_Cone->evaluate_triangulation();
        }

        if(skip_remaining_pyr){
            evaluate_stored_pyramids(level+1);
        }
    
    } while(skip_remaining_tri || skip_remaining_pyr);
     
    if (verbose) {
        verboseOutput() << "**************************************************" << endl;
        verboseOutput() << "all pyramids on level "<< level << " done!"<<endl;
        for (size_t l=0; l<=level; ++l) {
            if (nrPyramids[l]>0) {
                verboseOutput() << "level " << l << " pyramids remaining: "
                                << nrPyramids[l] << endl;
            }
        }
        verboseOutput() << "**************************************************" << endl;
    }
    if(check_evaluation_buffer())
    {
        Top_Cone->evaluate_triangulation();
    }
     
    Pyramids[level].clear();
    nrPyramids[level]=0;
    evaluate_stored_pyramids(level+1);
}
    
//---------------------------------------------------------------------------
/* builds the top cone successively by inserting generators, computes all essential data
except global reduction */
template<typename Integer>
void Full_Cone<Integer>::build_top_cone() {
    
    if(dim==0)
        return;
    if (verbose) {
        verboseOutput()<<endl<<"************************************************************"<<endl;
        verboseOutput()<<"starting primal algorithm ";
        if (do_partial_triangulation) verboseOutput()<<"with partial triangulation ";
        if (do_triangulation) {
            verboseOutput()<<"with full triangulation ";
        }
        if (!do_triangulation && !do_partial_triangulation) verboseOutput()<<"(only support hyperplanes) ";
        verboseOutput()<<"..."<<endl;
    }
    while(!Done){        
        extend_cone();
        evaluate_rec_pyramids(0);
    }        
        
    evaluate_stored_pyramids(0);  // force evaluation of remaining non-recusrive pyramids                    
    
    if(!keep_triangulation) // force evaluation of remaining simplices
        evaluate_triangulation();          
        
    if(keep_triangulation)  // in this case triangulation now complete and stored
        is_Computed.set(ConeProperty::Triangulation);   

}

//---------------------------------------------------------------------------

/* starts building the cone by inserting generators until finished or
left because of recursive pyramids. In the latter case we must come back after these
have been finished */
template<typename Integer>
void Full_Cone<Integer>::extend_cone() {

    assert(nrRecPyramidsDone <= nrRecPyramidsDue);
    
    // cout << "In extend " << allRecPyramidsBuilt << " " <<  nrRecPyramidsDone << " " << nrRecPyramidsDue << endl; 

    if(!allRecPyramidsBuilt || nrRecPyramidsDone < nrRecPyramidsDue) // must wait for completion of subpyramids
        return;                                        // of recursive pyramids built from previous generator

    long i;
    typename list< FACETDATA >::iterator l;
    
    // DECIDE WHETHER TO BUILD RECURSIVE PYRAMIDS
    long long RecBoundSuppHyp = dim*dim;
    RecBoundSuppHyp *= RecBoundSuppHyp*3000; //dim^4 * 3000
//    int bound_div = nr_gen-dim+1;
//    if(bound_div > 3* (int) dim) bound_div = 3*dim;
//    RecBoundSuppHyp /= bound_div;

    if(nextGen==-1){ // indicates the first call of extend_cone for this cone
        find_and_evaluate_start_simplex();
        nextGen=0;
        last_to_be_inserted=nr_gen-1; 
        for(long j=nr_gen-1;j>=0;--j){
            if(isComputed(ConeProperty::ExtremeRays)){
                if(!in_triang[j] && Extreme_Rays[j]){
                    last_to_be_inserted=j;
                    break;
                }
            }
            else
                if(!in_triang[j]){
                    last_to_be_inserted=j;
                    break;
                }
        }
    }
    else // in this case we have left this function last time for this cone
         // via the explicit return since we had to form recursive pyramids.
         // The steps at the end of the loop that were skipped have to be done now.
    {
    
        // removing the negative hyperplanes if necessary
        if(do_all_hyperplanes || nextGen!=last_to_be_inserted){
            l=Facets.begin();
            for (size_t j=0; j<old_nr_supp_hyps;j++){
                if (l->ValNewGen<0) 
                    l=Facets.erase(l);
                else 
                    l++;
            }
        }
        
        if(verbose && !is_pyramid) {
            verboseOutput() << "gen="<< nextGen <<", "<<Facets.size()<<" hyp";
            if(nrPyramids[0]>0)
                verboseOutput() << ", " << nrPyramids[0] << " pyr"; 
                if(do_triangulation||do_partial_triangulation)
            verboseOutput() << ", " << TriangulationSize << " simpl";
            verboseOutput()<< endl;
        }
    
    } // else
    
    
    Integer scalar_product;
    bool is_new_generator;


    for (i=nextGen;i<static_cast<long>(nr_gen);++i) {
    
        if(in_triang[i] || (isComputed(ConeProperty::ExtremeRays) && !Extreme_Rays[i]))
            continue;
            
        if(do_triangulation && TriangulationSize > 2*RecBoundTriang) // emermergency brake
            tri_recursion=true;               // to switch off production of simplices in favor
                                              // of non-recursive pyramids
                                              
        is_new_generator=false;
        l=Facets.begin();

        long long nr_pos=0; long long nr_neg=0;
        vector<Integer> L;
        old_nr_supp_hyps=Facets.size(); // Facets will be xtended in the loop               
        
        size_t lpos=0;
        #pragma omp parallel for private(L,scalar_product) firstprivate(lpos,l) reduction(+: nr_pos, nr_neg) schedule(dynamic) if(old_nr_supp_hyps>10000)
        for (size_t k=0; k<old_nr_supp_hyps; k++) {
            for(;k > lpos; lpos++, l++) ;
            for(;k < lpos; lpos--, l--) ;

            L=Generators[i];
            scalar_product=v_scalar_product(L,(*l).Hyp);            
            // l->ValPrevGen=l->ValNewGen;  // last new generator is now previous generator
            l->ValNewGen=scalar_product;
            if (scalar_product<0) {
                is_new_generator=true;
                nr_neg++;
            }
            if (scalar_product>0) {
                nr_pos++;
            }
        }  //end parallel for
        
        if(!is_new_generator)
            continue;

        // the i-th generator is used in the triangulation
        in_triang[i]=true;
        if (deg1_triangulation && isComputed(ConeProperty::Grading))
            deg1_triangulation = (gen_degrees[i] == 1);
        
            
        // First we test whether to go to recursive pyramids because of too many supphyps
        // Once we have done so, we must stay with it
        if( supphyp_recursion || (recursion_allowed && nr_neg*nr_pos>RecBoundSuppHyp)){  // go to pyramids because of supphyps
             if(check_evaluation_buffer()){
                // cout << "Evaluation Build Mitte" << endl;
                    Top_Cone->evaluate_triangulation();
            }
            // cout << "In SuppHyp Rec" << endl;   
            supphyp_recursion=true;
            allRecPyramidsBuilt=false; // must lock extend_cone for this cone
            nrRecPyramidsDue=0;
            nrRecPyramidsDone=0;
            process_pyramids(i,true); //recursive
            allRecPyramidsBuilt=true;
            nextGen=i+1; 
            return; // in recursive mode we stop at this point and come back later
                    // to proceed with nextGen
        }
        else{ // now we check whether to go to pyramids because of the size of triangulation
            if( tri_recursion || (do_triangulation 
                && (nr_neg*TriangulationSize > RecBoundTriang 
                    || 3*omp_get_max_threads()*TriangulationSize>EvalBoundTriang ))){ // go to pyramids because of triangulation
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
        
        if(verbose && !is_pyramid) {
            verboseOutput() << "gen="<< i+1 <<", "<<Facets.size()<<" hyp";
            if(nrPyramids[0]>0)
                verboseOutput() << ", " << nrPyramids[0] << " pyr"; 
            if(do_triangulation||do_partial_triangulation)
                verboseOutput() << ", " << TriangulationSize << " simpl";
            verboseOutput()<< endl;
        }
        
    }

    // transfer Facets --> SupportHyperplanes
    if (do_all_hyperplanes) {
        typename list<FACETDATA>::const_iterator IHV=Facets.begin();
        for(;IHV!=Facets.end();IHV++){
            Support_Hyperplanes.push_back(IHV->Hyp);
        }
    }
    Facets.clear();
    is_Computed.set(ConeProperty::SupportHyperplanes);
    
    if(is_pyramid && do_all_hyperplanes){ // must give supphyps back to maother
        Mother->select_supphyps_from(Support_Hyperplanes,new_generator,in_Pyramid);
        #pragma omp atomic
        Mother->nrRecPyramidsDone++;
    }
    
    transfer_triangulation_to_top(); // transfer remaining simplices to top
    if(check_evaluation_buffer()){
        // cout << "Evaluating in build_cone at end, pyr level " << pyr_level << endl;
        // cout << "Evaluation Build Ende " << is_pyramid << endl;
        Top_Cone->evaluate_triangulation();
    }
    
    Done=true; // this cone now finished

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
               Top_Cone->TriangulationSize > EvalBoundTriang);
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
  
    typename list< SHORTSIMPLEX<Integer> >::iterator pyr_simp=Triangulation.begin();
    for(;pyr_simp!=Triangulation.end();pyr_simp++)
        for(i=0;i<dim;i++)
            pyr_simp->key[i]=Top_Key[pyr_simp->key[i]];

    // cout << "Keys transferred " << endl;
    #pragma omp critical(TRIANG)
    {
        Top_Cone->Triangulation.splice(Top_Cone->Triangulation.end(),Triangulation);
        Top_Cone->TriangulationSize+=TriangulationSize;
    }
    TriangulationSize  =  0;

    // cout << "Done." << endl;
  
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::evaluate_triangulation(){

    assert(omp_get_level()==0);
   
    if(TriangulationSize>0)
    {
    const long VERBOSE_STEPS = 50;
    long step_x_size = TriangulationSize-VERBOSE_STEPS;
    if (verbose) {
        verboseOutput() << "evaluating "<<TriangulationSize<<" simplices" <<endl;
        /* verboseOutput() << "---------+---------+---------+---------+---------+"
                        << " (one | per 2%)" << endl;*/
    }
    
    totalNrSimplices+=TriangulationSize;

    if(do_evaluation && !do_only_multiplicity) {
    #pragma omp parallel 
    {
        typename list< SHORTSIMPLEX<Integer> >::iterator s = Triangulation.begin();
        size_t spos=0;
        int tn = omp_get_thread_num();
        #pragma omp for schedule(dynamic) 
        for(size_t i=0; i<TriangulationSize; i++){
            for(; i > spos; ++spos, ++s) ;
            for(; i < spos; --spos, --s) ;

            if(keep_triangulation || do_Stanley_dec)
                sort(s->key.begin(),s->key.end());
            SimplexEval[tn].evaluate(*s);
            if (verbose) {
                #pragma omp critical(VERBOSE)
                while ((long)(i*VERBOSE_STEPS) >= step_x_size) {
                    step_x_size += TriangulationSize;
                    verboseOutput() << "|" <<flush;
                }
            }
        }
        SimplexEval[tn].transfer_candidates();
    } // end parallel
    if (verbose)
        verboseOutput()  << endl;
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
    
    if(!keep_triangulation){
        // Triangulation.clear();
        #pragma omp critical(FREESIMPL)
        FreeSimpl.splice(FreeSimpl.begin(),Triangulation);       
        TriangulationSize=0;
    }
    
    } // TriangulationSize

}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::primal_algorithm(){

    // set needed do_ vars
    if (do_Hilbert_basis||do_deg1_elements||do_h_vector)
        do_evaluation = true;
    // look for a grading if it is needed
    deg1_check();
    if (!isComputed(ConeProperty::Grading) && (do_multiplicity || do_deg1_elements || do_h_vector)) {
        if (!isComputed(ConeProperty::ExtremeRays)) {
            if (verbose) {
                verboseOutput() << "Cannot find grading s.t. all generators have the same degree! Computing Extreme rays first:" << endl;
            }
            compute_support_hyperplanes();
            extreme_rays_and_deg1_check();
            if(!pointed) return;

            // We keep the SupportHyperplanes, so we do not need to recompute them
            // for the last generator, and use them to make a global reduction earlier
            do_all_hyperplanes = false;
            supphyp_recursion = false;
            for(size_t i=0;i<nr_gen;i++)
                in_triang[i]=false;
            Done = false;
            nextGen = -1;
        }
    }
    set_degrees();
    sort_gens_by_degree();

    if (!is_pyramid) {
        SimplexEval = vector< SimplexEvaluator<Integer> >(omp_get_max_threads(),SimplexEvaluator<Integer>(*this));
    }

    /***** Main Work is done in build_cone() *****/
    build_top_cone();  // evaluates if keep_triangulation==false
    /***** Main Work is done in build_cone() *****/
    
    if (verbose) {
        verboseOutput() << "Total number of pyramids = "<< totalNrPyr << endl;
        // cout << "Uni "<< Unimod << " Ht1NonUni " << Ht1NonUni << " NonDecided " << NonDecided << " TotNonDec " << NonDecidedHyp<< endl;
        if(do_only_multiplicity)
            verboseOutput() << "Determinantes computed = " << TotDet << endl;
    }

    extreme_rays_and_deg1_check();
    if(!pointed) return;

    if (keep_triangulation) {
        if (isComputed(ConeProperty::Grading) && !deg1_generated) {
            deg1_triangulation = false;
        }
        evaluate_triangulation();
    }
    FreeSimpl.clear();

    // collect accumulated data from the SimplexEvaluators
    if(!is_pyramid) {
        for (int zi=0; zi<omp_get_max_threads(); zi++) {
            detSum += SimplexEval[zi].getDetSum();
            multiplicity += SimplexEval[zi].getMultiplicitySum(); 
            if (do_h_vector) {
                Hilbert_Series += SimplexEval[zi].getHilbertSeriesSum();
            }
        }
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
        global_reduction();
        is_Computed.set(ConeProperty::HilbertBasis,true);
        check_integrally_closed();
        if (isComputed(ConeProperty::Grading)) {
            select_deg1_elements();
            check_deg1_hilbert_basis();
        }
    }
    
    if (do_deg1_elements) {
        Deg1_Elements.splice(Deg1_Elements.begin(), Candidates);
        for(size_t i=0;i<nr_gen;i++)
            if(in_triang[i] && v_scalar_product(Grading,Generators[i])==1)
                Deg1_Elements.push_front(Generators[i]);
        Deg1_Elements.sort();
        Deg1_Elements.unique();  //TODO sort, unique needed?
        is_Computed.set(ConeProperty::Deg1Elements,true);
    }
    if (do_h_vector) {
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

// pure dualization
template<typename Integer>
void Full_Cone<Integer>::dualize_cone() {  
    compute_support_hyperplanes();
    reset_tasks();
}

// check the do_* bools, they must be set in advance
// this method (de)activate them according to dependencies between them
template<typename Integer>
void Full_Cone<Integer>::do_vars_check() {

    // activate implications
    if (do_Stanley_dec)     keep_triangulation = true;
    if (keep_triangulation) do_triangulation = true;
    if (do_multiplicity)    do_triangulation = true;
    if (do_h_vector)        do_triangulation = true;
    if (do_deg1_elements)   do_partial_triangulation = true;
    if (do_Hilbert_basis)   do_partial_triangulation = true;
    // activate 
    do_only_multiplicity = do_multiplicity;
    if (do_Stanley_dec || do_h_vector || do_deg1_elements || do_Hilbert_basis) {
        do_only_multiplicity = false;
        do_evaluation = true;
    }
    if (do_multiplicity)    do_evaluation = true;

    if (do_triangulation)   do_partial_triangulation = false;
    if (do_Hilbert_basis)   do_deg1_elements = false; //they will be extracted later
}


// general purpose compute method
// do_* bools must be set in advance, this method does sanity checks for it
// if no bool is set it does support hyperplanes and extreme rays
template<typename Integer>
void Full_Cone<Integer>::compute() {
    do_vars_check();

    if (!do_triangulation && !do_partial_triangulation)
        support_hyperplanes();
    else
        primal_algorithm();
}


// -s
template<typename Integer>
void Full_Cone<Integer>::support_hyperplanes() {
    // recursion_allowed=true;
    compute_support_hyperplanes();
    extreme_rays_and_deg1_check();
    reset_tasks();
}

template<typename Integer>
void Full_Cone<Integer>::dual_mode() {
    Support_Hyperplanes.sort();
    Support_Hyperplanes.unique();
    Support_Hyperplanes.remove(vector<Integer>(dim,0));

    if(dim>0) {            //correction needed to include the 0 cone;
        deg1_check();
        if (isComputed(ConeProperty::Grading)) {
            if (verbose) { 
                verboseOutput() << "Find degree 1 elements" << endl;
            }
            select_deg1_elements();
        }
    } else {
        deg1_extreme_rays = deg1_generated = true;
        Grading=vector<Integer>(dim);
        is_Computed.set(ConeProperty::IsDeg1ExtremeRays);
        is_Computed.set(ConeProperty::IsDeg1Generated);
        is_Computed.set(ConeProperty::Grading);
    }
    if (isComputed(ConeProperty::Grading)) check_deg1_hilbert_basis();
    check_integrally_closed();
}

//---------------------------------------------------------------------------
// Checks and auxiliary algorithms
//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::extreme_rays_and_deg1_check() {
    check_pointed();
    if(!pointed) return;
    compute_extreme_rays();
    deg1_check();
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::set_degrees() {
    if(gen_degrees.size()==0 && isComputed(ConeProperty::Grading)) // now we set the degrees
    {
        gen_degrees.resize(nr_gen);
        vector<Integer> gen_degrees_Integer=Generators.MxV(Grading);
        for (size_t i=0; i<nr_gen; i++) {
            if (gen_degrees_Integer[i] < 1) {
                errorOutput() << "Grading gives non-positive value " << gen_degrees_Integer[i] << " for generator " << i+1 << "." << endl;
                throw BadInputException();
            }
            gen_degrees[i] = explicit_cast_to_long(gen_degrees_Integer[i]);
        }
    }
    
}
    
//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::sort_gens_by_degree() {
    if(gen_degrees.size()==0 || deg1_extreme_rays)
        return;
    
    list<vector<Integer> > genList;
    vector<Integer> v(dim+3);
    vector<Integer> w(dim);
    unsigned long i,j;
    
    for(i=0;i<nr_gen;i++){
        v[0]=gen_degrees[i];
        v[1]=i;                // keep the input order as far as possible
        w=Generators[i];
        for(j=0;j<dim;j++)
            v[j+2]=w[j];
        v[dim+2]=0;
        if(Extreme_Rays[i]) // after sorting we must recover the extreme rays
            v[dim+2]=1;
        genList.push_back(v);
    }
    genList.sort();
    
    i=0;
    typename list<vector<Integer> >::iterator g=genList.begin();
    for(;g!=genList.end();++g){
        v=*g;
        gen_degrees[i]=explicit_cast_to_long<Integer>(v[0]);
        Extreme_Rays[i]=false;
        if(v[dim+2]>0)
            Extreme_Rays[i]=true;
        for(j=0;j<dim;j++)
            w[j]=v[j+2];
        Generators[i]=w;
        i++;
    }
    
    if (verbose) {
        verboseOutput() << endl << "Degrees after sort" << endl;
        verboseOutput() << count_in_map<long,long>(gen_degrees);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::compute_support_hyperplanes(){
    if(isComputed(ConeProperty::SupportHyperplanes))
        return;

    bool save_tri      = do_triangulation;
    bool save_part_tri = do_partial_triangulation;
    do_triangulation         = false;
    do_partial_triangulation = false;

    build_top_cone();

    do_triangulation         = save_tri;
    do_partial_triangulation = save_part_tri;
}

//---------------------------------------------------------------------------

template<typename Integer>
Simplex<Integer> Full_Cone<Integer>::find_start_simplex() const {
    if (isComputed(ConeProperty::ExtremeRays)) {
        vector<key_t> marked_extreme_rays(0);
        for (size_t i=0; i<nr_gen; i++) {
            if (Extreme_Rays[i])
                marked_extreme_rays.push_back(i);
        }
        vector<key_t> key_extreme = Generators.submatrix(Extreme_Rays).max_rank_submatrix_lex(dim);
        assert(key_extreme.size() == dim);
        vector<key_t> key(dim);
        for (key_t i=0; i<dim; i++) {
            key[i] = marked_extreme_rays[key_extreme[i]];
        }
        return Simplex<Integer>(key, Generators);
    } 
    else {
    // assert(Generators.rank()>=dim); 
        return Simplex<Integer>(Generators);
    }
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
void Full_Cone<Integer>::compute_extreme_rays(){

    if (isComputed(ConeProperty::ExtremeRays))
        return;
    assert(isComputed(ConeProperty::SupportHyperplanes));

    if(dim*Support_Hyperplanes.size() < nr_gen)
         compute_extreme_rays_rank();
    else
         compute_extreme_rays_compare();
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::compute_extreme_rays_rank(){

        size_t i,j;
    typename list<vector<Integer> >::iterator s;
    vector<size_t> gen_in_hyperplanes;
    gen_in_hyperplanes.reserve(Support_Hyperplanes.size());
    Matrix<Integer> M;
    
    for(i=0;i<nr_gen;++i){
        Extreme_Rays[i]=false;
        if (isComputed(ConeProperty::Triangulation) && !in_triang[i])
            continue;
        j=0;
        gen_in_hyperplanes.clear();
        for(s=Support_Hyperplanes.begin();s!=Support_Hyperplanes.end();++s){
            if(v_scalar_product(Generators[i],*s)==0)
                gen_in_hyperplanes.push_back(j);
            j++;
        }
        if(gen_in_hyperplanes.size()< dim-1)
            continue;
        M=select_matrix_from_list(Support_Hyperplanes,gen_in_hyperplanes);
        if(M.rank_destructive()>=dim-1)
            Extreme_Rays[i]=true;   
    }

    is_Computed.set(ConeProperty::ExtremeRays);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::compute_extreme_rays_compare(){

    size_t i,j,k,l,t;
    // Matrix<Integer> SH=getSupportHyperplanes().transpose();
    // Matrix<Integer> Val=Generators.multiplication(SH);
    size_t nc=Support_Hyperplanes.size();
    
    vector<vector<bool> > Val(nr_gen);
    for (i=0;i<nr_gen;++i)
       Val[i].resize(nc);
        
    // Attention: in this routine Val[i][j]==0, i.e. false, indicates that
    // the i-th generator is contained in the j-th support hyperplane
    
    vector<key_t> Zero(nc);
    vector<key_t> nr_zeroes(nr_gen);
    typename list<vector<Integer> >::iterator s;

    for (i = 0; i <nr_gen; i++) {
        k=0;
        Extreme_Rays[i]=true;
        if (isComputed(ConeProperty::Triangulation) && !in_triang[i])
            continue;
        s=Support_Hyperplanes.begin();
        for (j = 0; j <nc; ++j,++s) {
            if (v_scalar_product(Generators[i],*s)==0) {
                k++;
                Val[i][j]=false;                
            }
            else
                Val[i][j]=true;  
        }
        nr_zeroes[i]=k;
        if (k<dim-1||k==nc)  // not contained in enough facets or in all (0 as generator)
            Extreme_Rays[i]=false;
    }

    for (i = 0; i <nr_gen; i++) {
        if(!Extreme_Rays[i])  // already known to be non-extreme
            continue;

        k=0;
        for (j = 0; j <nc; j++) {
            if (Val[i][j]==false) {
                Zero[k]=j;
                k++;
            }
        }

        for (j = 0; j <nr_gen; j++) {
            if (i!=j && Extreme_Rays[j]                // not compare with itself or a known nonextreme ray
                     && nr_zeroes[i]<nr_zeroes[j]) {   // or something whose zeroes cannot be a superset
                l=0;
                for (t = 0; t < nr_zeroes[i]; t++) {
                    if (Val[j][Zero[t]]==false)
                        l++;
                    if (l>=nr_zeroes[i]) {
                        Extreme_Rays[i]=false;
                        break;
                    }
                }
            }
        }
    }

    is_Computed.set(ConeProperty::ExtremeRays);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::select_deg1_elements() {

    typename list<vector<Integer> >::iterator h = Hilbert_Basis.begin();
    for(;h!=Hilbert_Basis.end();h++)
        if(v_scalar_product(Grading,*h)==1)
            Deg1_Elements.push_back(*h);
    is_Computed.set(ConeProperty::Deg1Elements,true);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::check_pointed() {
    assert(isComputed(ConeProperty::SupportHyperplanes));
    if (isComputed(ConeProperty::IsPointed))
        return;
    Matrix<Integer> SH = getSupportHyperplanes();
    pointed = (SH.rank_destructive() == dim);
    is_Computed.set(ConeProperty::IsPointed);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::deg1_check() {
    if (!isComputed(ConeProperty::Grading)          // we still need it and
     && !isComputed(ConeProperty::IsDeg1ExtremeRays)) { // we have not tried it
        if (isComputed(ConeProperty::ExtremeRays)) {
            Matrix<Integer> Extreme=Generators.submatrix(Extreme_Rays);
            Grading = Extreme.find_linear_form();
            if (Grading.size() == dim) {
                is_Computed.set(ConeProperty::Grading);
            } else {
                deg1_extreme_rays = false;
                is_Computed.set(ConeProperty::IsDeg1ExtremeRays);
            }
        } else // extreme rays not known
        if (!isComputed(ConeProperty::IsDeg1Generated)) {
            Grading = Generators.find_linear_form();
            if (Grading.size() == dim) {
                is_Computed.set(ConeProperty::Grading);
            } else {
                deg1_generated = false;
                is_Computed.set(ConeProperty::IsDeg1Generated);
            }
        }
    }

    //now we hopefully have a grading

    if (!isComputed(ConeProperty::Grading)) {
        if (isComputed(ConeProperty::ExtremeRays)) {
            // there is no hope to find a grading later
            deg1_generated = false;
            is_Computed.set(ConeProperty::IsDeg1Generated);
            deg1_extreme_rays = false;
            is_Computed.set(ConeProperty::IsDeg1ExtremeRays);
            if (do_deg1_elements || do_h_vector) {
                errorOutput() << "No grading specified and cannot find one. "
                              << "Disabling some computations!" << endl;
                do_deg1_elements = false;
                do_h_vector = false;
            }
        }
        return; // we are done
    }
    
    set_degrees();
        
    if (!isComputed(ConeProperty::IsDeg1Generated)) {
        deg1_generated = true;
        for (size_t i = 0; i < nr_gen; i++) {
            if (gen_degrees[i] != 1) {
                deg1_generated = false;
                break;
            }
        }
        is_Computed.set(ConeProperty::IsDeg1Generated);
        if (deg1_generated) {
            deg1_extreme_rays = true;
            is_Computed.set(ConeProperty::IsDeg1ExtremeRays);
        }
    }
    if (!isComputed(ConeProperty::IsDeg1ExtremeRays)
      && isComputed(ConeProperty::ExtremeRays)) {
        deg1_extreme_rays = true;
        for (size_t i = 0; i < nr_gen; i++) {
            if (Extreme_Rays[i] && gen_degrees[i] != 1) {
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
    if (isComputed(ConeProperty::IsDeg1HilbertBasis))
        return;

    if ( !isComputed(ConeProperty::Grading) || !isComputed(ConeProperty::HilbertBasis)) {
        errorOutput() << "WARNING: unsatisfied preconditions in check_deg1_hilbert_basis()!" <<endl;
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

template<typename Integer>
void Full_Cone<Integer>::check_integrally_closed() {
    if (isComputed(ConeProperty::IsIntegrallyClosed))
        return;

    if ( !isComputed(ConeProperty::HilbertBasis)) {
        errorOutput() << "WARNING: unsatisfied preconditions in check_integrally_closed()!" <<endl;
        return;
    }
    integrally_closed = false;
    if (Hilbert_Basis.size() <= nr_gen) {
        integrally_closed = true;
        typename list< vector<Integer> >::iterator h;
        for (h = Hilbert_Basis.begin(); h != Hilbert_Basis.end(); ++h) {
            integrally_closed = false;
            for (size_t i=0; i< nr_gen; i++) {
                if ((*h) == Generators[i]) {
                    integrally_closed = true;
                    break;
                }
            }
            if (!integrally_closed) {
                break;
            }
        }
    }
    is_Computed.set(ConeProperty::IsIntegrallyClosed);
}

//---------------------------------------------------------------------------
// Global reduction
//---------------------------------------------------------------------------

// Returns true if new_element is reducible versus the elements in Irred
template<typename Integer>
bool Full_Cone<Integer>::is_reducible(list< vector<Integer>* >& Irred, const vector< Integer >& new_element){
    size_t i;
    size_t s=Support_Hyperplanes.size();
    // new_element can be longer than dim (it has one extra entry for the norm)
    // the scalar product function just takes the first dim entries
    vector <Integer> scalar_product=l_multiplication(Support_Hyperplanes,new_element);
    typename list< vector<Integer>* >::iterator j;
    vector<Integer> *reducer;
    for (j =Irred.begin(); j != Irred.end(); j++) {
        reducer=(*j);
        for (i = 0; i < s; i++) {
            if ((*reducer)[i]>scalar_product[i]){
                break;
            }
        }
        if (i==s) {
            //found a "reducer" and move it to the front
            Irred.push_front(*j);
            Irred.erase(j);
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------

// reduce the Candidates against itself and stores the remaining elements in Hilbert_Basis */
template<typename Integer>
void Full_Cone<Integer>::global_reduction() {
    Integer norm;
    
    list <vector<Integer> > HB;
    typename list <vector<Integer> >::iterator c;
    
    for (size_t i = 0; i <nr_gen; i++) {
        if (in_triang[i])
            Candidates.push_front(Generators[i]);
    }
/*    if(verbose) verboseOutput()<<"sorting the candidates... "<<flush;
    Candidates.sort();
    if(verbose) verboseOutput()<<"make them unique... "<<flush;
    Candidates.unique();
    if(verbose) verboseOutput()<<"done."<<endl;
*/  // Duplicates are avoided or removed earlier
    if (nr_gen == dim) { // cone is simplicial, therefore no global reduction is necessary
        Hilbert_Basis.splice(Hilbert_Basis.end(), Candidates);
        if (verbose) {
            verboseOutput()<<"Cone is simplicial, no global reduction necessary."<<endl;
            verboseOutput()<<Hilbert_Basis.size()<< " Hilbert Basis elements"<<endl;
        }
        return;
    }
    

    vector<Integer> degree_function=compute_degree_function();

    c = Candidates.begin();
    size_t cpos = 0;
    size_t csize=Candidates.size();
    
    if(verbose) {
        verboseOutput()<<"computing the degrees of the candidates... "<<flush;
    }
    //go over candidates: do single scalar product and save it at the end of the candidate
    //for (c = Candidates.begin(); c != Candidates.end(); c++) 
    vector<Integer> scalar_product;
    for (size_t j=0; j<csize; ++j) {
        for(;j > cpos; ++cpos, ++c) ;
        for(;j < cpos; --cpos, --c) ;

        norm=v_scalar_product(degree_function,(*c));
        c->reserve(dim+1);
        c->push_back(norm);

    }
    if(verbose) {
        verboseOutput()<<"sorting the list... "<<endl;
    }
    Candidates.sort(compare_last<Integer>);
    if (verbose) {
        verboseOutput()<< csize <<" candidate vectors sorted."<<endl;
    }
    
    // do global reduction
    list< vector<Integer> > HBtmp;
    Integer norm_crit;
    while ( !Candidates.empty() ) {
        //use norm criterion to find irreducible elements
        c=Candidates.begin();
        norm_crit=(*c)[dim]*2;  //candidates with smaller norm are irreducible
        if ( Candidates.back()[dim] < norm_crit) { //all candidates are irreducible
            if (verbose) {
                verboseOutput()<<Hilbert_Basis.size()+Candidates.size();
                verboseOutput()<<" Hilbert Basis elements of degree <= "<<norm_crit-1<<"; done"<<endl;
            }
            for (; c!=Candidates.end(); ++c) {
                c->pop_back();
            }
            Hilbert_Basis.splice(Hilbert_Basis.end(), Candidates);
            break;
        }
        while ( (*c)[dim] < norm_crit ) { //can't go over the end because of the previous if
            // remove norm
            c->pop_back();
            // push the scalar products to the reducer list
            HBtmp.push_back(l_multiplication(Support_Hyperplanes, *c));
            // and the candidate itself to the Hilbert basis
            Hilbert_Basis.splice(Hilbert_Basis.end(), Candidates, c++);
        }
        csize = Candidates.size();
        if (verbose) {
            verboseOutput()<<Hilbert_Basis.size()<< " Hilbert Basis elements of degree <= "<<norm_crit-1<<"; "<<csize<<" candidates left"<<endl;
        }

        // reduce candidates against HBtmp
        // fill pointer list
        list < vector <Integer>* >  HBpointers;  // used to put "reducer" to the front
        c = HBtmp.begin();
        while (c != HBtmp.end()) {
            HBpointers.push_back(&(*(c++)));
        }

        long VERBOSE_STEPS = 50;      //print | for 2%
        if (verbose && csize>50000) { //print | for 1000 candidates
            VERBOSE_STEPS=csize/1000;
        }
        long step_x_size = csize-VERBOSE_STEPS;
        long counter = 0;
        long steps_done = 0;
        if (verbose) {
            verboseOutput() << "---------+---------+---------+---------+---------+";
            if (VERBOSE_STEPS == 50) {
                verboseOutput() << " (one | per 2%)" << endl;
            } else { 
                verboseOutput() << " (one | per 1000 candidates)" << endl;
            }
        }


        #pragma omp parallel private(c,cpos) firstprivate(HBpointers)
        {
        
        c=Candidates.begin();
        cpos=0;
        #pragma omp for schedule(dynamic)
        for (size_t k=0; k<csize; ++k) {
            for(;k > cpos; ++cpos, ++c) ;
            for(;k < cpos; --cpos, --c) ;
            
            if ( is_reducible(HBpointers, *c) ) {
                (*c)[dim]=-1; //mark as reducible
            }

            if (verbose) {
                #pragma omp critical(VERBOSE)
                {
                counter++;

                while (counter*VERBOSE_STEPS >= step_x_size) {
                    steps_done++;
                    step_x_size += csize;
                    verboseOutput() << "|" <<flush;
                    if(VERBOSE_STEPS > 50 && steps_done%50 == 0) {
                        verboseOutput() << "  " << (steps_done) << "000" << endl;
                    }
                }
                } //end critical(VERBOSE)
            }
        } //end for
        } //end parallel
        if (verbose) verboseOutput() << endl;

        // delete reducible candidates
        c = Candidates.begin();
        while (c != Candidates.end()) {
            if ((*c)[dim]==-1) {
                c = Candidates.erase(c);
            } else {
                ++c;
            }
        }
        HBtmp.clear();
    }

    if (verbose) {
        verboseOutput()<<Hilbert_Basis.size()<< " Hilbert Basis elements"<<endl;
    }
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
        typename list< vector<Integer> >::const_iterator h;
        for (h=Support_Hyperplanes.begin(); h!=Support_Hyperplanes.end(); ++h) {
            for (i=0; i<dim; i++) {
                degree_function[i]+=(*h)[i];
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

template<typename Integer>
Integer Full_Cone<Integer>::primary_multiplicity() const{
    size_t i,j,k;
    Integer primary_multiplicity=0;
    vector <key_t> key,new_key(dim-1);
    Matrix<Integer> Projection(nr_gen,dim-1);
    for (i = 0; i < nr_gen; i++) {
        for (j = 0; j < dim-1; j++) {
            Projection.write(i,j,Generators[i][j]);
        }
    }
    typename list< vector<Integer> >::const_iterator h;
    typename list< SHORTSIMPLEX<Integer> >::const_iterator t;
    for (h =Support_Hyperplanes.begin(); h != Support_Hyperplanes.end(); ++h){
        if ((*h)[dim-1]!=0) {
            for (t =Triangulation.begin(); t!=Triangulation.end(); ++t){
                key=t->key;
                for (i = 0; i <dim; i++) {
                    k=0;
                    for (j = 0; j < dim; j++) {
                        if (j!=i && Generators[key[j]][dim-1]==1) {
                            if (v_scalar_product(Generators[key[j]],(*h))==0) {
                                k++;
                            }
                        }
                        if (k==dim-1) {
                            for (j = 0; j <i; j++) {
                                new_key[j]=key[j];
                            }
                            for (j = i; j <dim-1; j++) {
                                new_key[j]=key[j+1];
                            }
                            // add the volume of the projected simplex
                            primary_multiplicity +=
                              Projection.submatrix(new_key).vol_destructive();
                        }
                    }
                }
            }
        }
    }
    return primary_multiplicity;
}
//---------------------------------------------------------------------------
// Constructors
//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::reset_tasks(){
    do_triangulation = false;
    do_partial_triangulation = false;
    do_multiplicity=false;
    do_Hilbert_basis = false;
    do_deg1_elements = false;
    keep_triangulation = false;
    do_Stanley_dec=false;
    do_h_vector=false;
    
    do_evaluation = false;
    do_only_multiplicity=false;

    nrSimplicialPyr=0;
    totalNrPyr=0;
    is_pyramid = false;
}

//---------------------------------------------------------------------------

template<typename Integer>
Full_Cone<Integer>::Full_Cone(Matrix<Integer> M){ // constructor of the top cone
    dim=M.nr_of_columns();
    if (dim!=M.rank()) {
        error_msg("error: Matrix with rank = number of columns needed in the constructor of the object Full_Cone<Integer>.\nProbable reason: Cone not full dimensional (<=> dual cone not pointed)!");
        throw BadInputException();
    }
    Generators = M;
    nr_gen=Generators.nr_of_rows();
    if (nr_gen != static_cast<size_t>(static_cast<key_t>(nr_gen))) {
        error_msg("To many generators to fit in range of key_t!");
        throw FatalException();
    }
    //make the generators coprime, remove 0 rows and duplicates
    vector<Integer> gcds = Generators.make_prime();
    bool remove_some = false;
    vector<bool> key(nr_gen, true);
    for (size_t i = 0; i<nr_gen; i++) {
        if (gcds[i] == 0) {
           key[i] = false;
           remove_some = true;
           continue;
        }
        for (size_t j=0; j<i; j++) {
            if (Generators[i] == Generators[j]) {
                key[i] = false;
                remove_some = true;
                break;
            }
        }
    }
    if (remove_some) {
        Generators=Generators.submatrix(key);
        nr_gen=Generators.nr_of_rows();
    }
    multiplicity = 0;
    is_Computed = bitset<ConeProperty::EnumSize>();  //initialized to false
    is_Computed.set(ConeProperty::Generators);
    pointed = false;
    deg1_extreme_rays = false;
    deg1_generated = false;
    deg1_hilbert_basis = false;
    integrally_closed = false;
    
    reset_tasks();
    
    Extreme_Rays = vector<bool>(nr_gen,false);
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
    TriangulationSize=0;
    CandidatesSize=0;
    detSum = 0;
    
    FS.resize(omp_get_max_threads());
    
    Pyramids.resize(1);  // prepare storage for pyramids
    nrPyramids.resize(1);
    RecPyrs.resize(1);  // prepare storage for pyramids
    nrRecPyrs.resize(1);
    nrPyramids[0]=0;
    nrRecPyrs[0]=0;
    
    recursion_allowed=true;
    
    do_all_hyperplanes=true;
    parallel_inside_pyramid=true;
    
    supphyp_recursion=false; 
    tri_recursion=false;
    
    Done=false;
    
    nextGen=-1;
    
    nrRecPyramidsDue=0;  
    nrRecPyramidsDone=0;
    allRecPyramidsBuilt=true;

}

//---------------------------------------------------------------------------

template<typename Integer>
Full_Cone<Integer>::Full_Cone(const Cone_Dual_Mode<Integer> &C) {

    dim = C.dim;
    Generators = C.get_generators();
    nr_gen = Generators.nr_of_rows();

    multiplicity = 0;
    is_Computed =  bitset<ConeProperty::EnumSize>();  //initialized to false
    is_Computed.set(ConeProperty::Generators);
    pointed = true;
    is_Computed.set(ConeProperty::IsPointed);
    deg1_extreme_rays = false;
    deg1_generated = false;
    deg1_triangulation = false;
    deg1_hilbert_basis = false;
    integrally_closed = false;
    
    reset_tasks();
    
    Extreme_Rays = vector<bool>(nr_gen,true); //all generators are extreme rays
    is_Computed.set(ConeProperty::ExtremeRays);
    Matrix<Integer> SH = C.SupportHyperplanes;
    for (size_t i=0; i < SH.nr_of_rows(); i++) {
        Support_Hyperplanes.push_back(SH[i]);
    }
    is_Computed.set(ConeProperty::SupportHyperplanes);
    in_triang = vector<bool>(nr_gen,false);
    Hilbert_Basis = C.Hilbert_Basis;
    is_Computed.set(ConeProperty::HilbertBasis);
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
    TriangulationSize=0;
    CandidatesSize=0;
    detSum = 0;
    
    do_all_hyperplanes=true;
    
    supphyp_recursion=false; 
    tri_recursion=false;
    
    Done=false;
    
    nextGen=-1;
    
    nrRecPyramidsDue=0;  
    nrRecPyramidsDone=0;
    allRecPyramidsBuilt=true; 
}
//---------------------------------------------------------------------------

/* constructor for pyramids */
template<typename Integer>
Full_Cone<Integer>::Full_Cone(Full_Cone<Integer>& C, const vector<key_t>& Key) {

    Generators = C.Generators.submatrix(Key);
    dim = Generators.nr_of_columns();
    nr_gen = Generators.nr_of_rows();
    
    Top_Cone=C.Top_Cone; // relate to top cone
    Top_Key.resize(nr_gen);
    for(size_t i=0;i<nr_gen;i++)
        Top_Key[i]=C.Top_Key[Key[i]];
  
    multiplicity = 0;
    
    Extreme_Rays = vector<bool>(nr_gen,false);
    is_Computed.set(ConeProperty::ExtremeRays, C.isComputed(ConeProperty::ExtremeRays));
    if(isComputed(ConeProperty::ExtremeRays))
        for(size_t i=0;i<nr_gen;i++)
            Extreme_Rays[i]=C.Extreme_Rays[Key[i]];
    in_triang = vector<bool> (nr_gen,false);
    deg1_triangulation = true;
    
    Grading=C.Grading;
    is_Computed.set(ConeProperty::Grading, C.isComputed(ConeProperty::Grading));
    Order_Vector=C.Order_Vector;
    
    do_triangulation=C.do_triangulation;
    do_partial_triangulation=C.do_partial_triangulation;
    do_multiplicity=C.do_multiplicity;
    do_deg1_elements=C.do_deg1_elements;
    do_h_vector=C.do_h_vector;
    do_Hilbert_basis=C.do_Hilbert_basis;
    keep_triangulation=C.keep_triangulation;
    do_only_multiplicity=C.do_only_multiplicity;
    do_evaluation=C.do_evaluation;
    do_Stanley_dec=C.do_Stanley_dec;
    is_pyramid=true;
    
    // pyr_level set by the calling routine
    
    totalNrSimplices=0;
    detSum = 0;
    if(C.gen_degrees.size()>0){ // now we copy the degrees
    	gen_degrees.resize(nr_gen);
        for (size_t i=0; i<nr_gen; i++) {
            gen_degrees[i] = C.gen_degrees[Key[i]];
        }
    }
    TriangulationSize=0;
    CandidatesSize=0;
    
    recursion_allowed=C.recursion_allowed; // must be reset for non-recursive pyramids
    do_all_hyperplanes=true; //  must be reset for non-recursive pyramids
    parallel_inside_pyramid=false; // no parallelization inside proper pyramids
    
    supphyp_recursion=false; 
    tri_recursion=false;
    
    Done=false;
    
    nextGen=-1;
    
    nrRecPyramidsDue=0;  
    nrRecPyramidsDone=0;
    allRecPyramidsBuilt=true;
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

template<typename Integer>
bool Full_Cone<Integer>::isIntegrallyClosed() const{
    return integrally_closed;
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
const Matrix<Integer>& Full_Cone<Integer>::getGenerators()const{
    return Generators;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<bool> Full_Cone<Integer>::getExtremeRays()const{
    return Extreme_Rays;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Full_Cone<Integer>::getSupportHyperplanes()const{
    size_t s= Support_Hyperplanes.size();
    Matrix<Integer> M(s,dim);
    size_t i=0;
    typename list< vector<Integer> >::const_iterator l;
    for (l =Support_Hyperplanes.begin(); l != Support_Hyperplanes.end(); l++) {
        M.write(i,(*l));
        i++;
    }
    return M;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Full_Cone<Integer>::getTriangulation(list< vector<key_t> >& Triang, list<Integer>& TriangVol) const {
    Triang.clear();
    TriangVol.clear();
    vector<key_t> key(dim);
    typename list< SHORTSIMPLEX<Integer> >::const_iterator l;
    for (l =Triangulation.begin(); l != Triangulation.end(); l++) {
        key=l->key;
        Triang.push_back(key);
        TriangVol.push_back(l->height);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Full_Cone<Integer>::getHilbertBasis()const{
    size_t s= Hilbert_Basis.size();
    Matrix<Integer> M(s,dim);
    size_t i=0;
    typename list< vector<Integer> >::const_iterator l;
    for (l =Hilbert_Basis.begin(); l != Hilbert_Basis.end(); l++) {
        M.write(i,(*l));
        i++;
    }
    return M;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Full_Cone<Integer>::getDeg1Elements()const{
    size_t s= Deg1_Elements.size();
    Matrix<Integer> M(s,dim);
    size_t i=0;
    typename list< vector<Integer> >::const_iterator l;
    for (l =Deg1_Elements.begin(); l != Deg1_Elements.end(); l++) {
        M.write(i,(*l));
        i++;
    }
    return M;
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
    verboseOutput()<<"\nhyp_size="<<hyp_size<<".\n";
    verboseOutput()<<"\nGrading is:\n";
    verboseOutput()<< Grading;
    verboseOutput()<<"\nMultiplicity is "<<multiplicity<<".\n";
    verboseOutput()<<"\nGenerators are:\n";
    Generators.read();
    verboseOutput()<<"\nExtreme_rays are:\n";
    verboseOutput()<< Extreme_Rays;
    verboseOutput()<<"\nSupport Hyperplanes are:\n";
    verboseOutput()<< Support_Hyperplanes;
    verboseOutput()<<"\nTriangulation is:\n";
    verboseOutput()<< Triangulation;
    verboseOutput()<<"\nHilbert basis is:\n";
    verboseOutput()<< Hilbert_Basis;
    verboseOutput()<<"\nDeg1 elements are:\n";
    verboseOutput()<< Deg1_Elements;
    verboseOutput()<<"\nHilbert Series  is:\n";
    verboseOutput()<<Hilbert_Series;
}

} //end namespace


