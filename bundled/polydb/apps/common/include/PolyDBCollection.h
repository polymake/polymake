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
#include "polymake/common/PolyDBInstance.h"
#include "polymake/common/PolyDBCursor.h"
#include "polymake/common/PolyDBconfig.h"
#include "polymake/common/PolyDBFunctions.h"

#include "polymake/optional"
#include "polymake/Integer.h"
#include "polymake/Array.h"


namespace polymake {
namespace common {
namespace polydb {


class PolyDBCollection {

    public:
    
    ~PolyDBCollection() {
      if ( mongoc_collection_defined_ ) {
        mongoc_collection_destroy(data_);
        mongoc_collection_destroy(info_);
        mongoc_collection_defined_ = false;
      }
    }

    PolyDBCollection(const std::string& name,
                     const std::shared_ptr<mongoc_client_t>& client                    
                     ) : name_(name), 
                         client_(client) {
                          mongoc_database_t *database;
                          database = mongoc_client_get_database(client_.get(), "polydb");
                          data_ = mongoc_database_get_collection (database, name_.c_str());
                          info_ = mongoc_database_get_collection (database, ("_collectionInfo."+name_).c_str());
                          mongoc_database_destroy(database);
                          mongoc_collection_defined_ = true;
    };

    PolyDBCollection(PolyDBCollection &&pc ) {
      std::swap(data_,pc.data_);
      std::swap(info_,pc.info_);
      std::swap(client_,pc.client_);
      name_ = pc.name_;
      mongoc_collection_defined_ = true;
    }

    /**
     * @brief close the connection to the database
     * it is usually not necessary to call this function
     * may be useful for testing, if the user of this connection is removed
     */
    const void close_connection() const {
      std::cout << "closing connection to client" << std::endl;
      if ( mongoc_collection_defined_ ) {
        mongoc_collection_destroy(data_);
        mongoc_collection_destroy(info_);
        client_.reset();
        mongoc_collection_defined_ = false;
      }
    }

    /**
     * @brief find one document in the collection matching the query 
     * the options sort_by and limit may alter the search space
     * @param querystring 
     * @param options projection, sort_by, limit
     * the option limit limits the number of results considered
     * the option sort_by sorts the list of results considered
     * the option projection may define a projection applied to the returned document, it mus remain a valid polymake object!
     * @return const optional<std::string> 
     */
    const optional<std::string> find_one(const std::string& querystring, OptionSet options) const {

      bson_error_t error;
      // FIXME howto remove the cast?
      bson_t * query = bson_new_from_json ((unsigned char *)querystring.c_str(), -1, &error);

      std::string options_string = "{ ";

      bool comma = false;
      if ( options["projection"] ) {
        std::string proj = options["projection"];
        options_string += "\"projection\" : ";
        options_string += proj;
        comma = true;
      } 
      if ( options["sort_by"] ) {
        if ( comma ) {
          options_string += ", ";          
        }
        std::string sort = options["sort_by"];
        options_string += "\"sort\" : ";
        options_string += sort;
      }
      if ( comma ) {
        options_string += ", ";          
      }
      options_string += "\"limit\" : 1";
      options_string += " }";
      
       // FIXME howto remove the cast?
      bson_t * opts = bson_new_from_json ((unsigned char *)options_string.c_str(), -1, &error);

      mongoc_cursor_t *cursor;
      cursor = mongoc_collection_find_with_opts (data_, query, opts, nullptr);

      if (mongoc_cursor_error (cursor, &error)) {
        mongoc_cursor_destroy (cursor);
        bson_destroy (query);
        bson_destroy (opts);
        throw std::runtime_error(prepare_error_message (error, name_));
      }
      
      const bson_t *doc;
      bool result_found = false;
      std::string result;
      if ( mongoc_cursor_next (cursor, &doc)) {
        result_found = true;
        result = to_string_and_free(bson_as_relaxed_extended_json(doc, nullptr));
      }
      mongoc_cursor_destroy (cursor);
      bson_destroy (query);
      bson_destroy (opts);

      if ( result_found ) {
        return make_optional(result);
      } 

      return nullopt;
    }

    /**
     * @brief returns a cursor over all documents in the collection matching the query 
     * the options sort_by and limit may alter the search space
     * @param querystring 
     * @param options projection, sort_by, limit, skip, noCursorTimeout, batchSize
     * the option limit limits the number of results considered
     * the option sort_by sorts the list of results considered
     * the option skip skips the first N results
     * the option projection may define a projection applied to the returned document, it mus remain a valid polymake object!
     * the option noCursorTimeout prevents the cursor from disconnecting if no new batch of documents is requested from the server for some time. By default, the cursor requests about 100 documents in each batch, and closes the connection if no new batch s requested for about 10min. So set this option to true, if your computations take more time than 10 min per 100 documents. Note that the server also implements a timeout of about 30min, which cannot be influenced by a client. If the time between successive requests is longer then the connection will still close. You can use the option batchSize to request fewer documents in each request to prevent this. 
     * the option batchSize sets the number of documents returned with each request to the server. The default is 100.
     * @return PolyDB::Cursor
     */
    PolyDBCursor find(std::string querystring, OptionSet options) const {

      bson_error_t error;
      // FIXME howto remove the cast?
      bson_t * query = bson_new_from_json ((unsigned char *)querystring.c_str(), -1, &error);
      if ( !query ) {
        throw std::runtime_error(prepare_error_message(error,"bson_creation"));
      }

      std::string options_string = "{ ";

      bool comma = false;
      if ( options["sort_by"] ) {
        if ( comma ) {
          options_string += ", ";          
        }
        std::string sort = options["sort_by"];
        options_string += "\"sort\" : ";
        options_string += sort;
        comma = true;
      }
      if ( options["limit"] ) {
        if ( comma ) {
          options_string += ", ";          
        }
        Int limit = options["limit"];
        options_string += "\"limit\" : ";
        options_string += std::to_string(limit);
        comma = true;
      }
      if ( options["skip"] ) {
        if ( comma ) {
          options_string += ", ";          
        }
        Int skip = options["skip"];
        options_string += "\"skip\" : ";
        options_string += std::to_string(skip);
        comma = true;
      }
      if ( options["noCursorTimeout"] ) {
        if ( comma ) {
          options_string += ", ";          
        }
        bool noCursorTimeout = options["noCursorTimeout"];
        options_string += "\"noCursorTimeout\" : ";
        options_string += noCursorTimeout ? "true" : "false";
        comma = true;
      }
      if ( options["batchSize"] ) {
        if ( comma ) {
          options_string += ", ";          
        }
        Int batchSize = options["batchSize"];
        options_string += "\"batchSize\" : ";
        options_string += std::to_string(batchSize);
        comma = true;
      }

      options_string += " }";

       // FIXME howto remove the cast?
      bson_t * opts = bson_new_from_json ((unsigned char *)options_string.c_str(), -1, &error);
      if ( !opts ) {
        bson_destroy(query);
        throw std::runtime_error(prepare_error_message(error,"bson_creation"));
      }

      //char * str = bson_as_relaxed_extended_json (query, nullptr);
      //std::cout << "query: " << str << std::endl;

      mongoc_cursor_t * mongo_cursor = mongoc_collection_find_with_opts (data_, query, opts, nullptr);
      const std::shared_ptr<mongoc_cursor_t> cursor(mongo_cursor, mongoc_cursor_destroy);
      bson_destroy (query);
      bson_destroy (opts);

      if (mongoc_cursor_error (cursor.get(), &error)) {
        throw std::runtime_error(prepare_error_message (error, name_));
      }

      return PolyDBCursor(cursor);
    }

    /**
     * @brief returns the name of the collection
     * 
     * @return const std::string 
     */
    const std::string name() const {
      return name_;
    }

    /**
     * @brief count the number of documents matching the query
     * 
     * @param querystring the query
     * @param options we may limit the number of documents considered, or skip the first N documents
     * @return const long 
     */
    // FIXME count_documents returns int64_t (should be long long), which we cannot convert
    const long count(const std::string& querystring, OptionSet options) const {

      bson_error_t error;
      // FIXME howto remove the cast?
      bson_t * query = bson_new_from_json ((unsigned char *)querystring.c_str(), -1, &error);
      if ( !query ) {
        throw std::runtime_error(prepare_error_message(error,"bson_creation"));
      }
      std::string options_string = "{ ";

      bool comma = false;
      if ( options["limit"] ) {
        Int limit = options["limit"];
        options_string += "\"limit\" : ";
        options_string += std::to_string(limit);
        comma = true;
      }
      if ( options["skip"] ) {
        if ( comma ) {
          options_string += ", ";          
        }
        Int skip = options["skip"];
        options_string += "\"skip\" : ";
        options_string += std::to_string(skip);
        comma = true;
      }
      options_string += " }";

       // FIXME howto remove the cast?
      bson_t * opts = bson_new_from_json ((unsigned char *)options_string.c_str(), -1, &error);
      if ( !opts ) {
        bson_destroy(query);
        throw std::runtime_error(prepare_error_message(error,"bson_creation"));
      }
      long count = mongoc_collection_count_documents (data_, query, opts, nullptr, nullptr, &error);
      bson_destroy(query);
      bson_destroy(opts);

      if ( count < 0 ) {       
        throw std::runtime_error(prepare_error_message(error, name_));
      }

      return count;
    }


