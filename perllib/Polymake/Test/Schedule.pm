#  Copyright (c) 1997-2019
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

package Polymake::Test::Schedule;

use Polymake::Struct (
   [ '@ISA' => 'Case' ],
   [ new => '$$$%' ],
   [ '$expected' => '#2' ],
   [ '$rule_chain' => '#3' ],
);

sub execute {
   my ($self)=@_;
   if (defined $self->rule_chain) {
      my $got=join("\n", $self->rule_chain->list)."\n";
      unless (defined $self->expected) {
         $self->fail_log="schedule computed although nothing expected:\n$got\n";
         return 0;
      }
      foreach ($self->expected, $got) {
         s/^\s+//mg;
         s/[ \t]+(?=[:,]|$)//mg;
         s/[ \t]{2,}/ /g;
      }
      my $ok;
      foreach (split /^-{3,}.*\n/m, $self->expected) {
         $ok=1, last if $_ eq $got;
         # the ordering of independent rules may vary
         foreach my $rule (split /\n/, $got) {
            s/^\Q$rule\E\n//m  or  $_="!", last;
         }
         $ok=1, last if $_ !~ /\S/;
      }
      unless ($ok) {
         $self->fail_log="different schedules:\n".
                         "expected:\n".$self->expected."\ngot:\n".$got;
         return 0;
      }
   } elsif (defined $self->expected) {
      $self->fail_log="no schedule computed\n";
      return 0;
   }
   1
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
