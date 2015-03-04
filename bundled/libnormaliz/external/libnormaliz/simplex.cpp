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

#include <algorithm>
#include <string>
#include <iostream>
#include <set>
#include <deque>

#include "integer.h"
#include "vector_operations.h"
#include "matrix.h"
#include "simplex.h"
#include "list_operations.h"
#include "HilbertSeries.h"
#include "cone.h"
#include "my_omp.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------

template<typename Integer>
Simplex<Integer>::Simplex(const Matrix<Integer>& Map){
    dim=Map.nr_of_columns();
    key=Map.max_rank_submatrix_lex();
    Generators=Map.submatrix(key);
    diagonal = vector< Integer >(dim);
    Support_Hyperplanes=invert(Generators, diagonal, volume); //test for arithmetic
    //overflow performed
    v_abs(diagonal);
    Support_Hyperplanes = Support_Hyperplanes.transpose();
    multiplicators = Support_Hyperplanes.make_prime();
}

//---------------------------------------------------------------------------

template<typename Integer>
Simplex<Integer>::Simplex(const vector<key_t>& k, const Matrix<Integer>& Map){
    key=k;
    Generators=Map.submatrix(k);
    dim=k.size();
    diagonal = vector< Integer >(dim);
    Support_Hyperplanes=invert(Generators, diagonal, volume);  //test for arithmetic
    //overflow performed
    v_abs(diagonal);
    Support_Hyperplanes=Support_Hyperplanes.transpose();
    multiplicators=Support_Hyperplanes.make_prime();
}

//---------------------------------------------------------------------------

