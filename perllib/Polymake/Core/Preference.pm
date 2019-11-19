#  Copyright (c) 1997-2019
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

package Polymake::Core::Preference;
use constant clock_start => 100000000;
my $clock = clock_start;
my $compile_clock = 0;

use Polymake::Enum Mode => {
   strict => 0,
   create => 1,    # allow to create new sublevels
   rules => 2,     # 'prefer' statement comes from the rules
   global => 4,    # 'prefer' statement comes from a global preference file
};

####################################################################################
package Polymake::Core::Preference::ControlList;

use Polymake::Struct (
   '@items',                              # controlled items (rules, subs)
   '@labels',                             # corresponding Label
   [ '$ordered' => '0' ],                 # number of leading items which are known to be ordered due to the active preferences.
                                          #  The trailing items are considered equally ranked.
                                          #  Thus, a control list without any active preferences applied has ordered==0.
   [ '$cleanup_table' => 'undef' ],       # -> Scope::cleanup for lists changed by prefer_now
   [ '$destroy_on_change' => 'undef' ],   # optional list of things to be destroyed upon any change at the top position
                                          # their destructors are supposed to invalidate caches dependent on the top position item
);

# private:
sub register_copy {
   my ($self, $src) = @_;
   ++($_->controls->{$self}) for @{$src->labels};
}
####################################################################################
sub init {
   my ($label, $self, $item) = @_;
   @$self == 0
     or Carp::confess( "internal error: passing a non-empty anonymous array to Label::add_control" );
   @$self = ( [ $item ], [ $label ], defined($label->clock), undef, undef );
   bless $self;
}

sub clone {
   my ($self) = @_;
   inherit_class([ [ @{$self->items} ], [ @{$self->labels} ], $self->ordered, undef, undef ], $self);
}

sub dup {
   my ($self) = @_;
   my $copy = &clone;
   register_copy($copy, $self);
   $copy;
}
####################################################################################
# private:
sub merge_items {
   my ($self, $src) = @_;

   if (!$src->ordered ||
       $self->ordered && $self->labels->[0]->clock > $src->labels->[0]->clock) {
      # src unordered - attach to the tail
      push @{$self->items}, @{$src->items};
      push @{$self->labels}, @{$src->labels};

   } elsif (!$self->ordered  ||
            $src->ordered && $self->labels->[0]->clock < $src->labels->[0]->clock) {
      # self unordered - insert the new items at the beginning
      unshift @{$self->items}, @{$src->items};
      unshift @{$self->labels}, @{$src->labels};
      $self->ordered = $src->ordered;

   } else {
      # both ordered: merge carefully
      my $self_ord = $self->ordered;
      my ($s, $d) = (0, 0);
      for (; $s < $src->ordered && $d < $self_ord;  ++$d) {
         if ($self->labels->[$d]->rank > $src->labels->[$s]->rank) {
            splice @{$self->items}, $d, 0, $src->items->[$s];
            splice @{$self->labels}, $d, 0, $src->labels->[$s];
            ++$s;
            ++$self_ord;
         }
      }
      $self->ordered += $src->ordered;
      # insert the higher-ranked and unordered rest
      my $last = $#{$src->items};
      splice @{$self->items}, $self_ord, 0, @{$src->items}[$s..$last];
      splice @{$self->labels}, $self_ord, 0, @{$src->labels}[$s..$last];
   }
}
####################################################################################
# protected:
sub merge {
   my $src = shift;
   my $self = $src->dup;
   my ($clean_merge, @remerge);
   if (defined($src->cleanup_table)) {
      # $src has been temporarily modified by prefer_now: this list has to be re-merged after leaving the scope
      $self->cleanup_table = $src->cleanup_table;
      $self->cleanup_table->{$self} = $clean_merge = new ControlList;
      push @remerge, $src;
   }
   foreach $src (@_) {
      if (defined($src->cleanup_table)) {
         # $src has been temporarily modified by prefer_now: this list has to be re-merged after leaving the scope
         if (@remerge) {
            # record the cleanup table of the innermost scope
            if ($src->labels->[0]->clock > $self->labels->[0]->clock) {
               $self->cleanup_table = $src->cleanup_table;
            }
         } else {
            $clean_merge = clone($self);
            $self->cleanup_table = $src->cleanup_table;
         }
         $src->cleanup_table->{$self} //= $clean_merge;
         push @remerge, $src;
      } elsif (defined($clean_merge)) {
         merge_items($clean_merge, $src);
      }
      register_copy($self, $src);
      merge_items($self, $src);
   }
   $clean_merge->cleanup_table = \@remerge if defined($clean_merge);
   $self
}
####################################################################################
# called from Scope destructor
# merge the items afresh after the source lists have been reverted
sub cleanup {
   my ($self, $clean_merge) = @_;
   undef $self->cleanup_table;
   undef $self->destroy_on_change;
   my @remerge;
   foreach my $src (@{$clean_merge->cleanup_table}) {
      if (defined($src->cleanup_table)) {
         # $src has been temporarily modified in the outer scope
         if (@remerge) {
            # record the cleanup table of the innermost scope
            if ($src->labels->[0]->clock > $self->labels->[0]->clock) {
               $self->cleanup_table = $src->cleanup_table;
            }
         } else {
            $self->cleanup_table = $src->cleanup_table;
            @{$self->items} = @{$clean_merge->items};
            @{$self->labels} = @{$clean_merge->labels};
            $self->ordered = $clean_merge->ordered;
         }
         $src->cleanup_table->{$self} //= $clean_merge;
         push @remerge, $src;
      } else {
         merge_items($clean_merge, $src);
      }
      merge_items($self, $src) if @remerge;
   }
   if (@remerge) {
      $clean_merge->cleanup_table = \@remerge;
   } else {
      # $clean_merge can be recycled
      $self->items = $clean_merge->items;
      $self->labels = $clean_merge->labels;
      $self->ordered = $clean_merge->ordered;
   }
}
####################################################################################
sub DESTROY {
   my ($self)=@_;
   delete $_->controls->{$self} for @{$self->labels};
}

