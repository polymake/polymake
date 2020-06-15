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

package Polymake::Core::Permutation;

use Polymake::Struct (
   [ '@ISA' => 'Property' ],
   [ '$flags' => 'Flags::is_subobject | Flags::is_permutation' ],
   '%sensitive_props',                  # Property->key => Recovery  or  { $sub_key }->{ Property }-> ... -> Recovery
                                        # if there are rules transferring Property from the permuted subobject back into the basis
   '%sub_permutations',                 # Property->key (subobject) => Property (permutation)
   '%parent_permutations',              # Property->key (parent object) => ... => $sub_key => [ Property (permutation) ]
);

sub new {
   my $self=&_new;
   $self->type=$self->type->create_derived($self, $self->belongs_to);
   unless ($self->type->abstract) {
      $self->accept=\&accept_subobject;
      $self->flags |= Flags::is_concrete;
   }
   $self;
}

sub change_to_augmented {
   my ($self, $augm) = @_;
   if ($augm != $self->type->super->[0]) {
      $self->type->modify_super(0, 1, $augm);
   }
}

sub update_pure_type {
   my ($self, $proto) = @_;
   $self->type = $proto->create_derived($self, $self->belongs_to);
}

####################################################################################

declare $sub_key = \(1);         # singular key values used in sensitive_props
declare $is_non_recoverable = \(2);

# Rule => true if it is a recovering rule for a permuted property
sub analyze_rule {
   my ($self, $rule) = @_;
   my ($regular_out_seen, $perm_out_seen);
   foreach my $output (@{$rule->output}) {
      if (@$output == 1) {
         my $prop = $output->[0];
         if ($rule->code == \&Rule::nonexistent) {
            if ($enable_plausibility_checks && is_object($self->sensitive_props->{$prop->key})) {
               croak( "recovery of property ", $prop->name, " was enabled prior to this rule definition" );
            }
            $self->sensitive_props->{$prop->key} = $is_non_recoverable;
         } else {
            if ($enable_plausibility_checks && $self->sensitive_props->{$prop->key} == $is_non_recoverable) {
               croak( "recovery of property ", $prop->name, " was disabled prior to this rule definition" );
            }
            my $recovery = ($self->sensitive_props->{$prop->key} //= new Recovery($self, $prop));
            $rule->defined_for->add_producers_of($recovery, $rule);
         }
         $regular_out_seen = true;
      } else {
         if ($enable_plausibility_checks && get_array_flags($output) & Flags::is_multiple_new) {
            croak( "Rule dealing with permutations may not create new multiple subobjects" );
         }
         my $perm_pos = find_first_in_path($output, Flags::is_permutation);
         if ($perm_pos >= 0) {
            if ($perm_pos == 0) {
               if ($enable_plausibility_checks && $output->[0] != $self) {
                  croak( "dependence between two different permutation subobjects on the same level is not allowed" );
               }
            } else {
               add_sub_permutation($self, @$output[0..$perm_pos]);
            }
            $perm_out_seen = true;
         } else {
            add_sensitive_sub_property($self, @$output, $rule);
            $regular_out_seen = true;
         }
      }
   }
   if ($perm_out_seen && $regular_out_seen) {
      croak( "A production rule can't create properties in the base object and permutation subobject at the same time" );
   }
   $regular_out_seen
}
####################################################################################
sub descend_and_create {
   my $hash = shift;
   my $prop = pop;
   foreach $prop (@_) {
      $hash = ($hash->{$sub_key}->{$prop->key} //= { });
   }
   ($hash, $prop)
}
####################################################################################
# private:
sub add_sensitive_sub_property {
   my $self = shift;
   my $rule = pop;
   my ($hash, $prop) = descend_and_create($self->sensitive_props, @_);
   if ($rule->code == \&Rule::nonexistent) {
      if ($enable_plausibility_checks && is_object($hash->{$prop->key})) {
         croak( "recovery of property ", $prop->name, " was enabled prior to this rule definition" );
      }
      $hash->{$prop->key} = $is_non_recoverable;
   } else {
      if ($enable_plausibility_checks && $hash->{$prop->key} == $is_non_recoverable) {
         croak( "recovery of property ", $prop->name, " was disabled prior to this rule definition" );
      }
      my $recovery = ($hash->{$prop->key} //= new Recovery($self, $prop, @_));
      $rule->defined_for->add_producers_of($recovery, $rule);
   }
}
####################################################################################
# parent Permutation, (path to subobject permutation Property) =>
sub add_sub_permutation {
   my $self = shift;
   my $sub_permutation = pop;
   my ($hash, $prop) = descend_and_create($self->sub_permutations, @_);
   if (exists $hash->{$prop->key}) {
      $hash->{$prop->key} == $sub_permutation or
      croak( "ambiguous permutation propagation into ", join(".", map { $_->name } @_, $prop), " : ",
             $sub_permutation->name, " vs. ", $hash->{$prop->key}->name );
   }

   $hash->{$prop->key} = $sub_permutation;
   # provide for transitive closure
   $hash->{$sub_key}->{$prop->key} = $sub_permutation->sub_permutations;

   # establish reversed relation
   $hash = $sub_permutation->parent_permutations;
   foreach my $parent_prop ($prop, reverse @_) {
      $hash = ($hash->{$parent_prop->key} //= { });
   }
   push @{$hash->{$sub_key}}, $self;
}
####################################################################################
# ( Property, ... ) => Recovery
sub find_sensitive_sub_property {
   my $self = shift;
   my $prop = pop;
   my $hash = $self->sensitive_props;
   foreach $prop (@_) {
      defined($hash = $hash->{$sub_key}) or return;
      defined($hash = $hash->{$prop->key}) or return;
   }
   $hash->{$prop->key}
}
####################################################################################
sub find_sub_permutation {
   my $self = shift;
   my $prop = pop;
   my $hash = $self->sub_permutations;
   foreach $prop (@_) {
      defined($hash = $hash->{$sub_key}) or return;
      defined($hash = $hash->{$prop->key}) or return;
   }
   $hash->{$prop->key}
}

####################################################################################
package Polymake::Core::Permutation::Recovery;

use Polymake::Struct (
   [ new => '$$@' ],
   [ '$permutation' => 'weak(#1)' ],
   [ '$property' => '#2' ],
   [ '@descend' => '@' ],
);

sub key { $_[0] }

sub defined_for { $_[0]->property->defined_for }

sub name {
   my ($self) = @_;
   "recovery of " . join(".", @{$self->descend}, $self->property->name) .
   " after " . $self->permutation->belongs_to->full_name . "::" . $self->permutation->name
}

1

# Local Variables:
# cperl-indent-level: 3
# indent-tabs-mode:nil
# End:
