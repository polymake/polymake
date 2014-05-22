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

require 'Polymake/file_utils.pl';

package Polymake::Tempfile;
use POSIX ':errno_h';

my $dir;

sub dir { $dir ||= find_writable_dir($PrivateDir); }

sub new {
   dir();
   bless \(my $name), shift;
}

my $cnt="aaaa0000";
my $rename_cnt=0;
my ($registered_temp_dir, @to_delete);

sub unique_name { "poly".$$."T".(++$cnt) }

use overload '""' => sub { $ {$_[0]} ||= "$dir/".unique_name; };

sub basename { $ {$_[0]} =~ m|/(poly[^/]+)$|; $1 }
sub rename { my $name="$_[0]"; ++$rename_cnt; $name=~s/T(?=\w+$)/N${rename_cnt}_/; $name }

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

sub new_dir {
   shift;
   my $path;
   for (;;) {
      $path=dir()."/".unique_name();
      mkdir($path, 0700) and last;
      $!==EEXIST or die "could not create temporary directory $path: $!\n";
   }
   push @to_delete, $path;
   unless ($registered_temp_dir) {
      add AtEnd("tempdir", sub {
                   if ($DebugLevel) {
                      warn_print( "preserving temporary directories", map{ (" ", $_) } @to_delete );
                   } else {
                      foreach my $dir (@to_delete) {
                         File::Path::rmtree($dir);
                      }
                   }
                });
      $registered_temp_dir=1;
   }
   $path
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
