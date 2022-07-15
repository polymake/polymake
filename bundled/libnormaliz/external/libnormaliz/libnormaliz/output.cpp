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

#include <cstdlib>
#include <vector>
#include <list>
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <iomanip>

#include "libnormaliz/output.h"
#include "libnormaliz/general.h"
#include "output.h"
#include "libnormaliz/matrix.h"
#include "libnormaliz/vector_operations.h"
#include "libnormaliz/list_and_map_operations.h"
#include "libnormaliz/automorph.h"

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------

template <typename Integer>
Output<Integer>::Output() {
    out = true;
    inv = false;
    ext = false;
    esp = false;
    typ = false;
    egn = false;
    gen = false;
    cst = false;
    tri = false;
    tgn = false;
    ht1 = false;
    dec = false;
    lat = false;
    precomp = false;
    mod = false;
    msp = false;
    fac = false;
    aut = false;
    inc = false;

    lattice_ideal_input = false;
    no_ext_rays_output = false;
    no_supp_hyps_output = false;
    no_hilbert_basis_output = false;
    no_matrices_output = false;
    print_renf = true;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_lattice_ideal_input(bool value) {
    lattice_ideal_input = value;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_no_supp_hyps_output() {
    no_supp_hyps_output = true;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_no_ext_rays_output() {
    no_ext_rays_output = true;
}

template <typename Integer>
void Output<Integer>::set_no_hilbert_basis_output() {
    no_hilbert_basis_output = true;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_no_matrices_output() {
    no_matrices_output = true;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_name(const string& n) {
    name = n;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::setCone(Cone<Integer>& C) {
    this->Result = &C;
    dim = Result->getEmbeddingDim();
    homogeneous = !Result->isInhomogeneous();
    if (!using_renf<Integer>())
        lattice_or_space = "lattice";
    else
        lattice_or_space = "space";
    if (homogeneous) {
        of_cone = "";
        of_monoid = "";
        of_polyhedron = "";
        string absolute;
        if (!using_renf<Integer>())
            module_generators_name = " lattice points in polytope (Hilbert basis elements of degree 1)";
        else
            module_generators_name = " lattice points in polytope";
    }
    else {
        of_cone = " of recession cone";
        of_monoid = " of recession monoid";
        if (!using_renf<Integer>())
            monoid_or_cone = "monoid";
        else
            monoid_or_cone = "cone";
        of_polyhedron = " of polyhedron (homogenized)";
        if ((Result->isComputed(ConeProperty::ModuleGenerators) || Result->isComputed(ConeProperty::NumberLatticePoints)) &&
            Result->getRecessionRank() == 0) {
            if (!using_renf<Integer>())
                module_generators_name = " lattice points in polytope (module generators)";
            else
                module_generators_name = " lattice points in polytope";
        }
        else {
            if (using_renf<Integer>())
                module_generators_name = " lattice points in polytope";
            else
                module_generators_name = " module generators";
        }
    }
}

template <typename Number>
void Output<Number>::write_renf(ostream& os) const {
}

template <typename Number>
void Output<Number>::set_renf(const renf_class_shared renf, bool is_int_hull) {
}

#ifdef ENFNORMALIZ
template <>
void Output<renf_elem_class>::write_renf(ostream& os) const {
    if (print_renf) {
        auto polyemb = Cone<renf_elem_class>::getRenfData(&*Renf);
        os << "Real embedded number field:" << std::endl
           << "min_poly (" << polyemb[0] << ") embedding " << polyemb[1] << std::endl
           << std::endl;
    }
}

template <>
void Output<renf_elem_class>::set_renf(const renf_class_shared renf, bool is_int_hull) {
    Renf = renf;
    print_renf = !is_int_hull;
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_out(const bool& flag) {
    out = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_inv(const bool& flag) {
    inv = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_ext(const bool& flag) {
    ext = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_esp(const bool& flag) {
    esp = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_typ(const bool& flag) {
    typ = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_egn(const bool& flag) {
    egn = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_gen(const bool& flag) {
    gen = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_cst(const bool& flag) {
    cst = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_tri(const bool& flag) {
    tri = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_aut(const bool& flag) {
    aut = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_tgn(const bool& flag) {
    tgn = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_ht1(const bool& flag) {
    ht1 = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_dec(const bool& flag) {
    dec = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_precomp(const bool& flag) {
    precomp = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_mod(const bool& flag) {
    mod = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_lat(const bool& flag) {
    lat = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_msp(const bool& flag) {
    msp = flag;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_fac(const bool& flag) {
    fac = flag;
}
//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_inc(const bool& flag) {
    inc = flag;
}
//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_extra_files() {
    out = true;
    inv = true;
    gen = true;
    cst = true;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::set_write_all_files() {
    out = true;
    inv = true;
    ext = true;
    esp = true;
    typ = true;
    egn = true;
    gen = true;
    cst = true;
    ht1 = true;
    lat = true;
    mod = true;
    msp = true;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_matrix_ext(const Matrix<Integer>& M) const {
    if (ext == true) {
        M.print(name, "ext");
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_matrix_mod(const Matrix<Integer>& M) const {
    if (mod == true) {
        M.print(name, "mod");
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_matrix_lat(const Matrix<Integer>& M) const {
    if (ext == true) {
        M.print(name, "lat");
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_matrix_esp(const Matrix<Integer>& M) const {
    if (esp == true) {
        M.print(name, "esp");
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_matrix_typ(const Matrix<Integer>& M) const {
    if (typ == true) {
        M.print(name, "typ");
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_matrix_egn(const Matrix<Integer>& M) const {
    if (egn == true) {
        M.print(name, "egn");
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_matrix_gen(const Matrix<Integer>& M) const {
    if (gen == true) {
        M.print(name, "gen");
    }
}
//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_matrix_msp(const Matrix<Integer>& M) const {
    if (msp == true) {
        M.print(name, "msp");
    }
}

//---------------------------------------------------------------------------
template <typename Integer>
void Output<Integer>::write_perms_and_orbits(ofstream& out,
                                             const vector<vector<key_t> >& Perms,
                                             const vector<vector<key_t> >& Orbits,
                                             const string& type_string) const {
    size_t nr_objects = 0;
    if (Perms.size() > 0)
        nr_objects = Perms[0].size();

    out << Perms.size() << " permutations of " << nr_objects << " " << type_string << endl << endl;
    size_t nr_items = Perms.size();
    for (size_t i = 0; i < nr_items; ++i) {
        out << "Perm " << i + 1 << ":";
        for (unsigned int j : Perms[i])
            out << " " << j + 1;
        out << endl;
    }

    out << endl;

    out << "Cycle decompositions " << endl << endl;

    for (size_t i = 0; i < nr_items; ++i) {
        vector<vector<libnormaliz::key_t> > dec = cycle_decomposition(Perms[i]);
        out << "Perm " << i + 1 << ": ";
        pretty_print_cycle_dec(dec, out);
    }
    out << endl;

    out << Orbits.size() << " orbits of " << type_string << endl << endl;
    nr_items = Orbits.size();
    for (size_t i = 0; i < nr_items; ++i) {
        out << "Orbit " << i + 1 << " , length " << Orbits[i].size() << ": ";
        for (unsigned int j : Orbits[i])
            out << " " << j + 1;
        out << endl;
    }
    out << endl;
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_aut() const {
    string file_name = name + ".aut";
    ofstream out(file_name.c_str());

    string qualities_string = Result->getAutomorphismGroup().getQualitiesString();

    out << qualities_string << "automorphism group of order " << Result->getAutomorphismGroup().getOrder()
        << " (possibly approximation if very large)" << endl;

    if (Result->getAutomorphismGroup().getOrder() == 1)
        return;

    if (Result->getAutomorphismGroup().IsIntegralityChecked()) {
        if (Result->getAutomorphismGroup().IsIntegral())
            out << "Automorphisms are integral" << endl;
        else
            out << "Automorphisms are not integral" << endl;
    }
    else
        out << "Integrality not known" << endl;

    out << "************************************************************************" << endl;

    if (qualities_string.find("generators") != string::npos) {
        write_aut_ambient(out, "input generators");
        return;
    }

    if (qualities_string.find("inequalities") != string::npos) {
        write_aut_ambient(out, "input inequalities");
        return;
    }

    string extrays_string = "extreme rays";
    if (Result->isInhomogeneous()) {
        write_perms_and_orbits(out, Result->getAutomorphismGroup().getVerticesPerms(),
                               Result->getAutomorphismGroup().getVerticesOrbits(), "vertices of polyhedron");
        out << "************************************************************************" << endl;

        extrays_string = "extreme rays of recession cone";
    }

    if (Result->getNrExtremeRays() > 0) {
        write_perms_and_orbits(out, Result->getAutomorphismGroup().getExtremeRaysPerms(),
                               Result->getAutomorphismGroup().getExtremeRaysOrbits(), extrays_string);
        out << "************************************************************************" << endl;
    }

    write_perms_and_orbits(out, Result->getAutomorphismGroup().getSupportHyperplanesPerms(),
                           Result->getAutomorphismGroup().getSupportHyperplanesOrbits(), "support hyperplanes");

    out.close();
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_aut_ambient(ofstream& out, const string& gen_name) const {
    write_perms_and_orbits(out, Result->getAutomorphismGroup().getGensPerms(), Result->getAutomorphismGroup().getGensOrbits(),
                           gen_name);
    out << "************************************************************************" << endl;

    string qualities_string = Result->getAutomorphismGroup().getQualitiesString();

    if (qualities_string.find("Ambient") != string::npos) {
        write_perms_and_orbits(out, Result->getAutomorphismGroup().getLinFormsPerms(),
                               Result->getAutomorphismGroup().getLinFormsOrbits(), "Coordinates");
        out << "************************************************************************" << endl << endl;
    }
    out << gen_name << endl << endl;
    Result->getAutomorphismGroup().getGens().pretty_print(out, true, true);
    out.close();
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_precomp() const {
    if (!precomp)
        return;

    if (!Result->isComputed(ConeProperty::SupportHyperplanes)  // not all required data computed
        || !Result->isComputed(ConeProperty::ExtremeRays) || !Result->isComputed(ConeProperty::MaximalSubspace) ||
        !Result->isComputed(ConeProperty::Sublattice))
        return;

    string file_name = name + ".precomp.in";
    ofstream out(file_name.c_str());

    out << "amb_space " << Result->getEmbeddingDim() << endl;
#ifdef ENFNORMALIZ
    if (using_renf<Integer>()) {
        auto polyemb = Cone<renf_elem_class>::getRenfData(&*Renf);
        out << "number_field min_poly (" << polyemb[0] << ") embedding " << polyemb[1] << endl;
    }
#endif

    out << "support_hyperplanes " << Result->getNrSupportHyperplanes() << endl;
    Result->getSupportHyperplanesMatrix().pretty_print(out);
    size_t nr_ext = Result->getNrExtremeRays();
    if (Result->isComputed(ConeProperty::Dehomogenization))
        nr_ext += Result->getNrVerticesOfPolyhedron();
    out << "extreme_rays " << nr_ext << endl;
    Result->getExtremeRaysMatrix().pretty_print(out);
    if (Result->isComputed(ConeProperty::Dehomogenization))
        Result->getVerticesOfPolyhedronMatrix().pretty_print(out);
    const Sublattice_Representation<Integer>& BasisChange = Result->getSublattice();
    const Matrix<Integer>& LB = BasisChange.getEmbeddingMatrix();
    size_t nr_of_latt = LB.nr_of_rows();
    if (nr_of_latt < dim || BasisChange.getExternalIndex() != 1) {
        out << "generated_sublattice " << nr_of_latt << endl;
        LB.pretty_print(out);
    }
    if (Result->getDimMaximalSubspace() > 0) {
        out << "maximal_subspace " << Result->getDimMaximalSubspace() << endl;
        Result->getMaximalSubspaceMatrix().pretty_print(out);
    }
    if (Result->isComputed(ConeProperty::Grading)) {
        out << "grading" << endl;
        out << Result->getGrading();
    }
    if (Result->isComputed(ConeProperty::Dehomogenization)) {
        out << "dehomogenization" << endl;
        out << Result->getDehomogenization();
    }
    out.close();
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_tri() const {
    if (tri == true) {
        string file_name = name + ".tri";
        ofstream out(file_name.c_str());

        const pair<vector<SHORTSIMPLEX<Integer> >, Matrix<Integer> >& Tri = Result->getTriangulation();
        // const vector<vector<bool> >& Dec =
        //    Result->isComputed(ConeProperty::ConeDecomposition) ? Result->getOpenFacets() : vector<vector<bool> >();
        // auto idd = Dec.begin();

        out << Tri.first.size() << endl;
        size_t nr_extra_entries = 1;
        if (Result->isComputed(ConeProperty::ConeDecomposition))
            nr_extra_entries += Result->getSublattice().getRank() - Result->getDimMaximalSubspace();
        out << Result->getSublattice().getRank() - Result->getDimMaximalSubspace() + nr_extra_entries
            << endl;  // works also for empty list

        for (const auto& tit : Tri.first) {
            for (size_t i = 0; i < tit.key.size(); i++) {
                out << tit.key[i] + 1 << " ";
            }
            out << "   " << tit.vol;
            if (Result->isComputed(ConeProperty::ConeDecomposition)) {
                out << "   ";
                for (size_t i = 0; i < tit.key.size(); i++) {
                    out << " " << tit.Excluded[i];
                }
                // idd++;
            }
            out << endl;
        }
        /* if (Result->isTriangulationNested())
            out << "nested" << endl;
        else
            out << "plain" << endl;
        if (Result->isTriangulationPartial())
            out << "partial" << endl;*/
        out.close();
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_inc() const {
    if (inc == true) {
        string file_name = name + ".inc";
        ofstream out(file_name.c_str());

        size_t nr_vert = 0;
        if (Result->isInhomogeneous())
            nr_vert = Result->getNrVerticesOfPolyhedron();
        size_t nr_ext = Result->getNrExtremeRays();

        out << Result->getNrSupportHyperplanes() << endl;
        out << nr_vert << endl;
        out << nr_ext << endl;
        out << endl;

        for (size_t f = 0; f < Result->getIncidence().size(); ++f) {
            if (nr_vert > 0) {
                for (size_t j = 0; j < nr_vert; ++j)
                    out << Result->getIncidence()[f][j];
                out << "  ";
            }
            for (size_t j = 0; j < nr_ext; ++j)
                out << Result->getIncidence()[f][j + nr_vert];
            out << endl;
        }

        out << "primal" << endl;

        out.close();
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_dual_inc() const {
    if (inc == true) {
        string file_name = name + ".inc";
        ofstream out(file_name.c_str());

        size_t nr_vert = 0;
        if (Result->isInhomogeneous())
            nr_vert = Result->getNrVerticesOfPolyhedron();
        size_t nr_ext = Result->getNrExtremeRays();
        size_t nr_supp = Result->getNrSupportHyperplanes();

        out << nr_vert << endl;
        out << nr_ext << endl;
        out << nr_supp << endl;
        out << endl;

        for (size_t f = 0; f < Result->getDualIncidence().size(); ++f) {
            for (size_t j = 0; j < nr_supp; ++j)
                out << Result->getDualIncidence()[f][j];
            out << endl;
        }

        out << "dual" << endl;

        out.close();
    }
}
//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_fac() const {
    if (fac == true) {
        string file_name = name + ".fac";
        ofstream out(file_name.c_str());
        out << Result->getFaceLattice().size() << endl;
        out << Result->getNrSupportHyperplanes() << endl;
        out << endl;

        for (const auto& f : Result->getFaceLattice()) {
            for (size_t k = 0; k < f.first.size(); ++k)
                out << f.first[k];
            out << " " << f.second << endl;
        }

        out << "primal" << endl;

        out.close();
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_dual_fac() const {
    if (fac == true) {
        string file_name = name + ".fac";
        ofstream out(file_name.c_str());
        out << Result->getDualFaceLattice().size() << endl;
        if (Result->isInhomogeneous()) {
            out << Result->getNrVerticesOfPolyhedron() << endl;
        }
        else {
            out << Result->getNrExtremeRays() << endl;
        }
        out << endl;

        for (const auto& f : Result->getDualFaceLattice()) {
            for (size_t k = 0; k < f.first.size(); ++k)
                out << f.first[k];
            out << " " << f.second << endl;
        }

        out << "dual" << endl;

        out.close();
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_Stanley_dec() const {
    if (dec && Result->isComputed(ConeProperty::StanleyDec)) {
        ofstream out((name + ".dec").c_str());

        if (Result->isComputed(ConeProperty::InclusionExclusionData)) {
            const vector<pair<vector<libnormaliz::key_t>, long> >& InExData = Result->getInclusionExclusionData();
            out << "in_ex_data" << endl;
            out << InExData.size() << endl;
            for (const auto& i : InExData) {
                out << i.first.size() << " ";
                for (unsigned int j : i.first) {
                    out << j + 1 << " ";
                }
                out << i.second << endl;
            }
        }

        out << "Stanley_dec" << endl;
        const list<STANLEYDATA<Integer> >& StanleyDec = Result->getStanleyDec().first;  // generators not needed here
        auto S = StanleyDec.begin();
        size_t i;

        out << StanleyDec.size() << endl;
        for (; S != StanleyDec.end(); ++S) {
            for (i = 0; i < S->key.size(); ++i)
                out << S->key[i] + 1 << " ";
            out << endl;
            S->offsets.print(out);
            out << endl;
        }
        out.close();
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_matrix_ht1(const Matrix<Integer>& M) const {
    if (ht1 == true) {
        M.print(name, "ht1");
    }
}

//---------------------------------------------------------------------------

string is_maximal(long a, long b) {
    return (a == b) ? " (maximal)" : "";
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_inv_file() const {
    if (inv == true) {  // printing .inv file
        size_t i;
        string name_open = name + ".inv";  // preparing output files
        const char* file = name_open.c_str();
        ofstream inv(file);

        if (Result->isComputed(ConeProperty::Dehomogenization) && Result->isComputed(ConeProperty::NumberLatticePoints)) {
            inv << "integer number_module_generators = " << Result->getNumberLatticePoints() << endl;
        }
        if (Result->isComputed(ConeProperty::HilbertBasis)) {
            inv << "integer hilbert_basis_elements = " << Result->getNrHilbertBasis() << endl;
        }

        if (Result->isComputed(ConeProperty::VerticesOfPolyhedron)) {
            inv << "integer number_vertices_polyhedron = " << Result->getNrVerticesOfPolyhedron() << endl;
        }
        if (Result->isComputed(ConeProperty::ExtremeRays)) {
            size_t nr_ex_rays = Result->getNrExtremeRays();
            inv << "integer number_extreme_rays = " << nr_ex_rays << endl;
        }
        if (Result->isComputed(ConeProperty::FVector)) {
            inv << "vector " << Result->getFVector().size() << " f_vector = " << Result->getFVector();
        }
        if (Result->isComputed(ConeProperty::DualFVector)) {
            inv << "vector " << Result->getDualFVector().size() << " dual_f_vector = " << Result->getDualFVector();
        }
        if (Result->isComputed(ConeProperty::MaximalSubspace)) {
            size_t dim_max_subspace = Result->getDimMaximalSubspace();
            inv << "integer dim_max_subspace = " << dim_max_subspace << endl;
        }
        if (Result->isComputed(ConeProperty::ModuleGeneratorsOverOriginalMonoid)) {
            inv << "integer number_module_generators_original_monoid = " << Result->getNrModuleGeneratorsOverOriginalMonoid()
                << endl;
        }

        inv << "integer embedding_dim = " << dim << endl;
        if (homogeneous) {
            if (Result->isComputed(ConeProperty::Sublattice)) {
                inv << "integer rank = " << Result->getRank() << endl;
                if (!using_renf<Integer>())
                    inv << "integer external_index = " << Result->getSublattice().getExternalIndex() << endl;
            }
        }
        else {
            if (Result->isComputed(ConeProperty::AffineDim))
                inv << "integer affine_dim_polyhedron = " << Result->getAffineDim() << endl;
            if (Result->isComputed(ConeProperty::RecessionRank))
                inv << "integer recession_rank = " << Result->getRecessionRank() << endl;
        }

        if (Result->isComputed(ConeProperty::OriginalMonoidGenerators)) {
            inv << "integer internal_index = " << Result->getInternalIndex() << endl;
        }
        if (Result->isComputed(ConeProperty::SupportHyperplanes)) {
            inv << "integer number_support_hyperplanes = " << Result->getNrSupportHyperplanes() << endl;
        }
        if (Result->isComputed(ConeProperty::TriangulationSize)) {
            inv << "integer size_triangulation = " << Result->getTriangulationSize() << endl;
        }
        if (Result->isComputed(ConeProperty::TriangulationDetSum)) {
            inv << "integer sum_dets = " << Result->getTriangulationDetSum() << endl;
        }

        if (Result->isComputed(ConeProperty::IsIntegrallyClosed)) {
            if (Result->isIntegrallyClosed())
                inv << "boolean integrally_closed = true" << endl;
            else
                inv << "boolean integrally_closed = false" << endl;
        }

        if (!Result->isComputed(ConeProperty::Dehomogenization)) {
            inv << "boolean inhomogeneous = false" << endl;
        }
        else {
            inv << "boolean inhomogeneous = true" << endl;
            vector<Integer> Linear_Form = Result->getDehomogenization();
            inv << "vector " << Linear_Form.size() << " dehomogenization = " << Linear_Form;
        }
        if (Result->isComputed(ConeProperty::AxesScaling))
            inv << "vector " << Result->getAxesScaling().size() << " axes_scaling " << Result->getAxesScaling();
        if (Result->isComputed(ConeProperty::Grading) == false) {
            if (Result->isComputed(ConeProperty::ExtremeRays)) {
                inv << "boolean graded = "
                    << "false" << endl;
            }
        }
        else {
            inv << "boolean graded = "
                << "true" << endl;
            if (!Result->isComputed(ConeProperty::Dehomogenization) && Result->isComputed(ConeProperty::NumberLatticePoints)) {
                inv << "integer degree_1_elements = " << Result->getNumberLatticePoints() << endl;
            }
            vector<Integer> Linear_Form = Result->getGrading();
            inv << "vector " << Linear_Form.size() << " grading = ";
            for (i = 0; i < Linear_Form.size(); i++) {
                inv << Linear_Form[i] << " ";
            }
            inv << endl;
            inv << "integer grading_denom = " << Result->getGradingDenom() << endl;
            if (Result->isComputed(ConeProperty::Multiplicity)) {
                mpq_class mult = Result->getMultiplicity();
                inv << "integer multiplicity = " << mult.get_num() << endl;
                inv << "integer multiplicity_denom = " << mult.get_den() << endl;
            }
            if (Result->isComputed(ConeProperty::Volume) && !using_renf<Integer>()) {
                mpq_class vol = Result->getVolume();
                inv << "integer volume = " << vol.get_num() << endl;
                inv << "integer volume_denom = " << vol.get_den() << endl;
            }
            if (Result->isComputed(ConeProperty::WeightedEhrhartSeries)) {
                const HilbertSeries& HS = Result->getIntData().getWeightedEhrhartSeries().first;

                inv << "vector " << HS.getNum().size() << " weighted_ehrhart_series_num = " << HS.getNum();
                inv << "integer num_common_denom = " << Result->getIntData().getWeightedEhrhartSeries().second << endl;
                inv << "vector " << to_vector(HS.getDenom()).size()
                    << " weighted_ehrhart_series_num = " << to_vector(HS.getDenom());
                Result->getIntData().computeWeightedEhrhartQuasiPolynomial();
                if (Result->getIntData().isWeightedEhrhartQuasiPolynomialComputed()) {
                    if (HS.get_nr_coeff_quasipol() >= 0) {
                        inv << "integer nr_coeff_weightedEhrhart__quasipol = " << HS.get_nr_coeff_quasipol() << endl;
                    }
                    vector<vector<mpz_class> > hqp = Result->getIntData().getWeightedEhrhartQuasiPolynomial();
                    inv << "matrix " << hqp.size() << " " << hqp[0].size() << " weighted_ehrhart_quasipolynomial = ";
                    inv << endl << hqp;
                    inv << "integer weighted_ehrhart_quasipolynomial_denom = "
                        << Result->getIntData().getWeightedEhrhartQuasiPolynomialDenom() << endl;
                }
                if (HS.get_expansion_degree() > -1) {
                    vector<mpz_class> expansion = HS.getExpansion();
                    inv << "vector weighted_ehrhart_series_expansion " << expansion.size() << " = " << expansion;
                    inv << "integer expansion_coeff_common_denom = " << Result->getIntData().getWeightedEhrhartSeries().second
                        << endl;
                }
            }

            if (Result->isComputed(ConeProperty::VirtualMultiplicity)) {
                mpq_class mult = Result->getVirtualMultiplicity();
                inv << "integer virtual_multiplicity = " << mult.get_num() << endl;
                inv << "integer virtual_multiplicity_denom = " << mult.get_den() << endl;
            }
            if (Result->isComputed(ConeProperty::Integral)) {
                mpq_class mult = Result->getIntegral();
                inv << "integer integral = " << mult.get_num() << endl;
                inv << "integer integral_denom = " << mult.get_den() << endl;
            }
            if (Result->isComputed(ConeProperty::HilbertSeries)) {
                const HilbertSeries& HS = Result->getHilbertSeries();
                vector<mpz_class> HSnum;
                vector<denom_t> HSdenom;
                if (Result->isComputed(ConeProperty::HSOP)) {
                    HSnum = HS.getHSOPNum();
                    HSdenom = to_vector(HS.getHSOPDenom());
                }
                else {
                    HSnum = HS.getNum();
                    HSdenom = to_vector(HS.getDenom());
                }
                inv << "vector " << HSnum.size() << " hilbert_series_num = ";
                inv << HSnum;
                inv << "vector " << HSdenom.size() << " hilbert_series_denom = ";
                inv << HSdenom;
                HS.computeHilbertQuasiPolynomial();
                if (HS.isHilbertQuasiPolynomialComputed()) {
                    if (HS.get_nr_coeff_quasipol() >= 0) {
                        inv << "integer nr_coeff_hilbert_quasipol = " << HS.get_nr_coeff_quasipol() << endl;
                    }
                    vector<vector<mpz_class> > hqp = HS.getHilbertQuasiPolynomial();
                    inv << "matrix " << hqp.size() << " " << hqp[0].size() << " hilbert_quasipolynomial = ";
                    inv << endl << hqp;
                    inv << "integer hilbert_quasipolynomial_denom = " << HS.getHilbertQuasiPolynomialDenom() << endl;
                }
                if (HS.get_expansion_degree() > -1) {
                    vector<mpz_class> expansion = HS.getExpansion();
                    inv << "vector Hilbert_series_expansion " << expansion.size() << " = " << expansion;
                }
            }
        }
        if (Result->isComputed(ConeProperty::IsReesPrimary)) {
            if (Result->isReesPrimary()) {
                inv << "boolean primary = true" << endl;
            }
            else {
                inv << "boolean primary = false" << endl;
            }
        }
        if (Result->isComputed(ConeProperty::ReesPrimaryMultiplicity)) {
            inv << "integer ideal_multiplicity = " << Result->getReesPrimaryMultiplicity() << endl;
        }

        if (Result->isComputed(ConeProperty::ClassGroup)) {
            inv << "vector " << Result->getClassGroup().size() << " class_group = " << Result->getClassGroup();
        }

        if (Result->isComputed(ConeProperty::IsGorenstein)) {
            if (Result->isGorenstein()) {
                inv << "boolean Gorenstein = true" << endl;
                inv << "vector " << Result->getGeneratorOfInterior().size()
                    << " generator_of_interior = " << Result->getGeneratorOfInterior();
            }
            else
                inv << "boolean Gorenstein = false" << endl;
        }

        inv.close();
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::writeWeightedEhrhartSeries(ofstream& out) const {
    HilbertSeries HS = Result->getIntData().getWeightedEhrhartSeries().first;
    out << "Weighted Ehrhart series:" << endl;
    vector<mpz_class> num(HS.getNum());
    for (const auto& i : num)
        out << i << " ";
    out << endl << "Common denominator of coefficients: ";
    out << Result->getIntData().getWeightedEhrhartSeries().second << endl;
    map<long, long> HS_Denom = HS.getDenom();
    long nr_factors = 0;
    for (const auto& it : HS_Denom) {
        nr_factors += it.second;
    }
    out << "Series denominator with " << nr_factors << " factors:" << endl;
    out << HS.getDenom();
    if (HS.getShift() != 0) {
        out << "shift = " << HS.getShift() << endl << endl;
    }
    out << "degree of weighted Ehrhart series as rational function = " << HS.getDegreeAsRationalFunction() << endl << endl;

    if (HS.get_expansion_degree() > -1) {
        vector<mpz_class> expansion = HS.getExpansion();
        out << "Expansion of weighted Ehrhart series" << endl;
        for (long i = 0; i < (long)expansion.size(); ++i)
            out << i + HS.getShift() << ": " << expansion[i] << endl;
        out << "Common denominator of coefficients: = ";
        out << Result->getIntData().getWeightedEhrhartSeries().second << endl;
        out << endl;
    }

    long period = HS.getPeriod();
    if (period == 1) {
        out << "Weighted Ehrhart polynomial:" << endl;
        for (const auto& i : HS.getHilbertQuasiPolynomial()[0])
            out << i << " ";
        out << endl;
        out << "with common denominator = ";
        out << HS.getHilbertQuasiPolynomialDenom() * Result->getIntData().getNumeratorCommonDenom();
    }
    else {
        // output cyclonomic representation
        out << "Weighted Ehrhart series with cyclotomic denominator:" << endl;
        num = HS.getCyclotomicNum();
        for (const auto& i : num)
            out << i << " ";
        out << endl << "Common denominator of coefficients = ";
        out << Result->getIntData().getWeightedEhrhartSeries().second << endl;
        out << "Series cyclotomic denominator:" << endl;
        out << HS.getCyclotomicDenom();
        out << endl;
        // Weighted Ehrhart quasi-polynomial
        // vector< vector<mpz_class> > hilbert_quasi_poly = HS.getHilbertQuasiPolynomial();
        if (HS.isHilbertQuasiPolynomialComputed()) {
            out << "Weighted Ehrhart quasi-polynomial of period " << period << ":" << endl;
            if (HS.get_nr_coeff_quasipol() >= 0) {
                out << "only " << HS.get_nr_coeff_quasipol() << " highest coefficients computed" << endl;
                out << "their common period is " << HS.getHilbertQuasiPolynomial().size() << "." << endl;
            }
            Matrix<mpz_class> HQP(HS.getHilbertQuasiPolynomial());
            HQP.pretty_print(out, true);
            out << "with common denominator: " << Result->getIntData().getWeightedEhrhartQuasiPolynomialDenom() << endl;
        }
        else {
            out << "Weighted Ehrhart quasi-polynomial has period " << period << endl;
        }
    }

    out << endl << endl;

    if (HS.isHilbertQuasiPolynomialComputed()) {
        long deg = HS.getHilbertQuasiPolynomial()[0].size() - 1;
        out << "Degree of (quasi)polynomial: " << deg << endl;

        long virtDeg = Result->getRank() + Result->getIntData().getDegreeOfPolynomial() - 1;
        out << endl << "Expected degree = " << virtDeg << endl;
    }

    if (Result->isComputed(ConeProperty::VirtualMultiplicity)) {
        string virtual_mult_string = "Virtual multiplicity";
        if (Result->isComputed(ConeProperty::FixedPrecision))
            virtual_mult_string += " (fixed precision)";
        virtual_mult_string += " = ";
        out << endl << virtual_mult_string;
        out << Result->getIntData().getVirtualMultiplicity() << endl;
        if (Result->getIntData().getVirtualMultiplicity().get_den() != 1)
            out << "Virtual multiplicity (float) = " << std::setprecision(12)
                << mpq_to_nmz_float(Result->getIntData().getVirtualMultiplicity()) << endl;
        out << endl;
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::writeSeries(ofstream& out, const HilbertSeries& HS, string HilbertOrEhrhart) const {
    vector<mpz_class> HS_Num;
    map<long, long> HS_Denom;
    if (Result->isComputed(ConeProperty::HSOP)) {
        HS_Denom = HS.getHSOPDenom();
        HS_Num = HS.getHSOPNum();
        string HSOP;
        if (!HS_Denom.empty())  // we disable the HSOP attribute if the series is a polynomial
            HSOP = " (HSOP)";
        out << HilbertOrEhrhart << "series" << HSOP << ":" << endl << HS_Num;
    }
    else {
        HS_Denom = HS.getDenom();
        HS_Num = HS.getNum();
        out << HilbertOrEhrhart + "series:" << endl << HS_Num;
    }
    long nr_factors = 0;
    for (auto& it : HS_Denom) {
        nr_factors += it.second;
    }
    out << "denominator with " << nr_factors << " factors:" << endl;
    out << HS_Denom;
    out << endl;
    if (HS.getShift() != 0) {
        out << "shift = " << HS.getShift() << endl << endl;
    }

    out << "degree of " + HilbertOrEhrhart + "Series as rational function = " << HS.getDegreeAsRationalFunction() << endl << endl;
    if (v_is_symmetric(HS_Num)) {
        out << "The numerator of the " + HilbertOrEhrhart + "series is symmetric." << endl << endl;
    }
    if (HS.get_expansion_degree() > -1) {
        vector<mpz_class> expansion = HS.getExpansion();
        out << "Expansion of " + HilbertOrEhrhart + "series" << endl;
        for (size_t i = 0; i < expansion.size(); ++i)
            out << i + HS.getShift() << ": " << expansion[i] << endl;
        out << endl;
    }
    long period = HS.getPeriod();
    if (period == 1 && (HS_Denom.size() == 0 || HS_Denom.begin()->first == (long)HS_Denom.size())) {
        out << HilbertOrEhrhart + "polynomial:" << endl;
        out << HS.getHilbertQuasiPolynomial()[0];
        out << "with common denominator = ";
        out << HS.getHilbertQuasiPolynomialDenom();
        out << endl << endl;
    }
    else {
        // output cyclonomic representation
        out << HilbertOrEhrhart << "series with cyclotomic denominator:" << endl;
        out << HS.getCyclotomicNum();
        out << "cyclotomic denominator:" << endl;
        out << HS.getCyclotomicDenom();
        out << endl;
        // Hilbert quasi-polynomial
        HS.computeHilbertQuasiPolynomial();
        if (HS.isHilbertQuasiPolynomialComputed()) {
            out << HilbertOrEhrhart + "quasi-polynomial of period " << period << ":" << endl;
            if (HS.get_nr_coeff_quasipol() >= 0) {
                out << "only " << HS.get_nr_coeff_quasipol() << " highest coefficients computed" << endl;
                out << "their common period is " << HS.getHilbertQuasiPolynomial().size() << "" << endl;
            }
            Matrix<mpz_class> HQP(HS.getHilbertQuasiPolynomial());
            HQP.pretty_print(out, true);
            out << "with common denominator = " << HS.getHilbertQuasiPolynomialDenom();
        }
        else {
            out << HilbertOrEhrhart + "quasi-polynomial has period " << period << endl;
        }
        out << endl << endl;
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void Output<Integer>::write_files() const {
    size_t i, nr;
    vector<libnormaliz::key_t> rees_ideal_key;

    write_precomp();  // only if asked for

    if (esp && Result->isComputed(ConeProperty::SupportHyperplanes) && Result->isComputed(ConeProperty::Sublattice)) {
        // write the suport hyperplanes of the full dimensional cone
        const Sublattice_Representation<Integer>& BasisChange = Result->getSublattice();
        Matrix<Integer> Support_Hyperplanes_Full_Cone = BasisChange.to_sublattice_dual(Result->getSupportHyperplanesMatrix());
        // Support_Hyperplanes_Full_Cone.print(name,"esp");
        string esp_string = name + ".esp";
        const char* esp_file = esp_string.c_str();
        ofstream esp_out(esp_file);
        Support_Hyperplanes_Full_Cone.print(esp_out);
        esp_out << "inequalities" << endl;
        if (Result->isComputed(ConeProperty::Grading)) {
            esp_out << 1 << endl << Result->getRank() << endl;
            esp_out << BasisChange.to_sublattice_dual(Result->getGrading());
            esp_out << "grading" << endl;
        }
        if (Result->isComputed(ConeProperty::Dehomogenization)) {
            esp_out << 1 << endl << Result->getRank() << endl;
            esp_out << BasisChange.to_sublattice_dual(Result->getDehomogenization());
            esp_out << "dehomogenization" << endl;
        }
        esp_out.close();
    }
    if (tgn && (Result->getTriangulation().first.size() > 0 || Result->isComputed(ConeProperty::StanleyDec)))
        Result->getTriangulation().second.print(name, "tgn");

    if (tri && Result->getTriangulation().first.size() > 0) {  // write triangulation
        write_tri();
    }

    if (fac && Result->isComputed(ConeProperty::FaceLattice)) {  // write face lattice
        write_fac();
    }

    if (fac && Result->isComputed(ConeProperty::DualFaceLattice)) {  // write dual face lattice
        write_dual_fac();
    }

    if (inc && Result->isComputed(ConeProperty::Incidence)) {  // write incidence lattice
        write_inc();
    }

    if (inc && Result->isComputed(ConeProperty::DualIncidence)) {  // write incidence lattice
        write_dual_inc();
    }

    if (out == true) {                     // printing .out file
        string name_open = name + ".out";  // preparing output files
        const char* file = name_open.c_str();
        ofstream out(file);
        if (out.fail()) {
            throw BadInputException("Cannot write to output file. Typo in directory name?");
        }

        // write "header" of the .out file

        write_renf(out);

        size_t nr_orig_gens = 0;
        if (lattice_ideal_input) {
            nr_orig_gens = Result->getNrOriginalMonoidGenerators();
            out << nr_orig_gens << " original generators of the toric ring" << endl;
        }
        if (!homogeneous && Result->isComputed(ConeProperty::NumberLatticePoints) && !Result->isIntHullCone()) {
            out << Result->getNumberLatticePoints() << module_generators_name << endl;
        }
        if (Result->isComputed(ConeProperty::HilbertBasis) && !Result->isIntHullCone()) {
            out << Result->getNrHilbertBasis() << " Hilbert basis elements" << of_monoid << endl;
        }
        if (homogeneous && Result->isComputed(ConeProperty::NumberLatticePoints)) {
            out << Result->getNumberLatticePoints() << module_generators_name << endl;
        }
        if (Result->isComputed(ConeProperty::IsReesPrimary) && Result->isComputed(ConeProperty::HilbertBasis)) {
            const Matrix<Integer>& Hilbert_Basis = Result->getHilbertBasisMatrix();
            nr = Hilbert_Basis.nr_of_rows();
            for (i = 0; i < nr; i++) {
                if (Hilbert_Basis[i][dim - 1] == 1) {
                    rees_ideal_key.push_back(i);
                }
            }
            out << rees_ideal_key.size() << " generators of integral closure of the ideal" << endl;
        }
        if (Result->isComputed(ConeProperty::VerticesOfPolyhedron)) {
            out << Result->getNrVerticesOfPolyhedron() << " vertices of polyhedron" << endl;
        }
        if (Result->isComputed(ConeProperty::ExtremeRays)) {
            out << Result->getNrExtremeRays() << " extreme rays" << of_cone << endl;
        }
        if (Result->isComputed(ConeProperty::ModuleGeneratorsOverOriginalMonoid)) {
            out << Result->getNrModuleGeneratorsOverOriginalMonoid() << " module generators over original monoid" << endl;
        }
        if (Result->isComputed(ConeProperty::SupportHyperplanes)) {
            out << Result->getNrSupportHyperplanes() << " support hyperplanes" << of_polyhedron << endl;
        }
        out << endl;
        if (Result->isComputed(ConeProperty::FVector)) {
            string trunc = "";
            if (Result->getFVector()[0] != 1)
                trunc = " (possibly truncated)";
            out << "f-vector" << trunc << ":" << endl << Result->getFVector() << endl;
        }
        if (Result->isComputed(ConeProperty::DualFVector)) {
            string trunc = "";
            if (Result->getDualFVector()[0] != 1)
                trunc = " (possibly truncated)";
            out << "dual f-vector" << trunc << ":" << endl << Result->getDualFVector() << endl;
        }
        if (Result->isComputed(ConeProperty::ExcludedFaces)) {
            out << Result->getNrExcludedFaces() << " excluded faces" << endl;
            out << endl;
        }
        out << "embedding dimension = " << dim << endl;
        if (homogeneous) {
            if (Result->isComputed(ConeProperty::Sublattice)) {
                auto rank = Result->getRank();
                out << "rank = " << rank << is_maximal(rank, dim) << endl;
                if (!using_renf<Integer>())
                    out << "external index = " << Result->getSublattice().getExternalIndex() << endl;
            }
        }
        else {  // now inhomogeneous case
            if (Result->isComputed(ConeProperty::AffineDim))
                out << "affine dimension of the polyhedron = " << Result->getAffineDim()
                    << is_maximal(Result->getAffineDim(), dim - 1) << endl;
            if (Result->isComputed(ConeProperty::RecessionRank)) {
                out << "rank of recession " << monoid_or_cone << " = " << Result->getRecessionRank();
                if (Result->getRecessionRank() == 0)
                    out << " (polyhedron is polytope)";
                out << endl;
            }
        }

        if (Result->isComputed(ConeProperty::OriginalMonoidGenerators)) {
            out << "internal index = " << Result->getInternalIndex() << endl;
        }

        if (Result->isComputed(ConeProperty::MaximalSubspace)) {
            size_t dim_max_subspace = Result->getDimMaximalSubspace();
            if (dim_max_subspace > 0)
                out << "dimension of maximal subspace = " << dim_max_subspace << endl;
        }

        if (homogeneous && Result->isComputed(ConeProperty::IsIntegrallyClosed)) {
            if (Result->isIntegrallyClosed()) {
                out << "original monoid is integrally closed in chosen lattice" << endl;
            }
            else {
                out << "original monoid is not integrally closed in chosen lattice" << endl;
                if (Result->isComputed(ConeProperty::WitnessNotIntegrallyClosed)) {
                    out << "witness for not being integrally closed:" << endl;
                    out << Result->getWitnessNotIntegrallyClosed();
                }
                if (Result->getUnitGroupIndex() > 1) {
                    out << "unit group index = " << Result->getUnitGroupIndex() << endl;
                }
            }
        }
        out << endl;
        if (Result->isComputed(ConeProperty::AxesScaling)) {
            out << "scaling of axes" << endl;
            out << Result->getAxesScaling();
            out << endl;
        }

        if (Result->isComputed(ConeProperty::TriangulationSize)) {
            out << "size of ";
            if (Result->isTriangulationNested())
                out << "nested ";
            if (Result->isTriangulationPartial())
                out << "partial ";
            out << "triangulation   = " << Result->getTriangulationSize() << endl;
        }
        if (Result->isComputed(ConeProperty::TriangulationDetSum)) {
            out << "resulting sum of |det|s = " << Result->getTriangulationDetSum() << endl;
        }
        if (Result->isComputed(ConeProperty::TriangulationSize)) {
            out << endl;
        }
        if (Result->isComputed(ConeProperty::Dehomogenization)) {
            out << "dehomogenization:" << endl << Result->getDehomogenization() << endl;
        }

        if (Result->isComputed(ConeProperty::Grading)) {
            out << "grading:" << endl << Result->getGrading();
            Integer denom = Result->getGradingDenom();
            if (denom != 1) {
                out << "with denominator = " << denom << endl;
            }
            out << endl;
            if (homogeneous && Result->isComputed(ConeProperty::ExtremeRays)) {
                out << "degrees of extreme rays:" << endl;
                map<Integer, long> deg_count;
                vector<Integer> degs = Result->getExtremeRaysMatrix().MxV(Result->getGrading());
                for (i = 0; i < degs.size(); ++i) {
                    deg_count[degs[i] / denom]++;
                }
                out << deg_count;
            }
        }
        else if (Result->isComputed(ConeProperty::IsDeg1ExtremeRays) && !using_renf<Integer>()) {
            if (!Result->isDeg1ExtremeRays()) {
                out << "No implicit grading found" << endl;
            }
        }
        out << endl;
        if (homogeneous && Result->isComputed(ConeProperty::IsDeg1HilbertBasis) && Result->isDeg1ExtremeRays()) {
            if (Result->isDeg1HilbertBasis()) {
                out << "Hilbert basis elements are of degree 1";
            }
            else {
                out << "Hilbert basis elements are not of degree 1";
            }
            out << endl << endl;
        }
        if (Result->isComputed(ConeProperty::ModuleRank)) {
            out << "module rank = " << Result->getModuleRank() << endl;
        }
        if (Result->isComputed(ConeProperty::Multiplicity)) {
            string mult_string = "multiplicity ";
            if (Result->isComputed(ConeProperty::FixedPrecision))
                mult_string += "(fixed precision) ";
            mult_string += "= ";
            out << mult_string << Result->getMultiplicity() << endl;
            if (Result->getMultiplicity().get_den() != 1)
                out << "multiplicity (float) = " << std::setprecision(12) << mpq_to_nmz_float(Result->getMultiplicity()) << endl;
        }
        if (Result->isComputed(ConeProperty::Volume) && Result->isComputed(ConeProperty::Sublattice)) {
            if (!using_renf<Integer>())
                out << "volume (lattice normalized) = " << Result->getVolume() << endl;
            else
                out << "volume (lattice normalized) = " << Result->getRenfVolume() << endl;
            if (!using_renf<Integer>() && Result->getVolume().get_den() != 1)
                out << "volume (normalized, float) = " << std::setprecision(12) << mpq_to_nmz_float(Result->getVolume()) << endl;
            out << "volume (Euclidean) = " << std::setprecision(12) << Result->getEuclideanVolume() << endl;
        }
        if (Result->isComputed(ConeProperty::ModuleRank) || Result->isComputed(ConeProperty::Multiplicity) ||
            Result->isComputed(ConeProperty::Volume)) {
            out << endl;
        }

        if (Result->isComputed(ConeProperty::HilbertSeries)) {
            writeSeries(out, Result->getHilbertSeries(), "Hilbert ");
        }

        if (Result->isComputed(ConeProperty::EhrhartSeries)) {
            writeSeries(out, Result->getEhrhartSeries(), "Ehrhart ");
        }

        if (Result->isComputed(ConeProperty::WeightedEhrhartSeries))
            writeWeightedEhrhartSeries(out);

        if (Result->isComputed(ConeProperty::VirtualMultiplicity) &&
            !Result->isComputed(ConeProperty::WeightedEhrhartQuasiPolynomial)) {
            out << "virtual multiplicity = " << Result->getVirtualMultiplicity() << endl;
            if (Result->getVirtualMultiplicity().get_den() != 1)
                out << "virtual multiplicity (float) = " << std::setprecision(12)
                    << mpq_to_nmz_float(Result->getVirtualMultiplicity()) << endl;
            out << endl;
        }

        if (Result->isComputed(ConeProperty::Integral)) {
            string integral_string = "integral ";
            if (Result->isComputed(ConeProperty::FixedPrecision))
                integral_string += "(fixed precision) ";
            integral_string += "= ";
            out << integral_string << Result->getIntegral() << endl;
            if (Result->getIntegral().get_den() != 1)
                out << "integral (float) = " << std::setprecision(12) << mpq_to_nmz_float(Result->getIntegral()) << endl;
            if (Result->isComputed(ConeProperty::EuclideanIntegral))
                out << "integral (euclidean) = " << std::setprecision(12) << Result->getEuclideanIntegral() << endl;
            out << endl;
        }

        if (Result->isComputed(ConeProperty::IsReesPrimary)) {
            if (Result->isReesPrimary()) {
                out << "ideal is primary to the ideal generated by the indeterminates" << endl;
            }
            else {
                out << "ideal is not primary to the ideal generated by the indeterminates" << endl;
            }
            if (Result->isComputed(ConeProperty::ReesPrimaryMultiplicity)) {
                out << "multiplicity of the ideal = " << Result->getReesPrimaryMultiplicity() << endl;
            }
            out << endl;
        }

        if (Result->isComputed(ConeProperty::ClassGroup)) {
            vector<Integer> ClassGroup = Result->getClassGroup();
            out << "rank of class group = " << ClassGroup[0] << endl;
            if (ClassGroup.size() == 1)
                out << "class group is free" << endl << endl;
            else {
                ClassGroup.erase(ClassGroup.begin());
                out << "finite cyclic summands:" << endl;
                out << count_in_map<Integer, size_t>(ClassGroup);
                out << endl;
            }
        }

        if (Result->isComputed(ConeProperty::IsEmptySemiOpen)) {
            if (Result->isEmptySemiOpen()) {
                out << "Semiopen polyhedron is empty" << endl;
                out << "Covering face:" << endl;
                out << Result->getCoveringFace();
            }
            else
                out << "Semiopen polyhedron is nonempty " << endl;
            out << endl;
        }

        if (Result->isComputed(ConeProperty::IsGorenstein)) {
            if (Result->isGorenstein()) {
                out << "Monoid is Gorenstein " << endl;
                out << "Generator of interior:" << endl;
                out << Result->getGeneratorOfInterior();
            }
            else
                out << "Monoid is not Gorenstein " << endl;
            out << endl;
        }

        if (aut &&
            (Result->isComputed(ConeProperty::Automorphisms) || Result->isComputed(ConeProperty::AmbientAutomorphisms) ||
             Result->isComputed(ConeProperty::CombinatorialAutomorphisms) ||
             Result->isComputed(ConeProperty::InputAutomorphisms) || Result->isComputed(ConeProperty::RationalAutomorphisms) ||
             Result->isComputed(ConeProperty::EuclideanAutomorphisms))) {
            write_aut();
            out << Result->getAutomorphismGroup().getQualitiesString() << "automorphism group has order "
                << Result->getAutomorphismGroup().getOrder() << " (possibly approximation if very large)" << endl;

            if (Result->getAutomorphismGroup().IsIntegralityChecked()) {
                if (Result->getAutomorphismGroup().IsIntegral())
                    out << "Automorphisms are integral" << endl;
                else
                    out << "Automorphisms are not integral" << endl;
            }
            else
                out << "Integrality not known" << endl;
        }

        out << "***********************************************************************" << endl << endl;

        if (no_matrices_output) {
            out.close();
            return;
        }

        if (lattice_ideal_input) {
            out << nr_orig_gens << " original generators:" << endl;
            Result->getOriginalMonoidGeneratorsMatrix().pretty_print(out);
            out << endl;
        }
        if (Result->isComputed(ConeProperty::ModuleGenerators) && !Result->isIntHullCone() && !no_hilbert_basis_output) {
            out << Result->getNrModuleGenerators() << module_generators_name << ":" << endl;
            Result->getModuleGeneratorsMatrix().pretty_print(out);
            out << endl;
        }

        if (Result->isComputed(ConeProperty::Deg1Elements) && !no_hilbert_basis_output) {
            const Matrix<Integer>& Hom = Result->getDeg1ElementsMatrix();
            write_matrix_ht1(Hom);
            nr = Hom.nr_of_rows();
            out << nr << module_generators_name << ":" << endl;
            Hom.pretty_print(out);
            out << endl;
        }

        if (Result->isComputed(ConeProperty::HilbertBasis) && !Result->isIntHullCone() && !no_hilbert_basis_output) {
            const Matrix<Integer>& Hilbert_Basis = Result->getHilbertBasisMatrix();

            if (!Result->isComputed(ConeProperty::Deg1Elements)) {
                nr = Hilbert_Basis.nr_of_rows();
                out << nr << " Hilbert basis elements" << of_monoid << ":" << endl;
                Hilbert_Basis.pretty_print(out);
                out << endl;
            }
            else {
                nr = Hilbert_Basis.nr_of_rows() - Result->getNrDeg1Elements();
                out << nr << " further Hilbert basis elements" << of_monoid << " of higher degree:" << endl;
                Matrix<Integer> HighDeg(nr, dim);
                for (size_t i = 0; i < nr; ++i)
                    HighDeg[i] = Hilbert_Basis[i + Result->getNrDeg1Elements()];
                HighDeg.pretty_print(out);
                out << endl;
            }
            Matrix<Integer> complete_Hilbert_Basis(0, dim);
            if (gen || egn || typ) {
                // for these files we append the module generators if there are any
                if (Result->isComputed(ConeProperty::ModuleGenerators)) {
                    complete_Hilbert_Basis.append(Hilbert_Basis);
                    complete_Hilbert_Basis.append(Result->getModuleGeneratorsMatrix());
                    write_matrix_gen(complete_Hilbert_Basis);
                }
                else {
                    write_matrix_gen(Hilbert_Basis);
                }
            }
            if ((egn || typ) && Result->isComputed(ConeProperty::Sublattice)) {
                const Sublattice_Representation<Integer>& BasisChange = Result->getSublattice();
                Matrix<Integer> Hilbert_Basis_Full_Cone = BasisChange.to_sublattice(Hilbert_Basis);
                if (Result->isComputed(ConeProperty::ModuleGenerators)) {
                    Hilbert_Basis_Full_Cone.append(BasisChange.to_sublattice(Result->getModuleGeneratorsMatrix()));
                }
                if (egn) {
                    string egn_string = name + ".egn";
                    const char* egn_file = egn_string.c_str();
                    ofstream egn_out(egn_file);
                    Hilbert_Basis_Full_Cone.print(egn_out);
                    // egn_out<<"cone"<<endl;
                    egn_out.close();
                }

                if (typ && homogeneous) {
                    write_matrix_typ(Hilbert_Basis_Full_Cone.multiplication(
                        BasisChange.to_sublattice_dual(Result->getSupportHyperplanesMatrix()).transpose()));
                }
            }

            if (Result->isComputed(ConeProperty::IsReesPrimary)) {
                out << rees_ideal_key.size() << " generators of integral closure of the ideal:" << endl;
                Matrix<Integer> Ideal_Gens = Hilbert_Basis.submatrix(rees_ideal_key);
                Ideal_Gens.resize_columns(dim - 1);
                Ideal_Gens.pretty_print(out);
                out << endl;
            }
        }
        if (Result->isComputed(ConeProperty::VerticesOfPolyhedron) && !no_ext_rays_output) {
            out << Result->getNrVerticesOfPolyhedron() << " vertices of polyhedron:" << endl;
            if (Result->isComputed(ConeProperty::VerticesFloat))
                Result->getVerticesFloatMatrix().pretty_print(out);
            else
                Result->getVerticesOfPolyhedronMatrix().pretty_print(out);
            out << endl;
        }
        if (Result->isComputed(ConeProperty::ExtremeRays) && !no_ext_rays_output) {
            out << Result->getNrExtremeRays() << " extreme rays" << of_cone << ":" << endl;
            if (homogeneous &&
                (Result->isComputed(ConeProperty::VerticesFloat) || Result->isComputed(ConeProperty::VerticesFloat))) {
                if (Result->isComputed(ConeProperty::VerticesFloat))
                    Result->getVerticesFloatMatrix().pretty_print(out);
                else
                    Result->getExtremeRaysFloatMatrix().pretty_print(out);
            }
            else
                Result->getExtremeRaysMatrix().pretty_print(out);
            out << endl;
        }

        if (Result->isComputed(ConeProperty::ExtremeRays) && ext) {
            // for the .gen file we append the vertices of polyhedron if there are any
            if (Result->isComputed(ConeProperty::VerticesOfPolyhedron)) {
                Matrix<Integer> Extreme_Rays(Result->getVerticesOfPolyhedronMatrix());
                Extreme_Rays.append(Result->getExtremeRaysMatrix());
                write_matrix_ext(Extreme_Rays);
            }
            else {
                write_matrix_ext(Result->getExtremeRaysMatrix());
            }
        }

        if (Result->isComputed(ConeProperty::MaximalSubspace) && Result->getDimMaximalSubspace() > 0) {
            out << Result->getDimMaximalSubspace() << " basis elements of maximal subspace:" << endl;
            Result->getMaximalSubspaceMatrix().pretty_print(out);
            out << endl;
            if (msp)
                write_matrix_msp(Result->getMaximalSubspaceMatrix());
        }

        if (Result->isComputed(ConeProperty::ModuleGeneratorsOverOriginalMonoid)) {
            out << Result->getNrModuleGeneratorsOverOriginalMonoid() << " module generators over original monoid:" << endl;
            Result->getModuleGeneratorsOverOriginalMonoidMatrix().pretty_print(out);
            out << endl;
            if (mod)
                write_matrix_mod(Result->getModuleGeneratorsOverOriginalMonoidMatrix());
        }

        // write constrains (support hyperplanes, congruences, equations)

        if (Result->isComputed(ConeProperty::SupportHyperplanes) && !no_supp_hyps_output) {
            const Matrix<Integer>& Support_Hyperplanes = Result->getSupportHyperplanesMatrix();
            out << Support_Hyperplanes.nr_of_rows() << " support hyperplanes" << of_polyhedron << ":" << endl;
            if (Result->isComputed(ConeProperty::SuppHypsFloat))
                Result->getSuppHypsFloatMatrix().pretty_print(out);
            else
                Support_Hyperplanes.pretty_print(out);
            out << endl;
        }
        if (Result->isComputed(ConeProperty::Sublattice)) {
            const Sublattice_Representation<Integer>& BasisChange = Result->getSublattice();
            // equations
            const Matrix<Integer>& EQ = BasisChange.getEquationsMatrix();
            Matrix<Integer> Equations = EQ;
            // Equations.row_echelon_reduce();
            size_t nr_of_equ = Equations.nr_of_rows();
            if (nr_of_equ > 0) {
                out << nr_of_equ << " equations:" << endl;
                Equations.pretty_print(out);
                out << endl;
            }

            // congruences
            const Matrix<Integer>& Congruences = BasisChange.getCongruencesMatrix();
            size_t nr_of_cong = Congruences.nr_of_rows();
            if (nr_of_cong > 0) {
                out << nr_of_cong << " congruences:" << endl;
                Congruences.pretty_print(out);
                out << endl;
            }

            // lattice
            const Matrix<Integer>& LB = BasisChange.getEmbeddingMatrix();
            Matrix<Integer> LatticeBasis = LB;
            if (!using_renf<Integer>())  // superfluous in the case of numberfield
                LatticeBasis.row_echelon_reduce();
            size_t nr_of_latt = LatticeBasis.nr_of_rows();
            if (nr_of_latt < dim || BasisChange.getExternalIndex() != 1) {
                out << nr_of_latt << " basis elements of generated  " << lattice_or_space << ":" << endl;
                LatticeBasis.pretty_print(out);
                out << endl;
            }
            if (lat)
                write_matrix_lat(LatticeBasis);

            // excluded faces
            if (Result->isComputed(ConeProperty::ExcludedFaces)) {
                const Matrix<Integer>& ExFaces = Result->getExcludedFacesMatrix();
                out << ExFaces.nr_of_rows() << " excluded faces:" << endl;
                ExFaces.pretty_print(out);
                out << endl;
            }

            if (cst && Result->isComputed(ConeProperty::SupportHyperplanes)) {
                const Matrix<Integer>& Support_Hyperplanes = Result->getSupportHyperplanesMatrix();
                string cst_string = name + ".cst";
                const char* cst_file = cst_string.c_str();
                ofstream cst_out(cst_file);

                Support_Hyperplanes.print(cst_out);
                cst_out << "inequalities" << endl;
                Equations.print(cst_out);
                cst_out << "equations" << endl;
                Congruences.print(cst_out);
                cst_out << "congruences" << endl;
                if (Result->isComputed(ConeProperty::ExcludedFaces)) {
                    Result->getExcludedFacesMatrix().print(cst_out);
                    cst_out << "excluded_faces" << endl;
                }
                if (Result->isComputed(ConeProperty::Grading)) {
                    cst_out << 1 << endl << dim << endl;
                    cst_out << Result->getGrading();
                    cst_out << "grading" << endl;
                }
                if (Result->isComputed(ConeProperty::Dehomogenization)) {
                    cst_out << 1 << endl << dim << endl;
                    cst_out << Result->getDehomogenization();
                    cst_out << "dehomogenization" << endl;
                }
                cst_out.close();
            }
        }

        out.close();
    }

    write_inv_file();
    write_Stanley_dec();
}

#ifndef NMZ_MIC_OFFLOAD  // offload with long is not supported
template class Output<long>;
#endif
template class Output<long long>;
template class Output<mpz_class>;

#ifdef ENFNORMALIZ
template class Output<renf_elem_class>;
#endif

}  // namespace libnormaliz
