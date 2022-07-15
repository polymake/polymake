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

package Polymake::Core::Help::PlainText;

use Polymake::Struct (
   [ new => ';$' ],
   '$text',
   [ '$decorate' => '#1 // 1' ],
);

*clean_text=\&InteractiveCommands::clean_text;

sub underline {
   my ($self, $text)=@_;
   $self->decorate ? InteractiveCommands::underline($text) : $text
}

sub add_separator {
   my ($self, $sep)=@_;
   $self->text .= $sep;
}

sub description {
   my ($self, $text)=@_;
   if (length($text)) {
      clean_text($text);
   }
   $self->text .= $text;
}

sub depends {
   my ($self, $depends)=@_;
   clean_text($depends);
   $self->text .= "\nDepends on:\n  $depends\n";
}

sub examples {
   my ($self, $examples) = @_;
   $self->text .= @$examples > 1 ? "\nExamples:\n\n" : "\nExample:\n\n";
   my $prefix = @$examples > 1 ? "*) " : "   ";
   foreach (@$examples) {
      my $i = 0;
      foreach my $line (split /\n/, $_->body) {
         $self->text .= $i++ ? "   " : $prefix ;
         if ($line =~ s/^\s*\|\s*//) {
            $self->text .= "  $line\n";
         } elsif ($line =~ s/^\s*>\s*//) {
            $self->text .= "> $line\n";
         } else {
            clean_text($line);
            $self->text .= $line;
         }
      }
   }
}

sub header {
   my ($self, $text)=@_;
   $self->text .= $text;
}

sub specialized {
   my ($self, $text)=@_;
   clean_text($text);
   $self->text .= $text;
}

sub function_full {
   my ($self, $text)=@_;
   clean_text($text);
   $self->text .= "\n" . $text;
}

sub function_brief {
   my ($self, @options)=@_;
   if (@options) {
      $self->text .= "\nOptions: " . join(" ", @options);
   }
   $self->text .= "\n\n";
}

sub type_params {
   my $self = shift;
   $self->text .= "\nType Parameters:\n";
   foreach (@_) {
      my $comment = $_->text;
      clean_text($comment);
      $comment =~ s/\n[ \t]*(?=\S)/\n    /g;
      $self->text .= "  " . underline($self, $_->name) . " $comment";
   }
}

sub function_args {
   my ($self, $args) = @_;
   $self->text .= "\nArguments:\n";
   foreach (@$args) {
      my $comment = $_->text;
      clean_text($comment);
      $comment =~ s/\n[ \t]*(?=\S)/\n    /g;
      my $type = $_->type;
      $type =~ s/^__(\w+)__$/$1/;
      my ($name) = $_->name =~ /^($id_re)/;
      $self->text .= "  $type " . underline($self, $name) . " $comment";
      if (defined(my $value_list = $_->values)) {
         $self->text .= "    Possible values:\n";
         foreach my $value (@$value_list) {
            $comment = $value->text;
            clean_text($comment);
            $self->text .= "      " . $value->value . " : $comment";
         }
      }
   }
}

sub function_options {
   my ($self, $comment) = splice @_, 0, 2;
   clean_text($comment);
   $self->text .= "\nOptions: " . (length($comment) ? $comment : "\n") . key_descriptions($self, @_)
}

sub function_return {
   my ($self, $return) = @_;
   if (defined $return) {
      my $text .= $return->text;
      clean_text($text);
      $text =~ s/\n[ \t]*(?=\S)/\n    /g;
      $self->text .= "\nReturns " . $return->type . " $text";
   }
}

sub key_descriptions {
   my $self = shift;
   join("", map {
      my $comment = $_->text;
      clean_text($comment);
      $comment =~ s/\n[ \t]*(?=\S)/\n    /g;
      "  " . underline($self, $_->name) . " => " . $_->type . " $comment";
   } @_)
}

sub topics_keys {
   my $self = $_[0];
   $self->text .= "\n" . &key_descriptions
}

1


# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
