
#ifndef POLYMAKE_TOPAZ_LAWLER_H
#define POLYMAKE_TOPAZ_LAWLER_H

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"

namespace polymake { namespace topaz {

Array<Set<int>> lawler(const Array<Set<int> > F, const int n_vertices );

}}
#endif // POLYMAKE_TOPAZ_LAWLER_H

