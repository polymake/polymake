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

require XML::Writer;

package Polymake::Core::Help::HTML;

declare $xhtmlns="http://www.w3.org/1999/xhtml";

sub no_resolve { '#' }

use Polymake::Struct (
   [ new => '' ],
   [ '$writer' => 'undef' ],
   '$buf',
   [ '&resolve_ref' => '\&no_resolve' ],
);

sub new {
   my $self=&_new;
   $self->writer=new XML::Writer(OUTPUT => \($self->buf), NAMESPACES => 1, PREFIX_MAP => { $xhtmlns=>"" },
                                 DATA_MODE => 0, DATA_INDENT => 0, ENCODING => 'utf-8', UNSAFE => 1);
   $self->writer->xmlDecl;
   $self->writer->startTag([ $xhtmlns, "html" ]);
   $self->writer->startTag([ $xhtmlns, "body" ]);
   $self
}

sub text {
   my ($self)=@_;
   if (defined $self->writer) {
      $self->writer->endTag;  # </body>
      $self->writer->endTag;  # </html>
      undef $self->writer;
   }
   $self->buf
}

use Polymake::Struct (
   [ 'alt.constructor' => 'new_fragment' ],
   [ new => '$$' ],
   [ '$writer' => '#1' ],
   [ '$resolve_ref' => '#2' ],
);

