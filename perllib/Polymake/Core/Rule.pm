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
   my $last_limb;
   {
      $last_limb=local_pop($path);
      $seen=($seen->{$_->key} ||= { }) for @$path;
   }
   foreach my $prop ($last_limb->all_choices) {
      if (my $check=$tag>0 ? $seen->{$prop->key}++ : $seen->{$prop->key}--) {
         my $name=Property::print_path($path);
         if ($tag>0) {
            croak( "rule source '$name' occurs twice" );
         } elsif ($check<0) {
            croak( "rule target '$name' occurs twice" );
         } else {
            croak( "A rule may not have the property '$name' both as a source and a target" );
         }
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
         check_repeated_properties($input, $seen, 1);
         if (grep { $_->flags & $Property::is_multiple } (is_object($input) ? $input : $input->[-1])->all_choices) {
            croak( "A rule may not consume a multiple subobject as a whole but only its single properties" );
         }
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
            if (defined $prop) {
               $proto=$prop->specialization($proto, 1)->type;
            }
            unless ($prop=$proto->lookup_property($prop_name)) {
               $proto==$_[0]
               ? croak( "Unknown property ", $_[0]->full_name, "::$prop_name" )
               : croak( "Unknown property ", $proto->full_name, "::$_ in path $target" );
            }
            push @out, $prop;
            if ($prop->flags & $Property::is_multiple) {
               if (defined $attrib) {
                  if ($Application::plausibility_checks) {
                     foreach my $output (@{$self->output}) {
                        my $l=equal_list_prefixes(\@out, $output);
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
   my ($self, $not, $req, $proto)=@_;
   my $prop_list=$proto->encode_read_request($req);
   my $neg=length($not);
   my $precond=special Rule(':',
                            $neg
                            ? sub { is_boolean_false($_[0]->lookup_request($prop_list, 1)) }  # forbid shortcuts
                            : sub { is_object($_[0]->lookup_request($prop_list, 1)) },
                            $proto);
   $precond->header="precondition : ${not}exists($req) ( " . $self->header . " )";
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
   my ($self, $perm)=@_;
   if ($Application::plausibility_checks) {
      if (defined $self->with_permutation) {
         croak( "Sorry, multiple permutations per production rule are not implemented" );
      }
   }
   $self->with_permutation=new CreatingPermutation($self, $perm);
}
####################################################################################
sub matching_input {
   my ($self, $path)=@_;
   my $lp=@$path;
   foreach my $input (@{$self->input}) {
      if ($lp==@$input) {
         my $match=equal_list_prefixes($path, $input);
         return $input if $match==$lp or $match+1==$lp && $input->[-1]->matches($path->[-1]);
      }
   }
   undef
}
####################################################################################
sub needs_finalization {
   my ($self)=@_;

   my $input_perm;
   foreach my $input (@{$self->input}) {
      if (!is_object($input) and get_array_flags($input) & $Property::is_permutation) {
         if ($Application::plausibility_checks) {
            my @perms_in_path=grep { $_->flags & $Property::is_permutation } @$input;
            if ($self->flags & $is_initial) {
               croak( "initial rules may not trigger permutations" );
            }
            if (@perms_in_path>1 || defined($input_perm) && $input_perm != $perms_in_path[0]) {
               croak( "Properties from several permutation subobjects occur as inputs in the same rule" );
            }
            $input_perm //= $perms_in_path[0];
         } else {
            $input_perm=$input->[Property::find_first_in_path($input, $Property::is_permutation)];
            last;
         }
      }
   }

   if (defined $input_perm) {
      if ($input_perm->analyze_rule($self)) {
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
   my ($self)=@_;
   my $proto=$self->defined_for;
   my $need_invalidate=keys %{$proto->all_producers};
   my $perm_deputy=$self->with_permutation;
   my $permutation=$perm_deputy && $perm_deputy->permutation;

   if ($self->flags & $is_initial) {
      if (defined $perm_deputy) {
         die "rule ", $self->header, " can't trigger permutations at ", $self->source_location, "\n";
      }
      push @{$proto->producers->{ObjectType::init_pseudo_prop()->key}}, $self;
      $proto->invalidate_prod_cache(ObjectType::init_pseudo_prop()->key) if $need_invalidate;

      foreach my $input (grep { @$_ > 1 } @{$self->input}) {
         (undef, my @ancestors)=reverse(@$input);
         my $prod_key=ObjectType::init_pseudo_prop()->get_prod_key(@ancestors);
         push @{$proto->producers->{$prod_key}}, $self;
         $proto->invalidate_prod_cache($prod_key) if $need_invalidate;
      }

      if (@{$self->output}) {
         # an initial rule producing some new properties is only applicable if none of those exists
         my $non_existence_checker=sub {
            # fail on any existing or undefined property
            not grep { !is_boolean_false($_[0]->lookup_request($_, 1)) } @{$self->output}
         };
         my $precond=special Rule(":", $non_existence_checker, $self->defined_for);
         $precond->header="precondition: " . join(" && ", map { "!exists(" . Property::print_path($_) . ")" } @{$self->output}) . " ( " . $self->header . " )";
         $precond->flags=$is_precondition;
         push @{$self->preconditions}, $precond;
      }
      return;
   }

   my %created_new_multi;
   my $i=0;
   foreach my $output (@{$self->output}) {
      my ($created_prop, @prod);
      if (@$output==1) {
         $created_prop=$output->[0];
         if (defined $perm_deputy) {
            if (defined (my $prod_list=$permutation->sensitive_props->{$created_prop->key})) {
               my $action=new PermAction($self, $i, $prod_list);
               push @{$perm_deputy->actions}, $action;
               push @prod, $action if @$prod_list;
            } else {
               push @prod, $perm_deputy;
            }
         }
         push @{$proto->producers->{$created_prop->key}}, $self, @prod;
         $proto->invalidate_prod_cache($created_prop->key) if $need_invalidate;

      } else {
         my ($prop, @ancestors)=reverse(@$output);
         $created_prop=$prop;
         if (defined $perm_deputy) {
            my $sub_perm_hash=$permutation->sub_permutations;
            my $last=@$output-1;
            my $store_full_path=1;
            for (my $depth=1; $depth<=$last; ++$depth) {
               if (defined (my $sub_permutation=$sub_perm_hash->{$output->[$depth-1]->key})) {
                  if (defined (my $prod_list=
                               $depth==$last ? $sub_permutation->sensitive_props->{$output->[$depth]->key}
                                             : $sub_permutation->find_sensitive_sub_property(@$output[$depth..$last]))) {
                     my $action=new PermAction($self, $i, $prod_list, $depth, $sub_permutation);
                     push @{$perm_deputy->actions}, $action;
                     push @prod, $action;
                     $store_full_path=0;
                     last;
                  }
               }
               defined( $sub_perm_hash=$sub_perm_hash->{$Permutation::sub_key} ) &&
               defined( $sub_perm_hash=$sub_perm_hash->{$output->[$depth-1]->key} )
               or last;
            }
            if ($store_full_path) {
               if (defined (my $prod_list=$permutation->find_sensitive_sub_property(@$output))) {
                  my $action=new PermAction($self, $i, $prod_list);
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

         if ($flags & $Property::is_permutation) {
            # rules dealing with permutations may not create new subobjects: pretend a full match
            $common_prefix=$#$output;
         } else {
            if ($any_mult_depth==0 || $Application::plausibility_checks) {
               $common_prefix=0;
               foreach my $rule ($self, @{$self->preconditions}) {
                  foreach my $input (@{$rule->input}) {
                     unless (is_object($input)) {
                        my $l=equal_list_prefixes($output, $input);
                        assign_max($common_prefix, $l);
                        if ($Application::plausibility_checks) {
                           if ($flags & $Property::is_multiple_new &&
                               $l < $#$output &&
                               $l < $#$input &&
                               $output->[$l]->flags & $Property::is_multiple_new &&
                               $output->[$l]->property == $input->[$l]) {
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
                            " must be attributed with `any' or `new' because it does not occur among rule or precondition sources at ", $self->source_location, "\n";
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
               push @{$proto->producers->{$prod_key}}, $self, @prod;
               $proto->invalidate_prod_cache($prod_key) if $need_invalidate;
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
             $perm_deputy->permutation->name, " which pretends to be triggered at ", $self->source_location, "\n";
      }
      if ($self->flags & $is_restricted_spez && defined($self->defined_for->preconditions)) {
         foreach my $output (@{$self->output}) {
            foreach my $precond (@{$self->defined_for->preconditions}) {
               if (defined $precond->matching_input($output)) {
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
}

sub checking_precondition {
   my ($self)=@_;
   my $permutation=$self->with_permutation->permutation;
   $permutation->sensitivity_check //= new SensitivityCheck($permutation);
}
####################################################################################
# protected:
sub prepare_header_search_pattern {
   # remove whitespaces from input
   $_[0] =~ s/\s+//g;
   # expect optional spaces around punctuation characters
   $_[0] =~ s/(?<=\w)(?=[|:,])/\\s*/g;
   $_[0] =~ s/(?<=[|:,])(?=\w)/\\s*/g;
   # protect characters having special meaning
   $_[0] =~ s/([.|])/\\$1/g;
}

# protected:
# prepare a pattern for searching for rules by header
sub header_search_pattern {
   &prepare_header_search_pattern;
   # expect optional labels omitted in the search pattern
   $_[0] = qr/^(?: $hier_ids_re \s*:\s* )? (?-x:$_[0]) $/x;
}
####################################################################################
# protected:
# create a closure working as a filter in Object::copy
sub create_splitting_filter {
   my ($rule, $perm_dst, $permutation, $descend_path, $prop_hash, $sub_perm_hash, $depth)=@_;
   sub {
      my ($dst, $pv)=@_;
      my $prop=$pv->property;
      if (defined ($prop_hash) &&
          defined (my $prod_list=$prop_hash->{$prop->key})) {
         my $out=[ @$descend_path, $prop ];
         push @{$rule->output}, $out;
         my $action=new PermAction($rule, $#{$rule->output}, $prod_list,
                                   $depth ? ($depth, $permutation) : ());
         push @{$rule->with_permutation->actions}, $action;
         $perm_dst ||= $dst->add_perm_in_parent($#$descend_path-$depth+1, $permutation);
         local_clip_front($out, $depth) if $depth;
         my ($perm_obj)=$perm_dst->descend_and_create($out);
         return ($perm_obj, $pv->copy($perm_obj));
      }

      if (defined ($sub_perm_hash) &&
          defined (my $sub_permutation=$sub_perm_hash->{$prop->key})) {
         return ($dst, $pv->copy($dst, create_splitting_filter($rule, undef, $sub_permutation, [ @$descend_path, $prop ],
                                                               $sub_permutation->sensitive_props, $sub_permutation->sub_permutations,
                                                               $#$descend_path+2)));
      }

      my $down_prop=$prop_hash->{$Permutation::sub_key};
      $down_prop &&= $down_prop->{$prop->key};
      my $down_sub=$sub_perm_hash->{$Permutation::sub_key};
      $down_sub &&= $down_sub->{$prop->key};
      if (defined($down_prop) || defined($down_sub)) {
         return ($dst, $pv->copy($dst, create_splitting_filter($rule, $perm_dst, $permutation, [ @$descend_path, $prop ],
                                                               $down_prop, $down_sub, $depth)));
      }

      return ($dst, $pv->copy($dst));
   }
}

sub permuting {
   my ($pkg, $proto, $permutation, @descend_to_permuted)=@_;
   my $self=_new($pkg, 'copying', undef, $proto);
   $self->weight=$zero_weight;
   if (@descend_to_permuted) {
      $permutation=$proto->create_permutation_for_ancestor($permutation, @descend_to_permuted);
   }
   $self->with_permutation=new FakeCreatingPermutation($self, $permutation);
   ($self, create_splitting_filter($self, undef, $permutation, [ ], $permutation->sensitive_props, $permutation->sub_permutations, 0))
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
   my $trans=new RuleTransaction($object, $self);
   unless ($force || ($self->flags & $is_precondition) || defined($trans->changed)) {
      $trans->rollback($object);
      return $exec_OK;
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
      sigaction SIGPIPE, $sa_break;
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
      $object->commit;
      $died=0;
   };
   sigaction SIGINT, $sa_INT_save;
   $SIG{ALRM}='IGNORE';
   $SIG{PIPE}='DEFAULT';

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
      $object->rollback;
   }
   wantarray ? ($rc, $retval) : $rc;
}
####################################################################################
sub list_results {
   my $self=shift;
   $self->flags & ($is_function | $is_precondition)
     ||
   scalar(grep { get_array_flags($_) & $Property::is_permutation } @{$self->input})
   ? ()
   : map {
        my $proto=$_[0];
        my $i=$#$_;
        join(".", map {
           my $name=$proto->property_name($_);
           if ($i--) {
              $proto=$_->specialization($proto)->type;
           }
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
      if (($object, $prop)=$object->descend($self->input->[0])
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
package __::Deputy;

# Base class for various rule proxies, especially those used in the Scheduler.
use Polymake::Struct (
   [ new => '$' ],
   [ '$rule' => '#1' ],     # the proper Rule doing the hard job
   [ '$flags' => '#1 ->flags' ],
   [ '$weight' => '#1 ->weight' ],
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
sub matching_input { $_[0]->rule->matching_input($_[1]) }
####################################################################################
package __::CreatingPermutation;

use Polymake::Struct (
   [ '@ISA' => 'Deputy' ],
   [ new => '$$' ],
   [ '$rule' => 'weak( #1 )' ], # production Rule
   [ '$permutation' => '#2' ],  # permutation Property
   '@actions',                  # PermAction descriptors
);

sub header {
   my ($self)=@_;
   $self->rule->header . " ( creating " . $self->permutation->name . " )";
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
   $object->sensitive_to->{$self->permutation->key}=1;
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
   [ new => '$$$;$$' ],
   [ '$perm_trigger' => 'weak( #1 )' ], # production rule triggering the permutation
   [ '$output' => '#2' ],               # index into its output where the sensitive property lies
   [ '$producers' => '#3' ],            # ( Rule ) moving the output property back to the basis
   [ '$depth' => '#4' ],                # how deep to descend in the base object
   [ '$sub_permutation' => '#5' ],      # Permutation for the subobject
);

sub header {
   my $self=shift;
   Property::print_path($self->perm_trigger->output->[$self->output])
   . " extracted from " . $self->permutation->name . " after " . $self->perm_trigger->header;
}
sub list_results { () }
sub permutation { $_[0]->perm_trigger->with_permutation->permutation }
sub flags { $is_perm_action }
sub weight { $zero_weight }
sub enabled { @{$_[0]->producers}!=0 }

sub preconditions { [ ] }
sub with_permutation { undef }
sub overridden_in { undef }

####################################################################################
package __::SensitivityCheck;

use Polymake::Struct (
  [ new => '$' ],
  [ '$permutation' => 'weak( #1 )' ],
);

sub header { "sensitivity check for " . (shift)->permutation->name }
sub list_results { () }
sub flags { $is_function }
sub weight { $zero_weight }
sub preconditions { [ ] }
sub with_permutation { undef }

sub execute {
   my ($self, $object)=@_;
   dbg_print("performing ", $self->header) if $Verbose::rules>2;
   $object->has_sensitive_to($self->permutation) ? $exec_failed : $exec_OK
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
