#include <polymake/GenericSet.h>
#include <polymake/Set.h>
#include <polymake/TropicalNumber.h>
#include <polymake/Matrix.h>
#include <polymake/IncidenceMatrix.h>
#include <polymake/Vector.h>

#include <polymake/tropical/arithmetic.h>
#include <polymake/tropical/thomog.h>
#include <polymake/linalg.h>

#ifndef POLYMAKE_TROPICAL_PATCHWORK_H
#define POLYMAKE_TROPICAL_PATCHWORK_H

namespace polymake { namespace tropical {

// COMBINATORICS:

template <typename Addition>
IncidenceMatrix<NonSymmetric> optimal_monomials(const Matrix<Int>& monomials, const Vector<TropicalNumber<Addition>>& coefficients,
                                                const IncidenceMatrix<NonSymmetric>& cells, const Matrix<Rational>& vertices)
{
   const Int n_monoms = monomials.rows();
   const Int n_cells = cells.rows();

   // for each cell - compute a point in its relative interior
   Matrix<Rational> rel_int_points(n_cells, vertices.cols());
   for (Int r = 0; r < n_cells; ++r) {
      for (auto v = entire(cells.row(r)); !v.at_end(); ++v)
         rel_int_points.row(r) += vertices.row(*v);
      rel_int_points.row(r).dehomogenize();
   }
   rel_int_points = rel_int_points.minor(All, ~scalar2set(0));

   // for each cell - compute the monomials where the optimum is attained:
   IncidenceMatrix<NonSymmetric> opt(n_cells, n_monoms);
   for (Int c = 0; c < n_cells; ++c) {
      Vector<TropicalNumber<Addition>> substed{ monomials * rel_int_points[c] + Vector<Rational>(coefficients) };
      opt.row(c) = extreme_value_and_index<Addition>(substed).second;
   }

   return opt;
}

template <typename TVector>
Int count_monomials(unsigned long orthant, const TVector& monomials)
{
  Int result = 0;
  Int index = 0;
  for (; orthant != 0; orthant >>= 1, ++index) {
    if (orthant & 1) result += monomials[index];
  }
  return result;
}

// indices of those facets of the hypersurface present in orthant:
Set<Int> real_facets_in_orthant(unsigned long orthant, const IncidenceMatrix<NonSymmetric>& facets,
                                const Matrix<Int>& monomials, const Array<bool>& signs, const IncidenceMatrix<NonSymmetric>& opt)
{
   // sign distribution for this orthant:
   orthant <<= 1;
   Array<bool> S(monomials.rows());
   for (Int m = 0; m < monomials.rows(); ++m)
     S[m] = (count_monomials(orthant, monomials[m])%2 != 0) != signs[m];

   // facets for this orthand:
   Set<Int> real_facets{}; // init as empty
   for (Int c = 0; c < facets.rows(); ++c) {
      // put monomial indices in a vector, so we can compare their signs (dirty):
      Vector<Int> foo(2, entire(opt.row(c)));
      if (S[foo[0]] != S[foo[1]])
         real_facets += c;
   }

   return real_facets;
}

// indices of those facets of the hypersurface present in each orthant:
template <typename Addition>
IncidenceMatrix<NonSymmetric> real_facets(const Array<bool>& signs, const Matrix<Int>& monomials,
                                          const Vector<TropicalNumber<Addition>>& coefficients, const Matrix<Rational>& vertices,
                                          const IncidenceMatrix<NonSymmetric>& facets)
{
   const Int dim = monomials.cols()-1;
   const unsigned long n_orthants = 1UL << dim;

   IncidenceMatrix<NonSymmetric> viro_cells(n_orthants, facets.rows());
   IncidenceMatrix<NonSymmetric> opt = optimal_monomials(monomials, coefficients, facets, vertices);
   for (unsigned long o = 0; o < n_orthants; ++o)
     viro_cells[o] = real_facets_in_orthant(o, facets, monomials, signs, opt);

   return viro_cells;
}

// REALIZATION:

// translate vertices into positive orthant (and dehomogenize):
   template <typename Addition>
Matrix<Rational> move_to_positive(const Matrix<Rational>& vertices, const Set<Int>& far_vertices)
{
   Set<Int> finite_vertices = range(0, vertices.rows()-1)-far_vertices;
   Matrix<Rational> vertices_moved = (-Addition::orientation())*vertices.minor(All, range(2, vertices.cols()-1));
   /* Matrix<Rational> vertices_moved(vertices); */
   /* vertices_moved = (-Addition::orientation())*vertices.minor(All, range(2, dim)); */
   for (Int c = 0; c < vertices_moved.cols(); ++c) {
      Rational min = std::numeric_limits<Rational>::infinity();
      for (auto r = entire(finite_vertices); !r.at_end(); r++) {
         if (vertices_moved[*r][c] < min)
            min = vertices_moved[*r][c];
      }
      for (auto r = entire(finite_vertices); !r.at_end(); r++)
         vertices_moved[*r][c] += 1 - min;
   }
   vertices_moved = vertices.col(0)|vertices_moved;
   return vertices_moved;
}

// find the max. cells in the dual subdivision which are dual to the vertices:
   template <typename Addition>
IncidenceMatrix<NonSymmetric> dual_facets(const Matrix<Rational>& vertices, const Set<Int>& far_vertices,
                                          const Matrix<Int>& monomials, const Vector<TropicalNumber<Addition>>& coefficients)
{
   IncidenceMatrix<NonSymmetric> duals(vertices.rows(), monomials.rows());

   Matrix<Rational> sub = monomials*T(vertices.minor(All, ~scalar2set(0))) + repeat_col(Vector<Rational>(coefficients), vertices.rows());
   for (Int v = 0; v < sub.cols(); ++v) {
      if (!far_vertices.contains(v))
         duals.row(v) = (tropical::extreme_value_and_index<Addition>( Vector<TropicalNumber<Addition>>(sub.col(v)) )).second;
   }

   return duals;
}

// move vertices to barycenters of resp. facets in the dual subdivision:
   template <typename Addition>
Matrix<Rational> move_to_bary(const Matrix<Rational>& vertices, const Matrix<Int>& monomials,
                              const Vector<TropicalNumber<Addition>>& coefficients, const Set<Int>& far_vertices)
{
   Matrix<Rational> vertices_moved(vertices.rows(), vertices.cols()-2);
   IncidenceMatrix<NonSymmetric> duals = dual_facets(vertices, far_vertices, monomials, coefficients);

   for (Int v = 0; v < vertices_moved.rows(); ++v) {
      if (far_vertices.contains(v))
         vertices_moved.row(v) = (-Addition::orientation())*(vertices.row(v)).slice(range(2, vertices.cols()-1));
      else
         vertices_moved.row(v) = barycenter(Matrix<Rational>(monomials.minor(duals.row(v), ~scalar2set(0))));
   }

   return vertices.col(0)|vertices_moved;
}

auto nonzero(const Vector<Rational>& vector)
{
   return indices(attach_selector(vector, operations::non_zero()));
}

bool has_one_nonzero(const Vector<Rational>& vector)
{
   auto nz = nonzero(vector);
   auto nz_it = entire(nz);
   return !nz_it.at_end() && (++nz_it).at_end();
}

// realize the real part of the hypersurface in IR^dim:
template <typename Addition>
BigObject real_part_realize(const Matrix<Int>& monomials, const Vector<TropicalNumber<Addition>>& coefficients,
                               const Matrix<Rational>& vertices, const IncidenceMatrix<NonSymmetric>& cells,
                               const Set<Int>& far_vertices, const IncidenceMatrix<NonSymmetric>& viro_cells, const std::string& method)
{
   const Int dim = monomials.cols()-1;
   const unsigned long n_orthants = 1UL << dim;
   const Int n_vertices = vertices.rows();

   // find far vertices pointing in coordinate direction:
   Set<Int> far_vertices_cd;
   for (auto i = entire(far_vertices); !i.at_end(); ++i) {
      if (has_one_nonzero(vertices.row(*i)))
         far_vertices_cd += *i;
   }

   // dehomogenize and move vertices into positive orthant:
   Matrix<Rational> vertices_moved;
   if (method == "rigid")
      vertices_moved = move_to_positive<Addition>(vertices, far_vertices);
   else if (method == "uniform")
      vertices_moved = move_to_bary(vertices, monomials, coefficients, far_vertices);
   else
      throw std::runtime_error("Unknown realization method.");

   // build the complex:
   Matrix<Rational> points(0, vertices_moved.cols()-1);
   IncidenceMatrix<NonSymmetric> input_polytopes(0, n_vertices*n_orthants);
   Int current_row = 0; // ... of input_polytopes
   for (unsigned long O = 0; O < n_orthants; ++O) {

      // reflect vertices to current orthant:
      Matrix<Rational> vertices_reflected(vertices_moved);
      for (Int i = 0; i < dim; ++i) {
         if (O & (1UL << i))
            vertices_reflected.col(i+1).negate();
      }
      points /= vertices_reflected;

      // build maximal cells:
      input_polytopes.resize(input_polytopes.rows() + viro_cells[O].size(), input_polytopes.cols());
      for (auto c = entire(viro_cells[O]); !c.at_end(); ++c, ++current_row) {

         // find orthants c meets (i.e. mirror current orthant along all combinations of coordinate direction rays in c):
         Set<unsigned long> orthants = scalar2set(O);
         for (auto cd = entire(cells.row(*c)*far_vertices_cd); !cd.at_end(); ++cd) {
            Set<Int> tmp(orthants);
            for (auto o = entire(tmp); !o.at_end(); ++o) {
               Int index = *(nonzero(vertices_reflected.row(*cd)).begin())-1;
               unsigned long o_new = (*o)^(1UL << index);
               orthants += o_new;
            }
         }

         // build cell from all (incl. reflected) vertices:
         for (auto o = entire(orthants); !o.at_end(); o++) {
            for (auto v = entire(cells[*c]-far_vertices_cd); !v.at_end(); ++v)
               input_polytopes(current_row, n_vertices*(*o) + *v) = true;
         }
      }
   }

   BigObject r("fan::PolyhedralComplex<Rational>");
   r.take("POINTS") << points;
   r.take("INPUT_POLYTOPES") << input_polytopes;

   return r;
}

} }

#endif

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
