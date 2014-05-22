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
    key=Map.max_rank_submatrix_lex(dim);
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
  det_sum(0),
  mult_sum(0),
  candidates_size(0),
  collected_elements_size(0),
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
        hvector.resize(hv_max);
        inhom_hvector.resize(hv_max);
    }
    
    if(fc.inhomogeneous)
        ProjGen=Matrix<Integer>(dim-fc.level0_dim,dim-fc.level0_dim);    
    
    level0_gen_degrees.reserve(fc.dim);
    
    for(size_t i=0;i<fc.InExCollect.size();++i){
        InExSimplData[i].GenInFace.resize(fc.dim);
        InExSimplData[i].hvector.resize(hv_max);
        InExSimplData[i].gen_degrees.reserve(fc.dim);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::add_to_inex_faces(const vector<Integer> offset, size_t Deg){

    for(size_t i=0;i<nrInExSimplData;++i){
        bool in_face=true;
        for(size_t j=0;j<dim;++j)
            if((offset[j]!=0) && !InExSimplData[i].GenInFace.test(j)){  //  || Excluded[j] superfluous
                in_face=false;
                break;
            }
        if(!in_face)
            continue;
        InExSimplData[i].hvector[Deg]+=InExSimplData[i].mult;            
    }
    
}

//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::prepare_inclusion_exclusion_simpl(const vector<key_t>& key,size_t Deg) {
     
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
        for(size_t i=0;i<InExSimplData[nrInExSimplData].hvector.size();++i)
          InExSimplData[nrInExSimplData].hvector[i]=0;
        InExSimplData[nrInExSimplData].mult=F->second;
        nrInExSimplData++;  
     }
     
     if(C_ptr->do_h_vector){
        vector<Integer> ZeroV(dim,0);
        add_to_inex_faces(ZeroV,Deg);
     }
     
}

//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::update_inhom_hvector(long level_offset, size_t Deg){

    // cout << "*** " << level_offset << " " << Deg << endl;

    if(level_offset==1){
        inhom_hvector[Deg-1]++;
        return;
    }
    
    size_t Deg_i;
    
    assert(level_offset==0);
    
    for(size_t i=0;i<dim;++i){
        // cout << "+++ " << gen_levels[i] << " " << gen_degrees[i] << endl;
        if(gen_levels[i]==1){
            Deg_i=Deg+gen_degrees[i];
            inhom_hvector[Deg_i-1]++;
        }
    }
    // cout << "------ " << inhom_hvector << endl;
    // cout << level0_gen_degrees;
}

//---------------------------------------------------------------------------

size_t Unimod=0, Ht1NonUni=0, Gcd1NonUni=0, NonDecided=0, NonDecidedHyp=0;
size_t TotDet=0;

/* evaluates a simplex in regard to all data */
template<typename Integer>
Integer SimplexEvaluator<Integer>::evaluate(SHORTSIMPLEX<Integer>& s) {
    Integer& volume = s.vol;
    vector<key_t>& key = s.key;
    Full_Cone<Integer>& C = *C_ptr;

    bool do_only_multiplicity =
        C.do_only_multiplicity;
//        || (s.height==1 && C.do_partial_triangulation);

    size_t i,j;


    //degrees of the generators according to the Grading of C
    if(C.isComputed(ConeProperty::Grading))
        for (i=0; i<dim; i++)
            gen_degrees[i] = C.gen_degrees[key[i]];
            
    size_t nr_level0_gens=0;
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
        if(C.inhomogeneous){
            if(nr_level0_gens==C.level0_dim)
                update_mult_inhom(volume);
        }
        else
            addMult(volume);
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
        }
        else
            #pragma omp atomic
            Ht1NonUni++;
    }

    if (unimodular && !C.do_h_vector && !C.do_Stanley_dec) {
        if(C.inhomogeneous){
            if(nr_level0_gens==C.level0_dim)
                update_mult_inhom(volume);
        }
        else
            addMult(volume);
        return volume;
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

    if(C.inhomogeneous){
        if(nr_level0_gens==C.level0_dim)
                update_mult_inhom(volume);
        }
    else
        addMult(volume);


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

    Integer Test;
    size_t Deg=0;
    long level=0, level_offset=0; // level_offset is the level of the lement in par + its offset in the Stanley dec
    Integer level_Int=0;
    for(i=0;i<dim;i++)
        Excluded[i]=false;
    for(i=0;i<dim;i++){ // excluded facets and degree shift for 0-vector
        Test=Indicator[i];
        if(Test<0)
        {
            Excluded[i]=true; // the facet opposite to vertex i is excluded
            if(C.do_h_vector){
                Deg += gen_degrees[i];
                if(C.inhomogeneous)
                    level_offset+=gen_levels[i];                    
            }
        }
        if(Test==0){  // Order_Vector in facet, now lexicographic decision
            for(j=0;j<dim;j++){
                if(InvGenSelCols[j][i]<0){ // COLUMNS of InvGen give supp hyps
                    Excluded[i]=true;
                    if(C.do_h_vector){
                        Deg += gen_degrees[i];
                        if(C.inhomogeneous)
                            level_offset+=gen_levels[i];
                    }
                    break;
                }
                if(InvGenSelCols[j][i]>0) // facet included
                    break;
            }
        }
    }
    

    // prepare h-vector and inclusion/exclusion scheme if necessary, insert 0+offset
    if (C.do_h_vector) {
        if(C.inhomogeneous){
            for (i=0; i<inhom_hvector.size(); i++)
                inhom_hvector[i]=0;
            if(level_offset<=1)
                update_inhom_hvector(level_offset,Deg); // here we count 0+offset
        }
        else{
            for (i=0; i<hvector.size(); i++)
                hvector[i]=0;
            hvector[Deg]++; // here we count 0+offset
        }
    }
    
    // cout << "--- " << inhom_hvector;
    
    if(C.do_excluded_faces)
        prepare_inclusion_exclusion_simpl(key,Deg);

    Matrix<Integer>* StanleyMat=&GenCopy; //just to initialize it and make GCC happy

    if(C.do_Stanley_dec){                          // prepare space for Stanley dec
        STANLEYDATA<Integer> SimplStanley;         // key + matrix of offsets
        SimplStanley.key=key;
        Matrix<Integer> offsets(explicit_cast_to_long(volume),dim);  // volume rows, dim columns
        SimplStanley.offsets=offsets;
        #pragma omp critical(STANLEY)
        {
        C.StanleyDec.push_back(SimplStanley);      // extend the Stanley dec by a new matrix
        StanleyMat= &C.StanleyDec.back().offsets;  // and use this matrix for storage
        }
        for(i=0;i<dim;++i)                   // the first vector is 0+offset
            if(Excluded[i])
                (*StanleyMat)[0][i]=volume;
    }

    size_t StanIndex=1;  // counts the number of components in the Stanley dec. vector at 0 already filled if necessary

    if (unimodular) {
        if(C.do_h_vector){
            if(C.inhomogeneous)
                Hilbert_Series.add(inhom_hvector,level0_gen_degrees);    
            else{
                Hilbert_Series.add(hvector,gen_degrees);
                if(C.do_excluded_faces)
                    for(size_t i=0;i<nrInExSimplData;++i){
                        Hilbert_Series.add(InExSimplData[i].hvector,InExSimplData[i].gen_degrees);
                }
            }
        }
        return volume;
    } // the unimodular case has been taken care of


    // now we create and evaluate the points in par
    Integer norm;
    Integer normG;
    Candidates.clear();
    candidates_size = 0;
    typename list <vector <Integer> >::iterator c;
    size_t last;
    vector<Integer> point(dim,0);

    Matrix<Integer> elements(dim,dim); //all 0 matrix
    vector<Integer> help;


    //now we need to create the candidates
    while (true) {
        last = dim;
        for (int k = dim-1; k >= 0; k--) {
            if (point[k] < GDiag[k]-1) {
                last = k;
                break;
            }
        }
        if (last >= dim) {
            break;
        }

        point[last]++;
        v_add_to_mod(elements[last], InvGenSelRows[last], volume);

        for (i = last+1; i <dim; i++) {
            point[i]=0;
            elements[i] = elements[last];
        }
        
        // now the vector in par has been produced and is in elements[last]
        
        // DON'T FORGET: THE VECTOR PRODUCED IS THE "REAL" VECTOR*VOLUME !!

        norm=0; // norm is just the sum of coefficients, = volume*degree if homogenous
                // it is used to sort the Hilbert basis candidates
        normG = 0;  // the degree according to the grading
        for (i = 0; i < dim; i++) {  // since generators have degree 1
            norm+=elements[last][i];
            if(C.do_h_vector || C.do_deg1_elements) {
                normG += elements[last][i]*gen_degrees[i];
            }
        }
        

        level_Int=0;
        
        if(C.inhomogeneous){
            for(i=0;i<dim;i++)
                level_Int+=elements[last][i]*gen_levels[i];
            level=explicit_cast_to_long<Integer>(level_Int/volume); // have to divide by volume; see above
            // cout << level << " ++ " << volume << " -- " << elements[last];
            
            if(level>1) continue; // ***************** nothing to do for this vector
                                  // if we sahould decide to allow Stanley dec in the inhomogeneous case, this must be changed
            
            // cout << "Habe ihn" << endl;
            
            if(C.do_h_vector){
                level_offset=level; 
                for(i=0;i<dim;i++)
                    if(elements[last][i]==0 && Excluded[i])
                        level_offset+=gen_levels[i];
            }
        }


        size_t Deg=0;
        if(C.do_h_vector){
            Deg = explicit_cast_to_long<Integer>(normG/volume);
            for(i=0;i<dim;i++) { // take care of excluded facets and increase degree when necessary
                if(elements[last][i]==0 && Excluded[i]) {
                    Deg += gen_degrees[i];
                }
            }

            //count point in the h-vector
            if(C.inhomogeneous && level_offset<=1)
                update_inhom_hvector(level_offset,Deg);          
            else
                hvector[Deg]++;
            
            if(C.do_excluded_faces)
                add_to_inex_faces(elements[last],Deg);
        }

        if(C.do_Stanley_dec){
            (*StanleyMat)[StanIndex]=elements[last];
            for(i=0;i<dim;i++)
                if(Excluded[i]&&elements[last][i]==0)
                    (*StanleyMat)[StanIndex][i]+=volume;
            StanIndex++;
        }

        // the case of Hilbert bases and degree 1 elements, only added if height >=2
//        if (!C.do_partial_triangulation || s.height >= 2) {
            if (C.do_Hilbert_basis) {
                vector<Integer> candi = v_merge(elements[last],norm);
                if (!is_reducible(candi, Hilbert_Basis)) {
                    Candidates.push_back(candi);
                    candidates_size++;
                    if (candidates_size >= 1000) {
                        local_reduction();
                    }
                }
                continue;
            }
            if(C.do_deg1_elements && normG==volume && !isDuplicate(elements[last])) {
                help=GenCopy.VxM(elements[last]);
                v_scalar_division(help,volume);
                Collected_Elements.push_back(help);
                collected_elements_size++;
            }
//        }
    }


    if(C.do_h_vector) {
        if(C.inhomogeneous){
            Hilbert_Series.add(inhom_hvector,level0_gen_degrees);
            // cout << "WAU " << endl;
            }
        else{
            Hilbert_Series.add(hvector,gen_degrees);
            if(C.do_excluded_faces)
                for(size_t i=0;i<nrInExSimplData;++i)
                    Hilbert_Series.add(InExSimplData[i].hvector,InExSimplData[i].gen_degrees);
        }
    }
    
    // cout << Hilbert_Series << endl;


    if(!C.do_Hilbert_basis)
        return volume;  // no local reduction in this case

    local_reduction();

    //inverse transformation
    //some test for arithmetic overflow may be implemented here
    typename list< vector<Integer> >::iterator jj = Hilbert_Basis.begin();
    while (jj != Hilbert_Basis.end()) {
        if (isDuplicate(*jj)) { //delete the element
            jj = Hilbert_Basis.erase(jj);
        } else {
            jj->pop_back(); //remove the norm entry at the end
            *jj = GenCopy.VxM(*jj);
            v_scalar_division(*jj,volume);
            ++jj;
            collected_elements_size++;
        }
    }

    Collected_Elements.splice(Collected_Elements.end(),Hilbert_Basis);

    return volume;
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
void SimplexEvaluator<Integer>::update_mult_inhom(Integer volume){

    // cout << "Update " << volume << endl;

    if (volume==0) throw volume;
    det_sum += volume;
        if (!C_ptr->isComputed(ConeProperty::Grading))
            return;
    if(C_ptr->level0_dim==dim-1){ // the case of codimension 1
        size_t i;    
        for(i=0;i<dim;++i)
            if(gen_levels[i]>0){
                break;
            }
        assert(i<dim);
        volume*=gen_degrees[i];  // to correct division in addMult_inner
        volume/=gen_levels[i];
        addMult_inner(volume);
    } 
    else{ 
        size_t i,j=0;
        Integer corr_fact=1;
        for(i=0;i<dim;++i)
            if(gen_levels[i]>0){
                // cout << "i " << i << " j " << j << endl;
                ProjGen[j]=C_ptr->ProjToLevel0Quot.MxV(Generators[i]);
                corr_fact*=gen_degrees[i];
                j++;
            }
        volume/=ProjGen.vol_destructive();
        volume*=corr_fact;
        // cout << "After corr "  << volume << endl;      
        addMult_inner(volume);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::addMult(const Integer& volume) {

    if (volume==0) throw volume;
    det_sum += volume;
    if (!C_ptr->isComputed(ConeProperty::Grading))
        return;
    addMult_inner(volume);
}

//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::addMult_inner(const Integer& volume) {

    if (C_ptr->deg1_triangulation) {
        mult_sum += to_mpz(volume);
    } else {
        mpz_class deg_prod=gen_degrees[0];
        for (size_t i=1; i<dim; i++) {
            deg_prod *= gen_degrees[i];
        }
        mpq_class mult = to_mpz(volume);
        mult /= deg_prod;
        mult_sum += mult;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void SimplexEvaluator<Integer>::local_reduction() {
    // reduce new against old elements
//now done directly    reduce(Candidates, Hilbert_Basis);

    // interreduce
    Candidates.sort(compare_last<Integer>);
    reduce(Candidates, Candidates);
//cout << Candidates.size() << endl;

    // reduce old elements
    reduce(Hilbert_Basis, Candidates);
    Hilbert_Basis.merge(Candidates,compare_last<Integer>);
    candidates_size = 0;
}

template<typename Integer>
void SimplexEvaluator<Integer>::reduce(list< vector< Integer > >& Candi, list< vector<Integer> >& Reducers){
    typename list <vector <Integer> >::iterator cand=Candi.begin();
    while (cand != Candi.end()) {
        if (is_reducible(*cand, Reducers)) // erase the candidate
            cand = Candidates.erase(cand);
        else // continue
            ++cand;
    }
}


template<typename Integer>
bool SimplexEvaluator<Integer>::is_reducible(const vector< Integer >& new_element, list< vector<Integer> >& Reducers){
    // the norm is at position dim
    if (new_element[dim]==0) {
        return true; // new_element=0
    }
    else {
        size_t i,c=0;
        typename list< vector<Integer> >::iterator j;
        for (j = Reducers.begin(); j != Reducers.end(); ++j) {
            if (new_element[dim]<2*(*j)[dim]) {
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
}

//---------------------------------------------------------------------------


template<typename Integer>
void SimplexEvaluator<Integer>::transfer_candidates() {
    if ( (C_ptr->do_deg1_elements || C_ptr->do_Hilbert_basis)
       && collected_elements_size > 0 ) {
        #pragma omp critical(CANDIDATES)
        C_ptr->Candidates.splice(C_ptr->Candidates.begin(),Collected_Elements);
        #pragma omp atomic
        C_ptr->CandidatesSize += collected_elements_size;
        collected_elements_size = 0;
    }
}

template<typename Integer>
Integer SimplexEvaluator<Integer>::getDetSum() const {
    return det_sum;
}

template<typename Integer>
mpq_class SimplexEvaluator<Integer>::getMultiplicitySum() const {
    return mult_sum;
}

template<typename Integer>
const HilbertSeries& SimplexEvaluator<Integer>::getHilbertSeriesSum() const {
    return Hilbert_Series;
}

} /* end namespace */
