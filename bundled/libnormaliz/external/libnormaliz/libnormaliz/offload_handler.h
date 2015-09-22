#ifndef OFFLOAD_HANDLER_H
#define OFFLOAD_HANDLER_H

#ifdef NMZ_MIC_OFFLOAD
#define ONCE alloc_if(1) free_if(1)
#define ALLOC alloc_if(1) free_if(0)
#define FREE alloc_if(0) free_if(1)
#define REUSE alloc_if(0) free_if(0)

#include "libnormaliz/libnormaliz.h"
#include <list>
#include <vector>
#include "libnormaliz/offload.h"  // offload system header

namespace libnormaliz {

// forward-declarations
template<typename Integer> class Full_Cone;
template<typename Integer> class Matrix;


template<typename Integer>
class OffloadHandler
{
public:
  // transfers Full_Cone to mic:mic_nr and keeps handle
  OffloadHandler(Full_Cone<Integer>&, int mic_number = 0);

  // destructor deletes Full_Cone on mic
  ~OffloadHandler();

  void transfer_pyramids(const std::list< std::vector<key_t> >& pyramids);
  void evaluate_pyramids();
  void evaluate_triangulation();
  void complete_evaluation(); // call only once!
  void collect_data();        // call only once!

  // checks if a previous asynchronous offload is still running
  bool is_running();
  // waits until previous asynchronous offload has finished
  void wait();

private:
  const int mic_nr;
  bool running; // used to see if we still have a wait lock open
  Full_Cone<Integer>& local_fc_ref;
  Full_Cone<Integer>* offload_fc_ptr;

  // init routines
  void create_full_cone();
  void transfer_bools();
  void transfer_support_hyperplanes();
  void transfer_grading();
  void transfer_triangulation_info();
  void primal_algorithm_initialize();

  // collect data routines
  void collect_integers(); // TriangulationSize, DetSum, Multiplicity
  void collect_hilbert_series();
  void collect_candidates(); // Hilbert basis, degree 1 elements
};

template<typename Integer>
class MicOffloader
{
public:
  MicOffloader();
  ~MicOffloader();

  // on the first call it inits the offloaded cones
  void offload_pyramids(Full_Cone<Integer>& fc, const size_t level);

  // evaluates remaining triangulation
  void evaluate_triangulation();

  // collects the results and destructs the offloaded cones
  void finalize();

private:
  bool is_init;
  int nr_mics;
  int nr_handlers;
  std::vector< OffloadHandler<Integer>* > handlers;

  void init(Full_Cone<Integer>& fc);
};

} // end namespace libnormaliz

#endif //NMZ_MIC_OFFLOAD

#endif //OFFLOAD_HANDLER_H
