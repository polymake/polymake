#  Copyright (c) 1997-2015
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

require File::Path;
require File::Copy;

use namespaces;

package Polymake::SourceVersionControl;

sub new {
   my ($pkg, $dir)=@_;
   if (`svn info $dir 2>/dev/null` =~ /Revision:/) {
      bless \$dir, "$pkg\::SVN";
   } else {
      # @todo provide recognition and a module for git
      bless \$dir, "$pkg\::None";
   }
}

package _::SVN;

sub make_dir {
   my $self=shift;
   my $feedback=`cd $$self; svn mkdir --parents @_ 2>&1`;
   unless ($feedback =~ m{\A(?m:^A\s+\S+\n)+\Z}s) {
      None::make_dir($self, @_);
      warn_print( "svn mkdir --parents @_ failed:\n$feedback\n",
		  "The directories have been created, but not registered with svn.\n",
		  "Please investigate the reason of the failure and execute `svn add @_` manually." );
   }
}

sub add_file {
   my $self=shift;
   my $feedback=`cd $$self; svn add @_ 2>&1`;
   unless ($feedback =~ m{\A(?m:^A\s+\S+\n)+\Z}s) {
      warn_print( "svn add @_ failed:\n$feedback\n",
		  "Please investigate the reason of the failure and repeat the command manually." );
   }
}

sub copy_file {
   my $self=shift;
   my $feedback=`cd $$self; svn copy @_ 2>&1`;
   unless ($feedback =~ m{\A(?m:^A\s+\S+\n)+\Z}s) {
      None::copy_file($self, @_);
      warn_print( "svn copy @_ failed:\n$feedback\n",
		  "The file has been copied, but the copy has not been registered with svn.\n",
		  "Please investigate the reason of the failure and execute `svn add $_[-1]` manually." );
   }
}

sub rename_file {
   my $self=shift;
   my $feedback=`cd $$self; svn rename @_ 2>&1`;
   unless ($feedback =~ m{\A(?m:^[AD]\s+\S+\n)+\Z}s) {
      None::rename_file($self, @_);
      warn_print( "svn rename @_ failed:\n$feedback\n",
		  "The file has been renamed, but the new name has not been registered with svn.\n",
		  "Please investigate the reason of the failure and execute `svn add $_[-1]` manually.\n",
		  "Note that the old file $_[0] might reappear in the meanwhile with new contents!" );
   }
}

sub delete_file {
   my $self=shift;
   my $feedback=`cd $$self; svn delete @_ 2>&1`;
   unless ($feedback =~ m{\A(?m:^D\s+\S+\n)+\Z}s) {
      None::delete_file($self, @_);
      warn_print( "svn delete @_ failed:\n$feedback\n",
		  "The files have been deleted, but not registered with svn.\n",
		  "Please investigate the reason of the failure and execute `svn delete @_` manually." );
   }
}

sub set_ignored {
   my $self=shift;
   my %items=map { ("$_\n" => 1) } @_;
   my @ignored;
   foreach (`svn propget svn:ignore $$self 2>/dev/null`) {
      if (length($_)>1) {
	 push @ignored, $_;
	 delete $items{$_};
      }
   }
   if (keys %items) {
      open my $IGN, "| svn propset svn:ignore -F - $$self";
      print $IGN @ignored, keys %items;
      close $IGN;
      if ($?) {
	 warn_print( "Editing svn:ignore of directory $$self failed: svn propedit terminated with error.\n",
		     "Please investigate the reason and add the following items to the property svn:ignore:\n",
		     join(" ", keys %items) );
      }
   }
}

sub check_status {
   my $self=shift;
   my $response=`cd $$self; svn status -u @_ 2>&1`;
   if ($response =~ /^C /) {
      "conflict"
   } elsif ($response =~ /^\s*\* /) {
      "outdated"
   } elsif ($response =~ /^(?:[MA] |Status against revision:)/) {
      ""
   } else {
      $response
   }
}

package __::None;

sub new {
   my ($pkg, $dir)=@_;
   bless \$dir, $pkg;
}

sub make_dir {
   my $self=shift;
   File::Path::mkpath([ map { m{^/} ? $_ : "$$self/$_" } @_ ], 0, 0755);
}

sub copy_file {
   my $self=shift;
   File::Copy::copy(map { m{^/} ? $_ : "$$self/$_" } @_);
}

sub rename_file {
   my $self=shift;
   my ($from, $to)=map { m{^/} ? $_ : "$$self/$_" } @_;
   rename($from, $to) or die "rename @_ in $$self failed: $!\n";
}

sub delete_file {
   my $self=shift;
   unlink(map { m{^/} ? $_ : "$$self/$_" } @_) or die "unlink @_ in $$self failed: $!\n";
}

sub add_file {}
sub set_ignored {}
sub check_status { "" }

1;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
