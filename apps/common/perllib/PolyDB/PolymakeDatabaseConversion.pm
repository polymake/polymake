# Copyright (c) 2016- Silke Horn, Andreas Paffenholz
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


package PolyDB::PolymakeDatabaseConversion;
use Scalar::Util qw(looks_like_number);


use Data::Dumper;
use strict;
use Math::BigInt try => 'GMP';
use boolean;

require Cwd;
require Exporter;
use vars qw(@ISA @EXPORT @EXPORT_OK);

@ISA = qw(Exporter);
@EXPORT = qw(polymake_to_array db_data_to_polymake cursor2objectarray cursor2stringarray cursor2array);

my $DEBUG=0;

my $pmns="http://www.math.tu-berlin.de/polymake/#3";
my $simpletype_re = qr{^common::(Int|Integer|Rational|Bool|String|Float)$};
my $builtin_numbertype_re = qr{^common::(Int)$};
my $builtin_booltype_re = qr{^common::(Bool)$};
my $bigint_numbertype_re = qr{^common::(Integer)$};
my $rational_numbertype_re = qr{^common::(Rational)$};
my $float_numbertype_re = qr{^common::(Float)$};
my $unhandled = "this property is still unhandled";

sub type_attr {
	my ($type, $owner)=@_;
	( type => $type->qualified_name(defined($owner) ? $owner->type->application : undef) )
}

# FIXME find better conversion method
# Integer/Rational is converted to int via string to allow for int64
sub try_to_save_as_int {
	my ($type, $qual_name, $val) = @_;
	my $content;
	if ( $qual_name =~ $builtin_numbertype_re ) {
		$content = $val;
	} elsif ( $qual_name =~ $bigint_numbertype_re ) {
		$content = int($type->toString->($val));
	} elsif ( $qual_name =~ $rational_numbertype_re ) {
		$content = int($type->toString->($val));
	} else {
		$content = $type->toString->($val);
	}
	return $content;
}

# FIXME find better conversion method
sub try_to_save_as_bool {
	my ($type, $qual_name, $val) = @_;
	my $content;
	
	if ( $qual_name =~ $builtin_booltype_re ) {
		$content = $val == "true"? boolean::true : boolean::false;
	} else {
		$content = $type->toString->($val);
	}
	return $content;
}

##*************************************************************
##*************************************************************
# we always need to store the number of columns as this is not reconstructible from the 
# data even for dense matrices, if they are empty
# FIXME for reconstruction: a SparseMatrix can have dense rows, so cannot rely on sparse attribute of matrix
sub matrix_toARRAY {
	my ($pv, $projection) = @_;
	my $content = {};
	my $descr=$pv->type->cppoptions->descr;

	if ( @{$pv} ) {
		$content = [];
		foreach (@{$pv}) {
			push @{$content}, handle_cpp_content($_, $projection);
		}
	} 
	return $content;
}


##*************************************************************
##*************************************************************
# FIXME in analogy to matrices a vector could always record its type
# then a vector would always be an object with fields type, dim, sparse, data
sub vector_toARRAY {
	my ($pv, $projection) = @_;
	my $content = [];

	# get type description
	my $descr = $pv->type->cppoptions->descr;
	my $sparse = $descr->
is_sparse_container;
	my $dense = defined($projection) && ref($projection) eq "HASH" && defined($projection->{"dense"});
	my $store_as_int = defined($projection) && ref($projection) eq "HASH" && defined($projection->{"int"});
	my $value;
	if ( $dense ) {
		$value = dense($pv);
	} else {
		$value = $pv;
	}

	# get the type of the elements of the vector
	my $val_type=$descr->value_type;
	my $sub_qual_name= $val_type->qualified_name;

	# check if inner type is builtin or Rational/Integer
	if( $sub_qual_name =~ $simpletype_re ) {
		# check if data is sparse
		# FIXME apparently sparse vectors can only contain simple types?
		if ($sparse && !$dense ) {
			$content = {};
			$content->{'dim'} = $pv->dim;
			$content->{'sparse'} = 1;
			$content->{'data'} = {};
			for (my $it=args::entire($pv); $it; ++$it) {
				if ( $store_as_int ) {
					$content->{'data'}->{$it->index} = try_to_save_as_int($val_type,$sub_qual_name,$it->deref);
				} else {
					$content->{'data'}->{$it->index} = $val_type->toString->($it->deref);
				}
			}
		} else {
			my @pv_copy = map { 
				if ( $store_as_int ) {
					try_to_save_as_int($val_type,$sub_qual_name,$_);
				} else {
					$val_type->toString->($_);
				}
			} @$pv;
			$content = \@pv_copy;
		}
	} else {
		foreach (@{$pv}) {
		push @$content, handle_cpp_content($_,$projection);
		}
	}

	return $content;
}


##*************************************************************
##*************************************************************
sub array_toARRAY {

	my ($pv,$projection) = @_;
	my $content = [];
	my $descr=$pv->type->cppoptions->descr;
	my $val_type=$descr->value_type;
	my $sub_qual_name= $val_type->qualified_name;

	if( $sub_qual_name =~ $simpletype_re ) {
		my @pv_copy = @$pv;
		$content = \@pv_copy;
	} else {
		$content = [];
		foreach (@{$pv}) {
			push @$content, handle_cpp_content($_, $projection);
		}
	}

	return $content;
}


##*************************************************************
##*************************************************************
sub graph_toARRAY {
	
	my ($pv, $projection) = @_;
	my $content = {};
	
	print "[graph_toARRAY] called\n" if $DEBUG;

	if ($pv->has_gaps) {
		print "[graph_toARRAY] graphs with gaps are still unhandled\n" if $DEBUG;
		$content = \$unhandled;
	} else {
		$content = handle_cpp_content(adjacency_matrix($pv), $projection);
	}

	return $content;
}


##*************************************************************
##*************************************************************
sub nodeMap_toARRAY {
	my ($pv,$projection) = @_;
	my $content = [];
	foreach (@$pv) {
		push @$content, handle_cpp_content($_, $projection);
	}
	
	return $content;
}


##*************************************************************
##*************************************************************
sub map_toARRAY {
	my $pv=shift;
	my $content = [];

	my $descr=$pv->type->cppoptions->descr;
	my @kv_type=($descr->key_type, $descr->value_type);

   if ($kv_type[0]->type->qualified_name =~ $simpletype_re ) {
      $content = {};
      foreach (keys %$pv) {
         my ($val,$attr) = value_toARRAY($_,$kv_type[0]);
         $content->{$val} = value_toARRAY($pv->{$_},$kv_type[1]);
      }
   } else {
      foreach (keys %$pv) {
         my $entry = [];
         my ($key) = value_toARRAY($_,$kv_type[0]);
         my ($val) = value_toARRAY($pv->{$_},$kv_type[1]);
         push @$content, [$key,$val];
      }
   }

	foreach (keys %$pv) {
	}
	
	return $content;
}


##*************************************************************
##*************************************************************
sub homology_toARRAY {
    my $pv=shift;
    my $content = [];
    
    my $descr_torsion=$pv->torsion->type->cppoptions->descr;
    my $type_torsion = $descr_torsion->value_type;

    push @$content, handle_cpp_content($pv->torsion);
    push @$content, $pv->betti_number;
    return $content;
}

##*************************************************************
##*************************************************************
sub pair_toARRAY {
    my $pv=shift;
    my $content = [];
	my $attributes = {};
    
    my $descr=$pv->type->cppoptions->descr;
    my $types = $descr->member_types;
    
	my ($val) = value_toARRAY($pv->first,$types->[0]);
    push @$content, $val;
	($val) = value_toARRAY($pv->second,$types->[1]);
    push @$content, $val;
	
    return $content;
}


##*************************************************************
##*************************************************************
sub viaString_toARRAY {
    my $pv=shift;
    my $type = $pv->type;
    my $content = $type->toString->($pv);
    return $content;
}


