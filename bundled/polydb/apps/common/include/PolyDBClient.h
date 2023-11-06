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

#include "polymake/Array.h"

#include "polymake/common/PolyDB.h"
#include "polymake/common/PolyDBInstance.h"
#include "polymake/common/PolyDBconfig.h"
#include "polymake/common/PolyDBCollection.h"
#include "polymake/common/PolyDBSection.h"
#include "polymake/common/PolyDBFunctions.h"



namespace polymake {
namespace common {
namespace polydb {

  extern bool polydb_connection_started;

  class PolyDBUser {

    public:
    PolyDBUser(const std::string& username) : username(username) {
      hasPublicAccess = false;
      canChangePassword = false;
    }

    const std::string get_username() const { return username; }
    const bool has_public_access() const { return hasPublicAccess; }
    const bool can_change_password() const { return canChangePassword; }
    const std::vector<std::string> get_collections() const { return collections; }
    const std::vector<std::string> get_admin_collections() const { return admin_collections; }
    const std::vector<std::string> get_admin_roles() const { return admin_roles; }

    const void set_username(const std::string& val) {
      username = val;
    }

    const void set_has_public_access(const bool val) {
      hasPublicAccess = val;
    }

    const void set_can_change_password(const bool val) {
      canChangePassword = val;
    }

    const void add_collection(const std::string& val) {
      collections.push_back(val);
    }

    const void add_admin_collection(const std::string& val) {
      admin_collections.push_back(val);
    }

    const void add_admin_role(const std::string& val) {
      admin_roles.push_back(val);
    }

    const bool remove_collection(const std::string& val) {
      bool ret = false;
      auto it = std::find(collections.begin(), collections.end(), val);
      if(it != collections.end()) {
        ret = true;
        collections.erase(it);
      }
      return ret;
    }

    void get_bson(bson_t ** user, bool include_username ) {

      if ( include_username ) {
        bson_append_utf8(*user, "username", -1, username.c_str(), -1);
      }
      bson_append_bool(*user, "hasPublicAccess", -1, hasPublicAccess);
      bson_append_bool(*user, "canChangePassword", -1, canChangePassword);

      bson_t col_array;
      int i = 0;
      bson_append_array_begin(*user, "collections", -1, &col_array);
      std::sort ( collections.begin(), collections.end() );
      for ( auto col: collections ) {
        bson_append_utf8(&col_array, std::to_string(i++).c_str(), -1, col.c_str(), -1);
      }
      bson_append_array_end(*user, &col_array);

      bson_t admin_col_array;
      i = 0;
      bson_append_array_begin(*user, "admin_collections", -1, &admin_col_array);
      std::sort ( admin_collections.begin(), admin_collections.end() );
      for ( auto col: admin_collections ) {
        bson_append_utf8(&admin_col_array, std::to_string(i).c_str(), -1, col.c_str(), -1);
      }
      bson_append_array_end(*user, &admin_col_array);

      bson_t role_array;
      bson_append_array_begin(*user, "admin_roles", -1, &role_array);
      i = 0;
      std::sort ( admin_roles.begin(), admin_roles.end() );
      for ( auto role: admin_roles ) {
        bson_append_utf8(&role_array, std::to_string(i).c_str(), -1, role.c_str(), -1);
      }
      bson_append_array_end(*user, &role_array);
    }

    private: 
      std::string username;
      bool hasPublicAccess;
      bool canChangePassword;
      std::vector<std::string> collections;
      std::vector<std::string> admin_collections;
      std::vector<std::string> admin_roles;
  };

  struct PolyDBRolePrivilege {
    std::string db;
    std::string collection;
    std::vector<std::string> actions;
  };

  class PolyDBRole {

    public:
      PolyDBRole(const std::string &name) {
        roleName = name;
      }

      const void add_privilege(const std::string &db, 
                               const std::string &collection, 
                               const std::vector<std::string> &actions) {
        PolyDBRolePrivilege priv;
        priv.db = db;
        priv.collection = collection;
        priv.actions = actions;
        privileges.push_back(priv);
      }

      const std::string get_name() const {
        return roleName;
      }

      const void add_role(const std::string &role) {
        roles.push_back(role);
      }

      const std::vector<PolyDBRolePrivilege> get_privileges() const {
        return privileges;
      }

      const std::vector<std::string> get_roles() const {
        return roles;
      }

      const void print_role() {
        for (auto priv : privileges) {
          std::cout << "on db " 
                    << priv.db 
                    << " and collection " 
                    << priv.collection << ": " 
                    << std::endl
                    << "    ";

          for (auto action : priv.actions) {
            std::cout << action << " ";
          }
          std::cout << std::endl;
        }
      }

      void get_bson(bson_t ** role ) const {

        bson_t privileges_array, role_array;

        bson_append_utf8(*role, "createRole", -1, roleName.c_str(), -1);

        int i = 0;
        bson_append_array_begin(*role, "privileges", -1, &privileges_array);

        for (auto privilege: privileges) {
          bson_t priv, act, res;
          bson_append_document_begin(&privileges_array, std::to_string(i++).c_str(), -1, &priv);
            bson_append_document_begin(&priv, "resource", -1, &res);
              bson_append_utf8(&res, "db", -1, privilege.db.c_str(), -1);
              bson_append_utf8(&res, "collection", -1, privilege.collection.c_str(), -1);
            bson_append_document_end(&priv, &res);
            bson_append_array_begin(&priv, "actions", -1, &act);
            int j = 0;
            for (auto action: privilege.actions) {
              bson_append_utf8(&act, std::to_string(j++).c_str(), -1, action.c_str(), -1);
            } 
            bson_append_array_end(&priv, &act);
          bson_append_document_end(&privileges_array, &priv);
        }
        bson_append_array_end(*role, &privileges_array);

        bson_append_array_begin(*role, "roles", -1, &role_array);
        i = 0;
        for ( auto r: roles ) {
          bson_append_utf8(&role_array, std::to_string(i++).c_str(), -1, r.c_str(), -1);
        }
        bson_append_array_end(*role, &role_array);
      }

    private:
      std::string roleName;
      std::vector<PolyDBRolePrivilege> privileges;
      std::vector<std::string> roles;
  };

  void client_destroy(mongoc_client_t * client);

  class PolyDBClient {