template<typename Integer>
size_t Simplex<Integer>::read_dimension() const{
    return dim;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Simplex<Integer>::write_volume(const Integer& vol){
    volume=vol;
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer Simplex<Integer>::read_volume() const{
    return volume;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<key_t> Simplex<Integer>::read_key() const{
    return key;
}

//---------------------------------------------------------------------------

template<typename Integer>
Matrix<Integer> Simplex<Integer>::read_generators() const{
    return Generators;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Simplex<Integer>::read_diagonal() const{
    return diagonal;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> Simplex<Integer>::read_multiplicators() const{
    return multiplicators;
}

//---------------------------------------------------------------------------


template<typename Integer>
Matrix<Integer> Simplex<Integer>::read_support_hyperplanes() const{
    return Support_Hyperplanes;
}

//---------------------------------------------------------------------------
// SimplexEvaluator
//---------------------------------------------------------------------------

template<typename Integer>
SimplexEvaluator<Integer>::SimplexEvaluator(Full_Cone<Integer>& fc)
: C_ptr(&fc),
  dim(fc.dim),
  // det_sum(0),
  // mult_sum(0),
  key(dim),
  // candidates_size(0),
  // collected_elements_size(0),
  Generators(dim,dim),
  TGenerators(dim,dim),
  GenCopy(dim,dim),
  InvGenSelRows(dim,dim),
  InvGenSelCols(dim,dim),
  Sol(dim,dim+1),
  // ProjGen(dim-fc.level0_dim,dim-fc.level0_dim),
  InvSol(dim,dim+1),
  GDiag(dim),
  TDiag(dim),
  Excluded(dim),
  Indicator(dim),
  gen_degrees(dim),
  gen_levels(dim),
  RS(dim,1),
  InExSimplData(C_ptr->InExCollect.size())
{
    size_t hv_max=0;
    if (C_ptr->do_h_vector) {
        // we need the generators to be sorted by degree
        for (size_t i=C_ptr->nr_gen-dim; i<C_ptr->nr_gen; i++)
            hv_max += C_ptr->gen_degrees[i];
        if (hv_max > 1000000) {
            errorOutput() << "Error: generator degrees are to huge, h-vector would contain more than 10^6 entires." << endl;
            throw BadInputException();
        }
        // hvector.resize(hv_max);
        // inhom_hvector.resize(hv_max);
    }
    
    if(fc.inhomogeneous)
        ProjGen=Matrix<Integer>(dim-fc.level0_dim,dim-fc.level0_dim);    
    
    level0_gen_degrees.reserve(fc.dim);
    
    for(size_t i=0;i<fc.InExCollect.size();++i){
        InExSimplData[i].GenInFace.resize(fc.dim);
        // InExSimplData[i].hvector.resize(hv_max);
        InExSimplData[i].gen_degrees.reserve(fc.dim);
    }
    
    full_cone_simplicial=(C_ptr->nr_gen==C_ptr->dim);
    sequential_evaluation=true; // to be changed later if necessrary
}

template<typename Integer>
void SimplexEvaluator<Integer>::set_evaluator_tn(int threadnum){
    tn=threadnum;   
}

//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::add_to_inex_faces(const vector<Integer> offset, size_t Deg, Collector<Integer>& Coll){

    for(size_t i=0;i<nrInExSimplData;++i){
        bool in_face=true;
        for(size_t j=0;j<dim;++j)
            if((offset[j]!=0) && !InExSimplData[i].GenInFace.test(j)){  //  || Excluded[j] superfluous
                in_face=false;
                break;
            }
        if(!in_face)
            continue;
        Coll.InEx_hvector[i][Deg]+=InExSimplData[i].mult;            
    }
    
}

//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::prepare_inclusion_exclusion_simpl(size_t Deg, Collector<Integer>& Coll) {
     
     Full_Cone<Integer>& C = *C_ptr;
     // map<boost::dynamic_bitset<>, long> InExSimpl;      // local version of nExCollect   
     map<boost::dynamic_bitset<>, long>::iterator F;
     
     nrInExSimplData=0;
     
     for(F=C.InExCollect.begin();F!=C.InExCollect.end();++F){
        bool still_active=true;
        for(size_t i=0;i<dim;++i)
            if(Excluded[i] && !F->first.test(key[i])){
                still_active=false;
                break;
            }
        if(!still_active)
            continue;
        InExSimplData[nrInExSimplData].GenInFace.reset();
        for(size_t i=0;i<dim;++i)
            if(F->first.test(key[i]))
                InExSimplData[nrInExSimplData].GenInFace.set(i);
        InExSimplData[nrInExSimplData].gen_degrees.clear();
        for(size_t i=0;i<dim;++i)
            if(InExSimplData[nrInExSimplData].GenInFace.test(i))
                InExSimplData[nrInExSimplData].gen_degrees.push_back(gen_degrees[i]);
        InExSimplData[nrInExSimplData].mult=F->second;
        nrInExSimplData++;  
     }
     
     if(C_ptr->do_h_vector){
        vector<Integer> ZeroV(dim,0);               // allowed since we have only kept faces that contain 0+offset
        add_to_inex_faces(ZeroV,Deg,Coll);          // nothing would change if we took 0+offset here
     }
     
}

//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::update_inhom_hvector(long level_offset, size_t Deg, Collector<Integer>& Coll){

    // cout << "*** " << level_offset << " " << Deg << endl;

    if(level_offset==1){
        Coll.inhom_hvector[Deg-1]++;
        return;
    }
    
    size_t Deg_i;
    
    assert(level_offset==0);
    
    for(size_t i=0;i<dim;++i){
        // cout << "+++ " << gen_levels[i] << " " << gen_degrees[i] << endl;
        if(gen_levels[i]==1){
            Deg_i=Deg+gen_degrees[i];
            Coll.inhom_hvector[Deg_i-1]++;
        }
    }
    // cout << "------ " << Coll.inhom_hvector << endl;
    // cout << level0_gen_degrees;
}

//---------------------------------------------------------------------------

size_t Unimod=0, Ht1NonUni=0, Gcd1NonUni=0, NonDecided=0, NonDecidedHyp=0;
size_t TotDet=0;

//---------------------------------------------------------------------------

template<typename Integer>
Integer SimplexEvaluator<Integer>::start_evaluation(SHORTSIMPLEX<Integer>& s, Collector<Integer>& Coll) {

    volume = s.vol;
    key = s.key;
    Full_Cone<Integer>& C = *C_ptr;

    bool do_only_multiplicity =
        C.do_only_multiplicity;
//        || (s.height==1 && C.do_partial_triangulation);

    size_t i,j;


    //degrees of the generators according to the Grading of C
    if(C.isComputed(ConeProperty::Grading))
        for (i=0; i<dim; i++)
            gen_degrees[i] = C.gen_degrees[key[i]];
            
    nr_level0_gens=0;
    level0_gen_degrees.clear();
    
    if(C.inhomogeneous){
        for (i=0; i<dim; i++){
            gen_levels[i] = C.gen_levels[key[i]];
            if(gen_levels[i]==0){
                nr_level0_gens++;
                level0_gen_degrees.push_back(gen_degrees[i]);
            }
        }
    }
    

    if(do_only_multiplicity){
        if(volume == 0) { // not known in advance
            for(i=0; i<dim; ++i)
                Generators[i] = C.Generators[key[i]];
            volume=Generators.vol_destructive();
            #pragma omp atomic
            TotDet++;
        }
        addMult(volume,Coll);
        return volume;
    }  // done if only mult is asked for
    
    for(i=0; i<dim; ++i)
        Generators[i] = C.Generators[key[i]];

    bool unimodular=false;
    bool GDiag_computed=false;
    bool potentially_unimodular=(s.height==1);

    if(potentially_unimodular && C.isComputed(ConeProperty::Grading)){
        long g=0;
        for(i=0;i<dim;++i){
            g=gcd(g,gen_degrees[i]);
            if(g==1)
                break;
        }
        potentially_unimodular=(g==1);
    }

    if(potentially_unimodular){ // very likely unimodular, Indicator computed first, uses transpose of Gen
        for(i=0; i<dim; ++i)
            TGenerators.write_column(i,C.Generators[key[i]]);
        RS.write_column(0,C.Order_Vector);  // right hand side
        TGenerators.solve_destructive_Sol(RS,TDiag,volume,Sol);
        for (i=0; i<dim; i++)
            Indicator[i]=Sol[i][0];
        if(volume==1){
            unimodular=true;
            #pragma omp atomic
            Unimod++;
            for(i=0;i<dim;i++)
                GDiag[i]=1;
            GDiag_computed=true;
        }
        else
            #pragma omp atomic
            Ht1NonUni++;
    }


    // we need the GDiag if not unimodular (to be computed from Gen)
    // if potentially unimodular, we combine its computation with that of the i-th support forms for Ind[i]==0
    // stored in InvSol (transferred to InvGenSelCols later)
    // if unimodular and all Ind[i] !=0, then nothing is done here

    vector<key_t> Ind0_key;  //contains the indices i as above
    Ind0_key.reserve(dim-1);

    if(potentially_unimodular)
        for(i=0;i<dim;i++)
            if(Indicator[i]==0)
                Ind0_key.push_back(i);
    if(!unimodular || Ind0_key.size()>0){
        for(i=0; i<dim; ++i)  // (uses Gen)
            Generators[i] = C.Generators[key[i]];
        if(!unimodular)
            GenCopy=Generators;
        if(Ind0_key.size()>0){
            Matrix<Integer> RSmult(dim,Ind0_key.size());
            for(i=0;i<Ind0_key.size();i++) // insert unit vectors
                RSmult[Ind0_key[i]][i]=1;
            Generators.solve_destructive_Sol(RSmult,GDiag,volume,InvSol);
            v_abs(GDiag);
            GDiag_computed=true;
        }
        if(!GDiag_computed){
            Matrix<Integer> RSmult(dim,Ind0_key.size());
            Generators.solve_destructive_Sol(RSmult,GDiag,volume,InvSol);
            v_abs(GDiag);
            GDiag_computed=true;
        }
    }
    
    // take care of multiplicity unless do_only_multiplicity
    // Can't be done earlier since volume is not always known earlier


    addMult(volume,Coll);
        
    if (unimodular && !C.do_h_vector && !C.do_Stanley_dec) { // in this case done
        return volume;
    }


    // now we must compute the matrix InvGenSelRows (selected rows of InvGen)
    // for those i for which Gdiag[i]>1 combined with computation
    // of Indicator in case of potentially_unimodular==false (uses transpose of Gen)

    vector<key_t> Last_key;
    Last_key.reserve(dim);
    if (!unimodular) {
        for(i=0; i<dim; ++i) {
            TGenerators.write_column(i,C.Generators[key[i]]);
            if(GDiag[i]>1)
                Last_key.push_back(i);
        }

        size_t RScol;
        if(potentially_unimodular)
            RScol=Last_key.size();
        else
            RScol=Last_key.size()+1;
        Matrix<Integer> RSmult(dim,RScol);

        for(i=0;i<Last_key.size();i++) // insert unit vectors
            RSmult[Last_key[i]][i]=1;
        if(!potentially_unimodular) // insert order vector if necessary
            RSmult.write_column(Last_key.size(),C.Order_Vector);
        TGenerators.solve_destructive_Sol(RSmult,TDiag,volume,Sol);

        for(i=0;i<Last_key.size();i++) // write solutions as selected rows of InvDen
            for(j=0;j<dim;j++){
                InvGenSelRows[Last_key[i]][j]=Sol[j][i]%volume; //makes reduction mod volume easier
                if(InvGenSelRows[Last_key[i]][j] <0)
                    InvGenSelRows[Last_key[i]][j]+=volume;
            }
        if(!potentially_unimodular) // extract Indicator
            for (i=0; i<dim; i++)
                Indicator[i]=Sol[i][Last_key.size()];
    }


    if(!potentially_unimodular){
        for(i=0;i<dim;i++)
            if(Indicator[i]==0)
                Ind0_key.push_back(i);
        if(Ind0_key.size()>0){
            Generators=GenCopy;
            Matrix<Integer> RSmult(dim,Ind0_key.size());
            for(i=0;i<Ind0_key.size();i++) // insert unit vectors
                    RSmult[Ind0_key[i]][i]=1;
            Generators.solve_destructive_Sol(RSmult,TDiag,volume,InvSol);  // keep GDiag from above
        }
    }

    if(Ind0_key.size()>0){
        #pragma omp atomic
        NonDecided++;
        #pragma omp atomic
        NonDecidedHyp+=Ind0_key.size();
    }

    for(i=0;i<Ind0_key.size();i++) // insert selected columns of InvGen at right place
        for(j=0;j<dim;j++){
            InvGenSelCols[j][Ind0_key[i]]=InvSol[j][i];
        }
   
    
    return(volume);
    
}

//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::take_care_of_0vector(Collector<Integer>& Coll){

    size_t i,j;
    Integer Test;
    size_t Deg0_offset=0;
    long level_offset=0; // level_offset is the level of the lement in par + its offset in the Stanley dec
    for(i=0;i<dim;i++)
        Excluded[i]=false;
    for(i=0;i<dim;i++){ // excluded facets and degree shift for 0-vector
        Test=Indicator[i];
        if(Test<0)
        {
            Excluded[i]=true; // the facet opposite to vertex i is excluded
            if(C_ptr->do_h_vector){
                Deg0_offset += gen_degrees[i];
                if(C_ptr->inhomogeneous)
                    level_offset+=gen_levels[i];                    
            }
        }
        if(Test==0){  // Order_Vector in facet, now lexicographic decision
            for(j=0;j<dim;j++){
                if(InvGenSelCols[j][i]<0){ // COLUMNS of InvGen give supp hyps
                    Excluded[i]=true;
                    if(C_ptr->do_h_vector){
                        Deg0_offset += gen_degrees[i];
                        if(C_ptr->inhomogeneous)
                            level_offset+=gen_levels[i];
                    }
                    break;
                }
                if(InvGenSelCols[j][i]>0) // facet included
                    break;
            }
        }
    }

    if (C_ptr->do_h_vector) {
        if(C_ptr->inhomogeneous){
            if(level_offset<=1)
                update_inhom_hvector(level_offset,Deg0_offset, Coll); // here we count 0+offset
        }
        else{
            Coll.hvector[Deg0_offset]++; // here we count 0+offset
        }
    }
    
    // cout << "--- " << Coll.inhom_hvector;
    
    if(C_ptr->do_excluded_faces)
        prepare_inclusion_exclusion_simpl(Deg0_offset, Coll);

    if(C_ptr->do_Stanley_dec){                          // prepare space for Stanley dec
        STANLEYDATA<Integer> SimplStanley;         // key + matrix of offsets
        SimplStanley.key=key;
        Matrix<Integer> offsets(explicit_cast_to_long(volume),dim);  // volume rows, dim columns
        SimplStanley.offsets=offsets;
        #pragma omp critical(STANLEY)
        {
        C_ptr->StanleyDec.push_back(SimplStanley);      // extend the Stanley dec by a new matrix
        StanleyMat= &C_ptr->StanleyDec.back().offsets;  // and use this matrix for storage
        }
        for(i=0;i<dim;++i)                   // the first vector is 0+offset
            if(Excluded[i])
                (*StanleyMat)[0][i]=volume;
    }

    StanIndex=1;  // counts the number of components in the Stanley dec. Vector at 0 already filled if necessary

}


//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::evaluate_element(const vector<Integer>& element, Collector<Integer>& Coll){

    // now we create and evaluate the points in par
    Integer norm;
    Integer normG;
    size_t i;

    Full_Cone<Integer>& C = *C_ptr;
    
    typename list <vector <Integer> >::iterator c;
        
        // now the vector in par has been produced and is in element
        
        // DON'T FORGET: THE VECTOR PRODUCED IS THE "REAL" VECTOR*VOLUME !!

        norm=0; // norm is just the sum of coefficients, = volume*degree if homogenous
                // it is used to sort the Hilbert basis candidates
        normG = 0;  // the degree according to the grading
        for (i = 0; i < dim; i++) {  // since generators have degree 1
            norm+=element[i];
            if(C.do_h_vector || C.do_deg1_elements) {
                normG += element[i]*gen_degrees[i];
            }
        }
        

        long level,level_offset=0;
        Integer level_Int=0;
        
        if(C.inhomogeneous){
            for(i=0;i<dim;i++)
                level_Int+=element[i]*gen_levels[i];
            level=explicit_cast_to_long<Integer>(level_Int/volume); // have to divide by volume; see above
            // cout << level << " ++ " << volume << " -- " << element;
            
            if(level>1) return; // ***************** nothing to do for this vector
                                  // if we sahould decide to allow Stanley dec in the inhomogeneous case, this must be changed
            
            // cout << "Habe ihn" << endl;
            
            if(C.do_h_vector){
                level_offset=level; 
                for(i=0;i<dim;i++)
                    if(element[i]==0 && Excluded[i])
                        level_offset+=gen_levels[i];
            }
        }


        size_t Deg=0;
        if(C.do_h_vector){
            Deg = explicit_cast_to_long<Integer>(normG/volume);
            for(i=0;i<dim;i++) { // take care of excluded facets and increase degree when necessary
                if(element[i]==0 && Excluded[i]) {
                    Deg += gen_degrees[i];
                }
            }

            //count point in the h-vector
            if(C.inhomogeneous && level_offset<=1)
                update_inhom_hvector(level_offset,Deg, Coll);          
            else
                Coll.hvector[Deg]++;
            
            if(C.do_excluded_faces)
                add_to_inex_faces(element,Deg,Coll);
        }

        if(C.do_Stanley_dec){
            (*StanleyMat)[StanIndex]=element;
            for(i=0;i<dim;i++)
                if(Excluded[i]&&element[i]==0)
                    (*StanleyMat)[StanIndex][i]+=volume;
            StanIndex++;
        }

            if (C.do_Hilbert_basis) {
                vector<Integer> candi = v_merge(element,norm);
                if (!is_reducible(candi, Hilbert_Basis)) {
                    Coll.Candidates.push_back(candi);
                    Coll.candidates_size++;
                    if (Coll.candidates_size >= 1000 && sequential_evaluation) {
                        local_reduction(Coll);
                    }
                }
                return;
            }
            if(C.do_deg1_elements && normG==volume && !isDuplicate(element)) {
                vector<Integer> help=GenCopy.VxM(element);
                v_scalar_division(help,volume);
                Coll.Deg1_Elements.push_back(help);
                Coll.collected_elements_size++;
            }
}


