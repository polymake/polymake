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

#pragma once

#include "polymake/polytope/cdd_interface.h"

// this define means that we want to use gmp rationals in cdd
// but also prefixes all float functions/types with ddf
// for the ddf stuff myfloat should be used instead of mytype
//
// there are a few macros to deal with this below
#define GMPRATIONAL

// cddlib headers:
extern "C" {
#include "setoper.h"
#include "cdd.h"
}

namespace polymake { namespace polytope { namespace cdd_interface {

template <typename Scalar> struct traits;

// this double indirection is needed for the macros to work!
#define CDDRESOLVE(a)      CDDRESOLVE_REAL(a)

#ifdef USE_DD_

#define CDDRESOLVE_REAL(a)  dd_ ## a

template <>
struct traits<Rational> {
   static const CDDRESOLVE(NumberType) number_type=CDDRESOLVE(Rational);

   // PTL -> cddlib
   template <typename Iterator>
   static
   void store(mytype *dst, const Iterator& src)
   {
      CDDRESOLVE(set)(*dst, src->get_rep());
   }

   typedef operations::move getter;
};

#else // use USE_DDF_

#define CDDRESOLVE_REAL(a)  ddf_ ## a
#define mytype             myfloat

template <>
struct traits<double> {
   static const CDDRESOLVE(NumberType) number_type=CDDRESOLVE(Real);

   template <typename Iterator>
   static void store(mytype *dst, const Iterator& src)
   {
      CDDRESOLVE(set_d)(*dst, *src);
   }

   struct getter {
      typedef mytype argument_type;
      typedef double result_type;

      result_type operator() (argument_type& src) const
      {
         return CDDRESOLVE(get_d)(src);
      }
   };
};
#endif

template <typename Scalar>
class vector_output {
public:
   typedef traits<Scalar> traits_t;

   vector_output(mytype* start_arg, Int sz_arg)
      : start(start_arg)
      , sz(sz_arg) {}

   typedef std::input_iterator_tag container_category;
   typedef Scalar value_type;
   typedef value_type reference;
   typedef value_type const_reference;
   typedef pm::unary_transform_iterator<pm::pointer2iterator_t<mytype*>, typename traits_t::getter> iterator;
   typedef iterator const_iterator;

   iterator begin() const { return iterator(start); }
   iterator end() const { return iterator(start+sz); }

private:
   mytype* start;
   Int sz;
};

template <typename Scalar>
class matrix_output_rows_iterator {
public:
   typedef std::input_iterator_tag iterator_category;
   typedef vector_output<Scalar> value_type;
   typedef value_type reference;
   typedef void pointer;
   typedef ptrdiff_t difference_type;

   typedef ListMatrix<Vector<Scalar>> lin_matrix_t;

   matrix_output_rows_iterator(mytype** start, Int n_rows, Int n_cols_arg,
                               dd_rowset lin_set_arg, lin_matrix_t& lin_out_arg)
      : cur(start)
      , end(start+n_rows)
      , n_cols(n_cols_arg)
      , row_index(1)         // cddlib starts counting at 1
      , lin_set(lin_set_arg)
      , lin_out(lin_out_arg)
   {
      valid_position();
   }

   value_type operator* () const
   {
      return value_type(*cur, n_cols);
   }

   matrix_output_rows_iterator& operator++ ()
   {
      ++cur; ++row_index;
      valid_position();
      return *this;
   }

   bool at_end() const { return cur == end; }

private:
   void valid_position()
   {
      while (!at_end() && set_member(row_index, lin_set)) {
         lin_out /= Vector<Scalar>(n_cols, typename value_type::iterator(*cur));
         ++cur;
         ++row_index;
      }
   }

   mytype** cur;
   mytype** end;
   Int n_cols;
   Int row_index;
   dd_rowset lin_set;
   lin_matrix_t& lin_out;
};


template <typename Scalar> class cdd_polyhedron;
template <typename Scalar> class cdd_lp;
template <typename Scalar> class cdd_lp_sol;
template <typename Scalar> class cdd_matrix;

template <typename Scalar>
class cdd_vector : public traits<Scalar> {
   typedef traits<Scalar> traits_t;

   friend class cdd_matrix<Scalar>;
public:
   explicit cdd_vector(Int dim_arg) : dim(dim_arg)
   {
      CDDRESOLVE(InitializeArow)(dim, &ptr);
   }