    /**
     * @brief return a list of distinct values for a property
     * 
     * @param key the property we want the distinct values for
     * @param querystring an additional query to select the search space for the key
     * @param options currently no options are supported
     * @return const std::string 
     */
    const std::string distinct(const std::string& key, const std::string& querystring, OptionSet options) const {

      bson_t * mongo_command = bson_new();
      bson_append_utf8(mongo_command,"distinct",-1,name_.c_str(),-1);
      bson_append_utf8(mongo_command,"key",-1,key.c_str(),-11);

      bson_error_t error;
      // FIXME howto remove the cast?
      bson_t * query = bson_new_from_json ((unsigned char *)querystring.c_str(), -1, &error);
      if ( !query ) {
        bson_destroy(mongo_command);
        throw std::runtime_error(prepare_error_message(error,"bson_creation"));
      }

      bson_append_document(mongo_command,"query",-1,query);

      mongoc_database_t *database;
      database = mongoc_client_get_database(client_.get(), "polydb");
      bson_t reply;
      bool res = mongoc_database_command_simple (database, mongo_command, nullptr, &reply, &error);

      mongoc_database_destroy(database);
      bson_destroy(query);
      bson_destroy(mongo_command);

      if ( !res ) {
        throw std::runtime_error(prepare_error_message(error,name_));
      }


      bson_iter_t iter;
      bson_iter_init (&iter, &reply);
      bson_iter_find(&iter, "values");
      bson_iter_t sub_iter;
      bson_iter_recurse (&iter, &sub_iter);

      bson_t * values = bson_new();
      int i = 0;
      while( bson_iter_next (&sub_iter) ) {
        bson_append_value(values,std::to_string(i++).c_str(),-1,bson_iter_value (&sub_iter));
      }

      // bson_array_as_json seems to be deprecated, but none of the two replacements
      // bson_as_relaxed_extended_json or beson_as_canonical_extended_json can produce 
      // a json with a top-level array
      // so we need to hope that the function will persist
      std::string ret = to_string_and_free(bson_array_as_json(values, nullptr));
      bson_destroy(values);
      bson_destroy(&reply);

      return ret;
    }


    /**
     * @brief runs an aggregation pipeline on the collection and returns a cursor over the matching objects
     * 
     * @param pipeline_string the pipeline
     * @return const PolyDBCursor 
     */
    const PolyDBCursor aggregate(const std::string& pipeline_string) const {

      bson_error_t error;
      bson_t * pipeline = bson_new_from_json((unsigned char *)pipeline_string.c_str(),-1,&error); 
      if ( !pipeline ) {
        throw std::runtime_error(prepare_error_message(error,"bson_creation"));
      }
      //char * str = bson_as_relaxed_extended_json (pipeline, nullptr);
      //std::cout << "pipeline: " << str << std::endl;

      // option MONGOC_QUERY_NO_CURSOR_TIMEOUT does not work?
      std::shared_ptr<mongoc_cursor_t> cursor(mongoc_collection_aggregate(data_,MONGOC_QUERY_NONE,pipeline,nullptr,nullptr), mongoc_cursor_destroy);
      bson_destroy(pipeline);

      // FIXME handle error with mongoc_cursor_error_document

      return PolyDBCursor(cursor);
    }

    /**
     * @brief insert one document into the collection
     * 
     * @param docstring the document to insert
     * @return bool true if the document was inserted
     */
    const bool insert_one(const std::string& docstring) const {

      bson_error_t error;
      bson_t * doc = bson_new_from_json((unsigned char *)docstring.c_str(),-1,&error); 
      if ( !doc ) {
        throw std::runtime_error(prepare_error_message(error,"bson_creation"));
      }
      bson_t reply;

      bool success = mongoc_collection_insert_one ( data_, doc, nullptr, &reply, &error );
      if ( !success ) {
        bson_destroy(doc);
        throw std::runtime_error(prepare_error_message(error,name_));
      }

      bson_iter_t iter;
      int inserted_count = 0;
      if (bson_iter_init (&iter, &reply)) {
        while (bson_iter_next (&iter)) {
          std::string key = bson_iter_key(&iter);
          if ( key == "insertedCount" ) {
            inserted_count = bson_iter_int32(&iter);
          }
          if ( key == "writeErrors" || key == "writeConcernErrors" ) {
            std::string message = "insertion failed with write errors";
            message += error.message;
            bson_destroy(doc);
            bson_destroy(&reply);
            throw(message);
          }
        }
      }

      bson_destroy(doc);
      bson_destroy(&reply);

      return inserted_count > 0;
    }

    /**
     * @brief insert an array of objects into the collection
     * 
     * @param docstring_array the array of objects as json strings
     * @return int the number of inserted objects
     */
    const int insert_many(const pm::Array<std::string>& docstring_array) const {

      bson_error_t error;

      size_t n = docstring_array.size();
      bson_t * documents[n];  // = (const bson_t **)malloc(sizeof(bson_t *)*n);
      for ( size_t i = 0; i < n; ++i ) {
        documents[i] = bson_new_from_json((unsigned char *)docstring_array[i].c_str(),-1,&error); 
        if ( !documents[i] ) {
          for ( size_t j = 0; j < i; ++j )
             bson_destroy(documents[j]);
          throw std::runtime_error(prepare_error_message(error,"bson_creation"));
        }
      }

      bson_t reply;

      // the cast is also done in the mongoc test cases, so this seems to be the official way to do it
      bool success = mongoc_collection_insert_many ( data_, (const bson_t **) documents, n, nullptr, &reply, &error ) ;
      if ( !success ) {
        bson_destroy(&reply);
        for ( size_t i = 0; i < n; ++i )
           bson_destroy(documents[i]);
        throw std::runtime_error(prepare_error_message(error,name_));
      }

      bson_iter_t iter;
      int inserted_count = 0;
      if (bson_iter_init (&iter, &reply)) {
        while (bson_iter_next (&iter)) {
          std::string key = bson_iter_key(&iter);
          if ( key == "insertedCount" ) {
            inserted_count = bson_iter_int32(&iter);
          }
          if ( key == "writeErrors" || key == "writeConcernErrors" ) {
            std::string message = "insertion failed with write errors";
            message += error.message;
            bson_destroy(&reply);
            for ( size_t i = 0; i < n; ++i )
               bson_destroy(documents[i]);
            throw std::runtime_error(message);
          }
        }
      }

      for ( size_t i = 0; i < n; ++i ) {
        bson_destroy(documents[i]);
      }
      bson_destroy(&reply);

      return inserted_count;
    }

    /**
     * @brief update a single document in the collection based on filter
     * @param filterstring the filter to find the document to update
     * @param docstring the document with the update
     */
    const void update_one(const std::string& filterstring, const std::string& docstring) const {

      bson_error_t error;
      bson_t * doc = bson_new_from_json((unsigned char *)docstring.c_str(),-1,&error); 
      bson_t * filter = bson_new_from_json((unsigned char *)filterstring.c_str(),-1,&error); 
      bson_t reply;

      //char * str = bson_as_relaxed_extended_json (filter, nullptr);
      //std::cout << "filter: " << str << std::endl;

      bool ret = mongoc_collection_update_one (data_, filter, doc, nullptr, &reply, &error);
      bson_destroy(doc);
      bson_destroy(filter);

      if ( !ret ) {
        throw std::runtime_error(prepare_error_message(error,name_));
      }

      // FIXME do something with the reply first
      bson_destroy(&reply);
    }

    /**
     * @brief replace one entry of the collection based on its id
     * 
     * @param docstring the object that should replace the one with the same id in the collection
     */
    const bool replace_one(const std::string& docstring) const {

      bson_error_t error;
      bson_t * doc = bson_new_from_json((unsigned char *)docstring.c_str(),-1,&error); 
      bson_t * filter = bson_new();
      bson_t reply;

      // get the id from the document
      bson_iter_t iter;
      bson_iter_init (&iter, doc);
      bson_iter_find (&iter, "_id");
      bson_append_value (filter, "_id", -1, bson_iter_value (&iter));

      bool ret = mongoc_collection_replace_one (data_, filter, doc, nullptr, &reply, &error);
      bson_destroy(doc);
      bson_destroy(filter);

      if ( !ret ) {
        std::string message = prepare_error_message(error,name_);
        throw std::runtime_error(message);
      }

      int modified_count = 0;
      if (bson_iter_init (&iter, &reply)) {
        while (bson_iter_next (&iter)) {
          std::string key = bson_iter_key(&iter);
          if ( key == "modifiedCount" ) {
            modified_count = bson_iter_int32(&iter);
          }
          if ( key == "writeErrors" || key == "writeConcernErrors" ) {
            std::string message = "insertion failed with write errors";
            message += error.message;
            bson_destroy(&reply);
            throw std::runtime_error(message);
          }
        }
      }

      bson_destroy(&reply);

      return modified_count > 0;
    }

    /**
     * @brief remove one entry of the collection based on its id
     * 
     * @param filterstring 
     * @return const int 
     */
    const bool delete_one(const std::string& id) const {
      bson_error_t error;
      bson_t * filter = bson_new();
      bson_append_utf8(filter, "_id", -1, id.c_str(), -1);
      bson_t reply;

      bool ret = mongoc_collection_delete_one (data_, filter, nullptr, &reply, &error);
      bson_destroy(filter);
      if ( !ret ) {
        std::string message = prepare_error_message(error,name_);
        bson_destroy(&reply);
        throw std::runtime_error(message);
      }

      bson_iter_t iter;
      int del_count = 0;
      if (bson_iter_init (&iter, &reply)) {
        while (bson_iter_next (&iter)) {
          std::string key = bson_iter_key(&iter);
          if ( key == "deletedCount" ) {
            del_count = bson_iter_int32(&iter);
          }
          if ( key == "writeErrors" || key == "writeConcernErrors" ) {
            std::string message = "deletion failed with write errors";
            message += error.message;
            bson_destroy(&reply);
            throw(message);
          }
        }
      }
      bson_destroy(&reply);

      return del_count > 0;
    }

