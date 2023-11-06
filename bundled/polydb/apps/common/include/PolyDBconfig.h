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

#include <string>

#include "polymake/common/PolyDB.h"

namespace polymake {
namespace common {
namespace polydb {


/*
 A list of roles a user can have that allows her/him to access all collections in polyDB
 such roles should only be assigned to db admins, and to the admin user in test cases
 this list is used to check which collections can be accessed in get_allowed_collection_names, 
 as a user with this role not necessarily has a _userInfo collection (e.g. in the test cases)
*/
const std::vector<std::string> system_roles = {"dbOwner", "read", "readWrite", "readAnyDatabase", "readWriteAnyDatabase", "dbAdminAnyDatabase", "root"};

/* 
    a list of roles a normal user should have on a collection
*/
const std::vector<std::string> collection_roles = {"find"};

/*
    a list of roles an admin user should have 
*/
const std::vector<std::string> admin_collection_roles = {"find", "insert" , "update", "remove", "createIndex", "listIndexes"};

/*
    regex to check that a string can be a substring of a section name, . is allowed for subsections
*/
const std::string regex_section_names = "[a-zA-Z0-9_.-]*";

/*
    regex to check that a string can be a substring of a short collection name, no qualified collection names
*/
const std::string regex_collection_names = "[a-zA-Z0-9_-]*";

/*
    the name of the polymake default role
*/
const std::string defaultPolymakeRole = "polymakeUser";

/*
    the name of the role that allows users to modify their own account data (password, custom data)
*/
const std::string changeOwnAccount = "changeOwnAccount";

/*
    the polyDB version
*/
const std::string polyDBVersion = "2.1";

}}}
