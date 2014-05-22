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

#include <list>
#include "cone.h"
#include "full_cone.h"
// #include "vector_operations.h"

namespace libnormaliz {
using namespace std;

// adds the signs inequalities given by Signs to Inequalities
template<typename Integer>
Matrix<Integer> sign_inequalities(const vector< vector<Integer> >& Signs) {
    if (Signs.size() != 1) {
        errorOutput() << "ERROR: Bad signs matrix, has "
                      << Signs.size() << " rows (should be 1)!" << endl;
        throw BadInputException();
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
            errorOutput() << "ERROR: Bad signs matrix, has entry "
                          << sign << " (should be -1, 1 or 0)!" << endl;
            throw BadInputException();
        }
    }
    return Inequ;
}

template<typename Integer>
Matrix<Integer> strict_sign_inequalities(const vector< vector<Integer> >& Signs) {
    if (Signs.size() != 1) {
        errorOutput() << "ERROR: Bad signs matrix, has "
                      << Signs.size() << " rows (should be 1)!" << endl;
        throw BadInputException();
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
            errorOutput() << "ERROR: Bad signs matrix, has entry "
                          << sign << " (should be -1, 1 or 0)!" << endl;
            throw BadInputException();
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
            case Type::excluded_faces:
            case Type::dehomogenization:
                errorOutput() << "This InputType combination is currently not supported!"<< endl;
                throw BadInputException();
                break;
            case Type::inhom_inequalities: // nothing to do
            case Type::inhom_equations:    // ditto
            case Type::inhom_congruences:  // ditto
            case Type::grading:  // already taken care of
                break;
            case Type::strict_inequalities:
                insert_column<Integer>(it->second,dim-1,-1);
                break;
            default:  // is correct for signs and strict_signs !
                insert_zero_column<Integer>(it->second,dim-1);
                break;
        }
    }   

}


//---------------------------------------------------------------------------

template<typename Integer>
Cone<Integer>::Cone(const vector< vector<Integer> >& Input, InputType input_type) {
    initialize();
    if(Input.size()==0){
        errorOutput() << "All input matrices empty!"<< endl;
        throw BadInputException();
    }
    // convert single matrix into a map
    map< InputType, vector< vector<Integer> > > multi_input_data;
    multi_input_data.insert(pair< InputType, vector< vector<Integer> > >(input_type,Input));
    process_multi_input(multi_input_data);
}

template<typename Integer>
Cone<Integer>::Cone(const map< InputType, vector< vector<Integer> > >& multi_input_data) {
    initialize();
    process_multi_input(multi_input_data);
}

//---------------------------------------------------------------------------


template<typename Integer>
void Cone<Integer>::process_multi_input(const map< InputType, vector< vector<Integer> > >& multi_input_data_const) {
    initialize();
    map< InputType, vector< vector<Integer> > > multi_input_data(multi_input_data_const);
    typename map< InputType , vector< vector<Integer> > >::iterator it=multi_input_data.begin();
    
    // find basic input type
    bool constraints_input=false, generators_input=false, lattice_ideal_input=false;
    size_t nr_types=0;
    size_t nr_strict_input=0; // grading, dehomogenization and excluded_faces are non-strict input
    bool inhom_input=false;
    
    it = multi_input_data.begin();
    for(; it != multi_input_data.end(); ++it) {
        switch (it->first) {
            case Type::inhom_inequalities:
            case Type::inhom_equations:
            case Type::inhom_congruences:
            case Type::strict_inequalities:
            case Type::strict_signs:
                inhom_input=true;             
            case Type::signs:              
            case Type::inequalities:
            case Type::equations:
            case Type::congruences:
                if(!constraints_input)
                    nr_types++;
                nr_strict_input++;
                constraints_input=true;
                break;
            case Type::lattice_ideal:
            if(!lattice_ideal_input)
                    nr_types++;
                nr_strict_input++;
                lattice_ideal_input=true;
                break;
            case Type::polyhedron:
                inhom_input=true;
            case Type::integral_closure:
            case Type::normalization:
            case Type::rees_algebra:
            case Type::polytope:
            if(!generators_input)
                    nr_types++;
                nr_strict_input++;
                generators_input=true;
                break;
            default:
                break;        
        }
    
    }
    if(nr_types>=2){
        errorOutput() << "(1) This InputType combination is currently not supported!"<< endl;
        throw BadInputException();
    }
    if(nr_types==0){  // we have only a grading, dehomogenization or excluded faces
        constraints_input=true;       
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
        errorOutput() << "All input matrices empty!"<< endl;
        throw BadInputException();
    }
        
    //determine dimension
    it = multi_input_data.begin();
    
    if(inhom_input){
        for(; it != multi_input_data.end(); ++it) { // there must be at least one inhom_input type
            switch(it->first){
                case Type::strict_inequalities:
                case Type::strict_signs:
                    dim = it->second.front().size()+1;
                    break;
                case Type::inhom_inequalities:
                case Type::inhom_equations:
                case Type::polyhedron:
                    dim = it->second.front().size();
                    break;
                case Type::inhom_congruences:
                    dim = it->second.front().size()-1; //congruences have one extra column
                    break;
                default: break;
            }
        }
    }
    else{
        for(; it != multi_input_data.end(); ++it) {
            dim = it->second.front().size();
            if (it->first == Type::rees_algebra || it->first == Type::polytope) {
                dim++; // we add one component
            }
            if (it->first == Type::congruences) {
                dim--; //congruences have one extra column
            }
            break;
        }
    }
    
    // cout << "inhom " << inhom_input << " dim " << dim << endl;

    // for generators we can have only one strict input
    if(generators_input && nr_strict_input >1){
        errorOutput() << "This InputType combination is currently not supported!"<< endl;
        throw BadInputException();        
    }
    
     
    // We now process input types that are independent of generators, constraints, lattice_ideal
    
    // check for excluded faces
    ExcludedFaces = find_input_matrix(multi_input_data,Type::excluded_faces);
    
    // check for a grading
    vector< vector<Integer> > lf = find_input_matrix(multi_input_data,Type::grading);
    if (lf.size() > 1) {
        errorOutput() << "ERROR: Bad grading, has "
                      << lf.size() << " rows (should be 1)!" << endl;
        throw BadInputException();
    }
    if(lf.size()==1){
        if(inhom_input)
            lf[0].push_back(0); // first we extend grading trivially to have the right dimension
        setGrading (lf[0]);     // will eveantually be set in full_cone.cpp
        
    }
    
    // check for a dehomogenization
    lf = find_input_matrix(multi_input_data,Type::dehomogenization);
    if (lf.size() > 1) {
        errorOutput() << "ERROR: Bad dehomogenization, has "
                      << lf.size() << " rows (should be 1)!" << endl;
        throw BadInputException();
    }
    if(lf.size()==1){
        setDehomogenization(lf[0]);
    }
    
    // check consistence of dimension
    size_t inhom_corr=0; // coorection in the inhom_input case
    if(inhom_input)
        inhom_corr=1;
    it = multi_input_data.begin();
    size_t current_dim, test_dim;
    for(; it != multi_input_data.end(); ++it) {
        current_dim=it->second[0].size()+inhom_corr;      
        switch (it->first) {
            case Type::inhom_congruences:
                test_dim=current_dim-2;
                break;
            case Type::polyhedron:
            case Type::inhom_inequalities:
            case Type::inhom_equations:
            case Type::congruences:
                test_dim=current_dim-1;
                break;
            case Type::polytope:
            case Type::rees_algebra:
                test_dim=current_dim+1;
                break;
            default:
                test_dim=current_dim;
                break;        
        }
        if(test_dim!=dim){
            errorOutput() << "Inconsistent dimensions in input!"<< endl;
            throw BadInputException();           
        }
    }
    
    if(inhom_input && constraints_input)
        homogenize_input(multi_input_data);
        
    // now we can unify implicit and explicit truncation
    // Note: implicit and explicit truncation have already been excluded
    if (inhom_input) {
        Dehomogenization.resize(dim),
        Dehomogenization[dim-1]=1;
        is_Computed.set(ConeProperty::Dehomogenization);
    }        
    if(isComputed(ConeProperty::Dehomogenization))
        inhomogeneous=true;
        
    if(inhomogeneous && ExcludedFaces.nr_of_rows()>0){
        errorOutput() << "This InputType combination is currently not supported!"<< endl;
        throw BadInputException();
    }
    
    if(lattice_ideal_input){
        prepare_input_lattice_ideal(multi_input_data);
    }
    
    if(generators_input){
        prepare_input_generators(multi_input_data);
    }
    
    if(constraints_input){
        prepare_input_constraints(multi_input_data);
    }
    
    if(!BC_set) compose_basis_change(Sublattice_Representation<Integer>(dim));
}

//---------------------------------------------------------------------------
        
template<typename Integer>
void Cone<Integer>::prepare_input_constraints(const map< InputType, vector< vector<Integer> > >& multi_input_data) {

    Matrix<Integer> Equations(0,dim), Congruences(0,dim+1), Signs(0,dim), StrictSigns(0,dim);
    
    Matrix<Integer> Inequalities(0,dim);
    
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

    prepare_input_type_456(Congruences, Equations, Inequalities);
}

//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::check_trunc_nonneg(const vector< vector<Integer> >& input_gens){

    if(!inhomogeneous)
        return;
    for(size_t i=0;i<input_gens.size();++i)
        if(v_scalar_product(input_gens[i],Dehomogenization)<0){
            errorOutput() << "Negative value of dehomogenization on generator " << i+1 << " !" << endl;
            throw BadInputException();
        }
}

//---------------------------------------------------------------------------
template<typename Integer>
void Cone<Integer>::prepare_input_generators(const map< InputType, vector< vector<Integer> > >& multi_input_data) {

    typename map< InputType , vector< vector<Integer> > >::const_iterator it=multi_input_data.begin();    
    // find specific generator type -- there is only one, as checked already
    for(; it != multi_input_data.end(); ++it) {
        switch (it->first) {
            case Type::polyhedron:
                check_trunc_nonneg(it->second);
            case Type::integral_closure:
                check_trunc_nonneg(it->second);
                prepare_input_type_0(it->second); 
                break;
            case Type::normalization:
                if(inhomogeneous){
                    errorOutput() << "Dehomogenization not allowed for normalization!" << endl;
                    throw BadInputException();
                }    
                prepare_input_type_1(it->second); 
                break;
            case Type::polytope:         
                if(isComputed(ConeProperty::Grading)){
                    errorOutput() << "Explicit grading not allowed for polytope!" << endl;
                    throw BadInputException();
                }
                if(inhomogeneous){
                    errorOutput() << "Dehomogenization not allowed for polytope!" << endl;
                    throw BadInputException();
                }
                prepare_input_type_2(it->second); 
                break;
            case Type::rees_algebra: 
                if(ExcludedFaces.nr_of_rows()>0){
                    errorOutput() << "excluded_faces not allowed for rees_algebra!" << endl;
                    throw BadInputException();
                }
                if(inhomogeneous){
                    errorOutput() << "Dehomogenization not allowed for rees_algrebra!" << endl;
                    throw BadInputException();
                }
                prepare_input_type_3(it->second); 
                break;
            default: break;
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::prepare_input_lattice_ideal(const map< InputType, vector< vector<Integer> > >& multi_input_data) {

    if(ExcludedFaces.nr_of_rows()>0){
        errorOutput() << "Excluded faces not allowed for lattice ideal input!" << endl;
        throw BadInputException();
    }
    if(inhomogeneous){ // if true and not yet caught, a dehomogenization must have appeared explicitly
        errorOutput() << "Dehomogenization not allowed for lattice ideal input!" << endl;
        throw BadInputException();
    }
        
    Matrix<Integer> Binomials(find_input_matrix(multi_input_data,Type::lattice_ideal));
    
    if (isComputed(ConeProperty::Grading)) {
        //check if binomials are homogeneous
        vector<Integer> degrees = Binomials.MxV(Grading);
        for (size_t i=0; i<degrees.size(); ++i) {
            if (degrees[i]!=0) {
                errorOutput() << "Grading gives non-zero value " << degrees[i]
                              << " for binomial " << i+1 << "!" << endl;
                throw BadInputException();
            }
            if (Grading[i] <= 0) {
                errorOutput() << "Grading gives non-positive value " << Grading[i]
                            << " for generator " << i+1 << "!" << endl;
                throw BadInputException();
            }
        }
    }
    
    Matrix<Integer> Generators=Binomials.kernel().transpose();
    Full_Cone<Integer> FC(Generators);
    //TODO verboseOutput(), what is happening here?

    FC.support_hyperplanes();
    Matrix<Integer> Supp_Hyp=FC.getSupportHyperplanes();
    Matrix<Integer> Selected_Supp_Hyp_Trans=(Supp_Hyp.submatrix(Supp_Hyp.max_rank_submatrix_lex())).transpose();
    Matrix<Integer> Positive_Embedded_Generators=Generators.multiplication(Selected_Supp_Hyp_Trans);
    GeneratorsOfToricRing = Positive_Embedded_Generators;
    is_Computed.set(ConeProperty::GeneratorsOfToricRing);
    dim = Positive_Embedded_Generators.nr_of_columns();

    if (isComputed(ConeProperty::Grading)) {
        // solve GeneratorsOfToricRing * grading = old_grading
        Grading = Positive_Embedded_Generators.solve(Grading);
        if (Grading.size() != dim) {
            errorOutput() << "Grading could not be transfered!"<<endl;
            is_Computed.set(ConeProperty::Grading, false);
        }
    }
    prepare_input_type_1(GeneratorsOfToricRing.get_elements()); //TODO
}

/* only used by the constructors */
template<typename Integer>
void Cone<Integer>::initialize() {
    BC_set=false;
    is_Computed = bitset<ConeProperty::EnumSize>();  //initialized to false
    dim = 0;
    inhomogeneous=false;
    rees_primary = false;
}

/* check what is computed */
template<typename Integer>
bool Cone<Integer>::isComputed(ConeProperty::Enum prop) const {
    return is_Computed.test(prop);
}

template<typename Integer>
bool Cone<Integer>::isComputed(ConeProperties CheckComputed) const {
    return CheckComputed.reset(is_Computed).any();
}


/* getter */
template<typename Integer>
size_t Cone<Integer>::getRank() const {
    return BasisChange.get_rank();
}

template<typename Integer>
size_t Cone<Integer>::getRecessionRank() const {
    return recession_rank;
}

template<typename Integer>
long Cone<Integer>::getAffineDim() const {
    return affine_dim;
}

template<typename Integer>
Sublattice_Representation<Integer> Cone<Integer>::getBasisChange() const{
    return BasisChange;
}

template<typename Integer>
Matrix<Integer> Cone<Integer>::getGeneratorsOfToricRingMatrix() const {
    return GeneratorsOfToricRing;
}
template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getGeneratorsOfToricRing() const {
    return GeneratorsOfToricRing.get_elements();
}

template<typename Integer>
Matrix<Integer> Cone<Integer>::getGeneratorsMatrix() const {
    return Generators;
}
template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getGenerators() const {
    return Generators.get_elements();
}

template<typename Integer>
Matrix<Integer> Cone<Integer>::getExtremeRaysMatrix() const {
    if (inhomogeneous) { // return only the rays of the recession cone
        return Generators.submatrix(v_bool_andnot(ExtremeRays,VerticesOfPolyhedron));
    }
    // homogeneous case
    return Generators.submatrix(ExtremeRays);
}
template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getExtremeRays() const {
    return getExtremeRaysMatrix().get_elements();
}

template<typename Integer>
Matrix<Integer> Cone<Integer>::getVerticesOfPolyhedronMatrix() const {
    return Generators.submatrix(VerticesOfPolyhedron);
}
template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getVerticesOfPolyhedron() const {
    return Generators.submatrix(VerticesOfPolyhedron).get_elements();
}

template<typename Integer>
Matrix<Integer> Cone<Integer>::getSupportHyperplanesMatrix() const {
   return SupportHyperplanes;
}
template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getSupportHyperplanes() const {
   return SupportHyperplanes.get_elements();
}

template<typename Integer>
Matrix<Integer> Cone<Integer>::getEquationsMatrix() const {
    size_t rank = BasisChange.get_rank();
    if (rank == 0)                   // the zero cone
        return Matrix<Integer>(dim); // identity matrix
    else 
        return Generators.submatrix(ExtremeRays).kernel();
}
template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getEquations() const {
    return getEquationsMatrix().get_elements();
}

template<typename Integer>
Matrix<Integer> Cone<Integer>::getCongruencesMatrix() const {
    return BasisChange.get_congruences();
}
template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getCongruences() const {
    return BasisChange.get_congruences().get_elements();
}

template<typename Integer>
map< InputType , vector< vector<Integer> > > Cone<Integer>::getConstraints () const {
    map<InputType, vector< vector<Integer> > > c;
    c[Type::inequalities] = SupportHyperplanes.get_elements();
    c[Type::equations] = getEquations();
    c[Type::congruences] = getCongruences();
    return c;
}

template<typename Integer>
Matrix<Integer> Cone<Integer>::getExcludedFacesMatrix() const {
    return ExcludedFaces;
}
template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getExcludedFaces() const {
    return ExcludedFaces.get_elements();
}

template<typename Integer>
const vector< pair<vector<key_t>,Integer> >& Cone<Integer>::getTriangulation() const {
    return Triangulation;
}

template<typename Integer>
const vector< pair<vector<key_t>,long> >& Cone<Integer>::getInclusionExclusionData() const {
    return InExData;
}

template<typename Integer>
const list< STANLEYDATA<Integer> >& Cone<Integer>::getStanleyDec() const {
    return StanleyDec;
}

template<typename Integer>
size_t Cone<Integer>::getTriangulationSize() const {
    return TriangulationSize;
}

template<typename Integer>
Integer Cone<Integer>::getTriangulationDetSum() const {
    return TriangulationDetSum;
}

template<typename Integer>
Matrix<Integer> Cone<Integer>::getHilbertBasisMatrix() const {
    return HilbertBasis;
}
template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getHilbertBasis() const {
    return HilbertBasis.get_elements();
}

template<typename Integer>
Matrix<Integer> Cone<Integer>::getModuleGeneratorsMatrix() const {
    return ModuleGenerators;
}
template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getModuleGenerators() const {
    return ModuleGenerators.get_elements();
}

template<typename Integer>
Matrix<Integer> Cone<Integer>::getDeg1ElementsMatrix() const {
    return Deg1Elements;
}
template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getDeg1Elements() const {
    return Deg1Elements.get_elements();
}

template<typename Integer>
const HilbertSeries& Cone<Integer>::getHilbertSeries() const {
    return HSeries;
}

template<typename Integer>
vector<Integer> Cone<Integer>::getGrading() const {
    return Grading;
}

template<typename Integer>
Integer Cone<Integer>::getGradingDenom() const {
    return GradingDenom;
}

template<typename Integer>
vector<Integer> Cone<Integer>::getDehomogenization() const {
    return Dehomogenization;
}

template<typename Integer>
mpq_class Cone<Integer>::getMultiplicity() const {
    return multiplicity;
}

template<typename Integer>
bool Cone<Integer>::isPointed() const {
    return pointed;
}

template<typename Integer>
bool Cone<Integer>::isInhomogeneous() const {
    return inhomogeneous;
}

template<typename Integer>
bool Cone<Integer>::isDeg1ExtremeRays() const {
    return deg1_extreme_rays;
}

template<typename Integer>
bool Cone<Integer>::isDeg1HilbertBasis() const {
    return deg1_hilbert_basis;
}

template<typename Integer>
bool Cone<Integer>::isIntegrallyClosed() const {
    return integrally_closed;
}

template<typename Integer>
bool Cone<Integer>::isReesPrimary() const {
    return rees_primary;
}

template<typename Integer>
Integer Cone<Integer>::getReesPrimaryMultiplicity() const {
    return ReesPrimaryMultiplicity;
}


template<typename Integer>
Integer Cone<Integer>::getShift() const {
    return shift;
}

template<typename Integer>
size_t Cone<Integer>::getModuleRank() const {
    return module_rank;
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
void Cone<Integer>::prepare_input_type_0(const vector< vector<Integer> >& Input) {
    Generators = Input;
    is_Computed.set(ConeProperty::Generators);
    Sublattice_Representation<Integer> Basis_Change(Generators,true);
    compose_basis_change(Basis_Change);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::prepare_input_type_1(const vector< vector<Integer> >& Input) {
    Generators = Input;
    is_Computed.set(ConeProperty::Generators);

    Sublattice_Representation<Integer> Basis_Change(Generators,false);
    compose_basis_change(Basis_Change);
}

//---------------------------------------------------------------------------

/* polytope input */
template<typename Integer>
void Cone<Integer>::prepare_input_type_2(const vector< vector<Integer> >& Input) {
    size_t j;
    size_t nr = Input.size();
    //append a column of 1
    Generators = Matrix<Integer>(nr, dim);
    for (size_t i=0; i<nr; i++) {
        for (j=0; j<dim-1; j++) 
            Generators[i][j] = Input[i][j];
        Generators[i][dim-1]=1;
    }
    is_Computed.set(ConeProperty::Generators);

    compose_basis_change(Sublattice_Representation<Integer>(Generators,true));

    // use the added last component as grading
    Grading = vector<Integer>(dim,0);
    Grading[dim-1] = 1;
    is_Computed.set(ConeProperty::Grading);
}

//---------------------------------------------------------------------------

/* rees input */
template<typename Integer>
void Cone<Integer>::prepare_input_type_3(const vector< vector<Integer> >& InputV) {
    Matrix<Integer> Input(InputV);
    int i,j,k,l,nr_rows=Input.nr_of_rows(), nr_columns=Input.nr_of_columns();
    rees_primary=true;
    Integer number;
    Matrix<Integer> Full_Cone_Generators(nr_rows+nr_columns,nr_columns+1,0);
    for (i = 0; i < nr_columns; i++) {
        Full_Cone_Generators.write(i,i,1);
    }
    for(i=0; i<nr_rows; i++){
        Full_Cone_Generators.write(i+nr_columns,nr_columns,1);
        for(j=0; j<nr_columns; j++) {
            number=Input.read(i,j);
            Full_Cone_Generators.write(i+nr_columns,j,number);
        }
    }
    Matrix<Integer> Prim_Test=Input;
    for(i=0; i<nr_rows; i++){           //preparing the matrix for primarity test
        k=0;
        for(j=0; j<nr_columns; j++) {
            if (k<2) {
                if (Input.read(i,j)!=0 )
                    k++;
            }
            if (k==2) {
                for (l = 0; l < nr_columns; l++) {
                    Prim_Test.write(i,l,0);
                }
                break;
            }
        }
    }
    for(j=0; j<nr_columns; j++) {         //primarity test
        for(i=0; i<nr_rows && Prim_Test.read(i,j)==0; i++) {}
        if (i>=nr_rows) {
            rees_primary=false;
            break;
        }
    }
    is_Computed.set(ConeProperty::ReesPrimary);
    Generators = Full_Cone_Generators;
    is_Computed.set(ConeProperty::Generators);

    compose_basis_change(Sublattice_Representation<Integer>(Full_Cone_Generators.nr_of_columns()));

}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::prepare_input_type_456(const Matrix<Integer>& Congruences, const Matrix<Integer>& Equations, Matrix<Integer>& Inequalities) {

    size_t nr_cong = Congruences.nr_of_rows();
    // handle Congurences
    if (nr_cong > 0) {
        size_t i,j;

        //add slack variables to convert congruences into equaitions
        Matrix<Integer> Cong_Slack(nr_cong, dim+nr_cong);
        for (i = 0; i < nr_cong; i++) {
            for (j = 0; j < dim; j++) {
                Cong_Slack[i][j]=Congruences[i][j];
            }
            Cong_Slack[i][dim+i]=Congruences[i][dim];
            if(Congruences[i][dim]==0){
                errorOutput() << "Modulus 0 in congruence!" << endl;
                throw BadInputException();
            }
        }

        //compute kernel
        
        Matrix<Integer> Help=Cong_Slack.kernel(); // gives the solutions to the the system with slack variables
        Matrix<Integer> Ker_Basis(dim,dim);   // must now project to first dim coordinates to get rid of them
        for(size_t i=0;i<dim;++i)
            for(size_t j=0;j<dim;++j)
                Ker_Basis[i][j]=Help[i][j];


        //TODO now a new linear transformation is computed, necessary??
        Sublattice_Representation<Integer> Basis_Change(Ker_Basis,false);
        compose_basis_change(Basis_Change);
    }

    prepare_input_type_45(Equations, Inequalities);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::prepare_input_type_45(const Matrix<Integer>& Equations, Matrix<Integer>& Inequalities) {

    // use positive orthant if no inequalities are given
    
    if(inhomogeneous){
        SupportHyperplanes=Matrix<Integer>(1,dim);  // insert truncation as first inequality
        SupportHyperplanes[0]=Dehomogenization;
    }
    else
       SupportHyperplanes=Matrix<Integer>(0,dim); // here we start from the empty matrix

    if (Inequalities.nr_of_rows() == 0) {
        if (verbose) {
            verboseOutput() << "No inequalities specified in constraint mode, using non-negative orthant." << endl;
        }
        if(inhomogeneous){
            vector<Integer> test(dim);
            test[dim-1]=1;
            size_t matsize=dim;
            if(test==Dehomogenization) // in this case "last coordinate >= 0" is already there
                matsize=dim-1;   // we don't check for any other coincidence
            Inequalities= Matrix<Integer>(matsize,dim);
            for(size_t j=0;j<matsize;++j)
                Inequalities[j][j]=1;    
        }  
        else
            Inequalities = Matrix<Integer>(dim);
    }

    SupportHyperplanes.append(Inequalities);  // now the (remaining) inequalities are inserted 
    
    // is_Computed.set(ConeProperty::SupportHyperplanes);

    if(!BC_set) compose_basis_change(Sublattice_Representation<Integer>(dim));

    if (Equations.nr_of_rows()>0) {
        Matrix<Integer> Ker_Basis=BasisChange.to_sublattice_dual(Equations).kernel();
        Sublattice_Representation<Integer> Basis_Change(Ker_Basis,true);
        compose_basis_change(Basis_Change);
    }
}



//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::setGrading (const vector<Integer>& lf) {
    if (lf.size() != dim) {
        errorOutput() << "Grading linear form has wrong dimension " << lf.size()
                      << " (should be " << dim << ")" << endl;
        throw BadInputException();
    }
    if (isComputed(ConeProperty::Generators) && Generators.nr_of_rows() > 0) {
        vector<Integer> degrees = Generators.MxV(lf);
        for (size_t i=0; i<degrees.size(); ++i) {
            if (degrees[i]<1 && (!inhomogeneous || Generators[i][dim-1]==0)) { // in the inhomogeneous case: test only generators of tail cone
                errorOutput() << "Grading gives non-positive value " << degrees[i]
                              << " for generator " << i+1 << "!" << endl;
                throw BadInputException();
            }
        }
        // GradingDenom = degrees[0] / v_scalar_product(BasisChange.to_sublattice_dual(lf),BasisChange.to_sublattice(Generators[0])); //TODO in Sublattice Rep berechnen lassen
        vector<Integer> test_grading=BasisChange.to_sublattice_dual_no_div(lf);
        GradingDenom=v_make_prime(test_grading);
    } else {
        GradingDenom = 1;
    }
    //check if the linear forms are the same
    if (isComputed(ConeProperty::Grading) && Grading == lf) {
        return;
    }
    Grading = lf;
    is_Computed.set(ConeProperty::Grading);

    //remove data that depend on the grading 
    is_Computed.reset(ConeProperty::IsDeg1Generated);
    is_Computed.reset(ConeProperty::IsDeg1ExtremeRays);
    is_Computed.reset(ConeProperty::IsDeg1HilbertBasis);
    is_Computed.reset(ConeProperty::Deg1Elements);
    Deg1Elements = Matrix<Integer>(0,dim);
    is_Computed.reset(ConeProperty::HilbertSeries);
    is_Computed.reset(ConeProperty::Multiplicity);

}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::setDehomogenization (const vector<Integer>& lf) {
    if (lf.size() != dim) {
        errorOutput() << "Dehomogenizing linear form has wrong dimension " << lf.size()
                      << " (should be " << dim << ")" << endl;
        throw BadInputException();
    }
    Dehomogenization=lf;
    is_Computed.set(ConeProperty::Dehomogenization);
}

//---------------------------------------------------------------------------

template<typename Integer>
ConeProperties Cone<Integer>::compute(ConeProperties ToCompute) {
    
    // handle zero cone as special case, makes our life easier
    if (BasisChange.get_rank() == 0) {
        set_zero_cone();
        ToCompute.reset(is_Computed);
        return ToCompute;
    }                                    

    ToCompute.set_preconditions();
    ToCompute.prepare_compute_options();
    ToCompute.check_sanity(inhomogeneous);

    if (ToCompute.test(ConeProperty::DualMode)) {
        compute_dual(ToCompute);
        if (ToCompute.none())
            return ToCompute;
    }

    /* preparation: get generators if necessary */
    compute_generators();
    if (BasisChange.get_rank() == 0) {
        set_zero_cone();
        ToCompute.reset(is_Computed);
        return ToCompute;
    }
    if (!isComputed(ConeProperty::Generators)) {
        errorOutput()<<"FATAL ERROR: Could not get Generators. This should not happen!"<<endl;
        throw FatalException();
    }


    ToCompute.reset(is_Computed); // already computed
    if (ToCompute.none()) {
        return ToCompute;  
    }

    if (rees_primary) // && ToCompute.test(ConeProperty::ReesPrimaryMultiplicity))
        ToCompute.set(ConeProperty::Triangulation);

    /* Create a Full_Cone FC */
    Full_Cone<Integer> FC(BasisChange.to_sublattice(Generators));

    /* activate bools in FC */

    FC.inhomogeneous=inhomogeneous;
    
    if (ToCompute.test(ConeProperty::HilbertSeries)) {
        FC.do_h_vector = true;
    }
    if (ToCompute.test(ConeProperty::HilbertBasis)) {
        FC.do_Hilbert_basis = true;
    }
    if (ToCompute.test(ConeProperty::Triangulation)) {
        FC.keep_triangulation = true;
    }
    if (ToCompute.test(ConeProperty::Multiplicity)) {
        FC.do_multiplicity = true;
    }
    if (ToCompute.test(ConeProperty::TriangulationSize)) {
        FC.do_triangulation = true;
    }
    if (ToCompute.test(ConeProperty::Deg1Elements)) {
        FC.do_deg1_elements = true;
    }
    if (ToCompute.test(ConeProperty::StanleyDec)) {
        FC.do_Stanley_dec = true;
    }
    if (ToCompute.test(ConeProperty::ApproximateRatPolytope)) {
        FC.do_approximation = true;
        is_Computed.set(ConeProperty::ApproximateRatPolytope);
    }
    if (ToCompute.test(ConeProperty::DefaultMode)) {
        FC.do_default_mode = true;
        is_Computed.set(ConeProperty::DefaultMode);
    }

    /* Give extra data to FC */
    if ( isComputed(ConeProperty::ExtremeRays) ) {
        FC.Extreme_Rays = ExtremeRays;
        FC.is_Computed.set(ConeProperty::ExtremeRays);
    }
    if (ExcludedFaces.nr_of_rows()!=0) {
        FC.ExcludedFaces = BasisChange.to_sublattice_dual(ExcludedFaces);
    }
    
    if (inhomogeneous){
        FC.Truncation = BasisChange.to_sublattice_dual_no_div(Dehomogenization);
    }
    if ( isComputed(ConeProperty::Grading) ) {  // IMPORTANT: Truncation must be set before Grading
        FC.Grading = BasisChange.to_sublattice_dual(Grading);
        FC.is_Computed.set(ConeProperty::Grading);
        if (inhomogeneous)
            FC.find_grading_inhom();
        FC.set_degrees();
    }
    
    if (SupportHyperplanes.nr_of_rows()!=0) {
        vector< vector<Integer> > vvSH = BasisChange.to_sublattice_dual(SupportHyperplanes).get_elements();
        FC.Support_Hyperplanes = list< vector<Integer> >(vvSH.begin(), vvSH.end());
    }
    if (isComputed(ConeProperty::SupportHyperplanes)){
        FC.is_Computed.set(ConeProperty::SupportHyperplanes);
        FC.do_all_hyperplanes = false;
    }

    /* do the computation */
    FC.compute();
    
    extract_data(FC);
    
    /* check if everything is computed*/
    ToCompute.reset(is_Computed); //remove what is now computed
    if (ToCompute.any()) {
        errorOutput() << "Warning: Cone could not compute everything that was asked for!"<<endl;
        errorOutput() << "Missing: " << ToCompute << endl;
    }
    return ToCompute;
}


template<typename Integer>
void Cone<Integer>::compute_generators() {
    //create Generators from SupportHyperplanes
    //if (!isComputed(ConeProperty::Generators) && isComputed(ConeProperty::SupportHyperplanes)) {
    if (!isComputed(ConeProperty::Generators) && SupportHyperplanes.nr_of_rows()!=0) {
        if (verbose) {
            verboseOutput() <<endl<< "Computing extreme rays as support hyperplanes of the dual cone:";
        }
        Full_Cone<Integer> Dual_Cone(BasisChange.to_sublattice_dual(SupportHyperplanes));
        Dual_Cone.support_hyperplanes();
        if (Dual_Cone.isComputed(ConeProperty::SupportHyperplanes)) {
            //get the extreme rays of the primal cone
            Matrix<Integer> Extreme_Rays=Dual_Cone.getSupportHyperplanes();
            Generators = BasisChange.from_sublattice(Extreme_Rays);
            is_Computed.set(ConeProperty::Generators);
            ExtremeRays = vector<bool>(Generators.nr_of_rows(),true);
            is_Computed.set(ConeProperty::ExtremeRays);
            if (Dual_Cone.isComputed(ConeProperty::ExtremeRays)) {
                //get minmal set of support_hyperplanes
                Matrix<Integer> Supp_Hyp = Dual_Cone.getGenerators().submatrix(Dual_Cone.getExtremeRays());
                SupportHyperplanes = BasisChange.from_sublattice_dual(Supp_Hyp);
                is_Computed.set(ConeProperty::SupportHyperplanes);
            }
            Sublattice_Representation<Integer> Basis_Change(Extreme_Rays,true);
            compose_basis_change(Basis_Change);

            // check grading and compute denominator
            if (isComputed(ConeProperty::Grading) && Generators.nr_of_rows() > 0) {
                setGrading(Grading);
            }
            // compute grading, so that it is also known if nothing else is done afterwards
            if (!isComputed(ConeProperty::Grading)) {
                // Generators = ExtremeRays
                vector<Integer> lf = BasisChange.to_sublattice(Generators).find_linear_form();
                if (lf.size() == BasisChange.get_rank()) {
                    setGrading(BasisChange.from_sublattice_dual(lf));
                }
            }
        }
    }
}


template<typename Integer>
ConeProperties Cone<Integer>::compute_dual(ConeProperties ToCompute) {

    bool do_only_Deg1_Elements=ToCompute.test(ConeProperty::Deg1Elements) && !ToCompute.test(ConeProperty::HilbertBasis);
        
    //if(isComputed(ConeProperty::Generators) && !isComputed(ConeProperty::SupportHyperplanes)){
    if(isComputed(ConeProperty::Generators) && SupportHyperplanes.nr_of_rows()==0){
        if (verbose) {
            verboseOutput() <<endl<< "Computing support hyperplanes for the dual mode:";
        }
        Full_Cone<Integer> Tmp_Cone(BasisChange.to_sublattice(Generators));
        Tmp_Cone.inhomogeneous=inhomogeneous;  // necessary to prevent computation of grading in the inhomogeneous case
        Tmp_Cone.support_hyperplanes();        // also marks extreme rays
        extract_data(Tmp_Cone);
        if(inhomogeneous){
            Matrix<Integer> Help(SupportHyperplanes.nr_of_rows()+1,dim);  // make Dehomogenization the first inequality
            Help[0]=Dehomogenization;
            Help.append(SupportHyperplanes);
            SupportHyperplanes=Help;
        }
        
    }
    
    if(isComputed(ConeProperty::Generators) && !isComputed(ConeProperty::ExtremeRays)){
        errorOutput() << "Generators computed, but extreme rays not marked in dual cone. THIS SHOULD NOT HAPPEN!" << endl;
        throw FatalException();
    
    }
    
    if((do_only_Deg1_Elements || inhomogeneous) && !isComputed(ConeProperty::ExtremeRays)){
        if (verbose) {
            verboseOutput() <<endl<< "Computing extreme rays for the dual mode:";
        }
        Matrix<Integer> Help(0,dim);
        if(inhomogeneous)                        // we must guard ourselves against loosing the truncation
            Help=SupportHyperplanes;
        compute_generators();   // computes extreme rays, but does not find grading ! 
        if(inhomogeneous)                        // we must guard ourselves against loosing the truncation
            SupportHyperplanes=Help;   
        if (BasisChange.get_rank() == 0) {
            set_zero_cone();
            ToCompute.reset(is_Computed);
            return ToCompute;
        }
    }
    
    if(do_only_Deg1_Elements && !isComputed(ConeProperty::Grading)){
        vector<Integer> lf= Generators.submatrix(ExtremeRays).find_linear_form_low_dim();
        // cout << "lf " << lf;
        if(lf.size()==dim)
            setGrading(lf); 
        else{
            errorOutput() << "Need grading to compute degree 1 elements and cannot find one." << endl;
            throw BadInputException();
        }  
    }
    
    //if (!isComputed(ConeProperty::SupportHyperplanes)) {
    if (SupportHyperplanes.nr_of_rows()==0) {
        errorOutput()<<"FATAL ERROR: Could not get SupportHyperplanes. This should not happen!"<<endl;
        throw FatalException();
    }

    size_t i,j;
    Matrix<Integer> Inequ_on_Ker = BasisChange.to_sublattice_dual(SupportHyperplanes);
    size_t newdim = Inequ_on_Ker.nr_of_columns();
    //now sort the inequalities, hopefully this makes the computation faster
    Integer norm;
    vector< Integer > hyperplane;
    multimap <Integer , vector <Integer> >  SortingHelp;
    typename multimap <Integer , vector <Integer> >::const_iterator ii;
    
    size_t i_start=0;
    if(inhomogeneous){  // in the inhomogeneous case the truncation will be inserted below
        i_start=1;
        // cout << "Trunc " << BasisChange.to_sublattice_dual_no_div(Truncation);
        //cout << "First " << Inequ_on_Ker[0];
        assert(Inequ_on_Ker[0]==BasisChange.to_sublattice_dual_no_div(Dehomogenization));
    }
    for (i = i_start; i < Inequ_on_Ker.nr_of_rows() ; i++) {
        hyperplane=Inequ_on_Ker[i];
        norm=0;
        for (j = 0; j <newdim; j++) {
            norm+=Iabs(hyperplane[j]);
        }
        SortingHelp.insert(pair <Integer , vector <Integer> > (norm,hyperplane));
    }
    size_t inhom_corr=0;
    if(inhomogeneous || do_only_Deg1_Elements)
        inhom_corr=1;
    Matrix<Integer> Inequ_Ordered(Inequ_on_Ker.nr_of_rows()+inhom_corr,newdim);
    if(inhomogeneous)
        Inequ_Ordered[0]=BasisChange.to_sublattice_dual_no_div(Dehomogenization);   // inseert truncation as the first inequality
    if(do_only_Deg1_Elements)
        Inequ_Ordered[0]=BasisChange.to_sublattice_dual(Grading);           // in this case the grading acts as truncation and it is a NEW inrquality       
    i=inhom_corr;
    for (ii=SortingHelp.begin(); ii != SortingHelp.end(); ii++) {
        Inequ_Ordered[i]=(*ii).second;
        i++;
    }
    
    Cone_Dual_Mode<Integer> ConeDM(Inequ_Ordered);
    ConeDM.inhomogeneous=inhomogeneous;
    ConeDM.do_only_Deg1_Elements=do_only_Deg1_Elements;
    if(isComputed(ConeProperty::Generators))
        ConeDM.Generators=BasisChange.to_sublattice(Generators);
    if(isComputed(ConeProperty::ExtremeRays))
        ConeDM.ExtremeRays=ExtremeRays;
    ConeDM.hilbert_basis_dual();
    
    //create a Full_Cone out of ConeDM
    //if ( ConeDM.Generators.rank() < ConeDM.dim ) {
        Sublattice_Representation<Integer> SR(ConeDM.Generators,true);
        ConeDM.to_sublattice(SR);
        compose_basis_change(SR);
        // handle zero cone as special case, makes our life easier
        if (BasisChange.get_rank() == 0) {
            set_zero_cone();
            ToCompute.reset(is_Computed);
            return ToCompute;
        }
    //}
    Full_Cone<Integer> FC(ConeDM);
    // Give extra data to FC
    if ( isComputed(ConeProperty::Grading) ) {
        FC.Grading = BasisChange.to_sublattice_dual(Grading);
        FC.is_Computed.set(ConeProperty::Grading);
        
        if(!inhomogeneous)
            FC.set_degrees();
    }
    if(inhomogeneous)
        FC.Truncation=BasisChange.to_sublattice_dual(Dehomogenization);
    FC.dual_mode();
    extract_data(FC);
    
    is_Computed.set(ConeProperty::DualMode);
    return ConeProperties();
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::extract_data(Full_Cone<Integer>& FC) {
    //this function extracts ALL available data from the Full_Cone
    //even if it was in Cone already <- this may change
    //it is possible to delete the data in Full_Cone after extracting it

    if(verbose) {
        verboseOutput() << "transforming data..."<<flush;
        // cout << "inhom "<<inhomogeneous<<endl;
    }
    
    if (rees_primary && FC.isComputed(ConeProperty::Triangulation)) {
        //here are some computations involved, made first so that data can be deleted in FC later
        ReesPrimaryMultiplicity = FC.primary_multiplicity();
        is_Computed.set(ConeProperty::ReesPrimaryMultiplicity);
    }
    
    if (FC.isComputed(ConeProperty::Generators)) {
        Generators = BasisChange.from_sublattice(FC.getGenerators());
        is_Computed.set(ConeProperty::Generators);
    }
    if (FC.isComputed(ConeProperty::ExtremeRays)) {
        ExtremeRays = FC.getExtremeRays();
        assert(ExtremeRays.size() == Generators.nr_of_rows());
        if (inhomogeneous) {
            // separate extreme rays to rays of the level 0 cone
            // and the verticies of the polyhedron, which are in level >=1
            size_t nr_gen = Generators.nr_of_rows();
            VerticesOfPolyhedron = vector<bool>(nr_gen);
            for (size_t i=0; i<nr_gen; i++) {
                if (ExtremeRays[i] && v_scalar_product(Generators[i],Dehomogenization) != 0) {
                    VerticesOfPolyhedron[i] = true;
                }
            }
            is_Computed.set(ConeProperty::VerticesOfPolyhedron);
        }
        is_Computed.set(ConeProperty::ExtremeRays);
    }
    if (FC.isComputed(ConeProperty::SupportHyperplanes)) {
        if (inhomogeneous) {
            // remove irrelevant support hyperplane 0 ... 0 1
            vector<Integer> irr_hyp_subl = BasisChange.to_sublattice_dual(Dehomogenization);
            FC.Support_Hyperplanes.remove(irr_hyp_subl);
        }
        SupportHyperplanes = BasisChange.from_sublattice_dual(FC.getSupportHyperplanes());
        is_Computed.set(ConeProperty::SupportHyperplanes);
    }
    if (FC.isComputed(ConeProperty::TriangulationSize)) {
        TriangulationSize = FC.totalNrSimplices;
        is_Computed.set(ConeProperty::TriangulationSize);
    }
    if (FC.isComputed(ConeProperty::TriangulationDetSum)) {
        TriangulationDetSum = FC.detSum;
        is_Computed.set(ConeProperty::TriangulationDetSum);
    }
    if (FC.isComputed(ConeProperty::Triangulation)) {
        size_t tri_size = FC.Triangulation.size();
        Triangulation = vector< pair<vector<key_t>, Integer> >(tri_size);
        SHORTSIMPLEX<Integer> simp;
        for (size_t i = 0; i<tri_size; ++i) {
            simp = FC.Triangulation.front();
            Triangulation[i].first.swap(simp.key);
            sort(Triangulation[i].first.begin(), Triangulation[i].first.end());
            if (FC.isComputed(ConeProperty::TriangulationDetSum))
                Triangulation[i].second = simp.vol;
            else
                Triangulation[i].second = 0;
            FC.Triangulation.pop_front();
        }
        is_Computed.set(ConeProperty::Triangulation);
    }
    if (FC.isComputed(ConeProperty::StanleyDec)) {
        StanleyDec.clear();
        StanleyDec.splice(StanleyDec.end(),FC.StanleyDec);
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
                    key.push_back(i+1);
                }
            }
            InExData.push_back(make_pair(key,F->second));
        }
        is_Computed.set(ConeProperty::InclusionExclusionData);
    }
    if (FC.isComputed(ConeProperty::Shift)) {
        shift = FC.getShift();
        is_Computed.set(ConeProperty::Shift);
    }
    if (FC.isComputed(ConeProperty::RecessionRank)) {
        recession_rank = FC.level0_dim;
        is_Computed.set(ConeProperty::RecessionRank);
        if (getRank() == recession_rank) {
            affine_dim = -1;
        } else {
            affine_dim = getRank()-1;
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
        } else if (isComputed(ConeProperty::ModuleRank)) {
            multiplicity = FC.getMultiplicity()*module_rank;
            is_Computed.set(ConeProperty::Multiplicity);
        }
    }
    if (FC.isComputed(ConeProperty::HilbertBasis)) {
        if (inhomogeneous) {
            // separate (capped) Hilbert basis to the Hilbert basis of the level 0 cone
            // and the module generators in level 1
            HilbertBasis = Matrix<Integer>(0,dim);
            ModuleGenerators = Matrix<Integer>(0,dim);
            typename list< vector<Integer> >::const_iterator FCHB(FC.Hilbert_Basis.begin());
            vector<Integer> tmp;
            for (; FCHB != FC.Hilbert_Basis.end(); ++FCHB) {
                tmp = BasisChange.from_sublattice(*FCHB);
                if (v_scalar_product(tmp,Dehomogenization) == 0) { // Hilbert basis element of the cone at level 0
                    HilbertBasis.append(tmp);
                } else {              // module generator
                    ModuleGenerators.append(tmp);
                }
            }
            is_Computed.set(ConeProperty::ModuleGenerators);
        } else { // homogeneous
            HilbertBasis = BasisChange.from_sublattice(FC.getHilbertBasis());
        }
        is_Computed.set(ConeProperty::HilbertBasis);
    }
    if (FC.isComputed(ConeProperty::Deg1Elements)) {
        Deg1Elements = BasisChange.from_sublattice(FC.getDeg1Elements());
        is_Computed.set(ConeProperty::Deg1Elements);
    }
    if (FC.isComputed(ConeProperty::HilbertSeries)) {
        HSeries = FC.Hilbert_Series;
        is_Computed.set(ConeProperty::HilbertSeries);
    }
    if (FC.isComputed(ConeProperty::IsPointed)) {
        pointed = FC.isPointed();
        is_Computed.set(ConeProperty::IsPointed);
    }
    if (FC.isComputed(ConeProperty::IsDeg1ExtremeRays)) {
        deg1_extreme_rays = FC.isDeg1ExtremeRays();
        is_Computed.set(ConeProperty::IsDeg1ExtremeRays);
    }
    if (FC.isComputed(ConeProperty::ExcludedFaces)) {
        ExcludedFaces = BasisChange.from_sublattice_dual(FC.getExcludedFaces());
        is_Computed.set(ConeProperty::ExcludedFaces);
    }

    if (FC.isComputed(ConeProperty::Grading)) {
        if (!isComputed(ConeProperty::Grading)) {
            Grading = BasisChange.from_sublattice_dual(FC.getGrading());
            is_Computed.set(ConeProperty::Grading);
        }
        //compute denominator of Grading
        /* if (Generators.size() > 0) {
            GradingDenom  = v_scalar_product(Grading,Generators[0]);
            GradingDenom /= v_scalar_product(FC.getGrading(),FC.Generators[0]);
        } */
        if(BasisChange.get_rank()!=0){
            vector<Integer> test_grading=BasisChange.to_sublattice_dual_no_div(Grading);
            GradingDenom=v_make_prime(test_grading);
        }
    }
    
    if (FC.isComputed(ConeProperty::IsDeg1HilbertBasis)) {
        deg1_hilbert_basis = FC.isDeg1HilbertBasis();
        is_Computed.set(ConeProperty::IsDeg1HilbertBasis);
    }
    if (FC.isComputed(ConeProperty::IsIntegrallyClosed)) {
        integrally_closed = FC.isIntegrallyClosed();
        is_Computed.set(ConeProperty::IsIntegrallyClosed);
    }

    if (verbose) {
        verboseOutput() << " done." <<endl;
    }
}



//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::set_zero_cone() {
    // GeneratorsOfToricRing needs no handling

    Generators = Matrix<Integer>(0,dim);
    is_Computed.set(ConeProperty::Generators);

    ExtremeRays = vector<bool>(Generators.nr_of_rows(), false);
    is_Computed.set(ConeProperty::ExtremeRays);

    SupportHyperplanes = Matrix<Integer>(0,dim);
    is_Computed.set(ConeProperty::SupportHyperplanes);

    TriangulationSize = 0;
    is_Computed.set(ConeProperty::TriangulationSize);

    TriangulationDetSum = 0;
    is_Computed.set(ConeProperty::TriangulationDetSum);

    Triangulation.clear();
    is_Computed.set(ConeProperty::Triangulation);

    StanleyDec.clear();
    is_Computed.set(ConeProperty::StanleyDec);

    multiplicity = 1;
    is_Computed.set(ConeProperty::Multiplicity);

    HilbertBasis = Matrix<Integer>(0,dim);
    is_Computed.set(ConeProperty::HilbertBasis);

    Deg1Elements = Matrix<Integer>(0,dim);
    is_Computed.set(ConeProperty::Deg1Elements);

    HSeries = HilbertSeries(vector<num_t>(1,1),vector<denom_t>()); // 1/1
    is_Computed.set(ConeProperty::HilbertSeries);

    if (!is_Computed.test(ConeProperty::Grading)) {
        Grading = vector<Integer>(dim);
        GradingDenom = 1;
        is_Computed.set(ConeProperty::Grading);
    }

    pointed = true;

    deg1_extreme_rays = true;
    is_Computed.set(ConeProperty::IsDeg1ExtremeRays);

    deg1_hilbert_basis = true;
    is_Computed.set(ConeProperty::IsDeg1HilbertBasis);

    integrally_closed = true;
    is_Computed.set(ConeProperty::IsIntegrallyClosed);


    if (ExcludedFaces.nr_of_rows() != 0) {
        is_Computed.set(ConeProperty::ExcludedFaces);
        InExData.clear();
        InExData.push_back(make_pair(vector<key_t>(),-1));
        is_Computed.set(ConeProperty::InclusionExclusionData);
    }

    if (inhomogeneous) {  // empty set of solutions
        VerticesOfPolyhedron = vector<bool>(Generators.nr_of_rows(), false);
        is_Computed.set(ConeProperty::VerticesOfPolyhedron);

        shift = 0;
        is_Computed.set(ConeProperty::Shift);

        module_rank = 0;
        is_Computed.set(ConeProperty::ModuleRank);

        ModuleGenerators = Matrix<Integer>(0,dim);
        is_Computed.set(ConeProperty::ModuleGenerators);
    }

    if (inhomogeneous || ExcludedFaces.nr_of_rows() != 0) {
        multiplicity = 0;
        is_Computed.set(ConeProperty::Multiplicity);

        HSeries.reset(); // 0/1
        is_Computed.set(ConeProperty::HilbertSeries);

    }
}

} // end namespace libnormaliz
