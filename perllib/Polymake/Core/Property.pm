#  Copyright (c) 1997-2015
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

use strict;
use namespaces;

package Polymake::Core::Property;

#  Constructor
#
#  new Property('Name', $type, ObjectType, options);

use Polymake::Struct (
   [ 'new' => '$$$%' ],
   [ '$name' => '#1' ],           # 'property name'
   [ '$belongs_to' => '#3' ],     # ObjectType
   [ '$application' => 'undef' ], # Application where it's defined (if != belongs_to->application)
   [ '$extension' => '$Application::extension' ],
   [ '$type' => '#2' ],           # PropertyType | ObjectType
   [ '$flags' => '#%', default => '0' ],

                                  # the following fields contain \&sub or "method name",
                                  #   methods are called via the Object
   [ '$canonical' => '#%', default => 'undef' ],
   [ '$equal' => '#%', default => 'undef' ],

                                  # other property or path to a subobject property needed to construct the value
   [ '$construct' => '#%', default => 'undef' ],

   '&accept',                     # value, object, trusted_flag =>
   '&copy',                       # value, object =>

                                  # for abstract types only:
   [ '$inst_cache' => 'undef' ],  #  concrete ObjectType => cloned Property
   [ '$cloned_from' => 'undef' ], #  back reference from a concrete clone to an abstract Property
                                  # for aliases defined in derived objects:
   [ '$overrides' => 'undef' ],   #  [ Property, ObjectType ] original property overridden by this one and the object type the overriding belongs to
                                  # for atomic properties of abstract locally derived subobjects:
   [ '$up' => '0' ],              #  how many parent objects to travel upwards in order to get a proper type context

   '%key | property_key',         # The address of the hash itself serves as a search key in the objects' dictionaries,
                                  # production rule lists, permutation sensitivity lists, etc. - all kinds of lists where
                                  # all properties being concrete type instances or aliases of the same root property have
                                  # to be matched to the same data.
                                  # Contained is a hierarchy of further hashes: the keys belong to subobject properties, and
                                  # the values serve again as search keys in the lists of producers.
                                  # For example, if the object type O has a rule R = A.B : X,
                                  # then there is a hash $H=$Property_B->key->{$Property_A->key}
                                  # such that the list $ObjectType_O->producers->{$H} contains $Rule_R.

   [ '$mixin' => 'undef' ],       # for locally derived subobjects: parent ObjectType => LocalDerivationMixin

   [ '$new_instance_deputy' => 'undef' ],
);

# Property->flags
use Polymake::Enum qw( is: mutable=1 subobject=2 locally_derived=4 multiple=8 multiple_new=16
                           permutation=32 non_storable=64 subobject_array=128 produced_as_whole=256 restricted_spez=512 );

sub new {
   my $self=&_new;
   if (instanceof ObjectType($self->type)) {
      $self->flags |= $is_subobject;
      if ($self->flags & $is_multiple) {
         $self->new_instance_deputy=new NewMultiInstance($self);
      }
   }
   if ($Application::plausibility_checks) {
      if (($self->flags & ($is_multiple | $is_subobject)) == $is_multiple) {
         croak( "an atomic property may not be declared multiple" );
      }
      if (($self->flags & ($is_non_storable | $is_subobject)) == ($is_non_storable | $is_subobject)) {
         croak( "only atomic properties can be declared non-storable" );
      }
   }

   $self->construct &&= [ $self->belongs_to->encode_property_list($self->construct) ];

   if ($self->type->abstract) {
      $self->inst_cache={ };
      if (defined $self->type->context_pkg) {
         my $obj_proto=$self->belongs_to;
         while ($obj_proto->pkg ne $self->type->context_pkg  and  ! defined($obj_proto->descend_to_generic($self->type->context_pkg))) {
            $obj_proto=$obj_proto->belongs_to;
            $self->up++;
         }
      }
      $self->accept=sub : method {
         my $self=shift;
         clone($self, $_[1])->accept->(@_);
      };
   } else {
      ($self->accept, $self->copy)=choose_methods($self);
   }

   if ($self->flags & $is_multiple) {
      my $spez_type=$self->belongs_to;
      while ($spez_type->enclosed_in_restricted_spez) {
         $spez_type=$spez_type->belongs_to;
      }
      if (defined $spez_type->preconditions) {
         $self->flags |= $is_restricted_spez;
      }
   }

   $self;
}
####################################################################################
sub create_alias {
   my ($src, $alias_name, $override_for)=@_;
   my $self=inherit_class([ @$src ], $src);
   $self->name=$alias_name;
   if ($override_for) {
      $self->overrides=[ $src, $override_for ];
   }
   $self;
}
####################################################################################
# trace back the chain of overridings,
# find the property instance suitable for the given object type,
# substitute it in the callers variable
# Property, ObjectType =>
sub upcast {
   while (defined($_[0]->overrides) && !$_[1]->isa($_[0]->overrides->[1])) {
      $_[0]=$_[0]->overrides->[0];
   }
}
####################################################################################
sub analyze {
   my ($self, $pkg)=@_;
   if ($self->flags & $is_subobject) {
      create_local_derivation_mixin($self, $self->belongs_to);
   } else {
      namespaces::using($pkg, $self->type->pkg);
      my $symtab=get_pkg($pkg);
      foreach (qw(canonical equal)) {
         if (exists &{$symtab->{$_}}) {
            $self->$_=\&{$symtab->{$_}};
         }
      }
   }
}
####################################################################################
# private: Property, type => cloned Property
sub _clone {
   my $src=shift;
   my $self=inherit_class([ @$src ], $src);
   $self->cloned_from=$src;
   $self->type=shift;
   ($self->accept, $self->copy)=choose_methods($self);
   $self
}
####################################################################################
sub clone {
   my ($self, $obj)=@_;
   my $parent_proto=$obj->type;
   my $context_owner=$parent_proto;
   if (my $up=$self->up) {
      do { $obj=$obj->parent } while (--$up);
      $context_owner=$obj->type;
   }
   $self->inst_cache->{$context_owner} ||= do {
      my $concrete_type=$self->type->concrete_type($context_owner);
      $self->flags & $is_locally_derived  &&
      $self->create_locally_derived_type($parent_proto, $concrete_type)
        or
      _clone($self, $concrete_type);
   }
}
####################################################################################
# only for concrete base types; abstract types are handled within clone()
sub clone_locally_derived {
   my ($self, $parent_proto)=@_;
   $self->inst_cache->{$parent_proto} ||=
      $self->create_locally_derived_type($parent_proto, $self->type) || _clone($self, $self->type);
}
####################################################################################
sub closest_concrete_descendant {
   my ($abstract_proto, $final_proto)=@_;
   my $result=$final_proto;
   foreach (@{$final_proto->super}) {
      if ($_->abstract) {
         last if $_==$abstract_proto;
      } elsif (list_index($_->super, $abstract_proto)>=0) {
         $result=$_;
      }
   }
   $result
}
####################################################################################
sub closest_local_derivation_mixin {
   my ($self, $parent_proto)=@_;
   my @indep_mixins;
   foreach my $super_proto (@{$parent_proto->super}) {
      if (defined (my $mixin=$self->mixin->{$super_proto})) {
         unless (grep { list_index($_->super, $mixin)>=0 } @indep_mixins) {
            push @indep_mixins, $mixin;
         }
      }
   }
   if (@indep_mixins>1) {
      my $closest_common=$parent_proto;
      foreach my $super_proto (@{$parent_proto->super}) {
         if ($super_proto==$indep_mixins[0]->belongs_to) {
            last;
         } elsif ((grep { list_index($super_proto->super, $_->belongs_to)>=0 } @indep_mixins)==@indep_mixins) {
            $closest_common=$super_proto;
         }
      }
      $self->mixin->{$closest_common}=new ObjectType::LocalDerivationMixin($self, $closest_common);
   } else {
      pop @indep_mixins;
   }
}
####################################################################################
sub create_locally_derived_type : method {
   my ($self, $parent_proto, $subobj_proto, $allow_abstract)=@_;
   if (defined (my $mixin=$self->mixin->{$parent_proto} // closest_local_derivation_mixin($self, $parent_proto))) {
      my $mixin_owner=$mixin->belongs_to;
      if ($mixin_owner->abstract && !$allow_abstract) {
         $mixin_owner=closest_concrete_descendant($mixin_owner, $parent_proto);
      }
      if ($mixin_owner != $parent_proto) {
         $self->inst_cache->{$mixin_owner} ||= _clone($self, $mixin->local_type($subobj_proto));
      } else {
         _clone($self, $mixin->local_type($subobj_proto));
      }
   } else {
      undef
   }
}
####################################################################################
sub create_local_derivation_mixin {
   my ($self, $obj_proto)=@_;
   unless ($self->flags & $is_locally_derived) {
      $self->flags |= $is_locally_derived;
      $self->mixin={ };
      unless ($self->type->abstract) {
         $self->inst_cache={ };
         $self->accept=sub : method {
            my $self=shift;
            clone_locally_derived($self, $_[1]->type)->accept->(@_);
         };
      }
   }
   $self->mixin->{$obj_proto} ||= new ObjectType::LocalDerivationMixin($self, $obj_proto);
}
####################################################################################
sub concrete {
   my ($self, $obj)=@_;
   $self->type->abstract
   ? clone($self, $obj) :
   $self->flags & $is_locally_derived
   ? clone_locally_derived($self, $obj->type)
   : $self
}
####################################################################################
sub declared {
   my ($self)=@_;
   $self->cloned_from // $self
}
####################################################################################
sub specialization {
   my ($self, $parent_proto, $allow_abstract)=@_;
   if ($self->flags & $is_locally_derived) {
      if ($self->type->abstract) {
         $self->inst_cache->{$parent_proto} ||= do {
            my $subobj_type= $parent_proto->abstract ? $self->type : $self->type->concrete_type($parent_proto);
            $self->create_locally_derived_type($parent_proto, $subobj_type, $allow_abstract)
         }
      } else {
         &clone_locally_derived
      }
   } elsif ($self->type->abstract && !$allow_abstract) {
      $parent_proto->abstract
        and croak( "internal error: attempt to construct the concrete specialization of property ", $self->name,
                   " for an abstract parent object type ", $parent_proto->full_name );
      $self->inst_cache->{$parent_proto} ||= _clone($self, $self->type->concrete_type($parent_proto));
   } else {
      $self
   }
}
####################################################################################
sub choose_methods {
   my ($self)=@_;
   $self->flags & $is_subobject
   ? (\&accept_subobject, undef) :
   ($self->type->super == typeof BigObjectArray)
   ? do {
      $self->flags |= $is_subobject_array;
      (\&accept_subobject_array, \&copy_subobject_array)
   } :
   defined($self->construct)
   ? (\&accept_special_constructed, \&copy_special_constructed) :
   $self->type->cppoptions && !$self->type->cppoptions->builtin
   ? (\&accept_composite, \&copy_composite)
   : (\&accept_builtin, \&copy_builtin);
}
####################################################################################
sub accept_subobject : method {
   my ($self, $value, $obj, $trusted, $temp)=@_;
   my $subobj_type=$self->subobject_type;
   unless (defined $value) {
      if (!$trusted and $self->flags & ($is_multiple | $is_subobject_array)) {
         croak( "undefined value not allowed for the multiple subobject property ", $self->name );
      }
      return new PropertyValue($self, $value);
   }

   unless ($trusted) {
      is_object($value) or croak( "property ", $self->name, " needs an Object\n" );
      my $expected=$subobj_type->pure_type;
      unless (UNIVERSAL::isa($value, $expected->pkg)) {
         # if the source object can't be converted to our type, it will croak():
         $value=$expected->construct->($value);
      }
   }

   if (defined($value->parent) || defined($value->persistent)) {
      $value=Object::new_copy($subobj_type, $value, $obj);
   } else {
      weak($value->parent=$obj);
      if ($self->flags & $is_locally_derived) {
         my $orig_type=$value->type;
         if ($orig_type != $subobj_type) {
            bless $value, $subobj_type->local_type($orig_type)->pkg;
         }
      }
   }
   $value->property=$self;
   $value->is_temporary= $temp || defined($obj->parent) && $obj->is_temporary;
   if (defined($value->transaction)) {
      if ($value->has_cleanup) {
         Transaction::merge_temporaries($value->transaction->temporaries, delete $Scope->cleanup->{$value});
         $value->has_cleanup=0;
      }
      if (defined($obj->transaction)) {
         $obj->transaction->descend($value, 1) unless ($temp && defined($obj->transaction->rule));
      } else {
         $value->transaction->commit($value);
      }
   } elsif (defined($obj->transaction)) {
      unless ($temp && defined($obj->transaction->rule)) {
         ## FIXME: suspicious logical expression
         if (not($self->flags & $is_subobject_array) || $value->has_cleanup) {
            $obj->transaction->descend($value, !($self->flags & $is_permutation));
            if ($value->has_cleanup) {
               $value->transaction->temporaries=delete $Scope->cleanup->{$value};
               $value->has_cleanup=0;
            }
         }
      }
   }
   $value
}

# parent Object, temporary flag => empty Object
sub create_subobject : method {
   my $self=&concrete;
   accept_subobject($self, Object::new($self->subobject_type->pkg), $_[1], 1, $_[2]);
}

sub subobject_type {
   my ($self)=@_;
   $self->flags & $is_subobject_array ? $self->type->params->[0] : $self->type;
}
####################################################################################
sub accept_subobject_array : method {
   my ($self, $value, $obj, $trusted, $temp)=@_;
   if (ref($value) eq "ARRAY") {
      bless $value, $self->type->pkg;
   } elsif (!$trusted && !$self->type->isa->($value)) {
      croak( ref($value) || "'$value'", " passed as property ", $self->name, " while ", $self->type->full_name, " expected" );
   }
   foreach my $subobj (@$value) {
      accept_subobject($self, $subobj, $obj, $trusted, $temp);
   }
   new PropertyValue($self, $value, $temp);
}

sub copy_subobject_array : method {
   my ($self, $value)=splice @_, 0, 2;
   my $index=-1;
   new PropertyValue($self, bless([ map { $_->copy(@_) } @$value ], $self->type->pkg));
}
####################################################################################
sub accept_composite : method {
   my ($self, $value, $obj, $trusted, $temp)=@_;
   if (defined $value) {
      my $needs_canonicalization=!$trusted && defined($self->canonical);
      my ($is_object, $type_mismatch);
      if (!($is_object=is_object($value)) or
          $type_mismatch=!$self->type->isa->($value) or
          $self->type->cppoptions && !$self->type->cppoptions->builtin &&
          CPlusPlus::must_be_copied($value, $temp, $needs_canonicalization)) {
         my $target_type= $type_mismatch
                          ? $self->type->coherent_type->($value)
                          : $is_object && $value->type;
         local $PropertyType::trusted_value=$trusted;
         $value=($target_type || $self->type)->construct->($value);
      }
      if ($needs_canonicalization) {
         select_method($self->canonical, $obj, 1)->($value);
      }
   }
   new PropertyValue($self, $value, $temp);
}

sub copy_composite : method {
   my ($self, $value)=@_;
   new PropertyValue($self, $self->type->construct->($value));
}
####################################################################################
sub accept_special_constructed : method {
   my ($self, $value, $obj, $trusted, $temp)=@_;
   if (defined($value)) {
      my $needs_canonicalization=!$trusted && defined($self->canonical);
      if (!is_object($value) or
          !$self->type->isa->($value) or
          $self->type->cppoptions && !$self->type->cppoptions->builtin &&
          CPlusPlus::must_be_copied($value,$temp,$needs_canonicalization)) {
         if (my $construct_arg=$obj->lookup_property_path($self->construct)) {
            local $PropertyType::trusted_value=$trusted;
            $value=$self->type->construct->($construct_arg->value, $value);
         } else {
            croak( "can't add property ", $self->name, " because of lacking prerequisite ", print_path($self->construct) );
         }
      }
      if ($needs_canonicalization) {
         select_method($self->canonical,$obj,1)->($value);
      }
   }
   new PropertyValue($self, $value, $temp);
}

sub copy_special_constructed : method {
   my ($self, $value, $obj)=@_;
   if (my $construct_arg=$obj->lookup_property_path($self->construct)) {
      new PropertyValue($self, $self->type->construct->($construct_arg->value, $value));
   } else {
      croak( "can't copy property ", $self->name, " because of lacking prerequisite ", print_path($self->construct) );
   }
}
####################################################################################
sub accept_builtin : method {
   my ($self, $value, $obj, $trusted, $temp)=@_;
   if (defined $value) {
      local $PropertyType::trusted_value=$trusted;
      $value=$self->type->construct->($value);
      if (!$trusted && defined($self->canonical)) {
         select_method($self->canonical, $obj, 1)->($value);
      }
   }
   new PropertyValue($self, $value, $temp);
}

sub copy_builtin : method {
   my ($self, $value)=@_;
   new PropertyValue($self, $value);
}
####################################################################################
sub qual_name {
   my ($self)=@_;
   defined($self->application) ? $self->application->name . "::" . $self->name : $self->name
}
sub print_path {
   my ($path)=@_;
   is_object($path->[0]) ? join(".", map { $_->name } @$path) : join(" | ", map { join(".", map { $_->name } @$_) } @$path)
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
package _::SelectMultiInstance;

# used only in requests and 'down' paths processed by Scheduler

use Polymake::Struct (
   [ new => '$$' ],
   [ '$property' => '#1' ],
   [ '$index' => '#2' ],
);

sub key { $_[0]->property->key }
*property_key=\&key;
sub flags { $is_multiple }

sub name {
   my ($self)=@_;
   $self->property->name . "[" . (is_object($self->index) ? $self->index->header : $self->index) . "]"
}

package __;

sub index { 0 }           # by default, the 0-th multiple subobject instance is selected
sub property { $_[0] }

####################################################################################
package _::SubobjKey;
RefHash::allow(__PACKAGE__);

my $prop_key=\(1);
my $subobj_key=\(2);
my $perm_key=\(3);

sub new {
   my $self=bless { $prop_key => $_[1], $subobj_key => $_[2] }, $_[0];
   weak($_) for values %$self;
   $self->{$perm_key}=1 if $_[3];
   $self;
}

sub key { $_[0] }
sub property { $_[0]->{$prop_key} }
sub subobject_property { $_[0]->{$subobj_key} }
sub property_key { (&property)->key }
sub belongs_to { (&subobject_property)->belongs_to }
sub on_permutation_path { $_[0]->{$perm_key} }

####################################################################################
package __;

# protected: used solely from Rule::finalize
# ancestor Properties come in reverse (bottom-up) order!
sub get_prod_key {
   my $self=shift;
   my $hash=$self->key;
   my $perm_seen;
   foreach (@_) {
      $perm_seen ||= $_->flags & $is_permutation;
      $hash=($hash->{$_->key} //= new SubobjKey($self, $_, $perm_seen));
   }
   $hash
}
####################################################################################
package _::NewMultiInstance;

# used only in 'out' paths of Rules creating new instances of multiple subobjects

use Polymake::Struct (
   [ new => '$' ],
   [ '$property' => 'weak( #1 )' ],
   '%key',
   [ '$name' => '#1 ->name . "(new)"' ],
   [ '$flags' => '#1 ->flags | $is_multiple_new' ],
   [ '$belongs_to' => '#1 ->belongs_to' ],
   [ '$property_key' => '#1 ->property_key' ],
);

sub index { 0 }

*get_prod_key=\&Property::get_prod_key;

sub create_subobject : method {
   my ($self, $parent_obj, $temp)=@_;
   my $prop=$self->property;
   my $subobj=$prop->create_subobject($parent_obj, $temp);
   my $pv=new PropertyMultipleValue($subobj->property, [ $subobj ]);
   $pv->created_by_rule={ ($parent_obj->transaction->rule, 0) };
   $pv->ensure_unique_name($subobj, $temp);
   $pv;
}

1

# Local Variables:
# cperl-indent-level: 3
# indent-tabs-mode:nil
# End:
