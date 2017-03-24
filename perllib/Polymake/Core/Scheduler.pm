#  Copyright (c) 1997-2017
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
package Polymake::Core::Scheduler;

# RuleGraph arc states; the values are filled in the extension module RuleGraph.xxz
declare $rgr_inactive_arc,  # the arc is incident to an eliminated node or not activated yet
        $rgr_weak_arc,      # keeps the tail node reachable from the final target, does not constitute a prerequsite for the head node
        $rgr_initial_arc,   # connects an initial rule and the final target
        $rgr_exclusive_arc, # only one head node in a star of exclusive arcs may be resolved
        $rgr_unique_arc,    # a mandatory and unique prerequisite
        $rgr_resolved_arc,  # a resolved input, that is, the tail node is a scheduled rule or one of target properties of a scheduled rule
        $rgr_source_arc;    # a value starting with this encodes a group of alternative inputs/prerequisites

####################################################################################
#
#  A rule chain under construction
#
package Polymake::Core::Scheduler::TentativeRuleChain;

use Polymake::Struct (
   [ '$heap_agent' => 'undef' ], # ChainAgent in Scheduler::Heap, maintained by the heap
   [ '$rgr' => 'undef' ],        # RuleGraph shared among all chains
   [ '$rgr_state' => 'undef' ],  # local state of RuleGraph nodes and arcs

   '%run',                      # (precondition or weight) RuleDeputy => success code

   '@rules',                    # (Rule)  rules scheduled for execution, in proper order
   '@ready',                    # (RuleDeputy)  rules whose inputs are all resolved, that is, created by other rules scheduled earlier.

   [ '$debug' => 'undef' ],     # Debugging info
);

sub clone {
   my ($self)=@_;
   Struct::make_body(
      @$self[0..3],    # heap_agent .. run
      [ @{$self->rules} ],
      [ @{$self->ready} ],
      defined($self->debug) ? $self->debug->clone : undef,
      __PACKAGE__);
}

####################################################################################
package Polymake::Core::Scheduler::RuleDeputy;

# Constructor: (Rule, DeputyObject, perm_trigger)
use Polymake::Struct (
   [ '@ISA' => 'Rule::Deputy' ],
   [ new => '$$;$' ],
   # navigation from the root object to the one the rule refers to
   [ '$up' => '#2->up' ],                             # how many parent levels above the root object
   [ '@down' => '#2->down' ],                         # [ Property | _::SelectMultiInstance ] path descending to the subobject
   [ '@multi_selector' => '#2->multi_selector' ],     # [ Property | _::SelectMultiInstance ] path descending through multiple subobjects
   [ '$created_multi_instances' => 'undef' ],         # [ DeputyObject ] for multiple subobjects created by this rule

   # RuleDeputy objects for associated rules
   [ '$dyn_weight' => 'undef', ],
   '@preconditions',
   [ '$with_permutation' => 'undef' ],     # production rule: RuleDeputy for CreatingPermutation
   [ '$_without_permutation | rules_to_block' => 'undef' ],   # CreatingPermutation: RuleDeputy for production rule
                                                              # PermAction: list of sensitive rules stored in the DeputyObject of permuted subobject
   [ '$perm_trigger | actions' => 'weak(#3)' ],

   '@prop_vertex_sets',         # [ int ... ]  vertices of Scheduler::Heap encoding properties created by this rule
   [ '$rgr_node' => 'undef' ],  # node in RuleGraph corresponding to this rule

   '$temp_state',     # scratch variable for temporary states
);

use Polymake::Struct (
   [ 'alt.constructor' => 'new_precondition' ],
   [ new => '$$' ],
   [ '$multi_selector' => '#2->multi_selector && multi_selector_for_precondition(#2->multi_selector, #1)' ],
   [ '$perm_trigger' => 'weak(#2->perm_trigger)' ],
);

sub new {
   my $cache=pop;
   my $self=&_new;
   my $obj=$_[1];
   @{$self->preconditions}=map { new_precondition($self, $_, $self) } @{$self->rule->preconditions};
   if (defined $self->rule->dyn_weight) {
      ($self->dyn_weight)=grep { $_->rule == $self->rule->dyn_weight } @{$self->preconditions};
   }
   if ($self->flags & $Rule::is_restricted_spez) {
      my $spez_deputy=$obj;
      my $spez_type=$self->rule->defined_for;
      while ($spez_type->enclosed_in_restricted_spez) {
         $spez_deputy=$spez_deputy->navigate_up;
         $spez_type=$spez_type->parent_property->belongs_to;
      }
      push @{$self->preconditions}, map { $spez_deputy->rule_instance($_) } @{$spez_type->preconditions};
   }
   if (defined $self->rule->with_permutation) {
      $cache->{$self->rule->with_permutation} =
      $self->with_permutation = _new($self, $self->rule->with_permutation, $obj);
      $self->with_permutation->preconditions=$self->preconditions;
      $self->with_permutation->actions=[ map { $cache->{$_}=_new($self, $_, $obj, $self) } @{$self->rule->with_permutation->actions} ];
      weak($self->with_permutation->_without_permutation = $self);
   }
   $self;
}

# decide whether the given precondition should inherit the multiple subobject selector from its master rule
sub multi_selector_for_precondition {
   my ($selector, $precond)=@_;
   my $new_subobj_pos;
   my $i=0;
   foreach (@$selector) {
      if (is_object($_->index)) {
         $new_subobj_pos=$i;  last;
      }
      ++$i;
   }
   if (defined $new_subobj_pos) {
      foreach my $input (@{$precond->input}) {
         foreach my $input_path (@$input) {
            if ($#$input_path > $new_subobj_pos && Property::equal_path_prefixes($input_path, $selector)>=$new_subobj_pos) {
               return $selector;
            }
         }
      }
      # none of the input paths has matched the selector - the precondition does not need it
      undef $selector;
   }
   $selector;
}

sub copy4schedule {
   my ($self, $chain)=@_;
   my $copy=_new($self, $self->rule, $self);
   if (defined $self->with_permutation) {
      my $subobj=$chain->deputy_root->navigate_to_rule_owner($self);
      if (defined $subobj->real_object) {
         return (_new($self, $self->rule->sensitivity_check, $self), $copy);
      }
   }
   $copy
}

sub perm_path { $_[0]->rule->perm_path }

sub without_permutation {
   my ($self)=@_;
   $self->_without_permutation // $self
}

sub path_prefix {
   my ($self)=@_;
   defined($self->down)
   ? join(".", ("parent") x $self->up, map { $_->name } @{$self->down})
   : join(".", ("parent") x $self->up)
}

sub header {
   my ($self)=@_;
   my $prefix=&path_prefix;
   $self->rule->header . ($prefix && " ( applied to $prefix )") .
   ( $self->multi_selector ? " ( selecting " . join(".", map { $_->name } @{$self->multi_selector}) . " )" : "" ) .
   ( is_object($self->perm_trigger) && !($self->flags & $Rule::is_perm_action) && " after " . $self->perm_trigger->header )
}

sub level {
   my ($self)=@_;
   (defined($self->down) && @{$self->down}) - (defined($self->up) && $self->up);
}

sub list_results {
   my ($self, $proto)=@_;
   my $prefix=&path_prefix;
   $prefix ? map { "$prefix.$_" } $self->rule->list_results($proto) : $self->rule->list_results($proto)
}

# real object => rule result
# $force>0: always execute the rule even if all targets already exist
# $force<0: $object is already the right one, no navigation needed
sub execute {
   my ($self, $object, $force)=@_;
   my $scope;
   if ($force >= 0) {
      for (my $i=$self->up; $i>0; --$i) {
         defined (my $parent=$object->parent)
           or return $Rule::exec_infeasible;
         if ($object->property->flags & $Property::is_multiple) {
            $object->select_multi_instance($scope);
         }
         $object=$parent;
      }
      if (defined $self->down) {
         foreach my $prop (@{$self->down}) {
            if (defined (my $i=$object->dictionary->{$prop->key})) {
               $object=$object->contents->[$i];
               if ($prop->flags & $Property::is_multiple) {
                  $object=$object->select_now($prop->index)
                    or return $Rule::exec_infeasible;
               } elsif ($prop->flags & $Property::is_twin) {
                  $object=$object->value;
               }
            } else {
               return $Rule::exec_infeasible;
            }
         }
      }
   }
   if (defined $self->multi_selector) {
      my $subobject=$object;
      foreach my $prop (@{$self->multi_selector}) {
         if (defined (my $i=$subobject->dictionary->{$prop->key})) {
            $subobject=$subobject->contents->[$i];
            if ($prop->flags & $Property::is_multiple) {
               $subobject=$subobject->select_now($prop->index, $scope)
                 or return $Rule::exec_infeasible;
            }
         } else {
            return $Rule::exec_infeasible;
         }
      }
   }
   $self->rule->execute($object, $force);
}

####################################################################################
package Polymake::Core::Scheduler::DeputyObject;

