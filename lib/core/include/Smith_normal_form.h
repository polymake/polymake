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

#pragma once

#include "polymake/SparseMatrix.h"
#include "polymake/Bitset.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"
#include "polymake/numerical_functions.h"
#include "polymake/list"
#include "polymake/GenericStruct.h"
#if POLYMAKE_DEBUG
#  include "polymake/client.h"
#endif

namespace pm {

class dummy_companion_logger {
public:
   template <typename E>
   void from_right(const SparseMatrix2x2<E>&) const {}
   template <typename E>
   void from_left(const SparseMatrix2x2<E>&) const {}
   template <typename TPerm>
   void permute_rows(const TPerm&) const {}
   template <typename TPerm>
   void permute_cols(const TPerm&) const {}
};

template <typename Logger>
class TransposedLogger : public Logger {
protected:
   TransposedLogger();
   ~TransposedLogger();
public:
   template <typename E>
   void from_right(const SparseMatrix2x2<E>& U) const { Logger::from_left(T(U)); }
   template <typename E>
   void from_left(const SparseMatrix2x2<E>& U) const { Logger::from_right(T(U)); }
};

template <typename Logger> inline
const TransposedLogger<Logger>& transpose_logger(const Logger& l)
{
   return static_cast<const TransposedLogger<Logger>&>(l);
}

template <typename E, bool inverse_companions>
class SNF_companion_logger {
protected:
   SparseMatrix<E>* L;
   SparseMatrix<E>* R;
public:
   // U is always unimodular, only the sign of det(U) can vary
   static
   bool det_pos(const SparseMatrix2x2<E>& U)
   {
      return U.a_ii*U.a_jj > U.a_ij*U.a_ji;
   }

   static
   SparseMatrix2x2<E> inv(const SparseMatrix2x2<E>& U)
   {
      return det_pos(U) ? SparseMatrix2x2<E>(U.i, U.j, U.a_jj, -U.a_ij, -U.a_ji, U.a_ii)
                        : SparseMatrix2x2<E>(U.i, U.j, -U.a_jj, U.a_ij, U.a_ji, -U.a_ii);
   }

   static
   SparseMatrix2x2<E> inv(const Transposed< SparseMatrix2x2<E> >& U)
   {
      return det_pos(U) ? SparseMatrix2x2<E>(U.i, U.j, U.a_jj, -U.a_ji, -U.a_ij, U.a_ii)
                        : SparseMatrix2x2<E>(U.i, U.j, -U.a_jj, U.a_ji, U.a_ij, -U.a_ii);
   }
public:
   SNF_companion_logger(SparseMatrix<E>* Larg, SparseMatrix<E>* Rarg)
      : L(Larg), R(Rarg) {}

   template <typename Matrix>
   void from_right(const Matrix& U) const
   {
      if (inverse_companions)
         R->multiply_from_left(inv(U));
      else
         R->multiply_from_right(U);
   }

   template <typename Matrix>
   void from_left(const Matrix& U) const
   {
      if (inverse_companions)
         L->multiply_from_right(inv(U));
      else
         L->multiply_from_left(U);
   }

   template <typename TPerm>
   void permute_rows(const TPerm& perm) const
   {
      if (inverse_companions)
         L->permute_cols(perm);
      else
         L->permute_rows(perm);
   }

