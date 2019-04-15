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

require JSON;

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);
use feature 'state';

package Polymake::Core::Serializer;

# private:
sub default_ns {
   return { polymake => [ $mainURL, $Version ], default => "polymake" }
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

sub complain : method {
   die "don't know how to serialize ", $_[0]->full_name, "\n";
}

sub set_methods : method {
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
   # if called from the serializer, do the real job right now
   if (@_) {
      $proto->serialize->(@_);
   }
}

sub generate_methods {
   my ($proto, $type_descr)=@_;
   if ($proto->serialize && $proto->serialize != \&set_methods && $proto->cppoptions->descr == $type_descr) {
      # reuse already generated methods
      return ($proto->serialize, $proto->JSONschema);
   }
   my ($serialize, $schema) = do {
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
         ()
      } else {
         die noSerialization;
      }
   };
   if ($type_descr->is_sparse_container) {
      my $serialize_sparse = $serialize;
      $serialize = sub {
         my ($obj, $options) = @_;
         if (is_defined_and_false($options->{allow_sparse})) {
            my $dense = args::dense($obj);
            $dense->type->serialize->($dense, $options)
         } else {
            &$serialize_sparse
         }
      };
   }
   if (defined($serialize) && $proto->cppoptions->descr == $type_descr) {
      # cache the methods for later use
      $proto->serialize = $serialize;
      $proto->JSONschema = $schema;
   }
   ($serialize, $schema)
}
#############################################################################################
sub serialize_sparse_sequence {
   my ($value_serialize) = @_;
   sub {
      my ($obj, $options) = @_;
      my $dim = scalar @$obj;
      local $options->{names} = false;
      my $suppress_dim = is_defined_and_false(delete local $options->{dim});
      if ($obj->size == $dim) {
         $value_serialize ? [ map { $value_serialize->($_, $options) } @$obj ] : [ @$obj ]
      } else {
         my @result;
         for (my $it = args::entire($obj); $it; ++$it) {
            push @result, $it->index, $value_serialize ? $value_serialize->($it->deref, $options) : $it->deref;
         }
         unless ($suppress_dim) {
            push @result, $dim;
         }
         bless \@result, "Polymake::Core::Serializer::Sparse";
      }
   }
}
#############################################################################################
sub generate_methods_for_sparse_container {
   my ($type_descr) = @_;
   my $value_proto = $type_descr->value_type;
   my $value_descr = $type_descr->value_descr;
   defined($value_proto) && defined($value_descr) or die noSerialization;
   my ($value_serialize, $value_schema) = $value_proto->serialize ? generate_methods($value_proto, $value_descr) : ();
   ( serialize_sparse_sequence($value_serialize),
     sub {
        # TODO
     } )
}
#############################################################################################
sub generate_methods_for_dense_container {
   my ($type_descr) = @_;
   my $value_proto = $type_descr->value_type;
   my $value_descr = $type_descr->value_descr;
   defined($value_proto) && defined($value_descr) or die noSerialization;

   my ($value_serialize, $value_schema) = generate_methods($value_proto, $value_descr);
   if ($value_serialize) {
      ( sub {
           my ($obj, $options) = @_;
           local $options->{names} = false;
           delete local $options->{dim};
           [ map { $value_serialize->($_, $options) } @$obj ]
        },
        sub {
           # TODO
        } )
   } else {
      ( sub {
           my ($obj) = @_;
           [ @$obj ]
        },
        sub {
           # TODO
        } )
   }
}
#############################################################################################
sub generate_methods_for_matrix {
   my ($type_descr) = @_;
   my $row_proto = $type_descr->value_type;
   my $row_descr = $type_descr->value_descr;
   defined($row_proto) && defined($row_descr) or die noSerialization;

   my $may_have_gaps = $type_descr->is_sparse_serialized;
   my ($row_serialize, $row_schema) = generate_methods($row_proto, $row_descr);

   if ($may_have_gaps) {
      my $serialize_rows = serialize_sparse_sequence($row_serialize);
      ( sub {
           my ($obj, $options) = @_;
           local $options->{names} = false;
           $serialize_rows->(args::rows($obj), $options)
        },
        sub {
           # TODO
        } )
   } else {
      my $store_cols = $row_descr->is_sparse_container || $row_descr->is_set;
      my $suppress_dim = $row_descr->is_sparse_container;
      ( sub {
           my ($obj, $options) = @_;
           local $options->{names} = false;
           local $options->{dim} = false if $suppress_dim;
           my @result = map { $row_serialize->($_, $options) } @$obj;
           if ($store_cols || !@result) {
              push @result, { cols => $obj->cols };
           }
           \@result
        },
        sub {
           # TODO
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
   defined($key_proto) && defined($key_descr) && defined($value_proto) && defined($value_descr) or die noSerialization;

   my ($key_serialize, $key_schema) = generate_methods($key_proto, $key_descr);
   my ($value_serialize, $value_schema) = generate_methods($value_proto, $value_descr);

   if ($key_serialize) {
      if ($value_serialize) {
         ( sub {
              my ($obj, $options) = @_;
              local $options->{names} = false;
              my @result;
              while (my ($k, $v) = each %$obj) {
                 push @result, [ $key_serialize->($k, $options), $value_serialize->($v, $options) ];
              }
              \@result
           },
           sub {
              # TODO
           } )
      } else {
         ( sub {
              my ($obj, $options) = @_;
              local $options->{names} = false;
              my @result;
              while (my ($k, $v) = each %$obj) {
                 push @result, [ $key_serialize->($k, $options), $v ];
              }
              \@result
           },
           sub {
              # TODO
           } )
      }
   } elsif ($key_proto->name eq "Int") {
      # generate numeric keys, not strings, and preserve the order
      if ($value_serialize) {
         ( sub {
              my ($obj, $options) = @_;
              local $options->{names} = false;
              my @result;
              while (my ($k, $v) = each %$obj) {
                 push @result, $k, $value_serialize->($v, $options);
              }
              bless \@result, "Polymake::Core::Serializer::Sparse"
           },
           sub {
              # TODO
           } )
      } else {
         ( sub {
              my ($obj) = @_;
              bless [ %$obj ], "Polymake::Core::Serializer::Sparse"
           },
           sub {
              # TODO
           } )
      }
   } else {
      if ($value_serialize) {
         ( sub {
              my ($obj, $options) = @_;
              local $options->{names} = false;
              my %result;
              while (my ($k, $v) = each %$obj) {
                 $result{$k} = $value_serialize->($v, $options);
              }
              \%result
           },
           sub {
              # TODO
           } )
      } else {
         ( sub {
              return { %{$_[0]} };
           },
           sub {
              # TODO
           } )
      }
   }
}
#############################################################################################
sub generate_methods_for_composite {
   my ($proto, $type_descr) = @_;
   my ($member_types, $member_descrs) = ($type_descr->member_types, $type_descr->member_descrs);
   defined($member_types) && defined($member_descrs) or die noSerialization;

   my (@member_serialize, @member_schema);
   my $i = 0;
   foreach my $member_proto (@$member_types) {
      defined($member_proto) or die noSerialization;
      my ($member_serialize, $member_schema) = generate_methods($member_proto, $member_descrs->[$i]);
      push @member_serialize, $member_serialize;
      push @member_schema, $member_schema;
   } continue { ++$i }

   my ($serialize_to_array, $serialized_array_schema) =
   ( sub {
        my ($obj, $options) = @_;
        local $options->{names} = false;
        my $i = 0;
        [ map { my $method = $member_serialize[$i++]; defined($method) ? $method->($_, $options) : $_ } @$obj ]
     },
     sub {
        # TODO
     } );
   if (defined(my $member_names = $type_descr->member_names)) {
      ( sub {
           my ($obj, $options) = @_;
           unless (is_defined_and_false($options->{names})) {
              my $i = 0;
              return { map { my $method = $member_serialize[$i]; ($member_names->[$i++] => (defined($method) ? $method->($_, $options) : $_)) } @$obj };
           }
           &$serialize_to_array
        },
        sub {
           # TODO
        } )
   } else {
      ($serialize_to_array, $serialized_array_schema)
   }
}
#############################################################################################
sub generate_methods_for_serializable {
   my ($type_descr) = @_;
   my ($serialized_proto, $serialized_descr) = ($type_descr->serialized_type, $type_descr->serialized_descr);
   defined($serialized_proto) && defined($serialized_descr) or die noSerialization;
   my ($serialize, $schema) = generate_methods($serialized_proto, $serialized_descr);
   ( sub {
        my ($obj, $options) = @_;
        $serialize->(CPlusPlus::convert_to_serialized($obj), $options)
     },
     sub {
        # TODO
     } )
}
#############################################################################################
# top-level function for serializing a `small' or `big' object or an anonymous array or hash thereof
# optional trailing argument is an options hash to be passed to Object serialize methods,
# see Object::serialize for possible entries
sub serialize {
   if (@_ == 0) {
      # nothing to serialize
      return;
   }
   my $options = is_hash($_[-1]) ? pop : {};
   if (@_ <= 1 && !ref($_[0])) {
      # a single simple scalar, no transformation needed
      return @_;
   }
   if (@_ > 1) {
      # several loose items
      return [ map { serialize($_, $options) } @_ ];
   }
   my ($src) = @_;
   if (is_array($src)) {
      # anonymous array of loose items
      return [ map { serialize($_, $options) } @$src ];
   }
   if (is_hash($src)) {
      # anonymous hash of loose items
      return { map { ($_ => serialize($src->{$_})) } keys %$src };
   }
   my $ns = default_ns();
   my %ext2index;
   local $options->{ext2index} = sub {
      my $ignore = shift;
      return map { exists $ignore->{$_} ? () : ($_, $ext2index{$_} //= keys(%ext2index) - 1) } @_;
   };
   my $result;
   my $proto = eval { $src->type };
   if (instanceof ObjectType($proto)) {
      defined($src->parent)
         and croak( "can't serialize a subobject without its parent" );
      $result = $src->serialize($options);
   } elsif (instanceof PropertyType($proto)) {
      local $options->{exts_used} = { $options->{ext2index}->({}, $proto->required_extensions) };
      $result = { data => $proto->serialize->($src, $options), _type => $proto->qualified_name() };
      if (keys %{$options->{exts_used}}) {
         push @{$result->{_ext}}, values %{$options->{exts_used}};
      }
   } else {
      # non-polymake object, don't know how to serialize
      return $src;
   }
   while (my ($ext, $index) = each %ext2index) {
      $ns->{polymake}->[$index + 2] = [ $ext->URI, $ext->version ];
   }
   $result->{_ns} = $ns;
   $result;
}
#############################################################################################
# top-level function for deserializing a `small' or `big' object or an anonymous array or hash thereof
sub deserialize {
   if (@_ > 1) {
      croak( "can deserialize exactly one thing at a time" );
   }
   my ($src) = @_;
   if (!ref($src)) {
      return @_;
   }
   if (is_array($src)) {
      return [ map { deserialize($_) } @$src ];
   }
   if (!is_hash($src)) {
      croak( "can't deserialize from an object ", ref($src) );
   }
   if (is_hash(my $ns = $src->{_ns}) && is_string(my $type = $src->{_type})) {
      # looks like our serialized data
      my ($ns_prefix, $ns_data, @exts, @mandatory, @ext_versions);
      unless (defined($ns_data = $ns->{$ns->{default}}) && $ns_data->[0] eq $mainURL) {
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

      if (defined(my $top_ext = $src->{_ext})) {
         $mandatory{$_} = true for @$top_ext;
      }
      my $ext_index = 0;
      foreach my $ext_data (@$ns_data[2..$#$ns_data]) {
         $exts[$ext_index] = provide Extension($ext_data->[0], $mandatory[$ext_index])
           and
         $ext_versions[$ext_index] = $ns_data->[1];
         ++$ext_index;
      }
      my $options = { ext => sub { not grep { !is_object($exts[$_]) } @{$_[0]} } };

      # TODO: apply transformation scripts for polymake core and extensions

      strip_prefix($type, $ns_prefix)
        or croak( "top-level object type is not known to polymake" );
      $type =~ s/^($id_re):://
        or croak( "top-level object type does not contain an application name" );
      my $app = add Application($1);
      my $proto = $app->eval_type($type)
        or croak( "invalid or unknown data type $type" );

      if (instanceof ObjectType($proto)) {
         Object::deserialize($proto, $src, $options)
      } else {
         $proto->deserialize->($src->{data} // croak( "missing 'data' element" ), $options)
      }
   } else {
      # hash of deserialized objects?
      return { map { $_ => deserialize($src->{$_}) } keys %$src };
   }
}

1;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
