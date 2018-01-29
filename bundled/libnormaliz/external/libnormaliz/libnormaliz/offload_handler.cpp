#ifdef NMZ_MIC_OFFLOAD

#pragma offload_attribute (push, target(mic))
#include "libnormaliz/offload_handler.h"
#include <offload.h>  // offload system header
#include "libnormaliz/matrix.h"
#include "libnormaliz/full_cone.h"
#include "libnormaliz/list_operations.h"
#include "libnormaliz/vector_operations.h"
#include "libnormaliz/my_omp.h"
#include "libnormaliz/HilbertSeries.h"
#include <cstring> // for strcpy
#include <iostream>
#include <fstream>

namespace libnormaliz {

using namespace std;

// transfering vector
template<typename Integer>
void fill_vector(vector<Integer>& v, long size, Integer* data)
{
  for (long i=0; i<size; i++)
    v[i] = data[i];
}

template<typename Integer>
void fill_plain(Integer* data, long size, const vector<Integer>& v)
{
  for (long i=0; i<size; i++)
      data[i] = v[i];
}

// transfering Matrix
template<typename Integer>
void fill_matrix(Matrix<Integer>& M, long rows, long cols, Integer* data)
{
  for (long i=0; i<rows; i++)
    for (long j=0; j<cols; j++)
      M[i][j] = data[i*cols+j];
}

template<typename Integer>
void fill_plain(Integer* data, long rows, long cols, const Matrix<Integer>& M)
{
  for (long i=0; i<rows; i++)
    for (long j=0; j<cols; j++)
      data[i*cols+j] = M[i][j];
}

// transfering list<vector>
// the vectors may have different lengths
// fill_list_vector also creates the list entries and appendes them to the list!
template<typename Integer>
void fill_list_vector(list< vector<Integer> >& l, long plain_size, Integer* data)
{
  Integer* data_end = data + plain_size; // position after last entry
  while (data < data_end)
  {
    l.push_back(vector<Integer>(*data));
    fill_vector(l.back(), *data, data+1);
    data += *data + 1;
  }
}

template<typename Integer>
void fill_plain(Integer* data, long size, const list< vector<Integer> >& l)
{
  long v_size;
  typename list< vector<Integer> >::const_iterator it;
  for (it = l.begin(); it != l.end(); it++)
  {
    v_size = it->size();
    *data = v_size;
    fill_plain(++data, v_size, *it);
    data += v_size;
  }
}

template<typename Integer>
long plain_size(const list< vector<Integer> >& l)
{
  long size = 0;
  typename list< vector<Integer> >::const_iterator it;
  for (it = l.begin(); it != l.end(); it++)
    size += it->size() + 1;
  return size;
}
#pragma offload_attribute (pop)

//-------------------------- OffloadHandler ---------------------------------

template<typename Integer>
OffloadHandler<Integer>::OffloadHandler(Full_Cone<Integer>& fc, int mic_number)
  : mic_nr(mic_number),
    running(false),
    local_fc_ref(fc)
{
  cout << "mic " << mic_nr<< ": Offload and initialize Full_Cone..." << endl;
  create_full_cone();

  transfer_bools();
  transfer_support_hyperplanes();
  transfer_grading();            // including truncation and shift
  transfer_triangulation_info(); // extreme rays, deg1_triangulation, Order_Vector

  primal_algorithm_initialize();
  cout << "mic " << mic_nr<< ": Full_Cone initialized." << endl;
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::create_full_cone()
{
  const Matrix<Integer>& M = local_fc_ref.Generators;
  long nr = M.nr_of_rows();
  long nc = M.nr_of_columns();
  long size = nr*nc;
  Integer *data = new Integer[size];
  fill_plain(data, nr, nc, M);

  //  cout << "mic " << mic_nr<< ": Offload Full_Cone..." << endl;
  // offload to mic, copy data and free it afterwards, but keep a pointer to the created Full_Cone
  #pragma offload target(mic:mic_nr) in(nr,nc) in(data: length(size) ONCE)
  {
    omp_set_num_threads(118);
    Matrix<Integer> gens(nr, nc);
    fill_matrix(gens, nr, nc, data);
    offload_fc_ptr = new Full_Cone<Integer>(gens);
  }
//  cout << "mic " << mic_nr<< ": Offload Full_Cone completed." << endl;
  delete[] data;
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::transfer_bools()
{
//  cout << "mic " << mic_nr<< ": transfer_bools" << endl;
  Full_Cone<Integer>& foo_loc = local_fc_ref;  // prevents segfault
  //TODO segfaults should be resolved in intel compiler version 2015
  bool is_computed_pointed = local_fc_ref.isComputed(ConeProperty::IsPointed);
    bool inhomogeneous      = foo_loc.inhomogeneous;
    bool do_Hilbert_basis   = foo_loc.do_Hilbert_basis;
    bool do_h_vector        = foo_loc.do_h_vector;
    bool keep_triangulation = foo_loc.keep_triangulation;
    bool do_multiplicity    = foo_loc.do_multiplicity;
    bool do_determinants    = foo_loc.do_determinants;
    bool do_triangulation   = foo_loc.do_triangulation;
    bool do_deg1_elements   = foo_loc.do_deg1_elements;
    bool do_Stanley_dec     = foo_loc.do_Stanley_dec;
    bool do_approximation   = foo_loc.do_approximation;
    bool do_default_mode    = foo_loc.do_default_mode;
    bool deg1_generated = foo_loc.deg1_generated;
    bool pointed = foo_loc.pointed;
  #pragma offload target(mic:mic_nr) in(mic_nr)
  {
    // bool foo = offload_fc_ptr->inhomogeneous;  // prevents segfault
    offload_fc_ptr->inhomogeneous      = inhomogeneous;
    offload_fc_ptr->do_Hilbert_basis   = do_Hilbert_basis;
    offload_fc_ptr->do_h_vector        = do_h_vector;
    offload_fc_ptr->keep_triangulation = keep_triangulation;
    offload_fc_ptr->do_multiplicity    = do_multiplicity;
    offload_fc_ptr->do_determinants    = do_determinants;
    offload_fc_ptr->do_triangulation   = do_triangulation;
    offload_fc_ptr->do_deg1_elements   = do_deg1_elements;
    offload_fc_ptr->do_Stanley_dec     = do_Stanley_dec;
    offload_fc_ptr->do_approximation   = do_approximation;
    offload_fc_ptr->do_default_mode    = do_default_mode;
    offload_fc_ptr->do_all_hyperplanes = false;
    // deg1_generated could be set more precise
    offload_fc_ptr->deg1_triangulation = deg1_generated;
    if (is_computed_pointed)
    {
      offload_fc_ptr->pointed = pointed;
      offload_fc_ptr->is_Computed.set(ConeProperty::IsPointed);
    }
    offload_fc_ptr->verbose = true;
    string fstr = "/tmp/mic";
    fstr.push_back(static_cast<char>('0'+mic_nr));
    fstr.append(".log");
    cout << "writing to " << fstr << endl;
    ofstream *fout = new ofstream(fstr.c_str());
    setVerboseOutput(*fout);
    setErrorOutput(*fout);
    verboseOutput() << "Start logging" << endl;
  }
//  cout << "mic " << mic_nr<< ": transfer_bools done" << endl;
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::transfer_support_hyperplanes()
{
  if (!local_fc_ref.isComputed(ConeProperty::SupportHyperplanes))
    return;

//  cout << "mic " << mic_nr<< ": transfer_support_hyperplanes" << endl;
  const Matrix<Integer>& M = local_fc_ref.Support_Hyperplanes;
  long nr = M.nr_of_rows();
  long nc = M.nr_of_columns();
  long size = nr*nc;
  assert(size > 0); // make sure there are support hyperplanes computed
  Integer *data = new Integer[size];
  fill_plain(data, nr, nc, M);

  // offload to mic, copy data and create the C++ matrix in the offloaded Full_Cone
  #pragma offload target(mic:mic_nr) in(nr,nc) in(data: length(size) ONCE)
  {
    offload_fc_ptr->Support_Hyperplanes = Matrix<Integer>(nr, nc);
    fill_matrix(offload_fc_ptr->Support_Hyperplanes, nr, nc, data);
    offload_fc_ptr->nrSupport_Hyperplanes = nr;
    offload_fc_ptr->is_Computed.set(ConeProperty::SupportHyperplanes);
  }
  delete[] data;

//  cout << "mic " << mic_nr<< ": transfer_support_hyperplanes done" << endl;
  if (local_fc_ref.ExcludedFaces.nr_of_rows() > 0)
  {
    const Matrix<Integer>& ExFaces = local_fc_ref.ExcludedFaces;

    nr = ExFaces.nr_of_rows();
    nc = ExFaces.nr_of_columns();
    size = nr*nc;
    assert(size > 0); // make sure there are support hyperplanes computed
    Integer *data = new Integer[size];
    fill_plain(data, nr, nc, ExFaces);

    // offload to mic, copy data and create the C++ matrix in the offloaded Full_Cone
    #pragma offload target(mic:mic_nr) in(nr,nc) in(data: length(size) ONCE)
    {
      offload_fc_ptr->ExcludedFaces = Matrix<Integer>(nr, nc);
      fill_matrix(offload_fc_ptr->ExcludedFaces, nr, nc, data);
    }
    delete[] data;
  }
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::transfer_grading()
{
//  cout << "mic " << mic_nr<< ": transfer_grading" << endl;
  long dim = local_fc_ref.dim;
  if (local_fc_ref.inhomogeneous)
  {
    assert(local_fc_ref.isComputed(ConeProperty::RecessionRank));
    Integer *data = new Integer[dim];
    fill_plain(data, dim, local_fc_ref.Truncation);
    long level0_dim = local_fc_ref.level0_dim;

    #pragma offload target(mic:mic_nr) in(dim) in(level0_dim) in(data: length(dim) ONCE)
    {
      offload_fc_ptr->Truncation = vector<Integer>(dim);
      fill_vector(offload_fc_ptr->Truncation, dim, data);
      offload_fc_ptr->level0_dim = level0_dim;
    }
    delete[] data;
  }

  if (local_fc_ref.isComputed(ConeProperty::Grading))
  {
    Integer *data = new Integer[dim];
    fill_plain(data, dim, local_fc_ref.Grading);

    #pragma offload target(mic:mic_nr) in(dim) in(data: length(dim) ONCE)
    {
      offload_fc_ptr->Grading = vector<Integer>(dim);
      fill_vector(offload_fc_ptr->Grading, dim, data);
      offload_fc_ptr->is_Computed.set(ConeProperty::Grading);
    }
    delete[] data;
  }

  if (local_fc_ref.shift != 0)
  {
    auto shift = local_fc_ref.shift;
    #pragma offload target(mic:mic_nr) in(shift)
    {
      offload_fc_ptr->shift = shift;
    }
  }

//  cout << "mic " << mic_nr<< ": transfer_grading done" << endl;
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::transfer_triangulation_info()
{
//  cout << "mic " << mic_nr<< ": transfer_triangulation_info" << endl;
  long dim = local_fc_ref.dim;
  long nr_gen = local_fc_ref.nr_gen;

  if (local_fc_ref.isComputed(ConeProperty::ExtremeRays))
  {
    bool *data = new bool[nr_gen];
    fill_plain(data, nr_gen, local_fc_ref.Extreme_Rays_Ind);

    #pragma offload target(mic:mic_nr) in(nr_gen) in(data: length(nr_gen) ONCE)
    {

      offload_fc_ptr->Extreme_Rays_Ind = vector<bool>(nr_gen);
      fill_vector(offload_fc_ptr->Extreme_Rays_Ind, nr_gen, data);
      offload_fc_ptr->is_Computed.set(ConeProperty::ExtremeRays);
    }
    delete[] data;
  }

  // always transfer the order vector
  {
    Integer *data = new Integer[dim];
    fill_plain(data, dim, local_fc_ref.Order_Vector);

    #pragma offload target(mic:mic_nr) in(dim) in(data: length(dim) ONCE)
    {
      offload_fc_ptr->Order_Vector = vector<Integer>(dim);
      fill_vector(offload_fc_ptr->Order_Vector, dim, data);
    }
    delete[] data;
  }

  if (!local_fc_ref.Comparisons.empty())
  {
    long size = local_fc_ref.Comparisons.size();

    size_t *data = new size_t[size];
    fill_plain(data, size, local_fc_ref.Comparisons);

    #pragma offload target(mic:mic_nr) in(size) in(data: length(size) ONCE)
    {
      offload_fc_ptr->Comparisons.resize(size);
      fill_vector(offload_fc_ptr->Comparisons, size, data);
      offload_fc_ptr->nrTotalComparisons = offload_fc_ptr->Comparisons[size-1];
    }
    delete[] data;
  }
//  cout << "mic " << mic_nr<< ": transfer_triangulation_info done" << endl;
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::primal_algorithm_initialize()
{
  #pragma offload target(mic:mic_nr) signal(&running)
  {
    offload_fc_ptr->do_vars_check(false);
    if (offload_fc_ptr->inhomogeneous)
      offload_fc_ptr->set_levels();
    offload_fc_ptr->set_degrees();
    offload_fc_ptr->primal_algorithm_initialize();

    cout << "mic " << mic_nr<< ": create 3 mio empty simplices ..." << endl;
    SHORTSIMPLEX<Integer> simp;
    simp.key = vector<key_t>(offload_fc_ptr->dim);
    simp.height = 0;
    simp.vol = 0;
    offload_fc_ptr->FreeSimpl.insert(offload_fc_ptr->FreeSimpl.end(), 3000000, simp);
    cout << "mic " << mic_nr<< ": creating simplices done." << endl;
  }
  running = true;
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::transfer_pyramids(const list< vector<key_t> >& pyramids)
{
  long size = plain_size(pyramids);

  key_t *data = new key_t[size];
  fill_plain(data, size, pyramids);

  transfer_pyramids_inner(data, size);

  delete[] data;
  cout << "mic " << mic_nr << ": transfered " << pyramids.size() << " pyramids. avg. key size:" << static_cast<double>(size)/pyramids.size()-1 << endl;
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::transfer_pyramids_inner(key_t *data, long size)
{
  wait();
  #pragma offload target(mic:mic_nr) in(size) in(data: length(size) ONCE)
  {
    fill_list_vector(offload_fc_ptr->Pyramids[0], size, data);
    offload_fc_ptr->nrPyramids[0] = offload_fc_ptr->Pyramids[0].size();
  }
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::evaluate_pyramids()
{
  wait();
  cout << "mic " << mic_nr<< ": evaluate_pyramids" << endl;
  #pragma offload target(mic:mic_nr) signal(&running)
  {
    offload_fc_ptr->evaluate_stored_pyramids(0);
    offload_fc_ptr->evaluate_triangulation();
  }
    // cout << "Nach Start evaluate mic" << mic_nr << endl;
  running = true;
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::evaluate_triangulation()
{
  wait();
  cout << "mic " << mic_nr<< ": evaluate_triangulation" << endl;
  #pragma offload target(mic:mic_nr) signal(&running)
  {
    offload_fc_ptr->evaluate_triangulation();
  }
  running = true;
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::complete_evaluation()
{
  wait();
  cout << "mic " << mic_nr<< ": complete_evaluation" << endl;
  #pragma offload target(mic:mic_nr) signal(&running)
  {
    offload_fc_ptr->primal_algorithm_finalize();
  }
  running = true;
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::collect_data()
{
  wait();
  cout << "mic " << mic_nr<< ": collect_data" << endl;
  collect_integers(); // TriangulationSize, DetSum, Multiplicity, ...
  collect_hilbert_series();
  collect_candidates(); // Hilbert basis, degree 1 elements
  cout << "mic " << mic_nr<< ": collect_data done" << endl;
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::collect_integers()
{
  {
    size_t col_totalNrSimplices, col_nrSimplicialPyr, col_totalNrPyr;
    Integer col_detSum;
    #pragma offload target(mic:mic_nr) out(col_totalNrSimplices) out(col_nrSimplicialPyr) out(col_totalNrPyr) out(col_detSum)
    {
      col_totalNrSimplices = offload_fc_ptr->totalNrSimplices;
      col_nrSimplicialPyr  = offload_fc_ptr->nrSimplicialPyr;
      col_totalNrPyr       = offload_fc_ptr->totalNrPyr;
      col_detSum           = offload_fc_ptr->detSum;
    }
    local_fc_ref.totalNrSimplices += col_totalNrSimplices;
    local_fc_ref.nrSimplicialPyr  += col_nrSimplicialPyr;
    local_fc_ref.totalNrPyr       += col_totalNrPyr;
    local_fc_ref.detSum           += col_detSum;
  }

  if (local_fc_ref.do_triangulation && local_fc_ref.do_evaluation
      && local_fc_ref.isComputed(ConeProperty::Grading))
  {
//    cout << "mic " << mic_nr<< ": collecting multiplicity ..." << endl;
    long size;
    std::string* str_ptr;
    #pragma offload target(mic:mic_nr) out(size) nocopy(str_ptr: length(0))
    {
      str_ptr = new std::string(offload_fc_ptr->multiplicity.get_str());
      size = str_ptr->length()+1;
    }

    char* c_str = new char[size];
    #pragma offload target(mic:mic_nr) in(size) out(c_str: length(size)) nocopy(str_ptr: length(0))
    {
      std::strcpy (c_str, str_ptr->c_str());
      delete str_ptr;
    }
    mpq_class coll_mult(c_str);
    delete c_str;
    local_fc_ref.multiplicity += coll_mult;
//    cout << "mic " << mic_nr<< ": collecting multiplicity done" << endl;
  }
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::collect_hilbert_series()
{

  if (local_fc_ref.do_h_vector)
  {
//    cout << "mic " << mic_nr<< ": collecting Hilbert series ..." << endl;
    long size;
    std::string* str_ptr;
    #pragma offload target(mic:mic_nr) out(size) nocopy(str_ptr: length(0))
    {
      str_ptr = new std::string(offload_fc_ptr->Hilbert_Series.to_string_rep());
      size = str_ptr->length()+1;
    }

    char* c_str = new char[size];
    #pragma offload target(mic:mic_nr) in(size) out(c_str: length(size)) nocopy(str_ptr: length(0))
    {
      std::strcpy (c_str, str_ptr->c_str());
      delete str_ptr;
    }
    HilbertSeries col_HS = HilbertSeries(string(c_str));
    delete c_str;
    local_fc_ref.Hilbert_Series += col_HS;
//    cout << "mic " << mic_nr<< ": collecting Hilbert series done" << endl;
  }
}

//---------------------------------------------------------------------------

#pragma offload_attribute (push, target(mic))
template<typename Integer>
bool is_ori_gen(const Candidate<Integer>& c)
{
  return c.original_generator;
}
#pragma offload_attribute (push, target(mic))

//---------------------------------------------------------------------------


template<typename Integer>
void OffloadHandler<Integer>::collect_candidates()
{
  if (local_fc_ref.do_Hilbert_basis)
  {
//    cout << "mic " << mic_nr<< ": collect Hilbert basis" << endl;
    long size;

    #pragma offload target(mic:mic_nr) out(size)
    {
      // remove all original generators, they are also inserted on the host
      offload_fc_ptr->OldCandidates.Candidates.remove_if(is_ori_gen<Integer>);
      // or in c++11 with lambda function
      // offload_fc_ptr->OldCandidates.Candidates.remove_if([](const Candidate<Integer>& c){ return c.original_generator; });
      offload_fc_ptr->OldCandidates.extract(offload_fc_ptr->Hilbert_Basis);
      offload_fc_ptr->OldCandidates.Candidates.clear();

      // using the same methods as for pyramids
      // handling list of vectors of possible different lenghts
      size = offload_fc_ptr->Hilbert_Basis.size() * (offload_fc_ptr->dim + 1);
    }
    if (size > 0) {
      Integer *data = new Integer[size];

      #pragma offload target(mic:mic_nr) in(size) out(data: length(size) ONCE)
      {
        fill_plain(data, size, offload_fc_ptr->Hilbert_Basis);
      }
//      fill_list_vector(local_fc_ref.Hilbert_Basis, size, data);
      list< vector<Integer> > coll_HB;
      fill_list_vector(coll_HB, size, data);
      delete[] data;
      CandidateList<Integer> cand_l;
      while (!coll_HB.empty())
      {
        cand_l.push_back(Candidate<Integer>(coll_HB.front(),local_fc_ref));
        coll_HB.pop_front();
      }
//      cout << "mic " << mic_nr<< ": CandidateList complete" << endl;
//      #pragma omp critical(CANDIDATES)
      local_fc_ref.NewCandidates.splice(cand_l);

      local_fc_ref.NewCandidates.reduce_by(local_fc_ref.OldCandidates);
      local_fc_ref.update_reducers();

    } // if (size > 0)
//    cout << "mic " << mic_nr<< ": collect Hilbert basis done" << endl;
  }

  if (local_fc_ref.do_deg1_elements)
  {
//    cout << "mic " << mic_nr<< ": collect degree 1 elements" << endl;
    long size;

    #pragma offload target(mic:mic_nr) out(size)
    {
      // using the same methods as for pyramids
      // handling list of vectors of possible different lenghts
      size = offload_fc_ptr->Deg1_Elements.size() * (offload_fc_ptr->dim + 1);
    }
    if (size > 0) {
      Integer *data = new Integer[size];

      #pragma offload target(mic:mic_nr) in(size) out(data: length(size) ONCE)
      {
        fill_plain(data, size, offload_fc_ptr->Deg1_Elements);
      }
      list< vector<Integer> > coll_Deg1;
      fill_list_vector(coll_Deg1, size, data);
      delete[] data;
      local_fc_ref.Deg1_Elements.splice(local_fc_ref.Deg1_Elements.end(),coll_Deg1);
    }
//    cout << "mic " << mic_nr<< ": collect degree 1 elements done" << endl;
  }
}

//---------------------------------------------------------------------------

template<typename Integer>
bool OffloadHandler<Integer>::is_running()
{
#ifndef __MIC__
  if (running)
  {
    running = ! _Offload_signaled(mic_nr, &running);
  }
#endif
  return running;
}

//---------------------------------------------------------------------------

template<typename Integer>
void OffloadHandler<Integer>::wait()
{
  if (is_running())
  {
    cout << "mic " << mic_nr<< ": waiting ..." << endl;
    #pragma offload_wait target(mic:mic_nr) wait(&running)
    running = false;
    cout << "mic " << mic_nr<< ": waiting completed." << endl;
  }
}

//---------------------------------------------------------------------------

template<typename Integer>
OffloadHandler<Integer>::~OffloadHandler()
{
  cout << "mic " << mic_nr<< ": OffloadHandler destructor" << endl;
  #pragma offload target(mic:mic_nr)
  {
    delete offload_fc_ptr;
  }
}

//-------------------------- MicOffloader -----------------------------------

template<typename Integer>
MicOffloader<Integer>::MicOffloader()
: is_init(false),
  nr_mics(_Offload_number_of_devices()),
  nr_handlers(nr_mics)
{
  handlers.resize(nr_handlers);
  // cout << "Constructor " << nr_handlers << endl;
}

//---------------------------------------------------------------------------

template<typename Integer>
MicOffloader<Integer>::~MicOffloader()
{
  if (is_init)
    for (int i=0; i<nr_handlers; ++i)
      delete handlers[i];
}

//---------------------------------------------------------------------------

template<typename Integer>
void MicOffloader<Integer>::init(Full_Cone<Integer>& fc)
{
  if (!is_init)
  {
    //TODO check preconditions
    assert(fc.Order_Vector.size() == fc.dim);
    if (fc.do_Hilbert_basis) {
      fc.get_supphyps_from_copy(false);          // (bool from_scratch)
      fc.check_pointed();
    }

    // create handler
    for (int i=0; i<nr_handlers; ++i)
      handlers[i] = new OffloadHandler<Integer>(fc, i % nr_mics);
    is_init = true;
    // wait for completed creation // TODO keep it?
    for (int i=0; i<nr_handlers; ++i)
      handlers[i]->wait();
  }
}

//---------------------------------------------------------------------------

// template<typename T>
bool compare_sizes(const vector<key_t>& v, const vector<key_t>& w){
    return v.size() < w.size();
}

template<typename Integer>
void MicOffloader<Integer>::offload_pyramids(Full_Cone<Integer>& fc, const size_t level)
{
    if (!is_init) init(fc);
// cout << "nr_handlers " << nr_handlers << endl;

    size_t fraction = 6;
    if (fc.start_from == fc.nr_gen) { //all gens are done
        fraction = 20;
        if (level > 0) fraction = 30;
    }
    if (fraction < nr_handlers) fraction = nr_handlers; //ensure every card can get some


    size_t nr_transfer = min(fc.nrPyramids[level]/fraction, 25000ul);
    // cout << "transfer " << nr_transfer << endl;
    if (nr_transfer == 0) return;

    bool at_least_one_idle=false;
    for (int i=0; i<nr_handlers; ++i){
      if (!handlers[i]->is_running()){
        at_least_one_idle=true;
        /* cout << "Mixing ======================" << endl;
        random_order(fc.Pyramids[level]);
        auto py=fc.Pyramids[level].begin();
        for(;py!=fc.Pyramids[level].end();++py)
            cout << py->size() << " ";
        cout << "======================" << endl;*/
        break;
      }
    }
    if(!at_least_one_idle)
        return;
    
    if(fc.Pyramids_scrambled[level]){
        fc.Pyramids[level].reverse(); // bring the big pyramids to the rear
    } else{
        fc.Pyramids_scrambled[level]=true; // will not be scrambeld again
        fc.Pyramids[level].sort(compare_sizes);
        
        typename list< vector<key_t> >::iterator p;
        size_t size_sum=0;
        for(p=fc.Pyramids[level].begin(); p!=fc.Pyramids[level].end();++p)
            size_sum+=p->size();
        size_t size_bound=2*size_sum/fc.nrPyramids[level]; // 2*average size
        
        for(p=fc.Pyramids[level].begin(); p!=fc.Pyramids[level].end();++p){
                if(p->size() > size_bound)
                    break;
        }
        
        random_order(fc.Pyramids[level],fc.Pyramids[level].begin(),p);
    }
    
    // offload some pyramids
    list< vector<key_t> > pyrs;
    vector<bool> started(nr_handlers, false);

    for (int i=0; i<nr_handlers; ++i)
    {
      if (!handlers[i]->is_running())
      {
// cout << "Testing mic" << i << endl;
        started[i] = true;
        typename list< vector<key_t> >::iterator transfer_end(fc.Pyramids[level].begin());
        for (size_t j = 0; j < nr_transfer; ++j, ++transfer_end) ;
        pyrs.splice(pyrs.end(), fc.Pyramids[level], fc.Pyramids[level].begin(), transfer_end);
        fc.nrPyramids[level] -= nr_transfer;
        handlers[i]->transfer_pyramids(pyrs);
        pyrs.clear();
      }
    }
    
    fc.Pyramids[level].reverse(); // bring the big pyramids to the front

    //compute on mics
    for (int i=0; i<nr_handlers; ++i)
      if (started[i])
        handlers[i]->evaluate_pyramids();
}


//---------------------------------------------------------------------------

template<typename Integer>
void MicOffloader<Integer>::evaluate_triangulation()
{
  if (is_init)
  {
    for (int i=0; i<nr_handlers; ++i)
      if (!handlers[i]->is_running())
        handlers[i]->evaluate_triangulation();
  }
}

//---------------------------------------------------------------------------

template<typename Integer>
void MicOffloader<Integer>::finalize()
{
  if (is_init)
  {
    list<int> to_complete, to_collect;
    list<int>::iterator it;

    for (int i=0; i<nr_handlers; ++i)
      to_complete.push_back(i);
    // first start it on all idle mics
    while (!to_complete.empty() || !to_collect.empty())
    {
      for (it = to_complete.begin(); it != to_complete.end(); )
      {
        if (!handlers[*it]->is_running())
        {
          handlers[*it]->complete_evaluation();
          to_collect.push_back(*it);
          it = to_complete.erase(it);
        }
        else
        {
          ++it;
        }
      }

      for (it = to_collect.begin(); it != to_collect.end(); )
      {
        if (!handlers[*it]->is_running())
        {
          handlers[*it]->collect_data();
          delete handlers[*it];
          handlers[*it] = NULL;
          it = to_collect.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
    is_init = false;
  }
}

/***************** Instantiation for template parameter long long *****************/

template class MicOffloader<long long int>;
template class OffloadHandler<long long int>;

} // end namespace libnormaliz




#endif //NMZ_MIC_OFFLOAD
