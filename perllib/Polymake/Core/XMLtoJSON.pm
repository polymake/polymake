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
use feature 'state';

# legacy XML namespace
my $pmns="http://www.math.tu-berlin.de/polymake/#3";

package Polymake::Core::XMLtoJSON::Handler;

use Polymake::Struct (
   '@cur_value',
   '@cur_property',
   [ '$top' => 'undef' ],
   '%by_id',
);

sub create_top {
   my ($self, $attrs) = @_;
   $self->top = { _id => $attrs->{'{}name'}->{Value},
                  _type => $attrs->{'{}type'}->{Value},
                  _ns => Serializer::default_ns($attrs->{'{}version'}->{Value}) };
   if (defined(my $ext = $attrs->{'{}ext'})) {
      $self->top->{_ext} = add_ext($self, $ext->{Value});
   }
}

sub add_value {
   my ($self, $value, $index, $tag) = @_;
   my $outer_value = $self->cur_value->[-1];
   if (is_string($value) && $tag ne 'e') {
      my @words;
      while ($value =~ /(?: $quoted_re | (?'word' \S+))/xg) {
         push @words, $+{quoted} // $+{word};
      }
      # { } signifies an empty vector embedded in a sparse matrix
      $value = @words || $tag ne 'v' || !is_integer($self->cur_property->[-1]) ? \@words : { };
   }
   if (is_string($outer_value)) {
      if ($outer_value =~ /\S/) {
         die "unexpected characters before <$tag>\n";
      }
      $outer_value = $self->cur_value->[-1] = defined($index) ? { } : [ ];
   }
   if (defined($index)) {
      if (is_array($outer_value)) {
         if (@$outer_value) {
            die "unexpected attribute i\n";
         }
         $outer_value = $self->cur_value->[-1] = { };
      }
      $outer_value->{$index->{Value}} = $value;
   } elsif (is_array($outer_value)) {
      push @$outer_value, $value;
      $value
   } elsif (!defined($outer_value)) {
      $self->cur_value->[-1] = $value;
   } else {
      die "unexpected <$tag>\n";
   }
}

sub start_property {
   my ($self, $attrs) = @_;
   my ($name, $type, $value, $ext, $construct) = @$attrs{'{}name', '{}type', '{}value', '{}ext', '{}construct'};
   $name = $name->{Value};
   $type &&= $type->{Value};
   $value &&= $value->{Value};
   $ext &&= add_ext($self, $ext->{Value});
   if ($type eq "text" && !defined($value)) {
      undef $type;
      $value = "";
   }
   push @{$self->cur_property}, [ $name, $type, $ext, defined($construct) ? ($construct->{Value}) : () ];
   push @{$self->cur_value}, $value;
}

