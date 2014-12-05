#  Copyright (c) 1997-2014
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
   [ '$context_pkg' => 'undef' ],       # for abstract parameterized object types: class of other ObjectType in whose scope the abstract typeholders are defined
   [ '$params' => 'undef' ],            # for type templates
   '&abstract',
   [ '&perform_typecheck' => '\&PropertyType::concrete_typecheck' ],
   [ '&construct' => '\&PropertyType::construct_object' ],
   [ '&parse' => '\&Object::new_named' ],
   [ '&toString' => '\&Object::print_me' ],
#
#  Derivation relations
#
   '@super',                    # ( ObjectType )  super classes: transitive closure in MRO C3 order
   '%auto_casts',               # derived ObjectType => Rule::AutoCast
#
#  Own components
#
   '%properties',               # 'name' => Property
   '%permutations',             # 'name' => permutation Property
   '%producers',                # own rules producing a property: Property/Key => [ Rule, ... ]
   '%all_producers',            # cached own and inherited rules: Property/Key => [ Rule, ... ]
   '%shortcuts',                # cached own and inherited shortcut rules
   '%rules_by_label',           # own labeled producing rules: '*.label' => ControlList
   '%all_rules_by_label',       # cached own and inherited labeled rules
   '@production_rules',         # ( Rule ) all production rules directly applicable to this object type and descendants (for search by header)
#
   [ '$help' => 'undef' ],      # InteractiveHelp (in interactive mode, when comments are supplied for properties, user methods, etc.)
);

####################################################################################
#
#  Constructor
#
#  new ObjectType('name', Application, [ template params ], [ super ObjectType, ... ]);
#
sub new {
   my $self=&_new;
   Overload::learn_package_retrieval($self, \&pkg);
   my $tparams=splice @_, 0, 3;
   my $self_sub=sub { $self };

   unless ($construct_node) {
      $construct_node=new Overload::Node(undef, undef, 0);
      Overload::add_instance("Polymake::Core::Object", ".construct",
                             \&Object::new_empty, undef,
                             [0, 0], undef, $construct_node);
      Overload::add_fallback_to_node($construct_node, \&Object::new_filled);
   }

   if (defined($self->application)) {
      $self->pkg=$self->application->pkg."::".$self->name;
      my $symtab=get_pkg($self->pkg, 1);
      if ($tparams) {
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
      $self->application=$_[0]->application;
      $self->extension=$_[0]->extension;
      $self->pkg=$_[0]->pkg;
      $self->params=$tparams;
      PropertyParamedType::scan_params($self);
      if ($self->abstract) {
         if (defined($self->context_pkg)) {
            push @{$self->super}, $_[0], @{$_[0]->super};
         }
         return $self;
      }
   }

   define_function($self->pkg, "type", $self_sub, 1);
   if (!$self->abstract) {
      define_function($self->pkg, ".type", $self_sub);
   }

   # proceed with parent classes
   if (@_) {
      my @isa;
      foreach my $super_proto (@_) {
         push @isa, $super_proto->pkg;
         PropertyParamedType::set_extension($self->extension, $super_proto->extension);
      }
      establish_inheritance($self, @isa);
   } else {
      no strict 'refs';
      push @{$self->pkg."::ISA"}, "Polymake::Core::Object";
      mro::set_mro($self->pkg, "c3");
   }

   push @{$self->application->object_types}, $self;
   $self;
}

####################################################################################
sub establish_inheritance {
   my $self=shift;
   namespaces::using($self->pkg, @_);
   {  no strict 'refs';  push @{$self->pkg."::ISA"}, @_;  }
   mro::set_mro($self->pkg, "c3");
   my $linear_isa=mro::get_linear_isa($self->pkg);
   # ignore the type itself and ubiquitous Object/Object_4test
   @{$self->super}=map { $_->type } grep { !/^Polymake::Core::/ } @$linear_isa[1..$#$linear_isa];
}
####################################################################################
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
*performs_deduction=\&PropertyType::performs_deduction;
*type_param_index=\&PropertyType::type_param_index;

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
      return $self if $self->super->[0]->pkg eq $pkg;
      foreach my $super_proto (@{$self->super}) {
         return $super_proto if !$super_proto->abstract && @{$super_proto->super} && $super_proto->super->[0]->pkg eq $pkg;
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

sub is_one_of {
   my ($self, $others)=@_;
   foreach my $other (@$others) {
      return 1 if $self==$other || list_index($self->super, $other)>=0;
   }
   0
}
####################################################################################
# "property name" => Property or undef
sub lookup_property {
   my ($self, $prop_name, $allow_autocast)=@_;
   $self->properties->{$prop_name} //= do {
      my $prop;
      foreach (@{$self->super}) {
         last if defined ($prop=$_->properties->{$prop_name});
      }
      unless (defined($prop)) {
         if ($allow_autocast) {
            foreach (keys %{$self->auto_casts}) {
               if (defined ($prop=$_->properties->{$prop_name})) {
                  keys %{$self->auto_casts};       # reset the iterator!
                  last;
               }
            }
         }
         delete $self->properties->{$prop_name};
         return $prop;    # don't cache negative results and properties of derived types
      }
      $prop
   };
}

sub property {
   &lookup_property or croak( "unknown property ", $_[0]->full_name, "::$_[1]" );
}

declare $init_pseudo_prop=_new Property(undef, ".initial", undef);

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

sub lookup_permutation {
   my ($self, $name)=@_;
   $self->permutations->{$name} || do {
      my $perm;
      foreach my $super_proto (@{$self->super}) {
         $perm=$super_proto->permutations->{$name} and last;
      }
      $perm;
   };
}
####################################################################################
sub lookup_auto_cast {
   my ($self, $prop_name, $object, $allow_methods)=@_;
   my ($prop, $parent_prop);
   while (my ($target_proto, $rule)=each %{$self->auto_casts}) {
      if (defined ($prop=$target_proto->lookup_property($prop_name)) ||
          ($allow_methods && defined ($prop=UNIVERSAL::can($target_proto->pkg, $prop_name)))) {
         keys %{$self->auto_casts};             # reset the iterator!
         return ($rule, $object, $prop);
      }
   }

   foreach my $proto (@{$self->super}) {
      while (my ($target_proto, $rule)=each %{$proto->auto_casts}) {
         if (defined ($prop=$target_proto->lookup_property($prop_name)) ||
             ($allow_methods && defined ($prop=UNIVERSAL::can($target_proto->pkg, $prop_name)))
               and
             # the current object type and the target object type may be two fork ends with a common ancestor,
             # in which case the auto_cast is not allowed
             isa($target_proto, $self)) {
            $self->auto_casts->{$target_proto}=$rule;
            keys %{$proto->auto_casts};             # reset the iterator!
            return ($rule, $object, $prop);
         }
      }
   }

   # an auto-cast can also be triggered by asking for a property of a locally derived subobject
   if (defined($object) && defined($parent_prop=$object->property) && $parent_prop->flags & $Property::is_locally_derived) {
      while (my ($parent_proto, $mixin)=each %{$parent_prop->mixin}) {
         if (defined ($prop=$mixin->lookup_property($prop_name)) ||
             ($allow_methods && defined ($prop=UNIVERSAL::can($mixin->pkg, $prop_name)))) {
            keys %{$parent_prop->mixin};  # reset iterator
            $object=$object->parent;
            while (instanceof LocalDerivationMixin($parent_proto)) {
               $object=$object->parent;
               $parent_proto=$parent_proto->belongs_to;
            }
            $self=$object->type;
            foreach my $proto ($self, @{$self->super}) {
               while (my ($target_proto, $rule)=each %{$proto->auto_casts}) {
                  if (isa($target_proto, $self)) {
                     $self->auto_casts->{$target_proto}=$rule if $proto != $self;
                     keys %{$proto->auto_casts};             # reset the iterator!
                     return ($rule, $object, $prop);
                  }
               }
            }
         }
      }
   }

   ()
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
sub add_property_definition {
   my $self=$_[0];
   add_property($self, new Property(@_));
}

sub add_property_alias {        # "new name", "old name", override => Property
   my ($self, $prop_name, $old_prop_name, $override)=@_;
   my $old_prop=$self->lookup_property($old_prop_name)
      or croak( "unknown property ", $self->name, "::$old_prop_name" );
   my $new_prop=add_property($self, $old_prop->create_alias($prop_name, $override && $self));
   if ($override) {
      if ($Application::plausibility_checks and
          $old_prop->belongs_to == $self ||
          @{$self->super} && $self->super->[0]->abstract && $self->super->[0]->name eq $self->name) {
         croak( "only properties inherited from other object types can be overridden" );
      }
      $self->properties->{$old_prop_name}=$new_prop;
      define_function($self->pkg, $old_prop_name, UNIVERSAL::can($self->pkg, $prop_name));
   }
   $new_prop;
}
####################################################################################
sub add_permutation {
   my $self=$_[0];
   add_property($self, new Permutation(@_));
}

# create a temporary permutation type ostensibly working on an ancestor
# of the object to be really permuted
sub create_permutation_for_ancestor {
   my ($self, $perm, @descend)=@_;
   my $prop=new Permutation($self, 'PermuteAncestor');
   $prop->add_sub_permutation(@descend, $perm);
   $prop
}
####################################################################################
# private:
sub fill_prod_cache {
   my ($self, $prop, $list)=@_;
   if (defined (my $own_prod=$self->producers->{$prop->key})) {
      push @$list, @$own_prod;
   }
   if ($prop->belongs_to != $self) {
      foreach my $super_proto (@{$self->super}) {
         if (!defined($prop->belongs_to) || $super_proto->isa($prop->belongs_to)) {
            get_producers_of($super_proto, $prop);
            if (defined (my $own_prod=$super_proto->producers->{$prop->key})) {
               push @$list, grep { !defined($_->overridden_in) || !is_one_of($self, $_->overridden_in) } @$own_prod;
            }
         }
      }
   }
   $list
}

# protected:
sub invalidate_prod_cache {
   my ($self, $key)=@_;
   if (defined (delete $self->all_producers->{$key})) {
      delete $self->shortcuts->{$key};
      invalidate_prod_cache($_, $key) for $self->derived;
   }
}

sub get_producers_of {
   my ($self, $prop)=@_;
   $self->all_producers->{$prop->key} ||= fill_prod_cache(@_,[ ]);
}

# only shortcuts directly creating the property asked for are stored here
sub get_shortcuts_for {
   my ($self, $prop)=@_;
   $self->shortcuts->{$prop->key} ||=
      [ grep { instanceof Rule::Shortcut($_) and
               do { my $target=$_->output->[0];
                    is_object($target) || $target->[-1]->key == $prop->property_key
               }
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
#  mode = set of flags, see Object::try_auto_cast
sub encode_request_element {
   my ($self, $req, $obj, $mode)=@_;
   my $prop;
   map {
      croak( "invalid property name '$_'\n" ) if $Application::plausibility_checks && /\W/;
      if (defined($prop)) {
         $self=$prop->specialization($self, !defined($_[2]) && $self->abstract)->type;
         undef $obj;
      }
      $prop=$self->lookup_property($_) ||
            defined($obj) && $obj->try_auto_cast($_,$mode)
        or
      $self==$_[0]
      ? croak( "unknown property ", $_[0]->full_name, "::$_" )
      : croak( "unknown property $_ in path ", $_[0]->full_name, "::$req" );
   } split /\./, $req;
}

# 'NAME | NAME ...' => [ PropertyPath, ... ]
# flags(RETVAL)==$Property::is_{multiple,permutation} if the path descends into a subobject property declared as such
sub encode_read_request {
   my ($self, $req, $obj, $mode)=@_;
   my $mult_path;
   my $flag=0;
   my $result=[
      map {
         my @path=encode_request_element($self,$_,$obj,$mode);
         if (@path>1) {
            if ($flag) {
               foreach (0..$#$mult_path) {
                  $path[$_]==$mult_path->[$_]
                     or croak( "mixing ", $flag==$Property::is_multiple ? "multiple" : "permutation", " subobject ",
                               join(".", map { $_->name } @$mult_path),
                               " with other properties in one request is not allowed" );
               }
            } else {
               foreach (0..@path-2) {
                  if (my $flag_here=$path[$_]->flags & ($Property::is_multiple | $Property::is_permutation)) {
                     $flag=$flag_here;
                     $mult_path=[ @path[0..$_] ];
                     last;
                  }
               }
            }
            \@path
         } else {
            @path
         }
      } split /\s*\|\s*/, $req
   ];
   set_array_flags($result, $flag) if $flag;
   $result;
}
####################################################################################
# for compatibility with LocallyDerived types, e.g. when only derived owner types define some additional properties
*pure_type=\&type;
sub local_type { pop }
####################################################################################
sub override_rule {
   my ($self, $super, $label)=@_;
   my $matched;
   foreach my $rule ($label->list_all_rules) {
      if (is_object($super)) {
         next if $super!=$rule->defined_for;
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
sub add_auto_cast {
   my ($self, $proto)=@_;
   if ($Application::plausibility_checks) {
      isa($proto, $self) or croak( "Invalid cast declaration: ", $proto->full_name, " is not derived from ", $self->full_name );
      exists $self->auto_casts->{$proto} and croak( "duplicate auto_cast declaration" );
   }
   $self->auto_casts->{$proto}=my $rule=new Rule::AutoCast("auto_cast ".$self->full_name." => ".$proto->full_name, $proto, $self);
   push @{$self->application->rules}, $rule;
   Overload::add_instance($proto->pkg, ".construct", sub : method { Object::new_with_auto_cast($rule, @_) }, undef,
                          [1, 1, $self->pkg], undef, $construct_node);
   Overload::add_instance($proto->pkg, ".construct", sub : method { Object::new_filled_with_auto_cast($rule, @_) }, undef,
                          [3, 3+$Overload::has_trailing_list, $self->pkg, '$', '$'], undef, $construct_node);
}
####################################################################################
sub add_method_rule {
   my ($self, $header, $code, $name)=@_;
   my $rule=special Rule($header, $code, $self);
   $rule->flags=$Rule::is_function;
   push @{$self->application->rules}, $rule;
   substr($rule->header,0,0)="user_method ";
   if (defined($name)) {
      # non-polymorphic method, no call redirection via overload resolution
      define_function($self->pkg, $name,
                      sub : method {
                         &{ Scheduler::resolve_method([ $rule ], \@_)
                            or croak("failed providing input properties") };
                      });
   } else {
      # an instance of a polymorphic method
      $rule;
   }
}
####################################################################################
sub help_topic {
   my $self=shift;
   $self->help || PropertyType::locate_own_help_topic($self, "objects", @_);
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
      if ($Application::plausibility_checks && !instanceof ObjectType($prop->type)) {
         croak( "can't reopen the definition scope of an atomic property" );
      }
      $self=$prop->create_local_derivation_mixin($self);
   }
}
####################################################################################
package Polymake::Core::ObjectType::MethodAsRule;

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
   &Scheduler::resolve_method
     or croak( "could not provide all required input properties" );
}

####################################################################################
package Polymake::Core::ObjectType::LocalDerivationMixin;

use Polymake::Struct (
   [ '@ISA' => 'ObjectType' ],
   [ aliases => (belongs_to => 'params') ],
   [ '$name' => '#2 ->name ."__". #1 ->name ."__mix_in"' ],
   [ '$belongs_to' => '#2' ],
   [ '$application' => '#2 ->application' ],
   [ '$parent_property' => 'weak( #1 )' ],
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
   $self->local_type_cache->{$autonomous_type} ||= new LocallyDerived($autonomous_type, $self);
}

sub lookup_property {
   my ($self, $prop_name)=@_;
   # don't cache properties of the pure type in the mixin
   $self->SUPER::lookup_property($prop_name) || $self->parent_property->type->pure_type->lookup_property($prop_name);
}

sub property {
   my ($self, $prop_name)=@_;
   # don't cache properties of the pure type in the mixin
   $self->SUPER::lookup_property($prop_name) || $self->parent_property->type->pure_type->property($prop_name);
}

sub full_name {
   my $self=shift;
   $self->belongs_to->full_name . "::" . $self->parent_property->name;
}

####################################################################################
sub help_topic {
   my ($self, $force)=@_;
   $self->help ||= do {
      if (my $father_topic=$self->belongs_to->help_topic($force)) {
         my $topic=$father_topic->find("!rel", "properties", $self->parent_property->name);
         if ($force) {
            my ($prop_topic, $refer_to);
            $topic ||= $father_topic->add([ "properties", $self->parent_property->name ],
                                          $self->belongs_to != $self->parent_property->belongs_to
                                            &&
                                          ( $refer_to=($self->super->[0] || $self->parent_property)->belongs_to,
                                            $prop_topic=$self->parent_property->belongs_to->help_topic->find("properties", $self->parent_property->name)
                                              and
                                            $prop_topic->parent->category
                                              and
                                            '# @category ' . $prop_topic->parent->name . "\n"
                                          )
                                          . "# Specialization of [["
                                          . ( $refer_to->application != $self->belongs_to->application && $refer_to->application->name . "::" )
                                          . $refer_to->full_name . "::" . $self->parent_property->name
                                          . "]] for " . $self->belongs_to->full_name . "\n");

            push @{$topic->related}, uniq( map { ($_, @{$_->related}) } map { $_->help_topic } @{$self->super}, $self->parent_property->type );
         }
         $topic
      }
   };
}
####################################################################################
package Polymake::Core::ObjectType::LocallyDerived;

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

####################################################################################
package Polymake::Core::ObjectType::Permuted;

use Polymake::Struct (
   [ '@ISA' => 'LocallyDerived' ],
);

sub get_producers_of {
   my ($self, $prop)=@_;
   # don't inherit any production rule from the basis type, otherwise the scheduler would drive crazy
   $self->super->[0]->producers->{$prop->key} || [ ];
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
