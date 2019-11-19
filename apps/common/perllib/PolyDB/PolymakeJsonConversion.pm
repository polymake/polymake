#  Copyright (c) 1997-2019
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
#   http://solros.de
#   http://www.mathematik.tu-darmstadt.de/~paffenholz
#


package PolyDB::PolymakeJsonConversion;

use strict;
use boolean;
use JSON;


# conversion from polymake format to json
# this is done using the perl module JSON
# this requires the translation of the polymake object into a perl hash
# the actual conversion to json is then a single step by calling json->encode on this hash

my $simpletype_re = qr{^common::(Int|Integer|Rational|Bool|String|Float)$};
my $builtin_numbertype_re = qr{^common::Int$};
my $builtin_booltype_re = qr{^common::Bool$};
my $bigint_numbertype_re = qr{^common::Integer$};
my $rational_numbertype_re = qr{^common::Rational$};
my $float_numbertype_re = qr{^common::Float$};


# store a builtin number type
sub store_builtin {
   my ($value,$type,$handling) = @_;
   my $qualified_name = $type->qualified_name;

   my $content;
   if ( $qualified_name =~ $builtin_numbertype_re || $qualified_name =~ $builtin_booltype_re || defined($handling->{'int'}) || defined($handling->{'boolean'}) ) {
      $content = $value;
   } else {
      $content = $type->toString->($value);
   }
   return $content;
}

sub store_scalar {
   my ($value,$type,$handling) = @_;
   my $qualified_name = $type->qualified_name;

   my $content;
   if ( $qualified_name =~ $bigint_numbertype_re || defined($handling->{'int'}) ) {
      $content = int($type->toString->($value));
   } elsif ( $qualified_name =~ $rational_numbertype_re || defined($handling->{'string'}) ) {
      $content = $type->toString->($value);
   } else {
      $content = $type->toString->($value);
   }
   return $content;
}

sub store_matrix {
   my ($value, $rowtype, $sparse, $handling) = @_;

   my @ret;
   foreach (@{$value}) {
      push @ret, store_container($_,$rowtype,$sparse, $handling);
   }
   return \@ret;
}

sub store_assoc_container {
   my ($content, $type, $save_at_string, $handling) = @_;
   my $qname = $type->qualified_name;
   my $cpp = $type->cppoptions;
   my $builtin = $cpp->builtin;
   my $fields = $cpp->fields;
   my $descr = $cpp->descr;

   my @vals = values %{$content};
   if ( $descr->key_type == "String" || $save_at_string ) {
      my %ret;
      foreach (keys %{$content}) {
         my $key = $descr->key_type->toString->($_);
         my $val_raw = $content->{$_};
         my $val = store_value($val_raw,$descr->value_type,$handling);
         $ret{$key} = $val;
      }
      return \%ret;
   } else {
      my @ret;
      foreach (keys %{$content}) {
         my $key = store_value($_,$descr->key_type,$handling);
         my $val_raw = $content->{$_};
         my $val = store_value($val_raw,$descr->value_type,$handling);
         push @ret, [$key,$val];
      }
      return \@ret;
   }
}

sub store_container {
   my ($value, $ptype, $sparse, $handling) = @_;
   my $pdescr=$ptype->cppoptions->descr;

   my @ret;
   my $type = $pdescr->value_type;

   if ( $pdescr->own_dimension == 2 ) {
      return store_matrix($value,$type,$sparse, $handling);
   } elsif ( $pdescr->own_dimension == 1 ) {
      if ( $sparse && !$handling->{'dense'} ) {
         my @vals;
         for (my $it=args::entire($value); $it; ++$it) {
            push @vals, [$it->index, store_value($it->deref,$type,$handling)];
         }
         push @ret, $value->dim;
         push @ret, @vals;
      } else {
         my $value_temp = defined($handling->{'dense'}) && $sparse ? dense($value) : $value;
         foreach my $val (@{$value_temp}) {
            push @ret, store_value($val,$type,$handling);
         }
      }
   } else {
      croak("containers of dim > 2 should not exist\n");
   }
   return \@ret;
}

sub store_dense_container {
   my ($content) = @_;
   croak("conversion of dense containers is not implemented\n");
}

sub store_sparse_container {
   my ($content) = @_;
   croak("conversion of sparse containers is not implemented\n");
}

sub store_serialized {
   my ($value, $type,$handling) = @_;
   my $serialized_type = $type->cppoptions->descr->serialized_type;
   my $serialized=Core::CPlusPlus::convert_to_serialized($value);
   return store_value($serialized,$serialized_type,$handling);
}

sub store_simple_item {
   my ($value, $type) = @_;
   croak("conversion of simple items not implemented\n");
}

sub store_composite {
   my ($value, $type,$handling) = @_;
   my $fields = $type->cppoptions->fields;
   my @member_types = @{$type->cppoptions->descr->member_types};
   if ( $fields ) {
      if ( scalar @$fields == 2 && $fields->[0] eq "first" && $fields->[1] eq "second" ) {
         my @ret;
         push @ret, store_value($value->first, $member_types[0],$handling);
         push @ret, store_value($value->second, $member_types[1],$handling);
         return \@ret;
      } else {
         my $ret;
         foreach (0..scalar(@$fields)-1) {
            $ret->{$fields->[$_]} = store_value($value->[$_], $member_types[$_],$handling);
         }
         return $ret;
      }
   } else {
      my @ret;
      foreach (0.. scalar(@$value)-1) {
         push @ret, store_value(@{$value}[$_], $member_types[$_],$handling);
      }
      return \@ret;
   }
}

sub store_value {
   my ($value, $type, $handling) = @_;

   # first handle builtin types
   # and scalar types
   if ( $type->cppoptions->builtin ) {
      return store_builtin($value,$type,$handling);
   } elsif ( $type->cppoptions->descr->is_scalar ) {
      return store_scalar($value,$type,$handling);
   } else {
      # now we have a propert C++ container type
      my $descr = $type->cppoptions->descr;
      if ( $descr->is_composite ) {
         return store_composite($value,$type, $handling);
      } elsif ( $descr->is_container ) {
         # now we are left with a container or an associative container
         # we also need to check whether it should be stored in sparse notation

         my $sparse = 0;
         if ( $descr->is_sparse_container ) {
            $sparse = 1;
         }
         if ( $descr->is_assoc_container ) {
            return store_assoc_container($value,$type,1, $handling);
         } else {
            return store_container($value,$type,$sparse, $handling);
         }
      } elsif ( $type->cppoptions->descr->is_serializable ) {

         # the last possible type: properties with a C++ class type that does not fall into one of the container categories
         # matrix, array, set, vector
         # e.g. a graph
         # in its serialization a graph is represented as an adjecency matrix, which is storable as a container
         return store_serialized($value,$type,$handling);
      } else {
         # FIXME bail out if nothing applies to indicate that something needs to be implemented
         croak("unknown cppotions type encountered\n");
      }
   }
}

sub store_multiple_subobject {
   my ($obj_array, $property_mask, $excludes, $keep_all_props) = @_;
   my $ret;
   foreach (@{$obj_array->values}) {
#      if ( !defined($_->name) || $_->name eq "" ) {
#         croak("multiple subobjects must have a name to store them in the database\n");
#      }
      push @$ret, store_subobject($_,$property_mask, $excludes, $keep_all_props);
   }
   return $ret;
}

sub store_subobject_array {
   my ($content, $property_mask, $excludes, $keep_all_props) = @_;
   my $ret;

   foreach (@{$content} ) {
      push @$ret, store_subobject($_,$property_mask, $excludes, $keep_all_props);
   }
   return $ret;
}