sub add_ext {
   my ($self, $ext_attr) = @_;
   my @exts;
   while ($ext_attr =~ /\G (\d+) (?: = ([^\s\#]+) (?: \#([\d.]+))? )? (?:\s+|$)/xgc) {
      my ($index, $URI, $version) = ($1, $2, $3);
      push @exts, $index;
      if (defined($URI)) {
         $self->top->{_ns}->{polymake}->[2 + $index] = [ $URI, $version ];
      }
   }
   \@exts
}

my %start_element = (
   object => sub {
      my ($self, $attrs) = @_;
      if (defined($self->top)) {
         # subobject
         my $obj = { };
         my ($name, $type, $ext) = @$attrs{'{}name', '{}type', '{}ext'};
         if (defined($type)) {
            $obj->{_type} = $type->{Value};
         }
         if (defined($name)) {
            $obj->{_id} = $name->{Value};
         }
         if (defined($ext)) {
            $obj->{_ext} = add_ext($self, $ext->{Value});
         }
         push @{$self->cur_value}, $obj;
      } else {
         # top-level object
         &create_top;
         push @{$self->cur_value}, $self->top;
      }
   },

   data => sub {
      my ($self, $attrs) = @_;
      if (defined($self->top)) {
         die "unexpected <data>\n";
      }
      &create_top;
      my $value = $attrs->{'{}value'};
      push @{$self->cur_value}, $value && $value->{Value};
   },

   property => \&start_property,

   attachment => \&start_property,

   credit => sub {
      my ($self, $attrs) = @_;
      push @{$self->cur_property}, $attrs->{'{}product'}->{Value};
      push @{$self->cur_value}, "";
   },

   description => sub {
      my ($self, $attrs) = @_;
      push @{$self->cur_value}, "";
   },

   m => sub {
      my ($self, $attrs) = @_;
      my ($cols, $dim) = @$attrs{'{}cols', '{}dim'};
      if (defined($dim)) {
         push @{$self->cur_value}, { Serializer::Sparse::dim_key() => 0 + $dim->{Value} };
         push @{$self->cur_property}, undef;
      } else {
         push @{$self->cur_value}, [ ];
         push @{$self->cur_property}, $cols && 0 + $cols->{Value};
      }
   },

   t => sub {
      my ($self, $attrs) = @_;
      push @{$self->cur_value}, "";
      push @{$self->cur_property}, [ $attrs->{'{}i'}, $attrs->{'{}id'} ];
   },

   v => sub {
      my ($self, $attrs) = @_;
      my ($index, $dim) = @$attrs{'{}i', '{}dim'};
      push @{$self->cur_value}, defined($dim) ? { Serializer::Sparse::dim_key() => 0 + $dim->{Value} } : "";
      push @{$self->cur_property}, $index;
   },

   e => sub {
      my ($self, $attrs) = @_;
      push @{$self->cur_value}, "";
      push @{$self->cur_property}, $attrs->{'{}i'};
   },

   r => sub {
      my ($self, $attrs) = @_;
      my $id = $attrs->{'{}id'};
      my $value = $self->by_id->{$id ? $id->{Value} : "1"} //
        die "unexpected <r> or invalid id attribute\n";
      add_value($self, $value, undef, 'e');
   },
);

my %end_element = (
   object => sub {
      my ($self) = @_;
      my $obj = pop @{$self->cur_value};
      if ($obj != $self->top) {
         my $outer_value = $self->cur_value->[-1];
         if (is_array($outer_value)) {
            # third or later multiple subobject or an element of a subobject array
            push @$outer_value, $obj;
         } elsif (is_hash($outer_value)) {
            # second multiple subobject
            $self->cur_value->[-1] = [ $outer_value, $obj ];
         } elsif (!defined($outer_value)) {
            # first or single subobject
            $self->cur_value->[-1] = $obj;
         } else {
            die "unexpected <object>\n";
         }
      }
   },

   data => sub {
      my ($self) = @_;
      $self->top->{data} = pop @{$self->cur_value};
   },

   property => sub {
      my ($self) = @_;
      my $value = pop @{$self->cur_value};
      my ($name, $type, $ext) = @{pop @{$self->cur_property}};
      $name =~ s/^($id_re):://
        and my $app_name = $1;
      my $obj = $self->cur_value->[-1];
      $obj->{$name} = $value;
      if (defined($type)) {
         $obj->{_attrs}->{$name}->{_type} = $type;
      }
      if (defined($ext)) {
         $obj->{_attrs}->{$name}->{_ext} = $ext;
      }
      if (defined($app_name)) {
         push @{$obj->{_load}}, $app_name;
      }
   },

   attachment => sub {
      my ($self) = @_;
      my $value = pop @{$self->cur_value};
      my ($name, $type, $ext, $construct) = @{pop @{$self->cur_property}};
      my $obj = $self->cur_value->[-1];
      $obj->{$name} = $value;
      my $attrs = $obj->{_attrs}->{$name} = { attachment => true };
      if (defined($type)) {
         $attrs->{_type} = $type;
      }
      if (defined($ext)) {
         $attrs->{_ext} = $ext;
      }
      if (defined($construct)) {
         $attrs->{construct} = $construct;
      }
   },

   credit => sub {
      my ($self) = @_;
      my $text = pop @{$self->cur_value};
      my $name = pop @{$self->cur_property};
      (@{$self->cur_value} > 1 ? $self->cur_value->[-1] : $self->top)->{_info}->{credits}->{$name} = $text;
   },

   description => sub {
      my ($self) = @_;
      my $text = pop @{$self->cur_value};
      (@{$self->cur_value} > 1 ? $self->cur_value->[-1] : $self->top)->{_info}->{description} = $text;
   },

   m => sub {
      my ($self) = @_;
      my $value = pop @{$self->cur_value};
      if (defined(my $cols = pop @{$self->cur_property})) {
         push @$value, { cols => $cols };
      }
      add_value($self, $value, undef, 'm');
   },

   t => sub {
      my ($self) = @_;
      my $value = pop @{$self->cur_value};
      my ($index, $id) = @{pop @{$self->cur_property}};
      if (defined($id)) {
         # a singular case from v3.0: polynomial variable list to be replaced with the number of variables
         if (is_array($value) && @$value == 1 && is_array($value->[0])) {
            $value = scalar @{$value->[0]};
            add_value($self, $value, undef, 'e');
            $self->by_id->{$id->{Value}} = $value;
         } else {
            die "unexpected id attribute\n";
         }
      } else {
         add_value($self, $value, $index, 't');
      }
   },

   v => sub {
      my ($self) = @_;
      add_value($self, pop @{$self->cur_value}, pop @{$self->cur_property}, 'v');
   },

   e => sub {
      my ($self) = @_;
      add_value($self, pop @{$self->cur_value}, pop @{$self->cur_property}, 'e');
   },

   r => sub {},
);

sub start_element {
   my ($self, $data) = @_;
   my ($ns, $name, $attrs) = @$data{'NamespaceURI', 'LocalName', 'Attributes'};
   if ($ns ne $pmns) {
      die "not a polymake data file element\n";
   }
   $start_element{$name}->($self, $attrs);
}

sub end_element {
   my ($self, $data) = @_;
   $end_element{$data->{LocalName}}->($self);
}

sub characters {
   my ($self, $data) = @_;
   my $text = $data->{Data};
   if (is_string($self->cur_value->[-1])) {
      $self->cur_value->[-1] .= $text;
   } elsif ($text =~ /\S/) {
      die "unexpected characters\n";
   }
}

package Polymake::Core::XMLtoJSON;

use XML::SAX;

sub from_file {
   my ($filename) = @_;
   open my $F, "<", $filename or die "can't read $filename: #!\n";
   from_filehandle($F);
}

sub from_filehandle {
   my ($F) = @_;
   my $handler = new Handler();
   XML::SAX::ParserFactory->parser(Handler => $handler)->parse_file($F);
   $handler->top
}

sub from_string {
   my $handler = new Handler();
   XML::SAX::ParserFactory->parser(Handler => $handler)->parse_string($_[0]);
   $handler->top
}

1


# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
