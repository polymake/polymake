#include <polymake/GenericSet.h>
#include <polymake/Set.h>
/* #include <polymake/PowerSet.h> */
/* #include <polymake/Map.h> */
#include <polymake/TropicalNumber.h>
#include <polymake/Matrix.h>
#include <polymake/IncidenceMatrix.h>
#include <polymake/Vector.h>

#include <polymake/tropical/arithmetic.h>
#include <polymake/tropical/thomog.h>
#include <polymake/linalg.h>
/* #include <bitset> */

#ifndef POLYMAKE_TROPICAL_PATCHWORK_H
#define POLYMAKE_TROPICAL_PATCHWORK_H

namespace polymake { namespace tropical {

// COMBINATORICS:

// iterate through orthants (as 0/1-vectors, and the numbers they represent)
struct orthant_iterator {
   Vector<bool> vec;
   int ord;
   bool carry;
   orthant_iterator(int dim) : vec(dim), ord(0), carry(false) {};
   void operator++ () {
      if (carry)
         return;
      carry = true;
      for (int i = 0; i < vec.size() && carry; i++) {
         carry = vec[i];
         vec[i] = !vec[i];
      }
      ord++;
   }
   Vector<bool>& operator* () {return vec;}
   bool at_end() {return carry;}
};

   template <typename Addition>
IncidenceMatrix<NonSymmetric> optimal_monomials (const Matrix<int>& monomials, const Vector<TropicalNumber<Addition>>& coefficients, const IncidenceMatrix<NonSymmetric>& cells, const Matrix<Rational>& vertices)
{
   int n_monoms = monomials.rows();
   int n_cells = cells.rows();

   // for each cell - compute a point in its relative interior
   Matrix<Rational> rel_int_points(n_cells, vertices.cols());
   for (int r = 0; r < n_cells; r++) {
      for (auto v = entire(cells.row(r)); !v.at_end(); v++)
         rel_int_points.row(r) += vertices.row(*v);
      rel_int_points.row(r).dehomogenize();
   }
   rel_int_points = rel_int_points.minor(All, ~scalar2set(0));

   // for each cell - compute the monomials where the optimum is attained:
   IncidenceMatrix<NonSymmetric> opt(n_cells, n_monoms);
   for (int c = 0; c < n_cells; c++) {
      Vector<TropicalNumber<Addition>> substed = Vector<TropicalNumber<Addition>>(monomials*rel_int_points[c] + Vector<Rational>(coefficients));
      opt.row(c) = (tropical::extreme_value_and_index<Addition>(substed)).second;
   }

   return opt;
}

// indices of those facets of the hypersurface present in orthant:
Set<int> real_facets_in_orthant (const Vector<bool>& orthant, const IncidenceMatrix<NonSymmetric>& facets, const Matrix<int>& monomials, const Vector<bool>& signs, const IncidenceMatrix<NonSymmetric>& opt)
{
   // sign distribution for this orthant:
   Vector<bool> S(monomials.rows());
   for (int m = 0; m < monomials.rows(); m++)
      S[m] = (((0|orthant)*monomials[m] % 2 != 0) != signs[m]);
      /* S[m] = (((0|orthant)*monomials[m] % 2) != signs[m]); // TODO */

   // facets for this orthand:
   Set<int> real_facets = Set<int>(); // init as empty
   for (int c = 0; c < facets.rows(); c++) {
      // put monomial indices in a vector, so we can compare their signs (dirty):
      Vector<int> foo(2, entire(opt.row(c)));
      if (S[foo[0]] != S[foo[1]])
         real_facets += c;
   }

   return real_facets;
}

// indices of those facets of the hypersurface present in each orthant:
   template <typename Addition>
IncidenceMatrix<NonSymmetric> real_facets (const Vector<bool>& signs, const Matrix<int>& monomials, const Vector<TropicalNumber<Addition>>& coefficients, const Matrix<Rational>& vertices, const IncidenceMatrix<NonSymmetric>& facets)
{
   const int n_orthants = 1<<(monomials.cols()-1);
   const int dim = monomials.cols() - 1;

   IncidenceMatrix<NonSymmetric> viro_cells(n_orthants, facets.rows());
   IncidenceMatrix<NonSymmetric> opt = optimal_monomials(monomials, coefficients, facets, vertices);
   for (auto o = orthant_iterator(dim); !o.at_end(); ++o)
      viro_cells[o.ord] = real_facets_in_orthant(o.vec, facets, monomials, signs, opt);
   /* for (int o = 0; o < n_orthants; o++) */
   /*    viro_cells[o] = real_facets_in_orthant(o, facets, monomials, signs, opt); */

   return viro_cells;
}

// REALIZATION:

// translate vertices into positive orthant (and dehomogenize):
   template <typename Addition>
Matrix<Rational> move_to_positive(const Matrix<Rational>& vertices, const Set<int>& far_vertices)
{
   Set<int> finite_vertices = range(0, vertices.rows()-1)-far_vertices;
   Matrix<Rational> vertices_moved = (-Addition::orientation())*vertices.minor(All, range(2, vertices.cols()-1));
   /* Matrix<Rational> vertices_moved(vertices); */
   /* vertices_moved = (-Addition::orientation())*vertices.minor(All, range(2, dim)); */
   for (int c = 0; c < vertices_moved.cols(); c++) {
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
IncidenceMatrix<NonSymmetric> dual_facets(const Matrix<Rational>& vertices, const Set<int>& far_vertices, const Matrix<int>& monomials, const Vector<TropicalNumber<Addition>>& coefficients)
{
   IncidenceMatrix<NonSymmetric> duals(vertices.rows(), monomials.rows());

   Matrix<Rational> sub = monomials*T(vertices.minor(All, ~scalar2set(0))) + repeat_col(Vector<Rational>(coefficients), vertices.rows());
   for (int v = 0; v < sub.cols(); v++) {
      if (!far_vertices.contains(v))
         duals.row(v) = (tropical::extreme_value_and_index<Addition>( Vector<TropicalNumber<Addition>>(sub.col(v)) )).second;
   }

   return duals;
}

// move vertices to barycenters of resp. facets in the dual subdivision:
   template <typename Addition>
Matrix<Rational> move_to_bary(const Matrix<Rational>& vertices, const Matrix<int>& monomials, const Vector<TropicalNumber<Addition>>& coefficients, const Set<int>& far_vertices)
{
   Matrix<Rational> vertices_moved(vertices.rows(), vertices.cols()-2);
   IncidenceMatrix<NonSymmetric> duals = dual_facets(vertices, far_vertices, monomials, coefficients);

   for (int v = 0; v < vertices_moved.rows(); v++) {
      if (far_vertices.contains(v))
         vertices_moved.row(v) = (-Addition::orientation())*(vertices.row(v)).slice(range(2, vertices.cols()-1));
      else
         vertices_moved.row(v) = barycenter(Matrix<Rational>(monomials.minor(duals.row(v), ~scalar2set(0))));
   }

   return vertices.col(0)|vertices_moved;
}

Set<int> nonzero(Vector<Rational> vector)
{
   Set<int> nz;
   for (int i = 0; i < vector.size(); i++) {
      if (vector[i] != 0)
         nz += i;
   }
   return nz;
}

// realize the real part of the hypersurface in IR^dim:
   template <typename Addition>
perl::Object real_part_realize (const Matrix<int>& monomials, const Vector<TropicalNumber<Addition>>& coefficients, const Matrix<Rational>& vertices, const IncidenceMatrix<NonSymmetric>& cells, const Set<int>& far_vertices, const IncidenceMatrix<NonSymmetric>& viro_cells, const std::string& method)
{
   const int dim = monomials.cols() - 1;
   const int n_orthants = 1<<dim;
   const int n_vertices = vertices.rows();

   // find far vertices pointing in coordinate direction:
   Set<int> far_vertices_cd;
   for (auto i = entire(far_vertices); !i.at_end(); i++) {
      if (nonzero(vertices.row(*i)).size() == 1)
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
   Matrix<Rational> points(0, vertices_moved.cols() - 1);
   IncidenceMatrix<NonSymmetric> input_polytopes(0, n_vertices*n_orthants);
   int current_row = 0; // ... of input_polytopes
   /* for (int O = 0; O < n_orthants; O++) { */
   for (auto O = orthant_iterator(dim); !O.at_end(); ++O) {

      // reflect vertices to current orthant:
      Matrix<Rational> vertices_reflected(vertices_moved);
      for (int i = 0; i < dim; i++) {
         if ((O.vec)[i])
            vertices_reflected.col(i+1).negate();
      }
      points /= vertices_reflected;

      // build maximal cells:
      input_polytopes.resize(input_polytopes.rows() + viro_cells[O.ord].size(), input_polytopes.cols());
      for (auto c = entire(viro_cells[O.ord]); !c.at_end(); c++, current_row++) {

         // find orthants c meets (i.e. mirror current orthant along all combinations of coordinate direction rays in c):
         Set<int> orthants = scalar2set(O.ord);
         for (auto cd = entire(cells.row(*c)*far_vertices_cd); !cd.at_end(); cd++) {
            Set<int> tmp(orthants);
            for (auto o = entire(tmp); !o.at_end(); o++) {
               int index = *entire(nonzero(vertices_reflected.row(*cd))) - 1;
               int o_new = (*o)^(1<<index);
               orthants += o_new;
            }
         }

         // build cell from all (incl. reflected) vertices:
         for (auto o = entire(orthants); !o.at_end(); o++) {
            for (auto v = entire(cells[*c]-far_vertices_cd); !v.at_end(); v++)
               input_polytopes.row(current_row) += (n_vertices*(*o) + *v);
         }

      }

   }

   perl::Object r("fan::PolyhedralComplex<Rational>");
   r.take("POINTS") << points;
   r.take("INPUT_POLYTOPES") << input_polytopes;

   return r;
}

} }

#endif