# $obj: the object to store
# %options
#   template: a template listing properties to store, must be present
#   excludes: a list of properties NOT to store, even if listed (needed to explude twin properties (e.g. DUAL in app matroid))
sub store_subobject {
   my ($obj, $property_mask, $excludes, $keep_all_props) = @_;

   # get the contents of the object
   my @contents = @{$obj->contents};

   # variable to store result
   my $po = {};
   if ( defined($obj->name) ) {
      $po->{"name"} = $obj->name;
   }

   foreach (@contents) {

      # the name of the property, e.g. FACETS
      my $name = $_->property->name;

      # continue if a template is defined and the property is not listed
      next if ($property_mask) && ( !( $keep_all_props || exists($property_mask->{$name}) ) || (exists($property_mask->{$name}) && $property_mask->{$name} == 0 ) ) || exists($excludes->{$name});

      if ( !defined($_->value) ) {
         $po->{$name} = "undef";
         next;
      }

      # there are two different options to obtain a type:
      # property->type gives us the type as defined in the declaration of the property (e.g. property FACETS: Matrix<Rational>)
      # value->type    gives us the actual type of the value stored for the property
      #                (might e.g. be SparseMatrix<Rational for FACETS, even if property is just Matrix<Rational> )
      #                however, this is not defined for builtin types
      #                one last exception: Text does not have cppoptions, so catch this first
      my $type;
      if ( !(defined($_->property->type->cppoptions)) or $name eq "CHIROTOPE" ) {
         $po->{$name} = $_->value;
         return $po;
      }
      if ( $_->property->type->cppoptions->builtin ) {
         $type = $_->property->type;
      } else {
         $type = $_->value->type;
      }

      # first deal with subobjects of the object (e.g. a TRIANGULATION or a GRAPH)
      if ( $_->property->flags & Core::Property::Flags::is_subobject ) {
         my $options_for_next_level = {};
         my $property_mask_for_next_level;
         my $excludes_for_next_level;
         if ( defined($property_mask) ) {
            $property_mask_for_next_level = $property_mask->{$name};
         }
         # if a property is "twin", then it will contain itself as a property pointing back to the original object (to deal with duality)
         # however, we (a) don't want to descend here, and (b) only a BackRef is stored
         # so we add the proerty to excludes, so it will be jumped over when we descend into the object
         if ( $_->property->flags & Core::Property::Flags::is_twin ) {
            $excludes_for_next_level->{$name} = 1;
            $po->{$name} = store_subobject($_, $property_mask_for_next_level, $excludes_for_next_level, $keep_all_props);
         } else {
            # a property can have the attribute multiple
            # in this case more than one instance of the property can exist in the object
            # the type is necessarily an object, and it has a (maybe generated) name
            # we store as hash name => object
            if ( $_->property->flags & Core::Property::Flags::is_multiple ) {
               $po->{$name}=store_multiple_subobject($_,$property_mask_for_next_level, $excludes_for_next_level, $keep_all_props);
            } else {
               # now we have a simple subobject
               $po->{$name} = store_subobject($_,$property_mask_for_next_level, $excludes_for_next_level, $keep_all_props);
            }
         }
      } elsif ( $_->property->flags & Core::Property::Flags::is_subobject_array ) {
         # FIXME I currently don't know of an subobject array (Array<Polytope> ?)
         my $property_mask_for_next_level;
         my $excludes_for_next_level;
         if ( defined($property_mask) ) {
            my $property_mask_for_next_level = $property_mask->{$name};
         }
         $po->{$name} = store_subobject_array($_->value,$property_mask_for_next_level, $excludes_for_next_level, $keep_all_props);
      } else {
         my $handling = undef;
         if ( defined($property_mask) && ref $property_mask->{$name} eq "HASH" ) {
            $handling = $property_mask->{$name};
         }
         $po->{$name} = store_value($_->value,$type,$handling);
      }
   }
   return $po;
}

sub store_attachments {
   my ($obj, $property_mask ) = @_;

   my $po;

   foreach my $at (keys %{$obj->attachments}) {
      next if defined($property_mask) && (!exists($property_mask->{$at}) || $property_mask->{$at} == 0 );
      my $pv = $obj->attachments->{$at}->[0];
      my $handling = undef;
      if ( defined($property_mask) && ref $property_mask->{$at} eq "HASH" ) {
         $handling = $property_mask->{$at};
      }
      if ( is_object($pv) ) {
         ($po->{$at}) = store_value($pv,$pv->type,$handling);
      } else {
         ($po->{$at}) = $pv;
      }
   }
   return $po;
}

sub data2json {
   my ($pv, $options) = @_;
   my $po;
   if ( is_object($pv) ) {
      if ( instanceof Core::Object($pv) ) {
         $po = store_subobject($pv,$options->{'property_mask'});
      } else {
         ($po) = store_value($pv,$pv->type,$options->{'handling'});
      }
   } else {
      ($po) = $pv;
   }

   return $po;
}

# $obj: The object to store
# %options:
#    property_mask: a property mask for the file
sub polymake2perl {
   my ($obj, $options) = @_;

   my $po = {};

   if (!is_object($obj)) {
      croak("only complex data types can be stored in the database");
   }

   if(!instanceof Core::Object($obj)) {
      croak("currently only polymake objects can be stored in the database");
   }

   my $property_mask = defined($options->{'property_mask'}) ? $options->{'property_mask'} : undef;
   my $json_modifier = defined($options->{'json_modifier'}) ? $options->{'json_modifier'} : undef;
   my $excludes;

   $po = store_subobject($obj, $property_mask, $excludes, $options->{"keep_all_props"});
   my $attachments = store_attachments($obj, $property_mask->{"attachments"});
   foreach (keys %$attachments) {
      if ( exists($po->{$_}) ) {
         croak("names of attachments may not coincide with property names\n");
      }
      $po->{$_} = $attachments->{$_};
   }

   # description is optional, so check
   if (length($obj->description)) {
      $po->{"description"} = $obj->description;
   }

   # an object may have multiple credits
   my @credits = ();
   while (my ($product, $credit_string)=each %{$obj->credits}) {
   my %credit =();
   $credit{"credit"} = Polymake::is_object($credit_string) ? $credit_string->toFileString : $credit_string;
      $credit{"product"} = $product;
      push @credits, \%credit;
   }
   $po->{"credits"} = \@credits;

   # FIXME deal with extensions

   if ( defined($json_modifier) ) {
      my $func = eval($json_modifier);
      $func->($po);
   }

   return $po;
}

sub polymake2json {
   my ($obj, $options) = @_;
   my $po = polymake2perl($obj,$options);

   my $json = JSON->new->allow_nonref;
   $json = $json->convert_blessed(1);
   my $pretty_printed = $json->pretty->encode($po);
   return $pretty_printed;
}

sub perl2polymake {

   my ($polymake_object, $db_name, $col_name, $construct_object_function ) = @_;

   my $metadata = do {
      if ( $polymake_object->{"polyDB"}->{"package"} ) {
          $polymake_object->{"polyDB"}->{"package"}->{"polymake"};
       } else {
          $polymake_object->{"polyDB"};
       }
    };

   if ($db_name && !defined($metadata->{"database"}) ) {
      $metadata->{"database"} = $db_name;
   }

   if ($col_name && !defined($metadata->{"collection"}) ) {
      $metadata->{"collection"} = $col_name;
   }

   my $p = new Core::Object();
   if ( defined($construct_object_function) ) {
      my $func = eval($construct_object_function);
      $p = $func->($polymake_object);
   } else {
      # suppress xml transformation messages
      local $Verbose::files=0;
      # read the polytope from the xml of the db
      load Core::XMLstring($p,$metadata->{'xml'});
      delete $metadata->{'xml'};
   }

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

# This is a helper function that transforms a database cursor into an array of strings (IDs).
sub cursor2IdArray {
   my ($cursor) = @_;

   my @parray;
   while (my $p = $cursor->next) {
      push @parray, $p->{_id};
   }
   return @parray;
}

# This is a helper function that transforms a database cursor into an array of polymake objects.
sub cursor2ObjectArray {
   my ($cursor, $db_name, $col_name, $construct_object_function) = @_;

   my @objects = $cursor->all;

   my ($app, $type);
   if ( defined($app=$objects[0]->{app}) ) { # old version of polyDB
      $type = $objects[0]->{type};
   } else {
      $app  = $objects[0]->{polyDB}->{package}->{polymake}->{app};
      $type = $objects[0]->{polyDB}->{package}->{polymake}->{type};
   }

   my $obj_type = User::application($app)->eval_type($type);

   return new Array<$obj_type>(map {perl2polymake($_, $db_name, $col_name, $construct_object_function->{$_->{'polyDB'}->{'type_information_key'}})} @objects);
}

sub cursor2array {
  my ($cursor) = @_;

  my @parray;
  while (my $p = $cursor->next) {
    push @parray, $p;
  }
  return @parray;
}

1;

# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
