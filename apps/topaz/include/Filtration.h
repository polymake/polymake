#ifndef POLYMAKE_TOPAZ_FILTRATION_H
#define POLYMAKE_TOPAZ_FILTRATION_H

#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/SparseVector.h"
#include "polymake/graph/HasseDiagram.h"
#include "polymake/internal/matrix_methods.h"

namespace polymake { namespace topaz {

    //dimension and index are needed to find the corresponding boundary via the given matrices.
    struct Cell{
        int deg,dim,idx;
        Cell(){
            deg=dim=idx=0;
        }

        Cell(int deg_in, int dim_in, int idx_in){
            deg = deg_in;
            dim = dim_in;
            idx = idx_in;
        }

        bool operator!= (const Cell & other) const{
            return deg!=other.deg || dim!=other.dim || idx!=other.idx;
       }

        //for testing.
        friend std::ostream & operator<<(std::ostream & os, const Cell & c){
            os << "(" << c.deg << "," << c.dim << "," << c.idx << ")";
            return os;
        }

    };

    template<typename MatrixType>
        class Filtration{
            typedef typename MatrixType::value_type Coeff;
            typedef typename MatrixType::persistent_nonsymmetric_type MatNS;

            public:
            Array<Cell> C; //after object initialization, this array is always sorted as required by the persistent homology algorithm.
            Array<MatrixType> bd_matrix; //boundary matrices by dimension. idx of the cells corresponds to the row in the corresponding matrix
            Array<Array<int> > ind; //keep track of indices to implement proper bd function

            Filtration() : C(), bd_matrix(), ind(){ }

            Filtration(Array<Cell> & C_in, const Array<MatrixType> & bd_in, bool sorted=false) : C(C_in), bd_matrix(bd_in), ind(bd_in.size()){
                if(sorted) update_indices();
                else sort();
            }

            Filtration(const graph::HasseDiagram & HD, Array<int> degs) : C(HD.nodes()-2), //-2 for empty set and dummy
            bd_matrix(HD.dim()+1) {

                int dim = HD.dim();

                std::vector<int> dim_index = HD.dims();

                int n_bd = dim_index[dim]-dim_index[dim-1]; //number of vertices
                bd_matrix[0] = ones_matrix<Coeff>(n_bd,1);
                for(int i=dim_index[dim-1]; i<dim_index[dim-1]+n_bd; ++i)
                    C[i-1] = Cell(degs[i-1],0,i-dim_index[dim-1]);

                //compute boundary matrices for d>0:
                for(int d=1; d<dim; d++){
                    int n = dim_index[dim-d]-dim_index[dim-d-1];//number of d-simplices
                    MatrixType bd(n,n_bd);
                    n_bd = n;

                    int r=0; //row index;
                    for (auto f=entire(HD.node_range_of_dim(d)); !f.at_end(); ++f){//iterate over d-faces
                        Coeff sgn = Coeff(1);
                        C[*f-1] = Cell(degs[*f-1],d,r);//put indices into cell array
                        for (auto subf=HD.in_adjacent_nodes(*f).begin();  !subf.at_end(); ++subf){//iterate over their boundary
                            int s = *subf;
                            bd[r][s-dim_index[dim-d]]=sgn;
                            sgn = -sgn;
                        }
                        r++;
                    }

                    bd_matrix[d] = bd;
                }

                sort();
            }


            //keeps track of ind matrix for easy access of boundaries
            void update_indices(){
                ind.resize(bd_matrix.size());
                for(auto i = ensure(ind,(pm::cons<pm::end_sensitive, pm::indexed>*)0).begin(); !i.at_end(); ++i)
                    (*i).resize(bd_matrix[i.index()].rows());
                for (auto c=ensure(C, (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin(); !c.at_end(); ++c) {
                    ind[(*c).dim][(*c).idx] = c.index();
                }
            }

            public:
            int n_cells() const{
                return C.size();
            }

            int n_frames() const{
                return C[n_cells()-1].deg; //TODO this only works with sorted array...
            }

            int dim() const{
                return bd_matrix.size()-1;
            }

            typedef SparseVector<Coeff> Chain;
            Chain bd(int i) const{
                Cell cell = C[i];
                int d = cell.dim;
                Chain c(C.size());
                if(d==0) return c; //points have empty boundary
                Chain b = bd_matrix[d].row(cell.idx);
                for (auto e = entire(b); !e.at_end(); ++e) {
                    c[ind[d-1][e.index()]] = *e;
                }
                return c;
            }

            // sort cells by degree first and dimension second, as required by persistent homology algo.
            private:
            struct cellComparator {
                bool operator()(const Cell& c1, const Cell& c2) {
                    if (c1.deg < c2.deg) return true;
                    if (c1.deg == c2.deg){
                        if(c1.dim < c2.dim) return true;
                        if(c1.dim == c2.dim) return c1.idx < c2.idx; // idx gets sorted lex to allow equality checking
                    }
                    return false;
                }
            };

            void sort(){
                std::sort(C.begin(), C.end(), cellComparator());
                update_indices();//TODO do this while sorting?
            }

            public:
            const Cell operator[](int i) const{
                return C[i];
            }

            const MatrixType boundary_matrix(int d){
                return bd_matrix[d];
            }

			const Array<Cell> cells() const{
			    return C;
			}

            //returns d-bd matrix of t-th frame TODO accept non-sorted filtrations?
            pm::MatrixMinor<MatrixType&, const Set<int>&, const Set<int>& > boundary_matrix(int d, int t){
                if(t>n_frames()) throw std::runtime_error("Filtration: input exceeds number of frames");
                if(d>dim()) throw std::runtime_error("Filtration: input exceeds filtration dimension");
                MatrixType B = bd_matrix[d];
                Set<int> frame; //indices of d-simplices present in this frame
                Set<int> frame_bd; //indices of d-1-simplices present

                for(auto i = ensure(ind[d],(pm::cons<pm::end_sensitive, pm::indexed>*)0).begin(); !i.at_end(); ++i){
                    if(C[*i].deg <= t) frame += i.index();
                }
                if(d>0){
                    for(auto i = ensure(ind[d-1],(pm::cons<pm::end_sensitive, pm::indexed>*)0).begin(); !i.at_end(); ++i){
                        if(C[*i].deg <= t) frame_bd += i.index();
                    }
                }else frame_bd = sequence(0,B.cols());

                return B.minor(frame,frame_bd);
            }

            pm::ensure_features<Array<Cell>, pm::cons<pm::end_sensitive, pm::indexed> >::const_iterator get_iter() const{
                return ensure(C, (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin();
            }

            //for testing.
            friend std::ostream & operator<<(std::ostream & os, const Filtration & c){
                for(int i=0; i<c.n_cells(); ++i) os << c[i] << ",";
                return os;
            }

            template<typename MatrixType2>
            bool operator==(const Filtration<MatrixType2> & other) const{
                return bd_matrix == other.bd_matrix && C == other.C;
            }

            template <typename> friend struct pm::spec_object_traits;
        };


}}

namespace pm{

    template <>
        struct spec_object_traits< Serialized< polymake::topaz::Cell > > :
        spec_object_traits<is_composite> {

            typedef polymake::topaz::Cell masquerade_for;

            typedef cons<int,cons<int,int> > elements;

            template <typename Me, typename Visitor>
                static void visit_elements(Me& me, Visitor& v)
                {
                    v << me.deg << me.dim << me.idx;
                }
        };
    template <typename MatrixType>
        struct spec_object_traits< Serialized< polymake::topaz::Filtration<MatrixType> > > :
        spec_object_traits<is_composite> {

            typedef polymake::topaz::Filtration<MatrixType> masquerade_for;

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
