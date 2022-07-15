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

package Polymake::Test::Environment;

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
   [ '$shuffle_seed' => '#%' ],
   [ '&shuffle' => 'sub { @_ }' ],
   [ '$annotate_mode' => '#%' ],
   [ '$preserve_load' => '#%' ],
   [ '$cur_group' => 'undef' ],
   '@skipped',
   '@random_failed',
   '@failed',
   [ '$save_autoflush' => 'undef' ],
   '%schemas',
);

sub new {
   my $self=&_new;
   my ($scope)=@_;

   if (defined $self->shuffle_seed) {
      require List::Util;
      $self->shuffle=\&List::Util::shuffle;
      if ($self->shuffle_seed) {
         srand($self->shuffle_seed);
      } else {
         $self->shuffle_seed=srand();
      }
   }

   local with($scope->locals) {
      local $Core::Serializer::reject_unknown_properties = true;

      # suppress printing credit notes
      local $Polymake::User::Verbose::credits=0;

      # replace the standard 'load' user function
      local *Polymake::User::load = sub {
         load_object_file($self, $_[0], $User::application);
      }
      unless $self->preserve_load;

      # replace the standard 'load_data' user function
      local *Polymake::User::load_data = sub {
         load_data_file($self, $_[0]);
      }
      unless $self->preserve_load;
   }

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
   my ($self, $jenkins_report, $app_name)=@_;
   if (length($jenkins_report)) {
      require XML::Writer;
      my $report_file = "${jenkins_report}_${app_name}.xml";
      open my $report_fh, ">", $report_file or die "can't create $report_file: $!\n";
      binmode $report_fh, ":utf8";
      $self->report_writer = new XML::Writer(OUTPUT => $report_fh, NAMESPACES => 0, DATA_MODE => 1, DATA_INDENT => 2, ENCODING => 'utf-8', UNSAFE => 0);
      $self->report_writer->xmlDecl;
      $self->report_writer->startTag("testsuites");
      $self->report_fh=$report_fh;
   } elsif ($app_name eq "tutorials") {
      print "\n*** Testing $app_name ***\n\n";
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
   $self->last_timestamp=[gettimeofday()];
}

sub read_timers {
   my ($self)=@_;
   my $cpu_time=get_user_cpu_time();
   my $now=[gettimeofday()];
   my @results=($cpu_time-$self->last_cputime, tv_interval($self->last_timestamp, $now));
   $self->last_cputime=$cpu_time;
   $self->last_timestamp=$now;
   @results;
}

##################################################################
sub load_trusted_data_file { load Core::Datafile(@_) }

sub find_file_with_alternatives {
   my ($filename) = @_;
   foreach my $alt (@alternative_suffixes) {
      -f "$filename.$alt" and return "$filename.$alt";
   }
   -f $filename ? $filename : undef;
}

sub find_object_file {
   my ($self, $stem, $app) = @_;
   my $result;
   foreach my $filename ($stem =~ /\.[a-z]+$/ ? $stem : map { "$stem.$_" } map { @{$_->file_suffixes} } $app, values %{$app->used}) {
      defined($result = find_file_with_alternatives($filename)) and return $result;
   }
   die "no matching object file for $stem\n";
}

sub find_data_file {
   my ($self, $filename) = @_;
   find_file_with_alternatives($filename) // die "no matching data file for $filename\n";
}

sub load_object_file {
   my ($self, $filename, $app) = @_;
   $self->cur_group->file_cache->{$filename} //= do {
      $filename = &find_object_file;
      my $obj;
      if ($self->validate) {
         my $full_path = Cwd::abs_path($filename) =~ s{^\Q$InstallTop\E/}{}r;
         local $Verbose::files = 0;
         $obj = load_trusted_data_file($filename);
         open my $f, "<", $filename;
         my $raw_input = do { local $/; <$f> };
         close $f;
         if ($raw_input =~ /^\s*\{/) {
            my $serialized = Core::Serializer::serialize($obj);
            my $str = encode_json($serialized);
            local $Core::PropertyType::trusted_value = 0;
            my $serialized2 = decode_json($str);
            my $restored = Core::Serializer::deserialize($serialized2);
            if (my $diff = BigObject::compare_and_report($obj, $restored, ignore_shortcuts => true)) {
               die "serialize & deserialize loop for object from file $full_path failed:\n", $diff;
            }

            if ($filename !~ /\.gz$/ && !is_a Tempfile($filename) && $Core::Serializer::reject_unknown_properties) {
               my $schema = ($self->schemas->{$obj->type} //=
                             Core::Serializer::create_permissive_schema($obj->type)->validate_self());
               $schema->validate(decode_json($raw_input), "validation of data file $full_path failed");
               $schema->validate($serialized2, "validation of re-serialized object loaded from $full_path failed");
            }
         }
      } else {
         $obj = load_trusted_data_file($filename);
      }
      if ($obj->changed) {
         # may be upgraded after a version bump
         $obj->persistent->save($obj);
      }
      # prevent storing any further changes
      undef $obj->persistent;
      $obj
   }
}

sub load_data_file {
   my ($self, $filename) = @_;
   $self->cur_group->file_cache->{$filename} //= do {
      $filename = &find_data_file;
      if ($self->validate) {
         my $full_path = Cwd::abs_path($filename) =~ s{^\Q$InstallTop\E/}{}r;
         local $Verbose::files = 0;
         my $data = load_trusted_data_file($filename);
         my $str = encode_json(Core::Serializer::serialize($data));
         local $Core::PropertyType::trusted_value = 0;
         my $restored = Core::Serializer::deserialize(decode_json($str));
         if (my $diff = Value::compare_and_report($data, $restored)) {
            die "serialize & deserialize loop for data from file $full_path failed:\n", $diff;
         }

         $data
      } else {
         load_trusted_data_file($filename);
      }
   }
}
##################################################################
sub print_summary {
   my ($self)=@_;
   print "\n*** Summary ***\n\n";
   if (@{$self->failed}) {
      print "*** Failed tests ***\n\n", @{$self->failed}, "\n";
   } elsif (@{$self->random_failed}) {
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