    /**
     * @brief delete entries of the collection matching the filter
     * 
     * @param filterstring the filter
     * @return const int the number of deleted entries
     */
    const int delete_many(const std::string& filterstring) const {
      bson_error_t error;
      bson_t * filter = bson_new_from_json((unsigned char *)filterstring.c_str(),-1,&error); 
      bson_t reply;

      //char * str = bson_as_relaxed_extended_json (filter, nullptr);
      //std::cout << "delete_many_command: " << str << std::endl;


      bool ret = mongoc_collection_delete_many (data_, filter, nullptr, &reply, &error);
      bson_destroy(filter);
      if ( !ret ) {
        std::string message = prepare_error_message(error,name_);
        bson_destroy(&reply);
        throw std::runtime_error(message);
      }

      bson_iter_t iter;
      int del_count = 0;
      if (bson_iter_init (&iter, &reply)) {
        while (bson_iter_next (&iter)) {
          std::string key = bson_iter_key(&iter);
          if ( key == "deletedCount" ) {
            del_count = bson_iter_int32(&iter);
          }
          if ( key == "writeErrors" || key == "writeConcernErrors" ) {
            std::string message = "deletion failed with write errors";
            bson_destroy(&reply);
            throw std::runtime_error(message);
          }
        }
      }
      
      bson_destroy(&reply);

      return del_count;
    }


    /**
     * @brief drop the connection to the database
     * useful for testing
     */
    const void drop() const {

      bson_error_t error;
      bool res;
      res = mongoc_collection_drop(data_,&error);
      if ( !res ) {
        // check if the collection just did not exist
        if ( !( error.domain == MONGOC_ERROR_SERVER && error.code == 26 ) ) {
          std::string message = "drop data collection failed with error ";
          message += error.message;
          message += "and error code ";
          message += std::to_string(error.domain);
          message += std::to_string(MONGOC_ERROR_SERVER);
          throw std::runtime_error(message);
        }
      }
      res = mongoc_collection_drop(info_,&error);
      if ( !res ) {
        // check if the collection just did not exist
        if ( !( error.domain == MONGOC_ERROR_SERVER && error.code == 26 ) ) {
          std::string message = "drop info collection failed with error ";
          message += error.message;
          message += "and error code ";
          message += std::to_string(error.code);
          throw std::runtime_error(message);        
        }
      }
    }

    /**
     * @brief adds indices to the collection
     * This function adds indices to the collection
     * indices must be given as an array of documents containing the keys 
     *  - name
     *  - key
     * they may contain further options like unique
     * @param index_definitions 
     * @return true on success
     */
    const bool add_indices( const Array<std::string>& index_definitions ) const {

      bson_t * mongo_command = bson_new();
      bson_t index_array;
      bson_error_t error;
      bson_t reply;
      bson_append_utf8(mongo_command,"createIndexes",-1,name_.c_str(),-1);
      bson_append_array_begin(mongo_command,"indexes",-1,&index_array);
      for ( int i = 0; i < index_definitions.size(); i++ ) {
        bson_t * idef = bson_new_from_json((unsigned char *)index_definitions[i].c_str(),-1,&error);
        bson_append_document(&index_array,std::to_string(i).c_str(),-1, idef);
        bson_destroy(idef);
      }
      bson_append_array_end(mongo_command,&index_array);

      //char * str = bson_as_relaxed_extended_json (mongo_command, nullptr);
      //std::cout << "pipeline: " << str << std::endl;

      mongoc_database_t *database;
      database = mongoc_client_get_database(client_.get(), "polydb");

      bool res = mongoc_database_command_simple (database, mongo_command, nullptr, &reply, &error);
      bson_destroy(mongo_command);
      bson_destroy(&reply);
      mongoc_database_destroy(database);

      if ( !res ) {
        throw std::runtime_error(prepare_error_message(error,name_));
      }

      return true;
    }

    /**
     * @brief adds a single index to the collection
     * A name mus be given, the index definition hsould contain the key
     * @param name a name for the index
     * @param index_definition the key for the index
     * @param unique true if the index describes a unique property
     * @return true on success
     */
    const bool add_index(const std::string&& name, std::string&& index_definition, const OptionSet options ) const {

      bson_t * index = bson_new();
      bson_error_t error;
      bson_append_utf8(index,"name",-1,name.c_str(),-1);
      bson_t * idef = bson_new_from_json((unsigned char *)index_definition.c_str(),-1,&error);
      bson_append_document(index,"key",-1, idef);
      bson_destroy(idef);
      if ( options["unique"] ) {
        bson_append_bool(index,"unique",-1,true);
      }

      std::string index_definition_str = to_string_and_free(bson_as_relaxed_extended_json(index, nullptr));
      Array<std::string> index_definitions(1);
      index_definitions[0] = index_definition_str;

      bson_destroy(index);

      // FIXME check result
      return add_indices(index_definitions);
    }

