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

#include "cone_dual_mode.h"
#include "vector_operations.h"
#include "lineare_transformation.h"
#include "list_operations.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------
//private

//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::unique_vectors(list< vector< Integer > >& HB){

    if(HB.empty())
        return;
        
    typename list<vector<Integer> >::iterator h,h_start,prev;
    h_start=HB.begin();
    size_t i;
    size_t t=(*h_start).size();
    size_t s=t-dim;

    h_start++;    
    for(h=h_start;h!=HB.end();){
        prev=h;
        prev--;
        for(i=s;i<t;++i)
            if((*h)[i]!=(*prev)[i])
                break;
        if(i==t)
            h=HB.erase(h);
        else
            ++h;
        
    }
}
//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::splice_them(list< vector< Integer > >& Total, vector<list< vector< Integer > > >& Parts){

    for(int i=0;i<omp_get_max_threads();i++)
        Total.splice(Total.end(),Parts[i]);
}

//---------------------------------------------------------------------------


template<typename Integer>
void Cone_Dual_Mode<Integer>::record_order(list< vector< Integer > >& Elements, list< vector< Integer >* >& Order){

    Order.clear();
    typename list < vector<Integer> >::iterator it=Elements.begin();
    for(;it!=Elements.end();++it)
            Order.push_back(&(*it));
}

//---------------------------------------------------------------------------


template<typename Integer>
void Cone_Dual_Mode<Integer>::record_reducers(vector<list<vector<Integer> > >& Register, list< vector< Integer >* >& Order){

    Order.clear();
    typename list < vector<Integer> >::iterator it;
    for(size_t i=0; i<Register.size();++i)
        for(it=Register[i].begin();it!=Register[i].end();++it) {
            Order.push_back(&(*it));
    }
}

//---------------------------------------------------------------------------


template<typename Integer>
void Cone_Dual_Mode<Integer>::record_partners(vector<list<vector<Integer> > >& Register, 
                 list< vector< Integer >* >& partners, size_t& nr_partners, size_t deg){

    if(Register.size()<=deg)
        return ;
    
    typename list < vector<Integer> >::iterator it;
    for(it=Register[deg].begin();it!=Register[deg].end();++it) {
        partners.push_front(&(*it));
        nr_partners++;
    }
}


