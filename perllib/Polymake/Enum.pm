#  Copyright (c) 1997-2018
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

package Polymake::Enum;

use integer;
use strict qw(vars subs);
use Carp;

my %named_enums;

sub import {
   shift;                       # drop the own package name
   my $cnt=0;
   my $pkg=caller;

   my ($cache, $prefix);

   if (@_==1 && $_[0] =~ /::/) {
      my $src=shift;
      (my $referred_pkg=$src) =~ s/::([^:]+)$//;
      $prefix=$1;
      if (!defined ($cache=$named_enums{$src})) {
         $referred_pkg=namespaces::lookup_class($pkg,$referred_pkg);
         if (!defined($referred_pkg) || !defined ($cache=$named_enums{"$referred_pkg\::$prefix"})) {
            croak "Enum: unknown enumeration type $src";
         }
      }
      foreach (@$cache) {
         *{"$pkg\::$_"}=\${"$referred_pkg\::$_"};
      }
      return;
   }

   $prefix="";
   if ($_[0] =~ /:$/) {
      $prefix="$`_";
      $named_enums{"$pkg\::$`"}=$cache=[ ];
      shift;
   }

   foreach (@_) {
      my $name=$prefix.$_;
      if ($name =~ /=/) {
         $cnt=eval("package $pkg; $'");
         croak "Enum:: invalid initializer: $@\n" if $@;
         $name=$`;
      }
      croak "Enum: redefinition of \$$name\n" if defined $ {"$pkg\::$name"};
      namespaces::declare_const(*{"$pkg\::$name"}, $cnt);
      push @$cache, "$name" if defined $cache;
      $cnt++;
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
