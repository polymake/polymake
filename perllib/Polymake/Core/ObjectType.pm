#  Copyright (c) 1997-2016
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
use feature 'state';

package Polymake::Core::ObjectType;

my $construct_node;
sub construct_node : lvalue { $construct_node }

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
   [ '$extension' => '$Application::extension' ],
   '$pkg',                              # perl class
   [ '$context_pkg' => 'undef' ],       # for abstract parameterized object types resulting from local derivation
                                        #  as a property of another parameterized ObjectType:
                                        #  perl class of the enclosing ObjectType (i.e. property owner)
   [ '$params' => 'undef' ],            # for parameterized types: list of ObjectType or PropertyType describing the parameters
   '&abstract',
   [ '&perform_typecheck' => '\&PropertyType::concrete_typecheck' ],
   [ '&construct' => '\&PropertyType::construct_object' ],
   [ '&parse' => '\&Object::new_named' ],
   [ '&toString' => '\&Object::print_me' ],
#
#  Derivation relations
#
   '@super',                       # ( ObjectType )  super classes: transitive closure in MRO C3 order
   [ '$generic' => 'undef' ],      # for instances of parametrized types: ObjectType of the own generic type
   [ '$full_spez' => 'undef' ],    # for parametrized types: concrete ObjectType => Specialization of its full specialization
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
   [ '$help' => 'undef' ],         # InteractiveHelp (in interactive mode, when comments are supplied for properties, user methods, etc.)
);

