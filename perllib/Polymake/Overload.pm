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

$Carp::Internal{'Polymake::Overload'} = 1;
$Carp::Internal{'Polymake::Overload::Node'} = 1;

package Polymake::Overload;
use Polymake::Ext;

use Polymake::Enum SignatureFlags => {
   has_trailing_list => 1<<30,
   has_keywords => 1<<29,
   has_repeated => 1<<28,
   num_args_mask => (1<<28)-1
};

use Polymake::Enum TypecheckFlags => {
   has_final_sub => 1,
   has_object => 2,
   has_sub => 4
};

package Polymake::Overload::Node;

use Polymake::Struct (
   [ new => '$$$' ],
   [ '$min_arg' => '#1' ],               # min possible number of arguments
   [ '$max_arg' => '#2' ],               # max possible number of arguments, optionally combined with SignatureFlags
   [ '$cur_arg' => 'undef' ],            # index of the argument this node has been chosen upon
   [ '$next_arg' => 'SignatureFlags::num_args_mask' ],  # index of the next argument to check;  num_args_mask => stop checking, return the code
   '$signature',                         # method name to look up in the package of the next argument
   [ '$backtrack' => '#3' ],             # another Node to try if no method is found
   [ '$typecheck' => 'undef' ],          # optional dynamic type checking subroutine/object and arguments to pass
   '@code',                              # \&sub or (label controlled) list, indexed by #args-min_arg
   [ '$ellipsis_code' => 'undef' ],      # the same, applicable by non-empty trailing argument list
   '@flags',                             # 0 or has_keywords; indexed like @code
);

sub new_root {
   new(@_, undef, undef, 0);
}
####################################################################################
sub clone_and_drop_code {
   my ($self)=@_;
   $self->backtrack = inherit_class([ @$self ], $self);
   $self->next_arg = SignatureFlags::num_args_mask;
   $self->code = [ ];
   $self->ellipsis_code = undef;
   $self->flags = [ ];
   $self
}
####################################################################################
sub add_fallback {
   my ($self, $code)=@_;
   while (is_object($self->backtrack)) { $self=$self->backtrack; }
   $self->backtrack=new($self, 0, SignatureFlags::has_trailing_list, 0);
   $self->backtrack->ellipsis_code=$code;
}
####################################################################################
sub split_node {
   my ($src, $min)=@_;
   my ($new_min, $split_code)= $min >= $src->min_arg ? ($min, $min-$src->min_arg) : ($src->min_arg, 0);
   my $ellipsis_code = $src->ellipsis_code
      and undef $src->ellipsis_code;
   inherit_class([ $new_min,
                   @$src[1..4],  # max_arg, cur_arg, next_arg, signature
                   0,            # backtrack
                   undef,        # typecheck
                   [ splice @{$src->code}, $split_code ],
                   $ellipsis_code,
                   [ splice @{$src->flags}, $split_code ],
                 ], $src);
}
####################################################################################
sub demangle {
   my ($self, $n_args)=@_;
   my $sig=$self->signature;
   $sig =~ s/\@/::/g;
   if (@_==1) {
      $sig =~ s/^\.([^,]+)(?:,(.*))?$/$1($2)/;
   } elsif ($n_args > $sig =~ tr/,/,/) {
      $sig =~ s/^\.([^,]+)(?:,(.*))?$/$1($2,...)/;
   } else {
      $sig =~ s/^\.([^,]+)(?:,((?:[^,]+.*?){$n_args}).*)?$/$1($2)/;
   }
   $sig;
}
####################################################################################
sub expand {
   my ($self, $new_min, $new_max, $new_code)=@_;

   if ($new_min < $self->min_arg) {
      if (@{$self->code}) {
         unshift @{$self->code},  (undef) x ($self->min_arg - $new_min);
         unshift @{$self->flags}, (undef) x ($self->min_arg - $new_min);
      }
      $self->min_arg=$new_min;
   }
   assign_max($self->max_arg, $new_max);

   my $limit=min($self->next_arg, $new_max & SignatureFlags::num_args_mask);
   my $insert_code_upto=$limit-$self->min_arg;
   my $set_ellipsis_code= $new_max == ($limit | SignatureFlags::has_trailing_list);

   if ($insert_code_upto > $#{$self->code}) {
      if (defined($self->ellipsis_code)) {
         $self->dup_ellipsis_code($insert_code_upto);
      }
   } elsif ($set_ellipsis_code) {
      $insert_code_upto=$#{$self->code};
   }
   for (my $i=$new_min-$self->min_arg; $i <= $insert_code_upto; ++$i) {
      $self->store_code($i, $new_code);
      $self->flags->[$i] |= $new_max & SignatureFlags::has_keywords;
   }
   if ($set_ellipsis_code && $self->next_arg >= SignatureFlags::num_args_mask) {
      $self->store_ellipsis_code($new_code);
   }
}
####################################################################################
sub store_code {
   my ($self, $i, $code)=@_;
   ( $self->code->[$i] &&= croak( "ambiguous overloading for ", demangle($self, $i+$self->min_arg) ))=$code;
}
####################################################################################
sub store_ellipsis_code {
   my ($self, $code)=@_;
   ( $self->ellipsis_code &&= croak( "ambiguous overloading for ", demangle($self) ))=$code;
}
####################################################################################
sub push_code {
   my ($self, $n, $code)=@_;
   push @{$self->code}, ($code) x $n;
}
####################################################################################
sub dup_ellipsis_code {
   croak( "ambiguous overloading for ", demangle($_[0]) );
}
####################################################################################
sub perform_typechecks {
   my ($self, $args, $backtrack, $bundled_repeated) = @_;
   my $tc = $self->typecheck;
   my $tc_flags = $tc->[0];
   my $tc_index = 1;
   if ($tc_flags & TypecheckFlags::has_final_sub) {
      $_[4] = $tc->[$tc_index++];
   }
   my $tc_proto;
   my $arg_index = $self->cur_arg;
   my $full_args=$args;
   if ($bundled_repeated) {
      $tc_flags & SignatureFlags::has_repeated or return 0;
      $args = $full_args->[$arg_index];
      $arg_index = 0;
   }
   if ($tc_flags & TypecheckFlags::has_object) {
      $tc_proto = $tc->[$tc_index++]->typecheck($full_args, $args, $arg_index, $backtrack)
        or return;
   }
   if ($tc_flags & SignatureFlags::has_repeated) {
      my $n_args=@$args;
      if ($tc_flags & TypecheckFlags::has_object) {
         while (++$arg_index < $n_args && $tc_proto->typecheck($full_args, $args, $arg_index, $backtrack)) { }
      } else {
         my $repeat_sig=$self->signature."+";
         while (++$arg_index < $n_args && can_signature($args->[$arg_index], $repeat_sig, 0)) { }
      }
      unless ($bundled_repeated) {
         bundle_repeated_args($args, $self->cur_arg, $arg_index);
         push @$backtrack, $self->cur_arg, \&unbundle_repeated_args;
      }
   }
   not($tc_flags & TypecheckFlags::has_sub)
   or $tc->[$tc_index]->($full_args, $self->cur_arg, $backtrack, @$tc[$tc_index+1 .. $#$tc])
}
####################################################################################
# Node, \(original @_) => application CODE | control list | undef
sub resolve {
   my ($node, $args)=@_;
   my (@backtrack, $bundled_repeated);

   for (;;) {
      if ($node->backtrack) {
         push @backtrack, $node;
      }
      my $final_typecheck;
      if (defined($node->typecheck)
          ? perform_typechecks($node, $args, \@backtrack, $bundled_repeated, $final_typecheck)
          : !$bundled_repeated) {
         my $n_args=@$args;
         if ($n_args >= $node->min_arg && $n_args <= $node->max_arg) {
            my ($kw_search_limit, $candidate);
            if ($n_args <= $node->next_arg) {
               # no more arguments to check
               if (!defined($final_typecheck) || $final_typecheck->($args, \@backtrack)) {
                  if (defined (my $flags=$node->flags->[$n_args - $node->min_arg])) {
                     $candidate=$node->code->[$n_args - $node->min_arg];
                     if ($flags == SignatureFlags::has_keywords) {
                        $kw_search_limit=$n_args;
                     } else {
                        return $candidate;
                     }
                  } elsif (defined ($node->ellipsis_code)) {
                     # list of trailing arbitrary arguments
                     return $node->ellipsis_code;
                  } elsif ($node->max_arg & SignatureFlags::has_keywords) {
                     # list of key-value pairs?
                     $kw_search_limit=$node->min_arg+@{$node->code};
                  }
                  undef $final_typecheck;
               }
            } elsif ($node->max_arg & SignatureFlags::has_keywords  and  $node->min_arg <= $node->next_arg) {
               $kw_search_limit=$node->next_arg+1;
            }
            if (defined $kw_search_limit) {
               for (my ($pos, $arg)=(0, $node->min_arg);
                    $arg < $kw_search_limit;
                    ++$pos, ++$arg) {
                  if ($node->flags->[$pos] & SignatureFlags::has_keywords  and  is_keyword_or_hash($args->[$arg])) {
                     if (!defined($final_typecheck) || $final_typecheck->($args, \@backtrack)) {
                        store_kw_args($args, $arg);
                        return $node->code->[$pos];
                     }
                     $n_args=-1;  # go straight to backtracking
                     last;
                  }
               }
               return $candidate if defined $candidate;
            }

            # got to check the next argument
            if ($n_args > $node->next_arg  and
                ($bundled_repeated, my $nodesub)=can_signature($args->[$node->next_arg], $node->signature, $node->max_arg & SignatureFlags::has_repeated)) {
               $node=&$nodesub;
               next;
            }
         }
      }

      for (;;) {
         defined (my $bt=pop @backtrack) or return;  # failed to resolve, no fallback registered
         if (is_object($bt)) {
            if (is_code($bt->backtrack)) {
               if (my $other_node_sub=can_next($args->[$bt->cur_arg], $bt->backtrack)) {
                  $node=&$other_node_sub;
                  last;
               }
            } else {
               $node=$bt->backtrack;
               last;
            }
         } else {
            $bt->($args, \@backtrack);
         }
      }
      $bundled_repeated=0;
   }
}

####################################################################################
package Polymake::Overload::LabeledNode;

use Polymake::Struct (
   '@control_list',
);

sub add {
   my ($self, $label, $node)=@_;
   $label->add_control($self->control_list, $node);
}

sub resolve {
   my ($self, $args)=@_;
   my $code;
   for my $node (@{$self->control_list->items}) {
      last if defined($code=$node->resolve($args));
   }
   $code
}

sub invalidate_on_change {
   my ($self, $ptr)=@_;
   push @{$self->control_list->destroy_on_change //= [ ]}, $ptr;
   $ptr
}

####################################################################################
package Polymake::Overload::GlobalLabeledNode;

use Polymake::Struct (
   [ '@ISA' => 'Node' ],
);

sub store_code {
   my ($self, $i, $method_with_label)=@_;
   my ($method, $label)=@$method_with_label;
   $label->add_control($self->code->[$i] //= [ ], $method);
}

sub store_ellipsis_code {
   my ($self, $method_with_label)=@_;
   my ($method, $label)=@$method_with_label;
   $label->add_control($self->ellipsis_code //= [ ], $method);
}

sub dup_ellipsis_code {
   my ($self, $upto)=@_;
   push @{$self->code}, map { $self->ellipsis_code->dup } @{$self->code}..$upto;
}

sub push_code {
   my ($self, $n, $method_with_label)=@_;
   my ($method, $label)=@$method_with_label;
   push @{$self->code}, map { $label->add_control(my $cl=[ ], $method); $cl } 1..$n;
}

sub resolve {
   if (my $cl=&Node::resolve) {
      $cl->items->[0]
   } else {
      undef
   }
}

####################################################################################
package Polymake::Overload;

my $string_pkg="Polymake::Overload::string";
my $integer_pkg="Polymake::Overload::integer";
my $float_pkg="Polymake::Overload::float";

my ($string_proto, $integer_proto, $float_proto);

sub set_string_type {
   $string_proto=$_[0];
   $string_pkg=$_[0]->pkg;
   store_string_package_stash(get_symtab($_[0]->pkg));
}

sub set_integer_type {
   $integer_proto=$_[0];
   $integer_pkg=$_[0]->pkg;
   store_integer_package_stash(get_symtab($_[0]->pkg));
}

sub set_float_type {
   $float_proto=$_[0];
   $float_pkg=$_[0]->pkg;
   store_float_package_stash(get_symtab($_[0]->pkg));
}

sub string_package { $string_pkg }
sub integer_package { $integer_pkg }
sub float_pkg { $float_pkg }
####################################################################################
# TODO: unify with CPlusPlus::classify_scalar
sub fetch_type {
   if (defined($_[0])) {
      if (is_object($_[0])) {
         if (defined (my $typesub=UNIVERSAL::can($_[0], ".type"))) {
            return &$typesub;
         }
      } elsif (is_integer($_[0])) {
         return $integer_proto
      } elsif (is_float($_[0])) {
         return $float_proto
      } elsif (is_string($_[0])) {
         return $string_proto
      }
      # TODO: anon arrays and hashes
   }
   undef
}
####################################################################################
# Used as a typecheck routine for functions with explicit type parameters:
# Checks that the number of explicitly specified types fits into the expected bounds.
sub check_explicit_typelist {
   my ($args, undef, undef, $min, $max)=@_;
   my $explicit_size=namespaces::store_explicit_typelist($args);
   $explicit_size >= $min && $explicit_size <= $max
}

sub restore_type_param {
   my ($args, $backtrack)=@_;
   my $restore=pop @$backtrack;
   namespaces::fetch_explicit_typelist($args)->[pop @$backtrack]=$restore;
}

sub restore_type_param_list {
   my ($args, $backtrack)=@_;
   my ($typelist, $explicit_size)=namespaces::fetch_explicit_typelist($args);
   my $lasttype=pop @$backtrack;
   if ($lasttype >= $explicit_size) {
      splice @$typelist, $explicit_size, @$typelist-$explicit_size, splice @$backtrack, $explicit_size-$lasttype-1;
   } else {
      $#$typelist=$lasttype;
   }
}
####################################################################################
my %dictionary;

sub dict_node {
   my ($pkg, $name, $is_method, $label)=@_;
   my $node=$dictionary{$pkg}->{$name};
   if (!defined($node)) {
      if ($is_method) {
         no strict 'refs';
         if (exists &{"$pkg\::$name"}) {
            croak( "non-overloaded method $pkg\::$name already defined" );
         }
      } elsif (defined (my $sub=namespaces::lookup_sub($pkg, $name))) {
         my $owner_pkg=method_owner($sub);
         if (defined (($node=$dictionary{$owner_pkg}) &&= $node->{$name})) {
            # expanding the imported overloaded function
            $dictionary{$pkg}->{$name}=$node;
         } else {
            croak( "non-overloaded function $pkg\::$name already ",
                   $owner_pkg ne $pkg ? "imported from package $owner_pkg" : "defined" );
         }
      }
   }
   if (defined($node) && instanceof Node($node) == defined($label)) {
      croak( $is_method ? "method" : "function", " $pkg\::$name already defined ",
             defined($label) ? "without labels" : "with labels" );
   }
   $node;
}
####################################################################################
sub add_fallback {
   my ($pkg, $name, $code)=@_;
   (($dictionary{$pkg} && $dictionary{$pkg}->{$name}) // croak( "unknown overloaded function $pkg\::$name" ))->add_fallback($code);
}
####################################################################################
# private:
sub analyze_typecheck {
   my ($arg_list, $arg, $min, $tparams, $final_typecheck) = @_;
   my @typecheck;

   if (defined($tparams)) {
      push @typecheck, TypecheckFlags::has_sub, \&check_explicit_typelist, @$tparams;
   }

   add_final_typecheck($arg_list, $arg, $min, \@typecheck, $final_typecheck)
      if defined($final_typecheck);

   @typecheck ? \@typecheck : undef
}
####################################################################################
# private:
sub add_final_typecheck {
   my ($arg_list, $arg, $min, $typecheck, $final_typecheck) = @_;

   # final typecheck applies if the resolving procedure can stop after this argument
   my $elem;
   do { ++$arg } while ($arg < $min && !ref($elem = $arg_list->[$arg]) && $elem eq '$');
   if ($arg >= $min) {
      $typecheck->[0] |= TypecheckFlags::has_final_sub;
      splice @$typecheck, 1, 0, $final_typecheck;
   }
}
####################################################################################
# private:
sub analyze_signature_element {
   my ($arg_list, $arg, $min, $method_context_pkg, $final_typecheck)=@_;
   my ($arg_type, $pkg, @typecheck);
   my $elem=$arg_list->[$arg];
   if (ref($elem) eq "ARRAY") {
      ($arg_type, @typecheck)=@$elem;
   } else {
      $arg_type=$elem;
   }
   if (@typecheck) {
      if (@typecheck==1 && $typecheck[0] eq "+") {
         @typecheck = (SignatureFlags::has_repeated);
      } elsif (ref($typecheck[0] eq "CODE")) {
         unshift @typecheck, TypecheckFlags::has_sub;
      } else {
         croak( "invalid argument amendment ", ref($typecheck[0]) || "'$typecheck[0]'" );
      }
   }
   if (is_object($arg_type)) {
      if ($arg_type->abstract) {
         if (defined($arg_type->context_pkg)) {
            defined($method_context_pkg) && UNIVERSAL::isa($method_context_pkg, $arg_type->context_pkg)
              or croak( "context-dependent type parameter ", $arg_type->full_name, " can only be referred to in methods of ", $arg_type->context_pkg );
         }
         if (defined($arg_type->perform_typecheck)) {
            $typecheck[0] |= TypecheckFlags::has_object;
            splice @typecheck, 1, 0, $arg_type;
         }
      }
      $pkg=$arg_type->pkg;
   } else {
      defined($pkg=$arg_type) or croak( "unknown type in signature" );
   }

   add_final_typecheck($arg_list, $arg, $min, \@typecheck, $final_typecheck)
      if defined($final_typecheck);

   ($pkg, @typecheck ? \@typecheck : undef);
}
####################################################################################
# private:
sub compare_typechecks {
   my ($node, $arg, $min, $max, $typecheck, $signature)=@_;
   if (defined $typecheck) {
      for (my $similar_node=$node; ;) {
         if (defined($similar_node->typecheck)) {
            if (equal_lists($typecheck, $similar_node->typecheck)) {
               $_[0]=$similar_node;
               return;   # continue descending
            }
         } else {
            # previous instance without typecheck
            last;
         }
         $similar_node=$similar_node->backtrack;
         is_object($similar_node) && index($similar_node->signature, $signature)==0 or last;
      }
      # Assume the new instance being more specific than all ones defined prior to it: insert at the beginning of the candidate list
      my $sibling_node=$node->clone_and_drop_code;
      $sibling_node->min_arg = $min;
      $sibling_node->max_arg = $max - ($typecheck->[0] & SignatureFlags::has_repeated);
      $sibling_node->cur_arg = $arg;
      $sibling_node->typecheck=$typecheck;
      if (!defined($arg)) {
         # If the old root node describes functions without type parameters, they must be protected against parameterized cousins
         $node->backtrack->typecheck //= [ TypecheckFlags::has_sub, \&check_explicit_typelist, 0, 0 ];
      }
      $sibling_node
   } elsif (defined $node->typecheck) {
      # A generic signature introduced after a specific one (or an instance without repeats after an instance with repeats):
      # append to the end of the candidate list
      while (is_object($node->backtrack) && index($node->backtrack->signature, $signature)==0) {
         $node=$node->backtrack;
      }
      if (!defined($node->typecheck) or !defined($arg) && $node->typecheck->[3]==0) {
         # already have a node without typechecks or with a typecheck protecting against unsolicited type parameters: continue descending
         $_[0]=$node;
         return;
      }
      my $sibling_node=$node->new($min, $max, $node->backtrack);
      if (!defined($arg)) {
         # The new root node describes functions without type parameters: must protect them against parameterized cousins
         $sibling_node->typecheck=[ TypecheckFlags::has_sub, \&check_explicit_typelist, 0, 0 ];
      }
      $node->backtrack=$sibling_node;
   } else {
      undef
   }
}
####################################################################################
sub add_instance {
   my ($caller, $name, $label, $code, $arg_types, $tparams, $root_node, $node_type, $invalidate_on_change)=@_;
   my ($min, $max, @arg_list)=@$arg_types;
   my $pkg=$name =~ s/^(.*)::([^:]+)$/$2/ ? $1 : $caller;
   my ($is_method, $is_lvalue)= is_object($code) ? (1, 0) : $pkg eq "Polymake::Overload::Global" ? (0, 0) : (is_method($code), is_lvalue($code));
   my $method_context_pkg;
   if ($is_method) {
      ++$min;
      ++$max;
      unshift @arg_list, $pkg;
      $method_context_pkg=$pkg;
   }
   my ($signature, @last_glob, $sibling_node, $backtrack_node, $arg_pkg, $final_typecheck, $node);
   $final_typecheck = pop(@arg_list) if @arg_list && is_code($arg_list[-1]);
   my $arg = $is_method && !defined($root_node) ? 0 : -1;
   my $typecheck = analyze_typecheck(\@arg_list, $arg, $min, $tparams, $final_typecheck);
   $node_type //= "Polymake::Overload::Node";

   if (defined($root_node)) {
      if (defined($root_node->min_arg)) {
         # revisiting a root node created outside
         $node=$root_node;
      } else {
         # first use of a root node created outside
         $root_node->min_arg=$min;
         $root_node->max_arg=$max;
         $root_node->typecheck=$typecheck;
         $signature=$name;
         $sibling_node=$root_node;
      }
   } elsif (defined($root_node=dict_node($pkg, $name, $is_method, $label))) {
      $node= defined($label) ? $root_node->control_list->find_item_of_label($label) : $root_node;
   }

   if (defined($node)) {
      ($signature)=$node->signature =~ /^([^,\[]+)/;

      if (!defined( $sibling_node=compare_typechecks($node, undef, $min, $max, $typecheck, $signature))) {

         if ($typecheck) {
            $signature.=sprintf("[%x]", $node->typecheck);
         }
       DESCEND: {
            # $node => existing Node with arguments 0..$arg identical to those in the new signature
            while (++$arg <= $#arg_list && !ref($arg_list[$arg]) && $arg_list[$arg] eq '$') { $signature.=',$' }

            if ($typecheck and $typecheck->[0] & SignatureFlags::has_repeated) {
               $max -= SignatureFlags::has_repeated;
            }

            if ($arg <= $#arg_list) {
               # $arg = index of the next non-trivial argument in the new signature
               ($arg_pkg, $typecheck)=analyze_signature_element(\@arg_list, $arg, $min, $method_context_pkg, $final_typecheck);
             EXISTING: {
                  my $next_arg=$node->next_arg;
                  if ($next_arg == $arg) {
                     # $node and the new signature have the next non-trivial arguments at the same position
                     $node->expand($min, $max, $code);
                     $min=$arg+1 if $min<=$arg;
                     my $next_node_sub=UNIVERSAL::can($arg_pkg, $signature);
                     if (defined($next_node_sub)  &&  $arg_pkg eq method_owner($next_node_sub)) {
                        $node=&$next_node_sub;
                        if (!defined( $sibling_node=compare_typechecks($node, $arg, $min, $max, $typecheck, $signature))) {

                           # the argument types themselves coincide too: the distinction will be based on a following argument
                           $signature .= ",$arg_pkg";
                           $signature =~ s/::/\@/g;
                           if ($typecheck) {
                              $signature.=sprintf("[%x]", $node->typecheck);
                           }
                           redo DESCEND;
                        }
                     }

                  } elsif ($next_arg < $arg) {
                     # $node has a non-trivial argument at a position where the new signature has a wildcard '$':
                     # create a node for UNIVERSAL package or follow it if it already exists
                     $node->expand($min, $max, $code);
                     $min=$next_arg+1 if $min<=$next_arg;
                     if (defined (my $next_node_sub=UNIVERSAL->can($node->signature))) {
                        $node=&$next_node_sub;
                        redo EXISTING;
                     }

                     my $uni_node=$node_type->new($min, $max, 0);
                     if ($min <= $arg) {
                        $uni_node->push_code($arg-$min+1, $code);
                        push @{$uni_node->flags}, ($max & SignatureFlags::has_keywords) x ($arg-$min+1);
                        $min=$arg+1;
                     }
                     $uni_node->signature=$signature;
                     $uni_node->next_arg=$arg;
                     define_function("UNIVERSAL", $node->signature, sub : method { $uni_node });

                  } elsif ($next_arg < SignatureFlags::num_args_mask) {
                     # $node has a wildcard '$' at a position where the new signature has a non-trivial argument:
                     # create a new intermediate node for UNIVERSAL package
                     my $uni_node=$node->split_node($arg+1);
                     if ($arg_pkg eq "UNIVERSAL") {
                        # The new node subroutine will be created at the same place.
                        # Presumably the new node has some typechecks, while uni_node does not.
                        $backtrack_node=$uni_node;
                     } else {
                        define_function("UNIVERSAL", $signature, sub : method { $uni_node });
                     }
                     $node->expand($min, $max, $code);
                     $min=$arg+1 if $min<=$arg;
                     $node->signature=$signature;
                     $node->next_arg=$arg;

                  } else {
                     # $node does not have any non-trivial arguments more
                     if (($node->max_arg & (SignatureFlags::num_args_mask | SignatureFlags::has_trailing_list)) > $arg) {
                        # $node has a wildcard at the position $arg or covers it with the trailing list:
                        # create an intermediate node for UNIVERSAL package
                        my $uni_node=$node->split_node($arg+1);
                        if ($arg_pkg eq "UNIVERSAL") {
                           # The new node subroutine will be created at the same place.
                           # Presumably the new node has some typechecks, while uni_node does not.
                           $backtrack_node=$uni_node;
                        } else {
                           define_function("UNIVERSAL", $signature, sub : method { $uni_node });
                        }
                     }
                     $node->signature=$signature;
                     $node->next_arg=$arg;
                     $node->expand($min, $max, $code);
                     $min=$arg+1 if $min<=$arg;
                  }

                  @last_glob=($arg_pkg, $signature);
                  $signature .= ",$arg_pkg";
                  $signature =~ s/::/\@/g;
               }
            } else {
               # the new signature does not have any non-trivial arguments more
               for (;;) {
                  $node->expand($min, $max, $code);
                  my $next_arg=$node->next_arg;
                  if ($next_arg >= SignatureFlags::num_args_mask || $next_arg >= ($max & (SignatureFlags::num_args_mask | SignatureFlags::has_trailing_list))) {
                     # nothing to do more if:
                     # - the signature of $node ends as well here
                     # - new signature does not have a trailing list and ends before the next non-trivial argument of $node
                     return;
                  }
                  # create a new intermediate node for UNIVERSAL package or follow the existing one
                  if (defined (my $next_node_sub=UNIVERSAL->can($node->signature))) {
                     $node=&$next_node_sub;
                     last if defined( $sibling_node=compare_typechecks($node, $arg, $min, $max, $typecheck, $signature));
                     $min=$next_arg+1 if $min<=$next_arg;
                  } else {
                     @last_glob=("UNIVERSAL", $node->signature);
                     last;
                  }
               }
               --$arg;
            }
         }
      }
   } else {
      # first instance of this function
      $signature=".$pkg\@$name";
      $signature =~ s/::/\@/g;
      if (defined($label)) {
         $signature .= "#" . $label->full_name;
      }
   }

   if (defined $typecheck) {
      $signature.=sprintf("[%x]", $typecheck);
   }

   for (;;) {
      my $last_arg=$arg;
      while (++$arg <= $#arg_list && !ref($arg_list[$arg]) && $arg_list[$arg] eq '$') { $signature.=',$' }

      if ($typecheck and $typecheck->[0] & SignatureFlags::has_repeated) {
         $max -= SignatureFlags::has_repeated;
      }
      my $node=$sibling_node // $node_type->new($min, $max, 0);
      if ($min<=$arg) {
         $node->push_code($arg-$min+1, $code);
         push @{$node->flags}, ($max & SignatureFlags::has_keywords) x ($arg-$min+1);
         $min=$arg+1;
      }
      $node->signature=$signature;

      if (defined $sibling_node) {
         undef $sibling_node;
      } else {
         if (@last_glob) {
            my $nodesub=define_function(@last_glob, sub : method { state %next_method_cache; $node });
            $node->backtrack= $backtrack_node // ($last_glob[0] !~ /^UNIVERSAL\b/ && $nodesub);
            $node->cur_arg=$last_arg;
            if (defined($typecheck) and @$typecheck==1) {
               # simple repeated argument
               define_function($last_glob[0], "$signature+", sub : method { 1 });
            }
            undef $backtrack_node;
         } elsif (defined($root_node)) {
            $root_node->add($label, $node);
         } else {
            if (defined($label)) {
               $root_node=new LabeledNode();
               $root_node->add($label, $node);
            } else {
               $root_node=$node;
            }
            $dictionary{$pkg}->{$name}=$root_node;
            if ($pkg ne "Polymake::Overload::Global") {
               my $head_code;
               if ($invalidate_on_change) {
                  $head_code=define_function($pkg, $name,
                                             sub { $root_node->invalidate_on_change(&{ $root_node->resolve(\@_) // complain($pkg, $name, $is_method, \@_) }) });
               } else {
                  no warnings 'void';
                  use namespaces::AnonLvalue '$is_lvalue';
                  $head_code=define_function($pkg, $name,
                                             sub { $is_lvalue; &{ $root_node->resolve(\@_) // complain($pkg, $name, $is_method, \@_) } });
               }
               set_method($head_code) if $is_method;
            } else {
               define_function($pkg, $name, sub { $root_node->resolve(\@_) // complain($pkg, $name, $is_method, \@_) });
            }
         }
         $node->typecheck=$typecheck;
      }

      if ($arg <= $#arg_list) {
         ($arg_pkg, $typecheck)=analyze_signature_element(\@arg_list, $arg, $min, $method_context_pkg, $final_typecheck);
         $node->next_arg=$arg;
         @last_glob=($arg_pkg ne '$' ? $arg_pkg : "UNIVERSAL", $signature);
         $signature .= ",$arg_pkg";
         $signature =~ s/::/\@/g;
         if (defined $typecheck) {
            $signature.=sprintf("[%x]", $typecheck);
         }
      } else {
         if ($max >= SignatureFlags::has_trailing_list) {
            $node->store_ellipsis_code($code, $label);
         }
         last;
      }
   }
}
####################################################################################
sub complain {
   my ($pkg, $name, $is_method, $args)=@_;
   my $type;
   if (my ($typelist, $explicit_size)=namespaces::fetch_explicit_typelist($args)) {
      if ($explicit_size) {
         $name .= "<" . join(", ", map { $_->full_name } @$typelist[0..$explicit_size-1]) . ">";
      }
   }
   my @args=map { defined($type=fetch_type($_)) ? $type->full_name : ref($_) || "\$" } @$args;
   if (my $leading_object=$is_method && shift @args) {
      croak( "no matching overloaded instance of $leading_object\->$name(" . join(", ", @args) . ")" );
   } elsif ($pkg eq "__OPERATOR__") {
      if (@args==1) {
         croak( "no matching overloaded instance of operator $name $args[0]" );
      } else {
         croak( "no matching overloaded instance of operator $args[0] $name $args[1]" );
      }
   } else {
      croak( "no matching overloaded instance of $pkg\::$name(" . join(", ", @args) . ")" );
   }
}
####################################################################################
# 'name', Label, \&code, [ min_arg, max_arg, @arg_list ], options  =>
sub add {
   ((undef, my ($name, $label, $code, $arg_types, %opts))=@_) >= 5 or return;
   my $caller=caller;
   add_instance($caller, $name, $label, $code, $arg_types, @opts{qw( tparams root_node node_type invalidate_on_change )});
}
####################################################################################
sub add_global {
   (undef, my ($name, $label, $code, $arg_types, %opts))=@_;
   my $caller=caller;
   croak( "cannot declare a non-method '$name' as global" ) unless is_method($code);
   croak( "package $caller tries to declare method '$name' as global although it comes from different package" )
      unless method_owner($code) eq $caller;
   if (defined $label) {
      add_instance("Polymake::Overload::Global", $name, undef, [$code, $label], $arg_types, undef, undef, "Polymake::Overload::GlobalLabeledNode");
   } else {
      add_instance("Polymake::Overload::Global", $name, undef, $code, $arg_types);
   }
   add_instance($caller, $name, undef, $code, $arg_types, $opts{tparams});
}
####################################################################################
sub find_root_node {
   my ($pkg, $function)=@_;
   my $node=$dictionary{$pkg};
   my $sub;
   $node &&= $node->{$function}  or
   ($sub=namespaces::lookup_sub($pkg, $function) and
    $node=$dictionary{method_owner($sub)} and
    $function =~ /::($id_re)$/o, $node=$node->{$1 // $function})
}

sub resolve {
   my ($pkg, $function, $args)=@_;
   if (my $node=&find_root_node) {
      $node->resolve($args)
   }
}

sub resolve_global {
   my ($function, $args)=@_;
   my $node=$dictionary{"Polymake::Overload::Global"};
   $node &&= $node->{$function}  and  Node::resolve($node, $args);
}

sub resolve_method {
   my $head=&UNIVERSAL::can or return undef;
   my ($method)=splice @_, 1, 1;
   my $node=$dictionary{method_owner($head)};
   $node &&= $node->{$method}  and  $node->resolve(\@_);
}

####################################################################################
sub process_kw_args {
   # for performance reasons we duplicate some code instead of branching in the loop
   my $args=shift;
   my $stored_kw_args=fetch_stored_kw_args($args);

   my (@unknown, @processed_args, $table, $t);
   if (@_ > 1) {
      my $direct_table=0;
      push @processed_args, {} for 0..$#_;
      for (my ($i, $last)=(0, $#$stored_kw_args);  $i<=$last;  ++$i) {
         my $key=$stored_kw_args->[$i];
         if (is_keyword($stored_kw_args->[$i]) && ++$i <= $last) {
            my $known;
            $t=0;
            foreach $table (@_) {
               if (exists $table->{$key}) {
                  $processed_args[$t]->{$key} = $stored_kw_args->[$i];
                  $known=1;
               }
               ++$t;
            }
            push @unknown, $key unless $known;

         } elsif (ref($key) eq "HASH") {
            if ($direct_table > $#_) {
               croak( "too many hash arguments" );
            }
            push %{$processed_args[$direct_table++]}, %$key;

         } else {
            croak( "KEYWORD => value pairs expected, got ", ref($key) || "'$key'" );
         }
      }

   } else {
      my %processed_args;
      push @processed_args, \%processed_args;
      $table=$_[0];
      for (my ($i, $last)=(0, $#$stored_kw_args);  $i<=$last;  ++$i) {
         my $key=$stored_kw_args->[$i];
         if (is_keyword($stored_kw_args->[$i]) && ++$i <= $last) {
            if (!defined($table) || exists $table->{$key}) {
               $processed_args{$key} = $stored_kw_args->[$i];
            } else {
               push @unknown, $key;
            }
         } elsif (ref($key) eq "HASH") {
            @processed_args{ keys %$key }=values %$key;
         } else {
            croak( "KEYWORD => value pairs expected, got ", ref($key) || "'$key'" );
         }
      }
   }

   if (@unknown) {
      my %known;
      push %known, %$_ for @_;
      delete @known{ keys %$_ } for @processed_args;
      croak( "unknown keyword argument", (@unknown>1 && "s"), ": ", join(", ", @unknown),
             "\nallowed keywords are: ", join(", ", sort keys %known) );
   }

   # filter the values and fill in the defaults
   $t=0;
   foreach $table (@_) {
      my $processed_args = $processed_args[$t++];
      while (my ($key, $descr) = each %$table) {
         if (exists $processed_args->{$key}) {
            $descr->($processed_args, $key, $processed_args->{$key}) if is_code($descr);
         } elsif (defined($descr)) {
            if (is_code($descr)) {
               $descr->($processed_args, $key);
            } else {
               $processed_args->{$key} = $descr;
            }
         }
      }
   }

   push @$args, @processed_args;
}

sub Polymake::enum {
   my ($default, %enum);
   foreach my $name (@_) {
      if ($name =~ /=default$/) {
         $default = $`;
         $enum{$`} = 1;
      } else {
         $enum{$name} = 1;
      }
   }
   sub {
      if ((my ($args, $key, $value) = @_) == 3) {
         $enum{$value}
           or croak( "unknown option value: $key => $value" );
      } elsif (defined($default)) {
         $args->{$key} = $default;
      }
   }
}

####################################################################################

package Polymake::Overload::Global;
# just to pull it into being

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
