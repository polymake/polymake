#  Copyright (c) 1997-2021
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

require Polymake::Core::UpgradeRule;
require Polymake::Core::UpgradeGroup;

package Polymake::Core::Upgrades;

my $cur_loading;

use Polymake::Struct (
   [ new => '$' ],
   [ '$to_version' => '#1' ],
   [ '$to_v' => 'eval("v" . #1)' ],
   '@groups',
);

sub prepare {
   my ($self, $expect_complete) = @_;
   local scalar $cur_loading = $self;
   dbg_print( "reading upgrade rules from $InstallTop/upgrades/".$self->to_version ) if $Verbose::rules > 1;
   do "upgrades/".$self->to_version;
   if ($@) {
      die "failed to load upgrade rules for ", $self->to_version, ": $@";
   }
   if ($expect_complete && !@{$self->groups}) {
      die "upgrade file $InstallTop/upgrades/" . $self->to_version . " does not define any rules\n";
   }

   # assign big object inventories to groups
   # versions without own inventories are assumed to share it with their predecessors
   my $big_objects;
   foreach my $group (reverse @{$self->groups}) {
      my $inv_file = "$InstallTop/upgrades/big_objects-".$group->to_version;
      if (-f $inv_file) {
         open my $F, $inv_file
           or die "can't read inventory file $inv_file: $!\n";
         local $/;
         $big_objects = decode_json(<$F>);
         is_hash($big_objects->{descend}) &&
         is_hash($big_objects->{super})
           or die "invalid inventory file $inv_file: missing mandatory elements 'descend' and/or 'super'\n";
      }
      $group->big_objects = $big_objects;
      if ($expect_complete) {
         defined($big_objects)
           or die "missing inventory file $inv_file\n";
         $group->prepare;
      }
   }
}

sub add_rule {
   my ($to_version, $to_v, $type, $paths, $body) = @_;
   if ($to_v ge $cur_loading->to_v) {
      croak("target version of an upgrade rule $to_version is higher than the target version of the entire rule file ", $cur_loading->to_version);
   }
   my $rule = new UpgradeRule($type, $paths, $body);
   my $self = $cur_loading;
   # insert rules in descending version order
   for (my ($i, $last) = (0, $#{$self->groups}); $i <= $last; ++$i) {
      my $group = $self->groups->[$i];
      my $cmp_versions = $group->to_v cmp $to_v;
      if ($cmp_versions < 0) {
         $group = new UpgradeGroup($to_version, $to_v);
         splice @{$self->groups}, $i, 0, $group;
      }
      if ($cmp_versions <= 0) {
         push @{$group->rules}, $rule;
         return;
      }
   }
   my $group = new UpgradeGroup($to_version, $to_v);
   push @{$self->groups}, $group;
   push @{$group->rules}, $rule;
}

my @repo;

sub get_groups {
   my ($from_version) = @_;
   unless (@repo) {
      @repo = sort { $b->to_v cmp $a->to_v } map { m{/([\d.]+)$} ? new(__PACKAGE__, $1) : () } glob("$InstallTop/upgrades/*");
   }
   my $index = -1;
   foreach my $updates (@repo) {
      last if $updates->to_v le $from_version;
      unless (@{$updates->groups}) {
         prepare($updates, true);
      }
      ++$index;
   }
   reverse((map { @{$repo[$_]->groups} } 0..$index-1),
           $index >= 0 ? (grep { $_->to_v gt $from_version } @{$repo[$index]->groups}) : ());
}

sub apply {
   my ($obj, $version, $default_type) = @_;
   my $cnt = 0;
   foreach my $group (get_groups($version)) {
      $cnt += $group->apply($obj, $default_type);
   }
   $cnt
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
