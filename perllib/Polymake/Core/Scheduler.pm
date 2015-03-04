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

package Polymake::Core::Scheduler;
use Polymake::Enum qw( is_mult_chooser=256 );    # must not intersect with Rule::is_* flags

declare $dry_run;               # boolean flag used in compatibility mode

####################################################################################
#
#  A rule chain under construction
#
package Polymake::Core::Scheduler::TentativeRuleChain;
use Polymake::Struct (
   [ '$weight' => 'new Rule::Weight' ],                 # sum weight of the rules
   '@rules',                    # (Rule)  rules scheduled for execution, in proper order
   '%consumer',                 # Rule => { Rule => 1 }
                                #   outgoing arcs in rule precedence graph: rules consuming the output of the given rule
   '%supplier',                 # Rule => [ { Rule => 1 } ]
                                #   incoming arcs in rule precedence graph: rules producing data needed as input by given rule.
                                #   Suppliers are grouped by input properties of the given rule.  As soon as the
                                #   supplier list become empty, the rule is put in the 'ready' list (below).
                                #   additional supplier lists triggered by other rules scheduled prior to this
   '%pending_perms',            # Property => { PermAction => 1 }  expected but not yet included
   '@ready',                    # (Rule)  rules whose inputs are all resolved, that is, created by other rules scheduled earlier.
   '%preferred',                # Rule => [ Rules preferred over this one ]
   '$final',                    # Rule  final rule (usually request)
   '%run',                      # (precondition or weight) Rule => success code
   '%dyn_weight',               # weight Rule => result

   [ '$debug' => 'undef' ],     # Debugging info
);

sub clone {
   my $self=shift;
   Struct::make_body(
      $self->weight,
      [ @{$self->rules} ],
      { %{$self->consumer} },
      { %{$self->supplier} },
      $self->pending_perms,
      [ @{$self->ready} ],
      $self->preferred, $self->final, $self->run, $self->dyn_weight,
      defined($self->debug) ? $self->debug->clone : undef,
      __PACKAGE__);
}

package Polymake::Core::Scheduler::Debug;

use Polymake::Struct (
   [ '$id' => 'undef' ],        # unique identifier
   [ '$children' => '0' ],      # number of variants derived from this chain
);

sub clone {
   my $self=shift;
   Struct::make_body(
      (defined($self->id) ? $self->id."." : "").(++$self->children),
      0,
      $self);
}
####################################################################################
package Polymake::Core::Scheduler::RuleDeputy;

use Polymake::Struct (
   [ '@ISA' => 'Rule::Deputy' ],
   [ new => '$$;$$' ],
   [ '$up' => '#2' ],
   [ '@down' => '#3' ],
   [ '$dyn_weight' => 'undef', ],
   '@preconditions',
   [ '$with_permutation' => 'undef' ],
   [ '$_without_permutation' => 'undef' ],
   [ '$perm_trigger | actions' => 'weak( #4 )' ],
);

sub new {
   my $cache=pop;
   my $self=&_new;
   @{$self->preconditions}=map { _new($self, $_, @_[1..3]) } @{$self->rule->preconditions};
   if (defined $self->rule->dyn_weight) {
      $self->dyn_weight = _new($self, $self->rule->dyn_weight, @_[1..3]);
   }
   if (defined $self->rule->with_permutation) {
      $cache->{$self->rule->with_permutation} =
      $self->with_permutation = _new($self, $self->rule->with_permutation, @_[1..2]);
      $self->with_permutation->actions=[ map { $cache->{$_}=_new($self, $_, @_[1..2], $self) }
                                             @{$self->rule->with_permutation->actions} ];
      weak($self->with_permutation->_without_permutation = $self);
   }
   $self;
}

sub copy4schedule {
   my ($self, $tentative)=@_;
   (my $rule=$self->rule)->copy4schedule($tentative);
   $_[0]=_new($self, $rule, $self->up, $self->down);
}

sub permutation { $_[0]->rule->permutation }
sub enabled { $_[0]->rule->enabled }

sub without_permutation {
   my ($self)=@_;
   $self->_without_permutation // $self
}

sub checking_precondition {
   my ($self)=@_;
   _new($self, $self->rule->checking_precondition, $self->up, $self->down);
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
   ( is_object($self->perm_trigger) && !($self->flags & $Rule::is_perm_action) && " after " . $self->perm_trigger->header )
}

sub list_results {
   my ($self, $proto)=@_;
   my $prefix=&path_prefix;
   $prefix ? map { "$prefix.$_" } $self->rule->list_results($proto) : $self->rule->list_results($proto)
}

# real object => rule result
sub execute {
   my ($self, $object)=splice @_,0,2;
   for (my $i=$self->up; $i>0; --$i) {
      defined ($object=$object->parent) or return $Rule::exec_infeasible;
   }
   if (defined $self->down) {
      foreach my $prop (@{$self->down}) {
         if (defined (my $i=$object->dictionary->{$prop->key})) {
            $object=$object->contents->[$i]->value;
         } else {
            return $Rule::exec_infeasible;
         }
      }
   }
   $self->rule->execute($object,@_);
}

# deputy object => deputy sub-object
sub navigate {
   my ($self, $object)=@_;
   for (my $i=$self->up; $i>0; --$i) {
      $object=$object->parent;
   }
   if (defined $self->down) {
      foreach my $prop (@{$self->down}) {
         $object=$object->subobj_cache->{$prop->key};
      }
   }
   $object;
}

####################################################################################
package Polymake::Core::Scheduler::MultipleChooser;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$prop' => '#1' ],
   [ '$bag' => 'undef' ],
   [ '$indices' => '#2' ],
);

sub header {
   my $self=shift;
   "multiple subobject chooser ".$self->prop->name."(".(defined($self->bag) ? join(",",@{$self->indices}) : $self->indices).")"
}
sub list_results { () }
sub preconditions { [ ] }
sub dyn_weight { undef }
sub weight { $Rule::zero_weight }
sub flags { $is_mult_chooser }
sub with_permutation { undef }
sub without_permutation { $_[0] }
sub rule { $_[0] }      # for uniform interface with Rule::Deputy

# the entire argument list looks like: (self, object, force_flag, tentative_chain, scope)
sub execute {
   my ($self, $object, $force, $tentative)=@_;
   if (defined (my $index= defined($self->bag)
                           ? first_set_intersection($self->indices, $tentative->supplier->{$self->bag})
                           : $self->indices)) {
      dbg_print( "choosing multiple subobject ", $self->prop->name, "($index)" ) if $Verbose::rules>2;
      if ($index) {
         my ($pv, $content_index);
         if (defined ($content_index=$object->dictionary->{$self->prop->key}) &&
             defined ($pv=$object->contents->[$content_index])) {
            ($_[4] ||= new Scope())->begin_locals;
            local_swap($pv->values,0,$index);
            $_[4]->end_locals;
            $Rule::exec_OK
         } else {
            $Rule::exec_infeasible
         }
      } else {
         $Rule::exec_OK
      }
   } else {
      $Rule::exec_infeasible
   }
}

sub copy4schedule {
   my ($self, $tentative)=@_;
   if (defined($self->bag)) {
      $_[0]=_new($self, $self->prop, first_set_intersection($self->indices, $tentative->supplier->{$self->bag}));
   }
}

####################################################################################
package Polymake::Core::Scheduler::DeputyObject;

use Polymake::Struct (
   [ new => '$;$$$$' ],
   [ '$real_object' => '#1' ],  # Object  undef if not yet created
   [ '$up' => '#2' ],           # how many levels upwards from root
   [ '$down' => '#3 && ( #3 ->down ? [ @{ #3 ->down }, #4 ] : [ #4 ] )' ],      # (Property)  path to the subobject
   [ '$parent' => 'weak( #3 )' ],
   [ '$property' => '#4 || #1 ->parent && #1 ->property->declared' ],
   [ '$type' => 'defined( #1 ) ? #1 ->type : #4 ->concrete( #3 )->type' ],
   [ '%rule_cache' => '@_>2 && { }' ],  # Rule => RuleDeputy

   '%rule_cache_for_perm_trigger',      # Rule => RuleDeputy created for current permutation trigger (all but the first round in gather_rules)
   [ '$valid_for_perm_trigger' => '#5' ],       # this deputy or one of its ancestors relates to a permutation subobject:
                                                # permutation trigger for which rule_cache_for_perm_trigger are valid
   '%prod_cache',               # Property->key => [ producing rules: Rule if depth==0, RuleDeputy otherwise ]
   '%subobj_cache',             # subobject Property->key => corresponding DeputyObject
   '%labels',
   '%sensitive_to',             # = real_object->sensitive_to at the begin of scheduling
                                # During phase 2 some rules creating sensitive properties might be called,
                                # but get_schedule must produce the chain according to the initial state of the object.
);
####################################################################################
sub navigate_up {
   my $self=shift;
   $self->parent ||= do {
      my $pd=new DeputyObject($self->real_object->parent, $self->up+1);
      weak($pd->subobj_cache->{$self->property->key}=$self);
      if ($self->property->flags & $Property::is_multiple) {
         $pd->prod_cache->{$self->property->key}=rule_instance($pd, new MultipleChooser($self->property, $self->real_object->parent_index));
      }
      $pd
   };
}

sub navigate_down {
   my ($self, $cur_perm_trigger, @path)=@_;
   my $prop=pop @path if wantarray;
   my $inherit_trigger=defined($self->valid_for_perm_trigger);
   foreach $prop (@path) {
      $inherit_trigger ||= ($prop->flags & $Property::is_permutation) if defined($cur_perm_trigger);
      $self=($self->subobj_cache->{$prop->key} ||= do {
         my $real_subobj;
         if (!$inherit_trigger &&
             defined($self->real_object) &&
             defined (my $ix=$self->real_object->dictionary->{$prop->key})) {
            $real_subobj=$self->real_object->contents->[$ix]->value or return;
         }
         new DeputyObject($real_subobj, $self->up, $self, $prop, $inherit_trigger ? $cur_perm_trigger : undef);
      });
      $self=$self->[0] unless is_object($self);
   }
   if ($inherit_trigger && $self->valid_for_perm_trigger != $cur_perm_trigger) {
      invalidate_caches($self, $cur_perm_trigger);
   }
   wantarray ? ($self, $prop) : $self
}

sub navigate_down_in_all_multiple_subobj {
   my ($self, $pv)=@_;
   my $prop=$pv->property;
   $self->subobj_cache->{$prop->key} ||= [ map { new DeputyObject($_, $self->up, $self, $prop) } @{$pv->values} ];
}

sub navigate_down_in_selected_multiple_subobj {
   my ($self, $input)=@_;
   foreach my $prop (@$input) {
      if ($prop->flags & $Property::is_multiple) {
         my $content_index=$self->real_object->dictionary->{$prop->key};
         my $pv=$self->real_object->contents->[$content_index];
         return ($self->prod_cache->{$prop->key}, $pv->values->[0]->parent_index);
      } else {
         $self=$self->subobj_cache->{$prop->key};
      }
   }
}

sub invalidate_caches {
   my ($self, $cur_perm_trigger)=@_;
   %{$self->prod_cache}=();
   if (keys %{$self->rule_cache}) {
      $self->rule_cache_for_perm_trigger->{$self->valid_for_perm_trigger}=$self->rule_cache;
      $self->rule_cache={ };
   }
   $self->valid_for_perm_trigger=$cur_perm_trigger;
}

sub find_mult_chooser {
   my ($self, $chain, $bag, $prop, $indices)=@_;
   foreach my $mult_chooser (@$bag) {
      return $mult_chooser if equal_lists($mult_chooser->rule->indices, $indices);
   }
   push @$bag, rule_instance($self, my $mult_chooser=new MultipleChooser($prop,$indices));
   weak($mult_chooser->bag=$bag);
   push @{$chain->ready}, $mult_chooser;
   $chain->queued_rules->{$mult_chooser}=1;
   $mult_chooser;
}

sub rule_instance {
   my ($self, $rule)=@_;
   if ($self->rule_cache) {
      $self->rule_cache->{$rule} ||= new RuleDeputy($rule, $self->up, $self->down, $self->valid_for_perm_trigger, $self->rule_cache);
   } else {
      $rule
   }
}

sub rule_instance_for_perm_trigger {
   my ($self, $rule, $perm_trigger)=@_;
   if (defined $self->valid_for_perm_trigger) {
      if ($self->valid_for_perm_trigger != $perm_trigger) {
         invalidate_caches($self, $perm_trigger);
      }
      &rule_instance;
   } else {
      my $perm_cache=($self->rule_cache_for_perm_trigger->{$perm_trigger} ||= { });
      $perm_cache->{$rule} ||= new RuleDeputy($rule, $self->up, $self->down, $perm_trigger, $perm_cache);
   }
}

sub has_sensitive_to {
   my ($self, $prop)=@_;
   return 0 unless defined $self->real_object;
   $self->sensitive_to->{$prop->key} //= $self->real_object->has_sensitive_to($prop);
}

####################################################################################
package Polymake::Core::Scheduler::TentativeRuleChain;

sub CoWconsumer($) {
   if (refcnt($_[0])>1) {
      $_[0]={ %{$_[0]} }
   } else {
      $_[0]
   }
}

sub CoWsupplier($) {
   if (refcnt($_[0])>1) {
      $_[0]=[ map { ({ %$_ }) } @{$_[0]} ]
   } else {
      $_[0]
   }
}

sub CoWpending_perms($) {
   if (refcnt($_[0])>1) {
      my %copy;
      while (my ($perm, $list)=each %{$_[0]}) {
         @{$copy{$perm}}{keys %$list}=();
      }
      $_[0]=\%copy;
   } else {
      $_[0]
   }
}

sub CoWbag($) {
   if (refcnt($_[0])>1) {
      $_[0]=[ @{$_[0]} ];
   } else {
      $_[0]
   }
}
####################################################################################
#  eliminates given rules, as well as rules becoming infeasible (without supplier)
#  or useless (without consumer) due to this elimination

my $sizeof=sizeof();