//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::insert_into_Register(vector<list<vector<Integer> > >& Register,
                    list<vector<Integer> >& HB, typename list< vector<Integer> >::iterator h){

    
    long deg=explicit_cast_to_long<Integer>((*h)[nr_sh+1]);
    if((long) Register.size()<=deg)
        Register.resize(deg+1);
    Register[deg].splice(Register[deg].end(),HB, h);                   
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::insert_all_into_Register(vector<list<vector<Integer> > >& Register,
                    list<vector<Integer> >& HB){
                    
    if(HB.empty())
        return;
                    
    long deg=explicit_cast_to_long<Integer>((*HB.begin())[nr_sh+1]);
    if((long) Register.size()<=deg)
        Register.resize(deg+1);
    Register[deg].splice(Register[deg].end(),HB);  
                    
}

//---------------------------------------------------------------------------

template<typename Integer>
bool Cone_Dual_Mode<Integer>::reducible( list< vector< Integer >* >& Irred, const vector< Integer >& new_element, 
                              const size_t& size, const bool ordered){
    size_t i,c=1;
    typename list< vector<Integer>* >::iterator j;
    vector<Integer> *reducer;
    for (j =Irred.begin(); j != Irred.end(); j++) {
        reducer=(*j);
        if ( ordered &&  new_element[0]<(*reducer)[0]) {  // ordered && new_element[0]<2*(*reducer
            break; // if element is reducible, divisor of smallest degree will be found later
        }
        if (new_element[0]<=(*reducer)[0])
            continue;
        if ((*reducer)[c]<=new_element[c]){
            for (i = 1; i <=size ; i++) {
                if ((*reducer)[i]>new_element[i]){
                    c=i;
                    break;
                }
            }
            if (i==size+1) {
                Irred.splice(Irred.begin(),Irred,j);
                return true;
            }
        }
        //new_element is reducible
    }
    return false;
}

//---------------------------------------------------------------------------

template<typename Integer>
bool Cone_Dual_Mode<Integer>::reducible_pointed( list< vector< Integer >* >& Irred, const vector< Integer >& new_element, 
                              const size_t& size){
    size_t i,c=1;
    typename list< vector<Integer>* >::iterator j;
    vector<Integer> *reducer;
    for (j =Irred.begin(); j != Irred.end(); j++) {
        reducer=(*j);
        if ( new_element[nr_sh+1]<2*(*reducer)[nr_sh+1]) {  // 
            break; // element is irreducible in the pointed case
        }
        if (new_element[0]<=(*reducer)[0])
            continue;
        if ((*reducer)[c]<=new_element[c]){
            for (i = 1; i <=size ; i++) {
                if ((*reducer)[i]>new_element[i]){
                    c=i;
                    break;
                }
            }
            if (i==size+1) {
                Irred.splice(Irred.begin(),Irred,j);
                return true;
            }
        }
        //new_element is reducible
    }
    return false;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::reduce(list< vector< Integer > >& Irred, list< vector< Integer > >& Red, const size_t& size){

    list< vector< Integer >* > Irred_Reorder;
    record_order(Irred, Irred_Reorder);
    
    typename list< vector<Integer> >::iterator s;
    for (s = Red.begin(); s != Red.end();) 
        if(reducible(Irred_Reorder,*s,size,true))
            s=Red.erase(s);
        else
            ++s;
}
    
//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::reduce_and_insert(const vector< Integer >& new_element, 
                        list<vector<Integer> >& Irred,const size_t& size){
    size_t i,c=1;
    typename list< vector<Integer> >::iterator j;
    for (j =Irred.begin(); j != Irred.end(); j++) {
        if (new_element[0]<(*j)[0]) {                    // new_element[0]<2*(*j)[0]
            break; // if element is reducible, divisor of smallest degree will be found later
        }
        else  {

            if ((*j)[c]<=new_element[c]){
                for (i = 1; i <= size; i++) {
                    if ((*j)[i]>new_element[i]){
                        c=i;
                        break;
                    }
                }
                if (i==size+1) {
                    Irred.splice(Irred.begin(),Irred,j);
                    return;
                }
            }
        }
    }
    Irred.push_back(new_element);
}


//---------------------------------------------------------------------------
template<typename Integer>
void Cone_Dual_Mode<Integer>::auto_reduce(list< vector< Integer> >& To_Reduce, const size_t& size, bool no_pos_in_level0){

        To_Reduce.sort();
        To_Reduce.unique();
        if(truncate && no_pos_in_level0) // in this case we have only to make unique
            return;
        list<vector<Integer> > Irred;
        typename list < vector <Integer> >::iterator c;

        for (c=To_Reduce.begin(); c != To_Reduce.end(); ++c) {
            reduce_and_insert((*c),Irred,size);
        }
        To_Reduce.clear();
        To_Reduce.splice(To_Reduce.begin(),Irred);
        To_Reduce.sort();
}

//---------------------------------------------------------------------------
//public
//---------------------------------------------------------------------------

template<typename Integer>
Cone_Dual_Mode<Integer>::Cone_Dual_Mode(Matrix<Integer> M){
    dim=M.nr_of_columns();
    if (dim!=M.rank()) {
        errorOutput()<<"Cone_Dual_Mode error: constraints do not define pointed cone!"<<endl;
        // M.pretty_print(errorOutput());
        throw BadInputException();
    }
    SupportHyperplanes = M;
    nr_sh=SupportHyperplanes.nr_of_rows();
    if (nr_sh != static_cast<size_t>(static_cast<key_t>(nr_sh))) {
        errorOutput()<<"Too many support hyperplanes to fit in range of key_t!"<<endl;
        throw FatalException();
    }
    //make the generators coprime and remove 0 rows
    vector<Integer> gcds = SupportHyperplanes.make_prime();
    vector<key_t> key=v_non_zero_pos(gcds);
    if (key.size() < nr_sh) {
        SupportHyperplanes=SupportHyperplanes.submatrix(key);
        nr_sh=SupportHyperplanes.nr_of_rows();
    }
    hyp_size=dim+nr_sh;
    first_pointed=true;

}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::print()const{
    verboseOutput()<<"dim="<<dim<<".\n";
    verboseOutput()<<"nr_sh="<<nr_sh<<".\n";
    verboseOutput()<<"hyp_size="<<hyp_size<<".\n";
    verboseOutput()<<"GeneratorList are:\n";
    verboseOutput()<<GeneratorList<<endl;
    verboseOutput()<<"Support Hyperplanes are:\n";
    SupportHyperplanes.read();verboseOutput()<<endl;
    verboseOutput()<<"Hilbert Basis is:\n";
    verboseOutput()<<Hilbert_Basis<<endl;
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

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Cone_Dual_Mode<Integer>::read_hilbert_basis()const{
    size_t s= Hilbert_Basis.size();
    Matrix<Integer> M(s,dim);
    size_t i;
    typename list< vector<Integer> >::const_iterator l;
    for (i=0, l =Hilbert_Basis.begin(); l != Hilbert_Basis.end(); ++l, ++i) {
        M.write(i,(*l));
    }
    return M;
}

size_t counter=0,counter1=0;

//---------------------------------------------------------------------------

// The vectors in GeneratorList are composed as follows:
// [0]: current total degree (sum of linear forms applied to vector so far)
// [1.. nr_sh]: values under the linear forms
// [nr_sh+1]: generation (in the sense of creation): we match vectors of which at least one has gen >=1
//            and the sum gets gen = 2.
// [nr_sh+2]: value of "mother" under the current linear form (given to newly produced vector)
// last dim coordinates: original coordinates of vector
//
// [nr_sh+1] is used as the degree function as soon as a pointed cone is to be split
// into two halfs. [0] is the total degree in each of the two halves, and not additive 
// in the whole cone.//
//
// In the inhomogeneous case or when only degree 1 elements are to be found,
// we truncate the Hilbert basis at level 1. The level is the ordinaryl degree in
// for degree 1 elements and the degree of the homogenizing variable 
// in the inhomogeneous case.
//
// As soon as there are no positive or neutral (with respect to the current hyperplane)
// elements in the cirrent Hilbert basis and truncate==true, new elements can only 
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
        verboseOutput()<<"cut with halfspace "<<hyp_counter<<" ..."<<endl;
    }
    truncate=inhomogeneous || do_only_Deg1_Elements;
    
    size_t sh_1=nr_sh+1, sh_2=nr_sh+2, sh_3=nr_sh+3;
    
    size_t i;
    int sign;
    bool not_done;
    list < vector<Integer> > Positive_Irred,Negative_Irred,Neutral_Irred;
    Integer orientation, scalar_product,diff,factor;
    vector <Integer> hyperplane=SupportHyperplanes.read(hyp_counter-1);
    typename list< vector<Integer> >::iterator h;
    if (lifting==true) {
        orientation=v_scalar_product<Integer>(hyperplane,old_lin_subspace_half);
        if(orientation<0){
            orientation=-orientation;
            v_scalar_multiplication<Integer>(old_lin_subspace_half,-1); //transforming into the generator of the positive half of the old max lin subsapce
        }
        for (h = Hilbert_Basis.begin(); h != Hilbert_Basis.end(); ++h) { //reduction  modulo  the generators of the two halves of the old max lin subspace
            scalar_product=v_scalar_product_unequal_vectors_end(hyperplane,(*h)); // this step is in principle szperfluous
            sign=1;                                                               // since the following steps would cover it
            if (scalar_product<0) {
                scalar_product=-scalar_product;
                sign=-1;
            }
            factor=scalar_product/orientation;
            for (i = 0; i < dim; i++) {
                (*h)[sh_3+i]=(*h)[sh_3+i]-sign*factor*old_lin_subspace_half[i];
            }
        }
        //adding the generators of the halves of the old max lin subspaces to the the "positive" and the "negative" generators
        // ABSOLUTELY NECESSARY since we need a monoid system of generators of the full "old" cone
        vector <Integer> hyp_element(hyp_size+3,0);
        for (i = 0; i < dim; i++) {
            hyp_element[sh_3+i]= old_lin_subspace_half[i];
        }
        hyp_element[hyp_counter]=orientation;
        hyp_element[0]=orientation;
        if (orientation==0){ //never
            // Hilbert_Basis.push_front(hyp_element); //
            Neutral_Irred.push_back(hyp_element);
        }
        else{
            // Hilbert_Basis.push_front(hyp_element); // 
            Positive_Irred.push_back(hyp_element);
            v_scalar_multiplication<Integer>(hyp_element,-1);
            hyp_element[hyp_counter]=orientation;
            hyp_element[0]=orientation;
            // Hilbert_Basis.push_front(hyp_element); //
            Negative_Irred.push_back(hyp_element);
        }
    } //end lifting
    
    bool no_pos_in_level0=pointed;
    bool all_positice_level=pointed;
    for (h = Hilbert_Basis.begin(); h != Hilbert_Basis.end(); ++h) { //dividing into negative and positive
        (*h)[hyp_counter]=v_scalar_product_unequal_vectors_end<Integer>(hyperplane,(*h));
        if ((*h)[hyp_counter]>0) {
            (*h)[sh_1]=1;     // generation
            (*h)[sh_2]=0;     //not sum
            (*h)[0]+=(*h)[hyp_counter];
            Positive_Irred.push_back((*h));
            if((*h)[1]==0){
                no_pos_in_level0=false;
                all_positice_level=false;
            }
        }
        if ((*h)[hyp_counter]<0) {
            (*h)[hyp_counter]=-(*h)[hyp_counter];
            (*h)[sh_1]=1;
            (*h)[sh_2]=0;
            (*h)[0]+=(*h)[hyp_counter];
            Negative_Irred.push_back((*h));
            if((*h)[1]==0){
                all_positice_level=false;
            }
        }
        if ((*h)[hyp_counter]==0) {
            (*h)[sh_1]=0;
            (*h)[sh_2]=0;
            Neutral_Irred.push_back((*h));
            if((*h)[1]==0){
                no_pos_in_level0=false;
                all_positice_level=false;
            }
        }
    }
    
    // cout << "no_pos_in_level0 = " << no_pos_in_level0 <<endl;
    // cout << "all_positive_level = " << all_positice_level <<endl;
    
    if((truncate && (no_pos_in_level0 && !all_positice_level))){
        if(verbose){
            verboseOutput() << "Eliminating negative generators of level > 0" << endl;
        }
        for (h = Negative_Irred.begin(); h != Negative_Irred.end();){
            if((*h)[1]>0)
                h=Negative_Irred.erase(h);
            else
                ++h;
        }
    }
    
    #pragma omp parallel
    {
        
        #pragma omp single nowait
        {
        check_size(Negative_Irred);
        Negative_Irred.sort();
        }
        
        #pragma omp single nowait
        {
        check_size(Positive_Irred);
        Positive_Irred.sort();
        }
        
        #pragma omp single nowait    
        Neutral_Irred.sort();
    }
    
    //long int counter=0;
    list < vector <Integer> > New_Positive,New_Negative,New_Neutral,Pos,Neg,Neu;
    vector<list< vector< Integer > > > New_Positive_thread(omp_get_max_threads()),
                      New_Negative_thread(omp_get_max_threads()),
                      New_Neutral_thread(omp_get_max_threads());
    typename list < vector<Integer> >::const_iterator n,p;
    typename list < vector <Integer> >::iterator c;
    not_done=true;
    while(not_done && !(truncate && all_positice_level)) {
        not_done=false;
        New_Positive.clear();
        New_Negative.clear();
        New_Neutral.clear();
        
        //generating new elements
        
        list < vector<Integer>* > Positive,Negative,Neutral; // pointer lists, used to move reducers to the front
        size_t psize=Positive_Irred.size();
        #pragma omp parallel if (Negative_Irred.size()*psize>10000)
        {        
        #pragma omp single nowait
        record_order(Negative_Irred,Negative);
        
        #pragma omp single nowait
        record_order(Positive_Irred,Positive);
        
        #pragma omp single nowait
        record_order(Neutral_Irred,Neutral);
        } // END PARALLEL
    

        if (verbose) {
            size_t nsize=Negative_Irred.size();
            size_t zsize=Neutral_Irred.size();
            if (psize*nsize>1000000)
                verboseOutput()<<"Positive: "<<psize<<"  Negative: "<<nsize<<"  Neutral: "<<zsize<<endl;
        }
        #pragma omp parallel private(p,n,diff) firstprivate(Positive,Negative,Neutral)
        {
        vector <Integer> new_candidate(hyp_size+3);
        size_t ppos=0;
        p = Positive_Irred.begin();
        #pragma omp for schedule(dynamic)
        for(i = 0; i<psize; ++i){
            for(;i > ppos; ++ppos, ++p) ;
            for(;i < ppos; --ppos, --p) ;

            for (n = Negative_Irred.begin(); n != Negative_Irred.end(); ++n){
            
                // cout << "In Schleife " << endl;
            
            
                if(truncate && (*p)[1]+(*n)[1]>=2) // in the inhomogeneous case we truncate at level 1
                    continue;
                    
                if ((*p)[sh_1]<=1 && (*n)[sh_1]<=1
                && ((*p)[sh_1]!=0 || (*n)[sh_1]!=0)) {
                    if (((*p)[sh_2]!=0&&(*p)[sh_2]<=(*n)[hyp_counter])
                    || ((*n)[sh_2]!=0&&(*n)[sh_2]<=(*p)[hyp_counter])){
                        #pragma omp atomic
                        counter1++;                                    
                        continue;
                    }
                    #pragma omp atomic                    
                    counter++;
                    diff=(*p)[hyp_counter]-(*n)[hyp_counter];
                    v_add_result(new_candidate,(*p),(*n));   // new_candidate=v_add

                    if (diff>0) {
                        new_candidate[hyp_counter]=diff;
                        new_candidate[0]-=2*(*n)[hyp_counter];
                        if(!(truncate && no_pos_in_level0) && (reducible(Positive,new_candidate,hyp_counter,false) ||
                                    reducible(Neutral,new_candidate,hyp_counter-1,false))) 
                            continue;
                        new_candidate[sh_1]=2;
                        new_candidate[sh_2]=(*p)[hyp_counter];
                        // #pragma omp critical(NEW_POSITIVE)
                        New_Positive_thread[omp_get_thread_num()].push_back(new_candidate);
                    }
                    if (diff<0) {
                        if(truncate && no_pos_in_level0) // don't need new negative elements anymore
                            continue;
                        new_candidate[hyp_counter]=-diff;
                        new_candidate[0]-=2*(*p)[hyp_counter];
                        if(reducible(Negative,new_candidate,hyp_counter,false)) {
                            continue;
                        }
                        if(reducible(Neutral,new_candidate,hyp_counter-1,false)) {
                            continue;
                        }
                        new_candidate[sh_1]=2;
                        new_candidate[sh_2]=(*n)[hyp_counter];
                        // #pragma omp critical(NEW_NEGATIVE)
                        New_Negative_thread[omp_get_thread_num()].push_back(new_candidate);
                    }
                    if (diff==0) {
                        new_candidate[hyp_counter]=0;
                        new_candidate[0]-=2*(*p)[hyp_counter];
                        if(!(truncate && no_pos_in_level0) && reducible(Neutral,new_candidate,hyp_counter-1,false)) {
                            continue;
                        }
                        new_candidate[sh_1]=0;
                        new_candidate[sh_2]=0;
                        // #pragma omp critical(NEW_NEUTRAL)
                        New_Neutral_thread[omp_get_thread_num()].push_back(new_candidate);
                    }
                }
            }
        }
 
        //end generation of new elements

        } //END PARALLEL

        #pragma omp parallel 
        {
        #pragma omp single nowait
        {
        splice_them(New_Neutral,New_Neutral_thread);
        auto_reduce(New_Neutral,hyp_counter-1,no_pos_in_level0);
        }
        #pragma omp single nowait
        {
        splice_them(New_Positive,New_Positive_thread);
        auto_reduce(New_Positive,hyp_counter-1,no_pos_in_level0);
        }
        #pragma omp single nowait
        {
        splice_them(New_Negative,New_Negative_thread);
        auto_reduce(New_Negative,hyp_counter-1,no_pos_in_level0);
        }
        } // END PARALLEL

        
        if (New_Neutral.size()!=0) {
            #pragma omp parallel
            {
            #pragma omp single nowait
            {
            if(!(truncate && no_pos_in_level0))
                reduce(New_Neutral,New_Positive, hyp_counter-1);
            }

            #pragma omp single nowait
            reduce(New_Neutral,New_Negative, hyp_counter-1);
            
            #pragma omp single nowait
            {
            if(!(truncate && no_pos_in_level0))
                reduce(New_Neutral,Neutral_Irred, hyp_counter-1);
            }

            #pragma omp single nowait
            {
            if(!(truncate && no_pos_in_level0))
                reduce(New_Neutral,Positive_Irred, hyp_counter-1);
            }

            #pragma omp single nowait
            reduce(New_Neutral,Negative_Irred, hyp_counter-1);
            } // END PARALLEL
            Neutral_Irred.merge(New_Neutral);
        }
        
        #pragma omp parallel
        {
        #pragma omp single nowait
        if (New_Positive.size()!=0) {
            not_done=true;
            if(!(truncate && no_pos_in_level0))
                reduce(New_Positive,Positive_Irred, hyp_counter);
            check_size(New_Positive);
            Positive_Irred.merge(New_Positive);
        }
        #pragma omp single nowait
        if (New_Negative.size()!=0) {
            not_done=true;
            reduce(New_Negative,Negative_Irred, hyp_counter);
            check_size(New_Negative);
            Negative_Irred.merge(New_Negative);
        }
        } // PARALLEL
        
        // adjust generation

        #pragma omp parallel
        {
        #pragma omp single nowait
        for (c = Positive_Irred.begin(); c != Positive_Irred.end(); ++c){
            if((*c)[sh_1]>0) {
                (*c)[sh_1]--;
            }
        }
        #pragma omp single nowait
        for (typename list < vector <Integer> >::iterator c2 = Negative_Irred.begin(); c2 != Negative_Irred.end(); ++c2){
            if((*c2)[sh_1]>0) {
                (*c2)[sh_1]--;
            }
        }
        } // END PARALLEL
//      verboseOutput()<<not_done;
    }
    
    
    Hilbert_Basis.clear();
    Hilbert_Basis.splice(Hilbert_Basis.begin(),Positive_Irred);
    Hilbert_Basis.splice(Hilbert_Basis.end(),Neutral_Irred);
    
    //still possible to have double elements in the Hilbert basis, coming from different generations

    for (c = Hilbert_Basis.begin(); c != Hilbert_Basis.end(); ++c) {
        (*c)[sh_1]=0;
        (*c)[sh_2]=0;
    }
    Hilbert_Basis.sort();
    unique_vectors(Hilbert_Basis); 
    
    // (Matrix<Integer>(Hilbert_Basis)).print(cout);

    if (verbose) {
        verboseOutput()<<"Hilbert basis size="<<Hilbert_Basis.size()<<endl;
    }
}