   ~cdd_vector() { CDDRESOLVE(FreeArow)(dim, ptr); }

   Vector<Scalar> get(Int start_at = 0) const
   {
      Vector<Scalar> result(dim-start_at, typename vector_output<Scalar>::iterator(ptr+start_at));
      for (mytype *cur=ptr+start_at, *end=ptr+dim; cur != end; ++cur)
         CDDRESOLVE(init)(*cur);
      return result;
   }

private:
   Int dim;
   CDDRESOLVE(Arow) ptr;
};

template <typename Scalar>
class cdd_matrix : public traits<Scalar> {
   using traits_t = traits<Scalar>;

   friend class cdd_polyhedron<Scalar>;
   friend class cdd_lp<Scalar>;
   friend class ConvexHullSolver<Scalar>;
   friend class LP_Solver<Scalar>;
public:
   // build from points and rays alone
   cdd_matrix(const Matrix<Scalar>& P);

   // build from inequalities and equations or rays and lineality
   cdd_matrix(const Matrix<Scalar>& I, const Matrix<Scalar>& E, const representation source_rep);

   // build from computed convex hull
   cdd_matrix(const cdd_polyhedron<Scalar>&, const representation target_rep);

   ~cdd_matrix() { CDDRESOLVE(FreeMatrix)(ptr); }

   void add_objective(const Vector<Scalar>& obj, bool maximize);

   convex_hull_result<Scalar> representation_conversion(const bool isCone, const representation target_rep) const;

   ListMatrix< Vector<Scalar> > vertex_normals(Bitset& Vertices);

   std::pair<Bitset, Set<Int>> canonicalize();

   void canonicalize_lineality(Bitset& Lin);

private:
   CDDRESOLVE(MatrixPtr) ptr;
   const Int num_rays_ineqs;
};

template <typename Scalar>
class cdd_polyhedron {
   friend class cdd_matrix<Scalar>;
   friend class ConvexHullSolver<Scalar>;
public:
   cdd_polyhedron(const cdd_matrix<Scalar>& M)
      : ptr(CDDRESOLVE(DDMatrix2Poly)(M.ptr, &err)) {}
   ~cdd_polyhedron() { CDDRESOLVE(FreePolyhedra)(ptr); }
   void verify();
protected:
   CDDRESOLVE(PolyhedraPtr) ptr;
   CDDRESOLVE(ErrorType) err;
};

template <typename Scalar>
class cdd_lp : public traits<Scalar> {
   typedef traits<Scalar> traits_t;
   friend class cdd_lp_sol<Scalar>;
public:
   cdd_lp(const cdd_matrix<Scalar>& M)
      : ptr(CDDRESOLVE(Matrix2LP)(M.ptr, &err)) {}
   ~cdd_lp() { CDDRESOLVE(FreeLPData)(ptr); }

   Vector<Scalar> optimal_vertex() const;
protected:
   CDDRESOLVE(LPPtr) ptr;
   CDDRESOLVE(ErrorType) err;
   CDDRESOLVE(LPSolutionPtr) get_solution();
};

template <typename Scalar>
class cdd_lp_sol {
public:
   cdd_lp_sol(cdd_lp<Scalar>& L) : ptr(L.get_solution()) {}
   ~cdd_lp_sol() { CDDRESOLVE(FreeLPSolution)(ptr); }

   Scalar optimal_value() const;
   LP_status get_status(bool throw_when_bad_dual) const;
protected:
   CDDRESOLVE(LPSolutionPtr) ptr;
};

class cdd_bitset;

class cdd_bitset_iterator {
   friend class cdd_bitset;
public:
   typedef std::forward_iterator_tag iterator_category;
   typedef Int value_type;
   typedef const Int& reference;
   typedef const Int* pointer;
   typedef ptrdiff_t difference_type;

   typedef cdd_bitset_iterator iterator;
   typedef cdd_bitset_iterator const_iterator;

   cdd_bitset_iterator() {}

protected:
   explicit cdd_bitset_iterator(set_type s_arg) :
      s(s_arg+1), cur(0), end(set_groundsize(s_arg)), bit(1)
   {
      valid_position();
   }

   cdd_bitset_iterator(set_type s_arg, bool /* _end */) :
      s(s_arg+1), cur(set_groundsize(s_arg)), end(cur), bit(0) {}

public:
   reference operator* () const { return cur; }

