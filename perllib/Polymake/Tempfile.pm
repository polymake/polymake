#  Copyright (c) 1997-2023
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

require 'Polymake/file_utils.pl';

package Polymake::Tempfile;

sub root_dir {
   state $dir = find_writable_dir($PrivateDir);
}

sub new {
   root_dir();
   bless \(my $name), shift;
}

my $cnt = "aaaa0000";
my $rename_cnt = 0;

sub unique_name { "poly".$$."T".(++$cnt) }

use overload '""' => sub { ${$_[0]} ||= root_dir()."/".unique_name(); };

sub basename { $ {$_[0]} =~ m|/(poly[^/]+)$|; $1 }
sub dirname { $ {$_[0]} =~ m|(.*)/poly[^/]+$|; $1 }
sub rename { my $name="$_[0]"; ++$rename_cnt; $name=~s/T(?=\w+$)/N${rename_cnt}_/; $name }

# filename => boolean
sub is_a { index($_[1], root_dir())==0 }

sub DESTROY {
   my $stem = ${$_[0]};
   if (defined($stem) and my @files = glob("$stem*")) {
      if ($DebugLevel && $@) {
         warn_print( "Preserving temporary files: @files\n" );
      } else {
         unlink @files;
      }
   }
}


1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
