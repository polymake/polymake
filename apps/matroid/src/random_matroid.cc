/* Copyright (c) 1997-2023
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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
#include "polymake/list"
#include "polymake/Set.h"
#include "polymake/Graph.h"
#include "polymake/PowerSet.h"
#include "polymake/matroid/check_axioms.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/RandomGenerators.h"

namespace polymake { namespace matroid {

using SetOfFlats = Set< Set< Int > >;

namespace
{

using SetOfFlats = Set< Set< Int > >;

    void insert(SetOfFlats& newF, const SetOfFlats& oldF, Set<Int> setToInsert) {

        bool contains_old_flat = false;
        for (auto flat:oldF){
            if (incl(flat, setToInsert) < 0) {
                contains_old_flat = true;
            }
        }
        if (!contains_old_flat){
            return;
        }

        SetOfFlats result;
        // Check if any of the intersections with any of the sets in the same part
        // are in the part one lower
        for (auto S:newF) {
            bool iscontained = false;
            for (auto C:oldF) {
                if(incl(S*setToInsert, C) <= 0) {
                    iscontained=true;
                }
            }

            if(!iscontained){
                //check if S is not a subset of A.
                if (incl(S,setToInsert)>0){
                    setToInsert += S;
                }
            } else {
                result += S;
            }
        }


        result += setToInsert;
        newF = result;
    }

SetOfFlats generate(
        const SetOfFlats& F_r,
        const Set<Int>& E) {

    //Generates covers of the current set of flats
    SetOfFlats newF;
    for (auto A:F_r) {
        Set<Int> Ac (E-A);
        for (auto C:newF) {
            if (C*A==A) {
                Ac=Ac*(E-C);
            }
        }
        for (auto x:Ac) {
            Set<Int> B(A);
            B+=x;
            insert(newF,F_r,B);
        }
    }

    return newF;
}

void enlarge(
        SetOfFlats& newF,
        SetOfFlats& oldF,
        SetOfFlats& setsOfSets) {

    //Adds every set of setsOfSets to the current set of flats newF, oldF is the flats of one rank lower.
    for (auto A:setsOfSets) {
        insert(newF,oldF,A);
    }
}



}

BigObject sets_to_flats(const Array<SetOfFlats>& arrayOfSets_in, Int n, OptionSet options) {
    Array<SetOfFlats> arrayOfSets(arrayOfSets_in);
    bool check(options["check"]);
    Set<Int> groundset = sequence(0,n);
    SetOfFlats hyperplanes;
    SetOfFlats F_0 {Set<Int>()};
    std::list<SetOfFlats> partition {F_0};

    //max_r is the highest rank we want one of our sets to have.
    //The input is an array of sets of sets, where the position of a set in the array defines the rank it should have in the lattice.
    //example
    //[{{1,2},{4,5,6},{{1,3,4}}]
    //means that {1,2} should have rank 1 and {1,3,4} should have rank 2.
    Int max_r = arrayOfSets.size();
    Int r = 1;

    while (true) {

        SetOfFlats oldPart = partition.back();
        SetOfFlats newPart = generate(oldPart,groundset);

        if (r<max_r) {
            //If r<max_r there are still sets to add.
            enlarge(newPart,oldPart,arrayOfSets[r]);

        }
        //if the groundset is a flat we break since it is the biggest element in the lattice of flats.
        if (newPart.contains(groundset)) {
            hyperplanes = partition.back();
            partition.push_back(newPart);
            break;
        } else {
            partition.push_back(newPart);
        }
        r++;
    }


    if (check) {
        bool mat_check = check_hyperplane_axiom_impl(Array<Set<Int>>(hyperplanes));;
        if (!mat_check){
            std::cout << "This is not a Matroid" << std::endl;
        }
    }

    BigObject matroid("matroid::Matroid",
            "MATROID_HYPERPLANES",hyperplanes,
            "N_ELEMENTS",n);
    return matroid;

}

Set<Int> random_set(Int n, Int groundset_size, UniformlyRandomRanged<long>& R){
    //Generates random subsets of the The groundset
    Set<Int> ret;
    for (Int i=0;i<n;i++){
       ret+=R.get();
    }

    return ret;
}

BigObject random_matroid(Int groundset_size, OptionSet options){
    RandomSeed seed(options["seed"]);
    UniformlyRandomRanged<long> R(groundset_size,seed);
    Array<SetOfFlats> ret(groundset_size);

    // Generate random sets of random size
    for (Int i=0; i<ret.size(); i++){
        Int number_of_sets = R.get();

        while (ret[i].size() < number_of_sets) {
            ret[i] += random_set(R.get(), groundset_size, R);
        }
    }
    //Call the function sets_to_flats which then applies Knuths algorithm
    BigObject mat = sets_to_flats(ret,groundset_size,OptionSet());
    return mat;
}

Function4perl(&sets_to_flats, "sets_to_flats(Array<Set<Set<Int>>>,Int,{check=>1})");

UserFunction4perl("# @category Producing a matroid from scratch"
                  "# Produces a random matroid, with //n// elements, using the algorithm"
                  "# proposed in Donald E. Knuth's paper RANDOM MATROIDS from 1975."
                  "# @param Int n the number of elements"
                  "# @option Int seed controls the outcome of the random number generator;"
                  "# @return Matroid",
                &random_matroid, "random_matroid(Int,{seed=>undef})");
}}