  public:
    /**
     * @brief Construct a new PolyDB Client object
     * 
     * @param[in] db_uri the server address
     * @param[in] port the port on which the database is running
     * @param[in] use_ssl whether to use tls for the connection
     * @param[in] tlsAllowInvalidHostnames whether the hostname presented by the mongodb server should be validated
     * @param[in] tlsAllowInvalidCertificates whether the certificate presented by the mongodb server should be validated
     * @param[in] serverSelectionTimeout the server selection timeout
     * @param[in] socketTimeout the socket timeout
     * @param[in] username the username to authenticate with
     * @param[in] password the password to authenticate with
     * @param[in] default_username the default username of this polydb instance
     * @param[in] default_password the default password of this polydb instance
     * @param[in] auth_db_name the name of the mongodb admin database
     * @param[in] db_name the name of the database containing the polydb data
     * @param[in] polydb_version the version of polydb
     * @param[in] options options, if any
     */
    PolyDBClient(const std::string &db_uri,
                 const std::string &port,
                 const bool use_ssl,
                 const bool tlsAllowInvalidHostnames,
                 const bool tlsAllowInvalidCertificates,
                 const long serverSelectionTimeout,
                 const long socketTimeout,
                 const std::string &username,
                 const std::string &password,
                 const std::string &default_username,
                 const std::string &default_password,
                 const std::string &auth_db_name,
                 const std::string &db_name,
                 const std::string &polydb_version,
                 OptionSet options) {

      host_ = db_uri;
      port_ = port;
      useSSL_ = use_ssl;
      tlsAllowInvalidHostnames_ = tlsAllowInvalidHostnames;
      tlsAllowInvalidCertificates_ = tlsAllowInvalidCertificates;
      serverSelectionTimeout_ = serverSelectionTimeout;
      socket_timeout_ = socketTimeout;
      username_ = username;
      default_username_ = default_username;
      default_password_ = default_password;
      auth_db_name_ = auth_db_name;
      db_name_ = db_name;
      polydb_version_ = polydb_version;

      if (!polydb_connection_started)
        polydb_init();

      std::string db_options = "/" + auth_db_name;
      std::string uri_option_concat = "?";
      if (useSSL_) {
        db_options.append(uri_option_concat + "tls=true");
        uri_option_concat = "&";
      }
      if (options["serverSelectionTimeout"]) {
        int timeout = options["serverSelectionTimeout"];
        db_options.append(uri_option_concat + "serverselectiontimeoutms=" + std::to_string(timeout));
        uri_option_concat = "&";
      }
      if (tlsAllowInvalidHostnames_) {
        db_options.append(uri_option_concat + "tlsAllowInvalidHostnames=true");
        uri_option_concat = "&";
      }
      if (tlsAllowInvalidCertificates_) {
        db_options.append(uri_option_concat + "tlsAllowInvalidCertificates=true");
        uri_option_concat = "&";
      }

      std::string uri("mongodb://" + username + ":" + password + "@" + db_uri + ":" + port + db_options);

      bson_error_t error;
      mongoc_init();
      mongoc_uri_t * mongoc_uri_ = mongoc_uri_new_with_error(uri.c_str(), &error);
      // no make_shared, so copy
      mongoc_client_ = std::shared_ptr<mongoc_client_t>(mongoc_client_new_from_uri(mongoc_uri_),client_destroy);
      mongoc_uri_destroy (mongoc_uri_);
      if ( !mongoc_client_.get() ) {
        std::string message = prepare_error_message(error, "client connection");
        throw std::runtime_error(message);
      }

      mongoc_client_set_error_api (mongoc_client_.get(), MONGOC_ERROR_API_VERSION_2 );
      
      client_id_ = generate_client_id(20);
      mongoc_client_set_appname (mongoc_client_.get(), client_id_.c_str());

      bson_t * command = BCON_NEW ("ping", BCON_INT32 (1));
      bson_t reply;
      bool success = mongoc_client_command_simple ( mongoc_client_.get(), "admin", command, nullptr, &reply, &error);
      bson_destroy (command);
      bson_destroy (&reply);
      if ( !success ) {
        throw std::runtime_error(prepare_error_message(error, "ping"));
      }

      auth_db_name_ = auth_db_name;
      username_ = username;
      mongoc_client_defined_ = true;
    }

    /**
     * @brief close the connection to the mongodb server and deallocate the mongoc_client_t structure
     * close the connection and deallocate the mongoc_client_t structure
     * this happens normally when the variable goes out of scope
     * explicitely necessary e.g. before we delete the user or remove access rights
     */
    const void close_connection() const {
      if ( mongoc_client_defined_ ) {
        mongoc_client_.reset();
        mongoc_client_defined_ = false;
      }
    }

    /**
     * @brief connect to a collection
     * 
     * @param[in] collection_name the name of the collection
     * @return PolyDBCollection the collection
     */
    PolyDBCollection get_collection(const std::string& collection_name) const {      
      return PolyDBCollection(collection_name,mongoc_client_);
    }

    /**
     * @brief connect to a section
     * 
     * FIXME a separate class is probably not needed here
     * @param[in] section_name the name of the section
     * @return PolyDBSection the section
     */
    PolyDBSection get_section(const std::string& section_name) const {
      return PolyDBSection(section_name,mongoc_client_);
    }

    /**
     * @brief creates a new collection
     * prepares a new collection.
     * This requires two new roles with the name of the collection:
     *  - one that gives read access to the collection and its info collection
     *  - one that gives read and write access to both
     * If public is true, the read role is added to the default user
     *
     *  @param[in] name the name of the collection
     *  @param[in] public_colection whether the collection should be readable for the default user
     *  @param[in] users list of users with read access (only useful if not public)
     *  @param[in] admin_user list of users with write access
     *  @return true on success
     */
    const bool new_collection(const std::string& name,
                              const bool public_collection, 
                              const Array<std::string> users, 
                              const Array<std::string> admin_users ) const {

      bool ret = true;

      if ( !create_roles_for_collection(name) ) { ret = false; }
      if ( public_collection ) {
        if ( !add_role_to_role(defaultPolymakeRole, name) ) { ret = false; }
      }

      for ( auto&& user: users) {
        // add a user only if she is not also admin_user
        auto it = std::find(admin_users.begin(), admin_users.end(), user);
        if(it != admin_users.end()) {
          if ( !add_role_to_user(user, name) ) { ret = false; }
        }
      }

      std::string admin_name(name+".admin");
      for ( auto&& user : admin_users ) {
        if ( !add_role_to_user(user, admin_name) ) { ret = false; }
      }

      return ret;
    }

    /**
    * @brief deletes a collection
    * drops a collection and its corresponding info collection from the database    
    * @param[in] collection the name of the collection to be deleted
    * @return true on success
    */
    const bool drop_collection(const std::string& name) const {

      if ( !collection_exists(name) ) {
        return false;
      }

      bson_t * dropRole = bson_new();
      bson_t * dropAdminRole = bson_new();
      bson_t reply;
      bson_error_t error;
      mongoc_database_t * database;

      database = mongoc_client_get_database(mongoc_client_.get(), "admin");

      // drop the read role
      std::string role_name = name;
      bson_append_utf8(dropRole, "dropRole", -1, role_name.c_str(), -1);
      bool res = mongoc_database_command_simple (database, dropRole, nullptr, &reply, &error);
      bson_destroy(&reply);
      bson_destroy(dropRole);
      if ( !res ) {
        mongoc_database_destroy(database);
        throw std::runtime_error(prepare_error_message(error,"database_command"));
      }

      // drop the write role
      role_name += ".admin";
      bson_append_utf8(dropAdminRole, "dropRole", -1, role_name.c_str(), -1);
      res = mongoc_database_command_simple (database, dropAdminRole, nullptr, &reply, &error);
      bson_destroy(&reply);
      bson_destroy(dropAdminRole);
      mongoc_database_destroy(database);
      if ( !res ) {
        throw std::runtime_error(prepare_error_message(error,"database_command"));
      }

      // drop the collections
      PolyDBCollection pc(get_collection(name));
      pc.drop();


      return true;
    }

