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

#include <boost/dynamic_bitset.hpp>

#include "libnormaliz/integer.h"
#include "libnormaliz/matrix.h"
#include "libnormaliz/nmz_nauty.h"
#include "libnormaliz/vector_operations.h"

#ifdef NMZ_NAUTY

// #define MAXN 5000    /* Define this before including nauty.h */
// we use dynamic allocation

#include <nauty/nauty.h>

namespace libnormaliz {
using namespace std;

vector<vector<long> > CollectedAutoms;

void getmyautoms(int count, int *perm, int *orbits,
               int numorbits, int stabvertex, int n){
    int i;
    vector<long> this_perm(n);
    for (i = 0; i < n; ++i) this_perm[i] = perm[i];
    CollectedAutoms.push_back(this_perm);
}

template<typename Integer>
vector<vector<long> > compute_automs_by_nauty(const vector<vector<Integer> >& Generators, size_t nr_special_gens, 
                                              const vector<vector<Integer> >& LinForms, 
                                              const size_t nr_special_linforms, mpz_class& group_order,
                                              BinaryMatrix<Integer>& CanType){
    CollectedAutoms.clear();
    
    DYNALLSTAT(graph,g,g_sz);
    DYNALLSTAT(graph,cg,cg_sz);
    DYNALLSTAT(int,lab,lab_sz);
    DYNALLSTAT(int,ptn,ptn_sz);
    DYNALLSTAT(int,orbits,orbits_sz);
    static DEFAULTOPTIONS_GRAPH(options);
    statsblk stats;
    
    options.userautomproc = getmyautoms;
    options.getcanon = TRUE;
    
    int n,m;
    
    options.writeautoms = FALSE;
    options.defaultptn = FALSE;
    
    size_t mm=Generators.size();
    size_t nn=LinForms.size();
    
    BinaryMatrix<Integer> MM(mm,nn);
    Matrix<Integer> MVal(mm,nn);
    
    key_t i,j,k;

    bool first=true;
    Integer mini=0;
    for(i=0;i<mm; ++i){
        for(j=0;j<nn;++j){
            MVal[i][j]=v_scalar_product(Generators[i],LinForms[j]);
            if(MVal[i][j]<mini || first){
                mini=MVal[i][j];
                first=false;
            }
        }
    }
        
    MM.set_offset(mini);
    for(i=0;i<mm; ++i){
        for(j=0;j<nn;++j)
            MM.insert(MVal[i][j]-mini,i,j);
    }
        
    size_t ll=MM.nr_layers();
        
    size_t layer_size=mm+nn;
    n=ll*layer_size;
    m = SETWORDSNEEDED(n);
    
    nauty_check(WORDSIZE,m,n,NAUTYVERSIONID);
    
    DYNALLOC2(graph,g,g_sz,m,n,"malloc");
    DYNALLOC2(graph,cg,cg_sz,n,m,"malloc");
    DYNALLOC1(int,lab,lab_sz,n,"malloc");
    DYNALLOC1(int,ptn,ptn_sz,n,"malloc");
    DYNALLOC1(int,orbits,orbits_sz,n,"malloc");
    
    EMPTYGRAPH(g,m,n);
    
    for(i=0;i<layer_size;++i){   // make vertical edges over all layers
        for(k=1;k<ll;++k)
            ADDONEEDGE(g,(k-1)*layer_size+i,k*layer_size+i,m);
    }
    
    for(i=0;i<mm;++i){   // make horizontal edges layer by layer
        for(j=0;j<nn;++j){
            for(k=0;k<ll;++k){
                if(MM.test(i,j,k))  // k is the number of layers below the current one
                    ADDONEEDGE(g,k*layer_size+i,k*layer_size+mm+j,m);
            }
        }
    }           
    
    for(int ii=0;ii<n;++ii){ // prepare partitions
        lab[ii]=ii;
        ptn[ii]=1;
    }
    
    for(k=0;k<ll;++k){ // make partitions layer by layer
        ptn[k*layer_size+ mm-1]=0; // row vertices in one partition
        for(size_t s=0; s< nr_special_gens;++s) // speciall generators in extra partitions (makes them fixed points)
            ptn[k*layer_size+s]=0;
        ptn[(k+1)*layer_size-1]=0; // column indices in the next
        for(size_t s=0; s< nr_special_linforms;++s) // special linear forms in extra partitions
            ptn[(k+1)*layer_size-2-s]=0;            
    } 

    densenauty(g,lab,ptn,orbits,&options,&stats,m,n,cg);
    
    vector<vector<long> > AutomsAndOrbits(2*CollectedAutoms.size());
    AutomsAndOrbits.reserve(2*CollectedAutoms.size()+3);

    for(k=0;k<CollectedAutoms.size();++k){
        vector<long> GenPerm(mm);
        for(i=0;i<mm;++i)
            GenPerm[i]=CollectedAutoms[k][i];
        AutomsAndOrbits[k]=GenPerm;
        vector<long> LFPerm(nn-nr_special_linforms);  // we remove the special linear forms here
        for(i=mm;i<mm+nn-nr_special_linforms;++i)
            LFPerm[i-mm]=CollectedAutoms[k][i]-mm;
        AutomsAndOrbits[k+CollectedAutoms.size()]=LFPerm;
        AutomsAndOrbits[k+CollectedAutoms.size()];
        
    }    
    
    vector<long> GenOrbits(mm);
    for(i=0;i<mm;++i)
        GenOrbits[i]=orbits[i];
    AutomsAndOrbits.push_back(GenOrbits);
    
    vector<long> LFOrbits(nn-nr_special_linforms); // we remove the special linear forms here
    for(i=0;i<nn-nr_special_linforms;++i)
        LFOrbits[i]=orbits[i+mm]-mm;
    AutomsAndOrbits.push_back(LFOrbits);
 
    group_order=mpz_class(stats.grpsize1);
    
    vector<long> row_order(mm), col_order(nn);
    for(key_t i=0;i<mm;++i)
        row_order[i]=lab[i];
    for(key_t i=0;i<nn;++i)
        col_order[i]=lab[mm+i]-mm;
    
    AutomsAndOrbits.push_back(row_order);
    
    nauty_freedyn();
    
    CanType=MM.reordered(row_order,col_order);
    
    return AutomsAndOrbits;
        
}

#ifndef NMZ_MIC_OFFLOAD  //offload with long is not supported
template vector<vector<long> > compute_automs_by_nauty(const vector<vector<long> >& Generators, size_t nr_special_gens, 
                        const vector<vector<long> >& LinForms,const size_t nr_special_linforms,   
                        mpz_class& group_order, BinaryMatrix<long>& CanType);
#endif // NMZ_MIC_OFFLOAD
template vector<vector<long> > compute_automs_by_nauty(const vector<vector<long long> >& Generators, size_t nr_special_gens, 
                        const vector<vector<long long> >& LinForms,const size_t nr_special_linforms,   
                        mpz_class& group_order, BinaryMatrix<long long>& CanType);
template vector<vector<long> > compute_automs_by_nauty(const vector<vector<mpz_class> >& Generators, size_t nr_special_gens, 
                        const vector<vector<mpz_class> >& LinForms,const size_t nr_special_linforms,   
                        mpz_class& group_order,BinaryMatrix<mpz_class>& CanType);

} // namespace

#endif // NMZ_NAUTY

