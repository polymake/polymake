#  Copyright (c) 1997-2020
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

package Polymake::Test::Examples;

# all examples found in an application
use Polymake::Struct (
   [ '@ISA' => 'Group' ],
   [ new => '$$$' ],
   [ '$extension' => 'undef' ],
   [ '$name' => '"Examples"' ],
);

sub new {
   my $self = &_new;
   if ($self->env->report_writer) {
      substr($self->name, 0, 0) .= $self->application->name . ".";
   }
   my $filter;
   if (defined(my $pattern = $self->dir)) {
      if ($pattern =~ s/\*/.*/g ||
	  $pattern =~ s/\?/./g) {
	 $filter = sub { $_[0]->name =~ m{^$pattern$} && scalar($_[0]->get_examples) };
      } else {
	 $filter = sub { $_[0]->name eq $pattern && scalar($_[0]->get_examples) };
      }
   } else {
      $filter = sub { scalar($_[0]->get_examples) };
   }
   # help topics are sorted for the sake of reproducibility of test runs
   # because otherwise they appear in hash-map (=random) order
   @{$self->subgroups} = map { new ExamplesInTopic($_, $self) }
                         sort { $a->name cmp $b->name || $a->full_path cmp $b->full_path }
                         map { $_->list_matching_leaves($filter) }
                         ( $self->application->help, $self->application->name eq "common" ? $Core::Help::core : () );
   $self;
}

sub run_context { undef }

package Polymake::Test::ExamplesInTopic;

# examples provided for a single help topic
use Polymake::Struct (
   [ '@ISA' => 'Subgroup' ],
   [ '$source_file' => 'undef' ],
   [ '$name' => '#1 ->full_path' ],
   [ '$topic' => '#1' ],
   [ '$source_line' => 'undef' ],
);

sub new {
   my $self=&_new;
   ($self->source_file, $self->source_line)= $self->topic->defined_at =~ m{^(.*), line (\d+)};
   $self;
}

