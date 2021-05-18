#  Copyright (c) 1997-2021
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

use Polymake::Core::Shell::Mock;

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);
use feature 'state';

package Polymake::Test::Shell;

use Polymake::Struct (
   [ '@ISA' => 'Polymake::Core::Shell::Mock' ],
);

sub instance {
   state $inst = new Shell();
}

sub complete {
   &Mock::complete;
   my ($self) = @_;
   ($self->completion_offset, $self->completion_append_character, @{$self->completion_proposals})
}

sub context_help {
   my ($self, $string) = @_;
   my $pos = length($string);
   my @headers;
   while (Mock::context_help($self, $string, $pos)) {
      push @headers, map { $_->full_path } @{$self->help_topics};
   }
   @headers
}

##################################################################
package Polymake::Test::Shell::Completion;

use Polymake::Struct(
   [ '@ISA' => 'Case' ],
   [ new => '$$$@' ],
   [ '$max_exec_time' => '0' ],
   [ '$partial_input' => '#2' ],
   [ '$expected_offset' => '#3' ],
   [ '$expected_words' => '@' ],
   [ '$expected_append' => 'undef' ],
   '@gotten_words',
   '$gotten_offset',
   '$gotten_append',
);

sub new {
   my $self=&Case::new;
   if (@{$self->expected_words} > 1 && length($self->expected_words->[-1]) == 1) {
      $self->expected_append = pop @{$self->expected_words};
   }
   ($self->gotten_offset, $self->gotten_append, @{$self->gotten_words}) = instance()->complete($self->partial_input);
   $self
}

sub execute {
   my ($self) = @_;
   my $eq = $self->expected_words->[-1] eq "..."
            ? @{$self->expected_words}-1 == equal_string_list_prefixes($self->expected_words, $self->gotten_words)
            : equal_nested_lists($self->expected_words, $self->gotten_words);
   my $eq_append = !defined($self->expected_append) || $self->expected_append eq $self->gotten_append;
   unless ($eq && $eq_append && $self->expected_offset==$self->gotten_offset) {
      unless ($eq) {
         $self->fail_log .= "expected:\n@{$self->expected_words}\n".
                            "     got:\n@{$self->gotten_words}\n";
      }
      unless ($self->expected_offset == $self->gotten_offset) {
         $self->fail_log .= "expected offset:".$self->expected_offset."\n".
                            "     got offset:".$self->gotten_offset."\n";
      }
      unless ($eq_append) {
         $self->fail_log .= "expected append char:".$self->expected_append."\n".
                            "     got append char:".$self->gotten_append."\n";
      }
      0;
   }
}

##################################################################
package Polymake::Test::Shell::ContextHelp;

use Polymake::Struct(
   [ '@ISA' => 'Case' ],
   [ new => '$$@' ],
   [ '$max_exec_time' => '0' ],
   [ '$partial_input' => '#2' ],
   [ '$expected_headers' => '@' ],
   '@gotten_headers',
);

sub new {
   my $self = &Case::new;
   @{$self->gotten_headers} = instance()->context_help($self->partial_input);
   $self
}

sub execute {
   my ($self) = @_;
   unless (equal_nested_lists($self->expected_headers, $self->gotten_headers)) {
      $self->fail_log = "expected:\n".
                        join("", (map { "  $_\n" } @{$self->expected_headers})).
                        "     got:\n".
                        join("", (map { "  $_\n" } @{$self->gotten_headers}));
      0;
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
