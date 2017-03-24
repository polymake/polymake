#  Copyright (c) 1997-2017
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

package Polymake::Test::Group;

use Polymake::Struct (
   [ new => '$$$$' ],
   [ '$env' => '#1' ],
   [ '$dir' => '#2' ],
   [ '$application' => '#3' ],
   [ '$extension' => '#4' ],
   '$name',
   '@subgroups',
   '%file_cache',
   [ '$loaded_subgroups' => '0' ],
   [ '$cursor_pos' => '0' ],
);

sub new {
   my $self=&_new;
   my $single_script;
   if (-f $self->dir) {
      $single_script=$self->dir;
      $self->dir =~ s/$directory_re/$1/o;
   }
   my ($dir_name)=$self->dir =~ $filename_re;
   $dir_name =~ /\./ and die "Testgroups may not contain dots ('.') in the folder name: $dir_name\n";

   if ($self->extension && $self->extension->is_bundled) {
      ($self->name=$self->extension->URI) =~ s/:/./g;
      if ($self->env->report_writer) {
         $self->name.=".".$self->application->name;
      }
      $self->name.=".$dir_name";
   } elsif (index($self->dir, $self->application->installTop)==0) {
      $self->name=($self->env->report_writer && $self->application->name.".").$dir_name;
   } else {
      $self->name=$self->dir;
   }

   if ($single_script) {
      push @{$self->subgroups}, new Subgroup($single_script, $self);
   } else {
      @{$self->subgroups}=map { new Subgroup($_, $self) } glob($self->dir."/test*.pl");
   }
   $self;
}

sub run_context {
   my ($self)=@_;
   new TempChangeDir($self->dir);
}

# => 1: success | 0: failure | -1: error, with message in $@
sub run {
   my ($self)=@_;
   my $save_context=$self->run_context;
   local_scalar($self->env->cur_group, $self);
   my $report_writer=$self->env->report_writer;
   if ($report_writer) {
      $report_writer->startTag("testsuite", package => $self->application->name, name => $self->name);
   } else {
      print_title($self, "testing " . $self->name . ":");
   }
   my $OK=1;
   foreach my $subgroup ($self->env->shuffle->(@{$self->subgroups})) {
      my $rc=$subgroup->run;
      $OK &&= $rc;
      last if $rc<0;
   }
   if ($report_writer) {
      $report_writer->endTag("testsuite");
   }
   $OK
}

sub print_title {
   my ($self, $title)=@_;
   $self->cursor_pos=length($title);
   print $title;
}

#############################################################################
package Polymake::Test::Subgroup;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$source_file' => '#1' ],
   [ '$group' => 'weak(#2)' ],
   [ '$name' => 'undef' ],
   [ '$disabled' => 'undef' ],
   [ '$case_names_length' => '0' ],
   '$is_random',
   '@cases',
   '%case_names',
   '@subgroups',
   [ '$cleanup' => 'undef' ],
);

declare $current;

# to be temporarily inserted into $application->myInc
declare $preamble=[ ".",
                    "\$current->group->env->start_timers;\n",
                    "package Polymake::User;\n" ];

sub new {
   my $self=&_new;
   if ($self->source_file =~ /test_(.*)\.pl$/) {
      $self->name=$1;
   }
   $self
}

sub shorten_source_file {
   my ($self, $source_file)=@_;
   if (index($source_file, $self->group->application->installTop)==0) {
      substr($source_file, length($self->group->application->installTop)+1);
   } else {
      $source_file;
   }
}

sub add_case {
   my ($self, $case)=@_;
   weak($case->subgroup=$self);
   ($case->exec_time, $case->duration)=$self->group->env->read_timers;

   if (defined (my $line=$self->case_names->{$case->name})) {
      croak( "multiple use of test case name '", $case->name, "' within the same subgroup; first occurrence at line $line" );
   }
   $self->case_names->{$case->name}=$case->source_line;

   push @{$self->cases}, $case;

   unless ($self->group->env->report_writer || $case->hidden) {
      if (@{$self->cases}==1) {
         my $subgroups_before=$self->group->loaded_subgroups++;
         if ($subgroups_before==0 && defined($self->name)) {
            print "\n";
            $self->group->print_title(" [ " . $self->name . " ]");
         } elsif ($subgroups_before) {
            $self->group->print_title(" [ " . ($self->name // "test") . " ]");
         }
      }
      print " ", $case->name;
      $self->case_names_length += length($case->name)+1;
   }
}

sub add_subgroup {
   my ($self, $subgroup)=@_;
   weak($subgroup->group=$self->group);

   push @{$self->subgroups}, $subgroup;

   $subgroup->source_file=$self->source_file;
   $subgroup->is_random=$self->is_random;
   if (defined $self->name) {
      $subgroup->name=$self->name." => ".$subgroup->name;
   }
}

sub create_testcases {
   my ($self)=@_;
   my ($script_file)=$self->source_file =~ $filename_re;
   do $script_file;
}

sub run {
   my ($self)=@_;
   local $current=$self;
   my $env=$self->group->env;
   my $report_writer=$env->report_writer;
   $self->case_names_length=0;

   local $Scope=new Scope();   # prevent the effects of prefer_now and such from spreading beyond the current test script
   $self->create_testcases;
   if ($@ eq "") {
      if (@{$self->cases} || @{$self->subgroups}) {
         my $OK=1;
         if (@{$self->cases}) {
            $self->assess_cases or $OK=0;
         }
         foreach my $subgroup ($env->shuffle->(@{$self->subgroups})) {
            $subgroup->run or return 0;
         }
         if (defined $self->cleanup) {
            eval { $self->cleanup->() };
            unless ($@) {
               return $OK;
            }
         } else {
            return $OK;
         }

      } elsif (length($self->disabled)) {
         if ($self->disabled ne "notest") {
            if ($report_writer) {
               $report_writer->startTag("testcase", classname => $self->group->name,
                                        name => ($self->name && "[ ".$self->name." ] ")."setup",
                                        status => "ignore");
               $report_writer->dataElement("system-out", $self->describe_location);
               $report_writer->emptyTag("skipped", message => $self->disabled);
               $report_writer->endTag("testcase");
            } else {
               if ($self->group->loaded_subgroups==0 && $self==$self->group->subgroups->[-1]) {
                  print " SKIPPED\n";
               }
               (my $script_file=$self->source_file) =~ s{^\Q$InstallTop/\E}{};
               push @{$env->skipped}, $script_file.": ".$self->disabled."\n";
            }
         }
         return 1;

      } else {
         $@="no testcases defined";
      }
   }
   report_error($self);
}

sub report_error {
   my ($self)=@_;
   my $env=$self->group->env;
   my $report_writer=$env->report_writer;
   if ($report_writer) {
      $report_writer->startTag("testcase", classname => $self->group->name,
                               name => ($self->name && "[ ".$self->name." ] ")."setup");
      $report_writer->dataElement("system-out", $self->describe_location);
      $report_writer->emptyTag("error", message=>$@);
      $report_writer->endTag("testcase");
   } else {
      my ($local_source_file)=$self->source_file =~ $filename_re;
      $@ =~ s{^(.*) at \S*\Q$local_source_file\E line (\d+)\.?$}{$env->present_source_location($self->source_file, $2).": ERROR: $1\n"}e
        or
      $@=$env->present_source_location($self->source_file, 1).": ERROR: $@\n";
      push @{$env->failed}, $@;
      print " ERROR\n";
   }
   $@="";
}

sub describe_location {
   my ($self)=@_;
   "testfile: ".shorten_source_file($self, $self->source_file);
}

sub assess_cases {
   my ($self)=@_;
   my $env=$self->group->env;
   my $report_writer=$env->report_writer;
   my $OK=1;

   unless ($report_writer) {
      $env->rewind_cursor($self->group->cursor_pos, $self->case_names_length);
   }

   foreach my $case (@{$self->cases}) {
      if ($case->run) {
         unless ($report_writer || $case->hidden) {
            $env->print_case_name($case->name, 1);
         }
      } else {
         $OK=0;
         unless ($report_writer) {
            $env->print_case_name($case->name, 0);
            my $loc=$env->present_source_location($case->source_file, $case->source_line);
            if ($self->is_random && $env->ignore_random_failures) {
               push @{$env->random_failed}, "$loc: testcase ".$case->name."\n".$case->fail_log;
            } else {
               push @{$env->failed}, "$loc: testcase ".$case->name."\n".$case->fail_log;
            }
         }
      }
   }
   unless ($report_writer) {
      if ($OK) {
         print " OK\n";
      } elsif ($self->is_random && $env->ignore_random_failures) {
         print " FAILED but ignored as RANDOM\n";
         $OK=1;
      } else {
         print " FAILED\n";
      }
   }
   $OK
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
