#  Copyright (c) 1997-2023
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

package Polymake::Test::BigObject;

use Polymake::Struct(
   [ '@ISA' => 'Case' ],
   [ new => '$$%' ],
   [ '$gotten' => '#2' ],
   [ '$ignore' => '#%', default => 'undef' ],
   [ '$permuted' => '#%', default => 'undef' ],
   [ '$expected' => '#%', default => 'undef' ],
   [ '$after_cleanup' => '#%' ],
);

sub execute {
   my ($self) = @_;
   my $expected = ($self->expected //= $self->load_object_file);
   my $gotten = $self->gotten;
   if (instanceof Core::BigObject($gotten)) {
      if (defined $self->permuted) {
         ref($self->permuted) eq "ARRAY" && @{$self->permuted}>0
           or croak( "option `permuted' must provide a non-empty list of property names" );
         my @explicit_perm;
         my @prop0 = $expected->type->encode_descending_path($self->permuted->[0]);
         if ($prop0[-1]->flags & Core::Property::Flags::is_permutation) {
            shift @{$self->permuted};
            push @explicit_perm, permutation => \@prop0;
         }
         @{$self->permuted}
           or croak( "option `permuted' should specify at least one data property" );
         my @perm_defining_props = map { $_ => $expected->lookup($_) // croak( "property $_ lacking in object ", $expected->name ) } @{$self->permuted};
         $gotten = $gotten->copy_permuted(@explicit_perm, @perm_defining_props);
      }
      $gotten->cleanup_now if $self->after_cleanup;
   }
   not($self->fail_log = compare_and_report($expected, $gotten, defined($self->ignore) ? (ignore => $self->ignore) : ()));
}

sub load_object_file {
   my ($self) = @_;
   if (is_object(my $obj = $self->name)) {
      $self->name = $obj->name;
      $obj
   } else {
      my $group = $self->subgroup->group;
      $group->env->load_object_file($self->name, $group->application);
   }
}

sub compare_and_report {
   my ($expected, $gotten, @options) = @_;
   if (ref($expected) ne ref($gotten)) {
      return join("",
                  "result type mismatch:\n",
                  "  expected: ", $expected->type->full_name, "\n",
                  "       got: ", instanceof Core::BigObject($gotten) ? $gotten->type->full_name : ref($gotten) || (defined($gotten) ? "SCALAR" : "UNDEF"), "\n");
   }
   if (my @diff = $expected->diff($gotten, @options)) {
      return join("", print_diff($expected, @diff));
   }
   if (my @diff = diff_attachments($expected, $gotten, @options)) {
      return join("", @diff);
   }
   if (length($expected->description)) {
      if (length($gotten->description)) {
         if ($expected->description ne $gotten->description) {
            return "different descriptions: expected '".$expected->description."'\n".
                   "                             got '".$gotten->description.  "'\n";
         }
      } else {
         return "lacking description\n";
      }
   } elsif (length($gotten->description)) {
      return "unexpected description\n";
   }
   ""
}

sub print_diff {
   my $object = shift;
   map {
      my ($expected, $gotten, $subobj) = @$_;
      my $prefix = "";
      while (defined($subobj) && $subobj != $object) {
         $prefix = $subobj->property->name.".".$prefix;
         $subobj = $subobj->parent;
      }
      if (!defined($expected)) {
         ("unexpected property $prefix", $gotten->property->name, "\n")
      } elsif (!defined($gotten)) {
         ("lacking property $prefix", $expected->property->name, "\n")
      } elsif (instanceof Core::PropertyValue::Multiple($expected)) {
         ("multiple subobject number mismatch for property $prefix", $expected->property->name, ":\n",
          "  expected: ", scalar @{$expected->values}, "\n",
          "       got: ", scalar @{$gotten->values}, "\n")
      } elsif (instanceof Core::BigObject($expected->value)) {
         ("object type mismatch for property $prefix", $expected->property->name, ":\n",
          "  expected ", $expected->value->type->full_name, "\n",
          "       got ", $gotten->value->type->full_name, "\n")
      } else {
         my ($expected_val, $gotten_val) = map { substr($_,-1) eq "\n" ? "\n$_" : " $_\n" } ($expected->toString, $gotten->toString);
         my ($expected_canned, $gotten_canned) = map { Core::CPlusPlus::get_canned_cpp_class($_) } ($expected->value, $gotten->value);

         ("different property $prefix", $expected->property->name, ":\n",
          ref($expected->value) ne ref($gotten->value) || $expected_canned ne $gotten_canned
          ? ("  expected: ", ref($expected->value), ($expected_canned && " (=>$expected_canned)"), ":", $expected_val, "\n",
             "       got: ", ref($gotten->value),     ($gotten_canned && " (=>$gotten_canned)"),   ":", $gotten_val, "\n")
          : ("  expected:$expected_val\n",
             "       got:$gotten_val\n"))
      }
   } @_;
}

sub diff_attachments {
   my ($expected, $gotten, %options) = @_;
   my $ignore = $options{ignore};
   my @diff;
   while (my ($name, $at) = each %{$expected->attachments}) {
      next if defined($ignore) && string_list_index($ignore, $name) >= 0;
      if (defined(my $gotten_at = $gotten->attachments->{$name})) {
         if (my $report = Value::compare_and_report($at->[0], $gotten_at->[0], Core::BigObject::ignore_subproperties($ignore, $name))) {
            push @diff, "attachment $name:\n$report\n";
         }
      } else {
         push @diff, "attachment $name: MISSING\n";
      }
   }
   if ($ignore || keys(%{$expected->attachments}) != keys(%{$gotten->attachments})) {
      push @diff, map { "attachment $_: UNEXPECTED\n" }
                  grep { not(exists($expected->attachments->{$_}) || string_list_index($ignore, $_) >= 0)  }
                       keys %{$gotten->attachments};
   }
   @diff
}

####################################################################################
package Polymake::Test::BigObject::Transform;

use Polymake::Struct(
   [ '@ISA' => 'BigObject' ],
   [ new => '$%' ],
);

sub execute {
   my ($self) = @_;
   my $expected = $self->load_object_file;
   local $Verbose::files = 0;
   my $group = $self->subgroup->group;
   my $transformed = load Core::Datafile($group->env->find_object_file($self->name."-in", $group->application));
   # don't update the input file
   $transformed->dont_save;
   not( $self->fail_log = compare_and_report($expected, $transformed) );
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
