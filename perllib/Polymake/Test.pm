#  Copyright (c) 1997-2021
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

package Polymake::Test;

use Time::HiRes qw(gettimeofday tv_interval);

require Polymake::Test::Group;
require Polymake::Test::Case;
require Polymake::Test::Environment;
require Polymake::Test::Value;
require Polymake::Test::Stream;
require Polymake::Test::Output;
require Polymake::Test::BigObject;
require Polymake::Test::Schedule;
require Polymake::Test::Rule;
require Polymake::Test::Shell;
require Polymake::Test::Examples;

declare $disable_viewers = 0;
declare @alternative_suffixes;

use Exporter 'import';

declare @EXPORT=
 qw(check_if_configured disable_test expect_random_failures test_cleanup
    compare_values check_boolean compare_data compare_attachment
    compare_object compare_transformed_object compare_schedule check_rules
    compare_output compare_expected_error diff_with
    check_completion check_context_help neutralized_ERROR);

##################################################################
#
#  Public function for global test setup, e.g. in test.rules
#
#  Add an alternative suffix for data files
#  @param String suffix
sub add_alternative_suffix(&$) {
   my ($suffix)=@_;
   if (string_list_index(\@alternative_suffixes, $suffix) >= 0) {
      die "multiple definition of alternative suffix $suffix\n";
   }
   push @alternative_suffixes, $suffix;
}

##################################################################
#
#  Public function for test script preamble
#

# Check configurable features (extensions or rule files).
# @param String+ feature_name bundled Extension URI or rulefile name, optionally qualified with application name
# @return TRUE if at least one of the specified features is configured and active.
sub check_if_configured {
   foreach (@_) {
      if (/^bundled:${id_re}$/o) {
         my $ext = $Core::Extension::registered_by_URI{$_}
           or die "unknown extension $_\n";
         return 1 if $ext->is_active;
      } elsif (/^($id_re)::(.*)/o) {
         my $app = $User::application->name eq $1 ? $User::application : $User::application->used->{$1}
           or die lookup Application($1) ? "referring to rules of application $1 is not allowed here\n" : "unknown application $1\n";
         return 1 unless disabled_rules($app, $2);
      } else {
         return 1 unless disabled_rules($User::application, $_);
      }
   }
   $Subgroup::current->disabled = join(", ", @_)." disabled by configuration";
   0
}

