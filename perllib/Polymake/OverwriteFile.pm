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

package Polymake::OverwriteFile;
use Symbol;

use Polymake::Struct (
   [ new => '$;$' ],
   [ '$dst' => '#1' ],
   '$tempname',
   [ '$handle' => 'undef' ],
);

# constructor: new OverwriteFile('path' [, 'layers'])
# 'layers' can be something like ':utf8' for proper storing of XML files.

sub new {
   my $self=&_new;
   my ($dir)= $self->dst =~ $directory_re;
   $dir .= "/" if $dir;
   $self->tempname=$dir . Tempfile->unique_name;
   open $self->handle, ">$_[1]", $self->tempname
      or die "can't create a new copy of ", $self->dst, ": $!\n";
   ($self->handle, $self);
}

sub DESTROY {
   my ($self)=@_;
   if (defined(fileno($self->handle))) {
      close($self->handle);
      if ($@) {
         if ($DebugLevel) {
            warn_print( "incomplete new copy of ", $self->dst, " kept as ", $self->tempname );
         } else {
            unlink $self->tempname;
         }
      } else {
         unlink $self->tempname;
         Carp::croak( "creation of new copy of ", $self->dst, " is not properly completed" );
      }
   } elsif (!rename($self->tempname, $self->dst)) {
      warn_print( "could not rename temporary file ", $self->tempname, " to destination ", $self->dst, ": $!" );
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
