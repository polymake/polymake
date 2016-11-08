#ifndef POLYMAKE_TOPAZ_CHAIN_COMPLEX_H
#define POLYMAKE_TOPAZ_CHAIN_COMPLEX_H

#include "polymake/Array.h"
#include "polymake/topaz/HomologyComplex.h"

namespace polymake { namespace topaz {

    template<typename MatrixType>
        class ChainComplex{
            public:
            //the matrices are maps via _left_ multiplication
            Array<MatrixType> bd_matrix;

            ChainComplex() : bd_matrix(){ }

            ChainComplex(const Array<MatrixType> & bd_in, bool check = false) :  bd_matrix(bd_in){
                if(check) sanity_check();
            }

            void sanity_check(){
                for (auto d = entire(bd_matrix); !d.at_end() && !std::next(d).at_end(); ++d){
                    auto next = std::next(d);
                    if ((*d).rows() != (*next).cols())
                        throw std::runtime_error("ChainComplex - matrix dimensions incompatible");
                    else{
                         MatrixType prod =(*next)*(*d);
                        if(prod != zero_matrix<typename MatrixType::value_type>(prod.rows(),prod.cols()))
                            throw std::runtime_error("ChainComplex - differential condition not satisfied");
                    }
                }
            }


            int dim() const{ return bd_matrix.size(); }

            //this lets us compute homology for any compatible coefficient type
            //the return type is either SparseMatrix<E> or Matrix<E> depending on whether MatrixType is sparse or not
            template<typename E>
               typename std::conditional< MatrixType::is_sparse,SparseMatrix<E,typename MatrixType::sym_discr>,Matrix<E>>::type
               boundary_matrix(int d) const{
                    if (d<0) d+=dim()+1;
                    if (d>dim()) return zero_matrix<E>(1, bd_matrix[dim()-1].rows());
                    if (d==0) return ones_matrix<E>(bd_matrix[0].cols(), 1);
                    return convert_to<E>(bd_matrix[d-1]);
                }

            //for the perl user method, templates are not possible
            MatrixType boundary_matrix(int d) const{
                return boundary_matrix<typename MatrixType::value_type>(d);
            }

            template<typename MatrixType2>
            bool operator==(const ChainComplex<MatrixType2> & other) const{
                return bd_matrix == other.bd_matrix;
            }
        };
}}

namespace pm{
    template <typename MatrixType>
        struct spec_object_traits< Serialized< polymake::topaz::ChainComplex<MatrixType> > > :
        spec_object_traits<is_composite> {

            typedef polymake::topaz::ChainComplex<MatrixType> masquerade_for;

            typedef Array<MatrixType> elements;

            template <typename Me, typename Visitor>
                static void visit_elements(Me& me, Visitor& v) //for data_load
                {
                    v << me.bd_matrix;
                }

            template <typename Visitor>
                static void visit_elements(const pm::Serialized<masquerade_for>& me, Visitor& v) //for data_save
                {
                    v << me.bd_matrix;
                }
        };

}

#endif
