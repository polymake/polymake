#  Copyright (c) 1997-2022
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

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);

require Scalar::Util;

package Polymake::Test::Stream;

use Polymake::Struct (
   [ '@ISA' => 'Case' ],
   '$buffer',
   '$handle',
   [ '$filters' => '#%', default => 'undef' ],
);

sub new {
   my $self=&Case::new;
   open my $h, ">", \($self->buffer);
   $self->handle=$h;
   $self;
}

sub execute {
   my ($self) = @_;
   close $self->handle;
   my (@expected, @gotten);
   local $_;
   my $expected_file = $self->subgroup->group->env->find_data_file($self->name."$_.OK");
   open my $expected, "<", $expected_file
     or die "can't read $expected_file: $!\n";

   while (<$expected>) {
      if (defined(my $filters = $self->filters)) {
         foreach my $filter (@$filters) {
            &$filter;
            defined($_) or last;
         }
      }
      if (defined($_)) {
         push @expected, Scalar::Util::dualvar($., $_);
      }
   }

   foreach (split /(?<=\n)/, $self->buffer) {
      if (defined(my $filters = $self->filters)) {
         foreach my $filter (@$filters) {
            &$filter;
            defined($_) or last;
         }
      }
      if (defined($_)) {
         push @gotten, $_;
      }
   }

   my $lineno;
   my $gotten_lineno = -1;
   $self->fail_log = "";
   foreach my $expected_line (@expected) {
      $lineno = $expected_line+0;
      if (++$gotten_lineno > $#gotten) {
         $self->fail_log .= "line $lineno:\n".
                            "expected: $expected_line\n".
                            "     got: __END__\n";
         last;
      }
      if ($expected_line ne $gotten[$gotten_lineno]) {
         $self->fail_log .= "line $lineno:\n".
                            "expected: $expected_line\n".
                            "     got: $gotten[$gotten_lineno]\n";
      }
   }
   if (++$gotten_lineno <= $#gotten) {
      ++$lineno;
      $self->fail_log .= "line $lineno:\n".
                         "expected: __END__\n".
                         "     got: $gotten[$gotten_lineno]\n";
   }
   length($self->fail_log) == 0
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