    /**
     * @brief adds simple indices for a list of properties
     * For the given list of properties an index is created on this property
     * @param properties the list of properties
     * @param options ascending: whether the index should be ascending, unique: whether the property is unique
     * @return true on success
     */
    const bool add_indices_from_properties( const Array<std::string>& properties, const OptionSet options ) const {

      Array<std::string> index_definitions(properties.size());

      int i = 0;
      for ( auto property : properties ) {
        bson_t * index = bson_new();
        bson_t key;
        bson_append_utf8(index,"name",-1,property.c_str(),-1);
        bson_append_document_begin(index,"key",-1,&key);
        bson_append_int32(&key,property.c_str(),-1, options["ascending"] ? 1 : -1);
        bson_append_document_end(index,&key);
        if ( options["unique"] ) {
          bson_append_bool(index,"unique",-1,true);
        }
        index_definitions[i++] = to_string_and_free(bson_as_relaxed_extended_json(index, nullptr));
        bson_destroy(index);
      }

      // FIXME check result
      return add_indices(index_definitions);
    }

    /**
     * @brief adds an index for a single property
     * For a given property, an index is created on this property
     * @param property the property
     * @param options ascending: whether the index should be ascending, unique: whether the property is unique
     * @return true on success
     */
    const bool add_index_from_property(const std::string&& property, const OptionSet options ) const {

      Array<std::string> index(1);
      index[0] = property;

      // FIXME check result
      return add_indices_from_properties(index, options);
    }


    /**
     * @brief Obtains a list of the names of all indexes defined for this collection
     * 
     * @return Array<std::string> the names of all indexes defined for this collection
     */
    const Array<std::string> get_index_names() const {
      
      std::vector<std::string> index_names;
      mongoc_cursor_t * index_cursor = mongoc_collection_find_indexes_with_opts (data_, nullptr);

      const bson_t *index;
      bson_iter_t index_iter;
      bson_error_t error;
      const bson_t * reply;

      while (mongoc_cursor_next (index_cursor, &index)) {
        if (bson_iter_init (&index_iter, index) &&
            bson_iter_find (&index_iter, "name") &&
            BSON_ITER_HOLDS_UTF8 (&index_iter)) {
            std::string index_name(bson_iter_utf8 (&index_iter, nullptr));
            index_names.push_back (index_name);
        }
      }

      if (mongoc_cursor_error_document (index_cursor, &error, &reply)) {
        std::string message = "obtaining index names failed with error ";
        message += error.message;
        message += "\n";
        message += to_string_and_free(bson_as_relaxed_extended_json (reply, nullptr));
        mongoc_cursor_destroy(index_cursor);
        throw std::runtime_error(message);
      }

      mongoc_cursor_destroy(index_cursor);
      return Array<std::string>(index_names.size(), entire(index_names));
    }

    /**
     * @brief Get a description of all indexes defined for this collection
     * 
     * @return const Array<std::string> an array of json strings each describing an index
     */
    const Array<std::string> get_indexes() const {
      
      std::vector<std::string> indexes;
      mongoc_cursor_t * index_cursor = mongoc_collection_find_indexes_with_opts (data_, nullptr);

      const bson_t *index;
      bson_error_t error;

      while (mongoc_cursor_next (index_cursor, &index)) {
        bson_t * filtered_index = bson_new();
        bson_iter_t index_iter;
        bson_iter_init (&index_iter, index);
        bson_iter_find (&index_iter, "name");
        if ( strcmp(bson_iter_utf8 (&index_iter, nullptr),"_id_") != 0 ) {
          bson_iter_init (&index_iter, index);
          while (bson_iter_next (&index_iter)) {
            if ( *bson_iter_key(&index_iter) != 'v' &&  strcmp(bson_iter_key(&index_iter),"ns") != 0 ) {
              bson_append_value (filtered_index, bson_iter_key (&index_iter),-1,bson_iter_value (&index_iter));
            }
          }

          indexes.push_back(to_string_and_free(bson_as_relaxed_extended_json (filtered_index, nullptr)));
        }
        bson_destroy(filtered_index);
      }

      const bson_t * reply;
      if (mongoc_cursor_error_document (index_cursor, &error, &reply)) {
        std::string message = "obtaining indexes failed with error ";
        message += error.message;
        message += "\n";
        message += to_string_and_free(bson_as_relaxed_extended_json (reply, nullptr));
        mongoc_cursor_destroy(index_cursor);
        throw std::runtime_error(message);
      }

      mongoc_cursor_destroy(index_cursor);
      return Array<std::string>(indexes.size(), entire(indexes));
    }