# Constructor: (real Object, up, Parent, Property, perm_trigger)
use Polymake::Struct (
   [ new => '$;$$$$' ],
   [ '$real_object' => '#1' ],  # Object  undef if not yet created
   [ '$up' => '#2' ],           # how many levels upwards from root
   [ '$down' => '#3 && ( #3->down ? [ @{#3->down}, #4 ] : [ #4 ] )' ],  # (Property)  path to the subobject
   [ '$parent' => 'weak(#3)' ],
   [ '$property' => '#4 || #1->parent && #1->property' ],

   [ '$multi_selector' => 'undef' ],    # (Property | _::SelectMultiInstance)  path to a multiple subobject to select

   [ '$siblings' => 'undef' ],          # for the 0-th multiple subobject instance: [ DeputyObject of other instances ]
   [ '$sibling_of' => 'undef' ],        # for non-default or to-be-created multiple subobject instances: DeputyObject of the 0-th instance
                                        # for their ancestors in the hierarchy: corresponding ancestors of the 0-th instance
   '$is_sibling_ancestor',              # is an ancestor of a non-default or to-be-created multiple subobject instance
   [ '$rgr_prop_node' => 'undef' ],     # node in RuleGraph corresponding to this instance of a to-be-created multiple subobject

   [ '$type' => '(defined(#1) ? #1 : #3->type->property(#4->name))->type' ],

   [ '$valid_for_perm_trigger' => '#5' ],       # this deputy or one of its ancestors relates to a permutation subobject:
                                                # permutation trigger for which rule_cache, prod_cache, and prop_vertex_sets are valid

   '%rule_cache',                       # Rule => RuleDeputy
   '%rule_cache_for_perm_trigger',      # rule_cache created for current permutation trigger (all but the first round in gather_rules)
   '%prod_cache',                       # Property->key => [ RuleDeputy of producing rules ]
   '%prop_vertex_sets',                 # Property->key => [ int ... ]  vertices of Scheduler::Heap encoding this property

   '%subobj_cache',             # subobject Property->key => corresponding DeputyObject (for multiple: the 0-th instance)
   '%labels',
   '%sensitive_to',             # = real_object->sensitive_to at the begin of scheduling
                                # During phase 2 some rules creating sensitive properties might be called,
                                # but get_schedule must produce the chain according to the initial state of the object.
);

# Sibling constructor for multiple subobject instances: (real Object, 0-th instance DeputyObject, Property::SelectMultiInstance)
use Polymake::Struct (
   [ 'alt.constructor' => 'new_sibling' ],
   [ new => '$$$' ],
   [ '$up' => '#2->up' ],
   [ '$down' => 'do { local_pop(#2->down); [ @{#2->down}, #3 ]}' ],
   [ '$parent' => 'undef' ],
   [ '$property' => '#3' ],
   [ '$sibling_of' => 'weak(#2)' ],
   [ '$type' => '#2->type' ],
   [ '$valid_for_perm_trigger' => 'undef' ],
   [ '$sensitive_to' => 'undef' ],
);

# Constructor for ancestors of siblings
use Polymake::Struct (
   [ 'alt.constructor' => 'new_sibling_ancestor' ],
   [ new => '$' ],
   [ '$real_object' => '#1->real_object' ],
   [ '$up' => '#1->up' ],
   [ '$down' => '#1->down' ],
   [ '$parent' => 'undef' ],
   [ '$property' => '#1->property' ],
   [ '$sibling_of' => 'weak(#1)' ],
   [ '$is_sibling_ancestor' => 1 ],
   [ '$type' => '#1->type' ],
   [ '$valid_for_perm_trigger' => 'undef' ],
   [ '$prod_cache' => 'undef' ],
   [ '$prop_vertex_sets' => 'undef' ],
   [ '$sensitive_to' => '#1->sensitive_to' ],
);

####################################################################################

# move to the deputy of a parent object
# returns undef when the top-level object is reached
sub navigate_up {
   my ($self)=@_;
   $self->parent //= do {
      my $parent_deputy= defined($self->sibling_of)
                         ? new_sibling_ancestor DeputyObject($self->sibling_of->navigate_up)
                         : new DeputyObject($self->real_object->parent, $self->up+1);
      if (defined $self->multi_selector) {
         $parent_deputy->multi_selector=[ $self->property, @{$self->multi_selector} ];
      } elsif ($self->property->index) {
         $parent_deputy->multi_selector=[ $self->property ];
      }

      weak($parent_deputy->subobj_cache->{$self->property->property_key}=$self);
      $parent_deputy
   };
}

# move to a deputy of a subobject
# $prod_rule: RuleDeputy of a production rule being analyzed
# $how: 2 for an input of a production rule
#       1 for an output of a production rule w/o or above a new multiple subobject instance
#       0 for a recursive call processing an output below a new multiple subobject instance
# @path: a mix of Property, _::SelectMultiInstance, and _::NewMultiInstance
sub navigate_down {
   my ($self, $init_chain, $prod_rule, $how, @path)=@_;
   # whether the deputies created downstream should be marked as `valid for the given permutation trigger'
   my $inherit_trigger=defined($self->valid_for_perm_trigger);
   my $perm_trigger= $how==2 ? $init_chain->cur_perm_trigger : $prod_rule;
   my ($pv, $index);
   my $depth=0;
   foreach my $prop (@path) {
      $inherit_trigger ||= ($prop->flags & $Property::is_permutation) if defined($perm_trigger);

      my $subobj_deputy;
      while ($self->is_sibling_ancestor &&
             !defined( $subobj_deputy=$self->subobj_cache->{$prop->property_key} )) {
         # This property is not involved in multiple instance selection, or is a permutation subobject:
         # change to the main trunk of the object tree
         $self=$self->sibling_of;
      }

      $self=$subobj_deputy //
      ($self->subobj_cache->{$prop->property_key} //= do {
         my $real_subobj;
         if (!$inherit_trigger) {
            if (defined($self->real_object) &&
                defined ($index=$self->real_object->dictionary->{$prop->key})) {
               $pv=$self->real_object->contents->[$index] or return;
               $real_subobj=$pv->value;
            } else {
               # creation of new multiple subobjects for initial rules is not allowed
               return if $prod_rule->flags == $Rule::is_initial && $prop->flags & $Property::is_multiple;
            }
         }
         $subobj_deputy=new DeputyObject($real_subobj, $self->up, $self, $prop->property,
                                         $inherit_trigger ? $perm_trigger : undef);

         if ($prop->flags & $Property::is_twin) {
            weak($subobj_deputy->subobj_cache->{$prop->property_key}=$self);
         }
         $subobj_deputy
      });
      ++$depth;

      if ($prop->flags & $Property::is_multiple) {
         if ($prop->index) {
            # refers to a non-default multiple subobject instance
            $self=(($self->siblings //= [ ])->[$prop->index] //=
                   do {
                      my $real_parent=$self->parent->real_object;
                      if (defined($real_parent) &&
                          defined ($index=$real_parent->dictionary->{$prop->key}) &&
                          defined ($pv=$real_parent->contents->[$index])) {
                         new_sibling DeputyObject($pv->values->[$prop->index], $self, $prop);
                      } else {
                         die "internal error: SelectMultiInstance refers to a non-existing multiple subobject\n";
                      }
                   });
         } elsif (!$inherit_trigger && !defined($self->real_object) && !defined($self->sibling_of)) {
            if ($how==2) {
               $prop->key == $prop->property_key
                 or die "internal error: NewMultiInstance occurs in a rule input";

               # A rule requests a non-existing multiple subobject,
               # and we are neither within a side subtree of a new instance nor within a permutation subtree,
               # nor it's a nested new instance: present all possible new instances
               $self->siblings //= [ map { create_sibling_for_new_multi($self, $init_chain, 1, $_) }
                                         @{ $init_chain->producers_of_property($self->parent, $self->property->new_instance_deputy) } ];
               splice @path, 0, $depth;
               return map { navigate_down($_, $init_chain, $prod_rule, 2, @path) } @{$self->siblings};
            }
            if (defined($self->siblings) && defined($prod_rule->created_multi_instances)
                  and
                my ($sibling)=grep { $_->sibling_of==$self } @{$prod_rule->created_multi_instances}) {
               # revisiting an output path of a production rule creating a new instance, e.g. from gather_permutation_rules:
               # change to the corresponding side subtree
               $self=$sibling;
            } else {
               # It's a nested new instance, or an unsolicited new instance created as a side effect:
               # create a sibling just for this rule
               my $sibling=create_sibling_for_new_multi($self, $init_chain, $how, $prod_rule);
               push @{$self->siblings //= [ ]}, $sibling;
               $self=$sibling;
            }
         }
      }
   }
   validate_caches($self, $perm_trigger) if $inherit_trigger;
   $self
}

