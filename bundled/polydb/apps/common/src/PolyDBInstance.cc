
#include "polymake/common/PolyDB.h"

#include "polymake/common/PolyDBInstance.h"

namespace polymake {
namespace common {
namespace polydb {

bool polydb_connection_started = false;

void polydb_init() {

    if ( polydb_connection_started ) {
        return;
    }

    mongoc_init();
    std::atexit(mongoc_cleanup);
    polydb_connection_started = true;
}


}}}