    /**
     * @brief remove a section
     * 
     * removes a section from the database if
     *  - it exists
     *  - it has no subsections
     *  - it does not contain collections
     * @param[in] name the name of the section  
     * @return true on success
     */
    const bool drop_section(const std::string& name) const {

      Array<std::string> section = get_allowed_collection_names("",name,"","",true,false,false);
      if ( section.size() == 0 ) {
        throw std::runtime_error("Documentation for section " + name + " does not exist");
      }

      Array<std::string> subsections = get_allowed_collection_names("",name + "\\.","","",true,false,false);
      if ( subsections.size() > 0 ) {
        throw std::runtime_error("Section " + name + " contains subsections");
      }

      Array<std::string> collections = get_allowed_collection_names("",name,"","",false,true,true);
      if ( collections.size() > 0 ) {
        throw std::runtime_error("Section " + name + " contains collections");
      }

      PolyDBSection sec(get_section(name));
      sec.drop();
      
      return true;
    }
      
    /**
     * @brief Create a user in the database
     * Create a user in the database
     * @param[in] username the username
     * @param[in] password the password
     * @param[in] options public_read_access: whether the user has access to public collections, 
     *                    canChangeOwnAccount: whether the user can change their password
     *                    collections: a list of collections the user has read access to
     *                    adminCollections: a list of collections the user has write access to
     * @return true on success
     */
    const bool create_user(const std::string &username,
                           const std::string &password,
                           const OptionSet options) const {

      /*
        We need to do:
          * create the user in the db
          * create the associated user role
          * add collection roles to this
          * add admin collection roles to this
      */

      // the list of roles for the user
      std::vector<std::string> roles;

      // allow user to change password etc.
      if (options["canChangeOwnAccount"]) {
        roles.push_back("changeOwnAccount");
      }

      // add role polymakeUser if user should have access to all public collections
      if (options["public_read_access"]) {
        roles.push_back("polymakeUser");
      }

      // list of collections the user has read access
      // if user has public access then public collections can, but need not be incuded
      std::vector<std::string> read_collections;

      // list of collections the user has admin access to
      std::vector<std::string> admin_collections;

      Array<std::string> cols = options["collections"];
      for (std::string col : cols) {
        roles.push_back(col);
        read_collections.push_back(col);
      }

      Array<std::string> admin_cols = options["admin_collections"];
      for (std::string col : admin_cols) {
        std::string col_admin = col + ".admin";
        roles.push_back(col);
        roles.push_back(col_admin);

        read_collections.push_back(col);
        admin_collections.push_back(col);
      }

      bson_t *mongo_command = bson_new();
      bson_t child;

      bson_append_utf8(mongo_command, "createUser", -1, username.c_str(), -1);
      bson_append_utf8(mongo_command, "pwd", -1, password.c_str(), -1);
      bson_append_array_begin(mongo_command, "roles", -1, &child);
      for (size_t i = 0; i < roles.size(); ++i ) {
        bson_t child2;
        bson_append_document_begin(&child, std::to_string(i).c_str(), -1, &child2);
        bson_append_utf8(&child2, "role", -1, roles[i].c_str(), -1);
        bson_append_utf8(&child2, "db", -1, auth_db_name_.c_str(), -1);
        bson_append_document_end(&child, &child2);
      }
      bson_append_array_end(mongo_command,&child);

      //char * str = bson_as_canonical_extended_json (mongo_command, nullptr);
      //std::cout << str << std::endl;

      mongoc_database_t *database;
      bson_t reply;
      bson_error_t error;
      database = mongoc_client_get_database(mongoc_client_.get(), "admin");

      bool res = mongoc_database_command_simple (database, mongo_command, nullptr, &reply, &error);
      bson_destroy(&reply);
      bson_destroy(mongo_command);
      mongoc_database_destroy(database);
      if  (!res ) {
        throw std::runtime_error(prepare_error_message(error,"database_command"));
      }

      return res;
    }

    /*
       recursive: makes only sense if only sections are listed
       false: list only sections of one level
       true: recursvely list all sections
    */
    const Array<std::string> get_allowed_collection_names(const std::string& section,
                                                          const std::string &filter,
                                                          const std::string &filter_sections,
                                                          const std::string &filter_collections,
                                                          const bool only_sections,
                                                          const bool only_collections,
                                                          const bool recursive) const {

      if (filter.length() != 0 && (filter_collections.length() != 0 || filter_sections.length() != 0)) {
        throw std::runtime_error("option filter cannot be used with filter_collections or filter_sections");
      }
      if (filter_collections != "") {
        if (!std::regex_match(filter_collections, std::regex(regex_collection_names))) {
          throw std::runtime_error("use filter on qualified names for regular expressions");
        }
      }
      if (filter_sections != "") {
        if (!std::regex_match(filter_sections, std::regex(regex_section_names))) {
          throw std::runtime_error("use filter on qualified names for regular expressions");
        }
      }
      if (only_sections && only_collections) {
        throw std::runtime_error("only one of only_sections and only_collections can be given");
      }
      if (only_sections && filter_collections.length() != 0) {
        throw std::runtime_error("filter_collections cannot be used with only_sections ");
      }

      // build up the regex for list_collections
      // we only search the _collectionInfo and _sectionInfo collections
      // as we need to distinguish between
      // sections and collections
      // a regular expression can only be given in filter, not in filter_collections or filter_sections
      std::string regex_filter;

      // fix the start of the regex depending on only_sections and only_collections
      // we need to escape "." twice, so "\\\\."
      if (only_collections) {
        regex_filter = "^_collectionInfo\\.";
      }
      else if (only_sections) {
        regex_filter = "^_sectionInfo\\.";
      }
      else {
        regex_filter = "^_(collection|section)Info\\.";
      }
      if ( section.length()!= 0 ) {
        regex_filter += std::string(section) + "\\.";
      }

      // first deal with filter_sections, filter_collections
      if (filter_sections.length() != 0 || filter_collections.length() != 0) {
        // filter is empty and we may have to deal with
        // filter_sections and filter_collections
        regex_filter += ".*";
        if (filter_sections.length() != 0) {
          regex_filter += filter_sections;
          if (only_collections) {
            regex_filter += regex_collection_names;
          }
          else if (recursive) {
            regex_filter += ".*";
          }
          else {
            regex_filter += regex_collection_names;
            regex_filter += "$";
          }
        }
        if (only_collections) {
          regex_filter += "\\.";
        }
        if (filter_collections.length() != 0) {
          regex_filter += regex_collection_names;
          regex_filter += filter_collections;
          regex_filter += regex_collection_names;
        }
      }
      else {
        // filter may be empty, in which case we search
        //   * everything, if recursive is true or only_sections false
        //   * only one level of section is not recursive and only sections
        //
        // if the regex says we want to be at the start of the string, then
        // remove the caret and direcly append
        // else allow for arbytrary chars before the start of the regex
        if (filter.find("^", 0) == 0) {
          regex_filter += filter.substr(1);
        }
        else {
          if (only_sections && !recursive) {
            regex_filter += regex_section_names;
          }
          else {
            if ( recursive ) {
              regex_filter += ".*";
            } else {
              regex_filter += "[^.]*";
            }
          }
          regex_filter += filter;
        }
        if (only_sections && !recursive) {
          regex_filter += regex_collection_names;
          regex_filter += "$";
        }
      }

      //std::cout << regex_filter << std::endl;

      Array<std::string> names = list_collection_names(regex_filter);
      for (int i = 0; i < names.size(); ++i) {
        names[i] = std::regex_replace(names[i], std::regex("_(collection|section)Info\\."), "", std::regex_constants::format_first_only);
      }
      return names;
    }

