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

require Polymake::Tempfile;

package Polymake::TempTextFile;

use Polymake::Struct (
   [ '$file' => 'new Tempfile' ],
   [ '$handle' => 'undef' ],
);

sub new {
   my $self = &_new;
   open my $handle, ">", ($self->file . ".txt")
     or die "can't create temporary file: $!\n";
   $self->handle = $handle;
   $self;
}

use overload '*{}' => \&handle,
             '""' => sub {
   my ($self) = @_;
   if (defined($self->handle)) {
     close $self->handle;
     undef $self->handle;
   }
   $self->file . ".txt"
};


1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
