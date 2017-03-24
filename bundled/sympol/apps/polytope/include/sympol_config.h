#ifndef POLYMAKE_SYMPOL_CONFIG_H
#define POLYMAKE_SYMPOL_CONFIG_H
#include "sympol/config.h"

// we want to use plain sympol without any dependencies

#undef HAVE_EIGEN
#undef HAVE_NTL
#undef HAVE_BLISS
#undef HAVE_NAUTY

#endif
