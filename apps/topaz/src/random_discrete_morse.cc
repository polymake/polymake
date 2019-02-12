/* Copyright (c) 2014
   Michael Joswig, Frank Lutz, Mimi Tsuruga
   tsuruga@math.tu-berlin.de

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
   $Project: polymake $$Id$
*/

#include "polymake/client.h"
#include "polymake/graph/ShrinkingLattice.h"
#include "polymake/topaz/complex_tools.h"
#include <sys/time.h>
#include "polymake/RandomSubset.h"


namespace polymake { namespace topaz {

using graph::ShrinkingLattice;

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
///////////      L E X - F I R S T / L E X - L A S T     /////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////


/* *@class CompareByHasseDiagram
*   @brief Compares two ints by comparing the corresponding faces in the Hasse diagram (lex)
*/
class CompareByHasseDiagram
{
   const ShrinkingLattice<BasicDecoration>& HD_;
   const Array<int>& relabel_;

public:
  CompareByHasseDiagram(const ShrinkingLattice<BasicDecoration>& HD,const Array<int>& relabel) :
      HD_(HD),
      relabel_(relabel)
  { }

  Set<int> newlabels(const Set<int>& s) const
  {
      Set<int> permuted_vertices;
      for (auto it = entire(s); !it.at_end(); ++it) {
	 const int orig_vertex=*it;
	 permuted_vertices += relabel_[orig_vertex];
      }
      return permuted_vertices;
  }

