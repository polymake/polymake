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

#ifndef CONE_H_
#define CONE_H_

#include <vector>
#include <map>
#include "libnormaliz.h"
#include "cone_property.h"
#include "sublattice_representation.h"
#include "HilbertSeries.h"

namespace libnormaliz {
using std::vector;
using std::map;
using std::pair;

template<typename Integer> class Full_Cone;
template<typename Integer> class Matrix;

// type for simplex, short in contrast to class Simplex
template<typename Integer> struct SHORTSIMPLEX {
    vector<key_t> key;                // full key of simplex
    Integer height;                   // height of last vertex over opposite facet
    Integer vol;                      // volume if computed, 0 else
};

template<typename Integer> struct STANLEYDATA {
    vector<key_t> key;
    Matrix<Integer> offsets;
};

template<typename Integer>
class Cone {

//---------------------------------------------------------------------------
//                               public methods
//---------------------------------------------------------------------------
public:

//---------------------------------------------------------------------------
//                    Constructors, they preprocess the input 
//---------------------------------------------------------------------------

    /* give a single matrix as input */
    Cone(const vector< vector<Integer> >& input_data,
         InputType type = Type::integral_closure);
    /* give multiple input */
    Cone(const map< InputType , vector< vector<Integer> > >& multi_input_data);

//---------------------------------------------------------------------------
//                          give additional data
//---------------------------------------------------------------------------

    /* Sets the linear form which is used to grade. 
     * It has to be an N-grading, i.e. all generators must have a value >=1.
     * If it is not, a subclass of NormalizException will be thrown at the 
     * time of detection which can be in this method or later!
     * It will delete all data from the cone that depend on the grading!
     */
    void setGrading (const vector<Integer>& lf);


//---------------------------------------------------------------------------
//                           make computations
//---------------------------------------------------------------------------

    // return what was NOT computed
    ConeProperties compute(ComputationMode mode = Mode::hilbertBasisSeries); //default: everything
    ConeProperties compute(ConeProperties ToCompute);
//is done by compiler throug creation of CPies   // return true iff it could be computed
    ConeProperties compute(ConeProperty::Enum prop);

//---------------------------------------------------------------------------
//                         check what is computed
//---------------------------------------------------------------------------

    bool isComputed(ConeProperty::Enum prop) const;
    //returns true, when ALL properties in CheckComputed are computed
    bool isComputed(ConeProperties CheckComputed) const;

//---------------------------------------------------------------------------
//          get the results, these methods do not start a computation
//---------------------------------------------------------------------------

    size_t getDim() const { return dim; };
    vector< vector<Integer> > getGenerators() const;
    vector< vector<Integer> > getExtremeRays() const;
    vector< vector<Integer> > getSupportHyperplanes() const;
    vector< vector<Integer> > getEquations() const;
    vector< vector<Integer> > getCongruences() const;
    map< InputType , vector< vector<Integer> > > getConstraints() const;
    size_t getTriangulationSize() const;
    Integer getTriangulationDetSum() const;
    vector< vector<Integer> > getHilbertBasis() const;
    vector< vector<Integer> > getDeg1Elements() const;
    vector<Integer> getGrading() const;
    Integer getGradingDenom() const;
    mpq_class getMultiplicity() const;
    bool isPointed() const;
    bool isDeg1ExtremeRays() const;
    bool isDeg1HilbertBasis() const;
    bool isIntegrallyClosed() const;
    bool isReesPrimary() const;
    Integer getReesPrimaryMultiplicity() const;
    vector< vector<Integer> > getGeneratorsOfToricRing() const;
    Sublattice_Representation<Integer> getBasisChange() const;
    // the following methods return const refs to avoid copying of big data objects
    const HilbertSeries& getHilbertSeries() const; //general purpose object
    const vector< pair<vector<key_t>, Integer> >& getTriangulation() const;
    const list< STANLEYDATA<Integer> >& getStanleyDec() const;
    
//---------------------------------------------------------------------------
//                          private part
//---------------------------------------------------------------------------

private:    
    size_t dim;

    Sublattice_Representation<Integer> BasisChange;  //always use compose_basis_change() !
    bool BC_set;
    ConeProperties is_Computed;
    vector< vector<Integer> > GeneratorsOfToricRing;
    vector< vector<Integer> > Generators;
    vector<bool> ExtremeRays;
    vector< vector<Integer> > SupportHyperplanes;
    size_t TriangulationSize;
    Integer TriangulationDetSum;
    vector< pair<vector<key_t>, Integer> > Triangulation;
    list< STANLEYDATA<Integer> > StanleyDec;
    mpq_class multiplicity;
    vector< vector<Integer> > HilbertBasis;
    vector< vector<Integer> > Deg1Elements;
    HilbertSeries HSeries;
    vector< vector<Integer> > HilbertQuasiPolynomial;
    vector<Integer> Grading;
    Integer GradingDenom;
    bool pointed;
    bool deg1_extreme_rays;
    bool deg1_hilbert_basis;
    bool integrally_closed;
    bool rees_primary;
    Integer ReesPrimaryMultiplicity;

    void compose_basis_change(const Sublattice_Representation<Integer>& SR); // composes SR


    /* Progress input, depending on input_type */
    void prepare_input_type_0(const vector< vector<Integer> >& Input);
    void prepare_input_type_1(const vector< vector<Integer> >& Input);
    void prepare_input_type_2(const vector< vector<Integer> >& Input);
    void prepare_input_type_3(const vector< vector<Integer> >& Input);
    void prepare_input_type_10(const vector< vector<Integer> >& Binomials);
    void prepare_input_type_456(const Matrix<Integer>& Congruences, const Matrix<Integer>& Equations, const Matrix<Integer>& Inequalities);
    void prepare_input_type_45(const Matrix<Integer>& Equations, const Matrix<Integer>& Inequalities);

    /* only used by the constructors */
    void initialize();
    void single_matrix_input(const vector< vector<Integer> >& Input, InputType input_type);
    /* compute the generators using the support hyperplanes */
    void compute_generators();
    /* compute method for the dual_mode, used in compute(mode) */
    ConeProperties compute_dual();

    /* extract the data from Full_Cone, this may remove data from Full_Cone!*/
    void extract_data(Full_Cone<Integer>& FC);

};

}  //end namespace libnormaliz

#endif /* CONE_H_ */
