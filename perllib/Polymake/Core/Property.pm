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

package Polymake::Core::Property;

#  Constructor
#
#  new Property('name', $type, BigObjectType, options);

use Polymake::Struct (
   [ 'new' => '$$$%' ],
   [ '$name | property_name' => '#1' ],   # 'property name'
   [ '$defined_for' => '#3' ],    # BigObjectType where the property definition appeared, common for all clones and aliases
   [ '$belongs_to' => '#3' ],     # BigObjectType owning this instance
   [ '$application' => 'undef' ], # Application where it's defined (if != defined_for->application)
   [ '$extension' => '$Extension::loading' ],
   [ '$type' => '#2' ],           # PropertyType | BigObjectType
   [ '$flags' => '#%', default => '0' ],

                                  # the following fields contain \&sub or "method name",
                                  #   methods are called via the BigObject
   [ '$canonical' => '#%', default => 'undef' ],
   [ '$equal' => '#%', default => 'undef' ],

                                  # other properties needed to construct the value
   [ '$construct' => '#%', default => 'undef' ],

   '&accept',                     # value, object, trusted_flag =>
   '&copy',                       # value, object =>

                                    # for aliases defined in derived objects:
   [ '$overrides' => 'undef' ],     # "PropName" original property overridden by this one
   [ '$overrides_for' => 'undef' ], # BigObjectType where the overriding alias is defined

   '%key | property_key',         # The address of the hash itself serves as a search key in the objects' dictionaries,
                                  # production rule lists, permutation sensitivity lists, etc. - all kinds of lists where
                                  # all properties being concrete type instances or aliases of the same root property have
                                  # to be matched to the same data.
                                  # Contained is a hierarchy of further hashes: the keys belong to subobject properties, and
                                  # the values serve again as search keys in the lists of producers.
                                  # For example, if the object type O has a rule R = A.B : X,
                                  # then there is a hash $H=$Property_B->key->{$Property_A->key}
                                  # such that the list $ObjectType_O->producers->{$H} contains $Rule_R.

   [ '$new_instance_deputy' => 'undef' ],  # NewMultiInstance for multiple properties
);

use Polymake::Enum Flags => {
   is_mutable => 0x1,
   is_subobject => 0x2,
   is_augmented => 0x4,
   is_multiple => 0x8,
   is_multiple_new => 0x10,
   is_permutation => 0x20,
   is_non_storable => 0x40,
   is_subobject_array => 0x80,
   is_produced_as_whole => 0x100,
   in_restricted_spez => 0x200,
   is_concrete => 0x400,
   is_twin => 0x800
};

