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

#include "libnormaliz/options.h"

#include <sstream>
#include <fstream>

namespace libnormaliz {
using std::cout;
using std::endl;
using std::ifstream;

static void printCopying() {
    cout << "Copyright (C) 2007-2022  The Normaliz Team, University of Osnabrueck." << endl
         << "This program comes with ABSOLUTELY NO WARRANTY; This is free software," << endl
         << "and you are welcome to redistribute it under certain conditions;" << endl
         << "See COPYING for details." << endl;
}

static void printVersion() {
    cout << "Normaliz " << string(STRINGIFY(NMZ_VERSION)) << endl;
    string optional_packages = package_string();
    if (optional_packages.size() > 0) {
        cout << "------------------------------------------------------------" << endl;
        cout << "with package(s)" << optional_packages << endl;
    }
    printCopying();
}

static string pureName(const string& fullName) {
    // extracts the pure filename

    string slash = "/";
    string back_slash = "\\";

    size_t found = fullName.rfind(slash);
    if (found == std::string::npos) {
        found = fullName.rfind(back_slash);
        if (found == std::string::npos)
            return (fullName);
    }
    found++;
    size_t length = fullName.size() - found;
    return (fullName.substr(found, length));
}

void OptionsHandler::setProjectName(const string& s) {
    if (project_name_set) {
        cerr << "Error: Second project name " << s << " in command line!" << endl;
        exit(1);
    }
    project_name = s;
    // check if we can read the .in file
    string name_in = project_name + ".in";
    const char* file_in = name_in.c_str();
    ifstream in2;
    in2.open(file_in, ifstream::in);
    if (in2.is_open() == false) {
        // check if user added ".in" and ignore it in this case
        string suffix(".in");
        size_t found = project_name.rfind(suffix);
        if (found != string::npos) {
            project_name.erase(found);
        }
    }
    else {
        in2.close();
    }
    project_name_set = true;
}

bool OptionsHandler::handle_commandline(int argc, char* argv[]) {
    vector<string> LongOptions;
    string ShortOptions;  // all options concatenated (including -)
    // read command line options
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] != '\0') {
                if (argv[i][1] != 'x') {
                    if (argv[i][1] == '-') {
                        string LO = argv[i];
                        LO.erase(0, 2);
                        LongOptions.push_back(LO);
                    }
                    else
                        ShortOptions = ShortOptions + argv[i];
                }
                else if (argv[i][2] == '=') {
#ifdef _OPENMP
                    string Threads = argv[i];
                    Threads.erase(0, 3);
                    if ((istringstream(Threads) >> nr_threads) && nr_threads >= 0) {
                        set_thread_limit(nr_threads);
                        // omp_set_num_threads(nr_threads); -- now in cone.cpp
                    }
                    else {
                        cerr << "Error: Invalid option string " << argv[i] << endl;
                        exit(1);
                    }
#else
                    cerr << "Warning: Compiled without OpenMP support, option " << argv[i] << " ignored." << endl;
#endif
                }
                else {
                    cerr << "Error: Invalid option string " << argv[i] << endl;
                    exit(1);
                }
            }
        }
        else {
            setProjectName(argv[i]);
        }
    }
    return handle_options(LongOptions, ShortOptions);
}

void OptionsHandler::setOutputDirName(const string& s) {
    if (s.size() == 0)
        throw BadInputException("Empty output directory name");
    output_dir = s;
    char slash = '/';
    char back_slash = '\\';
    if (output_dir[output_dir.size() - 1] != slash && output_dir[output_dir.size() - 1] != back_slash)
        output_dir += slash;
    output_dir_set = true;
}

