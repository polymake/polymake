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

package Polymake::Test::Value;

use Polymake::Struct (
   [ '@ISA' => 'Case' ],
   [ new => '$$$%' ],
   [ '$expected' => '#2' ],
   [ '$gotten' => '#3' ],
);

sub execute {
   my ($self)=@_;
   not( $self->fail_log=compare_and_report($self->expected, $self->gotten) );
};

sub compare_and_report {
   my ($expected, $gotten)=@_;
   if (ref($expected) ne ref($gotten)) {
      return "result type mismatch: expected ".(ref($expected) || "SCALAR").
             " got ".(ref($gotten) || (defined($gotten) ? "SCALAR" : "UNDEF"))."\n";
   }
   if (is_object($expected)) {
      unless ($expected->type->equal->($expected, $gotten)) {
         if (UNIVERSAL::isa($expected->type->pkg, "Polymake::Core::BigObjectArray")) {
            if (@$expected != @$gotten) {
               return "expected:\n".scalar(@$expected)." objects\n".
                      "     got:\n".scalar(@$gotten)." objects\n";
            } else {
               my $sum_report="";
               for (my $i=0; $i<@$expected; ++$i) {
                  if (my $report=Object::compare_and_report($expected->[$i], $gotten->[$i])) {
                     $sum_report.="diff for object [$i]:\n$report\n";
                  }
               }
               return $sum_report;
            }
         } else {
            return "expected:\n$expected\n".
                   "     got:\n$gotten\n";
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

##################################################################
package Polymake::Test::Value::FromData;

use Polymake::Struct(
   [ '@ISA' => 'Value' ],
   [ new => '$$%' ],
   [ '$expected' => 'undef' ],
   [ '$gotten' => '#2' ],
);

sub execute {
   my ($self)=@_;
   $self->expected=$self->load_OK_data_file;
   &Value::execute;
}

sub load_OK_data_file {
   my ($self)=@_;
   $self->subgroup->group->env->load_data_file($self->name.".OK");
}

##################################################################
package Polymake::Test::Value::FromAttachment;

use Polymake::Struct(
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

use Polymake::Struct(
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