   cdd_bitset_iterator& operator++ ()
   {
      incr();
      valid_position();
      return *this;
   }

   const cdd_bitset_iterator operator++ (int) { cdd_bitset_iterator copy(*this); operator++(); return copy; }

   bool at_end() const { return cur>=end; }

   bool operator== (const cdd_bitset_iterator& it) const { return cur==it.cur; }
   bool operator!= (const cdd_bitset_iterator& it) const { return cur!=it.cur; }

protected:
   void valid_position()
   {
      while (!at_end() && !(*s & bit)) incr();
   }

   void incr()
   {
      ++cur;
      if (!(bit <<= 1)) {
         ++s;
         bit=1;
      }
   }

   set_type s;
   Int cur, end;
   unsigned long bit;
};

class cdd_bitset : public GenericSet<cdd_bitset, Int, pm::operations::cmp> {
   template <typename> friend class cdd_matrix;
public:
   // CAUTION: sets are initialized within cddlib functions.
   cdd_bitset() : ptr(nullptr) {}
   ~cdd_bitset() { set_free(ptr); }

   Int size() const { return set_card(ptr); }
   bool empty() const { return size()==0; }
   Int dim() const { return set_groundsize(ptr); }

   typedef Int value_type;
   typedef Int reference;
   typedef Int const_reference;
   typedef cdd_bitset_iterator iterator;
   typedef iterator const_iterator;

   iterator begin() const { return iterator(ptr); }
   iterator end() const { return iterator(ptr,true); }