####################################################################################
#
#  Constructor
#
#  new ObjectType('name', Application, [ type params ], [ super ObjectType, ... ]);
#
sub new {
   my $self=&_new;
   Overload::learn_package_retrieval($self, \&pkg);
   my $tparams=splice @_, 0, 3;
   my $self_sub=sub { $self };
   my $generic_type;

   unless ($construct_node) {
      $construct_node=new Overload::Node(undef, undef, 0);
      Overload::add_instance("Polymake::Core::Object", ".construct",
                             \&Object::new_empty, undef,
                             [0, 0], undef, $construct_node);
      Overload::add_fallback_to_node($construct_node, \&Object::new_filled);
   }

   if (defined($self->application)) {
      $self->pkg=$self->application->pkg."::".$self->name;
      if (defined $tparams) {
         $self->abstract=\&type;
         undef $self->perform_typecheck;
         $self->params=new_generic PropertyParamedType($self->name, $self->pkg, $self->application, $tparams, undef, "objects");
      }
      PropertyType::create_method_new($self);
      Overload::add_instance($self->pkg, ".construct", \&Object::new_copy,        undef,
                             [1, 1, $self->pkg ], undef, $construct_node);
      Overload::add_instance($self->pkg, ".construct", \&Object::new_filled_copy, undef,
                             [3, 3+$Overload::has_trailing_list, $self->pkg, '$', '$'], undef, $construct_node);
   } else {
      # it is an instance of an abstract Object type
      # the first super-Object is always the own abstract type
      $generic_type=$_[0];
      $self->application=$generic_type->application;
      $self->extension=$generic_type->extension;
      $self->pkg=$generic_type->pkg;
      $self->params=$tparams;
      PropertyParamedType::scan_params($self);
      if ($self->abstract) {
         if (defined($self->context_pkg)) {
            push @{$self->super}, $generic_type, @{$generic_type->super};
         }
         return $self;
      }
      define_function($self->pkg, "typeof", $self_sub, 1);
   }

   define_function($self->pkg, "type", $self_sub, 1);
   if (!$self->abstract) {
      define_function($self->pkg, ".type", $self_sub);
   }

   # proceed with parent classes
   if (@_) {
      establish_inheritance($self, map { PropertyParamedType::set_extension($self->extension, $_->extension); $_->pkg } @_);
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

            my @spez_pkgs=map { $_->pkg } @matching_spezs;
            namespaces::using($self->pkg, @spez_pkgs);
            no strict 'refs';
            unshift @{$self->pkg."::ISA"}, @spez_pkgs;
            update_inheritance($self);
         }

         # verify correctness of inheritance relationship between specializations
         while (my ($spez, $status)=each %required_spezs) {
            if ($status == 1 && list_index($self->super, $spez)<0) {
               my @bad=grep { defined($_->borrowed_from) && $_->borrowed_from->{$spez} } @{$self->super};
               die "Invalid inheritance: ", join(", ", map { $_->full_name } @bad),
                            "\n refer", @bad==1 && "s", " to properties defined in ", $spez->full_name,
                            "\n which does not match the object type ", $self->full_name, "\n";
            }
         }
      }
   } else {
      no strict 'refs';
      push @{$self->pkg."::ISA"}, "Polymake::Core::Object";
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
     map { defined($_->specializations) ? @{$_->specializations} : () } @{$self->super}
   )
}
####################################################################################
sub full_specialization {
   my ($self, $concrete)=@_;
   ($self->full_spez //= { })->{$concrete->pkg} //= new Specialization(undef, $concrete->pkg."::_Full_Spez", $concrete);
}
####################################################################################
sub establish_inheritance {
   my $self=shift;
   namespaces::using($self->pkg, @_);
   { no strict 'refs'; push @{$self->pkg."::ISA"}, @_; }
   mro::set_mro($self->pkg, "c3");
   my $linear_isa=mro::get_linear_isa($self->pkg);
   # ignore the type itself and ubiquitous Object
   @{$self->super}=map { $_->type } grep { !/^Polymake::Core::/ } @$linear_isa[1..$#$linear_isa];
}

sub update_inheritance {
   my ($self)=@_;
   my $linear_isa=mro::get_linear_isa($self->pkg);
   @{$self->super}=map { $_->type } grep { !/^Polymake::Core::/ } @$linear_isa[1..$#$linear_isa];
}

sub derived {
   my ($self)=@_;
   map { $_->type } @{mro::get_isarev($self->pkg)}
}
####################################################################################
# for compatibility with PropertyType:

*type=\&PropertyType::type;
*mangled_name=\&PropertyParamedType::mangled_name;
*full_name=\&PropertyParamedType::full_name;
*typecheck=\&PropertyType::typecheck;
*add_constructor=\&PropertyType::add_constructor;
*performs_deduction=\&PropertyParamedType::performs_deduction;
*type_param_index=\&PropertyType::type_param_index;
define_function(__PACKAGE__, ".type", \&type);

sub qualified_name {
   defined($_[0]->params) ? &PropertyParamedType::qualified_name : &PropertyType::qualified_name
}

sub concrete_type {
   # undef context_pkg means we are in a generic parameterized object class, hence nothing to deduce
   defined($_[0]->context_pkg) ? &PropertyParamedType::concrete_type : pop
}

sub descend_to_generic {
   my ($self, $pkg)=@_;
   if (@{$self->super}) {
      return $self if defined($self->generic) && $self->generic->pkg eq $pkg;
      foreach my $super_proto (@{$self->super}) {
         return $super_proto if defined($super_proto->generic) && $super_proto->generic->pkg eq $pkg;
      }
   }
   undef
}
####################################################################################
sub isa {
   my ($self, $other)=@_;
   if (is_string($other)) {
      $other=$self->application->eval_type($other, 1) or return;
   }
   return $self==$other || list_index($self->super, $other)>=0;
}
####################################################################################
# "property name" => Property or undef
sub lookup_property {
   my ($self, $prop_name)=@_;
   # don't cache negative results
   $self->properties->{$prop_name} // do {
      my $prop;
      foreach (@{$self->super}) {
         if (defined ($prop=$_->properties->{$prop_name})) {
            return $self->properties->{$prop_name}=$prop;
         }
      }
      if ($self->abstract) {
         my $gen_proto=$self->generic // $self;
         foreach my $spez ($gen_proto->all_partial_specializations($self)) {
            if ($spez != $self &&
                defined ($prop=$spez->properties->{$prop_name})) {

               ($self->borrowed_from //= { })->{$spez} ||= do {
                  # verify the correctness of borrowing
                  foreach my $proto (grep { $_->generic==$gen_proto } $self->derived) {
                     if (list_index($proto->super, $spez)<0) {
                        die "Invalid inheritance: ", $self->full_name,
                        "\n refers to property $prop_name defined in ", $spez->full_name,
                        "\n which does not match the object type ", $proto->full_name, "\n";
                     }
                  }
                  1
               };
               return $self->properties->{$prop_name}=$prop;
            }
         }
      }
      undef
   }
}

sub property {
   my ($self, $prop_name)=@_;
   $self->lookup_property($prop_name) or croak( "unknown property ", $self->full_name, "::$prop_name" );
}

sub init_pseudo_prop {
   state $prop=_new Property(".initial", undef, undef);
}

####################################################################################
# "property name" => bool
# Returns TRUE if the property is overridden for this object type or one of its ancestors.
# Returns undef if the property is not known at all.
sub is_overridden {
   my ($self, $prop_name)=@_;
   if (defined (my $prop=&lookup_property)) {
      $prop->name ne $prop_name;
   } else {
      undef
   }
}
####################################################################################
# Property => "name"
# Returns the name of the overriding property if appropriate.
sub property_name {
   my ($self, $prop)=@_;
   lookup_property($self, $prop->name)->name
}
####################################################################################
sub list_permutations {
   my $self=shift;
   (values %{$self->permutations}), (map { values %{$_->permutations} } @{$self->super})
}
####################################################################################
# private:
sub add_property {      # => Property
   my ($self, $prop)=@_;
   if ($Application::plausibility_checks && defined (my $old_prop=$self->lookup_property($prop->name))) {
      croak( $old_prop->belongs_to == $self
             ? "multiple definition of property '".$prop->name."'"
             : "redefinition of inherited property '".$prop->name."' not allowed" );
   }

   define_function($self->pkg, $prop->name,
                   create_prop_accessor($prop->flags & $Property::is_multiple
                                        ? [ $prop, \&Object::get_multi, \&Object::put_multi ]
                                        : [ $prop, \&Object::get,       \&Object::put       ],
                                        $self->pkg));

   if ($prop->flags & $Property::is_permutation) {
      $self->permutations->{$prop->name}=$prop;
   } else {
      $self->application->EXPORT->{$prop->name}="prop";
   }
   $self->properties->{$prop->name}=$prop;
}
####################################################################################
sub add_property_alias {        # "new name", "old name", Help, override => Property
   my ($self, $prop_name, $old_prop_name, $help, $override)=@_;
   my $old_prop=$self->lookup_property($old_prop_name)
      or croak( "unknown property ", $self->name, "::$old_prop_name" );
   my $new_prop=add_property($self, $old_prop->create_alias($prop_name, $override && $self));
   if ($override) {
      if ($Application::plausibility_checks and
          $old_prop->belongs_to == $self ||
          $old_prop->belongs_to == $self->generic) {
         croak( "only properties inherited from other object types can be overridden" );
      }
      $self->properties->{$old_prop_name}=$new_prop;
      define_function($self->pkg, $old_prop_name, UNIVERSAL::can($self->pkg, $prop_name));
   }
   if (defined $help) {
      $help->annex->{header}="property $prop_name : ".$new_prop->type->full_name."\n";
      weak($help->annex->{property}=$new_prop);
      $help->text .= " Alias for property [[" . $old_prop->belongs_to->pure_type->name . "::$old_prop_name]].\n";
   }
   $new_prop;
}
####################################################################################
sub add_permutation {
   add_property($_[0], new Permutation(@_));
}
####################################################################################
# protected:
sub invalidate_prod_cache {
   my ($self, $key)=@_;
   if (defined (delete $self->all_producers->{$key})) {
      delete $self->shortcuts->{$key};
      invalidate_prod_cache($_, $key) for $self->derived;
   }
}


sub rule_is_applicable {
   my ($self, $rule)=@_;
   if (defined (my $overridden_in=$rule->overridden_in)) {
      foreach my $other (@$overridden_in) {
         return 0 if $self==$other || list_index($self->super, $other)>=0;
      }
   }
   1
}

sub get_producers_of {
   my ($self, $prop)=@_;
   $self->all_producers->{$prop->key} //= do {
      my @list;
      if (defined (my $own_prod=$self->producers->{$prop->key})) {
         @list=@$own_prod;
      }
      if ($prop->belongs_to != $self) {
         foreach my $super_proto (@{$self->super}) {
            if (defined (my $super_prod=$super_proto->producers->{$prop->key})) {
               push @list, grep { rule_is_applicable($self, $_) } @$super_prod;
            }
         }
      }
      \@list
   };
}

# only shortcuts directly creating the property asked for are stored here
sub get_shortcuts_for {
   my ($self, $prop)=@_;
   $self->shortcuts->{$prop->key} //=
      [ grep { instanceof Rule::Shortcut($_) and
               $_->output->[0]->[-1]->key == $prop->property_key
        } @{ $self->get_producers_of($prop) }
      ]
}
####################################################################################
# protected:
sub invalidate_label_cache {
   my ($self, $wildcard)=@_;
   if (defined (delete $self->all_rules_by_label->{$wildcard})) {
      invalidate_label_cache($_,$wildcard) for $self->derived;
   }
}

sub get_rules_by_label {
   my ($self, $wildcard)=@_;
   $self->all_rules_by_label->{$wildcard} ||= do {
      # signal for invalidate_label_cache should it happen later
      $_->all_rules_by_label->{$wildcard} //= 0 for @{$self->super};
      Preference::merge_controls(grep { defined($_) } map { $_->rules_by_label->{$wildcard} } $self, @{$self->super})
   };
}

####################################################################################
# protected:
sub add_rule_labels {
   my ($self, $rule, $labels)=@_;
   foreach my $label (@$labels) {
      my $wildcard=$label->wildcard_name;
      my $ctl_list;
      if (defined ($ctl_list=$self->rules_by_label->{$wildcard})) {
         invalidate_label_cache($self, $wildcard);
      } else {
         $self->rules_by_label->{$wildcard}=$ctl_list=[ ];
      }
      $label->add_control($ctl_list, $rule);
   }
}
####################################################################################
# protected:
# "NAME.NAME" => (Property)
sub encode_property_list {
   my ($self, $req, $obj)=@_;
   my $prop;
   my $allow_abstract=!defined($obj) && $self->abstract;
   map {
      if (defined($prop)) {
         $self=$prop->specialization($self, $allow_abstract)->type;
      }
      $prop=$self->lookup_property($_)
        or
      croak( "unknown property ", $self->full_name, "::$_" );
   } split /\./, $req;
}

# 'NAME.NAME | ...' => [ [ Property, ... ] ]
# flags(RETVAL)==$Property::is_permutation if the paths descend into a permutation subobject
sub encode_read_request {
   my ($self, $req, $obj)=@_;
   my $perm_seen;
   my @alternatives=map {
      my @path=encode_property_list($self, $_, $obj);
      $perm_seen ||= Property::find_first_in_path(\@path, $Property::is_permutation)>=0;
      \@path
   } split /\s*\|\s*/, $req;
   if ($perm_seen) {
      set_array_flags(\@alternatives, $Property::is_permutation);
   }
   \@alternatives
}
####################################################################################
# for compatibility with LocallyDerived types, e.g. when only derived owner types define some additional properties
*pure_type=\&type;
sub local_type { pop }
sub enclosed_in_restricted_spez { 0 }

# for compatibility with Specialization
sub preconditions { undef }
####################################################################################
sub override_rule {
   my ($self, $super, $label)=@_;
   my $matched;
   foreach my $rule ($label->list_all_rules) {
      if (is_object($super)) {
         next if !$super->isa($rule->defined_for);
      } else {
         next if $self==$rule->defined_for || list_index($self->super, $rule->defined_for)<0;
      }
      $matched=1;
      if (defined($rule->overridden_in)) {
         push @{$rule->overridden_in}, $self if list_index($rule->overridden_in, $self)<0;
      } else {
         $rule->overridden_in=[ $self ];
      }
   }
   if ($Application::plausibility_checks && !$matched) {
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
      grep { $_->header =~ $pattern } map { @{$_->production_rules} } $self, @{$self->super}

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
   my $rule=special Rule($header, $code, $self);
   $rule->flags=$Rule::is_function;
   substr($rule->header,0,0)="user_method ";
   $rule->analyze_spez_preconditions;
   if (defined($name)) {
      # non-polymorphic method, no call redirection via overload resolution
      my $rules=[ $rule ];
      define_function($self->pkg, $name,
                      sub : method {
                         &{ (Scheduler::resolve_rules($_[0], $rules) // croak("could not provide all required input properties") )->code };
                      });
   }
   $rule;
}
####################################################################################
sub help_topic {
   my $self=shift;
   $self->help // PropertyType::locate_own_help_topic($self, "objects", @_);
}
####################################################################################
sub override_help {
   my ($self, $group, $item, $text)=@_;
   my ($inherited_topic)=map { $_->help_topic(1)->find($group, $item) } @{$self->super}
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
   my ($self, $suffix)=@_;
   define_function($self->pkg, "default_file_suffix", sub : method { $suffix });
   push @{$self->application->file_suffixes}, $suffix;
}
####################################################################################
sub reopen_subobject {
   my ($self, $path)=@_;
   foreach my $prop_name (split /\./, $path) {
      my $prop=$self->property($prop_name);
      $prop->flags & $Property::is_subobject
         or croak( "can't reopen the definition scope of an atomic property" );
      $self=$prop->create_local_derivation_mixin($self);
   }
}
####################################################################################
package _::MethodAsRule;

sub add_control {
   my ($pkg, $list, $rule)=@_;
   if (!@$list) {
      bless $list, $pkg;
   }
   push @$list, $rule;
}

sub dup {
   my $list=shift;
   inherit_class([ @$list ], $list);
}

sub resolve {
   my ($self, $args)=@_;
   (Scheduler::resolve_rules($args->[0], $self) // croak( "could not provide all required input properties" ))->code;
}

####################################################################################
package __::LocalDerivationMixin;

use Polymake::Struct (
   [ '@ISA' => 'ObjectType' ],
   [ new => '$$' ],
   [ aliases => (belongs_to => 'params') ],
   [ '$name' => '#2 ->name ."__". #1 ->name ."__mix_in"' ],
   [ '$belongs_to' => '#2' ],
   [ '$application' => '#2 ->application' ],
   [ '$parent_property' => 'weak( #1 )' ],
   [ '$enclosed_in_restricted_spez' => '#2 ->enclosed_in_restricted_spez || defined( #2 ->preconditions)' ],
   '%local_type_cache',                       # (autonomous) ObjectType => LocallyDerived
);

# constructor: (parent) Property, (owner) ObjectType =>
sub new {
   my $self=&_new;
   my $forefather=$self->parent_property->belongs_to;
   $self->pkg=$self->belongs_to->pkg."::_prop_".$self->parent_property->name;

   define_function($self->pkg, "type", sub { $self }, 1);
   define_function($self->pkg, "self",  # for the rule parser
                   sub { &check_object_pkg; $self });

   # establish inheritance relations with mixins generated for ascendants and descendants of $parent

   unless ($self->parent_property->flags & $Property::is_permutation) {
      my ($closest_super_mixin, $closest_derived_mixin);

      if ($self->belongs_to != $forefather) {
         foreach my $super_proto (@{$self->belongs_to->super}) {
            if ($super_proto==$forefather  or
                list_index($super_proto->super, $forefather)>=0) {
               if (defined ($closest_super_mixin=$self->parent_property->mixin->{$super_proto})) {
                  establish_inheritance($self, $closest_super_mixin->pkg);
                  last;
               }
            }
         }
      }
      if (defined $closest_super_mixin) {
         # prepend that one with the new mixin in all descendants
         foreach my $derived_proto ($closest_super_mixin->derived) {
            if ($derived_proto != $self &&
                $derived_proto->super->[0]==$closest_super_mixin &&
                instanceof LocalDerivationMixin($derived_proto)) {
               # this is the closest descendant, it must directly inherit from the new mixin now
               $closest_derived_mixin=$derived_proto;
               unshift @{$closest_derived_mixin->super}, $self;
               no strict 'refs';
               @{$closest_derived_mixin->pkg."::ISA"}=$self->pkg;
               last;
            }
         }
      } else {
         # maybe there are more derived mixins downstream

         foreach my $derived_proto ($self->belongs_to->derived) {
            if (defined (my $derived_mixin=$self->parent_property->mixin->{$derived_proto})) {
               if (@{$derived_mixin->super}==0) {
                  $closest_derived_mixin=$derived_mixin;
                  establish_inheritance($closest_derived_mixin, $self->pkg);
                  last;
               }
            }
         }
      }

      if (defined $closest_derived_mixin) {
         foreach my $derived_proto ($closest_derived_mixin->derived) {
            my $pos=list_index($derived_proto->super, $closest_derived_mixin);
            splice @{$derived_proto->super}, $pos+1, 0, $self;
            if (instanceof LocallyDerived($derived_proto)) {
               ++$derived_proto->pure_type_at;
            }
         }
      }
   }

   $self;
}

sub local_type {
   my ($self, $autonomous_type)=@_;
   my $autonomous_pure_type=$autonomous_type->pure_type;
   if ($autonomous_type != $autonomous_pure_type &&
       $autonomous_type->super->[0]->parent_property->key == $self->parent_property->key) {
      $autonomous_type=$autonomous_pure_type;
   }
   $self->local_type_cache->{$autonomous_type} //= new LocallyDerived($autonomous_type, $self);
}

sub lookup_property {
   my ($self, $prop_name)=@_;
   # don't cache properties of the pure type in the mixin
   $self->SUPER::lookup_property($prop_name) // $self->parent_property->type->pure_type->lookup_property($prop_name);
}

sub property {
   my ($self, $prop_name)=@_;
   # don't cache properties of the pure type in the mixin
   $self->SUPER::lookup_property($prop_name) // $self->parent_property->type->pure_type->property($prop_name);
}

sub full_name {
   my $self=shift;
   $self->belongs_to->full_name . "::" . $self->parent_property->name;
}

####################################################################################
# private:
sub provide_help_topic {
   my ($owner, $force, @descend)=@_;
   my $topic=$owner->help_topic($force) or return;
   foreach my $mixin (@descend) {
      my $prop=$mixin->parent_property;
      my $help_group=$prop->flags & $Property::is_permutation ? "permutations" : "properties";
      $topic=$topic->find("!rel", $help_group, $prop->name) // do {
         $force or return;
         my $text="";
         if ($owner != $prop->belongs_to) {
            if (my $prop_topic=$prop->belongs_to->help_topic->find($help_group, $prop->name)) {
               if ($prop_topic->parent->category) {
                  $text='# @category ' . $prop_topic->parent->name . "\n";
               }
            }
            my $refer_to=($mixin->super->[0] || $prop)->belongs_to;
            $text .= "Amendment of [[" . ( $refer_to->application != $mixin->application && $refer_to->application->name . "::" )
                     . $refer_to->full_name . "::" . $prop->name
                     . "]] for " . $mixin->full_name . "\n"
                     . "# \@display noshow\n";
         }
         $topic=$topic->add([ $help_group, $prop->name ], $text);
         $topic->annex->{property}=$prop;
         $topic
      };
      $owner=$mixin;
   }
   $topic
}

sub help_topic {
   my ($self, $force)=@_;
   $self->help //= do {
      my @descend;
      my $owner=$self;
      do {
         push @descend, $owner;
         $owner=$owner->belongs_to;
      } while (instanceof LocalDerivationMixin($owner));

      if (instanceof Specialization($owner)) {
         (provide_help_topic($owner->generic // $owner->full_spez_for, $force, reverse @descend) // return)->new_specialization($owner);
      } else {
         my $topic=provide_help_topic($owner, $force, reverse @descend) or return;
         push @{$topic->related}, uniq( map { ($_, @{$_->related}) } grep { defined } map { $_->help_topic($force) } @{$self->super} );
         $topic
      }
   };
}
####################################################################################
package __::LocallyDerived;

use Polymake::Struct (
   [ '@ISA' => 'ObjectType' ],
   [ new => '$$' ],
   [ '$name' => '#1 ->name' ],
   [ '$application' => '#1 ->application' ],
   [ '$extension' => '#1 ->extension' ],
   '$pure_type_at',
);

sub new {
   my $self=&_new;
   my ($autonomous_type, $mix_in)=@_;
   $self->pkg=$autonomous_type->pkg."::__as__".$mix_in->pkg;
   if ($autonomous_type->abstract) {
      $self->context_pkg=$autonomous_type->context_pkg;
      $self->abstract=sub : method { (&pure_type)->abstract->($_[1]) };
      @{$self->super}=($mix_in, @{$mix_in->super}, $autonomous_type, @{$autonomous_type->super});
      $self->pure_type_at=@{$mix_in->super}+1;
   } else {
      define_function($self->pkg, "type", sub { $self }, 1);
      establish_inheritance($self, $mix_in->pkg, $autonomous_type->pkg);
      $self->pure_type_at=list_index($self->super, $autonomous_type);
   }
   PropertyParamedType::set_extension($self->extension, $mix_in->extension);
   $self;
}

sub pure_type {
   my ($self)=@_;
   $self->super->[$self->pure_type_at]
}

sub full_name {
   my ($self)=@_;
   $self->pure_type->full_name . " as " . $self->super->[0]->full_name;
}

# independent ObjectType => LocallyDerived
sub local_type {
   my ($self, $proto)=@_;
   if ($proto==&pure_type) {
      $self
   } else {
      $self->super->[0]->local_type($proto);
   }
}

sub find_rule_label {
   &ObjectType::find_rule_label // do {
      my $mix_in_app=$_[0]->super->[0]->application;
      if ($mix_in_app != $_[0]->application) {
         $mix_in_app->prefs->find_label($_[1])
      } else {
         undef
      }
   }
}

sub update_inheritance {
   my ($self)=@_;
   my $pure_type=&pure_type;
   &ObjectType::update_inheritance;
   $self->pure_type_at=list_index($self->super, $pure_type);
}

####################################################################################
package __::Permuted;

use Polymake::Struct (
   [ '@ISA' => 'LocallyDerived' ],
);

sub get_producers_of {
   my ($self, $prop)=@_;
   # don't inherit any production rule from the basis type, otherwise the scheduler would drive crazy
   $self->super->[0]->producers->{$prop->key} || [ ];
}

####################################################################################
package __::Specialization;

use Polymake::Struct (
   [ '@ISA' => 'ObjectType' ],
   [ new => '$$$;$' ],
   [ '$application' => '#3 ->application' ],
   [ '$match_node' => 'undef' ],
   [ '$preconditions' => 'undef' ],
   [ '$full_spez_for' => 'undef' ],
);

# Constructor: new Specialization('name', 'pkg', generic_ObjectType | generic_Specialization | concrete_ObjectType, [ type params ] )
sub new {
   my $self=&_new;
   (undef, my ($pkg, $gen_proto, $tparams))=@_;
   my $concrete_proto;

   if (defined $pkg) {
      $self->pkg=$pkg;
      if ($tparams) {
         # new abstract specialization
         $self->abstract=\&type;
         undef $self->perform_typecheck;
         my $param_index=-1;
         $self->params=[ map { new ClassTypeParam($_, $pkg, $self->application, ++$param_index) } @$tparams ];
         $self->name //= do {
            (undef, my ($file, $line))=caller;
            $def_line-=2;       # accounting for code inserted in RuleFilter
            $gen_proto->name." defined at $file, line $line"
         };
         $self->match_node=new Overload::Node(undef, undef, 0);
         push @{$gen_proto->specializations //= [ ]}, $self;
      } else {
         # new full specialization
         $concrete_proto=$gen_proto;
         $gen_proto=$concrete_proto->generic;
         if (defined $self->name) {
            (($gen_proto // $concrete_proto)->full_spez //= { })->{$self->name}=$self;
         } else {
            $self->name=$concrete_proto->full_name;
         }
         $self->params=$concrete_proto->params;
      }
   } else {
      # a concrete instance of a specialization
      $self->name=$gen_proto->name;
      $self->extension=$gen_proto->extension;
      $self->pkg=$gen_proto->pkg;
      $self->params=$tparams;
      PropertyParamedType::scan_params($self);
   }
   my $self_sub=define_function($self->pkg, "type", sub { $self }, 1);
   $self->generic=$gen_proto;
   if (defined $concrete_proto) {
      define_function($self->pkg, "self", sub { &check_object_pkg; $self });  # for rulefile parser
      no strict 'refs';
      my $concrete_ISA=\@{$concrete_proto->pkg."::ISA"};
      establish_inheritance($self, @$concrete_ISA);
      unshift @$concrete_ISA, $self->pkg;
      $_->update_inheritance for $concrete_proto, $concrete_proto->derived;
      weak($self->full_spez_for=$concrete_proto);
   } else {
      establish_inheritance($self, $gen_proto->pkg);
   }
   $self;
}

sub apply_to_existing_types {
   my ($self)=@_;
   my $gen_proto=$self->generic;
   my @derived_types=grep { !$_->abstract } $gen_proto->derived;
   # first update the ISA lists of matching instances of the same generic type
   foreach my $proto (@derived_types) {
      if ($proto->generic == $gen_proto and
          my ($spez)=pick_specialization($self, $proto)) {
         namespaces::using($proto->pkg, $spez->pkg);
         no strict 'refs';
         unshift @{$proto->pkg."::ISA"}, $spez->pkg;
      }
   }
   # then regenerate the super lists of all derived types
   $_->update_inheritance for @derived_types;
}

sub lookup_property {
   my ($self, $prop_name)=@_;
   if (defined $self->full_spez_for) {
      # the final object type contain this among its super types, it will find any property defined here
      $self->full_spez_for->lookup_property($prop_name)
   } else {
      &ObjectType::lookup_property
   }
}
####################################################################################

sub append_precondition {
   my ($self, $header, $code, $checks_definedness)=@_;
   my $precond=special Rule($header, $code, $self);
   $precond->flags=($checks_definedness ? $Rule::is_precondition | $Rule::is_definedness_check : $Rule::is_precondition) | $Rule::is_spez_precondition;
   $precond->header="precondition " . $precond->header . " ( specialization " . $self->name . " )";
   push @{$self->preconditions //= [ ]}, $precond;
}

sub add_permutation {
   croak("A type specialization can't have own permutation types; consider introducing a derived `big' object type for this");
}
####################################################################################
sub full_name {
   my ($self)=@_;
   if (defined $self->full_spez_for) {
      # full specialization
      $self->name
   } elsif ($self->name =~ / defined at /) {
      # unnamed partial specialization
      "$`<" . join(",", map { $_->full_name } @{$self->params}) . ">$&$'"
   } else {
      # named partial specialization
      &PropertyParamedType::full_name
   }
}

sub help_topic {
   my ($self, $force)=@_;
   $self->help // do {
      if ($force) {
         if ($Application::plausibility_checks && $self->name =~ / defined at /) {
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
   [ '&toXML' => '\&toXML_meth' ],
   [ '&equal' => '\&equal_sub' ],
   [ '&isa' => '\&isa_meth' ],
);

sub toXML_meth : method {
   my ($self, $arr, $writer, @attr)=@_;
   if (@$arr) {
      $writer->startTag("m", @attr);
      foreach my $obj (@$arr) {
         XMLwriter::write_subobject($writer, $obj, $obj->parent, $self->params->[0]);
      }
      $writer->endTag("m");
   } else {
      $writer->emptyTag("m", @attr);
   }
}

Struct::pass_original_object(\&toXML_meth);

sub equal_sub {
   my ($arr1, $arr2)=@_;
   my $e=$#$arr1;
   return 0 if $e != $#$arr2;
   for (my $i=0; $i<=$e; ++$i) {
      return 0 if $arr1->[$i]->diff($arr2->[$i]);
   }
   1
}

sub isa_meth : method {
   my ($self, $arr)=@_;
   UNIVERSAL::isa($arr, __PACKAGE__) && $arr->type->params->[0]->isa($self->params->[0]);
}

Struct::pass_original_object(\&isa_meth);

sub construct_from_list : method {
   my ($self, $list)=@_;
   # the contained Objects have already been checked during overload resolution
   bless $list, $self->pkg;
}

sub construct_with_size : method {
   my ($self, $n)=@_;
   bless [ map { Object::new_named($self->params->[0]) } 1..$n ], $self->pkg;
}

# For the exotic case of size passed in string form.
# Everything else will lead to an exception.
sub construct_with_size_str : method {
   construct_with_size($_[0], extract_integer($_[1]));
}

sub new_generic {
   my $self=&_new;
   $self->construct_node=new Overload::Node(undef, undef, 0);
   Overload::add_instance(__PACKAGE__, ".construct",
                          \&construct_with_size, undef,
                          [1, 1, Overload::integer_package()], undef, $self->construct_node);
   $self;
}

sub typeof { state $me=&new_generic; }

sub init_constructor : method {
   my ($super, $self)=@_;
   $self->construct_node=$super->construct_node;
   Overload::add_instance($self->pkg, ".construct",
                          \&construct_from_list, undef,
                          [1, 1+$Overload::has_repeated, [$self->params->[0]->pkg, "+"]], undef, $self->construct_node);

   $self->parse=\&construct_with_size_str;
   overload::OVERLOAD($self->pkg, '""' => ($self->toString = sub { "BigObjectArray" }));

   # put BigObjectArray in front of generic Array as to inherit the correct constructors
   # TODO: find a more elegant/generic way of achieving this, e.g. by specializing the generic types in the rules.
   no strict 'refs';
   @{$self->pkg."::ISA"}=(__PACKAGE__, $self->generic->pkg);
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
