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


package PolyDB::Collection;

use Polymake::Struct (
   [ new => ';$$' ],
   '$client',
   '$collection',
   '$cache',
);

sub new {
   my $self=&_new;
   my ($db, $name, $options) = @_;
   $self->client = $db;

   $self->cache = {};

   #$self->collection = $self->client->database->get_collection($name);
   my $client_tmp = $self->client->client;
   $self->collection = $client_tmp->get_collection($name);   

   $self;
}

sub deserialize_safe {
   my ($data) = @_;
   if (defined($data)) {
      Core::Serializer::deserialize($data)
   } else {
      undef
   }
}

sub sanitize_projection {
   my ($projection) = @_;
   my (%result, $is_positive);
   unless (is_hash($projection)) {
      croak( "invalid projection specification: must be a hash map" );
   }
   while (my ($k, $v) = each %$projection) {
      if (is_integer($v)) {
         if ($v == 0) {
            # silently ignore suppressing important metadata properties, also in subobjects
            next if ($k =~ /(?:^|\.)_(?:id|ns|info|type|attrs|ext|load|polyDB)\b/);
         } elsif ($v != 1) {
            croak( "invalid projection specification: allowed values are 0 and 1" );
         }
         if (($is_positive //= $v) != $v) {
            croak( "invalid projection specification: mixed 0's and 1's" );
         }
         $result{$k} = $v;
      } else {
         croak( "invalid projection specification: allowed values are 0 and 1" );
      }
   }
   if ($is_positive) {
      # silently add important metadata properties
      $result{$_} = 1 for qw( _ns _info _type _attrs _ext _load _polyDB );
   }
   keys %result ? \%result : undef
}  

sub list_property_completions {
   my ($self, $text) = @_;
   if (defined(my $schema = $self->get_schema())) {
      # do not propose meta-properties
      $text =~ s/(?:^|\.)\K$/(?:[a-zA-Z]|_id)/;
      $schema->list_property_completions($text);
   } else {
      ()
   }
}

# the current date as a string in the form yyyy-mm-dd
sub get_date {
   my $now = `date '+%Y-%m-%d'`;
   chomp $now;
   return $now;
}

sub make_cursor {
   my ($self, $query, $options) = @_;
   if (defined($options->{projection})) {
      $option->{projection} = sanitize_projection($options->{projection});
   } 
   my $json = JSON->new;

   if ( defined($options->{"sort_by"} ) ) {
      $options->{"sort_by"} = $json->encode($options->{"sort_by"});
   }

   new Cursor($self->collection->find($json->encode($query), $options), $options->{raw} ? undef : \&deserialize_safe);
}

1;
