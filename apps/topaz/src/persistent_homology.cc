#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Smith_normal_form.h"
#include "polymake/SparseVector.h"
#include "polymake/topaz/Filtration.h"
#include "polymake/integer_linalg.h"
#include "polymake/list"
#include "polymake/vector"
#include <string>
#include <sstream>

namespace polymake{ namespace topaz{

template <typename Matrix_type>
class PersistentHomology {

  using Coeff = typename Matrix_type::value_type;
  using Chain = SparseVector<Coeff>;

private:
  const Filtration<Matrix_type>& F;
  int dim;
  std::vector<bool> marked;
  Array<std::pair<int, Chain>> T;

public:
  PersistentHomology(const Filtration<Matrix_type>& F_in)
    : F(F_in)
    , dim(F.dim())
    , marked(F.n_cells())
    , T(F.n_cells())
  {
    // zero as first pair value means empty. that's ok, as the zeroth filtration entry is always a point and thus has empty bd and does not get an entry in T.
  }

  Chain remove_pivot_rows(int j)
  {
    Chain d = F.bd(j);

    for (auto m = entire<indexed>(d); !m.at_end(); ) {
      if (!marked[m.index()]) d.erase(m++); //remove unmarked cells
      else ++m;
    }

    while (!d.empty()) {
      int i = indices(d).back(); //maximal index
      if (!T[i].first) break;
      Chain t = T[i].second;
      Coeff q = t[i];
      Coeff qq = d[i];
      d -= qq*(inv(q)*t); //TODO this differs from what's in the paper. they don't multiply with qq. why?
    }
    return d;
  }

public:
  Array<std::list<std::pair<int, int>>> compute_intervals()
  {
    Array<std::list<std::pair<int,int>>> L(dim+1);

    for (auto c = F.get_iter(); !c.at_end(); ++c) {
      int j = c.index();
      Chain d = remove_pivot_rows(j);
      if (d.empty()) {
        marked[j] = true;
      } else {
        int i = indices(d).back(); // maximal index
        int k = F[i].dim;
        T[i].first = j;
        T[i].second = d;
        int a = F[i].deg;
        int b = (*c).deg;
        if (a<b) //skip empty intervals
          L[k].push_back(std::pair<int,int>(a,b));
      }
    }

    for (auto c = F.get_iter(); !c.at_end(); ++c) {
      int j = c.index();
      if (marked[j] && !T[j].first) {
        L[(*c).dim].push_back(std::pair<int,int>((*c).deg,-1));
      }
    }

    return L;
  }
};

template<typename MatrixType>
std::enable_if_t<pm::is_field<typename MatrixType::value_type>::value, Array<std::list<std::pair<int, int>>> > //for field coefficients
persistent_homology(const Filtration<MatrixType>& F)
{
  PersistentHomology<MatrixType> P(F);
  return P.compute_intervals();
}

// using snf for nullspace and span computation for performance reasons
template<typename MatrixType>
SparseMatrix<typename MatrixType::value_type> rowspan_snf(const MatrixType& M_in)
{
  SmithNormalForm<typename MatrixType::value_type> S = smith_normal_form(M_in);
  return S.form.minor(All,sequence(0,S.rank)) * S.right_companion.minor(sequence(0,S.rank),All);
}

template<typename MatrixType>
SparseMatrix<typename MatrixType::value_type> null_space_snf(const MatrixType& M_in)
{
  SmithNormalForm<typename MatrixType::value_type> S = smith_normal_form(M_in,1);
  return S.left_companion.minor(range(S.rank,S.left_companion.rows()-1),All);
}

template<typename MatrixType>
std::enable_if_t< std::numeric_limits<typename MatrixType::value_type>::is_integer, //for integer coefficients
                  std::pair<SparseMatrix<typename MatrixType::value_type>,
                            std::list< std::pair<typename MatrixType::value_type, SparseMatrix<typename MatrixType::value_type> > > > >
//less verbose: pair< SparseMatrix<Coeff>, list<pair< Coeff, SparseMatrix<Coeff>>>>
persistent_homology(Filtration<MatrixType> F, int i, int p, int k)
{
  using Coeff = typename MatrixType::value_type;
  using Generators = SparseMatrix<Coeff>;    // the _rows_ of this matrix represent a generating set
  using Torsion = std::list<std::pair<Coeff, Generators>>;  //torsion coefficient and generating set of the corresponding torsion module

  Set<int> M_frame, M_frame_bd;
  const MatrixType M = F.boundary_matrix_with_frame_sets(k, i, M_frame, M_frame_bd);

  if (!M.rows()) return std::pair<Generators, Torsion>(); //Z empty.

  Generators Z;
  if (k==0)
    Z = unit_matrix<Coeff>(M.rows()); //1-cells always have empty boundary
  else
    Z = null_space_snf(M); //ker(d_k)

  if (!Z.rows()) return std::pair<Generators, Torsion>(); //Z empty

  Set<int> M2_frame, simpl_p;
  const MatrixType M2 = F.boundary_matrix_with_frame_sets(k+1, i+p, M2_frame, simpl_p);

  Set<int> frame = simpl_p - M_frame; //indices of k-simplices present int frame i+p, but not yet in frame i

  // embedding: insert zero cols for cells missing in Z, i.e. not yet there in frame i.
  ListMatrix<SparseVector<Coeff>> ZZlist(0, Z.cols());
  int j=0;
  int maxindex = simpl_p.back();
  for (auto c = entire(cols(Z)); maxindex>=j; ++j) {
    if (frame.contains(j)) {
      ZZlist /= zero_vector<Coeff>(Z.rows());  //cell only present in B
    } else if(!c.at_end() && simpl_p.contains(j)) {  // cell present in both
      ZZlist /= *c; ++c;
    }
  }
  SparseMatrix<Coeff> ZZ(T(ZZlist));

  if (!M2.rows()) return std::pair<Generators, Torsion>(ZZ,Torsion()); //B empty

  const SparseMatrix<Coeff> B = rowspan_snf(M2); // im(d_k+1)

  if (!B.rows()) return std::pair<Generators, Torsion>(ZZ,Torsion()); //B empty

  Generators U = null_space_snf<MatrixType>((ZZ/B)).minor(All,sequence(0,Z.rows()));
  // U*ZZ is a basis of the intersection. U is thus the relation matrix for the persistence module relative to ZZ

  if (!U.rows()) return std::pair<Generators, Torsion>(ZZ,Torsion()); //intersection empty

  SmithNormalForm<Coeff> SNF4 = smith_normal_form(U);

  // find a basis for torsional part: rows of left companion corresponding to non-zero non-one diagonal entries
  Generators R = SNF4.right_companion * ZZ;
  Torsion tor;
  int r = SNF4.rank;
  for (auto t=entire<reversed>(SNF4.torsion); !t.at_end(); ++t) {
    const int mult = t->second;
    tor.push_front(std::pair<Coeff, Generators>{ t->first, R.minor(sequence(r-mult, mult),All) });
    r -= mult;
  }

  // basis for the non-torsional part...
  Generators free;
  int prank = Z.rows() - SNF4.rank;
  if (prank>0) free = R.minor(sequence(SNF4.rank,prank),All);

  return std::pair<Generators, Torsion>(free, tor);
}

UserFunctionTemplate4perl("# @category Other"
                          "# Given a Filtration and three indices i,p and k, this computes the p-persistent k-th homology group of the i-th frame of the filtration for coefficients from any PID. Returns a basis for the free part and a list of torsion coefficients with bases."
                          "# @param Filtration<MatrixType> F"
                          "# @param Int i the filtration frame"
                          "# @param Int p the number of frames to consider"
                          "# @param Int k the dimension in which to compute"
                          "# @tparam MatrixType type of boundary matrices"
                          "# @return Pair<SparseMatrix<SCALAR>, List< Pair<SCALAR, SparseMatrix<SCALAR> > > >",
                          "persistent_homology<MatrixType>($$$$)");

UserFunctionTemplate4perl("# @category Other"
                          "# Given a Filtration, this computes its persistence barcodes in all dimension, using the algorithm described in the 2005 paper 'Computing Persistent Homology' by Afra Zomorodian and Gunnar Carlsson. It only works for field coefficients."
                          "# @param Filtration<MatrixType> F"
                          "# @tparam MatrixType type of the boundary matrices"
                          "# @return Array<List<Pair<Int, Int> > >",
                          "persistent_homology<MatrixType>($)");
} }
