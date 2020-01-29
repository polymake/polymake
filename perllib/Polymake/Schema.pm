#  Copyright (c) 1997-2020
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

require JSON;
require POSIX;

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);
use feature 'state';

package Polymake::Schema;

use Polymake::Struct (
   [ new => '$' ],
   [ '$source' => '#1' ],
   [ '$id' => 'undef' ],
   [ '$filename' => 'undef' ],
   [ '$draft' => 'undef' ],
   [ '$validator' => 'undef' ],
   '%refs',
);

declare $mainSchemaURI = "$mainURL/schemas";

my $metaSchemaDraft = 7;
my $metaSchemaURIpattern = "http://json-schema.org/draft-%02d/schema#";
my $metaSchemaURIre = $metaSchemaURIpattern =~ s/%02d/(\\d+)/r;

sub metaSchemaURI {
   sprintf $metaSchemaURIpattern, $_[0]
}

my %by_URI;

# public API

sub load {
   my ($pkg, $filename) = @_;
   -f $filename
     or die "schema file $filename does not exist\n";
   open my $F, $filename
     or die "can't read schema file $filename: $!\n";
   local $/;
   my $self = new($pkg, decode_json(<$F>));
   $self->filename = Cwd::abs_path($filename);
   if ($self->id =~ /^https?:/) {
      $by_URI{$self->id} //= $self;
   }
   $self
}

sub save {
   my ($self, $filename, $options) = @_;
   my $canonical = delete $options->{canonical};
   if (keys %$options) {
      croak( "Schema::save - unknown option(s) ", join(", ", keys %$options) );
   }
   my $encoder = JSON->new;
   if ($canonical) {
      $encoder->indent->space_after->canonical;
   }
   my ($of, $of_k) = new OverwriteFile($filename);
   $encoder->write($self->source, $of);
   close $of;
}

sub TO_JSON {
   my $self = shift;
   return $self->source;
}

sub new_by_URI {
   my ($pkg, $URI, $referrer) = @_;
   if ($URI =~ m{^ \./ | ([^/:]+ (?: $ | /))}) {
      my $rel_path = $1.$';
      if ($referrer->id =~ $directory_re) {
         $URI = $1.$rel_path;
         $by_URI{$URI} //= load($pkg, ($referrer->filename =~ $directory_re)[0].$rel_path)
      } elsif ($referrer->filename =~ $directory_re) {
         load($pkg, $1.$rel_path)
      } else {
         die "does not know how to resolve relative schema URI $URI\n";
      }
   } else {
      $by_URI{$URI} //= do {
         my $filename;
         if ($URI =~ m{^\Q$mainSchemaURI\E/}) {
            $filename = "$Resources/schemas/$'";
         } elsif ($URI =~ /^${metaSchemaURIre}$/o) {
            $filename = sprintf "$Resources/schemas/meta-schema-%02d.json", $1+0;
         } else {
            $filename = $URI =~ s{^file:(?://[^/]*)?}{}r
              or die "does not know how to resolve schema URI $URI\n";
         }
         load($pkg, $filename)
      }
   }
}

sub empty {
   return { '$schema' => metaSchemaURI($metaSchemaDraft) };
}

sub validate {
   my ($self, $data, $message) = @_;
   my $error = ($self->validator //= Validator::create($self))->validate($data);
   if (defined($error)) {
      die $message // "validation failed", ":\n", $error;
   }
   true
}

sub validate_self {
   my ($self) = @_;
   $self->validator = Validator::create($self);
   if (!defined($self->draft)) {
      die "schema URI is missing in ", $self->source->{title} || $self->filename, "\n";
   }
   my $meta_schema = new_by_URI(__PACKAGE__, metaSchemaURI($self->draft));
   $meta_schema->validate($self->source, "invalid schema " . $self->source->{title} || $self->filename);
   $self;
}

# Produce a simple schema without conditional parts and groups (allOf, oneOf...)
# All conditions are evaluated against the given data instance.
# References ('$ref') matching the pattern in $Ref::exempt_flat are not followed but replaced by a trivial 'true' schema.
sub flatten {
   my ($self, $data) = @_;
   ($self->validator //= Validator::create($self))->flatten($data);
}

# find all completions for a given property name prefix
# can be a dotted path descending into nested documents and arrays
# proposals for property names are extracted from 'required' and 'properties' schema elements
sub list_property_completions {
   my ($self, $text) = @_;
   sorted_uniq(sort(($self->validator //= Validator::create($self))->list_property_completions(split /\./, $text)))
}

####################################################################################
package Polymake::Schema::Validator;

my %common_elements = ( '$schema' => false, title => false, description => false, default => false, '$comment' => false, examples => false,
                        if => true, then => true, else => true, id => true, '$id' => true, type => true, enum => true, const => true,
                        '$ref' => sub { new Ref(@_) },
                        oneOf => sub { new One(@_) },
                        allOf => sub { new All(@_) },
                        anyOf => sub { new Any(@_) },
                        not => sub { new Not(@_) }
                      );

my @types = qw(boolean string number null array object);
my %type2index;
@type2index{@types} = 0..$#types;
$type2index{integer} = $type2index{number};

sub data2type_index {
   my ($data, $for_enum) = @_;
   if (is_boolean($data)) {
      0
   } elsif (is_string($data)) {
      1
   } elsif (is_numeric($data)) {
      2
   } elsif (!defined($data)) {
      3
   } elsif ($for_enum) {
      die "invalid enum constant $data\n";
   } elsif (is_array($data)) {
      4
   } elsif (is_hash($data)) {
      5
   } else {
      undef
   }
}

sub data2type { $types[&data2type_index] }

my @type2validator = map { "Polymake::Schema::Validator::".ucfirst($_) } @types;
my %validator2type_index = map { ($type2validator[$_] => $_) } 0..$#types;

my %type_specific_elements;
foreach my $typename (qw(string number array object)) {
   my $type_index = $type2index{$typename};
   foreach my $key (keys %{&{UNIVERSAL::can($type2validator[$type_index], ".keys")}}) {
      unless ($common_elements{$key}) {
         $type_specific_elements{$key} = $type_index;
      }
   }
}

sub create {
   my ($self) = @_;
   local $self->refs->{".UNDEF."} = my $unresolved = { };
   if ($self->source->{'$schema'} =~ /^${metaSchemaURIre}$/o) {
      $self->draft = $1+0;
      if ($self->draft < 4) {
         die "too old schema draft, min required version is 4\n";
      }
   } else {
      die defined($self->source->{'$schema'}) ? "unknown schema URI\n" : '"$schema" attribute missing in the schema document'."\n";
   }
   prepare($self, $self->source, "#");
   if (keys %$unresolved) {
      die "schema ", $self->source->{title} || $self->filename,
          " refers to non-existing sub-schemas: ", join(", ", keys %$unresolved), "\n";
   }
   $self->refs->{'#'}
}

sub prepare {
   my ($self, $schema, $path) = @_;
   if (is_boolean($schema)) {
      return $schema ? new True() : new False();
   }
   my @queue;
   do {
      my (@validators, $action, %deferred, $id, @type_specific);
      while (my ($k, $v) = each %$schema) {
         if (defined($action = $common_elements{$k})) {
            next if !$action;
            if (is_code($action)) {
               push @validators, $action->($self, $v);
            } else {
               $deferred{$k} = $v;
            }
         } elsif (defined(my $type_index = $type_specific_elements{$k})) {
            $type_specific[$type_index]->{$k} = $v;
         } elsif (defined($path) && is_hash($v)) {
            push @queue, $v, "$path/$k";
         } else {
            die "unknown schema property $k\n";
         }
      }

      if (defined($id = $deferred{ $self->draft == 4 ? 'id' : '$id'})) {
         if ($path eq "#") {
            $id =~ m{^\#$id_re$}
              and die "fragment id assigned to the top-level schema\n";
            $self->id = $id;
         } else {
            $id =~ m{^\#$id_re$}
              or die "non-fragment id assigned to a subschema\n";
         }
      }
      if ($self->draft > 4 && defined($deferred{id})) {
         push @queue, $deferred{id}, "$path/id";
      }

      if ($self->draft >= 7 && defined($deferred{if})) {
         push @validators, new If($self, @deferred{qw(if then else)});
      }

      if ($self->draft >= 6 && defined($deferred{const})) {
         ($deferred{enum} &&= die "conflicting elements 'enum' and 'const'\n") = [ $deferred{const} ];
      }

      if (@type_specific && defined($deferred{enum})) {
         die "value constraints are redundant when combined with enum\n";
      }

      if (@type_specific || defined($deferred{type}) || defined($deferred{enum})) {
         unshift @validators, prepare_type_check($self, \@type_specific, $deferred{type}, $deferred{enum});
      }

      my $validator = @validators > 1 ? bless \@validators, "Polymake::Schema::Validator::All" : $validators[0] // new True();
      if (@validators) {
         foreach my $ref_as ($path, $id) {
            if (defined($ref_as)) {
               $self->refs->{$ref_as} = $validator;
               delete $self->refs->{".UNDEF."}->{$ref_as};
            }
         }
      }
      return $validator unless defined($path);
   } while (($schema, $path) = splice @queue, 0, 2);
}
####################################################################################
sub prepare_type_check {
   my ($self, $type_specific, $allowed_types, $enum) = @_;
   my (@allowed, $single_type, $type_index);

   if (defined($allowed_types)) {
      if (is_array($allowed_types)) {
         foreach my $typename (@$allowed_types) {
            $allowed[$single_type = $type2index{$typename}] = true;
            if ($typename eq "integer") {
               $type_specific->[$single_type]->{integer} = true;
            }
         }
         undef $single_type if @$allowed_types > 1;
      } else {
         $allowed[$single_type = $type2index{$allowed_types}] = true;
         if ($allowed_types eq "integer") {
            $type_specific->[$single_type]->{integer} = true;
         }
      }
      if (defined($enum)) {
         foreach my $const (@$enum) {
            $type_index = data2type_index($const, true);
            if (!$allowed[$type_index]) {
               die "enum constant '$const' does not match any allowed data type\n";
            }
            push @{$type_specific->[$type_index]->{enum}}, $const;
         }
      }
      $type_index = 0;
      foreach my $args (@$type_specific) {
         if (defined($args)) {
            if ($allowed[$type_index]) {
               $args = $type2validator[$type_index]->new($self, $args);
            } else {
               die "type-specific element(s) [", join(", ", keys %$args), "] are incompatible with allowed type(s)\n";
            }
         }
         ++$type_index;
      }
   } else {
      my $type_cnt = 0;
      if (defined($enum)) {
         foreach my $const (@$enum) {
            $type_index = data2type_index($const, true);
            $allowed[$type_index] = true;
            push @{$type_specific->[$type_index]->{enum}}, $const;
         }
      } else {
         @allowed = (true) x ($type_cnt = @types);
      }
      $type_index = 0;
      foreach my $args (@$type_specific) {
         if (defined($args)) {
            if (++$type_cnt == 1) {
               $single_type = $type_index;
            } else {
               undef $single_type;
            }
            $args = $type2validator[$type_index]->new($self, $args);
         }
         ++$type_index;
      }
   }
   if (defined($single_type)) {
      $type_specific->[$single_type] // $type2validator[$single_type]->new($self)
   } else {
      $allowed[scalar @types] = $type_specific;
      bless \@allowed, "Polymake::Schema::Validator::MultiType"
   }
}
####################################################################################
sub combine_flat {
   my ($schema1, $schema2) = @_;
   if (is_boolean($schema1)) {
      return $schema1 && $schema2;
   }
   if (is_boolean($schema2)) {
      return $schema2 && $schema1;
   }
   if (is_array($schema1) && is_array($schema2)) {
      return [ map { combine_flat($schema1->[$_], $schema2->[$_]) } 0..min($#$schema1, $#$schema2) ];
   }
   if (is_array($schema1)) {
      return [ map { combine_flat($_, $schema2) } @$schema1 ];
   }
   if (is_array($schema2)) {
      return [ map { combine_flat($_, $schema1) } @$schema2 ];
   }

   my $properties1 = $schema1->{properties};
   my $properties2 = $schema2->{properties};

   if (is_defined_and_false($schema2->{additionalProperties})) {
      if (defined($properties1)) {
         foreach my $prop_name (keys %$properties1) {
            exists $properties2->{$prop_name} or delete $properties1->{$prop_name};
         }
      }
      $schema1->{additionalProperties} = false;
   }
   my $accept2 = ($schema1->{additionalProperties} //= $schema2->{additionalProperties} // true);

   if (defined($properties1) && defined($properties2)) {
      while (my ($prop_name, $prop_schema2) = each %$properties2) {
         if (defined(my $prop_schema1 = $properties1->{$prop_name})) {
            $properties1->{$prop_name} = combine_flat($prop_schema1, $prop_schema2);
         } elsif ($accept2) {
            $properties1->{$prop_name} = $prop_schema2;
         }
      }
   } elsif (defined($properties2)) {
      $schema1->{properties} = $properties2;
   }

   my $required1 = $schema1->{required};
   my $required2 = $schema2->{required};
   if (defined($required1) && defined($required2)) {
      $schema1->{required} = [ uniq(@$required1, @$required2) ];
   } elsif (defined($required2)) {
      $schema1->{required} = $required2;
   }

   $schema1
}

my $inject_into_flat = \(1);

####################################################################################
package Polymake::Schema::Validator::MultiType;

sub validate {
   my ($self, $data) = @_;
   my $type_index = data2type_index($data);
   if (defined($type_index) && $self->[$type_index]) {
      my $validator = $self->[-1]->[$type_index];
      $validator && $validator->validate($data)
   } else {
      " is not of an allowed type\n"
   }
}

sub flatten {
   my ($self, $data) = @_;
   if (is_code($data)) {
      # lazy computation of a property
      $data = $data->();
   }
   my $type_index = data2type_index($data);
   if (defined($type_index) && $self->[$type_index]) {
      my $validator = $self->[-1]->[$type_index];
      !defined($validator) || $validator->flatten($data)
   } else {
      undef
   }
}

sub list_property_completions {
   my $self = shift;
   my $type_specific;
   map { defined($type_specific = $self->[-1]->[$_]) ? $type_specific->list_property_completions(@_) : () } grep { $self->[$_] } @type2index{qw(array object)}
}

####################################################################################
package Polymake::Schema::Validator::Object;

use Polymake::Struct (
   [ new => '$%' ],
   [ '$minProperties' => '#%', default => 'undef' ],
   [ '$maxProperties' => '#%', default => 'undef' ],
   [ '$properties' => '#%', default => 'undef' ],
   [ '$additionalProperties' => '#%', default => 'undef' ],
   [ '$patternProperties' => '#%', default => 'undef' ],
   [ '$required' => '#%', default => 'undef' ],
   [ '$propertyNames' => '#%', default => 'undef' ],
   [ '$dependencies' => '#%', default => 'undef' ],
   '$iterate_over_data',
);

sub new {
   my $self = &_new;
   my $schema = $_[0];
   if (defined($self->additionalProperties)) {
      if (is_boolean($self->additionalProperties) && $self->additionalProperties) {
         undef $self->additionalProperties;
      } else {
         $self->additionalProperties = prepare($schema, $self->additionalProperties);
         $self->iterate_over_data = true;
      }
   }
   if (defined(my $properties = $self->patternProperties)) {
      my @patterns;
      while (my ($pattern, $prop_schema) = each %$properties) {
         push @patterns, [ qr/$pattern/, prepare($schema, $prop_schema) ];
      }
      $self->patternProperties = \@patterns;
      $self->iterate_over_data = true;
   }
   if (defined(my $name_schema = $self->propertyNames)) {
      $self->propertyNames = prepare($schema, $name_schema);
      $self->iterate_over_data = true;
   }
   if (defined(my $properties = $self->properties)) {
      if ($self->iterate_over_data) {
         my %props;
         while (my ($prop_name, $prop_schema) = each %$properties) {
            $props{$prop_name} = prepare($schema, $prop_schema);
         }
         $self->properties = \%props;
      } else {
         my @props;
         while (my ($prop_name, $prop_schema) = each %$properties) {
            push @props, [ $prop_name, prepare($schema, $prop_schema) ];
         }
         $self->properties = \@props;
      }
   }
   if (defined(my $dependencies = $self->dependencies)) {
      my @deps;
      while (my ($prop_name, $dep) = each %$dependencies) {
         push @deps, [ $prop_name, prepare($schema, is_hash($dep) ? $dep : { required => $dep }) ];
      }
      $self->dependencies = \@deps;
   }
   $self;
}

sub validate {
   my ($self, $data) = @_;
   if (!is_hash($data)) {
      return " is not an object\n";
   }
   if (defined($self->minProperties) && keys %$data < $self->minProperties) {
      return " number of properties below minimum ".$self->minProperties."\n";
   }
   if (defined($self->maxProperties) && keys %$data > $self->maxProperties) {
      return " number of properties above maximum ".$self->maxProperties."\n";
   }
   if (defined($self->required)) {
      foreach my $prop_name (@{$self->required}) {
         unless (exists $data->{$prop_name}) {
            return "/$prop_name missing required property\n";
         }
      }
   }
   my $error;
   if ($self->iterate_over_data) {
      while (my ($prop_name, $prop_value) = each %$data) {
         if (defined(my $validator = $self->properties && $self->properties->{$prop_name})) {
            if (defined($error = $validator->validate($prop_value))) {
               keys %$data;
               return "/$prop_name$error";
            }
         } else {
            my $matched;
            if (defined(my $pat_props = $self->patternProperties)) {
               foreach (@$pat_props) {
                  if ($prop_name =~ $_->[0]) {
                     if (defined($error = $_->[1]->validate($prop_value))) {
                        keys %$data;
                        return "/$prop_name$error";
                     }
                     $matched = true;
                     last;
                  }
               }
            }
            if (!$matched && defined($self->additionalProperties)) {
               if (defined($error = $self->additionalProperties->validate($prop_value))) {
                  keys %$data;
                  return "/$prop_name$error";
               }
            }
         }
         if (defined($self->propertyNames) && defined($error = $self->propertyNames->validate($prop_name))) {
            keys %$data;
            return "/$prop_name name$error";
         }
      }
   } elsif (defined(my $properties = $self->properties)) {
      foreach (@$properties) {
         my ($prop_name, $prop_validator) = @$_;
         if (exists($data->{$prop_name}) && defined($error = $prop_validator->validate($data->{$prop_name}))) {
            return "/$prop_name$error";
         }
      }
   }
   if (defined(my $dependencies = $self->dependencies)) {
      foreach (@$dependencies) {
         my ($prop_name, $dep_validator) = @$_;
         if (exists($data->{$prop_name}) && defined($error = $dep_validator->validate($data))) {
            return $error;
         }
      }
   }
   ()
}

sub flatten {
   my ($self, $data) = @_;
   if (is_code($data) || is_object($data)) {
      # atomic property should not be validated against a general schema
      return true;
   }

   my %flat;
   if (defined(my $properties = $self->properties)) {
      if ($self->iterate_over_data) {
         while (my ($prop_name, $prop_validator) = each %$properties) {
            if (defined(my $prop_value = $data->{$prop_name})) {
               unless (defined($flat{properties}->{$prop_name} = $prop_validator->flatten($prop_value))) {
                  keys %$properties;
                  return;
               }
            }
         }
      } else {
         foreach (@$properties) {
            my ($prop_name, $prop_validator) = @$_;
            if (defined(my $prop_value = $data->{$prop_name})) {
               unless (defined($flat{properties}->{$prop_name} = $prop_validator->flatten($prop_value))) {
                 return;
              }
            }
         }
      }
   }
   if (defined($self->additionalProperties)) {
      $flat{additionalProperties} = $self->additionalProperties->flatten($data);
   }
   if (defined($self->required)) {
      $flat{required} = $self->required;
   }

   my $result = keys %flat ? \%flat : true;

   if (defined(my $dependencies = $self->dependencies)) {
      foreach (@$dependencies) {
         my ($prop_name, $dep_validator) = @$_;
         if (exists $data->{$prop_name}) {
            defined($result = combine_flat($result, $dep_validator->flatten($data))) or last;
         }
      }
   }

   $result
}

sub list_property_completions {
   my ($self, $prop_name, @tail) = @_;
   my $properties = $self->properties;
   my @proposals;
   if (@tail) {
      if (is_hash($properties)) {
         if (defined(my $prop_validator = $properties->{$prop_name})) {
            push @proposals, map { "$prop_name.$_" } $prop_validator->list_property_completions(@tail);
         }
      } elsif (is_array($properties)) {
         foreach (@$properties) {
            if ($_->[0] eq $prop_name) {
               push @proposals, map { "$prop_name.$_" } $_->[1]->list_property_completions(@tail);
               last;
            }
         }
      }
      if (defined(my $dependencies = $self->dependencies)) {
         push @proposals, map { $_->[1]->list_property_completions($prop_name, @tail) } @$dependencies;
      }
   } else {
      if (is_hash($properties)) {
         push @proposals, grep { /^$prop_name/ } keys %$properties;
      } elsif (is_array($properties)) {
         push @proposals, grep { /^$prop_name/ } map { $_->[0] } @$properties;
      }
      if (defined(my $required = $self->required)) {
         push @proposals, grep { /^$prop_name/ } @$required;
      }
      if (defined(my $dependencies = $self->dependencies)) {
         foreach (@$dependencies) {
            if ($_->[0] =~  /^$prop_name/) {
               push @proposals, $_->[0];
            }
            push @proposals, $_->[1]->list_property_completions($prop_name);
         }
      }
   }
   @proposals;
}

####################################################################################
package Polymake::Schema::Validator::Array;

use Polymake::Struct (
   [ new => '$%' ],
   [ '$minItems' => '#%', default => 'undef' ],
   [ '$maxItems' => '#%', default => 'undef' ],
   [ '$items' => '#%', default => 'undef' ],
   [ '$additionalItems' => '#%', default => 'undef' ],
   [ '$contains' => '#%', default => 'undef' ],
   [ '$uniqueItems' => '#%', default => 'false' ],
);

sub new {
   my $self = &_new;
   my $schema = $_[0];
   if (is_hash($self->items)) {
      if (defined($self->additionalItems)) {
         die "additionalItems element without effect for a homogeneous array\n";
      }
      $self->additionalItems = prepare($schema, $self->items);
      undef $self->items;
   } elsif (is_array($self->items)) {
      my $explicit_items = scalar @{$self->items};
      $self->items = [ map { prepare($schema, $_) } @{$self->items} ];
      if (defined($self->maxItems) && $self->maxItems < $explicit_items) {
         die "maxItems conflicts with items: must be at least $explicit_items\n";
      }
      if (defined($self->additionalItems)) {
         if (defined($self->maxItems) && $self->maxItems == $explicit_items) {
            die "additionalItems element without effect until maxItems exceeds $explicit_items\n";
         }
         if (is_boolean($self->additionalItems)) {
            if (!$self->additionalItems) {
               $self->maxItems = $explicit_items;
            }
            undef $self->additionalItems;
         } else {
            $self->additionalItems = prepare($schema, $self->additionalItems);
         }
      }
   } else {
      if (defined($self->additionalItems)) {
         die "additionalItems element without effect for a homogeneous array\n";
      }
      undef $self->items;
   }
   if (defined($self->contains)) {
      $self->contains = prepare($schema, $self->contains);
   }
   $self
}

sub validate {
   my ($self, $data) = @_;
   if (!is_array($data)) {
      return " is not an array\n";
   }
   if (defined($self->minItems) && @$data < $self->minItems) {
      return " size below minimum ".$self->minItems."\n";
   }
   if (defined($self->maxItems) && @$data > $self->maxItems) {
      return " size above maximum ".$self->maxItems."\n";
   }
   my $item_validator = $self->items;
   my $tail_validator = $self->additionalItems;
   if (defined($item_validator) || defined($tail_validator)) {
      my $index = 0;
      my $last_item_validator = defined($item_validator) ? $#$item_validator : -1;
      foreach my $item (@$data) {
         if (defined(my $error = ($index <= $last_item_validator ? $item_validator->[$index] : $tail_validator // last)->validate($item))) {
            return "/$index$error";
         }
         ++$index;
      }
   }
   if (defined(my $validator = $self->contains)) {
      my $found;
      foreach my $item (@$data) {
         $found = !defined($validator->validate($item)) and last;
      }
      $found or return " does not contain required item\n";
   }
   if ($self->uniqueItems) {
      my %seen;
      foreach my $item (@$data) {
         if ($seen{$item}++) {
            my $printable = is_string($item) ? qq<"$item"> : is_boolean($item) ? to_boolean_string($item) : $item;
            return " multiple occurrence of item $printable\n";
         }
      }
   }
   ()
}

sub flatten {
   my ($self, $data) = @_;
   if (is_code($data) || is_object($data)) {
      # atomic property should not be validated against a general schema
      return true;
   }
   my $item_validator = $self->items;
   my $tail_validator = $self->additionalItems;
   my $cnt_valid = 0;
   my $max_valid = $self->maxItems // @$data;
   my @result;
   my $index = 0;
   my $last_item_validator = defined($item_validator) ? $#$item_validator : -1;

   foreach my $item (@$data) {
      my $item_result;
      if ($index <= $last_item_validator) {
         $item_result = $item_validator->[$index]->flatten($item);
      } elsif (defined($tail_validator)) {
         $item_result = $tail_validator->flatten($item);
      } else {
         my $append = min($max_valid - $cnt_valid, @$data - $index);
         push @result, (true) x $append;
         $cnt_valid += $append;
         last;
      }
      push @result, $item_result;
      if ($item_result) {
         last if ++$cnt_valid >= $max_valid;
      }
      ++$index;
   }

   $cnt_valid >= $self->minItems ? \@result : undef
}

sub list_property_completions {
   my $self = shift;
   my $item_validator = $self->items;
   my $tail_validator = $self->additionalItems;
   if (is_integer($_[0]) || $_[0] =~ /^\d+$/) {
      # requesting an element at concrete position
      my $index = shift;
      if (defined($item_validator) && $index <= $#$item_validator) {
         $item_validator->[$index]->list_property_completions(@_)
      } elsif (defined($tail_validator)) {
         $tail_validator->list_property_completions(@_)
      } else {
         ()
      }
   } else {
      # requesting any array element
      map { $_->list_property_completions(@_) }
          defined($item_validator) ? @$item_validator : (), defined($tail_validator) ? $tail_validator : ();
   }
}

####################################################################################
package Polymake::Schema::Validator::Scalar;

use Polymake::Struct (
   [ new => '$%' ],
   [ '$enum' => '#%', default => 'undef' ],
);

sub flatten {
   my ($self, $data) = @_;
   if ($data == $inject_into_flat) {
      return $self->enum;
   }
   !$self->has_constraints || !defined($self->validate(is_code($data) ? $data->() : $data)) || undef
}

sub list_property_completions { }

####################################################################################
package Polymake::Schema::Validator::Number;

use Polymake::Struct (
   [ '@ISA' => 'Scalar' ],
   [ '$integer' => '#%' ],
   [ '$minimum' => '#%', default => 'undef' ],
   [ '$exclusiveMinimum' => '#%', default => 'undef' ],
   [ '$maximum' => '#%', default => 'undef' ],
   [ '$exclusiveMaximum' => '#%', default => 'undef' ],
   [ '$multipleOf' => '#%', default => 'undef' ],
);

sub new {
   my $self = &Scalar::new;
   my $schema = $_[0];
   if ($schema->draft == 4) {
      if ($self->exclusiveMinimum) {
         $self->exclusiveMinimum = $self->minimum;
         undef $self->minimum;
      } else {
         undef $self->exclusiveMinimum;
      }
      if ($self->exclusiveMaximum) {
         $self->exclusiveMaximum = $self->maximum;
         undef $self->maximum;
      } else {
         undef $self->exclusiveMaximum;
      }
   }
   if (defined($self->multipleOf)) {
      if (is_integer($self->multipleOf)) {
         $self->integer = true;
      } elsif ($self->integer) {
         die "multipleOf value is not integral while integer type required\n";
      }
   }
   $self;
}

sub validate {
   my ($self, $data) = @_;
   if (!is_numeric($data) || is_boolean($data)) {
      return $self->integer ? " is not an integer\n" : " is not a number\n";
   }
   if ($self->integer && !is_integer($data)) {
      return " is not an integer\n";
   }
   if (defined($self->minimum) && $data < $self->minimum) {
      return " is below minimum ".$self->minimum."\n";
   }
   if (defined($self->exclusiveMinimum) && $data <= $self->exclusiveMinimum) {
      return " is not above exclusiveMinimum ".$self->exclusiveMinimum."\n";
   }
   if (defined($self->maximum) && $data > $self->maximum) {
      return " is above maximum ".$self->maximum."\n";
   }
   if (defined($self->exclusiveMaximum) && $data >= $self->exclusiveMaximum) {
      return " is not below exclusiveMaximum ".$self->exclusiveMaximum."\n";
   }
   if (defined($self->enum) && list_index($self->enum, $data) < 0) {
      return " is not among allowed values\n";
   }
   if (defined($self->multipleOf) &&
       ($self->integer ? $data % $self->multipleOf : POSIX::fmod($data, $self->multipleOf)) != 0) {
      return " is not multiple of ".$self->multipleOf."\n";
   }
   ()
}

sub has_constraints {
   my ($self) = @_;
   defined($self->enum) ||
   defined($self->minimum) || defined($self->exclusiveMinimum) ||
   defined($self->maximum) || defined($self->exclusiveMaximum) ||
   defined($self->multipleOf)
}

####################################################################################
package Polymake::Schema::Validator::Boolean;

use Polymake::Struct (
   [ '@ISA' => 'Scalar' ],
);

sub validate {
   my ($self, $data) = @_;
   if (!is_boolean($data)) {
      return " is not a boolean\n";
   }
   if (defined($self->enum) && list_index($self->enum, $data) < 0) {
      return " is not among allowed values\n";
   }
   ()
}

sub has_constraints {
   defined($_[0]->enum)
}

####################################################################################
package Polymake::Schema::Validator::String;

use Polymake::Struct (
   [ '@ISA' => 'Scalar' ],
   [ '$pattern' => '#%', default => 'undef' ],
   [ '$format' => '#%', default => 'undef' ],
   [ '$minLength' => '#%', default => 'undef' ],
   [ '$maxLength' => '#%', default => 'undef' ],
);

sub new {
   my $self = &Scalar::new;
   if (defined(my $pattern = $self->pattern)) {
      $self->pattern = qr/$pattern/;
   }
   $self
}

sub validate {
   my ($self, $data) = @_;
   if (!is_string($data)) {
      return " is not a string\n";
   }
   if (defined($self->minLength) && length($data) < $self->minLength) {
      return " is shorter than minimum ".$self->minLength."\n";
   }
   if (defined($self->maxLength) && length($data) > $self->maxLength) {
      return " is longer than maximum ".$self->maxLength."\n";
   }
   if (defined($self->pattern) && $data !~ $self->pattern) {
      return " does not match the pattern\n";
   }
   if (defined($self->enum) && string_list_index($self->enum, $data) < 0) {
      return " is not among allowed values\n";
   }
   ()
}

sub has_constraints {
   my ($self) = @_;
   defined($self->enum) || defined($self->pattern) ||
   defined($self->minLength) || defined($self->maxLength)
}

####################################################################################
package Polymake::Schema::Validator::Null;

sub new {
   state $instance = bless \(my $x);
}

sub validate {
   if (defined($_[1])) {
      return " is not null\n";
   }
   ()
}

sub flatten {
   my ($self, $data) = @_;
   !defined(is_code($data) ? $data->() : $data) || undef;
}

sub list_property_completions { }

####################################################################################
package Polymake::Schema::Validator::True;

sub new {
   state $instance = bless \(my $x);
}

sub validate { () }

sub flatten { true }

sub list_property_completions { }

####################################################################################
package Polymake::Schema::Validator::False;

sub new {
   state $instance  = bless \(my $x);
}

sub validate { " is not allowed\n" }

sub flatten { false }

sub list_property_completions { }

####################################################################################
package Polymake::Schema::Validator::Ref;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$schema' => 'weak(#1)' ],
   [ '$ref' => '#2' ],
);

declare $exempt_flat = qr/(?!.)/;

sub new {
   (undef, my ($schema, $ref)) = @_;
   if (defined(my $refto = $schema->refs->{$ref})) {
      return $refto;
   }
   if ((my $fragment = index($ref, '#')) != 0) {
      my $other_schema = new_by_URI Schema($fragment > 0 ? substr($ref, 0, $fragment) : $ref, $schema);
      $other_schema->validator //= create($other_schema);
      return $schema->refs->{$ref} = $other_schema->refs->{$fragment > 0 ? substr($ref, $fragment) : '#'}
                                     // die "invalid reference to a non-existing schema $ref\n";
   }
   my $self = &_new;
   $self->schema->refs->{".UNDEF."}->{$self->ref} = true;
   $self
}

sub validate {
   my ($self, $data) = @_;
   $self->schema->refs->{$self->ref}->validate($data);
}

sub flatten {
   my ($self, $data) = @_;
   $self->ref =~ $exempt_flat or
     $self->schema->refs->{$self->ref}->flatten($data)
}

sub list_property_completions {
   my $self = shift;
   $self->schema->refs->{$self->ref}->list_property_completions(@_);
}

####################################################################################
package Polymake::Schema::Validator::Not;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$subschema' => 'prepare(#1, #2)' ],
);

sub validate {
   my ($self, $data) = @_;
   defined($self->subschema->validate($data)) ? () : " satisfies negated subschema\n"
}

sub flatten {
   my ($self, $data) = @_;
   !$self->subschema->flatten($data) || undef;
}

sub list_property_completions { }

####################################################################################
package Polymake::Schema::Validator::Any;

sub new {
   my ($pkg, $schema, $branches) = @_;
   if (@$branches > 1) {
      my @validators = map { prepare($schema, $_) } @$branches;
      my (@allowed_types, @validators_by_type);
      my $add_validator = sub {
         my ($validator, $type_index) = @_;
         if (defined(my $other_validator = $validators_by_type[$type_index])) {
            if (is_object($other_validator)) {
               $validators_by_type[$type_index] = [ $other_validator, $validator ];
            } else {
               push @$other_validator, $validator;
            }
         } else {
            $validators_by_type[$type_index] = $validator;
         }
      };
      foreach my $validator (@validators) {
         if (defined(my $type_index = $validator2type_index{ref($validator)})) {
            $allowed_types[$type_index] = true;
            if ($type_index != $type2index{null}) {
               $add_validator->($validator, $type_index);
            }
         } elsif (instanceof MultiType($validator)) {
            foreach my $type_index (0..$#types) {
               if ($validator->[$type_index]) {
                  $allowed_types[$type_index] = true;
                  $add_validator->($validator->[-1]->[$type_index] // new True(), $type_index);
               }
            }
         } else {
            @allowed_types = ();
            last;
         }
      }
      if (1 < grep { $_ } @allowed_types) {
         foreach my $validator (@validators_by_type) {
            if (is_array($validator)) {
               bless $validator, $pkg;
            } elsif (ref($validator) eq "Polymake::Schema::Validator::True") {
               $validator = undef;
            }
         }
         $allowed_types[scalar @types] = \@validators_by_type;
         bless \@allowed_types, "Polymake::Schema::Validator::MultiType"
      } else {
         bless \@validators, $pkg
      }
   } else {
      prepare($schema, $branches->[0])
   }
}

sub validate {
   my ($self, $data) = @_;
   my @errors;
   foreach (@$self) {
      if (defined(my $error = $_->validate($data))) {
         push @errors, $error;
      } else {
         return ();
      }
   }
   " does not satisfy any subschema:\n" . join("", map { $_.": ".$errors[$_] } 0..$#errors)
}

sub flatten {
   my ($self, $data) = @_;
   my $result;
   foreach (@$self) {
      $result = $_->flatten($data) and last;
   }
   $result
}

sub list_property_completions {
   my $self = shift;
   map { $_->list_property_completions(@_) } @$self
}

####################################################################################
package Polymake::Schema::Validator::One;

*new = \&Any::new;

sub validate {
   my ($self, $data) = @_;
   my @errors;
   my $satisfied = 0;
   foreach (@$self) {
      if (defined(my $error = $_->validate($data))) {
         push @errors, $error;
      } elsif ($satisfied++) {
         return " satisfies more than one subschema\n";
      }
   }
   $satisfied ? () : " does not satisfy any subschema:\n" . join("", map { $_.": ".$errors[$_] } 0..$#errors)
}

*flatten = \&Any::flatten;
*list_property_completions = \&Any::list_property_completions;

####################################################################################
package Polymake::Schema::Validator::All;

sub new {
   my ($pkg, $schema, $branches) = @_;
   if (@$branches > 1) {
      bless [ map { prepare($schema, $_) } @$branches ], $pkg;
   } else {
      prepare($schema, $branches->[0])
   }
}

sub validate {
   my ($self, $data) = @_;
   foreach (@$self) {
      if (defined(my $error = $_->validate($data))) {
         return $error;
      }
   }
   ()
}

sub flatten {
   my ($self, $data) = @_;
   my $result = true;
   foreach (@$self) {
      if (defined(my $member_result = $_->flatten($data))) {
         $result = combine_flat($result, $member_result) or last;
      } else {
         return;
      }
   }
   $result
}

*list_property_completions = \&Any::list_property_completions;

####################################################################################
package Polymake::Schema::Validator::If;

use Polymake::Struct (
   [ new => '$$$$' ],
   [ '$cond' => 'prepare(#1, #2)' ],
   [ '$then_branch' => '#3 && prepare(#1, #3)' ],
   [ '$else_branch' => '#4 && prepare(#1, #4)' ],
);

sub validate {
   my ($self, $data) = @_;
   my $branch = $self->cond->validate($data) ? $self->else_branch : $self->then_branch;
   $branch && $branch->validate($data)
}

sub flatten {
   my ($self, $data) = @_;
   my $branch = do {
      $self->cond->flatten($data) ? $self->then_branch : $self->else_branch
   };
   !defined($branch) || $branch->flatten($data)
}

sub list_property_completions {
   my $self = shift;
   map { $_->list_property_completions(@_) } grep { defined } $self->cond, $self->then_branch, $self->else_branch
}

####################################################################################
package Polymake::Schema::Validator::MergeIntoFlat;

sub TIEHASH {
   my $pkg = shift;
   bless [ @_ ], $pkg;
}

sub FETCH {
   my ($self, $key) = @_;
   if ($key =~ $self->[0]) {
      if (@$self > 1) {
         merge_into_flat(@$self[1..$#$self]);
      } else {
         $inject_into_flat
      }
   } else {
      undef
   }
}

sub Polymake::Schema::Validator::merge_into_flat {
   my %dummy;
   tie %dummy, __PACKAGE__, @_;
   \%dummy;
}

1;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
