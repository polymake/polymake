/*
 * Normaliz
 * Copyright (C) 2007-2013  Winfried Bruns, Bogdan Ichim, Christof Soeger
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
 */

#include <list>
#include "cone.h"
#include "full_cone.h"

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
Cone<Integer>::Cone(const vector< vector<Integer> >& Input, InputType input_type) {
    initialize();
    if (!Input.empty()) {
        dim = Input.front().size();
        if (input_type == Type::rees_algebra || input_type == Type::polytope) {
            dim++; // we add one component
        }
        if (input_type == Type::congruences) {
            dim--; //congruences have one extra column
        }
    }
    single_matrix_input(Input, input_type);
}

template<typename Integer>
Cone<Integer>::Cone(const map< InputType, vector< vector<Integer> > >& multi_input_data) {
    initialize();
    typename map< InputType , vector< vector<Integer> > >::const_iterator it;
    //determine dimension
    it = multi_input_data.begin();
    for(; it != multi_input_data.end(); ++it) {
        if (!it->second.empty()) {
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
    //check for a grading
    it = multi_input_data.find(Type::grading);
    if (it != multi_input_data.end()) {
        vector< vector<Integer> > lf = it->second;
        if (lf.size() != 1) {
            errorOutput() << "ERROR: Bad grading, has "
                          << lf.size() << " rows (should be 1)!" << endl;
            throw BadInputException();
        }
        setGrading (lf[0]);
    }

    it = multi_input_data.begin();
    if (multi_input_data.size() == 1) { // use the "one-matrix-input" method
        single_matrix_input(it->second,it->first);
    } else if (multi_input_data.size() == 2 && isComputed(ConeProperty::Grading)) {
        if (it->first == Type::grading) it++;
        single_matrix_input(it->second,it->first);
    } else {               // now we have to have constraints!
        Matrix<Integer> Inequalities(0,dim), Equations(0,dim), Congruences(0,dim+1), Signs(0,dim);
        for (; it != multi_input_data.end(); ++it) {
            if (it->second.size() == 0) {
                continue;
            }
            switch (it->first) {
                case Type::hyperplanes:
                    if (it->second.begin()->size() != dim) {
                        errorOutput() << "Dimensions of hyperplanes ("<<it->second.begin()->size()<<") do not match dimension of other constraints ("<<dim<<")!"<<endl;
                        throw BadInputException();
                    }
                    Inequalities = it->second;
                    break;
                case Type::equations:
                    if (it->second.begin()->size() != dim) {
                        errorOutput() << "Dimensions of equations ("<<it->second.begin()->size()<<") do not match dimension of other constraints ("<<dim<<")!"<<endl;
                        throw BadInputException();
                    }
                    Equations = it->second;
                    break;
                case Type::congruences:
                    if (it->second.begin()->size() != dim+1) {
                        errorOutput() << "Dimensions of congruences ("<<it->second.begin()->size()<<") do not match dimension of other constraints ("<<dim<<")!"<<endl;
                        throw BadInputException();
                    }
                    Congruences = it->second;
                    break;
                case Type::signs:
                    if (it->second.begin()->size() != dim) {
                        errorOutput() << "Dimensions of hyperplanes ("<<it->second.begin()->size()<<") do not match dimension of other constraints ("<<dim<<")!"<<endl;
                        throw BadInputException();
                    }
                    Signs = sign_inequalities(it->second);
                    break;
                case Type::grading:
                    break; //skip the grading
                default:
                    errorOutput() << "This InputType combination is currently not supported!" << endl;
                    throw BadInputException();
            }
        }
        if(!BC_set) compose_basis_change(Sublattice_Representation<Integer>(dim));
        Inequalities.append(Signs);
        prepare_input_type_456(Congruences, Equations, Inequalities);
    }
}
    
/* only used by the constructors */
template<typename Integer>
void Cone<Integer>::initialize() {
    BC_set=false;
    is_Computed = bitset<ConeProperty::EnumSize>();  //initialized to false
    dim = 0;
    rees_primary = false;
}

template<typename Integer>
void Cone<Integer>::single_matrix_input(const vector< vector<Integer> >& Input, InputType input_type) {

    switch (input_type) {
        case Type::integral_closure: prepare_input_type_0(Input); break;
        case Type::normalization:    prepare_input_type_1(Input); break;
        case Type::polytope:         prepare_input_type_2(Input); break;
        case Type::rees_algebra:     prepare_input_type_3(Input); break;
        case Type::signs:
          prepare_input_type_45(vector<vector<Integer> >(), sign_inequalities(Input));
          break;
        case Type::hyperplanes:
          prepare_input_type_45(vector<vector<Integer> >(), Input);
          break;
        case Type::equations:
          prepare_input_type_45(Input, vector<vector<Integer> >());
          break;
        case Type::congruences:
          prepare_input_type_456(Input, vector<vector<Integer> >(), vector<vector<Integer> >());
          break;
        case Type::lattice_ideal:    prepare_input_type_10(Input); break;
        case Type::grading:
          errorOutput() << "Grading as only input not supported!" << endl;
          // no break, go to default
        default:
          throw BadInputException();
    }
    if(!BC_set) compose_basis_change(Sublattice_Representation<Integer>(dim));
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
Sublattice_Representation<Integer> Cone<Integer>::getBasisChange() const{
    return BasisChange;
}

template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getGeneratorsOfToricRing() const {
    return GeneratorsOfToricRing;
}

template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getGenerators() const {
    return Generators;
}

template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getExtremeRays() const {
    return Matrix<Integer>(Generators).submatrix(ExtremeRays).get_elements();
}

template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getSupportHyperplanes() const {
   return SupportHyperplanes;
}

//TODO gehts nicht auch in der SR?
template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getEquations() const {
    size_t rank = BasisChange.get_rank();
    size_t nr_of_equ = dim-rank;
    if (rank == 0)                   // the zero cone
        return Matrix<Integer>(dim).get_elements(); // identity matrix
    Matrix<Integer> Equations(nr_of_equ,dim);
    if (nr_of_equ > 0) {
        Lineare_Transformation<Integer> NewLT = Transformation(Matrix<Integer>(getExtremeRays()));
        Matrix<Integer> Help = NewLT.get_right().transpose();
        for (size_t i = rank; i < dim; i++) {
            Equations.write(i-rank,Help.read(i));
        }
    }
    return Equations.get_elements();
}

template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getCongruences() const {
    return BasisChange.get_congruences().get_elements();
}

template<typename Integer>
map< InputType , vector< vector<Integer> > > Cone<Integer>::getConstraints () const {
    map<InputType, vector< vector<Integer> > > c;
    c.insert(pair< InputType,vector< vector<Integer> > >(Type::hyperplanes,SupportHyperplanes));
    c.insert(pair< InputType,vector< vector<Integer> > >(Type::equations,getEquations()));
    c.insert(pair< InputType,vector< vector<Integer> > >(Type::congruences,getCongruences()));
    return c;
}


template<typename Integer>
const vector< pair<vector<key_t>,Integer> >& Cone<Integer>::getTriangulation() const {
    return Triangulation;
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
vector< vector<Integer> > Cone<Integer>::getHilbertBasis() const {
    return HilbertBasis;
}

template<typename Integer>
vector< vector<Integer> > Cone<Integer>::getDeg1Elements() const {
    return Deg1Elements;
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
mpq_class Cone<Integer>::getMultiplicity() const {
    return multiplicity;
}

template<typename Integer>
bool Cone<Integer>::isPointed() const {
    return pointed;
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

    Sublattice_Representation<Integer> Basis_Change(Matrix<Integer>(Input),true);
    compose_basis_change(Basis_Change);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::prepare_input_type_1(const vector< vector<Integer> >& Input) {
    Generators = Input;
    is_Computed.set(ConeProperty::Generators);

    Sublattice_Representation<Integer> Basis_Change(Matrix<Integer>(Input),false);
    compose_basis_change(Basis_Change);
}

//---------------------------------------------------------------------------

/* polytope input */
template<typename Integer>
void Cone<Integer>::prepare_input_type_2(const vector< vector<Integer> >& Input) {
    size_t j;
    size_t nr = Input.size();
    if (nr == 0) {
        Generators = Input;
    } else { //append a column of 1
        Generators = vector< vector<Integer> >(nr, vector<Integer>(dim));
        for (size_t i=0; i<nr; i++) {
            for (j=0; j<dim-1; j++) 
                Generators[i][j] = Input[i][j];
            Generators[i][dim-1]=1;
        }
    }
    is_Computed.set(ConeProperty::Generators);

    compose_basis_change(Sublattice_Representation<Integer>(Matrix<Integer>(Generators),true));

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
    Generators = Full_Cone_Generators.get_elements();
    is_Computed.set(ConeProperty::Generators);

    compose_basis_change(Sublattice_Representation<Integer>(Full_Cone_Generators.nr_of_columns()));

}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::prepare_input_type_456(const Matrix<Integer>& Congruences, const Matrix<Integer>& Equations, const Matrix<Integer>& Inequalities) {

    size_t nr_cong = Congruences.nr_of_rows();
    // handle Congurences
    if (nr_cong > 0) {
        size_t i,j;

        //add slack variables
        Matrix<Integer> Cong_Slack(nr_cong, dim+nr_cong);
        for (i = 0; i < nr_cong; i++) {
            for (j = 0; j < dim; j++) {
                Cong_Slack.write(i,j,Congruences.read(i,j));
            }
            Cong_Slack.write(i,dim+i,Congruences.read(i,dim));
        }

        //compute kernel
        Lineare_Transformation<Integer> Diagonalization = Transformation(Cong_Slack);
        size_t rank = Diagonalization.get_rank();
        Matrix<Integer> H = Diagonalization.get_right();
        Matrix<Integer> Ker_Basis_Transpose(dim, dim+nr_cong-rank);
        for (i = 0; i < dim; i++) {
            for (j = rank; j < dim+nr_cong; j++) {
                Ker_Basis_Transpose.write(i, j-rank, H.read(i,j));
            }
        }

        //TODO now a new linear transformation is computed, necessary??
        Sublattice_Representation<Integer> Basis_Change(Ker_Basis_Transpose.transpose(),false);
        compose_basis_change(Basis_Change);
    }

    prepare_input_type_45(Equations, Inequalities);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::prepare_input_type_45(const Matrix<Integer>& Equations, const Matrix<Integer>& Inequalities) {

    // use positive orthant if no inequalities are given
    if (Inequalities.nr_of_rows() == 0) {
        if (verbose) {
            verboseOutput() << "No inequalities specified in constraint mode, using non-negative space." << endl;
        }
        SupportHyperplanes = (Matrix<Integer>(dim)).get_elements();
    } else {
        SupportHyperplanes = Inequalities.get_elements();
    }
    is_Computed.set(ConeProperty::SupportHyperplanes);

    if(!BC_set) compose_basis_change(Sublattice_Representation<Integer>(dim));

    size_t i,j;
    if (Equations.nr_of_rows()>0) {
        Lineare_Transformation<Integer> Diagonalization = Transformation(BasisChange.to_sublattice_dual(Equations));
        size_t rank=Diagonalization.get_rank();

        Matrix<Integer> Help=Diagonalization.get_right();
        Matrix<Integer> Ker_Basis_Transpose(dim,dim-rank);
        for (i = 0; i < dim; i++) {
            for (j = rank; j < dim; j++) {
                Ker_Basis_Transpose.write(i,j-rank,Help.read(i,j));
            }
        }
        Sublattice_Representation<Integer> Basis_Change(Ker_Basis_Transpose.transpose(),true);
        compose_basis_change(Basis_Change);
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::prepare_input_type_10(const vector< vector<Integer> >& BinomialsV) {
    Matrix<Integer> Binomials(BinomialsV);
    size_t i,j, nr_of_monoid_generators = dim;
    if (isComputed(ConeProperty::Grading)) {
        //check if binomials are homogeneous
        vector<Integer> degrees = Binomials.MxV(Grading);
        for (size_t i=0; i<degrees.size(); ++i) {
            if (degrees[i]!=0) {
                errorOutput() << "Grading gives non-zero value " << degrees[i]
                              << " for binomial " << i+1 << "." << endl;
                throw BadInputException();
            }
            if (Grading[i] <= 0) {
                errorOutput() << "Grading gives non-positive value " << Grading[i]
                            << " for generator " << i+1 << "." << endl;
                throw BadInputException();
            }
        }
    }
    Lineare_Transformation<Integer> Diagonalization=Transformation(Binomials);
    size_t rank=Diagonalization.get_rank();
    Matrix<Integer> Help=Diagonalization.get_right();
    Matrix<Integer> Generators(nr_of_monoid_generators,nr_of_monoid_generators-rank);
    for (i = 0; i < nr_of_monoid_generators; i++) {
        for (j = rank; j < nr_of_monoid_generators; j++) {
            Generators.write(i,j-rank,Help.read(i,j));
        }
    }
    Full_Cone<Integer> FC(Generators);
    //TODO verboseOutput(), what is happening here?

    FC.support_hyperplanes();
    Matrix<Integer> Supp_Hyp=FC.getSupportHyperplanes();
    Matrix<Integer> Selected_Supp_Hyp_Trans=(Supp_Hyp.submatrix(Supp_Hyp.max_rank_submatrix_lex())).transpose();
    Matrix<Integer> Positive_Embedded_Generators=Generators.multiplication(Selected_Supp_Hyp_Trans);
    GeneratorsOfToricRing = Positive_Embedded_Generators.get_elements();
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
    prepare_input_type_1(GeneratorsOfToricRing);
}

//---------------------------------------------------------------------------

template<typename Integer>
void Cone<Integer>::setGrading (const vector<Integer>& lf) {
    if (lf.size() != dim) {
        errorOutput() << "Grading linear form has wrong dimension " << lf.size()
                      << " (should be " << dim << ")" << endl;
        throw BadInputException();
    }
    //check if the linear forms are the same
    if (isComputed(ConeProperty::Grading) && Grading == lf) {
        return;
    }
    if (isComputed(ConeProperty::Generators)) {
        vector<Integer> degrees = Matrix<Integer>(Generators).MxV(lf);
        for (size_t i=0; i<degrees.size(); ++i) {
            if (degrees[i]<1) {
                errorOutput() << "Grading gives non-positive value " << degrees[i]
                              << " for generator " << i+1 << "." << endl;
                throw BadInputException();
            }
        }
    }
    Grading = lf;
    GradingDenom = 1;
    is_Computed.set(ConeProperty::Grading);

    //remove data that depends on the grading 
    Deg1Elements.clear();
    HilbertQuasiPolynomial.clear();
    is_Computed.reset(ConeProperty::IsDeg1Generated);
    is_Computed.reset(ConeProperty::IsDeg1ExtremeRays);
    is_Computed.reset(ConeProperty::IsDeg1HilbertBasis);
    is_Computed.reset(ConeProperty::Deg1Elements);
    is_Computed.reset(ConeProperty::HilbertSeries);
    is_Computed.reset(ConeProperty::Multiplicity);
}

//---------------------------------------------------------------------------

template<typename Integer>
ConeProperties Cone<Integer>::compute(ConeProperties ToCompute) {
    if (ToCompute.test(ConeProperty::DualMode))
        compute_dual();

    /* preparation: get generators if necessary */
    compute_generators();
    if (!isComputed(ConeProperty::Generators)) {
        errorOutput()<<"FATAL ERROR: Could not get Generators. This should not happen!"<<endl;
        throw FatalException();
    }

    ToCompute.reset(is_Computed); // already computed
    if (ToCompute.none()) {
        return ToCompute;
    }

    //TODO workaround for zero cones :(
    //die dimension bleibt in der liste nicht gespeichert, wenn sie leer ist, darum passt es dann beim transformieren nicht
    if(Generators.size()==0) {
        return ToCompute;
    }

    /* add preconditions */
    if (ToCompute.test(ConeProperty::IsIntegrallyClosed))
        ToCompute.set(ConeProperty::HilbertBasis);
    if (ToCompute.test(ConeProperty::IsDeg1HilbertBasis))
        ToCompute.set(ConeProperty::HilbertBasis);
    if (ToCompute.test(ConeProperty::IsDeg1ExtremeRays))
        ToCompute.set(ConeProperty::ExtremeRays);
    if (ToCompute.test(ConeProperty::Grading))
        ToCompute.set(ConeProperty::ExtremeRays);
    if (ToCompute.test(ConeProperty::IsPointed))
        ToCompute.set(ConeProperty::ExtremeRays);
    if (ToCompute.test(ConeProperty::Generators))
        ToCompute.set(ConeProperty::ExtremeRays);
    if (ToCompute.test(ConeProperty::ExtremeRays))
        ToCompute.set(ConeProperty::SupportHyperplanes);
    if (ToCompute.test(ConeProperty::TriangulationDetSum))
        ToCompute.set(ConeProperty::Multiplicity);
    
    if (rees_primary) // && ToCompute.test(ConeProperty::ReesPrimaryMultiplicity))
        ToCompute.set(ConeProperty::Triangulation);

    /* Create a Full_Cone FC */
    Full_Cone<Integer> FC(BasisChange.to_sublattice(Matrix<Integer>(Generators)));

    /* activate bools in FC */
    bool only_support_hyperplanes = true;
    if (ToCompute.test(ConeProperty::HilbertSeries)) {
        FC.do_h_vector = true;
        only_support_hyperplanes = false;
    }
    if (ToCompute.test(ConeProperty::HilbertBasis)) {
        FC.do_Hilbert_basis = true;
        only_support_hyperplanes = false;
    }
    if (ToCompute.test(ConeProperty::Triangulation)) {
        FC.keep_triangulation = true;
        only_support_hyperplanes = false;
    }
    if (ToCompute.test(ConeProperty::Multiplicity)) {
        FC.do_multiplicity = true;
        only_support_hyperplanes = false;
    }
    if (ToCompute.test(ConeProperty::TriangulationSize)) {
        FC.do_triangulation = true;
        only_support_hyperplanes = false;
    }
    if (ToCompute.test(ConeProperty::Deg1Elements)) {
        FC.do_deg1_elements = true;
        only_support_hyperplanes = false;
    }
    if (ToCompute.test(ConeProperty::StanleyDec)) {
        FC.do_Stanley_dec = true;
        only_support_hyperplanes = false;
    }

    /* Give extra data to FC */
    if ( isComputed(ConeProperty::ExtremeRays) ) {
        FC.Extreme_Rays = ExtremeRays;
        FC.is_Computed.set(ConeProperty::ExtremeRays);
    }
    if ( isComputed(ConeProperty::Grading) ) {
        FC.Grading = BasisChange.to_sublattice_dual(Grading);
        FC.is_Computed.set(ConeProperty::Grading);
        FC.set_degrees();
    }
    /* Do the computation! */
    if (only_support_hyperplanes) {
        // workaround for not dualizing twice
        if (isComputed(ConeProperty::SupportHyperplanes)) {
            vector< vector<Integer> > vvSH = BasisChange.to_sublattice_dual(Matrix<Integer>(SupportHyperplanes)).get_elements();
            FC.Support_Hyperplanes = list< vector<Integer> >(vvSH.begin(), vvSH.end());
            FC.is_Computed.set(ConeProperty::SupportHyperplanes);
        }
        FC.support_hyperplanes();
    } else {
        FC.compute();
    }
    
    extract_data(FC);
    
    /* check if everything is computed*/
    ToCompute.reset(is_Computed); //remove what is now computed
    if (ToCompute.any()) {
        errorOutput() << "Warning: Cone could not compute everything, that it was asked for!"<<endl;
        errorOutput() << "Missing: " << ToCompute << endl;
    }
    return ToCompute;
}


template<typename Integer>
void Cone<Integer>::compute_generators() {
    //create Generators from SupportHyperplanes
    if (!isComputed(ConeProperty::Generators) && isComputed(ConeProperty::SupportHyperplanes)) {
        if (verbose) {
            verboseOutput() <<endl<< "Computing extreme rays as support hyperplanes of the dual cone:";
        }
        Full_Cone<Integer> Dual_Cone(BasisChange.to_sublattice_dual(Matrix<Integer>(SupportHyperplanes)));
        Dual_Cone.support_hyperplanes();
        if (Dual_Cone.isComputed(ConeProperty::SupportHyperplanes)) {
            //get the extreme rays of the primal cone
            Matrix<Integer> Extreme_Rays=Dual_Cone.getSupportHyperplanes();
            Generators = BasisChange.from_sublattice(Extreme_Rays).get_elements();
            is_Computed.set(ConeProperty::Generators);
            ExtremeRays = vector<bool>(Generators.size(),true);
            is_Computed.set(ConeProperty::ExtremeRays);
            if (Dual_Cone.isComputed(ConeProperty::ExtremeRays)) {
                //get minmal set of support_hyperplanes
                Matrix<Integer> Supp_Hyp = Dual_Cone.getGenerators().submatrix(Dual_Cone.getExtremeRays());
                SupportHyperplanes = BasisChange.from_sublattice_dual(Supp_Hyp).get_elements();
            }
            Sublattice_Representation<Integer> Basis_Change(Extreme_Rays,true);
            compose_basis_change(Basis_Change);

            //compute denominator of Grading
            if (isComputed(ConeProperty::Grading) && Generators.size() > 0) {
                GradingDenom  = v_scalar_product(Grading,Generators[0]);
                GradingDenom /= v_scalar_product(BasisChange.to_sublattice_dual(Grading),BasisChange.to_sublattice(Generators[0])); //TODO in Sublattice Rep berechnen lassen
            }
        }
    }
}



template<typename Integer>
ConeProperties Cone<Integer>::compute(ComputationMode mode) {
    if (mode == Mode::dual) {
        return compute_dual();
    } else {
        ConeProperties cps;
        cps.set(mode);
        return compute(cps);
    }
}

template<typename Integer>
ConeProperties Cone<Integer>::compute_dual() {
    if(isComputed(ConeProperty::Generators) && !isComputed(ConeProperty::SupportHyperplanes)){
        if (verbose) {
            verboseOutput() <<endl<< "Computing support hyperplanes for the dual mode:";
        }
        Full_Cone<Integer> Tmp_Cone(BasisChange.to_sublattice(Matrix<Integer>(Generators)));
        Tmp_Cone.support_hyperplanes();
        extract_data(Tmp_Cone);
    }

    if (!isComputed(ConeProperty::SupportHyperplanes)) {
        errorOutput()<<"FATAL ERROR: Could not get SupportHyperplanes. This should not happen!"<<endl;
        throw FatalException();
    }

    size_t i,j;
    Matrix<Integer> Inequ_on_Ker = BasisChange.to_sublattice_dual(Matrix<Integer>(SupportHyperplanes));
    size_t newdim = Inequ_on_Ker.nr_of_columns();
    //now sort the inequalities, hopefully this makes the computation faster
    Integer norm;
    vector< Integer > hyperplane;
    multimap <Integer , vector <Integer> >  SortingHelp;
    typename multimap <Integer , vector <Integer> >::const_iterator ii;
    for (i = 0; i < Inequ_on_Ker.nr_of_rows() ; i++) {
        hyperplane=Inequ_on_Ker.read(i);
        norm=0;
        for (j = 0; j <newdim; j++) {
            norm+=Iabs(hyperplane[j]);
        }
        SortingHelp.insert(pair <Integer , vector <Integer> > (norm,hyperplane));
    }
    Matrix<Integer> Inequ_Ordered(Inequ_on_Ker.nr_of_rows(),newdim);
    i=0;
    for (ii=SortingHelp.begin(); ii != SortingHelp.end(); ii++) {
        Inequ_Ordered.write(i,(*ii).second);
        i++;
    }
    Cone_Dual_Mode<Integer> ConeDM(Inequ_Ordered);
    ConeDM.hilbert_basis_dual();
    //create a Full_Cone out of ConeDM
    if ( ConeDM.Generators.rank() < ConeDM.dim ) {
        Sublattice_Representation<Integer> SR(ConeDM.Generators,true);
        ConeDM.to_sublattice(SR);
        compose_basis_change(SR);
    }
    Full_Cone<Integer> FC(ConeDM);
    // Give extra data to FC
    if ( isComputed(ConeProperty::Grading) ) {
        FC.Grading = BasisChange.to_sublattice_dual(Grading);
        FC.is_Computed.set(ConeProperty::Grading);
        FC.set_degrees();
    }
    FC.dual_mode();
    extract_data(FC);
    is_Computed.set(ConeProperty::DualMode);
    return ConeProperties();
}

template<typename Integer>
void Cone<Integer>::extract_data(Full_Cone<Integer>& FC) {
    //this function extracts ALL available data from the Full_Cone
    //even if it was in Cone already <- this may change
    //it is possible to delete the data in Full_Cone after extracting it

    if(verbose) {
        verboseOutput() << "transforming data..."<<flush;
    }
    
    if (rees_primary && FC.isComputed(ConeProperty::Triangulation)) {
        //here are some computations involved, made first so that data can be deleted in FC later
        ReesPrimaryMultiplicity = FC.primary_multiplicity();
        is_Computed.set(ConeProperty::ReesPrimaryMultiplicity);
    }
    
    if (FC.isComputed(ConeProperty::Generators)) {
        Generators = BasisChange.from_sublattice(FC.getGenerators()).get_elements();
        is_Computed.set(ConeProperty::Generators);
    }
    if (FC.isComputed(ConeProperty::ExtremeRays)) {
        ExtremeRays = FC.getExtremeRays();
        is_Computed.set(ConeProperty::ExtremeRays);
    }
    if (FC.isComputed(ConeProperty::SupportHyperplanes)) {
        SupportHyperplanes = BasisChange.from_sublattice_dual(FC.getSupportHyperplanes()).get_elements();
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
    if (FC.isComputed(ConeProperty::Multiplicity)) {
        multiplicity = FC.getMultiplicity();
        is_Computed.set(ConeProperty::Multiplicity);
    }
    if (FC.isComputed(ConeProperty::HilbertBasis)) {
        HilbertBasis = BasisChange.from_sublattice(FC.getHilbertBasis()).get_elements();
        is_Computed.set(ConeProperty::HilbertBasis);
    }
    if (FC.isComputed(ConeProperty::Deg1Elements)) {
        Deg1Elements = BasisChange.from_sublattice(FC.getDeg1Elements()).get_elements();
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
    if (FC.isComputed(ConeProperty::Grading)) {
        if (!isComputed(ConeProperty::Grading)) {
            Grading = BasisChange.from_sublattice_dual(FC.getGrading());
            is_Computed.set(ConeProperty::Grading);
        }
        //compute denominator of Grading
        if (Generators.size() > 0) {
            GradingDenom  = v_scalar_product(Grading,Generators[0]);
            GradingDenom /= v_scalar_product(FC.getGrading(),FC.Generators[0]);
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

} // end namespace libnormaliz