//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::conclude_evaluation(Collector<Integer>& Coll) {

    Full_Cone<Integer>& C = *C_ptr;

    if(C.do_h_vector) {
        if(C.inhomogeneous){
            Coll.Hilbert_Series.add(Coll.inhom_hvector,level0_gen_degrees);
            for (size_t i=0; i<Coll.inhom_hvector.size(); i++)
                Coll.inhom_hvector[i]=0;
            // cout << "WAU " << endl;
            }
        else{
            Coll.Hilbert_Series.add(Coll.hvector,gen_degrees);
            for (size_t i=0; i<Coll.hvector.size(); i++)
                Coll.hvector[i]=0;
            if(C.do_excluded_faces)
                for(size_t i=0;i<nrInExSimplData;++i){
                    Coll.Hilbert_Series.add(Coll.InEx_hvector[i],InExSimplData[i].gen_degrees);
                    for(size_t j=0;j<Coll.InEx_hvector[i].size();++j)
                        Coll.InEx_hvector[i][j]=0;
                    
                }
        }
    }
    
    // cout << Coll.Hilbert_Series << endl;


    if(volume==1 || !C.do_Hilbert_basis || !sequential_evaluation)
        return;  // no further action in this case

    // cout << "Starting local reduction" << endl;
        
    local_reduction(Coll);

    // cout << "local HB " << Hilbert_Basis.size() << endl;
    
    //inverse transformation and reduction against global reducers
    //some test for arithmetic overflow may be implemented here
    bool inserted;
    typename list< vector<Integer> >::iterator jj = Hilbert_Basis.begin();
    for(;jj != Hilbert_Basis.end();++jj) {
        if (!isDuplicate(*jj)) { //skip the element
            jj->pop_back(); //remove the norm entry at the end
            
            // transform to global coordinates 
            *jj = GenCopy.VxM(*jj);
            v_scalar_division(*jj,volume);
            
            // reduce against global reducers in C.OldCandidates and insert into HB_Elements
            if(full_cone_simplicial){ // no global reduction necessary
                Coll.HB_Elements.Candidates.push_back(Candidate<Integer>(*jj,C));
                inserted=true;
            }
            else         
                inserted=Coll.HB_Elements.reduce_by_and_insert(*jj,C,C.OldCandidates);
            if(inserted)
                Coll.collected_elements_size++;
        }
    }
	// cout << "local reduction finished " << Coll.collected_elements_size << endl;
    

    Hilbert_Basis.clear(); // this is not a local variable !!    
}

