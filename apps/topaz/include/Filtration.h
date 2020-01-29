#ifndef POLYMAKE_TOPAZ_FILTRATION_H
#define POLYMAKE_TOPAZ_FILTRATION_H

#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/SparseVector.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/internal/matrix_methods.h"

namespace polymake { namespace topaz {

// dimension and index are needed to find the corresponding boundary via the given matrices.
struct Cell {
  Int deg, dim, idx;

  Cell() : deg(0), dim(0), idx(0) {}

  Cell(Int deg_in, Int dim_in, Int idx_in) : deg(deg_in), dim(dim_in), idx(idx_in) {}

  bool operator!= (const Cell & other) const
  {
    return deg!=other.deg || dim!=other.dim || idx!=other.idx;
  }

  // for testing.
  friend std::ostream& operator<< (std::ostream & os, const Cell& c)
  {
    os << "(" << c.deg << "," << c.dim << "," << c.idx << ")";
    return os;
  }
};

template <typename MatrixType>
class Filtration {
  using Coeff = typename MatrixType::value_type;
  using MatNS = typename MatrixType::persistent_nonsymmetric_type;

public:
  Array<Cell> C; //after object initialization, this array is always sorted as required by the persistent homology algorithm.
  Array<MatrixType> bd_matrix; //boundary matrices by dimension. idx of the cells corresponds to the row in the corresponding matrix
  Array<Array<Int>> ind; //keep track of indices to implement proper bd function

  Filtration() {}

  Filtration(const Array<Cell>& C_in, const Array<MatrixType>& bd_in, bool sorted=false)
    : C(C_in)
    , bd_matrix(bd_in)
    , ind(bd_in.size())
  {
    if (sorted) update_indices();
    else sort();
  }

  Filtration(const graph::Lattice<graph::lattice::BasicDecoration>& HD, const Array<Int>& degs)
    : C(HD.nodes()-2)   // -2 for empty set and dummy
    , bd_matrix(HD.rank())
  {
    Int dim = HD.rank()-1;

    const auto vertex_set = HD.nodes_of_rank(1); // nodes of dim 0
    Int n_bd = vertex_set.size(); // number of vertices
    bd_matrix[0] = ones_matrix<Coeff>(n_bd, 1);
    Map<Int, Int> reindex_map; // map index in hasse diagram to index in boundary matrix.
    Int count_index = 0;
    for (Int i : vertex_set) {
      C[i-1] = Cell(degs[i-1], 0, count_index);
      reindex_map[i] = count_index;
      ++count_index;
    }

    // compute boundary matrices for d>0:
    for (Int d = 1; d < dim; ++d) {
      const auto d_set = HD.nodes_of_rank(d+1);
      Int n = d_set.size();//number of d-simplices
      MatrixType bd(n, n_bd);
      n_bd = n;
      Map<Int, Int> new_reindex_map;

      ///////////////////////
      Int r = 0; //row index
      for (Int f : d_set) { // iterate d-simplices
        auto face = HD.face(f);
        new_reindex_map[f] = r; // fill in index map for next iteration
        C[f-1] = Cell(degs[f-1],d,r);//put indices into cell array

        for (Int sf : HD.in_adjacent_nodes(f)) { // iterate d-1-simplices
          auto subface = HD.face(sf);
          Int i = 0;   // find index of the vertex missing in current subface
          for (auto f_it = entire(face), sf_it = entire(subface);
               (*f_it) == (*sf_it) && !sf_it.at_end(); ++f_it, ++sf_it)
            ++i;

          bd(r, reindex_map[sf]) = i%2 ? Coeff(1) : Coeff(-1);
        }
        ++r;
      }
      ///////////////////////////
      bd_matrix[d] = bd;
      reindex_map = new_reindex_map;
    }

    sort();
  }

  // keeps track of ind matrix for easy access of boundaries
  void update_indices()
  {
    ind.resize(bd_matrix.size());
    for (auto i = entire<indexed>(ind); !i.at_end(); ++i)
      i->resize(bd_matrix[i.index()].rows());
    for (auto c=entire<indexed>(C); !c.at_end(); ++c) {
      ind[c->dim][c->idx] = c.index();
    }
  }

public:
  Int n_cells() const
  {
    return C.size();
  }

  Int n_frames() const
  {
    return C[n_cells()-1].deg; //TODO this only works with sorted array...
  }

  Int dim() const
  {
    return bd_matrix.size()-1;
  }

  using Chain = SparseVector<Coeff>;
  Chain bd(Int i) const
  {
    Cell cell = C[i];
    Int d = cell.dim;
    Chain c(C.size());
    if (d==0) return c; //points have empty boundary
    Chain b = bd_matrix[d].row(cell.idx);
    for (auto e = entire(b); !e.at_end(); ++e) {
      c[ind[d-1][e.index()]] = *e;
    }
    return c;
  }

  // sort cells by degree first and dimension second, as required by persistent homology algo.
private:
  struct cellComparator {
    bool operator()(const Cell& c1, const Cell& c2) const
    {
      if (c1.deg < c2.deg) return true;
      if (c1.deg == c2.deg) {
        if (c1.dim < c2.dim) return true;
        if (c1.dim == c2.dim) return c1.idx < c2.idx; // idx gets sorted lex to allow equality checking
      }
      return false;
    }
  };

  void sort()
  {
    std::sort(C.begin(), C.end(), cellComparator());
    update_indices(); //TODO do this while sorting?
  }

public:
  const Cell& operator[](Int i) const
  {
    return C[i];
  }

  const MatrixType& boundary_matrix(Int d) const
  {
    return bd_matrix[d];
  }

  const Array<Cell> cells() const
  {
    return C;
  }

  /// returns d-bd matrix of t-th frame TODO accept non-sorted filtrations?
  /// @param[out] frame indices of d-simplices present in this frame
  /// @param[out] frame_bd indices of d-1-simplices present
  MatrixType boundary_matrix_with_frame_sets(Int d, Int t, Set<Int>& frame, Set<Int>& frame_bd) const
  {
    if (t>n_frames()) throw std::runtime_error("Filtration: input exceeds number of frames");
    if (d>dim()) throw std::runtime_error("Filtration: input exceeds filtration dimension");
    const MatrixType& B = bd_matrix[d];

    for (auto i = entire<indexed>(ind[d]); !i.at_end(); ++i) {
      if (C[*i].deg <= t) frame += i.index();
    }
    if (d>0) {
      for (auto i = entire<indexed>(ind[d-1]); !i.at_end(); ++i) {
        if (C[*i].deg <= t) frame_bd += i.index();
      }
    } else {
      frame_bd = sequence(0, B.cols());
    }

    return B.minor(frame, frame_bd);
  }

  MatrixType boundary_matrix(Int d, Int t) const
  {
    Set<Int> frame, frame_bd;
    return boundary_matrix_with_frame_sets(d, t, frame, frame_bd);
  }

  auto get_iter() const
  {
    return entire<indexed>(C);
  }

  // for testing.
  friend std::ostream & operator<< (std::ostream& os, const Filtration& c)
  {
    for (Int i = 0; i < c.n_cells(); ++i) os << c[i] << ",";
    return os;
  }

  template <typename MatrixType2>
  bool operator== (const Filtration<MatrixType2>& other) const
  {
    return bd_matrix == other.bd_matrix && C == other.C;
  }

  template <typename> friend struct pm::spec_object_traits;
};

} }

namespace pm{

template <>
struct spec_object_traits< Serialized< polymake::topaz::Cell > >
  : spec_object_traits<is_composite> {

  using masquerade_for = polymake::topaz::Cell;

  typedef cons<Int, cons<Int, Int>> elements;

  template <typename Me, typename Visitor>
  static void visit_elements(Me& me, Visitor& v)
  {
    v << me.deg << me.dim << me.idx;
  }
};

template <typename MatrixType>
struct spec_object_traits< Serialized< polymake::topaz::Filtration<MatrixType> > >
  : spec_object_traits<is_composite> {

  using masquerade_for = polymake::topaz::Filtration<MatrixType>;

  typedef cons<Array<polymake::topaz::Cell>, Array<MatrixType> > elements;

  template <typename Me, typename Visitor>
  static void visit_elements(Me& me, Visitor& v) //for data_load
  {
    v << me.C << me.bd_matrix;
    me.update_indices();
  }

  template <typename Visitor>
  static void visit_elements(const pm::Serialized<masquerade_for>& me, Visitor& v) //for data_save
  {
    v << me.C << me.bd_matrix;
  }
};

}
#endif
