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

use Polymake::Enum qw( clock_start=100000000 );
my $clock=$clock_start;
my $compile_clock=0;

####################################################################################
package Polymake::Core::Preference::ControlList;

use Polymake::Struct(
   '@items',                          # even positions: controlled items (rules, subs); uneven positions: Label
   [ '$ordered' => '0' ],             # number of leading items which are known to be ordered due to the active preferences.
                                      #  The trailing items are considered equally ranked.
                                      #  Thus, a control list without any active preferences applied has ordered==0.
   [ '$cleanup_table' => 'undef' ],   # -> Scope::cleanup for lists changed by prefer_now
);

# the preferred item resides at the top position
sub resolve { $_[0]->items->[0] }

# private:
sub register_copy {
   my ($self, $src)=@_;
   for (my ($i, $last)=(1, $#{$src->items}); $i<=$last; $i+=2) {
      ++($src->items->[$i]->controls->{$self});
   }
}
####################################################################################
sub dup {
   my ($self)=@_;
   my $copy=inherit_class([ [ @{$self->items} ], $self->ordered, undef ], $self);
   register_copy($copy, $self);
   $copy;
}
####################################################################################
# protected:
sub merge_items {
   my ($self, $src)=@_;

   if (!$src->ordered  ||
       $self->ordered && $self->items->[1]->clock > $src->items->[1]->clock) {
      # src unordered - attach to the tail
      push @{$self->items}, @{$src->items};

   } elsif (!$self->ordered  ||
            $src->ordered && $self->items->[1]->clock < $src->items->[1]->clock) {
      # self unordered - insert the new items at the beginning
      unshift @{$self->items}, @{$src->items};
      $self->ordered=$src->ordered;

   } else {
      # both ordered: merge carefully
      my $self_ord=$self->ordered;
      my ($s, $d);
      for (($s, $d)=(0, 0);  $s<$src->ordered && $d<$self_ord;  $d+=2) {
         if ($self->items->[$d+1]->rank > $src->items->[$s+1]->rank) {
            splice @{$self->items}, $d, 0, @{$src->items}[$s, $s+1];
            $s+=2;
            $self_ord+=2;
         }
      }
      $self->ordered += $src->ordered;
      # insert the higher-ranked and unordered rest
      splice @{$self->items}, $self_ord, 0, @{$src->items}[$s..$#{$src->items}];
   }
}
####################################################################################
# protected:
sub merge {
   my $src=shift;
   my $self=$src->dup;
   my $mergeinfo;
   if (defined($src->cleanup_table)) {
      # $src has been temporarily modified by prefer_now: this list has to be re-merged after leaving the scope
      $src->cleanup_table->{$self}=$mergeinfo=[ [ ], 0, $src ];
   }
   foreach $src (@_) {
      if (defined($src->cleanup_table)) {
         # $src has been temporarily modified by prefer_now: this list has to be re-merged after leaving the scope
         # mergeinfo holds the invariant part gathered so far
         $mergeinfo //= [ [ @{$self->items} ], $self->ordered ];
         $src->cleanup_table->{$self} //= $mergeinfo;
      }
      if (defined($mergeinfo)) {
         push @$mergeinfo, $src;
      }
      register_copy($self, $src);
      merge_items($self, $src);
   }
   $self
}
####################################################################################
# called from Scope destructor
# merge the items afresh after the source lists have been reverted
sub cleanup {
   my ($self, $mergeinfo)=@_;
   # $mergeinfo may be shared between several scope cleanup tables, hence should be immutable
   @{$self->items}=@{$mergeinfo->[0]};
   $self->ordered=$mergeinfo->[1];
   foreach my $src (@{$mergeinfo}[2..$#$mergeinfo]) {
      merge_items($self, $src);
   }
}
####################################################################################
sub DESTROY {
   my ($self)=@_;
   for (my ($i, $last)=(1, $#{$self->items}); $i<=$last; $i+=2) {
      delete $self->items->[$i]->controls->{$self};
   }
}

END {
   if ($] >= 5.012) {
      undef &DESTROY;
   } else {
      # perl 5.10.1 crashes when a destructor becomes undefined
      *DESTROY=sub {}
   }
}
####################################################################################
# return the items from a control list, sorted by rank
# each bag starts with the rank value
# the last bag may contain unordered items (and rank is undef)
sub get_items_by_rank {
   my ($self)=@_;
   my (%seen, @bags);
   my $i;
   for ($i=0; $i<$self->ordered; $i+=2) {
      my $cur_rank=$self->items->[$i+1]->rank;
      if (!@bags) {
         push @bags, [ $cur_rank ];
      } elsif (@{$bags[-1]}==1) {
         $bags[-1]->[0]=$cur_rank;
      } elsif ($bags[-1]->[0]<$cur_rank) {
         push @bags, [ $cur_rank ];
      }
      push @{$bags[-1]}, $self->items->[$i] unless $seen{$self->items->[$i]}++;
   }
   my $last=$#{$self->items};
   if ($i<$last) {
      if (!@bags || @{$bags[-1]}>1) {
         push @bags, [ undef ];
      } else {
         undef $bags[-1]->[0];
      }
      for (; $i<$last; $i+=2) {
         push @{$bags[-1]}, $self->items->[$i] unless $seen{$self->items->[$i]}++;
      }
   }
   @bags;
}

####################################################################################
sub describe_control_item {
   my $item=shift;
   if (is_code($item)) {
      $item=$item->() if sub_file($item) =~ m|Polymake/Overload\.pm$|;
      (is_method($item) ? "method " : "sub ").method_owner($item)."::".method_name($item)."(".prototype($item).")"
   } else {
      "rule of ".$item->defined_for->full_name." ".$item->header;
   }
}

sub describe {
   my $self=shift;
   my $ordered_until=$self->ordered;
   my @result;
   for (my ($i, $last)=(0, $#{$self->items}); $i<$last; $i+=2) {
      if ($i==$self->ordered) {
	 push @result, "------";
      }
      push @result, $self->items->[$i+1]->full_name, describe_control_item($self->items->[$i]);
   }
   @result;
}
####################################################################################
package Polymake::Core::Preference::Label;

use Polymake::Struct (
   [ new => '$;$$$$' ],
   [ '$name' => '#1' ],
   [ '$parent' => 'weak( #2 )' ],       # Label higher in the hierarchy
   [ '$wildcard_name' => '#3 || "*"' ],
   '%children',                         # Labels lower in the hierarchy
   '%controls',                         # control list -> number of items in this list
   [ '$clock' => '#4' ],                # sequential number of the last 'prefer' command
   [ '$rank' => '#5' ],                 # rank in this command
   [ '$application' => 'undef' ],       # Application and ...
   [ '$extension' => 'undef' ],         # Extension where this label occurs
);

####################################################################################
sub set_application {
   my ($self, $app, $ext)=@_;
   while ($self->parent) {
      if ($self->application) {
         if ($self->application != $app && $self->application->imported->{$app->name}) {
            $self->application=$app;
         }
         last;
      }
      $self->application=$app;
      $self->extension=$ext;
      $self=$self->parent;
   }
}
####################################################################################
sub child {
   my ($self, $name)=@_;
   $self->children->{$name} ||=
      new Label( $name, $self, $self->wildcard_name.".$name", $self->clock, $self->rank );
}
####################################################################################
sub add_control {
   my ($self, $list, $item)=@_;
   ++$self->controls->{$list};
   if (is_object($list)) {
      my $pos=@{$list->items};
      if (defined($self->clock)) {
         if ($list->ordered==0  or
             (my $clock_diff=$list->items->[1]->clock - $self->clock) < 0) {
            $pos=0;
            $list->ordered=2;
         } elsif ($clock_diff==0) {
            for ($pos=0;
                 $pos<$list->ordered && $list->items->[$pos+1]->rank <= $self->rank;
                 $pos+=2) { }
            $list->ordered+=2;
         }
      }
      splice @{$list->items}, $pos, 0, $item, $self;
   } else {
      @$list==0
        or confess( "internal error: passing a non-empty anonymous array to Label::add_control" );
      bless $list, "Polymake::Core::Preference::ControlList";
      @$list=( [ $item, $self ], defined($self->clock)*2, undef );
   }
}
####################################################################################
sub list_all_rules {
   my $self=shift;
   my @rules;
   while (my ($list, $cnt)=each %{$self->controls}) {
      for (my ($pos, $last)=(0, $#{$list->items}); $pos < $last; $pos+=2) {
         if ($list->items->[$pos+1]==$self && instanceof Rule(my $rule=$list->items->[$pos])) {
            push @rules, $rule;
            --$cnt or last;
         }
      }
   }
   keys %{$self->controls};  # reset iterator
   (@rules, map { list_all_rules($_) } values %{$self->children})
}
####################################################################################
# clock, rank => clock values of preference lists having lost effect
sub set_preferred {
   my $self=shift;
   my @out_of_effect;
   if (defined $self->clock) {
      if ($self->clock==$_[0]) {
         warn_print( $self->full_name, " occurs in the preference list at positions ", $self->rank, " and $_[1]" );
         return;
      }
      push @out_of_effect, $self->clock;
   }
   ($self->clock, $self->rank)=@_;
   while (my ($list, $cnt)=each %{$self->controls}) {
      # what has to be done with this control list?
      if ($list->ordered) {
         my $clock_cmp=$list->items->[1]->clock <=> $self->clock;
         if ($clock_cmp<0) {
            # the control list is obsolete, current label moves to its head
            push @out_of_effect, $list->items->[1]->clock;
            $list->ordered=0;
         } elsif ($clock_cmp>0) {
            # default preference from the rules defeated by an active setting
            push @out_of_effect, $self->clock;
            next;
         } elsif ($list->items->[1]==$self) {
            # nothing changes
            $list->ordered=2*$cnt;
            next;
         }
      }

      # lift all occurences of the current label to the bottom end of the active region of the control list
      my $new_pos=$list->ordered;
      $list->ordered+=2*$cnt;
      for (my ($pos, $last)=($new_pos, $#{$list->items}); $pos<$last; $pos+=2) {
         if ($list->items->[$pos+1] == $self) {
            if ($pos != $new_pos) {
               splice @{$list->items}, $new_pos, 0, splice @{$list->items}, $pos, 2;
            }
            --$cnt or last;
            $new_pos+=2;
         }
      }
      confess( "corrupted control list for label ", $self->full_name ) if $cnt;
   }

   (@out_of_effect, map { $_->set_preferred(@_) } values %{$self->children})
}
####################################################################################
sub neutralize_controls {
   my ($self, $deep)=@_;
   foreach my $list (keys %{$self->controls}) {
      if ($list->ordered && $list->items->[1]==$self) {
         $list->ordered=0;
      }
   }

   if ($deep) {
      foreach my $c (values %{$self->children}) {
         neutralize_controls($c, $deep);
      }
   }
}
####################################################################################
sub set_temp_preferred {
   my ($self, $scope, $clock, $rank)=@_;
   $scope->begin_locals;
   local_scalar($self->rank, $rank);
   local_scalar($self->clock, $clock);
   $scope->end_locals;

   while (my ($list, $cnt)=each %{$self->controls}) {
      my ($new_items, $new_ordered);
      if (@{$list->items} == $cnt*2) {
         # no competitors
         if ($list->ordered) {
	    # nothing to do
            next;
	 } else {
	    $new_items=[ @{$list->items}[0..$cnt*2-1] ];
            $new_ordered=$cnt*2;
         }
      } else {
         if ($rank && $list->items->[1]->clock == $clock && $list->items->[1]->rank < $rank) {
            # already modified this list via a higher-ranked label
            $list->cleanup_table == $scope->cleanup
              or confess( "internal error: cleanup table mismatch: ", $list->cleanup_table, " instead of ", $scope->cleanup );
            $new_items=$list->items;
            $new_ordered=$list->ordered;
         } else {
            $new_items=[ @{$list->items} ];
            $new_ordered=0;
         }
         my $higher=$new_ordered/2;
         for (my ($i, $last)=($new_ordered, $#$new_items); $i<$last; $i+=2) {
            if ($new_items->[$i+1] == $self) {
               if ($i != $new_ordered) {
                  # put the controlled item in the first position
                  splice @$new_items, $new_ordered, 0, splice @$new_items, $i, 2;
               }
               $new_ordered+=2;
               last unless --$cnt;
            }
         }
         if ($Verbose::scheduler) {
            dbg_print( $self->full_name, " temporarily preferred over ",
                       join(", ", map { $new_items->[$_*2+1]->full_name } $higher+$self->controls->{$list}..($#$new_items-1)/2) );
         }
         $cnt==0
           or confess( "corrupted control list for label ", $self->full_name );
      }
      if ($list->cleanup_table == $scope->cleanup) {
         # already modified in this scope
         if ($new_items != $list->items) {
            $list->items=$new_items;
         }
         $list->ordered=$new_ordered;
      } else {
         $scope->begin_locals;
         local_array($list->items, $new_items);
         local_scalar($list->ordered, $new_ordered);
         local_scalar($list->cleanup_table, $scope->cleanup);
         $scope->end_locals;
      }
   }

   foreach my $c (values %{$self->children}) {
      $c->set_temp_preferred($scope, $clock, $rank);
   }
}
####################################################################################
sub full_name {
   my $self=shift;
   my $n=$self->name;
   while (defined($self=$self->parent)) {
      $n=$self->name.".$n";
   }
   $n
}

sub parent_name {
   my $self=shift;
   $self=$self->parent while defined($self->parent);
   $self->name
}
####################################################################################
sub descend {
   my $self=shift;
   foreach (@_) {
      $self=$self->children->{$_} or return;
   }
   $self
}

sub list_completions {
   my $prefix=pop;
   my $self=&descend or return ();
   grep { /^\Q$prefix\E/ } keys %{$self->children}
}
####################################################################################
#
#  Subtraction of preference lists

# => 1 - nothing more in effect
# => 0 - partially (not all controls or not all children)
# => 2 - fully
sub status {
   my ($self)=@_;
   my $status=3;
   foreach my $list (keys %{$self->controls}) {
      $status &= ($list->ordered && $list->items->[1]->clock==$self->clock) ? 2 : 1
      or return 0;
   }

   foreach my $c (values %{$self->children}) {
      $status &= status($c)
      or last;
   }

   $status
}
####################################################################################
sub add_to_pref_tree {
   my ($self, $list)=@_;
   if (is_ARRAY($list)) {
      push @$list, $self;
   } else {
      while (my ($name, $c)=each %{$self->children}) {
         if (!exists $list->{$name} || is_ARRAY($list->{$name})) {
            push @{$list->{$name}}, $c;
         } elsif ($list->{$name}) {
            add_to_pref_tree($c, $list->{$name});
         }
      }
   }
}
####################################################################################
sub subtract {
   my ($self, $clock, $new_wildcard, $wildcard_cmp, $tree)=@_;

   if ($self->clock != $clock) {
      # already involved in the new pref list - nothing to do
      return;
   }

   if (defined (my $subtree=$tree->{$self->name})) {
      if ($subtree) {
         # positive result already known
         add_to_pref_tree($self, $subtree);
      } else {
         # negative result already known
         neutralize_controls($self, 1);
      }
      return;
   }

   if ($wildcard_cmp<=0  and
       ($wildcard_cmp=prefix_cmp($self->wildcard_name, $new_wildcard, ".")) == 2) {
      # no intersection with new pref list - remains in effect
      $tree->{$self->name}=[ $self ];
      return;
   }

   my $status=status($self);
   if ($status & 1) {
      # completely out of control
      $tree->{$self->name}=0;
      neutralize_controls($self, 1);

   } elsif ($status) {
      if ($wildcard_cmp>0) {
         # this branch has survived
         $tree->{$self->name}=[ $self ];
      }

   } else {
      # injured - handle children individually
      my $subtree=$tree->{$self->name}={ };
      neutralize_controls($self);
      subtract($_, $clock, $new_wildcard, $wildcard_cmp, $subtree) for values %{$self->children};
   }
}
####################################################################################
package Polymake::Core::Preference::List;

use Polymake::Struct (
   [ new => '$@' ],
   [ '$clock' => '#1' ],
   [ '$provenience' => '0' ],   # 2: from the rules  4: from a global preference file
   [ '@labels' => '@' ],
);

sub activate {
   my $self=shift;
   my $rank=0;
   map { $_->set_preferred($self->clock, $rank++) } @{$self->labels};
}

sub deactivate {
   my $self=shift;
   foreach (@{$self->labels}) {
      if ($_->clock == $self->clock) {
         $_->neutralize_controls(1);
      }
   }
}
####################################################################################
sub compare {
   my ($p1, $p2)=@_;
   my $l=$#{$p1->labels};
   return 2 if $l != $#{$p2->labels};
   my $result=0;
   for (my $i=0; $i<=$l; ++$i) {
      if ($p1->labels->[$i] != $p2->labels->[$i]) {
         my $cmp=prefix_cmp($p1->labels->[$i]->full_name, $p2->labels->[$i]->full_name, ".");
         return 2 if $cmp==2  or  $result && $result != $cmp;
         $result=$cmp;
      }
   }
   $result;
}
####################################################################################
sub subtract {
   my ($self, $new_wildcard)=@_;
   my (@result, %tree);
   $_->subtract($self->clock, $new_wildcard, -1, \%tree) for @{$self->labels};
   my @sublists=values %tree;
   for (my $i=0; $i<=$#sublists; ++$i) {
      my $list=$sublists[$i];
      if (is_ARRAY($list)) {
         push @result, new List($self->clock, @$list);
      } elsif ($list) {
         push @sublists, values %$list;
      }
   }
   @result;
}
####################################################################################
sub toString {
   my $self=shift;
   if (@{$self->labels}==1) {
      '"'.$self->labels->[0]->full_name.'"'
   } else {
      '"' . $self->labels->[0]->wildcard_name . " " . join(", ", map { $_->parent_name } @{$self->labels}) . '"'
   }
}
####################################################################################
sub belongs_to {
   my ($self, $app)=@_;
   my $answer=0;
   foreach my $label (@{$self->labels}) {
      if (defined $label->application)  {
         if ($label->application==$app) {
            $answer=1;
         } else {
            $app->imported->{$label->application->name} or return 0;
         }
      } else {
         warn_print( "label ", $label->full_name, " seems to be obsolete" );
         return 0;
      }
   }
   $answer;
}

sub visible_from {
   my ($self, $app)=@_;
   foreach my $label (@{$self->labels}) {
      return 0 unless $label->application==$app || $app->imported->{$label->application->name};
   }
   1
}
####################################################################################
package Polymake::Core::Preference::perApplication;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$handler' => 'weak( #1 )' ],
   [ '$application' => 'weak( #2 )' ],
   '@imported',                      # perApplication objects of imported applications
   '%labels',
   '@default_prefs',
   '$global_commands',
   '$private_commands',
);

####################################################################################
sub find_label {
   my ($self, $name, $create)=@_;
   my ($name, @sublevels)=split /\./, $name;
   my $label=$self->labels->{$name};
   unless ($label) {
      foreach (@{$self->imported}) {
         $label=$_->labels->{$name}  and  last;
      }
      return unless $label;
   }
   foreach $name (@sublevels) {
      $label= $create ? $label->child($name) : $label->children->{$name}
      or return;
   }
   $label;
}
####################################################################################
# private:
sub list_completions {
   my ($self, $expr)=@_;
   if (length($expr)) {
      if ($expr =~ s/^\*\.($hier_id_re)\s+//o) {
         my @sublevels=split /\./, $1, -1;
         my %ignore;
         while ($expr =~ s/^($id_re)\s*,\s*//go) {
            $ignore{$1}=1;
         }
         grep { !$ignore{$_} && /^\Q$expr\E/ } map { $_->descend(@sublevels) ? ($_->name) : () } values %{$self->labels}
      } else {
         my ($top, @sublevels)=split /\./, $expr, -1;
         if (@sublevels) {
            if ($top eq "*") {
               map { $_->list_completions(@sublevels) } values %{$self->labels}
            } else {
               my $label=$self->labels->{$top} or return ();
               $label->list_completions(@sublevels)
            }
         } else {
            grep { /^\Q$top\E/ } keys %{$self->labels}
         }
      }
   } else {
      keys %{$self->labels}
   }
}
####################################################################################
# private:
sub parse_label_expr {
   my ($self, $expr, $mode)=@_;
   my (@err, @l);

   if ($expr =~ /^ $hier_id_re $/xo) {
      if (defined (my $label=find_label($self, $expr, $mode & 1))) {
         return $label;
      } else {
         push @err, $expr;
      }
   } elsif (my ($sublevel, $list)= $expr =~ /^ \*\.($hier_id_re) \s+ ($hier_ids_re) $/xo) {
      @l=map { find_label($self, "$_.$sublevel", $mode & 1) or
               push @err, "$_.$sublevel"
         } split /\s*,\s*/, $list;
   } else {
      croak( "syntax error in preference list" );
   }

   if (@err) {
      if ($mode & 2) {
         croak( "unknown label", @err>1 && "s", " @err" );
      } else {
         warn_print( "stored preference statements for label", @err>1 && "s", " @err\n",
                     "are not in effect - probably excluded by auto-configuration" );
         $self->handler->need_save=1;
      }
   }

   @l
}
####################################################################################
# $mode & 1 => allow to create new sublevels
# $mode & 2 => 'prefer' command comes from the rules
# $mode & 4 => 'prefer' command comes from a global preference file
sub add_preference {
   my ($self, $expr, $mode)=@_;
   my @l=&parse_label_expr or return;
   my $pref=new List($mode & 2 ? ++$compile_clock : ++$clock, @l);
   $pref->provenience |= $mode & 6;

   if ($mode & 2) {
      push @{$self->default_prefs}, $pref;
   } else {
      if (defined (my $dominating=$self->handler->check_repeating($pref))) {
         if ($mode==1) {
            # loading private file
            warn_print( "preference list ", $pref->toString, " ignored since another list ", $dominating->toString, " is already in effect" );
            $self->handler->need_save=1;
         }
         return;
      }
      $self->handler->need_save=1 if $mode==0;
      $self->handler->activate(0,$pref);
   }
}
####################################################################################
sub set_temp_preference {
   my ($self, $scope, $expr)=@_;
   my @l=parse_label_expr($self, $expr, 2);
   $scope->begin_locals;
   local_incr($clock);
   $scope->end_locals;
   my $rank=0;
   $_->set_temp_preferred($scope, $clock, $rank++) for @l;
}
####################################################################################
# private:
sub matching_default_prefs {
   my ($self, $expr)=@_;
   my @matched;
   if ($expr =~ /^ $hier_id_re $/xo) {
      foreach my $pref (@{$self->default_prefs}) {
         my $cmp=prefix_cmp($expr, $pref->labels->[0]->full_name, ".");
         if ($cmp == 0) {
            # exact match
            return $pref;
         }
         if ($cmp==1) {
            # re-activating at a sublevel
            if (defined (my $label=find_label($self, $expr))) {
               return new List(++$clock, $label);
            }
         } elsif ($cmp==-1) {
            # re-activating this list and maybe more others
            push @matched, $pref;
         }
      }

   } elsif ($expr =~ /^ \*\.$hier_id_re $/xo) {
      foreach my $pref (@{$self->default_prefs}) {
         my $cmp=prefix_cmp($expr, $pref->labels->[0]->wildcard_name, ".");
         if ($cmp==0) {
            # exact match
            return $pref;
         }
         if ($cmp==1) {
            # re-activating at a sublevel
            my @sublevels=split /\./, substr($expr, length($pref->labels->[0]->wildcard_name)+1);
            if (my @sublabels=map { $_->descend(@sublevels) } @{$pref->labels}) {
               return new List(++$clock, @sublabels);
            }
         } elsif ($cmp==-1) {
            push @matched, $pref;
         }
      }

   } else {
      croak( "invalid label expression '$expr'" );
   }
   @matched;
}
####################################################################################
sub list_active {
   my $self=shift;
   map { $_->toString . (($_->provenience & 2) ? " (#)" : ($_->provenience & 4) ? " (G)" : "") }
   grep { $_->visible_from($self->application) } @{$self->handler->active_prefs};
}
####################################################################################
# private:
sub parse_piece {
   my ($self, $text, $mode)=@_;
   while ($text =~ s/^[ \t]* prefer \s+ (['"])?(.*?)(?(1)\1) [ \t]* ;? (?= \s*$ )//xm) {
      add_preference($self, $2, $mode);
   }
   $text;
}

sub end_loading {
   my $self=shift;
   push @{$self->handler->applications}, $self;

   $self->handler->activate(0,$_) for @{$self->default_prefs};

   my $text=parse_piece($self, delete $self->handler->global_pieces->{$self->application->name}, 5);
   if ($text =~ $significant_line_re) {
      $self->global_commands=$text;
   }
   $text=parse_piece($self, delete $self->handler->private_pieces->{$self->application->name}, 1);
   if ($text =~ $significant_line_re) {
      $self->private_commands=$text;
   }
}
####################################################################################
sub user_commands {
   my $self=shift;
   $self->global_commands . $self->private_commands
}
####################################################################################

package Polymake::Core::Preference;

use Polymake::Struct (
   [ '$private_file' => 'undef' ],      # Customize::File
   [ '$custom' => 'undef' ],            # Customize::perApplication for User custom variables
   '@applications',                     # perApplication
   '%global_pieces',                    # package => global file fragment
   '%private_pieces',                   # package => private file fragment
   '$need_save',                        # boolean
   '@active_prefs',
);

sub load_private {
   my ($self, $filename)=@_;
   if (-f $filename) {
      if (defined ($self->private_file=new Customize::File($filename))) {
         $self->private_pieces=$self->private_file->pieces;
         $self->need_save= !defined($Version) || $self->private_file->version lt $VersionNumber;
         add AtEnd("Preference", sub { $self->save if $self->need_save }, before => "Customize");
      }
   } else {
      $self->need_save=1;
      add AtEnd("Preference", sub { $self->save($filename) }, before => "Customize");
   }
}

*load_global=\&Customize::load_global;

sub create_custom {
   my ($self, $pkg)=@_;
   if (defined (my $global=delete $Custom->global_pieces->{$pkg})) {
      substr($self->global_pieces->{$pkg},0,0) .= $global;
   }
   $self->custom=new Customize::perApplication(@_);
   delete $self->private_pieces->{$pkg};
   $self->custom
}

sub app_handler { new perApplication(@_) }

my $sep_line="\n#########################################\n";

my $preface=<<".";
#########################################################################
#
#  This file contains preference settings that were in effect
#  as you closed your last polymake session.
#
#  Initially it contains copies of "prefer" commands scattered over the
#  rule files.  They are commented out, since they come into action
#  as soon as the rule files are loaded.
#
#  Later on, each interactive "prefer" command you type in the polymake
#  shell is also recorded here, in the chronological order.
#  Prior commands having lost any effect are wiped out from the file
#  automatically.
#
#  You can also edit this file manually, including or deleting "prefer"
#  commands, or even other commands recognized by the interactive shell.
#  But never edit it while polymake processes are running, otherwise
#  you risk to do it in vain as all your changes may be overwritten.
#
#  To revert to the default preferences later, comment out or delete
#  your changes, or execute the interactive command 'reset_preference'.
#
#  Please be aware that this file is interpreted by polymake after all
#  rule files, unlike "customize.pl".
#
#########################################################################
#
#  The rule files are rescanned for new preference lists as soon as you
#  run a polymake version newer than recorded here, or use an application
#  for the first time.
#  If you have inserted new "prefer" commands in the rules and want them
#  to appear here right now, comment out the following line and rerun polymake.
\$version=v$Version;

#########################################
#
#  Settings common for all applications

.

####################################################################################
sub activate {
   my ($self, $incr_clock)=splice @_, 0, 2;
   foreach my $pref (@_) {
      push @{$self->active_prefs}, $pref;
      $pref->clock=++$clock if $incr_clock;

      if (my %old_clocks=map { $_=>1 } $pref->activate) {
         # some older preference lists need modification or even must be discarded
         my $new_wildcard=$pref->labels->[0]->wildcard_name;
         for (my $i=$#{$self->active_prefs}; $i>=0; --$i) {
            my $old_pref=$self->active_prefs->[$i];
            if ($old_clocks{$old_pref->clock}) {
               splice @{$self->active_prefs}, $i, 1, $old_pref->subtract($new_wildcard);
            }
         }
      }
   }
}
####################################################################################
sub check_repeating {
   my ($self, $pref)=@_;
   foreach my $p (@{$self->active_prefs}) {
      my $cmp=$p->compare($pref);
      if ($cmp <= 0) {
         return ($cmp, $p);
      } elsif ($cmp==1) {
         # has absorbed some existing preference list - can't have duplicates
         last;
      }
   }
   ()
}
####################################################################################
sub reset {
   my ($self, $app, $expr)=@_;
   if (my @prefs=map { $_->matching_default_prefs($expr) } reverse($app->prefs, @{$app->prefs->imported})) {
      activate($self, 1, grep {
                            if (my ($cmp, $p)=check_repeating($self,$_)) {
                               $p->provenience=2 if $cmp==0;
                               0
                            } else {
                               $self->need_save=1
                            }
                         } @prefs);

   } else {
      my $matched=0;
      my @sublevels=split /\./, $expr;
      my $use_wildcard= $sublevels[0] eq "*" and shift @sublevels;
      for (my $i=$#{$self->active_prefs}; $i>=0; --$i) {
         my $pref=$self->active_prefs->[$i];
         if (!($pref->provenience & 2) && $pref->visible_from($app)) {
            if ($use_wildcard
                ? prefix_cmp($expr, $pref->labels->[0]->wildcard_name, ".")<=0
                : @{$pref->labels}==1 && prefix_cmp($expr, $pref->labels->[0]->full_name, ".")<=0) {
               $pref->deactivate;
               splice @{$self->active_prefs}, $i, 1;
               $matched=1;
            } elsif ($use_wildcard and
                     $pref->labels->[0]->wildcard_name eq "*"
                     ? $pref->labels->[0]->descend(@sublevels)
                     : prefix_cmp($pref->labels->[0]->wildcard_name, $expr, ".")<=0) {
               splice @{$self->active_prefs}, $i, 1, $pref->subtract($expr);
               $matched=1;
            }
         }
      }
      if ($matched) {
         $self->need_save=1;
         ++$clock;
      } else {
         croak( "no active or default preferences matching '$expr'" );
      }
   }
}
####################################################################################
sub reset_all {
   my ($self, $app)=@_;
   for (my $i=$#{$self->active_prefs}; $i>=0; --$i) {
      my $pref=$self->active_prefs->[$i];
      if (!($pref->provenience & 2) && $pref->visible_from($app)) {
         $pref->deactivate;
         splice @{$self->active_prefs}, $i, 1;
         $self->need_save=1;
      }
   }
   foreach (reverse($app->prefs, @{$app->prefs->imported})) {
      activate($self, 1, grep { !check_repeating($self,$_) and $self->need_save=1 } @{$_->default_prefs});
   }
}
####################################################################################
sub obliterate_extension {
   my ($self, $ext)=@_;
   for (my $i=$#{$self->active_prefs}; $i>=0; --$i) {
      my $pref=$self->active_prefs->[$i];
      my $cnt=0;
      foreach (@{$pref->labels}) {
         ++$cnt if $_->extension==$ext;
      }
      if ($cnt==@{$pref->labels}) {
         $pref->deactivate;
         splice @{$self->active_prefs}, $i, 1;
         $self->need_save=1;
      } elsif ($cnt) {
         splice @{$self->active_prefs}, $i, 1;
         activate($self, 0, new List(++$clock, grep { $_->extension != $ext } @{$pref->labels}));
         $self->need_save=1;
      }
   }
}

sub obliterate_application {
   my ($self, $per_app)=@_;
   delete_from_list($self->applications, $per_app)
     and $self->need_save=1;
}
####################################################################################
# private:
sub clean_borders {
   local $_=shift;
   if (defined($_)) {
      s/\A (?: $empty_or_separator_line )+ //xom;
      s/(?: $empty_or_separator_line )+ \Z//xom;
   }
   $_;
}

sub save {
   my ($self, $filename)=@_;
   if (!defined($filename)) {
      die "no preference file to save\n" unless $self->private_file;
      $filename=$self->private_file->filename;
   }
   my ($pf, $pf_k)=new OverwriteFile($filename);
   print $pf $preface;
   $self->custom->printMe($pf);
   print $pf $sep_line;

   my @active_prefs=@{$self->active_prefs};
   foreach my $per_app (@{$self->applications}) {
      print $pf "application ", $per_app->application->name, ";\n";

      for (my $i=0; $i<=$#active_prefs; ) {
         my $pref=$active_prefs[$i];
         if (!($pref->provenience & 4) && $pref->belongs_to($per_app->application)) {
            print $pf "\n", ($pref->provenience & 2 ? "# prefer " : "prefer "), $pref->toString, ";\n";
            splice @active_prefs, $i, 1;
         } else {
            ++$i;
         }
      }

      if (defined (my $text=clean_borders($per_app->private_commands))) {
         print $pf "\n", $text;
      }
      print $pf $sep_line;
   }

   while (my ($app_name, $text)=each %{$self->private_pieces}) {
      $text =~ s/^\#line.*\n//mg;
      print $pf "application $app_name;\n$text";
   }

   close $pf;
   $self->need_save=0;
}
####################################################################################
# merge several control lists together
# If some of input lists is temporarily changed by prefer_now,
# the resulting list will be re-merged after the changes are reverted when the enclosing scope is left.
sub merge_controls {
   @_<=1 ? $_[0] : &ControlList::merge
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
