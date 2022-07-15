#  Copyright (c) 1997-2022
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

package Polymake::Core::UpgradeGroup;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$to_version' => '#1' ],
   [ '$to_v' => '#2' ],
   '@rules',
   [ '$big_objects' => 'undef' ],
   '%rules_by_type',
   '%subobjects_by_type',
);

sub prepare {
   my ($self) = @_;

   # for each type, collect rules from all its super types
   my (%by_type, %derived);
   foreach my $rule (@{$self->rules}) {
      push @{$by_type{$rule->type}}, $rule;
   }
   %{$self->rules_by_type} = %by_type;
   my $all_super = $self->big_objects->{super};
   while (my ($type, $super) = each %$all_super) {
      push @{$derived{$_}}, $type for @$super;
      if (my @inherit_from = grep { defined } @by_type{@$super}) {
         push @{$self->rules_by_type->{$type} //= [ ]}, map { @$_ } @inherit_from;
      }
   }

   # mark "interesting" types, that is, having subobjects with applicable rules
   # this may require several rounds of search
   my (%interesting, $repeat);
   my $is_interesting_subobject = sub {
      my ($type) = @_;
      exists($self->rules_by_type->{ANY_DATA_TYPE}) || exists($self->rules_by_type->{$type}) || $interesting{$type}
   };
   my $all_descends = $self->big_objects->{descend};
   do {
      $repeat = false;
      while (my ($type, $descends) = each %$all_descends) {
         $interesting{$type} ||= has_targets_among_subobjects($descends, $is_interesting_subobject) && do {
            if (defined(my $derived = $derived{$type})) {
               $interesting{$_} = true for @$derived;
            }
            $repeat = true;
         }
      }
   } while ($repeat);

   # filter out uninteresting subobjects and merge the subtrees from super types
   %by_type = ();
   while (my ($type, $descends) = each %$all_descends) {
      if (defined(my $filtered = filter_interesting_subobjects($descends, $is_interesting_subobject))) {
         $by_type{$type} = $filtered;
      }
   }
   %{$self->subobjects_by_type} = %by_type;
   while (my ($type, $super) = each %$all_super) {
      merge_subobjects($self->subobjects_by_type->{$type} //= { }, $_) for grep { defined } @by_type{@$super};
   }
}

sub has_targets_among_subobjects {
   my ($descends, $is_interesting_subobject) = @_;
   foreach my $descend (values %$descends) {
      if (is_array($descend)) {
         if ($is_interesting_subobject->($descend->[0]) ||
             has_targets_among_subobjects($descend->[1], $is_interesting_subobject)) {
            keys %$descends;
            return true;
         }
      } elsif ($is_interesting_subobject->($descend)) {
         keys %$descends;
         return true;
      }
   }
   false
}

sub filter_interesting_subobjects {
   my ($descends, $is_interesting_subobject) = @_;
   my %filtered;
   while (my ($prop_name, $descend) = each %$descends) {
      if (is_array($descend)) {
         if (defined(my $next_level = filter_interesting_subobjects($descend->[1], $is_interesting_subobject))) {
            $filtered{$prop_name} = [ $descend->[0], $next_level ];
         } elsif ($is_interesting_subobject->($descend->[0])) {
            $filtered{$prop_name} = $descend->[0];
         }
      } elsif ($is_interesting_subobject->($descend)) {
         $filtered{$prop_name} = $descend;
      }
   }
   keys(%filtered) ? \%filtered : undef
}

sub merge_subobjects {
   my ($subobjects, $descends) = @_;
   while (my ($prop_name, $descend) = each %$descends) {
      if (is_array($subobjects->{$prop_name})) {
         if (is_array($descend)) {
            if ($subobjects->{$prop_name}->[0] ne $descend->[0]) {
               die "contradicting subobject types for property $prop_name in big object inventory\n";
            }
            if (refcnt($subobjects->{$prop_name}) > 1) {
               $subobjects->{$prop_name} = deep_copy_list($subobjects->{$prop_name});
            }
            merge_subobjects($subobjects->{$prop_name}->[1], $descend->[1]);
         } elsif ($subobjects->{$prop_name}->[0] ne $descend) {
            die "contradicting subobject types for property $prop_name in big object inventory\n";
         }
      } else {
         $subobjects->{$prop_name} = $descend;
      }
   }
}

sub apply {
   my ($self, $obj, $default_type, $descends) = @_;
   # strip off type parameters, if any
   my $type = is_hash($obj) && $obj->{_type} =~ s/^$qual_id_re\K.*//r || $default_type;
   defined($type)
     or die "can't determine the object type\n";

   my $cnt = 0;
   if (!defined($default_type) && defined(my $data = $obj->{data})) {
      foreach my $type_tag ($type, "ANY_DATA_TYPE") {
         if (defined(my $rules = $self->rules_by_type->{$type_tag})) {
            $cnt += $_->apply($data, $obj) for @$rules;
         }
      }
      return $cnt;
   }
   if (defined(my $rules = $self->rules_by_type->{$type})) {
      $cnt += $_->apply($obj) for @$rules;
   }
   # this is still necessary because of a hack in Serializer::upgrade_data
   return $cnt unless is_hash($obj);

   my $subobjects = $self->subobjects_by_type->{$type};
   if (defined($descends)) {
      if (defined($subobjects)) {
         if (refcnt($subobjects) > 2) {
            $subobjects = deep_copy_hash($subobjects);
         }
         merge_subobjects($subobjects, $descends);
      } else {
         $subobjects = $descends;
      }
   }

   # convert attachments and small data properties with explicit types

   if (defined(my $attrs = $obj->{_attrs})) {
      my $any_data_rules = $self->rules_by_type->{ANY_DATA_TYPE};
      while (my ($prop_name, $prop_attrs) = each %$attrs) {
         if (defined($type = $prop_attrs->{_type}) and
             $prop_attrs->{attachment} ||
             defined($any_data_rules) && !exists $subobjects->{$prop_name}) {
            $type =~ s/^$qual_id_re\K.*//;
            if (defined(my $rules = $self->rules_by_type->{$type})) {
               $cnt += $_->apply($obj->{$prop_name}, $prop_attrs) for @$rules;
            }
            if (defined($any_data_rules)) {
               $cnt += $_->apply($obj->{$prop_name}, $prop_attrs) for @$any_data_rules;
            }
         }
      }
   }

   if (defined($subobjects)) {
      # can't use each %$subobjects here because we might re-enter the same place recursively,
      # while one hash iterator can't be shared between two scopes (perl is a catastrophe)
      foreach my $prop_name (keys %$subobjects) {
         if (defined(my $child = $obj->{$prop_name})) {
            my $descend = $subobjects->{$prop_name};
            if (is_array($child)) {
               foreach my $instance (@$child) {
                  $cnt += apply($self, $instance, is_array($descend) ? @$descend : $descend);
               }
            } else {
               $cnt += apply($self, $child, is_array($descend) ? @$descend : $descend);
            }
         }
      }
   }

   $cnt
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