    /**
     * @brief Get the info doc 
     * 
     * @return const std::string the json representation of the info doc as string
     */
    const std::string get_info() const {

      if ( info_doc_.length() > 0 ) {
        return info_doc_;
      }

      std::string id = "info."+polyDBVersion;

      mongoc_cursor_t *cursor;
      bson_t * query = bson_new();
      bson_append_utf8(query, "_id",-1,id.c_str(),-1);
      cursor = mongoc_collection_find_with_opts (info_, query, nullptr, nullptr);
      bson_destroy (query);

      bson_error_t error;
      if (mongoc_cursor_error (cursor, &error)) {
        std::string message = "obtaining the info document failed with error ";
        message += error.message;
        message += "\n";
        mongoc_cursor_destroy (cursor);
        throw std::runtime_error(message);
      }
      
      const bson_t *doc;
      bool result_found = false;
      if ( mongoc_cursor_next (cursor, &doc)) {
        info_doc_ = to_string_and_free(bson_as_relaxed_extended_json (doc, nullptr));
        bson_iter_t iter;
        if ( bson_iter_init (&iter, doc) &&
        bson_iter_find (&iter, "schema") &&
        BSON_ITER_HOLDS_UTF8 (&iter) ) {
          result_found = true;
          schema_key_ = bson_iter_utf8 (&iter, nullptr);
        }
      }
      mongoc_cursor_destroy (cursor);

      if ( !result_found ) {
        throw std::runtime_error("obtaining the info document failed: no info document found - corrupted metadata?");
      } 

      return info_doc_;
    }

    /**
     * @brief Get the schema 
     * 
     * @return const std::string the json representation of the schema as string
     */
    const std::string get_schema() const {

      if ( schema_doc_.length() > 0 ) {
        return schema_doc_;
      }

      if ( schema_key_.length() == 0 ) {
        get_info();
      }

      mongoc_cursor_t *cursor;
      bson_t * query = bson_new();
      bson_append_utf8(query, "_id", -1, schema_key_.c_str(), -1);
      cursor = mongoc_collection_find_with_opts (info_, query, nullptr, nullptr);
      bson_destroy (query);

      bson_error_t error;
      if (mongoc_cursor_error (cursor, &error)) {
        std::string message = "obtaining the schema failed with error ";
        message += error.message;
        message += "\n";
        mongoc_cursor_destroy (cursor);
        throw std::runtime_error(message);
      }

      const bson_t *doc;
      bool result_found = false;
      if ( mongoc_cursor_next (cursor, &doc)) {
        schema_doc_ = to_string_and_free(bson_as_relaxed_extended_json (doc, nullptr));
        result_found = true;
      }
      mongoc_cursor_destroy (cursor);

      if ( !result_found ) {
        throw std::runtime_error("obtaining the schema failed: no info document found - corrupted metadata?");
      } 

      return schema_doc_;
    }


    /**
     * @brief Set the schema for this collection
     * 
     * @param schema_string the schema
     * @param schema_key_in the key/id for the schema in the database
     */
    const void set_schema(const std::string &schema_string, const std::string& schema_key_in ) const {

      std::string schema_key = schema_key_in;
      if ( schema_key.length() == 0 ) {
        schema_key = "schema." + polyDBVersion;
      }

      bson_error_t error;
      bson_t * schema = bson_new_from_json ((const unsigned char *)schema_string.c_str(), -1, &error);

      bson_t * schema_doc = bson_new();
      bson_append_utf8(schema_doc, "_id", -1, schema_key.c_str(), -1);
      bson_append_document(schema_doc, "schema", -1, schema);
      bson_destroy(schema);

      const bson_t *schema_test;
      bson_t * filter = bson_new();
      bson_append_utf8(filter, "_id", -1, schema_key.c_str(), -1);
      mongoc_cursor_t * cursor = mongoc_collection_find_with_opts (info_, filter, nullptr, nullptr);
      bool schema_exists = false;
      if ( mongoc_cursor_next (cursor, &schema_test) ) {
        schema_exists = true;
      }
      mongoc_cursor_destroy (cursor);

      if ( schema_exists ) {
        bool ret = mongoc_collection_replace_one (info_, filter, schema_doc, nullptr, nullptr, &error);
        bson_destroy(filter);
        if ( !ret ) {
          std::string message = "replacing schmema failed with error ";
          message += error.message;
          message += " and error code ";
          message += std::to_string(error.domain);
          message += std::to_string(MONGOC_ERROR_SERVER);
          bson_destroy(schema_doc);
          throw std::runtime_error(message);
        }
      } else {
        bson_destroy(filter);
        bool ret = mongoc_collection_insert_one ( info_, schema_doc, nullptr, nullptr, &error ) ;
        if ( !ret ) {
          std::string message = "inserting schema failed with error ";
          message += error.message;
          message += " and error code ";
          message += std::to_string(error.domain);
          message += std::to_string(MONGOC_ERROR_SERVER);
          bson_destroy(schema_doc);
          throw std::runtime_error(message);
        }
      }

      schema_doc_ = to_string_and_free(bson_as_relaxed_extended_json (schema_doc, nullptr));
      bson_destroy(schema_doc);
    }

