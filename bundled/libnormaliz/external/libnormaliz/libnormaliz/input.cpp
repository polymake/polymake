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

#include <iostream>
#include <cctype>  // std::isdigit
#include <limits>  // numeric_limits

// #include <boost/smart_ptr/intrusive_ptr.hpp>

#include "libnormaliz/input.h"
#include "libnormaliz/list_and_map_operations.h"

namespace libnormaliz {

//---------------------------------------------------------------------------
//                     Number input
//---------------------------------------------------------------------------


#ifdef ENFNORMALIZ

static int xalloc = std::ios_base::xalloc();

// Normaliz implementation of deprecated e-antic functions

std::istream & nmz_set_pword(boost::intrusive_ptr<const renf_class> our_renf, std::istream & is)
{
    is.pword(xalloc) = const_cast<void*>(reinterpret_cast<const void*>(&*our_renf));
    return is;
}


boost::intrusive_ptr<const renf_class> nmz_get_pword(std::istream& is) {
    return reinterpret_cast<renf_class*>(is.pword(xalloc));
}
#endif


// To be used in input.cpp
inline void string2coeff(mpq_class& coeff, istream& in, const string& s) {  // in here superfluous parameter

    stringstream sin(s);
    coeff = mpq_read(sin);
    // coeff=mpq_class(s);
}

inline void read_number(istream& in, mpq_class& number) {
    number = mpq_read(in);
}

inline void read_number(istream& in, long& number) {
    in >> number;
}

inline void read_number(istream& in, long long& number) {
    in >> number;
}

inline void read_number(istream& in, nmz_float& number) {
    in >> number;
}

inline void read_number(istream& in, mpz_class& number) {
    in >> number;
}

#ifdef ENFNORMALIZ

inline void string2coeff(renf_elem_class& coeff, istream& in, const string& s) {  // we need in to access the renf

    try {
        coeff = renf_elem_class(*nmz_get_pword(in), s);
        // coeff = renf_elem_class(*renf_class::get_pword(in), s);
    } catch (const std::exception& e) {
        cerr << e.what() << endl;
        throw BadInputException("Illegal number string " + s + " in input, Exiting.");
    }
}

inline void read_number(istream& in, renf_elem_class& number) {
    // in >> number;

    char c;

    in >> ws;
    c = in.peek();
    if (c != '(' && c != '\'' && c != '\"') {  // rational number
        mpq_class rat = mpq_read(in);
        number = renf_elem_class(rat);
        return;
    }

    // now we have a proper field element

    in >> c;  // read (

    string num_string;
    bool skip = false;
    while (in.good()) {
        c = in.peek();
        if (c == ')' || c == '\'' || c == '\"') {
            in >> c;
            break;
        }
        if (c == '~' || c == '=' || c == '[')  // skip the approximation
            skip = true;
        in.get(c);
        if (in.fail())
            throw BadInputException("Error in reading number: field element not terminated");
        if (!skip)
            num_string += c;
    }
    string2coeff(number, in, num_string);
}
#endif

// matrix input

template <typename Integer>
Matrix<Integer> readMatrix(const string project) {
    // reads one matrix from file with name project
    // format: nr of rows, nr of colimns, entries
    // all separated by white space

    string name_in = project;
    const char* file_in = name_in.c_str();
    ifstream in;
    in.open(file_in, ifstream::in);
    if (in.is_open() == false)
        throw BadInputException("readMatrix cannot find file");
    int nrows, ncols;
    in >> nrows;
    in >> ncols;

    if (nrows == 0 || ncols == 0)
        throw BadInputException("readMatrix finds matrix empty");

    int i, j;
    Matrix<Integer> result(nrows, ncols);

    for (i = 0; i < nrows; ++i)
        for (j = 0; j < ncols; ++j) {
            read_number(in, result[i][j]);
            if (in.fail())
                throw BadInputException("readMatrix finds matrix corrupted");
        }
    return result;
}

//--------------------------------------------------------------------------

// eats up a comment, stream must start with "/*", eats everything until "*/"
void skip_comment(istream& in) {
    int i = in.get();
    int j = in.get();
    if (i != '/' || j != '*') {
        throw BadInputException("Bad comment start!");
    }
    while (in.good()) {
        in.ignore(numeric_limits<streamsize>::max(), '*');  // ignore everything until next '*'
        i = in.get();
        if (in.good() && i == '/')
            return;  // successfully skipped comment
    }
    throw BadInputException("Incomplete comment!");
}

template <typename Number>
void save_matrix(map<Type::InputType, Matrix<Number> >& input_map,
                 InputType input_type,
                 const Matrix<Number>& M) {
    // check if this type already exists
    if (contains(input_map, input_type)) {
        /*throw BadInputException("Multiple inputs of type \"" + type_string
                + "\" are not allowed!");*/
        input_map[input_type].append(M);
        return;
    }
    input_map[input_type] = M;
}

template <typename Number>
void save_empty_matrix(map<Type::InputType, Matrix<Number> >& input_map, InputType input_type) {
    Matrix<Number> M;
    save_matrix(input_map, input_type, M);
}
 /*
template <typename Number>
Matrix<Number> transpose_mat(const Matrix<Number>& mat) {
    if (mat.nr_of_rows() == 0 || mat[0].size() == 0)
        return Matrix<Number>(0);
    size_t m = mat[0].size();
    size_t n = mat.nr_of_rows()
    Matrix<Number> transpose(m, vector<Number>(n, 0));
    for (size_t i = 0; i < m; ++i)
        for (size_t j = 0; j < n; ++j)
            transpose[i][j] = mat[j][i];
    return transpose;
}
*/

template <typename Number>
void append_row(const vector<Number> row, map<Type::InputType, Matrix<Number> >& input_map, Type::InputType input_type) {
    Matrix<Number> one_row(row);
    save_matrix(input_map, input_type, one_row);
}

template <typename Number>
void process_constraint(const string& rel,
                        const vector<Number>& left,
                        Number right,
                        const Number modulus,
                        map<Type::InputType, Matrix<Number> >& input_map,
                        bool forced_hom) {
    vector<Number> row = left;
    bool inhomogeneous = false;
    if (right != 0 || rel == "<" || rel == ">")
        inhomogeneous = true;
    string modified_rel = rel;
    bool strict_inequality = false;
    if (rel == "<") {
        strict_inequality = true;
        right -= 1;
        modified_rel = "<=";
    }
    if (rel == ">") {
        strict_inequality = true;
        right += 1;
        modified_rel = ">=";
    }
    if (strict_inequality && forced_hom) {
        throw BadInputException("Strict inequality not allowed in hom_constraints!");
    }
    if (strict_inequality && using_renf<Number>()) {
        throw BadInputException("Strict inequality not allowed for algebraic polyhedra!");
    }
    if (inhomogeneous || forced_hom)
        row.push_back(-right);   // rhs --> lhs
    if (modified_rel == "<=") {  // convert <= to >=
        for (size_t j = 0; j < row.size(); ++j)
            row[j] = -row[j];
        modified_rel = ">=";
    }
    if (rel == "~") {
        if (using_renf<Number>())
            throw BadInputException("Congruence not allowed for algebraic polyhedra");
        row.push_back(modulus);
    }

    if (inhomogeneous && !forced_hom) {
        if (modified_rel == "=") {
            append_row(row, input_map, Type::inhom_equations);
            return;
        }
        if (modified_rel == ">=") {
            append_row(row, input_map, Type::inhom_inequalities);
            return;
        }
        if (modified_rel == "~") {
            append_row(row, input_map, Type::inhom_congruences);
            return;
        }
    }
    else {
        if (modified_rel == "=") {
            append_row(row, input_map, Type::equations);
            return;
        }
        if (modified_rel == ">=") {
            append_row(row, input_map, Type::inequalities);
            return;
        }
        if (modified_rel == "~") {
            append_row(row, input_map, Type::congruences);
            return;
        }
    }
    throw BadInputException("Illegal constrint type " + rel + " !");
}

template <typename Number>
bool read_modulus(istream& in, Number& modulus) {
    if (using_renf<Number>())
        throw BadInputException("Congruence not allowed for field coefficients");
    in >> std::ws;  // gobble any leading white space
    char dummy;
    in >> dummy;
    if (dummy != '(')
        return false;
    in >> modulus;
    if (in.fail() || modulus == 0)
        return false;
    in >> std::ws;  // gobble any white space before closing
    in >> dummy;
    if (dummy != ')')
        return false;
    return true;
}

void check_modulus(mpq_class& modulus) {
    if (modulus <= 0 || modulus.get_den() != 1)
        throw BadInputException("Error in modulus of congruence");
}

#ifdef ENFNORMALIZ
void check_modulus(renf_elem_class& modulus) {
    throw BadInputException("Congruences not allowed for algebraic polyhedra");
}
#endif

template <typename Number>
void read_symbolic_constraint(istream& in, string& rel, vector<Number>& left, Number& right, Number& modulus, bool forced_hom) {
    string constraint;

    while (in.good()) {
        char c;
        c = in.get();
        if (in.fail())
            throw BadInputException("Symbolic constraint does not end with semicolon");
        if (c == ';')
            break;
        constraint += c;
    }

    // remove white space
    // we must take care that the removal of white space does not
    // shadow syntax errors
    string without_spaces;
    bool digit_then_spaces = false;
    bool has_content = false;
    for (size_t j = 0; j < constraint.size(); ++j) {
        char test = constraint[j];
        if (!isspace(test))
            has_content = true;
        if (isspace(test))
            continue;
        if (test == '.') {
            if (j == constraint.size() - 1 || isspace(constraint[j + 1]))
                throw BadInputException("Incomplete number");
        }
        if (test == 'e') {
            if (j == constraint.size() - 1 || isspace(constraint[j + 1]))
                throw BadInputException("Incomplete number");
            if (j <= constraint.size() - 3 && (constraint[j + 1] == '+' || constraint[j + 1] == '-') &&
                isspace(constraint[j + 2]))
                throw BadInputException("Incomplete number");
        }
        if (!isdigit(test))
            digit_then_spaces = false;
        else {
            if (digit_then_spaces)
                throw BadInputException("Incomplete number");
            // cout << "jjjj " << j << " |" << constraint[j+1] << "|" << endl;
            if (j < constraint.size() - 1 && isspace(constraint[j + 1])) {
                digit_then_spaces = true;
                // cout << "Drin" << endl;
            }
        }
        without_spaces += test;
    }
    if (!has_content)
        throw BadInputException("Empty symbolic constraint");

    // split into terms
    // we separate by + and -
    // except: first on lhs or rhs, between ( and ) and following e.
    bool first_sign = true;
    bool in_brackets = false;
    bool relation_read = false;
    size_t RHS_start = 0;
    vector<string> terms;
    string current_term;
    for (size_t j = 0; j < without_spaces.size(); ++j) {
        char test = without_spaces[j];
        if (test == '(')
            in_brackets = true;
        if (test == ')') {
            if (!in_brackets)
                throw BadInputException("Closing bracket without opening bracket");
            in_brackets = false;
        }
        if (test == '+' || test == '-') {
            if (!first_sign && !in_brackets) {
                terms.push_back(current_term);
                current_term.clear();
            }
        }
        first_sign = false;

        if (test == 'e') {
            current_term += test;
            if (j == without_spaces.size() - 1)
                throw BadInputException("Incomplete number");
            if (without_spaces[j + 1] == '+' || without_spaces[j + 1] == '-') {
                current_term += without_spaces[j + 1];
                j++;
            }
            continue;
        }

        if (test == '=' || test == '<' || test == '>' || test == '~') {
            terms.push_back(current_term);
            current_term.clear();
            rel += test;
            RHS_start = terms.size();
            if (relation_read)
                throw BadInputException("Double relation in constraint");
            relation_read = true;
            if (j == without_spaces.size() - 1)
                throw BadInputException("Relation last character in constraint");
            if (without_spaces[j + 1] == '=') {
                rel += without_spaces[j + 1];
                j++;
            }
            first_sign = true;
            continue;
        }

        current_term += test;
    }
    terms.push_back(current_term);
    if (!relation_read)
        throw BadInputException("No relation in constraint");

    // for(size_t i=0;i<terms.size();++i)
    //   cout << i << ": " << terms[i] << "| " << terms[i].size() << endl;

    // now we split off the modulus if necessary
    if (rel == "~") {
        if (using_renf<Number>())
            throw BadInputException("Congruence not allowed for algebraic polyhedra");
        string last_term = terms.back();
        size_t last_bracket_at = 0;
        bool has_bracket = false;
        for (size_t i = 0; i < last_term.size(); ++i) {
            if (last_term[i] == '(') {
                last_bracket_at = i;
                has_bracket = true;
            }
        }
        if (!has_bracket || last_term.back() != ')')
            throw BadInputException("Error in modulus of congruence");
        string modulus_string = last_term.substr(last_bracket_at + 1, last_term.size() - last_bracket_at - 2);
        terms.back() = last_term.substr(0, last_bracket_at);
        if (terms.back() == "")
            terms.pop_back();
        modulus = mpq_class(modulus_string);
        // modulus.canonicalize();
        // cout << "mod " << modulus << endl;
        check_modulus(modulus);
    }

    // for(size_t i=0;i<terms.size();++i)
    //     cout << i << ": " << terms[i] << "| " << terms[i].size() << endl;

    // now we must process the terns

    right = 0;
    mpq_class side = 1;

    for (size_t i = 0; i < terms.size(); ++i) {
        if (i == RHS_start)
            side = -1;

        Number sign = 1;

        string& this_term = terms[i];

        if (this_term == "")
            throw BadInputException("Empty term in symbolic constraint");

        if (this_term[0] == '+')              // we must remove leading signs for the input operator of renf_class_elem
            this_term = this_term.substr(1);  // also for mpq_class a+ is not allowed
        else {
            if (this_term[0] == '-') {
                this_term = this_term.substr(1);
                sign = -1;
            }
        }

        if (this_term == "+" || this_term == "-" || this_term == "")
            throw BadInputException("Double sign or incomplete number");
        size_t coeff_length = 0;
        for (char j : this_term) {
            if (j != 'x')
                coeff_length++;
            else
                break;
        }
        string coeff_string = this_term.substr(0, coeff_length);
        string comp_string = this_term.substr(coeff_length, this_term.size() - coeff_length);
        Number coeff = 0;
        if (coeff_length == 0 || (coeff_length == 1 && coeff_string[0] == '+'))
            coeff = 1;
        if (coeff_length == 1 && coeff_string[0] == '-')
            coeff = -1;
        if (coeff == 0) {
            // cout << i << " coeff string: " << coeff_string << endl;
            const string numeric = "+-0123456789/a^*().e";
            for (char j : coeff_string) {
                size_t pos = numeric.find(j);
                if (pos == string::npos)
                    throw BadInputException("Illegal character in number");
            }
            if (coeff_string[0] == '(') {  // remove ( and ) for renf elements
                if (coeff_string[coeff_string.size() - 1] != ')')
                    throw BadInputException("number field element not terminated by )");
                coeff_string = coeff_string.substr(1, coeff_string.size() - 2);
            }
            string2coeff(coeff, in, coeff_string);
        }

        if (comp_string != "") {
            bool bracket_read = false;
            string expo_string;
            for (char j : comp_string) {
                if (j == ']')
                    break;
                if (j == '[') {
                    bracket_read = true;
                    continue;
                }
                if (bracket_read)
                    expo_string += j;
            }
            if (expo_string.size() != comp_string.size() - 3)
                throw BadInputException("Error in naming variable in symbolic constraint");

            long index = stol(expo_string);
            if (index < 1 || index > (long)left.size())
                throw BadInputException("Index " + expo_string + " in symbolic constraint out of bounds");
            index--;
            left[index] += side * sign * coeff;
        }
        else {  // absolute term
            right -= side * sign * coeff;
        }
    }

    // cout << "constraint " << left << rel << " " << right << endl;
}

template <typename Number>
void read_constraints(istream& in, long dim, map<Type::InputType, Matrix<Number> >& input_map, bool forced_hom) {
    long nr_constraints;
    in >> nr_constraints;

    if (in.fail() || nr_constraints < 0) {
        throw BadInputException("Cannot read " + toString(nr_constraints) + " constraints!");
    }

    if (nr_constraints == 0)
        return;

    bool symbolic = false;

    in >> std::ws;
    int c = in.peek();
    if (c == 's') {
        string dummy;
        in >> dummy;
        if (dummy != "symbolic")
            throw BadInputException("Illegal keyword " + dummy + " in input!");
        symbolic = true;
    }

    long hom_correction = 0;
    if (forced_hom)
        hom_correction = 1;

    for (long i = 0; i < nr_constraints; ++i) {
        vector<Number> left(dim - hom_correction);
        string rel, modulus_str;
        Number right, modulus = 0;

        if (symbolic) {
            read_symbolic_constraint(in, rel, left, right, modulus, forced_hom);
        }
        else {  // ordinary constraint read here
            for (long j = 0; j < dim - hom_correction; ++j) {
                read_number(in, left[j]);
            }
            in >> rel;
            read_number(in, right);
            if (rel == "~") {
                if (!read_modulus(in, modulus))
                    throw BadInputException("Error while reading modulus!");
            }
            if (in.fail()) {
                throw BadInputException("Error while reading constraint!");
            }
        }
        process_constraint(rel, left, right, modulus, input_map, forced_hom);
    }
}

template <typename Number>
bool read_sparse_vector(istream& in, vector<Number>& input_vec, long length) {
    input_vec = vector<Number>(length, 0);
    char dummy;

    while (in.good()) {
        in >> std::ws;
        char c = in.peek();
        if (c == ';') {
            in >> dummy;  // swallow ;
            return true;
        }
        string range;
        while (true) {
            in >> c;
            if (in.fail())
                return false;
            if (c != ':')
                range += c;
            else
                break;
        }
        int first_pos = -1, last_pos = -1;
        size_t found_dots = range.find("..", 0);
        if (found_dots != string::npos) {
            if (found_dots == 0)
                return false;
            first_pos = stoi(range.substr(0, found_dots));
            first_pos--;
            last_pos = stoi(range.substr(found_dots + 2));
            last_pos--;
        }
        else {
            first_pos = stoi(range);
            first_pos--;
            last_pos = first_pos;
        }

        if (first_pos < 0 || first_pos >= length)
            return false;
        if (last_pos < first_pos || last_pos >= length)
            return false;

        Number value;
        read_number(in, value);
        if (in.fail())
            return false;
        for (int i = first_pos; i <= last_pos; ++i)
            input_vec[i] = value;
    }

    return false;
}

template <typename Number>
bool read_formatted_vector(istream& in, vector<Number>& input_vec) {
    input_vec.clear();
    in >> std::ws;
    char dummy;
    in >> dummy;  // read first proper character
    if (dummy != '[')
        return false;
    bool one_more_entry_required = false;
    while (in.good()) {
        in >> std::ws;
        if (!one_more_entry_required && in.peek() == ']') {
            in >> dummy;
            return true;
        }
        Number number;
        read_number(in, number);
        if (in.fail())
            return false;
        input_vec.push_back(number);
        in >> std::ws;
        one_more_entry_required = false;
        if (in.peek() == ',' || in.peek() == ';') {  // skip potential separator
            in >> dummy;
            one_more_entry_required = true;
        }
    }
    return false;
}

void read_polynomial(istream& in, string& polynomial) {
    char c;
    while (in.good()) {
        in >> c;
        if (in.fail())
            throw BadInputException("Error while reading polynomial!");
        if (c == ';') {
            if (polynomial.size() == 0)
                throw BadInputException("Error while reading polynomial!");
            return;
        }
        polynomial += c;
    }
}

template <typename Number>
bool read_formatted_matrix(istream& in, Matrix<Number>& input_Matrix, bool transpose) {
    vector<vector<Number> > input_mat;
    input_mat.clear();
    in >> std::ws;
    char dummy;
    in >> dummy;  // read first proper character
    if (dummy != '[')
        return false;
    bool one_more_entry_required = false;
    while (in.good()) {
        in >> std::ws;
        if (!one_more_entry_required && in.peek() == ']') {  // closing ] found
            in >> dummy;
            input_Matrix = Matrix<Number>(input_mat);
            if (transpose)
                input_Matrix = input_Matrix.transpose();
            return true;
        }
        vector<Number> input_vec;
        if (!read_formatted_vector(in, input_vec)) {
            throw BadInputException("Error in reading input vector!");
        }
        if (input_mat.size() > 0 && input_vec.size() != input_mat[0].size()) {
            throw BadInputException("Rows of input matrix have unequal lengths!");
        }
        input_mat.push_back(input_vec);
        in >> std::ws;
        one_more_entry_required = false;
        if (in.peek() == ',' || in.peek() == ';') {  // skip potential separator
            in >> dummy;
            one_more_entry_required = true;
        }
    }

    return false;
}

void read_number_field_strings(istream& in, string& mp_string, string& indet, string& emb_string) {
    char c;
    string s;
    in >> s;
    if (s != "min_poly" && s != "minpoly")
        throw BadInputException("Error in reading number field: expected keyword min_poly or minpoly");
    in >> ws;
    c = in.peek();
    if (c != '(')
        throw BadInputException("Error in reading number field: min_poly does not start with (");
    in >> c;

    while (in.good()) {
        c = in.peek();
        if (c == ')') {
            in.get(c);
            break;
        }
        in.get(c);
        if (in.fail())
            throw BadInputException("Error in reading number field: min_poly not terminated by )");
        mp_string += c;
    }
    // omp_set_num_threads(1);

    for (auto& g : mp_string) {
        if (isalpha(g)) {
            indet = g;
            break;
        }
    }

    if (indet == "e" || indet == "x")
        throw BadInputException("Letters e and x not allowed for field generator");

    in >> s;
    if (s != "embedding")
        throw BadInputException("Error in reading number field: expected keyword embedding");
    in >> ws;
    c = in.peek();
    if (c == '[') {
        in >> c;
        while (in.good()) {
            in >> c;
            if (c == ']')
                break;
            emb_string += c;
        }
    }
    else
        throw BadInputException("Error in reading number field: definition of embedding does not start with [");

    if (c != ']')
        throw BadInputException("Error in reading number field: definition of embedding does not end with ]");

    if (in.fail())
        throw BadInputException("Could not read number field!");
}

#ifdef ENFNORMALIZ
renf_class_shared read_number_field(istream& in) {
    string mp_string, indet, emb_string;
    read_number_field_strings(in, mp_string, indet, emb_string);

    auto renf = renf_class::make(mp_string, indet, emb_string);
    // renf->set_pword(in);
    nmz_set_pword(renf,in);

    return renf;
}
#endif

void read_num_param(istream& in, map<NumParam::Param, long>& num_param_input, NumParam::Param numpar, const string& type_string) {
    long value;
    in >> value;
    if (in.fail())
        throw BadInputException("Error in reading " + type_string);
    num_param_input[numpar] = value;
}

template <typename Number>
InputMap<Number> readNormalizInput(istream& in,
                                                                 OptionsHandler& options,
                                                                 map<NumParam::Param, long>& num_param_input,
                                                                 string& polynomial,
                                                                 renf_class_shared& number_field) {
    string type_string;
    long i, j;
    long nr_rows, nr_columns, nr_rows_or_columns;
    InputType input_type;
    Number number;
    ConeProperty::Enum cp;
    NumParam::Param numpar;
    set<NumParam::Param> num_par_already_set;
    bool we_have_a_polynomial = false;

    InputMap<Number> input_map;

    in >> std::ws;  // eat up any leading white spaces
    int c = in.peek();
    if (c == EOF) {
        throw BadInputException("Empty input file!");
    }
    bool new_input_syntax = !std::isdigit(c);

    if (new_input_syntax) {
        long dim;
        while (in.peek() == '/') {
            skip_comment(in);
            in >> std::ws;
        }
        in >> type_string;
        if (!in.good() || type_string != "amb_space") {
            throw BadInputException("First entry must be \"amb_space\"!");
        }
        bool dim_known = false;
        in >> std::ws;
        c = in.peek();
        if (c == 'a') {
            string dummy;
            in >> dummy;
            if (dummy != "auto") {
                throw BadInputException("Bad amb_space value!");
            }
        }
        else {
            in >> dim;
            if (!in.good() || dim < 0) {
                throw BadInputException("Bad amb_space value!");
            }
            dim_known = true;
        }
        while (in.good()) {  // main loop

            bool transpose = false;
            in >> std::ws;  // eat up any leading white spaces
            c = in.peek();
            if (c == EOF)
                break;
            if (c == '/') {
                skip_comment(in);
            }
            else {
                in >> type_string;
                if (in.fail()) {
                    throw BadInputException("Could not read type string!");
                }
                if (std::isdigit(c)) {
                    throw BadInputException("Unexpected number " + type_string + " when expecting a type!");
                }
                if (isConeProperty(cp, type_string)) {
                    options.activateInputFileConeProperty(cp);
                    continue;
                }
                if (isNumParam(numpar, type_string)) {
                    auto ns = num_par_already_set.find(numpar);
                    if (ns != num_par_already_set.end())
                        throw BadInputException("Numerical parameter " + type_string + " set twice");
                    read_num_param(in, num_param_input, numpar, type_string);
                    num_par_already_set.insert(numpar);
                    continue;
                }
                if (type_string == "LongLong") {
                    options.activateInputFileLongLong();
                    continue;
                }
                if (type_string == "NoExtRaysOutput") {
                    options.activateNoExtRaysOutput();
                    continue;
                }
                if (type_string == "NoHilbertBasisOutput") {
                    options.activateNoHilbertBasisOutput();
                    continue;
                }
                if (type_string == "NoMatricesOutput") {
                    options.activateNoMatricesOutput();
                    continue;
                }
                if (type_string == "NoSuppHypsOutput") {
                    options.activateNoSuppHypsOutput();
                    continue;
                }
                if (type_string == "number_field") {
#ifndef ENFNORMALIZ
                    throw BadInputException("number_field only allowed for Normaliz with e-antic");
#else
                    if (!std::is_same<Number, renf_elem_class>::value) {
                        throw NumberFieldInputException();
                    }
                    // TODO: Err if Number is not renf_elem_class
                    number_field = read_number_field(in);
#endif
                    continue;
                }
                if (type_string == "total_degree") {
                    if (!dim_known) {
                        throw BadInputException("Ambient space must be known for " + type_string + "!");
                    }
                    input_type = Type::grading;
                    vector<Number> TotDeg(dim + type_nr_columns_correction(input_type),1);
                    Matrix<Number> TotDegMat(TotDeg);
                    save_matrix(input_map, input_type, TotDegMat);
                    continue;
                }
                if (type_string == "nonnegative") {
                    if (!dim_known) {
                        throw BadInputException("Ambient space must be known for " + type_string + "!");
                    }
                    input_type = Type::signs;
                    Matrix<Number> SignMat(vector<Number>(dim + type_nr_columns_correction(input_type), 1));
                    save_matrix(input_map, input_type, SignMat);
                    continue;
                }
                if (type_string == "constraints") {
                    if (!dim_known) {
                        throw BadInputException("Ambient space must be known for " + type_string + "!");
                    }
                    read_constraints(in, dim, input_map, false);
                    continue;
                }
                if (type_string == "hom_constraints") {
                    if (!dim_known) {
                        throw BadInputException("Ambient space must be known for " + type_string + "!");
                    }
                    read_constraints(in, dim, input_map, true);
                    continue;
                }
                if (type_string == "polynomial") {
                    if (we_have_a_polynomial)
                        throw BadInputException("Only one polynomial allowed");
                    read_polynomial(in, polynomial);
                    we_have_a_polynomial = true;
                    continue;
                }

                input_type = to_type(type_string);
                if (dim_known)
                    nr_columns = dim + type_nr_columns_correction(input_type);

                if (type_is_vector(input_type)) {
                    nr_rows_or_columns = nr_rows = 1;
                    in >> std::ws;  // eat up any leading white spaces
                    c = in.peek();
                    if (c == 'u') {  // must be unit vector
                        string vec_kind;
                        in >> vec_kind;
                        if (vec_kind != "unit_vector") {
                            throw BadInputException("Error while reading " + type_string + ": unit_vector expected!");
                        }

                        long pos = 0;
                        in >> pos;
                        if (in.fail()) {
                            throw BadInputException("Error while reading " + type_string + " as a unit_vector!");
                        }

                        if (!dim_known) {
                            throw BadInputException("Ambient space must be known for unit vector " + type_string + "!");
                        }

                        Matrix<Number> e_i = Matrix<Number>(vector<Number>(nr_columns, 0));
                        if (pos < 1 || pos > static_cast<long>(e_i[0].size())) {
                            throw BadInputException("Error while reading " + type_string + " as a unit_vector " + toString(pos) +
                                                    "!");
                        }
                        pos--;  // in input file counting starts from 1
                        e_i[0].at(pos) = 1;
                        save_matrix(input_map, input_type, e_i);
                        continue;
                    }  // end unit vector

                    if (c == 's') {  // must be "sparse"
                        string vec_kind;
                        in >> vec_kind;
                        if (vec_kind != "sparse") {
                            throw BadInputException("Error while reading " + type_string + ": sparse vector expected!");
                        }

                        if (!dim_known) {
                            throw BadInputException("Ambient space must be known for sparse vector " + type_string + "!");
                        }

                        vector<Number> sparse_vec;
                        nr_columns = dim + type_nr_columns_correction(input_type);
                        bool success = read_sparse_vector(in, sparse_vec, nr_columns);
                        if (!success) {
                            throw BadInputException("Error while reading " + type_string + " as a sparse vector!");
                        }
                        save_matrix(input_map, input_type, Matrix<Number>(sparse_vec));
                        continue;
                    }

                    if (c == '[') {  // must be formatted vector
                        vector<Number> formatted_vec;
                        bool success = read_formatted_vector(in, formatted_vec);
                        if (!dim_known) {
                            dim = formatted_vec.size() - type_nr_columns_correction(input_type);
                            dim_known = true;
                            nr_columns = dim + type_nr_columns_correction(input_type);
                        }
                        if (!success || (long)formatted_vec.size() != nr_columns) {
                            throw BadInputException("Error while reading " + type_string + " as a formatted vector!");
                        }
                        save_matrix(input_map, input_type, Matrix<Number>(formatted_vec));
                        continue;
                    }  // end formatted vector
                }
                else {  // end vector, it is a matrix. Plain vector read as a one row matrix later on
                    in >> std::ws;
                    c = in.peek();

                    if (c != '[' && c != 'u' && !std::isdigit(c)) {  // must be transpose
                        string transpose_str;
                        in >> transpose_str;
                        if (transpose_str != "transpose") {
                            throw BadInputException("Illegal keyword " + transpose_str + " following matrix type!");
                        }
                        transpose = true;
                        in >> std::ws;
                        c = in.peek();
                    }
                    if (c == '[') {  // it is a formatted matrix
                        Matrix<Number> formatted_mat;
                        bool success = read_formatted_matrix(in, formatted_mat, transpose);
                        if (!success) {
                            throw BadInputException("Error while reading formatted matrix " + type_string + "!");
                        }
                        if (formatted_mat.nr_of_rows() == 0) {  // empty matrix
                            input_type = to_type(type_string);
                            save_empty_matrix(input_map, input_type);
                            continue;
                        }
                        if (!dim_known) {
                            dim = formatted_mat[0].size() - type_nr_columns_correction(input_type);
                            dim_known = true;
                            nr_columns = dim + type_nr_columns_correction(input_type);
                        }

                        if ((long)formatted_mat[0].size() != nr_columns) {
                            throw BadInputException("Error while reading formatted matrix " + type_string + "!");
                        }

                        save_matrix(input_map, input_type, formatted_mat);
                        continue;
                    }
                    if (c == 'u') {  // must be unit matrix
                        string unit_test;
                        in >> unit_test;
                        if (unit_test != "unit_matrix") {
                            throw BadInputException("Error while reading " + type_string + ": unit matrix expected!");
                        }
                        if (!dim_known) {
                            throw BadInputException("Dimension must be known for unit matrix!");
                        }
                        nr_columns = dim + type_nr_columns_correction(input_type);
                        Matrix<Number> unit_mat(nr_columns,nr_columns);
                        nr_columns = dim + type_nr_columns_correction(input_type);
                        for (long i = 0; i < nr_columns; ++i) {
                            unit_mat[i][i] = 1;
                        }
                        save_matrix(input_map, input_type, unit_mat);
                        continue;
                    }

                    // only plain matrix left

                    in >> nr_rows_or_columns;      // is number of columns if transposed
                    nr_rows = nr_rows_or_columns;  // most of the time
                }

                if (!dim_known) {
                    throw BadInputException("Ambient space must be known for plain matrix or vector " + type_string + "!");
                }

                if (transpose)
                    swap(nr_rows, nr_columns);

                if (in.fail() || nr_rows_or_columns < 0) {
                    throw BadInputException("Error while reading " + type_string + " (a " + toString(nr_rows) + "x" +
                                            toString(nr_columns) + " matrix) !");
                }
                if (nr_rows == 0) {
                    input_type = to_type(type_string);
                    save_empty_matrix(input_map, input_type);
                    continue;
                }

                Matrix<Number> M(nr_rows,nr_columns);
                bool dense_matrix = true;
                in >> std::ws;
                c = in.peek();
                if (c == 's') {  // must be sparse
                    dense_matrix = false;
                    string sparse_test;
                    in >> sparse_test;
                    if (sparse_test != "sparse") {
                        throw BadInputException("Error while reading " + type_string + ": sparse matrix expected!");
                    }
                    for (long i = 0; i < nr_rows; ++i) {
                        bool success = read_sparse_vector(in, M[i], nr_columns);
                        if (!success) {
                            throw BadInputException("Error while reading " + type_string + ": corrupted sparse matrix");
                        }
                    }
                }
                if (dense_matrix) {  // dense matrix
                    for (i = 0; i < nr_rows; i++) {
                        M[i].resize(nr_columns);
                        for (j = 0; j < nr_columns; j++) {
                            read_number(in, M[i][j]);
                            // cout << M[i][j] << endl;
                        }
                    }
                }
                if (transpose)
                    M = M.transpose();
                save_matrix(input_map, input_type, M);
            }
            if (in.fail()) {
                throw BadInputException("Error while reading " + type_string + " (a " + toString(nr_rows) + "x" +
                                        toString(nr_columns) + " matrix) !");
            }
        }
    }
    else {
        // old input syntax
        while (in.good()) {
            in >> nr_rows;
            if (in.fail())
                break;
            in >> nr_columns;
            if ((nr_rows < 0) || (nr_columns < 0)) {
                throw BadInputException("Error while reading a " + toString(nr_rows) + "x" + toString(nr_columns) + " matrix !");
            }
            Matrix<Number> M(nr_rows, nr_columns);
            for (i = 0; i < nr_rows; i++) {
                for (j = 0; j < nr_columns; j++) {
                    read_number(in, M[i][j]);
                }
            }

            in >> type_string;

            if (in.fail()) {
                throw BadInputException("Error while reading a " + toString(nr_rows) + "x" + toString(nr_columns) + " matrix!");
            }

            input_type = to_type(type_string);

            // check if this type already exists
            save_matrix(input_map, input_type, M);
        }
    }
    return input_map;
}

template InputMap<mpq_class> readNormalizInput(istream& in,
                                                                             OptionsHandler& options,
                                                                             map<NumParam::Param, long>& num_param_input,
                                                                             string& polynomial,
                                                                             renf_class_shared& number_field);

#ifdef ENFNORMALIZ
template InputMap<renf_elem_class> readNormalizInput(istream& in,
                                                                                   OptionsHandler& options,
                                                                                   map<NumParam::Param, long>& num_param_input,
                                                                                   string& polynomial,
                                                                                   renf_class_shared& number_field);
#endif

#ifndef NMZ_MIC_OFFLOAD  // offload with long is not supported
template Matrix<long> readMatrix(const string project);
#endif  // NMZ_MIC_OFFLOAD
template Matrix<long long> readMatrix(const string project);
template Matrix<mpz_class> readMatrix(const string project);

}  // namespace libnormaliz
