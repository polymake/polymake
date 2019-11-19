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

@ARGV or die "usage: perl $0 FILE.c ... >FILE.h\n";

my $proto="XS(boot_DynaLoader);\n";
my $bind=<<'.';
   newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, "Main.cc");
.

foreach my $file (@ARGV) {
   open my $C, $file or die "can't read $file: $!\n";
   my ($filename)= $file =~ m{(?:^|/)([^/]+)$};
   while (<$C>) {
      if (/^XS(?:_EXTERNAL)?\((boot_(\w+))\);/) {
         $proto .= "$&\n";
         my $func=$1;
         (my $pkg=$2) =~ s/_/:/g;
         $bind .= qq{   newXS("$pkg\::bootstrap", $func, "$filename");\n};
      }
   }
}

print <<".";
/* CAUTION: this file is created automatically.
   Please make all changes in $0, not here.

   This header collects all the package bootstrap functions which must be called
   during the initial loading of the callable library.
*/

#ifndef POLYMAKE_XS_EXT_BOOTSTRAP_H
#define POLYMAKE_XS_EXT_BOOTSTRAP_H

#ifndef XS_EXTERNAL
#define XS_EXTERNAL(name) XS(name)
#endif

$proto
static void xs_init(pTHX)
{
$bind
}

#endif // POLYMAKE_XS_EXT_BOOTSTRAP_H
.

# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