END { undef &DESTROY; }
####################################################################################
# return the items from a control list, sorted by rank
# each bag starts with the rank value
# the last bag may contain unordered items (and rank is undef)
sub get_items_by_rank {
   my ($self)=@_;
   my (%seen, @bags);
   my $pos;
   for ($pos=0; $pos < $self->ordered; ++$pos) {
      my $cur_rank=$self->labels->[$pos]->rank;
      if (!@bags) {
         push @bags, [ $cur_rank ];
      } elsif (@{$bags[-1]}==1) {
         $bags[-1]->[0]=$cur_rank;
      } elsif ($bags[-1]->[0]<$cur_rank) {
         push @bags, [ $cur_rank ];
      }
      push @{$bags[-1]}, $self->items->[$pos] unless $seen{$self->items->[$pos]}++;
   }
   my $end = @{$self->items};
   if ($pos < $end) {
      if (!@bags || @{$bags[-1]}>1) {
         push @bags, [ undef ];
      } else {
         undef $bags[-1]->[0];
      }
      for (; $pos < $end; ++$pos) {
         push @{$bags[-1]}, $self->items->[$pos] unless $seen{$self->items->[$pos]}++;
      }
   }
   @bags;
}
####################################################################################
sub find_item_of_label {
   my ($self, $label)=@_;
   if ((my $pos=list_index($self->labels, $label)) >= 0) {
      $self->items->[$pos]
   } else {
      undef
   }
}
####################################################################################
package Polymake::Core::Preference::Label;

use Polymake::Struct (
   [ new => '$;$$$$' ],
   [ '$name' => '#1' ],
   [ '$parent' => 'weak(#2)' ],         # Label higher in the hierarchy
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
      my $pos = @{$list->items};
      if (defined($self->clock)) {
         if ($list->ordered == 0  or
             (my $clock_diff = $list->labels->[0]->clock - $self->clock) < 0) {
            $pos = 0;
            $list->ordered = 1;
         } elsif ($clock_diff == 0) {
            for ($pos = 0;
                 $pos < $list->ordered && $list->labels->[$pos]->rank <= $self->rank;
                 ++$pos) { }
            ++$list->ordered;
         }
      }
      splice @{$list->items}, $pos, 0, $item;
      splice @{$list->labels}, $pos, 0, $self;
   } else {
      &ControlList::init;
   }
}
####################################################################################
sub list_all_rules {
   my ($self)=@_;
   my @rules;
   while (my ($list, $cnt)=each %{$self->controls}) {
      for (my ($pos, $end)=(0, scalar @{$list->items}); $pos < $end; ++$pos) {
         if ($list->labels->[$pos]==$self && instanceof Rule(my $rule=$list->items->[$pos])) {
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
         my $clock_cmp=$list->labels->[0]->clock <=> $self->clock;
         if ($clock_cmp<0) {
            # the control list is obsolete, current label moves to its head
            push @out_of_effect, $list->labels->[0]->clock;
            $list->ordered=0;
         } elsif ($clock_cmp>0) {
            # default preference from the rules defeated by an active setting
            push @out_of_effect, $self->clock;
            next;
         } elsif ($list->labels->[0]==$self) {
            # nothing changes
            $list->ordered=$cnt;
            next;
         }
      }

      # lift all occurences of the current label to the bottom end of the active region of the control list
      my $new_pos = $list->ordered;
      $list->destroy_on_change = undef if $new_pos == 0;
      $list->ordered += $cnt;
      for (my ($pos, $end) = ($new_pos, scalar @{$list->items}); $pos < $end; ++$pos) {
         if ($list->labels->[$pos] == $self) {
            if ($pos != $new_pos) {
               splice @{$list->items}, $new_pos, 0, splice @{$list->items}, $pos, 1;
               splice @{$list->labels}, $new_pos, 0, splice @{$list->labels}, $pos, 1;
            }
            --$cnt or last;
            ++$new_pos;
         }
      }
      Carp::confess( "corrupted control list for label ", $self->full_name ) if $cnt;
   }

   (@out_of_effect, map { $_->set_preferred(@_) } values %{$self->children})
}
####################################################################################
sub neutralize_controls {
   my ($self, $deep)=@_;
   foreach my $list (keys %{$self->controls}) {
      if ($list->ordered && $list->labels->[0]==$self) {
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
   my ($self, $scope, $clock, $rank) = @_;
   local with($scope->locals) {
      local scalar $self->rank = $rank;
      local scalar $self->clock = $clock;
   }

   while (my ($list, $cnt) = each %{$self->controls}) {
      my ($new_items, $new_labels, $new_ordered);
      if (@{$list->items} == $cnt) {
         # no competitors
         if ($list->ordered) {
	    # nothing to do
            next;
	 } else {
	    $new_items = [ @{$list->items} ];
            $new_labels = [ @{$list->labels} ];
            $new_ordered = $cnt;
         }
      } else {
         if ($rank && $list->labels->[0]->clock == $clock && $list->labels->[0]->rank < $rank) {
            # already modified this list via a higher-ranked label
            $list->cleanup_table == $scope->cleanup
              or Carp::confess( "internal error: cleanup table mismatch: ", $list->cleanup_table, " instead of ", $scope->cleanup );
            $new_items = $list->items;
            $new_labels = $list->labels;
            $new_ordered = $list->ordered;
         } else {
            $new_items = [ @{$list->items} ];
            $new_labels = [ @{$list->labels} ];
            $new_ordered = 0;
         }
         my $higher = $new_ordered;
         for (my ($i, $end) = ($new_ordered, scalar @$new_items); $i < $end; ++$i) {
            if ($new_labels->[$i] == $self) {
               if ($i != $new_ordered) {
                  # put the controlled item in the first position
                  splice @$new_items, $new_ordered, 0, splice @$new_items, $i, 1;
                  splice @$new_labels, $new_ordered, 0, splice @$new_labels, $i, 1;
               }
               ++$new_ordered;
               last unless --$cnt;
            }
         }
         $cnt == 0
           or Carp::confess( "corrupted control list for label ", $self->full_name );
      }
      undef $list->destroy_on_change;
      if ($list->cleanup_table == $scope->cleanup) {
         # already modified in this scope
         if ($new_items != $list->items) {
            $list->items = $new_items;
            $list->labels = $new_labels;
         }
         $list->ordered = $new_ordered;
      } else {
         local with($scope->locals) {
            local ref $list->items = $new_items;
            local ref $list->labels = $new_labels;
            local scalar $list->ordered = $new_ordered;
            local scalar $list->cleanup_table = $scope->cleanup;
            local scalar $list->destroy_on_change = undef;
         }
      }
   }

   foreach my $c (values %{$self->children}) {
      $c->set_temp_preferred($scope, $clock, $rank);
   }
}
####################################################################################
sub full_name {
   my ($self)=@_;
   my $n=$self->name;
   while (defined($self=$self->parent)) {
      $n=$self->name.".$n";
   }
   $n
}

sub parent_name {
   my ($self)=@_;
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
      $status &= ($list->ordered && $list->labels->[0]->clock==$self->clock) ? 2 : 1
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
   if (is_array($list)) {
      push @$list, $self;
   } else {
      while (my ($name, $c)=each %{$self->children}) {
         if (!exists $list->{$name} || is_array($list->{$name})) {
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
   [ '$provenience' => '0' ],
   [ '@labels' => '@' ],
);

sub activate {
   my ($self)=@_;
   my $rank=0;
   map { $_->set_preferred($self->clock, $rank++) } @{$self->labels};
}

sub deactivate {
   my ($self)=@_;
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
   my ($self, $new_wildcard) = @_;
   my (@result, %tree);
   $_->subtract($self->clock, $new_wildcard, -1, \%tree) for @{$self->labels};
   my @sublists = values %tree;
   for (my $i = 0; $i <= $#sublists; ++$i) {
      my $list = $sublists[$i];
      if (is_array($list)) {
         push @result, new List($self->clock, @$list);
      } elsif ($list) {
         push @sublists, values %$list;
      }
   }
   @result;
}
####################################################################################
sub toString {
   my ($self) = @_;
   if (@{$self->labels} == 1) {
      '"' . $self->labels->[0]->full_name . '"'
   } else {
      '"' . $self->labels->[0]->wildcard_name . " " . join(", ", map { $_->parent_name } @{$self->labels}) . '"'
   }
}
####################################################################################
sub belongs_to {
   my ($self, $app) = @_;
   my $answer = false;
   foreach my $label (@{$self->labels}) {
      if (defined $label->application)  {
         if ($label->application == $app) {
            $answer = true;
         } else {
            $app->imported->{$label->application->name} or return false;
         }
      } else {
         warn_print( "label ", $label->full_name, " might to be obsolete" ) if $DeveloperMode && ! -d "$InstallTop/bundled/".($label->parent_name);
         return false;
      }
   }
   $answer;
}

sub visible_from {
   my ($self, $app)=@_;
   foreach my $label (@{$self->labels}) {
      next unless defined($label->application);
      return 0 unless $label->application==$app || $app->imported->{$label->application->name};
   }
   1
}
####################################################################################
package Polymake::Core::Preference::perApplication;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$handler' => 'weak(#1)' ],
   [ '$application' => 'weak(#2)' ],
   '@imported',                      # perApplication objects of imported applications
   '%labels',
   '@default_prefs',
   '$global_commands',
   '$private_commands',
);

####################################################################################
sub find_label {
   my ($self, $name, $create)=@_;
   ($name, my @sublevels) = split /\./, $name;
   my $label = $self->labels->{$name};
   unless ($label) {
      foreach (@{$self->imported}) {
         $label=$_->labels->{$name}  and  last;
      }
      return unless $label;
   }
   foreach $name (@sublevels) {
      $label = $create ? $label->child($name) : $label->children->{$name}
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
   my $create_sublevels = $mode & Mode::create;

   if ($expr =~ /^ $hier_id_re $/xo) {
      if (defined (my $label = find_label($self, $expr, $create_sublevels))) {
         return $label;
      } else {
         push @err, $expr;
      }
   } elsif (my ($sublevel, $list) = $expr =~ /^ \*\.($hier_id_re) \s+ ($hier_ids_re) $/xo) {
      @l=map { find_label($self, "$_.$sublevel", $create_sublevels) or
               push @err, "$_.$sublevel" and ()
         } split /\s*,\s*/, $list;
   } else {
      croak( "syntax error in preference list" );
   }

   if (@err) {
      if ($mode & Mode::rules) {
         croak( "unknown label", @err > 1 && "s", " @err" );
      } else {
         warn_print( "stored preference statements for label", @err > 1 && "s", " @err\n",
                     "are not in effect - probably excluded by auto-configuration" );
         $self->handler->need_save = true;
      }
   }

   @l
}
####################################################################################
sub add_preference {
   my ($self, $expr, $mode) = @_;
   my @l = &parse_label_expr or return;
   my $pref = new List($mode & Mode::rules ? ++$compile_clock : ++$clock, @l);
   $pref->provenience = $mode & (Mode::rules + Mode::global);

   if ($mode & Mode::rules) {
      push @{$self->default_prefs}, $pref;
      # activate this preference right now if the application
      # is already active, otherwise end_loading will do this
      if (contains($self->handler->applications,$self)) {
         $self->handler->activate(false, $pref);
      }
   } else {
      if (defined (my $dominating = $self->handler->check_repeating($pref))) {
         if ($mode == Mode::create) {
            # loading private file
            warn_print( "preference list ", $pref->toString, " ignored since another list ", $dominating->toString, " is already in effect" );
            $self->handler->need_save = true;
         }
         return;
      }
      $self->handler->need_save = true if $mode == Mode::strict;
      $self->handler->activate(false, $pref);
   }
}
####################################################################################
sub set_temp_preference {
   my ($self, $scope, $expr)=@_;
   my @l = parse_label_expr($self, $expr, Mode::rules);
   local with($scope->locals) {
      local scalar ++$clock;
   }
   my $rank = 0;
   $_->set_temp_preferred($scope, $clock, $rank++) for @l;
}
####################################################################################
# private:
sub matching_default_prefs {
   my ($self, $expr)=@_;
   my @matched;
   if ($expr =~ /^ $hier_id_re $/xo) {
      foreach my $pref (@{$self->default_prefs}) {
         my $cmp = prefix_cmp($expr, $pref->labels->[0]->full_name, ".");
         if ($cmp == 0) {
            # exact match
            return $pref;
         }
         if ($cmp == 1) {
            # re-activating at a sublevel
            if (defined (my $label = find_label($self, $expr))) {
               return new List(++$clock, $label);
            }
         } elsif ($cmp == -1) {
            # re-activating this list and maybe more others
            push @matched, $pref;
         }
      }

   } elsif ($expr =~ /^ \*\.$hier_id_re $/xo) {
      foreach my $pref (@{$self->default_prefs}) {
         my $cmp = prefix_cmp($expr, $pref->labels->[0]->wildcard_name, ".");
         if ($cmp == 0) {
            # exact match
            return $pref;
         }
         if ($cmp == 1) {
            # re-activating at a sublevel
            my @sublevels = split /\./, substr($expr, length($pref->labels->[0]->wildcard_name) + 1);
            if (my @sublabels = map { $_->descend(@sublevels) } @{$pref->labels}) {
               return new List(++$clock, @sublabels);
            }
         } elsif ($cmp == -1) {
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
   my ($self) = @_;
   map { $_->toString . (($_->provenience == Mode::rules) ? " (#)" : ($_->provenience == Mode::global) ? " (G)" : "") }
   grep { $_->visible_from($self->application) } @{$self->handler->active_prefs};
}
####################################################################################
# private:
sub parse_piece {
   my ($self, $text, $mode) = @_;
   while ($text =~ s/^[ \t]* prefer \s+ (['"])?(.*?)(?(1)\1) [ \t]* ;? (?= \s*$ )//xm) {
      add_preference($self, $2, $mode);
   }
   $text;
}

sub end_loading {
   my ($self) = @_;
   push @{$self->handler->applications}, $self;

   $self->handler->activate(false, @{$self->default_prefs});

   my $text = parse_piece($self, delete $self->handler->global_pieces->{$self->application->name}, Mode::create | Mode::global);
   if ($text =~ $significant_line_re) {
      $self->global_commands = $text;
   }
   $text = parse_piece($self, delete $self->handler->private_pieces->{$self->application->name}, Mode::create);
   if ($text =~ $significant_line_re) {
      $self->private_commands = $text;
   }
}
####################################################################################
sub user_commands {
   my ($self) = @_;
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
   my ($self, $filename) = @_;
   if (-f $filename) {
      if (defined ($self->private_file = new Customize::File($filename))) {
         $self->private_pieces = $self->private_file->pieces;
         $self->need_save = !defined($Version) || $self->private_file->version lt $VersionNumber;
         add AtEnd("Preference", sub { $self->save if $self->need_save }, before => "Customize");
      }
   } else {
      $self->need_save = true;
      add AtEnd("Preference", sub { $self->save($filename) }, before => "Customize");
   }
}

*load_global=\&Customize::load_global;

sub create_custom {
   my ($self, $pkg) = @_;
   if (defined (my $global = delete $Custom->global_pieces->{$pkg})) {
      substr($self->global_pieces->{$pkg}, 0, 0) .= $global;
   }
   $self->custom = new Customize::perApplication(@_);
   delete $self->private_pieces->{$pkg};
   $self->custom
}

sub app_handler { new perApplication(@_) }

my $sep_line = "\n#########################################\n";

my $preface = <<".";
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
   my ($self, $incr_clock) = splice @_, 0, 2;
   foreach my $pref (@_) {
      push @{$self->active_prefs}, $pref;
      $pref->clock = ++$clock if $incr_clock;

      if (my %old_clocks = map { $_ => 1 } $pref->activate) {
         # some older preference lists need modification or even must be discarded
         my $new_wildcard = $pref->labels->[0]->wildcard_name;
         for (my $i = $#{$self->active_prefs} - 1; $i >= 0; --$i) {
            my $old_pref = $self->active_prefs->[$i];
            if ($old_clocks{$old_pref->clock}) {
               splice @{$self->active_prefs}, $i, 1, $old_pref->subtract($new_wildcard);
            }
         }
      }
   }
}
####################################################################################
sub check_repeating {
   my ($self, $pref) = @_;
   foreach my $p (@{$self->active_prefs}) {
      my $cmp = $p->compare($pref);
      if ($cmp <= 0) {
         return ($cmp, $p);
      } elsif ($cmp == 1) {
         # has absorbed some existing preference list - can't have duplicates
         last;
      }
   }
   ()
}
####################################################################################
sub reset {
   my ($self, $app, $expr)=@_;
   if (my @prefs = map { $_->matching_default_prefs($expr) } reverse($app->prefs, @{$app->prefs->imported})) {
      activate($self, true,
               grep {
                  if (my ($cmp, $p) = check_repeating($self, $_)) {
                     $p->provenience = Mode::rules if $cmp == 0;
                     false
                  } else {
                     $self->need_save = true
                  }
               } @prefs);

   } else {
      my $matched = 0;
      my @sublevels = split /\./, $expr;
      my $use_wildcard = $sublevels[0] eq "*" and shift @sublevels;
      for (my $i = $#{$self->active_prefs}; $i >= 0; --$i) {
         my $pref = $self->active_prefs->[$i];
         if ($pref->provenience != Mode::rules && $pref->visible_from($app)) {
            if ($use_wildcard
                ? prefix_cmp($expr, $pref->labels->[0]->wildcard_name, ".") <= 0
                : @{$pref->labels} == 1 && prefix_cmp($expr, $pref->labels->[0]->full_name, ".") <= 0) {
               $pref->deactivate;
               splice @{$self->active_prefs}, $i, 1;
               $matched = true;
            } elsif ($use_wildcard and
                     $pref->labels->[0]->wildcard_name eq "*"
                     ? $pref->labels->[0]->descend(@sublevels)
                     : prefix_cmp($pref->labels->[0]->wildcard_name, $expr, ".") <= 0) {
               splice @{$self->active_prefs}, $i, 1, $pref->subtract($expr);
               $matched = true;
            }
         }
      }
      if ($matched) {
         $self->need_save = true;
         ++$clock;
      } else {
         croak( "no active or default preferences matching '$expr'" );
      }
   }
}
####################################################################################
sub reset_all {
   my ($self, $app) = @_;
   for (my $i = $#{$self->active_prefs}; $i >= 0; --$i) {
      my $pref = $self->active_prefs->[$i];
      if ($pref->provenience != Mode::rules && $pref->visible_from($app)) {
         $pref->deactivate;
         splice @{$self->active_prefs}, $i, 1;
         $self->need_save = true;
      }
   }
   foreach (reverse($app->prefs, @{$app->prefs->imported})) {
      activate($self, true, grep { !check_repeating($self,$_) and $self->need_save = true } @{$_->default_prefs});
   }
}
####################################################################################
sub obliterate_extension {
   my ($self, $ext) = @_;
   for (my $i = $#{$self->active_prefs}; $i >= 0; --$i) {
      my $pref = $self->active_prefs->[$i];
      my $cnt = 0;
      foreach (@{$pref->labels}) {
         ++$cnt if $_->extension == $ext;
      }
      if ($cnt == @{$pref->labels}) {
         $pref->deactivate;
         splice @{$self->active_prefs}, $i, 1;
         $self->need_save = true;
      } elsif ($cnt) {
         splice @{$self->active_prefs}, $i, 1;
         activate($self, false, new List(++$clock, grep { $_->extension != $ext } @{$pref->labels}));
         $self->need_save = true;
      }
   }
}

sub obliterate_application {
   my ($self, $per_app) = @_;
   delete_from_list($self->applications, $per_app)
     and $self->need_save = true;
}
####################################################################################
# private:
sub clean_borders {
   my ($text) = @_;
   if (defined($text)) {
      $text =~ s/\A (?: $empty_or_separator_line )+ //xom;
      $text =~ s/(?: $empty_or_separator_line )+ \Z//xom;
   }
   $text;
}

sub save {
   my ($self, $filename) = @_;
   if (!defined($filename)) {
      die "no preference file to save\n" unless $self->private_file;
      $filename = $self->private_file->filename;
   }
   my ($pf, $pf_k) = new OverwriteFile($filename);
   print $pf $preface;
   $self->custom->printMe($pf);
   print $pf $sep_line;

   my @active_prefs = @{$self->active_prefs};
   foreach my $per_app (@{$self->applications}) {
      print $pf "application ", $per_app->application->name, ";\n";

      for (my $i = 0; $i <= $#active_prefs; ) {
         my $pref = $active_prefs[$i];
         if ($pref->provenience != Mode::global && $pref->belongs_to($per_app->application)) {
            print $pf "\n", ($pref->provenience == Mode::rules ? "# prefer " : "prefer "), $pref->toString, ";\n";
            splice @active_prefs, $i, 1;
         } else {
            ++$i;
         }
      }

      if (defined (my $text = clean_borders($per_app->private_commands))) {
         print $pf "\n", $text;
      }
      print $pf $sep_line;
   }

   while (my ($app_name, $text) = each %{$self->private_pieces}) {
      $text =~ s/^\#line.*\n//mg;
      print $pf "application $app_name;\n$text";
   }

   close $pf;
   $self->need_save = false;
}
####################################################################################
# merge several control lists together
# If some of input lists is temporarily changed by prefer_now,
# the resulting list will be re-merged after the changes are reverted when the enclosing scope is left.
sub merge_controls {
   @_ <= 1 ? $_[0] : &ControlList::merge
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