sub create_sibling_for_new_multi {
   my ($self, $init_chain, $create_property_leaves, $rule)=@_;
   my $sibling=new_sibling DeputyObject(undef, $self, new Property::SelectMultiInstance($self->property, $rule->rule));
   push @{$rule->created_multi_instances //= [ ]}, $sibling;
   if ($create_property_leaves) {
      $self->rgr_prop_node //= $init_chain->rgr->add_node;
      $sibling->rgr_prop_node=$init_chain->rgr->add_node;
      $init_chain->rgr->add_arc($self->rgr_prop_node, $sibling->rgr_prop_node, $rgr_exclusive_arc);
      $init_chain->rgr->add_arc($rule, $sibling->rgr_prop_node, $rgr_source_arc);

      foreach my $out (grep { get_array_flags($_) & $Property::is_multiple_new } @{$rule->output}) {
         for (my ($depth, $last)=(0, $#$out-1);  $depth <= $last;  ++$depth) {
            if ($out->[$depth]->key == $self->property->new_instance_deputy->key) {
               my $subobject= $depth==$last ? $sibling : navigate_down($sibling, $init_chain, $rule, 0, @$out[$depth+1..$last]);
               my $prop_key=$out->[-1]->key;
               $subobject->prod_cache->{$prop_key}=[ $rule ];
               my $vertex_set=$subobject->prop_vertex_sets->{$prop_key}=[ $init_chain->last_prop_vertex++ ];
               push @{$rule->prop_vertex_sets}, $vertex_set;
               my $prop_node=$init_chain->prop_nodes->[$vertex_set->[0]]=$init_chain->rgr->add_node;
               $init_chain->rgr->add_arc($sibling->rgr_prop_node, $prop_node, $rgr_unique_arc);
               last;
            }
         }
      }
   }
   $sibling
}

# deputy object => deputy sub-object
sub navigate_to_rule_owner {
   my ($self, $rule)=@_;
   for (my $i=$rule->up; $i>0; --$i) {
      $self=$self->parent;
   }
   if (defined $rule->down) {
      $self=navigate_down($self, undef, $rule, 0, @{$rule->down});
   }
   if (defined $rule->multi_selector) {
      $self=navigate_down($self, undef, $rule, 0, @{$rule->multi_selector});
      for (my $i=@{$rule->multi_selector}; $i>0; --$i) {
         $self=$self->parent;
      }
   }
   $self;
}

sub navigate_to_non_permuted {
   my ($self)=@_;
   my (@descend, $parent);
   for (;;) {
      $parent=$self->parent
        or die "internal error: permutation parent object not found\n";
      last if $self->property->flags & $Property::is_permutation;
      push @descend, $self->property;
      $self=$parent;
   }
   navigate_down($parent, undef, undef, 0, reverse @descend)
}

sub rule_instance {
   my ($self, $rule, $perm_trigger, $cache)=@_;
   return () if defined($self->property) && $self->property->key == $rule->not_for_twin;
   $cache //= $self->rule_cache;
   $cache->{$rule} //= new RuleDeputy($rule, $self, $perm_trigger // $self->valid_for_perm_trigger, $cache);
}

sub rule_instance_for_perm_trigger {
   my ($self, $rule, $perm_trigger)=@_;
   if (defined $self->valid_for_perm_trigger) {
      validate_caches($self, $perm_trigger);
      &rule_instance;
   } else {
      rule_instance(@_, $self->rule_cache_for_perm_trigger->{$perm_trigger} //= { });
   }
}

sub validate_caches {
   my ($self, $perm_trigger)=@_;
   if ($self->valid_for_perm_trigger != $perm_trigger) {
      %{$self->prod_cache}=();
      %{$self->prop_vertex_sets}=();
      if (keys %{$self->rule_cache}) {
         $self->rule_cache_for_perm_trigger->{$self->valid_for_perm_trigger}=$self->rule_cache;
         $self->rule_cache={ };
      }
      $self->valid_for_perm_trigger=$perm_trigger;
   }
}

sub has_sensitive_to {
   my ($self, $prop)=@_;
   return 0 unless defined $self->real_object;
   $self->sensitive_to->{$prop->key} //= $self->real_object->has_sensitive_to($prop);
}

sub lookup_rule_input {
   my ($self, $init_chain, $prod_rule, $input)=@_;
   defined($self->real_object) or return;
   my ($obj, $real_obj, $content_index, $pv);
   foreach my $path (@$input) {
      my $prop=local_pop($path);
      if (@$path) {
         ($obj)=navigate_down($self, $init_chain, $prod_rule, 2, @$path) or next;
      } else {
         $obj=$self;
      }
      last if defined($real_obj=$obj->real_object) &&
              defined($content_index=$real_obj->dictionary->{$prop->key}) &&
              defined($pv=$real_obj->contents->[$content_index]);
   }
   $pv
}

####################################################################################
package Polymake::Core::Scheduler::TentativeRuleChain;

# private:
# in:  $pending: [ RuleDeputy ] preconditions ready to evaluate
# out: $pending: [ RuleDeputy ] postponed preconditions (their suppliers were too expensive)
# return: ( RuleDeputy ) preconditions to be evaluated now (their suppliers were cheap)
# $suppliers: [ RuleDeputy ] : suppliers of filtered preconditions to be executed first

sub filter_preconditions_with_cheap_suppliers {
   my ($self, $pending, $suppliers)=@_;
   my $expensive_wt=$Rule::std_weight->[0];
   my $rule;

   # initialize the rule states
   foreach $rule (@{$self->rules}) {
      # set to 1 if too expensive
      $rule->temp_state= (!exists $self->run->{$rule} && $rule->weight->[0] >= $expensive_wt) ||
                         grep { !exists $self->run->{$_} } @{$rule->preconditions};
   }
   $_->temp_state=0 for @$pending;

   foreach $rule (@{$self->rules}) {
      if ($rule->temp_state) {
         # propagate the "too expensive" status to all dependees
         $_->temp_state=1 for $self->get_resolved_consumers($rule);
      }
   }

   my @result;
   for (my ($i, $last)=(0, $#$pending); $i<=$last; ) {
      if ($pending->[$i]->temp_state) {
         ++$i;
      } else {
         # mark all suppliers not executed yet
         ($rule)=splice @$pending, $i, 1;
         push @result, $rule;
         $_->temp_state=2 for grep { !exists $self->run->{$_} } $self->get_resolved_suppliers($rule);
         --$last;
      }
   }

   if (@result) {
      # make the transitive closure of all suppliers
      foreach $rule (reverse @{$self->rules}) {
         if ($rule->temp_state==2) {
            $_->temp_state=2 for grep { !exists $self->run->{$_} } $self->get_resolved_suppliers($rule);
         }
      }
      @$suppliers=grep { $_->temp_state==2 } @{$self->rules};
   }
   @result;
}
####################################################################################
# private:
sub weave_preconditions {
   my ($self)=@_;
   my %precond_seen;
   my @full_list=map {
      ((grep { not(exists $self->run->{$_} ||
                  ($_->flags & $Rule::is_spez_precondition && $precond_seen{$_}++))
        } @{$_->preconditions}), $_)
   } @{$self->rules};
   $self->rules=\@full_list;
}
####################################################################################
sub execute {           # => number of the failed rule
   my ($self, $object)=@_;
   my $i=0;

   foreach my $rule (@{$self->rules}) {
      my $rc=$rule->execute($object, 1);
      $self->run->{$rule}=$rc;
      if ($rc != $Rule::exec_OK) {
         if ($@ && $rule->flags != $Rule::is_initial) {
            if ($Verbose::rules) {
               chomp $@;
               warn_print( !($rule->flags & $Rule::is_precondition) && "rule ", $rule->header, " failed: $@" );
            }
            undef $@;
         }
         return $i;
      }
      ++$i;
   }
   undef;
}
####################################################################################
sub report {
   my ($self, $heap)=@_;
   return @{$self->rules}
          ? ( (map { ("  ", $_->header, "\n") } @{$self->rules}),
              $heap ? ("  Sum weight=", &tell_weight) : () )
          : ("nothing to do");
}

sub tell_weight {
   my ($self, $heap)=@_;
   join(".", $heap->unpack_weight($self))
}
####################################################################################
package Polymake::Core::Scheduler::InitRuleChain;

use Polymake::Struct (
   [ new => '$;$$' ],
   [ '@ISA' => 'TentativeRuleChain' ],
   [ '$give_schedule' => '#3' ],
   [ '$phase2germ' => 'undef' ],
   [ '$deputy_root' => 'undef' ],
   [ '$final' => 'undef' ],             # final pseudo-rule
   '$initial',                          # resolving an initial request
   '%preferred',                        # RuleDeputy => [ RuleDeputy preferred over this one ]
   '%dyn_weight',                       # weight RuleDeputy => result
   '%depending_on_perms',               # Rule => Rule => 1 if both rules trigger permutations identical or induced (either way round)
   [ '$last_prop_vertex' => '0' ],      # global counter for vertices of Scheduler::Heap assigned to properties created by production rules
   '@prop_nodes',                       # mapping of property vertex indices (in Scheduler::Heap) to RuleGraph nodes
   [ '$cur_perm_trigger' => 'undef' ],  # Rule triggering a permutation: during the gather phase for this permutation's subtree
   [ '$Tstart' => 'undef' ],
);

sub new {
   my $self=&_new;
   my ($object, $final_rule)=@_;
   if ($Verbose::scheduler) {
      dbg_print( "gathering viable rules" );
      $self->Tstart=[ gettimeofday() ];
      if ($Verbose::scheduler>=3 && $DebugLevel>=2) {
         require Polymake::Core::Scheduler_debug;
         $self->debug=new Debug();
      }
   }
   $self->deputy_root=new DeputyObject($object);
   $final_rule //= (state $final_dummy=create Rule('final', []));
   $self->rgr=new RuleGraph();
   $self->final=$self->deputy_root->rule_instance($final_rule);
   $self->rgr->add_node($self->final);
   $self;
}
####################################################################################
sub gettimeofday() {
   # don't expect it installed everywhere
   eval { require Time::HiRes };
   if ($@) {
      *gettimeofday=sub() { times };
      *tv_interval=sub { (times)[0]-(shift)->[0] };
   } else {
      import Time::HiRes qw( gettimeofday tv_interval );
   }
   gettimeofday();
}
####################################################################################
sub good_rules {
   my ($self, $object, $list)=@_;
   grep { !($self->run->{$_} & $Rule::exec_failed) }
      map { $object->rule_instance($_) }
         grep { !defined($object->real_object) || !$object->real_object->failed_rules->{$_} } @$list;
}

my $viable_rules=\&good_rules;
####################################################################################
sub consider_rules {
   my ($self, $object)=splice @_, 0, 2;
   foreach my $rule (@_) {
      unless (defined $rule->rgr_node) {
         $self->rgr->add_node($rule);
         push @{$self->rules}, $object, $rule;
         if (defined $rule->with_permutation) {
            # Don't process the "variant with permutation" separately;
            # prevent it from popping off the ready queue in the 2nd phase.
            $self->rgr->add_node($rule->with_permutation);
            $self->rgr->add_arc($rule, $rule->with_permutation, $rgr_unique_arc);
            foreach my $action (@{$rule->with_permutation->actions}) {
               $self->rgr->add_node($action);
               $self->rgr->add_arc($rule->with_permutation, $action, $rgr_unique_arc);
               $self->rgr->add_arc($action, $rule->with_permutation, $rgr_weak_arc);
               $self->rgr->add_arc($action, $self->final, $rgr_inactive_arc);
            }
         }
      }
   }
}
####################################################################################
sub producers_of_property {
   my ($self, $object, $prop, $source_arc)=@_;
   my $prod_key=$prop->key;

   while ($object->is_sibling_ancestor) {
      # sibling ancestors don't possess producer caches:
      # properties created here do not depend on multiple subobject instances dangling downstream
      $object=$object->sibling_of;
   }

   $object->prod_cache->{$prod_key} //= do {
      my (@rules, @list);
      if (@rules=$viable_rules->($self, $object, $object->type->get_producers_of($prop))) {
         consider_rules($self, $object, @rules);
         push @list, @rules;
      }
      my $this_level=$object;
      my ($ancestor, $lookup_for_perm_trigger);

      while (defined ($this_level->property) &&
             defined ($prod_key=$prod_key->{$this_level->property->key}) &&
             defined ($ancestor=$this_level->navigate_up)) {

         $lookup_for_perm_trigger ||= defined($self->cur_perm_trigger) && $this_level->property->flags & $Property::is_permutation;
         my $ancestor_rules=$ancestor->type->get_producers_of($prod_key);

         if (@$ancestor_rules  and
             @rules = $lookup_for_perm_trigger
                      ? (map { $ancestor->rule_instance_for_perm_trigger($_, $self->cur_perm_trigger) } @$ancestor_rules)
                      : $viable_rules->($self, $ancestor, $ancestor_rules)) {

            consider_rules($self, $ancestor, @rules);
            push @list, @rules;
         }
         $this_level=$ancestor;
      }

      if (@list && $source_arc) {
         # store the vertex set in all production rules but actions
         my $vertex_set=($object->prop_vertex_sets->{$prop->key} //= [ $self->last_prop_vertex++ ]);
         my $prop_node=($self->prop_nodes->[$vertex_set->[0]] //= $self->rgr->add_node);
         foreach my $rule (@list) {
            if ($rule->flags & $Rule::is_production) {
               push @{$rule->prop_vertex_sets}, $vertex_set;
            }
            $self->rgr->add_arc($rule, $prop_node, $source_arc);
         }
         if (defined($object->valid_for_perm_trigger)
             and not($object->property->flags & $Property::is_permutation && $object->type->isa($prop->defined_for))) {

            push @{$object->navigate_to_non_permuted->prop_vertex_sets->{$prop->key} //= [ $self->last_prop_vertex++ ]}, @$vertex_set;
         }
      }

      \@list
   }
}
####################################################################################
# private:
#  => undef if applicable, 'text' if infeasible
sub rule_status {
   my ($self, $object, $rule)=@_;
   my $input_arc=$rgr_source_arc;

   foreach my $input (@{$rule->input}) {
      my $input_flags=get_array_flags($input);
      if ($input_flags != $Property::is_permutation &&
          defined (my $pv=$object->lookup_rule_input($self, $rule, $input))) {
         if (defined($pv->value) || $rule->flags & $Rule::is_definedness_check) {
            next;
         } else {
            return $Verbose::scheduler>=2 && "undefined source " . Property::print_path($input);
         }
      }

      my $input_can_be_supplied;
      foreach my $input_path (@$input) {
         my $prop=local_pop($input_path);
         foreach my $subobj ($object->navigate_down($self, $rule, 2, @$input_path)) {
            $prop=$prop->new_instance_deputy if $prop->flags & $Property::is_multiple;
            my $prop_can_be_produced;
            foreach my $prod_rule (@{ producers_of_property($self, $subobj, $prop, $rgr_source_arc) }) {
               unless (defined($self->cur_perm_trigger)  &&
                       (# the "variant without permutation" can't serve as a supplier as we are building the permutation subtree here
                        $prod_rule==$self->cur_perm_trigger
                          or
                        # prevent a loop where a rule working on a permutation subobject tries to compute something
                        # using the property from the main object, but this property moves back to the main object
                        # after the the processing of the permutation has been finished
                        $prod_rule->flags & $Rule::is_perm_action        &&
                        $input_flags != $Property::is_permutation        &&
                        $rule->perm_trigger == $prod_rule->perm_trigger  &&
                        grep { get_array_flags($_) == $Property::is_permutation &&
                               list_index($_->[0], $prod_rule->perm_path->[-1])>=0
                        } @{$rule->input})) {

                  $prop_can_be_produced=1;
                  last;
               }
            }
            if ($prop_can_be_produced) {
               my $prop_owner=$subobj;
               while ($prop_owner->is_sibling_ancestor) {
                  $prop_owner=$prop_owner->sibling_of;
               }
               my $vertex_set=$prop_owner->prop_vertex_sets->{$prop->key};
               my $prop_node=$self->prop_nodes->[$vertex_set->[0]];
               $self->rgr->add_arc($prop_node, $rule, $input_arc);
               if (defined (my $perm_deputy=$rule->with_permutation)) {
                  $self->rgr->add_arc($prop_node, $perm_deputy, $input_arc);
               }
               $input_can_be_supplied=1;
            }
         }
      }

      unless ($input_can_be_supplied) {
         return $Verbose::scheduler>=2 && "no available rules to produce " . Property::print_path($input);
      }

      ++$input_arc;
   }
   undef;
}
####################################################################################
sub store_preferred_rules {
   my ($self, $family, $object)=@_;
   if (! $object->labels->{$family}++) {
      my (@pref, @next);
      foreach my $bag ($object->type->get_rules_by_label($family)->get_items_by_rank) {
         shift @$bag;   # get rid of the rank
         @next=$viable_rules->($self, $object, $bag);
         push @{$self->preferred->{$_}}, @pref for @next;
         push @pref, @next;
      }
   }
}
####################################################################################
# private:
sub gather_permutation_rules {
   my ($self, $object, $perm_trigger, $infeasible, $perm_actions_alive)=@_;
   $self->cur_perm_trigger=$perm_trigger;
   my $perm_deputy=$perm_trigger->with_permutation;
   $perm_deputy->created_multi_instances=$perm_trigger->created_multi_instances;
   my $prod_rule=$perm_trigger->rule;
   my $permutation_possible;

   # The fake rule created for Object::copy_permuted does not have body;
   # for it we should keep at least one action, otherwise the permutation will be dropped completely.
   my $remaining_actions=@{$perm_deputy->actions}+defined($perm_trigger->rule->code);

   foreach my $action_deputy (@{$perm_deputy->actions}) {
      my $out_path=$prod_rule->output->[$action_deputy->output];
      # Prepare for property restoring unless it already exists in the real object
      my $action_depth=$action_deputy->rule->depth;
      my $perm_obj=$object->navigate_down($self, $perm_trigger, 1,
                                          $action_depth ? (@$out_path[0..$action_depth-1], $action_deputy->rule->sub_permutation)
                                                        : @{$perm_deputy->perm_path});
      my ($object_for_restoring_rules, $final_vertex_set);
      {
         local_clip_front($out_path, $action_depth);
         $object_for_restoring_rules=$perm_obj->parent;
         --$remaining_actions;
         if (defined($object_for_restoring_rules->real_object) &&
             is_object($object_for_restoring_rules->real_object->lookup_property_path($out_path, 1)) &&
             $remaining_actions) {
            undef $object_for_restoring_rules;
         }
         my $prop=local_pop($out_path);
         my $prop_owner=$perm_obj->parent->navigate_down($self, $perm_trigger, 1, @$out_path);
         my $permuted_prop_owner=$perm_obj->navigate_down($self, $perm_trigger, 1, @$out_path);

         my $vertex_set=($prop_owner->prop_vertex_sets->{$prop->key} //= [ $self->last_prop_vertex++ ]);
         if (defined $object_for_restoring_rules) {
            # Ensure a separate one-element vertex set for restoring rules
            $final_vertex_set=($prop_owner->prop_vertex_sets->{$vertex_set} //= [ $vertex_set->[0] ]);
         }
         my $perm_vertex_set=$permuted_prop_owner->prop_vertex_sets->{$prop->key}=[ $self->last_prop_vertex++ ];
         push @{$perm_deputy->prop_vertex_sets}, $perm_vertex_set;
         push @{$vertex_set}, @$perm_vertex_set;

         $permuted_prop_owner->prod_cache->{$prop->key}=[ $perm_deputy ];
         my $prop_node=$self->prop_nodes->[$perm_vertex_set->[0]]=$self->rgr->add_node;
         $self->rgr->add_arc($perm_deputy, $prop_node, $rgr_source_arc);

         $perm_obj->sensitive_to->{$perm_trigger}=1;
      }
      if (defined $object_for_restoring_rules) {
         # If this output does not exist, it must be reconstructed from the permuted subobject even it was not asked for by other rules.
         my @restoring_rules=map { $object_for_restoring_rules->rule_instance_for_perm_trigger($_, $perm_trigger) } @{$action_deputy->rule->producers};
         consider_rules($self, $object_for_restoring_rules, @restoring_rules);
         foreach my $restoring_rule (@restoring_rules) {
            $self->rgr->add_arc($restoring_rule, $action_deputy, $rgr_source_arc);
            push @{$restoring_rule->prop_vertex_sets}, $final_vertex_set;
         }
         $action_deputy->rules_to_block=$perm_obj->sensitive_to;
         push @$perm_actions_alive, $action_deputy;
         $permutation_possible=1;
         dbg_print( $action_deputy->rule->enabled ? "  applicable: " : "  impossible: ", $action_deputy->header ) if $Verbose::scheduler>=2;
      } else {
         push @$infeasible, $action_deputy;
         dbg_print( "  not needed: ", $action_deputy->header ) if $Verbose::scheduler>=2;
      }
   }

   unless ($permutation_possible) {
      dbg_print( "  not needed: ", $perm_deputy->header) if $Verbose::scheduler>=2;
      push @$infeasible, $perm_deputy;
   }
}
####################################################################################
# protected:
sub gather_initial_rules {
   my ($self)=@_;
   $self->initial=1;
   my %protected_multi;

   foreach my $input (@{$self->final->input}) {
      my $found=0;
      {
         my $prop=local_pop($input);
         foreach my $initial_rule (@{ producers_of_property($self, $self->deputy_root->navigate_down($self, $self->final, 2, @$input), $prop) }) {
            $self->rgr->add_arc($initial_rule, $self->final, $rgr_initial_arc);
            push @{$initial_rule->prop_vertex_sets}, [ $self->last_prop_vertex++ ];
            $found=1;
         }
      }
      if ((my $depth=Property::find_first_in_path($input, $Property::is_restricted_spez))>=0) {
         local_clip_back($input, $depth);
         my $multi_deputy=$self->deputy_root->navigate_down($self, $self->final, 2, @$input);
         unless ($protected_multi{$multi_deputy}++) {
            my $checker_header= $Verbose::scheduler>=2 && "specialization checker for ".Property::print_path($input);
            my $checker_rule=new Rule::Dummy($checker_header, $Rule::is_production | $Rule::is_restricted_spez, $input->[$depth]->belongs_to);
            my $spez_deputy=$multi_deputy->navigate_up;
            my $checker_deputy=$spez_deputy->rule_instance($checker_rule);
            push @{$checker_deputy->prop_vertex_sets}, [ $self->last_prop_vertex++ ];
            consider_rules($self, $spez_deputy, $checker_deputy);
            $self->rgr->add_arc($checker_deputy, $self->final, $rgr_unique_arc);
         }
         $found=1;
      }
      if ($Verbose::scheduler>=2 && !$found) {
         dbg_print( "  skipped: no available rules to produce " . Property::print_path($input) );
      }
   }

   my $root_prop=$self->deputy_root->property;
   if (defined($root_prop) && $root_prop->flags & $Property::is_restricted_spez) {
      my $checker_header= $Verbose::scheduler>=2 && "specialization checker for this=".$root_prop->name;
      my $checker_rule=new Rule::Dummy($checker_header, $Rule::is_production | $Rule::is_restricted_spez, $root_prop->belongs_to);
      my $spez_deputy=$self->deputy_root->navigate_up;
      my $checker_deputy=$spez_deputy->rule_instance($checker_rule);
      push @{$checker_deputy->prop_vertex_sets}, [ $self->last_prop_vertex++ ];
      consider_rules($self, $spez_deputy, $checker_deputy);
      $self->rgr->add_arc($checker_deputy, $self->final, $rgr_unique_arc);
   }
}
####################################################################################
# protected:
sub gather_rules {
   my ($self)=@_;
   push @{$self->rules}, $self->deputy_root, $self->final;
   &gather_more_rules
}
####################################################################################
# protected:
sub gather_rules_for_prescribed {
   my ($self, $rules)=@_;
   my @rule_deputies=map { $self->deputy_root->rule_instance($_) } @$rules;
   my $unique_vertex_set=[ $self->last_prop_vertex++ ];
   foreach my $rule (@rule_deputies) {
      consider_rules($self, $self->deputy_root, $rule);
      push @{$rule->prop_vertex_sets}, $unique_vertex_set;
      $self->rgr->add_arc($rule, $self->final, $rgr_source_arc);
      $self->rgr->add_arc($rule->with_permutation, $self->final, $rgr_source_arc)
         if (defined $rule->with_permutation);
   }
   gather_more_rules($self);
}
####################################################################################
# must be kept in sync with RuleGraph::announce_t
my @elim_reasons=("no more supplier", "no more consumer", "no path to request");
sub tell_eliminated {
   my ($rule, $reason)=@_;
   dbg_print( "  discarding ", $rule->header, " : $elim_reasons[$reason]" );
}

# protected:
sub gather_more_rules {
   my ($self)=@_;
   my (@infeasible, %labels, @finalize_perms, @perm_actions_alive);

   # breadth-first search in the implicit graph of object states
   for (;;) {
    RULE:
      while (my ($object, $rule)=splice @{$self->rules}, 0, 2) {

         foreach my $precond_rule (@{$rule->preconditions}) {
            my $object_for_precond=$object;
            if ($precond_rule->flags & $Rule::is_spez_precondition) {
               for (my ($object_level, $spez_level)=($rule->level, $precond_rule->level); $object_level > $spez_level; --$object_level) {
                  $object_for_precond=$object_for_precond->navigate_up;
               }
            }

            if (defined($object_for_precond->real_object) &&
                $object_for_precond->real_object->failed_rules->{$precond_rule->rule}) {
               dbg_print( "  infeasible: ", $rule->header, ": failed ", $precond_rule->header, " (tested earlier)" )
                 if $Verbose::scheduler>=2;
               push @infeasible, $rule;
               next RULE;
            }

            # preconditions of object specializations may occur repeatedly
            if (defined (my $rc=$self->run->{$precond_rule})) {
               if ($rc == $Rule::exec_infeasible) {
                  dbg_print( "  infeasible: ", $rule->header, ": unsatisfied ", $precond_rule->header, " (tested above)" )
                    if $Verbose::scheduler>=2;
                  $self->run->{$rule}=$Rule::exec_infeasible;
                  next RULE;
               }
               next if $rc == $Rule::exec_OK;
            }

            if (!defined($precond_rule->rgr_node)) {
               $self->rgr->add_node($precond_rule);

               if (defined (my $explain=rule_status($self, $object_for_precond, $precond_rule))) {
                  dbg_print( "  infeasible: ", $precond_rule->header, ": $explain\n",
                             "  infeasible: ", $rule->header, " due to precondition above" )
                    if $explain;
                  push @infeasible, $rule, $precond_rule;
                  $self->run->{$precond_rule}=$Rule::exec_infeasible;
                  $self->run->{$rule}=$Rule::exec_infeasible;
                  next RULE;
               }

               if ($self->rule_is_ready_to_use($precond_rule)) {
                  dbg_print( "  ready to evaluate: ", $precond_rule->header ) if $Verbose::scheduler>=2;
                  my ($rc, $retval)=$precond_rule->execute($object_for_precond->real_object, -1);
                  if (($self->run->{$precond_rule}=$rc) != $Rule::exec_OK) {
                     if ($@) {
                        chomp $@;
                        warn_print( $precond_rule->header, " failed: $@" );
                        undef $@;
                     }
                     dbg_print( "  infeasible: ", $rule->header, " due to precondition above" )
                       if $Verbose::scheduler>=2;
                     push @infeasible, $rule;
                     $self->run->{$rule}=$Rule::exec_infeasible;
                     next RULE;
                  }
                  $self->dyn_weight->{$precond_rule}=$retval if $precond_rule->flags & $Rule::is_dyn_weight;
                  next;
               }

               dbg_print( "  postponed: ", $precond_rule->header ) if $Verbose::scheduler>=2;
            }

            # the precondition rule must be evaluated later, let it become an exclusive supplier of the current production rule
            $self->rgr->add_arc($precond_rule, $rule, $rgr_unique_arc);
         }

         if (defined (my $explain_failure=rule_status($self, $object, $rule))) {
            dbg_print( "  infeasible: ", $rule->header, ": $explain_failure" ) if $explain_failure;
            return if $rule==$self->final;
            push @infeasible, $rule;
            $self->run->{$rule}=$Rule::exec_infeasible;
            if (defined $rule->with_permutation) {
               foreach ($rule->with_permutation, @{$rule->with_permutation->actions}) {
                  push @infeasible, $_;
                  $self->run->{$_}=$Rule::exec_infeasible;
               }
            }
            next RULE;

         } else {
            dbg_print( "  applicable: ", $rule->header ) if $Verbose::scheduler>=2;
         }

         if (defined $rule->with_permutation) {
            push @finalize_perms, $object, $rule;
         }
         foreach my $label (@{$rule->labels}) {
            store_preferred_rules($self, $label->wildcard_name, $object);
         }
      }

      if (@finalize_perms) {
         gather_permutation_rules($self, splice(@finalize_perms, 0, 2), \@infeasible, \@perm_actions_alive);
      } else {
         last;
      }
   }

   undef $self->cur_perm_trigger;
   foreach my $action (@perm_actions_alive) {
      foreach my $sensitive_rule (keys %{$action->rules_to_block}) {
         if ($sensitive_rule != $action->perm_trigger) {
            $self->rgr->add_arc($action, $sensitive_rule, $rgr_inactive_arc);
         }
      }
   }

   finalize_gather($self, $Verbose::scheduler>=2 && \&tell_eliminated, @infeasible) or return;
   undef $self->prop_nodes;
   squeeze_prefs($self);
   $self->debug_print("gathered", undef, 1) if defined $self->debug;
   1
}
####################################################################################
sub squeeze_prefs {
   my ($self)=@_;
   while (my ($rule, $list)=each %{$self->preferred}) {
      next if !$self->rule_is_alive($rule); # not relevant
      my $r=0;
      while ($r<=$#$list) {
         my $pref_rule=$list->[$r];
         if (!$self->rule_is_alive($pref_rule)      # eliminated
             || ($self->run->{$pref_rule} & $Rule::exec_failed)) {
            splice @$list, $r, 1;
         } else {
            ++$r;
         }
      }
      if ($Verbose::scheduler>=2) {
         if (!$r) {
            dbg_print( "  preferred: ", $rule->header );
         } else {
            dbg_print( "  not preferred: ", $rule->header,
                       map { "\n             after ".$_->header."\n" } @$list );
         }
      }
   }
}
####################################################################################
sub find_relative_path {
   my ($object, $r1, $r2)=@_;
   my $up1=$r1->up;
   my $up2=$r2->up;
   my ($diff, @path);
   if (defined $r1->down) {
      if (defined $r2->down) {
         return if $up1!=$up2;
         # starts in the same node: check whether the shorter path is the subset of the longer one
         my $d1=@{$r1->down};
         my $d2=@{$r2->down};
         $diff= $d2<=>$d1;
         return if !$diff;
         ($r1,$d1,$r2,$d2)=($r2,$d2,$r1,$d1) if $diff<0;
         for (my $i=0; $i<$d1; ++$i) {
            return if $r1->down->[$i]->key != $r2->down->[$i]->key;
         }
         return ($diff, @{$r2->path}[$d1..$d2-1]);

      } else {
         return if $up1>$up2;
         $diff=-1;
         @path=@{$r1->down};
      }

   } elsif (defined $r2->down) {
      return if $up2>$up1;
      $diff=1;
      @path=@{$r2->down};

   } else {
      # both sit on the direct ascending path from the root object
      $diff= $up1<=>$up2;
      return if !$diff;
   }

   ($up1,$up2)=($up2,$up1) if $diff<0;
   while ($up2>0) { $object=$object->parent; --$up2; --$up1; }
   while ($up1>0) { unshift @path, $object->property; $object=$object->parent; --$up1; }

   ($diff, @path);
}

sub compare_permutations {
   my ($perm_path_upper, $perm_path_lower, @path_between)=@_;
   if (Property::equal_path_prefixes($perm_path_upper, \@path_between) == $#$perm_path_upper) {
      splice @path_between, 0, $#$perm_path_upper;
      my $perm=local_pop($perm_path_lower);
      $perm_path_upper->[-1]->find_sub_permutation(@path_between, @$perm_path_lower)==$perm;
   }
}

sub creating_related_permutation {
   my ($self, $rule)=splice @_,0,2;
   my $perm_path=$rule->with_permutation->perm_path;

   foreach my $prev_rule (@_) {
      my $prev_perm_path=$prev_rule->with_permutation->perm_path;
      return 1 if equal_lists($perm_path, $prev_perm_path);

      if (my $dep=($self->depending_on_perms->{$rule}->{$prev_rule} //= do {
            my $result=0;
            if (my ($rel_pos, @path_between)=find_relative_path($self->deputy_root, $rule, $prev_rule)) {
               if ($rel_pos<0) {
                  # must ascend from $rule's object to $prev_rule's object
                  $result=compare_permutations($prev_perm_path, $perm_path, @path_between);
               } else {
                  # must descend from $rule's object to $prev_rule's object
                  $result=compare_permutations($perm_path, $prev_perm_path, @path_between);
               }
            }
            $self->depending_on_perms->{$prev_rule}->{$rule}=$result
         })) {
         return 1;
      }
   }
   0
}

sub add_ready_rule {
   my ($self, $var, $rule, $enforced, $heap)=@_;
   my $rule_to_add=$rule;
   my $perm_deputy=$rule->with_permutation;
   if (defined($perm_deputy) && $self->rule_is_alive($perm_deputy)) {
      my $subobj=$self->deputy_root->navigate_to_rule_owner($rule);
      my $perm_path=$perm_deputy->perm_path;
      if (@$perm_path>1) {
         $subobj=$subobj->navigate_down(undef, $rule, 0, @$perm_path[0..$#$perm_path-1]);
      }
      if ($subobj->has_sensitive_to($perm_path->[-1])
          || creating_related_permutation($self, $rule, grep { defined $_->with_permutation } @{$var->rules})) {

         # must create permutation subobject: check whether its further processing is still possible
         if ($var->rule_is_alive($perm_deputy)) {
            $rule_to_add=$perm_deputy;
         } else {
            return 0;
         }
      }
   }

   if (defined $heap) {
      $heap->new_tentative_chain($var);
      $heap->add_weight($rule->weight);
      if (defined (my $dwr=$rule->dyn_weight)) {
         if (defined (my $wt=$self->dyn_weight->{$dwr})) {
            $heap->add_weight($wt);
         }
      }
      my $pref=$self->preferred->{$rule};
      if (defined($pref) && @$pref) {
         state $pref_violation=[ $Rule::max_major, 1 ];
         $heap->add_weight($pref_violation);
      }
      $heap->is_promising($rule_to_add->prop_vertex_sets) or return 0;
   }

   if ($var->add_scheduled_rule($rule_to_add, $enforced, $rule)) {
      push @{$var->rules}, $rule_to_add;
      1
   }
}
####################################################################################
# private:
# => ( Rule ) complete schedule
# FinalVar->rules contains at return only pending preconditions and their suppliers

sub prepare_schedule {
   my ($self)=@_;
   my ($rule, @result, %precond_seen);

   # mark the direct suppliers of pending preconditions
   foreach $rule (@{$self->rules}) {
      $rule->temp_state=0;
      foreach my $precond (grep { !exists $self->run->{$_} } @{$rule->preconditions}) {
         $_->temp_state=1 for $self->get_resolved_suppliers($precond);
      }
   }

   # make the transitive closure of suppliers
   foreach $rule (reverse @{$self->rules}) {
      if ($rule->temp_state) {
         $_->temp_state=1 for $self->get_resolved_suppliers($rule);
      }
   }

   for (my $i=0; $i<=$#{$self->rules}; ) {
      $rule=$self->rules->[$i];
      my @uniq_precond=grep { not($_->flags & $Rule::is_spez_precondition && $precond_seen{$_}++) } @{$rule->preconditions};
      push @result, @uniq_precond, $rule;
      my @pending=grep { !exists $self->run->{$_} } @uniq_precond;
      splice @{$self->rules}, $i, !$rule->temp_state, @pending;
      $i += @pending+$rule->temp_state;
   }

   @result;
}
####################################################################################
sub replay_rules {
   my ($self, $heap, @replay_rules)=@_;
   @replay_rules or return;

   my $print_dumps=defined($self->debug);
   my ($i, $last, $rule);

   while (@replay_rules) {
      if ($print_dumps) {
         $self->debug_print("replay");
      }
      # filter out preconditions tested earlier, don't execute others
      my @satisfied;
      for (($i, $last)=(0, $#{$self->ready}); $i<=$last; ) {
         $rule=$self->ready->[$i];
         if ($rule->flags & $Rule::is_precondition && exists $self->run->{$rule}) {
            if ($Application::plausibility_checks && $self->run->{$rule} != $Rule::exec_OK) {
               die "internal error: rule ", $rule->header, " occurs in the 'ready' list although known to have failed";
            }
            push @satisfied, $rule;
            splice @{$self->ready}, $i, 1;  --$last;
         } else {
            ++$i;
         }
      }

      foreach $rule (@satisfied) {
         $self->add_scheduled_rule($rule, 1)
           or die "internal error: initial chain became infeasible while replaying ", $rule->header, "\n";
      }

      while (defined ($rule=shift @replay_rules) && $self->rule_is_alive($rule)) {
         $i=list_index($self->ready, $rule);
         if ($i>=0) {
            if ($self->give_schedule) {
               push @{$self->phase2germ->rules}, $rule;
            }
            splice @{$self->ready}, $i, 1;

            if (add_ready_rule($self, $self, $rule, 1) and @{$self->ready}) {
               $heap->add_to_vertex_filter($self->rules->[-1]->prop_vertex_sets);
               last;
            } else {
               die "internal error: progress stopped while replaying rules from the previous run; replay list is:",
                   (map { "\n  ".$_->header } $rule, @replay_rules);
            }
         } else {
            # This rule or one of its preconditions depends on an expensive supplier which was executed
            # in a previous (failed) round.  As expensive rules are exempt from replaying, their dependees
            # must also be processed in regular manner.
            $self->give_schedule
               or die "internal error: rule ", $rule->header, "\nhas to be replayed but missing in the ready list";
         }
      }
   }

   # replaying of the previous run finished: forget the already run rules, start producing variants
   if ($Application::plausibility_checks) {
      if (my @wrong=grep { !exists $self->run->{$_} || $self->run->{$_} != $Rule::exec_OK } @{$self->rules}) {
         die "internal error: unexecuted or failed rules included during replaying phase:",
             (map { "\n  ".$_->header." => ".($self->run->{$_} // "NOT RUN") } @wrong);
      }
      if ($self->is_complete) {
         die "internal error: schedule allegedly resolved while still replaying rules from the previous run:",
             ( map { "\n  ".$_->header } @_[1..$#_] );
      }
   }
   @{$self->rules}=();
}
####################################################################################
sub build_cheapest_chain {
   my ($self, $heap, $object)=@_;
   dbg_print( "composing a minimum weight rule chain" ) if $Verbose::scheduler;
   my $print_dumps=defined($self->debug);
   my $pop_dump_title="reset";
   my ($i, $last, $rule);

   my $top= $self->give_schedule==2 ? $self->phase2germ : clone($self);
   for (;;) {
      if ($print_dumps) {
         $top->debug_print($pop_dump_title, $heap, 1);
      }
      # If there are ready-to-evaluate pending preconditions, process them now, don't split off new variants on them.
      my (@pending, @satisfied);
      for (($i, $last)=(0, $#{$top->ready}); $i<=$last; ) {
         $rule=$top->ready->[$i];
         if ($rule->flags & $Rule::is_precondition) {
            if (exists $self->run->{$rule}) {
               if ($Application::plausibility_checks && $self->run->{$rule} != $Rule::exec_OK) {
                  die "internal error: rule ", $rule->header, " occurs in the 'ready' list although known to have failed";
               }
               push @satisfied, $rule;
            } else {
               push @pending, $rule;
            }
            splice @{$top->ready}, $i, 1;  --$last;
         } else {
            ++$i;
         }
      }

      if (@pending) {
         if ($self->give_schedule==2) {
            die "internal error: discovered unchecked preconditions during the second phase of resolving"
         }
         my @cheap_suppliers;
         if (my @precond=filter_preconditions_with_cheap_suppliers($top, \@pending, \@cheap_suppliers)) {
            dbg_print( "checking pending preconditions:", map { "\n".$_->header } @precond ) if $Verbose::scheduler>=2;
            my (@succeeded_suppliers, @failed);

            foreach $rule (@cheap_suppliers) {
               if (($self->run->{$rule}=$rule->execute($object, 0)) == $Rule::exec_OK) {
                  push @succeeded_suppliers, $rule;
               } else {
                  if ($@) {
                     chomp $@;
                     warn_print( "rule ", $rule->header, " failed: $@" );
                     undef $@;
                  }
                  push @failed, $rule;
                  last;
               }
            }

            unless (@failed) {
               foreach $rule (@precond) {
                  my ($rc, $retval)=$rule->execute($object, 1);
                  $self->run->{$rule}=$rc;
                  if ($rc==$Rule::exec_OK) {
                     $self->dyn_weight->{$rule}=$retval if $rule->flags & $Rule::is_dyn_weight;
                  } else {
                     dbg_print( $rule->header, " failed" ) if $Verbose::scheduler>=2;
                     push @failed, $rule;
                  }
               }
            }
            if (@failed) {
               eliminate($self, $rgr_initial_arc, @failed) or last;
               squeeze_prefs($self);
            }
            $heap->reset;
            replay_rules($self, $heap, grep { $self->rule_is_alive($_) } @succeeded_suppliers);
            $top=clone($self);
            $pop_dump_title="reset";
            next;
         }
      }

      foreach $rule (@pending, @satisfied) {
         unless ($top->add_scheduled_rule($rule, 1)) {
            $#{$top->ready}=-1;  # usually shouldn't happen
            last;
         }
      }

      my $premature_next;

      # For an initial request, schedule all initial rule in a single batch.
      # For other requests, unconditionally schedule trivial rules and rules concluding the restoration of permuted properties
      # (the latter becomes indispensable by definition once activated).
      #
      # In the second phase, however, trivial rules must prove their usefulness much like any other production rule.
      # Preconditions and initial rules cannot occur there.

      for ($i=0; $i<=$#{$top->ready}; ) {
         $rule=$top->ready->[$i];
         if ($rule->flags & $Rule::is_precondition) {
            $premature_next=1;
            ++$i;
         } elsif ($self->initial
                  ? $rule->flags & $Rule::is_initial
                  : ($rule->flags & $Rule::is_perm_restoring
                       or
                     $self->give_schedule!=2 && $rule->weight->[0]==0 && $rule->weight->[1]<=1)) {

            splice @{$top->ready}, $i, 1;
            unless (add_ready_rule($self, $top, $rule, 1, $self->initial ? undef : $heap)) {
               $#{$top->ready}=-1;
               last;
            }
            return $top if $top->is_complete;
            $i=0 unless $self->initial;  # some other ready rules might be deleted as superfluous, must rescan
         } else {
            ++$i;
         }
      }
      next if $premature_next;

      # Split off feasible variants for each ready production rule.
      while ((my $more_ready=$#{$top->ready}) >= 0) {
         ($rule, my $var)= $more_ready ? ($top->select_ready_rule, clone($top)) : (shift @{$top->ready}, $top);
         if (add_ready_rule($self, $var, $rule, 0, $heap) && @{$var->ready}) {
            $heap->push($var);
            if ($print_dumps) {
               $var->debug_print("push", $heap);
               $pop_dump_title="pop";
            }
         }

         # eliminate the skipped rule in the stem variant,
         # treat initial rules as mandatory prerequisites
         $more_ready && eliminate($top, $rgr_weak_arc, $rule)
           or last;
      }

      defined ($top=$heap->pop) or last;
      if ($top->is_complete) {
         if ($print_dumps) {
            $top->debug_print("pop", $heap);
         }
         last;
      }
   }
   $top;
}
####################################################################################
#
# return value:
# => 1 if some rule chain successfully executed (or there was nothing to do)
# => 0 if no suitable rule chain found
# => -n if n different rule chains tried, but none succeeded
# RuleChain || undef if $give_schedule

sub resolve {
   my ($self)=@_;
   my $object=$self->deputy_root->real_object;
   if ($self->is_complete) {
      if ($self->give_schedule) {
         return new RuleChain($object);
      } else {
         dbg_print( "nothing to do" ) if $Verbose::scheduler;
         return 1;
      }
   }
   my $tries=0;
   if (@{$self->ready}) {
      my $heap=new Heap($Rule::max_major, $self->last_prop_vertex);
      if (defined $self->debug) {
         $heap->tell_dropped(\&tell_dropped);
      }
      if ($self->give_schedule) {
         $self->phase2germ=clone($self);
      }

      while (defined (my $top=build_cheapest_chain($self, $heap, $object))) {
         if ($Verbose::scheduler) {
            dbg_print( $self->give_schedule==2 ? "rule chain refined" : "minimum weight rule chain constructed",
                       sprintf(" in %.3f sec.\n", tv_interval($self->Tstart)),
                       "      |heap|: cur=", $heap->size+1, ", max=", $heap->maxsize, ", tot=", $heap->totalsize, ", #pop=", $heap->popcount );
         }
         my $last_failed;
         if ($self->give_schedule) {
            # pick out pending preconditions and check them now
            # pick the scheduled production rules before they are intermixed with preconditions
            my @rules_for_phase2=map { $_->without_permutation } (@{$top->rules}, @{$self->phase2germ->rules})
              if $self->give_schedule==1;
            my @chain=prepare_schedule($top);
            unless (@{$top->rules} and defined( $last_failed=execute($top, $object) )) {
               if ($self->give_schedule==2) {
                  return new RuleChain($object, map { $_->copy4schedule($self) } @chain);
               }
               if ($Verbose::scheduler) {
                  dbg_print( "all preconditions satisfied; refining the rule chain" );
                  $self->Tstart=[ gettimeofday() ];
               }
               my $perm_deputy;
               $self->phase2germ->constrain_to_rules($self, $top, map {
                  $perm_deputy=$_->with_permutation;
                  ($_, @{$_->preconditions}, defined($perm_deputy) ? ($perm_deputy, @{$perm_deputy->actions}) : ())
               } @rules_for_phase2);
               @{$self->phase2germ->rules}=();
               $heap->reset;
               $heap->clear_vertex_filter;
               $self->give_schedule=2;
               next;
            }
         } else {
            weave_preconditions($top);
            dbg_print( "rules to execute:\n", report($top, $heap) ) if $Verbose::scheduler;
            my $ret= @{$top->rules} && $top->rules->[-1]->flags & $Rule::is_function ? (pop @{$top->rules})->rule : 1;
            unless (defined( $last_failed=execute($top, $object) )) {
               return $ret;
            }
         }

         my ($failed_rule)=splice @{$top->rules}, $last_failed;
         return if $failed_rule->flags == $Rule::is_initial;
         if ($Verbose::scheduler) {
            dbg_print( "trying to find an alternative way" );
            $self->Tstart=[ gettimeofday() ];
         }
         # if it was a precondition, let the object create the undef properties afterwards
         ++$tries unless $failed_rule->flags & $Rule::is_precondition;
         my @succeeded=@{$top->rules};
         eliminate($self, $rgr_initial_arc, $failed_rule) or last;
         squeeze_prefs($self);

         # Only replay production rules.  Preconditions and PermActions are added automatically.
         my @replay_rules=map { $_->without_permutation } grep { $_->flags & $Rule::is_production && $self->rule_is_alive($_) } @succeeded;

         if (@{$self->ready}) {
            if ($self->give_schedule) {
               # do not force applying expensive rules, the resulting schedule might become far from optimum
               my $std_wt=$Rule::std_weight->[0];
               @replay_rules=grep { $_->weight->[0] <= $std_wt } @replay_rules;
            }
            if ($Verbose::scheduler>=3 && @replay_rules) {
               dbg_print( "replaying rules:\n", map { "    ".$_->header."\n" } @replay_rules );
            }
            $heap->reset;
            replay_rules($self, $heap, @replay_rules);
         } else {
            last;
         }
      }
   }
   return $self->give_schedule ? undef : -$tries;
}
####################################################################################
package Polymake::Core::Scheduler::Heap;
use Polymake::Ext;

package Polymake::Core::Scheduler::RuleGraph;
use Polymake::Ext;

####################################################################################
#  top-level public functions

package Polymake::Core::Scheduler;

# object, [ rules ] => success code
sub resolve_rules {
   my ($object, $rules)=@_;
   my $self=new InitRuleChain($object);
   $self->gather_rules_for_prescribed($rules) &&
   $self->resolve;
}

sub get_chain {
   my ($object, $request)=@_;
   my $self=new InitRuleChain($object, create Rule('request', $request, 1), 1);
   $self->gather_rules &&
   $self->resolve;
}

sub resolve_permutation {
   my ($object, $rule)=@_;
   my $self=new InitRuleChain($object);
   $rule=$self->deputy_root->rule_instance($rule);
   {
      my $perm_path=$rule->with_permutation->perm_path;
      my $perm=local_pop($perm_path);
      $self->deputy_root->navigate_down($self, $rule, 1, @$perm_path)->sensitive_to->{$perm->key}=1;
   }
   $self->consider_rules($self->deputy_root, $rule);
   $self->rgr->add_arc($rule, $self->final, $rgr_source_arc);
   $self->rgr->add_arc($rule->with_permutation, $self->final, $rgr_source_arc);
   $self->gather_more_rules && $self->resolve;
}
####################################################################################
sub resolve_initial_request {
   my ($object, $request)=@_;
   my $self=new InitRuleChain($object, create Rule('initial commit', $request, 1));

   # Only a limited subset of production rules is allowed:
   # - it must not involve any permutations
   # - it must not create subobjects by applying default values
   my $further_rule_filter=$viable_rules;
   local_scalar($viable_rules,
                sub {
                   my $rules=pop;
                   $further_rule_filter->(@_,
                      [ grep {
                           $_->flags & ($Rule::is_precondition | $Rule::is_initial)
                             or
                           $_->flags & $Rule::is_production &&
                           ( $_->flags & $Rule::is_default_value
                             ? defined($_[1]->real_object)
                             : !$_->with_permutation )
                        } @$rules
                      ] )
                });
   $self->gather_initial_rules;
   @{$self->rules}==0
     or $self->gather_more_rules && $self->resolve > 0;
}
####################################################################################
sub resolve_request {
   my ($object, $request)=@_;
   my $final=create Rule('request', $request, 1);
   my $self=new InitRuleChain($object, $final);
   my $success=$self->gather_rules && $self->resolve;

   if ($success<=0) {
      my $shortening_scope;
      shorten_input($object, $_, $shortening_scope) for @{$final->input};
      if (defined $shortening_scope) {
         $self=new InitRuleChain($object, $final);
         if ($self->gather_rules && $self->resolve > 0) {
            undef $shortening_scope;
            $self=new InitRuleChain($object, $final);
            $success=$self->gather_rules && $self->resolve;
         }
      }
      return ($success, $self) if $success==0;
   }
   $success;
}
####################################################################################
# private:
sub shorten_input {
   my ($obj, $input, $scope)=@_;
   my ($pv, $content_index);
   foreach my $input_path (@$input) {
      for (my $i=0; $i<$#$input_path; ++$i) {
         next if defined ($obj) &&
                 defined ($content_index=$obj->dictionary->{$input_path->[$i]->key}) &&
                 defined ($pv=$obj->contents->[$content_index]) &&
                 defined ($obj=$pv->value);

         if ($input_path->[$i]->flags & $Property::is_produced_as_whole) {
            $scope //= ($_[2]=new Scope());
            $scope->begin_locals;
            local_clip_back($input_path, $i);
            $scope->end_locals;
            last;
         }
      }
   }
}
####################################################################################
my $disabled_rules;

sub temp_disable_rules {
   unless (defined $disabled_rules) {
      my $further_rule_filter=$viable_rules;
      $Scope->begin_locals;
      local_scalar($disabled_rules, { });
      local_scalar($viable_rules,
                   sub {
                      my $rules=pop;
                      $further_rule_filter->(@_, [ grep { !$disabled_rules->{$_} } @$rules ] )
                   });
      $Scope->end_locals;
   }
   foreach my $rule (@_) {
      $disabled_rules->{$rule}=1;
      if (defined($rule->with_permutation)) {
         $disabled_rules->{$_}=1 for ($rule->with_permutation, @{$rule->with_permutation->actions});
      }
   }
}
####################################################################################
package Polymake::Core::Scheduler::RuleChain;

use Polymake::Struct (
   [ new => '$@' ],
   [ '$genesis' => 'weak(#1)' ],
   [ '@rules' => '@' ],
);

sub list { map { $_->header } @{$_[0]->rules} }

sub apply {
   my ($self, $object)=@_;
   if ($object==$self->genesis) {
      foreach my $rule (@{$self->rules}) {
         unless ($rule->flags & ($Rule::is_function | $Rule::is_precondition)) {
            $rule->execute($object, 0)==$Rule::exec_OK or return;
         }
      }
   } else {
      foreach my $rule (@{$self->rules}) {
         $rule->execute($object, 1)==$Rule::exec_OK or return;
      }
   }
   1
}

sub list_new_properties {
   my ($self)=@_;
   uniq(map { $_->list_results($self->genesis->type) } @{$self->rules})
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
