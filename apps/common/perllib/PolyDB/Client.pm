#  Copyright (c) 1997-2020
#  Ewgenij Gawrilow, Michael Joswig, and the polymake team
#  Technische Universität Berlin, Germany
#  https://polymake.org
#
#  This program is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by the
#  Free Software Foundation; either version 2, or (at your option) any
#  later version: http://www.gnu.org/licenses/gpl.txt.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#-------------------------------------------------------------------------------
#
#  This file is part of the polymake database interface polyDB.
#
#   @author Silke Horn, Andreas Paffenholz
#   http://www.mathematik.tu-darmstadt.de/~paffenholz
#

package PolyDB;

use MongoDB::Error;
use Try::Tiny;
use Safe::Isa;

sub die_neatly {
   my ($mongo_error, $collection) = @_;
   my $parent = (caller(3))[3];
   if (is_object($collection)) {
      $collection = $collection->full_name;
   }
   my $err = "PolyDB: an error occured in $parent:\n";
   if ( $mongo_error->$_isa("MongoDB::ConnectionError") ) {
      die $err, "PolyDB: Connection to database failed:\n", $mongo_error->message, "\n";
   } elsif ( $mongo_error->$_isa("MongoDB::NetworkError") ) {
      die $err, "PolyDB: Network access failed:\n", $mongo_error->message, "\n";
   } elsif ( $mongo_error->$_isa("MongoDB::AuthError") ) {
      die $err, "PolyDB: ", $mongo_error->message, "\n";
   } elsif ( $mongo_error->$_isa("MongoDB::DuplicateKeyError") ) {
      die $err, "PolyDB: Cannot insert document with the same id:\n", $mongo_error->message, "\n";
   } elsif ( $mongo_error->$_isa("MongoDB::DatabaseError") ) {
      if ($mongo_error->message =~ /^not authorized/ && defined($collection)) {
         die $err, "PolyDB: Missing access permission to collection $collection\n";
      } else {
         die $err, "PolyDB: Error in database operation:\n", $mongo_error->message, "\n";
      }
   } elsif ( $mongo_error->$_isa("MongoDB::SelectionError") ) {
      die $err, "PolyDB: Cannot select writeable server, probably due to failed connection to database:\n", $mongo_error->message, "\n";
   } elsif ( $mongo_error->$_can("message") ) {
      die $err, "PolyDB: an unhandled error occured:\n", $mongo_error->message, "\n";
   } else {
      die $err, $mongo_error;
   }
      
}

package PolyDB::Client;

@ISA = qw( MongoDB::MongoClient );

sub new {
   my ($pkg, $host, $options) = @_;
   if (defined($options->{user})) {
      if ($Shell->interactive) {
         $options->{password} //= $Shell->enter_string("", { prompt => 'password for Mongo DB at '.$options->{host}, secret => true });
      }
      if (!defined($options->{password})) {
         croak( "password is required for Mongo DB user $options->{user}" );
      }
   } elsif (defined($options->{password})) {
      croak( "option 'password' must be given together with option 'user'" );
   } elsif (!defined($host)) {
      $options->{user} = $default::db_user;
      $options->{password} = $default::db_pwd;
   }

   try {
      my $client = new MongoDB::MongoClient(host => $host // $default::db_host,
                                            db_name => $options->{auth_db} // $default::db_auth_db,
                                            username => $options->{user}, password => $options->{password},
                                            ssl => $default::useSSL, socket_timeout_ms => $default::db_socket_timeout,
                                            connect_timeout_ms => $default::db_socket_timeout)
                   // die "Failed to open database connection\n";
      bless($client, $pkg);
   } catch {
      die_neatly($_);
   }
}

# returns a collection object
sub get_collection {
   my ($self, $col_name) = @_;
   $col_name ||= $default::db_collection_name
     or die "no collection name specified and PolyDB::default::db_collection_name not set\n";
   if ($col_name !~ /\./) {
      $col_name = disambiguate_collection_name($self, $col_name);
   }
   my $db = $self->SUPER::get_database($default::db_name);
   try {
      prime Collection($db->get_collection($col_name)
                       // die "Failed to connect to collection $col_name\n");
   } catch {
       die_neatly($_);
   }
}

sub disambiguate_collection_name {
   my ($self, $col_name) = @_;
   my @candidates = get_collection_names($self, undef, { filter => "\\.$col_name\$", recursive => -1 })
     or die "there is no accessible collection with name $col_name\n";
   if (@candidates > 1) {
      if (length($default::db_section_name)) {
         @candidates = grep { /^\Q$default::db_section_name\E\./ } @candidates;
      }
      if (@candidates != 1) {
         die "short collection name $col_name is ambiguous, possible candidates are:\n",
             join(", ", @candidates), "\nPlease specify the full name of the desired collection\n";
      }
   }
   $candidates[0]
}

sub get_collection_names {
   my ($self, $top_section, $options) = @_;
   my $root = $options->{recursive} < 0 ? "_collectionInfo" : "_sectionInfo";
   my $regex = "^$root\\.";
   if (length($top_section)) {
      $regex .= ($top_section =~ s/\./\\./gr) . '\\.';
   }
   if (defined(my $filter = $options->{filter})) {
      $filter =~ s/^\^//
        or
      $filter =~ s/^/[.\\w]*/;
      $regex .= $filter;
   } elsif (!$options->{recursive}) {
      $regex .= '\\w+$';
   }
   try {
      my $db = $self->SUPER::get_database($default::db_name);
      map { $_->{name} =~ s/^$root\.//r }
          $db->list_collections({ name => { '$regex' => $regex } }, { authorizedCollections => true, nameOnly => true })->all;
   } catch {
      die_neatly($_);
   }
}

sub collection_exists {
   my ($self, $collection) = @_;

   try {
      my $db = $self->SUPER::get_database($default::db_name);
      my @cols = $db->list_collections( { name => $collection} )->all;
      scalar(@cols);
   } catch {
      die_neatly($_);
   }
}

sub role_exists {
   my ($self, $rolename) = @_;
      
   my $command = [
      rolesInfo   => {
         role => $rolename,
         db => $PolyDB::default::db_auth_db
      }
   ];

   my $db = $self->SUPER::get_database($default::db_name);
   my $output = $db->run_command($command);

   return scalar(@{$output->{roles}}) > 0;
}

sub add_role_to_role {
   my ($self, $role, $subrole) = @_;

   my $command = [ 
      grantRolesToRole => $role,
      roles => [ $subrole ]
   ];

   my $db = $self->SUPER::get_database($default::db_auth_db);
   $db->run_command($command);

   return $role;
}

sub add_role_for_user { 
   my ($self, $user, $role) = @_;

   my $command = [
      grantRolesToUser   => $user,
      roles => [ $role ]
   ];

   my $db = $self->SUPER::get_database($default::db_auth_db);
   $db->run_command($command);

   return $role;
}

# creates the two necessary roles for a new collection
sub create_roles_for_collection {
   my ($self, $collection) = @_;

   die "user role $collection already exists" if $self->role_exists($collection);
   die "admin role $rolename already exists" if $self->role_exists($collection.".admin");

   my $actions = [ "find" ];
   my $admin_actions = [ "find", "insert" , "update", "remove", "createIndex" ];
   
   my $section_privileges;
   my ($sec,$sub) = $collection =~ /(.*)\.(.*)/;
   while ( $sub ) {
      push @$section_privileges, (
         { 
            resource => { db => $PolyDB::default::db_name, collection => "_sectionInfo".".".$sec }, 
            actions => $actions
         },
      );
      ($sec,$sub) = $sec =~ /(.*)\.(.*)/;
   }

   my $user_privileges;
   push @$user_privileges, (
      { 
         resource => { db => $PolyDB::default::db_name, collection => $collection }, 
         actions => $actions
      },
      { 
         resource => { db => $PolyDB::default::db_name, collection => "_collectionInfo.".$collection }, 
         actions => $actions
      }
   );

   my $admin_privileges;
   push @$admin_privileges, (
      { 
         resource => { db => $PolyDB::default::db_name, collection => $collection }, 
         actions => $admin_actions
      },
      { 
         resource => { db => $PolyDB::default::db_name, collection => "_collectionInfo.".$collection }, 
         actions => $admin_actions
      }
   );

   my $rolename = $collection;
   my $rolename_admin = $collection.".admin";

   my $user_command = [ createRole   => $collection, roles => [], privileges => [ @$section_privileges, @$user_privileges] ];
   my $admin_command = [ createRole   => $collection.".admin", roles => [], privileges => [ @$section_privileges, @$admin_privileges ] ];

   my $auth_db = $self->SUPER::get_database($default::db_auth_db);
   $auth_db->run_command($user_command);
   $auth_db->run_command($admin_command);

   return $collection;
}

# creates the default polymake user and role
# and the custom rule for changing user passwords and custom data
# only needed once for a newly set up database (e.g. testing)
sub create_default_user_and_role {
   my ($self) = @_;

   my $db = $self->SUPER::get_database($default::db_auth_db);
   if ( !$self->role_exists("polymakeUser") ) {
      my $command = [ 
         createRole   => "polymakeUser", 
         privileges => [],
         roles => []
      ];
      $db->run_command($command) or die "Could not create default role\n";
   }

   if ( !$self->role_exists("changeOwnAccount") ) {
      my $command = [
         createRole =>  "changeOwnAccount",
         privileges => [
            {
               resource => { db => "admin", collection => "" },
               actions => [ "changeOwnPassword", "changeOwnCustomData" ]
            }
         ],
         roles => []
      ];
      $db->run_command($command) or die "Could not create role for changing own password\n";
   }

   ## FIXME change once check_user is implemented
   my $output = $db->run_command([ usersInfo => $PolyDB::default::db_user ] );
   if ( scalar(@{$output->{users}}) == 0 ) {
      my $command = [ 
         createUser   => $PolyDB::default::db_user,
         pwd => $PolyDB::default::db_pwd,
         roles => [ "polymakeUser" ]
      ];
      $db->run_command($command) or die "Could not create default user\n";
   }
}

# checks whether a user is already defined
sub user_exists {
   my ($self, $user) = @_;

   my $command = [
         usersInfo   => {
            user => $user,
            db => $PolyDB::default::db_auth_db
         }
   ];
   my $db = $self->SUPER::get_database($default::db_name);
   my $output = $db->run_command($command);
   return scalar(@{$output->{users}}) > 0;
}


1;


# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
