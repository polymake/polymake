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

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);
use feature 'state';

package Polymake::Core::BigObjectType;

my $construct_node;
sub construct_node : lvalue { $construct_node }

# outer BigObjectType during rulefile loading
declare $scope_owner;

##############################################################################
#  Collection of information common to all Objects of the same type
#  (such as property names, rules, etc.)

use Polymake::Struct (
   [ new => '$$@' ],
#
#  Environmental data
#
   [ '$name' => '#1' ],                 # own name
   [ '$application' => '#2' ],          # -> Application
   [ '$extension' => '$Extension::loading' ],
   '$pkg',                              # perl class
   [ '$context_pkg' => 'undef' ],       # for abstract parameterized object types resulting from local derivation
                                        #  as a property of another parameterized BigObjectType:
                                        #  perl class of the enclosing BigObjectType (i.e. property owner)
   [ '$params' => 'undef' ],            # for parameterized types: list of BigObjectType or PropertyType describing the parameters
   '&abstract',
   [ '&perform_typecheck' => '\&PropertyType::concrete_typecheck' ],
   [ '&construct' => '\&PropertyType::construct_object' ],
   [ '&parse' => '\&BigObject::new_named' ],
   [ '&toString' => '\&BigObject::print_me' ],
   [ '&JSONschema' => '\&Serializer::big_object_schema' ],
#
#  Derivation relations
#
   '@super',                       # ( BigObjectType ) base classes
                                   #   For non-parametrized and generic types it exactly reflects the object type declaration.
                                   #   For concrete instances of parametrized types the base classes follow in the order:
                                   #     - named full specializations (restricted by preconditions)
                                   #     - anonymous full specialization (w/o preconditions)
                                   #     - concrete instances of matching partial specializations
                                   #     - own generic type
                                   #     - concrete instance(s) of base types
   '@linear_isa',                  # transitive closure of @super in C3 resolution order
   [ '$generic' => 'undef' ],      # BigObjectType of the own generic type;
                                   #   for concrete instances of partial specialization: the parametrized specialization
   [ '$full_spez' => 'undef' ],    # full specializations of parametrized types: concrete BigObjectType pkg or spez. name => Specialization
#
#  Own components
#
   '%properties',                  # 'name' => Property
   '%permutations',                # 'name' => permutation Property
   '%producers',                   # own rules producing a property: Property/Key => [ Rule, ... ]
   '%all_producers',               # cached own and inherited rules: Property/Key => [ Rule, ... ]
   '%shortcuts',                   # cached own and inherited shortcut rules
   '%rules_by_label',              # own labeled producing rules: '*.label' => ControlList
   '%all_rules_by_label',          # cached own and inherited labeled rules
   '@production_rules',            # ( Rule ) all production rules directly applicable to this object type and descendants (for search by header)
#
   [ '$specializations' => 'undef' ],  # ( Specialization ) for parameterized types with partial specializations: the abstract objects
   [ '$borrowed_from' => 'undef' ],    # other Specializations defining properties this Specialization or generic type refers to
#
   [ '$help' => 'undef' ],         # Help::Topic if loaded
);

####################################################################################
#
#  Constructor
#
#  new BigObjectType('name', Application, [ type params ], [ super BigObjectType, ... ]);
#
sub new {
   my $self = &_new;
   Struct::learn_package_retrieval($self, \&pkg);
   my $tparams = splice @_, 0, 3;
   my $self_sub = sub { $self };
   my $generic_type;

   unless ($construct_node) {
      $construct_node = new_root Overload::Node;
      Overload::add_instance("Polymake::Core::BigObject", ".construct", undef, \&BigObject::new_empty,
                             [0, 0], undef, $construct_node);
      $construct_node->add_fallback(\&BigObject::new_filled);
   }

   if (defined($self->application)) {
      $self->pkg = $self->application->pkg."::".$self->name;
      if (defined($tparams)) {
         $self->abstract = \&type;
         undef $self->perform_typecheck;
         $self->params = PropertyParamedType::create_param_placeholders($self->pkg, $self->application, $tparams);
      }
      Overload::add_instance($self->pkg, ".construct", undef, \&BigObject::new_copy,
                             [1, 1, $self->pkg ], undef, $construct_node);
      Overload::add_instance($self->pkg, ".construct", undef, \&BigObject::new_filled_copy,
                             [3, 3+Overload::SignatureFlags::has_trailing_list, $self->pkg, '$', '$'], undef, $construct_node);
   } else {
      # it is an instance of an abstract BigObject type
      # the first super-BigObject is always the own abstract type
      $generic_type = $_[0];
      $self->application = $generic_type->application;
      $self->extension = $generic_type->extension;
      $self->pkg = $generic_type->pkg;
      $self->params = $tparams;
      PropertyParamedType::scan_params($self);
      if ($self->abstract) {
         if (defined($self->context_pkg)) {
            push @{$self->super}, $generic_type;
            push @{$self->linear_isa}, $generic_type, @{$generic_type->linear_isa};
            $self->generic=$generic_type;
         }
         return $self;
      }
      define_function($self->pkg, "typeof", $self_sub, 1);
   }

   define_function($self->pkg, "type", $self_sub, 1);
   if (!$self->abstract) {
      define_function($self->pkg, ".type", $self_sub);
   }
   PropertyType::create_method_new($self);

   # proceed with parent classes
   if (@_) {
      set_extension($self->extension, $_->extension) for @_;
      modify_super($self, 0, 0, @_);
      if (defined $generic_type) {
         $self->generic=$generic_type;

         my %required_spezs;
         if (defined $generic_type->borrowed_from) {
            $required_spezs{$_}=1 for keys %{$generic_type->borrowed_from};
         }
         if (defined $generic_type->specializations) {
            my @matching_spezs=map { pick_specialization($_, $self) } @{$generic_type->specializations};

            foreach my $spez (@matching_spezs) {
               my $gen_spez=$spez->generic;
               $required_spezs{$gen_spez} |= 2;
               if (defined $gen_spez->borrowed_from) {
                  $required_spezs{$_} |= 1 for keys %{$gen_spez->borrowed_from};
               }
            }

            modify_super($self, 0, 0, @matching_spezs);
         }

         # verify correctness of inheritance relationship between specializations
         while (my ($spez, $status)=each %required_spezs) {
            if ($status == 1 && list_index($self->linear_isa, $spez)<0) {
               my @bad=grep { defined($_->borrowed_from) && $_->borrowed_from->{$spez} } @{$self->super};
               die "Invalid inheritance: ", join(", ", map { $_->full_name } @bad),
                            "\n refer", @bad==1 && "s", " to properties defined in ", $spez->full_name,
                            "\n which does not match the object type ", $self->full_name, "\n";
            }
         }
      }
   } else {
      no strict 'refs';
      push @{$self->pkg."::ISA"}, "Polymake::Core::BigObject";
      mro::set_mro($self->pkg, "c3");
   }

   push @{$self->application->object_types}, $self;
   $self;
}