bool OptionsHandler::handle_options(vector<string>& LongOptions, string& ShortOptions) {
    // Analyzing short command line options
    for (size_t i = 1; i < ShortOptions.size(); i++) {
        switch (ShortOptions[i]) {
            case '-':
                break;
            case 'c':
                verbose = true;
                break;
            case 'f':
                write_extra_files = true;
                break;
            case 'a':
                write_all_files = true;
                break;
            case 'T':
                to_compute.set(ConeProperty::Triangulation);
                // to_compute.set(ConeProperty::Multiplicity);
                break;
            case 'F':
                to_compute.set(ConeProperty::Descent);
                break;
            case 's':
                to_compute.set(ConeProperty::SupportHyperplanes);
                break;
            case 'S':
                to_compute.set(ConeProperty::Sublattice);
                break;
            case 't':
                to_compute.set(ConeProperty::TriangulationSize);
                break;
            case 'v':
                to_compute.set(ConeProperty::Multiplicity);
                break;
            case 'V':
                to_compute.set(ConeProperty::Volume);
                break;
            case 'n':
                to_compute.set(ConeProperty::HilbertBasis);
                to_compute.set(ConeProperty::Multiplicity);
                break;
            case 'N':
                to_compute.set(ConeProperty::HilbertBasis);
                break;
            case 'w':
                to_compute.set(ConeProperty::WitnessNotIntegrallyClosed);
                break;
            case '1':
                to_compute.set(ConeProperty::Deg1Elements);
                break;
            case 'q':
                to_compute.set(ConeProperty::HilbertSeries);
                break;
            case 'p':
                to_compute.set(ConeProperty::HilbertSeries);
                to_compute.set(ConeProperty::Deg1Elements);
                break;
            case 'h':
                to_compute.set(ConeProperty::HilbertBasis);
                to_compute.set(ConeProperty::HilbertSeries);
                break;
            case 'y':
                to_compute.set(ConeProperty::StanleyDec);
                break;
            case 'd':
                to_compute.set(ConeProperty::DualMode);
                break;
            case 'r':
                to_compute.set(ConeProperty::Approximate);
                break;
            case 'e':  // check for arithmetic overflow
                // test_arithmetic_overflow=true;
                cerr << "WARNING: deprecated option -e is ignored." << endl;
                break;
            case 'B':                                  // use Big Integer
                to_compute.set(ConeProperty::BigInt);  // use_Big_Integer=true;
                break;
            case 'b':  // use the bottom decomposition for the triangulation
                to_compute.set(ConeProperty::BottomDecomposition);
                break;
            case 'C':  // compute the class group
                to_compute.set(ConeProperty::ClassGroup);
                break;
            case 'k':  // keep the order of the generators in Full_Cone
                to_compute.set(ConeProperty::KeepOrder);
                break;
            case 'o':  // suppress bottom decomposition in Full_Cone
                to_compute.set(ConeProperty::NoBottomDec);
                break;
            case 'M':  // compute minimal system of generators of integral closure
                       // as a module over original monoid
                to_compute.set(ConeProperty::ModuleGeneratorsOverOriginalMonoid);
                break;
            case '?':  // print help text and exit
                return true;
                break;
            case 'x':  // should be separated from other options
                cerr << "Error: Option -x=<T> has to be separated from other options" << endl;
                exit(1);
                break;
            case 'I':
                to_compute.set(ConeProperty::Integral);
                break;
            case 'L':
                to_compute.set(ConeProperty::VirtualMultiplicity);
                break;
            case 'E':
                to_compute.set(ConeProperty::WeightedEhrhartSeries);
                break;
            case 'i':
                ignoreInFileOpt = true;
                break;
            case 'H':
                to_compute.set(ConeProperty::IntegerHull);
                break;
            case 'D':
                to_compute.set(ConeProperty::ConeDecomposition);
                break;
            case 'P':
                to_compute.set(ConeProperty::PrimalMode);
                break;
            case 'Y':
                to_compute.set(ConeProperty::Symmetrize);
                break;
            case 'X':
                to_compute.set(ConeProperty::NoSymmetrization);
                break;
            case 'G':
                to_compute.set(ConeProperty::IsGorenstein);
                break;
            case 'j':
                to_compute.set(ConeProperty::Projection);
                break;
            case 'J':
                to_compute.set(ConeProperty::ProjectionFloat);
                break;
            default:
                cerr << "Error: Unknown option -" << ShortOptions[i] << endl;
                exit(1);
                break;
        }
    }

    // Remember to update also the --help text and the documentation when changing this!
    vector<string> AdmissibleOut;
    string AdmissibleOutarray[] = {"gen", "cst", "inv", "ext", "ht1", "esp",
                                   "egn", "typ", "lat", "msp", "mod"};  // "mod" must be last
    for (const auto& i : AdmissibleOutarray)
        AdmissibleOut.push_back(i);
    assert(AdmissibleOut.back() == "mod");

    // analyzing long options
    for (const auto& LongOption : LongOptions) {
        size_t j;
        for (j = 0; j < LongOption.size(); ++j) {
            if (LongOption[j] == '=')
                break;
        }
        if (j < LongOption.size()) {
            string OptName = LongOption.substr(0, j);
            string OptValue = LongOption.substr(j + 1, LongOption.size() - 1);
            if (OptName == "OutputDir") {
                setOutputDirName(OptValue);
                continue;
            }
        }
        if (LongOption == "help") {
            return true;  // indicate printing of help text
        }
        if (LongOption == "verbose") {
            verbose = true;
            continue;
        }
        if (LongOption == "version") {
            printVersion();
            exit(0);
        }
        /* if(LongOptions[i]=="BigInt"){
            use_Big_Integer=true;
            continue;
        }*/
        if (LongOption == "LongLong") {
            use_long_long = true;
            continue;
        }
        if (LongOption == "Chunk") {
            use_chunk = true;
            continue;
        }
        if (LongOption == "AddChunks") {
            use_add_chunks = true;
            continue;
        }
        if (LongOption == "NoExtRaysOutput") {
            no_ext_rays_output = true;
            continue;
        }
        if (LongOption == "NoSuppHypsOutput") {
            no_supp_hyps_output = true;
            continue;
        }
        if (LongOption == "NoMatricesOutput") {
            no_matrices_output = true;
            continue;
        }
        if (LongOption == "NoHilbertBasisOutput") {
            no_hilbert_basis_output = true;
            continue;
        }
        if (LongOption == "ignore") {
            ignoreInFileOpt = true;
            continue;
        }
        if (LongOption == "files") {
            write_extra_files = true;
            continue;
        }
        if (LongOption == "all-files") {
            write_all_files = true;
            continue;
        }
        if (find(AdmissibleOut.begin(), AdmissibleOut.end(), LongOption) != AdmissibleOut.end()) {
            OutFiles.push_back(LongOption);
            continue;
        }
        try {
            to_compute.set(toConeProperty(LongOption));
            continue;
        } catch (const BadInputException&) {
        };
        cerr << "Error: Unknown option --" << LongOption << endl;
        exit(1);
    }

    if (output_dir_set) {
        output_file = output_dir + pureName(project_name);
    }
    else
        output_file = project_name;

    return false;  // no need to print help text
}

bool OptionsHandler::activateDefaultMode() {
    if (to_compute.goals().none() && !to_compute.test(ConeProperty::DualMode)) {
        to_compute.set(ConeProperty::DefaultMode);
        return true;
    }
    return false;
}

}  // namespace libnormaliz