   reference front() const { return *begin(); }
protected:
   set_type ptr;
};

} } }
namespace pm {

template <>
struct check_iterator_feature<polymake::polytope::cdd_interface::cdd_bitset_iterator, end_sensitive> : std::true_type {};

template <>
struct check_container_feature<polymake::polytope::cdd_interface::cdd_bitset, sparse_compatible> : std::true_type {};

template <typename Scalar>
struct check_iterator_feature<polymake::polytope::cdd_interface::matrix_output_rows_iterator<Scalar>, end_sensitive> : std::true_type {};

template <typename Scalar>
struct spec_object_traits<polymake::polytope::cdd_interface::vector_output<Scalar>> : spec_object_traits<is_container> {};

}
namespace polymake { namespace polytope { namespace cdd_interface {

// FIXME: method currently used for RAY_SEPARATORS/VERTEX_NORMALS from RAYS
//        for this computation LINEALITY_SPACE is not needed
// should not be used for INEQUALITES (set representation type)
template <typename Scalar>
cdd_matrix<Scalar>::cdd_matrix(const Matrix<Scalar>& P)
   : ptr(CDDRESOLVE(CreateMatrix)(P.rows(), P.cols()))
   , num_rays_ineqs(P.rows())
{
   // get size of the input matrix
   Int m = P.rows();
   Int n = P.cols();

   ptr->representation = CDDRESOLVE(Generator);    // Input type: points
   ptr->numbtype = traits_t::number_type;

   // copy data: polymake -> cdd
   auto p=concat_rows(P).begin();
   for (mytype **r=ptr->matrix, **rend=r+m;  r!=rend;  ++r) {
      mytype *c=*r;
      mytype *cend=c+n;
      for (;  c!=cend;  ++c, ++p)
         traits_t::store(c, p);
   }
}

template <typename Scalar>
cdd_matrix<Scalar>::cdd_matrix(const Matrix<Scalar>& I, const Matrix<Scalar>& E, const representation source_rep)
   : ptr(CDDRESOLVE(CreateMatrix)(I.rows() + E.rows(), I.cols() ? I.cols() : E.cols()))
   , num_rays_ineqs(I.rows())
{
   // get size of the input matrix
   Int mi = I.rows();
   Int me = E.rows();
   Int n  = I.cols() ? I.cols() : E.cols();

   // avoid segfault in some cdd versions for degenerate cases
   if (n == 0) {
      CDDRESOLVE(FreeMatrix)(ptr);
      throw std::runtime_error("cdd_interface - cannot properly handle ambient dimension 0");
   }

   if (source_rep == representation::H)
      ptr->representation = CDDRESOLVE(Inequality);   // Input type: inequalities
   else
      ptr->representation = CDDRESOLVE(Generator);    // Input type: points
   ptr->numbtype = traits_t::number_type;

   auto p=concat_rows(I).begin();
   mytype **r, **rend;
   for (r=ptr->matrix, rend=r+mi;  r!=rend;  ++r) {
      mytype *c=*r;
      mytype *cend=c+n;
      for (;  c!=cend;  ++c, ++p)
         traits_t::store(c,p);
   }

   p=concat_rows(E).begin();
   ++mi; // cddlib starts counting at 1
   for (rend+=me;  r!=rend;  ++r, ++mi) {
      mytype *c=*r;
      mytype *cend=c+n;
      for (;  c!=cend;  ++c, ++p)
         traits_t::store(c,p);
      set_addelem(ptr->linset, mi);
   }
}

template <typename Scalar>
void cdd_matrix<Scalar>::add_objective(const Vector<Scalar>& obj, bool maximize)
{
   auto o=obj.begin();
   for (mytype *r=ptr->rowvec, *rend=r+obj.size();  r!=rend;  ++r, ++o)
      traits_t::store(r,o);
   ptr->objective= maximize ? CDDRESOLVE(LPmax) : CDDRESOLVE(LPmin);
}

template <typename Scalar>
void cdd_polyhedron<Scalar>::verify()
{
   if (err != CDDRESOLVE(NoError)) {
      std::ostringstream err_msg;
      err_msg << "Error in dd_DDMatrix2Poly: " << err << endl;
      throw std::runtime_error(err_msg.str());
   }
}

template <typename Scalar>
CDDRESOLVE(LPSolutionPtr) cdd_lp<Scalar>::get_solution()
{
   if (err != CDDRESOLVE(NoError)) {
      std::ostringstream err_msg;
      err_msg << "Error in dd_Matrix2LP: " << err << endl;
      throw std::runtime_error(err_msg.str());
   }

   if (! CDDRESOLVE(LPSolve)(ptr, CDDRESOLVE(DualSimplex), &err)) {
      std::ostringstream err_msg;
      err_msg << "Error in dd_LPSolve: " << err << endl;
      throw std::runtime_error(err_msg.str());
   }

   return CDDRESOLVE(CopyLPSolution)(ptr);
}

template <typename Scalar>
cdd_matrix<Scalar>::cdd_matrix(const cdd_polyhedron<Scalar>& P, const representation target_rep)
   : ptr(target_rep == representation::H ? CDDRESOLVE(CopyInequalities)(P.ptr) : CDDRESOLVE(CopyGenerators)(P.ptr))
   , num_rays_ineqs(0) { }

// extracts the dual description from a cdd matrix
// and makes some adjustements for polymake needs
template <typename Scalar>
convex_hull_result<Scalar>
cdd_matrix<Scalar>::representation_conversion(const bool isCone, const representation target_rep) const
{
   const CDDRESOLVE(rowrange) m = ptr->rowsize;
   const CDDRESOLVE(colrange) n = ptr->colsize;
   const long linsize = set_card(ptr->linset);

   if (target_rep == representation::V && m<=0) throw infeasible();

   ListMatrix<Vector<Scalar>> Lin(0, n);
   Matrix<Scalar> Pt(m-linsize, n, matrix_output_rows_iterator<Scalar>(ptr->matrix, m, n, ptr->linset, Lin));

   if (target_rep == representation::V) {
      // for a 0-dimensional cone cdd returns the origin as a vertex
      // we have to remove this in our interpretation
      if (isCone && !linsize && Pt.rows()==1 && Pt(0,0)==1)
         Pt.resize(0, Pt.cols());

      // in the case of a homogeneous cone, the cddlib omits the origin point
      // in the case of a cone we don't want to add this!
      if (!linsize && is_zero(Pt.col(0)) && !isCone)
         Pt /= unit_vector<Scalar>(Pt.cols(), 0);
   }

   return { Pt, Matrix<Scalar>(linsize, n, operations::move(), entire(rows(Lin))) };
}


template <typename Scalar>
ListMatrix< Vector<Scalar> >
cdd_matrix<Scalar>::vertex_normals(Bitset& Vertices)
{
   ListMatrix< Vector<Scalar> > VN(0, ptr->colsize+1);
   auto vn_front=rows(VN).begin();
   cdd_vector<Scalar> cert(ptr->colsize+1);
   CDDRESOLVE(ErrorType) err;
   for (Int i = ptr->rowsize; i >= 1; --i) {
      const bool is_redundant=CDDRESOLVE(Redundant)(ptr, i, cert.ptr, &err);
      if (err != CDDRESOLVE(NoError)) {
         std::ostringstream err_msg;
         err_msg << "Error in dd_Redundant: " << err << endl;
         throw std::runtime_error(err_msg.str());
      }
      if (is_redundant) {
         CDDRESOLVE(MatrixRowRemove)(&ptr, i);
      } else {
         Vertices += i-1;
         Vector<Scalar> vertex_normal=cert.get(1);
         //  we redefined vertex normals to be affine functionals
         //  vertex_normal[0]=0;
         // vertex_normal.negate();
         // TODO: when move constructors for Vector are implemented,
         // std::move(vertex_normal) or pass the return value directly
         vn_front=VN.insert_row(vn_front, vertex_normal);
      }
   }
   return VN;
}


template <typename Scalar>
std::pair<Bitset, Set<Int>> cdd_matrix<Scalar>::canonicalize()
{
   cdd_bitset impl_linset, redset;
   CDDRESOLVE(rowindex) newpos;
   CDDRESOLVE(ErrorType) err;
   const CDDRESOLVE(rowrange) m = ptr->rowsize;

   const bool success=CDDRESOLVE(MatrixCanonicalize)(&ptr, &impl_linset.ptr, &redset.ptr, &newpos, &err);

   if (!success || err != CDDRESOLVE(NoError)) {
      std::ostringstream err_msg;
      err_msg << "Error in dd_MatrixCanonicalize: " << err << endl;
      throw std::runtime_error(err_msg.str());
   }

   std::pair<Bitset, Set<Int>> result{ Bitset(num_rays_ineqs), Set<Int>() };
   const long linsize = set_card(ptr->linset);
   for (Int i = 1; i <= m; ++i) {
      if (newpos[i] > 0) {
         if (newpos[i] <= linsize)
            result.second += i-1;
         else
            result.first += i-1;
      }
   }

   free(newpos);
   return result;
}


template <typename Scalar>
void
cdd_matrix<Scalar>::canonicalize_lineality(Bitset& Lin)
{
   cdd_bitset impl_linset;
   CDDRESOLVE(rowindex) newpos;
   CDDRESOLVE(ErrorType) err;
   const CDDRESOLVE(rowrange) m = ptr->rowsize;

   const bool success=CDDRESOLVE(MatrixCanonicalizeLinearity)(&ptr, &impl_linset.ptr, &newpos, &err);

   if (!success || err != CDDRESOLVE(NoError)) {
      std::ostringstream err_msg;
      err_msg << "Error in dd_MatrixCanonicalizeLinearity: " << err << endl;
      throw std::runtime_error(err_msg.str());
   }

   const long linsize = set_card(ptr->linset);

   for (Int i = 1; i <= m; ++i )
      if ( newpos[i] > 0 && newpos[i] <= linsize )
         Lin += i-1;

   free(newpos);
}

template <typename Scalar>
LP_status cdd_lp_sol<Scalar>::get_status(bool throw_when_bad_dual) const
{
   switch (ptr->LPS) {
   case CDDRESOLVE(Optimal):
      return LP_status::valid;
   case CDDRESOLVE(StrucInconsistent):
   case CDDRESOLVE(Inconsistent):
      return LP_status::infeasible;
   case CDDRESOLVE(Unbounded):
      return LP_status::unbounded;
   case CDDRESOLVE(StrucDualInconsistent):
   case CDDRESOLVE(DualInconsistent):
   case CDDRESOLVE(DualUnbounded):
      if (throw_when_bad_dual)
         throw infeasible();
      return LP_status::infeasible;
   default:
      std::ostringstream err_msg;
      err_msg << "cannot handle lp solution: cdd returned: " << ptr->LPS;
      throw std::runtime_error(err_msg.str());
   }
}

template <typename Scalar>
Vector<Scalar> cdd_lp<Scalar>::optimal_vertex() const
{
   return Vector<Scalar>(ptr->d, typename vector_output<Scalar>::iterator(ptr->sol));
}

// end of cdd classes
//  implementation of ConvexHullSolver methods

template <typename Scalar>
convex_hull_result<Scalar>
ConvexHullSolver<Scalar>::enumerate_facets(const Matrix<Scalar>& Points, const Matrix<Scalar>& Lineality, const bool isCone) const
{
   if(Points.rows() == 0 && Lineality.rows() == 0){
      // We deal with the trivial case here, since cdd fails for being called with two empty matrices.
      return convex_hull_result<Scalar>(Matrix<Scalar>(0,Points.cols()), Matrix<Scalar>(unit_matrix<Scalar>(Points.cols())));
   }
   dd_debug = verbose ? dd_TRUE : dd_FALSE;
   cdd_matrix<Scalar> IN(Points, Lineality, representation::V);
   cdd_polyhedron<Scalar> P(IN);
   dd_debug = dd_FALSE;
   P.verify();
   return cdd_matrix<Scalar>(P, representation::H).representation_conversion(isCone, representation::H);
}

template <typename Scalar>
convex_hull_result<Scalar>
ConvexHullSolver<Scalar>::enumerate_vertices(const Matrix<Scalar>& Inequalities, const Matrix<Scalar>& Equations, const bool isCone) const
{
   dd_debug = verbose ? dd_TRUE : dd_FALSE;
   cdd_matrix<Scalar> IN(Inequalities, Equations, representation::H);
   cdd_polyhedron<Scalar> P(IN);
   dd_debug = dd_FALSE;
   P.verify();
   return cdd_matrix<Scalar>(P, representation::V).representation_conversion(isCone, representation::V);
}

template <typename Scalar>
std::pair<Bitset, Set<Int>>
ConvexHullSolver<Scalar>::get_non_redundant_points(const Matrix<Scalar>& Pt, const Matrix<Scalar>& Lin, bool /* isCone */) const
{
   cdd_matrix<Scalar> IN(Pt, Lin, representation::V);
   // in some cases cdd does not return the facet x_0>=0 for the primal reduction
   // so we always use the dual reduction
   // FIXME this has to be checked carefully
   return IN.canonicalize();
}

template <typename Scalar>
std::pair<Bitset, Set<Int>>
ConvexHullSolver<Scalar>::get_non_redundant_inequalities(const Matrix<Scalar>& Ineq, const Matrix<Scalar>& Eq, bool /* isCone */) const
{
   cdd_matrix<Scalar> IN(Ineq, Eq, representation::H);
   return IN.canonicalize();
}

template <typename Scalar>
Bitset
ConvexHullSolver<Scalar>::canonicalize_lineality(const Matrix<Scalar>& Pt, const Matrix<Scalar>& Lin, const representation source_rep)
{
   cdd_matrix<Scalar> IN(Pt, Lin, source_rep);
   Bitset red_Lin(Pt.rows());
   IN.canonicalize_lineality(red_Lin);
   return red_Lin;
}

// FIXME Why is there no dual version of this?
template <typename Scalar>
typename ConvexHullSolver<Scalar>::non_redundant
ConvexHullSolver<Scalar>::find_vertices_among_points(const Matrix<Scalar>& Points)
{
   cdd_matrix<Scalar> IN(Points);
   Bitset Vertices(Points.rows());
   return non_redundant(Vertices, IN.vertex_normals(Vertices));
}

template <typename Scalar>
LP_Solution<Scalar>
LP_Solver<Scalar>::solve(const Matrix<Scalar>& Inequalities, const Matrix<Scalar>& Equations,
                         const Vector<Scalar>& Objective, bool maximize, bool feasibility_known) const
{
   LP_Solution<Scalar> result;
   try {
      cdd_matrix<Scalar> IN(Inequalities, Equations, representation::H);
      IN.add_objective(Objective, maximize);
      cdd_lp<Scalar> LP(IN);
      cdd_lp_sol<Scalar> Sol(LP);
      result.status = Sol.get_status(true);
      if (result.status == LP_status::valid) {
         result.objective_value = Sol.optimal_value();
         result.solution = LP.optimal_vertex();
      }
   }
   catch (const infeasible&) {
      // could not figure out whether the problem is infeasible or unbounded, retry with a trivial objective function
      if (feasibility_known) {
         result.status = Inequalities.rows() == 0 ? LP_status::infeasible : LP_status::unbounded;
      } else {
         cdd_matrix<Scalar> IN(Inequalities, Equations, representation::H);
         const Vector<Scalar> unit = unit_vector<Scalar>(Inequalities.cols(), 0);
         IN.add_objective(unit, true);
         cdd_lp<Scalar> LP(IN);
         cdd_lp_sol<Scalar> Sol(LP);
         result.status = Sol.get_status(false) == LP_status::infeasible ? LP_status::infeasible : LP_status::unbounded;
      }
   }
   return result;
}

} } }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
