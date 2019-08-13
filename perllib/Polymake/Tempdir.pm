#  Copyright (c) 1997-2019
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
use warnings qw(FATAL void syntax misc);
use feature 'state';

require Polymake::Tempfile;
require Polymake::Scope;
require File::Path;

package Polymake::Tempdir;
use POSIX ':errno_h';

my @to_delete;

sub new {
   my ($pkg, $till_exit) = @_;
   if ($till_exit && $till_exit ne "till_exit") {
      croak( "unknown Tempdir option $till_exit" );
   }
   my $path;
   for (;;) {
      $path = Tempfile::root_dir() . "/" . Tempfile::unique_name();
      mkdir($path, 0700) and last;
      $! == EEXIST or die "could not create temporary directory $path: $!\n";
   }
   my $self = bless \$path, $pkg;
   if ($till_exit) {
      @to_delete or add AtEnd("tempdir", sub { undef @to_delete });
      push @to_delete, $self;
   }
   $self
}

use overload '""' => sub { ${$_[0]} };

sub DESTROY {
   my $path = ${$_[0]};
   if ($DebugLevel && $@) {
      warn_print( "Preserving temporary directory: $path" );
   } else {
      File::Path::remove_tree($path);
   }
}


1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
