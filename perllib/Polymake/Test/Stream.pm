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

use strict;
use namespaces;

require Scalar::Util;

package Polymake::Test::Stream;

use Polymake::Struct (
   [ '@ISA' => 'Case' ],
   [ new => '$$%' ],
   '$buffer',
   '$handle',
   [ '$filters' => '#2' ],
);

sub new {
   my $self=&Case::new;
   open my $h, ">", \($self->buffer);
   $self->handle=$h;
   $self;
}

sub execute {
   my ($self)=@_;
   close $self->handle;
   my (@expected, @gotten);
   local $_;
   my $expected_file=find_matching_file($self->name.".OK");
   open my $expected, "<", $expected_file
     or die "can't read $expected_file: $!\n";

   while (<$expected>) {
      foreach my $filter (@{$self->filters}) {
         &$filter;
         defined($_) or last;
      }
      if (defined $_) {
         push @expected, Scalar::Util::dualvar($., $_);
      }
   }
   foreach (split /(?<=\n)/, $self->buffer) {
      foreach my $filter (@{$self->filters}) {
         &$filter;
         defined($_) or last;
      }
      if (defined $_) {
         push @gotten, $_;
      }
   }

   my $success=1;
   my $lineno;
   foreach my $expected_line (@expected) {
      my $gotten_line=shift @gotten;
      $lineno=$expected_line+0;
      if ($expected_line ne $gotten_line) {
         $self->fail_log.="line $lineno:\n".
                          "expected: $expected_line\n".
                          "     got: ".($gotten_line // "__END__")."\n";
         $success=0;
         last unless @gotten;
      }
   }
   if (@gotten) {
      ++$lineno;
      $self->fail_log.="line $lineno:\n".
                       "expected: __END__\n".
                       "     got: $gotten[0]\n";
      $success=0;
   }
   $success;
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
