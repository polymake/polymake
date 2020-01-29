#  Copyright (c) 1997-2020
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

require Polymake::Background;

package Polymake::Background::Viewer;

use Polymake::Struct (
   [ '$graphics' => 'undef' ],
   [ '$tempfile' => 'new Tempfile' ],
);

sub run {
   my ($self, $synchronous)=@_;
   my $outfile=$self->tempfile . $self->file_suffix;
   open my $out, ">", $outfile or die "can't create the temporary file $outfile: $!\n";
   print $out $self->graphics->toString or die "can't write to the temporary file $outfile: $!\n";
   close $out;
   my $command=$self->command($outfile);
   $synchronous
   ? system($command)
   : new Process({ CLEANUP => $self->tempfile }, $command);
   undef $self->tempfile;
}

###########################################################################################
#
#  Viewer not capable to show multiple pages in one process

package Polymake::SimpleViewer;

use Polymake::Struct [ '@ISA' => 'Background::Viewer' ];

sub new_drawing {
   my ($self, $title)=@_;
   if ($self == $self->graphics) {
      # it's a File::Writer
      $self->title=$title;
   } else {
      if ($self->graphics) {
	 run($self);
	 $self->tempfile=new Tempfile;
      }
      (my $pkg=ref($self)) =~ s/^(\w+)::.*/$1\::File/;
      $self->graphics=$pkg->new($title);
   }
   $self
}


1

# Local Variables:
# c-basic-offset:3
# End:
