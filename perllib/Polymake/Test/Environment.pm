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

use strict;
use namespaces;
use feature 'state';

package Polymake::Test::Environment;

use Time::HiRes qw(gettimeofday tv_interval);

##################################################################
use Polymake::Struct (
   [ new => '$%' ],
   [ '$report_writer' => 'undef' ],
   [ '$report_fh' => 'undef' ],
   [ '$tgetent' => 'undef' ],
   [ '$line_width' => '0' ],
   [ '$last_cputime' => 'undef' ],
   [ '$last_timestamp' => 'undef' ],
   [ '$ignore_random_failures' => '#%' ],
   [ '$validate' => '#%' ],
   [ '$no_new_glue_code' => '#%' ],
   [ '$shuffle_seed' => '#%' ],
   [ '&shuffle' => 'sub { @_ }' ],
   [ '$cur_group' => 'undef' ],
   '@skipped',
   '@random_failed',
   '@failed',
   '%big_types',
   '%validation_groups',
   [ '$save_autoflush' => 'undef' ],
);

sub new {
   my $self=&_new;
   my ($scope)=@_;

   # suppress using and generating C++ glue code in the private wrappers extension
   Core::CPlusPlus::forbid_code_generation($scope, $self->no_new_glue_code);

   if ($self->validate) {
      Core::XMLfile::enforce_validation($scope);
   } else {
      Core::XMLfile::suppress_validation($scope);
   }
   Core::XMLfile::reject_unknown_properties($scope);

   if (defined $self->shuffle_seed) {
      require List::Util;
      $self->shuffle=\&List::Util::shuffle;
      if ($self->shuffle_seed) {
         srand($self->shuffle_seed);
      } else {
         $self->shuffle_seed=srand();
      }
   }

   $scope->begin_locals;

   # suppress printing credit notes
   local $Polymake::User::Verbose::credits=0;

   # replace the standard 'load' user function
   local *Polymake::User::load = sub {
      load_object_file($self, find_object_file($_[0], $User::application));
   };

   # replace the standard 'load_data' user function
   local *Polymake::User::load_data = sub {
      load_data_file($self, find_matching_file($_[0]));
   };

   $scope->end_locals;

   $self
}

sub prepare_pretty_output {
   my ($self, $emacs_style)=@_;
   unless ($emacs_style) {
      require Term::Cap;
      require Term::ANSIColor;
      require Term::ReadKey;
      $self->tgetent=Term::Cap->Tgetent;
      ($self->line_width)=Term::ReadKey::GetTerminalSize();
   }
   $self->save_autoflush=STDOUT->autoflush(1);
}

sub DESTROY {
   my ($self)=@_;
   if (defined $self->save_autoflush) {
      STDOUT->autoflush($self->save_autoflush);
   }
}

sub start_testsuite {
   my ($self, $jenkins_report, $app_name, $validation)=@_;
   if (length($jenkins_report)) {
      my $report_file="${jenkins_report}_${app_name}".($validation ? "_validation" : "").".xml";
      open my $report_fh, ">", $report_file or die "can't create $report_file: $!\n";
      binmode $report_fh, ":utf8";
      $self->report_writer=new XML::Writer(OUTPUT => $report_fh, NAMESPACES => 0, DATA_MODE => 1, DATA_INDENT => 2, ENCODING => 'utf-8', UNSAFE => 0);
      $self->report_writer->xmlDecl;
      $self->report_writer->startTag("testsuites", name => $app_name);
      $self->report_fh=$report_fh;

   } elsif ($validation) {
      print "\n*** Validating data files in application $app_name ***\n\n";
   } else {
      print "\n*** Testing in application $app_name ***\n\n";
   }
}

sub close_testsuite {
   my ($self)=@_;
   if ($self->report_writer) {
      $self->report_writer->endTag("testsuites");
      $self->report_writer->end;
      undef $self->report_writer;
      close $self->report_fh;
      undef $self->report_fh;
   }
}

sub rewind_cursor {
   my ($self, $head, $tail)=@_;
   if (defined($self->tgetent) && $self->line_width != 0) {
      use integer;
      my $vertical=($head+$tail)/$self->line_width - $head/$self->line_width;
      if ($vertical>0) {
         $self->tgetent->Tgoto('UP', 0, $vertical, *STDOUT);
      }
      my $horizontal=($head+$tail)%$self->line_width - $head%$self->line_width;
      if ($horizontal>0) {
         $self->tgetent->Tgoto('LE', 0, $horizontal, *STDOUT);
      } elsif ($horizontal<0) {
         $self->tgetent->Tgoto('RI', 0, -$horizontal, *STDOUT);
      }
   } else {
      print "\nverifying:";
   }
}

sub print_case_name {
   my ($self, $name, $success)=@_;
   if (defined $self->tgetent) {
      print " ", Term::ANSIColor::colored($name, $success ? "green" : "red");
   } else {
      print " ", $name;
   }
}

sub present_source_location {
   my ($self, $file, $line)=@_;
   if (defined $self->tgetent) {
      # perl style for on-screen report
      qq{"$file", line $line}
   } else {
      # emacs compilation mode style
      qq{$file:$line}
   }
}

sub start_timers {
   my ($self)=@_;
   $self->last_cputime=get_user_cpu_time();
   $self->last_timestamp=[gettimeofday];
}

sub read_timers {
   my ($self)=@_;
   my $cpu_time=get_user_cpu_time();
   my $now=[gettimeofday];
   my @results=($cpu_time-$self->last_cputime, tv_interval($self->last_timestamp, $now));
   $self->last_cputime=$cpu_time;
   $self->last_timestamp=$now;
   @results;
}

##################################################################
sub load_trusted_object_file { load Core::Object(@_) }
sub load_trusted_data_file { scalar((new Core::XMLfile($_[0]))->load_data) }

##################################################################

sub full_path_and_copy {
   my ($filename)=@_;
   state $copies_dir=Tempfile->new_dir;
   (my $full_path=Cwd::abs_path($filename)) =~ s{^\Q$InstallTop\E}{};
   $full_path =~ $directory_re;
   File::Path::mkpath($copies_dir.$1);
   (substr($full_path, 1), $copies_dir.$full_path)
}

sub load_object_file {
   my ($self, $filename)=@_;
   $self->cur_group->file_cache->{$filename} //= do {
      my $obj;
      if ($self->validate) {
         local $Verbose::files=0;
         $obj=load_trusted_object_file($filename);
         my $obj_file=$obj->persistent->filename;
         my ($full_path, $copy)=full_path_and_copy($filename);
         User::save($obj, $copy);
         if (my $diff=Object::compare_and_report(load_trusted_object_file($copy), $obj, ignore_shortcuts => 1)) {
            die "save & load validation of object file $full_path failed:\n", $diff;
         }
         unlink $copy;
         if ($filename !~ /\.gz$/ && !is_a Tempfile($filename) && $Core::XMLreader::reject_unknown_properties) {
            my $app_name=$obj->type->application->name;
            $self->big_types->{$app_name}->{$obj->type}=1;
            my $validation_groups=($self->validation_groups->{$self->cur_group->application->name} //= [ ]);
            if (!@$validation_groups || $validation_groups->[-1]->dir ne $self->cur_group->dir) {
               push @$validation_groups, new ValidationGroup($self->cur_group);
            }
            push @{$validation_groups->[-1]->files_to_validate}, [ $filename, $app_name, $obj_file ];
         }
      } else {
         $obj=load_trusted_object_file($filename);
      }
      if ($obj->changed) {
         # may be upgraded after a version bump
         User::save($obj);
      }
      # prevent storing any further changes
      undef $obj->persistent;
      $obj
   }
}

sub load_data_file {
   my ($self, $filename)=@_;
   $self->cur_group->file_cache->{$filename} //= do {
      if ($self->validate) {
         local $Verbose::files=0;
         my $data=load_trusted_data_file($filename);
         my ($full_path, $copy)=full_path_and_copy($filename);
         User::save_data($data, $copy);
         if (my $diff=Value::compare_and_report(load_trusted_data_file($copy), $data)) {
            die "save & load validation of data file $full_path failed:\n", $diff;
         }
         unlink $copy;
         $data
      } else {
         load_trusted_data_file($filename);
      }
   }
}

sub create_validation_schemata {
   my ($self)=@_;
   my %schemata;
   my $tmp_dir=Tempfile->new_dir;
   while (my ($app_name, $types)=each %{$self->big_types}) {
      my $schema_file="$tmp_dir/$app_name.rng";
      open my $schema_of, ">", $schema_file or die "can't create temporary schema file $schema_file: $!\n";
      Core::XMLwriter::save_schema($schema_of, keys %$types);
      close $schema_of;
      $schemata{$app_name}=new XML::LibXML::RelaxNG(location => $schema_file);
   }
   \%schemata;
}

##################################################################
sub print_summary {
   my ($self)=@_;
   print "\n*** Summary ***\n\n";
   if (@{$self->failed}) {
      print "*** Failed tests ***\n\n", @{$self->failed}, "\n";
   }
   if (@{$self->random_failed}) {
      print "*** Failed random tests ***\n\n", @{$self->random_failed}, "\n";
   }
   if (!@{$self->failed}) {
      if (@{$self->skipped}) {
         print "*** Skipped tests ***\n\n", @{$self->skipped}, "\n",
               "*** All other tests successful ***\n\n";
      } elsif (@{$self->random_failed}) {
         print "*** All other tests successful ***\n\n";
      } else {
         print "*** All tests successful ***\n\n";
      }
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
