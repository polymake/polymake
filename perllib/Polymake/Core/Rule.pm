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

package Polymake::Core::Rule;
use POSIX qw( :signal_h :sys_wait_h );

# Rule::flags :
#   is_function:  does not return anything, can be called arbitrarily many times
#   is_precondition:  returns a boolean value
#   is_definedness_check: additional flag for a precondition consisting of a mere check defined($this->PROPERTY)
#   is_dyn_weight:  precondition returning an additional weight [major, minor] or 'false' disabling the production rule connected to this
#   is_production:  production rule with one or more sources and targets
#   is_perm_action:  pseudo-rule concluding the restoration of a property from a permuted subobject
#   is_perm_restoring:  production rule directly suplying a PermAction
#   is_default_value: production rule with a single target and without sources
#   is_initial:    rule executed at the initial commit of a new object, maybe without targets
#   is_spez_precondition:  precondition for a restricted object type specialization
#   is_restricted_spez:  rule defined within a scope of a restricted object type spezialization

use Polymake::Enum qw( is: function=1 precondition=2 definedness_check=4 dyn_weight=8 production=16
                           perm_action=32 perm_restoring=64 default_value=128 initial=256
                           spez_precondition=512 restricted_spez=1024
                     );

#  The result code of a rule execution
#   exec_OK:    execution successful
#   exec_retry:  execution was not successful, but it's not the rule's fault
#   exec_failed:  the rule has failed and should be disabled for the object permanently
#   exec_infeasible: the rule can't be applied due to lacking input properties and/or unsatisfied preconditions
#
use Polymake::Enum qw( exec: OK retry failed infeasible );

use Polymake::Struct (
   [ 'new' => '$;$$$' ],
   [ '$header' => '#1' ],               # 'ORIGINAL HEADER'  for diagnostic and debug output
   '@input',                            # ( [ prop_ref ] )  each inner list encodes a group of alternatives
                                        #     prop_ref ::= SimpleProperty | [ SubobjProperty, ...,  SimpleProperty ]
   '@output',                           # ( prop_ref )
   [ '$flags' => '0' ],                 # is_*
   [ '$code' => '#2' ],                 # sub { ... }  compiled perl code of the rule body
   [ '$defined_for' => '#3' ],          # ObjectType
   [ '$overridden_in' => 'undef' ],     # [ ObjectType ... ]
   '@labels',                           # rule labels
   '$weight',                           # [ weight_category, weight_value ]
   '@preconditions',                    # ( Rule )
   [ '$dyn_weight' => 'undef' ],        # Rule  also inserted in preconditions
   [ '$credit' => '#4' ],               # optional Credit from the rule file
   [ '$with_permutation' => 'undef' ],  # CreatingPermutation
   [ '$not_for_twin' => 'undef' ],      # Property->key of a twin property this rule has been flipped for
);

#  The ARRAY elements in the input lists may have following array flags (see get_array_flags),
#  reflecting the occurrence of corresponding properties:
#    Property::is_permutation
#  The ARRAY elements in the output lists may have following array flags,
#  reflecting the occurrence of corresponding properties:
#    Property::is_permutation
#    Property::is_multiple_new  for a multiple subobject property with attribute (new)
#    Property::is_multiple      for a multiple subobject property with attribute (any)

declare $std_weight=[2, 10];    # standard rule weight
declare $zero_weight=[0, 0];

declare $max_major=$std_weight->[0]+1;
declare $timeout;

sub without_permutation { $_[0] }

####################################################################################
# private:
sub check_repeated_properties {
   my ($path, $seen, $tag)=@_;
   my ($prop, $state);
   {
      $prop=local_pop($path);
      $seen=($seen->{$_->key} //= { }) for @$path;
   }
   if ($tag != ($state=($seen->{$prop->key} += $tag))) {
      my $name=Property::print_path($path);
      if ($state>0) {
         croak( "Rule source '$name' occurs twice" );
      } elsif ($state<0) {
         croak( "Rule target '$name' occurs twice" );
      } else {
         croak( "Property '$name' occurs both as a source and a target" );
      }
   }
   if ($tag>0 and $prop->flags & $Property::is_multiple) {
      croak( "A rule may not have an entire multiple subobject as a source but only its single properties" );
   }
   if ($prop->flags & $Property::is_permutation) {
      if ($tag>0) {
         croak( "A rule may not have an entire permuted subobject as a source but only its single properties" );
      } else {
         croak( "A permuted subobject can't be produced as a whole" );
      }
   }
}
####################################################################################
# private:
sub parse_input {
   my ($self, $sources, $seen)=@_;
   @{$self->input}=map {
      my $input=$self->defined_for->encode_read_request($_);
      if ($Application::plausibility_checks) {
         check_repeated_properties($_, $seen, 1) for @$input;
      }
      $input
   } split /\s*,\s*/, $sources;
}
####################################################################################
# bit offset for storing the depth of a multiple subobject with `any' attribute in a rule target path
my $any_mult_depth_shift=10;

# private:
sub parse_output {
   my ($self, $targets, $seen)=@_;

   foreach my $target (split /\s*,\s*/, $targets) {
      my $proto=$self->defined_for;
      my ($prop, @out);
      my $array_flag=0;
      my $any_mult_depth=0;

      foreach (split /\./, $target) {
         if (my ($prop_name, $attrib)= /^($id_re) (?: \(\s* ($id_re) \s*\) )? $/xo) {
            $prop=$proto->property($prop_name);
            $proto=$prop->type;
            push @out, $prop;
            if ($prop->flags & $Property::is_multiple) {
               if (defined $attrib) {
                  if ($Application::plausibility_checks) {
                     foreach my $output (@{$self->output}) {
                        my $l=Property::equal_path_prefixes(\@out, $output);
                        if ($l==@out) {
                           $attrib eq "any"
                             or croak( "Contradicting attributes `any' and `new' for multiple subobject $prop_name in rule targets ",
                                       Property::print_path($output), " and $target" );
                        } elsif ($l==$#out && $output->[$l]==$prop->new_instance_deputy) {
                           $attrib eq "new"
                             or croak( "Contradicting attributes `new' and `any' for multiple subobject $prop_name in rule targets ",
                                       Property::print_path($output), " and $target" );
                        }
                     }
                  }
                  if ($attrib eq "new") {
                     $array_flag & $Property::is_permutation
                       and croak( "Rule dealing with a permutation may not create a new instance of multiple subobject $prop_name" );
                     $out[-1]=$prop->new_instance_deputy;
                     $array_flag |= $Property::is_multiple_new;
                  } elsif ($attrib eq "any") {
                     $array_flag & $Property::is_multiple_new
                       and croak( "Attribute `any' may not appear underneath a new multiple subobject instance in path $target" );
                     $array_flag |= $Property::is_multiple;
                     $any_mult_depth ||= @out;
                  } else {
                     croak( "Unknown property attribute '$attrib'" );
                  }
               } else {
                  $array_flag & $Property::is_multiple_new
                    and croak( "Multiple subobject $prop_name in path $target must be attributed with `new' because it belongs to a new instance of another multiple subobject" );
                  $any_mult_depth
                    and croak( "Multiple subobject $prop_name in path $target must be attributed with `any' because it does not occur among rule inputs" );
                  $array_flag |= $Property::is_multiple;
               }
            } else {
               if ($Application::plausibility_checks && defined($attrib)) {
                  $attrib eq "new" || $attrib eq "any"
                  ? croak( "Attribute '$attrib' is only applicable to multiple subobject properties" )
                  : croak( "Unknown property attribute '$attrib'" );
               }
               if ($prop->flags & $Property::is_permutation) {
                  if ($Application::plausibility_checks && $array_flag == $Property::is_multiple_new) {
                     croak( "Rule dealing with a permutation may not create new instances of multiple subobjects" );
                  }
                  $array_flag |= $Property::is_permutation;
               }
            }
         } else {
            croak( "Invalid property name '$_'\n" );
         }
      }

      if ($Application::plausibility_checks) {
         if ($prop->flags & $Property::is_multiple) {
            croak( "A rule may not create a multiple subobject as a whole but only its single properties" );
         }
         check_repeated_properties(\@out, $seen, -1);
      }

      push @{$self->output}, \@out;

      if ($array_flag) {
         set_array_flags($self->output->[-1], $array_flag | ($any_mult_depth << $any_mult_depth_shift));
      }
   }
}
####################################################################################
# private:
sub parse_labels {
   my ($self, $labels)=@_;
   @{$self->labels}=$self->defined_for->application->add_labels($labels);
}

sub nonexistent {
   die "attempted to execute a rule labeled as nonexistent\n";
}

# for error messages pointing to the place of definition in the rulefile
sub source_location {
   my $self=shift;
   (sub_file($self->code), " line ", sub_firstline($self->code))
}
####################################################################################
#
#  Constructor - compile the rule
#
#  new Rule(header, \&body, ObjectType);
#
sub new {
   my $self=&_new;
   my @parts;
   my ($is_nonexistent);
   if (defined $self->code) {
      @parts=split /\s*:\s*/, $self->header, 3;
      if ($Application::plausibility_checks and @parts < 2 || $parts[-1] =~ /:/) {
         croak( "ill-formed rule header" );
      }

      # digest the pseudo-labels
      if ($parts[0] eq "initial") {
         # initial rules may have output properties
         if (@parts==3) {
            shift @parts;
         } else {
            $parts[0]="";
         }
         $self->flags=$is_initial;
      } elsif ($is_nonexistent=$parts[0] eq "nonexistent") {
         shift @parts;
      }

      parse_labels($self, shift @parts) if @parts==3;

      if (defined &{$self->code}) {
         if ($self->flags==$is_initial) {
            $self->weight=$zero_weight;
         } else {
            $self->flags=$is_production;
            $self->weight=$std_weight;
         }
      } else {
         $is_nonexistent or croak( "rules without code must be labeled as nonexistent" );
         $self->code=\&nonexistent;
      }

   } else {
      @parts=split /\s*=\s*/, $self->header;
      if (@parts!=2) {
         croak( "ill-formed shortcut rule header" );
      }
      bless $self, "Polymake::Core::Rule::Shortcut";
      $self->flags=$is_production;
      $self->weight=$zero_weight;
      pop @parts if $parts[1] =~ /^\s*\$this\s*$/;
   }

   my %seen;
   parse_input($self, $parts[1], \%seen);
   parse_output($self, $parts[0], \%seen);

   if ($Application::plausibility_checks && !defined($self->code)) {
      if (@{$self->input}>1 || @{$self->output}>1) {
         croak( "A shortcut rule must have exactly one source and one target" );
      }
   }

   analyze_spez_preconditions($self);
   $self;
}
####################################################################################
# decide whether to protect the rule defined in a specialization with the preconditions of the latter
sub analyze_spez_preconditions {
   my ($self)=@_;
   if ($self->defined_for->enclosed_in_restricted_spez || defined($self->defined_for->preconditions)) {
      $self->flags |= $is_restricted_spez;
   }
}
####################################################################################
#
#  Constructor for special rules
#
#  special Rule 'Header', \&body, ObjectType
#
sub special {
   my $self=&_new;
   my %seen;
   if ($self->header =~ /\s*:\s* (.*\S) \s*$/x) {
      my $labels=$`;
      parse_input($self, $1, \%seen);
      parse_labels($self, $labels) if length($labels);
   }
   undef $self->output;
   $self->weight=$zero_weight;
   $self;
}
####################################################################################
sub append_precondition {
   my ($self, $precond, $checks_definedness, $override)=@_;
   $precond->flags= $checks_definedness ? $is_precondition | $is_definedness_check : $is_precondition;
   $precond->header="precondition " . $precond->header . " ( " . $self->header . " )";
   push @{$self->preconditions}, $precond;
   if ($override) {
      if ($Application::plausibility_checks) {
         $self->flags & $is_restricted_spez
           or croak( "useless or repeated `override precondition'" );
      }
      $self->flags &= ~$is_restricted_spez;
   }
}

sub append_existence_check {
   my ($self, $not, $req_string, $proto)=@_;
   my $req=$proto->encode_read_request($req_string);
   my $neg=length($not);
   my $precond=special Rule(':',
                            $neg
                            ? sub { !defined($_[0]) || is_boolean_false($_[0]->lookup_request($req, 1)) }  # forbid shortcuts
                            : sub { defined($_[0]) && is_object($_[0]->lookup_request($req, 1)) },
                            $proto);
   $precond->header="precondition : ${not}exists($req_string) ( " . $self->header . " )";
   $precond->flags=$is_precondition;
   push @{$self->preconditions}, $precond;
}

sub append_weight {
   my ($self, $major, $minor, $spec)=@_;
   if (defined $major) {
      $self->weight=[ $major, $minor ];
      assign_max($max_major, $major+1); # reserve the highest category for preference violation penalties
   } else {
      $self->weight=$zero_weight;
   }
   if ($spec) {
      $spec->flags=$is_precondition | $is_dyn_weight;
      $spec->header="weight " . $spec->header . " ( " . $self->header . " )";
      $self->dyn_weight=$spec;
      push @{$self->preconditions}, $spec;
   }
}

sub append_permutation {
   my ($self, $perm_name)=@_;
   my @perm_path=$self->defined_for->encode_descending_path($perm_name);
   if ($Application::plausibility_checks) {
      if (Property::find_first_in_path(\@perm_path, $Property::is_permutation) != $#perm_path) {
         croak( "$perm_name is not a permutation" );
      }
      if (defined $self->with_permutation) {
         croak( "Sorry, multiple permutations per production rule are not implemented" );
      }
   }
   $self->with_permutation=new CreatingPermutation($self, bless \@perm_path, "Polymake::Core::Rule::PermutationPath");
}
####################################################################################
sub has_matching_input {
   my ($self, $path)=@_;
   my $lp=@$path;
   foreach my $input (@{$self->input}) {
      foreach my $input_path (@$input) {
        if ($lp==@$input_path) {
           my $match=Property::equal_path_prefixes($path, $input_path);
           return 1 if $match==$lp or $match+1==$lp && $input_path->[-1]->key == $path->[-1]->key;
        }
      }
   }
   0
}
####################################################################################
sub flip_path_for_twin {
   my ($path, $proto, $twin_prop)=@_;
   my $result;
   if ($path->[0]->key == $twin_prop->key) {
      $result=[ @$path[1..$#$path] ];
   } elsif ($twin_prop->type->isa($path->[0]->defined_for)) {
      $result=[ $twin_prop, @$path ];
   } else {
      return;
   }
   if (is_object($path)) {
      inherit_class($result, $path);
   } elsif (my $flags=get_array_flags($path)) {
      set_array_flags($result, $flags);
   }
   $result
}

sub flip_rule_for_twin {
   my ($self, $twin_prop)=@_;
   my $flipped_input=[ map { [ map { flip_path_for_twin($_, $self->defined_for, $twin_prop) // return } @$_ ] } @{$self->input} ];
   my $flipped_output= defined($self->output) ? [ map { flip_path_for_twin($_, $self->defined_for, $twin_prop) // return } @{$self->output} ] : undef;
   my $flipped_perm_path= defined($self->with_permutation) ? flip_path_for_twin($self->with_permutation->perm_path, $self->defined_for, $twin_prop) // return : undef;
   my $code=$self->code;
   my $flipped_rule=_new($self, $self->header." (applied to ".$twin_prop->name.")",
                         $code && ($self->flags & $is_precondition
                                   ? sub { my ($this)=@_; $code->($this->descend([ $twin_prop, undef ])) }
                                   : sub { my ($this)=@_; $code->($this->descend_and_create([ $twin_prop, undef ])) } ),
                         $self->defined_for, $self->credit);
   $flipped_rule->input=$flipped_input;
   $flipped_rule->output=$flipped_output;
   $flipped_rule->flags=$self->flags;
   $flipped_rule->weight=$self->weight;
   @{$flipped_rule->preconditions}=map { flip_rule_for_twin($_, $twin_prop) } @{$self->preconditions};
   if (defined($self->dyn_weight)) {
      $flipped_rule->dyn_weight=$flipped_rule->preconditions->[list_index($self->preconditions, $self->dyn_weight)];
   }
   if (defined($flipped_perm_path)) {
      $flipped_rule->with_permutation=new CreatingPermutation($flipped_rule, $flipped_perm_path);
   }
   $flipped_rule->not_for_twin=$twin_prop->key;
   $flipped_rule;
}
####################################################################################
sub needs_finalization {
   my ($self)=@_;

   my @path_to_perm;
   foreach my $input (@{$self->input}) {
      if (get_array_flags($input) & $Property::is_permutation) {
         if ($Application::plausibility_checks) {
            if ($self->flags & $is_initial) {
               croak( "initial rules may not trigger permutations" );
            }
            foreach my $path (@$input) {
               my $perm_index=Property::find_first_in_path($path, $Property::is_permutation);
               if ($perm_index<0) {
                  croak( "A permuted subobject mixed with non-permuted properties in one source group" );
               }
               if (@path_to_perm) {
                  if (Property::equal_path_prefixes(\@path_to_perm, $path) <= $perm_index) {
                     croak( "Different permuted subobjects occur as sources in the same rule" );
                  }
               }
               @path_to_perm=@$path[0..$perm_index];
               if (grep { $_->flags & $Property::is_permutation } @$path[$perm_index+1..$#$path]) {
                  croak( "Nested permuted subobjects are not supported" );
               }
            }
         } else {
            @path_to_perm = @{$input->[0]}[0..Property::find_first_in_path($input->[0], $Property::is_permutation)];
            last;
         }
      }
   }

   if (@path_to_perm) {
      if ($path_to_perm[-1]->analyze_rule($self)) {
         $self->flags |= $is_perm_restoring;
         return;
      }
   } else {
      croak( "Only permutation rules may be labeled as nonexistent") if $self->code==\&nonexistent;
      push @{$self->defined_for->production_rules}, $self;
   }
   $self
}
####################################################################################
sub finalize {
   my ($self, $for_twin_rule)=@_;
   my $proto=$self->defined_for;
   my $perm_deputy=$self->with_permutation;
   my $perm_path=$perm_deputy && $perm_deputy->perm_path;

   if ($self->flags & $is_initial) {
      if (defined $perm_deputy) {
         die "rule ", $self->header, " can't trigger permutations at ", $self->source_location, "\n";
      }
      $proto->add_producers_of(ObjectType::init_pseudo_prop()->key, $self);

      foreach my $input (@{$self->input}) {
         foreach my $input_path (grep { @$_ > 1 } @$input) {
            (undef, my @ancestors)=reverse(@$input_path);
            my $prod_key=ObjectType::init_pseudo_prop()->get_prod_key(@ancestors);
            $proto->add_producers_of($prod_key, $self);
         }
      }

      if (@{$self->output}) {
         # an initial rule producing some new properties is only applicable if none of those exists
         my $non_existence_checker=sub {
            # fail on any existing or undefined property
            not grep { !is_boolean_false($_[0]->lookup_descending_path($_, 1)) } @{$self->output}
         };
         my $precond=special Rule(":", $non_existence_checker, $self->defined_for);
         $precond->header="precondition: " . join(" && ", map { "!exists(" . Property::print_path($_) . ")" } @{$self->output}) . " ( " . $self->header . " )";
         $precond->flags=$is_precondition;
         push @{$self->preconditions}, $precond;
      }
      return;
   }

   my (%created_new_multi, %twin_targets);
   my $i=0;
   foreach my $output (@{$self->output}) {
      my ($created_prop, @prod);
      if (@$output==1) {
         $created_prop=$output->[0];
         if ($perm_path && @$perm_path==1) {
            if (defined (my $prod_list=$perm_path->[0]->sensitive_props->{$created_prop->key})) {
               my $action=new PermAction($self, $i, $prod_list, 0);
               push @{$perm_deputy->actions}, $action;
               push @prod, $action if @$prod_list;
            } else {
               push @prod, $perm_deputy;
            }
         }
         $proto->add_producers_of($created_prop->key, $self, @prod);

      } else {
         my ($prop, @ancestors)=reverse(@$output);
         $created_prop=$prop;

         if ($perm_path && Property::equal_path_prefixes($perm_path, $output)==(my $perm_depth=$#$perm_path)) {
            my $permutation=$perm_path->[-1];
            my $sub_perm_hash=$permutation->sub_permutations;
            local_clip_front($output, $perm_depth);
            my $last=@$output-1;
            my $sub_perm_exists;
            for (my $depth=1; $depth<=$last; ++$depth) {
               if (defined (my $sub_permutation=$sub_perm_hash->{$output->[$depth-1]->key})) {
                  if (defined (my $prod_list=
                               $depth==$last ? $sub_permutation->sensitive_props->{$output->[$depth]->key}
                                             : $sub_permutation->find_sensitive_sub_property(@$output[$depth..$last]))) {
                     my $action=new PermAction($self, $i, $prod_list, $depth+$perm_depth, $sub_permutation);
                     push @{$perm_deputy->actions}, $action;
                     push @prod, $action;
                     $sub_perm_exists=1;
                     last;
                  }
               }
               defined( $sub_perm_hash=$sub_perm_hash->{$Permutation::sub_key} ) &&
               defined( $sub_perm_hash=$sub_perm_hash->{$output->[$depth-1]->key} )
               or last;
            }
            unless ($sub_perm_exists) {
               if (defined (my $prod_list=$permutation->find_sensitive_sub_property(@$output))) {
                  my $action=new PermAction($self, $i, $prod_list, $perm_depth, $perm_depth && $permutation);
                  push @{$perm_deputy->actions}, $action;
                  push @prod, $action;
               } else {
                  push @prod, $perm_deputy;
               }
            }
         }

         my $flags=get_array_flags($output);
         my $any_mult_depth=$flags >> $any_mult_depth_shift;
         my $common_prefix;  # the longest initial subpath of output occurring among inputs
         my $bad_twin;

         if ($flags & $Property::is_permutation) {
            # rules dealing with permutations may not create new subobjects: pretend a full match
            $common_prefix=$#$output;
         } else {
            if (!$for_twin_rule && $ancestors[-1]->flags & $Property::is_twin) {
               $twin_targets{$ancestors[-1]}=1;
               $bad_twin=1;
            }
            if ($any_mult_depth==0 || $Application::plausibility_checks) {
               $common_prefix=0;
               foreach my $rule ($self, @{$self->preconditions}) {
                  foreach my $input (@{$rule->input}) {
                     foreach my $input_path (@$input) {
                        my $l=Property::equal_path_prefixes($output, $input_path);
                        assign_max($common_prefix, $l);
                        if ($Application::plausibility_checks) {
                           $bad_twin &&= $l>0 if $rule==$self;

                           if ($flags & $Property::is_multiple_new &&
                               $l < $#$output &&
                               $l < $#$input_path &&
                               $output->[$l]->flags & $Property::is_multiple_new &&
                               $output->[$l]->property == $input_path->[$l]) {
                              die "Multiple subobject ", $output->[$l]->name, " can't be created because it occurs among ",
                                  $rule==$self ? "rule" : "precondition", " sources at ", $self->source_location, "\n";
                           }
                           if ($any_mult_depth && $any_mult_depth <= $l) {
                              die "Superfluous attribute `any' for multiple subobject ", $output->[$any_mult_depth-1]->name,
                                  " because it occurs among ",
                                  $rule==$self ? "rule" : "precondition", " sources at ", $self->source_location, "\n";
                           }
                        }
                     }
                  }
               }
               if ($Application::plausibility_checks && $bad_twin) {
                  die "Twin property ", $ancestors[-1]->name, " occurs in all sources and in at least one target - rule should be flipped at ",
                      $self->source_location, "\n";
               }
            }
            if ($flags & $Property::is_multiple_new) {
               # strip off all properties below the first new instance,
               # they are not stored in producers lists
               my $pos=Property::find_first_in_path($output, $Property::is_multiple_new);
               $prop=$output->[$pos];
               splice @ancestors, 0, @ancestors-$pos;
            }
            if ($flags & $Property::is_multiple) {
               if ($Application::plausibility_checks) {
                  foreach ($common_prefix..($any_mult_depth || @$output)-2) {
                     if (($output->[$_]->flags & ($Property::is_multiple | $Property::is_multiple_new)) == $Property::is_multiple) {
                        die "Multiple subobject ", $output->[$_]->name, " in path ", Property::print_path($output),
                            " must be attributed with `any' or `new' because it does not occur among rule or precondition sources at ",
                            $self->source_location, "\n";
                     }
                  }
               }
               if ($any_mult_depth) {
                  # find the last multiple property with `any' attribute
                  $common_prefix=$any_mult_depth;
                  foreach ($any_mult_depth+1..$#$output-1) {
                     if (($output->[$_]->flags & ($Property::is_multiple | $Property::is_multiple_new)) == $Property::is_multiple) {
                        $common_prefix=$_;
                     }
                  }
               }
            }
         }

         for (;;) {
            my $prod_key=$prop->get_prod_key(@ancestors);
            unless ($flags & $Property::is_multiple_new && $created_new_multi{$prod_key}++) {
               $proto->add_producers_of($prod_key, $self, @prod);
            }
            last if @ancestors <= $common_prefix;
            $prop=shift @ancestors;
            @prod=($perm_deputy) if @prod;  # subobjects are created as soon as the rule starts, not when the atomic property is permuted back
         }
      }
      if ($created_prop->flags & $Property::is_subobject) {
         $created_prop->flags |= $Property::is_produced_as_whole;
      }
   } continue { ++$i }

   if ($Application::plausibility_checks) {
      if (defined($perm_deputy) && !@{$perm_deputy->actions}) {
         die "None of the target properties is sensitive to the permutation ",
             Property::print_path($perm_deputy->perm_path), " which pretends to be triggered at ", $self->source_location, "\n";
      }
      if ($self->flags & $is_restricted_spez && defined($self->defined_for->preconditions)) {
         foreach my $output (@{$self->output}) {
            foreach my $precond (@{$self->defined_for->preconditions}) {
               if ($precond->has_matching_input($output)) {
                  die "Target ", Property::print_path($output), " is an input to a specialization ", $precond->header, "\n",
                      (@{$self->preconditions} ? "one of preconditions of the production rule must be declared as `override'"
                                               : "the production rule must be equipped with an `override' precondition"),
                      " at ", $self->source_location, "\n";
               }
            }
         }
      }
   }

   if (@{$self->labels}) {
      $proto->add_rule_labels($self, $self->labels);
      $proto->add_rule_labels($perm_deputy, $self->labels) if defined $perm_deputy;
   }

   if (defined $self->overridden_in) {
      $proto->override_rule(@$_) for @{$self->overridden_in};
      undef $self->overridden_in;
   }

   if (!$for_twin_rule && keys %twin_targets) {
      foreach my $twin_prop (keys %twin_targets) {
         if (defined (my $flipped_rule=flip_rule_for_twin($self, $twin_prop))) {
            push @{$proto->production_rules}, $flipped_rule;
            finalize($flipped_rule, 1);
         }
      }
   }
}

sub sensitivity_check { $_[0]->with_permutation->perm_path }

####################################################################################
# protected:
sub prepare_header_search_pattern {
   # remove whitespaces from input
   $_[0] =~ s/\s+//g;
   # expect optional spaces around punctuation characters
   $_[0] =~ s/(?<=\w)(?=[|:,])/\\s*/g;
   $_[0] =~ s/(?<=[|:,])(?=\w)/\\s*/g;
   # protect characters having special meaning
   $_[0] =~ s/([.|()])/\\$1/g;
}

# protected:
# prepare a pattern for searching for rules by header
sub header_search_pattern {
   &prepare_header_search_pattern;
   # expect optional labels omitted in the search pattern
   $_[0] = qr/^(?: $hier_ids_re \s*:\s* )? (?-x:$_[0]) $/x;
}
####################################################################################
# private:
# create a closure working as a filter in Object::copy
sub create_splitting_filter {
   my ($rule, $permutation, $descend_path, $prop_hash, $sub_perm_hash, $depth)=@_;
   sub {
      my ($dst, $pv, $prop)=@_;
      $prop //= $pv->property;
      if (defined ($prop_hash) &&
          defined (my $prod_list=$prop_hash->{$prop->key})) {
         my $out=[ @$descend_path, $prop ];
         push @{$rule->output}, $out;
         my $action=new PermAction($rule, $#{$rule->output}, $prod_list, $depth, $depth && $permutation);
         push @{$rule->with_permutation->actions}, $action;
         my $perm_dst=$dst->add_perm_in_parent($#$descend_path-$depth+1, $permutation, $rule);
         local_clip_front($out, $depth) if $depth;
         my ($perm_obj)=$perm_dst->descend_and_create($out);
         return ($perm_obj, $pv->copy($perm_obj, $prop));
      }

      if (defined ($sub_perm_hash) &&
          defined (my $sub_permutation=$sub_perm_hash->{$prop->key})) {
         return ($dst, $pv->copy($dst, $prop, create_splitting_filter($rule, $sub_permutation, [ @$descend_path, $prop ],
                                                                      $sub_permutation->sensitive_props, $sub_permutation->sub_permutations,
                                                                      $#$descend_path+2)));
      }

      my $down_prop=$prop_hash->{$Permutation::sub_key};
      $down_prop &&= $down_prop->{$prop->key};
      my $down_sub=$sub_perm_hash->{$Permutation::sub_key};
      $down_sub &&= $down_sub->{$prop->key};
      if (defined($down_prop) || defined($down_sub)) {
         return ($dst, $pv->copy($dst, $prop, create_splitting_filter($rule, $permutation, [ @$descend_path, $prop ],
                                                                      $down_prop, $down_sub, $depth)));
      }

      return ($dst, $pv->copy($dst, $prop));
   }
}

sub permuting {
   my ($pkg, $proto, $perm_path)=@_;
   my $self=_new($pkg, 'copying', undef, $proto);
   $self->weight=$zero_weight;
   $self->with_permutation=new FakeCreatingPermutation($self, $perm_path);
   my $permutation=local_pop($perm_path);
   my $filter;
   if (@$perm_path) {
      my ($hash, $prop)=Permutation::descend_and_create(my $sub_perms={ }, @$perm_path);
      $hash->{$prop->key}=$permutation;
      $hash->{$Permutation::sub_key}->{$prop->key}=$permutation->sub_permutations;
      $filter=create_splitting_filter($self, undef, [ ], undef, $sub_perms, 0);
   } else {
      $filter=create_splitting_filter($self, $permutation, [ ], $permutation->sensitive_props, $permutation->sub_permutations, 0);
   }
   ($self, $filter)
}

####################################################################################
#
#  Reduced constructor for pseudo-rules
#
#  create Rule 'Header', [ input list ], allow_undefs
sub create {
   my $self=_new(splice @_, 0, 2);
   $self->input=shift;
   undef $self->output;
   $self->flags= (shift) ? $is_function | $is_definedness_check : $is_function;
   $self->weight=$zero_weight;
   $self
}
####################################################################################
my $break_reason;

sub break_rule {
   $break_reason=shift;
   if ($break_reason eq 'ALRM') {
      $SIG{INT}='IGNORE';
      $SIG{ALRM}='IGNORE';
      kill -(SIGINT), $$;       # kill the subprocesses
   }
   die "\n" if waitpid(-$$,WNOHANG)==-1;        # there were no subprocesses - leave eval{} in execute().
}

my $breaksignals=new POSIX::SigSet(SIGINT, SIGALRM);
my $sa_break=new POSIX::SigAction(\&break_rule, $breaksignals, 0);
my $sa_INT_save=new POSIX::SigAction('IGNORE');

# Execute the rule on a separate transaction level
sub execute {
   my ($self, $object, $force)=@_;
   unless ($self->flags & $Rule::is_precondition) {
      my $trans=new RuleTransaction($object, $self);
      unless ($force || defined($trans->changed)) {
         $trans->rollback($object);
         return $exec_OK;
      }
   }
   dbg_print("applying rule ", $self->header) if $Verbose::rules>2;
   &_execute;
}

# private:
sub _execute {
   my ($self, $object)=@_;
   my $died=1;
   my ($rc, $retval)=($exec_failed);
   undef $break_reason;
   eval {
      sigaction SIGINT, $sa_break, $sa_INT_save;
      sigaction SIGALRM, $sa_break;
      my $alarm_time=$timeout && !($self->flags & $is_precondition);
      alarm $alarm_time if $alarm_time;
      if (wantarray || ($self->flags & $is_precondition)) {
         $retval=$self->code->($object);
      } else {
         # call production rules in void context
         $self->code->($object);
      }
      alarm 0 if $alarm_time;
      if ($self->flags & $is_precondition) {
         $rc= $retval ? $exec_OK : $exec_failed;
         if ($Verbose::rules>1) {
            if ($retval) {
               dbg_print( $self->header, " satisfied" ) if $self->flags & $is_precondition and $Verbose::rules>2;
            } else {
               warn_print( $self->header, $self->flags & $is_precondition ? " not satisfied" : " failed" );
            }
         }
      } else {
         $object->add_credit($self->credit) if defined($self->credit);
         # ignore the return code of the production rule
         $rc=$exec_OK;
      }
      $object->commit unless ($self->flags & $Rule::is_precondition);
      $died=0;
   };
   sigaction SIGINT, $sa_INT_save;
   $SIG{ALRM}='IGNORE';

   if ($died) {
      $rc=$exec_failed;
      if ($break_reason eq "ALRM") {
         $@="timeout elapsed\n";
      } elsif ($break_reason eq "INT") {
         $@="killed by signal\n";
         $rc=$exec_retry;
      }
      $object->failed_rules->{$self}=1;
      $object->failed_rules->{$self->with_permutation}=1 if defined $self->with_permutation;
      $object->rollback unless ($self->flags & $Rule::is_precondition);
   }
   wantarray ? ($rc, $retval) : $rc;
}
####################################################################################
sub list_results {
   my ($self, $proto)=@_;
   $self->flags & ($is_function | $is_precondition)
     ||
   scalar(grep { get_array_flags($_) & $Property::is_permutation } @{$self->input})
   ? ()
   : map {
        my $obj_type=$proto;
        join(".", map {
           my $name=$obj_type->property_name($_);
           $obj_type=$_->type;
           $name
        } @$_)
     } @{$self->output}
}
####################################################################################
package Polymake::Core::Rule::Shortcut;

use Polymake::Struct (
   [ '@ISA' => 'Rule' ],
   [ new => '$$$' ],
   [ '$code' => 'undef' ],
   [ '$output' => '[ #1 ]' ],
   [ '$input' => '[ #2 ]' ],
   [ '$flags' => '$is_production' ],
   [ '$weight' => '$zero_weight' ],
);

sub new {
   my $self=&_new;
   $self->header=Property::print_path($self->output->[0]) . " = " . Property::print_path($self->input->[0]);
   $self;
}

sub append_weight {
   croak( "A shortcut rule has always zero weight" );
}

sub append_permutation {
   croak( "A shortcut rule cannot trigger any permutations" );
}

sub descend_to_source {
   my ($self, $object)=@_;
   my ($prop, $content_index);
   if (@{$self->input}) {
      if (($object, $prop)=$object->descend($self->input->[0]->[0])
          and defined ($content_index=$object->dictionary->{$prop->key})) {
         $object->contents->[$content_index]
      } else {
         undef
      }
   } else {
      $object
   }
}

sub execute {
   my ($self, $object)=@_;
   dbg_print("applying rule ", $self->header) if $Verbose::rules>2;
   if (defined (my $pv=&descend_to_source)) {
      my $trans=$object->begin_transaction;
      my ($subobj, $prop)=$object->descend_and_create($self->output->[0]);
      my $ref_pv=new PropertyValue($prop, ($pv!=$object) ? ($pv->value, defined($pv->value) && $PropertyValue::is_strong_ref)
                                                         : ($object, $PropertyValue::is_weak_ref));
      push @{$subobj->contents}, $ref_pv;
      $subobj->dictionary->{$prop->key}=$#{$subobj->contents};
      $trans->commit($object);
      wantarray ? ($exec_OK, $ref_pv) : $exec_OK;
   } else {
      $exec_infeasible;
   }
}

sub source_location {
   my $self=shift;
   ($self->credit->[0], " line ", $self->credit->[1])
}
####################################################################################
package Polymake::Core::Rule::Dummy;

use Polymake::Struct (
   [ '@ISA' => 'Rule' ],
   [ new => '$$$' ],
   [ '$flags' => '#2' ],
   [ '$weight' => '$zero_weight' ],
);

*new=\&_new;

sub execute { $exec_OK }

####################################################################################
package Polymake::Core::Rule::Deputy;

# Base class for various rule proxies, especially those used in the Scheduler.
use Polymake::Struct (
   [ new => '$' ],
   [ '$rule' => '#1' ],     # the proper Rule doing the hard job
   [ '$flags' => '#1->flags' ],
   [ '$weight' => '#1->weight' ],
);

sub header { $_[0]->rule->header }
sub list_results { $_[0]->rule->list_results($_[1]) }
sub input { $_[0]->rule->input }
sub output { $_[0]->rule->output }
sub labels { $_[0]->rule->labels }
sub preconditions { $_[0]->rule->preconditions }
sub dyn_weight { $_[0]->rule->dyn_weight }
sub overridden_in { $_[0]->rule->overridden_in }
sub with_permutation { $_[0]->rule->with_permutation }
sub defined_for { $_[0]->rule->defined_for }
sub has_matching_input { $_[0]->rule->has_matching_input($_[1]) }
sub not_for_twin { $_[0]->rule->not_for_twin }

####################################################################################
package __::CreatingPermutation;

use Polymake::Struct (
   [ '@ISA' => 'Deputy' ],
   [ new => '$$' ],
   [ '$rule' => 'weak(#1)' ], # production Rule
   [ '$perm_path' => '#2' ],    # property path leading to a permutation
   '@actions',                  # PermAction descriptors
);

sub header {
   my ($self)=@_;
   $self->rule->header . " ( creating " . Property::print_path($self->perm_path) . " )";
}

sub with_permutation { undef }
sub without_permutation { $_[0]->rule }

sub execute {
   my ($self, $object)=@_;
   new RuleTransactionWithPerm($object, $self);
   dbg_print("applying rule ", $self->header) if $Verbose::rules>2;
   _execute($self->rule, $object);
}

sub record_sensitivity {
   my ($self, $object)=@_;
   my ($sub_obj, $perm)=$object->descend($self->perm_path);
   $sub_obj->sensitive_to->{$perm->key}=1;
   foreach my $action (@{$self->actions}) {
      if ($action->depth) {
         my $out=$self->rule->output->[$action->output];
         local_clip_back($out, $action->depth);
         my ($sub_obj)=$object->descend($out);
         $sub_obj->sensitive_to->{$action->sub_permutation->key}=1;
      }
   }
}

package __::FakeCreatingPermutation;

use Polymake::Struct [ '@ISA' => 'CreatingPermutation' ];

sub execute {
   my $object=$_[1];
   $object->commit;
   &record_sensitivity;
   $exec_OK
}

package __::PermAction;

use Polymake::Struct (
   [ new => '$$$$;$' ],
   [ '$perm_trigger' => 'weak(#1)' ], # production rule triggering the permutation
   [ '$output' => '#2' ],               # index into its output where the sensitive property lies
   [ '$producers' => '#3' ],            # ( Rule ) moving the output property back to the basis
   [ '$depth' => '#4' ],                # how deep to descend in the base object
   [ '$sub_permutation' => '#5' ],      # Permutation for the subobject
);

sub header {
   my $self=shift;
   Property::print_path($self->perm_trigger->output->[$self->output])
   . " extracted from " . Property::print_path($self->perm_path) . " after " . $self->perm_trigger->header;
}
sub list_results { () }
sub perm_path { $_[0]->perm_trigger->with_permutation->perm_path }
sub flags { $is_perm_action }
sub weight { $zero_weight }
sub enabled { @{$_[0]->producers}!=0 }

sub preconditions { [ ] }
sub with_permutation { undef }
sub overridden_in { undef }
sub not_for_twin { $_[0]->perm_trigger->not_for_twin }

####################################################################################
package __::PermutationPath;

sub header { "sensitivity check for " . &Property::print_path }
sub list_results { () }
sub flags { $is_function }
sub weight { $zero_weight }
sub preconditions { [ ] }
sub with_permutation { undef }

sub execute {
   my ($self, $object)=@_;
   dbg_print("performing ", $self->header) if $Verbose::rules>2;
   my ($sub_obj, $perm)=$object->descend($self);
   $sub_obj->has_sensitive_to($perm) ? $exec_failed : $exec_OK
}

sub find_sensitive_sub_property {
   my ($self, @path)=@_;
   if (Property::equal_path_prefixes($self, \@path)==$#$self) {
      $self->[-1]->find_sensitive_sub_property(splice @path, $#$self)
   } else {
      undef
   }
}

####################################################################################
package __::Credit;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$product' => '#1' ],
   [ '$text' => '#2' ],
   [ '$file_string' => 'undef' ],
   [ '$shown' => '0' ],
);

declare $hide=3;   # suppresses display if assigned to ->shown

sub display {
   my $self=shift;
   dbg_print( "used package ", $self->product, "\n", $self->text, "\n" );
   $self->shown=1;
}

sub toFileString {
   my $self=shift;
   $self->file_string ||= do {
      my ($copyright)= $self->text =~ /(copyright\b.*)/im;
      my ($URL)= $self->text =~ /(?:url:?\s+)(\S+)/im;
      if (!defined($URL) && $self->text =~ m{(?:\w+://|(?:www|ftp)\.)\S+}) {
         $URL=$&;
      }
      defined($copyright) && defined($URL) ? "\n$copyright\n$URL\n" : $self->text
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