    /**
     * @brief Set the info for this collection
     * 
     * @param info_string the info
     * @param schema_id the id of the corresponding schema
     */
    const void set_info(const std::string &info_string, const std::string & schema_id ) const {

      std::string info_id = "info." + polyDBVersion;
      bson_error_t error;
      bson_t * info = bson_new_from_json ((const unsigned char *)info_string.c_str(), -1, &error);
      bson_append_utf8(info, "_id", -1, info_id.c_str(), -1);
      bson_append_utf8(info,"schema",-1,schema_id.c_str(),-1);

      const bson_t *info_test;
      bson_t * filter = bson_new();
      bson_append_utf8(filter, "_id", -1, info_id.c_str(), -1);
      mongoc_cursor_t * cursor = mongoc_collection_find_with_opts (info_, filter, nullptr, nullptr);
      bool info_exists = false;
      if ( mongoc_cursor_next (cursor, &info_test) ) {
        info_exists = true;
      }
      mongoc_cursor_destroy (cursor);

      if ( info_exists ) {
        bool ret = mongoc_collection_replace_one (info_, filter, info, nullptr, nullptr, &error);
        bson_destroy(filter);
        bson_destroy(info);
        if ( !ret ) {
          std::string message = "replacing info failed with error ";
          message += error.message;
          message += " and error code ";
          message += std::to_string(error.domain);
          message += std::to_string(MONGOC_ERROR_SERVER);
          throw std::runtime_error(message);
        }
      } else {
        bson_destroy(filter);
        bool ret = mongoc_collection_insert_one ( info_, info, nullptr, nullptr, &error ) ;
        bson_destroy(info);
        if ( !ret ) {
          std::string message = "inserting info failed with error ";
          message += error.message;
          message += " and error code ";
          message += std::to_string(error.domain);
          message += std::to_string(MONGOC_ERROR_SERVER);
          throw std::runtime_error(message);
        }
      }

    }

    /**
     * @brief Set the documentation for this collection
     * 
     * @param docstring the documentation
     * @param options update: true if an existing document should be updated
     */
    const void set_doc(const std::string &docstring, const OptionSet options ) const {

      std::string id = name_+"."+polyDBVersion;
      bson_error_t error;
      bson_t * doc = bson_new_from_json ((const unsigned char *)docstring.c_str(), -1, &error);

      bson_t *query = bson_new();

      bson_append_utf8(query, "_id", -1, id.c_str(), -1);
      mongoc_cursor_t * cursor = mongoc_collection_find_with_opts (info_, query, nullptr, nullptr);
      bson_destroy (query);
      if (mongoc_cursor_error (cursor, &error)) {
        std::string message = "check for collection id failed with error ";
        message += error.message;
        message += "and error code ";
        message += std::to_string(error.domain);
        message += std::to_string(MONGOC_ERROR_SERVER);
        mongoc_cursor_destroy (cursor);
        throw std::runtime_error(message);
      }
      
      const bson_t *doc_test;
      bool doc_exists = false;
      if ( mongoc_cursor_next (cursor, &doc_test) ) {
        doc_exists = true;
      }
      mongoc_cursor_destroy (cursor);

      if ( !doc_exists && options["update"] ) {
        throw std::runtime_error("updating non-existing documentation");
      }

      if ( options["update"] ) {
        bson_t * mongo_doc = bson_new();
        bson_append_document(mongo_doc, "$set", -1, doc);
        bson_destroy (doc);

        bson_t * filter = bson_new();
        bson_append_utf8(filter, "_id", -1, id.c_str(), -1);
    
        bson_t reply;

        bool ret = mongoc_collection_update_one (info_, filter, mongo_doc, nullptr, &reply, &error);
        bson_destroy(&reply);
        bson_destroy(filter);
        bson_destroy (mongo_doc);
        if ( !ret ) {
          std::string message = "updating doc info failed with error ";
          message += error.message;
          message += "and error code ";
          message += std::to_string(error.domain);
          message += std::to_string(MONGOC_ERROR_SERVER);
          throw std::runtime_error(message);
        }
      } else {
        // if we do not update, then we need to add the correct id
        bson_append_utf8(doc, "_id", -1, id.c_str(), -1);
        if ( doc_exists ) {
          bson_t * filter = bson_new();
          bson_append_utf8(filter, "_id", -1, id.c_str(), -1);
          bson_t reply;

          bool ret = mongoc_collection_replace_one (info_, filter, doc, nullptr, &reply, &error);
          bson_destroy(filter);
          bson_destroy (doc);
          bson_destroy(&reply);
          if ( !ret ) {
            std::string message = "replacing doc info failed with error ";
            message += error.message;
            message += "and error code ";
            message += std::to_string(error.domain);
            message += std::to_string(MONGOC_ERROR_SERVER);
            throw std::runtime_error(message);
          }

        } else {
          bson_t reply;
          bool success = mongoc_collection_insert_one ( info_, doc, nullptr, &reply, &error ) ;
          bson_destroy (doc);
          bson_destroy(&reply);
          if ( !success ) {
            throw std::runtime_error(prepare_error_message(error,name_));
          }
        }
      }
    }

    private:
    mutable bool mongoc_collection_defined_;
    std::string name_;
    
    // cache
    // provided by get_info()
    mutable std::string info_doc_;
    mutable std::string schema_key_;
    // provided by get_schema()
    mutable std::string schema_doc_;

    mongoc_collection_t * data_;
    mongoc_collection_t * info_;

    mutable std::shared_ptr<mongoc_client_t> client_;
};


}}}
