/* Copyright (c) 1997-2014
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

// cddlib headers:
extern "C" {
#include "setoper.h"
#include "cdd.h"
}

namespace polymake { namespace polytope { namespace cdd_interface {

template <typename Coord> struct traits;

#ifdef GMPRATIONAL
template <>
struct traits<Rational> {
   static const dd_NumberType number_type=dd_Rational;

   // PTL -> cddlib
   static void store(mytype *dst, const Rational *src)
   {
      dd_set(*dst, src->get_rep());
   }

   // cddlib -> PTL
   static void fetch(Rational* dst, const mytype* src)
   {
      dst->set(*src);
   }
};
#else
template <>
struct traits<double> {
   static const dd_NumberType number_type=dd_Real;

   static void store(mytype *dst, const double *src)
   {
      dd_set_d(*dst, *src);
   }

   static void fetch(double *dst, const mytype* src)
   {
      *dst=dd_get_d(*const_cast<mytype*>(src));         // cddlib is not const-clean
   }
};
#endif

template <typename Coord> class cdd_polyhedron;
template <typename Coord> class cdd_lp;
template <typename Coord> class cdd_lp_sol;
template <typename Coord> class cdd_matrix;

template <typename Coord>
class cdd_vector : public traits<Coord> {
   typedef traits<Coord> _super;

   friend class cdd_matrix<Coord>;
   friend class cdd_polyhedron<Coord>;
   friend class cdd_lp<Coord>;
   friend class solver<Coord>;
public:
   explicit cdd_vector(int dim_arg) : dim(dim_arg)
   {
      dd_InitializeArow(dim, &ptr);
   }

   ~cdd_vector() { dd_FreeArow(dim, ptr); }

   Vector<Coord> get(int start_at=0) const;

private:
   int dim;
   dd_Arow ptr;
};

template <typename Coord>
Vector<Coord> cdd_vector<Coord>::get(int start_at) const
{
   Vector<Coord> ret(dim-start_at);
   typename Vector<Coord>::iterator dst=ret.begin();
   for (mytype *src=ptr+start_at, *end=ptr+dim;  src!=end;  ++src, ++dst)
      _super::fetch(dst, src);
   return ret;
}

template <typename Coord>
class cdd_matrix : public traits<Coord> {
   typedef traits<Coord> _super;

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

   ~cdd_matrix() { dd_FreeMatrix(ptr); }

   void add_objective(const Vector<Coord>& obj, bool maximize);

   typename solver<Coord>::matrix_pair representation_conversion(const bool isCone = false, const bool primal = true) const;

   ListMatrix< Vector<Coord> > vertex_normals(Bitset& Vertices);

   void canonicalize(Bitset& Pt, Bitset& Lin);

   void canonicalize_lineality(Bitset& Lin);

private:
   dd_MatrixPtr ptr;
};

template <typename Coord>
class cdd_polyhedron {
   friend class cdd_matrix<Coord>;
   friend class solver<Coord>;
public:
   cdd_polyhedron(const cdd_matrix<Coord>& M) : ptr(dd_DDMatrix2Poly(M.ptr, &err)) {}
   ~cdd_polyhedron() { dd_FreePolyhedra(ptr); }
   void verify();
protected:
   dd_PolyhedraPtr ptr;
   dd_ErrorType err;
};

template <typename Coord>
class cdd_lp : public traits<Coord> {
   typedef traits<Coord> _super;
   friend class cdd_lp_sol<Coord>;
public:
   cdd_lp(const cdd_matrix<Coord>& M) : ptr(dd_Matrix2LP(M.ptr, &err)) {}
   ~cdd_lp() { dd_FreeLPData(ptr); }

   Vector<Coord> optimal_vertex() const;
protected:
   dd_LPPtr ptr;
   dd_ErrorType err;
   dd_LPSolutionPtr get_solution();
};

template <typename Coord>
class cdd_lp_sol {
public:
   void verify();
   cdd_lp_sol(cdd_lp<Coord>& L) : ptr(L.get_solution()) {}
   ~cdd_lp_sol() { dd_FreeLPSolution(ptr); }

   Coord optimal_value() const;
protected:
   dd_LPSolutionPtr ptr;
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
struct check_iterator_feature<polymake::polytope::cdd_interface::cdd_bitset_iterator, end_sensitive> : True {};

template <>
struct check_container_feature<polymake::polytope::cdd_interface::cdd_bitset, sparse_compatible> : True {};

}
namespace polymake { namespace polytope { namespace cdd_interface {

// FIXME: method currently used for RAY_SEPARATORS/VERTEX_NORMALS from RAYS
//        for this computation LINEALITY_SPACE is not needed
// should not be used for INEQUALITES (set representation type)
template <typename Coord>
cdd_matrix<Coord>::cdd_matrix(const Matrix<Coord>& P)
   : ptr(dd_CreateMatrix(P.rows(), P.cols()))   
{

   // get size of the input matrix
   int m = P.rows();
   int n = P.cols();

   ptr->representation = dd_Generator;    // Input type: points
   ptr->numbtype = _super::number_type;
  
   // copy data: polymake -> cdd
   typename ConcatRows< Matrix<Coord> >::const_iterator p=concat_rows(P).begin();
   for (mytype **r=ptr->matrix, **rend=r+m;  r!=rend;  ++r) {
      mytype *c=*r;
      mytype *cend=c+n;
      for (;  c!=cend;  ++c, ++p) 
         _super::store(c,p);
   }

}

template <typename Coord>
cdd_matrix<Coord>::cdd_matrix(const Matrix<Coord>& I, const Matrix<Coord>& E, const bool primal)
   : ptr(dd_CreateMatrix(I.rows() + E.rows(),  ( I.cols() | E.cols() ) ) ) {

   // get size of the input matrix
   int mi = I.rows();
   int me = E.rows();
   int n  = I.cols() | E.cols();

   if ( primal )
      ptr->representation = dd_Inequality;   // Input type: inequalities
   else
      ptr->representation = dd_Generator;    // Input type: points
   ptr->numbtype = _super::number_type;
  
   typename ConcatRows< Matrix<Coord> >::const_iterator p=concat_rows(I).begin();
   mytype **r, **rend;
   for (r=ptr->matrix, rend=r+mi;  r!=rend;  ++r) {
      mytype *c=*r;
      mytype *cend=c+n;  
      for (;  c!=cend;  ++c, ++p) 
         _super::store(c,p);
   }
   
   p=concat_rows(E).begin();
   ++mi; // cddlib starts counting at 1
   for (rend+=me;  r!=rend;  ++r, ++mi) {
      mytype *c=*r;
      mytype *cend=c+n;
      for (;  c!=cend;  ++c, ++p)
         _super::store(c,p);
      set_addelem(ptr->linset, mi);
   }
}

template <typename Coord>
void cdd_matrix<Coord>::add_objective(const Vector<Coord>& obj, bool maximize)
{
   typename Vector<Coord>::const_iterator o=obj.begin();
   for (mytype *r=ptr->rowvec, *rend=r+obj.size();  r!=rend;  ++r, ++o)
      _super::store(r,o);
   ptr->objective= maximize ? dd_LPmax : dd_LPmin;
}

template <typename Coord>
void cdd_polyhedron<Coord>::verify()
{
   if (err != dd_NoError) {
      std::ostringstream err_msg;
      err_msg << "Error in dd_DDMatrix2Poly: " << err << endl;
      throw std::runtime_error(err_msg.str());
   }
}

template <typename Coord>
dd_LPSolutionPtr cdd_lp<Coord>::get_solution()
{
   if (err != dd_NoError) {
      std::ostringstream err_msg;
      err_msg << "Error in dd_Matrix2LP: " << err << endl;
      throw std::runtime_error(err_msg.str());
   }

   if (! dd_LPSolve(ptr, dd_DualSimplex, &err)) {
      std::ostringstream err_msg;
      err_msg << "Error in dd_LPSolve: " << err << endl;
      throw std::runtime_error(err_msg.str());
   }

   return dd_CopyLPSolution(ptr);
}

template <typename Coord>
cdd_matrix<Coord>::cdd_matrix(const cdd_polyhedron<Coord>& P, bool primal)
   : ptr(primal ? dd_CopyInequalities(P.ptr) : dd_CopyGenerators(P.ptr)) { }

         // extracts the dual description from a cdd matrix
         // and makes some adjustements for polymake needs
         // primal <-> input representation by inequalities/equations
template <typename Coord>
typename solver<Coord>::matrix_pair
cdd_matrix<Coord>::representation_conversion(const bool isCone, const bool primal ) const
{
   const dd_rowrange m = ptr->rowsize;
   const dd_colrange n = ptr->colsize;
   const long linsize = set_card(ptr->linset); 

   if (primal && m<=0) throw infeasible();

   Matrix<Coord> Pt(m-linsize,n);
   Matrix<Coord> Lin(linsize, n);

   typename ConcatRows< Matrix<Coord> >::iterator pt=concat_rows(Pt).begin(), lin=concat_rows(Lin).begin();
   int index=1;         // cddlib starts counting at 1
   for (mytype **r=ptr->matrix, **rend=r+m;  r!=rend;  ++r, ++index) {
      if (set_member(index, ptr->linset))
         // the current row is a lineality 
         for (mytype *c=*r, *cend=c+n;  c!=cend;  ++c, ++lin)
            _super::fetch(lin,c);
      else 
         // the current row is a generator of the pointed cone
         for (mytype *c=*r, *cend=c+n;  c!=cend;  ++c, ++pt)
            _super::fetch(pt,c);
   }

   if ( primal ) {
      // for a 0-dimensional cone cdd returns the origin as a vertex
      // we have to remove this in our interpretation
      if (isCone && !linsize && Pt.rows()==1 && Pt(0,0)==1 ) { Pt.resize(0,Pt.cols()); }
      
      // in the case of a homogeneous cone, the cddlib omits the origin point
      // in the case of a cone we don't want to add this!
      if (!linsize && is_zero(Pt.col(0)) && !isCone) Pt /= unit_vector<Coord>(Pt.cols(), 0);
   } 

   return typename solver<Coord>::matrix_pair(Pt,Lin);
}


template <typename Coord>
ListMatrix< Vector<Coord> >
cdd_matrix<Coord>::vertex_normals(Bitset& Vertices)
{
   ListMatrix< Vector<Coord> > VN;
   typename Rows< ListMatrix< Vector<Coord> > >::iterator vn_front=rows(VN).begin();
   cdd_vector<Coord> cert(ptr->colsize+1);
   dd_ErrorType err;
   for (int i=ptr->rowsize; i>=1; --i) {
      const bool is_redundant=dd_Redundant(ptr, i, cert.ptr, &err);
      if (err != dd_NoError) {
         std::ostringstream err_msg;
         err_msg << "Error in dd_Redundant: " << err << endl;
         throw std::runtime_error(err_msg.str());
      }
      if (is_redundant) {
         dd_MatrixRowRemove(&ptr, i);
      } else {
         Vertices += i-1;
         Vector<Coord> vertex_normal=cert.get(1);
         //  we redefined vertex normals to be affine functionals
         //  vertex_normal[0]=0; 
         // vertex_normal.negate();
         vn_front=VN.insert_row(vn_front, vertex_normal);
      }
   }
   return VN;
}


template <typename Coord>
void
cdd_matrix<Coord>::canonicalize(Bitset& Pt, Bitset& Lin) {
   cdd_bitset impl_linset, redset;
   dd_rowindex newpos;
   dd_ErrorType err;
   const dd_rowrange m = ptr->rowsize;

   const bool success=dd_MatrixCanonicalize(&ptr, &impl_linset.ptr, &redset.ptr, &newpos, &err);

   if (!success || err != dd_NoError) {
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
   dd_rowindex newpos;
   dd_ErrorType err;
   const dd_rowrange m = ptr->rowsize;

   const bool success=dd_MatrixCanonicalizeLinearity(&ptr, &impl_linset.ptr, &newpos, &err);

   if (!success || err != dd_NoError) {
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
   case dd_Optimal: 
       break;
   case dd_StrucInconsistent:   // if the problem is inconsistent then mark that there is no valid point
   case dd_Inconsistent:       
      throw infeasible();
   case dd_Unbounded:      // if the problem is unbounded then mark that there is no optimal solution
      throw unbounded();
   case dd_StrucDualInconsistent:  // if the dual problem is either unbounded or
   case dd_DualUnbounded:          // inconsistent we don't know whether the primal 
   case dd_DualInconsistent:       // is inconsistent or unbounded
      throw std::runtime_error("cannot handle lp solution: problem is either inconsistent or unbounded");
   default:  // all other cases are handled here ....
      std::ostringstream err_msg;
      err_msg << "cannot handle lp solution: cdd returned: " << ptr->LPS;
      throw std::runtime_error(err_msg.str());
   }
}

template <typename Coord>
Vector<Coord> cdd_lp<Coord>::optimal_vertex() const
{
   const int n=ptr->d;
   Vector<Coord> Opt(n);
   typename Vector<Coord>::iterator o=Opt.begin();
   for (mytype *s=ptr->sol, *send=s+n;  s!=send;  ++s, ++o)
      _super::fetch(o,s);
   return Opt;
}

// end of cdd classes
//  implementation of solver methods

template <typename Coord>
struct solver<Coord>::initializer {
   initializer() { dd_set_global_constants(); }
   ~initializer() { dd_free_global_constants(); }
};

template <typename Coord>
solver<Coord>::solver()
{
   static initializer init;
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