##*************************************************************
##*************************************************************
# handle C++ types
# this is the most difficult case as 
# C++-types can be arbitrarily nested
sub handle_cpp_content {

	my ($pv, $projection) = @_;
	my $content = {};
	my $qualified_value_name = $pv->type->qualified_name;
	my $descr=$pv->type->cppoptions->descr;

	if ( $DEBUG ) {
		if ($descr->is_container) {		
			print $qualified_value_name, " class is container\n";
			if ($descr->is_assoc_container) {	
				print $qualified_value_name, " class is assoc container\n";
			}
		} elsif ($descr->is_composite) {
			print $qualified_value_name, " class is composite\n";
		} else {
			print $qualified_value_name, " has unknown class structure\n";
		}
	}

	if( $qualified_value_name =~ /^common::(SparseMatrix|Matrix|IncidenceMatrix)/ ) {
		$content = matrix_toARRAY($pv, $projection);

	} elsif( $qualified_value_name =~ /^common::(Array|Set|List)/ ) {
		$content = array_toARRAY($pv, $projection);
	
	} elsif( $qualified_value_name =~ /^common::(SparseVector|Vector)/ ) {
		$content = vector_toARRAY($pv, $projection);
	
	} elsif( $qualified_value_name =~ /^common::Graph/ ) {
		$content = graph_toARRAY($pv, $projection);

	} elsif( $qualified_value_name =~ /^common::NodeMap/ ) {
		$content = nodeMap_toARRAY($pv, $projection);

	} elsif( $qualified_value_name =~ /^common::Map/ ) {
		$content = map_toARRAY($pv);

	} elsif( $qualified_value_name =~ /^common::Pair/ ) {
		$content = pair_toARRAY($pv);

	} elsif( $qualified_value_name =~ /^common::UniPolynomial/ ) {
		$content = viaString_toARRAY($pv);

	} elsif( $qualified_value_name =~ /^common::Polynomial/ ) {
		$content = viaString_toARRAY($pv);

	} elsif( $qualified_value_name =~ /^common::PuiseuxFraction/ ) {
		$content = viaString_toARRAY($pv);

	} elsif( $qualified_value_name =~ /^common::QuadraticExtension/ ) {
		$content = viaString_toARRAY($pv);

	} elsif( $qualified_value_name =~ /^common::TropicalNumber/ ) {
		$content = viaString_toARRAY($pv);
		
	} elsif( $qualified_value_name =~ /^topaz::HomologyGroup/ ) {
		$content = homology_toARRAY($pv);
	} else {
		print $qualified_value_name, "is still unhandled\n" if $DEBUG;
		$content = $qualified_value_name." ".$unhandled;
	}

	return $content;
}


##*************************************************************
##*************************************************************
# this is only a distributor function that calls the 
# correct handler depending on whether the value is a 
# polymake object, builtin type, or C++ type
sub value_toARRAY {

	my ($val, $type, $projection) = @_;
	my $content;

	my $attributes = {};
	$attributes->{"type"} = $type->qualified_name;
	
	if ( $type->qualified_name =~ /^common::(SparseMatrix|Matrix|IncidenceMatrix)/ ) {
		$attributes->{"rows"} = $val->rows;
		$attributes->{"cols"} = $val->cols;
	}
	if ( $type->qualified_name =~ /^common::(SparseVector|Vector)/ ) {
		$attributes->{"dim"} = $val->dim;
	}
	
	if ( $type->qualified_name =~ $simpletype_re ) {
		# check whether we want to store the value as int
		if ( defined($projection) && ref($projection) eq "HASH" ) {
			if ( defined($projection->{"int"}) ) {
				$content = try_to_save_as_int($type,$type->qualified_name,$val);
			} elsif ( defined($projection->{"bool"}) ) {
				$content = try_to_save_as_bool($type,$type->qualified_name,$val);
			} else {
				$content = $type->toString->($val);
			}
		} else {
			$content = $type->toString->($val);
		}
	} else {  # now we are dealing with a C++ type
		$content = handle_cpp_content($val, $projection);
	}

	return ($content,$attributes);
}


##*************************************************************
##*************************************************************
sub subobject_toARRAY {

	my ($pv, $projection) = @_;
	my $main_type=$pv->type->qualified_name;
	my $content = {};
	my $attributes = {};
	$attributes->{'type'} = $main_type;
	
	
	$content->{"type"} = $pv->type->qualified_name;
	if (length($pv->name)) {
		# FIXME here we need to copy the name into a separate variable 
		# before assigning to content
		# otherwise we run into a weird loop if property has no name, 
		# e.g. for $c=cube(3); $c->TRIANGULATION;
		my $name = $pv->name;
		$content->{"name"} = "$name";
	}
	$content->{"tag"} = "object";
	if (length($pv->description)) {
		$content->{"description"} = $pv->description;
	} 
	
	my @credits = ();
	while (my ($product, $credit_string)=each %{$pv->credits}) {
		my %credit =();
		$credit{"credit"} = Polymake::is_object($credit_string) ? $credit_string->toFileString : $credit_string;
		$credit{"product"} = $product;
		push @credits, \%credit;
	}
	$content->{"credits"} = \@credits;

	foreach my $pv (@{$pv->contents}) {
		# skip backreferences for twin objects
		next if ( instanceof Polymake::Core::PropertyValue::BackRefToParent($pv) );

		my $property = $pv->property->name;
		$attributes->{$property} = {};
		print "encoding property $property in subobject\n" if $DEBUG;
		print "defined projection: ", $projection if $DEBUG;
		if ( defined($projection) ) {
			
			next if !defined($projection->{$property}) || $projection->{$property} == 0 ;
			($content->{$property},$attributes->{$property}) = property_toARRAY($pv,$projection->{$property});
		} else {
			($content->{$property},$attributes->{$property}) = property_toARRAY($pv);
		}
	}

	return ($content,$attributes);
}


##*************************************************************
##*************************************************************
sub property_toARRAY {
	my ($pv, $projection) = @_;
	my $type= $pv->property->type;
	my $content;
	my $attributes = {};
	
	# multiple subobjects are stored as an array of objects
	# in this case we know from the start that the value of this 
	# property is a polymake object, hence no need to detect this as
	# will be done in the non-multiple case
	if ($pv->property->flags & $Polymake::Core::Property::is_multiple) {
		$content = [];
		$attributes = [];
		foreach (@{$pv->values}) {
			my ($c,$a) = subobject_toARRAY($_,$projection);
			push @$content, $c;
			push @$attributes, $a;
		}
	} elsif ($pv->property->flags & $Polymake::Core::Property::is_subobject) {
		($content,$attributes) = subobject_toARRAY($pv->value, $projection);
	} else {
		# can be a builtin or a C++ type
		($content,$attributes) = value_toARRAY($pv->value,$type,$projection);
	}

	return ($content,$attributes);
}


##*************************************************************
##*************************************************************
## object: the polymake object
## metadata: 
## id: the id that should be assigned to the object
## options:
##     projection: hash specifying what should be stored in the db
sub polymake_to_array {
    my ($object, $metadata, $id, $options)=@_;

	# create a perl hash that contains the data from the polymake object
	my $polymake_object = {"_id"=>$id};

	# extra structure for types of properties and maybe additional information
	my $attributes = {};
	
	# encode properties of the polytope
	# we run through the top level and handle the rest recursively
	foreach my $pv (@{$object->contents}) {
		
		# advance to next prop if property is non-storable
		# FIXME we might still store it in db, if wanted for database search?
		next if !defined($pv) || $pv->property->flags & $Property::is_non_storable;

		# get the name of the property
		my $property = $pv->property->name;

		# we need a variable to catch the attributes collected during recursion
		my $attr;
		
		# projection is a hash that specifies which properties we actually want to keep in the db json
		# format: 
		# PROPERTY : 1|dense|{ PROPERTY : ... }
		# where subobjects get heir own hash of the same format,
		# 1 just means to store the property (which then has a builtin od C++ type)
		# dense is a special marker for Matrices, Vectors to convert sparse into dense notation
		if ( defined($options->{'projection'}) ) {	
			next if !defined($options->{'projection'}->{$property}) || $options->{'projection'}->{$property} == 0 ;
			($polymake_object->{$property}, $attr) = property_toARRAY($pv,$options->{'projection'}->{$property});
		} else {
			($polymake_object->{$property}, $attr) = property_toARRAY($pv);
		}
		# now deal with the attributes
		# first create an empty hash for them
		$attributes->{$property} = {};
		# add the type
		if ( ref($attr) eq "HASH" ) {
			foreach ( keys %$attr ) {
				$attributes->{$property}->{$_} = $attr->{$_};
			}
		} elsif ( ref($attr) eq "ARRAY" ) {
			$attributes->{$property} = $attr;
		} else {
			$attributes->{$property}->{'type'} = $pv->property->type->qualified_name;
		}
	}
	

	foreach my $at (keys %{$object->attachments}) {
		my $pv = $object->attachments->{$at}->[0];
		if ( is_object($pv) ) {
			my $type= $pv->type;
			if ( defined($options->{'projection'}) ) {
				next if !defined($options->{'projection'}->{"attachments"}->{$at}) || $options->{'projection'}->{"attachments"}->{$at} == 0 ;
				($polymake_object->{$at}) = value_toARRAY($pv,$type,$options->{'projection'}->{"attachments"}->{$at});	
			} else {
				($polymake_object->{$at}) = value_toARRAY($pv,$type);	
			}
		} else {
			if ( defined($options->{'projection'}) ) {
				next if !defined($options->{'projection'}->{"attachments"}->{$at}) || $options->{'projection'}->{"attachments"}->{$at} == 0 ;
			}
			$polymake_object->{$at} = $pv;	
		}
	}
	

	# add the meta properties of the polytope
	# first those also contained in a polymake object
	$polymake_object->{"type"}  = $object->type->qualified_name;
	($polymake_object->{"app"}) = $polymake_object->{"type"} =~ /^(.+?)(?=::)/;
	$polymake_object->{"name"}  = $object->name;

	# description is optional, so check
	if (length($object->description)) { 
		$polymake_object->{"description"} = $object->description;
	} 

	# an object may have multiple credits
	my @credits = ();
	while (my ($product, $credit_string)=each %{$object->credits}) {
	my %credit =();
	$credit{"credit"} = Polymake::is_object($credit_string) ? $credit_string->toFileString : $credit_string;
		$credit{"product"} = $product;
		push @credits, \%credit;
	}
	$polymake_object->{"credits"} = \@credits;	
	
	# FIXME deal with extensions
	
	my $version = $Polymake::Version;
	if ( defined($options->{'version'}) ) {
		$version = $options->{'version'};
	}
	
	# data base specific meta properties are stored in a separate hash
	# will be added as attachment upon reeading
	$metadata->{'attributes'}    = $attributes;
	$metadata->{"tag"}           = "object";
	$metadata->{"creation_date"} = get_date();
	$metadata->{"version"}       = $version;
	$polymake_object->{"polyDB"} = $metadata;

	$object->remove_attachment("polyDB");
	my $xml = save Core::XMLstring($object);
	
	if ( defined($options->{'version'}) ) {
		$xml =~ s/version="(\d+.)+\d+" xmlns/version=\"$version\" xmlns/g;
	}	
	$polymake_object->{"polyDB"}->{'xml'} = $xml;
	
	if ( defined($options->{'modifier'}) ) {
		my $func = eval($options->{'modifier'});
		$func->($polymake_object);
	}
	
	return $polymake_object;
}


sub db_data_to_polymake {
	
	my ($polymake_object, $db_name, $col_name ) = @_;
	
	# take application and type from the document, if defined
	# otherwise use the information from the template
	my $app  = defined($polymake_object->{'app'})  ? $polymake_object->{'app'}  : $t->{'app'};
	my $type = defined($polymake_object->{'type'}) ? $polymake_object->{'type'} : $t->{'type'};

	print Dumper($polymake_object) if $DEBUG;

	my $metadata = $polymake_object->{"polyDB"};
	
	if ($db_name && !defined($metadata->{"database"}) ) {
		$metadata->{"database"} = $db_name;
	}	
	
	if ($col_name && !defined($metadata->{"collection"}) ) {
		$metadata->{"collection"} = $col_name;	
	}
		
   # suppress xml transformation messages
   local $Verbose::files=0;
	# read the polytope from the xml of the db
	my $p = new Core::Object();
	load Core::XMLstring($p,$metadata->{'xml'});	
	delete $metadata->{'xml'};
	delete $metadata->{'attributes'};
		
	# assign a name if it does not have one already
	# first try if one is stored in the db, then use the id
	if ( !defined($p->name) ) {
		if ( defined($polymake_object->{'name'}) ) {
			$p->name = $polymake_object->{'name'};
		} else {
			if ( defined($polymake_object->{'_id'}) ) {
				$p->name = $polymake_object->{'_id'};
			}
		}
	}
	
	my $MD = new Map<String,String>;
	$MD->{"id"} = $polymake_object->{"_id"};
	foreach ( keys %$metadata ) {
		$MD->{$_} = $metadata->{$_};
	}
	$p->attach("polyDB", $MD);
	
	return $p;	
}


# This is a helper function that transforms a database cursor into an array of polymake objects.
sub cursor2objectarray {
	my ($cursor, $t, $db_name, $col_name) = @_;
	
	my @objects = $cursor->all;

	my $app = defined($t) ? $t->{'app'}:$objects[0]->{'app'};
	my $type = defined($t) ? $t->{'type'}:$objects[0]->{'type'};

	my $obj_type = User::application($app)->eval_type($type);
	my $arr_type = User::application($app)->eval_type("Array<$type>");
	
	return $arr_type->construct->(map {db_data_to_polymake($_, $db_name, $col_name)} @objects);
}


# This is a helper function that transforms a database cursor into an array of strings (IDs).
sub cursor2stringarray {
	my $cursor = shift;
	
	my @parray = ();
	while (my $p = $cursor->next) {
		push @parray, $p->{_id};
	}
	return @parray;

}


# This is a helper function that transforms a database cursor into an array of strings (IDs).
sub cursor2array {
	my $cursor = shift;
	
	my @parray = ();
	while (my $p = $cursor->next) {
		push @parray, $p;
	}
	return @parray;

}

1;
