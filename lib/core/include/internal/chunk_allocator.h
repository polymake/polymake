#ifndef POLYMAKE_INTERNAL_CHUNK_ALLOCATOR_H
#define POLYMAKE_INTERNAL_CHUNK_ALLOCATOR_H

#include <cstdlib>

namespace pm {

//! Maintains a list of private memory chunks of fixed size.
class chunk_allocator {
public:
   static const size_t default_chunk_size=4096;

   explicit chunk_allocator(size_t obj_size_arg, size_t n_objects_in_chunk_arg = 0);

   void* allocate();
   void reclaim(void* p);
   // give all chunks back to the system
   void clear();

   ~chunk_allocator() { release(); }

   size_t get_object_size() const { return obj_size; }

protected:
   void release();

   size_t obj_size;
   size_t n_objects_in_chunk;
   char* free_obj;
   char* last_obj;
   char* chunk_end;

private:
   // deleted
   chunk_allocator(const chunk_allocator&);
   void operator= (const chunk_allocator&);
};

}

#endif // POLYMAKE_INTERNAL_CHUNK_ALLOCATOR_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