sub eliminate_rules {           # Rule, ... =>
   my $self=shift;
   my @elim=@_ or return 1;
   my $init= @$self > $sizeof;
   my $verbose= $init && $Verbose::scheduler>=2;
   my (%marked, @queue, $rule, $supp_group, $supp_rule, $second_round);

   do {
      @marked{@elim}=();

      while (defined ($rule=shift @elim)) {

         unless ($init) {
            # if all consumers of a scheduled rule have gone, this variant has no sense more.
            foreach my $sched_rule (@{$self->rules}) {
               my $sched_cons=CoWconsumer($self->consumer->{$sched_rule});
               return if delete $sched_cons->{$rule}  and  !keys %{$sched_cons};
            }
         }

         if ($rule->flags & $Rule::is_perm_action && defined (my $list=$self->pending_perms->{$rule->permutation})) {
            return if exists $list->{$rule};
         }

         # some rules depending on the one being eliminated may become infeasible
         if (defined (my $cons=delete $self->consumer->{$rule})) {
            foreach my $cons_rule (keys %$cons) {
               next if exists $marked{$cons_rule};
               # each set of suppliers must contain at least one feasible rule
               my $supp_list=CoWsupplier($self->supplier->{$cons_rule}) or next;
               for (my ($i, $last)=(0, $#$supp_list); $i<=$last; ++$i) {
                  $supp_group=$supp_list->[$i];
                  if (delete $supp_group->{$rule}) {
                     if (! keys %$supp_group) {
                        if ($rule->flags == $Rule::is_initial) {
                           splice @$supp_list, $i--, 1;
                           --$last;
                        } else {
                           dbg_print( "  discarding ", $cons_rule->header, ": no more supplier" )
                              if $verbose;
                           return if $cons_rule == $self->final;   # request become infeasible
                           push @elim, $cons_rule;
                           $marked{$cons_rule}=1;
                           last;
                        }
                     }
                  }
               }
            }
         }

         # some supplier of the current rule may become useless
         if (my $supp_list=delete $self->supplier->{$rule}) {
            foreach $supp_group (@$supp_list) {
               foreach $supp_rule (keys %$supp_group) {
                  next if exists $marked{$supp_rule};
                  my $cons=CoWconsumer($self->consumer->{$supp_rule});
                  delete $cons->{$rule};
                  if (! keys %$cons) {
                     dbg_print( "  discarding ", $supp_rule->header, ": no more consumer" )
                        if $verbose;
                     delete $self->consumer->{$supp_rule};
                     push @elim, $supp_rule;
                     $marked{$supp_rule}=1;
                  }
               }
            }
         }

         delete $self->preferred->{$rule} if $init;
      }

      # Check the remaining rules to be (reversely) accessible from the final rule (kind of garbage collecting):
      # eliminate the dead loops.

      return 1 if $second_round++ || !@{$self->supplier->{$self->final}};

      %marked=($self->final => 1);
      @queue=($self->final);
      while (defined ($rule=shift @queue)) {
         foreach $supp_group (@{$self->supplier->{$rule}}) {
            foreach $supp_rule (keys %$supp_group) {
               unless ($marked{$supp_rule}++) {
                  push @queue, $supp_rule if $self->supplier->{$supp_rule};
               }
            }
         }
      }
      while (($rule, $supp_group)=each %{$self->supplier}) {
         if (!$marked{$rule} && is_ARRAY($supp_group) && is_object($rule)) {
            push @elim, $rule;
            dbg_print( "  discarding ", $rule->header, ": no path to request" )
               if $verbose;
         }
      }

      %marked=();
   } while (@elim);
   1
}
####################################################################################
sub add_rule {          # Rule =>
   my ($self, $rule, $check_if_run, @perm_actions)=@_;

   # check the consumers - some of them might become ready
   my (@ready, @to_elim, $i, $last, $for_perm_action);
   if (@perm_actions) {
      # eliminate the "variant without permutation" anyway
      push @to_elim, $perm_actions[0]->perm_trigger;
   }
   do {
      foreach my $cons_rule (keys %{$self->consumer->{$rule}}) {
         my $supp_list=CoWsupplier($self->supplier->{$cons_rule});
         if ($Application::plausibility_checks && !ref($supp_list)) {
           die "internal error: ready or failed rule ", $cons_rule->header, " spuriously re-appeared as consumer of ", $rule->header, "\n";
        }
         my %supplier;
         for (($i,$last)=(0,$#$supp_list); $i<=$last; ) {
            my $supp_group=$supp_list->[$i];
            if (delete $supp_group->{$rule}) {
               $supplier{$_} |= 1 for keys %$supp_group;
               if ($for_perm_action) {
                  # the back transformation is not ready now, but all other suppliers can be already removed
                  %$supp_group=($rule => 1);
                  ++$i;
               } else {
                  splice @$supp_list, $i, 1;  --$last;
               }
               next;
            }
            $supplier{$_} |= 2 for keys %$supp_group;
            ++$i;
         }

         push @ready, $cons_rule if $last<0 && !($check_if_run && exists $self->run->{$cons_rule});

         while (my ($supp_rule, $code)=each %supplier) {
            if ($code==1) {
               my $cons=CoWconsumer($self->consumer->{$supp_rule});
               delete $cons->{$cons_rule};
               if (keys(%$cons) < ($supp_rule->flags & $Rule::is_perm_action ? 2 : 1)) {
                  # found a superfluous supplier?
                  push @to_elim, $supp_rule;
               }
            }
         }
      }
      $for_perm_action=1;
   } while (defined ($rule=shift @perm_actions));

   eliminate_rules($self, @to_elim) && do {
      # some of "ready" rules may have become superfluous and been eliminated right now
      push @{$self->ready}, grep { exists $self->consumer->{$_} } @ready;
      1
   }
}
####################################################################################
sub add_ready_rule {
   my ($self, $rule)=@_;
   $self->supplier->{$rule}=0;

   if ($rule->flags & $is_mult_chooser) {
      if (defined(my $bag=$rule->rule->bag)) {
         my $add_me=1;
         my $chooser_mask=CoWbag($self->supplier->{$bag});
         set_intersection($chooser_mask, $rule->rule->indices, 0)
            or die "Scheduler: internal error: multiple subobject chooser conflicts with the current mask\n";
         eliminate_rules($self, grep { $_!=$rule and
                                       is_string($self->supplier->{$_})
                                       ? ($add_me=0)
                                       : !defined(first_set_intersection($chooser_mask, $_->rule->indices))
                                } @$bag)
            or return;
         push @{$self->rules}, $rule if $add_me;
      } else {
         push @{$self->rules}, $rule;
      }

   } else {
      $self->weight += $rule->weight;
      if (defined (my $dwr=$rule->dyn_weight)) {
         if (defined (my $wt=$self->dyn_weight->{$dwr})) {
            $self->weight += $wt;
         }
      }
      my $pref=$self->preferred->{$rule};
      if (defined($pref) && @$pref) {
         $self->weight->add_atom($Rule::Weight::max_major, 1);  # preference violation penalty
      }
      push @{$self->rules}, $rule;
   }

   &add_rule;
}
####################################################################################
sub eliminate_mult_chooser {
   my ($self, $rule)=@_;
   my $bag=$rule->rule->bag;
   my $chooser_mask=CoWbag($self->supplier->{$bag});
   set_intersection($chooser_mask, $rule->rule->indices, 1) &&
   eliminate_rules($self, $rule, grep { $_!=$rule && is_ARRAY($self->supplier->{$_}) &&
                                        !defined(first_set_intersection($chooser_mask, $_->rule->indices)) } @$bag);
}
####################################################################################
# private:
# in: pending -> [ Rule ] : preconditions ready to evaluate
# out: pending -> [ Rule ] : postponed preconditions (their suppliers were too expensive)
#                            A production rule is treated as expensive if its weight is of same category as Rule::std_height or heavier
#      retval = ( Rule ) : preconditions to be evaluated now
#      suppliers -> [ Rule ] : their (cheap) suppliers

sub filter_cheap_suppliers {
   my ($self, $pending, $suppliers, $expensive)=@_;
   my %depends=map { $_ => [ $_ ] } @$pending;

   # visit in reverse order, since the consumer are always executed later than their supplier
   foreach my $rule (reverse @{$self->rules}) {
      if ($rule->flags & $Rule::is_any_precondition ||
          (!exists $self->run->{$rule} && $rule->weight->[0] >= $expensive) ||
          grep { !exists $self->run->{$_} } @{$rule->preconditions}) {
         # the supplier rule is too expensive to be executed speculatively
         # or depends itself on unchecked preconditions
         foreach (keys %{$self->consumer->{$rule}}) {
            if (defined (my $deps=$depends{$_})) {
               delete @depends{ @$deps };
            } elsif ($_->flags & $Rule::is_perm_action) {
               foreach (keys %{$self->consumer->{$_}}) {
                  if (defined (my $deps=$depends{$_})) {
                     delete @depends{ @$deps };
                  }
               }
            }
         }
      } else {
         foreach (keys %{$self->consumer->{$rule}}) {
            if (defined (my $deps=$depends{$_})) {
               push @{ $depends{$rule} ||= [ $rule ] }, @$deps;
            }
         }
      }
   }
   my @result;
   if (keys %depends) {
      for (my ($i,$last)=(0,$#$pending); $i<=$last; ) {
         if (exists $depends{$pending->[$i]}) {
            push @result, splice @$pending, $i, 1;
            --$last;
         } else {
            ++$i;
         }
      }
      @$suppliers=grep { exists $depends{$_} && !exists $self->run->{$_} } @{$self->rules};
   }
   @result;
}
####################################################################################
# private:
sub weave_preconditions {
   my $self=shift;
   for (my $i=$#{$self->rules}; $i>=0; --$i) {
      if (my @pending=grep { !exists $self->run->{$_} } @{$self->rules->[$i]->preconditions}) {
         splice @{$self->rules}, $i, 0, @pending;
      }
   }
}
####################################################################################
sub execute {           # => number of the failed rule
   my ($self, $object)=@_;
   my $chain_scope;
   my ($i,$force_after)=(0,-1);

   for (;;) {
      foreach my $rule (@{$self->rules}) {
         my ($rc, $retval)=$rule->execute($object, $i>$force_after, $self, $chain_scope);
         if ($rule->flags & $Rule::is_multi_precondition
               and
             defined($retval)       # if the precondition rule died, we don't want to retry it on the sibling subobjects
               and
             ref(my $rc_array=$self->run->{$rule})) {
            my ($chooser_bag, $subobj)=$rule->navigate($self->deputy_root)->navigate_down_in_selected_multiple_subobj(grep { get_array_flags($_) & $Property::is_multiple } @{$rule->input});
            $rc_array->[$subobj->parent_index]=$rc;
            if ($rc != $Rule::exec_OK) {
               undef $chain_scope;
               if (set_remove($self->supplier->{$chooser_bag}, $subobj->parent_index)) {
                  $force_after=$i; $i=0;
                  last;
               } else {
                  return $i;
               }
            }
         } else {
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
         }
         ++$i;
      }
      return undef;
   }
}
####################################################################################
sub report {
   my ($self)=@_;
   return @{$self->rules}
          ? ( (map { ("  ", $_->header, "\n") } @{$self->rules}),
              "  Sum weight=", $self->weight )
          : ("nothing to do");
}
####################################################################################
package Polymake::Core::Scheduler::InitRuleChain;

use Polymake::Struct (
   [ new => '$$' ],
   [ '@ISA' => 'TentativeRuleChain' ],
   [ '$final' => '#2' ],
   [ '$deputy_root' => 'undef' ],
   '%queued_rules',             # Rule => 1 : suppliers computed, rule pushed in the @rules queue
   '%depending_on_perms',       # Rule => Rule => 1 if both rules trigger permutations identical or induced (either way round)
   [ '$cur_perm_trigger' => 'undef' ],  # Rule triggering a permutation: during the gather phase for this permutation's subtree
   [ '$Tstart' => 'undef' ],
);

sub new {
   my $self=&_new;
   my ($object)=@_;
   if ($Verbose::scheduler) {
      dbg_print( "gathering viable rules" );
      $self->Tstart=[ gettimeofday() ];
      if ($Verbose::scheduler>=3) {
         require Polymake::Core::Scheduler_debug;
         $self->debug=new Debug();
      }
   }
   $self->deputy_root=new DeputyObject($object);
   $self->consumer->{$self->final}={ };
   push @{$self->rules}, $self->deputy_root, $self->final;
   $self;
}
####################################################################################
my $final_dummy=create Rule('final', []);

# convenience function
# Object, [ Rule ] => success code
sub create_and_resolve {
   my ($pkg, $object, $rules)=@_;
   my $self=new($pkg, $object, $final_dummy);
   $self->supplier->{$final_dummy}=[ { map { push @{$self->rules}, $self->deputy_root, $_;
                                             $self->consumer->{$_}->{$final_dummy}=1;
                                             ($_ => 1) } @$rules } ];
   gather_rules($self) && resolve($self, $object)
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
      $self->queued_rules->{$rule} ||= do {
         $self->supplier->{$rule} ||= [ ];
         push @{$self->rules}, $object, $rule;
         if (defined $rule->with_permutation) {
            # Don't process the "variant with permutation" separately;
            # prevent it from popping off the ready queue in the 2nd phase.
            $self->queued_rules->{$rule->with_permutation}=1;
            $self->supplier->{$rule->with_permutation}=[ { $rule => 1 }, { } ];
            $self->consumer->{$rule}->{$rule->with_permutation}=1;
            foreach my $action (@{$rule->with_permutation->actions}) {
               $self->queued_rules->{$action}=1;
               # will be filled further after this grand round of gather_rules
               $self->supplier->{$action}=[ { $rule->with_permutation => 1 } ];
               $self->consumer->{$rule->with_permutation}->{$action}=1;
               $self->consumer->{$action}={ };
            }
         }
         1
      };
   }
}
####################################################################################
sub producers {
   my ($self, $object, $prop)=@_;
   ($object, $prop)=$object->navigate_down($self->cur_perm_trigger, @$prop) unless is_object($prop);
   $object->prod_cache->{$prop->key} ||= do {
      my (@rules, @list, $mult_chooser_passed);
      if (@rules=$viable_rules->($self, $object, $object->type->get_producers_of($prop))) {
         consider_rules($self, $object, @rules);
         push @list, @rules;
      }
      my $this_level=$object;
      my $prod_key=$prop->key;
      my $lookup_for_perm_trigger;

      while (defined($this_level->parent) || defined($this_level->real_object->parent)  and
             defined($prod_key=$prod_key->{$this_level->property->key})) {

         $lookup_for_perm_trigger ||= defined($self->cur_perm_trigger) && $this_level->property->flags & $Property::is_permutation;
         my $ancestor=$this_level->navigate_up;
         my $ancestor_rules=$ancestor->type->get_producers_of($prod_key);

         if (@$ancestor_rules  and
             @rules = $lookup_for_perm_trigger
                      ? (map { $ancestor->rule_instance_for_perm_trigger($_, $self->cur_perm_trigger) } @$ancestor_rules)
                      : $viable_rules->($self, $ancestor, $ancestor_rules)) {

            consider_rules($self, $ancestor, @rules);
            push @list, @rules;
            if ($this_level->property->flags & $Property::is_multiple
                  and
                defined($this_level->real_object)
                  and
                $this_level->real_object->parent_index != 0) {
               my $mult_chooser=$ancestor->prod_cache->{$this_level->property->key};
               if (is_object($mult_chooser)) {
                  # only this instance of the multiple subobject is interesting
                  if (defined $mult_chooser_passed) {
                     add_chooser_as_supplier($self,$mult_chooser_passed,$mult_chooser);
                     $self->queued_rules->{$mult_chooser_passed} ||= do {
                        push @{$self->ready}, $mult_chooser_passed;  1
                     }
                  }
                  $mult_chooser_passed=$mult_chooser;
               }
            }
            if (defined $mult_chooser_passed) {
               add_chooser_as_supplier($self,$mult_chooser_passed,@rules);
               $self->queued_rules->{$mult_chooser_passed} ||= do {
                  push @{$self->ready}, $mult_chooser_passed;  1
               }
            }
         }
         $this_level=$ancestor;
      }
      \@list
   };
}
####################################################################################
sub add_chooser_as_supplier {
   my ($self, $chooser)=splice @_, 0, 2;
   foreach my $rule (@_) {
      $self->consumer->{$chooser}->{$rule} ||= do {
         my $add_new_group=!defined($chooser->rule->bag);
         unless ($add_new_group) {
            $add_new_group=1;
            foreach my $supp_group (@{$self->supplier->{$rule}}) {
               my ($other_supp)=keys %$supp_group;
               if ($other_supp->flags & $is_mult_chooser and $other_supp->rule->bag==$chooser->rule->bag) {
                  $supp_group->{$chooser}=1;
                  $add_new_group=0;
                  last;
               }
            }
         }
         if ($add_new_group) {
            push @{$self->supplier->{$rule}}, { $chooser => 1 };
         }
         1
      };
   }
}
####################################################################################
sub descend_thru_multiple_subobject {
   my ($self, $rule, $path, $object)=@_;
   my $real_obj=$object->real_object;
   my $this_level=$object;
   my $depth=0;
   my ($pv, $content_index);
   foreach my $prop (@$path) {
      if (defined ($content_index=$real_obj->dictionary->{$prop->key}) &&
          defined ($pv=$real_obj->contents->[$content_index])) {
         if ($prop->flags & $Property::is_multiple and @{$pv->values}>1) {
            $this_level=$this_level->navigate_down($self->cur_perm_trigger, @$path[0..$depth-1]) if $depth;
            my $mult_choosers=($this_level->prod_cache->{$prop->key} ||= [ ]);
            if (is_object($mult_choosers)) {
               # triggered by navigating upwards
               defined($real_obj=$pv->values->[$mult_choosers->rule->indices]) or return;
               ++$depth;
            } else {
               my $rc= $rule->flags & $Rule::is_multi_precondition ? ($self->run->{$rule} //= [ ]) : undef;
               my (@positive, @negative, $impossible);
               local_shorten($path, -$depth-1);
               foreach my $mult_level (@{$object->navigate_down_in_all_multiple_subobj($pv)}) {
                  my $rc_here;
                  if (defined($rc) && defined ($rc_here= ref($rc) ? $rc->[$mult_level->real_object->parent_index] : $rc)) {
                     undef $rc_here if $rc_here != $Rule::exec_OK;
                  } else {
                     $rc_here=descend_thru_multiple_subobject($self, $rule, $path, $mult_level);
                  }
                  if ($rc_here) {
                     push @positive, $mult_level->real_object->parent_index;
                  } elsif (defined $rc_here) {
                     push @negative, $mult_level->real_object->parent_index;
                  } else {
                     $impossible=1;
                  }
               }
               if ($impossible) {
                  return undef unless @positive || @negative;
               } else {
                  return 1 unless @negative;
                  return 0 unless @positive;
               }
               if (@_<6) {
                  die "cascaded multiple objects in a single request path are not supported\n";
               }
               unless (@$mult_choosers) {
                  # set the common mask
                  $self->supplier->{$mult_choosers}=[ $impossible ? (sort { $a <=> $b } @positive, @negative) : 0..$#{$pv->values} ];
               }
               if (@positive) {
                  $_[4]=$object->find_mult_chooser($self, $mult_choosers, $prop, \@positive);
               }
               if (@negative) {
                  $_[5]=$object->find_mult_chooser($self, $mult_choosers, $prop, \@negative);
               }
               return 0;
            }
         } else {
            if (++$depth==@$path) {
               $real_obj=$pv->value;
            } else {
               defined($real_obj=$pv->value) or return;
            }
         }
      } else {
         return 0;
      }
   }
   ($rule->flags & $Rule::is_definedness_check) || defined($real_obj) || undef;
}
####################################################################################
sub eval_input_list {
   my ($self, $rule, $input_list, $object)=@_;
   if (my $flags=get_array_flags($input_list)) {
      ($flags & $Property::is_multiple) and do {
         # descending via multiple subobject: need an especially copious procedure here
         my $rc;
         foreach my $path (@$input_list) {
            my $rc_here=descend_thru_multiple_subobject($self,$rule,$path,$object,@_[4,5]) and return 1;
            $rc //= $rc_here;
         }
         $rc
      }
      # else tried to descend via permutation subobject: it never exists
   } else {
      $object->real_object->eval_input_list($input_list, $rule->flags & $Rule::is_definedness_check);
   }
}
####################################################################################
# private:
#  => undef if applicable, 'text' if infeasible
sub rule_status {
   my ($self, $object, $rule)=@_;
   $self->supplier->{$rule} ||= [ ];

   foreach my $input_list (@{$rule->input}) {
      my ($positive_chooser, $negative_chooser);
      unless (my $s=defined($object->real_object) &&
                    eval_input_list($self, $rule, $input_list, $object, $positive_chooser, $negative_chooser)) {
         defined($s) or return $Verbose::scheduler>=2 && "undefined input properties " . Rule::print_input_list($input_list);
         if (defined($positive_chooser) && !defined($negative_chooser)) {
            # some instances of the multiple subobject have been evaluated as 'undef'
            add_chooser_as_supplier($self, $positive_chooser, $rule);
            next;
         }

         my (%supp_group, @all_group);
         foreach my $input (@$input_list) {
            foreach my $supp_rule (@{producers($self, $object, $input)}) {
               if ($supp_rule->flags == $Rule::is_initial) {
                  push @all_group, $supp_rule;
               } elsif (not(defined($self->cur_perm_trigger)  &&
                            (# the "variant without permutation" can't serve as a supplier as we are building the permutation subtree here
                             $supp_rule==$self->cur_perm_trigger
                               or
                             # prevent a loop where a rule working on a permutation subobject tries to compute something using the
                             # property from the main object - but this property only moves back to the main object after the
                             # the processing of the permutation is finished
                             $supp_rule->flags & $Rule::is_perm_action                   &&
                             !(get_array_flags($input_list) & $Property::is_permutation) &&
                             $rule->perm_trigger == $supp_rule->perm_trigger             &&
                             grep { $_==$supp_rule->permutation }
                             map { @{$_->[0]} }
                             grep { get_array_flags($_) & $Property::is_permutation && @$_==1 } @{$rule->input}))) {
                  $supp_group{$supp_rule}=1;
               }
            }
         }

         if (@all_group) {
            push @{$self->supplier->{$rule}}, map { { ($_ => 1) } } @all_group;
            $self->consumer->{$_}->{$rule}=1 for @all_group;

         } elsif (keys %supp_group or defined($positive_chooser)) {
            push @{$self->supplier->{$rule}}, \%supp_group;
            $self->consumer->{$_}->{$rule}=1 for keys %supp_group;
            if (defined (my $perm_deputy=$rule->with_permutation)) {
               push @{$self->supplier->{$perm_deputy}}, { %supp_group };
               $self->consumer->{$_}->{$perm_deputy}=1 for keys %supp_group;
            }
            if (keys %supp_group and defined($negative_chooser)) {
               add_chooser_as_supplier($self, $negative_chooser, keys %supp_group);
            }
            if (defined($positive_chooser)) {
               $self->queued_rules->{$positive_chooser} ||= do {
                  push @{$self->ready}, $positive_chooser;  1
               };
               $supp_group{$positive_chooser}=1;
               $self->consumer->{$positive_chooser}->{$rule}=1;
            }

         } elsif ($rule->header eq "initial commit") {
            if ($Verbose::scheduler>=2) {
               dbg_print( "  skipped: no available rules to produce " . Rule::print_input_list($input_list) );
            }
         } else {
            return $Verbose::scheduler>=2 && "no available rules to produce " . Rule::print_input_list($input_list);
         }
      }
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
         @next=$viable_rules->($self,$object,$bag);
         push @{$self->preferred->{$_}}, @pref for @next;
         push @pref, @next;
      }
   }
}
####################################################################################
# private:
sub gather_permutation_rules
{
   my ($self, $object, $rule)=@_;
   $self->cur_perm_trigger=$rule;
   my $perm_deputy= $object->rule_cache ? $rule->with_permutation->rule : $rule->with_permutation;
   my $prod_rule=$perm_deputy->rule;
   my $basis_perm_subobj=$object->navigate_down($rule, $perm_deputy->permutation);
   my ($perm_obj, $prop, @discard_actions);
   my $action_index=0;
   foreach my $action (@{$perm_deputy->actions}) {
      my $obj_for_perm_rules;
      my $out_path=$prod_rule->output->[$action->output];
      if ($action->depth) {
         $perm_obj=$object->navigate_down($rule, @$out_path[0..$action->depth-1], $action->sub_permutation);
         my $sub_permutation_parent=$perm_obj->parent;
         local_shorten($out_path, -$action->depth);
         ($perm_obj, $prop)=$perm_obj->navigate_down($rule, @$out_path);
         unless (defined($sub_permutation_parent->real_object) &&
                 $sub_permutation_parent->real_object->eval_input_list([ $out_path ], 1) &&
                 # enforce execution for pseudo-rule created in Object::copy_permuted
                 defined($prod_rule->code)) {
            $obj_for_perm_rules=$sub_permutation_parent;
         }
      } else {
         unless (defined($object->real_object) &&
                 $object->real_object->eval_input_list([ $out_path ], 1) &&
                 # enforce execution for pseudo-rule created in Object::copy_permuted
                 defined($prod_rule->code)) {
            $obj_for_perm_rules=$object;
         }
         if (is_object($out_path)) {
            ($perm_obj, $prop)=($basis_perm_subobj, $out_path);
         } else {
            ($perm_obj, $prop)=$basis_perm_subobj->navigate_down($rule, @$out_path);
         }
      }
      $perm_obj->prod_cache->{$prop->key}=[ $rule->with_permutation ];

      my $action_deputy=$rule->with_permutation->actions->[$action_index];
      if (defined($obj_for_perm_rules)) {
         # If this output does not exist, it must be reconstructed from the permuted subobject even it was not asked for by other rules.
         my %supp_group;
         foreach my $restoring_rule (map { $obj_for_perm_rules->rule_instance_for_perm_trigger($_,$rule) } @{$action->producers}) {
            $self->queued_rules->{$restoring_rule} ||= do {
               push @{$self->rules}, $obj_for_perm_rules, $restoring_rule;
               1
            };
            $supp_group{$restoring_rule}=1;
            $self->consumer->{$restoring_rule}->{$action_deputy}=1;
         }
         push @{$self->supplier->{$action_deputy}}, \%supp_group;
         $self->consumer->{$action_deputy}->{$rule->with_permutation}=1;
         $self->supplier->{$rule->with_permutation}->[1]->{$action_deputy}=1;
         dbg_print( $action->enabled ? "  applicable: " : "  impossible: ", $action_deputy->header ) if $Verbose::scheduler>=2;
      } else {
         push @discard_actions, $action_deputy;
         dbg_print( "  not needed: ", $action_deputy->header ) if $Verbose::scheduler>=2;
      }
      ++$action_index;
   }

   if ($action_index==@discard_actions) {
      dbg_print( "  not needed: ", $rule->with_permutation->header) if $Verbose::scheduler>=2;
      (@discard_actions, $rule->with_permutation)
   } else {
      @discard_actions
   }
}
####################################################################################
# protected:
sub gather_rules {
   my ($self, @finalize_perms)=@_;
   my (@infeasible, %labels);

   # breadth-first search in the implicit graph of object states
   for (;;) {
    RULE:
      while (my ($object, $rule)=splice @{$self->rules}, 0, 2) {

         foreach my $precond_rule (@{$rule->preconditions}) {
            if (defined($object->real_object) &&
                $object->real_object->failed_rules->{ $object->rule_cache ? $precond_rule->rule : $precond_rule }) {
               dbg_print( "  infeasible: ", $rule->header, ": failed ", $precond_rule->header, " (tested earlier)" )
                  if $Verbose::scheduler>=2;
               push @infeasible, $rule;
               next RULE;
            }

            if (defined (my $explain=rule_status($self, $object, $precond_rule))) {
               dbg_print( "  infeasible: ", $precond_rule->header, ": $explain\n",
                          "  infeasible: ", $rule->header, " due to failed precondition above" )
                  if $explain;
               push @infeasible, $rule, $precond_rule;
               $self->run->{$precond_rule}=$Rule::exec_infeasible;
               $self->run->{$rule}=$Rule::exec_infeasible;
               next RULE;
            }

            if (! @{$self->supplier->{$precond_rule}}) {
               dbg_print( "  ready to evaluate: ", $precond_rule->header ) if $Verbose::scheduler>=2;
               my ($rc, $retval)=($object->rule_cache ? $precond_rule->rule : $precond_rule)->execute($object->real_object);
               if (($self->run->{$precond_rule}=$rc) != $Rule::exec_OK) {
                  if ($@) {
                     chomp $@;
                     warn_print( "precondition ", $precond_rule->header, " failed: $@" );
                     undef $@;
                  }
                  dbg_print( "  infeasible: ", $rule->header, " due to failed precondition above" )
                     if $Verbose::scheduler>=2;
                  push @infeasible, $rule;
                  $self->run->{$rule}=$Rule::exec_infeasible;
                  next RULE;
               }
               $self->dyn_weight->{$precond_rule}=$retval if $precond_rule->flags & $Rule::is_dyn_weight;
            } else {
               # the precondition rule must be evaluated later, let it become an exclusive supplier of the current production rule
               $self->consumer->{$precond_rule}->{$rule}=1;
               push @{$self->supplier->{$rule}}, { $precond_rule => 1 };
            }
         }

         if (defined (my $explain=rule_status($self, $object, $rule))) {
            dbg_print( "  infeasible: ", $rule->header, ": $explain" ) if $explain;
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

         } elsif (@{$self->supplier->{$rule}}) {
            dbg_print( "  applicable: ", $rule->header ) if $Verbose::scheduler>=2;

         } else {
            # rule can be immediately applied
            push @{$self->ready}, $rule;
            dbg_print( "  ready to use: ", $rule->header ) if $Verbose::scheduler>=2;
         }

         if (defined $rule->with_permutation) {
            push @finalize_perms, $object, $rule;
         }
         foreach my $label (@{$rule->labels}) {
            store_preferred_rules($self, $label->wildcard_name, $object);
         }
      }

      if (@finalize_perms) {
         push @infeasible, gather_permutation_rules($self, splice @finalize_perms, 0, 2);
      } else {
         last;
      }
   }

   undef $self->cur_perm_trigger;
   eliminate_rules($self, @infeasible) or return;
   squeeze_prefs($self);
   $self->dump if $Verbose::scheduler>=2 && $DebugLevel>=3;
   1
}
####################################################################################
sub squeeze_prefs {
   my ($self)=@_;
   while (my ($rule, $list)=each %{$self->preferred}) {
      next if !exists $self->supplier->{$rule}; # not relevant
      my $r=0;
      while ($r<=$#$list) {
         my $pref_rule=$list->[$r];
         if (!exists $self->supplier->{$pref_rule}      # eliminated
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
sub constrain_to_preferred {
   my ($self)=@_;
   my @elim;
   foreach my $supp_group (@{$self->supplier->{$self->final}}) {
      foreach my $supp_rule (keys %$supp_group) {
         my $pref=$self->preferred->{$supp_rule};
         push @elim, $supp_rule if !defined($pref) || @$pref;
         if ($Verbose::scheduler>=2  and  !defined($pref) || @$pref) {
            dbg_print( "  excluding ", $supp_rule->header );
         }
      }
   }
   eliminate_rules($self, @elim)
   or die "no available preferred rules left over\n";
}
####################################################################################
sub find_relative_path {
   my ($object, $r1, $r2, $report_equality)=@_;
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
         return if !$diff && !$report_equality;
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
      if (!$diff) {
         return $report_equality ? (0) : ();
      }
   }

   ($up1,$up2)=($up2,$up1) if $diff<0;
   while ($up2>0) { $object=$object->parent; --$up2; --$up1; }
   while ($up1>0) { unshift @path, $object->property; $object=$object->parent; --$up1; }

   ($diff, @path);
}

sub creating_related_permutation {
   my ($self, $rule)=splice @_,0,2;
   my $permutation=$rule->with_permutation->permutation;
   foreach my $prev_rule (@_) {
      return 1 if $prev_rule->with_permutation->permutation == $permutation;
      if (my $dep=($self->depending_on_perms->{$rule}->{$prev_rule} //= do {
            my $result=0;
            if (my ($rel_pos, @path)=find_relative_path($self->deputy_root, $rule, $prev_rule)) {
               if ($rel_pos<0) {
                  # must ascend from $rule's object to $prev_rule's object
                  $result= $prev_rule->with_permutation->permutation->find_sub_permutation(@path)==$permutation;
               } else {
                  # must descend from $rule's object to $prev_rule's object
                  $result= $permutation->find_sub_permutation(@path)==$prev_rule->with_permutation->permutation;
               }
            }
            $self->depending_on_perms->{$prev_rule}->{$rule}=$result
         })) {
         return 1;
      }
   }
   0
}

sub add_ready_rule_with_permutation {
   my ($self, $var, $rule, $give_schedule)=@_;
   my $subobj=$rule->navigate($self->deputy_root);
   if (exists $self->supplier->{$rule->with_permutation}) {
      if ($subobj->has_sensitive_to($rule->with_permutation->permutation)
          || creating_related_permutation($self, $rule, grep { defined $_->with_permutation } @{$var->rules})) {

         # must create permutation subobject: check whether its further processing is still possible
         exists $var->supplier->{$rule->with_permutation} or return;

         if (my @actions_alive=grep { $_->enabled && exists $var->supplier->{$_} } @{$rule->with_permutation->actions}) {
            $var->supplier->{$rule->with_permutation}=[ ];
            delete CoWconsumer($var->consumer->{$rule})->{$rule->with_permutation};
            my %pending;
            @pending{@actions_alive}=();
            CoWpending_perms($var->pending_perms)->{$rule->with_permutation->permutation}=\%pending;
            # all properties produced for the permutation subobject and lacking in the basis object
            # must be transformed back
            my $final_supp=CoWsupplier($var->supplier->{$self->final});
            foreach my $action (@actions_alive) {
               push @$final_supp, { $action => 1 };
               my $cons=CoWconsumer($var->consumer->{$action});
               delete $cons->{$rule->with_permutation};
               $cons->{$self->final}=1;
            }
            return add_ready_rule($var, $rule->with_permutation, 0, @actions_alive);
         } else {
            eliminate_rules($var, $rule->with_permutation) or return;
         }
      } elsif ($give_schedule && defined($subobj->real_object)) {
         my $checking=$rule->checking_precondition;
         push @{$var->rules}, $checking;
         # prevent $rule from being treated as "too expensive" - don't store the consumer relation
         $var->consumer->{$checking}={ };
      }
   }
   # "variant with permutation" not needed
   add_ready_rule($var, $rule);
}
####################################################################################
# private:
# ( Rule ) executed during resolving procedure => ( Rule ) complete schedule
# FinalVar->rules contains at return only pending preconditions and their suppliers

sub prepare_schedule {
   my ($self)=@_;
   my ($rule, @result, %keep4precond);

   # mark preconditions and their suppliers
   foreach $rule (@{$self->rules}) {
      unless ($rule->flags & $Rule::is_perm_action) {
         @keep4precond{ @{$rule->preconditions} }=();
      }
   }

   foreach $rule (reverse @{$self->rules}) {
      foreach (keys %{$self->consumer->{$rule}}) {
         $keep4precond{$rule}=1, last if exists $keep4precond{$_};
      }
   }

   for (my $i=0; $i<=$#{$self->rules}; ) {
      $rule=$self->rules->[$i];
      if ($rule->flags & $Rule::is_perm_action) {
         splice @{$self->rules}, $i, 1;
      } else {
         push @result, @{$rule->preconditions}, $rule;
         my @pending=grep { !exists $self->run->{$_} } @{$rule->preconditions};
         my $keep=exists $keep4precond{$rule};
         splice @{$self->rules}, $i, !$keep, @pending;
         $i+=@pending+$keep;
      }
   }

   @result;
}
####################################################################################
#
# return value:
# => 1 if some rule chain successfully executed (or there was nothing to do)
# => 0 if no suitable rule chain found
# => -n if n different rule chains tried, but none succeeded
# RuleChain || undef if $give_schedule

sub resolve {
   my ($self, $object, $give_schedule)=@_;
   unless (@{$self->supplier->{$self->final}}) {
      if (defined($give_schedule)) {
         return $give_schedule ? new RuleChain($object, $self) : $self;
      } else {
         dbg_print( "nothing to do" ) if $Verbose::scheduler;
         return 1;
      }
   }
   my ($rule, $i, $last, @replay_rules);
   my $tries=0;
   if (@{$self->ready}) {
      dbg_print( "composing a minimum weight rule chain" ) if $Verbose::scheduler;
      my ($maxheap, $popcnt)=(1, 0);
      my $heap=new Heap(clone($self));
    HEAP_LOOP:
      while (defined (my $top=$heap->pop)) {
         $popcnt += $top != $self;
         my @blocked;

         if (keys %{$top->pending_perms}) {
            my $pending_perms;
            for (($i,$last)=(0,$#{$top->ready}); $i<=$last; ) {
               $rule=$top->ready->[$i];
               if ($rule->flags & $Rule::is_perm_action) {
                  add_ready_rule($top, $rule) or next HEAP_LOOP;
                  $pending_perms ||= CoWpending_perms($top->pending_perms);
                  my $perm=$rule->permutation;
                  my $list=$pending_perms->{$perm};
                  delete $list->{$rule};
                  delete $pending_perms->{$perm} unless keys %$list;
                  splice @{$top->ready}, $i, 1;  --$last;
               } else {
                  ++$i;
               }
            }
            if (keys %{$pending_perms ||= $top->pending_perms}) {
               for (($i,$last)=(0,$#{$top->ready}); $i<=$last; ) {
                  $rule=$top->ready->[$i];
                  if (defined($rule->with_permutation) && exists $pending_perms->{$rule->with_permutation->permutation}) {
                     push @blocked, $rule;
                     splice @{$top->ready}, $i, 1;  --$last;
                  } else {
                     ++$i;
                  }
               }
            }
         }

         if (@{$top->supplier->{$self->final}}) {
            # final target not resolved yet

            # If there are ready-to-evaluate pending preconditions, process them now, don't split off new variants on them.
            # Also separate initial rules, add them in one batch too.
            my (@pending, @satisfied, @initial);
            for (($i,$last)=(0,$#{$top->ready}); $i<=$last; ) {
               $rule=$top->ready->[$i];
               if (($rule->flags & $Rule::is_any_precondition) == $Rule::is_precondition) {
                  if (exists $self->run->{$rule}) {
                     if ($Application::plausibility_checks && $self->run->{$rule} != $Rule::exec_OK) {
                        die "internal error: rule ", $rule->header, " occurs in the 'ready' list although known to have failed\n";
                     }
                     push @satisfied, $rule;
                  } elsif (@replay_rules) {
                     # don't process new preconditions until the replaying is complete
                     push @blocked, $rule;
                  } else {
                     push @pending, $rule;
                  }
                  splice @{$top->ready}, $i, 1;  --$last;

               } elsif ($rule->flags & $Rule::is_initial) {
                  if (@replay_rules) {
                     push @blocked, $rule;
                  } else {
                     push @initial, $rule;
                  }
                  splice @{$top->ready}, $i, 1;  --$last;

               } else {
                  ++$i;
               }
            }
            if (@pending) {
               my @suppliers;
               if (my @cheap=filter_cheap_suppliers($top, \@pending, \@suppliers, $Rule::std_weight->[0])) {
                  if (@replay_rules && @suppliers) {
                     die "internal error: found unexecuted cheap suppliers: ", (map { "\n  ".$_->header } @suppliers ),
                         "\n while still replaying the rules from the previous run:",
                         (map { "\n  ".$_->header } @replay_rules), "\n";
                  }
                  dbg_print( "checking pending preconditions:", map { "\n".$_->header } @cheap ) if $Verbose::scheduler>=2;
                  my (@succeeded_suppliers, @failed);

                  foreach $rule (@suppliers) {
                     if (($self->run->{$rule}=$rule->execute($object,0)) == $Rule::exec_OK) {
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
                     foreach $rule (@cheap) {
                        my ($rc, $retval)=$rule->execute($object,1);
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
                     undef $top; $heap->reset();   # kill all other variants as to reduce refcounts in the supplier/consumer net
                     eliminate_rules($self, @failed) or last;
                     squeeze_prefs($self);
                     @{$self->ready}=grep { $self->run->{$_} <= $Rule::exec_retry && exists $self->consumer->{$_} } @{$self->ready};
                  }
                  unless ($give_schedule) {
                     @replay_rules=grep { exists $self->consumer->{$_} } @succeeded_suppliers;
                  }
                  $heap->reset(@replay_rules ? $self : clone($self));
                  next;
               }
            }

            foreach $rule (@pending, @satisfied) {
               $top->supplier->{$rule}=0;
               add_rule($top, $rule) or next HEAP_LOOP;
            }
            foreach $rule (@initial) {
               add_ready_rule($top, $rule) or next HEAP_LOOP;
            }
            if (!@initial || @{$top->supplier->{$self->final}}) {
               # Still something to resolve.
               if (@replay_rules) {
                  $top==$self or die "internal error: tentative chain cloned while replaying rules from the previous run\n";
                  $rule=shift @replay_rules;
                  my $i=list_index($self->ready, $rule);
                  $i>=0 or die "internal error: rule ", $rule->header, "\nhas to be replayed but missing in the ready list\n";
                  splice @{$self->ready}, $i, 1;
                  if (defined($rule->with_permutation)
                      ? add_ready_rule_with_permutation($self, $self, $rule, $give_schedule)
                      : add_ready_rule($self, $rule)) {

                     # revise remaining ready rules - some of them might become useless
                     if (@{$self->ready}=grep { exists $self->consumer->{$_} } @{$self->ready}, @blocked) {
                        if (@replay_rules=grep { exists $self->consumer->{$_} } @replay_rules) {
                           $heap->push($self);
                        } else {
                           # replaying of the previuos run finished: forget the already run rules, start producing variants
                           $self->weight->toZero();
                           if ($Application::plausibility_checks &&
                               (my @wrong=grep { !exists $self->run->{$_} || $self->run->{$_} != $Rule::exec_OK } @{$self->rules})) {
                              die "internal error: unexecuted or failed rules included during replaying phase:",
                                  (map { "\n  ".$_->header." => ".($self->run->{$_} // "NOT RUN") } @wrong), "\n";
                           }
                           @{$self->rules}=();
                           $heap->push(clone($self));
                        }
                        next;
                     }
                  }
                  die "internal error: progress stopped while replaying rules from the previous run; pending rules are:",
                      (map { "\n  ".$_->header } $rule, @replay_rules), "\n";

               } else {
                  # Split off feasible variants for each ready production rule.
                  # Process the heavier rules first: the lighter variants which will swim on the heap's top
                  # should have as few chances for further branching as possible.
                  # The minor weight component is not of importance here.
                  my @ready=sort { exists($top->consumer->{$b}->{$self->final}) <=> exists($top->consumer->{$a}->{$self->final})
                                    || $b->weight->[0] <=> $a->weight->[0]
                                 } @{$top->ready};

                  @{$top->ready}=();
                  while (defined ($rule=shift @ready)) {
                     my $var= @ready ? clone($top) : $top;
                     if (defined($rule->with_permutation)
                         ? add_ready_rule_with_permutation($self, $var, $rule, $give_schedule)
                         : add_ready_rule($var, $rule)) {
                        # revise remaining ready rules - some of them might become useless
                        push @{$var->ready}, grep { defined($var->consumer->{$_}) } @ready, @blocked;
                        $heap->push($var) if @{$var->ready};
                     }

                     # eliminate the skipped rule in the stem variant
                     @ready &&
                       ( ($rule->flags & $is_mult_chooser and defined($rule->rule->bag))
                         ? eliminate_mult_chooser($top, $rule)
                         : eliminate_rules($top, $rule) )
                       or last;
                  }
                  assign_max($maxheap, scalar(@$heap)) if $Verbose::scheduler;
               }
               next;
            }
         }

         # ready
         if (@replay_rules) {
            die "internal error: schedule allegedly resolved while still replaying rules from the previous run:",
                ( map { "\n  ".$_->header } @replay_rules ), "\n";
         }
         if ($Verbose::scheduler) {
            dbg_print( sprintf("minimum weight rule chain constructed in %.3f sec.\n", tv_interval($self->Tstart)),
                       "      |heap|: cur=", scalar(@$heap)+1, ", max=$maxheap, #pop=$popcnt" );
         }
         my $last_failed;
         if (defined $give_schedule) {
            return $top unless $give_schedule;
            # pick out pending preconditions and check them now
            my @chain=prepare_schedule($top);
            unless (@{$top->rules} and defined( $last_failed=execute($top, $object) )) {
               return new RuleChain($object, $top, @chain);
            }
         } else {
            weave_preconditions($top);
            dbg_print( "rules to execute:\n", $top->report ) if $Verbose::scheduler;
            my $ret= @{$top->rules} && $top->rules->[-1]->flags == $Rule::is_function ? pop @{$top->rules} : 1;
            unless (defined( $last_failed=execute($top, $object) )) {
               return $ret;
            }
         }

         my ($failed_rule, @skipped_rules)=splice @{$top->rules}, $last_failed;
         return if $failed_rule->flags == $Rule::is_initial;
         if ($Verbose::scheduler) {
            dbg_print( "trying to find an alternative way" );
            $self->Tstart=[ gettimeofday() ];
         }
         # if it was a precondition, let the object create the undef properties afterwards
         ++$tries unless $failed_rule->flags & $Rule::is_any_precondition;
         my @succeeded=@{$top->rules};
         undef $top; $heap->reset();  # kill all other variants as to reduce refcounts in the supplier/consumer net
         eliminate_rules($self, $failed_rule) or last;

         unless (defined $give_schedule) {
            # Only replay production rules.  Preconditions and PermActions are added automatically.
            @replay_rules=map { $_->without_permutation } grep { $_->flags == 0 && exists $self->consumer->{$_} } @succeeded;

            foreach my $action (grep { $_->flags & $Rule::is_perm_action } @skipped_rules) {
               unless (exists $self->supplier->{$action}) {
                  # it's not possible to finalize the permutation: bury the triggering rule if it's already run
                  if (delete_from_list(\@replay_rules, $action->perm_trigger)) {
                     eliminate_rules($self, $action->perm_trigger, $action->perm_trigger->with_permutation)
                       or last HEAP_LOOP;
                  }
               }
            }
         }
         squeeze_prefs($self);
         if (@{$self->ready}= grep { $self->run->{$_} <= $Rule::exec_retry && exists $self->consumer->{$_} } @{$self->ready}) {
            $heap->reset(@replay_rules ? $self : clone($self));
            $maxheap=1; $popcnt=0;
         }
      } # end while (heap)
   }
   return defined($give_schedule) ? undef : -$tries;
}
####################################################################################
package Polymake::Core::Scheduler::Heap;

sub new {
   my ($pkg, $chain)=@_;
   bless [ $chain ], $Verbose::scheduler>=3 ? "Polymake::Core::Scheduler::VerboseHeap" : $pkg;
}

sub push {
   my ($self, $var)=@_;
   push @$self, $var;
   my $i=$#$self;
   while ($i>0) {
      my $parent=($i-1)/2;
      last if $var->weight >= $self->[$parent]->weight;
      $self->[$i]=$self->[$parent];
      $i=$parent;
   }
   $self->[$i]=$var;
}

sub pop {
   my ($self)=@_;
   my $top=$self->[0];
   if ($#$self>0) {
      my $var=pop @$self;
      my $i=0;
      while ((my $child=$i*2+1)<=$#$self) {
         $child++  if $child<$#$self
                  and $self->[$child]->weight > $self->[$child+1]->weight;
         last if $var->weight <= $self->[$child]->weight;
         $self->[$i]=$self->[$child];
         $i=$child;
      }
      $self->[$i]=$var;
   } else {
      @$self=();
   }
   $top
}

sub reset {
   my $self=shift;
   @$self=@_;
}

####################################################################################
#  top-level public functions

package Polymake::Core::Scheduler;

# object, [ rules ] => success code
sub resolve_rules {
   InitRuleChain->create_and_resolve(@_) > 0
}

# [ rule_methods ], [ method args ] => code
sub resolve_method {
   my ($rules, $args)=@_;
   if (is_object( my $method=InitRuleChain->create_and_resolve($args->[0], $rules) )) {
      $method->code;
   } else {
      undef
   }
}

# used for auto-casts now; should die
sub resolve_rule {
   my ($object, $rule, $give_schedule)=@_;
   my $self=new InitRuleChain($object, $final_dummy);
   push @{$self->rules}, $self->deputy_root, $rule;
   $self->consumer->{$rule}->{$final_dummy}=1;
   $self->supplier->{$final_dummy}=[ { $rule => 1 } ];
   $self->gather_rules &&
   $self->resolve($object,$give_schedule);
}

sub resolve_permutation {
   my ($object, $rule, $give_schedule)=@_;
   my $self=new InitRuleChain($object, $final_dummy);
   $self->consumer->{$rule}->{$final_dummy}=1;
   $self->consumer->{$rule->with_permutation}->{$final_dummy}=1;
   $self->supplier->{$final_dummy}=[ { $rule => 1, $rule->with_permutation => 1 } ];
   $self->deputy_root->sensitive_to->{$rule->with_permutation->permutation->key}=1;
   $self->consider_rules($self->deputy_root,$rule);
   $self->run->{$_}=$Rule::exec_OK for @{$rule->preconditions};
   @{$self->rules}=();
   push @{$self->ready}, $rule;
   $self->gather_rules($self->deputy_root, $rule) &&
   $self->resolve($object,$give_schedule);
}
####################################################################################
sub resolve_initial_request {
   my ($object, $request)=@_;
   my $final=create Rule('initial commit', $request, 1);
   my $self=new InitRuleChain($object, $final);

   # Only a limited subset of production rules is allowed:
   # - it must not involve any permutations
   # - it must not create subobjects by applying default values
   my $further_rule_filter=$viable_rules;
   local_scalar($viable_rules,
                sub {
                   my $rules=pop;
                   $further_rule_filter->(@_,
                      [ grep {
                           $_->flags & ($Rule::is_any_precondition | $Rule::is_initial)
                             or
                           $_->flags == $Rule::is_default_value &&
                           defined($_[1]->real_object)
                             or
                           $_->flags==0 &&
                           !$_->with_permutation
                        } @$rules
                      ] )
                });

   if ($self->gather_rules && @{$self->supplier->{$final}}) {
      $self->resolve($object) > 0
   } else {
      1
   }
}
####################################################################################
sub resolve_request {
   my ($object, $request)=@_;
   my $final=create Rule('request', $request, 1);
   my $self=new InitRuleChain($object, $final);
   my $success=$self->gather_rules && $self->resolve($object);

   if ($success<=0) {
      my $shortening;
      foreach my $input_list (@{$final->input}) {
         shorten_input_list($object, $input_list, $shortening);
      }
      if (defined($shortening)) {
         $self=new InitRuleChain($object, $final);
         if ($self->gather_rules && $self->resolve($object)>0) {
            undef $shortening;
            $self=new InitRuleChain($object, $final);
            $success=$self->gather_rules && $self->resolve($object);
         }
      }
      return ($success, $self) if $success==0;
   }
   $success;
}
####################################################################################
# private:
sub shorten_input_list {
   my ($obj, $input_list, $scope)=@_;
   foreach my $path (@$input_list) {
      unless (is_object($path)) {
         my ($pv, $content_index);
         for (my $i=0; $i<$#$path; ++$i) {
            next if defined ($obj) &&
                    defined ($content_index=$obj->dictionary->{$path->[$i]->key}) &&
                    defined ($pv=$obj->contents->[$content_index]) &&
                    defined ($obj=$pv->value);

            if ($path->[$i]->flags & $Property::is_produced_as_whole) {
               $scope ||= ($_[2]=new Scope());
               $scope->begin_locals;
               local_shorten($path, $i);
               $scope->end_locals;
               last;
            }
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
   [ new => '$$@' ],
   [ '$genesis' => 'weak( #1 )' ],
   [ '@rules' => '@' ],
);

sub new {
   my $self=&_new;
   my $tentative=$_[1];
   foreach my $rule (@{$self->rules}) {
      $rule->copy4schedule($tentative);
   }
   $self
}

sub list { map { $_->header } @{$_[0]->rules} }

sub apply {
   my ($self, $object)=@_;
   my $chain_scope;
   if ($object==$self->genesis) {
      foreach my $rule (@{$self->rules}) {
         unless ($rule->flags & ($Rule::is_function | $Rule::is_precondition)) {
            $rule->execute($object,0,undef,$chain_scope)==$Rule::exec_OK or return;
         }
      }
   } else {
      foreach my $rule (@{$self->rules}) {
         $rule->execute($object,1,undef,$chain_scope)==$Rule::exec_OK or return;
      }
   }
   1
}

sub list_new_properties {
   my $self=shift;
   uniq(map { $_->list_results($self->genesis->type) } @{$self->rules})
}

package Polymake::Core::Rule;
sub up { 0 }
sub down { undef }
sub navigate { pop }
sub copy4schedule { }
sub perm_trigger { undef }

package Polymake::Core::Rule::CreatingPermutation;
sub copy4schedule { }
sub perm_trigger { $_[0]->rule };

package Polymake::Core::Rule::PermAction;
sub up { 0 }
sub down { undef }

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