//---------------------------------------------------------------------------


const long SimplexParallelEvaluationBound=10000000; // larger simplices are evaluated by parallel threads

//---------------------------------------------------------------------------


/* evaluates a simplex in regard to all data in a single thread*/
template<typename Integer>
bool SimplexEvaluator<Integer>::evaluate(SHORTSIMPLEX<Integer>& s) {

    start_evaluation(s,C_ptr->Results[tn]);
    s.vol=volume;
    if(C_ptr->do_only_multiplicity)
        return true;
    if(volume>SimplexParallelEvaluationBound && !C_ptr->do_Stanley_dec && omp_get_max_threads()>1) // to be postponed for parallel evaluation
        return false;
    take_care_of_0vector(C_ptr->Results[tn]);
    if(volume!=1)
        evaluate_block(1,explicit_cast_to_long(volume)-1,C_ptr->Results[tn]);
    conclude_evaluation(C_ptr->Results[tn]);

    return true;
}

//---------------------------------------------------------------------------

const size_t ParallelBlockLength=10000; // the length of the block of elements to be processed by a thread
// const size_t MaxNrBlocks=20000; // maximum number of blocks
const size_t LocalReductionBound= 10000; // number of candidates in a thread starting local reduction
const size_t SuperBlockLength=1000000; // number of blocks in a super block