   template <typename TPerm>
   void permute_cols(const TPerm& perm) const
   {
      if (inverse_companions)
         R->permute_rows(perm);
      else
         R->permute_cols(perm);
   }
};

template <typename Matrix, typename CompanionLogger>
Int smith_normal_form_steps(Matrix& M, CompanionLogger& Logger
#if POLYMAKE_DEBUG
                            , pm::SparseMatrixStatistics<typename Matrix::element_type>& stat,
                            Int debug_level, const char* line_name
#endif
                            )
{
   const bool Logger_dummy=is_derived_from<CompanionLogger, dummy_companion_logger>::value;
   using E = typename Matrix::element_type;

   // These are working variables in the following loops,
   // but they shouldn't be created and destroyed all over the time
   E pivot_elem=one_value<E>(),
              g=one_value<E>();
   SparseMatrix2x2<E> U;

   Int r = 0, c = 0, skipped = 0;
   const Int R = M.rows();
   while (r < R) {
      bool can_reduce = true, can_eliminate = false;
      {
         auto e = M.row(r).begin();
         if (e.at_end()  ||  M.row(r).size()==1 && M.col(e.index()).size()==1) {
            ++r; ++skipped; continue;
         }
         skipped=0;
         g=gcd(M.row(r));
         do {
            if (abs_equal(g, *e)) {
               can_reduce=false;
               if (is_one(g) || g == gcd(M.col(e.index()))) {
                  can_eliminate=true;
                  c=e.index();
                  pivot_elem=*e;
                  break;
               }
            }
            ++e;
         } while (!e.at_end());
      }

      if (can_reduce) {
         auto e = M.row(r).begin();
         pivot_elem = *e;
         c=e.index();
         for (++e; !e.at_end(); ++e) {
            ExtGCD<E> x = ext_gcd(pivot_elem, *e);
            if (!is_zero(x.q)) {
               if (!is_zero(x.p)) {
                  U.i=c;                    U.j=e.index();
                  U.a_ii = -x.k2;           std::swap(U.a_ij, x.p);
                  std::swap(U.a_ji, x.k1);  std::swap(U.a_jj, x.q);
                  M.multiply_from_right(U);
                  Logger.from_right(U);
               }
               pivot_elem=*e;
               c=e.index();
               if (g==x.g) break;
            }
         }
         can_eliminate= is_one(g) || g==gcd(M.col(c));

#if POLYMAKE_DEBUG
         if (debug_level) {
            stat.gather(M);
            if (debug_level > 1) cout << "smith_normal_form(reducing " << line_name << " " << r << " to " << g << " ):\n" << std::setw(6) << M;
         }
#endif
      }

      if (can_eliminate) {
         if (!Logger_dummy) {
            U.i=r;  U.a_ii=one_value<E>();  U.a_jj=one_value<E>();  U.a_ij=zero_value<E>();
         }
         Int next_r = -1;
         for (auto e = M.col(c).begin(); !e.at_end(); ) {
            if ((U.j = e.index()) == r) { ++e; continue; }
            if (next_r < 0) next_r = U.j;
            if (abs_equal(pivot_elem, *e)) {
               if (sign(pivot_elem) == sign(*e)) {
                  if (!Logger_dummy) U.a_ji = -one_value<E>();
                  ++e;
                  M.row(U.j) -= M.row(r);
               } else {
                  if (!Logger_dummy) U.a_ji = one_value<E>();
                  ++e;
                  M.row(U.j) += M.row(r);
               }
            } else {
               U.a_ji = -div_exact(*e,pivot_elem);
               ++e;
               M.row(U.j) += M.row(r) * U.a_ji;
            }
            Logger.from_left(U);
         }
         if (!Logger_dummy) {
            U.i = c; U.a_ji = zero_value<E>();
         }
         for (auto e = M.row(r).begin(); !e.at_end(); ) {
            if ((U.j = e.index()) == c) { ++e; continue; }
            if (!Logger_dummy) {
               U.a_ij = -div_exact(*e, pivot_elem);
               Logger.from_right(U);
            }
            M.row(r).erase(e++);
         }

#if POLYMAKE_DEBUG
         if (debug_level) {
            stat.gather(M);
            if (debug_level > 1) cout << "smith_normal_form(eliminating " << line_name << " " << r << " [" << c << "] ):\n" << std::setw(6) << M;
         }
#endif
         if (next_r >= 0) r = next_r;
         else ++r;
      } else {
         ++r;
      }
   }
   return skipped;
}

template <typename E, typename CompanionLogger, bool strict_diagonal>
Int smith_normal_form(SparseMatrix<E>& M, std::list<std::pair<E, Int>>& torsion,
                      const CompanionLogger& Logger, bool_constant<strict_diagonal>)
{
   constexpr bool Logger_dummy = is_derived_from<CompanionLogger, dummy_companion_logger>::value;

#if POLYMAKE_DEBUG
   const Int debug_level = polymake::get_debug_level();
   if (debug_level) cout << "smith_normal_form(initial):\n" << std::setw(6) << M;

   pm::SparseMatrixStatistics<E> stat;
   if (debug_level) {
      stat.gather(M);
      cout << "smith_normal_form(initial statistics):\n" << stat;
   }
   while (smith_normal_form_steps(M, Logger , stat, debug_level, "row") < M.rows() &&
          smith_normal_form_steps(T(M), transpose_logger(Logger) , stat, debug_level, "col") < M.cols()) ;

   if (debug_level) cout << "smith_normal_form(final statistics):\n" << stat << endl;
#else
   while (smith_normal_form_steps(M, Logger) < M.rows() &&
          smith_normal_form_steps(T(M), transpose_logger(Logger)) < M.cols()) ;
#endif
   Int rank = 0;
   torsion.clear();
   Array<Int> r_perm(strict_diagonal ? M.rows() : 0),
              c_perm(strict_diagonal ? M.cols() : 0);
   Array<Int>::iterator rp = r_perm.begin(), rpe = r_perm.end(),
                        cp = c_perm.begin(), cpe = c_perm.end();

   for (auto r = entire(rows(M)); !r.at_end(); ++r) {
      if (!r->empty()) {
         ++rank;
         auto e = r->begin();
         if (abs_equal(*e, 1)) {
            if (strict_diagonal) {
               *rp++ = r.index();
               *cp++ = e.index();
            }
         } else {
            torsion.push_back(std::make_pair(abs(*e), e.index()));
         }
      }
   }

   for (auto t = torsion.begin(), t_end = torsion.end(); t != t_end; ++t) {
      auto t2 = t;
      for (++t2; t2 != t_end; ) {
         ExtGCD<E> x = ext_gcd(t->first, t2->first);
         if (t->first == x.g) {
            std::swap(*t, *t2);  ++t2;
         } else if (t2->first == x.g) {
            ++t2;
         } else {
            if (!Logger_dummy) {
               negate(x.k1);
               Logger.from_left(SparseMatrix2x2<E>(M.col(t->second).begin().index(), M.col(t2->second).begin().index(),
                                                   x.k2, x.p*x.k1,
                                                   1,    x.q) );
               Logger.from_right(SparseMatrix2x2<E>(t->second, t2->second,
                                                    x.q*x.k2, x.p,
                                                    x.k1,     1) );
            }
            t->first *= x.k2;
            if (strict_diagonal) {
               M.col(t->second).front()*=x.k2;
               M.col(t2->second).front()=x.g;
            }
            if (is_one(x.g)) {
               if (strict_diagonal) *rp++=M.col(t2->second).begin().index(), *cp++=t2->second;
               torsion.erase(t2++);
            } else {
               t2->first=x.g;  ++t2;
            }
         }
      }
   }

   if (strict_diagonal) {
      for (auto t=entire<reversed>(torsion);  !t.at_end();  ++t)
         *rp++=M.col(t->second).begin().index(), *cp++=t->second;

      if (rp < rpe)
         for (auto r=rows(M).begin(); ; ++r)
            if (r->empty()) {
               *rp++=r.index();
               if (rp==rpe) break;
            }
      if (cp < cpe)
         for (auto c=cols(M).begin(); ; ++c)
            if (c->empty()) {
               *cp++=c.index();
               if (cp==cpe) break;
            }

      Logger.permute_rows(r_perm);
      Logger.permute_cols(c_perm);
      M.permute_rows(r_perm);
      M.permute_cols(c_perm);
   }

   return rank;
}

template <typename E>
void compress_torsion(std::list<std::pair<E, Int>>& torsion)
{
   for (auto t = torsion.begin(), t_end = torsion.end(); t != t_end; ++t) {
      t->second = 1;
      auto t2 = t;  ++t2;
      for (;;) {
         if (t2 == t_end) return;
         if (t->first == t2->first) {
            ++t->second;
            t2=torsion.erase(t2);
         } else {
            break;
         }
      }
   }
}

//return value is the number of rows cancelled out as they only contain one entry that is +-1
//hence it has to be added to the rank of the SNF later
template <typename E, typename CompanionLogger>
Int eliminate_ones(SparseMatrix<E>& M, Bitset& elim_rows, Bitset& elim_cols, const CompanionLogger& Logger)
{
   const bool Logger_dummy=is_derived_from<CompanionLogger, dummy_companion_logger>::value;

#if POLYMAKE_DEBUG
   const Int debug_level = polymake::get_debug_level();
   if (debug_level > 1) cout << "eliminate_ones(initial):\n" << std::setw(6) << M;
   pm::SparseMatrixStatistics<E> stat;
   if (debug_level) {
      stat.gather(M);
      cout << "eliminate_ones(initial statistics):\n" << stat;
   }
#endif

   SparseMatrix2x2<E> U;
   const Int R = M.rows(), C = M.cols();
   elim_rows.clear(); elim_rows.reserve(R);
   elim_cols.clear(); elim_cols.reserve(C);
   Int count = 0;

   for (Int c = 0, c_last = C;  c != c_last;  c == C && c_last < C && (c = 0)) {    // repeat until matrix is empty or contains no +1/-1 more
      if (M.col(c).empty() || elim_cols.contains(c)) { ++c; continue; }

      typename SparseMatrix<E>::col_type::iterator e=M.col(c).begin();
      while (!abs_equal(*e,1) && !(++e).at_end()) ;
      if (e.at_end()) { ++c; continue; } //skip zero col

      Int r = e.index(); //index of first non-zero (i.e. +-1) entry in this col

      if (!Logger_dummy) {
         U.i = r;
         U.a_ii = one_value<E>();
         U.a_jj = one_value<E>();
         U.a_ij = zero_value<E>();
      }
      for (auto e2 = M.col(c).begin(); !e2.at_end(); ) {
         if ((U.j = e2.index()) == r) {
            // skip row r
            ++e2;
            continue;
         }
         if (abs_equal(*e,*e2)) { // found more (+-1) entrys below r
            if (sign(*e) == sign(*e2)) {
               if (!Logger_dummy) U.a_ji = -one_value<E>();
               ++e2;
               M.row(U.j) -= M.row(r);//subtract rows -> row U.j now has zero entry at col c
            } else {
               if (!Logger_dummy) U.a_ji = one_value<E>();
               ++e2;
               M.row(U.j) += M.row(r);
            }
         } else {
            U.a_ji = (*e) > 0 ? -(*e2) : (*e2);
            ++e2;
            M.row(U.j) += U.a_ji * M.row(r);
         }
         Logger.from_left(U);
      }
      if (Logger_dummy) {
         M.row(r).clear();  // row is cleared, but in reality there still is an +-1 entry at (r,c)
         ++count;           // which is why we have to count the rank up by one
      } else {
         U.i = c;
         U.a_ji = zero_value<E>();
         for (auto e2 = M.row(r).begin(); !e2.at_end(); ) {
            if ((U.j = e2.index()) == c) {
               ++e2;
               continue;
            }
            U.a_ij = (*e)>0 ? -(*e2) : (*e2);
            Logger.from_right(U);
            M.row(r).erase(e2++);
         }
      }
      elim_rows += r;
      elim_cols += c;
#if POLYMAKE_DEBUG
      if (debug_level) {
         stat.gather(M);
         if (debug_level > 1) cout << "eliminate_ones(eliminating col " << c << " [" << r << "] ):\n" << std::setw(6) << M;
      }
#endif
      c_last=c++;
   }

#if POLYMAKE_DEBUG
   if (debug_level) cout << "eliminate_ones(final statistics):\n" << stat << endl;
#endif
   return count;
}

/// Compute the compact respresentation of the Smith normal form, without companion matrices.
/// Input matrix M is corrupted during the computations.
/// @param[out] torsion list of diagonal elements of SNF not equal 1 with multiplicities.
/// @return rank of M.
template <typename E>
Int smith_normal_form_only(SparseMatrix<E>& M, std::list<std::pair<E, Int>>& torsion)
{
   Int rank = smith_normal_form(M, torsion, dummy_companion_logger(), std::false_type());
   compress_torsion(torsion);
   return rank;
}

/// Complete result of computation of Smith normal form.
template <typename E>
class SmithNormalForm :
   public GenericStruct<SmithNormalForm<E> > {
public:
   using matrix_type = SparseMatrix<E> ;
   using torsion_type = std::list<std::pair<E, Int>>;

   DeclSTRUCT( DeclTemplFIELD(form, matrix_type)                // input matrix converted to the normal form
               DeclTemplFIELD(left_companion, matrix_type)      // input = left_companion * form * right_companion  OR
               DeclTemplFIELD(right_companion, matrix_type)     // form = left_companion * input * right_companion (inverted call)
               DeclTemplFIELD(torsion, torsion_type)            // list of torsion coefficients extracted from the diagonal of form
               DeclTemplFIELD(rank, Int) );                     // rank of form
};

/// Compute the Smith normal form and companion matrices.
/// @param inverse_companions if true: result.form = result.left_companion * M * result.right_companion
///                           if false: M = result.left_companion * result.form * result.right_companion
template <typename Matrix, typename E>
SmithNormalForm<E>
smith_normal_form(const GenericMatrix<Matrix, E>& M,
                  std::enable_if_t<std::numeric_limits<E>::is_integer, bool> inverse_companions = false)
{
   SmithNormalForm<E> res;
   res.form = M;
   res.left_companion = unit_matrix<E>(M.rows());
   res.right_companion = unit_matrix<E>(M.cols());
   if (inverse_companions)
      res.rank = smith_normal_form(res.form, res.torsion, SNF_companion_logger<E, false>(&res.left_companion, &res.right_companion), std::true_type());
   else
      res.rank = smith_normal_form(res.form, res.torsion, SNF_companion_logger<E, true>(&res.left_companion, &res.right_companion), std::true_type());
   compress_torsion(res.torsion);
   return res;
}

}
namespace polymake {

using pm::SmithNormalForm;
using pm::smith_normal_form;
using pm::smith_normal_form_only;

}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
