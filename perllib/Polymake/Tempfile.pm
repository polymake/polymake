#  Copyright (c) 1997-2017
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
use feature 'state';

require 'Polymake/file_utils.pl';
require Cwd;

package Polymake::Tempfile;
use POSIX ':errno_h';

sub dir {
   if (@_ && defined($ {$_[0]})) {
      &dirname
   } else {
      state $dir=find_writable_dir($PrivateDir);
   }
}

sub new {
   dir();
   bless \(my $name), shift;
}

my $cnt="aaaa0000";
my $rename_cnt=0;
my @to_delete;

sub unique_name { "poly".$$."T".(++$cnt) }

use overload '""' => sub { $ {$_[0]} ||= dir()."/".unique_name(); };

sub basename { $ {$_[0]} =~ m|/(poly[^/]+)$|; $1 }
sub dirname { $ {$_[0]} =~ m|(.*)/poly[^/]+$|; $1 }
sub rename { my $name="$_[0]"; ++$rename_cnt; $name=~s/T(?=\w+$)/N${rename_cnt}_/; $name }

# filename => boolean
sub is_a { index($_[1], dir())==0 }

sub DESTROY {
   my ($self)=@_;
   if (defined($$self)) {
      if (my @files=glob $$self."*") {
         if ($DebugLevel && $@) {
            warn_print( "temporary files kept: @files\n" );
         } else {
            unlink @files;
         }
      }
   }
}

sub descend_dir {
   my ($self) = @_;
   die "Can't use subdirectory if Tempfile has already been used!" if defined($$self);
   $$self = &new_dir;
   $$self = $$self . "/" . $self->basename;
}

sub new_dir {
   shift;
   my $path;
   for (;;) {
      $path=dir()."/".unique_name();
      mkdir($path, 0700) and last;
      $!==EEXIST or die "could not create temporary directory $path: $!\n";
   }
   push @to_delete, $path;
   state $register_temp_dir //= do {
      add AtEnd("tempdir", sub {
                   if ($DebugLevel) {
                      warn_print( "preserving temporary directories", map{ (" ", $_) } @to_delete );
                   } else {
                      foreach my $dir (@to_delete) {
                         File::Path::rmtree($dir);
                      }
                   }
                });
      1 };
   $path
}

package Polymake::TempChangeDir;

sub new {
   my ($pkg, $to_dir)=@_;
   my $cwd=Cwd::getcwd;
   chdir $to_dir or die "can't change into $to_dir: $!\n";
   bless \$cwd, $pkg;
}

sub DESTROY {
   chdir ${$_[0]} or warn "can't change back into ${$_[0]}: $!\n";
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