sub convertDokuWiki {
   my ($self, $text)=@_;
   while ($text =~ m{\G (.*?) (?: (''|__|\*\*|//) (.*?) \2
                                | \[\[ (.*?) (?: \| (.*?) )? \]\]
                                | $ )
                    }xgs) {
      $self->writer->characters($1) if length($1);
      if (defined $2) {
         my @decor;
         if ($2 eq "''") {
            @decor=('code');
         } elsif ($2 eq "**") {
            @decor=('strong');
         } elsif ($2 eq "//") {
            @decor=('em', class => 'param');
         } else {
            @decor=('em', class => 'u');
         }
         $self->writer->startTag([ $xhtmlns, shift @decor ], @decor);
         convertDokuWiki($self, $3);
         $self->writer->endTag;
      } elsif (defined $4) {
         my ($ref, $text)=($4, $5);
         $ref =~ s/^\s* (.*?) \s*$/$1/x;
         $self->writer->startTag([ $xhtmlns, 'a' ], href => $self->resolve_ref->($ref));
         if (defined $text) {
            convertDokuWiki($self, $text);
         } else {
            $self->writer->characters($ref);
         }
         $self->writer->endTag;
      }
   }
}


sub writeText {
   my ($self, $text)=@_;
   my $verbatim=0;
   foreach (split m{(&$id_re; | </? (?: su[bp] ) > | </span> | < (?: span (?:\s+ $id_re = ".*")?) > )}ox, $text) {
      if ($verbatim) {
         if (substr($_,0,1) eq "&") {
            $self->writer->getOutput->print($_);
         } elsif (substr($_,1,1) eq "/") {
            $self->writer->endTag;
         } else {
            $_ =~ /<( $id_re )(?:\s? ( $id_re )="(.*)" )?>/xo;
            #/$id_re/o;
            if (defined($2)){
                $self->writer->startTag([ $xhtmlns, $1 ], $2=>$3);
            } else {
                $self->writer->startTag([ $xhtmlns, $1 ]);
            }
         }
      } elsif (length) {
         convertDokuWiki($self, $_);
      }
      $verbatim^=1;
   }
}

sub writeBlock {
   my ($self, $text, $para_tag)=@_;
   my $save_mode=$self->writer->getDataMode()
     and $self->writer->setDataMode(0);
   while ($text =~ /\G(.*?)(?:\n\n|\Z|((?=^ *\t)))/msg) {
      if (length($1)>1) {
         $self->writer->startTag([ $xhtmlns, $para_tag ]);
         writeText($self, $1);
         $self->writer->endTag;
      }
      if ($para_tag eq "p" && defined $2) {
         $self->writer->startTag([ $xhtmlns, "blockquote" ]);
         while ($text =~ /\G^ *\t(.*)\n/mgc) {
            $self->writer->startTag([ $xhtmlns, "div" ]);
            writeText($self, $1);
            $self->writer->endTag;
         }
         $self->writer->endTag;
      }
   }
   $save_mode and $self->writer->setDataMode($save_mode);
}

sub add_separator {
   my ($self)=@_;
   $self->writer->emptyTag([ $xhtmlns, "hr" ]);
}

sub header {
   writeBlock(@_, "p");
}

sub description {
   writeBlock(@_, "p");
}

sub specialized {
   writeBlock(@_, "div");
}

sub depends {
   my ($self, $depends)=@_;
   writeBlock($self, "Depends on: $depends", "div");
}

sub example {
   my ($self, $example)=@_;
   my $save_mode=$self->writer->getDataMode()
     and $self->writer->setDataMode(0);
   my @buffer;
   foreach my $line (split /\n/, $example) {
      # match > or | at the beginning but check > with lookahead to keep it in $'
      if ($line =~ /^\s* (?: \| | (?= > ) ) /x) {
         $line = $';
         # escapes for html output, we dont want to parse the code sections as descriptions
         # due to stuff like [[1,2,3]] and to avoid [[]] around all types,...
         $line =~ s/&/&amp;/g;
         $line =~ s/</&lt;/g;
         $line =~ s/>/&gt;/g;
         if (@buffer > 0) {
            writeText($self, join("\n", @buffer));
            @buffer = ();
         }
         # empty code tags cause display problems
         $line = " " if ($line eq "");
         $self->writer->startTag([ $xhtmlns, "code" ]);
         $self->writer->getOutput->print($line);
         $self->writer->endTag;
      } else {
         push @buffer, $line;
      }
   }
   writeText($self, join("\n", @buffer)) if (@buffer > 0);
   $save_mode and $self->writer->setDataMode($save_mode);
}

sub examples {
   my ($self, $examples) = @_;
   writeBlock($self, @$examples > 1 ? "Examples:" : "Example:", "p");
   foreach (@$examples) {
      $self->writer->startTag([ $xhtmlns, "p" ]);
      example($self, $_->body);
      $self->writer->endTag;
   }
}

sub function_full {
   my ($self, $text)=@_;
   writeBlock($self, $text, "p");
   $self->writer->startTag([ $xhtmlns, "table" ]);
}

sub function_brief {
   my ($self, @options)=@_;
   if (@options) {
      writeBlock($self, "Options: ".join(" ", @options), "div");
   }
}

sub writeTableHeader {
   my ($self, $text)=@_;
   $self->writer->startTag([ $xhtmlns, "tr" ]);
   $self->writer->startTag([ $xhtmlns, "th" ], colspan => 3, align => 'left');
   $self->writer->characters($text);
   $self->writer->endTag;
   $self->writer->endTag;
}

sub writeTableRow {
   my ($self, $type, $name, $text, $values)=@_;
   $self->writer->startTag([ $xhtmlns, "tr" ], valign => 'top');
   $self->writer->dataElement([ $xhtmlns, "td" ], $type);
   if (defined($name)) {
      $self->writer->dataElement([ $xhtmlns, "td" ], $name);
   } else {
      $self->writer->emptyTag([ $xhtmlns, "td" ]);
   }
   $self->writer->startTag([ $xhtmlns, "td" ]);
   writeBlock($self, $text, "div");
   if (defined($values)) {
      write_possible_values($self, $values);
   }
   $self->writer->endTag;
   $self->writer->endTag;
}

sub write_possible_values {
   my ($self, $values)=@_;
   writeBlock($self, "Possible values:", "div");
   my $save_mode=$self->writer->getDataMode()
     and $self->writer->setDataMode(0);
   $self->writer->startTag([ $xhtmlns, "dl" ]);
   foreach my $value (@$values) {
      $self->writer->startTag([ $xhtmlns, "dt" ]);
      writeText($self, $value->value);
      $self->writer->endTag;
      $self->writer->startTag([ $xhtmlns, "dd" ]);
      writeText($self, $value->text);
      $self->writer->endTag;
   }
   $self->writer->endTag;
   $save_mode and $self->writer->setDataMode($save_mode);
}

sub type_params {
   my $self = shift;
   writeTableHeader($self, "Type Parameters:");
   foreach (@_) {
      writeTableRow($self, $_->name, "", $_->text);
   }
}

sub function_args {
   my ($self, $args) = @_;
   writeTableHeader($self, "Arguments:");
   foreach (@$args) {
      writeTableRow($self, $_->type, $_->name, $_->text, $_->values);
   }
}

sub function_options {
   my ($self, $comment, @options) = @_;
   writeTableHeader($self, "Options:");
   if (length($comment)) {
      $self->writer->startTag([ $xhtmlns, "tr" ]);
      $self->writer->startTag([ $xhtmlns, "td" ], colspan => 3, align => 'left');
      writeBlock($self, $comment, "div");
      $self->writer->endTag;
      $self->writer->endTag;
   }
   foreach (@options) {
      writeTableRow($self, $_->name." =>", $_->type, $_->text);
   }
}

sub function_return {
   my ($self, $return) = @_;
   if (defined($return)) {
      writeTableHeader($self, "Returns:");
      writeTableRow($self, $return->type, "", $return->text // "");
   }
   $self->writer->endTag;   # close the table with arguments
}

sub topics_keys {
   my ($self) = @_;
   $self->writer->startTag([ $xhtmlns, "table" ]);
   foreach (@_) {
      writeTableRow($self, $_->name." =>", $_->type, $_->text);
   }
   $self->writer->endTag;
}

1


# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