####################################################################################
sub pick_specialization {
   my $spez=shift;
   &{$spez->match_node->resolve(\@_) // return ()}
}

sub all_partial_specializations {
   my ($self, $for_spez)=@_;
   ( ($self != $for_spez && defined($self->specializations)
      ? (grep { $_ != $for_spez } @{$self->specializations}) : () ),
     map { defined($_->specializations) ? @{$_->specializations} : () } @{$self->linear_isa}
   )
}
####################################################################################
sub full_specialization {
   my ($self, $concrete) = @_;
   ($self->full_spez //= { })->{$concrete->pkg} //= new Specialization(undef, $concrete->pkg."::_Full_Spez", $concrete);
}
####################################################################################
sub modify_super {
   my ($self, $pos, $remove, @types) = @_;
   mro::set_mro($self->pkg, "c3") unless @{$self->super};
   splice @{$self->super}, $pos, $remove, @types;
   my @pkgs = map { $_->pkg } @types;
   namespaces::using($self->pkg, @pkgs);
   no strict 'refs';
   splice @{$self->pkg."::ISA"}, $pos, $remove, @pkgs;
   update_inheritance($self);
}

sub update_inheritance {
   my ($self) = @_;
   my $linear_isa = mro::get_linear_isa($self->pkg);
   # ignore the type itself and ubiquitous BigObject
   @{$self->linear_isa} = map { $_->type } grep { !/^Polymake::Core::/ } @$linear_isa[1..$#$linear_isa];
}

sub concrete_instances {
   my ($self) = @_;
   grep { $_->generic == $self && !$_->abstract } &derived
}

sub concrete_super_instances {
   my ($self) = @_;
   my @list;
   foreach my $super (reverse @{$self->super}) {
      last if $super->abstract || $super->full_spez_for == $self;
      push @list, $super;
   }
   reverse @list
}
####################################################################################
sub isa {
   my ($self, $other) = @_;
   if (is_string($other)) {
      $other = $self->application->eval_type($other, 1) or return;
   }
   return $self == $other || list_index($self->linear_isa, $other) >= 0;
}
####################################################################################
sub deserialize : method {
   my ($self, $src) = @_;
   if (is_hash($src)) {
      &BigObject::deserialize
   } else {
      croak( "wrong serialized representation of ", $self->full_name, " - anonymous hash expected" );
   }
}
####################################################################################
# for compatibility with PropertyType:

*type=\&PropertyType::type;
*mangled_name=\&PropertyParamedType::mangled_name;
*full_name=\&PropertyParamedType::full_name;
*typecheck=\&PropertyType::typecheck;
*add_constructor=\&PropertyType::add_constructor;
*performs_deduction=\&PropertyParamedType::performs_deduction;
*set_extension=\&PropertyParamedType::set_extension;
*type_param_index=\&PropertyType::type_param_index;
*derived=\&PropertyType::derived;
define_function(__PACKAGE__, ".type", \&type);

sub qualified_name {
   my ($self) = @_;
   defined($self->params) && !$self->abstract ? &PropertyParamedType::qualified_name : &PropertyType::qualified_name
}

sub required_extensions {
   my ($self) = @_;
   defined($self->params) && !$self->abstract ? &PropertyParamedType::required_extensions : &PropertyType::required_extensions
}

sub concrete_type {
   # undef context_pkg means we are in a generic parameterized object class, hence nothing to deduce
   defined($_[0]->context_pkg) ? &PropertyParamedType::concrete_type : pop
}

sub descend_to_generic {
   my ($self, $pkg)=@_;
   if (@{$self->linear_isa}) {
      return $self if defined($self->generic) && $self->generic->pkg eq $pkg;
      foreach my $super_proto (@{$self->linear_isa}) {
         return $super_proto if defined($super_proto->generic) && $super_proto->generic->pkg eq $pkg;
      }
   }
   undef
}
####################################################################################
sub find_super_type_param {
   my ($self, $name) = @_;
   my @result;
   foreach my $super (@{$self->linear_isa}) {
      if (!instanceof Augmented($super) and defined($super->params) and my ($param) = grep { $_->name eq $name } @{$super->params}) {
         @result = ($super, $param);
      }
   }
   @result;
}
####################################################################################
# "property name" => Property or undef
sub lookup_property {
   my ($obj_type, $prop_name) = @_;
   # full spezialization objects share all properties with full instances
   my $self = $obj_type->full_spez_for // $obj_type;
   # negative results are not cached
   $self->properties->{$prop_name} // do {
      my $prop;
      foreach my $super (@{$self->linear_isa}) {
         if (defined($prop = $super->properties->{$prop_name})) {
            if ($prop->flags & Property::Flags::is_permutation) {
               $prop = instantiate_permutation($self, $prop);
            } elsif (!$self->abstract && ($prop->flags & Property::Flags::is_subobject || !($prop->flags & Property::Flags::is_concrete))) {
               $prop = instantiate_property($self, $prop_name, $prop);
            }
            return $self->properties->{$prop_name} = $prop;
         }
      }
      if ($self->abstract) {
         foreach my $spez (($self->generic // $self)->all_partial_specializations($self)) {
            if (defined($prop = $spez->properties->{$prop_name})) {
               $spez = $prop->defined_for;
               my ($borrower, $lender) = ($self->outer_object_type, $spez->outer_object_type);
               ($borrower->borrowed_from //= { })->{$lender} ||= do {
                  # verify the correctness of borrowing
                  foreach my $proto ($borrower->concrete_instances) {
                     if (list_index($proto->linear_isa, $lender) < 0) {
                        die "Invalid inheritance: ", $self->full_name,
                            "\n refers to property $prop_name defined in ", $spez->full_name,
                            "\n which does not match the derived object type ", $proto->full_name, "\n";
                     }
                  }
                  1
               };
               return $self->properties->{$prop_name} = $prop;
            }
         }
      }
      undef
   }
}

sub instantiate_property {
   my ($self, $prop_name, $prop) = @_;
   if ($prop->name ne $prop_name) {
      # found an overridden property in one of the ancestors:
      # take the overriding instance 
      if ($prop->overrides ne $prop_name) {
         die "internal error: property ", $prop->name, " cached under the name '$prop_name'\n";
      }
      return lookup_property($self, $prop->name);
   }
   my $owner = $prop->belongs_to;
   if (defined(my $gen_proto = $owner->generic)) {
      # move from a specialization to the abstract type
      $owner = $gen_proto;
   }
   if ($owner == $self->generic) {
      unless ($prop->flags & Property::Flags::is_concrete) {
         $prop = $prop->clone_for_owner($self);
         create_prop_method($self, $prop);
      }
      $prop
   } else {
      if ($prop->flags & Property::Flags::is_subobject) {
         # instantiate the property for each of the concrete ancestors; they are dwelling at the end of the base class list
         # augmented instances might need to be mixed in the case of multiple inheritance
         if (my @instances = collect_augmented_super_instances($self, $prop_name)) {
            if (@instances > 1) {
               $prop = $prop->clone_for_augmented(new Augmented($prop, $self, map { $_->type } @instances));
               create_prop_method($self, $prop);
               return $prop;
            } else {
               return $instances[0];
            }
         } elsif ($prop->flags & Property::Flags::is_concrete) {
            return $prop;
         }
      }

      # instantiate the property at its birthplace
      foreach my $super (@{$self->linear_isa}) {
         # the concrete object type must appear before all specializations and generic types
         if ($super->generic == $owner) {
            # if $self is a parametrized augmented type, we might have found the abstract property instance cached in the generic augmented type
            # because it precedes the pure types in the inheritance list, thus in fact the concrete property instance might already exist
            return $super->properties->{$prop_name} //= do {
               $prop = $prop->clone_for_owner($super);
               create_prop_method($super, $prop);
               $prop
            };
         }
      }
      croak( "internal error: could not find the concrete instance of ", $owner->full_name, " among ancestors of ", $self->full_name,
                   " (", join(", ", map { $_->full_name } @{$self->super}), ") while instantiating property ", $prop->name );
   }
}

sub instantiate_permutation {
   my ($self, $prop) = @_;
   # only a dependent abstract type may inherit the permutation instance from its generic type,
   # all other types might introduce own properties and therefore need personalized permutation instances
   if ($self->abstract && defined($self->context_pkg)) {
      if ($prop->belongs_to != $self->generic) {
         $prop = lookup_property($self->generic, $prop->name);
      }
   } else {
      $prop = $prop->clone_for_owner($self);
      create_prop_method($self, $prop);
   }
   $prop
}

sub lookup_overridden_property {
   my ($self, $prop) = @_;
   foreach my $super (@{$self->linear_isa}) {
      if (!$super->isa($prop->overrides_for) && $super->isa($prop->defined_for)) {
         return lookup_property($super, $prop->overrides);
      }
   }
   undef
}

sub property {
   my ($self, $prop_name) = @_;
   lookup_property($self, $prop_name) or croak( "unknown property ", $self->full_name, "::$prop_name" );
}

sub init_pseudo_prop {
   state $prop = _new Property(".initial", undef, undef);
}

sub collect_augmented_super_instances {
   my ($self, $prop_name) = @_;
   my @list;
   foreach my $super ($self->concrete_super_instances) {
      if (defined(my $found_prop = lookup_property($super, $prop_name))) {
         if ($found_prop->flags & Property::Flags::is_augmented) {
            if (@list == 1 && !($list[0]->flags & Property::Flags::is_augmented)) {
               $list[0] = $found_prop;
            } elsif (!@list || list_index(\@list, $found_prop) < 0) {
               push @list, $found_prop;
            }
         } elsif (!@list) {
            push @list, $found_prop;
         }
      }
   }
   @list
}

sub help_ref_to_prop {
   my ($self, $from_app, $prop_name)=@_;
   ($self->application != $from_app && $self->application->name."::").$self->name."::$prop_name";
}
####################################################################################
# "property name" => bool
# Returns TRUE if the property is overridden for this object type or one of its ancestors.
# Returns undef if the property is not known at all.
sub is_overridden {
   my ($self, $prop_name) = @_;
   if (defined(my $prop = lookup_property($self, $prop_name))) {
      $prop->name ne $prop_name;
   } else {
      undef
   }
}
####################################################################################
# Property => "name"
# Returns the name of the overriding property if appropriate.
sub property_name {
   my ($self, $prop) = @_;
   lookup_property($self, $prop->property_name)->name
}
####################################################################################
sub list_permutations {
   my ($self) = @_;
   (values %{$self->permutations}), (map { values %{$_->permutations} } @{$self->linear_isa})
}
####################################################################################
# private:
sub create_prop_method {
   my ($self, $prop)=@_;
   define_function($self->pkg, $prop->name,
                   create_prop_accessor($prop->flags & Property::Flags::is_multiple
                                        ? [ $prop, \&BigObject::get_multi, \&BigObject::put_multi ]
                                        : [ $prop, \&BigObject::get,       \&BigObject::put       ],
                                        $self->pkg));
}

# private:
sub add_property {      # => Property
   my ($self, $prop) = @_;
   if ($enable_plausibility_checks && defined(my $old_prop = lookup_property($self, $prop->name))) {
      croak( $old_prop->belongs_to == $self
             ? "multiple definition of property '".$prop->name."'"
             : "redefinition of inherited property '".$prop->name."' not allowed" );
   }
   &create_prop_method;
   $self->properties->{$prop->name} = $prop;
}

sub add_permutation {
   my ($self, $name, $pure_prop) = @_;
   if ($enable_plausibility_checks && defined(my $old_prop = lookup_property($self, $name))) {
      croak( $old_prop->flags & Property::Flags::is_permutation
             ? ( $old_prop->belongs_to == $self
                 ? "multiple definition of permutation '$name'"
                 : "redefinition of inherited permutation '$name' not allowed" )
             : "permutation $name conflicts with property ".$old_prop->defined_for->full_name."::$name" );
   }
   my $perm = new Permutation($name, $self->augment($pure_prop), $self);
   create_prop_method($self, $perm);
   $pure_prop->name .= ".pure";
   $pure_prop->flags |= Property::Flags::is_non_storable;
   $self->properties->{$pure_prop->name} = $pure_prop;
   $self->permutations->{$name} =
   $self->properties->{$name} = $perm;
}
####################################################################################
# private:
sub find_overridden_property {
   my ($self, $prop_name) = @_;
   my $prop;
   foreach my $super (@{$self->linear_isa}) {
      # only properties inherited from other types or from other enclosing types may be overridden
      if ($super != $self->generic && $super->outer_object_type != $scope_owner) {
         defined($prop = lookup_property($super, $prop_name))
           and return $prop;
      }
   }
   # this will croak if $prop_name is unknown
   $prop = property($self, $prop_name);
   croak( "Can't override a property $prop_name defined in the same class family ", $prop_name->defined_for->full_name );
}
####################################################################################
sub override_property {        # "new name", "old name", Help => Property
   my ($self, $prop_name, $old_prop_name, $new_type, $help) = @_;
   if ($enable_plausibility_checks) {
      if (defined($old_prop_name)) {
         if (instanceof Specialization($scope_owner) && !$scope_owner->is_anon_full_spez) {
            croak( "A property can't be overridden in a partial or conditional specialization" );
         }
      } else {
         if (instanceof Specialization($self) || instanceof Augmented($self)) {
            croak( "Property type can only be overridden in a derived object type" );
         }
      }
   }
   my $old_prop = find_overridden_property($self, $old_prop_name // $prop_name);

   if ($enable_plausibility_checks && defined($new_type)) {
      $old_prop->flags & Property::Flags::is_subobject
        or croak( "invalid type override for an atomic property ", $old_prop_name // $prop_name );
      if ($old_prop->flags & Property::Flags::is_twin and $new_type != $self) {
         croak( "Twin property $prop_name automatically inherits the enclosing object type, the override definition must end with ': self'" );
      }
      if ($new_type->pkg eq $old_prop->type->pkg) {
         croak( "invalid property type override: new type is identical to the overridden one" );
      }
      ($new_type->generic // $new_type)->isa($old_prop->type->pure_type->generic // $old_prop->type->pure_type)
        or croak( "invalid property type override: ", $new_type->full_name, " is not derived from ", $old_prop->type->pure_type->full_name );
   }

   my $prop=$old_prop->override_by($prop_name, $self, $new_type);

   if (defined($new_type) and $old_prop->flags & Property::Flags::is_augmented) {
      $prop->change_to_augmented(new Augmented($prop, $self));
   }

   if (defined($old_prop_name)) {
      $self->properties->{$old_prop_name} = add_property($self, $prop);
      define_function($self->pkg, $old_prop_name, UNIVERSAL::can($self->pkg, $prop_name));

      if (defined($help)) {
         $help->annex->{header} = "property $prop_name : ".$prop->type->full_name."\n";
         weak($help->annex->{property} = $prop);
         $help->text =~ s/UNDOCUMENTED\n//;
         $help->text .= " Alias for property [[" . $old_prop->defined_for->help_ref_to_prop($self->application, $old_prop_name) . "]].\n";
      }
   } else {
      $self->properties->{$prop_name} = $prop;
      create_prop_method($self, $prop);

      # derived classes might have cached the original property or its concrete instances
      foreach my $derived ($self->derived) {
         my $cached_prop = $derived->properties->{$prop_name} or next;
         if ($cached_prop == $old_prop) {
            $derived->properties->{$prop_name} = $prop;
         } elsif ($cached_prop->flags & Property::Flags::is_concrete && $self->isa($cached_prop->belongs_to->generic // $cached_prop->belongs_to)) {
            delete $derived->properties->{$prop_name};
         }
      }
   }
   $prop;
}
####################################################################################
# protected:
sub invalidate_prod_cache {
   my ($self, $key) = @_;
   if (defined(delete $self->all_producers->{$key})) {
      delete $self->shortcuts->{$key};
      invalidate_prod_cache($_, $key) for $self->derived;
   }
}


sub rule_is_applicable {
   my ($self, $rule) = @_;
   if (defined(my $overridden_in = $rule->overridden_in)) {
      foreach my $other (@$overridden_in) {
         return 0 if $self == $other || list_index($self->linear_isa, $other) >= 0;
      }
   }
   1
}

sub add_producers_of {
   my ($self, $key) = splice @_, 0, 2;
   push @{$self->producers->{$key}}, @_;
   invalidate_prod_cache($self, $key);
}

sub get_producers_of {
   my ($self, $prop, $stop_after) = @_;
   my $key = $prop->key;
   $self->all_producers->{$key} ||= do {
      my @list;
      if (defined(my $own_prod = $self->producers->{$key})) {
         @list = @$own_prod;
      }
      $stop_after //= $prop->defined_for;
      if ($stop_after != $self) {
         foreach my $super_proto (@{$self->linear_isa}) {
            $super_proto->all_producers->{$key} //= 0;   # mark for cache invalidation
            if (defined(my $super_prod = $super_proto->producers->{$key})) {
               push @list, grep { rule_is_applicable($self, $_) } @$super_prod;
            }
            last if $stop_after == $super_proto;
         }
      }
      \@list
   };
}

# only shortcuts directly creating the property asked for are stored here
sub get_shortcuts_for {
   my ($self, $prop) = @_;
   $self->shortcuts->{$prop->key} //=
      [ grep { instanceof Rule::Shortcut($_) and
               $_->output->[0]->[-1]->key == $prop->property_key
        } @{ $self->get_producers_of($prop) }
      ]
}
####################################################################################
# protected:
sub invalidate_label_cache {
   my ($self, $wildcard) = @_;
   if (defined(delete $self->all_rules_by_label->{$wildcard})) {
      invalidate_label_cache($_, $wildcard) for $self->derived;
   }
}

sub get_rules_by_label {
   my ($self, $wildcard) = @_;
   $self->all_rules_by_label->{$wildcard} ||= do {
      # signal for invalidate_label_cache should it happen later
      $_->all_rules_by_label->{$wildcard} //= 0 for @{$self->linear_isa};
      Preference::merge_controls(grep { defined($_) } map { $_->rules_by_label->{$wildcard} } $self, @{$self->linear_isa})
   };
}

####################################################################################
# protected:
sub add_rule_labels {
   my ($self, $rule, $labels)=@_;
   foreach my $label (@$labels) {
      my $wildcard = $label->wildcard_name;
      my $ctl_list;
      if (defined ($ctl_list = $self->rules_by_label->{$wildcard})) {
         invalidate_label_cache($self, $wildcard);
      } else {
         $self->rules_by_label->{$wildcard} = $ctl_list = [ ];
      }
      $label->add_control($ctl_list, $rule);
   }
}
####################################################################################
# protected:
# "NAME.NAME" => (Property)
sub encode_descending_path {
   my ($self, $string) = @_;
   my $prop;
   map {
      $self = $prop->type if $prop;
      $prop = property($self, $_);
   } split /\./, $string;
}

# "parent.<...>.PROP_NAME.<...> => Property::Path
# Leading "parents" may also be expressed as "ancestor(TYPE)"
sub encode_property_path {
   my ($self, $string)=@_;
   my $up=0;
   while ($string =~ s/^parent\.//) {
      if (instanceof Augmented($self)) {
         ++$up;
         $self=$self->parent_property->belongs_to;
      } else {
         croak("can't refer to parents of a non-augmented object type ", $self->full_name);
      }
   }
   if (!$up && $string =~ s/^ancestor\(($balanced_re)\)\.//) {
      my $parent_type=eval "typeof $1";
      croak("invalid parent type $1: $@") if $@;
      for (;;) {
         if (instanceof Augmented($self)) {
            ++$up;
            $self=$self->parent_property->belongs_to;
            if ($self->isa($parent_type)) {
               last;
            }
         } elsif ($up) {
            croak($_[0]->full_name, " has no ascendants of type ", $parent_type->full_name);
         } else {
            croak("can't refer to parents of a non-augmented object type ", $self->full_name);
         }
      }
   }
   new Property::Path($up, encode_descending_path($self, $string));
}

# 'NAME.NAME | ...' => [ [ Property, ... ] ]
# flags(RETVAL)==Property::Flags::is_permutation if the paths descend into a permutation subobject
sub encode_read_request {
   my ($self, $req)=@_;
   my $perm_seen;
   my @alternatives=map {
      my @path=encode_descending_path($self, $_);
      $perm_seen ||= Property::find_first_in_path(\@path, Property::Flags::is_permutation)>=0;
      \@path
   } split /\s*\|\s*/, $req;
   if ($perm_seen) {
      set_array_flags(\@alternatives, Property::Flags::is_permutation);
   }
   \@alternatives
}
####################################################################################
# for compatibility with Augmented types, e.g. when only derived owner types define some additional properties
*pure_type=\&type;
sub final_type { pop }
sub outer_object_type { shift }
sub parent_property { undef }
sub enclosed_in_restricted_spez { 0 }

# for compatibility with Specialization
sub preconditions { undef }
sub full_spez_for { undef }
sub is_anon_full_spez { 0 }
####################################################################################
sub override_rule {
   my ($self, $super, $label)=@_;
   my $matched;
   foreach my $rule ($label->list_all_rules) {
      if (is_object($super)) {
         next if !$super->isa($rule->defined_for);
      } else {
         next if $self==$rule->defined_for || list_index($self->linear_isa, $rule->defined_for)<0;
      }
      $matched=1;
      if (defined($rule->overridden_in)) {
         push @{$rule->overridden_in}, $self if list_index($rule->overridden_in, $self)<0;
      } else {
         $rule->overridden_in=[ $self ];
      }
   }
   if ($enable_plausibility_checks && !$matched) {
      warn_print( "\"$_[3]\", line $_[4]: override declaration does not match any inherited rules" );
   }
}
####################################################################################
sub find_rule_label {
   my ($self, $pattern)=@_;
   $self->application->prefs->find_label($pattern)
}

sub find_rules_by_pattern {
   my ($self, $pattern)=@_;
   if ($pattern =~ /^ $hier_id_re $/xo) {
      # specified by label
      my $label=$self->find_rule_label($pattern)
        or die "unknown label \"$pattern\"\n";
      grep { $self->isa($_->defined_for) } $label->list_all_rules

   } elsif ($pattern =~ /:/) {
      # specified by header:
      Rule::header_search_pattern($pattern);
      grep { $_->header =~ $pattern } map { @{$_->production_rules} } $self, @{$self->linear_isa}

   } else {
      die "invalid rule search pattern: expected a label or a complete header in the form \"OUTPUT : INPUT\"\n";
   }
}

sub disable_rules {
   my @rules=&find_rules_by_pattern
     or die "no matching rules found\n";
   Scheduler::temp_disable_rules(@rules);
}
####################################################################################
sub add_method_rule {
   my ($self, $header, $code, $name)=@_;
   my $rule = special Rule($header, $code, $self);
   $rule->flags = Rule::Flags::is_function;
   substr($rule->header, 0, 0) = "user_method ";
   $rule->analyze_spez_preconditions;
   if (defined($name)) {
      # non-polymorphic method, no call redirection via overload resolution
      my $rules = [ $rule ];
      define_function($self->pkg, $name,
                      sub : method {
                         &{ (Scheduler::resolve_rules($_[0], $rules) // croak("could not provide all required input properties") )->code };
                      });
   }
   $rule;
}
####################################################################################
sub augment {
   my ($self, $prop)=@_;
   my ($augm, $augm_super, $cloned_prop, $update_caches);

   if ($prop->flags & Property::Flags::is_permutation) {
      my $pure_prop = $prop->type->pure_property;
      $augm = $self->augment($pure_prop);
      $prop->change_to_augmented($augm);
      # TODO: reactivate if needed, or delete, together with Permutation::update_pure_type
      if (false) {
         # update permuted types in every derived class to use the augmented base "pure" type
         foreach my $derived (grep { $_ != $self->full_spez_for && !defined($_->full_spez_for) } $self->derived) {
            my $cached_prop = $derived->properties->{$prop->name} or next;
            $cached_prop != $prop
              or croak( "internal error: permutation ", $prop->name, " instance shared between different types ",
                        $derived->full_name, " and ", $self->full_name );
            $cached_prop->update_pure_type(property($derived, $pure_prop->name)->type);
         }
      }

   } elsif ($prop->belongs_to == $self) {
      if ($prop->flags & Property::Flags::is_augmented  and
          !defined($prop->overrides) || $prop->type->parent_property == $prop) {
         return $prop->type;
      }
      $augm = new Augmented($prop, $self);
      $prop->change_to_augmented($augm);
      $update_caches = !($prop->flags & Property::Flags::is_concrete);

   } elsif ($prop->belongs_to == $self->full_spez_for) {
      return Specialization::augment_in_full_spez($self, $prop);

   } else {
      ($augm, $augm_super, $update_caches) = $self->create_augmentation($prop);
      $cloned_prop = $self->properties->{$prop->name} = $prop->clone_for_augmented($augm, $self);
      create_prop_method($self, $cloned_prop);
   }

   if ($update_caches) {
      # derived classes might have cached the original property; update or delete the cache entries
      foreach my $derived ($self->derived) {
         my $cached_prop = $derived->properties->{$prop->name} or next;
         if ($cached_prop == $prop) {
            if (defined($cloned_prop)) {
               # the derived class has inherited the original property, now it is shadowed by the cloned one
               if ($cloned_prop->flags & Property::Flags::is_concrete || $derived->abstract) {
                  $derived->properties->{$prop->name} = $cloned_prop;
               } else {
                  # augmentation is parametrized, the concrete instance will be created later on demand
                  delete $derived->properties->{$prop->name};
               }
            } elsif (!($prop->flags & Property::Flags::is_concrete || $derived->abstract) || $prop->flags & Property::Flags::is_permutation) {
               # augmentation is parametrized, the concrete instance will be created later on demand
               delete $derived->properties->{$prop->name};
            }

         } elsif (defined($augm_super)) {
            # other augmentations downstream have already been treated by the new augmentation itself;
            # only handle classes which inherited a concrete instance of augm_super
            if ($augm_super == $cached_prop->type->generic) {
               if ($derived->generic == ($self->generic // $self)) {
                  # clone the property for this concrete instance;
                  # the augmentation will propagate itself to all concrete descendants
                  my $cloned_concrete_prop =
                    $derived->properties->{$prop->name} =
                    $prop->clone_for_augmented($augm->concrete_type($derived, $cached_prop->type), $derived);
                  create_prop_method($derived, $cloned_concrete_prop);
               } else {
                  # it's an intermediate class without own augmentations
                  delete $derived->properties->{$prop->name};
               }
            }
         } elsif ($cached_prop->flags & Property::Flags::is_augmented) {
            # this augmentation was at the root of the hierarchy, now it has to absorb the new one
            if ($cached_prop->type->super->[0] == $cached_prop->type->pure_type) {
               $cached_prop->type->inject_inheritance($augm);
            }

         } else {
            # cached_prop must be a concrete instance of the generic original property
            ($cached_prop->flags & Property::Flags::is_concrete) > ($prop->flags & Property::Flags::is_concrete)
              or croak( "internal error: unexpected property instance of ", $cached_prop->name,
                        " found in ", $derived->full_name );
            if (defined($cloned_prop) || $derived->generic != ($self->generic // $self)) {
               # the derived class has inherited a concrete instance of the original property;
               # a new concrete instance will be created or fetched later on demand
               delete $derived->properties->{$prop->name};
            } else {
               # we can't delete the property because it's stored in the accessor methods;
               # this will create a new concrete augmentation which won't have any descendants yet
               $cached_prop->change_to_augmented($augm->concrete_type($derived));
            }
         }
      }
   }

   $augm
}

sub create_augmentation {
   my ($self, $prop)=@_;
   my @augm_super= $prop->flags & Property::Flags::is_augmented ? ($prop->type) : ();
   my $augm=new Augmented($prop, $self, @augm_super);
   ($augm, $augm_super[0], 1);
}
####################################################################################
sub help_topic {
   my $self=shift;
   $self->help // ($self->application // return)->help->find_type_topic($self, "objects", @_);
}
####################################################################################
sub override_help {
   my ($self, $group, $item, $text)=@_;
   my ($inherited_topic)=map { $_->help_topic(1)->find($group, $item) } @{$self->linear_isa}
     or croak( "help topic $group/$item not found anywhere in base types of ", $self->full_name );
   if ($inherited_topic->parent->category) {
      $text='# @category ' . $inherited_topic->parent->name . "\n" . $text;
   }
   my $topic=$self->help_topic(1)->add([ $group, $item ], $text);
   foreach (qw(spez property header)) {
      if (defined (my $ref=$inherited_topic->annex->{$_})) {
         if (ref($ref)) {
            weak($topic->annex->{$_}=$ref);
         } else {
            $topic->annex->{$_}=$ref;
         }
      }
   }
   $topic
}
####################################################################################
sub set_file_suffix {
   my ($self, $suffix) = @_;
   define_function($self->pkg, "default_file_suffix", sub : method { $suffix });
   push @{$self->application->file_suffixes}, $suffix;
}
####################################################################################
sub reopen_subobject {
   my ($self, $path) = @_;
   foreach my $prop_name (split /\./, $path) {
      my $prop = property($self, $prop_name);
      if (($prop->flags & (Property::Flags::is_subobject | Property::Flags::is_twin)) == Property::Flags::is_subobject) {
         $self = $self->augment($prop);
      } elsif ($prop->flags & Property::Flags::is_twin) {
         croak( "a twin property cannot be augmented" );
      } else {
         croak( "an atomic property definition cannot be augmented" );
      }
   }
   $self
}
####################################################################################
package Polymake::Core::BigObjectType::RuleLikeMethodNode;

use Polymake::Struct(
   [ '@ISA' => 'Overload::Node' ],
);

# instead of single sub references, every code slot contains a list of rule-like methods

sub store_code {
   my ($self, $i, $rule)=@_;
   push @{$self->code->[$i] //= [ ]}, $rule;
}

sub store_ellipsis_code {
   my ($self, $rule)=@_;
   push @{$self->ellipsis_code //= [ ]}, $rule;
}

sub dup_ellipsis_code {
   my ($self, $upto)=@_;
   push @{$self->code}, map { [ @{$self->ellipsis_code} ] } @{$self->code}..$upto;
}

sub push_code {
   my ($self, $n, $rule)=@_;
   push @{$self->code}, map { [ $rule ] } 1..$n;
}

sub resolve {
   my ($self, $args)=@_;
   if (defined (my $rulelist=&Overload::Node::resolve)) {
      (Scheduler::resolve_rules($args->[0], $rulelist) // croak( "could not provide all required input properties" ))->code;
   } else {
      undef
   }
}

####################################################################################
package Polymake::Core::BigObjectType::Augmented;

use Polymake::Struct (
   [ '@ISA' => 'BigObjectType' ],
   [ new => '$$@' ],
   [ '$name' => '#2->name ."__". #1->name ."__augmented"' ],
   [ '$application' => '#2->application' ],
   [ '$parent_property' => 'undef' ],
   [ '$outer_object_type' => 'undef' ],
   [ '$pure_type' => '#1->type->pure_type' ],
   [ '$enclosed_in_restricted_spez' => '#2->enclosed_in_restricted_spez || defined(#2->preconditions)' ],
   [ '$full_spez_for' => 'undef' ],
   '$is_anon_full_spez',
   '%inst_cache',             # for abstract types: owner BigObjectType => concrete instance
                              # for concrete types: autonomous BigObjectType => Augmented
);

# constructor: (parent) Property, (owner) BigObjectType, super Augmented =>
sub new {
   my $self = &_new;
   my ($prop, $owner, @super) = @_;

   $self->pkg = $owner->pkg."::_prop_".$prop->name =~ s/\.pure$//r;
   define_function($self->pkg, "type", sub { $self }, 1);
   RuleFilter::create_self_method_for_object($self);

   weak($self->outer_object_type = $owner->outer_object_type);

   if ($self->outer_object_type->abstract) {
      $self->context_pkg = $self->outer_object_type->context_pkg;
      $self->abstract = \&type;
   } elsif ($self->pure_type->abstract) {
      $self->pure_type = $self->pure_type->concrete_type($self->outer_object_type);
   }

   my $inherit_pure_type;
   if (!@super && defined($prop->overrides)) {
      my $overridden = $owner->lookup_overridden_property($prop);
      if ($overridden->flags & Property::Flags::is_augmented) {
         push @super, $overridden->type;
         $inherit_pure_type = $overridden->type->pure_type != $self->pure_type;
      }
   }
   modify_super($self, 0, 0, @super, $inherit_pure_type || !@super ? $self->pure_type : ());

   if (instanceof Specialization($scope_owner)) {
      if ($self->abstract) {
         my $augm_generic = $super[0];
         if (instanceof Specialization($augm_generic->outer_object_type)) {
            # derived augmentation of a property introduced in a partial specialization
         } else {
            # introduced a new augmentation in a partial specialization:
            # inject it into all matching concrete instances
            weak($self->generic = $augm_generic);
            push @{$augm_generic->specializations //= [ ]}, $self;

            while (my ($concrete_owner, $concrete_augm) = each %{$augm_generic->inst_cache}) {
               if ($concrete_owner->isa($owner)) {
                  (my $pos = list_index($concrete_augm->super, $augm_generic)) >= 0
                    or croak( "internal error: super(", $concrete_augm->full_name, ") does not contain the generic augmentation ", $augm_generic->full_name);
                  modify_super($concrete_augm, $pos, 0, $self);
                  $_->update_inheritance for $concrete_augm->derived;
               }
            }
         }
      }
      # augmentations in full specializations are included into the concrete instance directly in augment()

   } elsif (@super) {
      propagate_inheritance($self, $owner, @super);
   }

   $self;
}
####################################################################################
use Polymake::Struct (
   [ 'alt.constructor' => 'new_concrete' ],
   [ new => '$' ],
   [ '$name' => '#1->name' ],
   [ '$application' => '#1->application' ],
   [ '$extension' => '#1->extension' ],
   [ '$context_pkg' => '#1->context_pkg' ],
   [ '$generic' => 'weak(#1)' ],
   [ '$pure_type' => 'undef' ],
   [ '$enclosed_in_restricted_spez' => '0' ],
);

sub concrete_type {
   my ($src, $owner, @concrete_super) = @_;
   $src->abstract
     or croak( "internal error: attempt to construct a concrete instance of non-abstract augmentation ", $src->full_name );
   if (defined(my $augm_generic = $src->generic)) {
      $src = $augm_generic;
   }

   $src->inst_cache->{$owner} //= do {
      my $self = new_concrete($src, $src);
      my $prop_name = $src->parent_property->name;
      if ($owner->pkg =~ /::_Full_Spez\b/) {
         croak("internal error: Augmented::concrete_type called from a wrong context");
      }
      $self->pkg = $owner->pkg . "::_prop_" . ($prop_name =~ s/\.pure$//r);
      define_function($self->pkg, "type", sub { $self }, 1);

      weak($self->outer_object_type = $owner->outer_object_type);
      if (!@concrete_super) {
         @concrete_super = map { $_->type } grep { $_->flags & Property::Flags::is_augmented } $owner->collect_augmented_super_instances($prop_name);
      }
      $self->pure_type = @concrete_super
                         ? $concrete_super[0]->pure_type :
                         $src->pure_type->abstract
                         ? $src->pure_type->concrete_type($self->outer_object_type)
                         : $src->pure_type;
      modify_super($self, 0, 0, $src->matching_specializations($owner, $self->pure_type), $src,
                   @concrete_super || $self->pure_type == $src->pure_type ? @concrete_super : $self->pure_type);
      propagate_inheritance($self, $owner, @concrete_super);
      $self;
   };
}

sub concrete_specialization {
   my ($src, $owner, $pure_type)=@_;
   $src->abstract
     or croak( "internal error: attempt to construct a concrete instance of non-abstract augmentation ", $src->full_name );

   # find the concrete instance of the enclosing specialization
   my $owner_spez;
   my $gen_spez=$src->parent_property->belongs_to;
   foreach (@{$owner->linear_isa}) {
      if ($_->generic == $gen_spez) {
         $owner_spez=$_;
         last;
      }
   }

   $owner_spez
     or croak( "internal error: could not find a concrete instance of augmentation ", $gen_spez->full_name, " among ancestors of ", $owner->full_name );

   $src->inst_cache->{$owner_spez} //= do {
      my $self=new_concrete($src, $src);
      my $prop_name=$src->parent_property->name;
      $self->pkg=$owner_spez->pkg."::_prop_${prop_name}";
      define_function($self->pkg, "type", sub { $self }, 1);
      $self->pure_type=$pure_type;
      weak($self->outer_object_type=$owner_spez->outer_object_type);
      if ($src->generic) {
         # property defined in the generic enclosing type
         modify_super($self, 0, 0, $src);
      } else {
         # property borrowed from another specialization
         modify_super($self, 0, 0, $src, concrete_specialization($src->super->[0], $owner, $pure_type));
      }
      $self;
   };
}

sub matching_specializations {
   my ($self, $owner, $pure_type)=@_;
   if ($self->specializations) {
      map { concrete_specialization($_, $owner, $pure_type) } grep { $owner->isa($_->parent_property->belongs_to) } @{$self->specializations};
   } else {
      ()
   }
}

sub inject_inheritance {
   my ($self, $augm_super)=@_;
   $self->super->[0]=$augm_super;
   namespaces::using($self->pkg, $augm_super->pkg);
   {  no strict 'refs';
      ${$self->pkg."::ISA"}[0]=$augm_super->pkg;
   }
   if ($augm_super->abstract && $self->abstract) {
      my $super_generic=$augm_super->parent_property->belongs_to;
      while (my ($concrete_owner, $concrete_augm)=each %{$self->inst_cache}) {
         foreach my $super_concrete ($concrete_owner->concrete_super_instances) {
            if ($super_concrete->isa($super_generic)) {
               # the concrete augmentation can't have any concrete super classes so far
               modify_super($concrete_augm, -1, 0, $augm_super->concrete_type($super_concrete->descend_to_generic($super_generic->pkg)));
               last;
            }
         }
      }
   }
   $_->update_inheritance for $self, $self->derived;
}

sub propagate_inheritance {
   my ($self, $owner, @override)=@_;
   my %overridden;
   @overridden{@override}=();

   foreach my $derived (map { $_->derived } @override) {
      if ($derived != $self &&
          ($derived->abstract || !$self->abstract) &&
          $derived->parent_property->belongs_to->isa($owner)) {
         my $replaced=0;
         for (my $i=0; $i<=$#{$derived->super}; ) {
            if (exists $overridden{$derived->super->[$i]}) {
               if ($replaced) {
                  splice @{$derived->super}, $i, 1;
                  no strict 'refs';
                  splice @{$derived->pkg."::ISA"}, $i, 1;
               } else {
                  namespaces::using($derived->pkg, $self->pkg);
                  $derived->super->[$i]=$self;
                  no strict 'refs';
                  ${$derived->pkg."::ISA"}[$i]=$self->pkg;
                  ++$i;
                  $replaced=1;
               }
            } else {
               ++$i;
            }
         }
         if ($replaced) {
            $_->update_inheritance for $derived, $derived->derived;
         }
      }
   }
}

####################################################################################
sub final_type {
   my ($self, $given_type)=@_;
   $self->abstract || $given_type->abstract
     and croak( "internal error: attempt to construct a concrete augmented type from ",
                ($self->abstract && "abstract base "), $self->full_name,
                " and ", ($autonomous_type->abstract && "abstract autonomous type "), $autonomous_type->full_name );
   my $autonomous_type=$given_type->pure_type;
   if ($autonomous_type == $self->pure_type) {
      $self
   } else {
      $self->inst_cache->{$autonomous_type} //= create_derived($self, $self->parent_property, $autonomous_type);
   }
}
####################################################################################
sub full_name {
   my ($self)=@_;
   (@_==1 && $self->pure_type->full_name . " as ") . $self->parent_property->belongs_to->full_name(0) . "::" . $self->parent_property->name;
}

sub descend_to_generic {
   $_[0]->outer_object_type->descend_to_generic($_[1]) // $_[0]->pure_type->descend_to_generic($_[1]);
}

sub concrete_super_instances {
   my ($self)=@_;
   $self->super->[0] == $self->pure_type ? () : &BigObjectType::concrete_super_instances;
}
####################################################################################
sub augmentation_path {
   my ($self)=@_;
   my ($parent_prop, @path);
   while (defined ($parent_prop=$self->parent_property)) {
      push @path, $parent_prop;
      $self=$parent_prop->belongs_to;
   }
   reverse @path;
}

sub create_augmentation {
   my ($self, $prop) = @_;
   if (instanceof Specialization($scope_owner)) {
      &Specialization::create_augmentation;
   } else {
      my ($augm, @augm_super);
      my @owner_augm_path = augmentation_path($self);
      my @prop_augm_path = augmentation_path($prop->belongs_to);
      if (@prop_augm_path == @owner_augm_path) {
         # there is already another augmentation on the same nesting level
         push @augm_super, $prop->type;
      } else {
         # try to find or establish an augmentation with one or more outer enclosing object types stripped off
       STRIP_OFF:
         while (@owner_augm_path) {
            my $outer = shift(@owner_augm_path)->type->pure_type;
            if ($outer->abstract && defined($outer->generic)) {
               # pass through dependent abstract types
               $outer = $outer->generic;
            }
            foreach (@owner_augm_path, $prop) {
               if ($outer->isa($_->defined_for)) {
                  my $sub_prop = $outer->property($_->name);
                  $outer = $outer->augment($sub_prop);
               } else {
                  next STRIP_OFF;
               }
            }
            push @augm_super, $outer;
            last;
         }
      }
      $augm = new Augmented($prop, $self, @augm_super);
      ($augm, $augm_super[0], 1);
   }
}
####################################################################################
sub find_rule_label {
   &BigObjectType::find_rule_label // do {
      my $pure_type_app=$_[0]->pure_type->application;
      if ($pure_type_app != $_[0]->application) {
         $pure_type_app->prefs->find_label($_[1])
      } else {
         undef
      }
   }
}
####################################################################################
# private:
sub provide_help_topic {
   my ($owner, $force, @descend) = @_;
   my $topic = $owner->help_topic($force) or return;
   foreach my $prop (@descend) {
      my $help_group = $prop->flags & Property::Flags::is_permutation ? "permutations" : "properties";
      $topic = $topic->find("!rel", $help_group, $prop->name) // do {
         $force or return;
         my $text = "";
         my $refer_to = $prop->defined_for;
         if ($owner != $refer_to) {
            if (my $prop_topic = $prop->belongs_to->help_topic->find($help_group, $prop->name)) {
               if ($prop_topic->parent->category) {
                  $text='# @category ' . $prop_topic->parent->name . "\n";
               }
            }
            $text .= "Augmented subobject " . $refer_to->full_name . "::" . $prop->name . "\n"
                   . "# \@display noshow\n";
         }
         $topic = $topic->add([ $help_group, $prop->name ], $text);
         $topic->annex->{property}=$prop;
         $topic
      };
      $owner = $prop->belongs_to;
   }
   $topic
}

sub help_topic {
   my ($self, $force) = @_;
   $self->help //= do {
      if ($scope_owner) {
         # need a help node during rulefile loading
         my @descend = augmentation_path($self);
         if (instanceof Specialization($scope_owner)) {
            my $gen_topic = provide_help_topic($scope_owner->generic // $scope_owner->full_spez_for, $force, @descend) or return;
            ($self->generic // $self->full_spez_for)->help //= $gen_topic;
            $gen_topic->new_specialization($scope_owner);
         } elsif ($scope_owner != $descend[0]->belongs_to) {
            $force and die "internal error: augmentation path does not match the compilation scope\n";
            return;
         } else {
            my $topic = provide_help_topic($scope_owner, $force, @descend) or return;
            push @{$topic->related}, uniq( map { ($_, @{$_->related}) } grep { defined($_) && ref($_) !~ /\bSpecialization$/ }
                                           map { $_->help_topic } @{$self->linear_isa} );
            foreach my $other ($self->derived) {
               if (defined($other->help)) {
                  push @{$other->help->related}, $topic;
               }
            }
            $topic
         }
      } else {
         $self->pure_type->help_topic;
      }
   };
}

sub help_ref_to_prop {
   my ($self, $for_app, $prop_name) = @_;
   $self->outer_object_type->help_ref_to_prop($for_app, join(".", (map { $_->name } augmentation_path($self)), $prop_name));
}
####################################################################################
use Polymake::Struct (
   [ 'alt.constructor' => 'new_derived' ],
   [ new => '$$$' ],
   [ '$name' => '#1->name' ],
   [ '$application' => '#1->application' ],
   [ '$extension' => '#1->extension' ],
   [ '$parent_property' => 'weak(#2)' ],
   [ '$outer_object_type' => '#1->outer_object_type' ],
   [ '$pure_type' => '#3' ],
   [ '$enclosed_in_restricted_spez' => '#1->enclosed_in_restricted_spez' ],
   [ '$inst_cache' => 'undef' ],
);

sub create_derived {
   my ($src, $prop, $autonomous_type) = @_;
   my $self = new_derived($prop->flags & Property::Flags::is_permutation ? "Polymake::Core::BigObjectType::Permuted" : __PACKAGE__, @_);
   $self->pkg = $autonomous_type->pkg."::__as__".$src->pkg;
   define_function($self->pkg, "type", sub { $self }, 1);
   modify_super($self, 0, 0, $src, $autonomous_type);
   set_extension($self->extension, $autonomous_type->extension);
   if ($autonomous_type->abstract) {
      $self->abstract = \&type;
   }
   $self;
}
####################################################################################
package Polymake::Core::BigObjectType::Permuted;

use Polymake::Struct (
   [ '@ISA' => 'Augmented' ],
);

sub get_producers_of {
   my ($self)=@_;
   # No production rules should be inherited from the non-permuted type, otherwise the scheduler would drive crazy.
   # Therefore the collection should stop in the pure permutation type.
   BigObjectType::get_producers_of(@_, $self->super->[0]->pure_type);
}

sub pure_property { $_[0]->super->[0]->parent_property }

sub concrete_type {
   my ($self, $owner) = @_;
   create_derived($owner->property(&pure_property->name)->type, $self->parent_property, $owner);
}

sub full_name {
   my ($self)=@_;
   $self->pure_type->full_name . "::" . $self->parent_property->name
}
####################################################################################
package Polymake::Core::BigObjectType::Specialization;

use Polymake::Struct (
   [ '@ISA' => 'BigObjectType' ],
   [ new => '$$$;$' ],
   [ '$application' => '#3->application' ],
   [ '$match_node' => 'undef' ],
   [ '$preconditions' => 'undef' ],
   [ '$full_spez_for' => 'undef' ],
   '$is_anon_full_spez',
);

# Constructor: new Specialization('name', 'pkg', generic_ObjectType | generic_Specialization | concrete_ObjectType, [ type params ] )
sub new {
   my $self = &_new;
   my (undef, $pkg, $gen_proto, $tparams) = @_;
   my $concrete_proto;

   if (defined($pkg)) {
      $self->pkg = $pkg;
      if ($tparams) {
         # new abstract specialization
         $self->abstract = \&type;
         undef $self->perform_typecheck;
         my $param_index = -1;
         $self->params = [ map { new ClassTypeParam($_, $pkg, $self->application, ++$param_index) } @$tparams ];
         $self->name //= do {
            my (undef, $file, $line) = caller;
            $def_line -= 2;       # accounting for code inserted in RuleFilter
            $gen_proto->name." defined at $file, line $line"
         };
         $self->match_node = new_root Overload::Node;
         push @{$gen_proto->specializations //= [ ]}, $self;
      } else {
         # new full specialization
         $concrete_proto = $gen_proto;
         $gen_proto = $concrete_proto->generic;
         if (defined($self->name)) {
            (($gen_proto // $concrete_proto)->full_spez //= { })->{$self->name} = $self;
         } else {
            $self->is_anon_full_spez = true;
            $self->name = $concrete_proto->full_name;
         }
         $self->params = $concrete_proto->params;
      }
   } else {
      # a concrete instance of a specialization
      $self->name = $gen_proto->name;
      $self->extension = $gen_proto->extension;
      $self->pkg = $gen_proto->pkg;
      $self->params = $tparams;
      PropertyParamedType::scan_params($self);
   }
   my $self_sub = define_function($self->pkg, "type", sub { $self }, 1);
   $self->generic = $gen_proto;
   if (defined($concrete_proto)) {
      RuleFilter::create_self_method_for_object($self);

      weak($self->full_spez_for = $concrete_proto);

      # inherit all the same as the concrete type but restricted full specializations
      my ($anon_pos, $pos) = positions_of_full_spez($concrete_proto);
      modify_super($self, 0, 0, @{$concrete_proto->super}[$pos..$#{$concrete_proto->super}]);
      if ($self->is_anon_full_spez) {
         if (defined($anon_pos)) {
            croak( "internal error: created duplicate instance for full specialization ",
                   $concrete_proto->super->[$anon_pos]->full_name, " (", $concrete_proto->super->[$anon_pos]->pkg,
                   ") and ", $self->full_name, " (", $self->pkg, ")" );
         }
         # inject it in restricted specializations too, if any
         modify_super($_, 0, 0, $self) for @{$concrete_proto->super}[0..$pos-1];
      }
      modify_super($concrete_proto, $pos, 0, $self);
      $_->update_inheritance for $concrete_proto->derived;

   } else {
      modify_super($self, 0, 0, $gen_proto);
   }
   $self;
}

# inject concrete instances of this specialization into all matching object types
sub apply_to_existing_types {
   my ($self) = @_;
   my $gen_proto = $self->generic;
   my @concrete_types = grep { !$_->abstract } $gen_proto->derived;
   # first update the ISA lists of matching instances of the same generic type
   foreach my $concrete (@concrete_types) {
      if ($concrete->generic == $gen_proto
            and
          my ($spez) = pick_specialization($self, $concrete)) {
         # all partial specializations go in front of the generic type
         modify_super($concrete, list_index($concrete->super, $gen_proto), 0, $spez);
      }
   }
   # then regenerate the super lists of all derived types
   $_->update_inheritance for @concrete_types;
}

# concrete BigObjectType -> indexes into its super list
sub positions_of_full_spez {
   my ($self) = @_;
   my $pos = 0;
   foreach my $spez (@{$self->super}) {
      last if $spez->full_spez_for != $self;
      return ($pos, $pos) if $spez->is_anon_full_spez;
      ++$pos;
   }
   (undef, $pos)
}
####################################################################################
sub augment_in_full_spez {
   my ($self, $prop) = @_;
   my ($augm, $augm_generic, $augm_concrete);

   if ($prop->flags & Property::Flags::is_augmented) {
      $augm_concrete = $prop->type;
      $augm_generic = $augm_concrete->generic;
   } elsif (defined(my $gen_proto = $self->full_spez_for->generic)) {
      $augm_generic = $gen_proto->augment($gen_proto->property($prop->name));
      # $prop must be updated here
      $prop->flags & Property::Flags::is_augmented
        or croak( "internal error: property instance ", $prop->name, " for ", $prop->belongs_to->full_name,
                  " not updated after augment(", $gen_proto->full_name, ")" );
      $augm_concrete = $prop->type;
   } else {
      $augm_concrete = $self->full_spez_for->augment($prop);
      $prop->flags & Property::Flags::is_augmented
        or croak( "internal error: property instance ", $prop->name, " for ", $prop->belongs_to->full_name,
                  " not updated after augment(", $self->full_spez_for->full_name, ")" );
   }

   my ($anon_pos, $pos) = positions_of_full_spez($augm_concrete);
   if ($scope_owner->is_anon_full_spez) {
      if (defined($anon_pos)) {
         # already there?
         return $augm_concrete->super->[$anon_pos];
      }
   } elsif ($pos > 0) {
      # seen augmentations from some full specializations: maybe the desired one already exists?
      foreach my $spez (@{$augm_concrete->super}[0..$pos-1]) {
         if ($spez->parent_property->belongs_to == $self) {
            return $spez;
         }
      }
   }

   # need a separate augmentation for this specialization
   # inherit all the same as the concrete type but other restricted full specializations
   $augm = new Augmented($prop, $self, @{$augm_concrete->super}[$pos..$#{$augm_concrete->super}]);
   if ($scope_owner->is_anon_full_spez) {
      $augm->is_anon_full_spez = true;
      weak($augm->parent_property = $prop);
      $self->properties->{$prop->name} = $prop;
      # inject it into inheritance lists of augmentations for restricted specializations, if any
      modify_super($_, 0, 0, $augm) for @{$augm_concrete->super}[0..$pos-1];
   } else {
      $self->properties->{$prop->name} = $prop->clone_for_augmented($augm, $self);
   }
   $augm->generic = $augm_generic;
   weak($augm->full_spez_for = $augm_concrete);
   if ($augm_generic) {
      $augm_generic->inst_cache->{$self} = $augm;
   }
   modify_super($augm_concrete, $pos, 0, $augm);

   $augm
}

sub create_augmentation {
   my ($self, $prop) = @_;
   if (my $concrete_proto = $scope_owner->full_spez_for) {
      if ($concrete_proto != $self->full_spez_for) {
         # it's a nested augmentation; redirected here from Augmented::
         $concrete_proto = $self->full_spez_for // $self;
      }
      if ($concrete_proto->generic) {
         local $scope_owner = $concrete_proto->generic->outer_object_type;
         $concrete_proto->generic->augment($concrete_proto->generic->property($prop->name));
         $prop = $concrete_proto->property($prop->name);
      } else {
         $prop = $concrete_proto->augment($prop)->parent_property;
      }
      augment_in_full_spez($self, $prop);

   } elsif (instanceof Specialization($prop->belongs_to->outer_object_type) && $prop->belongs_to->abstract) {
      # The property must have been introduced in another specialization (borrowed from).
      my $augm_super = $prop->belongs_to->augment($prop);
      (new Augmented($augm_super->parent_property, $self, $augm_super), $augm_super, 1);

   } else {
      # The property is introduced in or inherited by the generic part of the object type:
      # always make sure there is a generic augmentation for it, even if not explicitly declared in the rules.
      my $augm_gen = do {
         # prevent endless recursion when redirected here from Augmented::
         local $scope_owner = $self->generic->outer_object_type;
         $self->generic->augment($prop);
      };
      new Augmented($augm_gen->parent_property, $self, $augm_gen);
   }
}
####################################################################################
sub append_precondition {
   my ($self, $header, $code, $checks_definedness) = @_;
   my $precond = special Rule($header, $code, $self);
   $precond->flags = ($checks_definedness ? Rule::Flags::is_precondition | Rule::Flags::is_definedness_check : Rule::Flags::is_precondition) | Rule::Flags::is_spez_precondition;
   $precond->header = "precondition " . $precond->header . " ( specialization " . $self->name . " )";
   push @{$self->preconditions //= [ ]}, $precond;
}

sub add_permutation {
   croak( "A type specialization can't have own permutation types; consider introducing a derived `big' object type for this" );
}
####################################################################################
sub full_name {
   my ($self)=@_;
   if (defined $self->full_spez_for) {
      # full specialization
      $self->name . " (specialization)"
   } elsif ($self->name =~ / defined at /) {
      # unnamed partial specialization
      "$`<" . join(",", map { $_->full_name } @{$self->params}) . ">$&$'"
   } else {
      # named partial specialization
      &PropertyParamedType::full_name
   }
}

sub help_ref_to_prop {
   my $self=shift;
   ($self->full_spez_for // $self->generic)->help_ref_to_prop(@_)
}

sub help_topic {
   my ($self, $force)=@_;
   $self->help // do {
      if ($force) {
         if ($enable_plausibility_checks && $self->name =~ / defined at /) {
            croak( "For the sake of better documentation, specializations introducing user-visible features (properties, methods, etc.) must be named" );
         }
         $self->help=($self->generic // $self->full_spez_for)->help_topic($force)->new_specialization($self);
      } else {
         undef
      }
   }
}

sub pure_type {
   my ($proto)=@_;
   if (defined $proto->full_spez_for) {
      $proto=$proto->full_spez_for;
   }
   while (defined $proto->generic) {
      $proto=$proto->generic;
   }
   $proto
}
####################################################################################
# special prototype objects for Array<BigObject>

package Polymake::Core::BigObjectArray;

use Polymake::Struct (
   [ '@ISA' => 'PropertyType' ],
   [ new => '' ],
   [ '$name' => '"BigObjectArray"' ],
   [ '$pkg' => '__PACKAGE__' ],
   [ '$application' => 'undef' ],
   [ '$extension' => 'undef' ],
   [ '$dimension' => '1' ],
   [ '&serialize' => '\&serialize_func' ],
   [ '&deserialize' => '\&deserialize_meth' ],
   [ '&equal' => '\&equal_sub' ],
   [ '&isa' => '\&isa_meth' ],
);

sub serialize_func {
   my ($arr, $options) = @_;
   [ map { $_->serialize($options) } @$arr ]
}

sub deserialize_meth : method {
   my ($self, $src, @options) = @_;
   if (is_array($src)) {
      my $obj_proto = $self->params->[0];
      bless [ map { BigObject::deserialize($obj_proto, $_, @options) } @$src ], $self->pkg
   } else {
      croak( "wrong serialized representation of ", $self->full_name, " - anonymous array expected" );
   }
}

Struct::pass_original_object(\&deserialize_meth);

sub equal_sub {
   my ($arr1, $arr2) = @_;
   my $e = $#$arr1;
   return 0 if $e != $#$arr2;
   for (my $i = 0; $i <= $e; ++$i) {
      return 0 if $arr1->[$i]->diff($arr2->[$i]);
   }
   1
}

sub isa_meth : method {
   my ($self, $arr) = @_;
   UNIVERSAL::isa($arr, __PACKAGE__) && $arr->type->params->[0]->isa($self->params->[0]);
}

Struct::pass_original_object(\&isa_meth);

sub construct_from_list : method {
   my ($self, $list) = @_;
   # the contained Objects have already been checked during overload resolution
   bless $list, $self->pkg;
}

sub construct_with_size : method {
   my ($self, $n) = @_;
   my $elem_type = $self->params->[0];
   bless [ map { BigObject::new_named($elem_type, $_) } 0..$n-1 ], $self->pkg;
}

sub resize {
   my ($self, $n) = @_;
   my $old_size = @$self;
   if ($n < $old_size) {
      $#$self = $n-1;
   } else {
      my $elem_type = $self->type->params->[0];
      push @$self, map { BigObject::new_named($elem_type, $_) } $old_size..$n-1;
   }
}

sub copy {
   my ($self) = @_;
   inherit_class([ map { $_->copy } @$self ], $self);
}

# For the exotic case of size passed in string form.
# Everything else will lead to an exception.
sub construct_with_size_str : method {
   construct_with_size($_[0], extract_integer($_[1]));
}

sub new_generic {
   my $self = &_new;
   $self->construct_node = new_root Overload::Node;
   Overload::add_instance(__PACKAGE__, ".construct", undef, \&construct_with_size,
                          [ 1, 1, Overload::integer_package() ], undef, $self->construct_node);
   $self;
}

sub typeof { state $me = &new_generic; }

sub init_constructor : method {
   my ($super, $self) = @_;
   $self->construct_node = $super->construct_node;
   Overload::add_instance($self->pkg, ".construct", undef, \&construct_from_list,
                          [ 1, 1+Overload::SignatureFlags::has_repeated, [ $self->params->[0]->pkg, "+" ] ], undef, $self->construct_node);

   $self->parse = \&construct_with_size_str;
   overload::OVERLOAD($self->pkg, '""' => ($self->toString = sub { "BigObjectArray" }));

   # put BigObjectArray in front of generic Array as to inherit the correct constructors
   # TODO: find a more elegant/generic way of achieving this, e.g. by specializing the generic types in the rules.
   no strict 'refs';
   @{$self->pkg."::ISA"} = (__PACKAGE__, $self->generic->pkg);
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
