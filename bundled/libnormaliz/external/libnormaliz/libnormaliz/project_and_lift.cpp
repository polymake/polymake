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

#include "libnormaliz/project_and_lift.h"
#include "libnormaliz/vector_operations.h"
#include "libnormaliz/my_omp.h"
#include "libnormaliz/sublattice_representation.h"
#include "libnormaliz/cone.h"

namespace libnormaliz {
using std::vector;

//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// computes c1*v1-c2*v2
template<typename Integer>
vector<Integer> FM_comb(Integer c1, const vector<Integer>& v1,Integer c2, const vector<Integer>& v2, bool& is_zero){
 
    size_t dim=v1.size();
    vector<Integer> new_supp(dim);
    is_zero=false;
    size_t k=0;
    for(;k<dim;++k){
        new_supp[k]=c1*v1[k]-c2*v2[k];
        if(!check_range(new_supp[k]))
            break;    
    }
    Integer g=0;
    if(k==dim)
        g=v_make_prime(new_supp);
    else{ // redo in GMP if necessary
        #pragma omp atomic
        GMP_hyp++;
        vector<mpz_class> mpz_neg(dim), mpz_pos(dim), mpz_sum(dim);
        convert(mpz_neg, v1);
        convert(mpz_pos, v2);
        for (k = 0; k <dim; k++)
            mpz_sum[k]=convertTo<mpz_class>(c1)*mpz_neg[k]-
                    convertTo<mpz_class>(c2)*mpz_pos[k];
        mpz_class GG=v_make_prime(mpz_sum);
        convert(new_supp, mpz_sum);
        convert(g,GG);
    }
    if(g==0)
        is_zero=true;
    return new_supp;
}

template<typename IntegerPL, typename IntegerRet>
vector<size_t>  ProjectAndLift<IntegerPL,IntegerRet>::order_supps(const Matrix<IntegerPL>& Supps){
    
    assert(Supps.nr_of_rows()>0);
    size_t dim=Supps.nr_of_columns();

    vector<pair<nmz_float,size_t> > NewPos,NewNeg, NewNeutr; // to record the order of the support haperplanes
    for(size_t i=0;i<Supps.nr_of_rows();++i){
        if(Supps[i][dim-1] == 0){
            NewNeutr.push_back(make_pair(0.0,i));
            continue;
        }
        nmz_float num,den;
        convert(num,Supps[i][0]);
        convert(den,Supps[i][dim-1]);
        nmz_float quot=num/den; 
        if(Supps[i][dim-1] > 0)
            NewPos.push_back(make_pair(Iabs(quot),i));
        else
            NewNeg.push_back(make_pair(Iabs(quot),i));
    }
    sort(NewPos.begin(),NewPos.end());
    sort(NewNeg.begin(),NewNeg.end());
    NewPos.insert(NewPos.end(),NewNeutr.begin(),NewNeutr.end());
    
    size_t min_length=NewNeg.size();
    if(NewPos.size()<min_length)
        min_length=NewPos.size();
    
    vector<size_t> Order;
    
    for(size_t i=0;i<min_length;++i){
        Order.push_back(NewPos[i].second);
        Order.push_back(NewNeg[i].second);
    }
    for(size_t i=min_length;i<NewPos.size();++i)
        Order.push_back(NewPos[i].second);
    for(size_t i=min_length;i<NewNeg.size();++i)
        Order.push_back(NewNeg[i].second);
    
    assert(Order.size()==Supps.nr_of_rows());
    
    /* for(size_t i=0;i<Order.size();++i)
        cout << Supps[Order[i]][dim-1] << " ";
    cout << endl;*/
    
    return Order;
}
//---------------------------------------------------------------------------
template<typename IntegerPL, typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::compute_projections(size_t dim, size_t down_to, vector< boost::dynamic_bitset<> >& Ind,
                vector< boost::dynamic_bitset<> >& Pair, vector< boost::dynamic_bitset<> >& ParaInPair,
                size_t rank, bool only_projections){
    
    INTERRUPT_COMPUTATION_BY_EXCEPTION
    
    const Matrix<IntegerPL> & Supps=AllSupps[dim];
    size_t dim1=dim-1;
    
    if(verbose)
        verboseOutput() << "embdim " << dim  << " inequalities " << Supps.nr_of_rows() << endl;
    
    if(dim==down_to)
        return;
    
    // Supps.pretty_print(cout);
    // cout << Ind;
    
    // cout << "SSS" << Ind.size() << " " << Ind;

    // We now augment the given cone by the last basis vector and its negative
    // Afterwards we project modulo the subspace spanned by them
    
    vector<key_t> Neg, Pos; // for the Fourier-Motzkin elimination of inequalities    
    Matrix<IntegerPL> SuppsProj(0,dim); // for the support hyperplanes of the projection
    Matrix<IntegerPL> EqusProj(0,dim); // for the equations (both later minimized)
    
    // First we make incidence vectors with the given generators    
    vector< boost::dynamic_bitset<> > NewInd; // for the incidence vectors of the new hyperplanes
    vector< boost::dynamic_bitset<> > NewPair; // for the incidence vectors of the new hyperplanes
    vector< boost::dynamic_bitset<> > NewParaInPair; // for the incidence vectors of the new hyperplanes

    boost::dynamic_bitset<> TRUE;
     if(!is_parallelotope){
         TRUE.resize(Ind[0].size());
         TRUE.set();
     }
     
     vector<bool> IsEquation(Supps.nr_of_rows());
     
    bool rank_goes_up=false; // if we add the last unit vector
    size_t PosEquAt=0; // we memorize the positions of pos/neg equations if rank goes up
    size_t NegEquAt=0;

    for(size_t i=0;i<Supps.nr_of_rows();++i){
        if(!is_parallelotope && Ind[i]==TRUE)
            IsEquation[i]=true;
        
        if(Supps[i][dim1]==0){  // already independent of last coordinate
            no_crunch=false;
            if(IsEquation[i])
                EqusProj.append(Supps[i]); // is equation
            else{
                SuppsProj.append(Supps[i]); // neutral support hyperplane
                if(!is_parallelotope)
                    NewInd.push_back(Ind[i]);
                else{
                    NewPair.push_back(Pair[i]);
                    NewParaInPair.push_back(ParaInPair[i]);
                }
            }
            continue;
        }
        if(IsEquation[i])
            rank_goes_up=true;
        if(Supps[i][dim1]>0){
            if(IsEquation[i])
                PosEquAt=i;
            Pos.push_back(i);
            continue;
        }
        Neg.push_back(i);
        if(IsEquation[i])
            NegEquAt=i;
    }
    
    // cout << "Nach Pos/Neg " << EqusProj.nr_of_rows() << " " << Pos.size() << " " << Neg.size() << endl;
    
    // now the elimination, matching Pos and Neg
    
    // cout << "rank_goes_up " << rank_goes_up << endl;
    
        bool skip_remaining;
#ifndef NCATCH
    std::exception_ptr tmp_exception;
#endif
    
    if(rank_goes_up){
        
        assert(!is_parallelotope);
        
        for(size_t i=0;i<Pos.size();++i){ // match pos and neg equations
            size_t p=Pos[i];
            if(!IsEquation[p])
                continue;
            IntegerPL PosVal=Supps[p][dim1];
            for(size_t j=0;j<Neg.size();++j){
                size_t n=Neg[j];
                if(!IsEquation[n])
                    continue;
                IntegerPL NegVal=Supps[n][dim1];
                bool is_zero;
                // cout << Supps[p];
                // cout << Supps[n];
                vector<IntegerPL> new_equ=FM_comb(PosVal,Supps[n],NegVal,Supps[p],is_zero);
                // cout << "zero " << is_zero << endl;
                // cout << "=====================" << endl;
                if(is_zero)
                    continue;
                EqusProj.append(new_equ);
            }
        }
        
        for(size_t i=0;i<Pos.size();++i){ // match pos inequalities with a negative equation
            size_t p=Pos[i];
            if(IsEquation[p])
                continue;
            IntegerPL PosVal=Supps[p][dim1];
            IntegerPL NegVal=Supps[NegEquAt][dim1];
            vector<IntegerPL> new_supp(dim);
            bool is_zero;
            new_supp=FM_comb(PosVal,Supps[NegEquAt],NegVal,Supps[p],is_zero);
            /* cout << Supps[NegEquAt];
            cout << Supps[p];
            cout << new_supp;
            cout << "zero " << is_zero << endl;
            cout << "+++++++++++++++++++++" << endl; */
            if(is_zero) // cannot happen, but included for analogy
                continue;
            SuppsProj.append(new_supp);
            NewInd.push_back(Ind[p]);            
        }
        
        for(size_t j=0;j<Neg.size();++j){ // match neg inequalities with a posizive equation
            size_t n=Neg[j];
            if(IsEquation[n])
                continue;
            IntegerPL PosVal=Supps[PosEquAt][dim1];
            IntegerPL NegVal=Supps[n][dim1];
            vector<IntegerPL> new_supp(dim);
            bool is_zero;
            new_supp=FM_comb(PosVal,Supps[n],NegVal,Supps[PosEquAt],is_zero);
            /* cout << Supps[PosEquAt];
            cout << Supps[n];
            cout << new_supp;
            cout << "zero " << is_zero << endl;
            cout << "=====================" << endl;*/
            
            if(is_zero) // cannot happen, but included for analogy
                continue;
            SuppsProj.append(new_supp);
            NewInd.push_back(Ind[n]);            
        }
    }
    
    // cout << "Nach RGU " << EqusProj.nr_of_rows() << " " << SuppsProj.nr_of_rows() << endl;
        
    if(!rank_goes_up && !is_parallelotope){ // must match pos and neg hyperplanes
        
        // cout << "Pos " << Pos.size() << " Neg " << Neg.size() << " Supps " << SuppsProj.nr_of_rows() << endl;
        
        skip_remaining=false;
        
        size_t min_nr_vertices=rank-2;
        /*if(rank>=3){
            min_nr_vertices=1;
            for(long i=0;i<(long) rank -3;++i)
                min_nr_vertices*=2;
                
        }*/
        
        #pragma omp parallel for schedule(dynamic)
        for(size_t i=0;i<Pos.size();++i){
            
            if (skip_remaining) continue;
        
#ifndef NCATCH
        try {
#endif
 
            INTERRUPT_COMPUTATION_BY_EXCEPTION
            
            size_t p=Pos[i];
            IntegerPL PosVal=Supps[p][dim1];
            vector<key_t> PosKey;
            for(size_t k=0;k<Ind[i].size();++k)
                if(Ind[p][k])
                    PosKey.push_back(k);
            
            for(size_t j=0;j<Neg.size();++j){
                size_t n=Neg[j];
                // // to give a facet of the extended cone
                // match incidence vectors
                boost::dynamic_bitset<> incidence(TRUE.size());
                size_t nr_match=0;
                vector<key_t> CommonKey;
                for(size_t k=0;k<PosKey.size();++k)
                    if(Ind[n][PosKey[k]]){
                        incidence[PosKey[k]]=true;
                        CommonKey.push_back(PosKey[k]);
                        nr_match++;
                    }
                if(rank>=2 && nr_match<min_nr_vertices) // cannot make subfacet of augmented cone
                    continue; 

                bool IsSubfacet=true;
                for(size_t k=0;k<Supps.nr_of_rows();++k){
                    if(k==p || k==n || IsEquation[k])
                        continue;
                    bool contained=true;
                    for(size_t j=0;j<CommonKey.size();++j){
                        if(!Ind[k][CommonKey[j]]){
                            contained=false;
                            break;                           
                        }                        
                    }
                    if(contained){
                        IsSubfacet=false;
                        break;
                    }
                }
                if(!IsSubfacet)
                    continue;
                //}
                
                IntegerPL NegVal=Supps[n][dim1];
                vector<IntegerPL> new_supp(dim);
                bool is_zero;
                new_supp=FM_comb(PosVal,Supps[n],NegVal,Supps[p],is_zero);
                if(is_zero) // linear combination is 0
                    continue;
                
                if(nr_match==TRUE.size()){ // gives an equation
                    #pragma omp critical(NEWEQ)
                    EqusProj.append(new_supp);
                    continue;
                }
                #pragma omp critical(NEWSUPP)
                {
                SuppsProj.append(new_supp);
                NewInd.push_back(incidence);
                }
            }
            
#ifndef NCATCH
            } catch(const std::exception& ) {
                tmp_exception = std::current_exception();
                skip_remaining = true;
                #pragma omp flush(skip_remaining)
            }
#endif
        }
        
    } // !rank_goes_up && !is_parallelotope
    
#ifndef NCATCH
        if (!(tmp_exception == 0)) std::rethrow_exception(tmp_exception);
#endif
    
   if(!rank_goes_up && is_parallelotope){ // must match pos and neg hyperplanes
       
       size_t codim=dim1-1; // the minimal codim a face of the original cone must have
                          // in order to project to a subfacet of the current one
       size_t original_dim=Pair[0].size()+1;
       size_t max_number_containing_factes=original_dim-codim;
        
        skip_remaining=false;
        
        size_t nr_pos=Pos.size();
        //if(nr_pos>10000)
        //    nr_pos=10000;
        size_t nr_neg=Neg.size();
        // if(nr_neg>10000)
        //    nr_neg=10000;
        
        #pragma omp parallel for schedule(dynamic)
        for(size_t i=0;i<nr_pos;++i){
            
            if (skip_remaining) continue;
        
#ifndef NCATCH
        try {
#endif
 
            INTERRUPT_COMPUTATION_BY_EXCEPTION
            
            size_t p=Pos[i];
            IntegerPL PosVal=Supps[p][dim1];
            
            for(size_t j=0;j<nr_neg;++j){
                size_t n=Neg[j];
                boost::dynamic_bitset<> IntersectionPair(Pair[p].size());
                size_t nr_hyp_intersection=0;
                bool in_parallel_hyperplanes=false;
                bool codim_too_small=false;

                for(size_t k=0;k<Pair[p].size();++k){ // run over all pairs
                    if(Pair[p][k] || Pair[n][k]){
                        nr_hyp_intersection++;
                        IntersectionPair[k]=true;
                        if(nr_hyp_intersection> max_number_containing_factes){
                            codim_too_small=true;
                            break;
                        }
                    }
                    if(Pair[p][k] && Pair[n][k]){
                        if(ParaInPair[p][k]!=ParaInPair[n][k]){
                            in_parallel_hyperplanes=true;
                            break;
                        }
                    }
                }
                if(in_parallel_hyperplanes || codim_too_small)
                    continue;
                
                boost::dynamic_bitset<> IntersectionParaInPair(Pair[p].size());
                for(size_t k=0;k<ParaInPair[p].size();++k){
                    if(Pair[p][k])
                        IntersectionParaInPair[k]=ParaInPair[p][k];
                    else
                        if(Pair[n][k])
                            IntersectionParaInPair[k]=ParaInPair[n][k];
                }
                
                // we must nevertheless use the comparison test
                bool IsSubfacet=true;
                if(!no_crunch){
                for(size_t k=0;k<Supps.nr_of_rows();++k){
                    if(k==p || k==n || IsEquation[k])
                        continue;
                    bool contained=true;
                    
                    for(size_t u=0;u<IntersectionPair.size();++u){
                        if(Pair[k][u] && !IntersectionPair[u]){  // hyperplane k contains facet of Supp
                            contained=false;                      // not our intersection
                            continue;
                        }
                        if(Pair[k][u] && IntersectionPair[u]){
                            if(ParaInPair[k][u]!=IntersectionParaInPair[u]){ // they are contained in parallel 
                                contained=false;                             // original facets
                                continue;
                            }
                        }
                    }

                    if(contained){
                        IsSubfacet=false;
                        break;
                    }
                }
                }
                if(!IsSubfacet)
                    continue;
                
                IntegerPL NegVal=Supps[n][dim1];
                bool dummy;
                vector<IntegerPL> new_supp=FM_comb(PosVal,Supps[n],NegVal,Supps[p],dummy);
                #pragma omp critical(NEWSUPP)
                {
                SuppsProj.append(new_supp);
                NewPair.push_back(IntersectionPair);
                NewParaInPair.push_back(IntersectionParaInPair);
                }
            }
            
#ifndef NCATCH
            } catch(const std::exception& ) {
                tmp_exception = std::current_exception();
                skip_remaining = true;
                #pragma omp flush(skip_remaining)
            }
#endif
        }
        
#ifndef NCATCH
        if (!(tmp_exception == 0)) std::rethrow_exception(tmp_exception);
#endif
        
    } // !rank_goes_up && is_parallelotope
    
    // cout << "Nach FM " << EqusProj.nr_of_rows() << " " << SuppsProj.nr_of_rows() << endl;
    
    Ind.clear(); // no longer needed
    
    EqusProj.resize_columns(dim1); // cut off the trailing 0     
    SuppsProj.resize_columns(dim1); // project hyperplanes
 
    // Equations have not yet been appended to support hypwerplanes
    EqusProj.row_echelon(); // reduce equations
    // cout << "Nach eche " << EqusProj.nr_of_rows() << endl;
    /* for(size_t i=0;i<EqusProj.nr_of_rows(); ++i)
        cout << EqusProj[i]; */
    SuppsProj.append(EqusProj); // append them as pairs of inequalities
    EqusProj.scalar_multiplication(-1);
    SuppsProj.append(EqusProj);
    AllNrEqus[dim1]=EqusProj.nr_of_rows();
    // We must add indictor vectors for the equations
    for(size_t i=0;i<2*EqusProj.nr_of_rows();++i)
        NewInd.push_back(TRUE);    

    if(dim1>1 && !only_projections)
        AllOrders[dim1]=order_supps(SuppsProj);
    swap(AllSupps[dim1],SuppsProj);
    
    size_t new_rank=dim1-EqusProj.nr_of_rows();
    
    compute_projections(dim-1,down_to,NewInd, NewPair, NewParaInPair,new_rank);
}

//---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
bool ProjectAndLift<IntegerPL,IntegerRet>::fiber_interval(IntegerRet& MinInterval, IntegerRet& MaxInterval,
                                                          const vector<IntegerRet>& base_point){
        size_t dim=base_point.size()+1;
        Matrix<IntegerPL>& Supps=AllSupps[dim];
        vector<size_t>& Order=AllOrders[dim];

        bool FirstMin=true, FirstMax=true;
        vector<IntegerPL> LiftedGen;
        convert(LiftedGen,base_point);
        // cout << LiftedGen;
        size_t check_supps=Supps.nr_of_rows();
        if(check_supps>1000 && dim<EmbDim && !no_relax)
            check_supps=1000;
        for(size_t j=0;j<check_supps;++j){
            
            INTERRUPT_COMPUTATION_BY_EXCEPTION
            
            IntegerPL Den=Supps[Order[j]].back();
            if(Den==0)
                continue;
            IntegerPL Num= -v_scalar_product_vectors_unequal_lungth(LiftedGen,Supps[Order[j]]);
            // cout << "Num " << Num << endl;
            IntegerRet Quot;
            bool frac=int_quotient(Quot,Num,Den);
            IntegerRet Bound=0;
            //frac=(Num % Den !=0);
            if(Den>0){ // we must produce a lower bound of the interval
                if(Num>=0){  // true quot >= 0
                    Bound=Quot;
                    if(frac)
                        Bound++;
                }
                else // true quot < 0
                    Bound=-Quot;
                if(FirstMin || Bound > MinInterval){
                    MinInterval=Bound;
                    FirstMin=false;
                }
            }
            if(Den<0){ // we must produce an upper bound of the interval
                if(Num >= 0){ // true quot <= 0
                    Bound=-Quot;
                    if(frac)
                        Bound--;                    
                }
                else // true quot > 0
                    Bound=Quot;
                if(FirstMax || Bound < MaxInterval){
                    MaxInterval=Bound;
                    FirstMax=false;
                }
            }
            if(!FirstMax && !FirstMin && MaxInterval<MinInterval)
                return false; // interval empty
    }
    return true; // interval nonempty
}

///---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::lift_points_to_this_dim(list<vector<IntegerRet> >& Deg1Lifted, 
                                                                   const list<vector<IntegerRet> >& Deg1Proj){

    if(Deg1Proj.empty())
        return;
    size_t dim=Deg1Proj.front().size()+1;
    size_t dim1=dim-1;
    vector<list<vector<IntegerRet> > > Deg1Thread(omp_get_max_threads());
    
    bool skip_remaining;
#ifndef NCATCH
    std::exception_ptr tmp_exception;
#endif
    
    skip_remaining=false;
    int omp_start_level=omp_get_level();
    
    #pragma omp parallel
    {
    int tn;
    if(omp_get_level()==omp_start_level)
        tn=0;
    else    
        tn = omp_get_ancestor_thread_num(omp_start_level+1);
    
    size_t nr_to_lift=Deg1Proj.size(); 
    size_t ppos=0;
    auto  p=Deg1Proj.begin();
    #pragma omp for schedule(dynamic)
    for(size_t i=0;i<nr_to_lift;++i){
        
        if (skip_remaining) continue;
        
        for(; i > ppos; ++ppos, ++p) ;
        for(; i < ppos; --ppos, --p) ;

        
#ifndef NCATCH
        try {
#endif

        IntegerRet MinInterval=0, MaxInterval=0; // the fiber over *p is an interval -- 0 to make gcc happy        
        fiber_interval(MinInterval, MaxInterval, *p);
        // cout << "Min " << MinInterval << " Max " << MaxInterval << endl;
        for(IntegerRet k=MinInterval;k<=MaxInterval;++k){
            
            INTERRUPT_COMPUTATION_BY_EXCEPTION
            
            vector<IntegerRet> NewPoint(dim);
            for(size_t j=0;j<dim1;++j)
                NewPoint[j]=(*p)[j];
            NewPoint[dim1]=k;
            Deg1Thread[tn].push_back(NewPoint);
        }
        
#ifndef NCATCH
        } catch(const std::exception& ) {
            tmp_exception = std::current_exception();
            skip_remaining = true;
            #pragma omp flush(skip_remaining)
        }
#endif

    } // lifting
    } // pararllel
    
#ifndef NCATCH
    if (!(tmp_exception == 0)) std::rethrow_exception(tmp_exception);
#endif
    
    for(size_t i=0;i<Deg1Thread.size();++i)
        Deg1Lifted.splice(Deg1Lifted.begin(),Deg1Thread[i]);   

    /* Deg1.pretty_print(cout);
    cout << "*******************" << endl; */
}

///---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::lift_points_by_generation(){

    assert(EmbDim>=2);

    list<vector<IntegerRet> > Deg1Lifted;
    list<vector<IntegerRet> > Deg1Proj;
    vector<IntegerRet> One(1,GD);
    Deg1Proj.push_back(One);
    
    for(size_t i=2; i<=EmbDim;++i){
        assert(Deg1Lifted.empty());
        lift_points_to_this_dim(Deg1Lifted,Deg1Proj);
        if(verbose)    
            verboseOutput() <<  "embdim " << i << " Deg1Elements " << Deg1Lifted.size() << endl;
        if(i<EmbDim){
            Deg1Proj.clear();
            swap(Deg1Lifted,Deg1Proj);
        }               
    }
    
    swap(Deg1Points,Deg1Lifted); // final result
    /* if(verbose)
        verboseOutput() << "Lifting done" << endl;*/
}

///---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::lift_points_by_generation_float(){
    
    ProjectAndLift<nmz_float,IntegerRet> FloatLift(*this);
    FloatLift.lift_points_by_generation();
    Deg1Points.swap(FloatLift.Deg1Points);

}

///---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::lift_point_recursively(vector<IntegerRet>& final_latt_point,
                                                                  const vector<IntegerRet>& latt_point_proj){
 
    size_t dim1=latt_point_proj.size();
    size_t dim=dim1+1;
    size_t final_dim=AllSupps.size()-1;
    
    IntegerRet MinInterval=0, MaxInterval=0; // the fiber over Deg1Proj[i] is an interval -- 0 to make gcc happy        
    fiber_interval(MinInterval, MaxInterval, latt_point_proj);
    for(IntegerRet k=MinInterval;k<=MaxInterval;++k){
        
        INTERRUPT_COMPUTATION_BY_EXCEPTION
        
        vector<IntegerRet> NewPoint(dim);
        for(size_t j=0;j<dim1;++j)
            NewPoint[j]=latt_point_proj[j];
        NewPoint[dim1]=k;
        if(dim==final_dim && NewPoint!=excluded_point){
            final_latt_point=NewPoint;
            break;
        }
        if(dim<final_dim){
            lift_point_recursively(final_latt_point, NewPoint);
            if(final_latt_point.size()>0)
                break;
        }
    }
}

