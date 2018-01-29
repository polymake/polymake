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

package Polymake::Test::Case;

declare $allowed_exec_time=0;

use Polymake::Struct (
   [ new => '$%' ],
   [ '$name' => '#1' ],
   [ '$subgroup' => 'undef' ],
   '$fail_log',
   [ '$duration' => '0' ],
   [ '$source_file' => 'undef' ],
   [ '$source_line' => 'undef' ],
   [ '$exec_time' => '0' ],
   [ '$max_exec_time' => '#%', default => '0' ],
);

sub new {
   my $self=&_new;
   my $env=$Subgroup::current->group->env;
   unless ($env->report_writer || defined($self->source_file)) {
      for (my $depth=1; ; ++$depth) {
         my ($pkg, $file, $line)=caller($depth) or last;
         if ($pkg eq "Polymake::User") {
            $self->source_file=$file;
            $self->source_line=$line;
            last;
         }
      }
   }
   $Subgroup::current->add_case($self);
   $self;
}

sub hidden { 0 }

sub run {
   my ($self)=@_;
   my $started_at=[gettimeofday()];
   my $cpu_time=get_user_cpu_time();
   my $result=eval { $self->execute };
   my $err=$@; $@="";
   # time spent on evaluation of results
   $self->exec_time += get_user_cpu_time()-$cpu_time;
   $self->duration += tv_interval($started_at, [gettimeofday()]);

   if ($result && !$err && $self->max_exec_time) {
      my $allowed=max($self->max_exec_time, $allowed_exec_time);
      if ($self->exec_time > $allowed) {
         $err="took too long: ".$self->exec_time." > max. allowed $allowed sec\n";
         $result=0;
      }
   }

   my $env=$self->subgroup->group->env;
   my $report_writer=$env->report_writer;
   if (!$report_writer && length($err)) {
      $self->fail_log.="ERROR: $err";
   }

   if ($report_writer) {
      $report_writer->startTag("testcase", $self->subgroup->testcase_attrs($self->name), time=>"".$self->duration);
      $report_writer->dataElement("system-out", $self->describe_location);
      unless ($result) {
         if (length($err)) {
            $report_writer->cdataElement("system-err", $self->fail_log) if length($self->fail_log);
            $report_writer->emptyTag("error", message=>$err);
         } else {
            $report_writer->emptyTag("failure", message=>($self->fail_log || "silent failure"));
         }
      }
      $report_writer->endTag("testcase");
      $env->report_fh->flush;
   }
   $result
}

sub describe_location {
   my ($self)=@_;
   "testfile: " . $self->subgroup->shorten_source_file($self->source_file // $self->subgroup->source_file);
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