    const Array<std::string> list_collection_names(const std::string &filter) const {

      bson_error_t error;
      bson_t *document = bson_new();
      bson_t child, child2;

      bson_append_document_begin(document, "filter", -1, &child);
      bson_append_document_begin(&child, "name", -1, &child2);
      bson_append_utf8(&child2, "$regex", -1, filter.c_str(), -1);
      bson_append_document_end(&child, &child2);
      bson_append_document_end(document, &child);
      bson_append_bool(document, "nameOnly", -1, true);
      bson_append_bool(document, "authorizedCollections", -1, true);

      mongoc_database_t *database;
      database = mongoc_client_get_database(mongoc_client_.get(), "polydb");
      char **names;
      std::vector<std::string> collection_names;
      if ((names = mongoc_database_get_collection_names_with_opts(database, document, &error))) {
        char **names_it = names;
        while (*names_it != nullptr) {
          collection_names.push_back(*(names_it));
          ++names_it;
        }
      }

      bson_destroy(document);
      mongoc_database_destroy(database);
      if (!names)
        throw std::runtime_error(prepare_error_message(error,"collection_names"));
      else
         bson_strfreev(names);

      return Array<std::string>(collection_names.begin(), collection_names.end());
    }

    /**
     * @brief checks if a collection is defined
     * 
     * this checks for the existence of the two roles associated with the collection
     * the collection itself need not yet exist in the database if no documentation and/or no data has been inserted
     * @param collection the name of the collection
     * @return true if the collection exists
     */
    const bool collection_exists(const std::string& collection) const {
      if ( role_exists(collection) ) {
        return true;
      }
      return false;
    }

    /**
     * @brief checks if a section is defined
     * 
     * this actually checks if documentation has been inserted in the _sectionInfo collection
     * this is the only thing that makes a section existing
     * @param section the name of the section
     * @return true if section is defined
     */
    const bool section_exists(const std::string& section) const {
      Array<std::string> names = list_collection_names("_sectionInfo."+section);
      if ( names.size() > 0 ) {
        for ( auto& name : names ) {
          if ( name == section ) {
            return true;
          }
        }
      }
      return false;
    }

    /**
     * @brief checks if the mongo collection of a collections exists, ie if it contains data
     * 
     * a mongo collection only exists if it contains data
     * so the collection as a polydb collection may already exist given its two roles
     * @param collection the name of the collection
     * @return true if the collection exists
     */
    const bool mongo_collection_exists(const std::string& collection) const {

      Array<std::string> collection_names = list_collection_names(collection);
      if ( collection_names.size() > 0 ) {
        for ( auto& name : collection_names ) {
          if ( name == collection ) {
            return true;
          }
        }
      }
      return false;
    }

    /**
    * @brief checks if the current user has a system role 
    * checks for systems role given to the current user
    * this indicates that the user is admin for the database
    * @return true on success
    */
    const bool user_has_system_role() const {

      bson_t *mongo_command = bson_new();

      bson_append_utf8(mongo_command, "usersInfo", -1, username_.c_str(), -1);
      bson_append_bool(mongo_command, "showPrivileges", -1, true);

      mongoc_database_t *database;
      bson_t reply;
      bson_error_t error;
      database = mongoc_client_get_database(mongoc_client_.get(), "admin");
      bool res = mongoc_database_command_simple (database, mongo_command, nullptr, &reply, &error);
      mongoc_database_destroy(database);
      bson_destroy(mongo_command);
      if ( !res ) {
        throw std::runtime_error(prepare_error_message(error,"database_command"));
      }

      bson_iter_t iter;
      bson_iter_t roles_iter;
      uint32_t array_len = 0;
      const uint8_t *array;
      if (bson_iter_init (&iter, &reply) && bson_iter_find_descendant (&iter, "users.0.roles", &roles_iter) &&  BSON_ITER_HOLDS_ARRAY (&iter)) {
        bson_iter_array (&iter, &array_len, &array);
      } else {
        bson_destroy(&reply);
        throw std::runtime_error(prepare_error_message(error,"roles"));
      }

      bson_iter_t role_iter;
      bson_iter_recurse(&roles_iter, &role_iter);
      std::vector<std::string> roles;
      while (bson_iter_next(&role_iter)) {
        bson_iter_t sub_iter;
        bson_iter_recurse(&role_iter, &sub_iter);

        while (bson_iter_next(&sub_iter) ) {
          uint32_t length;
          const char* key = bson_iter_key(&sub_iter);
          if ( strcmp(key,"role") == 0 ) {
            roles.push_back(bson_iter_utf8(&sub_iter,&length));
          }
        }
      }

      bson_destroy(&reply);

      for (auto role : roles) {
        auto found = std::find(std::begin(system_roles), std::end(system_roles), role);

        if (found != std::end(system_roles)) {
          return true;
        }
      }
      return false;
    }

    /**
     * @brief checks is a user is defined in the database
     * 
     * @param username the username
     * @return true if the user exists
     */
    const bool user_exists(const std::string &username) const {

      bson_t *usersInfo_command = bson_new();
      bson_t user;

      bson_append_document_begin(usersInfo_command, "usersInfo", -1, &user);
      bson_append_utf8(&user, "user", -1, username.c_str(), -1);
      bson_append_utf8(&user, "db", -1, auth_db_name_.c_str(), -1);
      bson_append_document_end(usersInfo_command,&user);

      mongoc_database_t *database;
      bson_t reply;
      bson_error_t error;
      database = mongoc_client_get_database(mongoc_client_.get(), "admin");
      bool res = mongoc_database_command_simple (database, usersInfo_command, nullptr, &reply, &error);
      mongoc_database_destroy(database);
      bson_destroy(usersInfo_command);
      if ( !res ) {
        throw std::runtime_error(prepare_error_message(error,"database_command")); 
      }

      bson_iter_t iter;
      bson_iter_t users_iter;
      uint32_t array_len = 0;
      const uint8_t *array;
      if (bson_iter_init (&iter, &reply) && bson_iter_find_descendant (&iter, "users", &users_iter) &&  BSON_ITER_HOLDS_ARRAY (&iter)) {
        bson_iter_array (&iter, &array_len, &array);
      } else {
        bson_destroy(&reply);
        throw std::runtime_error("user_exists: unexpected reply");
      }

      bson_iter_t user_iter;
      bson_iter_recurse(&users_iter, &user_iter);
      std::vector<std::string> users;
      while (bson_iter_next(&user_iter)) {
        bson_iter_t sub_iter;
        bson_iter_recurse(&user_iter, &sub_iter);

        while (bson_iter_next(&sub_iter) ) {
          uint32_t length;
          const char* key = bson_iter_key(&sub_iter);
          if ( strcmp(key,"user") == 0 ) {
            users.push_back(bson_iter_utf8(&sub_iter,&length));
          }
        }
      }

      long length = std::distance(users.begin(), users.end());

      bson_destroy(&reply);
      return length > 0;
    }

    /**
     * @brief checks if a role exists in the database
     * 
     * @param rolename the name of the role
     * @return true if the role exists
     */
    const bool role_exists(const std::string &rolename) const {

      bson_t *rolesInfo_command = bson_new();
      bson_t roles_array;

      bson_append_document_begin(rolesInfo_command, "rolesInfo", -1, &roles_array);
      bson_append_utf8(&roles_array, "role", -1, rolename.c_str(), -1);
      bson_append_utf8(&roles_array, "db", -1, auth_db_name_.c_str(), -1);
      bson_append_document_end(rolesInfo_command,&roles_array);

      mongoc_database_t *database;
      bson_t reply;
      bson_error_t error;
      database = mongoc_client_get_database(mongoc_client_.get(), "admin");
      bool res = mongoc_database_command_simple (database, rolesInfo_command, nullptr, &reply, &error);
      mongoc_database_destroy(database);
      bson_destroy(rolesInfo_command);
      if (!res) {
        throw std::runtime_error(prepare_error_message(error,"database_command")); 
      }

      bson_iter_t iter;
      bson_iter_t roles_iter;
      uint32_t array_len = 0;
      const uint8_t *array;
      if (bson_iter_init (&iter, &reply) && bson_iter_find_descendant (&iter, "roles", &roles_iter) &&  BSON_ITER_HOLDS_ARRAY (&iter)) {
        bson_iter_array (&iter, &array_len, &array);
      } else {
        bson_destroy(&reply);
        throw std::runtime_error(prepare_error_message(error,"roless"));
      }

      bson_iter_t role_iter;
      bson_iter_recurse(&roles_iter, &role_iter);
      std::vector<std::string> roles;
      while (bson_iter_next(&role_iter)) {
        bson_iter_t sub_iter;
        bson_iter_recurse(&role_iter, &sub_iter);

        while (bson_iter_next(&sub_iter) ) {
          uint32_t length;
          const char* key = bson_iter_key(&sub_iter);
          if ( strcmp(key,"role") == 0 ) {
            roles.push_back(bson_iter_utf8(&sub_iter,&length));
          }
        }
      }
      long length = std::distance(roles.begin(), roles.end());
      bson_destroy(&reply);
      return length > 0;
    }

    /**
     * @brief Create the default polymake user and role
     * only needed once per database instance
     * mostly useful for testing
     * @return true on success
     */
    const bool create_default_user_and_role() const {

      if (!role_exists(defaultPolymakeRole)) {
        PolyDBRole role = PolyDBRole(defaultPolymakeRole);
        createRole(role);
      }

      if (!role_exists(changeOwnAccount)) {
        PolyDBRole role = PolyDBRole(changeOwnAccount);
        role.add_privilege("admin", "", {"changeOwnPassword", "changeOwnCustomData"});
        createRole(role);
      }

      if (!user_exists(default_username_)) {
        Array<std::string> a;
        OptionSet options("public_read_access", true, "canChangeOwnAccount", false); 
        create_user(default_username_, default_password_, options);
      }

      return true;
    }

    /**
     * @brief Create a new role 
     * 
     * @param role the role with name and privileges
     * @return true on success
     */
    const bool createRole(const PolyDBRole &role) const {

      if ( role_exists(role.get_name()) ) {
        return false;
      }
      mongoc_database_t *database;
      database = mongoc_client_get_database(mongoc_client_.get(), "admin");

      bson_t reply;
      bson_error_t error;

      bson_t * user_command = bson_new();
      role.get_bson(&user_command);
      //char *str = bson_as_canonical_extended_json(bs_role, nullptr);
      //std::cout << str << std::endl;
      
      auto ret = mongoc_database_command_simple(database, user_command, nullptr, &reply, &error);
      mongoc_database_destroy(database);
      bson_destroy(user_command);
      bson_destroy(&reply);
      if (!ret) {
        throw std::runtime_error(prepare_error_message(error,"database_command"));
      }
      return true;
    }

    /**
     * @brief Create the read and write roles for a new collection
     * 
     * @param collection the name of the collection
     * @return true on success
     */
    bool create_roles_for_collection(const std::string &collection) const {

      if (role_exists(collection)) {
        std::cerr << "role exists" << std::endl;
        return false;
      }

      if (role_exists(collection + ".admin")) {
        std::cerr << "admin role exists" << std::endl;
        return false;
      }

      // create array for actions
      bson_t *user_actions = bson_new();
      bson_t *admin_actions = bson_new();
      int index = 0;

      for (auto role : collection_roles) {
        bson_append_utf8(user_actions, std::to_string(index++).c_str(), -1, role.c_str(), -1);
      }

      index = 0;
      for (auto role : admin_collection_roles) {
        bson_append_utf8(admin_actions, std::to_string(index++).c_str(), -1, role.c_str(), -1);
      }

      // we add section privileges to user_privileges
      index = 0;
      std::string section(collection);
      bson_t *user_privilege_array = bson_new();
      bson_t *admin_privilege_array = bson_new();
      if (section.find('.') != std::string::npos) {

        while (section.find('.') != std::string::npos) {
          section = section.substr(0, section.rfind("."));
          bson_t *privilege = bson_new();
          bson_t *resource = bson_new();
          bson_append_utf8(resource, "db", -1, "polydb", -1);
          bson_append_utf8(resource, "collection", -1, ("_sectionInfo." + section).c_str(), -1);
          bson_append_document(privilege, "resource", -1, resource);
          bson_append_array(privilege, "actions", -1, user_actions);
          bson_append_document(user_privilege_array, std::to_string(index).c_str(), -1, privilege);
          bson_append_document(admin_privilege_array, std::to_string(index++).c_str(), -1, privilege);
          bson_destroy(privilege);
          bson_destroy(resource);
        }
      }

      bson_t *user_privilege_collection_info = bson_new();
      bson_t *user_resource_collection_info = bson_new();
      bson_append_utf8(user_resource_collection_info, "db", -1, "polydb", -1);
      bson_append_utf8(user_resource_collection_info, "collection", -1, ("_collectionInfo." + collection).c_str(), -1);
      bson_append_document(user_privilege_collection_info, "resource", -1, user_resource_collection_info);
      bson_append_array(user_privilege_collection_info, "actions", -1, user_actions);
      bson_append_document(user_privilege_array, std::to_string(index).c_str(), -1, user_privilege_collection_info);
      bson_destroy(user_privilege_collection_info);
      bson_destroy(user_resource_collection_info);

      bson_t *user_privilege_collection = bson_new();
      bson_t *user_resource_collection = bson_new();
      bson_append_utf8(user_resource_collection, "db", -1, "polydb", -1);
      bson_append_utf8(user_resource_collection, "collection", -1, collection.c_str(), -1);
      bson_append_document(user_privilege_collection, "resource", -1, user_resource_collection);
      bson_append_array(user_privilege_collection, "actions", -1, user_actions);
      bson_append_document(user_privilege_array, std::to_string(index+1).c_str(), -1, user_privilege_collection);
      bson_destroy(user_privilege_collection);
      bson_destroy(user_resource_collection);

      bson_t *admin_privilege_collection_info = bson_new();
      bson_t *admin_resource_collection_info = bson_new();
      bson_append_utf8(admin_resource_collection_info, "db", -1, "polydb", -1);
      bson_append_utf8(admin_resource_collection_info, "collection", -1, ("_collectionInfo." + collection).c_str(), -1);
      bson_append_document(admin_privilege_collection_info, "resource", -1, admin_resource_collection_info);
      bson_append_array(admin_privilege_collection_info, "actions", -1, admin_actions);
      bson_append_document(admin_privilege_array, std::to_string(index).c_str(), -1, admin_privilege_collection_info);
      bson_destroy(admin_privilege_collection_info);
      bson_destroy(admin_resource_collection_info);

      bson_t *admin_privilege_collection = bson_new();
      bson_t *admin_resource_collection = bson_new();
      bson_append_utf8(admin_resource_collection, "db", -1, "polydb", -1);
      bson_append_utf8(admin_resource_collection, "collection", -1, collection.c_str(), -1);
      bson_append_document(admin_privilege_collection, "resource", -1, admin_resource_collection);
      bson_append_array(admin_privilege_collection, "actions", -1, admin_actions);
      bson_append_document(admin_privilege_array, std::to_string(index+1).c_str(), -1, admin_privilege_collection);
      bson_destroy(admin_privilege_collection);
      bson_destroy(admin_resource_collection);

      bson_t *user_command = bson_new();
      bson_t *admin_command = bson_new();
      bson_append_utf8(user_command, "createRole", -1, collection.c_str(), -1);
      bson_append_utf8(admin_command, "createRole", -1, (collection + ".admin").c_str(), -1);

      bson_t *roles = bson_new();
      bson_append_array(user_command, "roles", -1, roles);
      bson_append_array(admin_command, "roles", -1, roles);
      bson_append_array(user_command, "privileges", -1, user_privilege_array);
      bson_append_array(admin_command, "privileges", -1, admin_privilege_array);

      bson_destroy(user_privilege_array);
      bson_destroy(admin_privilege_array);
      bson_destroy(user_actions);
      bson_destroy(admin_actions);
      bson_destroy(roles);

      // char * str = bson_as_canonical_extended_json (user_command, nullptr);
      // char * str_admin = bson_as_canonical_extended_json (admin_command, nullptr);
      // std::cout << str << std::endl;
      // std::cout << str_admin << std::endl;

      mongoc_database_t *database;
      database = mongoc_client_get_database(mongoc_client_.get(), "admin");

      bson_t reply;
      bson_error_t error;
      auto ret = mongoc_database_command_simple(database, user_command, nullptr, &reply, &error);
      bson_destroy(user_command);
      bson_destroy(&reply);
      if ( !ret ) {
        bson_destroy(admin_command);
        mongoc_database_destroy(database);
        throw std::runtime_error(prepare_error_message(error,"database_command"));
      }

      ret = mongoc_database_command_simple(database, admin_command, nullptr, &reply, &error);
      bson_destroy(&reply);
      bson_destroy(admin_command);
      mongoc_database_destroy(database);
      if ( !ret ) {
        throw std::runtime_error(prepare_error_message(error,"database_command"));
      }

      return true;
    }

    /**
     * @brief remove a user from polydb
     * 
     * @param username the username
     * @return true on success
     */
    const bool remove_user(const std::string &username) const {

      /* need to do three things:
       * drop the user
       * drop the user role
       */

      bool ret = true;
      
      if ( user_exists(username) ) {
        bson_t *dropUser_command = bson_new();
        bson_t reply;
        bson_error_t error;
  
        mongoc_database_t *database = mongoc_client_get_database(mongoc_client_.get(), "admin");
        bson_append_utf8(dropUser_command, "dropUser", -1, username.c_str(), -1);

        bool res = mongoc_database_command_simple (database, dropUser_command, nullptr, &reply, &error);
        bson_destroy(dropUser_command);
        bson_destroy(&reply);
        mongoc_database_destroy(database);
        if ( !res ) {
          throw std::runtime_error(prepare_error_message(error, "database_command"));
        }
      }

      return ret;
    }

    /**
     * @brief Add a new role to another 
     * needed to add the collection role for reading to the polymake default user role
     * @param role the role (the default polymake role)
     * @param subrole the role to add (the read role of a collection)
     * @return true on success
     */
    const bool add_role_to_role(const std::string &role, const std::string &subrole) const {

      bson_t *addRole_command = bson_new();
      bson_t array;

      bson_append_utf8(addRole_command, "grantRolesToRole", -1, role.c_str(), -1);
      bson_append_array_begin(addRole_command,"roles",-1,&array);
      bson_append_utf8(&array,"0",-1,subrole.c_str(),-1);
      bson_append_array_end(addRole_command,&array);

      mongoc_database_t *database;
      bson_t reply;
      bson_error_t error;
      database = mongoc_client_get_database(mongoc_client_.get(), "admin");

      bool res = mongoc_database_command_simple (database, addRole_command, nullptr, &reply, &error);
      mongoc_database_destroy(database);
      bson_destroy(addRole_command);
      bson_destroy(&reply);
      if (!res) {
        throw std::runtime_error(prepare_error_message(error, "database_command")); 
      }

      return true;
    }

    /**
     * @brief equip a user with a new role
     * needed make a user database admin
     * @param user the username
     * @param role the role (usually the role associated to a collection)
     * @return true on success
     */
    const bool add_role_to_user(const std::string &user, const std::string &role) const {

      bson_t *addRole_command = bson_new();
      bson_t array;

      bson_append_utf8(addRole_command, "grantRolesToUser", -1, user.c_str(), -1);
      bson_append_array_begin(addRole_command,"roles",-1,&array);
      bson_append_utf8(&array,"0",-1,role.c_str(),-1);
      bson_append_array_end(addRole_command,&array);

      mongoc_database_t *database;
      bson_t reply;
      bson_error_t error;
      database = mongoc_client_get_database(mongoc_client_.get(), "admin");

      bool res = mongoc_database_command_simple (database, addRole_command, nullptr, &reply, &error);
      mongoc_database_destroy(database);
      bson_destroy(addRole_command);
      bson_destroy(&reply);
      if (!res) {
        std::string message = "Error with add_role_to_user: ";
        message += error.message;
        throw std::runtime_error(message); 
      }

      return true;
    }

