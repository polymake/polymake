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

use Polymake::Core::ShellMock;
require Polymake::Core::HelpAsHTML;

package Jupyter::CompletionWrapper;

use Polymake::Struct(
   [ '@ISA' => 'Polymake::Core::Shell::Mock' ],
);

sub instance {
  state $inst=new CompletionWrapper();
}

sub try_filename_completion {
  my ($self, $word, $quote, $prefix, $dir_cmd)=@_;
  my @files = (<$prefix*>);
  if ($prefix =~ m|^~/|) {
    map {$_ =~ s|^$ENV{HOME}|~|} @files;
  }
  Core::Shell::postprocess_file_list(@_, \@files);
}

sub context_help {
  my ($self, $string)=@_;
  $self->Attribs->{line_buffer}=$string;
  $self->Attribs->{point}=length($string);
  my $pos=prepare_partial_input($self);
  my @topics;
  while ($pos >= 0 && fetch_help_topics($self, $pos)) {
    last if (@topics=grep { $_->annex->{display} !~ /^\s*noshow\s*$/ } @{$self->help_topics});
    $pos=pos($self->partial_input)-1;
  }
  @topics
}

package Jupyter;

sub tab_completion {
   my ($code)=@_;
   my $cpw=CompletionWrapper::instance();
   $cpw->complete($code);
   my $completions=$cpw->completion_words;
   my $overlap;
   if (@$completions > 0) {
      my $partial_match=$completions->[0];
      my $l=length($partial_match);
      $partial_match =~ s/(.)/(?:$1/g;
      $partial_match .= '|$)' x $l;
      $partial_match .= '$';
      if ($code =~ $partial_match) {
         $overlap=length($&);
      } else {
         $overlap = 0;
      }
      if (@$completions==1) {
         my $append_char = $cpw->Attribs->{completion_append_character};
         # suppress ), ], ", and ' as append char as those are inserted by jupyter automatically
		 $completions->[0].= $append_char unless $append_char =~ /[])"']/ ;
         # empty append_char, otherwise, this will also be added to any subsequent calls
         $append_char = $cpw->Attribs->{completion_append_character} = "";
      }
   }
   return join "###", $overlap, @{$completions};
}

sub context_help {
   my ($code, $detail_level, $format)=@_;
   my $writer= $format eq "html" ? new Core::HelpAsHTML() : new Core::HelpAsPlainText();
   foreach (CompletionWrapper::instance()->context_help($code)) {
      if (is_object($_)) {
         $_->write_text($writer, $detail_level)
      } else {
         $_->($writer, $detail_level);
      }
   }
   $writer->text;
}

1

# Local Variables:
# mode: perl
# cperl-indent-level:3
# End:
