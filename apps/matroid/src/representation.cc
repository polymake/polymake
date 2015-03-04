/* Copyright (c) 1997-2015
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/Integer.h"
#include "polymake/linalg.h"

namespace polymake { namespace matroid {

namespace{
Array<int> get_index(Set<int> elements, const Array<int>& index){
   int j=0;
   int s=elements.size();
   Array<int> temparray(s);
   for(int i=0;i<index.size();++i){
      if(index[i]==elements.front()){
         temparray[j]=i;
         elements-=elements.front();
         ++j;
         if(j==s) break;
      }
   }
   return temparray;
}

int get_index(int element, const Array<int>& index){
   for(int i=0;i<index.size();++i){
      if(index[i]==element){
         return i;
      }
   }
   return -1;
}
}//end namespace

void finite_representation(perl::Object matroid){
   const Set< Set<int> > bases=matroid.give("BASES");
   const Set<Set<int> > non_bases = matroid.give("NON_BASES");
   const int r = matroid.give("RANK");
   const int n = matroid.give("N_ELEMENTS");

   //construct posibible point configuration:
   Set<int> basis = bases.front();

   Set<int> temp = basis;
   Array<int> basis_index(r);
   for(int i=0;i<r;++i){
      basis_index[i]=temp.front();
      temp-=temp.front();
   }
   
   if(!basis.empty()){//rank == 0
      Matrix<int> bpoints(n,r);
      Matrix<int> tpoints(n,r);

      int i=0;
      for (Entire<Set<int> >::const_iterator it=entire(basis);!it.at_end();++it,++i){
         bpoints(*it,i)=1;
         tpoints(*it,i)=1;
      }

      //binary:
      std::list< Set<int> > bases_selection;
      for (Entire<Set <Set<int> > >::const_iterator it=entire(bases);!it.at_end();++it){
         int intersection_size = (basis*(*it)).size();
         if(intersection_size==r-1){
            int temp=(*it-basis).front();
            int basis_element_index=get_index((basis-*it).front(),basis_index);
            bpoints(temp,basis_element_index)=1;
            tpoints(temp,basis_element_index)=1;
         }else if(intersection_size==r-2){
            bases_selection.push_back(*it);
         }
      }

      //ternary:
      for (Entire<std::list <Set<int> > >::const_iterator it=entire(bases_selection);!it.at_end();++it){
         Array<int> bases_index=get_index(basis-*it,basis_index);
         Matrix<int> temp=tpoints.minor(*it-basis,bases_index);
         if(rank(temp)==1){
            if(temp(1,0)==0){
               tpoints((*it-basis).back(),basis_index[1])=2*temp(0,1)%3;
            }else{
               tpoints((*it-basis).back(),basis_index[0])=2*temp(0,0)%3;
            }
         }
      }

      //find 2x2 binary rank 1  minors that are non bases, that is
      // 1 1
      // 1 1
      std::list< Set<int> > nonbases_block;
      for (Entire<Set<Set<int> > >::const_iterator it=entire(non_bases);!it.at_end();++it){
         if((basis*(*it)).size()==r-2){
            nonbases_block.push_back((*it-basis)+(basis-*it));
         }
      }

      //check whether an minor is an permutation of:
      //1 1 0
      //1 0 1
      //0 1 1
      for (Entire<Set<Set<int> > >::const_iterator it=entire(non_bases);!it.at_end();++it){
         if((basis*(*it)).size()==r-3){
            Set<int> diff0 = *it-basis;
            Array<int> temparray=get_index(basis-*it,basis_index);
            const Matrix<int> bminor=bpoints.minor(diff0,temparray);
            const Matrix<int> tminor=tpoints.minor(diff0,temparray);
            if((det(bminor)==2 || det(bminor)==-2) && det(tminor)%3!=0){
               if(tpoints(diff0.front(),temparray[2])!=0){
                  tpoints(diff0.front(),temparray[2])=2;
                  //change parallel vectors, too
                  Set<int> temp2;
                  temp2+=diff0.front();
                  temp2+=basis_index[temparray[2]];
                  for (Entire<std::list <Set<int> > >::const_iterator itt=entire(nonbases_block);!itt.at_end();++itt){
                     if((*itt*temp2).size()==2){
                        if(tpoints(((*itt-temp2)-basis).front(),get_index(((*itt-temp2)*basis).front(),basis_index))==1){
                           tpoints(((*itt-temp2)-basis).front(),get_index(((*itt-temp2)*basis).front(),basis_index))=2;
                        }//else ?
                     }
                  }
               }else{
                  tpoints(diff0.front(),temparray[0])=2;
                  Set<int> temp2;
                  temp2+=diff0.front();
                  temp2+=basis_index[temparray[0]];
                  for (Entire<std::list <Set<int> > >::const_iterator itt=entire(nonbases_block);!itt.at_end();++itt){
                     if((*itt*temp2).size()==2){
                        if(tpoints(((*itt-temp2)-basis).front(),get_index(((*itt-temp2)*basis).front(),basis_index))==1){
                           tpoints(((*itt-temp2)-basis).front(),get_index(((*itt-temp2)*basis).front(),basis_index))=2;
                        }//else ?
                     }
                  }
               }
            }
         }
      }

      //check point configuration:
      Set< Set<int> > bases_for_bpoints;
      Set< Set<int> > bases_for_tpoints;
      for (Entire< Subsets_of_k<const sequence&> >::const_iterator i=entire(all_subsets_of_k(sequence(0,n),r)); !i.at_end(); ++i) {
         const Matrix<int> subsetb=bpoints.minor(*i,All);
         const Matrix<int> subsett=tpoints.minor(*i,All);     
         if (det(subsetb) %2!=0) {
            bases_for_bpoints+=*i;
         }
         if (det(subsett) %3!=0) {
            bases_for_tpoints+=*i;
         }
      }
      if(bases_for_bpoints==bases){
         matroid.take("BINARY_VECTORS") << bpoints;
         matroid.take("BINARY") << 1;
      }else{
         matroid.take("BINARY") << 0;
      }

      if(bases_for_tpoints==bases){
         matroid.take("TERNARY_VECTORS") << tpoints;
         matroid.take("TERNARY") << 1;
      }else{
         //matroid.take("TERNARY") << 0; //FIXME #742
      }
   }else{
      Matrix<int> bpoints(n,1);
      Matrix<int> tpoints(n,1);
      matroid.take("BINARY_VECTORS") << bpoints;
      matroid.take("BINARY") << 1;
      matroid.take("TERNARY_VECTORS") << tpoints;
      matroid.take("TERNARY") << 1;
   }
}

Function4perl(&finite_representation, "finite_representation(Matroid)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
