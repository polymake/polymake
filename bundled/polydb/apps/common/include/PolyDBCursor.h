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

#include <iterator>

#include "polymake/common/PolyDB.h"
#include "polymake/common/PolyDBFunctions.h"

#include "polymake/Array.h"

namespace polymake {
namespace common {
namespace polydb {

/**
 * @brief a class containing a cursor into a collection of polyDB
 * 
 */
class PolyDBCursor {

    public:

    PolyDBCursor( std::shared_ptr<mongoc_cursor_t> c ) : cursor_(c) { 
        result_stored_ = false; 
    }

    PolyDBCursor ( PolyDBCursor &&c ) : cursor_(c.cursor_) {
        result_ = c.result_;
        result_stored_ = c.result_stored_;
    }

    // FIXME why do we need a copy constructor?
    PolyDBCursor ( const PolyDBCursor &c ) : cursor_(c.cursor_) {
        result_ = c.result_;
        result_stored_ = c.result_stored_;
    }

    /**
     * @brief checks if we can read another document from the cursor
     * 
     * @return true if we can read another document from the cursor
     */
    const bool has_next() const {
        if ( result_stored_ ) {
            return true;
        }
        const bson_t * result;
        bool has_next = mongoc_cursor_next(cursor_.get(), &result);
        if ( !has_next ) {
            return false;
        }
        result_ = to_string_and_free(bson_as_relaxed_extended_json ( result, nullptr ));
        result_stored_ = true;
        return true;
    }

    /**
     * @brief obtains the next document from the cursor
     * this may already be cached
     * 
     * @return const std::string the next object as json string
     */
    const std::string next() const {
        if ( !has_next() ) {
            throw std::runtime_error("No more documents in query");
        }
        result_stored_ = false;
        return result_;
    }

    /**
     * @brief read all documents from the cursor and return them as array
     * 
     * @return const Array<std::string> the list of objects as json string
     */
    const Array<std::string> all() const {
        std::vector<std::string> v;

        const bson_t * doc;
        while ( mongoc_cursor_next (cursor_.get(), &doc) ) {
          std::string res = to_string_and_free(bson_as_relaxed_extended_json (doc, nullptr));
          v.push_back(res);
        }
        
        return Array<std::string>(v.size(),entire(v));
    }

    private:

    // the cached next result from the  
    mutable std::string result_;

    // whether we have stored a result in the cache
    mutable bool result_stored_;

    // the actual mongoc cursor
    std::shared_ptr<mongoc_cursor_t> cursor_;

};

}}}

