# Copyright (c) 2013-2016 Silke Horn, Andreas Paffenholz
# http://solros.de/polymake/poly_db
# http://www.mathematik.tu-darmstadt.de/~paffenholz
# 
# This file is part of the polymake extension polyDB.
# 
# polyDB is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# polyDB is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with polyDB.  If not, see <http://www.gnu.org/licenses/>.


package PolyDB::DBCursor;
use Data::Dumper;

# options:
# db
# collection
# query
# local
# username
# password
# host
# skip
# sort_by
# type
# app
use Polymake::Struct (
   [ new => '$$@' ],
   [ '$query' => '#1' ],
   [ '%options' => '#2' ],
   '$local',
   '$database',
   '$collection',
   '$username',
   '$password',
   '$app',
   '$type',
   '$cursor',
);

sub new {
	my $self=&_new;	
	$self->local = 0;
	$self->username = $db_user;
	$self->password = $db_pwd;
	$self->database = defined($self->options->{db}) ? $self->options->{db} : $db_database_name;
	$self->collection = defined($self->options->{collection}) ? $self->options->{collection} : $db_database_name;

	
	my $client = get_client($self->local, $self->username, $self->password);
	my $template = get_type($client, $self->database, $self->collection);
	my $app = $template->{'app'};
	my $type = $template->{'type'};
	$self->app = $app;
	$self->type = $template;
	
	if ( !defined($self->options->{sort_by}) ) {
		$self->options->{sort_by} = {"_id" => 1};
	}
	if ( !defined($self->options->{skip}) ) {
		$self->options->{skip} = 0;
	}

	dbg_print("connection established as user ".$self->username."\n") if ($DebugLevel > 0);
	dbg_print("using database ".$self->database." and collection ".$self->collection."\n") if ($DebugLevel > 0);
	my $col = $client->get_database($self->database)->get_collection($self->collection);
	$self->cursor = $col->find($self->query)->sort($self->options->{sort_by})->limit($self->options->{limit})->skip($self->options->{skip});
	$self->cursor->immortal(1);
	$self->cursor->has_next; # this seems to be necessary to circumvent restricted hash problems...

	$self;
}

sub next {
	my $self = shift;
	my $p = $self->cursor->next;
	unless ($p) {
		warn_print("no further object in query");
		return;
	}

	return db_data_to_polymake($p, $self->database, $self->collection)
}

sub has_next {
	my $self = shift;
	return $self->cursor->has_next;
}

sub at_end {
	my $self = shift;
	return !$self->cursor->has_next;
}

# The number of objects matching [[QUERY]].
# @return Int
sub full_count {
	my $self = shift;
	return $self->cursor->count;
};

# The number of objects matching [[QUERY]] repecting the limit.
# @return Int
sub count {
	my $self = shift;
	return $self->cursor->count(1);
};

1
