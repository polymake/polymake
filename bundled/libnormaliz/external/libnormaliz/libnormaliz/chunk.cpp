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

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "libnormaliz/chunk.h"
#include "libnormaliz/full_cone.h"

namespace libnormaliz {
using std::cout;
using std::endl;
using std::ifstream;

void chunk() {
    // must do this here since no cone is built
    // parameter -x taken care of by normaliz

    omp_set_max_active_levels(1);

    if (thread_limit < 0)
        throw BadInputException("Invalid thread limit");

    if (parallelization_set) {
        if (thread_limit != 0)
            omp_set_num_threads(thread_limit);
    }
    else {
        if (std::getenv("OMP_NUM_THREADS") == NULL) {
            long old = omp_get_max_threads();
            if (old > default_thread_limit)
                set_thread_limit(default_thread_limit);
            omp_set_num_threads(thread_limit);
        }
    }

    string type;

    cin >> type;
    if (type != "Project") {
        throw BadInputException("Holow tri file spoiled");
    }
    string project_name;
    cin >> project_name;

    size_t this_chunk;
    cin >> type;
    if (type != "Block") {
        throw BadInputException("Holow tri file spoiled");
    }
    cin >> this_chunk;

    string name_in = project_name + ".basic.data";
    const char* file_in = name_in.c_str();
    ifstream in;
    in.open(file_in, ifstream::in);
    if (in.is_open() == false) {
        throw BadInputException("Cannot find basic.data");
    }

    in >> type;
    if (type != "Project")
        throw BadInputException("basic.data spoiled");
    string name_now;
    in >> name_now;
    if (name_now != project_name)
        throw BadInputException("basic.data spoiled");

    in >> type;
    if (type != "Dim")
        throw BadInputException("basic.data spoiled");
    size_t dim;
    in >> dim;

    in >> type;
    if (type != "Gen")
        throw BadInputException("basic.data spoiled");
    size_t nr_gen;
    in >> nr_gen;

    Matrix<mpz_class> Generators(nr_gen, dim);
    for (size_t i = 0; i < nr_gen; ++i) {
        for (size_t j = 0; j < dim; ++j)
            in >> Generators[i][j];
    }
    cout << "Generators" << endl;
    Generators.pretty_print(cout);

    in >> type;
    if (type != "Grad")
        throw BadInputException("basic.data spoiled");
    vector<mpz_class> GradingOnPrimal(dim);
    for (size_t j = 0; j < dim; ++j)
        in >> GradingOnPrimal[j];
    cout << "GradingOnPrimal" << endl;
    cout << GradingOnPrimal;

    in >> type;
    if (type != "Generic")
        throw BadInputException("basic.data spoiled");
    vector<mpz_class> Generic(dim);
    for (size_t j = 0; j < dim; ++j)
        in >> Generic[j];

    cout << "Generic" << endl;
    cout << Generic;

    in >> type;
    if (type != "Blocks")
        throw BadInputException("basic.data spoiled");
    size_t nr_blocks, dummy;
    in >> nr_blocks;
    cout << "Blocks " << nr_blocks << endl;
    vector<size_t> block_start(nr_blocks), block_end(nr_blocks);
    for (size_t i = 0; i < nr_blocks; ++i) {
        in >> dummy;
        if (dummy != i)
            throw BadInputException("basic.data spoiled");
        in >> block_start[i] >> block_end[i];
        cout << i << "  " << block_start[i] << "  " << block_end[i] << endl;
    }
    if (this_chunk >= nr_blocks)
        throw BadInputException("basic.data spoiled");
    cout << "This chunk " << this_chunk << endl;

    vector<pair<dynamic_bitset, dynamic_bitset> > Triangulation_ind(block_end[this_chunk] - block_start[this_chunk]);

    string input_string;
    for (size_t i = 0; i < Triangulation_ind.size(); ++i) {
        cin >> input_string;
        Triangulation_ind[i].first.resize(nr_gen);
        for (size_t j = 0; j < input_string.size(); ++j) {
            if (input_string[j] == '1')
                Triangulation_ind[i].first[nr_gen - 1 - j] = 1;
        }
        cin >> input_string;
        Triangulation_ind[i].second.resize(nr_gen);
        for (size_t j = 0; j < input_string.size(); ++j) {
            if (input_string[j] == '1')
                Triangulation_ind[i].second[nr_gen - 1 - j] = 1;
        }
    }

    // cout << Triangulation_ind.back().first << endl;

    cin >> type;
    if (type != "End")
        throw BadInputException("basic.data or hollow tri file spoiled");

    int omp_start_level = omp_get_level();

    SignedDec<mpz_class> SDMult(Triangulation_ind, Generators, GradingOnPrimal, omp_start_level);
    SDMult.verbose = true;
    SDMult.approximate = true;
    SDMult.decimal_digits = 100;
    SDMult.Generic = Generic;
    if (!SDMult.ComputeMultiplicity())
        assert(false);

    mpq_class multiplicity = SDMult.multiplicity;

    mpz_class corr_factor = v_gcd(GradingOnPrimal);  // search in code for corr_factor to find an explanation
    multiplicity *= corr_factor;

    cout << "Multiplicity" << endl;
    cout << multiplicity << endl;
    cout << "Mult (float) " << std::setprecision(12) << mpq_to_nmz_float(multiplicity) << endl;

    string file_name = project_name + ".mult.";
    file_name += to_string(this_chunk);
    ofstream out(file_name.c_str());
    out << "multiplicity " << this_chunk << endl << endl;
    out << multiplicity << endl;
    out.close();
}

void add_chunks(const string& project) {
    size_t nr_blocks;

    string name_in = project + ".basic.data";
    const char* file_in = name_in.c_str();
    ifstream in;
    in.open(file_in, ifstream::in);
    if (in.is_open() == false) {
        throw BadInputException("Cannot find basic.data");
    }
    string type;

    while (true) {
        in >> type;
        if (type != "Blocks")
            continue;
        in >> nr_blocks;
        break;
    }

    in.close();

    mpq_class total_mult = 0;

    cout << "Summing " << nr_blocks << " partial multiplicities" << endl;
    for (size_t i = 0; i < nr_blocks; ++i) {
        cout << "Reading block " << i << endl;
        string name_in = project + ".mult." + to_string(i);
        const char* file_in = name_in.c_str();
        ifstream in;
        in.open(file_in, ifstream::in);
        string type;
        in >> type;
        if (type != "multiplicity") {
            throw BadInputException("spoiled mult " + to_string(i));
        }
        size_t this_chunk;
        in >> this_chunk;
        if (i != this_chunk) {
            throw BadInputException("spoiled mult " + to_string(i));
        }
        mpq_class mult;
        in >> mult;
        total_mult += mult;
    }
    cout << "Toatl miultiplicity" << endl;
    cout << total_mult << endl;
    cout << "Toatl miultiplicity (float) " << std::setprecision(12) << mpq_to_nmz_float(total_mult) << endl;

    string file_name = project + ".total.mult";
    ofstream out(file_name.c_str());
    out << "total multiplicity " << total_mult << endl << endl;
    out << "toatl miultiplicity (float) " << std::setprecision(12) << mpq_to_nmz_float(total_mult) << endl;
    out.close();
}

}  // namespace libnormaliz
