/* Copyright (c) 1997-2022
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

#include <fcntl.h>
#include <stdio.h>

#include "polymake/polytope/lrs_interface.h"
#include "polymake/hash_set"
#include "polymake/ListMatrix.h"

#define MA
#define GMP
extern "C" {
#ifdef HAVE_LRSDRIVER
  #include <lrsdriver.h>
#endif
  #include <lrslib.h>
}
#undef positive
#undef negative
#undef zero
#undef one
#undef gcd
#undef copy
#undef sign
#undef GMP
#undef MA


namespace polymake { namespace polytope { namespace lrs_interface {

#ifdef POLYMAKE_LRS_STANDALONE_GLOBAL_INIT

namespace {

void global_construct_standalone()
{
   FILE* dummy_out = nullptr;
#ifdef POLYMAKE_LRS_SUPPRESS_OUTPUT
   dummy_out = fopen("/dev/null", "w");
#endif
   lrs_mp_init(0, nullptr, dummy_out);
}

void global_destroy_standalone()
{
#ifdef POLYMAKE_LRS_SUPPRESS_OUTPUT
   fclose(lrs_ofp);
#endif
}

}

void (* const LrsInstance::Initializer::global_construct)() = &global_construct_standalone;
void (* const LrsInstance::Initializer::global_destroy)() = &global_destroy_standalone;

#endif

LrsInstance::Initializer::Initializer()
{
   global_construct();
}

LrsInstance::Initializer::~Initializer()
{
   global_destroy();
}

class lrs_mp_vector_output {
public:
   explicit lrs_mp_vector_output(Int n)
      : d(n-1)
      , ptr(lrs_alloc_mp_vector(d))
   {
      if (!ptr) throw std::bad_alloc();
   }

   ~lrs_mp_vector_output() { lrs_clear_mp_vector(ptr, d); }

   operator lrs_mp_vector() { return ptr; }

   const mpz_t& front() const { return ptr[0]; }

   class iterator {
   public:
      typedef std::input_iterator_tag iterator_category;
      typedef Rational value_type;
      typedef Rational reference;
      typedef void pointer;
      typedef ptrdiff_t difference_type;

      iterator(lrs_mp_vector_output& vec, bool or_arg)
         : leading(vec.ptr)
         , cur(leading)
         , last(leading+vec.d)
         , oriented(or_arg)
      {}

      Rational operator* ()
      {
         if (cur==leading) {
            // looking for the leading non-zero
            if (int sgn = mpz_sgn(*leading)) {
               if (!oriented)
                  // all following elements will be divided through the leading non-zero
                  sgn = 1;
               else if (sgn < 0)
                  // all following elements will be divided through abs(leading non-zero)
                  mpz_neg(*leading, *leading);
               ++cur;
               return Rational(sgn);
            } else {
               ++leading;
               return Rational(std::move(*cur++));
            }
         } else if (cur < last) {
            return Rational(std::move(*cur++), *leading);
         } else {
            return Rational(std::move(*cur++), std::move(*leading));
         }
      }

      iterator& operator++ () { return *this; }

   private:
      mpz_t* leading;
      mpz_t* cur;
      mpz_t* const last;
      const bool oriented;
   };

   Vector<Rational> make_Vector(const bool oriented, const bool repair = true)
   {
      Vector<Rational> result(d+1, iterator(*this, oriented));
      if (repair) {
         mpz_t* last=ptr+d;
         if ((**last)._mp_alloc) --last;
         for (mpz_t* cur=ptr;  cur<=last;  ++cur)
            mpz_init(*cur);
      }
      return result;
   }

private:
   Int d;
   lrs_mp_vector ptr;
};

// lrs stores numerators and denominators in separate integer vectors
class lrs_mp_vector_input {
public:
   explicit lrs_mp_vector_input(Int n_arg)
      : n(n_arg)
      , nums(new mpz_t[n])
      , dens(new mpz_t[n]) {}

   ~lrs_mp_vector_input()
   {
      delete[] nums;
      delete[] dens;
   }

   lrs_mp_vector get_nums() const { return nums; }
   lrs_mp_vector get_dens() const { return dens; }

   template <typename Iterator>
   void consume(Iterator&& src)
   {
      for (Int i = 0; i < n; ++i, ++src) {
         *nums[i] = *mpq_numref(src->get_rep());
         *dens[i] = *mpq_denref(src->get_rep());
      }
   }

private:
   Int n;
   lrs_mp_vector nums;
   lrs_mp_vector dens;
};

class lrs_mp_matrix_output {
public:
   lrs_mp_matrix_output(lrs_mp_matrix A, Int m_arg, Int n_arg)
      : ptr(A)
      , m(m_arg)
      , n(n_arg) {}

   ~lrs_mp_matrix_output()
   {
      if (ptr) lrs_clear_mp_matrix(ptr, m, n);
   }

   class iterator {
   public:
      typedef std::input_iterator_tag iterator_category;
      typedef Rational value_type;
      typedef Rational reference;
      typedef void pointer;
      typedef ptrdiff_t difference_type;

      explicit iterator(lrs_mp_matrix_output& mat)
         : vec(mat.ptr)
         , i(0)
         , n(mat.n)
      {}

      Rational operator* ()
      {
         return Rational(std::move((*vec)[i]));
      }

      iterator& operator++ ()
      {
         if (++i==n) {
            i=0;
            ++vec;
         }
         return *this;
      }

   private:
      lrs_mp_vector* vec;
      Int i;
      const Int n;
   };

   Matrix<Rational> make_Matrix()
   {
      return Matrix<Rational>(m, n, iterator(*this));
   }

private:
   lrs_mp_matrix ptr;
   Int m, n;
};

struct dictionary {
   lrs_dat *Q;
   lrs_dic_struct *P;
   lrs_mp_matrix Lin;
   FILE* save_lrs_ofp = nullptr;
#if defined(POLYMAKE_LRS_SUPPRESS_OUTPUT) && POLYMAKE_LRS_SUPPRESS_OUTPUT == 2
   int save_stdout = -1;
#endif

   // stream cleanup and restore stdout
   void restore_ofp() {
      if (lrs_ofp == stderr) {
         fflush(lrs_ofp);
         lrs_ofp = save_lrs_ofp;
      }
#if defined(POLYMAKE_LRS_SUPPRESS_OUTPUT) && POLYMAKE_LRS_SUPPRESS_OUTPUT == 2
      else if (save_stdout != -1) {
         if (stdout != nullptr)
            fflush(stdout);
         dup2(save_stdout, 1);
         close(save_stdout);
      }
#endif
   }

   ~dictionary()
   {
      if (Lin) lrs_clear_mp_matrix(Lin, Q->nredundcol, Q->n);
      lrs_free_dic(P,Q);
      lrs_free_dat(Q);
      restore_ofp();
   }

   // parameter ge: primal case: true for vertex, false for linearity
   //                 dual case: true for inequality, false for equation
   void set_matrix(const Matrix<Rational>& A, Int start_row = 0, bool ge = true)
   {
      lrs_mp_vector_input vec(A.cols());
      auto x=concat_rows(A).begin();

      // lrs enumerates rows starting with 1
      for (Int r = start_row+1, r_end = r+A.rows(); r != r_end; ++r) {
         vec.consume(x);
         lrs_set_row_mp(P, Q, r, vec.get_nums(), vec.get_dens(), ge);
      }
   }

   void set_obj_vector(const Vector<Rational>& V, bool maximize)
   {
      const Int n = V.size();
      if (n != Q->n)
         throw std::runtime_error("lrs_interface - inequalities/objective dimension mismatch");
      lrs_mp_vector_input vec(n);
      vec.consume(V.begin());
      lrs_set_obj_mp(P, Q, vec.get_nums(), vec.get_dens(), maximize);
      Q->lponly=1;
   }


   dictionary(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations, const bool dual, const bool verbose=false)
   {
      // this avoids a segfault in some lrs versions
      if (dual && Inequalities.cols() == 0 && Equations.cols() == 0)
         throw std::runtime_error("lrs_interface - cannot handle ambient dimension 0 in dual mode");
      // initialize static lrs data
      Lin = nullptr;

      if (verbose) {
         save_lrs_ofp = lrs_ofp;
         lrs_ofp = stderr;
      }
#if defined(POLYMAKE_LRS_SUPPRESS_OUTPUT) && POLYMAKE_LRS_SUPPRESS_OUTPUT == 2
      else {
         save_stdout = dup(1);
         dup2(fileno(lrs_ofp), 1);
      }
#endif

      char name[] = "polymake";
      Q=lrs_alloc_dat(name);
      if (!Q) {
         restore_ofp();
         throw std::bad_alloc();
      }
      if (verbose)
         Q->debug=1;
      Q->m=Inequalities.rows()+Equations.rows();
      Q->n=Inequalities.cols();
      if (!Q->n) Q->n=Equations.cols();
      Q->hull=dual?0:1;

      // initialize dynamic lrs data
      P=lrs_alloc_dic(Q);
      if (!P) {
         restore_ofp();
         lrs_free_dat(Q);
         throw std::bad_alloc();
      }
      // store inequalities/points and equations/lineality in Q
      if (Inequalities.rows()) set_matrix(Inequalities);
      if (Equations.rows())    set_matrix(Equations, Inequalities.rows(), false);
   }


   // the following functions "run" lrs
   enum filter_nothing_t { filter_nothing };
   enum filter_bounded_t { filter_bounded };
   enum filter_rays_t { filter_rays };
   enum filter_facets_t { filter_facets };

   Matrix<Rational> get_solution_matrix(filter_nothing_t)
   {
      ListMatrix<Vector<Rational>> facets(0,Q->n);

      lrs_mp_vector_output output(Q->n);
      do {
         for (Int col = 0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col))
               facets /= output.make_Vector(true);
      } while (lrs_getnextbasis (&P, Q, 0));

      return Matrix<Rational>(facets.rows(), facets.cols(), operations::move(), entire(rows(facets)));
   }

   long count_solutions(filter_nothing_t)
   {
      long facets=0;

      lrs_mp_vector_output output(Q->n);
      do {
         for (Int col = 0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col))
               ++facets;
      } while (lrs_getnextbasis (&P, Q, 0));

      return facets;
   }

   Matrix<Rational> get_solution_matrix(filter_rays_t, bool isCone)
   {
      // each vertex is computed only once, but rays can appear multiple times.
      ListMatrix<Vector<Rational>> vertices(0,Q->n);
      hash_set<Vector<Rational>> rays;

      lrs_mp_vector_output output(Q->n);
      do {
         for (Int col = 0; col <= P->d; ++col) {
            if (lrs_getsolution(P, Q, output, col)) {
               if (!mpz_sgn(output.front())) {   // a ray starts with 0
                  rays.insert(output.make_Vector(true));
               } else if (!isCone) {
                  // lrs returns the origin as a vertex for cones
                  // we have to remove this in our interpretation
                  vertices /= output.make_Vector(false);
               }
            }
         }
      } while (lrs_getnextbasis (&P, Q, 0));

      if (isCone)
         return Matrix<Rational>(rays.size(), Q->n, operations::move(), entire(rays));
      else
         return Matrix<Rational>(rays.size()+vertices.rows(), Q->n, operations::move(), entire(rays), entire(rows(vertices)));
   }

   Matrix<Rational> get_linearities()
   {
      const Int m = Q->nredundcol, n = Q->n;
      lrs_mp_matrix_output output(Lin, m, n);
      Lin=nullptr;
      return output.make_Matrix();
   }

   std::pair<long, long> count_solutions(filter_rays_t)
   {
      std::pair<long, long> vertices(0, 0);
      hash_set<Vector<Rational>> rays;

      lrs_mp_vector_output output(Q->n);
      do {
         for (Int col = 0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col)) {
               if (mpz_sgn(output.front()))
                  ++vertices.second;
               else
                  rays.insert(output.make_Vector(true));
            }

      } while (lrs_getnextbasis (&P, Q, 0));

      vertices.first = vertices.second + rays.size();
      return vertices;
   }

   long count_solutions(filter_bounded_t)
   {
      long vertices=0;

      lrs_mp_vector_output output(Q->n);
      do {
         for (Int col = 0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col)) {
               if (mpz_sgn(output.front()))
                  ++vertices;
            }
      } while (lrs_getnextbasis (&P, Q, 0));

      return vertices;
   }

   Matrix<Rational> get_solution_matrix(filter_facets_t)
   {
      hash_set<Vector<Rational>> facets(Q->m * Q->n);

      lrs_mp_vector_output output(Q->n);
      do {
         for (Int col = 0; col <= P->d; ++col)
            if (lrs_getsolution(P, Q, output, col))
               facets.insert(output.make_Vector(true));
      } while (lrs_getnextbasis (&P, Q, 0));

      return Matrix<Rational>(facets.size(), Q->n, operations::move(), entire(facets));
   }
};

convex_hull_result<Rational>
ConvexHullSolver::enumerate_facets(const Matrix<Rational>& Points, const Matrix<Rational>& Lineality, const bool isCone) const
{
   dictionary D(Points, Lineality, false, verbose);
   // we have a polytope if and only if all first coordinates are !=0
   // FIXME find better name, vertex enumeration is the same for cones and polytopes
   D.Q->polytope= isCone || attach_selector(Points.col(0), operations::is_zero()).empty();

   if (!lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1) && !D.Q->nredundcol) throw infeasible();

   Matrix<Rational> AH = isCone ? D.get_linearities().minor(range_from(1), All) : D.get_linearities(); // always lrs returns the functional [1,0,0,0,...]
   Matrix<Rational> F = D.Q->polytope
                        ? D.get_solution_matrix(dictionary::filter_nothing)  // lrs computes facets only once if input is a polytope
                        : D.get_solution_matrix(dictionary::filter_facets);  // FIXME can facets appear several times for unbounded polyhedra?
   // TODO: std::move
   return { F, AH };
}

// FIXME check: why are unbounded polyhedra not allowed?
long
ConvexHullSolver::count_facets(const Matrix<Rational>& Points, const Matrix<Rational>& Lineality, const bool isCone) const
{
   dictionary D(Points, Lineality, verbose);

   // CHECK: the restriction to bounded polyhedra has been added prior to the implementation
   // of convex hull rules for unbounded polyhedra, and apparently the reason is not known anymore
   // there is no documentation of this function in the lrs manual, so if we want to remove this, then someone has to check the code
   if ( !isCone && !attach_selector(Points.col(0), operations::is_zero()).empty())
      throw std::runtime_error("count_facets is not applicable to unbounded polyhedra");

   if (!lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1)) throw infeasible();

   return D.Q->nredundcol+1==D.Q->n
          ? 0       // lrs does not treat the special case of a single point correctly
          : D.count_solutions(dictionary::filter_nothing);
}

convex_hull_result<Rational>
ConvexHullSolver::enumerate_vertices(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations, const bool isCone) const
{
   dictionary D(Inequalities, Equations, true, verbose);

   if (!lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1)) throw infeasible();

   Matrix<Rational> Lineality = D.get_linearities();
   Matrix<Rational> Vertices  = D.get_solution_matrix(dictionary::filter_rays, isCone);

   // TODO: std::move
   return { Vertices, Lineality };
}

ConvexHullSolver::vertex_count
ConvexHullSolver::count_vertices(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations, const bool only_bounded) const
{
   dictionary D(Inequalities, Equations, true, verbose);

   if (!lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1)) throw infeasible();

   vertex_count count;
   count.lineality_dim = D.Q->nredundcol;
   if (only_bounded) {
      count.n_vertices = 0;
      count.n_bounded_vertices = D.count_solutions(dictionary::filter_bounded);
   } else {
      std::tie(count.n_vertices, count.n_bounded_vertices) = D.count_solutions(dictionary::filter_rays);
   }

   return count;
}

//primal or dual
std::pair< Bitset, Matrix<Rational> >
ConvexHullSolver::find_irredundant_representation(const Matrix<Rational>& Points, const Matrix<Rational>& Lineality, const bool dual) const
{
   dictionary D(Points, Lineality, dual, verbose);

   if (!lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1)) throw infeasible();
   const Matrix<Rational> AH=D.get_linearities();

   Bitset V(Points.rows());
   for (Int index = D.Q->lastdv+1, end = D.P->m_A+D.P->d; index <= end; ++index)
      if ( !checkindex(D.P,D.Q,index) )
         V += D.Q->inequality[index - D.Q->lastdv]-1;

   return std::pair< Bitset, Matrix<Rational> >(V,AH);
}

bool LP_Solver::check_feasibility(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations) const
{
   dictionary D(Inequalities, Equations,true);
   return lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1);
}

bool LP_Solver::check_feasibility(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations, Vector<Rational>& ValidPoint) const
{
   dictionary D(Inequalities, Equations,true);

   if (lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1)) {
      lrs_mp_vector_output output(D.Q->n);
      for (Int col = 0; col <= D.P->d; ++col)
         if (lrs_getsolution(D.P, D.Q, output, col)) break;
      ValidPoint = output.make_Vector(false, false);
      return true;
   }
   return false;
}

LP_Solution<Rational>
LP_Solver::solve(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations,
                 const Vector<Rational>& Objective, bool maximize, bool) const
{
   dictionary D(Inequalities, Equations, true);
   D.set_obj_vector(Objective, maximize);

   LP_Solution<Rational> result;
   if (lrs_getfirstbasis(&D.P, D.Q, &D.Lin, 1)) {
      result.lineality_dim = D.Q->nredundcol;
      if (D.Q->unbounded) {
         result.status = LP_status::unbounded;
      } else {
         result.status = LP_status::valid;

         // sometimes there is lineality that fits the objective function but unbounded is not set
         // hence we check all lineality rows that lrs computed manually
         if (result.lineality_dim) {
            Matrix<Rational> lin = D.get_linearities();
            for (auto r = entire(rows(lin)); !r.at_end(); ++r) {
               if (Objective * (*r) != 0) {
                  result.status = LP_status::unbounded;
                  break;
               }
            }
         }

         if (result.status == LP_status::valid) {
            lrs_mp_vector_output output(D.Q->n);
            for (Int col = 0; col <= D.P->d; ++col)
               if (lrs_getsolution(D.P, D.Q, output, col)) break;

            result.objective_value.set(std::move(D.P->objnum), std::move(D.P->objden));
            result.solution = output.make_Vector(false, false);
         }
      }
   } else {
      result.status = LP_status::infeasible;
      result.lineality_dim = 0;
   }

   return result;
}

} } }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