///---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::find_single_point(){
    
    size_t dim=AllSupps.size()-1;
    assert(dim>=2);
    
    vector<IntegerRet> start(1,GD);
    vector<IntegerRet> final_latt_point;
    lift_point_recursively(final_latt_point,start);
    if(final_latt_point.size()>0){
        SingleDeg1Point=final_latt_point;
        if(verbose)
            verboseOutput() << "Found point" << endl;
    }
    else{
        if(verbose)
            verboseOutput() << "No point found" << endl;        
    }
}

//---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::initialize(const Matrix<IntegerPL>& Supps,size_t rank){

    EmbDim=Supps.nr_of_columns(); // our embedding dimension    
    AllSupps.resize(EmbDim+1);
    AllOrders.resize(EmbDim+1);
    AllNrEqus.resize(EmbDim+1);
    AllSupps[EmbDim]=Supps;
    AllOrders[EmbDim]=order_supps(Supps);
    StartRank=rank;
    GD=1; // the default choice
    verbose=true;
    is_parallelotope=false;
    no_crunch=true;
    use_LLL=false;
    no_relax=false;
    
    LLL_Coordinates=Sublattice_Representation<IntegerRet>(EmbDim); // identity
}




//---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
ProjectAndLift<IntegerPL,IntegerRet>::ProjectAndLift(){
    
}
//---------------------------------------------------------------------------
// General constructor
template<typename IntegerPL,typename IntegerRet>
ProjectAndLift<IntegerPL,IntegerRet>::ProjectAndLift(const Matrix<IntegerPL>& Supps,
                                                     const vector<boost::dynamic_bitset<> >& Ind,size_t rank){
    
    initialize(Supps,rank);
    StartInd=Ind;    
}

//---------------------------------------------------------------------------
// Constructor for parallelotopes
template<typename IntegerPL,typename IntegerRet>
ProjectAndLift<IntegerPL,IntegerRet>::ProjectAndLift(const Matrix<IntegerPL>& Supps,
            const vector<boost::dynamic_bitset<> >& Pair,
            const vector<boost::dynamic_bitset<> >& ParaInPair,size_t rank){
    
    initialize(Supps,rank);
    is_parallelotope=true;
    StartPair=Pair;
    StartParaInPair=ParaInPair;
}

//---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::set_verbose(bool on_off){
        verbose=on_off;
}

//---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::set_LLL(bool on_off){
        use_LLL=on_off;
}

//---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::set_no_relax(bool on_off){
        no_relax=on_off;
}

//---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::set_grading_denom(const IntegerRet GradingDenom){
        GD=GradingDenom;
}

//---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::set_excluded_point(const vector<IntegerRet>& excl_point){
        excluded_point=excl_point;
}

//---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::set_vertices(const Matrix<IntegerPL>& Verts){
        Vertices=Verts;
}

//---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::compute(bool all_points, bool lifting_float){

// Project-and-lift for lattice points in a polytope. 
// The first coordinate is homogenizing. Its value for polytope points ism set by GD so that
// a grading denominator 1=1 can be accomodated.
// We need only the support hyperplanes Supps and the facet-vertex incidence matrix Ind.
// Its rows correspond to facets.
    
    assert(all_points || !lifting_float); // only all points allowed with float

    if(use_LLL){
        LLL_coordinates_without_1st_col(LLL_Coordinates,AllSupps[EmbDim],Vertices,verbose);    
        Matrix<IntegerPL> Aconv; // we cannot use to_sublattice_dual directly (not even with convert) since the integer types may not match
        convert(Aconv,LLL_Coordinates.getEmbeddingMatrix());
        // Aconv.transpose().pretty_print(cout);
        AllSupps[EmbDim] = AllSupps[EmbDim].multiplication(Aconv.transpose());
    }

    if(verbose)
        verboseOutput() << "Projection" << endl;
    compute_projections(EmbDim, 1, StartInd,StartPair,StartParaInPair, StartRank);
    if(all_points){
        if(verbose)
            verboseOutput() << "Lifting" << endl;
        if(!lifting_float || (lifting_float && using_float<IntegerPL>()))
            lift_points_by_generation();
        else
            lift_points_by_generation_float(); // with intermediate conversion to float            
    }
    else{
        if(verbose)
            verboseOutput() << "Try finding a lattice point" << endl;
        find_single_point();
    }
}