//---------------------------------------------------------------------------


template<typename Integer>
void SimplexEvaluator<Integer>::evaluation_loop_parallel() {

    size_t block_length=ParallelBlockLength;
    size_t nr_elements=explicit_cast_to_long(volume)-1; // 0-vector already taken care of
    size_t nr_blocks=nr_elements/ParallelBlockLength;
    if(nr_elements%ParallelBlockLength != 0)
        ++nr_blocks;
        
    size_t nr_superblocks=nr_blocks/SuperBlockLength;
    if(nr_blocks%SuperBlockLength != 0)
        nr_superblocks++;

    /*if(nr_blocks>MaxNrBlocks){
        block_length=nr_elements/MaxNrBlocks;
        if(nr_elements%MaxNrBlocks != 0)
            ++block_length;
        nr_blocks=MaxNrBlocks;
    }*/
    // cout << "nr super " << nr_superblocks;
    
    for(size_t sbi=0;sbi < nr_superblocks;sbi++){
    
    if(verbose && nr_superblocks>1){
        if(sbi >0)
            verboseOutput() << endl;
        verboseOutput() << "Superblock " << sbi+1 << " ";
    }
    
    size_t actual_nr_blocks;
    
    if(sbi==nr_superblocks-1 && nr_blocks%SuperBlockLength!=0) // the last round of smaller length
        actual_nr_blocks=nr_blocks%SuperBlockLength;
    else
        actual_nr_blocks=SuperBlockLength;
        
    // cout << "actual " << actual_nr_blocks << endl;
    
    size_t progess_report=actual_nr_blocks/50;
    if(progess_report==0)
        progess_report=1;
    
    bool skip_remaining;
    deque<bool> done(actual_nr_blocks,false);
    
    do{
    skip_remaining=false;

    #pragma omp parallel
    {
    int tn = omp_get_thread_num();  // chooses the associated collector Results[tn]

    #pragma omp for schedule(dynamic)
    for(size_t i=0; i<actual_nr_blocks;++i){
    
        // cout << "i " << i << endl;
    
        if(skip_remaining || done[i])
            continue;
        if(verbose){
            if(i>0 && i%progess_report==0)
                verboseOutput() <<"." << flush;        
        }
        done[i]=true;
        long block_start=(sbi*SuperBlockLength+i)*block_length+1;  // we start at 1
        long block_end=block_start+block_length-1;
        if(block_end>(long) nr_elements)
            block_end=nr_elements;
        evaluate_block(block_start, block_end,C_ptr->Results[tn]);
        if(C_ptr->Results[tn].candidates_size>= LocalReductionBound) // >= (not > !! ) if 
            skip_remaining=true;                            // LocalReductionBound==ParallelBlockLength
    } // for
    
    } // parallel
    
    if(skip_remaining){
    
        /* #pragma omp parallel for schedule(dynamic)
        for(size_t i=0;i<C_ptr->Results.size();++i)
            reduce(C_ptr->Results[i].Candidates,C_ptr->Results[i].Candidates); 
        */
            
        if(verbose){
                verboseOutput() << "r" << flush;
            }
        collect_vectors();   
        local_reduction(C_ptr->Results[0]);
    }

    }while(skip_remaining);
    
    } // superblock loop
    
    
}

//---------------------------------------------------------------------------


template<typename Integer>
void SimplexEvaluator<Integer>::evaluate_block(long block_start, long block_end, Collector<Integer>& Coll) {


    size_t last;
    vector<Integer> point(dim,0); // represents the lattice element whose residue class is to be processed

    Matrix<Integer> elements(dim,dim); //all 0 matrix

    size_t one_back=block_start-1;
    long counter=one_back;
    
    if(one_back>0){                           // define the last point processed before if it isn't 0
        for(size_t i=1;i<=dim;++i){               
            point[dim-i]=one_back % GDiag[dim-i];
            one_back/= explicit_cast_to_long(GDiag[dim-i]);
        }
        
        for(size_t i=0;i<dim;++i){  // put elements into the state at the end of the previous block
            if(point[i]!=0){
                elements[i]=v_add(elements[i],v_scalar_multiplication_two(InvGenSelRows[i],point[i]));
                v_reduction_modulo(elements[i],volume);
                for(size_t j=i+1;j<dim;++j)
                    elements[j]=elements[i];
            }
        }
    }
    
    // cout << "VOl " << volume << " " << counter << " " << block_end << endl;
    // cout << point;
    // cout << GDiag;
    

    //now we  create the elements in par
    while (true) {
        last = dim;
        for (int k = dim-1; k >= 0; k--) {
            if (point[k] < GDiag[k]-1) {
                last = k;
                break;
            }
        }
        if (counter >= block_end) {
            break;
        }
        
        counter++;
        
        // cout << "COUNTER " << counter << " LAST " << last << endl;

        point[last]++;
        v_add_to_mod(elements[last], InvGenSelRows[last], volume);

        for (size_t i = last+1; i <dim; i++) {
            point[i]=0;
            elements[i] = elements[last];
        }
        
        // cout << "COUNTER " << counter << " LAST " << elements[last];

        
        evaluate_element(elements[last],Coll);
    }

}

//---------------------------------------------------------------------------

/* transfer the vector lists in the collectors to  C_ptr->Results[0] */
template<typename Integer>
void SimplexEvaluator<Integer>::collect_vectors(){

    if(C_ptr->do_Hilbert_basis){
        for(size_t i=1;i<C_ptr->Results.size();++i){
            C_ptr->Results[0].Candidates.splice(C_ptr->Results[0].Candidates.end(),C_ptr->Results[i].Candidates);
            C_ptr->Results[0].candidates_size+=C_ptr->Results[i].candidates_size;
            C_ptr->Results[i].candidates_size = 0;
        }
            
    }
}

//---------------------------------------------------------------------------

/* evaluates a simplex in parallel threads */
template<typename Integer>
void SimplexEvaluator<Integer>::Simplex_parallel_evaluation(){

    if(verbose){
        verboseOutput() << "simplex volume " << volume << endl;
    }

    take_care_of_0vector(C_ptr->Results[0]);
    sequential_evaluation=false;

    evaluation_loop_parallel();
    
    collect_vectors();   // --> Results[0]
    for(size_t i=1;i<C_ptr->Results.size();++i)  // takes care of h-vectors
        conclude_evaluation(C_ptr->Results[i]);
    sequential_evaluation=true;   
    conclude_evaluation(C_ptr->Results[0]);  // h-vector in Results[0] and collected elements
    
    if(verbose){
        verboseOutput() << endl;
    }    
}

//---------------------------------------------------------------------------

template<typename Integer>
bool SimplexEvaluator<Integer>::isDuplicate(const vector<Integer>& cand) const {
    for (size_t i=0; i<dim; i++)
        if (cand[i]==0 && Excluded[i])
            return true;
    return false;
}

//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::update_mult_inhom(Integer& multiplicity){

    if (!C_ptr->isComputed(ConeProperty::Grading) || !C_ptr->do_triangulation)
            return;
    if(C_ptr->level0_dim==dim-1){ // the case of codimension 1
        size_t i;    
        for(i=0;i<dim;++i)
            if(gen_levels[i]>0){
                break;
            }
        assert(i<dim);        
        multiplicity*=gen_degrees[i];  // to correct division in addMult_inner
        multiplicity/=gen_levels[i];
    } 
    else{ 
        size_t i,j=0;
        Integer corr_fact=1;
        for(i=0;i<dim;++i)
            if(gen_levels[i]>0){
                ProjGen[j]=C_ptr->ProjToLevel0Quot.MxV(C_ptr->Generators[key[i]]); // Generators of evaluator may be destroyed
                corr_fact*=gen_degrees[i];
                j++;
            }
        multiplicity*=corr_fact;
        multiplicity/=ProjGen.vol_destructive();
        // cout << "After corr "  << multiplicity << endl;      
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::addMult(Integer multiplicity, Collector<Integer>& Coll) {

    assert(multiplicity != 0);
    Coll.det_sum += multiplicity;
    if (!C_ptr->isComputed(ConeProperty::Grading) || !C_ptr->do_triangulation ||
                    (C_ptr->inhomogeneous && nr_level0_gens!=C_ptr->level0_dim))
        return;
    
    if(C_ptr->inhomogeneous){
        update_mult_inhom(multiplicity);
    }
    
    if (C_ptr->deg1_triangulation) {
        Coll.mult_sum += to_mpz(multiplicity);
    } else {
        mpz_class deg_prod=gen_degrees[0];
        for (size_t i=1; i<dim; i++) {
            deg_prod *= gen_degrees[i];
        }
        mpq_class mult = to_mpz(multiplicity);
        mult /= deg_prod;
        Coll.mult_sum += mult;
    }  
}

//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::local_reduction(Collector<Integer>& Coll) {
    // reduce new against old elements
    //now done directly    reduce(Coll.Candidates, Hilbert_Basis);

    // interreduce
    Coll.Candidates.sort(compare_last<Integer>);
    reduce(Coll.Candidates, Coll.Candidates,Coll.candidates_size);

    // reduce old elements by new ones
    count_and_reduce(Hilbert_Basis, Coll.Candidates);
    Hilbert_Basis.merge(Coll.Candidates,compare_last<Integer>);
    Coll.candidates_size = 0;
}

template<typename Integer>
void SimplexEvaluator<Integer>::count_and_reduce(list< vector< Integer > >& Candi, list< vector<Integer> >& Reducers){
    size_t dummy=Candi.size();
    reduce(Candi,Reducers,dummy);
}

template<typename Integer>
void SimplexEvaluator<Integer>::reduce(list< vector< Integer > >& Candi, list< vector<Integer> >& Reducers, size_t& Candi_size){

    #pragma omp parallel
    {
    typename list <vector <Integer> >::iterator cand=Candi.begin();
    size_t jjpos=0;
    
    #pragma omp for schedule(dynamic)
    for (size_t j=0; j<Candi_size; ++j) {  // remove negative subfacets shared
        for(;j > jjpos; ++jjpos, ++cand) ;       // by non-simpl neg or neutral facets 
        for(;j < jjpos; --jjpos, --cand) ;
        
        if (is_reducible(*cand, Reducers)) 
            (*cand)[dim]=0;                 // mark the candidate
    }
    
    } // parallel
    
    typename list <vector <Integer> >::iterator cand=Candi.begin(); // remove reducibles
    while(cand!=Candi.end()){
        if((*cand)[dim]==0){
            cand=Candi.erase(cand);
            --Candi_size;
        }
        else
            ++cand;
    }
}


template<typename Integer>
bool SimplexEvaluator<Integer>::is_reducible(const vector< Integer >& new_element, list< vector<Integer> >& Reducers){
    // the norm is at position dim

        size_t i,c=0;
        typename list< vector<Integer> >::iterator j;
        for (j = Reducers.begin(); j != Reducers.end(); ++j) {
            if (new_element[dim]< 2*(*j)[dim]) {
                break; //new_element is not reducible;
            }
            else {
                if ((*j)[c]<=new_element[c]){
                    for (i = 0; i < dim; i++) {
                        if ((*j)[i]>new_element[i]){
                            c=i;
                            break;
                        }
                    }
                    if (i==dim) {
                        // move the reducer to the begin
                        //Reducers.splice(Reducers.begin(), Reducers, j);
                        return true;
                    }
                    //new_element is not in the Hilbert Basis
                }
            }
        }
        return false;

}

//---------------------------------------------------------------------------


// Collector

template<typename Integer>
Collector<Integer>::Collector(Full_Cone<Integer>& fc):
  C_ptr(&fc),
  dim(fc.dim),
  det_sum(0),
  mult_sum(0),
  candidates_size(0),
  collected_elements_size(0),
  InEx_hvector(C_ptr->InExCollect.size())
{

    size_t hv_max=0;
    if (C_ptr->do_h_vector) {
        // we need the generators to be sorted by degree
        for (size_t i=C_ptr->nr_gen-dim; i<C_ptr->nr_gen; i++)
            hv_max += C_ptr->gen_degrees[i];
        hvector.resize(hv_max,0);
        inhom_hvector.resize(hv_max,0);
    }
    for(size_t i=0;i<InEx_hvector.size();++i)
        InEx_hvector[i].resize(hv_max,0);
}

template<typename Integer>
Integer Collector<Integer>::getDetSum() const {
    return det_sum;
}

template<typename Integer>
mpq_class Collector<Integer>::getMultiplicitySum() const {
    return mult_sum;
}

template<typename Integer>
const HilbertSeries& Collector<Integer>::getHilbertSeriesSum() const {
    return Hilbert_Series;
}

template<typename Integer>
void Collector<Integer>::transfer_candidates() {
    if(collected_elements_size==0)
        return;
    if (C_ptr->do_Hilbert_basis) {
        #pragma omp critical(CANDIDATES)
        C_ptr->NewCandidates.splice(HB_Elements);
        #pragma omp atomic
        C_ptr->CandidatesSize += collected_elements_size;
    }
    if (C_ptr->do_deg1_elements){
        #pragma omp critical(CANDIDATES)
        C_ptr->Deg1_Elements.splice(C_ptr->Deg1_Elements.begin(),Deg1_Elements);
        #pragma omp atomic
        C_ptr->CandidatesSize += collected_elements_size;
    }
    
    collected_elements_size = 0;
}


template<typename Integer>
size_t Collector<Integer>::get_collected_elements_size(){
     return collected_elements_size;
}



} /* end namespace */
