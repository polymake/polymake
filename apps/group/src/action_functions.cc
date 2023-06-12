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
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/group/orbit.h"
#include "polymake/group/induced_action.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Graph.h"
#include "polymake/hash_map"
#include "polymake/linalg.h"
#include "polymake/matrix_linalg.h"
#include "polymake/SparseMatrix.h"
#include "polymake/polytope/convex_hull.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/GenericMatrix.h"
#include "polymake/ApproximateSet.h"
#include <utility>
#include <iostream>
#include <sstream>

namespace polymake { namespace group {

BigObject induce_set_action(BigObject cone, BigObject action, const std::string& domain_name, OptionSet options){
    Array<Set<Int>> domain = cone.give(domain_name);
    hash_map<Set<Int>, Int> index_of_domain = call_function("common::index_of", domain);
    BigObject induced_action("PermutationActionOnSets");
    // set name
    induced_action.set_name("induced_set_action_of_" + action.name() + "_on_" + domain_name);
    // set generators
    auto gen = action.give("GENERATORS");
    Array<Array<Int>> induced_action_gen = call_function("induced_permutations", gen, domain, index_of_domain);
    induced_action.take("GENERATORS") << induced_action_gen;
    // set domain_name
    induced_action.take("DOMAIN_NAME") << domain_name;
    // set decription
    induced_action.set_description("induced from " + action.name() + " on " + domain_name);
    if (action.exists("CONJUGACY_CLASS_REPRESENTATIVES")) {
        auto conj_class_rep = action.give("CONJUGACY_CLASS_REPRESENTATIVES");
        Array<Array<Int>> conj_class_gen = call_function("induced_permutations",conj_class_rep, domain, index_of_domain);
        induced_action.take("CONJUGACY_CLASS_REPRESENTATIVES") << conj_class_gen;
    }
    if(options["store_index_of"]){
        induced_action.take("INDEX_OF", temporary) << index_of_domain;
    }
    BigObject group = cone.give("GROUP");
    if (!group.lookup_multi("SET_ACTION", OptionSet("DOMAIN_NAME", domain_name)).valid()){
        group.add("SET_ACTION", induced_action);
    }
    return induced_action;
}

// helper functions: Is a matrix a unit matrix?

template <typename Scalar>
bool is_unit_matrix(const Matrix<Scalar>& M){
    return (M == unit_matrix<Scalar>(M.rows())) ? 1:0;
}

template<typename Scalar>
bool is_unit_matrix(const Matrix<double>& M){
    ApproximateSet<Matrix<double>> AppSet;
    AppSet.insert(unit_matrix<double>(M.rows()));
    AppSet.insert(M);
    return (AppSet.size() == 1) ? 1:0;
}

// helper function: is the order of a matrix equal to the order of a permutation?

template <typename Scalar>
Int check_generator_order(const Array<Int>& perm, const Matrix<Scalar>& candidate_gen) {
    Matrix<Scalar> product(candidate_gen);
    Int perm_order = call_function("permutation_order", perm);
    for (Int i = 2; i <= perm_order; i++){
        product = product * candidate_gen;
    }
    bool check = is_unit_matrix(product);
    if(!check) {
        // create error message
        std::ostringstream errormessage;
        errormessage << "The matrix\n\n";
        wrap(errormessage) << candidate_gen;
        errormessage <<"\ncorresponding to the generator \n\n";
        wrap(errormessage) << perm;
        errormessage << "\n\nhas the wrong order. Thus, the given permutations do not induce a matrix action. Please check your assumptions, especially if the embedding of your polytope is really regular.";
        std::string error_string = errormessage.str();
        throw std::runtime_error(error_string);

    }
    return 1;
}

// convert an array of permutations of indices to an array of matrices acting on vertices/points

template <typename Scalar>
std::vector<Matrix<Scalar>> perms2matrices (const Matrix<Scalar>& pts_or_verts, const Array<Array<Int>>& perms, const Matrix<Scalar>& kernel) {
        std::vector<Matrix<Scalar>> gens;
        Matrix<Scalar> pts_plus_kernel;
        if (kernel.rows() != 0){
            pts_plus_kernel = Matrix<Scalar>(pts_or_verts/kernel);
        }else{
            pts_plus_kernel = pts_or_verts;
        }
        for (auto perm : perms){
            Matrix<Scalar> candidate_gen;
            Matrix<Scalar> perm_rows = permuted_rows(pts_or_verts, perm);
            if (kernel.rows() != 0){
                candidate_gen =  solve_right(pts_plus_kernel,Matrix<Scalar>(perm_rows/kernel));
            }else{
                candidate_gen = solve_right(pts_plus_kernel,perm_rows);
            }
            if (check_generator_order(perm, candidate_gen)){
                gens.push_back(candidate_gen);
            }
        }
        return gens;
    }

template <typename Scalar>
void induce_matrix_action_generators(BigObject this_one, const std::string& to_action, const std::string& from_action, const std::string& from_section, const Matrix<Scalar>& kernel){
    Matrix<Scalar> source = this_one.give(from_section);
    Array<Array<Int>> from_gens = this_one.give("GROUP." + from_action + ".GENERATORS");
    std::vector<Matrix<Scalar>> to_gens = perms2matrices(source, from_gens, kernel);
    this_one.take("GROUP." + to_action + ".GENERATORS")  << to_gens;
}

template <typename Scalar>
void induce_matrix_action_conjugacy_class_representatives(BigObject this_one, const std::string& to_action, const std::string& from_action, const std::string& from_section, const Matrix<Scalar>& kernel) {
    Matrix<Scalar> source = this_one.give(from_section);
    Array<Array<Int>> from_gens = this_one.give("GROUP." + from_action + ".CONJUGACY_CLASS_REPRESENTATIVES");
    std::vector<Matrix<Scalar>> class_rep = perms2matrices(source, from_gens, kernel);
    this_one.take("GROUP." + to_action + ".CONJUGACY_CLASS_REPRESENTATIVES")  << class_rep;
}

template<typename SetType>
BigObject induce_implicit_action(BigObject c, BigObject original_action, const Array<SetType>& induced_dom_reps, const std::string& induced_dom_name){
    std::string orig_name = original_action.name();
    std::vector<Set<Int>> reps;
    for (auto rep: induced_dom_reps){
        reps.push_back(Bitset(rep));
    }
    BigObject ia("ImplicitActionOnSets");
    // set name
    ia.set_name("induced_implicit_action_of_" + orig_name + "_on_"+ induced_dom_name);
    // set generators
    ia.take("GENERATORS") << original_action.give("GENERATORS");
    // set domain name
    ia.take("DOMAIN_NAME") << induced_dom_name;
    // set explicit orbit representatives
    ia.take("EXPLICIT_ORBIT_REPRESENTATIVES") << reps;
    // set description
    ia.set_description("induced from " + orig_name + " on " + induced_dom_name);
    Array<Array<Int>> cc;
    if (original_action.lookup("CONJUGACY_CLASS_REPRESENTATIVES") >> cc){
        ia.take("CONJUGACY_CLASS_REPRESENTATIVES") << cc;
    }
    BigObject group = c.give("GROUP");
    if (!group.lookup_multi("IMPLICIT_SET_ACTION", OptionSet("DOMAIN_NAME", induced_dom_name)).valid()){
        group.add("IMPLICIT_SET_ACTION", ia);
    }
    return ia;
}
template <typename Dir>
BigObject automorphism_group(const Graph<Dir>&  G) {
    Array<Array<Int>> autom = call_function("graph::automorphisms",G);
    BigObject permAction("PermutationAction","GENERATORS",autom);
    BigObject auto_group("Group","PERMUTATION_ACTION", permAction);
    return auto_group;
}

BigObject automorphism_group(const IncidenceMatrix<>& Inci, bool on_rows = true) {
    std::vector<std::pair<std::vector<Int>,std::vector<Int>>> autom = call_function("graph::automorphisms", Inci);
    const Int s = autom.size();
    std::vector<std::vector<Int>> row_gens;
    std::vector<std::vector<Int>> col_gens;
    BigObject permAction;
    for (Int i = 0; i < s; i++){
        row_gens.push_back(autom[i].first);
        col_gens.push_back(autom[i].second);
    }
    if (on_rows) {
        permAction = BigObject("PermutationAction","GENERATORS",row_gens);
    } else {
        permAction = BigObject("PermutationAction","GENERATORS",col_gens);
    }
    BigObject auto_group("Group","PERMUTATION_ACTION", permAction);
    return auto_group;
}

BigObject combinatorial_symmetries_impl(BigObject p, const IncidenceMatrix<>& Inci, const std::string& row, const std::string& col){
    std::vector<std::pair<std::vector<Int>,std::vector<Int>>> pairs_of_gens = call_function("graph::automorphisms", Inci);
    std::vector<std::vector<Int>> row_gens;
    std::vector<std::vector<Int>> col_gens;
    for (auto pair: pairs_of_gens) {
        row_gens.push_back(pair.first);
        col_gens.push_back(pair.second);
    }
    BigObject row_action("PermutationAction","GENERATORS",row_gens);
    BigObject col_action("PermutationAction","GENERATORS",col_gens);
    BigObject g("Group", "CombAut");
    BigObject temp;
    g.set_description("combinatorial symmetry group");
    if (!(p.lookup_multi("GROUP", "CombAut").valid())){
        p.add("GROUP", g, row, row_action, col, col_action);
    }
    return row_action;
}

UserFunctionTemplate4perl("# @category Symmetry"
                          "# Construct the induced action of a permutation action on a property that is an ordered collection of sets,"
                          "# such as MAX_INTERIOR_SIMPLICES."
                          "# @param polytope::Cone c the cone or polytope"
                          "# @param PermutationAction a a permutation action on, for example, the vertex indices"
                          "# @param String domain the property the induced action should act upon"
                          "# @return PermutationActionOnSets"
                          "# @example [application polytope]"
                          "# > $c=cube(3, group=>1, character_table=>0);"
                          "# > group::induce_set_action($c, $c->GROUP->VERTICES_ACTION, \"MAX_INTERIOR_SIMPLICES\")->properties();"
                          "# | name: induced_set_action_of_ray_action_on_MAX_INTERIOR_SIMPLICES"
                          "# | type: PermutationActionOnSets"
                          "# | description: induced from ray_action on MAX_INTERIOR_SIMPLICES"
                          "# |"
                          "# | DOMAIN_NAME"
                          "# | MAX_INTERIOR_SIMPLICES"
                          "# |"
                          "# | GENERATORS"
                          "# | 5 4 7 6 1 0 3 2 11 10 9 8 30 29 32 31 38 40 39 41 33 36 35 34 37 43 42 45 44 13 12 15 14 20 23 22 21 24 16 18 17 19 26 25 28 27 49 48 47 46 55 54 57 56 51 50 53 52"
                          "# | 0 2 1 3 12 14 13 15 16 17 18 19 4 6 5 7 8 9 10 11 21 20 22 24 23 25 27 26 28 29 31 30 32 34 33 35 37 36 46 47 48 49 50 52 51 53 38 39 40 41 42 44 43 45 54 56 55 57"
                          "# | 0 4 8 9 1 5 10 11 2 3 6 7 16 20 25 26 12 17 21 27 13 18 22 23 28 14 15 19 24 33 38 42 43 29 34 35 39 44 30 36 40 45 31 32 37 41 50 51 54 55 46 47 52 56 48 49 53 57",
                          "induce_set_action($, $, String, { store_index_of => 0 })");
FunctionTemplate4perl("check_generator_order<Scalar>($, Matrix<Scalar>)");
FunctionTemplate4perl("is_unit_matrix<Scalar>(Matrix<Scalar>)");
FunctionTemplate4perl("perms2matrices<Scalar>(Matrix<Scalar>, $, Matrix<Scalar>)");
FunctionTemplate4perl("induce_matrix_action_generators<Scalar>($,$,$,$,Matrix<Scalar>)");
FunctionTemplate4perl("induce_matrix_action_conjugacy_class_representatives<Scalar>($,$,$,$,Matrix<Scalar>)");
UserFunctionTemplate4perl("# @category Symmetry"
                          "# Construct an implicit action of the action induced on a collection of sets. Only a set of"
                          "# orbit representatives is stored, not the full induced action."
                          "# @param PermutationAction original_action the action of the group on indices"
                          "# @param String property the name of a property that describes an ordered list of sets on which the group should act"
                          "# @return ImplicitActionOnSets the action of the group on the given property, such that only representatives are stored"
                          "# @example [application polytope] To construct the implicit action of the symmetry group of a cube on its maximal simplices, type:"
                          "# > $c=cube(3, group=>1, character_table=>0);"
                          "# > group::induce_implicit_action($c, $c->GROUP->VERTICES_ACTION, $c->GROUP->REPRESENTATIVE_MAX_INTERIOR_SIMPLICES, \"MAX_INTERIOR_SIMPLICES\")->properties();"
                          "# | name: induced_implicit_action_of_ray_action_on_MAX_INTERIOR_SIMPLICES"
                          "# | type: ImplicitActionOnSets"
                          "# | description: induced from ray_action on MAX_INTERIOR_SIMPLICES"
                          "# |"
                          "# | DOMAIN_NAME"
                          "# | MAX_INTERIOR_SIMPLICES"
                          "# |"
                          "# | EXPLICIT_ORBIT_REPRESENTATIVES"
                          "# | {0 1 2 4}"
                          "# | {0 1 2 5}"
                          "# | {0 1 2 7}"
                          "# | {0 3 5 6}"
                          "# |"
                          "# |"
                          "# | GENERATORS"
                          "# | 1 0 3 2 5 4 7 6"
                          "# | 0 2 1 3 4 6 5 7"
                          "# | 0 1 4 5 2 3 6 7",
                          "induce_implicit_action<SetType>($,$, Array<SetType>, $)");
UserFunctionTemplate4perl("# @category Symmetry"
                          "# Find the automorphism group of the graph."
                          "# @param GraphAdjacency graph"
                          "# @return Group",
                          "automorphism_group(GraphAdjacency)");
UserFunctionTemplate4perl("# @category Symmetry"
                          "# Find the automorphism group of the incidence matrix."
                          "# @param IncidenceMatrix I"
                          "# @param Bool on_rows true (default) for row action"
                          "# @return Group",
                          "automorphism_group(IncidenceMatrix; $=1)");
Function4perl(&combinatorial_symmetries_impl,"combinatorial_symmetries_impl($,$,$,$)")
}
}