//---------------------------------------------------------------------------

// not used at present
template<typename Integer>
void Cone_Dual_Mode<Integer>::cut_with_halfspace_hilbert_basis_pointed(const size_t& hyp_counter){
    if (verbose==true) {
        verboseOutput()<<"cut pointed with halfspace "<<hyp_counter<<" ..."<<endl;
    }
    truncate=inhomogeneous || do_only_Deg1_Elements;
    
    size_t sh_1=nr_sh+1, sh_2=nr_sh+2;

    Integer diff;
    vector <Integer> hyperplane=SupportHyperplanes[hyp_counter-1];
    typename list< vector<Integer> >::iterator h, insert;
    
    vector<list<vector<Integer> > > Pos_Register, Neg_Register, Neu_Register; // collect HB elements by degree    

    bool no_pos_in_level0=true;
    bool all_positice_level=true;
                  
    for (h = Hilbert_Basis.begin(); h != Hilbert_Basis.end();) { //dividing into negative and positive
        (*h)[hyp_counter]=v_scalar_product_unequal_vectors_end<Integer>(hyperplane,(*h));
        if ((*h)[hyp_counter]>0) {
            if(first_pointed)
                (*h)[sh_1]=(*h)[0];     // the old total degree, used for the computation of new elements by degree
            (*h)[sh_2]=0;     // value of mother under the current linear form, initially 0
            (*h)[0]+=(*h)[hyp_counter];
            if((*h)[1]==0){
                no_pos_in_level0=false;
                all_positice_level=false;
            }
            // insert=h;
            // ++h;
            insert_into_Register(Pos_Register,Hilbert_Basis,h++); // elements get spliced into another list
            continue;
        }
        if ((*h)[hyp_counter]<0) {
            (*h)[hyp_counter]=-(*h)[hyp_counter];
            if(first_pointed)
                (*h)[sh_1]=(*h)[0];
            (*h)[sh_2]=0;
            (*h)[0]+=(*h)[hyp_counter];
            if((*h)[1]==0){
                all_positice_level=false;
            }
            // insert=h;
            // ++h;
            insert_into_Register(Neg_Register,Hilbert_Basis,h++);
            // cout << "Durch" << endl;
            continue;
        }
        if ((*h)[hyp_counter]==0) {
            if(first_pointed)
                (*h)[sh_1]=(*h)[0];
            (*h)[sh_2]=0;
            if((*h)[1]==0){
                no_pos_in_level0=false;
                all_positice_level=false;
            }
            // insert=h;
            // ++h;
            insert_into_Register(Neu_Register,Hilbert_Basis,h++);
        }
    }
    

    // cout << "no_pos_in_level0 = " << no_pos_in_level0 <<endl;
    //  cout << "all_positive_level = " << all_positice_level <<endl;

    if(truncate && (no_pos_in_level0 && !all_positice_level)){
        if(verbose){
            verboseOutput() << "Eliminating negative generators of level > 0" << endl;
        }
        for(size_t j=0;j<Neg_Register.size();++j)
          for (h = Neg_Register[j].begin(); h != Neg_Register[j].end();){
            if((*h)[1]>0)
                h=Neg_Register[j].erase(h);
            else
                ++h;
          }
    }

    //long int counter=0;
    list < vector <Integer> > New_Positive,New_Negative,New_Neutral,Pos,Neg,Neu;
    vector<list< vector< Integer > > > New_Positive_thread(omp_get_max_threads()),
                      New_Negative_thread(omp_get_max_threads()),
                      New_Neutral_thread(omp_get_max_threads());
    typename list < vector<Integer> >::const_iterator n;
    vector<Integer> *p;
    typename list < vector <Integer> >::iterator c;


    size_t total_degree=2; // we generate new elements with ascending total degree
                           // this is the degree of the newly generated elements
                           
    size_t reducer_degree=0;
    
    list < vector<Integer>* > Positive,Negative,Neutral; // pointer lists, used to move reducers to the front
    
    list < vector<Integer>* > Pos_Partners;
    
    size_t matches=0;
    
    size_t nr_pos_partners=0;

    while(Pos_Register.size()+Neg_Register.size()-2 >= total_degree  && !(truncate && all_positice_level)){
    
         if(verbose && total_degree%10==0)
            verboseOutput() << "total degree " << total_degree << endl;

        if(total_degree>reducer_degree){ 
            #pragma omp parallel
            {
            #pragma omp single nowait
            record_reducers(Neg_Register,Negative);

            #pragma omp single nowait
            record_reducers(Pos_Register,Positive);

            #pragma omp single nowait
            record_reducers(Neu_Register,Neutral);
            } // END PARALLEL
            
            reducer_degree=2*total_degree-1;
        }
        
           
        record_partners(Pos_Register,Pos_Partners, nr_pos_partners,total_degree-1);
        vector <Integer> new_candidate(hyp_size+3);
        typename list< vector<Integer>* >::iterator p_ptr=Pos_Partners.begin();
        
        size_t ppos=0;
        
        #pragma omp parallel for private(p,n,diff,new_candidate) firstprivate(Positive,Negative,Neutral,ppos,p_ptr)
        for(size_t ii=0;ii<nr_pos_partners;++ii){
        
            for(; ii > ppos; ++ppos, ++p_ptr) ;
            for(;ii < ppos; --ppos, --p_ptr) ;
            
            p=*p_ptr;
            size_t pos_deg=(size_t) explicit_cast_to_long<Integer>((*p)[sh_1]);
        
            // cout << "pos " << pos_deg << endl;

            // for(p=Pos_Register[pos_deg].begin();  p!=Pos_Register[pos_deg].end();++p)
            
                
                size_t compl_deg=total_degree-pos_deg;
                
                if(compl_deg>=Neg_Register.size())
                    continue;

                for(n=Neg_Register[compl_deg].begin();  n!=Neg_Register[compl_deg].end(); ++n){
                
                    matches++;

                    if(truncate && (*p)[1]+(*n)[1]>=2) //
                    continue;
                    
                    /* cout << "--------------------" << endl;
                    cout << *p;
                    cout << *n; */

                    if (((*p)[sh_2]!=0&&(*p)[sh_2]<=(*n)[hyp_counter])  // comparing with value of mother
                                    || ((*n)[sh_2]!=0&&(*n)[sh_2]<=(*p)[hyp_counter])){
                        #pragma omp atomic
                        counter1++;                                    
                        continue;
                    }
                        
                    #pragma omp atomic
                    counter++;

                    diff=(*p)[hyp_counter]-(*n)[hyp_counter];
                    new_candidate=v_add((*p),(*n));

                    if (diff>0) {
                        new_candidate[hyp_counter]=diff;
                        new_candidate[0]-=2*(*n)[hyp_counter];
                        if(!(truncate && no_pos_in_level0) && (reducible_pointed(Positive,new_candidate,hyp_counter) ||
                                    reducible_pointed(Neutral,new_candidate,hyp_counter-1)) ) {
                            continue;
                        }
                        // new_candidate[sh_1]=2; // generation no longer needed
                        new_candidate[sh_2]=(*p)[hyp_counter];
                        New_Positive_thread[omp_get_thread_num()].push_back(new_candidate);
                    }
                    if (diff<0) {
                        if(truncate && no_pos_in_level0) // don't need new negative elements anymore
                            continue;
                        new_candidate[hyp_counter]=-diff;
                        new_candidate[0]-=2*(*p)[hyp_counter];
                        if(reducible_pointed(Negative,new_candidate,hyp_counter)) {
                            continue;
                        }
                        if(reducible_pointed(Neutral,new_candidate,hyp_counter-1)) {
                            continue;
                        }
                        // new_candidate[sh_1]=2;
                        new_candidate[sh_2]=(*n)[hyp_counter];
                        New_Negative_thread[omp_get_thread_num()].push_back(new_candidate);
                    }
                    if (diff==0) {
                        new_candidate[hyp_counter]=0;
                        new_candidate[0]-=2*(*p)[hyp_counter];
                        if(!(truncate && no_pos_in_level0) && reducible_pointed(Neutral,new_candidate,hyp_counter-1)) {
                            continue;
                        }
                        // new_candidate[sh_1]=0;
                        new_candidate[sh_2]=0;
                        New_Neutral_thread[omp_get_thread_num()].push_back(new_candidate);
                    }
                }
            
        }
        //end generation of new elements
        

        #pragma omp parallel
        {
        #pragma omp single nowait
        {
        splice_them(New_Neutral,New_Neutral_thread);
        New_Neutral.sort();
        unique_vectors(New_Neutral);
        insert_all_into_Register(Neu_Register,New_Neutral);
        }
        #pragma omp single nowait
        {
        splice_them(New_Positive,New_Positive_thread);
        New_Positive.sort();
        unique_vectors(New_Positive);
        insert_all_into_Register(Pos_Register, New_Positive);
        }
        #pragma omp single nowait
        {
        splice_them(New_Negative,New_Negative_thread);
        New_Negative.sort();
        unique_vectors(New_Negative);
        insert_all_into_Register(Neg_Register,New_Negative);
        }
        } // END PARALLEL


        total_degree++;   //

     }  // loop total_deg


    // Hilbert_Basis.clear();
    
    size_t np=0;size_t nn=0;
    for (size_t i=0;i<Pos_Register.size();++i)
        np+=Pos_Register[i].size();
    for (size_t i=0;i<Neg_Register.size();++i)
        nn+=Neg_Register[i].size();
        
    
    size_t max_size=max(Pos_Register.size(),Neu_Register.size());
    for(size_t i=0;i<max_size;++i){
        if(i<Pos_Register.size())            
            Hilbert_Basis.splice(Hilbert_Basis.end(),Pos_Register[i]);            
        if(i<Neu_Register.size())
            Hilbert_Basis.splice(Hilbert_Basis.end(),Neu_Register[i]);
    }


    // (Matrix<Integer>(Hilbert_Basis)).print(cout);

    if (verbose) {
        verboseOutput()<<"Hilbert basis size="<<Hilbert_Basis.size()<<endl;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Cone_Dual_Mode<Integer>::cut_with_halfspace(const size_t& hyp_counter, const Matrix<Integer>& Basis_Max_Subspace){
    size_t i,j,rank_subspace=Basis_Max_Subspace.nr_of_rows();
    // cout << "Dim Unterraum vorher" << Basis_Max_Subspace.nr_of_rows() << endl;
    vector <Integer> scalar_product,hyperplane=SupportHyperplanes.read(hyp_counter-1),old_lin_subspace_half;
    bool lifting=false;
    Matrix<Integer> New_Basis_Max_Subspace=Basis_Max_Subspace;
    if (rank_subspace!=0) {
        scalar_product=Basis_Max_Subspace.MxV(hyperplane);
        for (i = 0; i <rank_subspace; i++)
            if (scalar_product[i]!=0)
                break;
        if (i!=rank_subspace) {    // the new hyperplane does not contain the intersection of the previous hyperplanes
                                   // so we must intersect the new hyperplane and Max_Subspace
            lifting=true;
            //computing new maximal subspace
            Matrix<Integer> M(1,rank_subspace); // this is the restriction of the new linear form to Max_Subspace
            M[0]=scalar_product;
 
            Lineare_Transformation<Integer> LT=Transformation(M);
            Matrix<Integer> Lifted_Basis_Factor_Space_over_Ker_and_Ker=LT.get_right();
            // the coordinate transfprmation yields a splitting of Max_Subspace into the direct sum of the kernel
            // of the linear form (columns 1^,..) and a complementary 1-dimensional space (column 0)
            // First we dualize from columns to rows:
            Lifted_Basis_Factor_Space_over_Ker_and_Ker=Lifted_Basis_Factor_Space_over_Ker_and_Ker.transpose();

            // Now we must embed the subspaces of Max_Subspace into the full ambient space
            // First the new maximal subspace
            Matrix<Integer>  Ker(rank_subspace-1,rank_subspace);
            for (j = 0; j < rank_subspace-1; j++) {
                Ker.write(j, Lifted_Basis_Factor_Space_over_Ker_and_Ker.read(j+1));
            }
            New_Basis_Max_Subspace=Ker.multiplication(Basis_Max_Subspace);
                        // and then the complementary 1-dim space
            // old_lin_subspace_half refers to the fact that the complementary space is subdivided into
            // two halfspaces generated by old_lin_subspace_half and -old_lin_subspace_half (taken care of in cut_with_halfspace_hilbert_basis
            old_lin_subspace_half=Basis_Max_Subspace.VxM(Lifted_Basis_Factor_Space_over_Ker_and_Ker.read(0));
        }
    }
    bool pointed=(Basis_Max_Subspace.nr_of_rows()==0);
    // if(Basis_Max_Subspace.nr_of_rows()>0){
        cut_with_halfspace_hilbert_basis(hyp_counter, lifting,old_lin_subspace_half,pointed);
        // cout << "Dim Unterraum nachher " << New_Basis_Max_Subspace.nr_of_rows() << endl;
    /*}
    else{
        cut_with_halfspace_hilbert_basis_pointed(hyp_counter); // not used at present
        first_pointed=false;
    }*/
    return New_Basis_Max_Subspace;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::extreme_rays_rank(){
    if (verbose) {
        verboseOutput() << "Find extreme rays" << endl;
    }
    
    typename list < vector <Integer> >::iterator c;
    list <key_t> zero_list;
    size_t i,j,k;
    for (c=Hilbert_Basis.begin(); c!=Hilbert_Basis.end(); ++c){
        zero_list.clear();
        for (i = 0; i < nr_sh; i++) {
            if ((*c)[i+1]==0) {
                zero_list.push_back(i);
            }
        }
        k=zero_list.size();
        if (k>=dim-1) {
            vector <key_t> zero_vector(k);
            for (j = 0; j < k; j++) {
                zero_vector[j]=zero_list.front();
                zero_list.pop_front();
            }
            Matrix<Integer> Test=SupportHyperplanes.submatrix(zero_vector);
            if (Test.rank()>=dim-1) {
                GeneratorList.push_back((*c));
            }
        }
    }
    size_t s = GeneratorList.size();
    Generators = Matrix<Integer>(s,dim);
   
    typename  list< vector<Integer> >::const_iterator l;
    for (i=0, l=GeneratorList.begin(); l != GeneratorList.end(); ++l, ++i) {
        Generators[i]= v_cut_front(*l, dim);
    ExtremeRays=vector<bool>(s,true);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::hilbert_basis_dual(){
    if(dim>0){            //correction needed to include the 0 cone;
        if (verbose==true) {
            verboseOutput()<<"\n************************************************************\n";
            verboseOutput()<<"computing Hilbert basis ..."<<endl;
        }
        
        if(Generators.nr_of_rows()!=ExtremeRays.size()){
            errorOutput() << "Mismatch of extreme rays and generators in cone dual mode. THIS SHOULD NOT HAPPEN." << endl;
            throw FatalException(); 
        }
        
        size_t hyp_counter;      // current hyperplane
        Matrix<Integer> Basis_Max_Subspace(dim);      //identity matrix
        for (hyp_counter = 1; hyp_counter <= nr_sh; hyp_counter++) {
            Basis_Max_Subspace=cut_with_halfspace(hyp_counter,Basis_Max_Subspace);
        }
        if(ExtremeRays.size()==0){  // no precomputed generators
            extreme_rays_rank();
            relevant_support_hyperplanes();
            GeneratorList.clear();
            
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
            
        l_cut_front(Hilbert_Basis,dim);
        if(verbose)
            verboseOutput() << "matches = " << counter << endl << "avoided = " << counter1 << endl;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::relevant_support_hyperplanes(){
    if (verbose) {
        verboseOutput() << "Find relevant support hyperplanes" << endl;
    }
    list <key_t> zero_list;
    typename list<vector<Integer> >::iterator gen_it;
    vector <key_t> relevant_sh;
    relevant_sh.reserve(nr_sh);
    size_t i,k;
    
    size_t realdim = Generators.rank();

    for (i = 0; i < nr_sh; ++i) {
        Matrix<Integer> Test(0,dim);
        k = 0;
        for (gen_it = GeneratorList.begin(); gen_it != GeneratorList.end(); ++gen_it) {
            if ((*gen_it)[i+1]==0) {
                Test.append( v_cut_front(*gen_it,dim) );
                k++;
            }
        }
        if (k >= realdim-1 && Test.rank_destructive()>=realdim-1) {
            relevant_sh.push_back(i);
        }
    }
    SupportHyperplanes = SupportHyperplanes.submatrix(relevant_sh);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::to_sublattice(Sublattice_Representation<Integer> SR) {
    assert(SR.get_dim() == dim);

    dim = SR.get_rank();
    hyp_size = dim+nr_sh;
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
