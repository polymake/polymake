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

require Polymake::Core::Shell;

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);

# A limited replacement for Shell in scenarions without direct access to a terminal.
# Mostly used for tests and connections to alternative front-ends like Jupyter.

package Polymake::Core::Shell::Mock;

use Polymake::Struct (
   '%Attribs',
   '@completion_proposals | help_topics',
   '$partial_input',
   '$line_cnt',
   '$completion_offset',
);

sub term { shift }
sub interactive { 0 }

sub complete {
   my ($self, $string)=@_;
   $self->Attribs->{line_buffer}=$string;
   $self->Attribs->{point}=length($string);
   $self->Attribs->{filename_completion_function}=\&filename_completion;
   $self->line_cnt=1;
   @{$self->completion_proposals}=();
   $self->completion_offset=0;
   Shell::all_completions($self);
}

sub completion_matches : method {
   my ($self, $prefix, $func)=@_;
   my @result=$func->($self, $prefix);
   if (@result > 1) {
      # true readline function inserts the common prefix here, but we never use it,
      # so it can have an arbitrary value
      unshift @result, undef;
   }
   @result;
}

sub filename_completion {
   my ($self, $prefix)=@_;
   my @files = glob("$prefix*");
   if ($prefix =~ m|^~/|) {
      $_ =~ s|^$ENV{HOME}|~| for @files;
   }
   @files
}

sub context_help {
   my ($self, $string, $pos)=@_;
   $self->partial_input=$string;
   while ($pos >= 0 && fetch_help_topics($self, $pos)) {
      $_[2]=$pos=pos($self->partial_input)-1;
      $self->help_topics=[ grep { $_->annex->{display} !~ /^\s*noshow\s*$/ } @{$self->help_topics} ];
      return 1 if @{$self->help_topics};
   }
}

sub process_input {
   my ($self, $input)=@_;
   local unshift @INC, $self;
   $self->partial_input=$input;
   { local $Scope=new Scope(); package Polymake::User; do "input:"; 1; }
   if ($self->line_cnt==-1) {
      # just an incomplete input
      $@="";
   } else {
      if ($self->line_cnt==-3) {
         $@=delete $self->Attribs->{parse_errors};
      }
      $@ =~ s{ at input line \d+[.,]}{}g;
   }
   return $self->line_cnt>=2;
}

sub Polymake::Core::Shell::Mock::INC {
   if ($_[1] eq "input:") {
      my $self=$_[0];
      $self->line_cnt=0;
      delete $self->Attribs->{parse_errors};
      (input_preamble(1), \&get_line, $self)
   } else {
      $User::application->INC($_[1]);
   }
}

# as a side effect, sets line_cnt depending on the syntax correctness of the input:
# >=2 when at least one non-empty input line and no syntax errors detected
# -1 for syntactically correct but incomplete input
# -2 for syntax errors reported in $@
# -3 for syntax errors preserved in Attribs->{parse_errors}

sub get_line {
   my ($l, $self)=@_;
   if ($self->line_cnt==0) {
      inject_error_preserving_source_filter();
   }
   ++$self->line_cnt;
   $self->partial_input =~ s/\A.*(?:\n|\Z)//m;
   if (length($&) != 0) {
      $_ .= $&;
   } else {
      remove_error_preserving_source_filter();
      if (my $err=get_preserved_errors()) {
         $self->Attribs->{parse_errors}=$err;
         $self->line_cnt=-3;
      } elsif (($l=line_continued()) != 0) {
         $self->line_cnt= $l<0 ? -2 : -1;
      }
   }
   return length;
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
