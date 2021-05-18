#  Copyright (c) 1997-2021
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

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);
use feature 'state';

package Polymake::Core::Serializer;

# [ version ] =>
sub default_ns {
   return { polymake => [ $mainURL, $_[0] // $Version ] }
}

# private:
sub strip_prefix {
   if (defined($_[1])) {
      $_[0] =~ s/^\Q$_[1]\E:(?=\w)//
   } else {
      $_[0] =~ /^$qual_id_re(?:<|$)/
   }
}

#############################################################################################

use constant noSerialization => "noSerialization\n";
use constant undecoded_tag => "==UNDECODED==";

declare $reject_unknown_properties = false;

sub complain : method {
   die "don't know how to serialize ", $_[0]->full_name, "\n";
}

sub prime {
   my ($proto) = @_;
   $proto->serialize = \&set_methods_and_serialize;
   $proto->JSONschema = \&set_methods_and_get_schema;
}

sub set_methods_and_serialize : method {
   my $proto = &set_methods;
   $proto->serialize->(@_);
}

sub set_methods_and_get_schema : method {
   my $proto = &set_methods;
   $proto->JSONschema->(@_);
}

sub set_methods {
   my $proto = shift;
   eval { generate_methods($proto, $proto->cppoptions->descr) };
   if ($@) {
      if ($@ eq noSerialization) {
         $proto->serialize = \&complain;
         $proto->JSONschema = \&complain;
      } else {
         die $@;
      }
   }
   $proto
}

sub generate_methods {
   my ($proto, $type_descr)=@_;
   defined($proto) && defined($type_descr) or die noSerialization;

   if ($proto->serialize && $proto->serialize != \&set_methods_and_serialize
       && $proto->cppoptions->descr == $type_descr) {
      # reuse already generated methods
      return ($proto->serialize, inject_type_schema($proto));
   }
   my ($serialize, $get_schema) = do {
      if ($type_descr->is_container) {
         if ($type_descr->is_assoc_container) {
            generate_methods_for_assoc_container($type_descr);
         } elsif ((my $dim = $type_descr->own_dimension) == 2) {
            generate_methods_for_matrix($type_descr);
         } elsif ($dim == 1) {
            if ($type_descr->is_sparse_container) {
               generate_methods_for_sparse_container($type_descr);
            } else {
               generate_methods_for_dense_container($type_descr);
            }
         } else {
            die noSerialization;
         }
      } elsif ($type_descr->is_composite) {
         generate_methods_for_composite($proto, $type_descr);
      } elsif ($type_descr->is_serializable) {
         generate_methods_for_serializable($type_descr);
      } elsif ($proto->cppoptions->builtin) {
         return (undef, inject_type_schema($proto));
      } else {
         die noSerialization;
      }
   };
   if ($proto->cppoptions->descr == $type_descr) {
      # cache the methods for later use
      $proto->serialize = $serialize;
      $proto->JSONschema = $get_schema;
      wantarray ? ($serialize, inject_type_schema($proto)) : ()
   } else {
      ($serialize, $get_schema)
   }
}
#############################################################################################
sub subschema_name {
   my ($proto, $rec) = @_;
   (!$rec && $proto->application->name . "-") . join("-", $proto->name, defined($proto->params) ? map { subschema_name($_, true) } @{$proto->params} : ())
}

sub inject_type_schema {
   my ($proto) = @_;
   my $ref = '#/definitions/' . subschema_name($proto);
   sub {
      @_ && $_[0]->collect($proto);
      return { '$ref' => $ref };
   }
}
#############################################################################################
sub pretty_format {
   my ($data, $multiline, $options) = @_;
   if ($options->{pretty}) {
      JSON::XS::set_multiline_flag($data, $multiline);
   }
   $data
}
#############################################################################################
sub serialize_sparse_sequence {
   my ($value_serialize, $multiline) = @_;
   sub {
      my ($obj, $options) = @_;
      my $dim = scalar @$obj;
      local $options->{names} = false;
      my $suppress_dim = is_defined_and_false(delete local $options->{dim});
      if ($obj->size == $dim) {
         pretty_format($value_serialize ? [ map { $value_serialize->($_, $options) } @$obj ] : [ @$obj ], $multiline, $options);
      } else {
         my @values;
         for (my $it = args::entire($obj); $it; ++$it) {
            push @values, $it->index, defined($value_serialize) ? $value_serialize->($$it, $options) : $$it;
         }
         unless ($suppress_dim) {
            push @values, Sparse::dim_key(), $dim;
         }
         my %result;
         tie %result, "Polymake::Core::Serializer::Sparse", \@values;
         pretty_format(\%result, $multiline, $options);
      }
   }
}

sub sparse_sequence_schema {
   my ($value_get_schema) = @_;
   sub {
      return { type => ["array", "object"],
               items => &$value_get_schema,
               properties => { _dim => { type => 'integer', minimum => 0 } },
               patternProperties => { '^\d+$' => &$value_get_schema },
               additionalProperties => false };
   }
}
#############################################################################################
sub generate_methods_for_sparse_container {
   my ($type_descr) = @_;
   my $value_proto = $type_descr->value_type;
   my $value_descr = $type_descr->value_descr;
   my ($value_serialize, $value_get_schema) = generate_methods($value_proto, $value_descr);
   ( serialize_sparse_sequence($value_serialize, $value_descr->dimension > 0),
     sparse_sequence_schema($value_get_schema) )
}
#############################################################################################
sub generate_methods_for_dense_container {
   my ($type_descr) = @_;
   my $value_proto = $type_descr->value_type;
   my $value_descr = $type_descr->value_descr;
   my ($value_serialize, $value_get_schema) = generate_methods($value_proto, $value_descr);
   my $multiline = $type_descr->dimension > 1;
   ( $value_serialize
     ? sub {
          my ($obj, $options) = @_;
          local $options->{names} = false;
          delete local $options->{dim};
          pretty_format([ map { $value_serialize->($_, $options) } @$obj ], $multiline, $options)
       }
     : sub {
          my ($obj, $options) = @_;
          pretty_format([ @$obj ], $multiline, $options)
       },

     $value_serialize || !$type_descr->is_set
     ? sub {
          return { type => "array", items => &$value_get_schema };
       }
     : sub {
          return { type => "array", items => &$value_get_schema, uniqueItems => true };
       } )
}
#############################################################################################
sub generate_methods_for_matrix {
   my ($type_descr) = @_;
   my $row_proto = $type_descr->value_type;
   my $row_descr = $type_descr->value_descr;
   my $may_have_gaps = $type_descr->is_sparse_serialized;
   my ($row_serialize, $row_get_schema) = generate_methods($row_proto, $row_descr);

   if ($may_have_gaps) {
      my $serialize_rows = serialize_sparse_sequence($row_serialize, true);
      ( sub {
           my ($obj, $options) = @_;
           local $options->{names} = false;
           $serialize_rows->(args::rows($obj), $options)
        },
        sparse_sequence_schema($row_get_schema)
      )
   } else {
      my $store_cols = $row_descr->is_sparse_container || $row_descr->is_set;
      my $suppress_dim = $row_descr->is_sparse_container;
      ( sub {
           my ($obj, $options) = @_;
           local $options->{names} = false;
           local $options->{dim} = false if $suppress_dim;
           my @result = map { $row_serialize->($_, $options) } @$obj;
           if ($store_cols || !@result) {
              push @result, pretty_format({ cols => $obj->cols }, false, $options);
           }
           \@result
        },
        sub {
           return { type => "array",
                    items => { oneOf => [ &$row_get_schema,
                      { type => "object",
                        properties => { cols => { type => 'integer', minimum => 0 } },
                        required => [ "cols" ],
                        additionalProperties => false } ] } };
        } )
   }
}
#############################################################################################
sub generate_methods_for_assoc_container {
   my ($type_descr) = @_;
   my $key_proto = $type_descr->key_type;
   my $key_descr = $type_descr->key_descr;
   my $value_proto = $type_descr->value_type;
   my $value_descr = $type_descr->value_descr;
   my ($key_serialize, $key_get_schema) = generate_methods($key_proto, $key_descr);
   my ($value_serialize, $value_get_schema) = generate_methods($value_proto, $value_descr);
   my $key_schema;

   if ($key_serialize or ($key_schema = $key_proto->JSONschema->(), keys(%{$key_schema}) != 1 || !exists $key_schema->{type})) {
      # complex keys: serialize as a list of key-value pairs
      ( sub {
           my ($obj, $options) = @_;
           local $options->{names} = false;
           my $multiline_pair = $key_descr->dimension > 1 || $value_descr->dimension > 1;
           my @result;
           while (my ($k, $v) = each %$obj) {
              push @result, pretty_format([ $key_serialize->($k, $options), defined($value_serialize) ? $value_serialize->($v, $options) : $v ],
                                          $multiline_pair, $options);
           }
           \@result
        },
        sub {
           return { type => "array",
                    items => {
                      type => "array",
                      items => [ &$key_get_schema, &$value_get_schema ],
                      minItems => 2, additionalItems => false } };
        } )
   } else {
      # simple numeric or string keys
      my $multiline = $value_descr->dimension > 0;
      my $serialize = $type_descr->is_ordered
         # don't convert to a plain hash, rather preserve the order
         ? sub {
              my ($obj, $options) = @_;
              local $options->{names} = false;
              my @values;
              while (my ($k, $v) = each %$obj) {
                 push @values, $k, defined($value_serialize) ? $value_serialize->($v, $options) : $v;
              }
              my %result;
              tie %result, "Polymake::Core::Serializer::Sparse", \@values;
              pretty_format(\%result, $multiline, $options);
           } :
         $value_serialize
         ? sub {
              my ($obj, $options) = @_;
              local $options->{names} = false;
              my %result;
              while (my ($k, $v) = each %$obj) {
                 $result{$k} = $value_serialize->($v, $options);
              }
              pretty_format(\%result, $multiline, $options);
           }
         : sub {
              my ($obj, $options) = @_;
              pretty_format({ %$obj }, $multiline, $options);
           };

      my $get_schema =
         $key_schema->{type} eq "integer"
         ? sub {
              return { type => "object",
                       patternProperties => { '^-?\d+$' => &$value_get_schema },
                       additionalProperties => false };
           }
         : sub {
             return { type => "object",
                      additionalProperties => &$value_get_schema };
           };

      ($serialize, $get_schema)
   }
}
#############################################################################################
sub generate_methods_for_composite {
   my ($proto, $type_descr) = @_;
   my ($member_types, $member_descrs) = ($type_descr->member_types, $type_descr->member_descrs);
   defined($member_types) && defined($member_descrs) or die noSerialization;

   my (@member_serialize, @member_get_schema);
   my $i = 0;
   my $multiline_list = false;
   my $multiline_object = false;
   foreach my $member_proto (@$member_types) {
      my ($member_serialize, $member_get_schema) = generate_methods($member_proto, $member_descrs->[$i]);
      push @member_serialize, $member_serialize;
      push @member_get_schema, $member_get_schema;
      my $member_dim = $member_descrs->[$i]->dimension;
      $multiline_list ||= $member_dim > 1;
      $multiline_object ||= $member_dim > 0;
   } continue { ++$i }

   my ($serialize_to_list, $serialized_list_get_schema) =
   ( sub {
        my ($obj, $options) = @_;
        local $options->{names} = false;
        my $i = 0;
        pretty_format([ map { my $method = $member_serialize[$i++]; defined($method) ? $method->($_, $options) : $_ } @$obj ],
                      $multiline_list, $options)
     },
     sub {
        return { type => "array",
                 items => [ map { &$_ } @member_get_schema ],
                 minItems => scalar(@member_get_schema),
                 additionalItems => false };
     } );
   if (defined(my $member_names = $type_descr->member_names)) {
      ( sub {
           my ($obj, $options) = @_;
           unless (is_defined_and_false($options->{names})) {
              my $i = 0;
              return pretty_format({ map { my $method = $member_serialize[$i];
                                           ($member_names->[$i++] => (defined($method) ? $method->($_, $options) : $_)) } @$obj },
                                   $multiline_object, $options);
           }
           &$serialize_to_list
        },
        sub {
           my $schema = &$serialized_list_get_schema;
           $schema->{type} = ["array", "object"];
           my $i = 0;
           $schema->{properties}->{$member_names->[$i++]} = $_ for @{$schema->{items}};
           $schema->{additionalProperties} = false;
           return $schema;
        } )
   } else {
      ($serialize_to_list, $serialized_list_get_schema)
   }
}
#############################################################################################
sub generate_methods_for_serializable {
   my ($type_descr) = @_;
   my ($serialized_proto, $serialized_descr) = ($type_descr->serialized_type, $type_descr->serialized_descr);
   my ($serialize, $get_schema) = generate_methods($serialized_proto, $serialized_descr);
   ( sub {
        my ($obj, $options) = @_;
        $serialize->(CPlusPlus::convert_to_serialized($obj), $options)
     },
     $get_schema
   )
}
#############################################################################################
sub add_big_object_type {
   my ($properties, $multi_type, $proto, $final_proto, $type_collector) = @_;
   while (my ($name, $prop) = each %{$proto->properties}) {
      next if $prop->belongs_to != $proto;
      if (defined($final_proto)) {
         $prop = $final_proto->property($name);  # pick the instance with correct type
      }
      next if $prop->name ne $name;
      if (defined($prop->overrides)) {
         $properties->{$prop->overrides} = false;
      }
      if ($prop->flags & (Property::Flags::is_non_storable | Property::Flags::is_permutation)) {
         $properties->{$name} = false;
         next;
      }

      if ($prop->flags & (Property::Flags::is_subobject | Property::Flags::is_subobject_array)) {
         my $schema = inject_type_schema($prop->subobject_type)->($type_collector);
         if ($prop->flags & (Property::Flags::is_multiple | Property::Flags::is_subobject_array)) {
            $schema = { type => "array", items => $schema };
         }
         $properties->{$name} = $schema;

      } else {
         my $default_type = { anyOf => [ { type => "null" }, inject_type_schema($prop->type)->($type_collector) ] };
         if (instanceof PropertyParamedType($prop->type)
               and
             my @derived = grep { defined } map { $prop->type->coherent_type->($_) } @{$prop->type->generic->derived_abstract}) {

            # multiple type property, make the schema conditional on _attrs.NAME._type
            push @$multi_type, { anyOf => [
                       { properties => { _attrs => { properties => { $name => { properties => { _type => false }}}},
                                         $name => $default_type } },
                 map { { properties => { _attrs => { properties => { $name => { properties => { _type => { const => $_->qualified_name($proto->application) }}}}},
                                         $name => inject_type_schema($_)->($type_collector) } }
                 } @derived ] };
         } else {
            # single type property
            $properties->{$name} = $default_type;
         }
      }
   }
}

sub big_object_schema : method {
   my ($proto, $type_collector) = @_;
   my (%properties, @multi_type_properties, @all, %skip);
   add_big_object_type(\%properties, \@multi_type_properties, $proto, undef, $type_collector);
   foreach my $super (@{$proto->linear_isa}) {
      next if $skip{$super};
      if (ref($super) eq "Polymake::Core::BigObjectType" && !defined($super->abstract)) {
         # top-level concrete object type
         push @all, inject_type_schema($super)->($type_collector);
         $skip{$_} = true for @{$super->linear_isa};
      } else {
         add_big_object_type(\%properties, \@multi_type_properties, $super, $proto, $type_collector);
      }
   }
   push @all, { '$ref' => "$Schema::mainSchemaURI/data.json#/definitions/big_obj" } if !@all;
   push @all, { properties => \%properties } if keys %properties;
   push @all, @multi_type_properties;
   return { allOf => \@all };
}

sub grep_distinct_schema {
   my ($schemas, $new_schema) = @_;
   foreach (@$schemas) {
      return () if same_schema($_, $new_schema);
   }
   $new_schema;
}

sub same_schema {
   my ($s1, $s2) = @_;
   if (keys(%$s1) == keys(%$s2) && keys(%{$s1->{properties}}) == keys(%{$s2->{properties}})) {
      my $result = true;
      while (my ($prop_name, $prop_schema1) = each %{$s1->properties}) {
         if (defined(my $prop_schema2 = $s2->{properties}->{$prop_name})) {
            unless (exists $prop_schema1->{'$ref'}
                    ? $prop_schema1->{'$ref'} eq $prop_schema2->{'$ref'} :
                    exists $prop_schema1->{items}
                    ? same_multi_schema($prop_schema1->{items}, $prop_schema2->{items})
                    : same_schema($prop_schema1, $prop_schema2)) {
               $result = false;  last;
            }
         } else {
            $result = false;  last;
         }
      }
      keys %$s1;
      $result
   }
}

sub same_multi_schema {
   my ($items1, $items2) = @_;
   $items1 = $items1->{oneOf} // [ $items1 ];
   $items2 = $items2->{oneOf} // [ $items2 ];
   return false if @$items1 != @$items2;
   foreach (@$items1) {
      return false if grep_distinct_schema($items2, $_);
   }
   true
}

#############################################################################################
# private:
sub serialize_data {
   my ($src, $options) = @_;
   if (is_object($src)) {
      my $proto = eval { $src->type };
      if (instanceof BigObjectType($proto)) {
         my $result = $src->serialize($options);
         if ($options->{pretty} && defined(my $load = $result->{_load})) {
            JSON::XS::set_multiline_flag($load, false);
         }
         return $result;
      } elsif (instanceof PropertyType($proto)) {
         my $result = { data => $proto->serialize->($src, $options), _type => $proto->qualified_name };
         $options->{ext}->collect($proto, $result);
         return $result;
      } else {
         # non-polymake object, don't know how to serialize (JSON encoder will probably croak)
         return $src;
      }

   } elsif (is_array($src)) {
      # anonymous array of loose items
      local $options->{name_proposal} = -1;
      return { data => [ map { $options->{name_proposal}++; serialize_data($_, $options) } @$src ], _type => undef };

   } elsif (is_hash($src)) {
      # anonymous hash of loose items
      local $options->{name_proposal} = undef;
      return { data => { map { $options->{name_proposal} = $_; ($_ => serialize_data($src->{$_}, $options)) } keys %$src }, _type => undef };

   } else {
      # simple scalar, no transformation needed
      # (if this is something exotic, JSON encoder will croak)
      return { data => $src, _type => undef };
   }
}
#############################################################################################
# private:
sub deserialize_data {
   my ($src, $flags, $options) = @_;
   my $data = $src->{data};
   my $type = $src->{_type};
   if (!defined($type)) {
      if (defined($data)) {
         if (is_array($data)) {
            return [ map { deserialize_data($_, $flags, $options) } @$data ];
         } elsif (is_hash($data)) {
            return { map { $_ => deserialize_data($data->{$_}, $flags, $options) } keys %$data };
         } elsif (is_object($data)) {
            croak( "can't deserialize from an object ", ref($data) );
         } else {
            return $data;
         }
      } else {
         croak( "top-level '_type' property missing" );
      }
   }
   strip_prefix($type, $options->{ns_prefix})
     or croak( "top-level data type $type is not known to polymake" );
   my @apps = map { add Application($_) } $type =~ /\b($id_re)::/g
     or croak( "missing application name in the top-level type $type" );
   my $proto = $apps[0]->eval_type($type)
     or croak( "invalid or unknown data type $type" );

   if (instanceof BigObjectType($proto)) {
      my $object = BigObject::deserialize($proto, $src, $options);
      if ($options->{set_changed_flag}) {
         $object->changed = false;
         $flags->{changed} = true;
      }
      $object
   } else {
      if ($options->{set_changed_flag}) {
         $flags->{changed} = true;
      }
      $proto->deserialize->($data // croak( "top-level 'data' property missing" ), $options)
   }
}
#############################################################################################
# private:
sub upgrade_data {
   my ($src, $from_version) = @_;
   my $upgrade_cnt = 0;
   my $data = $src->{data};
   my $type = $src->{_type};
   if (defined($type)) {
      # special hack for homogeneous big object arrays serialized in separate files in old days
      if (is_array($data) && $type =~ s/^(?:common::)?Array<($type_re)>$/$1/) {
         $upgrade_cnt += Upgrades::apply($_, $from_version, $type) for @$data;
      } else {
         $upgrade_cnt = Upgrades::apply($src, $from_version);
      }
   } elsif (defined($data)) {
      if (is_array($data)) {
         $upgrade_cnt += upgrade_data($_, $from_version) for @$data;
      } elsif (is_hash($data)) {
         $upgrade_cnt += upgrade_data($_, $from_version) for values %$data;
      } elsif (is_object($data)) {
         croak( "can't deserialize from an object ", ref($data) );
      }
   } else {
      croak( "top-level '_type' property missing" );
   }
}
#############################################################################################
# top-level function for serializing a `small' or `big' object or an anonymous array or hash thereof
# (data, options) => perl hash suitable for JSON encoding or schema validation
# supported options:
#   pretty => true   annotate embedded lists and objects for JSON::XS pretty printing
#   save => true  this is part of a save/save_data operation, clean up all big objects
#   schema => Schema  JSON schema prescribing some properties of the resulting document:
#                     {properties} => can contain properties, attachments, and user methods
#                     !{additionalProperties} => only explicitly listed {properties} are serialized
#                     {required} => enforce creation of missing properties or call user methods
#                     items, additionalItems, maxItems => control inclusion of multiple subobject instances
#                     Schema can contain conditional parts, referring to simple properties (booleans, integers, strings)
sub serialize {
   my ($src, $options) = @_;
   if (@_ == 0 or @_ > 2 or defined($options) && !is_hash($options)) {
      croak("serialize - expected exactly one data object and a hash with options");
      return;
   }
   $options //= { };
   local $options->{_ns} = my $ns = default_ns();
   local $options->{ext} = new ExtCollector($ns, $options);
   my $result = serialize_data($src, $options);
   if (keys(%$ns) > 1) {
      $ns{default} = "polymake";
   }
   $result->{_ns} = $ns;
   if ($options->{pretty}) {
      JSON::XS::set_multiline_flag($_, false) for values %$ns;
   }
   $result;
}
#############################################################################################
# top-level function for deserializing a `small' or `big' object or an anonymous array or hash thereof
sub deserialize {
   my ($src, $flags) = @_;
   if (is_hash($src) && is_hash(my $ns = $src->{_ns})) {
      # looks like our serialized data
      my ($ns_prefix, @exts, @mandatory, @ext_versions);
      my $ns_data = keys(%$ns) == 1 ? (values(%$ns))[0] : $ns->{$ns->{default}};
      unless (defined($ns_data) && $ns_data->[0] eq $mainURL) {
         undef $ns_data;
         while (my ($k, $v) = each %$ns) {
            if ($k ne "default" && $v->[0] eq $mainURL) {
               $ns_prefix = $k;
               $ns_data = $v;
               keys %$ns; last;
            }
         }
      }
      defined($ns_data)
        or croak( "no polymake data items in deserialized data" );
      my $main_version = eval("v".$ns_data->[1]);
      my $version_bump = $VersionNumber gt $main_version;

      if (defined(my $top_ext = $src->{_ext})) {
         $mandatory[$_] = true for @$top_ext;
      }
      my $ext_index = 0;
      foreach my $ext_data (@$ns_data[2..$#$ns_data]) {
         # ignore traces of bundled extensions leaking through old data files
         $exts[$ext_index] = $ext_data->[0] !~ /^bundled:/ && provide Extension($ext_data->[0], $mandatory[$ext_index]);
         # TODO: extension-specific upgrades
         #   and
         # $ext_versions[$ext_index] = $ext_data->[1]
         #   and
         # $version_bump ||= $exts[$ext_index]->version_num gt eval("v".$ext_versions[$ext_index]);
         ++$ext_index;
      }

      my $upgrade_cnt;
      if ($version_bump) {
         require Polymake::Core::Upgrades;
         $upgrade_cnt = upgrade_data($src, $main_version);
         if ($upgrade_cnt && $Verbose::files && defined($flags->{filename})) {
            dbg_print("upgrading ", $flags->{filename}, " from version ", $ns_data->[1]);
         }
      }
      local $PropertyType::trusted_value = false if $upgrade_cnt;
      my $set_changed_flag = defined($flags) && exists $flags->{changed} && (!$PropertyType::trusted_value || $version_bump);

      # true => hide data item as "undecoded"
      my $ext_check = sub { grep { !defined } @exts[@{$_[0]}] };
      my $ext_store = sub {
         [ map {
              if (is_object($exts[$_])) {
                 # known extension
                 $exts[$_]
              } elsif (!defined($exts[$_])) {
                 # unknown extension
                 $ns_data->[2 + $_]
              } else {
                 # extension to be ignored
                 ()
              }
           } @{$_[0]} ]
      };

      my %options = ( ext_check => $ext_check, ext_store => $ext_store,
                      ns_prefix => $ns_prefix,
                      set_changed_flag => $set_changed_flag );
      deserialize_data($src, $flags, \%options);
   } else {
      croak( "does not look like polymake data file" );
   }
}
#############################################################################################
sub create_permissive_schema {
   my $schema = Schema::empty();
   my $type_collector = new TypeCollector();
   my @type_names;
   my @by_type = map {
      my $type_name = $_->qualified_name;
      push @type_names, $type_name;
      [ { properties => { _type => { const => $type_name } } },
        { '$ref' => "$Schema::mainSchemaURI/data.json#/definitions/top_level" },
        inject_type_schema($_)->($type_collector) ]
   } @_;
   if (@by_type > 1) {
      $schema->{oneOf} = [ map { { allOf => $_ } } @by_type ];
   } else {
      $schema->{allOf} = $by_type[0];
   }
   $type_collector->finalize($schema);
   $schema->{title} = "polymake data objects of type" . (@type_names > 1 && "s") . " " . join(", ", @type_names);
   new Schema($schema)
}

#############################################################################################
sub create_restrictive_schema {
   my ($obj) = @_;
   $obj->prepare_for_schema_creation;
   my $schema = Schema::empty();
   $schema->{properties} = {
      _ns => {
        type => "object",
        properties => {
          polymake => ns_entry_for_schema($mainURL, $Version)
        },
        additionalProperties => false
      },
      _canonical => { type => "boolean" },
      _load => { type => "array", items => { type => "string" } }
   };
   $schema->{required} = [ '_ns' ];

   my $type_collector = new TypeCollector();
   my %exts;
   add_properties_to_schema($obj, $schema, $type_collector, \%exts);
   if (my @ext_ns_entries = map { ns_entry_for_schema($_->URI, $_->version) } keys %exts) {
      $schema->{properties}->{_ns}->{properties}->{polymake}->{additionalItems} =
         @ext_ns_entries > 1 ? { oneOf => \@ext_ns_entries } : $ext_ns_entries[0];
   }

   $type_collector->finalize($schema);
   new Schema($schema)
}

sub create_restrictive_subschema {
   my ($obj, $parent_schema) = @_;
   my $type_collector = new TypeCollector();
   my %exts;
   my $schema = add_properties_to_schema($obj, { }, $type_collector, \%exts);
   if (is_object($parent_schema)) {
      undef $parent_schema->validator;
      $type_collector->finalize($parent_schema->source);
   } else {
      $type_collector->finalize($parent_schema);
   }
   $schema
}

sub ns_entry_for_schema {
   my ($URI, $version) = @_;
   return {
      type => "array",
      items => [ { const => $URI }, { const => $version } ],
      additionalItems => false
   };
}
#############################################################################################
sub prescribe_property_types {
   my $src = shift;
   my $schema = is_object($src) ? deep_copy_hash($src->source) : $src;
   my ($app, $top_type, $proto);
   $top_type = descend_nested_hash($schema, qw(properties _type const))
     or die "schema does not look like a restrictive schema for big objects";
   $top_type =~ s/^($id_re):://
     and
   $app = add Application($1)
     and
   $proto = $app->eval_type($top_type)
     or die "schema refers to an invalid or unknown data type $top_type\n";
   instanceof BigObjectType($proto)
     or die "schema must describe a big object type\n";
   my $type_collector = new TypeCollector();

   while (my ($path, $type) = splice @_, 0, 2) {
      if (is_string($type)) {
         if ($type eq "undef" || $type eq "default") {
            undef $type;
         } else {
            $type = $app->eval_type($type)
              or die "invalid or unknown property type $type\n";
         }
      }
      if (defined($type) && !instanceof PropertyType($type)) {
         die "prescribed type must be a property type\n";
      }
      my @path = split /\./, $path;
      my $prop_name = pop @path;
      my $subobj_props = descend_to_properties($schema, @path);
      $subobj_props && exists $subobj_props->{$prop_name}
        or die "property $path does not occur in the schema\n";
      my @props = $proto->encode_descending_path($path);
      if (defined($type)) {
         $subobj_props->{$prop_name} = inject_type_schema($type)->($type_collector);
         $subobj_props->{_attrs}{properties}{$prop_name}{properties}{_type}
           = { const => $type->qualified_name((@path ? $props[-2]->subobject_type : $proto)->application) };
      } elsif (defined(my $attrs = descend_nested_hash($subobj_props, qw(_attrs properties)))) {
         $subobj_props->{$prop_name} = inject_type_schema($props[-1]->type)->($type_collector);
         delete $attrs->{$prop_name};
      }
   }

   $type_collector->finalize($schema);
   if ($schema != $src) {
      undef $src->validator;
      $src->source = $schema;
   }
}

sub descend_to_properties {
   my $schema = shift;
   for (;;) {
      my $props;
      unless ($schema->{type} eq "object" and $props = $schema->{properties}) {
         if (my $all = $schema->{allOf}) {
            foreach (@$all) {
               last if $_->{type} eq "object" and $props = $_->{properties};
            }
         }
      }
      $schema = $props or last;
      my $prop_name = shift(@_) or last;
      $schema = $schema->{$prop_name} or last;
      if ($schema->{type} eq "array") {
         if (my $items = $schema->{items}) {
            $schema = is_hash($items) ? $items : $items->[is_integer($_[0]) || $_[0] =~ /^\d+$/ ? shift(@_) : 0] or last;
         } else {
            return;
         }
      }
   }
   $schema
}
#############################################################################################
sub Polymake::Core::BigObject::serialize : method {
   my ($self, $options) = @_;
   my (%result, %attrs, $this_scope);
   my ($restricted, $flat_schema, $schema_props, $attrs_schema, $attrs_restricted, $attrs_props);
   my $proto = $self->type;
   if (defined($self->parent)) {
      $flat_schema = $options->{schema};
   } else {
      if ($options->{save}) {
         $self->prepare_for_save($options->{name_proposal});
      }
      if (defined(my $schema = $options->{schema})) {
         # do not evaluate standard property type subschemas
         local $Schema::Validator::Ref::exempt_flat = qr{/definitions/};
         $flat_schema = $schema->flatten(wrap_for_flat_schema($self, $proto));
         provide_for_flat_schema($self, $proto, $flat_schema);
      }
   }
   if (length($self->name)) {
      $result{_id} = $self->name;
   }
   if (length($self->description)) {
      $result{_info}->{description} = $self->description;
   }

   my %credits;
   while (my ($product, $credit) = each %{$self->credits}) {
      $credits{$product} = is_object($credit) ? $credit->toFileString : $credit;
   }
   if (keys %credits) {
      $result{_info}->{credits} = \%credits;
   }
   if (my $type_name = qualified_type_name($self, $proto)) {
      $result{_type} = $type_name;
   }

   my $ext_collector = $options->{ext};
   $ext_collector->collect($proto->pure_type, \%result, $this_scope);
   my $app = $proto->application;
   local $options->{known_apps} = { $app->name => true } if !defined($self->parent);
   my $allow_shortcuts;
   if (defined($flat_schema)) {
      $restricted = is_defined_and_false($flat_schema->{additionalProperties});
      $schema_props = $flat_schema->{properties};
      $attrs_schema = $schema_props && $schema_props->{_attrs};
      $attrs_restricted = defined($attrs_schema) ? is_hash($attrs_schema) && is_defined_and_false($attrs_schema->{additionalProperties}) : $restricted;
      $attrs_props = is_hash($attrs_schema) ? $attrs_schema->{properties} : undef;
      if (defined(my $required = $flat_schema->{required})) {
         # properties born by shortcut rules are only serialized when explicitly required
         $allow_shortcuts = sub { string_list_index($required, $_[0]) >= 0 };
      }
   }

   foreach my $pv (@{$self->contents}) {
      next unless $pv->is_storable($allow_shortcuts);
      my $prop_name = $pv->property->name;
      my $subschema = $schema_props && $schema_props->{$prop_name};
      next if $restricted && !$subschema;
      my (%prop_attrs, $prop_scope);
      $ext_collector->collect($pv->property, \%prop_attrs, $prop_scope);

      local if (defined($flat_schema)) {
         if ($pv->property->flags & Property::Flags::is_subobject) {
            local $options->{schema} = !is_boolean($subschema) ? $subschema : undef;
         } elsif ($pv->property->flags & (Property::Flags::is_subobject_array)) {
            local $options->{schema} = is_array($subschema) ? $subschema->[0] : undef;
         } elsif (defined(my $type_in_schema = $attrs_props && $attrs_props->{$prop_name})) {
            $type_in_schema = $type_in_schema->{properties};
            local $options->{type} = is_array($type_in_schema &&= $type_in_schema->{_type})
                                     && $proto->application->eval_type($type_in_schema->[0]);
         } elsif ($attrs_restricted) {
            local $options->{type} = false;
         }
      }
      if (my ($value, $type) = $pv->serialize($options)) {
         $result{$prop_name} = $value;
         if (defined($type)) {
            $prop_attrs{_type} = $type->qualified_name($app);
            $ext_collector->collect($type, \%prop_attrs);
         }
         $attrs{$prop_name} = \%prop_attrs if keys %prop_attrs;
         if (defined(my $other_app = $pv->property->application)) {
            add_other_app($options->{known_apps}, $other_app);
         }
      }
   }

   while (my ($name, $attachment) = each %{$self->attachments}) {
      my ($data, $construct, $undecoded_attrs) = @$attachment;
      if ($construct eq undecoded_tag) {
         next if $restricted;
         my %data_attrs;
         if (is_hash(my $stored_attrs = $undecoded_attrs->{attrs})) {
            push %data_attrs, %$stored_attrs;
         }
         my $ext_list = $undecoded_attrs->{ext};
         if ($undecoded_attrs->{multi}) {
            # skipped instances of a multiple subobject
            foreach (0..$#$data) {
               $data->[$_]->{_ext} = $ext_collector->collect_skipped($ext_list->[$_]);
            }
            if (exists $result{$name}) {
               push @{$result{$name}}, @$data;
            } else {
               $result{$name} = $data;
            }
         } elsif ($undecoded_attrs->{subobj}) {
            # skipped single subobject
            $data->{_ext} = $ext_collector->collect_skipped($ext_list);
            if (exists $result{$name}) {
               # in the meanwhile created a valid subobject at its place:
               # store as an attachment
               $name .= undecoded_tag;
               $data_attrs{attachment} = true;
            }
            $result{$name} = $data;
         } else {
            # skipped atomic property or attachment
            if (defined($ext_list)) {
               $data_attrs{_ext} = $ext_collector->collect_skipped($ext_list);
            }
            $result{$name} = $data;
         }
         if (keys %data_attrs) {
            $attrs{$name} = \%data_attrs;
         }
      } else {
         next if $restricted and not($schema_props && $schema_props->{$name});
         my %att_attrs = (attachment => true);
         if (is_object($data)) {
            my $data_type = $data->type;
            $result{$name} = $data_type->serialize->($data, $options);
            $att_attrs{_type} = $data_type->qualified_name($proto->application);
            if (defined $construct) {
               $att_attrs{construct} = $construct;
            }
            $ext_collector->collect($data_type, \%att_attrs);
         } else {
            $result{$name} = $data;
         }
         $attrs{$name} = \%att_attrs;
      }
   }

   if (defined($flat_schema) && defined(my $methods = $flat_schema->{methods})) {
      foreach my $name (@$methods) {
         my $value = eval { $self->$name() };
         if ($@) {
            die "Required user method $name failed: $@";
         }
         $attrs{$name}->{method} = true;
         if (is_object($value)) {
            my $type = value->type;
            $result{$name} = $value->type->serialize->($value);
            $attrs{$name}->{_type} = $type->qualified_name($proto->application);
            $ext_collector->collect($type, $attrs{$name});
         } else {
            $result{$name} = $value;
         }
      }
   }

   if (keys %attrs) {
      $result{_attrs} = \%attrs;
   }
   if (!defined($self->parent)) {
      my $known_apps = $options->{known_apps};
      delete $known_apps->{$app->name};
      if (keys %$known_apps) {
         $result{_load} = [ sort keys %$known_apps ];
      }
   }
   \%result
}

sub qualified_type_name {
   my ($self, $type) = @_;
   if (!defined($self->parent)) {
      $type->qualified_name
   } elsif ($self->property->subobject_type->pure_type != $type->pure_type) {
      $type->pure_type->qualified_name
   }
}

sub add_other_app {
   my ($known, $other_app) = @_;
   unless ($known->{$other_app->name}) {
      foreach my $app_name (keys %$known) {
         if ($other_app->used->{$app_name}) {
            delete $known->{$app_name};
         } elsif (lookup Application($app_name)->used->{$other_app->name}) {
            keys %$known;
            return;
         }
      }
   }
   $known->{$other_app->name} = true;
}
####################################################################################
package Polymake::Core::BigObject;

sub deserialize {
   my ($proto, $src, $options, $parent, $parent_prop) = @_;
   my $ext_check = $options->{ext_check};
   my $ext_list;

   if (defined($parent) && defined(my $type = $src->{_type})) {
      $proto = $parent->type->application->eval_type($type) // croak( "invalid subobject type $type" );
      $proto = $parent_prop->type->final_type($proto) if $parent_prop->flags & Property::Flags::is_augmented;
   }
   my $app = $proto->application;

   my $self = new($proto->pkg, $src->{_id});
   if (!defined($parent)) {
      $options->{deferred} = [ ];
      if (!$PropertyType::trusted_value) {
         begin_init_transaction($self);
      }
      if (defined(my $other_apps = $src->{_load})) {
         foreach (@$other_apps) {
            # TODO: filter out foreign prefixes?
            add Application($_);
         }
      }
   }

   if (defined(my $info = $src->{_info})) {
      $self->description = $info->{description};
      if (defined(my $credits = $info->{credits})) {
         foreach (my ($product, $text) = each %$credits) {
            $self->credits->{$product} = $app->lookup_credit($product) // $text;
         }
      }
   }

   my $attrs = $src->{_attrs} || { };
   while (my ($name, $value) = each %$src) {
      # TODO: recognize foreign prefixes
      my $prop_attrs = $attrs->{$name};
      next if $prop_attrs && $prop_attrs->{method};

      if (defined($ext_list = $prop_attrs && $prop_attrs->{_ext}) && $ext_check->($ext_list)) {
         add_undecoded($self, $name, $value, { attrs => $prop_attrs, ext => $options->{ext_store}->($ext_list) });
         next;
      }
      my $type = $prop_attrs && $prop_attrs->{_type};
      if (defined($type)) {
         $type = $app->eval_type($type) // croak( "invalid type $type for property $name" );
      }
      if ($prop_attrs && $prop_attrs->{attachment}) {
         if (instanceof BigObjectType($type)) {
            croak( "invalid type $prop_attrs->{_type} of attachment $name" );
         }
         if (defined(my $construct_args = $prop_attrs->{construct})) {
            defined($type)
              or croak( "invalid attachment $name: missing _type attribute" );
            push @{$options->{deferred}}, sub {
               $self->attachments->{$name} = [ $type->construct->($self->give_list($construct_args), $value), $construct_args ];
            };
         } else {
            $self->attachments->{$name} = [ defined($type) ? $type->construct->($value) : $value ];
         }
      } elsif ($name =~ /^_/) {
         next;
      } elsif (defined(my $prop = $proto->lookup_property($name))) {
         if ($prop->flags & Property::Flags::is_subobject) {
            if ($prop->flags & Property::Flags::is_multiple) {
               my (@undecoded, @undecoded_exts);
               if (my @subobjs = map {
                      if (defined($ext_list = $_->{_ext}) && $ext_check->($ext_list)) {
                         push @undecoded, $_;
                         push @undecoded_exts, $options->{ext_store}->($ext_list);
                         ()
                      } else {
                         deserialize($prop->subobject_type, $_, $options, $self, $prop)
                      }
                   } is_array($value) ? @$value : $value) {
                  _add_multis($self, $prop, \@subobjs, $PropertyType::trusted_value);
               }
               if (@undecoded) {
                  add_undecoded($self, $name, \@undecoded, { attrs => $prop_attrs, ext => \@undecoded_exts, multi => true });
               }
            } else {
               if (defined($ext_list = $value->{_ext}) && $ext_check->($ext_list)) {
                  add_undecoded($self, $name, $value, { attrs => $prop_attrs, ext => $options->{ext_store}->($ext_list), subobj => true });
               } else {
                  _add($self, $prop, deserialize($prop->subobject_type, $value, $options, $self, $prop), $PropertyType::trusted_value);
               }
            }
         } elsif ($prop->flags & Property::Flags::is_subobject_array) {
            _add($self, $prop, $prop->type->deserialize->($value, $options, $self, $prop), $PropertyType::trusted_value);
         } elsif (defined($prop->construct)) {
            push @{$options->{deferred}}, sub {
               _add($self, $prop, $value, $PropertyType::trusted_value);
            };
         } else {
            _add($self, $prop, defined($type) ? $type->deserialize->($value) : $value, $PropertyType::trusted_value);
         }
      } elsif ($Serializer::reject_unknown_properties) {
         croak( "unknown property name $name for object type ", $proto->full_name );
      } else {
         add_undecoded($self, $name, $value, { attrs => $prop_attrs });
      }
   }

   if (!defined($parent)) {
      &$_ for @{delete $options->{deferred}};
      if (!$PropertyType::trusted_value) {
         $self->transaction->changed = true;
         $self->transaction->commit($self);
      }
   }

   $self;
}

sub add_undecoded {
   my ($self, $name, $value, $attrs) = @_;
   $self->attachments->{$name} = [ $value, Polymake::Core::Serializer::undecoded_tag, $attrs ];
}

####################################################################################
package Polymake::Core::Serializer;

sub add_properties_to_schema {
   my ($obj, $schema, $type_collector, $exts) = @_;
   my $properties = ($schema->{properties} //= { });
   $properties->{_id} = { '$ref' => "$Schema::mainSchemaURI/data.json#/definitions/obj_id" };
   $properties->{_info} = { '$ref' => "$Schema::mainSchemaURI/data.json#/definitions/obj_info" };
   my $proto = $obj->type;
   if (my $type_name = qualified_type_name($obj, $proto)) {
      $properties->{_type} = { const => $type_name };
      push @{$schema->{required}}, '_type';
   }
   $schema->{additionalProperties} = false;
   $schema->{type} = "object";
   foreach ($proto->pure_type->required_extensions) {
      $exts->{$_} = true;
      $properties->{_ext} = { '$ref' => "$Schema::mainSchemaURI/data.json#/definitions/ext_list" };
   }

   foreach my $pv (@{$obj->contents}) {
      next unless $pv->is_storable(sub { true });  # include properties born by shortcut rules
      my $prop_name = $pv->property->name;
      if ($pv->property->flags & Property::Flags::is_subobject) {
         if ($pv->property->flags & Property::Flags::is_multiple) {
            # by default, we prescribe the contents of the 0-th subobject instance only
            # if needed in the future, this can be amended by maxItems, additionalItems, or replaced with items => { anyOf }
            $properties->{$prop_name} = { type => "array",
                                          items => [ add_properties_to_schema($pv->values->[0], { }, $type_collector, $exts) ],
                                          minItems => 1 };
         } else {
            $properties->{$prop_name} = add_properties_to_schema($pv->value, { }, $type_collector, $exts);
         }
      } elsif ($pv->property->flags & Property::Flags::is_subobject_array) {
         if (@{$pv->value}) {
            # for now, we assume homogeneous subobject arrays
            $properties->{$prop_name} = { type => "array", items => add_properties_to_schema($pv->value->[0], { }, $type_collector, $exts) };
         } else {
            # user has chosen a non-representative data instance, let's make the validation fail if anything appears here
            $properties->{$prop_name} = { type => "array", maxItems => 0 };
         }
      } else {
         my $prop_type = $pv->property->type;
         if (is_object($pv->value)) {
            my $real_type = $pv->value->type;
            if ($real_type != $prop_type) {
               $properties->{_attrs}{properties}{$prop_name}{properties}{_type}
                 = { const => $real_type->qualified_name($proto->application) };
               $prop_type = $real_type;
               $exts->{$_} = true for $real_type->required_extensions;
            }
         }
         $properties->{$prop_name} = inject_type_schema($prop_type)->($type_collector);
      }
      push @{$schema->{required}}, $prop_name;
      $exts->{$_} = true for $pv->property->required_extensions;
   }

   while (my ($att_name, $att) = each %{$obj->attachments}) {
      my ($value, $construct) = @$att;
      next if $construct eq undecoded_tag;
      my $att_props = $properties->{_attrs}->{properties}->{$att_name}->{properties} = { attachment => { const => true }};
      if (is_object($value)) {
         my $data_type = $value->type;
         $att_props->{_type} = { const => $data_type->qualified_name($proto->application) };
         $properties->{$att_name} = inject_type_schema($data_type)->($type_collector);
         $exts->{$_} = true for $data_type->required_extensions;
      } else {
         $properties->{$att_name} = { type => Schema::Validator::data2type($value) };
      }
   }

   if (defined(my $attrs = $properties->{_attrs})) {
      $attrs->{additionalProperties} = false;
      $attrs->{type} = "object";
   }
   $schema
}
####################################################################################
sub provide_for_flat_schema {
   my ($obj, $proto, $schema) = @_;
   if (is_hash($schema)) {
      my @requests;
      add_required_properties(\@requests, $schema, $obj, $proto, []);
      $obj->provide_request(\@requests) if @requests;
   } elsif (!$schema) {
      croak( "big object does not satisfy the schema" );
   }
}

sub add_required_properties {
   my ($requests, $schema, $obj, $proto, $parent_path) = @_;
   my $properties = $schema->{properties};
   my $restricted = is_defined_and_false($schema->{additionalProperties});

   if (defined(my $required = $schema->{required})) {
      foreach my $prop_name (@$required) {
         next if $prop_name =~ /^_/;
         if ($restricted && not($properties && $properties->{$prop_name})) {
            croak( "invalid schema: required property $prop_name not allowed per 'properties' or 'additionalProperties'" );
         }
         if (defined(my $prop = $proto->lookup_property($prop_name))) {
            my $pv;
            if (defined($obj) && defined(my $content_index = $obj->dictionary->{$prop->key})) {
               $pv = $obj->contents->[$content_index];
            }
            if ($prop->flags & Property::Flags::is_subobject) {
               if (defined(my $prop_schema = $properties && $properties->{$prop_name})) {
                  my $sub_proto = $prop->subobject_type;
                  if (is_hash($prop_schema)) {
                     local push @$parent_path, $prop;
                     add_required_properties($requests, $prop_schema, $pv, $sub_proto, $parent_path);
                  } elsif (is_array($prop_schema)) {
                     if (defined($pv)) {
                        foreach (1..$#$prop_schema) {
                           if (is_hash($prop_schema->[$_])) {
                              provide_for_flat_schema($pv->values->[$_], $sub_proto, $prop_schema->[$_]);
                           }
                        }
                     }
                     if (is_hash($prop_schema->[0])) {
                        local push @$parent_path, $prop;
                        add_required_properties($requests, $prop_schema->[0], $pv && $pv->values->[0], $sub_proto, $parent_path);
                     }
                  }
               } else {
                  croak( "invalid schema: no subschema for required subobject $prop_name" );
               }
            } elsif (!defined($pv)) {
               push @$requests, [ [ @$parent_path, $prop ] ];
            }
         } elsif (UNIVERSAL::can($proto->pkg, $prop_name)) {
             push @{$schema->{methods}}, $prop_name;
         }
      }
   }
}

#############################################################################################
package Polymake::Core::Serializer::TypeCollector;

use Polymake::Struct (
   '%types',
   '@queue',
);

sub new {
   my $self = &_new;
   if (defined(my $schema = shift)) {
      
   }
   $self
}

sub collect {
   my ($self, $proto) = @_;
   $self->types->{$proto} //= do { push @{$self->queue}, $proto; true };
}

sub finalize {
   my ($self, $schema) = @_;
   while (defined (my $proto = shift @{$self->queue})) {
      $schema->{definitions}{subschema_name($proto)} //= $proto->JSONschema->($self);
   }
}

#############################################################################################
package Polymake::Core::Serializer::ExtCollector;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$ns' => '#1' ],
   [ '$options' => '#2' ],
   '%ext2index',
   '%suppress',
);

sub collect {
   my ($self, $item, $attrs) = @_;
   my $added = false;
   foreach my $ext (grep { !exists $self->suppress->{$_} } $item->required_extensions) {
      my $index = ($self->ext2index->{$ext} //= do {
         push @{$self->ns->{polymake}}, pretty_format([ $ext->URI, $ext->version ], false, $self->options);
         @{$self->ns->{polymake}}-3
      });
      push @{$attrs->{_ext}}, $index;
      $added = true;
      if (@_ == 4) {
         local with($_[3]) {
            local $self->suppress->{$ext} = true;
         }
      }
   }
   if ($added) {
      my $ext_list = $attrs->{_ext};
      if (@$ext_list > 1) {
         pretty_format($attrs->{_ext} = [ num_sorted_uniq(sort { $a <=> $b } @$ext_list) ], false, $self->options);
      } else {
         pretty_format($ext_list, false, $self->options);
      }
   }
}

sub collect_skipped {
   my ($self, $ext_list) = @_;
   my @result = map {
      my $ext = $_;
      if (is_object($ext) && exists $self->suppress->{$ext}) {
         ()
      } else {
         $self->ext2index->{$ext} //= do {
            push @{$self->ns->{polymake}}, pretty_format(is_object($ext) ? [ $ext->URI, $ext->version ] : $ext, false, $self->options);
            @{$self->ns->{polymake}}-3
         }
      }
   } @$ext_list;
   pretty_format(@result > 1 ? [ num_sorted_uniq(sort { $a <=> $b } @result) ] : \@result, false, $self->options);
}

#############################################################################################
package Polymake::Core::Serializer::Sparse;

use Polymake::Struct (
   [ new => '$' ],
   [ '$values' => '#1' ],
   [ '$it' => 'undef' ],
);

*TIEHASH = \&_new;

sub FIRSTKEY {
   my ($self) = @_;
   $self->it = 0;
   $self->values->[0]
}

sub NEXTKEY {
   my ($self) = @_;
   $self->values->[$self->it += 2]
}

sub FETCH {
   my ($self, $key) = @_;
   if ($self->it >= @{$self->values}) {
      # first FETCH after many NEXTKEYS: values(%{hash}), %{hash}
      $self->it = 0;
   }
   if (is_integer($key) ? $key == $self->values->[$self->it] : $key eq $self->values->[$self->it]) {
      # FETCH after FIRST/NEXTKEY: regular iteration, each %{hash}
      $self->values->[$self->it+1]
   } elsif ($self->it+2 < @{$self->values} and
            is_integer($key) ? $key == $self->values->[$self->it+2] : $key eq $self->values->[$self->it+2]) {
      # next FETCH
      $self->it += 2;
      $self->values->[$self->it+1]
   } else {
      # random access???
      croak( "unexpected random access" );
   }
}

sub EXISTS {
   # this is only used for schema validation, exact index value does not matter
   my ($self, $key) = @_;
   if ($key eq dim_key()) {
      @{$self->values} && $self->values->[-2] eq $key
   } else {
      $key =~ /^\d+$/
   }
}
#############################################################################################
package Polymake::Core::Serializer::BigObjectForFlatSchema;

use Polymake::Struct (
   [ new => '$$;$' ],
   [ '$obj' => '#1' ],
   [ '$proto' => '#2' ],
   [ '$descend' => '#3' ],
);

sub TIEHASH { &_new }

sub FETCH {
   my ($self, $prop_name) = @_;
   if ($prop_name =~ /^_/) {
      if ($prop_name eq "_id") {
         !defined($self->descend) && $self->obj->name
      } elsif ($prop_name eq "_type") {
         !defined($self->descend) && qualified_type_name($self->obj, $self->proto)
      } elsif ($prop_name eq "_attrs") {
         # preserve type constraints
         Schema::Validator::merge_into_flat(qr/^$prop_name_re$/, qr/^_type$/);
      } else {
         # nothing else should be relevant for filtering schemas
         sub { undef }
      }
   } elsif (defined(my $prop = $self->proto->lookup_property($prop_name))) {
      if ($prop->flags & (Property::Flags::is_subobject | Property::Flags::is_subobject_array)) {
         my $content_index = defined($self->descend) ? undef : $self->obj->dictionary->{$prop->key};
         if (defined(my $pv = defined($content_index) ? $self->obj->contents->[$content_index] : undef)) {
            if ($prop->flags & Property::Flags::is_multiple) {
               [ map { wrap_for_flat_schema($_, $_->type) } @{$pv->values} ]
            } elsif ($prop->flags & Property::Flags::is_subobject_array) {
               if (defined(my $subobj = $pv->value->[0])) {
                  [ wrap_for_flat_schema($subobj, $subobj->type) ]
               } else {
                  # no schema needed for an empty array
                  [ ]
               }
            } else {
               wrap_for_flat_schema($pv, $pv->type)
            }
         } else {
            if ($prop->flags & (Property::Flags::is_multiple | Property::Flags::is_subobject_array)) {
               # multiple subobject does not exist yes, at most one instance might be created by rules
               # for a big object array, an attempt to evaluate an atomic property will fail, but the subschema will be copied properly
               [ wrap_for_flat_schema($self->obj, $prop->subobject_type, $self->descend . "$prop_name.") ]
            } else {
               wrap_for_flat_schema($self->obj, $prop->subobject_type, $self->descend . "$prop_name.")
            }
         }
      } else {
         sub { $self->obj->give($self->descend . $prop_name) }
      }
   } elsif (!defined($self->descend) && defined(my $att = $self->obj->attachments->{$prop_name})) {
      $att->[0]
   } elsif (defined(my $meth = UNIVERSAL::can($self->proto->pkg, $prop_name))) {
      # user method
      sub { $meth->(defined($self->descend) ? self->obj->give($self->descend =~ s/\.$//r) : $self->obj) }
   } else {
      undef
   }
}

sub EXISTS {
   my ($self, $prop_name) = @_;
   unless ($prop_name =~ /^_/) {
      if (!defined($self->descend)) {
         if (defined(my $prop = $self->proto->lookup_property($prop_name))) {
            my $content_index = $self->obj->dictionary->{$prop->key};
            defined($content_index) && defined($self->contents->[$content_index])
         } else {
            exists($self->obj->attachments->{$prop_name}) ||
            defined(UNIVERSAL::can($self->obj, $prop_name))
         }
      }
   }
}

sub Polymake::Core::Serializer::wrap_for_flat_schema {
   my %dummy;
   tie %dummy, __PACKAGE__, @_;
   \%dummy
}

1;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
