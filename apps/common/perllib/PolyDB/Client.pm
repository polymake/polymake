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


package PolyDB::Client;
require Exporter;
use vars qw(@ISA @EXPORT);

@ISA = qw(Exporter);
@EXPORT = qw(get_client get_type get_collection get_date generate_id check_type check_options prepare_query);

use Term::ReadKey;


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

	my $client;
	if ($local) {
		$client = MongoDB::MongoClient->new;
	} elsif (!$u || !$p) {
		$client = MongoDB::MongoClient->new(host=>$db_host.":".$db_port, db_name=>$db_auth_db, username=>$db_user, password=>$db_pwd);
	} else {
		$client = MongoDB::MongoClient->new(host=>$db_host.":".$db_port, db_name=>$db_auth_db, username=>$u, password=>$p);
	}
	return $client;
}


# returns the database entry with the type information for a given collection
sub get_type {
	my ($client, $db_name, $collection, $template_key) = @_;
	return $client->get_database($db_name)->get_collection("type_information")->find_one({db => $db_name, col => $collection, template_key => $template_key});
}

# returns a collection object
sub get_collection {
	my ($client, $db_name, $collection) = @_;
	my $db = $client->get_database($db_name);
	return $db->get_collection($collection);
}

# the current date as a string in the form yyyy-mm-dd
sub get_date {
	use DateTime;

	my $dt = DateTime->today;
	return $dt->date;
}


# checks if there is a template for the db and checks whether input data adheres to this template
# FIXME we might have more than one template
sub check_type {
	my ($obj, $db, $col, $template) = @_;
#	my $template = get_type($client, $db, $col);
	
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

sub prepare_query {
	my ($options) = @_;
	
	my $client;
	unless(defined($client = $options->{client})) {
		$client = get_client($options);
	}
	# get one template entry of the collection
	my $type = get_type($client, $options->{db}, $options->{collection});
	# get the actual collection
	my $col  = get_collection($client, $options->{db}, $options->{collection});	

	return $client, $type, $col;
}

sub check_options {
	my $options = shift;
	unless ($options->{db}) { croak("Please specify a database.\n"); }
	unless ($options->{collection}) { croak("Please specify a collection.\n"); }
}

#### currently unused old functions




# generates a unique ID for the object from the name of the file
# FIXME this has a rather restricted usecase
sub generate_id {
	my ($name, $db, $col) = @_;
	
	if ($col eq "SmoothReflexive") {
		if ($name =~ m/\.(\d+D)\.(\d+)/) {
			return "F.$1.$2";
		} else {
			croak("name $name does not yield valid id for collection $col\n");
			return;
		}
	}
	if ($col eq "SmoothReflexive9") {
		if ($name =~ m/\.(\d+D)\.f(\d+)\.(\d+)/) {
			return "F.$1.$2.$3";
		} else {
			croak("name $name does not yield valid id for collection $col\n");
			return;
		}		
	}
	return $name;
	#croak("no rule to generate id for collection $col\n");
	#return;
}


# generates a hash containing local, username and password from a possibly larger one
sub lup {
	my $o = shift;
	my $r = {};
	$r->{local}=$o->{local};
	$r->{username}=$o->{username};
	$r->{password}=$o->{password};
	return $r;
}



1;