  pm::cmp_value operator() (const int& a, const int& b) const
  {
     return operations::cmp()( newlabels( HD_.face(a) ),newlabels( HD_.face(b) ) );
  }
};


// free_faces
//   Initialize free_face_list for new max_d. Corresponds to the list "pairs"
//   from original GAP code by Lutz. Instead of tracking both the free face F
//   (dim=max_d - 1) and the face G (dim = max_d) that F is on the boundary of,
//   we only store F. A free face is a face that is contained in exactly one
//   max_d-dimensional face.
//   Procedure: Start with fresh new free_face_list. Add all faces F of
//              dim=max_d-1 such that F is on the boundary of only facet
//              (of dim=max_d).
// @param HasseDiagram newHD: updated HD, ie, some sublattice of orig_HD
// @param int max_d: dimension of maximal face in newHD
// @param Set<int,cmp> free_face_list: empty list of free faces (dim = max_d-1) in newHD sorted by relabeled label.
void lex_free_faces(const ShrinkingLattice<BasicDecoration>& newHD,
   const int& max_d,
   Set< int,CompareByHasseDiagram >& free_face_list)
{
   for (auto n=entire(newHD.nodes_of_rank(max_d)); !n.at_end(); ++n) {
      const int this_index = *n;
      if( newHD.out_degree(this_index) == 1) {
          const int remove_face = newHD.out_adjacent_nodes(this_index).front();
          if(newHD.rank(this_index)+1 == newHD.rank(remove_face)) {
                free_face_list += this_index;
          }
      }
   }
}

// collapse
//   Perform a collapse of specified face and update Hasse Diagram and list of
//   free faces.
//   Procedure: Find remove_face = face of remove_this. Delete nodes remove_this
//   		and remove_face from newHD. Check if boundary faces of
//		remove_face have become free, and if so add to free_face_list.
//		Also remove any faces from  free_face_list that are no longer
//              free faces.
// @param HasseDiagram newHD: ("global" param) will be updated within function
// @param Set<int> free_face_list: also will be updated here
// @param int remove_this: the face to begin collapse; will be of dim=max_d-1
void lex_collapse(ShrinkingLattice<BasicDecoration>& newHD, Set<int,CompareByHasseDiagram>& free_face_list,
	      const int& remove_this)
{
   Set<int> faces_of_remove_this=newHD.out_adjacent_nodes(remove_this);

   if (faces_of_remove_this.size() != 1) {
      throw std::runtime_error("random_discrete_morse::collapse: collapsing a non-free face");
   }

   // node of the face of remove_this
   const int remove_face= faces_of_remove_this.front();

   if(newHD.rank(remove_this)+1 != newHD.rank(remove_face)) {
      throw std::runtime_error("random_discrete_morse::collapse: dimensions of Hasse messed up");
   }


   // keep the nodes of boundary faces of remove_face for later use
   Set<int> bdy_of_remove_face=newHD.in_adjacent_nodes(remove_face);

   // update free_face_list
   // remember all elements of free_face_list are of dimension max_d-1

   // first remove remove_this from free_face_list
   free_face_list.erase(remove_this);


   // faces that were on the boundary of remove_face are no longer free
   for (auto s = entire(bdy_of_remove_face); !s.at_end(); ++s) {
      const int this_bdy_face = *s;
      free_face_list.erase(this_bdy_face);
   }

   // remove the nodes from the Hasse diagram
   newHD.delete_node(remove_this);
   newHD.delete_node(remove_face);


   // deletion of remove_face may add new free faces
   for (auto it=entire(bdy_of_remove_face); !it.at_end(); ++it) {
      const int this_index = *it;
      if ( newHD.out_degree(this_index) == 1) {
	 free_face_list += this_index;
      }
   }
}


Array<int> lex_discMorse(const int strategy, ShrinkingLattice<BasicDecoration> newHD, const pm::SharedRandomState& random_source, const bool print_collapsed, std::list<Set<int>>& remaining_facets)
{
   const int global_d(newHD.rank()-2);   // needed for "Warning" below
   int max_d(global_d);		   // dimension of maximum-dim face in newHD
   if ( max_d<1 ) throw std::runtime_error("random_discrete_morse::lex_discMorse: complex has only vertices ");

   int n_max_d_faces(newHD.nodes_of_rank(global_d+1).size());  // number of faces of dim = max_d
   const int n_verts(newHD.nodes_of_rank(1).size());  // number of vertices

   Array<int> morse_vector(max_d+1, 0);

   // random relabeling of vertices
   Array<int> relabel(n_verts, random_permutation(n_verts, random_source).begin());
   CompareByHasseDiagram cmp(newHD,relabel);

   // find free faces of newHD
   // remember elements of free_face_list are of dim=max_d-1
   Set< int,CompareByHasseDiagram > free_face_list(cmp);
   lex_free_faces(newHD,max_d,free_face_list);

   bool first_removed_face(true);
   bool save_remaining_faces(print_collapsed);

   while (true) {
      if (!free_face_list.empty()) {
	 // collapse anything that can be collapsed

	 const int remove_this( strategy==1 ? free_face_list.front() : free_face_list.back());

	 if (!newHD.node_exists(remove_this)) {
	    throw std::runtime_error("random_discrete_morse::lex_discMorse::can't remove this");
	 }

	 lex_collapse(newHD,free_face_list,remove_this);
	 --n_max_d_faces;

      }
      else {

	 if (n_max_d_faces==0) {
	    // if there are no more max_d faces move on to next dimension

	    --max_d;

	    if (max_d>0) {
	       n_max_d_faces = newHD.nodes_of_rank(max_d+1).size();

	       // reinitialize free_face_list
	       lex_free_faces(newHD,max_d,free_face_list);
	    }
	    else break;
	 }
	 else {
	    // otherwise, remove a face of maximal dimension
	    if (print_collapsed)
            if (!first_removed_face && save_remaining_faces) {
	       for (const auto n : newHD.nodes_of_rank_range(1, max_d+1))
		  remaining_facets.push_back(newHD.face(n));
	       save_remaining_faces=false;
	    }

	    Set<int, CompareByHasseDiagram> faces_of_maximal_dim(cmp);
	    for (const auto node_of_max_d : newHD.nodes_of_rank(max_d+1)) {
	       faces_of_maximal_dim += node_of_max_d;
	    }

	    const int critical_face( strategy==1 ? faces_of_maximal_dim.front() : faces_of_maximal_dim.back() );
	    Set<int> bdy_of_critical_face(newHD.in_adjacent_nodes(critical_face));

	    // remove critical face from Hasse
	    newHD.delete_node(critical_face);
	    n_max_d_faces--;

	    //update free_face_list
	    for (auto it=entire(bdy_of_critical_face); !it.at_end(); ++it) {
	       const int this_index(*it);
	       if ( newHD.out_degree(this_index) == 1) {
		  free_face_list += this_index;
	       }
	    }

	    ++(morse_vector[max_d]);
	 }
      }
   }


   // The remaining vertices are critical cells.
   morse_vector[0] += newHD.nodes_of_rank(1).size();


   return morse_vector;
}



//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
///////////      R A N D O M       ///////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////



// free_faces
//   Initialize free_face_list for new max_d. Corresponds to the list "pairs"
//   from original GAP code by Lutz. Instead of tracking both the free face F
//   (dim=max_d - 1) and the face G (dim = max_d) that F is on the boundary of,
//   we only store F. A free face is a face that is contained in exactly one
//   max_d-dimensional face.
//   Procedure: Start with fresh new free_face_list. Add all faces F of
//              dim=max_d-1 such that F is on the boundary of only facet
//              (of dim=max_d).
// @param HasseDiagram newHD: updated HD, ie, some sublattice of orig_HD
// @param int max_d: dimension of maximal face in newHD
// @param Set<int> free_face_list: list of free faces (dim = max_d-1) in newHD
void rand_free_faces(const ShrinkingLattice<BasicDecoration>& newHD,
const int& max_d,
Set<int>& free_face_list)
{
   for (auto n = entire(newHD.nodes_of_rank(max_d)); !n.at_end(); ++n) {
      const int this_index = *n;
      if( newHD.out_degree(this_index) == 1) {
          const int remove_face = newHD.out_adjacent_nodes(this_index).front();
          if(newHD.rank(this_index)+1 == newHD.rank(remove_face)) {
                free_face_list += this_index;
          }
      }
   }
}


// rand_collapse
//   Perform a collapse of specified face and update Hasse Diagram and list of
//   free faces.
//   Procedure: Find remove_face = face of remove_this. Delete nodes remove_thisbool first_removed_face(true);
//   		and remove_face from newHD. Check if boundary faces of
//		remove_face have become free, and if so add to free_face_list.
//		Also remove any faces from  free_face_list that are no longer
//              free faces.
// @param HasseDiagram newHD: ("global" param) will be updated within function
// @param Set<int> free_face_list: also will be updated here
// @param int remove_this: the face to begin collapse; will be of dim=max_d-1
void rand_collapse(ShrinkingLattice<BasicDecoration>& newHD, Set<int>& free_face_list,
	      const int& remove_this)
{
   Set<int> faces_of_remove_this=newHD.out_adjacent_nodes(remove_this);

   if (faces_of_remove_this.size() != 1) {
      throw std::runtime_error("random_discrete_morse::collapse: collapsing a non-free face");
   }

   // node of the face of remove_this
   const int remove_face = faces_of_remove_this.front();

   if(newHD.rank(remove_this)+1 != newHD.rank(remove_face)) {
      throw std::runtime_error("random_discrete_morse::collapse: dimensions of Hasse messed up");
   }

   // keep the nodes of boundary faces of remove_face for later use
   Set<int> bdy_of_remove_face=newHD.in_adjacent_nodes(remove_face);

   // update free_face_list
   // remember all elements of free_face_list are of dimension max_d-1

   // first remove remove_this from free_face_list
   free_face_list-=remove_this;

   // faces that were on the boundary of remove_face are no longer free
   for (auto s = entire(bdy_of_remove_face); !s.at_end(); ++s) {
      const int this_bdy_face = *s;
      free_face_list-=this_bdy_face;
   }

   // remove the nodes from the Hasse diagram
   newHD.delete_node(remove_this);
   newHD.delete_node(remove_face);

   // deletion of remove_face may add new free faces
   for (auto it=entire(bdy_of_remove_face); !it.at_end(); ++it) {
      const int this_index = *it;
      if ( newHD.out_degree(this_index) == 1) {
	 free_face_list+=this_index;
      }
   }

}


Array<int> rand_discMorse(ShrinkingLattice<BasicDecoration> newHD, const pm::SharedRandomState& random_source, const bool& print_collapsed, std::list< Set<int> >& remaining_facets)
{
   const int global_d = newHD.rank()-2;   // needed for "Warning" below
   int max_d(global_d);		   // dimension of maximum-dim face in newHD
   if ( max_d<1 ) throw std::runtime_error("random_discrete_morse::rand_discMorse: complex has only vertices");

   int n_max_d_faces=newHD.nodes_of_rank(global_d+1).size();  // number of faces of dim = max_d

   Array<int> morse_vector(max_d+1, 0);

   // find free faces of newHD
   // remember elements of free_face_list are of dim=max_d-1
   Set<int> free_face_list;
   rand_free_faces(newHD,max_d,free_face_list);

   bool first_removed_face(true);
   bool save_remaining_faces(print_collapsed);

   while (true) {
      if (!free_face_list.empty()) {
	 // collapse anything that can be collapsed

	 //choose remove_this uniformly at random
	 UniformlyRandomRanged<long> random(free_face_list.size(),random_source);
	 long r_long(random.get());

	 Set<int>::const_iterator elem=free_face_list.begin();
	 for (int elem_i=0; elem_i<r_long; ++elem_i) ++elem;
	 const int remove_this=*elem;

	 if(!newHD.node_exists(remove_this))
	    throw std::runtime_error("random_discrete_morse::rand_discMorse::can't remove this");

	 rand_collapse(newHD,free_face_list,remove_this);
	 first_removed_face=false;
	 --n_max_d_faces;

      }
      else {
	 if (n_max_d_faces==0) {
	    // if there are no more max_d faces move on to next dimension
	    --max_d;

	    if(max_d>0) {
	       n_max_d_faces = newHD.nodes_of_rank(max_d+1).size();

	       // reinitialize max_face_list and free_face_list
	       rand_free_faces(newHD,max_d,free_face_list);
	    }
	    else break;
	 }
	 else {
	    // otherwise, remove a face of maximal dimension
	    const auto faces_of_maximal_dim = newHD.nodes_of_rank(max_d+1);

	    if (!first_removed_face && save_remaining_faces) {
	       for (const auto n : newHD.nodes_of_rank_range(1, max_d+1))
	          remaining_facets.push_back(newHD.face(n));

	       save_remaining_faces=false;
	    }

	    // choose random critical face
	    UniformlyRandomRanged<long> rand(faces_of_maximal_dim.size(),random_source);
	    long r_long(rand.get());
	    auto elem=faces_of_maximal_dim.begin();
	    for (int elem_i=0; elem_i<r_long; ++elem_i) ++elem;
	    const int critical_face(*elem);

	    Set<int> bdy_of_critical_face(newHD.in_adjacent_nodes(critical_face));

	    // remove critical face from Hasse
	    newHD.delete_node(critical_face);
	    first_removed_face=false;
	    --n_max_d_faces;

	    // update free_face_list
	    for (auto it=entire(bdy_of_critical_face); !it.at_end(); ++it) {
	       const int this_index(*it);
	       if (newHD.out_degree(this_index) == 1) {
		  free_face_list+=this_index;
	       }
	    }

	    ++(morse_vector[max_d]);
	 }
      }
   }


   // The remaining vertices are critical cells.
   morse_vector[0] += newHD.nodes_of_rank(1).size();

   return morse_vector;
}


//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
///////////      M A I N       ///////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////


Map< Array<int>,int > random_discrete_morse(const Lattice<BasicDecoration> orig_HD, UniformlyRandom<long> random_source , const int strategy, const bool verbose, const int rounds, const Array<int> try_until_reached,  const Array<int> try_until_exception,  std::string save_to_filename)
{
   if (strategy<0 || strategy>2) throw std::runtime_error("random_discrete_morse::Invalid strategy type.");

   bool reached = false;

   const bool tries = !try_until_reached.empty();
   const bool try_exception = !try_until_exception.empty();

   if (tries && try_exception) throw std::runtime_error("random_discrete_morse::Can't run both try_until_reached and try_until_exception");

   const bool save_collapsed = (save_to_filename.length() != 0);

   if (verbose) {
      cout<<"random_discrete_morse version 02.02.2015"<<endl;

      cout<<"Options:"<<endl;
      cout<<"   strategy            = "<<strategy<<endl;
      cout<<"   rounds              = "<<rounds<<endl;
      if (tries)
      cout<<"   try_until_reached   = "<<try_until_reached<<endl;
      if (try_exception)
      cout<<"   try_until_exception = "<<try_until_exception<<endl;
      cout<<"   seed                = "<< random_source.get() <<endl;
      if (save_collapsed)
      cout<<"   save collapsed to   = "<<save_to_filename<<endl;
      cout<<endl;
   }

   Map< Array<int>,int > morse_table;
   long int avg_morse_vec_timing(0);

   for (int this_round=0; this_round<rounds && !reached ; ++this_round) {
      std::list< Set<int> > remaining_facets;

      timeval start_morse_vec_timing;
      timeval end_morse_vec_timing;

      if (verbose) gettimeofday(&start_morse_vec_timing,NULL);

////////////////////////////////////////////////////////////////////////////////////
///////////////////   MAIN PART   //////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
      Array<int> morse_vec;

      if (strategy==0)
         morse_vec= rand_discMorse(orig_HD,random_source,save_collapsed,remaining_facets);
      else
         morse_vec= lex_discMorse(strategy,orig_HD,random_source,save_collapsed,remaining_facets);

////////////////////////////////////////////////////////////////////////////////////

      if (verbose) {
	 gettimeofday(&end_morse_vec_timing,NULL);
	 const long int morse_vec_timing(end_morse_vec_timing.tv_sec - start_morse_vec_timing.tv_sec);
	 avg_morse_vec_timing += morse_vec_timing;
	 if ( this_round%verbose==0 || verbose==1 ) cout<<"round "<<this_round<<" ... done"<<endl;
      }

      if (tries) {
	 if (morse_vec == try_until_reached) {
	    reached=true;
	    cout<<"Reached ( "<<try_until_reached<<" ) in "<<this_round + 1<<" round"<<endl;
	 }
      }

      if (try_exception) {
	 if (morse_vec != try_until_exception) {
	    reached=true;
	    cout<<"Found ( "<<morse_vec<<" ) != ( "<<try_until_exception<<" ) at round "<<this_round +1<<endl;
	 }
      }

      ++(morse_table[morse_vec]);

      if (!remaining_facets.empty()) {
           perl::Object save_complex("SimplicialComplex");
	   save_complex.set_description()    << "Simplicial complex obtained by a sequence of random collapses."
                                             << "\nparameters for the random_discrete_morse function:"
                                             << "\nstrategy:       " << strategy
                                             << "\nseed:           " << random_source.get()
                                             << "\nfound on round: " << this_round
                                             << endl;
          save_complex.take("INPUT_FACES")   << remaining_facets;
          std::stringstream ss;
          ss <<save_to_filename<<"_"<<this_round;
          std::string filename(ss.str());
          save_complex.save(filename);
      }
   }

   if (verbose) cout<<"average time per round = "<< avg_morse_vec_timing/rounds <<" secs"<<endl;

   return morse_table;
}

Map< Array<int>,int > random_discrete_morse_sc(const perl::Object& p_in, perl::OptionSet options){

   const bool verbose = options["verbose"];

   if (verbose){
      const Array<int> fvec = p_in.give("F_VECTOR");
      const bool is_pure = p_in.give("PURE");
      const bool is_closed = p_in.give("CLOSED_PSEUDO_MANIFOLD");
      const bool is_pmf = p_in.give("PSEUDO_MANIFOLD");

      cout<< "A brief description of the input SimplicialComplex:"  << endl;
      cout<< "  f-vector:        " << fvec <<endl;
      cout<< "  pure:            " << (is_pure?"true":"false" ) <<endl;
      cout<< "  closed:          " << (is_closed?"true":"false" ) <<endl;
      cout<< "  pseudo-manifold: " << (is_pmf?"true":"false" ) <<endl<<endl;
   }

   timeval start_hasse_timing;
   timeval end_hasse_timing;

   gettimeofday(&start_hasse_timing,NULL);

   perl::Object orig_HD_obj = p_in.give("HASSE_DIAGRAM");
   const Lattice<BasicDecoration> orig_HD(orig_HD_obj); // = p_in.give("HASSE_DIAGRAM");

   gettimeofday(&end_hasse_timing,NULL);

   const unsigned int vert_label_chk=p_in.give("N_VERTICES");
   if (vert_label_chk!=orig_HD.nodes_of_rank(1).size())
      cout<<"random_discrete_morse::Vertex labels in FACETS not nice. Try using INPUT_FACES instead.";

   const long int hasse_timing(end_hasse_timing.tv_sec - start_hasse_timing.tv_sec);

   if (verbose) {
      cout<<"Hasse Diagram computed in "<< hasse_timing <<" secs" << endl;
   }

   // use of seperate parameters for options is necessary as OptionSets are read-only

   RandomSeed seed = options["seed"];
   UniformlyRandom<long> random_source(seed);
   int str = options["strategy"];
   bool ver = options["verbose"];
   int r = options["rounds"];
   Array<int> tur = options["try_until_reached"];
   Array<int> tue = options["try_until_exception"];
   std::string sc = options["save_collapsed"];

   return random_discrete_morse(orig_HD, random_source, str,ver,r,tur,tue,sc);

}

UserFunction4perl("# @category Other"
                  "# Implementation of random discrete Morse algorithms by Lutz and Benedetti"
                  "# Returns a map of the number of occurrences of different reduction results indexed by the corresponding discrete Morse vectors (containing the number of critical cells per dimension)"
                  "# @param SimplicialComplex complex"
                  "# @option Int rounds Run for //r// rounds"
                  "# @option Int seed Set seed number for random number generator"
                  "# @option Int strategy Set //strategy//=>0 (default) for random-random: uniformly random selecting of a face to collapse or as critical face"
                  "#               Set //strategy//=>1 for random-lex-first: uniformly random relabeling of vertices, then selecting lexicographically first face for collapse or as a critical face"
                  "#               Set //strategy//=>2 for random-lex-last: uniformly random relabeling of vertices, then selecting lexicographically last face for collapse or as a critical face"
                  "# @option Int verbose //v// Prints message after running every //v// rounds"
                  "# @option Array<Int> try_until_reached Used together with //rounds//=>r; When //try_until_reached//=>[a,...,b], runs for //r// rounds or until [a,...,b] is found"
                  "# @option Array<Int> try_until_exception Used together with //rounds//=>r; When //try_until_exception//=>[a,...,b], runs for //r// rounds or until anything other than [a,...,b] is found"
                  "# @option String save_collapsed Save all facets that remain after initial collapse to an XML file of a Simplicial Complex. Rounds that have Morse vector [1,0,...,0] or [1,0,...,0,1] will save nothing. Filename must have quotation marks: //save_collapsed//=>\"path/to/filename\". The XML files are saved as \"path/to/filename_currentround.top\"."
		  "# @return Map< Array<Int>, Int >",
		   &random_discrete_morse_sc,
		  "random_discrete_morse(SimplicialComplex { seed=> undef, strategy => 0, verbose => 0, rounds => 1, try_until_reached => undef, try_until_exception => undef, save_collapsed => undef })");

} }




// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
