#  Copyright (c) 1997-2016
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

require Polymake::Core::Shell;

use strict;
use namespaces;

# Mock the interactive shell object
# as far as needed to access the tab completion and F1 in isolation.
# CAUTION: filename completion is not supported.

package Polymake::Core::Shell::Mock;

use Polymake::Struct (
   '%Attribs',
   '@completion_words | help_topics',
   '$partial_input',
   [ '$line_cnt' => '1' ],
);

sub term { shift }

sub complete {
   my ($self, $string)=@_;
   $self->Attribs->{line_buffer}=$string;
   $self->Attribs->{point}=length($string);
   @{$self->completion_words}=();
   my ($word)= $string =~ m/(?:^|[-\s"\\'`\@\$><=;|&{(+*\/.\%})\[\]?!,])([\w:]*)$/;
   Shell::all_completions($self, $word);
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
