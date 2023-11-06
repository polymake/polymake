#  Copyright (c) 1997-2022
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
#   @author Andreas Paffenholz
#   (c) 2015 - 2023
#   https://polydb.org
#   https://www.mathematik.tu-darmstadt.de/~paffenholz
#

use strict;
use namespaces;

package PolyDB::Client;


use Polymake::Struct (
   [ new => '$' ],
   '$host',
   '$port',
   '$useSSL',
   '$tlsAllowInvalidHostnames',
   '$tlsAllowInvalidCertificates',
   '$timeout',
   '$socket_timeout',
   '$user',
   '$password',
   '$default_user',
   '$default_password',
   '$auth_db',
   '$db_name',
   '$polydb_version',
   '$client',
);


sub new {
   my $self=&_new;
   my ($options) = @_;

   if (defined($options->{user})) {

      # FIXME where is $Shell defined?
      #if ($Shell->interactive) {
      #   $options->{password} //= $Shell->enter_string("", { prompt => 'password for Mongo DB at '.$options->{host}, secret => true });
      #}
      if (!defined($options->{password})) {
         croak( "password is required for Mongo DB user $options->{user}" );
      }
   } elsif (defined($options->{password})) {
      croak( "option 'password' must be given together with option 'user'" );
   } elsif (!defined($options->{host})) {
      $options->{user} = $PolyDB::default::db_user;
      $options->{password} = $PolyDB::default::db_pwd;
   }

   $self->host                        = $options->{host} // $PolyDB::default::db_host;
   $self->port                        = $options->{port} // "$PolyDB::default::db_port";
   $self->useSSL                      = $options->{useSSL} // $PolyDB::default::useSSL;
   $self->tlsAllowInvalidHostnames    = $options->{tlsAllowInvalidHostnames} // $PolyDB::default::tlsAllowInvalidHostnames;
   $self->tlsAllowInvalidCertificates = $options->{tlsAllowInvalidCertificates} // $PolyDB::default::tlsAllowInvalidCertificates;
   $self->timeout                     = $options->{serverSelectionTimeout} // $PolyDB::default::serverSelectionTimeout;
   $self->socket_timeout              = $options->{socket_timeout} // $PolyDB::default::db_socket_timeout;
   $self->user                        = $options->{user} // $PolyDB::default::db_user;
   $self->password                    = $options->{password} // $PolyDB::default::db_pwd;
   $self->default_user                = $options->{default_user} // $PolyDB::default::db_default_user;
   $self->default_password            = $options->{default_password} // $PolyDB::default::db_default_pwd;
   $self->auth_db                     = $options->{auth_db} // $PolyDB::default::db_auth_db;
   $self->db_name                     = $options->{db_name} // $PolyDB::default::db_name;
   $self->polydb_version              = $options->{polydb_version} // "$PolyDB::default::db_polydb_version";

   my $opts = { 
      'tls' => $self->useSSL
   };
   if ( $self->timeout > 0 ) {
      $opts->{'serverSelectionTimeout'} = $self->timeout;
   }

   $self->client = new Polymake::common::PolyDBClient(
                                          $self->host,
                                          $self->port,
                                          $self->useSSL,
                                          $self->tlsAllowInvalidHostnames,
                                          $self->tlsAllowInvalidCertificates,
                                          $self->timeout,
                                          $self->socket_timeout,
                                          $self->user,
                                          $self->password,
                                          $self->default_user,
                                          $self->default_password,
                                          $self->auth_db,
                                          $self->db_name,
                                          $self->polydb_version,
                                          $opts);
   
   die "no client ", $_ if not defined $self->client;

   $self;
}

sub disambiguate_collection_name {
   my ($self, $col_name) = @_;

   my $filter = "\\.$col_name\$";
   if (length($default::db_section_name)) {
      $filter = $default::db_section_name.".*".$filter;
   }
   my @candidates = @{ get_allowed_collection_names($self, { filter => $filter, recursive => false, only_collections=>true }) }
     or die "there is no accessible collection with name $col_name\n";

   if (@candidates > 1) {
      die "short collection name $col_name is ambiguous, possible candidates are:\n",
          join(", ", @candidates), "\nPlease specify the full name of the desired collection\n";
   }
   $candidates[0]
}

sub get_collection {
   my ($self, $col_name) = @_;
   $col_name ||= $PolyDB::default::db_collection_name
     or die "no collection name spqecified and PolyDB::default::db_collection_name not set\n";
   if ($col_name !~ /\./) {
      $col_name = disambiguate_collection_name($self, $col_name);
   }

   new Collection($self,$col_name)
}

sub get_section {
   my ($self, $section_name) = @_;
   $section_name ||= $PolyDB::default::db_section_name
     or die "no collection name spqecified and PolyDB::default::db_section_name not set\n";

   new Section($self,$section_name)
} 


sub get_allowed_collection_names {
   my ($self, $options) = @_;

   my $section            = defined($options->{section})            ? $options->{section}            : "";
   my $filter             = defined($options->{filter})             ? $options->{filter}             : "";
   my $filter_collections = defined($options->{filter_collections}) ? $options->{filter_collections} : "";
   my $filter_sections    = defined($options->{filter_sections})    ? $options->{filter_sections}    : "";
   my $only_collections   = defined($options->{only_collections})   ? $options->{only_collections}   : false;
   my $only_sections      = defined($options->{only_sections})      ? $options->{only_sections}      : false;
   my $recursive          = defined($options->{recursive})          ? $options->{recursive}          : true;

   return $self->client->get_allowed_collection_names($section,$filter,$filter_sections,$filter_collections,$only_sections,$only_collections,$recursive);
}

1;