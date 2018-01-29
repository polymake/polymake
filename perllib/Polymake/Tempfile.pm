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

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);
use feature 'state';

require 'Polymake/file_utils.pl';
require Cwd;

package Polymake::Tempfile;

sub root_dir {
   state $dir=find_writable_dir($PrivateDir);
}

sub new {
   root_dir();
   bless \(my $name), shift;
}

my $cnt="aaaa0000";
my $rename_cnt=0;

sub unique_name { "poly".$$."T".(++$cnt) }

use overload '""' => sub { ${$_[0]} ||= root_dir()."/".unique_name(); };

sub basename { $ {$_[0]} =~ m|/(poly[^/]+)$|; $1 }
sub dirname { $ {$_[0]} =~ m|(.*)/poly[^/]+$|; $1 }
sub rename { my $name="$_[0]"; ++$rename_cnt; $name=~s/T(?=\w+$)/N${rename_cnt}_/; $name }

# filename => boolean
sub is_a { index($_[1], root_dir())==0 }

sub DESTROY {
   my $stem=${$_[0]};
   if (defined($stem) and my @files=glob("$stem*")) {
      if ($DebugLevel && $@) {
         warn_print( "Preserving temporary files: @files\n" );
      } else {
         unlink @files;
      }
   }
}

package Polymake::Tempdir;
use POSIX ':errno_h';

my @to_delete;

sub new {
   my ($pkg, $till_exit)=@_;
   if ($till_exit && $till_exit ne "till_exit") {
      croak( "unknown Tempdir option $till_exit" );
   }
   my $path;
   for (;;) {
      $path=Tempfile::root_dir()."/".Tempfile::unique_name();
      mkdir($path, 0700) and last;
      $!==EEXIST or die "could not create temporary directory $path: $!\n";
   }
   my $self=bless \$path, $pkg;
   if ($till_exit) {
      @to_delete or add AtEnd("tempdir", sub { undef @to_delete });
      push @to_delete, $self;
   }
   $self
}

use overload '""' => sub { ${$_[0]} };

sub DESTROY {
   my $path=${$_[0]};
   if ($DebugLevel && $@) {
      warn_print( "Preserving temporary directory: $path" );
   } else {
      File::Path::remove_tree($path);
   }
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
