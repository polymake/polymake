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

package SplitsTree::File;

use Polymake::Struct (
   [ '$title' => 'undef' ],
   '@metric',
   '$taxa',
);

sub toString {
   my ($self)=@_;
   my $title=$self->title || "unnamed";
   my $ntax=@{$self->metric};
   my $text = <<".";
#NEXUS

[! $title ]

BEGIN taxa;
    DIMENSIONS ntax=$ntax;
TAXLABELS
.
   map { $text .= "    ".$self->taxa->($_)."\n" } 0..$ntax-1;
   $text .= <<".";
;
END;

BEGIN distances;
    DIMENSIONS ntax=$ntax;
    FORMAT
        triangle=LOWER
        diagonal
        labels
        missing=?
    ;
    MATRIX
.
    for (my $n=0; $n < $ntax; ++$n) {
       $text .= "    ".$self->taxa->($n)."  ".Visual::print_coords($self->metric->[$n]->slice(0,$n+1))."\n";
    }
    $text .= << ".";
    ;
END;
.
}


1

# Local Variables:
# c-basic-offset:3
# End:
