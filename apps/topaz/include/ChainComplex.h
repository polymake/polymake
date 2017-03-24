#ifndef POLYMAKE_TOPAZ_CHAIN_COMPLEX_H
#define POLYMAKE_TOPAZ_CHAIN_COMPLEX_H

#include "polymake/Array.h"
#include "polymake/topaz/HomologyComplex.h"

namespace polymake { namespace topaz {

/** @class ChainComplex
    @brief A general chain complex represented by its differential matrices.
    @tparam MatrixType specifies the type of the differential matrices.
*/
    template<typename MatrixType>
        class ChainComplex{
            public:
            // The matrices is a map via multiplying them to a vector from the __left__.
            Array<MatrixType> bd_matrix;

            ///Create as empty.
            ChainComplex() : bd_matrix(){ }

            /**@brief Create from an Array of matrices.
             *
             * The matrices are interpreted as maps via multiplying them to a vector from the __left__.
             * @param sanity_check indicating whether to test if the input matrices' dimensions match
             *  and the maps satisfy the differential condition. default: 0
            */
            ChainComplex(const Array<MatrixType> & bd_in, bool check = false) :  bd_matrix(bd_in){
                if(check) sanity_check();
            }

            private:
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

            public:
            int dim() const{ return bd_matrix.size(); }

            /**@brief Convert the boundary matrix into a compatible type.

            @param d the number of the desired matrix.
            @tparam E The return type is either SparseMatrix<E> or Matrix<E>,
             depending on whether MatrixType is sparse or not.
             */
            template<typename E>
               typename std::conditional< MatrixType::is_sparse,SparseMatrix<E,typename MatrixType::sym_discr>,Matrix<E>>::type
               boundary_matrix(int d) const{
                    if (d<0) d+=dim()+1;
                    if (d>dim()) return zero_matrix<E>(0, bd_matrix[dim()-1].rows());
                    if (d==0) return zero_matrix<E>(bd_matrix[0].cols(), 0);
                    return convert_to<E>(bd_matrix[d-1]);
                }

            /**Return the n-th boundary matrix.
               @param n number of the desired matrix.
             */
            MatrixType boundary_matrix(int d) const{
                return boundary_matrix<typename MatrixType::value_type>(d);
            }

            ///Compare two ChainComplexes.
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
