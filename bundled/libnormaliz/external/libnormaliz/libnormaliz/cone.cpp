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

#include <stdlib.h>
#include <list>
#include <sys/stat.h>
#include <sys/types.h>

#include "libnormaliz/vector_operations.h"
#include "libnormaliz/project_and_lift.h"
#include "libnormaliz/map_operations.h"
#include "libnormaliz/convert.h"
#include "libnormaliz/cone.h"
#include "libnormaliz/full_cone.h"
#include "libnormaliz/my_omp.h"

namespace libnormaliz {
using namespace std;

// adds the signs inequalities given by Signs to Inequalities
template<typename Integer>
Matrix<Integer> sign_inequalities(const vector< vector<Integer> >& Signs) {
    if (Signs.size() != 1) {
        throw BadInputException("ERROR: Bad signs matrix, has "
                + toString(Signs.size()) + " rows (should be 1)!");
    }
    size_t dim = Signs[0].size();
    Matrix<Integer> Inequ(0,dim);
    vector<Integer> ineq(dim,0);
    for (size_t i=0; i<dim; i++) {
        Integer sign = Signs[0][i];
        if (sign == 1 || sign == -1) {
            ineq[i] = sign;
            Inequ.append(ineq);
            ineq[i] = 0;
        } else if (sign != 0) {
            throw BadInputException("Bad signs matrix, has entry "
                    + toString(sign) + " (should be -1, 1 or 0)!");
        }
    }
    return Inequ;
}

template<typename Integer>
Matrix<Integer> strict_sign_inequalities(const vector< vector<Integer> >& Signs) {
    if (Signs.size() != 1) {
        throw BadInputException("ERROR: Bad signs matrix, has "
                + toString(Signs.size()) + " rows (should be 1)!");
    }
    size_t dim = Signs[0].size();
    Matrix<Integer> Inequ(0,dim);
    vector<Integer> ineq(dim,0);
    ineq[dim-1]=-1;
    for (size_t i=0; i<dim-1; i++) {    // last component of strict_signs always 0
        Integer sign = Signs[0][i];
        if (sign == 1 || sign == -1) {
            ineq[i] = sign;
            Inequ.append(ineq);
            ineq[i] = 0;
        } else if (sign != 0) {
            throw BadInputException("Bad signs matrix, has entry "
                    + toString(sign) + " (should be -1, 1 or 0)!");
        }
    }
    return Inequ;
}

template<typename Integer>
vector<vector<Integer> > find_input_matrix(const map< InputType, vector< vector<Integer> > >& multi_input_data,
                               const InputType type){

    typename map< InputType , vector< vector<Integer> > >::const_iterator it;
    it = multi_input_data.find(type);
    if (it != multi_input_data.end())
        return(it->second);

     vector< vector<Integer> > dummy;
     return(dummy);
}

template<typename Integer>
void insert_column(vector< vector<Integer> >& mat, size_t col, Integer entry){

    if(mat.size()==0)
        return;
    vector<Integer> help(mat[0].size()+1);
    for(size_t i=0;i<mat.size();++i){
        for(size_t j=0;j<col;++j)
            help[j]=mat[i][j];
        help[col]=entry;
        for(size_t j=col;j<mat[i].size();++j)
            help[j+1]=mat[i][j];
        mat[i]=help;
    }
}

template<typename Integer>
void insert_zero_column(vector< vector<Integer> >& mat, size_t col){
    // Integer entry=0;
    insert_column<Integer>(mat,col,0);
}

template<typename Integer>
void Cone<Integer>::homogenize_input(map< InputType, vector< vector<Integer> > >& multi_input_data){

    typename map< InputType , vector< vector<Integer> > >::iterator it;
    it = multi_input_data.begin();
    for(;it!=multi_input_data.end();++it){
        switch(it->first){
            case Type::dehomogenization:
                throw BadInputException("Type dehomogenization not allowed with inhomogeneous input!");
                break;
            case Type::inhom_inequalities: // nothing to do
            case Type::inhom_equations:
            case Type::inhom_congruences:
            case Type::polyhedron:
            case Type::vertices:
            case Type::support_hyperplanes:
            case Type::open_facets:
            case Type::grading:  // already taken care of
                break;
            case Type::strict_inequalities:
                insert_column<Integer>(it->second,dim-1,-1);
                break;
            case Type::offset:
            case Type::projection_coordinates:
                insert_column<Integer>(it->second,dim-1,1);
                break;
            default:  // is correct for signs and strict_signs !
                insert_zero_column<Integer>(it->second,dim-1);
                break;
        }
    }
}

bool denominator_allowed(InputType input_type){
    
    switch(input_type){
        
        case Type::congruences:
        case Type::inhom_congruences:
        case Type::grading:
        case Type::dehomogenization:
        case Type::lattice:
        case Type::normalization:
        case Type::cone_and_lattice:
        case Type::offset:
        case Type::rees_algebra:
        case Type::lattice_ideal:
        case Type::signs:
        case Type::strict_signs:
        case Type::projection_coordinates:
//         case Type::open_facets:
            return false;
            break;
        default:
            return true;
        break;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
Cone<Integer>::Cone(){
}

template<typename Integer>
Cone<Integer>::Cone(InputType input_type, const vector< vector<Integer> >& Input) {
    // convert to a map
    map< InputType, vector< vector<Integer> > > multi_input_data;
    multi_input_data[input_type] = Input;
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(InputType type1, const vector< vector<Integer> >& Input1,
                    InputType type2, const vector< vector<Integer> >& Input2) {
    if (type1 == type2) {
        throw BadInputException("Input types must  pairwise different!");
    }
    // convert to a map
    map< InputType, vector< vector<Integer> > > multi_input_data;
    multi_input_data[type1] = Input1;
    multi_input_data[type2] = Input2;
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(InputType type1, const vector< vector<Integer> >& Input1,
                    InputType type2, const vector< vector<Integer> >& Input2,
                    InputType type3, const vector< vector<Integer> >& Input3) {
    if (type1 == type2 || type1 == type3 || type2 == type3) {
        throw BadInputException("Input types must be pairwise different!");
    }
    // convert to a map
    map< InputType, vector< vector<Integer> > > multi_input_data;
    multi_input_data[type1] = Input1;
    multi_input_data[type2] = Input2;
    multi_input_data[type3] = Input3;
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(const map< InputType, vector< vector<Integer> > >& multi_input_data) {
    process_multi_input(multi_input_data);
}

// now with mpq_class input

template<typename Integer>
Cone<Integer>::Cone(InputType input_type, const vector< vector<mpq_class> >& Input) {
    // convert to a map
    map< InputType, vector< vector<mpq_class> > > multi_input_data;
    multi_input_data[input_type] = Input;
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(InputType type1, const vector< vector<mpq_class> >& Input1,
                    InputType type2, const vector< vector<mpq_class> >& Input2) {
    if (type1 == type2) {
        throw BadInputException("Input types must  pairwise different!");
    }
    // convert to a map
    map< InputType, vector< vector<mpq_class> > > multi_input_data;
    multi_input_data[type1] = Input1;
    multi_input_data[type2] = Input2;
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(InputType type1, const vector< vector<mpq_class> >& Input1,
                    InputType type2, const vector< vector<mpq_class> >& Input2,
                    InputType type3, const vector< vector<mpq_class> >& Input3) {
    if (type1 == type2 || type1 == type3 || type2 == type3) {
        throw BadInputException("Input types must be pairwise different!");
    }
    // convert to a map
    map< InputType, vector< vector<mpq_class> > > multi_input_data;
    multi_input_data[type1] = Input1;
    multi_input_data[type2] = Input2;
    multi_input_data[type3] = Input3;
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(const map< InputType, vector< vector<mpq_class> > >& multi_input_data) {
    process_multi_input(multi_input_data);
}

// now with nmz_float input

template<typename Integer>
Cone<Integer>::Cone(InputType input_type, const vector< vector<nmz_float> >& Input) {
    // convert to a map
    map< InputType, vector< vector<nmz_float> > > multi_input_data;
    multi_input_data[input_type] = Input;
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(InputType type1, const vector< vector<nmz_float> >& Input1,
                    InputType type2, const vector< vector<nmz_float> >& Input2) {
    if (type1 == type2) {
        throw BadInputException("Input types must  pairwise different!");
    }
    // convert to a map
    map< InputType, vector< vector<nmz_float> > > multi_input_data;
    multi_input_data[type1] = Input1;
    multi_input_data[type2] = Input2;
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(InputType type1, const vector< vector<nmz_float> >& Input1,
                    InputType type2, const vector< vector<nmz_float> >& Input2,
                    InputType type3, const vector< vector<nmz_float> >& Input3) {
    if (type1 == type2 || type1 == type3 || type2 == type3) {
        throw BadInputException("Input types must be pairwise different!");
    }
    // convert to a map
    map< InputType, vector< vector<nmz_float> > > multi_input_data;
    multi_input_data[type1] = Input1;
    multi_input_data[type2] = Input2;
    multi_input_data[type3] = Input3;
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(const map< InputType, vector< vector<nmz_float> > >& multi_input_data) {
    process_multi_input(multi_input_data);
}

//---------------------------------------------------------------------------
// now with Matrix
//---------------------------------------------------------------------------

template<typename Integer>
Cone<Integer>::Cone(InputType input_type, const Matrix<Integer>& Input) {
    // convert to a map
    map< InputType, vector< vector<Integer> > >multi_input_data;
    multi_input_data[input_type] = Input.get_elements();
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(InputType type1, const Matrix<Integer>& Input1,
                    InputType type2, const Matrix<Integer>& Input2) {
    if (type1 == type2) {
        throw BadInputException("Input types must  pairwise different!");
    }
    // convert to a map
    map< InputType, vector< vector<Integer> > > multi_input_data;
    multi_input_data[type1] = Input1.get_elements();
    multi_input_data[type2] = Input2.get_elements();
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(InputType type1, const Matrix<Integer>& Input1,
                    InputType type2, const Matrix<Integer>& Input2,
                    InputType type3, const Matrix<Integer>& Input3) {
    if (type1 == type2 || type1 == type3 || type2 == type3) {
        throw BadInputException("Input types must be pairwise different!");
    }
    // convert to a map
    map< InputType, vector< vector<Integer> > > multi_input_data;
    multi_input_data[type1] = Input1.get_elements();
    multi_input_data[type2] = Input2.get_elements();
    multi_input_data[type3] = Input3.get_elements();
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(const map< InputType, Matrix<Integer> >& multi_input_data_Matrix){
    map< InputType, vector< vector<Integer> > > multi_input_data;
    auto it = multi_input_data_Matrix.begin();
    for(; it != multi_input_data_Matrix.end(); ++it){
        multi_input_data[it->first]=it->second.get_elements();
    }
    process_multi_input(multi_input_data);
}

//---------------------------------------------------------------------------
// now with Matrix and mpq_class

template<typename Integer>
Cone<Integer>::Cone(InputType input_type, const Matrix<mpq_class>& Input) {
    // convert to a map
    map< InputType, vector< vector<mpq_class> > >multi_input_data;
    multi_input_data[input_type] = Input.get_elements();
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(InputType type1, const Matrix<mpq_class>& Input1,
                    InputType type2, const Matrix<mpq_class>& Input2) {
    if (type1 == type2) {
        throw BadInputException("Input types must  pairwise different!");
    }
    // convert to a map
    map< InputType, vector< vector<mpq_class> > > multi_input_data;
    multi_input_data[type1] = Input1.get_elements();
    multi_input_data[type2] = Input2.get_elements();
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(InputType type1, const Matrix<mpq_class>& Input1,
                    InputType type2, const Matrix<mpq_class>& Input2,
                    InputType type3, const Matrix<mpq_class>& Input3) {
    if (type1 == type2 || type1 == type3 || type2 == type3) {
        throw BadInputException("Input types must be pairwise different!");
    }
    // convert to a map
    map< InputType, vector< vector<mpq_class> > > multi_input_data;
    multi_input_data[type1] = Input1.get_elements();
    multi_input_data[type2] = Input2.get_elements();
    multi_input_data[type3] = Input3.get_elements();
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(const map< InputType, Matrix<mpq_class> >& multi_input_data_Matrix){
    map< InputType, vector< vector<mpq_class> > > multi_input_data;
    auto it = multi_input_data_Matrix.begin();
    for(; it != multi_input_data_Matrix.end(); ++it){
        multi_input_data[it->first]=it->second.get_elements();
    }
    process_multi_input(multi_input_data);
}

//---------------------------------------------------------------------------
// now with Matrix and nmz_float

template<typename Integer>
Cone<Integer>::Cone(InputType input_type, const Matrix<nmz_float>& Input) {
    // convert to a map
    map< InputType, vector< vector<nmz_float> > >multi_input_data;
    multi_input_data[input_type] = Input.get_elements();
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(InputType type1, const Matrix<nmz_float>& Input1,
                    InputType type2, const Matrix<nmz_float>& Input2) {
    if (type1 == type2) {
        throw BadInputException("Input types must  pairwise different!");
    }
    // convert to a map
    map< InputType, vector< vector<nmz_float> > > multi_input_data;
    multi_input_data[type1] = Input1.get_elements();
    multi_input_data[type2] = Input2.get_elements();
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(InputType type1, const Matrix<nmz_float>& Input1,
                    InputType type2, const Matrix<nmz_float>& Input2,
                    InputType type3, const Matrix<nmz_float>& Input3) {
    if (type1 == type2 || type1 == type3 || type2 == type3) {
        throw BadInputException("Input types must be pairwise different!");
    }
    // convert to a map
    map< InputType, vector< vector<nmz_float> > > multi_input_data;
    multi_input_data[type1] = Input1.get_elements();
    multi_input_data[type2] = Input2.get_elements();
    multi_input_data[type3] = Input3.get_elements();
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(const map< InputType, Matrix<nmz_float> >& multi_input_data_Matrix){
    map< InputType, vector< vector<nmz_float> > > multi_input_data;
    auto it = multi_input_data_Matrix.begin();
    for(; it != multi_input_data_Matrix.end(); ++it){
        multi_input_data[it->first]=it->second.get_elements();
    }
    process_multi_input(multi_input_data);
}

//---------------------------------------------------------------------------

template<typename Integer>
Cone<Integer>::~Cone() {
    if(IntHullCone!=NULL)
        delete IntHullCone;
    if(IntHullCone!=NULL)
        delete SymmCone;
    if(ProjCone!=NULL)
        delete ProjCone;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::process_multi_input(const map< InputType, vector< vector<mpq_class> > >& multi_input_data_const) {

    map< InputType, vector< vector<mpq_class> > > multi_input_data(multi_input_data_const);    
    // since polytope will be comverted to cone, we must do some checks here
    if(exists_element(multi_input_data,Type::grading) && exists_element(multi_input_data,Type::polytope)){
           throw BadInputException("No explicit grading allowed with polytope!");
    }
    if(exists_element(multi_input_data,Type::cone) && exists_element(multi_input_data,Type::polytope)){
        throw BadInputException("Illegal combination of cone generator types!");
    }
    
    map< InputType, vector< vector<Integer> > > multi_input_data_ZZ;
    
    // special treatment of polytope. We convert it o cone
    if(exists_element(multi_input_data,Type::polytope)){
        size_t dim;
        if(multi_input_data[Type::polytope].size()>0){
            dim=multi_input_data[Type::polytope][0].size()+1;
            vector<vector<Integer> > grading;
            grading.push_back(vector<Integer>(dim));
            grading[0][dim-1]=1;
            multi_input_data_ZZ[Type::grading]=grading;
        }
        multi_input_data[Type::cone]=multi_input_data[Type::polytope];
        multi_input_data.erase(Type::polytope);
        for(size_t i=0;i<multi_input_data[Type::cone].size();++i){
            multi_input_data[Type::cone][i].resize(dim);
            multi_input_data[Type::cone][i][dim-1]=1;
        }
    }
    
    // now we clear denominators
    auto it = multi_input_data.begin();
    for(; it != multi_input_data.end(); ++it) {
        for(size_t i=0;i < it->second.size();++i){ 
            mpz_class common_denom=1;
            for(size_t j=0;j<it->second[i].size();++j){
                it->second[i][j].canonicalize();
                common_denom=libnormaliz::lcm(common_denom,it->second[i][j].get_den());
            }
            if(common_denom>1 && !denominator_allowed(it->first))
                throw BadInputException("Proper fraction not allowed in certain input types");
            vector<Integer> transfer(it->second[i].size());
            for(size_t j=0;j<it->second[i].size();++j){
                it->second[i][j]*=common_denom;
                convert(transfer[j],it->second[i][j].get_num());
            }
            multi_input_data_ZZ[it->first].push_back(transfer);
        }
    }
    
    process_multi_input_inner(multi_input_data_ZZ);
}

template<typename Integer>
void Cone<Integer>::process_multi_input(const map< InputType, vector< vector<nmz_float> > >& multi_input_data) {
    
    map< InputType, vector< vector<mpq_class> > > multi_input_data_QQ;
    auto it = multi_input_data.begin();
    for(; it != multi_input_data.end(); ++it) {
        vector<vector<mpq_class> > Transfer;
        vector<mpq_class> vt;
        for(size_t j=0;j<it->second.size();++j){
            for(size_t k=0;k<it->second[j].size();++k)
                vt.push_back(mpq_class(it->second[j][k]));
            Transfer.push_back(vt);
        }
        multi_input_data_QQ[it->first]=Transfer;
    }
    process_multi_input(multi_input_data_QQ);
}

template<typename Integer>
void Cone<Integer>::process_multi_input(const map< InputType, vector< vector<Integer> > >& multi_input_data_const) {
    initialize();
    map< InputType, vector< vector<Integer> > > multi_input_data(multi_input_data_const);
    process_multi_input_inner(multi_input_data);
}

template<typename Integer>
void Cone<Integer>::process_multi_input_inner(map< InputType, vector< vector<Integer> > >& multi_input_data) {
    initialize();
    // find basic input type
    lattice_ideal_input=false;
    nr_latt_gen=0, nr_cone_gen=0;
    bool inhom_input=false;
    
    inequalities_present=false; //control choice of positive orthant

    // NEW: Empty matrix have syntactical influence
    auto it = multi_input_data.begin();
    for(; it != multi_input_data.end(); ++it) {
        switch (it->first) {
            case Type::inhom_inequalities:
            case Type::inhom_equations:
            case Type::inhom_congruences:
            case Type::strict_inequalities:
            case Type::strict_signs:
            case Type::open_facets:
                inhom_input=true;
            case Type::signs:
            case Type::inequalities:
            case Type::equations:
            case Type::congruences:
                break;
            case Type::lattice_ideal:
                lattice_ideal_input=true;
                break;
            case Type::polyhedron:
                inhom_input=true;
            case Type::integral_closure:
            case Type::rees_algebra:
            case Type::polytope:
            case Type::cone:
            case Type::subspace:
                nr_cone_gen++;
                break;
            case Type::normalization:
            case Type::cone_and_lattice:
                nr_cone_gen++;
            case Type::lattice:
            case Type::saturation:
                nr_latt_gen++;
                break;
            case Type::vertices:
            case Type::offset:
                inhom_input=true;
            default:
                break;
        }

        switch (it->first) {  // chceck existence of inrqualities
            case Type::inhom_inequalities:
            case Type::strict_inequalities:
            case Type::strict_signs:
            case Type::signs:
            case Type::inequalities:
            case Type::excluded_faces:
            case Type::support_hyperplanes:
                inequalities_present=true;
            default:
                break;
        }

    }

    bool gen_error=false;
    if(nr_cone_gen>2)
        gen_error=true;

    if(nr_cone_gen==2 && (!exists_element(multi_input_data,Type::subspace)
                      || !(exists_element(multi_input_data,Type::cone)
                          || exists_element(multi_input_data,Type::cone_and_lattice)
                          || exists_element(multi_input_data,Type::integral_closure)
                          || exists_element(multi_input_data,Type::normalization) ) )
    )
        gen_error=true;
    
    if(gen_error){
        throw BadInputException("Illegal combination of cone generator types!");
    }
    
    
    if(nr_latt_gen>1){
        throw BadInputException("Only one matrix of lattice generators allowed!");
    }
    if(lattice_ideal_input){
        if(multi_input_data.size() > 2 || (multi_input_data.size()==2 && !exists_element(multi_input_data,Type::grading))){
            throw BadInputException("Only grading allowed with lattice_ideal!");
        }
    }
    if(exists_element(multi_input_data,Type::open_facets)){
        size_t allowed=0;
        auto it = multi_input_data.begin();
        for(; it != multi_input_data.end(); ++it) {
            switch (it->first) {
                case Type::open_facets:
                case Type::cone:
                case Type::grading:
                case Type::vertices:
                    allowed++;
                    break;
                default:
                    break;                
            }
        }
        if(allowed!=multi_input_data.size())
            throw BadInputException("Illegal combination of input types with open_facets!");        
        if(exists_element(multi_input_data,Type::vertices)){
            if(multi_input_data[Type::vertices].size()>1)
                throw BadInputException("At most one vertex allowed with open_facets!");            
        }
        
    }
    
    if(inhom_input){
        if(exists_element(multi_input_data,Type::dehomogenization) || exists_element(multi_input_data,Type::support_hyperplanes)){
            throw BadInputException("Types dehomogenization and support_hyperplanes not allowed with inhomogeneous input!");
        }
    }
    if(inhom_input || exists_element(multi_input_data,Type::dehomogenization)){
        if(exists_element(multi_input_data,Type::rees_algebra) || exists_element(multi_input_data,Type::polytope)){
            throw BadInputException("Types polytope and rees_algebra not allowed with inhomogeneous input or dehomogenization!");
        }
        if(exists_element(multi_input_data,Type::excluded_faces)){
            throw BadInputException("Type excluded_faces not allowed with inhomogeneous input or dehomogenization!");
        }
    }
    if(exists_element(multi_input_data,Type::grading) && exists_element(multi_input_data,Type::polytope)){
           throw BadInputException("No explicit grading allowed with polytope!");
    }
    
    // remove empty matrices
    it = multi_input_data.begin();
    for(; it != multi_input_data.end();) {
        if (it->second.size() == 0)
            multi_input_data.erase(it++);
        else
            ++it;
    }

    if(multi_input_data.size()==0){
        throw BadInputException("All input matrices empty!");
    }

    //determine dimension
    it = multi_input_data.begin();
    size_t inhom_corr = 0; // correction in the inhom_input case
    if (inhom_input) inhom_corr = 1;
    dim = it->second.front().size() - type_nr_columns_correction(it->first) + inhom_corr;

    // We now process input types that are independent of generators, constraints, lattice_ideal
    // check for excluded faces
    
    ExcludedFaces = find_input_matrix(multi_input_data,Type::excluded_faces);
    if(ExcludedFaces.nr_of_rows()==0)
        ExcludedFaces=Matrix<Integer>(0,dim); // we may need the correct number of columns
    PreComputedSupportHyperplanes = find_input_matrix(multi_input_data,Type::support_hyperplanes);
    
    // check for a grading
    vector< vector<Integer> > lf = find_input_matrix(multi_input_data,Type::grading);
    if (lf.size() > 1) {
        throw BadInputException("Bad grading, has "
                + toString(lf.size()) + " rows (should be 1)!");
    }
    if(lf.size()==1){
        if(inhom_input)
            lf[0].push_back(0); // first we extend grading trivially to have the right dimension
        setGrading (lf[0]);     // will eventually be set in full_cone.cpp

    }

    // cout << "Dim " << dim <<endl;

    // check consistence of dimension
    it = multi_input_data.begin();
    size_t test_dim;
    for (; it != multi_input_data.end(); ++it) {
        test_dim = it->second.front().size() - type_nr_columns_correction(it->first) + inhom_corr;
        if (test_dim != dim) {
            throw BadInputException("Inconsistent dimensions in input!");
        }
    }
    
    if(inhom_input)
        homogenize_input(multi_input_data);
    
    if(exists_element(multi_input_data,Type::projection_coordinates)){
        projection_coord_indicator.resize(dim);
        for(size_t i=0;i<dim;++i)
            if(multi_input_data[Type::projection_coordinates][0][i]!=0)
                projection_coord_indicator[i]=true;        
    }
    
    // check for dehomogenization
    lf = find_input_matrix(multi_input_data,Type::dehomogenization);
    if (lf.size() > 1) {
        throw BadInputException("Bad dehomogenization, has "
                + toString(lf.size()) + " rows (should be 1)!");
    }
    if(lf.size()==1){
        setDehomogenization(lf[0]);
    }

    // now we can unify implicit and explicit truncation
    // Note: implicit and explicit truncation have already been excluded
    if (inhom_input) {
        Dehomogenization.resize(dim,0),
        Dehomogenization[dim-1]=1;
        is_Computed.set(ConeProperty::Dehomogenization);
    }
    if(isComputed(ConeProperty::Dehomogenization))
        inhomogeneous=true;

    if(lattice_ideal_input){
        prepare_input_lattice_ideal(multi_input_data);
    }

    Matrix<Integer> LatticeGenerators(0,dim);
    prepare_input_generators(multi_input_data, LatticeGenerators);

    prepare_input_constraints(multi_input_data); // sets Equations,Congruences,Inequalities

    // set default values if necessary
    if(inhom_input && LatticeGenerators.nr_of_rows()!=0 && !exists_element(multi_input_data,Type::offset)){
        vector<Integer> offset(dim);
        offset[dim-1]=1;
        LatticeGenerators.append(offset);
    }
    if(inhom_input &&  Generators.nr_of_rows()!=0 && !exists_element(multi_input_data,Type::vertices) 
                && !exists_element(multi_input_data,Type::polyhedron)){
        vector<Integer> vertex(dim);
        vertex[dim-1]=1;
        Generators.append(vertex);
    }

    if(Inequalities.nr_of_rows()>0 && Generators.nr_of_rows()>0){ // eliminate superfluous inequalities
        vector<key_t> essential;
        for(size_t i=0;i<Inequalities.nr_of_rows();++i){
            for (size_t j=0;j<Generators.nr_of_rows();++j){
                if(v_scalar_product(Inequalities[i],Generators[j])<0){
                    essential.push_back(i);
                    break;
                }
            }
        }
        if(essential.size()<Inequalities.nr_of_rows())
            Inequalities=Inequalities.submatrix(essential);
    }

    // cout << "Ineq " << Inequalities.nr_of_rows() << endl;

    process_lattice_data(LatticeGenerators,Congruences,Equations);

    bool cone_sat_eq=no_lattice_restriction;
    bool cone_sat_cong=no_lattice_restriction;

    // cout << "nolatrest " << no_lattice_restriction << endl;

    if(Inequalities.nr_of_rows()==0 && Generators.nr_of_rows()!=0){
        if(!no_lattice_restriction){
            cone_sat_eq=true;
            for(size_t i=0;i<Generators.nr_of_rows() && cone_sat_eq;++i)
                for(size_t j=0;j<Equations.nr_of_rows()  && cone_sat_eq ;++j)
                    if(v_scalar_product(Generators[i],Equations[j])!=0){
                        cone_sat_eq=false;
            }
        }
        if(!no_lattice_restriction){
            cone_sat_cong=true;
            for(size_t i=0;i<Generators.nr_of_rows() && cone_sat_cong;++i){
                vector<Integer> test=Generators[i];
                test.resize(dim+1);
                for(size_t j=0;j<Congruences.nr_of_rows()  && cone_sat_cong ;++j)
                    if(v_scalar_product(test,Congruences[j]) % Congruences[j][dim] !=0)
                        cone_sat_cong=false;
            }
        }

        if(cone_sat_eq && cone_sat_cong){
            set_original_monoid_generators(Generators);
        }

        if(cone_sat_eq && !cone_sat_cong){ // multiply generators by anniullator mod sublattice
            for(size_t i=0;i<Generators.nr_of_rows();++i)
                v_scalar_multiplication(Generators[i],BasisChange.getAnnihilator());
            cone_sat_cong=true;
        }
    }

    if((Inequalities.nr_of_rows()!=0 || !cone_sat_eq) && Generators.nr_of_rows()!=0){
        Sublattice_Representation<Integer> ConeLatt(Generators,true);
        Full_Cone<Integer> TmpCone(ConeLatt.to_sublattice(Generators));
        TmpCone.dualize_cone();
        Inequalities.append(ConeLatt.from_sublattice_dual(TmpCone.Support_Hyperplanes));
        Generators=Matrix<Integer>(0,dim); // Generators now converted into inequalities
    }

    if(exists_element(multi_input_data,Type::open_facets)){
        // read manual for the computation that follows
        if(!isComputed(ConeProperty::OriginalMonoidGenerators)) // practically impossible, but better to check
            throw BadInputException("Error in connection with open_facets");
        if(Generators.nr_of_rows()!=BasisChange.getRank())
            throw BadInputException("Cone for open_facets not simplicial!");
        Matrix<Integer> TransformedGen=BasisChange.to_sublattice(Generators);
        vector<key_t> key(TransformedGen.nr_of_rows());
        for(size_t j=0;j<TransformedGen.nr_of_rows();++j)
            key[j]=j;
        Matrix<Integer> TransformedSupps;
        Integer dummy;
        TransformedGen.simplex_data(key,TransformedSupps,dummy,false);
        Matrix<Integer> NewSupps=BasisChange.from_sublattice_dual(TransformedSupps);
        NewSupps.remove_row(NewSupps.nr_of_rows()-1); // must remove the inequality for the homogenizing variable
        for(size_t j=0;j<NewSupps.nr_of_rows();++j){
            if(!(multi_input_data[Type::open_facets][0][j]==0 || multi_input_data[Type::open_facets][0][j]==1))
                throw BadInputException("Illegal entry in open_facets");
            NewSupps[j][dim-1]-=multi_input_data[Type::open_facets][0][j];
        }
        NewSupps.append(BasisChange.getEquationsMatrix());
        Matrix<Integer> Ker=NewSupps.kernel(); // gives the new verterx
        // Ker.pretty_print(cout);
        assert(Ker.nr_of_rows()==1);
        Generators[Generators.nr_of_rows()-1]=Ker[0];
    }

    assert(Inequalities.nr_of_rows()==0 || Generators.nr_of_rows()==0);    

    if(Generators.nr_of_rows()==0)
        prepare_input_type_4(Inequalities); // inserts default inequalties if necessary
    else{
        is_Computed.set(ConeProperty::Generators);
        is_Computed.set(ConeProperty::Sublattice); 
    }
    
    checkGrading();
    checkDehomogenization();
    
    if(isComputed(ConeProperty::Grading)) {// cone known to be pointed
        is_Computed.set(ConeProperty::MaximalSubspace);
        pointed=true;
        is_Computed.set(ConeProperty::IsPointed);
    }

    setWeights();  // make matrix of weights for sorting

    if(PreComputedSupportHyperplanes.nr_of_rows()>0){
        check_precomputed_support_hyperplanes();
        SupportHyperplanes=PreComputedSupportHyperplanes;
        is_Computed.set(ConeProperty::SupportHyperplanes);
    }
    
    BasisChangePointed=BasisChange;
    
    is_Computed.set(ConeProperty::IsInhomogeneous);
    is_Computed.set(ConeProperty::EmbeddingDim);


    /* if(ExcludedFaces.nr_of_rows()>0){ // Nothing to check anymore
        check_excluded_faces();
    } */

    /*
    cout <<"-----------------------" << endl;
    cout << "Gen " << endl;
    Generators.pretty_print(cout);
    cout << "Supp " << endl;
    SupportHyperplanes.pretty_print(cout);
    cout << "A" << endl;
    BasisChange.get_A().pretty_print(cout);
    cout << "B" << endl;
    BasisChange.get_B().pretty_print(cout);
    cout <<"-----------------------" << endl;
    */
}


//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::prepare_input_constraints(const map< InputType, vector< vector<Integer> > >& multi_input_data) {

    Matrix<Integer> Signs(0,dim), StrictSigns(0,dim);

    SupportHyperplanes=Matrix<Integer>(0,dim);
    Inequalities=Matrix<Integer>(0,dim);
    Equations=Matrix<Integer>(0,dim);
    Congruences=Matrix<Integer>(0,dim+1);

    typename map< InputType , vector< vector<Integer> > >::const_iterator it=multi_input_data.begin();

    it = multi_input_data.begin();
    for (; it != multi_input_data.end(); ++it) {

        switch (it->first) {
            case Type::strict_inequalities:
            case Type::inequalities:
            case Type::inhom_inequalities:
            case Type::excluded_faces:
                Inequalities.append(it->second);
                break;
            case Type::equations:
            case Type::inhom_equations:
                Equations.append(it->second);
                break;
            case Type::congruences:
            case Type::inhom_congruences:
                Congruences.append(it->second);
                break;
            case Type::signs:
                Signs = sign_inequalities(it->second);
                break;
            case Type::strict_signs:
                StrictSigns = strict_sign_inequalities(it->second);
                break;
            default:
                break;
        }
    }
    if(!BC_set) compose_basis_change(Sublattice_Representation<Integer>(dim));
    Matrix<Integer> Help(Signs);  // signs first !!
    Help.append(StrictSigns);   // then strict signs
    Help.append(Inequalities);
    Inequalities=Help;
}

//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::prepare_input_generators(map< InputType, vector< vector<Integer> > >& multi_input_data, Matrix<Integer>& LatticeGenerators) {

    if(exists_element(multi_input_data,Type::vertices)){
        for(size_t i=0;i<multi_input_data[Type::vertices].size();++i)
            if(multi_input_data[Type::vertices][i][dim-1] <= 0) {
                throw BadInputException("Vertex has non-positive denominator!");
            }
    }

    if(exists_element(multi_input_data,Type::polyhedron)){
        for(size_t i=0;i<multi_input_data[Type::polyhedron].size();++i)
            if(multi_input_data[Type::polyhedron][i][dim-1] < 0) {
                throw BadInputException("Polyhedron vertex has negative denominator!");
            }
    }

    typename map< InputType , vector< vector<Integer> > >::const_iterator it=multi_input_data.begin();
    // find specific generator type -- there is only one, as checked already

    normalization=false;
    
    // check for subspace
    BasisMaxSubspace = find_input_matrix(multi_input_data,Type::subspace);
    if(BasisMaxSubspace.nr_of_rows()==0)
        BasisMaxSubspace=Matrix<Integer>(0,dim);
    
    vector<Integer> neg_sum_subspace(dim,0);
    for(size_t i=0;i<BasisMaxSubspace.nr_of_rows();++i)
        neg_sum_subspace=v_add(neg_sum_subspace,BasisMaxSubspace[i]);
    v_scalar_multiplication<Integer>(neg_sum_subspace,-1);
    

    Generators=Matrix<Integer>(0,dim);
    for(; it != multi_input_data.end(); ++it) {
        switch (it->first) {
            case Type::normalization:
            case Type::cone_and_lattice:
                normalization=true;
                LatticeGenerators.append(it->second);
                if(BasisMaxSubspace.nr_of_rows()>0)
                    LatticeGenerators.append(BasisMaxSubspace);
            case Type::vertices:
            case Type::polyhedron:
            case Type::cone:
            case Type::integral_closure:
                Generators.append(it->second);
                break;
            case Type::subspace:
                Generators.append(it->second);
                Generators.append(neg_sum_subspace);
                break;
            case Type::polytope:
                Generators.append(prepare_input_type_2(it->second));
                break;
            case Type::rees_algebra:
                Generators.append(prepare_input_type_3(it->second));
                break;
            case Type::lattice:
                LatticeGenerators.append(it->second);
                break;
            case Type::saturation:
                LatticeGenerators.append(it->second);
                LatticeGenerators.saturate();
                break;
            case Type::offset:
                if(it->second.size()>1){
                  throw BadInputException("Only one offset allowed!");
                }
                LatticeGenerators.append(it->second);
                break;
            default: break;
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::process_lattice_data(const Matrix<Integer>& LatticeGenerators, Matrix<Integer>& Congruences, Matrix<Integer>& Equations) {

    if(!BC_set)
        compose_basis_change(Sublattice_Representation<Integer>(dim));

    bool no_constraints=(Congruences.nr_of_rows()==0) && (Equations.nr_of_rows()==0);
    bool only_cone_gen=(Generators.nr_of_rows()!=0) && no_constraints && (LatticeGenerators.nr_of_rows()==0);

    no_lattice_restriction=true;

    if(only_cone_gen){
        Sublattice_Representation<Integer> Basis_Change(Generators,true);
        compose_basis_change(Basis_Change);
        return;
    }

    if(normalization && no_constraints){
        Sublattice_Representation<Integer> Basis_Change(Generators,false);
        compose_basis_change(Basis_Change);
        return;
    }

    no_lattice_restriction=false;

    if(Generators.nr_of_rows()!=0){
        Equations.append(Generators.kernel());
    }

    if(LatticeGenerators.nr_of_rows()!=0){
        Sublattice_Representation<Integer> GenSublattice(LatticeGenerators,false);
        if((Equations.nr_of_rows()==0) && (Congruences.nr_of_rows()==0)){
            compose_basis_change(GenSublattice);
            return;
        }
        Congruences.append(GenSublattice.getCongruencesMatrix());
        Equations.append(GenSublattice.getEquationsMatrix());
    }

    if (Congruences.nr_of_rows() > 0) {
        bool zero_modulus;
        Matrix<Integer> Ker_Basis=Congruences.solve_congruences(zero_modulus);
        if(zero_modulus) {
            throw BadInputException("Modulus 0 in congruence!");
        }
        Sublattice_Representation<Integer> Basis_Change(Ker_Basis,false);
        compose_basis_change(Basis_Change);
    }

    if (Equations.nr_of_rows()>0) {
        Matrix<Integer> Ker_Basis=BasisChange.to_sublattice_dual(Equations).kernel();
        Sublattice_Representation<Integer> Basis_Change(Ker_Basis,true);
        compose_basis_change(Basis_Change);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::prepare_input_type_4(Matrix<Integer>& Inequalities) {

    if (!inequalities_present) {
        if (verbose) {
            verboseOutput() << "No inequalities specified in constraint mode, using non-negative orthant." << endl;
        }
        if(inhomogeneous){
            vector<Integer> test(dim);
            test[dim-1]=1;
            size_t matsize=dim;
            if(test==Dehomogenization) // in this case "last coordinate >= 0" will come in through the dehomogenization
                matsize=dim-1;   // we don't check for any other coincidence
            Inequalities= Matrix<Integer>(matsize,dim);
            for(size_t j=0;j<matsize;++j)
                Inequalities[j][j]=1;
        }
        else
            Inequalities = Matrix<Integer>(dim);
    }
    if(inhomogeneous)
        SupportHyperplanes.append(Dehomogenization);
    SupportHyperplanes.append(Inequalities);
}


//---------------------------------------------------------------------------

/* polytope input */
template<typename Integer>
Matrix<Integer> Cone<Integer>::prepare_input_type_2(const vector< vector<Integer> >& Input) {
    size_t j;
    size_t nr = Input.size();
    //append a column of 1
    Matrix<Integer> Generators(nr, dim);
    for (size_t i=0; i<nr; i++) {
        for (j=0; j<dim-1; j++)
            Generators[i][j] = Input[i][j];
        Generators[i][dim-1]=1;
    }
    // use the added last component as grading
    Grading = vector<Integer>(dim,0);
    Grading[dim-1] = 1;
    is_Computed.set(ConeProperty::Grading);
    GradingDenom=1;
    is_Computed.set(ConeProperty::GradingDenom);
    return Generators;
}

//---------------------------------------------------------------------------

/* rees input */
template<typename Integer>
Matrix<Integer> Cone<Integer>::prepare_input_type_3(const vector< vector<Integer> >& InputV) {
    Matrix<Integer> Input(InputV);
    int i,j,k,nr_rows=Input.nr_of_rows(), nr_columns=Input.nr_of_columns();
    // create cone generator matrix
    Matrix<Integer> Full_Cone_Generators(nr_rows+nr_columns,nr_columns+1,0);
    for (i = 0; i < nr_columns; i++) {
        Full_Cone_Generators[i][i]=1;
    }
    for(i=0; i<nr_rows; i++){
        Full_Cone_Generators[i+nr_columns][nr_columns]=1;
        for(j=0; j<nr_columns; j++) {
            Full_Cone_Generators[i+nr_columns][j]=Input[i][j];
        }
    }
    // primarity test
    vector<bool>  Prim_Test(nr_columns,false);
    for (i=0; i<nr_rows; i++) {
        k=0;
        size_t v=0;
        for(j=0; j<nr_columns; j++)
            if (Input[i][j]!=0 ){
                    k++;
                    v=j;
            }
        if(k==1)
            Prim_Test[v]=true;
    }
    rees_primary=true;
    for(i=0; i<nr_columns; i++)
        if(!Prim_Test[i])
            rees_primary=false;

    is_Computed.set(ConeProperty::IsReesPrimary);
    return Full_Cone_Generators;
}


//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::prepare_input_lattice_ideal(map< InputType, vector< vector<Integer> > >& multi_input_data) {

    Matrix<Integer> Binomials(find_input_matrix(multi_input_data,Type::lattice_ideal));

    if (Grading.size()>0) {
        //check if binomials are homogeneous
        vector<Integer> degrees = Binomials.MxV(Grading);
        for (size_t i=0; i<degrees.size(); ++i) {
            if (degrees[i]!=0) {
                throw BadInputException("Grading gives non-zero value "
                        + toString(degrees[i]) + " for binomial "
                        + toString(i+1) + "!");
            }
            if (Grading[i] <0) {
                throw BadInputException("Grading gives negative value "
                        + toString(Grading[i]) + " for generator "
                        + toString(i+1) + "!");
            }
        }
    }

    Matrix<Integer> Gens=Binomials.kernel().transpose();
    Full_Cone<Integer> FC(Gens);
    FC.verbose=verbose;
    if (verbose) verboseOutput() << "Computing a positive embedding..." << endl;

    FC.dualize_cone();
    Matrix<Integer> Supp_Hyp=FC.getSupportHyperplanes().sort_lex();
    Matrix<Integer> Selected_Supp_Hyp_Trans=(Supp_Hyp.submatrix(Supp_Hyp.max_rank_submatrix_lex())).transpose();
    Matrix<Integer> Positive_Embedded_Generators=Gens.multiplication(Selected_Supp_Hyp_Trans);
    // GeneratorsOfToricRing = Positive_Embedded_Generators;
    // is_Computed.set(ConeProperty::GeneratorsOfToricRing);
    dim = Positive_Embedded_Generators.nr_of_columns();
    multi_input_data.insert(make_pair(Type::normalization,Positive_Embedded_Generators.get_elements())); // this is the cone defined by the binomials

    if (Grading.size()>0) {
        // solve GeneratorsOfToricRing * grading = old_grading
        Integer dummyDenom;
        // Grading must be set directly since map entry has been processed already
        Grading = Positive_Embedded_Generators.solve_rectangular(Grading,dummyDenom);
        if (Grading.size() != dim) {
            errorOutput() << "Grading could not be transferred!"<<endl;
            is_Computed.set(ConeProperty::Grading, false);
        }
    }
}

/* only used by the constructors */
template<typename Integer>
void Cone<Integer>::initialize() {
    BC_set=false;
    is_Computed = bitset<ConeProperty::EnumSize>();  //initialized to false
    dim = 0;
    unit_group_index = 1;
    inhomogeneous=false;
    rees_primary = false;
    triangulation_is_nested = false;
    triangulation_is_partial = false;
    is_approximation=false;
    verbose = libnormaliz::verbose; //take the global default
    if (using_GMP<Integer>()) {
        change_integer_type = true;
    } else {
        change_integer_type = false;
    }
    IntHullCone=NULL;
    SymmCone=NULL;
    ProjCone=NULL;
    
    set_parallelization();
    nmz_interrupted=0;
    nmz_scip=false;
    
}

template<typename Integer>
void Cone<Integer>::set_parallelization() {
    
    omp_set_nested(0);
    
    if(thread_limit<0)
        throw BadInputException("Invalid thread limit");
    
    if(parallelization_set){
        if(thread_limit!=0)
            omp_set_num_threads(thread_limit);        
    }
    else{
        if(std::getenv("OMP_NUM_THREADS") == NULL){
            long old=omp_get_max_threads();
            if(old>default_thread_limit)
                set_thread_limit(default_thread_limit);        
            omp_set_num_threads(thread_limit);
        }       
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::compose_basis_change(const Sublattice_Representation<Integer>& BC) {
    if (BC_set) {
        BasisChange.compose(BC);
    } else {
        BasisChange = BC;
        BC_set = true;
    }
}
//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::check_precomputed_support_hyperplanes(){

    if (isComputed(ConeProperty::Generators)) {
        // check if the inequalities are at least valid
        // if (PreComputedSupportHyperplanes.nr_of_rows() != 0) {
            Integer sp;
            for (size_t i = 0; i < Generators.nr_of_rows(); ++i) {
                for (size_t j = 0; j < PreComputedSupportHyperplanes.nr_of_rows(); ++j) {
                    if ((sp = v_scalar_product(Generators[i], PreComputedSupportHyperplanes[j])) < 0) {
                        throw BadInputException("Precomputed inequality " + toString(j)
                                + " is not valid for generator " + toString(i)
                                + " (value " + toString(sp) + ")");
                    }
                }
            }
        // }
    }
}

//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::check_excluded_faces(){

    if (isComputed(ConeProperty::Generators)) {
        // check if the inequalities are at least valid
        // if (ExcludedFaces.nr_of_rows() != 0) {
            Integer sp;
            for (size_t i = 0; i < Generators.nr_of_rows(); ++i) {
                for (size_t j = 0; j < ExcludedFaces.nr_of_rows(); ++j) {
                    if ((sp = v_scalar_product(Generators[i], ExcludedFaces[j])) < 0) {
                        throw BadInputException("Excluded face " + toString(j)
                                + " is not valid for generator " + toString(i)
                                + " (value " + toString(sp) + ")");
                    }
                }
            }
        // }
    }
}


//---------------------------------------------------------------------------

template<typename Integer>
bool Cone<Integer>::setVerbose (bool v) {
    //we want to return the old value
    bool old = verbose;
    verbose = v;
    return old;
}
//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::deactivateChangeOfPrecision() {
    change_integer_type = false;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::checkGrading () {
    
    if (isComputed(ConeProperty::Grading) || Grading.size()==0) {
        return;
    }
    
    bool positively_graded=true;
    bool nonnegative=true;
    size_t neg_index=0;
    Integer neg_value;
    if (Generators.nr_of_rows() > 0) {
        vector<Integer> degrees = Generators.MxV(Grading);
        for (size_t i=0; i<degrees.size(); ++i) {
            if (degrees[i]<=0 && (!inhomogeneous || v_scalar_product(Generators[i],Dehomogenization)==0)) { 
                // in the inhomogeneous case: test only generators of tail cone
                positively_graded=false;;
                if(degrees[i]<0){
                    nonnegative=false;
                    neg_index=i;
                    neg_value=degrees[i];
                }
            }
        }
        if(positively_graded){
            vector<Integer> test_grading=BasisChange.to_sublattice_dual_no_div(Grading);
            GradingDenom=v_make_prime(test_grading);
        }
        else
            GradingDenom = 1; 
    } else {
        GradingDenom = 1;
    }

    if (isComputed(ConeProperty::Generators)){        
        if(!nonnegative){
            throw BadInputException("Grading gives negative value "
                    + toString(neg_value) + " for generator "
                    + toString(neg_index+1) + "!");
        }
        if(positively_graded){
            is_Computed.set(ConeProperty::Grading);
            is_Computed.set(ConeProperty::GradingDenom);            
        }
    }
    
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::checkDehomogenization () {
    if(Dehomogenization.size()>0){
        vector<Integer> test=Generators.MxV(Dehomogenization);
        for(size_t i=0;i<test.size();++i)
            if(test[i]<0){
                throw BadInputException(
                        "Dehomogenization has has negative value on generator "
                        + toString(Generators[i]));
            }
    }
}
//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::setGrading (const vector<Integer>& lf) {
    
    if (isComputed(ConeProperty::Grading) && Grading == lf) {
        return;
    }
    
    if (lf.size() != dim) {
        throw BadInputException("Grading linear form has wrong dimension "
                + toString(lf.size()) + " (should be " + toString(dim) + ")");
    }
    
    Grading = lf;
    checkGrading();
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::setWeights () {

    if(WeightsGrad.nr_of_columns()!=dim){
        WeightsGrad=Matrix<Integer> (0,dim);  // weight matrix for ordering
    }
    if(Grading.size()>0 && WeightsGrad.nr_of_rows()==0)
        WeightsGrad.append(Grading);
    GradAbs=vector<bool>(WeightsGrad.nr_of_rows(),false);
}
//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::setDehomogenization (const vector<Integer>& lf) {
    if (lf.size() != dim) {
        throw BadInputException("Dehomogenizing linear form has wrong dimension "
                + toString(lf.size()) + " (should be " + toString(dim) + ")");
    }
    Dehomogenization=lf;
    is_Computed.set(ConeProperty::Dehomogenization);
}

//---------------------------------------------------------------------------

/* check what is computed */
template<typename Integer>
bool Cone<Integer>::isComputed(ConeProperty::Enum prop) const {
    return is_Computed.test(prop);
}

template<typename Integer>
bool Cone<Integer>::isComputed(ConeProperties CheckComputed) const {
    return CheckComputed.reset(is_Computed).any();
}

template<typename Integer>
void Cone<Integer>::resetComputed(ConeProperty::Enum prop){
    is_Computed.reset(prop);
}


/* getter */

template<typename Integer>
Cone<Integer>& Cone<Integer>::getIntegerHullCone() const {
    return *IntHullCone;
}

template<typename Integer>
Cone<Integer>& Cone<Integer>::getProjectCone() const {
    return *ProjCone;
}

template<typename Integer>
Cone<Integer>& Cone<Integer>::getSymmetrizedCone() const {
    return *SymmCone;
}

template<typename Integer>
size_t Cone<Integer>::getRank() {
    compute(ConeProperty::Sublattice);
    return BasisChange.getRank();
}

template<typename Integer>
size_t Cone<Integer>::get_rank_internal() {
    if(!isComputed(ConeProperty::Sublattice))
        compute(ConeProperty::Sublattice);
    return BasisChange.getRank();
}

template<typename Integer>    // computation depends on OriginalMonoidGenerators
Integer Cone<Integer>::getIndex() {
    compute(ConeProperty::OriginalMonoidGenerators);
    return index;
}

template<typename Integer>    // computation depends on OriginalMonoidGenerators
Integer Cone<Integer>::getInternalIndex() {
    return getIndex();
}

template<typename Integer>
Integer Cone<Integer>::getUnitGroupIndex() {
    compute(ConeProperty::OriginalMonoidGenerators,ConeProperty::IsIntegrallyClosed);
    return unit_group_index;
}

template<typename Integer>
size_t Cone<Integer>::getRecessionRank() {
    compute(ConeProperty::RecessionRank);
    return recession_rank;
}

template<typename Integer>
long Cone<Integer>::getAffineDim() {
    compute(ConeProperty::AffineDim);
    return affine_dim;
}

template<typename Integer>
const Sublattice_Representation<Integer>& Cone<Integer>::getSublattice() {
    compute(ConeProperty::Sublattice);
    return BasisChange;
}

template<typename Integer>
const Sublattice_Representation<Integer>& Cone<Integer>::get_sublattice_internal() {
    if(!isComputed(ConeProperty::Sublattice))
        compute(ConeProperty::Sublattice);
    return BasisChange;
}

template<typename Integer>
const Matrix<Integer>& Cone<Integer>::getOriginalMonoidGeneratorsMatrix() {
    compute(ConeProperty::OriginalMonoidGenerators);
    return OriginalMonoidGenerators;
}
template<typename Integer>
const vector< vector<Integer> >& Cone<Integer>::getOriginalMonoidGenerators() {
    compute(ConeProperty::OriginalMonoidGenerators);
    return OriginalMonoidGenerators.get_elements();
}
template<typename Integer>
size_t Cone<Integer>::getNrOriginalMonoidGenerators() {
    compute(ConeProperty::OriginalMonoidGenerators);
    return OriginalMonoidGenerators.nr_of_rows();
}

template<typename Integer>
const vector< vector<Integer> >& Cone<Integer>::getMaximalSubspace() {
    compute(ConeProperty::MaximalSubspace);
    return BasisMaxSubspace.get_elements();
}
template<typename Integer>
const Matrix<Integer>& Cone<Integer>::getMaximalSubspaceMatrix() {
    compute(ConeProperty::MaximalSubspace);
    return BasisMaxSubspace;
}
template<typename Integer>
size_t Cone<Integer>::getDimMaximalSubspace() {
    compute(ConeProperty::MaximalSubspace);
    return BasisMaxSubspace.nr_of_rows();
}

template<typename Integer>
const Matrix<Integer>& Cone<Integer>::getGeneratorsMatrix() {
    compute(ConeProperty::Generators);
    return Generators;
}

template<typename Integer>
const vector< vector<Integer> >& Cone<Integer>::getGenerators() {
    compute(ConeProperty::Generators);
    return Generators.get_elements();
}

template<typename Integer>
size_t Cone<Integer>::getNrGenerators() {
    compute(ConeProperty::Generators);
    return Generators.nr_of_rows();
}

template<typename Integer>
const Matrix<Integer>& Cone<Integer>::getExtremeRaysMatrix() {
    compute(ConeProperty::ExtremeRays);
    return ExtremeRays;
}
template<typename Integer>
const vector< vector<Integer> >& Cone<Integer>::getExtremeRays() {
    compute(ConeProperty::ExtremeRays);
    return ExtremeRays.get_elements();
}
template<typename Integer>
size_t Cone<Integer>::getNrExtremeRays() {
    compute(ConeProperty::ExtremeRays);
    return ExtremeRays.nr_of_rows();
}

template<typename Integer>
const Matrix<nmz_float>& Cone<Integer>::getVerticesFloatMatrix() {
    compute(ConeProperty::VerticesFloat);
    return VerticesFloat;
}
template<typename Integer>
const vector< vector<nmz_float> >& Cone<Integer>::getVerticesFloat() {
    compute(ConeProperty::VerticesFloat);
    return VerticesFloat.get_elements();
}
template<typename Integer>
size_t Cone<Integer>::getNrVerticesFloat() {
    compute(ConeProperty::VerticesFloat);
    return VerticesFloat.nr_of_rows();
}

template<typename Integer>
const Matrix<Integer>& Cone<Integer>::getVerticesOfPolyhedronMatrix() {
    compute(ConeProperty::VerticesOfPolyhedron);
    return VerticesOfPolyhedron;
}
template<typename Integer>
const vector< vector<Integer> >& Cone<Integer>::getVerticesOfPolyhedron() {
    compute(ConeProperty::VerticesOfPolyhedron);
    return VerticesOfPolyhedron.get_elements();
}
template<typename Integer>
size_t Cone<Integer>::getNrVerticesOfPolyhedron() {
    compute(ConeProperty::VerticesOfPolyhedron);
    return VerticesOfPolyhedron.nr_of_rows();
}

template<typename Integer>
const Matrix<Integer>& Cone<Integer>::getSupportHyperplanesMatrix() {
    compute(ConeProperty::SupportHyperplanes);
    return SupportHyperplanes;
}
template<typename Integer>
const vector< vector<Integer> >& Cone<Integer>::getSupportHyperplanes() {
    compute(ConeProperty::SupportHyperplanes);
    return SupportHyperplanes.get_elements();
}
template<typename Integer>
size_t Cone<Integer>::getNrSupportHyperplanes() {
    compute(ConeProperty::SupportHyperplanes);
    return SupportHyperplanes.nr_of_rows();
}

template<typename Integer>
map< InputType , vector< vector<Integer> > > Cone<Integer>::getConstraints () {
    compute(ConeProperty::Sublattice, ConeProperty::SupportHyperplanes);
    map<InputType, vector< vector<Integer> > > c;
    c[Type::inequalities] = SupportHyperplanes.get_elements();
    c[Type::equations] = BasisChange.getEquations();
    c[Type::congruences] = BasisChange.getCongruences();
    return c;
}

template<typename Integer>
const Matrix<Integer>& Cone<Integer>::getExcludedFacesMatrix() {
    compute(ConeProperty::ExcludedFaces);
    return ExcludedFaces;
}
template<typename Integer>
const vector< vector<Integer> >& Cone<Integer>::getExcludedFaces() {
    compute(ConeProperty::ExcludedFaces);
    return ExcludedFaces.get_elements();
}
template<typename Integer>
size_t Cone<Integer>::getNrExcludedFaces() {
    compute(ConeProperty::ExcludedFaces);
    return ExcludedFaces.nr_of_rows();
}

template<typename Integer>
const vector< pair<vector<key_t>,Integer> >& Cone<Integer>::getTriangulation() {
    compute(ConeProperty::Triangulation);
    return Triangulation;
}

template<typename Integer>
const vector<vector<bool> >& Cone<Integer>::getOpenFacets() {
    compute(ConeProperty::ConeDecomposition);
    return OpenFacets;
}

template<typename Integer>
const vector< pair<vector<key_t>,long> >& Cone<Integer>::getInclusionExclusionData() {
    compute(ConeProperty::InclusionExclusionData);
    return InExData;
}

template<typename Integer>
void Cone<Integer>::make_StanleyDec_export() {
    if(!StanleyDec_export.empty())
        return;
    assert(isComputed(ConeProperty::StanleyDec));
    auto SD=StanleyDec.begin();
    for(;SD!=StanleyDec.end();++SD){
        STANLEYDATA<Integer> NewSt;
        NewSt.key=SD->key;
        convert(NewSt.offsets,SD->offsets);
        StanleyDec_export.push_back(NewSt);        
    }    
}

template<typename Integer>
const list< STANLEYDATA<Integer> >& Cone<Integer>::getStanleyDec() {
    compute(ConeProperty::StanleyDec);
    make_StanleyDec_export();
    return StanleyDec_export;
}

template<typename Integer>
list< STANLEYDATA_int >& Cone<Integer>::getStanleyDec_mutable() {
    assert(isComputed(ConeProperty::StanleyDec));
    return StanleyDec;
}

template<typename Integer>
size_t Cone<Integer>::getTriangulationSize() {
    compute(ConeProperty::TriangulationSize);
    return TriangulationSize;
}

template<typename Integer>
Integer Cone<Integer>::getTriangulationDetSum() {
    compute(ConeProperty::TriangulationDetSum);
    return TriangulationDetSum;
}

template<typename Integer>
vector<Integer> Cone<Integer>::getWitnessNotIntegrallyClosed() {
    compute(ConeProperty::WitnessNotIntegrallyClosed);
    return WitnessNotIntegrallyClosed;
}

template<typename Integer>
vector<Integer> Cone<Integer>::getGeneratorOfInterior() {
    compute(ConeProperty::GeneratorOfInterior);
    return GeneratorOfInterior;
}

template<typename Integer>
const Matrix<Integer>& Cone<Integer>::getHilbertBasisMatrix() {
    compute(ConeProperty::HilbertBasis);
    return HilbertBasis;
}
template<typename Integer>
const vector< vector<Integer> >& Cone<Integer>::getHilbertBasis() {
    compute(ConeProperty::HilbertBasis);
    return HilbertBasis.get_elements();
}
template<typename Integer>
size_t Cone<Integer>::getNrHilbertBasis() {
    compute(ConeProperty::HilbertBasis);
    return HilbertBasis.nr_of_rows();
}

template<typename Integer>
const Matrix<Integer>& Cone<Integer>::getModuleGeneratorsOverOriginalMonoidMatrix() {
    compute(ConeProperty::ModuleGeneratorsOverOriginalMonoid);
    return ModuleGeneratorsOverOriginalMonoid;
}
template<typename Integer>
const vector< vector<Integer> >& Cone<Integer>::getModuleGeneratorsOverOriginalMonoid() {
    compute(ConeProperty::ModuleGeneratorsOverOriginalMonoid);
    return ModuleGeneratorsOverOriginalMonoid.get_elements();
}
template<typename Integer>
size_t Cone<Integer>::getNrModuleGeneratorsOverOriginalMonoid() {
    compute(ConeProperty::ModuleGeneratorsOverOriginalMonoid);
    return ModuleGeneratorsOverOriginalMonoid.nr_of_rows();
}

template<typename Integer>
const Matrix<Integer>& Cone<Integer>::getModuleGeneratorsMatrix() {
    compute(ConeProperty::ModuleGenerators);
    return ModuleGenerators;
}
template<typename Integer>
const vector< vector<Integer> >& Cone<Integer>::getModuleGenerators() {
    compute(ConeProperty::ModuleGenerators);
    return ModuleGenerators.get_elements();
}
template<typename Integer>
size_t Cone<Integer>::getNrModuleGenerators() {
    compute(ConeProperty::ModuleGenerators);
    return ModuleGenerators.nr_of_rows();
}

template<typename Integer>
const Matrix<Integer>& Cone<Integer>::getDeg1ElementsMatrix() {
    compute(ConeProperty::Deg1Elements);
    return Deg1Elements;
}
template<typename Integer>
const vector< vector<Integer> >& Cone<Integer>::getDeg1Elements() {
    compute(ConeProperty::Deg1Elements);
    return Deg1Elements.get_elements();
}
template<typename Integer>
size_t Cone<Integer>::getNrDeg1Elements() {
    compute(ConeProperty::Deg1Elements);
    return Deg1Elements.nr_of_rows();
}

template<typename Integer>
const HilbertSeries& Cone<Integer>::getHilbertSeries() {
    compute(ConeProperty::HilbertSeries);
    return HSeries;
}

template<typename Integer>
vector<Integer> Cone<Integer>::getGrading() {
    compute(ConeProperty::Grading);
    return Grading;
}

template<typename Integer>
Integer Cone<Integer>::getGradingDenom() {
    compute(ConeProperty::Grading);
    return GradingDenom;
}

template<typename Integer>
vector<Integer> Cone<Integer>::getDehomogenization() {
    compute(ConeProperty::Dehomogenization);
    return Dehomogenization;
}

template<typename Integer>
mpq_class Cone<Integer>::getMultiplicity() {
    compute(ConeProperty::Multiplicity);
    return multiplicity;
}

template<typename Integer>
mpq_class Cone<Integer>::getVolume() {
    compute(ConeProperty::Volume);
    return volume;
}

template<typename Integer>
nmz_float Cone<Integer>::getEuclideanVolume() {
    compute(ConeProperty::Volume);
    return euclidean_volume;
}

template<typename Integer>
mpq_class Cone<Integer>::getVirtualMultiplicity() {
    if(!isComputed(ConeProperty::VirtualMultiplicity)) // in order not to compute the triangulation
        compute(ConeProperty::VirtualMultiplicity);    // which is deleted if not asked for explicitly
    return IntData.getVirtualMultiplicity();
}

template<typename Integer>
const pair<HilbertSeries, mpz_class>& Cone<Integer>::getWeightedEhrhartSeries(){
    if(!isComputed(ConeProperty::WeightedEhrhartSeries))  // see above
        compute(ConeProperty::WeightedEhrhartSeries);
    return getIntData().getWeightedEhrhartSeries();
}

template<typename Integer>
IntegrationData& Cone<Integer>::getIntData(){
    return IntData;
}

template<typename Integer>
mpq_class Cone<Integer>::getIntegral() {
    if(!isComputed(ConeProperty::Integral)) // see above
        compute(ConeProperty::Integral);
    return IntData.getIntegral();
}

template<typename Integer>
bool Cone<Integer>::isPointed() {
    compute(ConeProperty::IsPointed);
    return pointed;
}

template<typename Integer>
bool Cone<Integer>::isInhomogeneous() {
    return inhomogeneous;
}

template<typename Integer>
bool Cone<Integer>::isDeg1ExtremeRays() {
    compute(ConeProperty::IsDeg1ExtremeRays);
    return deg1_extreme_rays;
}

template<typename Integer>
bool Cone<Integer>::isGorenstein() {
    compute(ConeProperty::IsGorenstein);
    return Gorenstein;
}

template<typename Integer>
bool Cone<Integer>::isDeg1HilbertBasis() {
    compute(ConeProperty::IsDeg1HilbertBasis);
    return deg1_hilbert_basis;
}

template<typename Integer>
bool Cone<Integer>::isIntegrallyClosed() {
    compute(ConeProperty::IsIntegrallyClosed);
    return integrally_closed;
}

template<typename Integer>
bool Cone<Integer>::isReesPrimary() {
    compute(ConeProperty::IsReesPrimary);
    return rees_primary;
}

template<typename Integer>
Integer Cone<Integer>::getReesPrimaryMultiplicity() {
    compute(ConeProperty::ReesPrimaryMultiplicity);
    return ReesPrimaryMultiplicity;
}

// the information about the triangulation will just be returned
// if no triangulation was computed so far they return false
template<typename Integer>
bool Cone<Integer>::isTriangulationNested() {
    return triangulation_is_nested;
}
template<typename Integer>
bool Cone<Integer>::isTriangulationPartial() {
    return triangulation_is_partial;
}

template<typename Integer>
size_t Cone<Integer>::getModuleRank() {
    compute(ConeProperty::ModuleRank);
    return module_rank;
}

template<typename Integer>
vector<Integer> Cone<Integer>::getClassGroup() {
    compute(ConeProperty::ClassGroup);
    return ClassGroup;
}

//---------------------------------------------------------------------------

template<typename Integer>
ConeProperties Cone<Integer>::compute(ConeProperty::Enum cp) {

    if (isComputed(cp)) 
        return ConeProperties();
    return compute(ConeProperties(cp));
}

template<typename Integer>
ConeProperties Cone<Integer>::compute(ConeProperty::Enum cp1, ConeProperty::Enum cp2) {

    if (isComputed(cp1) &&  isComputed(cp2)) 
        return ConeProperties();
    return compute(ConeProperties(cp1,cp2));
}

template<typename Integer>
ConeProperties Cone<Integer>::compute(ConeProperty::Enum cp1, ConeProperty::Enum cp2,
                                      ConeProperty::Enum cp3) {

    if (isComputed(cp1) &&  isComputed(cp2)  &&  isComputed(cp3))
        return ConeProperties();
    return compute(ConeProperties(cp1,cp2,cp3));
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::set_implicit_dual_mode(ConeProperties& ToCompute) {
      
    if(ToCompute.test(ConeProperty::DualMode) || ToCompute.test(ConeProperty::PrimalMode)
                    || ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid)
                    || ToCompute.test(ConeProperty::Approximate)
                    || ToCompute.test(ConeProperty::Projection)
                    || nr_cone_gen>0 || nr_latt_gen>0 || SupportHyperplanes.nr_of_rows() > 2*dim
                    || SupportHyperplanes.nr_of_rows() 
                            <= BasisChangePointed.getRank()+ 50/(BasisChangePointed.getRank()+1))
        return;
    if(ToCompute.test(ConeProperty::HilbertBasis))
        ToCompute.set(ConeProperty::DualMode);
    if(ToCompute.test(ConeProperty::Deg1Elements) 
            && !(ToCompute.test(ConeProperty::HilbertSeries) || ToCompute.test(ConeProperty::Multiplicity)))
        ToCompute.set(ConeProperty::DualMode);
    return;
}

//---------------------------------------------------------------------------

template<typename Integer>
ConeProperties Cone<Integer>::compute(ConeProperties ToCompute) {

    set_parallelization();
    nmz_interrupted=0;
    if(ToCompute.test(ConeProperty::SCIP)){
#ifdef NMZ_SCIP
        nmz_scip=true;
        ToCompute.set(ConeProperty::SCIP);
#else
        throw BadInputException("Option SCIP only allowed if Normaliz was built with Scip");
#endif // NMZ_SCIP
    }
    
    if(ToCompute.test(ConeProperty::NoPeriodBound)){
        HSeries.set_period_bounded(false);
        IntData.getWeightedEhrhartSeries().first.set_period_bounded(false);        
    }
        
    
#ifndef NMZ_COCOA
   if(ToCompute.test(ConeProperty::VirtualMultiplicity) || ToCompute.test(ConeProperty::Integral) 
       || ToCompute.test(ConeProperty::WeightedEhrhartSeries))
       throw BadInputException("Integral, VirtualMultiplicity, WeightedEhrhartSeries only computable with CoCoALib");
#endif

    // default_mode=ToCompute.test(ConeProperty::DefaultMode);
    
    if(ToCompute.test(ConeProperty::BigInt)){
        if(!using_GMP<Integer>())
            throw BadInputException("BigInt can only be set for cones of Integer type GMP");
        change_integer_type=false;
    }
    
    if(ToCompute.test(ConeProperty::KeepOrder) && !isComputed(ConeProperty::OriginalMonoidGenerators))
        throw BadInputException("KeepOrder can only be set if OriginalMonoidGenerators are defined");
    
    INTERRUPT_COMPUTATION_BY_EXCEPTION
    
    if(BasisMaxSubspace.nr_of_rows()>0 && !isComputed(ConeProperty::MaximalSubspace)){
        BasisMaxSubspace=Matrix<Integer>(0,dim);
        compute(ConeProperty::MaximalSubspace);      
    }
    
    // if(ToCompute.test(ConeProperty::ExplicitHilbertSeries))
    //    throw BadInputException("ExplicitHilbertSeries only allowed for internal control");
    
    if(ToCompute.test(ConeProperty::HilbertSeries) || ToCompute.test(ConeProperty::HSOP))
        ToCompute.set(ConeProperty::ExplicitHilbertSeries);
    // must distiguish it from being set through DefaultMode;
    if(ToCompute.test(ConeProperty::DualMode) 
                && !(ToCompute.test(ConeProperty::HilbertBasis) || ToCompute.test(ConeProperty::Deg1Elements)))
        ToCompute.set(ConeProperty::NakedDual);
    // to control the computation of rational solutions in the inhomogeneous case
    
    ToCompute.reset(is_Computed);
    ToCompute.check_conflicting_variants();
    ToCompute.set_preconditions(inhomogeneous);
    ToCompute.prepare_compute_options(inhomogeneous);
    ToCompute.check_sanity(inhomogeneous);
    if (!isComputed(ConeProperty::OriginalMonoidGenerators)) {
        if (ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid)) {
            errorOutput() << "ERROR: Module generators over original monoid only computable if original monoid is defined!"
                << endl;
            throw NotComputableException(ConeProperty::ModuleGeneratorsOverOriginalMonoid);
        }
        if (ToCompute.test(ConeProperty::IsIntegrallyClosed)) {
            errorOutput() << "ERROR: Original monoid is not defined, cannot check it for being integrally closed."
                << endl;
            throw NotComputableException(ConeProperty::IsIntegrallyClosed);
        }
    }
    
    try_symmetrization(ToCompute);   
    ToCompute.reset(is_Computed);
    if (ToCompute.none()) {
        return ToCompute;
    }
    
    INTERRUPT_COMPUTATION_BY_EXCEPTION
    
    compute_projection(ToCompute);
    
    INTERRUPT_COMPUTATION_BY_EXCEPTION
    
    try_approximation_or_projection(ToCompute);
    
    ToCompute.reset(is_Computed); // already computed
    if (ToCompute.none()) {
        return ToCompute;
    }
    
    INTERRUPT_COMPUTATION_BY_EXCEPTION
    
    set_implicit_dual_mode(ToCompute);

    if (ToCompute.test(ConeProperty::DualMode)) {
        compute_dual(ToCompute);
    }

    if (ToCompute.test(ConeProperty::WitnessNotIntegrallyClosed)) {
        find_witness();
    }

    ToCompute.reset(is_Computed);
    if (ToCompute.none()) { 
        return ToCompute;
    }
    
    INTERRUPT_COMPUTATION_BY_EXCEPTION

    /* preparation: get generators if necessary */
    compute_generators();

    if (!isComputed(ConeProperty::Generators)) {
        throw FatalException("Could not get Generators.");
    }

    if (rees_primary && (ToCompute.test(ConeProperty::ReesPrimaryMultiplicity)
            || ToCompute.test(ConeProperty::Multiplicity)
            || ToCompute.test(ConeProperty::HilbertSeries)
            || ToCompute.test(ConeProperty::DefaultMode) ) ) {
        ReesPrimaryMultiplicity = compute_primary_multiplicity();
        is_Computed.set(ConeProperty::ReesPrimaryMultiplicity);
    }

    ToCompute.reset(is_Computed); // already computed
    if (ToCompute.none()) {
        return ToCompute;
    }

    // the computation of the full cone
    if (change_integer_type) {
        try {
            compute_full_cone<MachineInteger>(ToCompute);
        } catch(const ArithmeticException& e) {
            if (verbose) {
                verboseOutput() << e.what() << endl;
                verboseOutput() << "Restarting with a bigger type." << endl;
            }
            change_integer_type = false;
        }
    }
    
    if (!change_integer_type) {
        compute_full_cone<Integer>(ToCompute);
    }
    
    compute_volume(ToCompute);
    
    check_Gorenstein(ToCompute);
    
    if(ToCompute.test(ConeProperty::IntegerHull)) {
        compute_integer_hull();
    }
    
    INTERRUPT_COMPUTATION_BY_EXCEPTION
    
    complete_HilbertSeries_comp(ToCompute);
    
    complete_sublattice_comp(ToCompute);
    
    compute_vertices_float(ToCompute);
       
    if(ToCompute.test(ConeProperty::WeightedEhrhartSeries))
        compute_weighted_Ehrhart(ToCompute);
    ToCompute.reset(is_Computed);
    
    if(ToCompute.test(ConeProperty::Integral))
        compute_integral(ToCompute);
    ToCompute.reset(is_Computed);
    
    if(ToCompute.test(ConeProperty::VirtualMultiplicity))
        compute_virt_mult(ToCompute);
    ToCompute.reset(is_Computed);

    /* check if everything is computed */
    ToCompute.reset(is_Computed); //remove what is now computed
    if (ToCompute.test(ConeProperty::Deg1Elements) && isComputed(ConeProperty::Grading)) {
        // this can happen when we were looking for a witness earlier
        compute(ToCompute);
    }
    if (!ToCompute.test(ConeProperty::DefaultMode) && ToCompute.goals().any()) {
        throw NotComputableException(ToCompute.goals());
    }
    ToCompute.reset_compute_options();
    return ToCompute;
}

template<typename Integer>
void Cone<Integer>::compute_integer_hull() {
    
    if(verbose){
        verboseOutput() << "Computing integer hull" << endl;
    }

    Matrix<Integer> IntHullGen;
    bool IntHullComputable=true;
    size_t nr_extr=0;
    if(inhomogeneous){
        if(!isComputed(ConeProperty::HilbertBasis))
            IntHullComputable=false;
        IntHullGen=HilbertBasis;
        IntHullGen.append(ModuleGenerators);
    }
    else{
        if(!isComputed(ConeProperty::Deg1Elements))
            IntHullComputable=false;
        IntHullGen=Deg1Elements;
    }
    ConeProperties IntHullCompute;
    IntHullCompute.set(ConeProperty::SupportHyperplanes);
    if(!IntHullComputable){
        errorOutput() << "Integer hull not computable: no integer points available." << endl;
        throw NotComputableException(IntHullCompute);
    }
    
    if(IntHullGen.nr_of_rows()==0){
        IntHullGen.append(vector<Integer>(dim,0)); // we need a non-empty input matrix
    }
    
    INTERRUPT_COMPUTATION_BY_EXCEPTION
    
    if(!inhomogeneous || HilbertBasis.nr_of_rows()==0){ // polytoe since homogeneous or recession coe = 0
        nr_extr=IntHullGen.extreme_points_first(); // don't need a norm here since all points have degree or level 1
    }
    else{ // now an unbounded polyhedron
        if(isComputed(ConeProperty::Grading)){
            nr_extr=IntHullGen.extreme_points_first(Grading);
        }
        else{
            if(isComputed(ConeProperty::SupportHyperplanes)){
                vector<Integer> aux_grading=SupportHyperplanes.find_inner_point();
                nr_extr=IntHullGen.extreme_points_first(aux_grading);
            }    
        }
    }
    
    if(verbose){
        verboseOutput() << nr_extr << " extreme points found"  << endl;
    }
    
    // IntHullGen.pretty_print(cout);
    IntHullCone=new Cone<Integer>(InputType::cone_and_lattice,IntHullGen.get_elements(), Type::subspace,BasisMaxSubspace);
    if(nr_extr!=0)  // we suppress the ordering in full_cone only if we have found few extreme rays
        IntHullCompute.set(ConeProperty::KeepOrder);

    IntHullCone->inhomogeneous=true; // inhomogeneous;
    if(inhomogeneous)
        IntHullCone->Dehomogenization=Dehomogenization;
    else
        IntHullCone->Dehomogenization=Grading;
    IntHullCone->verbose=verbose;
    try{
        IntHullCone->compute(IntHullCompute);
        if(IntHullCone->isComputed(ConeProperty::SupportHyperplanes))
            is_Computed.set(ConeProperty::IntegerHull);
        if(verbose){
            verboseOutput() << "Integer hull finished" << endl;
        }
    }
    catch (const NotComputableException& ){
            errorOutput() << "Error in computation of integer hull" << endl;
    }
}

template<typename Integer>
void Cone<Integer>::check_vanishing_of_grading_and_dehom(){
    if(Grading.size()>0){
        vector<Integer> test=BasisMaxSubspace.MxV(Grading);
        if(test!=vector<Integer>(test.size())){
                throw BadInputException("Grading does not vanish on maximal subspace.");
        }
    }
    if(Dehomogenization.size()>0){
        vector<Integer> test=BasisMaxSubspace.MxV(Dehomogenization);
        if(test!=vector<Integer>(test.size())){
            throw BadInputException("Dehomogenization does not vanish on maximal subspace.");
        }
    }    
}

template<typename Integer>
template<typename IntegerFC>
void Cone<Integer>::compute_full_cone(ConeProperties& ToCompute) {
    
    if(ToCompute.test(ConeProperty::IsPointed) && Grading.size()==0){
        if (verbose) {
            verboseOutput()<<  "Checking pointedness first"<< endl;
        }
        ConeProperties Dualize;
        Dualize.set(ConeProperty::SupportHyperplanes);
        Dualize.set(ConeProperty::ExtremeRays);
        compute(Dualize);
    }
    
    Matrix<IntegerFC> FC_Gens;

    BasisChangePointed.convert_to_sublattice(FC_Gens, Generators);
    Full_Cone<IntegerFC> FC(FC_Gens,!ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid));
    // !ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid) blocks make_prime in full_cone.cpp

    /* activate bools in FC */

    FC.verbose=verbose;

    FC.inhomogeneous=inhomogeneous;
    FC.explicit_h_vector=(ToCompute.test(ConeProperty::ExplicitHilbertSeries) && !isComputed(ConeProperty::HilbertSeries));

    if (ToCompute.test(ConeProperty::HilbertSeries)) {
        FC.do_h_vector = true;
        FC.Hilbert_Series.set_period_bounded(HSeries.get_period_bounded());
    }
    if (ToCompute.test(ConeProperty::HilbertBasis)) {
        FC.do_Hilbert_basis = true;
    }
    if (ToCompute.test(ConeProperty::IsIntegrallyClosed)) {
        FC.do_integrally_closed = true;
    }
    if (ToCompute.test(ConeProperty::Triangulation)) {
        FC.keep_triangulation = true;
    }
    if (ToCompute.test(ConeProperty::ConeDecomposition)) {
        FC.do_cone_dec = true;
    }
    if (ToCompute.test(ConeProperty::Multiplicity) ) {
        FC.do_multiplicity = true;
    }
    if (ToCompute.test(ConeProperty::TriangulationDetSum) ) {
        FC.do_determinants = true;
    }
    if (ToCompute.test(ConeProperty::TriangulationSize)) {
        FC.do_triangulation = true;
    }
    if (ToCompute.test(ConeProperty::NoSubdivision)) {
        FC.use_bottom_points = false;
    }
    if (ToCompute.test(ConeProperty::Deg1Elements)) {
        FC.do_deg1_elements = true;
    }
    if (ToCompute.test(ConeProperty::StanleyDec)) {
        FC.do_Stanley_dec = true;
    }
    if (ToCompute.test(ConeProperty::Approximate)
     && ToCompute.test(ConeProperty::Deg1Elements)) {
        FC.do_approximation = true;
        FC.do_deg1_elements = true;
    }
    if (ToCompute.test(ConeProperty::DefaultMode)) {
        FC.do_default_mode = true;
    }
    if (ToCompute.test(ConeProperty::BottomDecomposition)) {
        FC.do_bottom_dec = true;
    }
    if (ToCompute.test(ConeProperty::NoBottomDec)) {
        FC.suppress_bottom_dec = true;
    }
    if (ToCompute.test(ConeProperty::KeepOrder)) {
        FC.keep_order = true;
    }
    if (ToCompute.test(ConeProperty::ClassGroup)) {
        FC.do_class_group=true;
    }
    if (ToCompute.test(ConeProperty::ModuleRank)) {
        FC.do_module_rank=true;
    }
    
    if (ToCompute.test(ConeProperty::HSOP)) {
        FC.do_hsop=true;
    }
    
    /* Give extra data to FC */
    if ( isComputed(ConeProperty::ExtremeRays) ) {
        FC.Extreme_Rays_Ind = ExtremeRaysIndicator;
        FC.is_Computed.set(ConeProperty::ExtremeRays);
    }
    
    /* if(isComputed(ConeProperty::Deg1Elements)){
        Matrix<IntegerFC> Deg1Converted;
        BasisChangePointed.convert_to_sublattice(Deg1Converted, Deg1Elements);
        for(size_t i=0;i<Deg1Elements.nr_of_rows();++i)
            FC.Deg1_Elements.push_back(Deg1Converted[i]);
        FC.is_Computed.set(ConeProperty::Deg1Elements); 
    }
    
    if(isComputed(ConeProperty::HilbertBasis)){
        Matrix<IntegerFC> HBConverted;
        BasisChangePointed.convert_to_sublattice(HBConverted, HilbertBasis);
        for(size_t i=0;i<HilbertBasis.nr_of_rows();++i)
            FC.Deg1_Elements.push_back(HBConverted[i]);
        FC.is_Computed.set(ConeProperty::HilbertBasis); 
    }*/
    
    if (ExcludedFaces.nr_of_rows()!=0) {
        BasisChangePointed.convert_to_sublattice_dual(FC.ExcludedFaces, ExcludedFaces);
    }
    if (isComputed(ConeProperty::ExcludedFaces)) {
        FC.is_Computed.set(ConeProperty::ExcludedFaces);
    }

    if (inhomogeneous){
        BasisChangePointed.convert_to_sublattice_dual_no_div(FC.Truncation, Dehomogenization);
    }
    if ( Grading.size()>0 ) {  // IMPORTANT: Truncation must be set before Grading
        BasisChangePointed.convert_to_sublattice_dual(FC.Grading, Grading);
        if(isComputed(ConeProperty::Grading) ){    // is grading positive?
            FC.is_Computed.set(ConeProperty::Grading);
            /*if (inhomogeneous)
                FC.find_grading_inhom();
            FC.set_degrees();*/
        }
    }

    if (SupportHyperplanes.nr_of_rows()!=0) {
        BasisChangePointed.convert_to_sublattice_dual(FC.Support_Hyperplanes, SupportHyperplanes);
   }
    if (isComputed(ConeProperty::SupportHyperplanes)){
        FC.is_Computed.set(ConeProperty::SupportHyperplanes);
        FC.do_all_hyperplanes = false;
    }

    if(ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid)){
        FC.do_module_gens_intcl=true;
    }
    
    if(is_approximation)
        give_data_of_approximated_cone_to(FC);

    /* do the computation */
    
    try {     
        try {
            FC.compute();
        } catch (const NotIntegrallyClosedException& ) {
        }
        is_Computed.set(ConeProperty::Sublattice);
        // make sure we minimize the excluded faces if requested
        if(ToCompute.test(ConeProperty::ExcludedFaces) || ToCompute.test(ConeProperty::SupportHyperplanes)) {
            FC.prepare_inclusion_exclusion();
        }
        extract_data(FC);
        if(isComputed(ConeProperty::IsPointed) && pointed)
            is_Computed.set(ConeProperty::MaximalSubspace);
    } catch(const NonpointedException& ) {
        is_Computed.set(ConeProperty::Sublattice);
        extract_data(FC);
        if(verbose){
            verboseOutput() << "Cone not pointed. Restarting computation." << endl;
        }
        FC=Full_Cone<IntegerFC>(Matrix<IntegerFC>(1)); // to kill the old FC (almost)
        Matrix<Integer> Dual_Gen;
        Dual_Gen=BasisChangePointed.to_sublattice_dual(SupportHyperplanes);
        Sublattice_Representation<Integer> Pointed(Dual_Gen,true); // sublattice of the dual lattice
        BasisMaxSubspace = BasisChangePointed.from_sublattice(Pointed.getEquationsMatrix());
        check_vanishing_of_grading_and_dehom();
        BasisChangePointed.compose_dual(Pointed);
        is_Computed.set(ConeProperty::MaximalSubspace);        
        // now we get the basis of the maximal subspace
        pointed = (BasisMaxSubspace.nr_of_rows() == 0);
        is_Computed.set(ConeProperty::IsPointed);
        compute_full_cone<IntegerFC>(ToCompute);           
    }
}


template<typename Integer>
void Cone<Integer>::compute_generators() {
    //create Generators from SupportHyperplanes
    if (!isComputed(ConeProperty::Generators) && (SupportHyperplanes.nr_of_rows()!=0 ||inhomogeneous)) {
        if (verbose) {
            verboseOutput() << "Computing extreme rays as support hyperplanes of the dual cone:" << endl;
        }
        if (change_integer_type) {
            try {
                compute_generators_inner<MachineInteger>();
            } catch(const ArithmeticException& e) {
                if (verbose) {
                    verboseOutput() << e.what() << endl;
                    verboseOutput() << "Restarting with a bigger type." << endl;
                }
                compute_generators_inner<Integer>();
            }
        } else {
            compute_generators_inner<Integer>();
        }
    }
    assert(isComputed(ConeProperty::Generators));
}

template<typename Integer>
template<typename IntegerFC>
void Cone<Integer>::compute_generators_inner() {
    
    Matrix<Integer> Dual_Gen;
    Dual_Gen=BasisChangePointed.to_sublattice_dual(SupportHyperplanes);
    // first we take the quotient of the efficient sublattice modulo the maximal subspace
    Sublattice_Representation<Integer> Pointed(Dual_Gen,true); // sublattice of the dual space

    // now we get the basis of the maximal subspace
    if(!isComputed(ConeProperty::MaximalSubspace)){
        BasisMaxSubspace = BasisChangePointed.from_sublattice(Pointed.getEquationsMatrix());
        check_vanishing_of_grading_and_dehom();
        is_Computed.set(ConeProperty::MaximalSubspace);
    }
    if(!isComputed(ConeProperty::IsPointed)){
        pointed = (BasisMaxSubspace.nr_of_rows() == 0);
        is_Computed.set(ConeProperty::IsPointed);
    }
    BasisChangePointed.compose_dual(Pointed); // primal cone now pointed, may not yet be full dimensional

    // restrict the supphyps to efficient sublattice and push to quotient mod subspace
    Matrix<IntegerFC> Dual_Gen_Pointed;
    BasisChangePointed.convert_to_sublattice_dual(Dual_Gen_Pointed, SupportHyperplanes);    
    Full_Cone<IntegerFC> Dual_Cone(Dual_Gen_Pointed);
    Dual_Cone.verbose=verbose;
    Dual_Cone.do_extreme_rays=true; // we try to find them, need not exist
    try {     
        Dual_Cone.dualize_cone();
    } catch(const NonpointedException& ){}; // we don't mind if the dual cone is not pointed
    
    if (Dual_Cone.isComputed(ConeProperty::SupportHyperplanes)) {
        //get the extreme rays of the primal cone
        BasisChangePointed.convert_from_sublattice(Generators,
                          Dual_Cone.getSupportHyperplanes());
        is_Computed.set(ConeProperty::Generators);
        
        //get minmal set of support_hyperplanes if possible
        if (Dual_Cone.isComputed(ConeProperty::ExtremeRays)) {            
            Matrix<IntegerFC> Supp_Hyp = Dual_Cone.getGenerators().submatrix(Dual_Cone.getExtremeRays());
            BasisChangePointed.convert_from_sublattice_dual(SupportHyperplanes, Supp_Hyp);
            SupportHyperplanes.sort_lex();
            is_Computed.set(ConeProperty::SupportHyperplanes);
        }
        
        // now the final transformations
        // only necessary if the basis changes computed so far do not make the cone full-dimensional
        // this is equaivalent to the dual cone bot being pointed
        if(!(Dual_Cone.isComputed(ConeProperty::IsPointed) && Dual_Cone.isPointed())){
            // first to full-dimensional pointed
            Matrix<Integer> Help;
            Help=BasisChangePointed.to_sublattice(Generators); // sublattice of the primal space
            Sublattice_Representation<Integer> PointedHelp(Help,true);
            BasisChangePointed.compose(PointedHelp);
            // second to efficient sublattice
            if(BasisMaxSubspace.nr_of_rows()==0){  // primal cone is pointed and we can copy
                BasisChange=BasisChangePointed;
            }
            else{
                Help=BasisChange.to_sublattice(Generators);
                Help.append(BasisChange.to_sublattice(BasisMaxSubspace));
                Sublattice_Representation<Integer> EmbHelp(Help,true); // sublattice of the primal space
                compose_basis_change(EmbHelp);
            }
        }
        is_Computed.set(ConeProperty::Sublattice); // will not be changed anymore

        checkGrading();
        // compute grading, so that it is also known if nothing else is done afterwards
        if (!isComputed(ConeProperty::Grading) && !inhomogeneous) {
            // Generators = ExtremeRays
            vector<Integer> lf = BasisChangePointed.to_sublattice(Generators).find_linear_form();
            if (lf.size() == BasisChange.getRank()) {
                vector<Integer> test_lf=BasisChange.from_sublattice_dual(lf);
                if(Generators.nr_of_rows()==0 || v_scalar_product(Generators[0],test_lf)==1)
                    setGrading(test_lf);
            }
        }
        setWeights();
        set_extreme_rays(vector<bool>(Generators.nr_of_rows(),true)); // here since they get sorted
        is_Computed.set(ConeProperty::ExtremeRays);
    }
}


template<typename Integer>
void Cone<Integer>::compute_dual(ConeProperties& ToCompute) {

    ToCompute.reset(is_Computed);
    if (ToCompute.none() || !( ToCompute.test(ConeProperty::Deg1Elements)
                            || ToCompute.test(ConeProperty::HilbertBasis))) {
        return;
    }

    if (change_integer_type) {
        try {
            compute_dual_inner<MachineInteger>(ToCompute);
        } catch(const ArithmeticException& e) {
            if (verbose) {
                verboseOutput() << e.what() << endl;
                verboseOutput() << "Restarting with a bigger type." << endl;
            }
            change_integer_type = false;
        }
    }
    if (!change_integer_type) {
        compute_dual_inner<Integer>(ToCompute);
    }
    ToCompute.reset(ConeProperty::DualMode);
    ToCompute.reset(is_Computed);
    // if (ToCompute.test(ConeProperty::DefaultMode) && ToCompute.goals().none()) {
    //    ToCompute.reset(ConeProperty::DefaultMode);
    // }
}

template<typename Integer>
vector<Sublattice_Representation<Integer> > MakeSubAndQuot(const Matrix<Integer>& Gen,
                                        const Matrix<Integer>& Ker){
    vector<Sublattice_Representation<Integer> > Result;                                        
    Matrix<Integer> Help=Gen;
    Help.append(Ker);
    Sublattice_Representation<Integer> Sub(Help,true);
    Sublattice_Representation<Integer> Quot=Sub;
    if(Ker.nr_of_rows()>0){
        Matrix<Integer> HelpQuot=Sub.to_sublattice(Ker).kernel();   // kernel here to be interpreted as subspace of the dual
                                                                    // namely the linear forms vanishing on Ker
        Sublattice_Representation<Integer> SubToQuot(HelpQuot,true); // sublattice of the dual
        Quot.compose_dual(SubToQuot);
    }
    Result.push_back(Sub);
    Result.push_back(Quot);
    
    return Result;    
}

template<typename Integer>
template<typename IntegerFC>
void Cone<Integer>::compute_dual_inner(ConeProperties& ToCompute) {

    bool do_only_Deg1_Elements = ToCompute.test(ConeProperty::Deg1Elements)
                                 && !ToCompute.test(ConeProperty::HilbertBasis);

    if(isComputed(ConeProperty::Generators) && SupportHyperplanes.nr_of_rows()==0){
        if (verbose) {
            verboseOutput()<<  "Computing support hyperplanes for the dual mode:"<< endl;
        }
        ConeProperties Dualize;
        Dualize.set(ConeProperty::SupportHyperplanes);
        Dualize.set(ConeProperty::ExtremeRays);
        compute(Dualize);
    }
    
    bool do_extreme_rays_first = false;
    if (!isComputed(ConeProperty::ExtremeRays)) {
        if (do_only_Deg1_Elements && Grading.size()==0)
            do_extreme_rays_first = true;
        else if ( (do_only_Deg1_Elements || inhomogeneous) &&
                   ( ToCompute.test(ConeProperty::NakedDual)
                 || ToCompute.test(ConeProperty::ExtremeRays)
                 || ToCompute.test(ConeProperty::SupportHyperplanes)
                 || ToCompute.test(ConeProperty::Sublattice) ) )
            do_extreme_rays_first = true;
    }

    if (do_extreme_rays_first) {
        if (verbose) {
            verboseOutput() << "Computing extreme rays for the dual mode:"<< endl;
        }
        compute_generators();   // computes extreme rays, but does not find grading !
    }

    if(do_only_Deg1_Elements && Grading.size()==0){
        vector<Integer> lf= Generators.submatrix(ExtremeRaysIndicator).find_linear_form_low_dim();
        if(Generators.nr_of_rows()==0 || (lf.size()==dim && v_scalar_product(Generators[0],lf)==1))
            setGrading(lf);
        else{
            throw BadInputException("Need grading to compute degree 1 elements and cannot find one.");
        }
    }

    if (SupportHyperplanes.nr_of_rows()==0 && !isComputed(ConeProperty::SupportHyperplanes)) {
        throw FatalException("Could not get SupportHyperplanes.");
    }

    Matrix<IntegerFC> Inequ_on_Ker;
    BasisChangePointed.convert_to_sublattice_dual(Inequ_on_Ker,SupportHyperplanes);
     
    vector<IntegerFC> Truncation;
    if(inhomogeneous){
        BasisChangePointed.convert_to_sublattice_dual_no_div(Truncation, Dehomogenization);
    }
    if (do_only_Deg1_Elements) {
        // in this case the grading acts as truncation and it is a NEW inequality
        BasisChangePointed.convert_to_sublattice_dual(Truncation, Grading);
    }

    Cone_Dual_Mode<IntegerFC> ConeDM(Inequ_on_Ker, Truncation); // Inequ_on_Ker is NOT const
    Inequ_on_Ker=Matrix<IntegerFC>(0,0);  // destroy it
    ConeDM.verbose=verbose;
    ConeDM.inhomogeneous=inhomogeneous;
    ConeDM.do_only_Deg1_Elements=do_only_Deg1_Elements;
    if(isComputed(ConeProperty::Generators))
        BasisChangePointed.convert_to_sublattice(ConeDM.Generators, Generators);
    if(isComputed(ConeProperty::ExtremeRays))
        ConeDM.ExtremeRaysInd=ExtremeRaysIndicator;
    ConeDM.hilbert_basis_dual();
    
    if(!isComputed(ConeProperty::MaximalSubspace)){
        BasisChangePointed.convert_from_sublattice(BasisMaxSubspace,ConeDM.BasisMaxSubspace);
        check_vanishing_of_grading_and_dehom(); // all this must be done here because to_sublattice may kill it
    }

    if (!isComputed(ConeProperty::Sublattice) || !isComputed(ConeProperty::MaximalSubspace)){
        if(!(do_only_Deg1_Elements || inhomogeneous)) {
            // At this point we still have BasisChange==BasisChangePointed
            // now we can pass to a pointed full-dimensional cone
            
            vector<Sublattice_Representation<IntegerFC> > BothRepFC=MakeSubAndQuot
                        (ConeDM.Generators,ConeDM.BasisMaxSubspace);
            if(!BothRepFC[0].IsIdentity())        
                BasisChange.compose(Sublattice_Representation<Integer>(BothRepFC[0]));
            is_Computed.set(ConeProperty::Sublattice);
            if(!BothRepFC[1].IsIdentity())
                BasisChangePointed.compose(Sublattice_Representation<Integer>(BothRepFC[1]));
            ConeDM.to_sublattice(BothRepFC[1]);
        }
    }
    
    is_Computed.set(ConeProperty::MaximalSubspace); // NOT EARLIER !!!!
    
    
    // create a Full_Cone out of ConeDM
    Full_Cone<IntegerFC> FC(ConeDM);
    FC.verbose=verbose;
    // Give extra data to FC
    if (Grading.size()>0) {
        BasisChangePointed.convert_to_sublattice_dual(FC.Grading, Grading);
        if(isComputed(ConeProperty::Grading))
            FC.is_Computed.set(ConeProperty::Grading);
    }
    if(inhomogeneous)
        BasisChangePointed.convert_to_sublattice_dual_no_div(FC.Truncation, Dehomogenization);
    FC.do_class_group=ToCompute.test(ConeProperty::ClassGroup);
    FC.dual_mode();
    extract_data(FC);
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer Cone<Integer>::compute_primary_multiplicity() {
    if (change_integer_type) {
        try {
            return compute_primary_multiplicity_inner<MachineInteger>();
        } catch(const ArithmeticException& e) {
            if (verbose) {
                verboseOutput() << e.what() << endl;
                verboseOutput() << "Restarting with a bigger type." << endl;
            }
            change_integer_type = false;
        }
    }
    return compute_primary_multiplicity_inner<Integer>();
}

//---------------------------------------------------------------------------

template<typename Integer>
template<typename IntegerFC>
Integer Cone<Integer>::compute_primary_multiplicity_inner() {
    Matrix<IntegerFC> Ideal(0,dim-1);
    vector<IntegerFC> help(dim-1);
    for(size_t i=0;i<Generators.nr_of_rows();++i){ // select ideal generators
        if(Generators[i][dim-1]==1){
            for(size_t j=0;j<dim-1;++j)
                convert(help[j],Generators[i][j]);
            Ideal.append(help);
        }
    }
    Full_Cone<IntegerFC> IdCone(Ideal,false);
    IdCone.do_bottom_dec=true;
    IdCone.do_determinants=true;
    IdCone.compute();
    return convertTo<Integer>(IdCone.detSum);
}

//---------------------------------------------------------------------------

template<typename Integer>
template<typename IntegerFC>
void Cone<Integer>::extract_data(Full_Cone<IntegerFC>& FC) {
    //this function extracts ALL available data from the Full_Cone
    //even if it was in Cone already <- this may change
    //it is possible to delete the data in Full_Cone after extracting it

    if(verbose) {
        verboseOutput() << "transforming data..."<<flush;
    }
    
    if (FC.isComputed(ConeProperty::Generators)) {
        BasisChangePointed.convert_from_sublattice(Generators,FC.getGenerators());
        is_Computed.set(ConeProperty::Generators);
    }
    
    if (FC.isComputed(ConeProperty::IsPointed) && !isComputed(ConeProperty::IsPointed)) {
        pointed = FC.isPointed();
        if(pointed)
            is_Computed.set(ConeProperty::MaximalSubspace);
        is_Computed.set(ConeProperty::IsPointed);
    }    
    
    if (FC.isComputed(ConeProperty::Grading)) {
        if (Grading.size()==0) {
            BasisChangePointed.convert_from_sublattice_dual(Grading, FC.getGrading());
        }
        is_Computed.set(ConeProperty::Grading);
        setWeights();
        //compute denominator of Grading
        if(BasisChangePointed.getRank()!=0){
            vector<Integer> test_grading = BasisChangePointed.to_sublattice_dual_no_div(Grading);
            GradingDenom=v_make_prime(test_grading);
        }
        else
            GradingDenom=1; 
        is_Computed.set(ConeProperty::GradingDenom);
    }
        
    if (FC.isComputed(ConeProperty::ModuleGeneratorsOverOriginalMonoid)) { // must precede extreme rays
        BasisChangePointed.convert_from_sublattice(ModuleGeneratorsOverOriginalMonoid, FC.getModuleGeneratorsOverOriginalMonoid());
        ModuleGeneratorsOverOriginalMonoid.sort_by_weights(WeightsGrad,GradAbs);
        is_Computed.set(ConeProperty::ModuleGeneratorsOverOriginalMonoid);
    }

    if (FC.isComputed(ConeProperty::ExtremeRays)) {
        set_extreme_rays(FC.getExtremeRays());
    }
    if (FC.isComputed(ConeProperty::SupportHyperplanes)) {
        /* if (inhomogeneous) {
            // remove irrelevant support hyperplane 0 ... 0 1
            vector<IntegerFC> irr_hyp_subl;
            BasisChangePointed.convert_to_sublattice_dual(irr_hyp_subl, Dehomogenization); 
            FC.Support_Hyperplanes.remove_row(irr_hyp_subl);
        } */
        // BasisChangePointed.convert_from_sublattice_dual(SupportHyperplanes, FC.getSupportHyperplanes());
        extract_supphyps(FC);
        if(inhomogeneous && FC.dim<dim){ // make inequality for the inhomogeneous variable appear as dehomogenization
            vector<Integer> dehom_restricted=BasisChangePointed.to_sublattice_dual(Dehomogenization);
            for(size_t i=0;i<SupportHyperplanes.nr_of_rows();++i){
                if(dehom_restricted==BasisChangePointed.to_sublattice_dual(SupportHyperplanes[i])){
                    SupportHyperplanes[i]=Dehomogenization;
                    break;
                }
            }
        }
        SupportHyperplanes.sort_lex();
        is_Computed.set(ConeProperty::SupportHyperplanes);
    }
    if (FC.isComputed(ConeProperty::TriangulationSize)) {
        TriangulationSize = FC.totalNrSimplices;
        triangulation_is_nested = FC.triangulation_is_nested;
        triangulation_is_partial= FC.triangulation_is_partial;
        is_Computed.set(ConeProperty::TriangulationSize);
        is_Computed.set(ConeProperty::IsTriangulationPartial);
        is_Computed.set(ConeProperty::IsTriangulationNested);
        is_Computed.reset(ConeProperty::Triangulation);
        Triangulation.clear(); // to get rid of a previously computed triangulation
    }
    if (FC.isComputed(ConeProperty::TriangulationDetSum)) {
        convert(TriangulationDetSum, FC.detSum);
        is_Computed.set(ConeProperty::TriangulationDetSum);
    }
    
    if (FC.isComputed(ConeProperty::Triangulation)) {
        size_t tri_size = FC.Triangulation.size();
        FC.Triangulation.sort(compareKeys<IntegerFC>); // necessary to make triangulation unique
        Triangulation = vector< pair<vector<key_t>, Integer> >(tri_size);
        if(FC.isComputed(ConeProperty::ConeDecomposition))
            OpenFacets.resize(tri_size);
        SHORTSIMPLEX<IntegerFC> simp;
        for (size_t i = 0; i<tri_size; ++i) {
            simp = FC.Triangulation.front();
            Triangulation[i].first.swap(simp.key);
            /* sort(Triangulation[i].first.begin(), Triangulation[i].first.end()); -- no longer allowed here because of ConeDecomposition. Done in full_cone.cpp, transfer_triangulation_to top */
            if (FC.isComputed(ConeProperty::TriangulationDetSum))
                convert(Triangulation[i].second, simp.vol);
            else
                Triangulation[i].second = 0;
            if(FC.isComputed(ConeProperty::ConeDecomposition))
                OpenFacets[i].swap(simp.Excluded);
            FC.Triangulation.pop_front();
        }
        if(FC.isComputed(ConeProperty::ConeDecomposition))
            is_Computed.set(ConeProperty::ConeDecomposition);
        is_Computed.set(ConeProperty::Triangulation);
    }

    if (FC.isComputed(ConeProperty::StanleyDec)) {
        StanleyDec.clear();
        StanleyDec.splice(StanleyDec.begin(),FC.StanleyDec);
        // At present, StanleyDec not sorted here
        is_Computed.set(ConeProperty::StanleyDec);
    }

    if (FC.isComputed(ConeProperty::InclusionExclusionData)) {
        InExData.clear();
        InExData.reserve(FC.InExCollect.size());
        map<boost::dynamic_bitset<>, long>::iterator F;
        vector<key_t> key;
        for (F=FC.InExCollect.begin(); F!=FC.InExCollect.end(); ++F) {
            key.clear();
            for (size_t i=0;i<FC.nr_gen;++i) {
                if (F->first.test(i)) {
                    key.push_back(i);
                }
            }
            InExData.push_back(make_pair(key,F->second));
        }
        is_Computed.set(ConeProperty::InclusionExclusionData);
    }
    if (FC.isComputed(ConeProperty::RecessionRank) && isComputed(ConeProperty::MaximalSubspace)) {
        recession_rank = FC.level0_dim+BasisMaxSubspace.nr_of_rows();
        is_Computed.set(ConeProperty::RecessionRank);
        if (get_rank_internal() == recession_rank) {
            affine_dim = -1;
        } else {
            affine_dim = get_rank_internal()-1;
        }
        is_Computed.set(ConeProperty::AffineDim);
    }
    if (FC.isComputed(ConeProperty::ModuleRank)) {
        module_rank = FC.getModuleRank();
        is_Computed.set(ConeProperty::ModuleRank);
    }
    if (FC.isComputed(ConeProperty::Multiplicity)) {
        if(!inhomogeneous) {
            multiplicity = FC.getMultiplicity();
            is_Computed.set(ConeProperty::Multiplicity);
        } else if (FC.isComputed(ConeProperty::ModuleRank)) {
            multiplicity = FC.getMultiplicity()*module_rank;
            is_Computed.set(ConeProperty::Multiplicity);
        }
    }
    if (FC.isComputed(ConeProperty::WitnessNotIntegrallyClosed)) {
        BasisChangePointed.convert_from_sublattice(WitnessNotIntegrallyClosed,FC.Witness);
        is_Computed.set(ConeProperty::WitnessNotIntegrallyClosed);
        integrally_closed = false;
        is_Computed.set(ConeProperty::IsIntegrallyClosed);
    }
    if (FC.isComputed(ConeProperty::HilbertBasis)) {
        if (inhomogeneous) {
            // separate (capped) Hilbert basis to the Hilbert basis of the level 0 cone
            // and the module generators in level 1
            HilbertBasis = Matrix<Integer>(0,dim);
            ModuleGenerators = Matrix<Integer>(0,dim);
            typename list< vector<IntegerFC> >::const_iterator FCHB(FC.Hilbert_Basis.begin());
            vector<Integer> tmp;
            for (; FCHB != FC.Hilbert_Basis.end(); ++FCHB) {
                
                INTERRUPT_COMPUTATION_BY_EXCEPTION
                
                BasisChangePointed.convert_from_sublattice(tmp,*FCHB);
                if (v_scalar_product(tmp,Dehomogenization) == 0) { // Hilbert basis element of the cone at level 0
                    HilbertBasis.append(tmp);
                } else {              // module generator
                    ModuleGenerators.append(tmp);
                }
            }
            ModuleGenerators.sort_by_weights(WeightsGrad,GradAbs);
            is_Computed.set(ConeProperty::ModuleGenerators);
        } else { // homogeneous
            HilbertBasis = Matrix<Integer>(0,dim);
            typename list< vector<IntegerFC> >::const_iterator FCHB(FC.Hilbert_Basis.begin());
            vector<Integer> tmp;
            for (; FCHB != FC.Hilbert_Basis.end(); ++FCHB) {
                BasisChangePointed.convert_from_sublattice(tmp,*FCHB);                
                HilbertBasis.append(tmp);
            }
        }
        HilbertBasis.sort_by_weights(WeightsGrad,GradAbs);
        is_Computed.set(ConeProperty::HilbertBasis);
    }
    if (FC.isComputed(ConeProperty::Deg1Elements)) {
        Deg1Elements = Matrix<Integer>(0,dim);
        typename list< vector<IntegerFC> >::const_iterator DFC(FC.Deg1_Elements.begin());
        vector<Integer> tmp;
        for (; DFC != FC.Deg1_Elements.end(); ++DFC) {
            
            INTERRUPT_COMPUTATION_BY_EXCEPTION
            
            BasisChangePointed.convert_from_sublattice(tmp,*DFC);                
            Deg1Elements.append(tmp);
        }
        Deg1Elements.sort_by_weights(WeightsGrad,GradAbs);
        is_Computed.set(ConeProperty::Deg1Elements);
    }
    if (FC.isComputed(ConeProperty::HilbertSeries)) {
        long save_nr_coeff_quasipol=HSeries.get_nr_coeff_quasipol(); // Full_Cone does not compute the quasipolynomial
        long save_expansion_degree=HSeries.get_expansion_degree();  // or the exoansion
        HSeries = FC.Hilbert_Series;
        HSeries.set_nr_coeff_quasipol(save_nr_coeff_quasipol);
        HSeries.set_expansion_degree(save_expansion_degree);
        is_Computed.set(ConeProperty::HilbertSeries);
        is_Computed.set(ConeProperty::ExplicitHilbertSeries);
    }
    if (FC.isComputed(ConeProperty::HSOP)) {
        is_Computed.set(ConeProperty::HSOP);
    }
    if (FC.isComputed(ConeProperty::IsDeg1ExtremeRays)) {
        deg1_extreme_rays = FC.isDeg1ExtremeRays();
        is_Computed.set(ConeProperty::IsDeg1ExtremeRays);
    }
    if (FC.isComputed(ConeProperty::ExcludedFaces)) {
        BasisChangePointed.convert_from_sublattice_dual(ExcludedFaces, FC.getExcludedFaces());
        ExcludedFaces.sort_lex();
        is_Computed.set(ConeProperty::ExcludedFaces);
    }

    if (FC.isComputed(ConeProperty::IsDeg1HilbertBasis)) {
        deg1_hilbert_basis = FC.isDeg1HilbertBasis();
        is_Computed.set(ConeProperty::IsDeg1HilbertBasis);
    }
    if (FC.isComputed(ConeProperty::ClassGroup)) {
        convert(ClassGroup, FC.ClassGroup);
        is_Computed.set(ConeProperty::ClassGroup);
    }
    
    /* if (FC.isComputed(ConeProperty::MaximalSubspace) && 
                                   !isComputed(ConeProperty::MaximalSubspace)) {
        BasisChangePointed.convert_from_sublattice(BasisMaxSubspace, FC.Basis_Max_Subspace);
        check_vanishing_of_grading_and_dehom();
        is_Computed.set(ConeProperty::MaximalSubspace);
    }*/

    check_integrally_closed();

    if (verbose) {
        verboseOutput() << " done." <<endl;
    }
}

//---------------------------------------------------------------------------
template<typename Integer>
template<typename IntegerFC>
void Cone<Integer>::extract_supphyps(Full_Cone<IntegerFC>& FC) {
        BasisChangePointed.convert_from_sublattice_dual(SupportHyperplanes, FC.getSupportHyperplanes());
}

template<typename Integer>
void Cone<Integer>::extract_supphyps(Full_Cone<Integer>& FC) {
    if(BasisChangePointed.IsIdentity())
        swap(SupportHyperplanes,FC.Support_Hyperplanes);
    else
        SupportHyperplanes=BasisChangePointed.from_sublattice_dual(FC.getSupportHyperplanes());
}


//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::check_integrally_closed() {
    if (!isComputed(ConeProperty::OriginalMonoidGenerators)
            || isComputed(ConeProperty::IsIntegrallyClosed)
            || !isComputed(ConeProperty::HilbertBasis) || inhomogeneous)
        return;

    unit_group_index=1;
    if(BasisMaxSubspace.nr_of_rows()>0)
        compute_unit_group_index();
    is_Computed.set(ConeProperty::UnitGroupIndex);
    if (index > 1 || HilbertBasis.nr_of_rows() > OriginalMonoidGenerators.nr_of_rows()
            || unit_group_index>1) {
        integrally_closed = false;
        is_Computed.set(ConeProperty::IsIntegrallyClosed);
        return;
    } 
    find_witness();
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::compute_unit_group_index() {
    assert(isComputed(ConeProperty::MaximalSubspace));
    // we want to compute in the maximal linear subspace
    Sublattice_Representation<Integer> Sub(BasisMaxSubspace,true);
    Matrix<Integer> origens_in_subspace(0,dim);

    // we must collect all original generetors that lie in the maximal subspace 

    for(size_t i=0;i<OriginalMonoidGenerators.nr_of_rows();++i){
        size_t j;
        for(j=0;j<SupportHyperplanes.nr_of_rows();++j){
                if(v_scalar_product(OriginalMonoidGenerators[i],SupportHyperplanes[j])!=0)
                    break;
        }
        if(j==SupportHyperplanes.nr_of_rows())
            origens_in_subspace.append(OriginalMonoidGenerators[i]);
    }
    Matrix<Integer> M=Sub.to_sublattice(origens_in_subspace);
    unit_group_index= M.full_rank_index();
    // cout << "Unit group index " << unit_group_index;
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::find_witness() {
    if (!isComputed(ConeProperty::OriginalMonoidGenerators)
            || inhomogeneous) {
        // no original monoid defined
        throw NotComputableException(ConeProperties(ConeProperty::WitnessNotIntegrallyClosed));
    }
    if (isComputed(ConeProperty::IsIntegrallyClosed) && integrally_closed) {
        // original monoid is integrally closed
        throw NotComputableException(ConeProperties(ConeProperty::WitnessNotIntegrallyClosed));
    }
    if (isComputed(ConeProperty::WitnessNotIntegrallyClosed)
            || !isComputed(ConeProperty::HilbertBasis) )
        return;

    long nr_gens = OriginalMonoidGenerators.nr_of_rows();
    long nr_hilb = HilbertBasis.nr_of_rows();
    // if the cone is not pointed, we have to check it on the quotion
    Matrix<Integer> gens_quot;
    Matrix<Integer> hilb_quot;
    if (!pointed) {
        gens_quot = BasisChangePointed.to_sublattice(OriginalMonoidGenerators);
        hilb_quot = BasisChangePointed.to_sublattice(HilbertBasis);
    }
    Matrix<Integer>& gens = pointed ? OriginalMonoidGenerators : gens_quot;
    Matrix<Integer>& hilb = pointed ? HilbertBasis : hilb_quot;
    integrally_closed = true;
    typename list< vector<Integer> >::iterator h;
    for (long h = 0; h < nr_hilb; ++h) {
        integrally_closed = false;
        for (long i = 0; i < nr_gens; ++i) {
            if (hilb[h] == gens[i]) {
                integrally_closed = true;
                break;
            }
        }
        if (!integrally_closed) {
            WitnessNotIntegrallyClosed = HilbertBasis[h];
            is_Computed.set(ConeProperty::WitnessNotIntegrallyClosed);
            break;
        }
    }
    is_Computed.set(ConeProperty::IsIntegrallyClosed);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::set_original_monoid_generators(const Matrix<Integer>& Input) {
    if (!isComputed(ConeProperty::OriginalMonoidGenerators)) {
        OriginalMonoidGenerators = Input;
        is_Computed.set(ConeProperty::OriginalMonoidGenerators);
    }
    // Generators = Input;
    // is_Computed.set(ConeProperty::Generators);
    Matrix<Integer> M=BasisChange.to_sublattice(Input);
    index=M.full_rank_index();
    is_Computed.set(ConeProperty::InternalIndex);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::set_extreme_rays(const vector<bool>& ext) {
    assert(ext.size() == Generators.nr_of_rows());
    ExtremeRaysIndicator=ext;
    vector<bool> choice=ext;
    if (inhomogeneous) {
        // separate extreme rays to rays of the level 0 cone
        // and the verticies of the polyhedron, which are in level >=1
        size_t nr_gen = Generators.nr_of_rows();
        vector<bool> VOP(nr_gen);
        for (size_t i=0; i<nr_gen; i++) {
            if (ext[i] && v_scalar_product(Generators[i],Dehomogenization) != 0) {
                VOP[i] = true;
                choice[i]=false;
            }
        }
        VerticesOfPolyhedron=Generators.submatrix(VOP).sort_by_weights(WeightsGrad,GradAbs);
        is_Computed.set(ConeProperty::VerticesOfPolyhedron);
    }
    ExtremeRays=Generators.submatrix(choice);
    if(inhomogeneous && !isComputed(ConeProperty::AffineDim) && isComputed(ConeProperty::MaximalSubspace)){
        size_t level0_dim=ExtremeRays.max_rank_submatrix_lex().size();
        recession_rank = level0_dim+BasisMaxSubspace.nr_of_rows();
        is_Computed.set(ConeProperty::RecessionRank);
        if (get_rank_internal() == recession_rank) {
            affine_dim = -1;
        } else {
            affine_dim = get_rank_internal()-1;
        }
        is_Computed.set(ConeProperty::AffineDim);
        
    }
    if(isComputed(ConeProperty::ModuleGeneratorsOverOriginalMonoid)){  // not possible in inhomogeneous case
        Matrix<Integer> ExteEmbedded=BasisChangePointed.to_sublattice(ExtremeRays);
        for(size_t i=0;i<ExteEmbedded.nr_of_rows();++i)
            v_make_prime(ExteEmbedded[i]);
        ExteEmbedded.remove_duplicate_and_zero_rows();
        ExtremeRays=BasisChangePointed.from_sublattice(ExteEmbedded);
    }
    ExtremeRays.sort_by_weights(WeightsGrad,GradAbs);
    is_Computed.set(ConeProperty::ExtremeRays);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::compute_vertices_float(ConeProperties& ToCompute) {
    if(!ToCompute.test(ConeProperty::VerticesFloat) || isComputed(ConeProperty::VerticesFloat))
        return;
    if(!isComputed(ConeProperty::ExtremeRays))
        throw NotComputableException("VerticesFloat not computable without extreme rays");
    if(inhomogeneous && !isComputed(ConeProperty::VerticesOfPolyhedron))
        throw NotComputableException("VerticesFloat not computable in the inhomogeneous case without vertices");
    if(!inhomogeneous && !isComputed(ConeProperty::Grading))
        throw NotComputableException("VerticesFloat not computable in the homogeneous case without a grading");
    if(inhomogeneous)
        convert(VerticesFloat, VerticesOfPolyhedron);
    else
        convert(VerticesFloat, ExtremeRays);
    vector<nmz_float> norm;
    if(inhomogeneous)
        convert(norm,Dehomogenization);
    else{
       convert( norm,Grading);
       nmz_float GD=1.0/convertTo<double>(GradingDenom);
       v_scalar_multiplication(norm,GD);
    }
    for(size_t i=0;i<VerticesFloat.nr_of_rows();++i){
        nmz_float den=1.0/v_scalar_product(VerticesFloat[i],norm);
        v_scalar_multiplication(VerticesFloat[i],den);        
    }
    is_Computed.set(ConeProperty::VerticesFloat);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::complete_sublattice_comp(ConeProperties& ToCompute) {
    
    if(!isComputed(ConeProperty::Sublattice))
        return;
    is_Computed.set(ConeProperty::Rank);
    if(ToCompute.test(ConeProperty::Equations)){
        BasisChange.getEquationsMatrix(); // just to force computation, ditto below
        is_Computed.set(ConeProperty::Equations);
    }
    if(ToCompute.test(ConeProperty::Congruences) || ToCompute.test(ConeProperty::ExternalIndex)){
        BasisChange.getCongruencesMatrix();
        BasisChange.getExternalIndex();
        is_Computed.set(ConeProperty::Congruences);
        is_Computed.set(ConeProperty::ExternalIndex);
    }
}

template<typename Integer>
void Cone<Integer>::complete_HilbertSeries_comp(ConeProperties& ToCompute) {
    if(!isComputed(ConeProperty::HilbertSeries))
        return;
    if(ToCompute.test(ConeProperty::HilbertQuasiPolynomial))
        HSeries.computeHilbertQuasiPolynomial();
    if(HSeries.isHilbertQuasiPolynomialComputed())
        is_Computed.set(ConeProperty::HilbertQuasiPolynomial);
        
    // in the case that HS was computed but not HSOP, we need to compute hsop
    if(ToCompute.test(ConeProperty::HSOP) && !isComputed(ConeProperty::HSOP)){
        // we need generators and support hyperplanes to compute hsop
        Matrix<Integer> FC_gens;
        Matrix<Integer> FC_hyps;
        BasisChangePointed.convert_to_sublattice(FC_gens,Generators);
        Full_Cone<Integer> FC(FC_gens);
        FC.Extreme_Rays_Ind = ExtremeRaysIndicator;
        FC.Grading = Grading;
        FC.inhomogeneous = inhomogeneous;
        // TRUNCATION?
        BasisChangePointed.convert_to_sublattice_dual(FC.Support_Hyperplanes,SupportHyperplanes);
        FC.compute_hsop();
        HSeries.setHSOPDenom(FC.Hilbert_Series.getHSOPDenom());
        HSeries.compute_hsop_num();
    }    
}

//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::set_project(string name){
    project=name;
}

template<typename Integer>
void Cone<Integer>::set_output_dir(string name){
    output_dir=name;
}

template<typename Integer>
void Cone<Integer>::set_nmz_call(const string& path){
    nmz_call=path;
}

template<typename Integer>
void Cone<Integer>::setPolynomial(string poly){
    IntData=IntegrationData(poly);
}

template<typename Integer>
void Cone<Integer>::setNrCoeffQuasiPol(long nr_coeff){
    HSeries.resetHilbertQuasiPolynomial();
    IntData.set_nr_coeff_quasipol(nr_coeff);
    IntData.resetHilbertQuasiPolynomial();
    HSeries.set_nr_coeff_quasipol(nr_coeff);
}

template<typename Integer>
void Cone<Integer>::setExpansionDegree(long degree){
    IntData.set_expansion_degree(degree);
    HSeries.set_expansion_degree(degree);
}

bool executable(string command){
//n check whether "command --version" cam be executed

    command +=" --version";
    string dev0= " > /dev/null";
#ifdef _WIN32 //for 32 and 64 bit windows
    dev0=" > NUL:";
#endif
    command+=dev0;
    if(system(command.c_str())==0)
        return true;
    else
        return false;
}

string command(const string& original_call, const string& to_replace, const string& by_this){
// in the original call we replace the program name to_replace by by_this
// we try variants with and without "lt-" preceding the names of executables
// since libtools may have inserted "lt-" before the original name

    string copy=original_call;
    // cout << "CALL " << original_call << endl;
    string search_lt="lt-"+to_replace;
    long length=to_replace.size();
    size_t found;
    found = copy.rfind(search_lt);
    if (found==std::string::npos) {
        found = copy.rfind(to_replace);
        if (found==std::string::npos){
            throw FatalException("Call "+ copy +" of "  +to_replace+" does not contain " +to_replace); 
        }
    }
    else{
            length+=3; //name includes lt-
    }
    string test_path=copy.replace (found,length,by_this);
    // cout << "TEST " << test_path << endl;
    if(executable(test_path)) // first without lt-
        return test_path;
    copy=original_call;
    string by_this_with_lt="lt-"+by_this; /// now with lt-
    test_path=copy.replace (found,length,by_this_with_lt);
    // cout << "TEST " << test_path << endl;
    if(executable(test_path))
        return test_path;
    return ""; // no executable found
}

//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::try_symmetrization(ConeProperties& ToCompute) {

    if(ToCompute.test(ConeProperty::NoSymmetrization))
        return;
    
    if(!(ToCompute.test(ConeProperty::Symmetrize) || ToCompute.test(ConeProperty::HilbertSeries) ||
               ToCompute.test(ConeProperty::Multiplicity)))
        return;
    
    if(inhomogeneous || nr_latt_gen>0|| nr_cone_gen>0 || lattice_ideal_input || Grading.size() < dim){
        if(ToCompute.test(ConeProperty::Symmetrize))
            throw BadInputException("Symmetrization not possible with the given input");
        else
            return;
    }
    
#ifndef NMZ_COCOA    
    if(project==""){
        if(ToCompute.test(ConeProperty::Symmetrize)){
            throw BadInputException("Symmetrization via libnormaliz not possible without CoCoALib");
        }
        else
            return;
    }
#endif
    
    Matrix<Integer> AllConst=ExcludedFaces;
    size_t nr_excl = AllConst.nr_of_rows();    
    AllConst. append(Equations);
    size_t nr_equ=AllConst.nr_of_rows()-nr_excl;
    vector<bool> unit_vector(dim,false);
    for(size_t i=0;i<Inequalities.nr_of_rows();++i){
        size_t nr_nonzero=0;
        size_t nonzero_coord;
        bool is_unit_vector=true;
        for(size_t j=0;j<dim;++j){
            if(Inequalities[i][j]==0)
                continue;
            if(nr_nonzero>0 || Inequalities[i][j]!=1){ // not a sign inequality
                is_unit_vector=false;                
                break;    
            }
            nr_nonzero++;
            nonzero_coord=j;
        }
        if(!is_unit_vector)
            AllConst.append(Inequalities[i]);
        else
            unit_vector[nonzero_coord]=true;    
    }
    
    size_t nr_inequ=AllConst.nr_of_rows()-nr_equ-nr_excl;
    
    for(size_t i=0;i<dim;++i)
        if(!unit_vector[i]){
            if(ToCompute.test(ConeProperty::Symmetrize))
                throw BadInputException("Symmetrization not possible: Not all sign inequalities in input");
            else
                return;
        }
    
    for(size_t i=0;i<Congruences.nr_of_rows();++i){
        vector<Integer> help=Congruences[i];
        help.resize(dim);
        AllConst.append(help);
    }
    // now we have collected all constraints and cehcked the existence of the sign inequalities
    
    
    AllConst.append(Grading);
    
    /* AllConst.pretty_print(cout);
    cout << "----------------------" << endl;
    cout << nr_excl << " " << nr_equ << " " << nr_inequ << endl; */
    
    AllConst=AllConst.transpose();
    
    map< vector<Integer>, size_t > classes;
    typename map< vector<Integer>, size_t >::iterator C;

    for(size_t j=0;j<AllConst.nr_of_rows();++j){
        C=classes.find(AllConst[j]);
        if(C!=classes.end())
            C->second++;
        else
            classes.insert(pair<vector<Integer>, size_t>(AllConst[j],1));
    }
    
    vector<size_t> multiplicities;
    Matrix<Integer> SymmConst(0,AllConst.nr_of_columns());
    
    for(C=classes.begin();C!=classes.end();++C){
            multiplicities.push_back(C->second);
            SymmConst.append(C->first);
    }
    SymmConst=SymmConst.transpose();
    
    vector<Integer> SymmGrad=SymmConst[SymmConst.nr_of_rows()-1];
    
    if(verbose){
        verboseOutput() << "Embedding dimension of symmetrized cone = " << SymmGrad.size() << endl;
    }
    
    if(SymmGrad.size() > 2*dim/3){
        if(!ToCompute.test(ConeProperty::Symmetrize)){
            return;
        }
    }
    
    /* compute_generators(); // we must protect against the zero cone
    if(get_rank_internal()==0)
        return; */
    
    Matrix<Integer> SymmInequ(0,SymmConst.nr_of_columns());
    Matrix<Integer> SymmEqu(0,SymmConst.nr_of_columns());
    Matrix<Integer> SymmCong(0,SymmConst.nr_of_columns());
    Matrix<Integer> SymmExcl(0,SymmConst.nr_of_columns());
 
    for(size_t i=0;i<nr_excl;++i)
        SymmExcl.append(SymmConst[i]);        
    for(size_t i=nr_excl;i<nr_excl+nr_equ;++i)
        SymmEqu.append(SymmConst[i]);    
    for(size_t i=nr_excl+nr_equ;i<nr_excl+nr_equ+nr_inequ;++i)
        SymmInequ.append(SymmConst[i]);    
    for(size_t i=nr_excl+nr_equ+nr_inequ;i<SymmConst.nr_of_rows()-1;++i){
        SymmCong.append(SymmConst[i]);
        SymmCong[SymmCong.nr_of_rows()-1].push_back(Congruences[i-(nr_inequ+nr_equ)][dim]); // restore modulus
    }

    string polynomial;
    
    for(size_t i=0;i<multiplicities.size();++i){
        for(size_t j=1;j<multiplicities[i];++j)
            polynomial+="(x["+to_string((unsigned long long) i+1)+"]+"+to_string((unsigned long long)j)+")*";
        
    }
    polynomial+="1";
    mpz_class fact=1;
    for(size_t i=0;i<multiplicities.size();++i){
        for(size_t j=1;j<multiplicities[i];++j)
            fact*=j;        
    }
    polynomial+="/"+fact.get_str()+";";

#ifdef NMZ_COCOA
    
    map< InputType, Matrix<Integer> > SymmInput;
    SymmInput[InputType::inequalities]=SymmInequ;
    SymmInput[InputType::equations]=SymmEqu;
    SymmInput[InputType::congruences]=SymmCong;
    SymmInput[InputType::excluded_faces]=SymmExcl;
    SymmInput[InputType::grading]=SymmGrad;
    vector<Integer>  NonNeg(SymmGrad.size(),1);
    SymmInput[InputType::signs]=NonNeg;
    SymmCone=new Cone<Integer>(SymmInput);
    SymmCone->setPolynomial(polynomial);
    SymmCone->setNrCoeffQuasiPol(HSeries.get_nr_coeff_quasipol());
    SymmCone->HSeries.set_period_bounded(HSeries.get_period_bounded());
    SymmCone->setVerbose(verbose);
    ConeProperties SymmToCompute;
    SymmToCompute.set(ConeProperty::SupportHyperplanes);
    SymmToCompute.set(ConeProperty::WeightedEhrhartSeries,ToCompute.test(ConeProperty::HilbertSeries));
    SymmToCompute.set(ConeProperty::VirtualMultiplicity,ToCompute.test(ConeProperty::Multiplicity));
    SymmToCompute.set(ConeProperty::BottomDecomposition,ToCompute.test(ConeProperty::BottomDecomposition));
    SymmCone->compute(SymmToCompute);
    if(SymmCone->isComputed(ConeProperty::WeightedEhrhartSeries)){
        long save_expansion_degree=HSeries.get_expansion_degree(); // not given to the symmetrization
        HSeries=SymmCone->getWeightedEhrhartSeries().first;
        HSeries.set_expansion_degree(save_expansion_degree);
        is_Computed.set(ConeProperty::HilbertSeries);
        is_Computed.set(ConeProperty::ExplicitHilbertSeries);
    }
    if(SymmCone->isComputed(ConeProperty::VirtualMultiplicity)){
        multiplicity=SymmCone->getVirtualMultiplicity();
        is_Computed.set(ConeProperty::Multiplicity);
    }
    is_Computed.set(ConeProperty::Symmetrize);
    return;
    
#endif

}



template<typename Integer>
void integrate(Cone<Integer>& C, const bool do_virt_mult);

template<typename Integer>
void generalizedEhrhartSeries(Cone<Integer>& C);

template<typename Integer>
void Cone<Integer>::compute_integral (ConeProperties& ToCompute){
    if(isComputed(ConeProperty::Integral) || !ToCompute.test(ConeProperty::Integral))
        return;
    if(IntData.getPolynomial()=="")
        throw BadInputException("Polynomial weight missing");
#ifdef NMZ_COCOA
    if(get_rank_internal()==0)
        getIntData().setIntegral(0);
    else
    integrate<Integer>(*this,false);
    is_Computed.set(ConeProperty::Integral);
#endif
}
    
template<typename Integer>
void Cone<Integer>::compute_virt_mult(ConeProperties& ToCompute){
    if(isComputed(ConeProperty::VirtualMultiplicity) || !ToCompute.test(ConeProperty::VirtualMultiplicity))
        return;
    if(IntData.getPolynomial()=="")
        throw BadInputException("Polynomial weight missing");
#ifdef NMZ_COCOA
    if(get_rank_internal()==0)
        getIntData().setVirtualMultiplicity(0);
    else
        integrate<Integer>(*this,true);
    is_Computed.set(ConeProperty::VirtualMultiplicity);
#endif
}

template<typename Integer>
void Cone<Integer>::compute_weighted_Ehrhart(ConeProperties& ToCompute){
    if(isComputed(ConeProperty::WeightedEhrhartSeries) || !ToCompute.test(ConeProperty::WeightedEhrhartSeries))
        return;
    if(IntData.getPolynomial()=="")
        throw BadInputException("Polynomial weight missing");    
    /* if(get_rank_internal()==0)
        throw NotComputableException("WeightedEhrhartSeries not computed in dimenison 0");*/
#ifdef NMZ_COCOA
    generalizedEhrhartSeries(*this);
    is_Computed.set(ConeProperty::WeightedEhrhartSeries);
    if(getIntData().isWeightedEhrhartQuasiPolynomialComputed()){
        is_Computed.set(ConeProperty::WeightedEhrhartQuasiPolynomial);
        is_Computed.set(ConeProperty::VirtualMultiplicity);
    }
#endif
}
//---------------------------------------------------------------------------
template<typename Integer>
bool Cone<Integer>::get_verbose (){
    return verbose;
}

//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::check_Gorenstein(ConeProperties&  ToCompute){
    
    if(!ToCompute.test(ConeProperty::IsGorenstein) || isComputed(ConeProperty::IsGorenstein))
        return;
    if(!isComputed(ConeProperty::SupportHyperplanes))
        compute(ConeProperty::SupportHyperplanes);
    if(!isComputed(ConeProperty::MaximalSubspace))
        compute(ConeProperty::MaximalSubspace);
    
    if(dim==0){
        Gorenstein=true;
        is_Computed.set(ConeProperty::IsGorenstein);
        GeneratorOfInterior=vector<Integer> (dim,0);
        is_Computed.set(ConeProperty::GeneratorOfInterior);
        return;        
    }
    Matrix<Integer> TransfSupps=BasisChangePointed.to_sublattice_dual(SupportHyperplanes);
    assert(TransfSupps.nr_of_rows()>0);
    Gorenstein=false;
    vector<Integer> TransfIntGen = TransfSupps.find_linear_form();
    if(TransfIntGen.size()!=0 && v_scalar_product(TransfIntGen,TransfSupps[0])==1){
        Gorenstein=true;
        GeneratorOfInterior=BasisChangePointed.from_sublattice(TransfIntGen);
        is_Computed.set(ConeProperty::GeneratorOfInterior);
    }
    is_Computed.set(ConeProperty::IsGorenstein);
}

//---------------------------------------------------------------------------
template<typename Integer>
template<typename IntegerFC>
void Cone<Integer>::give_data_of_approximated_cone_to(Full_Cone<IntegerFC>& FC){
    
    // *this is the approximatING cone. The support hyperplanes and equations of the approximatED 
    // cone are given to the Full_Cone produced from *this so that the superfluous points can
    // bre sorted out as early as possible.
    
    assert(is_approximation);
    assert(ApproximatedCone->inhomogeneous ||  ApproximatedCone->getGradingDenom()==1); // in case we generalize later
    
    FC.is_global_approximation=true;
    // FC.is_approximation=true; At present not allowed. Only used for approximation within Full_Cone
    
    // We must distinguish zwo cases: Approximated->Grading_Is_Coordinate or it is not

    // If it is not:
    // The first coordinate in *this is the degree given by the grading
    // in ApproximatedCone. We disregard it by setting the first coordinate
    // of the grading, inequalities and equations to 0, and then have 0 followed
    // by the grading, equations and inequalities resp. of ApproximatedCone.
    
    vector<Integer> help_g;
    if(ApproximatedCone->inhomogeneous)
        help_g=ApproximatedCone->Dehomogenization;
    else
        help_g=ApproximatedCone->Grading;
    
    if(ApproximatedCone->Grading_Is_Coordinate){
        swap(help_g[0],help_g[ApproximatedCone->GradingCoordinate]);
        BasisChangePointed.convert_to_sublattice_dual_no_div(FC.Subcone_Grading,help_g); 
    }        
    else{            
        vector<Integer> help(help_g.size()+1);
        help[0]=0;
        for(size_t j=0;j<help_g.size();++j)
            help[j+1]=help_g[j];
        BasisChangePointed.convert_to_sublattice_dual_no_div(FC.Subcone_Grading,help);
    }
    
    Matrix<Integer> Eq=ApproximatedCone->BasisChangePointed.getEquationsMatrix();
    FC.Subcone_Equations=Matrix<IntegerFC>(Eq.nr_of_rows(),BasisChangePointed.getRank());
    if(ApproximatedCone->Grading_Is_Coordinate){
        Eq.exchange_columns(0,ApproximatedCone->GradingCoordinate);
        BasisChangePointed.convert_to_sublattice_dual(FC.Subcone_Equations,Eq);
    }
    else{
        for(size_t i=0;i<Eq.nr_of_rows();++i){
            vector<Integer> help(Eq.nr_of_columns()+1,0);
            for(size_t j=0;j<Eq.nr_of_columns();++j)
                help[j+1]=Eq[i][j];
            BasisChangePointed.convert_to_sublattice_dual(FC.Subcone_Equations[i], help);       
        }
    }
    
    Matrix<Integer> Supp=ApproximatedCone->SupportHyperplanes;
    FC.Subcone_Support_Hyperplanes=Matrix<IntegerFC>(Supp.nr_of_rows(),BasisChangePointed.getRank());
    
    if(ApproximatedCone->Grading_Is_Coordinate){
        Supp.exchange_columns(0,ApproximatedCone->GradingCoordinate);
        BasisChangePointed.convert_to_sublattice_dual(FC.Subcone_Support_Hyperplanes,Supp);
    }
    else{
        for(size_t i=0;i<Supp.nr_of_rows();++i){
            vector<Integer> help(Supp.nr_of_columns()+1,0);
            for(size_t j=0;j<Supp.nr_of_columns();++j)
                help[j+1]=Supp[i][j];
            BasisChangePointed.convert_to_sublattice_dual(FC.Subcone_Support_Hyperplanes[i], help);       
        }
    }
}

//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::try_approximation_or_projection(ConeProperties& ToCompute){
    
    if((ToCompute.test(ConeProperty::NoProjection) && !ToCompute.test(ConeProperty::Approximate))
           || ToCompute.test(ConeProperty::DualMode) || ToCompute.test(ConeProperty::PrimalMode)
    )
        return;
    
    if(ToCompute.test(ConeProperty::ModuleGeneratorsOverOriginalMonoid))
        return;
    
    if(!inhomogeneous && (  !ToCompute.test(ConeProperty::Deg1Elements) 
                         || ToCompute.test(ConeProperty::HilbertBasis)
                         || ToCompute.test(ConeProperty::HilbertSeries)
                         )                        
      )
        return;
    
    if(inhomogeneous && !ToCompute.test(ConeProperty::HilbertBasis) )
        return;
    
    bool is_parallelotope=false;
    if(!ToCompute.test(ConeProperty::Approximate))
        is_parallelotope=check_parallelotope();
    if(verbose && is_parallelotope)
        verboseOutput() << "Polyhedron is parallelotope" << endl;
    
    if(is_parallelotope){
        SupportHyperplanes.remove_row(Dehomogenization);
        is_Computed.set(ConeProperty::SupportHyperplanes);
        is_Computed.set(ConeProperty::MaximalSubspace);
        is_Computed.set(ConeProperty::Sublattice);
        pointed=true;
        is_Computed.set(ConeProperty::IsPointed);
    }
   
    ConeProperties NeededHere;
    NeededHere.set(ConeProperty::SupportHyperplanes);
    NeededHere.set(ConeProperty::Sublattice);
    NeededHere.set(ConeProperty::MaximalSubspace);
    if(!inhomogeneous)
        NeededHere.set(ConeProperty::Grading);
    compute(NeededHere);
    
    if(!is_parallelotope && !ToCompute.test(ConeProperty::Approximate)){ // we try again
        is_parallelotope=check_parallelotope();
        if(is_parallelotope){
            SupportHyperplanes.remove_row(Dehomogenization);
            is_Computed.set(ConeProperty::SupportHyperplanes);
            is_Computed.set(ConeProperty::MaximalSubspace);
            is_Computed.set(ConeProperty::Sublattice);
            pointed=true;
            is_Computed.set(ConeProperty::IsPointed);
        }
    }
    
    if(!inhomogeneous && !isComputed(ConeProperty::Grading))
        return;
    
     if(!inhomogeneous && ToCompute.test(ConeProperty::Approximate) && GradingDenom!=1)
        return;
    
    if(!pointed || BasisChangePointed.getRank()==0)
        return;
    
    if(inhomogeneous){
        for(size_t i=0;i<Generators.nr_of_rows();++i){
            if(v_scalar_product(Generators[i],Dehomogenization)==0){
                if(ToCompute.test(ConeProperty::Approximate) || ToCompute.test(ConeProperty::Projection))
                    throw NotComputableException("Approximation or Projection not applicable to unbounded polyhedra");
                else
                    return;
            }                    
        }        
    }
    
    if(inhomogeneous){ // exclude that dehoogenization has a gcd > 1
        vector<Integer> test_dehom=BasisChange.to_sublattice_dual_no_div(Dehomogenization);
        if(v_make_prime(test_dehom)!=1)
            return;        
    }
    
    // ****************************************************************
    //
    // NOTE: THE FIRST COORDINATE IS (OR WILL BE MADE) THE GRADING !!!!
    //
    // ****************************************************************
    
    vector<Integer> GradForApprox;
    if(!inhomogeneous)
        GradForApprox=Grading;
    else{
        GradForApprox=Dehomogenization;
        GradingDenom=1;
    }
    
    Grading_Is_Coordinate=false;
    size_t nr_nonzero=0;
    for(size_t i=0;i<dim;++i){
        if(GradForApprox[i]!=0){
            nr_nonzero++;
            GradingCoordinate=i;
        }
    }
    if(nr_nonzero==1){
        if(GradForApprox[GradingCoordinate]==1)
            Grading_Is_Coordinate=true;        
    }
    
    Matrix<Integer> GradGen;
    if(Grading_Is_Coordinate){
        if(!ToCompute.test(ConeProperty::Approximate)){
            GradGen=Generators;
            GradGen.exchange_columns(0,GradingCoordinate); // we swap it into the first coordinate
        }
        else{ // we swap the grading into the first coordinate and approximate
            GradGen.resize(0,dim);
            for(size_t i=0;i<Generators.nr_of_rows();++i){
                vector<Integer> gg=Generators[i];
                swap(gg[0],gg[GradingCoordinate]);
                list<vector<Integer> > approx;
                approx_simplex(gg,approx,1);
                GradGen.append(Matrix<Integer>(approx));
            }    
        }           
    }    
    else{ // to avoid coordinate trabnsformations, we prepend the degree as the first coordinate
        GradGen.resize(0,dim+1); 
        for(size_t i=0;i<Generators.nr_of_rows();++i){
            vector<Integer> gg(dim+1);
            for(size_t j=0;j<dim;++j)
                gg[j+1]=Generators[i][j];
            gg[0]=v_scalar_product(Generators[i],GradForApprox);
            // cout << gg;
            if(ToCompute.test(ConeProperty::Approximate)){
                list<vector<Integer> > approx;
                approx_simplex(gg,approx,1);
                GradGen.append(Matrix<Integer>(approx));
            }
            else
                GradGen.append(gg);            
        }
    }
    
    // data prepared, bow nthe computation
    
    Matrix<Integer> Raw(0,GradGen.nr_of_columns()); // result is returned in this matrix
        
    if(ToCompute.test(ConeProperty::Approximate)){
        if(verbose)
            verboseOutput() << "Computing lattice points by approximation" << endl;
        Cone<Integer> HelperCone(InputType::cone,GradGen);
        HelperCone. ApproximatedCone=&(*this); // we will pass this infornation to the Full_Cone that computes the lattice points.
        HelperCone.is_approximation=true;  // It allows us to discard points outside *this as quickly as possible
        HelperCone.compute(ConeProperty::Deg1Elements,ConeProperty::PrimalMode);
        Raw=HelperCone.getDeg1ElementsMatrix();        
    }
    else{
        if(verbose)
             verboseOutput() << "Computing lattice points by project-and-lift" << endl;
        Matrix<Integer> Supps, Equs;
        if(Grading_Is_Coordinate){
            Supps=SupportHyperplanes;
            Supps.exchange_columns(0,GradingCoordinate);
            Equs=BasisChange.getEquationsMatrix();
            Equs.exchange_columns(0,GradingCoordinate);
        }
        else{
            vector<vector<Integer> > SuppHelp=SupportHyperplanes.get_elements();
            insert_column<Integer>(SuppHelp,0,0);
            Supps=Matrix<Integer>(SuppHelp);
            vector<vector<Integer> > EqusHelp=BasisChange.getEquations();
            if(EqusHelp.size()>0){
                insert_column<Integer>(EqusHelp,0,0);
                Equs=Matrix<Integer>(EqusHelp);
            }
            else
                Equs.resize(0,dim+1);
            vector<Integer> ExtraEqu(Equs.nr_of_columns());
            ExtraEqu[0]=-1;
            for(size_t i=0;i<Grading.size();++i)
                ExtraEqu[i+1]=Grading[i];
            Equs.append(ExtraEqu);
        }
        Supps.append(Equs);  // we must add the equations as pairs of inequalities
        Equs.scalar_multiplication(-1);
        Supps.append(Equs);
        project_and_lift(ToCompute, Raw, GradGen,Supps,ToCompute.test(ConeProperty::ProjectionFloat));        
    }
    
    // computation done. It remains to restore the old coordinates
    
    HilbertBasis=Matrix<Integer>(0,dim);
    Deg1Elements=Matrix<Integer>(0,dim);
    ModuleGenerators=Matrix<Integer>(0,dim);
    
    if(Grading_Is_Coordinate)
        Raw.exchange_columns(0,GradingCoordinate);
    
    Matrix<Integer> Cong=BasisChange.getCongruences();
    
    if(Grading_Is_Coordinate && Cong.nr_of_rows()==0){
        if(inhomogeneous)
            ModuleGenerators.swap(Raw);
        else
            Deg1Elements.swap(Raw);
    }
    else{
        for(size_t i=0;i<Raw.nr_of_rows();++i){
            vector<Integer> rr;
            if(Grading_Is_Coordinate){
                swap(rr,Raw[i]);
            }
            else{
                rr.resize(dim); // remove the prepended grading
                for(size_t j=0;j<dim;++j)
                    rr[j]=Raw[i][j+1];
            }
            bool not_in=false;
            for(size_t k=0;k<Cong.nr_of_rows();++k) {
                if(v_scalar_product_vectors_unequal_lungth(rr,Cong[k]) % Cong[k][dim] !=0) // not in original lattice
                    not_in=true;
                    break;
                }
            if(not_in)
                continue;
            if(inhomogeneous){
                ModuleGenerators.append(rr);
            }
            else
                Deg1Elements.append(rr);        
        }
    }

    setWeights();
    if(inhomogeneous)
         ModuleGenerators.sort_by_weights(WeightsGrad,GradAbs);
    else
        Deg1Elements.sort_by_weights(WeightsGrad,GradAbs);

    if(inhomogeneous){ // as in convert_polyhedron_to polytope of full_cone.cpp
        is_Computed.set(ConeProperty::HilbertBasis);
        is_Computed.set(ConeProperty::ModuleGenerators);
        module_rank= ModuleGenerators.nr_of_rows();
        is_Computed.set(ConeProperty::ModuleRank);
        recession_rank=0;
        is_Computed.set(ConeProperty::RecessionRank);
        if(isComputed(ConeProperty::Grading) && module_rank>0){
            multiplicity=module_rank; // of the recession cone;
            is_Computed.set(ConeProperty::Multiplicity);
            if(ToCompute.test(ConeProperty::HilbertSeries)){
                vector<num_t> hv(1);
                long raw_shift=convertTo<long>(v_scalar_product(Grading,ModuleGenerators[0]));
                for(size_t i=0;i<ModuleGenerators.nr_of_rows();++i){
                    long deg = convertTo<long>(v_scalar_product(Grading,ModuleGenerators[i]));
                    raw_shift=min(raw_shift,deg);                        
                }
                for(size_t i=0;i<ModuleGenerators.nr_of_rows();++i){
                    size_t deg = convertTo<long>(v_scalar_product(Grading,ModuleGenerators[i]))-raw_shift;
                    if(deg+1>hv.size())
                        hv.resize(deg+1);
                    hv[deg]++;                        
                }    
                HSeries.add(hv,vector<denom_t>());
                HSeries.setShift(raw_shift);
                HSeries.adjustShift();
                HSeries.simplify();
                is_Computed.set(ConeProperty::HilbertSeries);
                is_Computed.set(ConeProperty::ExplicitHilbertSeries);
            }
        }  

    }
    else
        is_Computed.set(ConeProperty::Deg1Elements);
    
    is_Computed.set(ConeProperty::Approximate);
    
    return;    
}

//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::project_and_lift(ConeProperties& ToCompute, Matrix<Integer>& Deg1, const Matrix<Integer>& Gens, Matrix<Integer>& Supps, bool float_projection){
    
    // if(verbose)
    //    verboseOutput() << "Starting projection" << endl;
    
    // vector<boost::dynamic_bitset<> > Pair;
   //  vector<boost::dynamic_bitset<> > ParaInPair;
    bool is_parallelotope=(Pair.size()>0);
    
    vector< boost::dynamic_bitset<> > Ind;

    if(!is_parallelotope){
        Ind=vector< boost::dynamic_bitset<> > (Supps.nr_of_rows(), boost::dynamic_bitset<> (Gens.nr_of_rows()));
        for(size_t i=0;i<Supps.nr_of_rows();++i)
            for(size_t j=0;j<Gens.nr_of_rows();++j)
                if(v_scalar_product(Supps[i],Gens[j])==0)
                    Ind[i][j]=true;
    }
        
    size_t rank=BasisChangePointed.getRank();
    
    Matrix<Integer> Verts;
    if(isComputed(ConeProperty::Generators)){
        vector<key_t> choice=identity_key(Gens.nr_of_rows());   //Gens.max_rank_submatrix_lex();
        if(choice.size()>=dim)
            Verts=Gens.submatrix(choice);        
    }
    
    if(float_projection){
        Matrix<nmz_float> SuppsFloat;
        convert(SuppsFloat,Supps);
        vector<Integer> Dummy;
        ProjectAndLift<Integer,MachineInteger> PL;
        if(!is_parallelotope)
            PL=ProjectAndLift<Integer,MachineInteger>(Supps,Ind,rank);
        else
            PL=ProjectAndLift<Integer,MachineInteger>(Supps,Pair,ParaInPair,rank);
        PL.set_grading_denom(convertTo<MachineInteger>(GradingDenom));
        PL.set_verbose(verbose);
        PL.set_LLL(!ToCompute.test(ConeProperty::NoLLL));
        PL.set_no_relax(ToCompute.test(ConeProperty::NoRelax));
        PL.set_vertices(Verts);
        PL.compute(true,true);  // the first true for all_points, the second for float
        Matrix<MachineInteger> Deg1MI(0,Deg1.nr_of_columns());
        PL.put_eg1Points_into(Deg1MI);
        convert(Deg1,Deg1MI);
    }
    else{
        if (change_integer_type) {
            Matrix<MachineInteger> Deg1MI(0,Deg1.nr_of_columns());
            // Matrix<MachineInteger> GensMI;
            Matrix<MachineInteger> SuppsMI;
            try {
                // convert(GensMI,Gens);
                convert(SuppsMI,Supps);
                MachineInteger GDMI=convertTo<MachineInteger>(GradingDenom);
                vector<MachineInteger> Dummy;
                // project_and_lift_inner<MachineInteger>(Deg1MI,SuppsMI,Ind, GDMI,rank,verbose,true,Dummy);
                ProjectAndLift<MachineInteger,MachineInteger> PL;
                if(!is_parallelotope)
                    PL=ProjectAndLift<MachineInteger,MachineInteger>(SuppsMI,Ind,rank);
                else
                    PL=ProjectAndLift<MachineInteger,MachineInteger>(SuppsMI,Pair,ParaInPair,rank);
                PL.set_grading_denom(GDMI);
                PL.set_verbose(verbose);
                PL.set_no_relax(ToCompute.test(ConeProperty::NoRelax));
                PL.set_LLL(!ToCompute.test(ConeProperty::NoLLL));
                Matrix<MachineInteger> VertsMI;
                convert(VertsMI,Verts);
                PL.set_vertices(VertsMI);
                PL.compute();
                PL.put_eg1Points_into(Deg1MI);
            } catch(const ArithmeticException& e) {
                if (verbose) {
                    verboseOutput() << e.what() << endl;
                    verboseOutput() << "Restarting with a bigger type." << endl;
                }
                change_integer_type = false;
            }
            if(change_integer_type){
                convert(Deg1,Deg1MI);                
            }
        }
        
        if (!change_integer_type) {
            vector<Integer> Dummy;
            // project_and_lift_inner<Integer>(Deg1,Supps,Ind,GradingDenom,rank,verbose,true,Dummy);
            ProjectAndLift<Integer,Integer> PL;
            if(!is_parallelotope)
                PL=ProjectAndLift<Integer,Integer>(Supps,Ind,rank);
            else
                PL=ProjectAndLift<Integer,Integer>(Supps,Pair,ParaInPair,rank);
            PL.set_grading_denom(GradingDenom);
            PL.set_verbose(verbose);
            PL.set_no_relax(ToCompute.test(ConeProperty::NoRelax));
            PL.set_LLL(!ToCompute.test(ConeProperty::NoLLL));
            PL.set_vertices(Verts);
            PL.compute();
            PL.put_eg1Points_into(Deg1);
        }        
    }

    is_Computed.set(ConeProperty::Projection);
    if(ToCompute.test(ConeProperty::NoRelax))
        is_Computed.set(ConeProperty::NoRelax);
    if(ToCompute.test(ConeProperty::NoLLL))
        is_Computed.set(ConeProperty::NoLLL);
    if(float_projection)
        is_Computed.set(ConeProperty::ProjectionFloat);
    
    if(verbose)
        verboseOutput() << "Project-and-lift complete" << endl <<
           "------------------------------------------------------------" << endl;
        
}

//---------------------------------------------------------------------------
template<typename Integer>
bool Cone<Integer>::check_parallelotope(){
    
    vector<Integer> Grad; // copy of Grading or Dehomogenization

    if(inhomogeneous){
        Grad=Dehomogenization;
    }
    else{
        if(!isComputed(ConeProperty::Grading))
            return false;
        Grad=Grading;
    }

    Grading_Is_Coordinate=false;
    size_t nr_nonzero=0;
    for(size_t i=0;i<Grad.size();++i){
        if(Grad[i]!=0){
            nr_nonzero++;
            GradingCoordinate=i;
        }
    }
    if(nr_nonzero==1){
        if(Grad[GradingCoordinate]==1)
            Grading_Is_Coordinate=true;        
    }
    if(!Grading_Is_Coordinate)
        return false;
    if(Equations.nr_of_rows()>0)
        return false;
    
    Matrix<Integer> Supps(SupportHyperplanes);
    if(inhomogeneous)
        Supps.remove_row(Grad);        

    size_t dim=Supps.nr_of_columns()-1; //affine dimension
    if(Supps.nr_of_rows()!=2*dim)
        return false;
    Pair.resize(2*dim);
    ParaInPair.resize(2*dim);
    for(size_t i=0;i<2*dim;++i){
        Pair[i].resize(dim);
        Pair[i].reset();
        ParaInPair[i].resize(dim);
        ParaInPair[i].reset();
    }

    vector<bool> done(2*dim);
    Matrix<Integer> M2(2,dim+1), M3(3,dim+1);
    M3[2]=Grad;
    size_t pair_counter=0;
    
    vector<key_t> Supp_1; // to find antipodal vertices
    vector<key_t> Supp_2;
    
    for(size_t i=0;i<2*dim;++i){
        if(done[i])
            continue;
        bool parallel_found=false;
        M2[0]=Supps[i];
        M3[0]=Supps[i];
        size_t j=i+1;
        for(;j<2*dim;++j){
            if(done[j]) continue;
            M2[1]=Supps[j];
            if(M2.rank()<2)
                continue;
            M3[1]=Supps[j];
            if(M3.rank()==3)
                continue;
            else{
                parallel_found=true;
                done[j]=true;
                break;
            }
        }
        if(!parallel_found)
            return false;
        Supp_1.push_back(i);
        Supp_2.push_back(j);
        Pair[i][pair_counter]=true;
        Pair[j][pair_counter]=true;
        ParaInPair[j][pair_counter]=true;
        pair_counter++;
    }
    
    Matrix<Integer> v1=Supps.submatrix(Supp_1).kernel(); // opposite vertices
    Matrix<Integer> v2=Supps.submatrix(Supp_2).kernel();
    Integer MinusOne=-1;
    if(v_scalar_product(v1[0],Grad)==0)
        return false;
    if(v_scalar_product(v2[0],Grad)==0)
        return false;
    if(v_scalar_product(v1[0],Grad)<0)
        v_scalar_multiplication(v1[0],MinusOne);
    if(v_scalar_product(v2[0],Grad)<0)
        v_scalar_multiplication(v2[0],MinusOne);
    if(v1.nr_of_rows()!=1 || v2.nr_of_rows()!=1)
        return false;
    for(size_t i=0;i<Supp_1.size();++i){
        if(!(v_scalar_product(Supps[Supp_1[i]],v2[0])>0))
            return false;
    }
    for(size_t i=0;i<Supp_2.size();++i){
        if(!(v_scalar_product(Supps[Supp_2[i]],v1[0])>0))
            return false;
    }
    
    // we have found opposite vertices
    
    return true;    
}

//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::compute_volume(ConeProperties& ToCompute){
    
    if(!ToCompute.test(ConeProperty::Volume))
        return;
    if(!inhomogeneous){
        volume=multiplicity;
        compute_euclidean_volume(Grading);
        is_Computed.set(ConeProperty::EuclideanVolume);
        is_Computed.set(ConeProperty::Volume);
        return;
    }

    compute(ConeProperty::Generators);
    compute(ConeProperty::AffineDim);
    
    if(affine_dim<=0){
        volume=1;
        euclidean_volume=0;
        is_Computed.set(ConeProperty::Volume);
        is_Computed.set(ConeProperty::EuclideanVolume);
        return;
    }
    
    for(size_t i=0;i<Generators.nr_of_rows();++i){
        if(v_scalar_product(Generators[i],Dehomogenization)==0)
            throw NotComputableException("Volume not computable for unbounded polyhedra");
    }
    Cone<Integer> VolCone(Type::cone,Generators,Type::lattice,
                          get_sublattice_internal().getEmbeddingMatrix(), Type::grading,Dehomogenization);
    VolCone.compute(ConeProperty::Multiplicity);
    volume=VolCone.getMultiplicity();
    compute_euclidean_volume(Dehomogenization);
    is_Computed.set(ConeProperty::Volume);
    is_Computed.set(ConeProperty::EuclideanVolume);
    return;
}

//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::compute_euclidean_volume(const vector<Integer>& Grad){ 

    Matrix<Integer> Simplex=BasisChangePointed.getEmbeddingMatrix();
    // Matrix<Integer> Simplex=Generators.submatrix(Generators.max_rank_submatrix_lex()); -- numerically bad !!!!
    size_t n=Simplex.nr_of_rows();
    vector<Integer> raw_degrees=Simplex.MxV(Grad);
    size_t non_zero=0;
    for(size_t i=0;i<raw_degrees.size();++i)
        if(raw_degrees[i]!=0){
            non_zero=i;
            break;
        }
    Integer MinusOne=-1;
    if(raw_degrees[non_zero]<0)
        v_scalar_multiplication(Simplex[non_zero],MinusOne); // makes this degree > 0
    for(size_t i=0;i<n;++i){
        if(raw_degrees[i]==0)
            Simplex[i]=v_add(Simplex[i],Simplex[non_zero]); // makes this degree > 0
        if(raw_degrees[i]<0)
            v_scalar_multiplication(Simplex[i],MinusOne); // ditto
    }
    vector<Integer> degrees=Simplex.MxV(Grad);
    
    // we compute the lattice normalized volume and later the euclidean volume
    // of the simplex defined by Simplex to get the correction factor
    Cone<Integer> VolCone(Type::cone,Simplex,Type::lattice,
                          get_sublattice_internal().getEmbeddingMatrix(), Type::grading,Grad);
    VolCone.setVerbose(false);
    VolCone.compute(ConeProperty::Multiplicity, ConeProperty::NoBottomDec);
    mpq_class norm_vol_simpl=VolCone.getMultiplicity();
    // lattice normalized volume of our simplex Simplex
        
    // now the euclideal volime
    Matrix<nmz_float> Bas;
    convert(Bas,Simplex);

    // go into hyperplane Grad=1
    for(size_t i=0;i<n;++i)
        v_scalar_division(Bas[i],convertTo<nmz_float>(degrees[i]));
    // choose an origin, namely Bas[0]
    Matrix<nmz_float> Bas1(n-1,dim);
    for(size_t i=1;i<n;++i)
        for(size_t j=0;j<dim;++j)
            Bas1[i-1][j]=Bas[i][j]-Bas[0][j]; 

    //orthogonalize Bas1
    Matrix<double> G(n,dim);
    Matrix<double> M(n,n);
    Bas1.GramSchmidt(G,M,0,n-1);
    // compute euclidean volume
    nmz_float eucl_vol_simpl=1;
    for(size_t i=0;i<n-1;++i)
        eucl_vol_simpl*=sqrt(v_scalar_product(G[i],G[i]));
    // now the correction
    nmz_float fact;
    convert(fact,nmz_factorial((long) n-1));
    nmz_float corr_factor=eucl_vol_simpl/mpq_to_nmz_float(norm_vol_simpl);
    euclidean_volume=mpq_to_nmz_float(volume)*corr_factor/fact;    
}

//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::compute_projection(ConeProperties& ToCompute){
    
    if(!ToCompute.test(ConeProperty::ProjectCone))
        return;
    
    if(projection_coord_indicator==vector<bool>(dim))
        throw BadInputException("Projection to zero coordinates make no sense");
    
    if(projection_coord_indicator==vector<bool>(dim,true))
        throw BadInputException("Projection to all coordinates make no sense");
        
    vector<Integer> GradOrDehom, GradOrDehomProj;
    if(inhomogeneous)
        GradOrDehom=Dehomogenization;
    else
        if(isComputed(ConeProperty::Grading))
            GradOrDehom=Grading;
    for(size_t i=0;i<GradOrDehom.size();++i){
        if(!projection_coord_indicator[i]){
            if(GradOrDehom[i]!=0)
                throw BadInputException("Grading or Dehomogenization not compatible with projection");
        }
        else
            GradOrDehomProj.push_back(GradOrDehom[i]);
    }
        
    if(isComputed(ConeProperty::Generators))
        compute_projection_from_gens(GradOrDehomProj);
    else
        compute_projection_from_constraints(GradOrDehomProj);
    
    is_Computed.set(ConeProperty::ProjectCone);
    
}
//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::compute_projection_from_gens(const vector<Integer>& GradOrDehomProj){
    
    Matrix<Integer> GensProj=Generators.select_columns(projection_coord_indicator);
    map< InputType, Matrix<Integer> > ProjInput;
    ProjInput[Type::cone]=GensProj;
    if(GradOrDehomProj.size()>0){
        if(inhomogeneous)
            ProjInput[Type::dehomogenization]=GradOrDehomProj;
        else
            ProjInput[Type::grading]=GradOrDehomProj;           
    }
    ProjCone=new Cone<Integer>(ProjInput);
    if(verbose)
        verboseOutput() << "Computing projection from generators" << endl;
    ProjCone->compute(ConeProperty::SupportHyperplanes);
}

//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::compute_projection_from_constraints(const vector<Integer>& GradOrDehomProj){

    compute_generators();
    Matrix<Integer> Gens=Generators.selected_columns_first(projection_coord_indicator);
    Matrix<Integer> ReorderedBasis=BasisMaxSubspace.selected_columns_first(projection_coord_indicator);
    Gens.append(ReorderedBasis);   
       
    Matrix<Integer> Supps=SupportHyperplanes.selected_columns_first(projection_coord_indicator);
    Matrix<Integer> ReorderedEquations= BasisChange.getEquationsMatrix().selected_columns_first(projection_coord_indicator);
    Supps.append(ReorderedEquations);
    Integer MinusOne=-1;
    ReorderedEquations.scalar_multiplication(MinusOne);
    Supps.append(ReorderedEquations);

    vector< boost::dynamic_bitset<> > Ind;

    Ind=vector< boost::dynamic_bitset<> > (Supps.nr_of_rows(), boost::dynamic_bitset<> (Gens.nr_of_rows()));
    for(size_t i=0;i<Supps.nr_of_rows();++i)
        for(size_t j=0;j<Gens.nr_of_rows();++j)
            if(v_scalar_product(Supps[i],Gens[j])==0)
                Ind[i][j]=true;

    size_t proj_dim=0;
    for(size_t i=0;i<projection_coord_indicator.size();++i)
        if(projection_coord_indicator[i])
            proj_dim++;
            
    ProjectAndLift<Integer,Integer> PL;    
    PL=ProjectAndLift<Integer,Integer>(Supps,Ind,BasisChangePointed.getRank());
    if(verbose)
        verboseOutput() << "Computing constraints of projection" << endl;
    PL.set_verbose(verbose);
    PL.compute_only_projection(proj_dim);
    Matrix<Integer> SuppsProj, EqusProj;
    PL.putSuppsAndEqus(SuppsProj,EqusProj,proj_dim);
    if(SuppsProj.nr_of_rows()==0)
        SuppsProj.append(vector<Integer>(SuppsProj.nr_of_columns(),0)); // to avoid completely empty input matrices

    map< InputType, Matrix<Integer> > ProjInput;
    if(GradOrDehomProj.size()>0){
        if(inhomogeneous)
            ProjInput[Type::dehomogenization]=GradOrDehomProj;
        else
            ProjInput[Type::grading]=GradOrDehomProj;           
    }
    ProjInput[Type::support_hyperplanes]=SuppsProj;
    ProjInput[Type::equations]=EqusProj;
    
    Matrix<Integer> GensProj=Generators.select_columns(projection_coord_indicator);
    Matrix<Integer> BasHelp=BasisMaxSubspace.select_columns(projection_coord_indicator);
    GensProj.append(BasHelp);
    BasHelp.scalar_multiplication(MinusOne);
    GensProj.append(BasHelp);   
    ProjInput[Type::cone]=GensProj;
    
    ProjCone=new Cone<Integer>(ProjInput);
    ProjCone->compute(ConeProperty::SupportHyperplanes, ConeProperty::ExtremeRays);

}   
    

#ifndef NMZ_MIC_OFFLOAD  //offload with long is not supported
template class Cone<long>;
#endif
template class Cone<long long>;
template class Cone<mpz_class>;

} // end namespace libnormaliz
