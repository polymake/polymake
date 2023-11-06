

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <regex>
#include <random>

#include "polymake/common/PolyDB.h"

#include "polymake/common/PolyDBInstance.h"

namespace polymake {
namespace common {
namespace polydb {

std::string prepare_error_message(bson_error_t& error, const std::string& name,  bool write = false, const char * caller_name = __builtin_FUNCTION()) {
    std::string message = "Error in ";
    message += caller_name;
    message += ": ";
    if ( name == "database") {
      message += "connection to the database failed with error: ";
      message += error.message;     
    } else if ( name == "database_command") {
      message += "running a command on the database failed with error: ";
      message += error.message;     
    } else if ( name == "collection_names") {
      message += "fetching collection names form the database failed with error: ";
      message += error.message;     
    } else if ( name == "roles") {
      message += "iterating the roles array failed with error: ";
      message += error.message;     
    } else if ( name == "bson_creation" ) {
      message += "converting json to bson failed with error: ";
      message += error.message;     
    } else     if ( name == "client connection" ) {
      message += "connection to server failed";        
    } else if ( std::regex_match(error.message, std::regex(".*not authorized on polydb.*")) ) {
      message += "Missing access permission for ";
      message += write ? "writing to " : "reading from ";
      message += "collection ";
      message += name;
    } else if ( error.code == MONGOC_ERROR_CLIENT_AUTHENTICATE ) {
      if ( name == "ping" ) {
        message += "Authentication failed for initial server ping. Missing access to polydb. ";
      } else {
        message += "Authentication failed for collection ";
        message += name;
      }
    } else if ( error.code == MONGOC_ERROR_SERVER_SELECTION_FAILURE ) {
      message += "(MONGOC_ERROR_SERVER_SELECTION_FAILURE) server selection failure: server not available? ";

    } else if ( error.code == MONGOC_ERROR_DUPLICATE_KEY ) {
      message += "Duplicate key: Object with this id has already been inserted into collection ";
      message += name;
    } else {
      message += error.message;
      message += " - ";
      message += std::to_string(error.code);
    }
    return message;
  }

  std::string generate_client_id(std::size_t length) {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(0, (int)chars.size() - 1);

    char random_string[length];
    for (std::size_t i = 0; i < length; ++i) {
      random_string[i] = chars[distribution(generator)];
    }

    return std::string(random_string, length);
}

}}}
