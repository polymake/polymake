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

use Polymake::Core::Shell::Mock;

package Sage;

sub tab_completion {
   my ($code)=@_;
   state $sh=new Polymake::Core::Shell::Mock;
   $sh->complete($code);
   return $sh->completion_proposals;
}

sub properties_for_type {
   my $type = shift;
   my $properties=new Map<String,String>;
   my @overridden_properties;

   foreach my $t (@{$type->linear_isa}, $type) {
      foreach (values %{$t->properties} ) {
         if ( !($_->flags & Core::Property::Flags::is_permutation) && $_->name !~ /\.pure$/ ) {
             my $fn;
             if (instanceof Polymake::Core::BigObjectType($_->type) ) {
                $fn = $_->type->pure_type->full_name;
             } else {
                $fn = $_->type->full_name;
             }
             $fn =~ s/, NonSymmetric>/>/g;
             $properties->{$_->name} = $fn;
             if (length($_->overrides) > 0) {
                 push @overridden_properties, $_->overrides;
             }
         }
      }
   }
   foreach (@overridden_properties) {
       delete $properties->{$_};
   }
   return $properties;
}

sub properties_for_object {
   my $obj = shift;
   return properties_for_type($obj->type);
}

sub methods_for_object {
   my $obj = shift;
   my $methods = new Map<String,String>;

   foreach my $t (@{$obj->type->linear_isa},$obj->type) {
      if ( defined($t->help_topic) ) {
         foreach ($t->help_topic->find("methods", ".*")) {
            my $rt = $_->return_type;
            $methods->{$_->[1]} = "$rt";
         }
      }
   }

   return $methods;
}

sub big_objects {
   my $app = User::application();
   my @big_objects;
   foreach ($app->help->find("objects",".*") ) {
      my $name = $_->name;
      if ( !($name =~ /any/) && !($name =~ /PermBase/ ) && !($name =~ /^Core/ ) && !($name =~ /^Visual/ )) {
         push @big_objects, $name;
      }
   }

   return new Array<String>(\@big_objects);
}


# How many overloads does a function have?
# Input: a help object or a string
# Output: the size of the hash containing the overloads
sub n_overloads {
   my ($f) = @_;
   return ref($f) eq "Polymake::Core::Help::Topic" ? scalar keys %{$f->topics}
          : scalar keys %{$User::application->help->find("!rel", "functions", $f)->topics};
}


# return a list of polymake functions whose names complete a given string.
# if called with an empty string, returns all functions
# Input: String
# Output: Array<String>
sub complete_function_name {
   my ($f) = @_;
   if (scalar $f == 0) {
      $f = ".*";
   }
   my @names;
   foreach ($User::application->help->find("!rel", "functions", $f)) {
      push @names, $_->name;
   }
   return new Array<String>(\@names);
}

# Input: a string with a function name, and an optional overload index (defaults to 0)
# Output: a help function object corresponding to these data
sub function_annex {
   my ($fname, $i) =  @_;
   my $f = $User::application->help->find("!rel", "functions", $fname);
   my $n_ov = n_overloads($f);
   if (!defined($i) && $n_ov > 0) { # if there are overloads, default to the 0-th
      $i=0;
   }
   if ($n_ov > 0) {
      if ($i < 0 || $i >= $n_ov) {
         croak("Index $i of overloaded function out of bounds. Must be between 0 and ", $n_ov-1, "\n");
      }
   } elsif ($i>0) {
      croak("The function $fname does not have overloads, but you specified overload $i");
   }
   return ($n_ov == 0) ? $f->annex : $f->topics->{"overload#$i"}->annex;
}

# Input: a function name and an overload index
# Output: An Array<String> containing the type and value of the return type
sub return_type {
    my ($fname, $i) = @_;
    return new Array<String>(function_annex($fname, $i)->{return});
}

# Input: a function name and an overload index
# Output: The types, names and explanations of the arguments
sub arguments {
    my ($fname, $i) = @_;
    return new Array<Array<String>>(function_annex($fname, $i)->{param});
}

# Input: a function name and an overload index
# Output: The types and explanations of the template parameters
sub template_parameters {
    my ($fname, $i) = @_;
    return new Array<Array<String>>(function_annex($fname, $i)->{tparam});
}

# Input: a function name and an overload index
# Output: The types and explanations of the optional arguments
sub optional_arguments {
    my ($fname, $i) = @_;
    return new Array<Pair<String,Array<String>>>(function_annex($fname, $i)->{options});
}

# Input: a function name and an overload index
# Output: The usage examples
sub examples {
    my ($fname, $i) = @_;
    my @examples;
    foreach (@{function_annex($fname,$i)->{examples}}) {
	push @examples, $_->[0];
    }
    return new Array<String>(\@examples);
}


sub is_polymake_object {
   my $p = shift;
   return instanceof Polymake::Core::BigObjectType($p->type);
}


sub completions_for_object {
   my $obj = shift;

   my $props = properties_for_object($obj);
   my $meth = methods_for_object($obj);
   foreach ( keys %$meth ) {
      $props->{$_} = $meth->{$_};
   }
   return $props;
}

1

# Local Variables:
# mode: perl
# cperl-indent-level:3
# End:
