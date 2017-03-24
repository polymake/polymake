/* Copyright (c) 1997-2017
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

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

#ifndef POLYMAKE_POLYTOPE_CDD_INTERFACE_IMPL_H
#define POLYMAKE_POLYTOPE_CDD_INTERFACE_IMPL_H

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

template <typename Coord> struct traits;

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

template <typename Coord>
class vector_output {
public:
   typedef traits<Coord> traits_t;

   vector_output(mytype* start_arg, int sz_arg)
      : start(start_arg)
      , sz(sz_arg) {}

   typedef std::input_iterator_tag container_category;
   typedef Coord value_type;
   typedef value_type reference;
   typedef value_type const_reference;
   typedef pm::unary_transform_iterator<pm::pointer2iterator_t<mytype*>, typename traits_t::getter> iterator;
   typedef iterator const_iterator;

   iterator begin() const { return iterator(start); }
   iterator end() const { return iterator(start+sz); }

private:
   mytype* start;
   int sz;
};

template <typename Coord>
class matrix_output_rows_iterator {
public:
   typedef std::input_iterator_tag iterator_category;
   typedef vector_output<Coord> value_type;
   typedef value_type reference;
   typedef void pointer;
   typedef ptrdiff_t difference_type;

   typedef ListMatrix<Vector<Coord>> lin_matrix_t;

   matrix_output_rows_iterator(mytype** start, int n_rows, int n_cols_arg,
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
         lin_out /= Vector<Coord>(n_cols, typename value_type::iterator(*cur));
         ++cur;
         ++row_index;
      }
   }

   mytype** cur;
   mytype** end;
   int n_cols;
   int row_index;
   dd_rowset lin_set;
   lin_matrix_t& lin_out;
};


template <typename Coord> class cdd_polyhedron;
template <typename Coord> class cdd_lp;
template <typename Coord> class cdd_lp_sol;
template <typename Coord> class cdd_matrix;

template <typename Coord>
class cdd_vector : public traits<Coord> {
   typedef traits<Coord> traits_t;

   friend class cdd_matrix<Coord>;
public:
   explicit cdd_vector(int dim_arg) : dim(dim_arg)
   {
      CDDRESOLVE(InitializeArow)(dim, &ptr);
   }

   ~cdd_vector() { CDDRESOLVE(FreeArow)(dim, ptr); }

   Vector<Coord> get(int start_at=0) const
   {
      Vector<Coord> result(dim-start_at, typename vector_output<Coord>::iterator(ptr+start_at));
      for (mytype *cur=ptr+start_at, *end=ptr+dim; cur != end; ++cur)
         CDDRESOLVE(init)(*cur);
      return result;
   }

private:
   int dim;
   CDDRESOLVE(Arow) ptr;
};

template <typename Coord>
class cdd_matrix : public traits<Coord> {
   typedef traits<Coord> traits_t;

   friend class cdd_polyhedron<Coord>;
   friend class cdd_lp<Coord>;
   friend class solver<Coord>;
public:
   // build from points and rays alone
   cdd_matrix(const Matrix<Coord>& P);
   // build from inequalities and equations or rays and lineality
   // primal <-> build from inequalities and equations ( seems to be the definition ... )
   cdd_matrix(const Matrix<Coord>& I, const Matrix<Coord>& E, const bool primal = true);
   // build from computed convex hull
   cdd_matrix(const cdd_polyhedron<Coord>&, bool primal);

   ~cdd_matrix() { CDDRESOLVE(FreeMatrix)(ptr); }

   void add_objective(const Vector<Coord>& obj, bool maximize);

   typename solver<Coord>::matrix_pair representation_conversion(const bool isCone = false, const bool primal = true) const;

   ListMatrix< Vector<Coord> > vertex_normals(Bitset& Vertices);

   void canonicalize(Bitset& Pt, Bitset& Lin);

   void canonicalize_lineality(Bitset& Lin);

private:
   CDDRESOLVE(MatrixPtr) ptr;
};

template <typename Coord>
class cdd_polyhedron {
   friend class cdd_matrix<Coord>;
   friend class solver<Coord>;
public:
   cdd_polyhedron(const cdd_matrix<Coord>& M) : ptr(CDDRESOLVE(DDMatrix2Poly)(M.ptr, &err)) {}
   ~cdd_polyhedron() { CDDRESOLVE(FreePolyhedra)(ptr); }
   void verify();
protected:
   CDDRESOLVE(PolyhedraPtr) ptr;
   CDDRESOLVE(ErrorType) err;
};

template <typename Coord>
class cdd_lp : public traits<Coord> {
   typedef traits<Coord> traits_t;
   friend class cdd_lp_sol<Coord>;
public:
   cdd_lp(const cdd_matrix<Coord>& M) : ptr(CDDRESOLVE(Matrix2LP)(M.ptr, &err)) {}
   ~cdd_lp() { CDDRESOLVE(FreeLPData)(ptr); }

   Vector<Coord> optimal_vertex() const;
protected:
   CDDRESOLVE(LPPtr) ptr;
   CDDRESOLVE(ErrorType) err;
   CDDRESOLVE(LPSolutionPtr) get_solution();
};

template <typename Coord>
class cdd_lp_sol {
public:
   void verify();
   cdd_lp_sol(cdd_lp<Coord>& L) : ptr(L.get_solution()) {}
   ~cdd_lp_sol() { CDDRESOLVE(FreeLPSolution)(ptr); }

   Coord optimal_value() const;
protected:
   CDDRESOLVE(LPSolutionPtr) ptr;
};

class cdd_bitset;

class cdd_bitset_iterator {
   friend class cdd_bitset;
public:
   typedef std::forward_iterator_tag iterator_category;
   typedef int value_type;
   typedef const int& reference;
   typedef const int* pointer;
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
   int cur, end;
   unsigned long bit;
};

class cdd_bitset : public GenericSet<cdd_bitset, int, pm::operations::cmp> {
   template <typename> friend class cdd_matrix;
public:
   // CAUTION: sets are initialized within cddlib functions.
   cdd_bitset() : ptr(0) {}
   ~cdd_bitset() { set_free(ptr); }

   int size() const { return set_card(ptr); }
   bool empty() const { return size()==0; }
   int dim() const { return set_groundsize(ptr); }

   typedef int value_type;
   typedef int reference;
   typedef int const_reference;
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

template <typename Coord>
struct check_iterator_feature<polymake::polytope::cdd_interface::matrix_output_rows_iterator<Coord>, end_sensitive> : std::true_type {};

template <typename Coord>
struct spec_object_traits<polymake::polytope::cdd_interface::vector_output<Coord>> : spec_object_traits<is_container> {};

}
namespace polymake { namespace polytope { namespace cdd_interface {

// FIXME: method currently used for RAY_SEPARATORS/VERTEX_NORMALS from RAYS
//        for this computation LINEALITY_SPACE is not needed
// should not be used for INEQUALITES (set representation type)
template <typename Coord>
cdd_matrix<Coord>::cdd_matrix(const Matrix<Coord>& P)
   : ptr(CDDRESOLVE(CreateMatrix)(P.rows(), P.cols()))
{
   // get size of the input matrix
   int m = P.rows();
   int n = P.cols();

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

template <typename Coord>
cdd_matrix<Coord>::cdd_matrix(const Matrix<Coord>& I, const Matrix<Coord>& E, const bool primal)
   : ptr(CDDRESOLVE(CreateMatrix)(I.rows() + E.rows(),  ( I.cols() | E.cols() ) ) ) {

   // get size of the input matrix
   int mi = I.rows();
   int me = E.rows();
   int n  = I.cols() | E.cols();

   // avoid segfault in some cdd versions for degenerate cases
   if (n == 0) {
      CDDRESOLVE(FreeMatrix)(ptr);
      throw std::runtime_error("cdd_interface - cannot properly handle ambient dimension 0");
   }

   if ( primal )
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

template <typename Coord>
void cdd_matrix<Coord>::add_objective(const Vector<Coord>& obj, bool maximize)
{
   auto o=obj.begin();
   for (mytype *r=ptr->rowvec, *rend=r+obj.size();  r!=rend;  ++r, ++o)
      traits_t::store(r,o);
   ptr->objective= maximize ? CDDRESOLVE(LPmax) : CDDRESOLVE(LPmin);
}

template <typename Coord>
void cdd_polyhedron<Coord>::verify()
{
   if (err != CDDRESOLVE(NoError)) {
      std::ostringstream err_msg;
      err_msg << "Error in dd_DDMatrix2Poly: " << err << endl;
      throw std::runtime_error(err_msg.str());
   }
}

template <typename Coord>
CDDRESOLVE(LPSolutionPtr) cdd_lp<Coord>::get_solution()
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

template <typename Coord>
cdd_matrix<Coord>::cdd_matrix(const cdd_polyhedron<Coord>& P, bool primal)
   : ptr(primal ? CDDRESOLVE(CopyInequalities)(P.ptr) : CDDRESOLVE(CopyGenerators)(P.ptr)) { }

// extracts the dual description from a cdd matrix
// and makes some adjustements for polymake needs
// primal <-> input representation by inequalities/equations
template <typename Coord>
typename solver<Coord>::matrix_pair
cdd_matrix<Coord>::representation_conversion(const bool isCone, const bool primal ) const
{
   const CDDRESOLVE(rowrange) m = ptr->rowsize;
   const CDDRESOLVE(colrange) n = ptr->colsize;
   const long linsize = set_card(ptr->linset);

   if (primal && m<=0) throw infeasible();

   ListMatrix<Vector<Coord>> Lin(0, n);
   Matrix<Coord> Pt(m-linsize, n, matrix_output_rows_iterator<Coord>(ptr->matrix, m, n, ptr->linset, Lin));

   if (primal) {
      // for a 0-dimensional cone cdd returns the origin as a vertex
      // we have to remove this in our interpretation
      if (isCone && !linsize && Pt.rows()==1 && Pt(0,0)==1)
         Pt.resize(0, Pt.cols());

      // in the case of a homogeneous cone, the cddlib omits the origin point
      // in the case of a cone we don't want to add this!
      if (!linsize && is_zero(Pt.col(0)) && !isCone)
         Pt /= unit_vector<Coord>(Pt.cols(), 0);
   }

   return typename solver<Coord>::matrix_pair(Pt, Matrix<Coord>(linsize, n, operations::move(), entire(rows(Lin))));
}


template <typename Coord>
ListMatrix< Vector<Coord> >
cdd_matrix<Coord>::vertex_normals(Bitset& Vertices)
{
   ListMatrix< Vector<Coord> > VN;
   auto vn_front=rows(VN).begin();
   cdd_vector<Coord> cert(ptr->colsize+1);
   CDDRESOLVE(ErrorType) err;
   for (int i=ptr->rowsize; i>=1; --i) {
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
         Vector<Coord> vertex_normal=cert.get(1);
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


template <typename Coord>
void
cdd_matrix<Coord>::canonicalize(Bitset& Pt, Bitset& Lin) {
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

   const long linsize = set_card(ptr->linset);

   for (int i = 1; i <= m; ++i )
      if ( newpos[i] > 0 ) {
         if ( newpos[i] <= linsize )
            Lin += i-1;
         else
            Pt += i-1;
      }
   free(newpos);
}


template <typename Coord>
void
cdd_matrix<Coord>::canonicalize_lineality(Bitset& Lin) {
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

   for (int i = 1; i <= m; ++i )
      if ( newpos[i] > 0 && newpos[i] <= linsize )
         Lin += i-1;

   free(newpos);
}

template <typename Coord>
void cdd_lp_sol<Coord>::verify()
{
   switch (ptr->LPS) {
   case CDDRESOLVE(Optimal):
       break;
   case CDDRESOLVE(StrucInconsistent):   // if the problem is inconsistent then mark that there is no valid point
   case CDDRESOLVE(Inconsistent):
      throw infeasible();
   case CDDRESOLVE(Unbounded):      // if the problem is unbounded then mark that there is no optimal solution
      throw unbounded();
   case CDDRESOLVE(StrucDualInconsistent):  // if the dual problem is either unbounded or
   case CDDRESOLVE(DualUnbounded):          // inconsistent we don't know whether the primal
   case CDDRESOLVE(DualInconsistent):       // is inconsistent or unbounded
      throw baddual("cannot handle lp solution: problem is either inconsistent or unbounded");
   default:  // all other cases are handled here ....
      std::ostringstream err_msg;
      err_msg << "cannot handle lp solution: cdd returned: " << ptr->LPS;
      throw std::runtime_error(err_msg.str());
   }
}

template <typename Coord>
Vector<Coord> cdd_lp<Coord>::optimal_vertex() const
{
   return Vector<Coord>(ptr->d, typename vector_output<Coord>::iterator(ptr->sol));
}

// end of cdd classes
//  implementation of solver methods

// count how many solver instances are active to make sure that they are initialized and freed correctly
// the variable lives in cdd_interface.cc
extern int solver_count;

// we only use the dd_ variant as this allocates both the dd_ and ddf_ global variables
// also to make sure there are no problems when only some are freed by the ddf_free function
template <typename Coord>
solver<Coord>::solver()
{
   if (solver_count++ == 0)
      dd_set_global_constants();
}

template <typename Coord>
solver<Coord>::~solver()
{
   if (--solver_count == 0)
      dd_free_global_constants();
}

// this and the following function are identical
// we still need both for consistency with lrs
template <typename Coord>
typename solver<Coord>::matrix_pair
solver<Coord>::enumerate_facets(const Matrix<Coord>& Points, const Matrix<Coord>& Lineality, const bool isCone, const bool primal)
{
   cdd_matrix<Coord> IN(Points, Lineality, primal);
   cdd_polyhedron<Coord> P(IN);
   P.verify();
   return cdd_matrix<Coord>(P, !primal).representation_conversion(isCone,primal);
}

template <typename Coord>
typename solver<Coord>::matrix_pair
solver<Coord>::enumerate_vertices(const Matrix<Coord>& Inequalities, const Matrix<Coord>& Equations, const bool isCone, const bool primal )
{
   cdd_matrix<Coord> IN(Inequalities, Equations, primal);
   cdd_polyhedron<Coord> P(IN);
   P.verify();
   return cdd_matrix<Coord>(P, !primal).representation_conversion(isCone,primal);
}

template <typename Coord>
typename solver<Coord>::non_redundant_canonical
solver<Coord>::canonicalize(const Matrix<Coord>& Pt, const Matrix<Coord>& Lin, bool primal)
{
   cdd_matrix<Coord> IN(Pt, Lin, false);  // in some cases cdd does not return the facet x_0>=0 for the primal reduction
                                          // so we always use the dual reduction
                                          // FIXME this has to be checked carefully
   Bitset red_Pt(Pt.rows());
   Bitset red_Lin(Pt.rows());
   IN.canonicalize(red_Pt,red_Lin);
   return non_redundant_canonical(red_Pt,red_Lin);
}

template <typename Coord>
Bitset
solver<Coord>::canonicalize_lineality(const Matrix<Coord>& Pt, const Matrix<Coord>& Lin, bool primal)
{
   cdd_matrix<Coord> IN(Pt, Lin, primal);
   Bitset red_Lin(Pt.rows());
   IN.canonicalize_lineality(red_Lin);
   return red_Lin;
}

// FIXME Why is there no dual version of this?
template <typename Coord>
typename solver<Coord>::non_redundant
solver<Coord>::find_vertices_among_points(const Matrix<Coord>& Points)
{
   cdd_matrix<Coord> IN(Points);
   Bitset Vertices(Points.rows());
   return non_redundant(Vertices, IN.vertex_normals(Vertices));
}

template <typename Coord>
typename solver<Coord>::lp_solution
solver<Coord>::solve_lp(const Matrix<Coord>& Inequalities, const Matrix<Coord>& Equations,
                        const Vector<Coord>& Objective, bool maximize)
{
   cdd_matrix<Coord> IN(Inequalities, Equations);
   IN.add_objective(Objective, maximize);
   cdd_lp<Coord> LP(IN);
   cdd_lp_sol<Coord> Sol(LP);
   Sol.verify();
   return lp_solution(Sol.optimal_value(), LP.optimal_vertex());
}

} } }

#endif // POLYMAKE_POLYTOPE_CDD_INTERFACE_IMPL_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
