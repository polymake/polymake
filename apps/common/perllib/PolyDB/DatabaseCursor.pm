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


package PolyDB::DBCursor;

# options:
# db
# collection
# query
# skip
# limit
# sort_by
# local
# username
# password
# host
# the two members used externally are cursor and size.
# The latter is necessery as the count method for cursors into MongoDB is deprecated. Hence we need to check the size of the collection instead
use Polymake::Struct (
   [ new => '$$@' ],
   [ '$query' => '#1' ],
   [ '%options' => '#2' ],
   '$database',
   '$collection',
   '$username',
   '$password',
   '$size',
   '$cursor',
   '$local',
);

sub new {
   my $self          = &_new;
   $self->local      = 0;
   $self->username   = $PolyDB::default::db_user;
   $self->password   = $PolyDB::default::db_pwd;
   $self->database   = defined($self->options->{db}) ? $self->options->{db} : $PolyDB::default::db_database_name;
   $self->collection = defined($self->options->{collection}) ? $self->options->{collection} : $PolyDB::default::db_database_name;

   my $client        = Client::get_client($self->local, $self->username, $self->password);

   if ( !defined($self->options->{sort_by}) ) {
      $self->options->{sort_by} = {"_id" => 1};
   }
   if ( !defined($self->options->{skip}) ) {
      $self->options->{skip} = 0;
   }

   dbg_print("connection established as user ".$self->username."\n") if ($DebugLevel > 0);
   dbg_print("using database ".$self->database." and collection ".$self->collection."\n") if ($DebugLevel > 0);

   my $collection = $client->get_database($self->database)->get_collection($self->collection);

   $self->size   = $collection->count($self->query, {limit=>$self->options->{limit}, skip => $self->options->{skip}});
   $self->cursor = $collection->find($self->query)->sort($self->options->{sort_by})->limit($self->options->{limit})->skip($self->options->{skip});

   $self->cursor->immortal(1);
   $self->cursor->has_next; # this seems to be necessary to circumvent restricted hash problems...

   $self;
}

sub next {
   my ($self) = @_;
   my $p = $self->cursor->next;
   unless ($p) {
      warn_print("no further object in query");
      return;
   }

   return PolymakeJsonConversion::perl2polymake($p, $self->database, $self->collection)
}

sub has_next {
   my ($self) = @_;
   return $self->cursor->has_next;
}

sub at_end {
   my ($self) = @_;
   return !$self->cursor->has_next;
}

# The number of objects matching [[QUERY]] repecting the limit.
# @return Int
sub count {
   my ($self) = @_;
   return $self->size;
};

1


# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
