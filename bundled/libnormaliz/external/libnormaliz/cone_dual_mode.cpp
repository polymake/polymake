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
void Cone_Dual_Mode<Integer>::splice_them(list< vector< Integer > >& Total, vector<list< vector< Integer > > >& Parts){

    for(int i=0;i<omp_get_max_threads();i++)
        Total.splice(Total.end(),Parts[i]);
}

//---------------------------------------------------------------------------


template<typename Integer>
void Cone_Dual_Mode<Integer>::record_order(list< vector< Integer > >& Elements, list< vector< Integer >* >& Order){

    Order.clear();
    typename list < vector<Integer> >::iterator it=Elements.begin();
    for(;it!=Elements.end();++it) {
            Order.push_back(&(*it));
    }
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
        if (ordered && new_element[0]<2*(*reducer)[0]) {
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
        if (new_element[0]<2*(*j)[0]) {
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
void Cone_Dual_Mode<Integer>::auto_reduce(list< vector< Integer> >& To_Reduce, const size_t& size){

        To_Reduce.sort();
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
        errorOutput()<<"Cone_Dual_Mode error: Matrix<Integer> with rank = number of columns needed in the constructor of the object Cone_Dual_Mode<Integer>.\nProbable reason: The Cone is not pointed!"<<endl;
        M.pretty_print(errorOutput());
        throw BadInputException();
    }
    SupportHyperplanes = M;
    nr_sh=SupportHyperplanes.nr_of_rows();
    if (nr_sh != static_cast<size_t>(static_cast<key_t>(nr_sh))) {
        errorOutput()<<"To many support hyperplanes to fit in range of key_t!"<<endl;
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
    GeneratorList = list< vector<Integer> >();
    Hilbert_Basis = list< vector<Integer> >();
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

//---------------------------------------------------------------------------

template<typename Integer>
void Cone_Dual_Mode<Integer>::cut_with_halfspace_hilbert_basis(const size_t& hyp_counter, const bool& lifting, vector<Integer>& halfspace){
    if (verbose==true) {
        verboseOutput()<<"cut with halfspace "<<hyp_counter<<" ..."<<endl;
    }
    size_t i;
    int sign;
    bool not_done;
    list < vector<Integer> > Positive_Irred,Negative_Irred,Neutral_Irred;
    Integer orientation, scalar_product,diff,factor;
    vector <Integer> hyperplane=SupportHyperplanes.read(hyp_counter-1);
    typename list< vector<Integer> >::iterator h;
    if (lifting==true) {
        orientation=v_scalar_product<Integer>(hyperplane,halfspace);
        if(orientation<0){
            orientation=-orientation;
            v_scalar_multiplication<Integer>(halfspace,-1); //transforming into the generator of the positive halfspace
        }
        for (h = Hilbert_Basis.begin(); h != Hilbert_Basis.end(); ++h) { //reduction  modulo  the generator of the positive halfspace
            scalar_product=v_scalar_product_unequal_vectors_end(hyperplane,(*h));
            sign=1;
            if (scalar_product<0) {
                scalar_product=-scalar_product;
                sign=-1;
            }
            factor=scalar_product/orientation;
            for (i = 0; i < dim; i++) {
                (*h)[nr_sh+3+i]=(*h)[nr_sh+3+i]-sign*factor*halfspace[i];
            }
        }
        //adding the generators of the halfspace to negative and positive
        vector <Integer> hyp_element(hyp_size+3,0);
        for (i = 0; i < dim; i++) {
            hyp_element[nr_sh+3+i]= halfspace[i];
        }
        hyp_element[hyp_counter]=orientation;
        hyp_element[0]=orientation;
        if (orientation==0){ //never
            Neutral_Irred.push_back(hyp_element);
        }
        else{
            Positive_Irred.push_back(hyp_element);
            v_scalar_multiplication<Integer>(hyp_element,-1);
            hyp_element[hyp_counter]=orientation;
            hyp_element[0]=orientation;
            Negative_Irred.push_back(hyp_element);
        }
    } //end lifting
    for (h = Hilbert_Basis.begin(); h != Hilbert_Basis.end(); ++h) { //dividing into negative and positive
        (*h)[hyp_counter]=v_scalar_product_unequal_vectors_end<Integer>(hyperplane,(*h));
        if ((*h)[hyp_counter]>0) {
            (*h)[nr_sh+1]=1;     // generation
            (*h)[nr_sh+2]=0;     //not sum
            (*h)[0]+=(*h)[hyp_counter];
            Positive_Irred.push_back((*h));
        }
        if ((*h)[hyp_counter]<0) {
            (*h)[hyp_counter]=-(*h)[hyp_counter];
            (*h)[nr_sh+1]=1;
            (*h)[nr_sh+2]=0;
            (*h)[0]+=(*h)[hyp_counter];
            Negative_Irred.push_back((*h));
        }
        if ((*h)[hyp_counter]==0) {
            (*h)[nr_sh+1]=0;
            (*h)[nr_sh+2]=0;
            Neutral_Irred.push_back((*h));
        }
    }
    Neutral_Irred.sort();
    Positive_Irred.sort();
    Negative_Irred.sort();
    //long int counter=0;
    list < vector <Integer> > New_Positive,New_Negative,New_Neutral,Pos,Neg,Neu;
    vector<list< vector< Integer > > > New_Positive_thread(omp_get_max_threads()),
                      New_Negative_thread(omp_get_max_threads()),
                      New_Neutral_thread(omp_get_max_threads());
    typename list < vector<Integer> >::const_iterator p,n;
    typename list < vector <Integer> >::iterator c;
    not_done=true;
    while(not_done){
        not_done=false;
        New_Positive.clear();
        New_Negative.clear();
        New_Neutral.clear();
        
        //generating new elements
        
        list < vector<Integer>* > Positive,Negative,Neutral; // pointer lists, used to move reducers to the front
        size_t psize=0;
        #pragma omp parallel
        {
        #pragma omp single nowait
        record_order(Negative_Irred,Negative);
        
        #pragma omp single nowait
        record_order(Positive_Irred,Positive);
        #pragma omp single nowait
        psize=Positive_Irred.size();
        
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
        size_t ppos=0;
        p = Positive_Irred.begin();
        #pragma omp for schedule(dynamic)
        for(i = 0; i<psize; ++i){
            for(;i > ppos; ++ppos, ++p) ;
            for(;i < ppos; --ppos, --p) ;

            for (n = Negative_Irred.begin(); n != Negative_Irred.end(); ++n){
                if ((*p)[nr_sh+1]<=1 && (*n)[nr_sh+1]<=1
                && ((*p)[nr_sh+1]!=0 || (*n)[nr_sh+1]!=0)) {
                    if (((*p)[nr_sh+2]!=0&&(*p)[nr_sh+2]<=(*n)[hyp_counter])
                    || ((*n)[nr_sh+2]!=0&&(*n)[nr_sh+2]<=(*p)[hyp_counter]))
                        continue;
                    //  counter++;
                    diff=(*p)[hyp_counter]-(*n)[hyp_counter];
                    vector <Integer> new_candidate=v_add((*p),(*n));

                    if (diff>0) {
                        new_candidate[hyp_counter]=diff;
                        new_candidate[0]-=2*(*n)[hyp_counter];
                        if(reducible(Positive,new_candidate,hyp_counter,false)) {
                            continue;
                        }
                        if(reducible(Neutral,new_candidate,hyp_counter-1,false)) {
                            continue;
                        }
                        new_candidate[nr_sh+1]=2;
                        new_candidate[nr_sh+2]=(*p)[hyp_counter];
                        // #pragma omp critical(NEW_POSITIVE)
                        New_Positive_thread[omp_get_thread_num()].push_back(new_candidate);
                    }
                    if (diff<0) {
                        new_candidate[hyp_counter]=-diff;
                        new_candidate[0]-=2*(*p)[hyp_counter];
                        if(reducible(Negative,new_candidate,hyp_counter,false)) {
                            continue;
                        }
                        if(reducible(Neutral,new_candidate,hyp_counter-1,false)) {
                            continue;
                        }
                        new_candidate[nr_sh+1]=2;
                        new_candidate[nr_sh+2]=(*n)[hyp_counter];
                        // #pragma omp critical(NEW_NEGATIVE)
                        New_Negative_thread[omp_get_thread_num()].push_back(new_candidate);
                    }
                    if (diff==0) {
                        new_candidate[hyp_counter]=0;
                        new_candidate[0]-=2*(*p)[hyp_counter];
                        if(reducible(Neutral,new_candidate,hyp_counter-1,false)) {
                            continue;
                        }
                        new_candidate[nr_sh+1]=0;
                        new_candidate[nr_sh+2]=0;
                        #pragma omp critical(NEW_NEUTRAL)
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
        auto_reduce(New_Neutral,hyp_counter-1);
        }
        #pragma omp single nowait
        {
        splice_them(New_Positive,New_Positive_thread);
        auto_reduce(New_Positive,hyp_counter-1);
        }
        #pragma omp single nowait
        {
        splice_them(New_Negative,New_Negative_thread);
        auto_reduce(New_Negative,hyp_counter-1);
        }
        } // END PARALLEL

        
        if (New_Neutral.size()!=0) {
            #pragma omp parallel
            {
            #pragma omp single nowait
            reduce(New_Neutral,New_Positive, hyp_counter-1);
            #pragma omp single nowait
            reduce(New_Neutral,New_Negative, hyp_counter-1);
            #pragma omp single nowait
            reduce(New_Neutral,Neutral_Irred, hyp_counter-1);
            #pragma omp single nowait
            reduce(New_Neutral,Positive_Irred, hyp_counter-1);
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
            reduce(New_Positive,Positive_Irred, hyp_counter);
            Positive_Irred.merge(New_Positive);
        }
        #pragma omp single nowait
        if (New_Negative.size()!=0) {
            not_done=true;
            reduce(New_Negative,Negative_Irred, hyp_counter);
            Negative_Irred.merge(New_Negative);
        }
        } // PARALLEL
        
        // adjust generation

        #pragma omp parallel
        {
        #pragma omp single nowait
        for (c = Positive_Irred.begin(); c != Positive_Irred.end(); ++c){
            if((*c)[nr_sh+1]>0) {
                (*c)[nr_sh+1]--;
            }
        }
        #pragma omp single nowait
        for (typename list < vector <Integer> >::iterator c2 = Negative_Irred.begin(); c2 != Negative_Irred.end(); ++c2){
            if((*c2)[nr_sh+1]>0) {
                (*c2)[nr_sh+1]--;
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
        (*c)[nr_sh+1]=0;
        (*c)[nr_sh+2]=0;
    }
    Hilbert_Basis.sort();
    Hilbert_Basis.unique(); 

    if (verbose) {
        verboseOutput()<<"Hilbert basis size="<<Hilbert_Basis.size()<<endl;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Cone_Dual_Mode<Integer>::cut_with_halfspace(const size_t& hyp_counter, const Matrix<Integer>& Basis_Max_Subspace){
    size_t i,j,rank_subspace=Basis_Max_Subspace.nr_of_rows();
    vector <Integer> scalar_product,hyperplane=SupportHyperplanes.read(hyp_counter-1),halfspace;
    bool lifting=false;
    Matrix<Integer> New_Basis_Max_Subspace=Basis_Max_Subspace;
    if (rank_subspace!=0) {
        scalar_product=Basis_Max_Subspace.MxV(hyperplane);
        for (i = 0; i <rank_subspace; i++)
            if (scalar_product[i]!=0)
                break;
        if (i!=rank_subspace) {    // the new hyperplane is not contained in the maximal subspace
            lifting=true;
            //computing new maximal subspace
            Matrix<Integer> M(1,rank_subspace);
            M.write(0,scalar_product);
            Lineare_Transformation<Integer> LT=Transformation(M);
            Matrix<Integer> Lifted_Basis_Factor_Space_over_Ker_and_Ker=LT.get_right();
            Lifted_Basis_Factor_Space_over_Ker_and_Ker=Lifted_Basis_Factor_Space_over_Ker_and_Ker.transpose();
            Matrix<Integer>  Ker(rank_subspace-1,rank_subspace);
            for (j = 0; j < rank_subspace-1; j++) {
                Ker.write(j, Lifted_Basis_Factor_Space_over_Ker_and_Ker.read(j+1));
            }
            New_Basis_Max_Subspace=Ker.multiplication(Basis_Max_Subspace);
            halfspace=Basis_Max_Subspace.VxM(Lifted_Basis_Factor_Space_over_Ker_and_Ker.read(0));
        }
    }
    cut_with_halfspace_hilbert_basis(hyp_counter, lifting, halfspace);
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
        Generators.write( i, v_cut_front(*l, dim) );
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
        size_t hyp_counter;      // current hyperplane
        Matrix<Integer> Basis_Max_Subspace(dim);      //identity matrix
        for (hyp_counter = 1; hyp_counter <= nr_sh; hyp_counter++) {
            Basis_Max_Subspace=cut_with_halfspace(hyp_counter,Basis_Max_Subspace);
        }
        extreme_rays_rank();
        l_cut_front(Hilbert_Basis,dim);

        relevant_support_hyperplanes();
        GeneratorList.clear();
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