sub disabled_rules {
   my ($app, $rulename) = @_;
   my $rc;
   if (defined($rc = $app->rulefiles->{$rulename})) {
      !$rc;
   } elsif (defined($rc = $app->configured->{$rulename})) {
      $rc <= 0;
   } else {
      my $answer;
      foreach my $used_app (values %{$app->used}) {
	 if (defined($answer = disabled_rules($used_app, $rulename))) {
            keys %{$app->used};
            last;
         }
      }
      $answer;
   }
}
##################################################################
# Disable the test unconditionally.
# In the test script preamble, this call must be followed by an unconditional return statement.
# @param String explanation; must refer to a Trac ticket
sub disable_test {
   if (@{$Subgroup::current->cases}) {
      croak( "disable_test must be executed in the test script preamble" );
   }
   unless ("@_" =~ /\[ticket:\d{2,}\]|[\s,;:(]\#\d{2,}(?:[\s.,;:)]|$)/) {
      croak( "test disabled without referring to a Trac ticket" );
   }
   $Subgroup::current->disabled="@_";
}

##################################################################
# Ignore failures if --ignore-random-failures option is set.
sub expect_random_failures {
   if (@{$Subgroup::current->cases}) {
      croak( "expect_random_failures must be executed in the test script preamble" );
   }
   $Subgroup::current->is_random=1;
}

##################################################################
#
#  Public function for test script epilogue
#
sub test_cleanup(&) {
   $Subgroup::current->cleanup=$_[0];
}

##################################################################
#
#  Public functions for test script body
#

##################################################################
# Compare a computed value with an expected one.
#
# @param String ID unique test name
# @param scalar expected value, either a plain perl scalar or a declared property type
# @param scalar computed value
#
# @option Float max_exec_time
#         maximal allowed execution time in seconds per object
#
sub compare_values {
   unless (@_>=3 && (is_string($_[0]) || is_integer($_[0]))) {
      croak( "usage: compare_values('ID', expected, computed)" );
   }
   new Value(@_);
}

##################################################################
# Compare a computed boolean value with expected TRUE.
#
# @param String ID unique test name
# @param Bool computed value
#
# @option Float max_exec_time
#         maximal allowed execution time in seconds per object
#
sub check_boolean {
   unless (@_>=2 && (is_string($_[0]) || is_integer($_[0]))) {
      croak( "usage: check_boolean('ID', computed)" );
   }
   new Value::Boolean(@_);
}

##################################################################
# Compare a computed value with an expected one stored in a data file.
# The data file name is derived from ID: either <ID>.OK or <ID>.<Arch>.OK
#
# @param String ID unique test name
# @param scalar computed value
#
# @option Float max_exec_time
#         maximal allowed execution time in seconds per object
#
sub compare_data {
   unless (@_>=2 && (is_string($_[0]) || is_integer($_[0]))) {
      croak( "usage: compare_data('ID', computed)" );
   }
   new Value::FromData(@_);
}

##################################################################
# Compare a computed value with an expected one stored as an attachment of a big object.
#
# @param BigObject object owner of the attachment; its name serves as test ID
# @param String attachment_name
# @param scalar computed value
#
# @option Float max_exec_time
#         maximal allowed execution time in seconds per object
#
sub compare_attachment {
   unless (@_>=3 && instanceof Core::BigObject($_[0]) && is_string($_[1])) {
      croak( "usage: compare_attachment(object, 'attachment_name', computed)" );
   }
   new Value::FromAttachment(@_);
}

##################################################################
# Compare a `big' object computed by a (user) function with an expected one
#
# @param ID unique test name; the expected object file name is derived from this
# @param BigObject computed result
#
# @option Bool after_cleanup
#         Before comparing the objects,
#         the cleanup procedure is performed on the test object
#         in order to wipe out all temporary properties.
#
# @option Array<String> ignore
#         list of property names not to be compared
#
# @option Array<String> permuted
#         list of property names defining a permutation.
#         The first item may explicitly name the permutation.
#         All other properties must be present in the expected object.
#         Their values and that in the computed result define a permutation
#         which is applied to the result before comparison.
#
# @option Float max_exec_time
#         maximal allowed execution time in seconds per object
#
sub compare_object {
   unless (@_>=2 && (is_string($_[0]) || is_integer($_[0]))) {
      croak( "usage: compare_object('ID', computed BigObject, options)" );
   }
   new BigObject(@_);
}

##################################################################
# Compare a `big' object after applying versioned transformations
#
# @param ID unique test name
#           the input object (of an older version) is loaded from a file "$ID-in" with an application specific suffix;
#           the expected transformed object is loaded from a file "$ID".
#
# @option Float max_exec_time
#         maximal allowed execution time in seconds per object
#
sub compare_transformed_object {
   unless (@_>=1 && (is_string($_[0]) || is_integer($_[0]))) {
      croak( "usage: compare_transformed_object('ID')" );
   }
   new BigObject::Transform(@_);
}

##################################################################
# Compare the computed rule chain with the expected one
#
# @param BigObject object
# @param String expected
#        multi-line string containing all expected rule headers in arbitrary order.
#        If several equivalent rule chains are accepted, they have to be separated with lines
#        containing a sequence of at least three dashes.
#        'undef' if no schedule is expected to be produced
#
# @param String+ property
#        names of properties to be provided by the rule chain.
#        Properties in subobjects are specified in dotted notation.
#
# @option Float max_exec_time
#         maximal allowed execution time in seconds per object
#
sub compare_schedule {
   unless (@_>=3 && instanceof Core::BigObject($_[0]) &&
           (is_string($_[1]) || !defined($_[1]))) {
      croak( "usage: compare_schedule(object, 'expected headers', 'PROPERTY', ...)" );
   }
   my $options=splice_options(\@_, 3);
   my ($object, $expected, @props)=@_;
   new Schedule($object->name, $expected, $object->get_schedule(@props), $options);
}

##################################################################
# Apply production rules and compare the results with contents stored in data files
#
# @param String tasks
#   a multi-line string with alternating rule headers and object name patterns:
#      RULE HEADER
#      obj1 obj2 ...
#   UNIX shell-style wildcards matching several objects are supported.
#   Application-specific file suffixes can be omitted.
#
#   Empty lines and lines starting with comment sign # may be freely inserted
#   between rule/pattern line couples.
#
#   Rule headers must match those in rule files exactly, including all labels.
#   White space differences are, however, tolerated.  Long rule headers
#   may be prolonged on subsequent lines, with trailing '\' as prolongation sign.
#
#   For disambiguation, rule headers may be adorned with specific preconditions
#   appended after '&&', or rulefile names appended after '@'.
#   This disambiguation is not necessary if every rule matching the specified
#   header is applicable to at least one object listed on the subsequent line.
#
# @option Bool after_cleanup
#         Before comparing the target properties with expectations,
#         the cleanup procedure is performed on the test object
#         in order to wipe out all temporary properties.
#
# @option String expected_failure
#         The test is succeeded when the rule terminates
#         with the specified error message.
#
# @option String on
#         Property path in dotted notation selecting a subobject
#         the rule should be applied to.
#
# @option ARRAY with_multi
#         Multiple subobject selector for temporarily setting a default instance.
#         The first element of the list is a property path leading to multiple subobjects.
#         The rest can be anything understood by give(): a unique intance name,
#         a property name with a value, or an subroute reference.
#
# @option Array<String> permuted
#         List of target properties in dotted notation which are expected to appear permuted
#         with respect to the corresponding properties stored with the test object file.
#         Only the permutation //incurred// by the rule under test is considered.
#
# @option Float max_exec_time
#         maximal allowed execution time in seconds per object
#
sub check_rules {
   unless (@_>0 && is_string($_[0])) {
      croak( "usage: check_rules('tasks', options...)" );
   }
   if (defined(wantarray)) {
      croak( "check_rules does not return any value" );
   }

   my ($tasks, @options)=@_;
   my ($pkg, $file, $line)=caller(0);
   ++$line;
   my ($prepend, $prolonged)=("", 0);
   while (length($tasks)>0) {
      if ($tasks =~ s/\A (?:[ \t]*|\#.*) \n//xm) {
         ++$line;
      } elsif ($tasks =~ s/\A (.*)\\\n//xm) {
         $prepend.=$1;
         ++$prolonged;
      } elsif ($tasks =~ s/\A [ \t]* (?!\#)(\S.*)\n [ \t]* (?!\#)(\S.*)\n//xm) {
         my ($header, $patterns)=($prepend.$1, $2);
         new Rule($header, $patterns, @options, source_file => $file, source_line => $line);
         $line+=$prolonged+2;
         ($prepend, $prolonged)=("", 0);
      } else {
         die( "expected 'RULE HEADER' and file name patterns on two subsequent lines at \"$file\", line $line\n" );
      }
   }
}

##################################################################
# Catch the output stream and compare it with the specimen file.
# The specimen file name is derived from ID: either <ID>.OK or <ID>.<Arch>.OK
#
# @param CODE body test code containing some print statements
#             and/or calling C++ functions sending some output to cout.
# @param String ID unique test name
#
# @option Float max_exec_time
#         maximal allowed execution time in seconds
#
sub compare_output(&$@) {
   new Output(@_);
}

##################################################################
# Execute the test code, catch an exception, and compare the error message with the specimen file.
# The specimen file name is derived from ID: either <ID>.OK or <ID>.<Arch>.OK
#
# @param CODE body test code which is expected to throw an exception
# @param String ID unique test name
#
# @option Float max_exec_time
#         maximal allowed execution time in seconds
# @option CODE filters list of subroutines transforming the error message
sub compare_expected_error(&$@) {
   new Output(@_, expected_error=>1);
}

##################################################################
# Compare the output file produced by a function with the specimen.
# The specimen file name is derived from ID: <ID>.OK
# Primarily for testing visualization functions offering a File option:
#   user_func(..., File => diff_with('ID'))
#
# @param String ID unique test name
# @param CODE+ filters optional filters converting both produced and expected contents
#              before comparison.  Each filter function should work linewise, with the
#              current line passed in $_ and modified in place.
#
# @option Float max_exec_time maximal allowed execution time in seconds
#
sub diff_with {
   unless (@_>0 && (is_string($_[0]) || is_integer($_[0]))) {
      croak( "usage: diff_with('ID', filters...)" );
   }
   my $id = shift;
   new Stream($id, @_ ? (filters => [ @_ ]) : ())->handle;
}

##################################################################
# Compare the TAB completion results with expectations.
#
# @param String ID unique test name
# @param String input_string partial input to be completed
# @param Int    expected completion offset from the end of input_string
# @param String+ expected completions;
#                the last one is interpreted as the expected append character if it consists of exactly one char
#                if the last word is "..." all subsequent words in the output are ignored
sub check_completion {
   unless (@_>3 && (is_string($_[0]) || is_integer($_[0])) && is_string($_[1]) && is_integer($_[2])) {
      croak( "usage: check_completion('ID', 'partial input', expected_offset, 'expected completion', ...)" );
   }
   new Shell::Completion(@_);
}

##################################################################
# Compare the help topics displayed on F1 with expectations.
#
# @param String ID unique test name
# @param String input_string partial input to be completed
# @param String+ expected headers (paths) of help topics
sub check_context_help {
   unless (@_>2 && (is_string($_[0]) || is_integer($_[0])) && is_string($_[1])) {
      croak( "usage: check_context_help('ID', 'partial input', 'expected topic path', ...)" );
   }
   new Shell::ContextHelp(@_);
}

##################################################################
#
#  Internal functions shared by various test case specializations
#

# makes the content of $@ invariant to current installation
sub neutralized_ERROR {
   $@ =~ s{\Q${InstallTop}\E(?=/)}{<TOP>}og;
   $@ =~ s{, <\$$id_re> line \d+\.}{.}og;
   $@
}

sub splice_options {
   my %options;
   my ($args, $from)=@_;
   while ($#$args > $from && is_keyword($args->[-2])) {
      $options{$args->[-2]}=$args->[-1];
      splice @$args, -2;
   }
   \%options
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
