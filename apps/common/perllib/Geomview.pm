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

use Visual::Drawing;

package Geomview::File;
use Polymake::Struct (
   '@geom',
);

my $setup=<<'.';
(bbox-draw "world" no)
.

sub toString {
   my ($self)=@_;
   $setup . join("", map { $_->toString } @{$self->geom});
}

package Geomview::geom;
use Polymake::Struct (
   [ '@ISA' => 'Visual::Drawing' ],
);

sub header {
   my ($self)=@_;
   my $id=$self->title;
   if (length($id)) {
      $id =~ s/\s/_/g;
   } else {
      $id="unnamed";
   }
   "(geometry $id { LIST\n";
}

sub trailer {
   "} # end LIST\n)\n";
}

package Geomview::element;

sub new {
   my $class=shift;
   my $self=bless { @_ }, $class;
   $self;
}

sub name : lvalue { (shift)->{name} }

sub toString {
   my ($self)=@_;
   "{" .
   ( defined($self->{appearance}) && " appearance { $self->{appearance} }\n" ) .
   $self->contents .
   "}\n";
}

sub points {
   my ($self)=@_;
   join("\n", map { Visual::print3dcoords($_) } @{$self->{points}})."\n";
}

sub colored_points {
   my ($self)=@_;
   my ($p, $c)=@$self{qw( points pointColors )};
   my $i=-1;
   is_code($c)
   ? join("", map { Visual::print3dcoords($_) . " " . $c->(++$i)->toFloat . " 0\n" } @$p)
   : do { $c=$c->toFloat; join("", map { Visual::print3dcoords($_) . " $c 0\n" } @$p) }
}

sub facets {
   my ($self)=@_;
   join("", map { scalar(@$_) . " @$_\n" } @{$self->facets})
}

sub colored_facets {
   my ($self)=@_;
   my ($f, $c, $a)=@$self{qw( facets facetColors facetAlpha )};
   my $i=-1;
   $c=$c->toFloat unless is_code($c);
   join("", map { ++$i; scalar(@$_) . " @$_ " .
		        (ref($c) ? $c->($i)->toFloat : $c) .
		        (defined($a) && " " . $a->($i)) . "\n"
		} @$f)
}

sub lines {
   my ($self)=@_;
   my $text="";
   for (my $l=$self->{lines}; $l; ++$l) {
      $text.="2 @$l\n";
   }
   $text
}

sub colored_lines {
   my ($self)=@_;
   my $c=$self->{lineColors};
   $c=$c->toFloat unless is_code($c);
   my $text="";
   for (my $l=$self->{lines}; $l; ++$l) {
      $text.="2 @$l " . (ref($c) ? $c->($l)->toFloat : $c) . "\n";
   }
}

package Geomview::OFF;
@ISA=qw( Geomview::element );

sub dim {
   my ($self)=@_;
   "OFF\n" . scalar(@{$self->{points}}) . " " . scalar(@{$self->{facets}}) . " 0\n"
}

sub contents {
   my ($self)=@_;
   ( defined $self->{pointColors}
     ? "C" . $self->dim . $self->colored_points
     : $self->dim . $self->points ) .
   ( defined $self->{facetColors} ? $self->colored_facets : $self->facets )
}

package Geomview::SKEL;
@ISA=qw( Geomview::element );

sub dim {
   my ($self)=@_;
   "SKEL\n" . scalar(@{$self->{points}}) . " " . $self->{n_lines} . "\n"
}

sub contents {
   my ($self)=@_;
   $self->dim . $self->points .
   ( defined $self->{lineColors} ? $self->colored_lines : $self->lines )
}

1

# Local Variables:
# c-basic-offset:3
# End:
