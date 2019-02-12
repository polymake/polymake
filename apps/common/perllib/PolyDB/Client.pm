#  Copyright (c) 1997-2018
#  Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
#  http://www.polymake.org
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
#   http://solros.de
#   http://www.mathematik.tu-darmstadt.de/~paffenholz
#

package PolyDB::Client;

use Term::ReadKey;

sub check_polydb_version {
   my $version = shift;
   my $min_version = shift;

   my ($min_version_major,$min_version_minor) = $min_version =~ m/^([0-9]+)(?:\.([0-9]*)){0,1}$/;

   my $vminor;
   my $vmajor;
   if ( $version !~ m/^([0-9]+)(?:\.([0-9]*)){0,1}$/ ) {
      croak("not a valid version string\n");
   } else {
      $vmajor = $1;
      $vminor = $2;
   }
   if ( $min_version_major < $vmajor or $min_version_major == $vmajor && $min_version_minor <= $vminor ) {
      return 1;
   }
   return 0;
}

# @category Database
# Takes local and (optionally) username and password and returns a mongo client.
# @param Bool local
# @param String username optional
# @param String password optional
# @return MongoClient
sub get_client {
   my @args = @_;
   my ($local, $u, $p);
   if (@args == 3) {
      ($local, $u, $p) = @args;
   } else {
      my $h = $args[0];
      $local = $h->{local};
      $u = $h->{username};
      $p = $h->{password};
   }

   if ( ($p && !$u) || (!$p && $u) ) {
      croak("if one of user/password is specified in the options then both must be specified\n");
   }

   my $client;
   if ($local) {
      $client = MongoDB::MongoClient->new;
   } elsif (!$u || !$p) {
      $client = MongoDB::MongoClient->new(ssl=>$PolyDB::default::useSSL, host=> $PolyDB::default::db_host.":".$PolyDB::default::db_port, db_name=> $PolyDB::default::db_auth_db, username=>$PolyDB::default::db_user, password=>$PolyDB::default::db_pwd, socket_timeout_ms=>$PolyDB::default::db_socket_timeout);
   } else {
      $client = MongoDB::MongoClient->new(ssl=>$PolyDB::default::useSSL, host=> $PolyDB::default::db_host.":".$PolyDB::default::db_port, db_name=> $PolyDB::default::db_auth_db, username=>$u, password=>$p, socket_timeout_ms=>$PolyDB::default::db_socket_timeout);
   }

   if( !defined($client) ) {
      croak("Failed to open database connection\n");
   }

   return $client;
}

# returns a collection object
sub get_collection {
   my ($client, $db_name, $collection) = @_;
   my $db = $client->get_database($db_name);
   return $db->get_collection($collection);
}

# sets polymake application name for the objects in a collection from the typer information of the collection
# need to consider version of the format of the type information
# no entry "polyDB_version": type information is in the initial format, app is a top level property
# "polyDB_version" is 2.0 or greater: type information is in the new format, app is an entry in the package informations for polymake
sub get_app_from_type_information {
   my ($type_information) = @_;

   if ( !defined($type_information->{'polyDB_version'}) ) {   # initial version of type information, app is recorded on top level
      return $type_information->{'app'};
   } elsif (check_polydb_version($type_information->{'polyDB_version'},"2.0") ) {
      return $type_information->{'package'}->{'polymake'}->{'app'};
   } else {
      croak("cannot determine version of type information");
   }
}

# sets polymake application name for the objects in a collection from the typer information of the collection
# need to consider version of the format of the type information
# no entry "polyDB_version": type information is in the initial format, app is a top level property
# "polyDB_version" is 2.0 or greater: type information is in the new format, app is an entry in the package informations for polymake
sub get_type_from_type_information {
   my ($type_information) = @_;

   if ( !defined($type_information->{'polyDB_version'}) ) {   # initial version of type information, app is recorded on top level
      return $type_information->{'type'};
   } elsif (check_polydb_version($type_information->{'polyDB_version'},"2.0") ) {
      return $type_information->{'package'}->{'polymake'}->{'type'};
   } else {
      croak("cannot determine version of type information");
   }
}

# returns a db handle and a collection handle
# FIXME the client handle must be returned as we need to close it after using the collection handle
# FIXME creation of the client handle should happen in the calling function!
sub get_collection_for_query {
   my ($options) = @_;

   $options->{db} ne "" or croak("database name is missing");
   $options->{collection} ne "" or croak("collection name is missing");

   my $client = $options->{client} // get_client($options);

   # get the actual collection
   my $col  = get_collection($client, $options->{db}, $options->{collection});

   return ($client, $col);
}

### broken/unused functions


# checks if there is a template for the db and checks whether input data adheres to this template
# FIXME we might have more than one template
# FIXME broken function
sub check_type {
   my ($obj, $db, $col, $template) = @_;

   my $col_type = $c->{type};

   unless ($obj->type->isa($template->{app}."::".$col_type) || grep $col_type, {map {$_->name} keys %{$obj->type->auto_casts}}) {
      croak("Type mismatch: Collection $db.$col only takes objects of type $col_type; given object is of type ".$obj->type->full_name."\n");
   }
   return 1;
}

# FIXME currently this function is unused
sub get_credentials {
   # TODO: cache, key chain??
   print "user name: ";
   my $u= <STDIN>;
   ReadMode 2;
   print "password: ";
   my $p= <STDIN>;
   ReadMode 0;
   print "\n";
   chomp($u);
   chomp($p);
   print "Do you want to save these credentials in your custom settings? (This will overwrite any current user and password settings.) [yes/NO]: ";
   my $answer = <STDIN>;
   chomp($answer);
   print "\n";
   if ($answer == "yes") {
      Polymake::User::set_custom $db_user = $u;
      Polymake::User::set_custom $db_pwd = $p;
      print "user settings will be saved at exit\n";
   }
   print "Successfully set user and password for $u.\n";
   return ($u,$p);
}

1;


# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