sub create_testcases {
   my ($self)=@_;
   my $id=0;
   state $testpkg="ExampleTestPkg000000";
   my $disable_cnt=0;
   my $disable_reason;

   foreach my $example ($self->topic->get_examples) {
      ++$id;
      ++$testpkg;
      my $app=$self->group->application;
      my $app_name=$app->name;

      my $source_file=$example->source_file;
      my $source_line=$example->source_line;
      my (@snippets, $disable_this, $nocompare);

      foreach my $hint (@{$example->hints}) {
	 if (my ($hint_app)= $hint =~ /^\s*application\s+($id_re)\s*$/o) {
	    if ($hint_app eq $self->group->application->name) {
	       $@="help topic ".$self->topic->full_path." example #$id contains a superfluous annotation [application $hint_app] at $source_file, line $source_line\n";
               return;
	    } else {
               $app_name=$hint_app;
               $app=eval { User::application($app_name) } or return;
            }
	 } elsif (my ($label)= $hint =~ /^\s*prefer\s+($id_re)\s*$/o) {
	    if (defined($app->prefs->find_label($label))) {
	       push @snippets, "prefer_now('$label');\n"
	    } else {
	       $disable_reason .= "requires an unknown preference label '$label'\n";
	       $disable_this=1;
	       last;
	    }
         } elsif (my ($URI)= $hint =~ /^\s*require\s+(bundled:$id_re)\s*$/o) {
            if (defined(my $ext = $Core::Extension::registered_by_URI{$URI})) {
               unless ($ext->is_active) {
                  $disable_this=1;
                  last;
               }
            } else {
	       $@="help topic ".$self->topic->full_path." example #$id requires to an unknown extension $URI at $source_file, line $source_line\n";
               return;
            }
	 } elsif ($hint eq "nocompare") {
	    $nocompare=1;
	 } elsif ($hint eq "notest") {
	    $disable_this=1;
	    last;
	 } else {
	    $@="help topic ".$self->topic->full_path." example #$id contains an unrecognized hint [$_] at $source_file, line $source_line\n";
	    return;
	 }
      }
      if ($disable_this) {
	 ++$disable_cnt;
	 next;
      }

      my $expected = "=== BEGIN ===\n";
      my $snippet_cnt = 0;
      while ($example->body =~ /((?: ^ [ \t]*>[ \t]* \S.*?\n)+) ((?: ^ [ \t]*\|(?:[ \t] .*)?\n)*)/xmg) {
         ++$snippet_cnt;
	 my ($snippet, $printout) = ($1, $2);
	 my $text_before = $`;
	 $snippet =~ s/^ [ \t]*>[ \t]?//xmg;                # remove the markup
	 $printout =~ s/^ [ \t]*\|[ \t]?//xmg;
	 my $end_marker = "=== END # $snippet_cnt ===";
	 $snippet =~ s/\n\Z/; print "\\n$end_marker\\n";\n/s;
	 $source_line = $example->source_line - ($snippet_cnt==1);
	 ++$source_line while $text_before =~ /\n/g;
         # the semicolon seems to soothe some pains in perl tokenizer when the first term in the expression
         # is an explicitely parametrized function template call
	 push @snippets, $snippet_cnt == 1 ? <<"--first--" : <<"--other--", $snippet;
#line $source_line "$source_file"
print "=== BEGIN ===\n";
--first--
#line $source_line
--other--
	 $expected .= $printout . "$end_marker\n";
      }
      if ($snippet_cnt) {
	 my $body = join("", @snippets);
	 $body = Core::RuleFilter::warn_options() . <<"---";
package Polymake::$testpkg;
use application '$app_name';  declare +auto;
$body
delete \$Polymake::{"$testpkg\::"};
---
	 new ExampleCase($id, $body, $nocompare ? undef : $expected, $source_file, $example->source_line, $app != $self->group->application);
      } else {
	 $@ = "help topic ".$self->topic->full_path." example #$id without any input at $source_file, line $source_line\n";
	 return;
      }
   }
   if ($disable_cnt == $id) {
      $self->disabled = $disable_reason // "notest";
   }
}

package Polymake::Test::ExampleCase;

use Polymake::Struct (
   [ '@ISA' => 'Output' ],
   [ 'new' => '$$$$$$' ],
   [ '$name' => '#1' ],
   [ '$source_file' => '#4' ],
   [ '$source_line' => '#5' ],
   [ '$body' => '#2' ],
   [ '$expected' => '#3' ],
   [ '$restore_application' => '#6' ],
);

sub run_code {
   my ($self) = @_;
   local $disable_viewers = true;
   local scalar $User::application if $self->restore_application;
   eval $self->body;
   $self->gotten_error = $@;
   $@ = "";
}

my $end_marker_re= qr/=== END \# \d+ ===/;

sub execute {
   my ($self)=@_;
   if (length($self->gotten_error)) {
      $self->fail_log="expected: regular return\n".
                      "     got: EXCEPTION: ".$self->gotten_error;
      return 0;
   }
   if (defined (my $expected=$self->expected)) {
      my $equal=1;
      my $gotten=$self->buffer;
      while ($expected =~ s/\A(.*)\n//m) {
         my $expected_line=$1;
         if ($gotten =~ s/\A(.*)\n//m) {
            my $gotten_line=$1;
            # ignore surrounding spaces
            $expected_line =~ s/^\s*(.*?)\s*$/$1/;
            $gotten_line =~ s/^\s*(.*?)\s*$/$1/;
            if ($expected_line ne $gotten_line) {
               # forgive empty lines at the end of the output block
               unless (length($expected_line)==0 &&
                       $gotten_line =~ /^$end_marker_re$/o &&
                       $expected =~ s/\A\n*$end_marker_re\n//mo
                         or
                       length($gotten_line)==0 &&
                       $expected_line =~ /^$end_marker_re$/o &&
                       $gotten =~ s/\A\n*$end_marker_re\n//mo) {
                  $equal=0;
                  last;
               }
            }
         } else {
            $equal=0; last;
         }
      }
      if (!$equal or $gotten =~ /\S/) {
         # remove distracting empty lines at the block ends
         $self->expected =~ s/(?:\A|\n) \K \n+ (?=$end_marker_re)//xmg;
         $self->buffer   =~ s/(?:\A|\n) \K \n+ (?=$end_marker_re)//xmg;

         $self->fail_log="expected:\n" . $self->expected .
                              "got:\n" . $self->buffer;
         return 0;
      }
   }
   1
}

sub describe_location {
   my ($self)=@_;
   "source file " . $self->subgroup->shorten_source_file($self->source_file) . " at line " . $self->source_line
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
