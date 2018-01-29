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

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);
use feature 'state';

package Polymake::Test::ValidationGroup;

# all data files used in one testgroup
use Polymake::Struct (
   [ '@ISA' => 'Group' ],
   [ new => '$' ],
   [ '$env' => 'weak(#1->env)' ],
   [ '$name' => '#1->name' ],
   [ '$dir' => '#1->dir' ],
   [ '$application' => '#1->application' ],
   [ '$extension' => '#1->extension' ],
   '@files_to_validate',
);

sub new {
   my $self=&_new;
   if ($self->env->report_writer) {
      $self->name =~ s{(?=\.[^.]+$)}{.validation};
   }
   push @{$self->subgroups}, new ValidationSubgroup($self);
   $self
}

sub run {
   my ($self, $schemata)=@_;
   my $report_writer=$self->env->report_writer;
   if ($report_writer) {
      $report_writer->startTag("testsuite", package => $self->application->name, name => $self->name);
   } else {
      print "validating in ", $self->name, ":";
   }
   my $subgroup=pop @{$self->subgroups};
   $subgroup->schemata=$schemata;
   my $OK=$subgroup->run;
   if ($report_writer) {
      $report_writer->endTag("testsuite");
   }
   $OK
}

package Polymake::Test::ValidationSubgroup;

use Polymake::Struct (
   [ '@ISA' => 'Subgroup' ],
   [ new => '$' ],
   [ '$group' => 'weak(#1)' ],
   [ '$source_file' => '#1->dir' ],
   [ '$parser' => 'undef' ],
   [ '$schemata' => 'undef' ],
);

sub new { &_new; }

sub create_testcases {
   my ($self)=@_;
   @{$self->cases}=map { new ValidationCase(@$_, $self) } @{$self->group->files_to_validate};
   $self->parser=new XML::LibXML();
}

sub describe_location { "testgroup: ".$_[0]->source_file }

package Polymake::Test::ValidationCase;

use Polymake::Struct (
   [ '@ISA' => 'Case' ],
   [ 'new' => '$$$$' ],
   [ '$source_file' => '#3' ],
   [ '$subgroup' => 'weak(#4)' ],
   [ '$app_name' => '#2' ],
);

sub new { &_new; }

sub execute {
   my ($self)=@_;
   my $schema=$self->subgroup->schemata->{$self->app_name}
     or die "internal error: schema for application ", $self->app_name, " missing\n";
   $schema->validate($self->subgroup->parser->parse_file($self->source_file));
   1
}

sub describe_location { "data file ".$_[0]->source_file }

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