    /**
     * @brief ontain a list of all user names
     * 
     * @return const Array<std::string> the user name
     */
    const Array<std::string> get_user_names() const {

      bson_t *mongo_command = bson_new();

      bson_append_utf8(mongo_command, "usersInfo", -1, "1", -1);
      bson_append_bool(mongo_command, "showCredentials", -1, false);
      bson_append_bool(mongo_command, "showPrivileges", -1, false);
      bson_append_bool(mongo_command, "showAuthenticationRestrictions", -1, false);


      mongoc_database_t *database;
      bson_t reply;
      bson_error_t error;
      database = mongoc_client_get_database(mongoc_client_.get(), "admin");
      bool res = mongoc_database_command_simple (database, mongo_command, nullptr, &reply, &error);
      bson_destroy(mongo_command);
      mongoc_database_destroy(database);
      if ( !res ) {
        bson_destroy(&reply);
        throw std::runtime_error(prepare_error_message(error, "database_command")); 
      }


      bson_iter_t iter;
      bson_iter_t users_iter;
      uint32_t array_len = 0;
      const uint8_t *array;
      if (bson_iter_init (&iter, &reply) && bson_iter_find_descendant (&iter, "users", &users_iter) &&  BSON_ITER_HOLDS_ARRAY (&iter)) {
        bson_iter_array (&iter, &array_len, &array);
      } else {
        bson_destroy(&reply);
        throw std::runtime_error("add_role_to_user: unexpected reply");
      }

      bson_iter_t user_iter;
      bson_iter_recurse(&users_iter, &user_iter);
      std::vector<std::string> users;
      while (bson_iter_next(&user_iter)) {
        bson_iter_t sub_iter;
        bson_iter_recurse(&user_iter, &sub_iter);

        while (bson_iter_next(&sub_iter) ) {
          uint32_t length;
          const char* key = bson_iter_key(&sub_iter);
          if ( strcmp(key,"user") == 0 ) {
            users.push_back(bson_iter_utf8(&sub_iter,&length));
          }
        }
      }

      bson_destroy(&reply);
      return Array<std::string>(users.begin(), users.end());
    }

    /**
     * @brief give a user acces to a private collection or make the user admin on a collection
     * 
     * @param username the username
     * @param collection the name of the collection
     * @param options admin: true if the user should be an admin
     * @return true on success
     */
    const bool add_collection_for_user(const std::string &username, const std::string &collection, const OptionSet options) const {
      /*
       * add collection role to user
       */

      bson_t *mongo_command = bson_new();
      bson_append_utf8(mongo_command, "grantRolesToUser", -1, username.c_str(), -1);
      bson_t array;
      bson_append_array_begin(mongo_command, "roles", -1, &array);
      bson_append_utf8(&array, "0", -1, collection.c_str(), -1);

      if ( options["admin"] ) {
        std::string collection_admin = collection + ".admin";
        bson_append_utf8(&array, "1", -1, collection_admin.c_str(), -1);
      }
      bson_append_array_end(mongo_command, &array);

      //char * str = bson_as_relaxed_extended_json (mongo_command, nullptr);
      //std::cout << "add_user_command: " << str << std::endl;


      mongoc_database_t *database;
      bson_t reply;
      bson_error_t error;
      database = mongoc_client_get_database(mongoc_client_.get(), "admin");
      bool res = mongoc_database_command_simple (database, mongo_command, nullptr, &reply, &error);
      mongoc_database_destroy(database);
      bson_destroy(&reply);
      bson_destroy(mongo_command);
      if ( !res ) {
        throw std::runtime_error(prepare_error_message(error,"add_collection_for_user"));
      }


      return true;
    }


    /**
     * @brief remove a user from a collection
     * this removes both read and write access to a collection, 
     * so the user cannot access the collection anymore if the collection is private
     * consider adding the user with read permissions to the collection in this case
     * @param username the username
     * @param collection the name of the collection
     * @return true on success
     */
    const bool remove_collection_for_user(const std::string &username, const std::string &collection) const {
      /*
       * remove collection role to user
       */

      std::vector<std::string> roles;
      roles.push_back(collection);
      roles.push_back(collection + ".admin");

      bson_t *mongo_command = bson_new();
      bson_append_utf8(mongo_command, "revokeRolesFromUser", -1, username.c_str(), -1);
      bson_t array;

      bson_append_array_begin(mongo_command, "roles", -1, &array);
      bson_append_utf8(&array, "0", -1, collection.c_str(), -1);

      std::string collection_admin = collection + ".admin";
      bson_append_utf8(&array, "1", -1, collection_admin.c_str(), -1);

      bson_append_array_end(mongo_command, &array);

      mongoc_database_t *database;
      bson_t reply;
      bson_error_t error;
      database = mongoc_client_get_database(mongoc_client_.get(), "admin");
      bool res = mongoc_database_command_simple (database, mongo_command, nullptr, &reply, &error);
      mongoc_database_destroy(database);
      bson_destroy(mongo_command);
      bson_destroy(&reply);
      if ( !res ) {
        throw std::runtime_error(prepare_error_message(error,"database_command"));
      }

      return true;
    }

    /**
     * @brief change the passowrd
     * 
     * @param password the new password
     * @return true on success
     */
    const bool change_password(const std::string &password) const {

      bson_t *mongo_command = bson_new();

      bson_append_utf8(mongo_command, "updateUser", -1, username_.c_str(), -1);
      bson_append_utf8(mongo_command, "pwd", -1, password.c_str(), -1);

      bson_t array;
      bson_append_array_begin(mongo_command, "mechanisms", -1, &array);
      bson_append_utf8(&array, "0", -1, "SCRAM-SHA-1", -1);
      bson_append_utf8(&array, "1", -1, "SCRAM-SHA-256", -1);
      bson_append_array_end(mongo_command, &array);

      //char * str = bson_as_canonical_extended_json (mongo_command, nullptr);
      //std::cout << str << std::endl;

      mongoc_database_t *database;
      bson_t reply;
      bson_error_t error;
      database = mongoc_client_get_database(mongoc_client_.get(), "admin");
      bool res = mongoc_database_command_simple (database, mongo_command, nullptr, &reply, &error);
      mongoc_database_destroy(database);
      bson_destroy(mongo_command);
      bson_destroy(&reply);
      if ( !res ) {
        throw std::runtime_error(prepare_error_message(error,"database_command"));
      }

      return true;
    }



