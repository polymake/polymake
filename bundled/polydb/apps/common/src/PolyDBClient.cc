
#include "polymake/common/PolyDBClient.h"


namespace polymake {
namespace common {
namespace polydb {

  void client_destroy(mongoc_client_t * client) {
    mongoc_client_destroy(client);
  }

}}}
