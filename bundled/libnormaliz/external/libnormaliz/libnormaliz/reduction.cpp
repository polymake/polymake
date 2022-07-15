/*
 * Normaliz
 * Copyright (C) 2007-2022  W. Bruns, B. Ichim, Ch. Soeger, U. v. d. Ohe
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

//---------------------------------------------------------------------------
#include <vector>

#include "libnormaliz/reduction.h"
#include "libnormaliz/vector_operations.h"

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------

template <typename Integer>
Candidate<Integer>::Candidate(const vector<Integer>& v, const vector<Integer>& val, long sd)
    : cand(v), values(val), sort_deg(sd), reducible(true), original_generator(false) {
}

//---------------------------------------------------------------------------

template <typename Integer>
Candidate<Integer>::Candidate(const vector<Integer>& v, const Full_Cone<Integer>& C) : cand(v) {
    compute_values_deg(C);
    reducible = true;
    original_generator = false;
}

//---------------------------------------------------------------------------

template <typename Integer>
Candidate<Integer>::Candidate(const vector<Integer>& v, size_t max_size) {
    cand = v;
    values.resize(max_size, 0);
    sort_deg = 0;
    reducible = true;
    original_generator = false;
}

//---------------------------------------------------------------------------

template <typename Integer>
Candidate<Integer>::Candidate(size_t cand_size, size_t val_size) {
    // cand=v;
    values.resize(val_size, 0);
    cand.resize(cand_size, 0);
    sort_deg = 0;
    reducible = true;
    original_generator = false;
}

//---------------------------------------------------------------------------

template <typename Integer>
Candidate<Integer> sum(const Candidate<Integer>& C, const Candidate<Integer>& D) {
    Candidate<Integer> the_sum = C;
    the_sum.cand = v_add(the_sum.cand, D.cand);
    the_sum.values = v_add(the_sum.values, D.values);
    the_sum.sort_deg += D.sort_deg;
    the_sum.original_generator = false;
    the_sum.reducible = true;
    return the_sum;
}

template Candidate<long> sum(const Candidate<long>&, const Candidate<long>&);
template Candidate<long long> sum(const Candidate<long long>&, const Candidate<long long>&);
template Candidate<mpz_class> sum(const Candidate<mpz_class>&, const Candidate<mpz_class>&);
#ifdef ENFNORMALIZ
template Candidate<renf_elem_class> sum(const Candidate<renf_elem_class>&, const Candidate<renf_elem_class>&);
#endif

//---------------------------------------------------------------------------

template <typename Integer>
void Candidate<Integer>::compute_values_deg(const Full_Cone<Integer>& C) {
    C.Support_Hyperplanes.MxV(values, cand);
    convert(sort_deg, v_scalar_product(cand, C.Sorting));
    if (C.do_module_gens_intcl || C.hilbert_basis_rec_cone_known)  // necessary to make all monoid generators subtractible
        sort_deg *= 2;
}

template <>
void Candidate<renf_elem_class>::compute_values_deg(const Full_Cone<renf_elem_class>& C) {
    assert(false);
}

//---------------------------------------------------------------------------

template <typename Integer>
CandidateList<Integer>::CandidateList() : tmp_candidate(0, 0) {
    verbose = false;
    dual = false;
    last_hyp = 0;
}

//---------------------------------------------------------------------------

template <typename Integer>
CandidateList<Integer>::CandidateList(bool dual_algorithm) : tmp_candidate(0, 0) {
    verbose = false;
    dual = dual_algorithm;
    last_hyp = 0;
}

// size_t NrCompVect=0;
// size_t NrCompVal=0;

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

template <typename Integer>
void CandidateList<Integer>::divide_sortdeg_by2() {
    for (auto& r : Candidates)
        r.sort_deg /= 2;
}

template <typename Integer>
bool CandidateList<Integer>::is_reducible(const vector<Integer>& values, const long sort_deg) const {
    long sd;
    /* if(dual)
        sd=sort_deg;
    else */
    sd = sort_deg / 2;
    size_t kk = 0;
    for (const auto& r : Candidates) {
        /* #pragma omp atomic
        NrCompVect++;
        #pragma omp atomic
        NrCompVal++; */
        if (sd < r.sort_deg) {
            return (false);
        }
        /* #pragma omp atomic
        NrCompVal++;*/
        size_t i = 0;
        if (values[kk] < r.values[kk])
            continue;
        for (; i < values.size(); ++i) {
            /* #pragma omp atomic
            NrCompVal++; */
            if (values[i] < r.values[i]) {
                kk = i;
                break;
            }
        }
        if (i == values.size()) {
            return (true);
        }
    }
    return (false);
}

//---------------------------------------------------------------------------

template <typename Integer>
bool CandidateList<Integer>::is_reducible(Candidate<Integer>& c) const {
    /*if(dual && c.in_HB)
        c.reducible=false;
    else */
    c.reducible = is_reducible(c.values, c.sort_deg);
    return (c.reducible);
}

//---------------------------------------------------------------------------

/* // not used at present
template <typename Integer>
bool CandidateList<Integer>::is_reducible(vector<Integer> v, Candidate<Integer>& cand, const Full_Cone<Integer>& C) const {
    cand = Candidate<Integer>(v, C);
    return ((*this).is_reducible(cand));
}
*/

//---------------------------------------------------------------------------

// Fourth version with parallelization and tables
template <typename Integer>
void CandidateList<Integer>::reduce_by(CandidateList<Integer>& Reducers) {
    size_t csize = Candidates.size();

    bool skip_remaining = false;
    std::exception_ptr tmp_exception;

// This parallel region cannot throw a NormalizException
#pragma omp parallel
    {
        CandidateTable<Integer> ReducerTable(Reducers);
        auto c = Candidates.begin();
        size_t cpos = 0;

#pragma omp for  // schedule(dynamic) removed because of clang problems
        for (size_t k = 0; k < csize; ++k) {
            for (; k > cpos; ++cpos, ++c)
                ;
            for (; k < cpos; --cpos, --c)
                ;

            if (skip_remaining)
                continue;
            try {
                INTERRUPT_COMPUTATION_BY_EXCEPTION

                ReducerTable.is_reducible(*c);

            } catch (const std::exception&) {
                tmp_exception = std::current_exception();
                skip_remaining = true;
#pragma omp flush(skip_remaining)
            }
        }

    }  // end parallel

    if (!(tmp_exception == 0))
        std::rethrow_exception(tmp_exception);

    // erase reducibles
    for (auto c = Candidates.begin(); c != Candidates.end();) {
        if ((*c).reducible)
            c = Candidates.erase(c);
        else  // continue
            ++c;
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void CandidateList<Integer>::auto_reduce() {
    if (empty())
        return;

    sort_by_deg();
    auto_reduce_sorted();
}

//---------------------------------------------------------------------------

template <typename Integer>
void CandidateList<Integer>::auto_reduce_sorted() {
    // uses generations defined by degrees

    if (empty())
        return;

    CandidateList<Integer> Irreducibles(dual), CurrentReducers(dual);
    Integer irred_degree;
    size_t cs = Candidates.size();
    if (verbose && cs > 1000) {
        verboseOutput() << "auto-reduce " << cs << " candidates, degrees <= ";
    }

    while (!Candidates.empty()) {
        irred_degree = Candidates.begin()->sort_deg * 2 - 1;
        if (verbose && cs > 1000) {
            verboseOutput() << irred_degree << " " << flush;
        }
        auto c = Candidates.begin();
        while (c != Candidates.end() && c->sort_deg <= irred_degree) {
            ++c;  // find location for splicing
        }
        CurrentReducers.Candidates.splice(CurrentReducers.Candidates.begin(), Candidates, Candidates.begin(), c);
        reduce_by(CurrentReducers);
        Irreducibles.Candidates.splice(Irreducibles.Candidates.end(), CurrentReducers.Candidates);
    }
    if (verbose && cs > 1000) {
        verboseOutput() << endl;
    }
    Candidates.splice(Candidates.begin(), Irreducibles.Candidates);
}

#ifdef ENFNORMALIZ
template <>
void CandidateList<renf_elem_class>::auto_reduce_sorted() {
    assert(false);
}
#endif
//---------------------------------------------------------------------------

template <typename Integer>
bool CandidateList<Integer>::reduce_by_and_insert(Candidate<Integer>& cand, const CandidateList<Integer>& Reducers) {
    bool irred = !Reducers.is_reducible(cand);
    if (irred)
        Candidates.push_back(cand);
    return irred;
}

//---------------------------------------------------------------------------

template <typename Integer>
bool CandidateList<Integer>::reduce_by_and_insert(const vector<Integer>& v,
                                                  Full_Cone<Integer>& C,
                                                  CandidateList<Integer>& Reducers) {
    Candidate<Integer> cand(v, C);
    return reduce_by_and_insert(cand, Reducers);
}

//---------------------------------------------------------------------------

template <typename Integer>
void CandidateList<Integer>::unique_vectors() {
    assert(dual);

    if (empty())
        return;

    // sort_by_val();

    auto h_start = Candidates.begin();

    h_start++;
    for (auto h = h_start; h != Candidates.end();) {
        auto prev = h;
        prev--;
        if (h->values == prev->values)  // since cone may not be pointed in the dual , vectors
            h = Candidates.erase(h);    // must be made unique modulo the unit group
        else                            // values gives standard embedding
            ++h;
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
bool deg_compare(const Candidate<Integer>& a, const Candidate<Integer>& b) {
    return (a.sort_deg < b.sort_deg);
}

//---------------------------------------------------------------------------

template <typename Integer>
bool val_compare(const Candidate<Integer>& a, const Candidate<Integer>& b) {
    if (a.sort_deg < b.sort_deg)
        return (true);
    if (a.sort_deg == b.sort_deg) {
        if (a.values < b.values)
            return true;
        if (a.values == b.values)
            return a.mother < b.mother;
    }
    return false;
}

//---------------------------------------------------------------------------

template <typename Integer>
void CandidateList<Integer>::sort_by_deg() {
    Candidates.sort(deg_compare<Integer>);
}

//---------------------------------------------------------------------------

template <typename Integer>
void CandidateList<Integer>::sort_by_val() {
    Candidates.sort(val_compare<Integer>);
}

//---------------------------------------------------------------------------

template <typename Integer>
void CandidateList<Integer>::clear() {
    Candidates.clear();
}

//---------------------------------------------------------------------------

template <typename Integer>
size_t CandidateList<Integer>::size() {
    return Candidates.size();
}

//---------------------------------------------------------------------------

template <typename Integer>
bool CandidateList<Integer>::empty() {
    return Candidates.empty();
}

/*
template<typename Integer>
void CandidateList<Integer>::search(){

    vector<Integer> TESTV(6);
    TESTV[0]=2;
    TESTV[1]=6;
    TESTV[2]=0;
    TESTV[3]=15;
    TESTV[4]=18;
    TESTV[5]=2;

    for(const auto& h : Candidates){
        if(h.cand==TESTV){
            assert(h.cand!=TESTV);
        }

    }

}
// */

//---------------------------------------------------------------------------

template <typename Integer>
void CandidateList<Integer>::merge(CandidateList<Integer>& NewCand) {
    Candidates.merge(NewCand.Candidates, deg_compare<Integer>);
}

//---------------------------------------------------------------------------

template <typename Integer>
void CandidateList<Integer>::merge_by_val(CandidateList<Integer>& NewCand) {
    list<Candidate<Integer>*> dummy;
    merge_by_val_inner(NewCand, false, dummy);
}

//---------------------------------------------------------------------------

template <typename Integer>
void CandidateList<Integer>::merge_by_val(CandidateList<Integer>& NewCand, list<Candidate<Integer>*>& New_Elements) {
    CandidateList<Integer> dummy;
    merge_by_val_inner(NewCand, true, New_Elements);
}

//---------------------------------------------------------------------------

template <typename Integer>
void CandidateList<Integer>::merge_by_val_inner(CandidateList<Integer>& NewCand,
                                                bool collect_new_elements,
                                                list<Candidate<Integer>*>& New_Elements) {
    CandidateList<Integer> Coll;
    Coll.dual = dual;
    Coll.last_hyp = last_hyp;

    while (!empty() || !NewCand.empty()) {
        if (NewCand.empty()) {
            Coll.Candidates.splice(Coll.Candidates.begin(), Candidates);
            break;
        }

        if (empty()) {
            typename list<Candidate<Integer> >::reverse_iterator h;
            if (collect_new_elements) {
                for (h = NewCand.Candidates.rbegin(); h != NewCand.Candidates.rend(); ++h)
                    New_Elements.push_front(&(*h));
            }
            Coll.Candidates.splice(Coll.Candidates.begin(), NewCand.Candidates);
            break;
        }

        if (NewCand.Candidates.back().values == Candidates.back().values) {  // if equal, new is erased
            if (NewCand.Candidates.back().mother < Candidates.back().mother)
                Candidates.back().mother = NewCand.Candidates.back().mother;
            NewCand.Candidates.pop_back();
            continue;
        }

        if (val_compare<Integer>(Candidates.back(), NewCand.Candidates.back())) {  // old is smaller, new must be inserteed
            if (collect_new_elements) {
                New_Elements.push_front(&(NewCand.Candidates.back()));
            }
            Coll.Candidates.splice(Coll.Candidates.begin(), NewCand.Candidates, --NewCand.Candidates.end());
            continue;
        }

        Coll.Candidates.splice(Coll.Candidates.begin(), Candidates, --Candidates.end());  // the remaining case
    }

    splice(Coll);  // Coll moved to this
}

//---------------------------------------------------------------------------

template <typename Integer>
void CandidateList<Integer>::push_back(const Candidate<Integer>& cand) {
    // cout << cand;
    Candidates.push_back(cand);
}

//---------------------------------------------------------------------------

template <typename Integer>
void CandidateList<Integer>::extract(list<vector<Integer> >& V_List) {
    for (const auto& c : Candidates)
        V_List.push_back(c.cand);
}

//---------------------------------------------------------------------------

template <typename Integer>
void CandidateList<Integer>::splice(CandidateList<Integer>& NewCand) {
    Candidates.splice(Candidates.begin(), NewCand.Candidates);
}

//---------------------------------------------------------------------------

template <typename Integer>
CandidateTable<Integer>::CandidateTable(CandidateList<Integer>& CandList) {
    for (auto& c : CandList.Candidates)
        ValPointers.push_back(make_pair(c.sort_deg, &(c.values)));
    dual = CandList.dual;
    last_hyp = CandList.last_hyp;
}

//---------------------------------------------------------------------------

template <typename Integer>
CandidateTable<Integer>::CandidateTable(bool dual, size_t last_hyp) {
    this->dual = dual;
    this->last_hyp = last_hyp;
}

//---------------------------------------------------------------------------

template <typename Integer>
bool CandidateTable<Integer>::is_reducible(Candidate<Integer>& c) {
    c.reducible = is_reducible(c.values, c.sort_deg);
    return (c.reducible);
}

//---------------------------------------------------------------------------

template <typename Integer>
bool CandidateTable<Integer>::is_reducible(const vector<Integer>& values, const long sort_deg) {
    long sd;
    /* if(dual)
        sd=sort_deg;
    else */
    sd = sort_deg / 2;
    size_t kk = 0;
    for (auto r = ValPointers.begin(); r != ValPointers.end(); ++r) {
        /* #pragma omp atomic
        NrCompVect++;
        #pragma omp atomic
        NrCompVal++;*/
        if (sd < (long)r->first) {
            return (false);
        }
        /* #pragma omp atomic
        NrCompVal++;*/
        size_t i = 0;
        if (values[kk] < (*(r->second))[kk])
            continue;
        for (; i < values.size(); ++i) {
            /* #pragma omp atomic
            NrCompVal++; */
            if (values[i] < (*(r->second))[i]) {
                kk = i;
                break;
            }
        }
        if (i == values.size()) {
            ValPointers.splice(ValPointers.begin(), ValPointers, r);
            return (true);
        }
    }
    return (false);
}

//---------------------------------------------------------------------------

template <typename Integer>
bool CandidateTable<Integer>::is_reducible_unordered(Candidate<Integer>& c) {
    c.reducible = is_reducible_unordered(c.values, c.sort_deg);
    return (c.reducible);
}

//---------------------------------------------------------------------------

template <typename Integer>
bool CandidateTable<Integer>::is_reducible_unordered(const vector<Integer>& values, const long sort_deg) {
    long sd;
    if (dual)
        sd = sort_deg;
    else
        sd = sort_deg / 2;
    size_t kk = 0;
    for (auto r = ValPointers.begin(); r != ValPointers.end(); ++r) {
        if (sd <= (long)r->first) {
            continue;  // in the ordered version we can say: return(false);
        }
        // #pragma omp atomic
        // redcounter++;
        vector<Integer>* reducer = r->second;
        if (values[last_hyp] < (*(r->second))[last_hyp])
            continue;
        size_t i = 0;
        if (values[kk] < (*(r->second))[kk])
            continue;
        for (; i < last_hyp; ++i)
            if (values[i] < (*reducer)[i]) {
                kk = i;
                break;
            }
        if (i == last_hyp) {
            ValPointers.splice(ValPointers.begin(), ValPointers, r);
            return (true);
        }
    }
    return (false);
}

#ifndef NMZ_MIC_OFFLOAD  // offload with long is not supported
template class CandidateList<long>;
template class CandidateTable<long>;
template class Candidate<long>;
#endif

template class CandidateList<long long>;
template class CandidateTable<long long>;
template class Candidate<long long>;

template class CandidateList<mpz_class>;
template class CandidateTable<mpz_class>;
template class Candidate<mpz_class>;

#ifdef ENFNORMALIZ
template class CandidateList<renf_elem_class>;
template class CandidateTable<renf_elem_class>;
template class Candidate<renf_elem_class>;
#endif

size_t redcounter = 0;

}  // namespace libnormaliz
