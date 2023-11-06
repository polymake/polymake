
/* Copyright (c) 1997-2023
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------

  This file is part of the polymake database interface polyDB.

   @author Andreas Paffenholz
   (c) 2015-2023
   https://polydb.org
   https://www.mathematik.tu-darmstadt.de/~paffenholz
*/

#pragma once

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <regex>

namespace polymake {
namespace common {
namespace polydb {

    /**
     * @brief create more user friendly error messages based on the mongoc error codes
     * 
     * @param[in] error the mongoc error structure
     * @param[in] name the name of the client/collection where the error occured
     * @param[in] write true if error occured because of missing admin rights
     * @param[in] caller_name the calling function, auto filled
     * @return std::string 
     */
    std::string prepare_error_message(bson_error_t& error, const std::string& name,  bool write = false, const char * caller_name = __builtin_FUNCTION());

    /**
     * @brief generate a unique id for the connection
     * this is used in the logs on the server
     * @param length the length of the id
     * @return std::string 
     */
    std::string generate_client_id(std::size_t length);

    inline std::string to_string_and_free(char * ptr) {
      std::string str = ptr;
      bson_free(ptr);
      return str;
    }
}}}