//---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::compute_only_projection(size_t down_to){
    
    assert(down_to>=1);    
    compute_projections(EmbDim, down_to, StartInd,StartPair,StartParaInPair, StartRank, true);    
}


//---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::put_eg1Points_into(Matrix<IntegerRet>& LattPoints){
    
    while(!Deg1Points.empty()){
        if(use_LLL){
            /* cout << "ori  " << Deg1Points.front();
            cout << "tra  " << LLL_Coordinates.from_sublattice(Deg1Points.front());
            cout << "tra1 " << LLL_Coordinates.A.VxM(Deg1Points.front());*/
            LattPoints.append(LLL_Coordinates.from_sublattice(Deg1Points.front()));
        }
        else
            LattPoints.append(Deg1Points.front());
        Deg1Points.pop_front();
    }
} 

//---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::put_single_point_into(vector<IntegerRet>& LattPoint){

     if(use_LLL)
         LattPoint=LLL_Coordinates.from_sublattice(SingleDeg1Point);
     else    
        LattPoint=SingleDeg1Point;
}

//---------------------------------------------------------------------------
template<typename IntegerPL,typename IntegerRet>
void ProjectAndLift<IntegerPL,IntegerRet>::putSuppsAndEqus(Matrix<IntegerPL>& SuppsRet, Matrix<IntegerPL>& EqusRet, size_t in_dim){

    assert(in_dim<EmbDim);
    assert(in_dim>0);
    
    EqusRet.resize(0,in_dim); // to make it well-definedf
    size_t equs_start_in_row=AllSupps[in_dim].nr_of_rows()-2*AllNrEqus[in_dim];
    for(size_t i=equs_start_in_row;i<AllSupps[in_dim].nr_of_rows();i+=2) // equations come in +- pairs
        EqusRet.append(AllSupps[in_dim][i]);
    AllSupps[in_dim].swap(SuppsRet);
    // SuppsRet.resize_colums(equs_start_in_row,in_dim);
    SuppsRet.resize(equs_start_in_row,true); // we must delete the superfluous rows because the transformation
                                             // to vector<vector> could else fail.
}
//---------------------------------------------------------------------------

template class ProjectAndLift<mpz_class,mpz_class>;
template class ProjectAndLift<long,long long>;
template class ProjectAndLift<mpz_class,long long>;
template class ProjectAndLift<long long ,long long>;
template class ProjectAndLift<nmz_float,mpz_class>;
template class ProjectAndLift<nmz_float,long long>;
// template class ProjectAndLift<nmz_float, nmz_float>;
#ifndef NMZ_MIC_OFFLOAD  //offload with long is not supported
template class ProjectAndLift<long ,long>;
template class ProjectAndLift<nmz_float,long>;
#endif


} //end namespace libnormaliz
