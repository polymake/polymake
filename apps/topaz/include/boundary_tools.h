
#ifndef POLYMAKE_TOPAZ_BOUNDARY_TOOLS_H
#define POLYMAKE_TOPAZ_BOUNDARY_TOOLS_H


namespace polymake { namespace topaz {

//the following takes non-redundant, non-including faces and adjusts the numbering,
//returning a map from new indices to old ones. used in connection with boundary constructions.
struct ind2map_consumer {
      mutable Array<int> map;
      mutable int n_verts = 0;
      ind2map_consumer(int n) : map(n){ };
      void operator() (const int& old_index, int& new_index) const {
         map[new_index] = old_index;
         n_verts = std::max(n_verts, new_index+1);
      }
      Array<int> give_map(){
         return Array<int>(n_verts,map.begin());
      }
   };

std::pair< Array<Set<int>>, Array<int> > squeeze_faces(IncidenceMatrix<> faces){
      auto c = ind2map_consumer(faces.cols());
      faces.squeeze_cols(c);
      return std::make_pair(Array<Set<int>>(rows(faces)), c.give_map());
   }

}}
#endif // POLYMAKE_TOPAZ_BOUNDARY_TOOLS_H

