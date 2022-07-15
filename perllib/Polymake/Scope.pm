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

package Polymake::Scope;

use Polymake::Struct (
   [ '$locals' => 'undef' ],
   '%cleanup',          # \Object => [ args ] || \&method
);

sub DESTROY {
   local interrupts(block);
   my ($self) = @_;

   # undo locals before performing cleanup actions
   delete $self->[0];

   while (my ($key, $action) = each %{$self->cleanup}) {
      local $@;
      eval {
         if (is_code($action)) {
            $action->($key);
         } else {
            $key->cleanup($action);
         }
      };
      err_print($@) if $@;
   }
}

package Polymake::AtEnd;

my (%names, @actions, @before, @after);

END {
   local interrupts(block);

   my ($delayed, $succeeded);
   # remove dependencies on unregistered actions
   my $n = keys %names;
   for (my $i = 0; $i <= $n; ++$i) {
      if (!defined($actions[$i])) {
         foreach (keys %{$before[$i]}) {
            delete $after[$_]->{$i};
         }
      }
   }

   while (1) {
      $delayed = $succeeded = 0;
      for (my $i = 0; $i <= $#actions; ++$i) {
         next if !defined($actions[$i]);
         if (keys(%{$after[$i]}) == 0) {
            eval { $actions[$i]->() };
            if ($@) {
               print STDERR "cleanup in package ", sub_pkg($actions[$i]), " failed:", $@ =~ /\n./ ? "\n" : " ", $@;
            }
            undef $actions[$i];
            foreach (keys %{$before[$i]}) {
               delete $after[$_]->{$i};
            }
            ++$succeeded;
         } else {
            ++$delayed;
         }
      }
      if ($delayed) {
         if (!$succeeded) {
            my @blocked;
            while (my ($name, $i) = each %names) {
               push @blocked, $name if defined($actions[$i]);
            }
            print STDERR "deadlock due to circular dependencies between AtEnd actions: ", join(", ", @blocked), "\n";
            last;
         }
      } else {
         last;
      }
   }
}

sub find_id {
   my ($name) = @_;
   $names{$name} //= do {
      my $i = keys %names;
      $before[$i] = { };
      $after[$i] = { };
      $i
   }
}

sub add {
   $_[0] eq __PACKAGE__ && shift;
   my ($name, $action, %relations) = @_;

   my $i = find_id($name);
   my $ignore_multiple = delete $relations{ignore_multiple};
   ($actions[$i] &&= do {
      if ($ignore_multiple) {
         return;
      } elsif (sub_pkg($actions[$i]) eq sub_pkg($action)) {
         croak( "package ", sub_pkg($action), " tries to install multiple AtEnd actions with the same key '$name'" );
      } else {
         croak( "packages ", sub_pkg($actions[$i]), " and ", sub_pkg($action), " use the same AtEnd action key '$name'" );
      }
   }) = $action;

   if (defined (my $before = delete $relations{before})) {
      foreach (ref($before) ? @$before : $before) {
         my $b = find_id($_);
         if (exists $after[$i]->{$b}) {
            croak( "circular dependency between AtEnd actions $name and $_" );
         }
         $before[$i]->{$b} = true;
         $after[$b]->{$i} = true;
      }
   }
   if (defined (my $after=delete $relations{after})) {
      foreach (ref($after) ? @$after : $after) {
         my $a=find_id($_);
         if (exists $before[$i]->{$a}) {
            croak( "circular dependency between AtEnd actions $name and $_" );
         }
         $before[$a]->{$i} = true;
         $after[$i]->{$a} = true;
      }
   }
   if (keys %relations) {
      croak( "unknown AtEnd option(s): ", join(",", keys %relations));
   }
}

sub forget {
   my $name=pop;
   if (defined (my $i = $names{$name})) {
      delete $actions[$i];
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
