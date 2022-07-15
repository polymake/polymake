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
#ifndef LIBNORMALIZ_OUTPUT_H
#define LIBNORMALIZ_OUTPUT_H
//---------------------------------------------------------------------------

#include "libnormaliz/cone.h"

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------

template <typename Number>
class Output {
    string name;
    bool out;
    bool inv;
    bool ext;
    bool esp;
    bool typ;
    bool egn;
    bool gen;
    bool cst;
    bool tri;
    bool aut;
    bool tgn;
    bool ht1;
    bool dec;
    bool lat;
    bool precomp;
    bool mod;
    bool msp;
    bool fac;
    bool inc;
    Cone<Number>* Result;
    size_t dim;
    bool homogeneous;
    bool print_renf;
    string of_cone;
    string of_monoid;
    string monoid_or_cone;
    string lattice_or_space;
    string of_polyhedron;
    string module_generators_name;
    // string HilbertOrEhrhart;

    bool lattice_ideal_input;

    bool no_ext_rays_output;
    bool no_supp_hyps_output;
    bool no_hilbert_basis_output;
    bool no_matrices_output;

#ifdef ENFNORMALIZ
    renf_class_shared Renf;
#endif

    //---------------------------------------------------------------------------
   public:
    //---------------------------------------------------------------------------
    //                        Construction and destruction
    //---------------------------------------------------------------------------

    Output();  // main constructor
    // default copy constructor and destructors are ok
    // the Cone Object is handled at another place

    //---------------------------------------------------------------------------
    //                                Data acces
    //---------------------------------------------------------------------------

    void set_name(const string& n);
    void setCone(Cone<Number>& C);

    void set_write_out(const bool& flag);      // sets the write .out flag
    void set_write_inv(const bool& flag);      // sets the write .inv flag
    void set_write_ext(const bool& flag);      // sets the write .ext flag
    void set_write_esp(const bool& flag);      // sets the write .esp flag
    void set_write_typ(const bool& flag);      // sets the write .typ flag
    void set_write_egn(const bool& flag);      // sets the write .egn flag
    void set_write_gen(const bool& flag);      // sets the write .gen flag
    void set_write_cst(const bool& flag);      // sets the write .cst flag
    void set_write_tri(const bool& flag);      // sets the write .tri flag
    void set_write_aut(const bool& flag);      // sets the write .aut flag
    void set_write_tgn(const bool& flag);      // sets the write .tgn flag
    void set_write_ht1(const bool& flag);      // sets the write .ht1 flag
    void set_write_dec(const bool& flag);      // sets the write .dec flag
    void set_write_lat(const bool& flag);      // sets the write .lat flag
    void set_write_precomp(const bool& flag);  // sets the write .precomp flag
    void set_write_mod(const bool& flag);      // sets the write .mod flag
    void set_write_msp(const bool& flag);      // sets the write .msp flag
    void set_write_fac(const bool& flag);      // sets the write .fac flag
    void set_write_inc(const bool& flag);      // sets the write .inc flag
    void set_write_extra_files();              // sets some flags to true
    void set_write_all_files();                // sets most flags to true

    void write_matrix_ext(const Matrix<Number>& M) const;  // writes M to file name.ext
    void write_matrix_lat(const Matrix<Number>& M) const;  // writes M to file name.lat
    void write_matrix_esp(const Matrix<Number>& M) const;  // writes M to file name.esp
    void write_matrix_typ(const Matrix<Number>& M) const;  // writes M to file name.typ
    void write_matrix_egn(const Matrix<Number>& M) const;  // writes M to file name.egn
    void write_matrix_gen(const Matrix<Number>& M) const;  // writes M to file name.gen
    void write_matrix_mod(const Matrix<Number>& M) const;  // writes M to file name.mod
    void write_matrix_msp(const Matrix<Number>& M) const;  // writes M to file name.msp
    void write_precomp() const;
    void write_tri() const;                                               // writes the .tri file
    void write_aut() const;                                               // writes the .aut file
    void write_aut_ambient(ofstream& out, const string& gen_name) const;  // ... in a special case
    void write_fac() const;                                               // writes the .fac file
    void write_dual_fac() const;                                          // writes the .fac file with duual face lattice
    void write_inc() const;                                               // writes the .inc file
    void write_dual_inc() const;                                          // writes the .inc file with dual incidence

    void write_Stanley_dec() const;
    void write_matrix_ht1(const Matrix<Number>& M) const;  // writes M to file name.ht1

    void write_float(ofstream& out, const Matrix<nmz_float>& mat, size_t nr, size_t nc) const;

    void write_inv_file() const;

    void set_lattice_ideal_input(bool lattice_odeal_input);

    void set_renf(const renf_class_shared renf, bool is_int_hull = false);
    /*
    // #ifdef ENFNORMALIZ
        void set_renf(renf_class *renf,bool is_int_hull=false);
    // #endif
    */
    void write_renf(ostream& os) const;  // prints the real embedded number field if present

    void set_no_ext_rays_output();
    void set_no_supp_hyps_output();
    void set_no_hilbert_basis_output();
    void set_no_matrices_output();

    //---------------------------------------------------------------------------
    //                         Output Algorithms
    //---------------------------------------------------------------------------

    void write_files() const;
    void writeWeightedEhrhartSeries(ofstream& out) const;
    void writeSeries(ofstream& out, const HilbertSeries& HS, string HilbertOrEhrhart) const;

    void write_perms_and_orbits(ofstream& out,
                                const vector<vector<key_t> >& Perms,
                                const vector<vector<key_t> >& Orbits,
                                const string& type_string) const;
};
// class end *****************************************************************

}  // namespace libnormaliz

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
