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

package Polymake::Test::Rule;

use Polymake::Struct (
   [ '@ISA' => 'Subgroup' ],
   [ new => '$$@' ],
   [ '$name' => '#1' ],
   [ '$group' => 'undef' ],
   [ '$header' => '#1' ],
   [ '$patterns' => '#2' ],
   [ '@options' => '@' ],
   [ '$rules' => 'undef' ],
   '@object_files',
);

sub new {
   my $self=&_new;
   $current->add_subgroup($self);
   $self->rules=find_matching_rules($self);
   my $app=$self->group->application;
   @{$self->object_files}=$self->group->env->shuffle->(
      map {
	 my @files=grep { -f $_ } glob(/\./ ? $_ : "$_.{".join(",", uniq(grep { length($_)>0 } map { @{$_->file_suffixes} } $app, values %{$app->used}))."}")
	   or die "no matching data files for pattern $_\n";
	 @files
      } split /\s+/, $self->patterns);
   $self;
}

sub run {
   my ($self)=@_;
   my $env=$self->group->env;
   $env->start_timers;
   my %tested_rules;
   foreach my $file (@{$self->object_files}) {
      if (defined (my $object=eval { $env->load_object_file($file) })) {
         $self->add_case(_new Object::WithRule($object, $self->rules, \%tested_rules, @{$self->options}));
      } else {
         return $self->report_error;
      }
   }
   if (@{$self->rules} > 1) {
      $self->add_case(_new TestFinalizer($self->rules, \%tested_rules));
   }
   $self->assess_cases
}

####################################################################################
package Polymake::Test::Rule::TestFinalizer;

use Polymake::Struct (
   [ '@ISA' => 'Case' ],
   [ new => '$$' ],
   [ '$name' => '"final"' ],
   [ '$max_exec_time' => '0' ],
   [ '$rules' => '#1' ],
   [ '$tested_rules' => '#2' ],
);

sub hidden { 1 }

sub execute {
   my ($self)=@_;
   if (my @untested=grep { !$self->tested_rules->{$_} } @{$self->rules}) {
      $self->fail_log="Following rules haven't been tested on any object due to missing sources, failing preconditions, or object type mismatch:\n".
		      join("", map { describe_rule($_)."\n" } @untested);
      0
   } else {
      1
   }
}

####################################################################################
package Polymake::Test::Rule;
my (%rule_cache, %rule_cache_fill);

sub rule_cache {
   my ($app)=@_;
   my $cache=($rule_cache{$app} //= { });
   my $skip = $rule_cache_fill{$app} // 0;
   if ($skip < @{$app->rules}) {
      $rule_cache_fill{$app}=@{$app->rules};

      foreach my $rule (@{$app->rules}[$skip..$#{$app->rules}]) {
         next if $rule->flags == Core::Rule::Flags::is_function || $rule->code == \&Core::Rule::nonexistent;
         my $header = $rule->header;
         $header =~ s/\s//g;
         push @{$cache->{$header}}, $rule;
      }
   }
   $cache
}

sub find_matching_rules {
   my ($self)=@_;
   my $header=$self->header;
   $header=~s/\s//g;
   my $in_rulefile= $header =~ s/\@(.*)// ? $1 : undef;
   my $precond= $header =~ s/&&(.*)// ? $1 : undef;

   if (my $rules=rule_cache($self->group->application)->{$header}) {
      if (defined($in_rulefile) || defined($precond)) {
	 my @matching_rules=@$rules;
	 if (defined $in_rulefile) {
	    @matching_rules=grep { defined($_->code) && sub_file($_->code) =~ m{(?:^|/)\Q$in_rulefile\E$} } @matching_rules
	      or croak( "no production rule is defined in file $in_rulefile" );
	 }
	 if (defined $precond) {
	    $precond=~s/!//g;
	    @matching_rules=grep { has_matching_precondition($_, $precond) } @matching_rules
	      or croak( "no production rule has a precondition matching '$precond'" );
	 }
	 \@matching_rules;
      } else {
	 $rules
      }
   } else {
      croak( "no production rules matching '", $self->header, "'" );
   }
}

sub has_matching_precondition {
   my ($rule, $wanted_precond)=@_;
   foreach my $precond (@{$rule->preconditions}) {
      my $header=$precond->header;
      $header=~s/\s//g;
      $header=~s/\(.*\)$//;
      return 1 if $header eq $wanted_precond;
   }
   0
}

sub describe_rule {
   my ($rule)=@_;
   if (defined $rule->code) {
      "'".$rule->header."' (defined at ".sub_file($rule->code).", line ".sub_firstline($rule->code).")";
   } else {
      "'".$rule->header."'"
   }
}

####################################################################################
package Polymake::Test::Object::WithRule;

use Polymake::Enum Verify => qw( OK missing_input missing_output unsatisfied_precond missing_permutation wrong_permutation wrong_result died );

use Polymake::Struct (
   [ '@ISA' => 'Case' ],
   [ new => '$$$%' ],
   [ '$name' => '#1->name' ],
   [ '$source_file' => '#%' ],
   [ '$source_line' => '#%' ],
   [ '$object' => '#1' ],
   [ '$rules' => '#2' ],
   [ '$tested_rules' => '#3' ],
   [ '$after_cleanup' => '#%' ],
   [ '$on' => '#%', default => 'undef' ],
   [ '$with_multi' => '#%', default => 'undef' ],
   [ '$expected_failure' => '#%', default => 'undef' ],
   [ '$permuted' => '#%', default => 'undef' ],
   '@perm_input',
);

sub execute {
   my ($self)=@_;
   my $success=0;

   if (defined $self->on) {
      $self->object= eval { $self->object->give($self->on) } ||
        do {
	   $self->fail_log="subobject selection failed: ".
			   ($@ || "property ".$self->on." does not exist\n");
	   return 0;
	};
   }

   local $Scope=new Scope() if defined $self->with_multi;
   if (defined $self->with_multi) {
      if (my $multi_instance=eval { $self->object->give(@{$self->with_multi}) }) {
         $multi_instance->set_as_default_now;
      } else {
         $self->fail_log="multiple subobject selection failed: ".
			 ($@ ? "syntax error in selector expression: $@\n"
                             : "no matching instance found\n");
	 return 0;
      }
   }

   my @perm_input_paths;
   if (defined $self->permuted) {
      ref($self->permuted) eq "ARRAY" && @{$self->permuted}>0
        or croak( "option `permuted' must provide a non-empty list of property names" );

      foreach (@{$self->permuted}) {
         my @path=$self->object->type->encode_descending_path($_);
         if (ref(my $pv=$self->object->lookup_descending_path(\@path))) {
            if (defined($pv->value)) {
               push @perm_input_paths, \@path;
               push @{$self->perm_input}, $_, $pv->value;
            } else {
               $self->fail_log="property `$_' has an undefined value\n";
               return 0;
            }
         } else {
            $self->fail_log="property `$_' is missing\n";
            return 0;
         }
      }
   }

   my $proto=$self->object->type;
   my %infeasible;
   foreach my $rule (@{$self->rules}) {
      if ($proto->isa($rule->defined_for) && $proto->rule_is_applicable($rule)) {
         if (@perm_input_paths) {
            if (defined($rule->with_permutation)) {
               if (grep { !defined($rule->with_permutation->perm_path->find_sensitive_sub_property(@$_)) } @perm_input_paths) {
                  $infeasible{$rule} = Verify::wrong_permutation;
                  next;
               }
            } else {
               $infeasible{$rule} = Verify::missing_permutation;
               next;
            }
         }

	 my ($status, @data) = verify_rule($self, $rule);
	 if ($status == Verify::OK) {
	    $self->tested_rules->{$rule}=1;
	    ++$success;
	 } elsif ($status == Verify::wrong_result) {
	    $self->tested_rules->{$rule}=1;
	    $self->fail_log.="testing rule ".Rule::describe_rule($rule)." on object ".$self->name." failed:\n".
	                     join("", Object::print_diff($self->object, @data));
	    return 0;
	 } elsif ($status == Verify::missing_output) {
	    $self->tested_rules->{$rule}=1;
	    $self->fail_log.="test object ".$self->name." can't be used for testing rule ".Rule::describe_rule($rule)."\n".join("", @data);
	    return 0;
	 } elsif ($status == Verify::died) {
	    $self->tested_rules->{$rule}=1;
	    $self->fail_log.=join("", @data);
	    return 0;
	 } else {
	    $infeasible{$rule}=$status;
	 }
      }
   }
   unless ($success) {
      $self->fail_log.="test object ".$self->name." can't be used for testing the rule(s):\n";
      foreach my $rule (@{$self->rules}) {
	 $self->fail_log.=" ".Rule::describe_rule($rule)." ";
	 if ($infeasible{$rule} == Verify::missing_input) {
	    $self->fail_log.="is infeasible: missing or undefined source properties\n";
	 } elsif ($infeasible{$rule} == Verify::unsatisfied_precond) {
	    $self->fail_log.="is infeasible: preconditions not satisfied\n";
	 } elsif ($infeasible{$rule} == Verify::missing_permutation) {
	    $self->fail_log.="is infeasible: does not incur any permutation while permuted target properties are specified\n";
	 } elsif ($infeasible{$rule} == Verify::wrong_permutation) {
	    $self->fail_log.="is infeasible: permutation incurred by the rule is not applicable to all specified target properties\n";
	 } elsif (!$proto->isa($rule->defined_for)) {
	    $self->fail_log.="is not applicable: defined for ".$rule->defined_for->full_name."\n";
	 } else {
	    $self->fail_log.="is not applicable: overridden for ".(grep { $proto->isa($_) } @{$rule->overridden_in})[0]->full_name."\n";
	 }
      }
      0
   }
}
####################################################################################
sub verify_rule {
   my ($self, $prod_rule)=@_;
   my ($obj, $t_obj, $index, $t_index, $pv, $prop);
   my $test_object=clone($self->object);

   foreach my $rule (@{$prod_rule->preconditions}, $prod_rule) {
      # link all input properties from the original object into the test object
      foreach my $input (@{$rule->input}) {
         ($pv, $prop)=();
         foreach my $input_path (@$input) {
            if (($obj, $t_obj, $prop)=descend_in_test_object($self->object, $test_object, $input_path)
                  and
                defined($index=$obj->dictionary->{$prop->key})
                  and
                defined($pv=$obj->contents->[$index])) {
               last;
            }
         }

         # try the shortcut rules in the original object as the last resort
         if ($pv //= $self->object->lookup_request($input)) {
            $prop //= $pv->property;
            $t_obj->dictionary->{$prop->key} //= do {
               push @{$t_obj->contents}, $pv;
               $#{$t_obj->contents}
            };
            unless (defined($pv->value) || $rule->flags & Core::Rule::Flags::is_definedness_check) {
               # implicit definedness precondition failed
               return Verify::missing_input;
            }
         } else {
            return Verify::missing_input;
         }
      }
   }

   # link all prerequisites for specially constructed properties
   foreach my $output (@{$prod_rule->output}) {
      if (defined (my $construct_list=$output->[-1]->construct)) {
       CONSTRUCT_ARG:
         foreach my $prop_path (@$construct_list) {
            my $full_path=$prop_path->prepend(@$output[0..$#$output-1]);
            if (!$full_path->up) {
               # don't link to the source object if the prerequisite property is created in the production rule under test as well
               foreach my $other_output (@{$prod_rule->output}) {
                  next CONSTRUCT_ARG if $other_output != $output && Core::Property::equal_path_prefixes($full_path->down, $other_output) == @{$full_path->down};
               }

               if (defined ($pv=$self->object->lookup_property_path($full_path))) {
                  ($obj, $t_obj, $prop)=descend_in_test_object($self->object, $test_object, $full_path->down);
                  $t_obj->dictionary->{$prop->key} //= do {
                     push @{$t_obj->contents}, $pv;
                     $#{$t_obj->contents};
                  };
               } else {
                  return (Verify::missing_output,
                          "missing property ", $full_path->toString,
                          " prerequisite for construction of target property ", Core::Property::print_path($output), "\n");
               }
            }
         }
      }
   }

   local *Polymake::Core::Object::lookup_descending_path=\&lookup_in_source;

   foreach my $rule (@{$prod_rule->preconditions}, $prod_rule) {
      my $rc=$rule->execute($test_object, 1);
      if ($rc == Core::Rule::Exec::OK) {
	 if ($rule == $prod_rule and
	     defined($self->expected_failure)) {
	    return (Verify::died, "rule ", Rule::describe_rule($prod_rule), " not failed as expected\n");
	 }
      } elsif ($rc == Core::Rule::Exec::infeasible) {
	 return Verify::missing_input;
      } elsif ($@) {
	 if ($rule == $prod_rule) {
	    if (defined($self->expected_failure)) {
	       # remove source code reference, the rules may freely wander through rulefiles
	       $@ =~ s{ at \Q${InstallTop}\E/\S+ line \d+\.?\n}{}o;
	       if ($@ eq $self->expected_failure) {
		  return Verify::OK;
	       } else {
		  return (Verify::died, "rule ", Rule::describe_rule($prod_rule), " failed with a different error message:\n",
			  "expected: ", $self->expected_failure, "\n",
			  "     got: $@\n");
	       }
	    } else {
	       return (Verify::died, "rule ", Rule::describe_rule($prod_rule), " failed: $@\n");
	    }
	 } else {
	    return (Verify::died, "prerequisite rule ", Rule::describe_rule($rule), " failed: $@\n");
	 }
      } elsif ($rule->flags & Core::Rule::Flags::is_precondition) {
	 return Verify::unsatisfied_precond;
      } else {
	 return (Verify::died, Rule::describe_rule($rule), " returned an unexpected code\n");
      }
   }

   $test_object->cleanup_now if $self->after_cleanup;

   if (defined $self->permuted) {
      $test_object=$test_object->copy_permuted(permutation => $prod_rule->with_permutation->perm_path, @{$self->perm_input});
   }

   my (@diff, @missing);
   foreach my $out (@{$prod_rule->output}) {
      if (($obj, $prop)=$self->object->descend($out)
	    and
	  defined ($index=$obj->dictionary->{$prop->property_key})) {
	 if (($t_obj, $prop)=$test_object->descend($out)
	       and
	     defined ($t_index=$t_obj->dictionary->{$prop->property_key})) {
	    push @diff, map { $_->[2] ||= $obj; $_ }
	                    $obj->diff_properties($obj->contents->[$index], $t_obj->contents->[$t_index]);
	 } else {
	    push @diff, [ $obj->contents->[$index], undef, $obj ];
	 }
      } else {
	 push @missing, $out;
      }
   }
   @missing ? (Verify::missing_output, map { ("missing target property ", Core::Property::print_path($_), "\n") } @missing) :
   @diff ? (Verify::wrong_result, @diff) :
   Verify::OK;
}
####################################################################################
sub clone {
   my ($object, $rec)=@_;
   my $test_object=Core::Object::new($object, $object->name);
   $test_object->attachments->{".source"}=$object;

   if (defined $object->parent) {
      $test_object->property=$object->property;
      unless ($rec) {
         weak($test_object->parent=$object->parent);
         # prevent transaction from being propagated upwards
         $test_object->is_temporary=1;
      }
   }
   $test_object;
}
####################################################################################
sub descend_in_test_object {
   my ($obj, $t_obj, $path)=@_;
   my $prop = local pop @$path;
   foreach my $sub_prop (@$path) {
      if (defined (my $content_index=$obj->dictionary->{$sub_prop->property_key})) {
	 $obj=$obj->contents->[$content_index]->value;
	 if (defined (my $t_ctx=$t_obj->dictionary->{$sub_prop->property_key})) {
	    $t_obj=$t_obj->contents->[$t_ctx]->value;
	 } else {
	    my $sub_t_obj=clone($obj, 1);
	    weak($sub_t_obj->parent=$t_obj);
	    push @{$t_obj->contents},
	         $sub_prop->flags & Core::Property::Flags::is_multiple ? new Core::PropertyValue::Multiple($obj->property, [ $sub_t_obj ]) : $sub_t_obj;
	    $t_obj->dictionary->{$sub_prop->property_key}=$#{$t_obj->contents};
	    $t_obj=$sub_t_obj;
	 }
      } else {
	 return;
      }
   }
   ($obj, $t_obj, $prop)
}
####################################################################################
# intercept lookup() requests coming from rule bodies
# and from the scheduler when it plans the permutation
my $lookup_descending_path=\&Polymake::Core::Object::lookup_descending_path;

sub lookup_in_source {
   my ($self, $path, $for_planning)=@_;
   if (my $source=$self->attachments->{".source"}) {
      &$lookup_descending_path || do {
         if (ref(my $pv=$lookup_descending_path->($source, $path, $for_planning))) {
            my ($obj, $t_obj, $prop)=descend_in_test_object($source, $self, $path);
            push @{$t_obj->contents}, $pv;
            $t_obj->dictionary->{$pv->property->key}=$#{$t_obj->contents};
            $pv
         }
      }
   } else {
      &$lookup_descending_path;
   }
}


1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