    /**
     * @brief Get a list of all users along with their permissions
     * 
     * @param usernames a list of usernames, may be all
     * @return const std::string the list of all users as json string
     */
    const std::string get_users(const pm::Array<std::string>& usernames) const {

      bson_t * mongo_command = bson_new();
      if ( usernames.size() > 0 ) {
        bson_t users;
        bson_append_array_begin(mongo_command, "usersInfo", -1, &users);
        for ( int i = 0; i < usernames.size(); ++i ) {
          bson_t user;
          bson_append_document_begin(&users,std::to_string(i).c_str(),-1,&user);
          bson_append_utf8(&user,"user",-1,usernames[i].c_str(),-1);
          bson_append_utf8(&user,"db",-1,auth_db_name_.c_str(),-1);
          bson_append_document_end(&users,&user);
        }
        bson_append_array_end(mongo_command,&users);
      } else {
        bson_append_int32(mongo_command,"usersInfo",-1,1);
      }

      mongoc_database_t *database;
      bson_t reply;
      bson_error_t error;
      database = mongoc_client_get_database(mongoc_client_.get(), "admin");
      bool res = mongoc_database_command_simple (database, mongo_command, nullptr, &reply, &error);
      bson_destroy(mongo_command);
      mongoc_database_destroy(database);
      if (!res) {
        std::string message = "Error with get_users: ";
        message += error.message;
        bson_destroy(&reply);
        throw std::runtime_error(message); 
      }


      //char * str = bson_as_canonical_extended_json (reply, nullptr);
      //std::cout << str << std::endl;

      std::vector<PolyDBUser> users;

      /* the expected relevant reply is of the form
      {
      "users": [
        {
        "_id": "admin.db_user",
        "user": "db_user",
        "db": "admin",
        "roles": [
          { "role": "changeOwnAccount", "db": "admin" },
          { "role": "readWrite", "db": "admin" },
          { "role": "polymakeUser", "db": "admin" },
          { 'role": "Section.Collection" "db": "admin" }
          { 'role": "Section.AdminCollection.admin" "db": "admin" }
        ]
      }]}
      */
      bson_iter_t iter;
      bson_iter_t users_iter;
      uint32_t array_len = 0;
      const uint8_t *array;
      if (bson_iter_init (&iter, &reply) 
          && bson_iter_find_descendant (&iter, "users", &users_iter) 
          &&  BSON_ITER_HOLDS_ARRAY (&iter)) {
        bson_iter_array (&iter, &array_len, &array);
      } else {
        bson_destroy(&reply);
        throw std::runtime_error("get_users: unexpected reply");
      }

      // we found users, now get an iterator over the elements in the array
      bson_iter_t user_iter;
      bson_iter_recurse(&users_iter, &user_iter);

      // match on the collection names
      const std::regex admin_collection_regex("([\\w_-]+\\.[\\w._-]+)\\.admin$");
      const std::regex collection_regex("[\\w_-]+\\.[\\w._-]+$");
      std::smatch collection_match;

      // iterate over the documents for each user
      while (bson_iter_next(&user_iter)) {
        const uint8_t * user_doc;
        uint32_t doc_length;        
        bson_iter_document(&user_iter,&doc_length,&user_doc);
        bson_t bson_data;
        bson_init_static(&bson_data, user_doc, doc_length);
        bson_iter_t sub_iter;
        bson_iter_init(&sub_iter,&bson_data);

        // we first search for the user field
        // and initialize a PolyDBUser
        bson_iter_find(&sub_iter,"user");
        uint32_t length;
        PolyDBUser user(bson_iter_utf8(&sub_iter,&length));

        // we reinitialize
        // and now search for the roles entry
        bson_iter_init(&sub_iter,&bson_data);
        bson_iter_find(&sub_iter,"roles");
        
        // we recurse on that level
        uint32_t role_array_len = 0;
        const uint8_t *role_array_data;
        bson_iter_array(&sub_iter, &role_array_len, &role_array_data);
        
        bson_t role_array;
        bson_init_static(&role_array, role_array_data, role_array_len);

        bson_iter_t roles_iter;
        bson_iter_init(&roles_iter, &role_array);
        
        while (bson_iter_next(&roles_iter) ) {
          
          const uint8_t * role_doc;
          uint32_t role_doc_length;        
          bson_iter_document(&roles_iter,&role_doc_length,&role_doc);
          bson_t role_data;
          bson_init_static(&role_data, role_doc, role_doc_length);
          bson_iter_t role_iter;
          bson_iter_init(&role_iter,&role_data);
          bson_destroy(&role_data);
          
          bson_iter_find(&role_iter,"role");
          std::string role = bson_iter_utf8(&role_iter,&length);
          if ( role == "changeOwnAccount") {
            user.set_can_change_password(true);
            continue;
          }

          if ( role == "polymakeUser") {
            user.set_has_public_access(true);
            continue;
          }

          if ( std::regex_match(role,collection_match,admin_collection_regex) ) {
            std::string admin_col = collection_match[1].str();
            user.add_admin_collection(admin_col);
            continue;
          }

          if ( std::regex_match(role,collection_regex) ) {
            user.add_collection(role);
            continue;
          }
              
          user.add_admin_role(role);
        }
        for ( auto col : user.get_admin_collections() ) {
          user.remove_collection(col);        
        }
        bson_destroy(&bson_data);      
        bson_destroy(&role_array);
        users.push_back(user);
      }

      bson_t * users_doc = bson_new();
      for ( auto user : users ) {
        bson_t * bson_user = bson_new();
        user.get_bson(&bson_user,false);
        bson_append_document(users_doc,user.get_username().c_str(),-1,bson_user);
        bson_destroy(bson_user);
      }
      bson_destroy(&reply);

      std::string users_ret = to_string_and_free(bson_as_relaxed_extended_json(users_doc, nullptr));
      bson_destroy(users_doc);
      return users_ret;
    }

    /**
     * @brief Set the default values
     * 
     * @param default_section_name the default section name
     * @param default_collection_name the default collection name
     * @param pretty_print_doc whether to print the documentation in a nice format (default should be true, false needed for testing)
     * @param section_color the color for section headings
     * @param collection_color the color for collection headings
     */
    const void set_defaults(const std::string &default_section_name,
                            const std::string &default_collection_name,
                            const bool pretty_print_doc,
                            const std::string &section_color,
                            const std::string &collection_color) const {

      default_section_name_ = default_section_name;
      default_collection_name_ = default_collection_name;
      pretty_print_doc_ = pretty_print_doc;
      section_color_ = section_color;
      collection_color_ = collection_color;
    }

    /**
     * @brief Get the client id
     * 
     * @return const std::string 
     */
    const std::string get_client_id() const {
      return client_id_;
    }
    

  private:
    // the following variables will be set during initialization and cannot be changed

    // corresponds to $db_auth_db
    std::string auth_db_name_;

    // corresponds to $db_name
    std::string db_name_;

    // corresponds tp $db_port
    std::string port_;

    // corresponds to $db_user
    std::string username_;

    // corresponds to $db_default_user
    std::string default_username_;

    // corresponds to $db_default_password
    std::string default_password_;

    // corresponds to $db_host
    std::string host_;

    // corresponds to $db_polydb_version
    std::string polydb_version_;

    // corresponds to $db_socket_timeout
    long socket_timeout_;

    // corresponds to $serverSelectionTimeout
    long serverSelectionTimeout_;

    // corresponds to $useSSL
    bool useSSL_;

    // corresponds to $tlsAllowInvalidHostnames
    bool tlsAllowInvalidHostnames_;

    // corresponds to $tlsAllowInvalidCertificates
    bool tlsAllowInvalidCertificates_;

    // the following variables will be set during initialisation and can be changed

    // FIXME there must be a better solution to allow functions to modify the next variables

    // corresponds to $db_section_name
    mutable std::string default_section_name_;

    // corresponds to $db_collection_name
    mutable std::string default_collection_name_;

    // corresponds to $pretty_print_doc
    mutable bool pretty_print_doc_;

    // corresponds tp $db_collection_color
    mutable std::string collection_color_;

    // corresponds tp $db_section_color
    mutable std::string section_color_;

    // whether the connection to the database is still defined
    mutable bool mongoc_client_defined_;

    // the actual connection to the database
    mutable std::shared_ptr<mongoc_client_t> mongoc_client_;

    // an id used on the connection to the server, 
    // useful to identify connection in the server logs
    std::string client_id_;
  };

}
  }
}
