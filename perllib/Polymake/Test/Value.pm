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

package Polymake::Test::Value;

use Polymake::Struct (
   [ '@ISA' => 'Case' ],
   [ new => '$$$%' ],
   [ '$expected' => '#2' ],
   [ '$gotten' => '#3' ],
);

sub execute {
   my ($self, @options) = @_;
   not( $self->fail_log = compare_and_report($self->expected, $self->gotten, @options) );
};

sub compare_and_report {
   my ($expected, $gotten, %options) = @_;

   if (ref($expected) ne ref($gotten) || defined($expected) != defined($gotten)) {
      return "result type mismatch: expected " . (ref($expected) || (defined($expected) ? "SCALAR" : "UNDEF")) .
             " got " . (ref($gotten) || (defined($gotten) ? "SCALAR" : "UNDEF")) . "\n";
   }

   my $elem_cmp = $options{cmp};
   my $is_big_object_array;
   if (is_object($expected)) {
      if ($is_big_object_array = UNIVERSAL::isa($expected->type->pkg, "Polymake::Core::BigObjectArray")) {
         $elem_cmp //= \&Test::BigObject::compare_and_report;
      } elsif (instanceof Core::BigObject($expected)) {
         return Test::BigObject::compare_and_report($expected, $gotten);
      } elsif ($expected->type->equal->($expected, $gotten)) {
         return "";
      } else {
         return "expected:\n$expected\n".
                "     got:\n$gotten\n";
      }
   }

   if (is_array($expected) || $is_big_object_array) {
      $elem_cmp //= \&compare_and_report;
      if (@$expected != @$gotten) {
         return "expected: " . scalar(@$expected) . " elements\n" .
                "     got: " . scalar(@$gotten) . " elements\n";
      }
      foreach my $i (0 .. $#$expected) {
         if (my $elem_report = $elem_cmp->($expected->[$i], $gotten->[$i])) {
            return "diff for element [$i]:\n$elem_report";
         }
      }

   } elsif (is_hash($expected)) {
      my $ignore = $options{ignore};
      if (!$ignore && keys(%$expected) != keys(%$gotten)) {
         return "expected: " . scalar(keys %$expected) . " elements\n" .
                "     got: " . scalar(keys %$gotten) . " elements\n";
      }
      while (my ($key, $expected_value) = each %$expected) {
         next if $ignore && string_list_index($ignore, $key) >= 0;
         if (!exists($gotten->{$key})) {
            keys %$expected;
            return "diff for element '$key':\n" .
                   "expected: EXISTS\n" .
                   "     got: NONE\n";
         }
         if (my $elem_report = compare_and_report($expected_value, $gotten->{$key})) {
            keys %$expected;
            return "diff for element '$key':\n$elem_report";
         }
      }
      if ($ignore) {
         foreach my $key (keys %$gotten) {
            unless (exists($expected->{$key}) || string_list_index($ignore, $key) >= 0) {
               keys %$gotten;
               return "diff for element '$key':\n" .
                      "expected: NONE\n" .
                      "     got: EXISTS\n";
            }
         }
      }

   } else {
      if (is_numeric($expected) ? $expected != $gotten : $expected ne $gotten) {
         return "expected: '$expected'\n".
                "     got: '$gotten'\n";
      }
   }
   ""
}

sub compare_arrays {
   my ($expected, $gotten, $cmp) = @_;
   ""
}

##################################################################
package Polymake::Test::Value::FromData;

use Polymake::Struct (
   [ '@ISA' => 'Value' ],
   [ new => '$$%' ],
   [ '$expected' => 'undef' ],
   [ '$gotten' => '#2' ],
   [ '$big_objects' => '#%' ],
);

sub execute {
   my ($self) = @_;
   $self->expected = $self->load_OK_data_file;
   if ($self->big_objects) {
      if (is_array($self->big_objects)) {
         Value::execute($self, cmp => sub { Test::BigObject::compare_and_report(@_, @{$self->big_objects}) });
      } else {
         "invalid comparison options for big objects: must be an array\n"
      }
   } else {
      &Value::execute;
   }
}

sub load_OK_data_file {
   my ($self) = @_;
   $self->subgroup->group->env->load_data_file($self->name.".OK");
}

##################################################################
package Polymake::Test::Value::FromAttachment;

use Polymake::Struct (
   [ '@ISA' => 'Value' ],
   [ new => '$$$%' ],
   [ '$name' => '#1->name' ],
   [ '$expected' => '#1' ],
   [ '$gotten' => '#3' ],
   [ '$attachment' => '#2' ],
);

sub execute {
   my ($self)=@_;
   $self->expected=$self->expected->get_attachment($self->attachment)
                   // die "attachment ", $self->attachment, " not found in file ", $self->expected->persistent->filename, "\n";
   &Value::execute;
}

##################################################################
package Polymake::Test::Value::Boolean;

use Polymake::Struct (
   [ '@ISA' => 'Value' ],
   [ new => '$$%' ],
   [ '$expected' => 'undef' ],
   [ '$gotten' => '#2' ],
);

sub execute {
   my ($self)=@_;
   if (is_boolean($self->gotten) || Core::CPlusPlus::is_proxy_for($self->gotten, "bool")) {
      if ($self->gotten) {
         return 1;
      }
      $self->fail_log="expected: TRUE\n".
                      "     got: FALSE\n";
   } else {
      $self->fail_log="expected: TRUE\n".
                      "     got: non-boolean value ".(ref($self->gotten) || "'".$self->gotten."'")."\n";
   }
   0;
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