sub new {
   my $self=&_new;
   if (instanceof BigObjectType($self->type)) {
      $self->flags |= Flags::is_subobject;
   }
   if ($enable_plausibility_checks) {
      if ($self->flags & Flags::is_subobject) {
         if ($self->flags & Flags::is_non_storable) {
            croak( "only atomic properties can be declared non-storable" );
         }
         if ($self->flags & Flags::is_twin) {
            if ($self->flags & Flags::is_multiple) {
               croak( "a twin property may not be declared as multiple" );
            }
            if (!$self->defined_for->isa($self->type)) {
               unless ($self->defined_for->abstract && $self->type->abstract &&
                       $self->type->context_pkg eq $self->defined_for->pkg) {
                  croak( "twin property type must be the same as the type of the enclosing big object or one of its base types" );
               }
            }
         }
      } else {
         if ($self->flags & (Flags::is_multiple | Flags::is_twin)) {
            croak( "an atomic property may not be declared as ", $self->flags & Flags::is_multiple ? "multiple" : "twin");
         }
      }
   }

   $self->construct &&= [ map { $self->belongs_to->encode_property_path($_) } split /\s*,\s*/, trim_spaces($self->construct) ];

   $self->flags |= Flags::is_concrete unless $self->type->abstract;
   choose_methods($self);

   if ($self->flags & Flags::is_multiple) {
      my $spez_type = $self->belongs_to;
      while ($spez_type->enclosed_in_restricted_spez) {
         $spez_type = $spez_type->parent_property->belongs_to;
      }
      if (defined $spez_type->preconditions) {
         $self->flags |= Flags::in_restricted_spez;
      }
      $self->new_instance_deputy=new NewMultiInstance($self, { });
   }

   $self;
}
####################################################################################
sub clone {
   my ($src) = @_;
   my $self = inherit_class([ @$src ], $src);
   if ($src->new_instance_deputy) {
      $self->new_instance_deputy = new NewMultiInstance($self, $src->new_instance_deputy->key);
   }
   $self
}
####################################################################################
sub override_by {
   my ($src, $name, $owner, $new_type) = @_;
   my $self = &clone;
   $self->name = $name;
   $self->belongs_to = $owner;
   unless ($self->flags & Flags::is_twin) {
      $self->overrides_for = $owner;
      $self->overrides = $src->name;
   }
   if (defined($new_type)) {
      $self->type = $new_type;
      if ($new_type->abstract) {
         $self->flags &= ~Flags::is_concrete;
      } else {
         $self->flags |= Flags::is_concrete;
      }
      choose_methods($self);
   }
   $self;
}
####################################################################################
# find the property instance suitable for the given object type,
# taking into account overrides and augmentations
# Property, BigObjectType, downcast_flag => Property
sub instance_for_owner {
   my ($self, $proto, $down) = @_;
   if ($self->belongs_to == $proto) {
      $self
   } elsif ($down) {
      $proto->property($self->name)
   } elsif ($proto->isa($self->defined_for)) {
      until ($proto->isa($self->belongs_to)) {
         $self = $proto->lookup_property($self->name) //
                 (defined($self->overrides) &&
                  $self->belongs_to->lookup_overridden_property($self)
                    or croak( "internal error: can't find an instance of property ", $self->name,
                              " for object type ", $proto->full_name ));
      }
      $self
   } else {
      undef
   }
}
####################################################################################
sub analyze {
   my ($self, $pkg) = @_;
   if ($self->flags & Flags::is_subobject) {
      if ($self->flags & Flags::is_twin) {
         croak( "a twin property cannot be augmented" );
      }
      $self->belongs_to->augment($self);
   } else {
      namespaces::using($pkg, $self->type->pkg);
      my $symtab = get_symtab($pkg);
      foreach (qw(canonical equal)) {
         if (exists &{$symtab->{$_}}) {
            $self->$_=\&{$symtab->{$_}};
         }
      }
   }
}
####################################################################################
sub change_to_augmented {
   my ($self, $augm) = @_;
   $self->type = $augm;
   if ($self->belongs_to->abstract) {
      $self->flags &= ~Flags::is_concrete;
   } else {
      $self->flags |= Flags::is_concrete;
   }
   $self->flags |= Flags::is_augmented;
   choose_methods($self);
   if ($self->new_instance_deputy) {
      $self->new_instance_deputy->update_flags;
   }
   weak($augm->parent_property = $self);
}
####################################################################################
sub clone_for_augmented {
   my ($src, $augm, $proto) = @_;
   my $self = &clone;
   $self->belongs_to = $proto;
   change_to_augmented($self, $augm);
   $self
}
####################################################################################
sub clone_for_owner {
   my ($src, $proto)=@_;
   my $self=&clone;
   $self->belongs_to=$proto;
   $self->type=$src->type->concrete_type($proto);
   if ($self->flags & Flags::is_augmented) {
      weak($self->type->parent_property = $self);
   }
   if (!$proto->abstract) {
      $self->flags |= Flags::is_concrete;

      if ($self->flags & Flags::is_twin && $self->type != $proto) {
         my $twin_prop=$self->type->property($self->name);
         if (!$proto->isa($twin_prop->type)) {
            die "invalid twin property ", $proto->full_name, "::", $self->name,
              ": back-reference type ", $twin_prop->full_name, " does not match the owner object\n";
         }
      }
   }
   if ($self->new_instance_deputy) {
      $self->new_instance_deputy->update_flags;
   }
   choose_methods($self);
   $self
}
####################################################################################
sub choose_methods {
   my ($self) = @_;
   if ($self->flags & Flags::is_concrete) {
      if ($self->flags & Flags::is_subobject) {
         $self->accept = \&accept_subobject;
         $self->copy = undef;
      } elsif ($self->type->super == typeof BigObjectArray) {
         $self->flags |= Flags::is_subobject_array;
         $self->accept = \&accept_subobject_array;
         $self->copy = \&copy_subobject_array;
      } elsif (defined $self->construct) {
         $self->accept = \&accept_special_constructed;
         $self->copy = \&copy_special_constructed;
      } elsif ($self->type->cppoptions && !$self->type->cppoptions->builtin) {
         $self->accept = \&accept_composite;
         $self->copy = \&copy_composite;
      } else {
         $self->accept = \&accept_builtin;
         $self->copy = \&copy_builtin;
      }
   } else {
      $self->accept=sub : method {
         my $prop=shift;
         $_[1]->type->property($prop->name)->accept->(@_);
      };
      unless ($self->flags & Flags::is_subobject) {
         $self->copy = sub : method {
            my $prop = shift;
            $_[1]->type->property($prop->name)->copy->(@_);
         };
      }
   }
}
####################################################################################
sub accept_subobject : method {
   my ($self, $value, $parent_obj, $trusted, $temp)=@_;
   my $subobj_type=$self->subobject_type;
   unless (defined $value) {
      if (!$trusted and $self->flags & (Flags::is_multiple | Flags::is_subobject_array | Flags::is_twin)) {
         croak( "undefined value not allowed for the ", ($self->flags & Flags::is_twin ? "twin object" : "multiple subobject"),
                " property ", $self->name );
      }
      return new PropertyValue($self, $value);
   }

   unless ($trusted) {
      is_object($value) or croak( "property ", $self->name, " needs a BigObject\n" );
      unless (UNIVERSAL::isa($value, $subobj_type->pure_type->pkg)) {
         # if the source object can't be converted to our type, it will croak():
         $value=$subobj_type->pure_type->construct->($value);
      }
   }

   if (defined($value->parent) || defined($value->persistent)) {
      $value=$value->copy($parent_obj, $self);
   } else {
      $value->property=$self;
      guarded_weak($value->parent=$parent_obj, $value, \&BigObject::forget_parent_property);
      if ($self->flags & Flags::is_augmented && (my $given_type=$value->type) != $subobj_type) {
         $value->perform_cast($subobj_type->final_type($given_type), 1);
      }
   }

   $value->is_temporary= $temp || defined($parent_obj->parent) && $parent_obj->is_temporary;

   if (defined($value->transaction)) {
      if ($value->has_cleanup) {
         Transaction::merge_temporaries($value->transaction->temporaries, delete $Scope->cleanup->{$value});
         $value->has_cleanup=0;
      }
      if (defined($parent_obj->transaction)) {
         $parent_obj->transaction->descend($value, 1) unless ($temp && defined($parent_obj->transaction->rule));
      } else {
         $value->transaction->commit($value);
      }
   } elsif (defined($parent_obj->transaction)) {
      unless ($temp && defined($parent_obj->transaction->rule)) {
         ## FIXME: suspicious logical expression
         if (not($self->flags & Flags::is_subobject_array) || $value->has_cleanup) {
            $parent_obj->transaction->descend($value, !($self->flags & Flags::is_permutation));
            if ($value->has_cleanup) {
               $value->transaction->temporaries = delete $Scope->cleanup->{$value};
               $value->has_cleanup = 0;
            }
         }
      }
   }

   if ($self->flags & Flags::is_twin) {
      $value->add_twin_backref($self);
   }
   $value
}

