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

#include "polymake/common/PolyDB.h"

#include "polymake/common/PolyDBconfig.h"
#include "polymake/common/PolyDBFunctions.h"


namespace polymake {
namespace common {
namespace polydb {

/**
 * @brief a class defining a section in polyDB
 * 
 */
class PolyDBSection {

    public:
    
    ~PolyDBSection() {
      mongoc_collection_destroy(section_);
    }

    PolyDBSection(const std::string& name,
                  const std::shared_ptr<mongoc_client_t>& client
                 ) : client_(client), name_(name) { 
                    mongoc_database_t *database;
                    database = mongoc_client_get_database(client_.get(), "polydb");
                    section_ = mongoc_database_get_collection (database, ("_sectionInfo."+name).c_str());
                    mongoc_database_destroy(database);
                 };

    PolyDBSection(PolyDBSection &&pc ) {
      std::swap(client_,pc.client_);
      std::swap(section_,pc.section_);
      name_ = pc.name_;
    }

    /**
     * @brief drop the section
     * this removes the documentation
     */
    const void drop() const {

      bson_error_t error;
      bool res = mongoc_collection_drop(section_,&error);
      if ( !res ) {
        std::string message = "drop section failed with error ";
        message += error.message;
        throw std::runtime_error(message);        
      }
    }

    /**
     * @brief Get the documentation
     * 
     * @param id the id for the section documentation
     * @return const std::string 
     */
    const std::string get_info(const std::string& id) const {
      bson_t *query = bson_new();
      bson_error_t error;

      bson_append_utf8(query, "_id", -1, id.c_str(), -1);
      mongoc_cursor_t * cursor = mongoc_collection_find_with_opts (section_, query, nullptr, nullptr);
      bson_destroy (query);
      if (mongoc_cursor_error (cursor, &error)) {
        std::string message = "check for section id failed with error ";
        message += error.message;
        message += "and error code ";
        message += std::to_string(error.domain);
        message += std::to_string(MONGOC_ERROR_SERVER);
        mongoc_cursor_destroy (cursor);
        throw std::runtime_error(message);
      }
      
      const bson_t *doc;
      if ( !mongoc_cursor_next (cursor, &doc)) {
        mongoc_cursor_destroy (cursor);
        throw std::runtime_error("no info with given id");
      }

      std::string info = to_string_and_free(bson_as_relaxed_extended_json(doc,nullptr));
      
      mongoc_cursor_destroy (cursor);

      return info;
    }

    /**
     * @brief checks if documentation with the given id exists
     * 
     * @param id the id
     * @return true if the doc exists
     */
    const bool check_for_id(const std::string& id) const {
      
      bson_t *query = bson_new();
      bson_error_t error;

      bson_append_utf8(query, "_id", -1, id.c_str(), -1);
      mongoc_cursor_t * cursor = mongoc_collection_find_with_opts (section_, query, nullptr, nullptr);
      bson_destroy (query);
      if (mongoc_cursor_error (cursor, &error)) {
        std::string message = "check for section id failed with error ";
        message += error.message;
        message += "and error code ";
        message += std::to_string(error.domain);
        message += std::to_string(MONGOC_ERROR_SERVER);
        mongoc_cursor_destroy (cursor);
        throw std::runtime_error(message);
      }
      
      const bson_t *doc;
      bool result_found = false;
      if ( mongoc_cursor_next (cursor, &doc)) {
        result_found = true;
      }
      mongoc_cursor_destroy (cursor);

      return result_found;
    }

    
    /**
     * @brief insert into the collection for this section
     * 
     * @param docstring 
     */
    // FIXME check result
    const void insert(const std::string& docstring) const {

      bson_error_t error;
      bson_t * doc = bson_new_from_json((unsigned char *)docstring.c_str(),-1,&error); 
      bson_t reply;

      //char * str = bson_as_relaxed_extended_json (doc, nullptr);
      //std::cout << "doc: " << str << std::endl;

      bool ret = mongoc_collection_insert_one ( section_, doc, nullptr, &reply, &error ) ;

      bson_destroy(doc);
      if ( !ret ) {
        std::string message = "inserting section info failed with error ";
        message += error.message;
        message += "and error code ";
        message += std::to_string(error.domain);
        message += std::to_string(MONGOC_ERROR_SERVER);
        throw std::runtime_error(message);
      }

      bson_destroy(&reply);
    }

    /**
     * @brief update a document in the collection for this section
     * 
     * @param id the id to update
     * @param docstring the update
     */
    // FIXME check result
    const void update(const std::string& id, const std::string& docstring) const {

      bson_error_t error;
      bson_t * doc = bson_new_from_json((unsigned char *)docstring.c_str(),-1,&error); 
      bson_t * filter = bson_new();
      bson_append_utf8(filter, "_id", -1, id.c_str(), -1);
      bson_t reply;

      bool ret = mongoc_collection_update_one (section_, filter, doc, nullptr, &reply, &error);
      bson_destroy(doc);
      bson_destroy(filter);
      if ( !ret ) {
        std::string message = "updating section info failed with error ";
        message += error.message;
        message += "and error code ";
        message += std::to_string(error.domain);
        message += std::to_string(MONGOC_ERROR_SERVER);
        throw std::runtime_error(message);
      }


      // FIXME do something with the reply first
      bson_destroy(&reply);
    }


    /**
     * @brief replace a document in the collection for this section
     * 
     * @param id the id to replace
     * @param docstring the replacement
     */
    // FIXME check result
    const void replace(const std::string& id, const std::string& docstring) const {

      bson_error_t error;
      bson_t * doc = bson_new_from_json((unsigned char *)docstring.c_str(),-1,&error); 
      bson_t * filter = bson_new();
      bson_append_utf8(filter, "_id", -1, id.c_str(), -1);
      bson_t reply;

      bool ret = mongoc_collection_replace_one (section_, filter, doc, nullptr, &reply, &error);
      bson_destroy(doc);
      bson_destroy(filter);
      bson_destroy(&reply);
      if ( !ret ) {
        std::string message = "replacing section info failed with error ";
        message += error.message;
        message += "and error code ";
        message += std::to_string(error.domain);
        message += std::to_string(MONGOC_ERROR_SERVER);
        throw std::runtime_error(message);
      }

    }

    /**
     * @brief Set the documentation of this section
     * 
     * @param doc the documentation
     * @param update_existing true if an existing documenation should be updated
     * @return true on success
     */
    const bool set_doc(const std::string& doc, bool update_existing ) const {

      bson_error_t error;
      bson_t * doc_bson = bson_new_from_json((unsigned char *)doc.c_str(), -1,&error);
      if ( !doc_bson ) {
        throw std::runtime_error(prepare_error_message(error,"bson_creation"));
      }
      bson_iter_t iter;
      bson_iter_init (&iter, doc_bson);
      std::string version = polyDBVersion;

      if ( bson_iter_find(&iter,"polydb_version") ) {
        version = bson_iter_utf8(&iter, nullptr);
      }

      std::string id_string = name_ +"."+version;
      bson_iter_init (&iter, doc_bson);
      if (bson_iter_find(&iter,"_id") ) {
        id_string = bson_iter_utf8(&iter, nullptr);
      } else {
        bson_append_utf8(doc_bson, "_id", -1, id_string.c_str(), -1);
      }

      int sectionDepth = 0;
      bson_iter_init (&iter, doc_bson);
      if ( !bson_iter_find(&iter,"section") ) {
        std::stringstream name(name_);
        std::string section;
        std::vector<std::string> sections;
        while(std::getline(name, section, '.')) {
          sectionDepth++;
          sections.push_back(section);
        }
        bson_t section_list;
        bson_append_array_begin(doc_bson, "section", -1, &section_list);
        for (size_t i = 0; i < sections.size(); i++) {
          bson_append_utf8(&section_list, std::to_string(i).c_str(), -1, sections[i].c_str(), -1);
        }
        bson_append_array_end(doc_bson, &section_list);
      }

      bson_iter_init (&iter, doc_bson);
      if ( !bson_iter_find(&iter,"sectionDepth") ) {
        if ( sectionDepth == 0 ) {
          bson_iter_t iter2;
          bson_iter_init (&iter2, doc_bson);
          bson_iter_find(&iter2,"section");
          bson_iter_t child;
          bson_iter_recurse (&iter2, &child);
          while (bson_iter_next (&child)) {
            sectionDepth++;
          }
        }
        bson_append_int32(doc_bson, "sectionDepth", -1, sectionDepth);
      } 

      bool doc_exits = check_for_id(id_string);
      if ( update_existing && !doc_exits ) { 
        bson_destroy(doc_bson);
        throw std::runtime_error("updating non-existent section documentation");
      }

      if ( update_existing ) {
        bson_t * update_doc = bson_new();
        bson_append_document(update_doc,"$set",-1,doc_bson);
        std::string update_doc_string = to_string_and_free(bson_as_canonical_extended_json(update_doc,nullptr));
        bson_destroy(update_doc);
        update(id_string,update_doc_string);
      } else {
        std::string insert_replace_doc_string = to_string_and_free(bson_as_canonical_extended_json(doc_bson,nullptr));
        if ( doc_exits ) {
          replace(id_string,insert_replace_doc_string);
        } else {
          insert(insert_replace_doc_string);
        }
      }
      bson_destroy(doc_bson);
      return true;
    }

    private:
        mongoc_collection_t * section_;
        std::shared_ptr<mongoc_client_t> client_;
        std::string name_;

};

}}}