# parent BigObject, temporary flag => empty Object
sub create_subobject : method {
   my $self=instance_for_owner($_[0], $_[1]->type, 1);
   accept_subobject($self, BigObject::new($self->subobject_type->pkg), $_[1], 1, $_[2]);
}

sub subobject_type {
   my ($self)=@_;
   $self->flags & Flags::is_subobject_array ? $self->type->params->[0] : $self->type;
}
####################################################################################
sub accept_subobject_array : method {
   my ($self, $value, $parent_obj, $trusted, $temp)=@_;
   if (ref($value) eq "ARRAY") {
      bless $value, $self->type->pkg;
   } elsif (!$trusted && !$self->type->isa->($value)) {
      croak( ref($value) || "'$value'", " passed as property ", $self->name, " while ", $self->type->full_name, " expected" );
   }
   foreach my $subobj (@$value) {
      accept_subobject($self, $subobj, $parent_obj, $trusted, $temp);
   }
   new PropertyValue($self, $value, $temp);
}

sub copy_subobject_array : method {
   my ($self, $value, $parent_obj)=@_;
   my $index=-1;
   new PropertyValue($self, bless([ map { $_->copy($parent_obj, $self) } @$value ], $self->type->pkg));
}
####################################################################################
sub accept_composite : method {
   my ($self, $value, $parent_obj, $trusted, $temp) = @_;
   if (defined $value) {
      my $needs_canonicalization = !$trusted && defined($self->canonical);
      my ($is_object, $type_mismatch);
      if (!($is_object = is_object($value)) or
          $type_mismatch = !$self->type->isa->($value) or
          $self->type->cppoptions && !$self->type->cppoptions->builtin &&
          CPlusPlus::must_be_copied($value, $temp, $needs_canonicalization)) {
         my $target_type = $type_mismatch
                           ? $self->type->coherent_type->($value)
                           : $is_object && $value->type;
         local $PropertyType::trusted_value = $trusted;
         $value=($target_type || $self->type)->construct->($value);
      }
      if ($needs_canonicalization) {
         select_method($self->canonical, $parent_obj, 1)->($value);
      }
   }
   new PropertyValue($self, $value, $temp);
}

sub copy_composite : method {
   my ($self, $value, $parent_obj, $type_mismatch) = @_;
   my $target_type = is_object($value) &&
                     ($type_mismatch && !$self->type->isa->($value)
                      ? $self->type->coherent_type->($value)
                      : $value->type);
   new PropertyValue($self, ($target_type || $self->type)->construct->($value));
}
####################################################################################
sub accept_special_constructed : method {
   my ($self, $value, $parent_obj, $trusted, $temp) = @_;
   if (defined($value)) {
      my $needs_canonicalization = !$trusted && defined($self->canonical);
      if (!is_object($value) or
          !$self->type->isa->($value) or
          $self->type->cppoptions && !$self->type->cppoptions->builtin &&
          CPlusPlus::must_be_copied($value,$temp,$needs_canonicalization)) {
         local $PropertyType::trusted_value = $trusted;
         $value = $self->type->construct->((map { $parent_obj->value_at_property_path($_) } @{$self->construct}), $value);
      }
      if ($needs_canonicalization) {
         select_method($self->canonical, $parent_obj, 1)->($value);
      }
   }
   new PropertyValue($self, $value, $temp);
}

sub copy_special_constructed : method {
   my ($self, $value, $parent_obj)=@_;
   new PropertyValue($self, $self->type->construct->((map { $parent_obj->value_at_property_path($_) } @{$self->construct}), $value));
}
####################################################################################
sub accept_builtin : method {
   my ($self, $value, $parent_obj, $trusted, $temp)=@_;
   if (defined $value) {
      local $PropertyType::trusted_value = $trusted;
      $value = $self->type->construct->($value);
      if (!$trusted && defined($self->canonical)) {
         select_method($self->canonical, $parent_obj, 1)->($value);
      }
   }
   new PropertyValue($self, $value, $temp);
}

sub copy_builtin : method {
   my ($self, $value, $parent_obj, $type_mismatch)=@_;
   new PropertyValue($self, $type_mismatch ? $self->type->construct->($value) : $value);
}
####################################################################################
sub qualified_name {
   my ($self) = @_;
   $self->defined_for->qualified_name . "." . $self->name
}

*required_extensions = \&PropertyType::required_extensions;

sub print_path {
   my ($path)=@_;
   is_object($path->[0]) ? join(".", map { $_->name } @$path) : join(" | ", map { join(".", map { $_->name } @$_) } @$path)
}

# => length of the prefix with equal property keys
sub equal_path_prefixes {
   my ($path1, $path2)=@_;
   my $l=min(scalar(@$path1), scalar(@$path2));
   for (my $i=0; $i<$l; ++$i) {
      $path1->[$i]->key == $path2->[$i]->key or return $i;
   }
   $l
}
####################################################################################
# [ Property ], flag => index of a first property with given flag; -1 if not found
sub find_first_in_path {
   my ($path, $flag)=@_;
   my $i=0;
   foreach (@$path) {
      return $i if $_->flags & $flag;
      ++$i;
   }
   -1
}

sub find_last_in_path {
   my ($path, $flag)=@_;
   my $i=$#$path;
   foreach (reverse @$path) {
      return $i if $_->flags & $flag;
      --$i;
   }
   -1
}
####################################################################################
package Polymake::Core::Property::SelectMultiInstance;

# used only in requests and 'down' paths processed by Scheduler

use Polymake::Struct (
   [ new => '$$' ],
   [ '$property' => '#1' ],
   [ '$index' => '#2' ],
);

sub key { $_[0]->property->key }
*property_key=\&key;
sub property_name { $_[0]->property->name }
sub type { $_[0]->property->type }
sub flags { Flags::is_multiple }

sub name {
   my ($self)=@_;
   $self->property->name . "[" . (is_object($self->index) ? $self->index->header : $self->index) . "]"
}

package Polymake::Core::Property;

sub index { 0 }           # by default, the 0-th multiple subobject instance is selected
sub property { $_[0] }

####################################################################################
package Polymake::Core::Property::SubobjKey;
RefHash::allow(__PACKAGE__);

my $prop_key=\(1);
my $subobj_key=\(2);
my $defined_for_key=\(3);
my $in_twins_key=\(4);

sub new {
   my ($pkg, $prop, $prop_down, $subobj_prop)=@_;
   # As long as we are ascending from the property in question along an augmentation path,
   # the `defined_for' attribute should refer to the scope enclosing the augmentation.
   my $defined_for=$prop_down->defined_for;
   $defined_for &&= $defined_for->parent_property;
   $defined_for &&= $defined_for->belongs_to;
   $defined_for //= $subobj_prop->defined_for;
   my $self=bless { $prop_key => $prop, $subobj_key => $subobj_prop, $defined_for_key => $defined_for }, $pkg;
   weak($_) for values %$self;
   $self;
}

sub key { $_[0] }
sub property { $_[0]->{$prop_key} }
sub subobject_property { $_[0]->{$subobj_key} }
sub defined_for { $_[0]->{$defined_for_key} }
sub property_key { &property->key }
*produced_in_twins=\&Property::produced_in_twins;

####################################################################################
package Polymake::Core::Property;

# protected: used solely from Rule::finalize
# ancestor Properties come in reverse (bottom-up) order!
sub get_prod_key {
   my $self=shift;
   my $hash=$self->key;
   my $this_level=$self;
   foreach (@_) {
      $this_level=$hash=($hash->{$_->key} //= new SubobjKey($self, $this_level, $_));
   }
   $hash
}

sub memorize_produced_in_twin {
   my ($self, $hash)=@_;
   $hash->{$in_twins_key}->{$self->key} //= $self;
}

sub produced_in_twins {
   my ($self)=@_;
   if (defined (my $twins=$self->key->{$in_twins_key})) {
      values %$twins
   } else {
      ()
   }
}

####################################################################################
package Polymake::Core::Property::NewMultiInstance;

# used only in 'out' paths of Rules creating new instances of multiple subobjects

use Polymake::Struct (
   [ new => '$$' ],
   [ '$property' => 'weak(#1)' ],
   [ '$key' => '#2' ],
   [ '$name' => '#1->name . "(new)"' ],
   [ '$flags' => '#1->flags | Flags::is_multiple_new' ],
   [ '$defined_for' => '#1->defined_for' ],
   [ '$belongs_to' => '#1->belongs_to' ],
   [ '$property_key' => '#1->property_key' ],
);

sub property_name { $_[0]->property->name }
sub type { $_[0]->property->type }
sub index { 0 }

sub update_flags {
   my ($self)=@_;
   $self->flags = $self->property->flags | Flags::is_multiple_new;
}

*get_prod_key=\&Property::get_prod_key;

sub create_subobject : method {
   my ($self, $parent_obj, $temp)=@_;
   my $subobj=$self->property->create_subobject($parent_obj, $temp);
   my $pv=new PropertyValue::Multiple($subobj->property, [ $subobj ]);
   $pv->created_by_rule={ ($parent_obj->transaction->rule, 0) };
   $pv->ensure_unique_name($subobj, $temp);
   $pv;
}

sub instance_for_owner {
   my ($self, $proto, $down)=@_;
   $self->belongs_to==$proto ? $self : $self->property->instance_for_owner($proto, $down)->new_instance_deputy;
}

####################################################################################
package Polymake::Core::Property::Path;

# path between two (sub-)objects in an object tree

use Polymake::Struct (
   [ new => '$@' ],
   [ '$up' => '#1' ],     # how many parent levels to ascend
   [ '@down' => '@' ],    # properties to descend
);

sub toString {
   my ($self)=@_;
   join(".", ("parent") x $self->up, map { $_->name } @{$self->down})
}

# => ±N ; N>0 - descending, N<0 - ascending
sub level_change {
   my ($self)=@_;
   @{$self->down} - $self->up
}

# Path, Property, ... => new Path
sub deeper {
   my $self=shift;
   inherit_class([ $self->up, [ @{$self->down}, @_ ]], $self)
}

# Path, +N => new Path
sub higher {
  my ($self, $up)=@_;
  my $diff=@{$self->down}-$up;
  inherit_class($diff > 0 ? [ $self->up, [ @{$self->down}[0..$diff-1] ]] : [ $self->up-$diff, [ ]], $self)
}

# Path, Property at ancestor, ... => new Path
sub prepend {
   my $self=shift;
   my $diff=@_-$self->up;
   inherit_class($diff > 0 ? [ 0, [ @_[0..$diff-1], @{$self->down} ]] : [ -$diff, [ @{$self->down} ]], $self)
}

# construct a property path leading from one subobject to another
# => (±N, Property...)  N<0 - self is a descendant of other,  N>0 - self is ancestor of other
# => undef  when there is no such path
sub find_relative_path {
   my ($self, $other, $object)=@_;
   my $up1=$self->up;
   my $up2=$other->up;
   my ($diff, @path);
   if (@{$self->down}) {
      if (@{$other->down}) {
         return if $up1!=$up2;
         # starts in the same node: check whether the shorter path is the subset of the longer one
         my $d1=@{$self->down};
         my $d2=@{$other->down};
         $diff= $d2<=>$d1;
         return if !$diff;
         ($self, $d1, $other, $d2)=($other, $d2, $self, $d1) if $diff<0;
         for (my $i=0; $i<$d1; ++$i) {
            return if $self->down->[$i]->key != $other->down->[$i]->key;
         }
         return ($diff, @{$other->down}[$d1..$d2-1]);

      } else {
         return if $up1>$up2;
         $diff=-1;
         @path=@{$self->down};
      }

   } elsif (@{$other->down}) {
      return if $up2>$up1;
      $diff=1;
      @path=@{$other->down};

   } else {
      # both sit on the direct ascending path from the root object
      $diff= $up1<=>$up2;
      return if !$diff;
   }

   ($up1, $up2)=($up2, $up1) if $diff<0;
   while ($up2>0) { $object=$object->parent; --$up2; --$up1; }
   while ($up1>0) { unshift @path, $object->property; $object=$object->parent; --$up1; }

   ($diff, @path);
}


1

# Local Variables:
# cperl-indent-level: 3
# indent-tabs-mode:nil
# End:
