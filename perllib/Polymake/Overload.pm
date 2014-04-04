#  Copyright (c) 1997-2014
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

require Symbol;

$Carp::Internal{'Polymake::Overload'}=1;
$Carp::Internal{'Polymake::Overload::Node'}=1;

package Polymake::Overload::Node;

use Polymake::Struct (
   [ new => '$$$' ],
   [ '$min_arg' => '#1' ],	# min and max possible number of arguments.
   [ '$max_arg' => '#2' ],	# undef => unlimited ( signature with trailing @ or keywords)
   [ '$backtrack' => '#3' ],	# another Node
   [ '$next_arg' => 'undef' ],	# skip untyped scalars which don't need checking;  undef => stop checking
   '$signature',
   '@code',			# \&sub or (label controlled) list
   '$ellipsis_code',		# \&sub with unlimited arglist
   [ '$default_args' => 'undef' ],	# to push into @_
   [ '$keywords' => 'undef' ],		# [ \%keyword_table, ... ] when signature with keywords
);

####################################################################################
sub clone {
   my ($src)=@_;
   bless [ $src->min_arg, $src->max_arg, 0, $src->next_arg, $src->signature,
	   [ @{$src->code} ],
	   $src->ellipsis_code,
	   $src->default_args,
	   $src->keywords,
	 ];
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
sub _expand {
   my ($self, $new_min, $new_max, $new_proto_length)=@_;
   if ($new_min < $self->min_arg) {
      if (@{$self->code}) {
	 unshift @{$self->code}, (undef) x ($self->min_arg - $new_min);
      }
      $self->min_arg=$new_min;
   }

   my $limit=$new_proto_length // $self->next_arg;

   if (defined $self->max_arg) {
      if (defined $new_max) {
	 assign_min($limit, $new_max);
	 assign_max($self->max_arg, $new_max);
      } else {
	 undef $self->max_arg;
      }
   }
   return $limit;
}
####################################################################################
sub expand {
   my ($self, $new_min, $new_max, $new_proto_length, $new_code)=@_;
   my $limit=&_expand;

   if ($self->ellipsis_code  &&  $#{$self->code} < $limit-$self->min_arg  &&  !$self->keywords) {
      croak( "ambiguous overloading for ", demangle($self) );
   }

   for (my $i=$new_min; $i<=$limit; ++$i) {
      ( $self->code->[$i-$self->min_arg] &&= croak( "ambiguous overloading for ", demangle($self, $i) )
      ) ||= $new_code;
   }

   if (defined($new_proto_length) && !defined($new_max)) {
      if ($self->ellipsis_code  ||  $#{$self->code} > $new_proto_length-$self->min_arg) {
	 croak( "ambiguous overloading for ", demangle($self) );
      }
      $self->ellipsis_code=$new_code;
   }
}
####################################################################################
sub push_code {
   my ($self, $new_code, $n)=@_;
   push @{$self->code}, ($new_code) x $n;
}
####################################################################################
sub push_ellipsis_code {
   my ($self, $new_code)=@_;
   $self->ellipsis_code=$new_code;
}
####################################################################################
sub find_backtrack {
   my ($nodesub)=@_;
   my $node_of_interest=&$nodesub;

   my $node=$node_of_interest;
   for (;;) {
      my $obj=method_owner($nodesub);
      if (my $nextnodesub=super_can($obj, method_name($nodesub))) {
	 my $nextnode=&$nextnodesub;
	 if ($nextnode!=$node) {
	    $node->backtrack=$nextnode;
	    last if ref($nextnode->backtrack) or !$nextnode->backtrack;
	    $node=$nextnode;
	    $nodesub=$nextnodesub;
	    next;
	 }
      }
      $node->backtrack=0;
      last;
   }

   $node_of_interest->backtrack;
}
####################################################################################
# private:
sub process_defaults {
   my ($list, $args, $from)=@_;
   if (is_object($list)) {
      # there are dynamic default values
      while ($from <= $#$list) {
	 push @$args, is_dynamic_default($list->[$from]) ? $list->[$from]->($args) : $list->[$from];
	 ++$from;
      }
   } else {
      # all default values are constants
      push @$args, @$list[ $from .. $#$list ];
   }
}

# Node, \(original @_) => \&target_sub | control list | undef
sub resolve {
   my ($node, $args)=@_;
   my @backtrack;
   my $fallback=$node->backtrack || undef;
   my $n_repeated=0;
   my $n_args=@$args;

   for (;;) {
      if ($n_args-$n_repeated >= $node->min_arg) {
	 if (!defined($node->max_arg) || $n_args <= $node->max_arg) {
	    if (!defined($node->next_arg) || $node->next_arg+$n_repeated >= $n_args) {
	       if ($node->keywords) {
		  my $first_kw=$node->min_arg+$n_repeated;
		  if ($node->default_args) {
		     while ($first_kw <= $#{$node->default_args}) {
			if ($first_kw >= $n_args || is_keyword($args->[$first_kw]) || ref($args->[$first_kw]) eq "HASH") {
			   my @keyed=process_keywords($node->keywords, splice @$args, $first_kw);
			   process_defaults($node->default_args, $args, $first_kw);
			   push @$args, @keyed;
			   return $node->ellipsis_code;
			}
			++$first_kw;
		     }
		  }
		  if ($first_kw >= $n_args || is_keyword($args->[$first_kw]) || ref($args->[$first_kw]) eq "HASH") {
		     push @$args, process_keywords($node->keywords, splice @$args, $first_kw);
		     return $node->ellipsis_code;
		  }
		  goto BACKTRACK;
	       } elsif ($node->default_args) {
		  process_defaults($node->default_args, $args, $n_args-$n_repeated);
	       }
	       return $node->code->[ $n_args-$n_repeated-$node->min_arg ] || $node->ellipsis_code;
	    }

	    if (defined (my $nodesub=can_signature($args->[$node->next_arg+$n_repeated], $node->signature, defined($node->keywords)))) {
	       $node=$nodesub->($n_repeated);
	       if (my $bt_node=$node->backtrack) {
		  push @backtrack, ref($bt_node) ? $bt_node : $nodesub;
	       }
	       next;
	    }
	 }
      }

      BACKTRACK: {
	 $node=pop @backtrack or return $fallback;
	 if (is_code($node)) {
	    $node=find_backtrack($node) or redo BACKTRACK;
	 }
	 if (my $bt_node=$node->backtrack) {
	    push @backtrack, $bt_node;
	 }
      }
   }
}
####################################################################################
sub complain_mismatch {
   "already defined without labels"
}
####################################################################################

package Polymake::Overload::LabeledNode;

use Polymake::Struct (
   [ '@ISA' => 'Node' ],
);

####################################################################################
sub dup_or_new_code {
   $_[0] ? $_[0]->dup : [ ];
}
####################################################################################
sub dup_code {
   $_[0] && $_[0]->dup;
}
####################################################################################
sub clone {
   my ($src)=@_;
   bless [ $src->min_arg, $src->max_arg, 0, $src->next_arg, $src->signature,
	   [ map { dup_code($_) } @{$src->code} ],
	   dup_code($src->ellipsis_code),
	   $src->default_args,
	   $src->keywords,
	 ];
}
####################################################################################
sub create_controls {
   my ($list, $c, $labels)=@_;
   foreach my $label (@$labels) {
      $label->add_control($list, $c);
   }
   $list;
}
####################################################################################
sub expand {
   my ($self, $new_min, $new_max, $new_proto_length, $new_code, $labels)=@_;
   my $limit=&Node::_expand;

   if (defined($new_proto_length) && !defined($new_max)) {
      create_controls(( $self->ellipsis_code ||= [ ] ), $new_code, $labels);
      assign_max($limit, $#{$self->code}+$self->min_arg);
   }
   for (my $i=$new_min; $i<=$limit; ++$i) {
      create_controls(( $self->code->[$i-$self->min_arg] ||= dup_or_new_code($self->ellipsis_code) ),
		      $new_code, $labels);
   }
}
####################################################################################
sub push_code {
   my ($self, $new_code, $n, $labels)=@_;
   while (--$n >= 0) {
      push @{$self->code}, create_controls([ ], $new_code, $labels);
   }
}
####################################################################################
sub push_ellipsis_code {
   my ($self, $new_code, $labels)=@_;
   $self->ellipsis_code=create_controls([ ], $new_code, $labels);
}
####################################################################################
sub resolve {
   my $l=&Node::resolve;
   $l && $l->resolve($_[1]);
}
####################################################################################
sub complain_mismatch {
   "already defined with labels and signature"
}
####################################################################################

package Polymake::Overload::Labeled;

use Polymake::Struct (
   '@control_list',
);

sub resolve {
   $_[0]->control_list->resolve;
}

sub complain_mismatch {
   "already defined without signature"
}
####################################################################################
package Polymake::Overload::TemplateParam;
use Polymake::Struct (
   [ new => '$$$' ],
   [ '$name' => '#1' ],
   [ '$index' => '#2' ],
   [ '$default' => '#3' ],
   '$deduced',
);

sub builtin { undef }
sub special { undef }

####################################################################################
package Polymake::Overload;
use Polymake::Ext;

# special value of Node->keywords signaling that the resolved instance wants to take care of keywords on its own
declare @deferred_keywords;

sub parse_default_value {
   (undef, my ($args, $pkg, $ctx, $value))=@_;
   $#{$_[0] ||= [ ]}=$args;		# fill in 'undef's for the omitted default values
   # Default values are dynamic when they refer to preceding arguments.
   # negative indexes are relative references;
   # they can be kept as is, because the argument list ends just before the position the default value is going to be inserted at.
   my $dynamic= $value =~ s/\$_ (?= \[\s*  -?\d+       \s*\])/\$_[0]->/gx;     # referencing a single argument
   $dynamic  += $value =~ s/\@_ (?= \[\s* ([-\d.,\s]+) \s*\])/\@{\$_[0]}/gx;   # referencing a list of arguments
   $value="sub { $value }" if $dynamic;
   $value="package $pkg; $value";
   push @{$_[0]}, $ctx ? $ctx->eval_expr->($value) : eval($value);
   if ($@) {
      $@ =~ s/ at \(eval \d+.*$//;
      croak( "invalid default value '$value' for argument ", $args+1, ": $@" );
   }
   mark_dynamic_default($_[0]) if $dynamic;
}

sub parse_signature {
   my ($sig, $pkg, $code, $opts)=@_;
   my (@arg_list, $min, $defaults, @repeated, @extra_template_params, @args_for_deduction, $percent_seen);
   my $type_eval_pkg=$pkg;
   my $type_eval_symtab;
   my $ctx=$opts->{context};
   my $type_args_may_be_omitted=0;

   if (defined (my $extra=$opts->{extra_template_params})) {
      $type_eval_pkg="Polymake::Overload::_type_eval";
      $type_eval_symtab=get_pkg($type_eval_pkg,1);
      namespaces::using($type_eval_pkg,$pkg);
      push @arg_list, 'namespaces::TemplateExpression';
      push @repeated, undef;
      $type_args_may_be_omitted=1;
      $extra =~ s/^\s+//;
      while ($extra =~ m{\G $type_param_re \s* (?: ,\s* | $ ) }xog) {
	 # TODO: process typechecks
	 my ($name, $default)=@+{qw(name default)};
	 my $placeholder=new Core::PropertyTypePlaceholder($name, $pkg, $ctx);
         my $tpcount=@extra_template_params;
	 $placeholder->cppoptions=new TemplateParam("T$tpcount", $tpcount, $default);
	 define_function($type_eval_symtab, $name, sub { $placeholder->cppoptions->deduced ||= defined($min)+1; $placeholder });
	 push @extra_template_params, $placeholder;
         $type_args_may_be_omitted &&= defined($default);
      }
   }

   $sig =~ s/^\s+//;
   my $fixed=1;
   while (pos($sig) < length($sig)) {
      if ($sig =~ /\G ; \s*/gxc) {
	 if (defined $min) {
	    croak( "invalid signature: multiple ';'" );
	 }
	 $min=@arg_list;
	 next;
      }
      if ($sig =~ /\G (?: \$ | (?'primitive' [if]\b) | (?'type' $type_alt_re) )
		      (?: \s*=\s* (?'default' $expression_re) | (?('type') \s*(?'repeated' \+)\s*) )? \s*(?:,\s*)?/gxco) {
	 my ($primitive, $type, $default_value, $repeated)=@+{qw(primitive type default repeated)};
	 if (defined $default_value) {
	    defined($min) or croak( "only optional arguments may have default values" );
	    parse_default_value($defaults, $#arg_list, $pkg, $ctx, $default_value);
	 }
	 if (defined $type) {
	    my $proto;
	    if ($type =~ /^$qual_id_re$/o) {
	       if ($type_eval_symtab && exists $type_eval_symtab->{$type}) {
		  $proto=$type_eval_symtab->{$type}->();
		  push @args_for_deduction, [ $proto, is_method($code)+@arg_list ];
		  undef $type;
	       } else {
		  $type=namespaces::lookup_class($pkg, $type, defined($ctx) ? ($ctx->pkg) : ())
		        || croak( "Unknown type $type" );
	       }
            } elsif ($type =~ /\|/) {
	       my @alt_list;
	       foreach (split /\s*\|\s*/, $type) {
		  if (/^$qual_id_re$/o) {
		     push @alt_list, namespaces::lookup_class($pkg, $_, defined($ctx) ? ($ctx->pkg) : ())
		                     || croak( $type_eval_symtab && exists $type_eval_symtab->{$_}
					       ? "Can't use template type argument $_ in an alternative"
					       : "Unknown type $_" );
		  } else {
		     translate_type(my $tt=$_);
		     $tt="package $type_eval_pkg; $tt";
		     $proto=defined($ctx) ? $ctx->eval_expr->($tt) : eval($tt)  or report_type_error($_);
		     if ($proto->abstract) {
			croak( "Can't use the type $_ depending on a template argument in an alternative" );
		     }
		     push @alt_list, $proto->pkg;
		  }
	       }
	       $type=\@alt_list;
	    } else {
	       translate_type(my $tt=$type);
	       $tt="package $type_eval_pkg; $tt";
	       $proto=defined($ctx) ? $ctx->eval_expr->($tt) : eval($tt)  or report_type_error($type);
	       if ($proto->abstract) {
		  if (defined($proto->context_pkg) && !exists $opts->{extra_template_params} && !is_method($code)) {
		     croak("template parameters in abstract type ", $proto->full_name, " does not match the function declaration" );
		  }
		  push @args_for_deduction, [ $proto, is_method($code)+@arg_list ];
	       }
	       $type=$proto->pkg;
	    }
	 } elsif ($primitive) {
	    $type= $primitive eq 'i' ? "Polymake::Overload::integer" : "Polymake::Overload::float";
	 }
	 push @arg_list, $type || '$';
	 push @repeated, $repeated;
	 undef $fixed if defined($repeated);
	 next;
      }
      if ($sig =~ /\G (?: (%) | \@) \s*$/gxc) {
	 $percent_seen=$1;
         undef $fixed;
	 if (defined($min) && $percent_seen && $#arg_list>=0 && !$repeated[-1]) {
	    # provide undef's as default values for the optional parameters
	    $#{$defaults ||= [ ]}=$#arg_list;
	 }
	 last;
      }
      croak( "invalid function signature, parser stopped at `!' : '", substr($sig, 0, pos($sig)), "!", substr($sig, pos($sig)), "'" );
   }

   if ($type_eval_symtab) {
      finalize_extra_template_params(\@extra_template_params, $pkg, $type_eval_pkg, $ctx);
      my $with_explicit_template_args=
         sub {
	    local_refs(localize_template_params(\@extra_template_params,
						deduce_extra_template_params(\@extra_template_params, \@args_for_deduction,
									     \@_, 1, shift, \&deny_builtin)));
	    &$code;
	 };
      $opts->{wrapper}=
         @args_for_deduction || $type_args_may_be_omitted
	 ? [ $with_explicit_template_args,
	     sub {
		local_refs(localize_template_params(\@extra_template_params,
						    deduce_extra_template_params(\@extra_template_params, \@args_for_deduction,
										 \@_, 1, undef, \&deny_builtin)));
		&$code;
	     } ]
	 : $with_explicit_template_args;

   } elsif (@args_for_deduction) {
      $opts->{wrapper}=sub { check_matching_template_params(\@args_for_deduction, \@_, \&deny_builtin); &$code; };
   }

   if (exists($opts->{keywords}) != defined($percent_seen)) {
      croak( defined($percent_seen) ? "missing keyword argument descriptions" : "missing '%' in signature" );
   }
   $min=@arg_list unless defined $min;

   ($min, $fixed && scalar(@arg_list), $defaults, \@repeated, @arg_list);
}
####################################################################################
sub finalize_extra_template_params {
   my ($extra_template_params, $pkg, $type_eval_pkg, $ctx)=@_;
   foreach my $placeholder (@$extra_template_params) {
      if (defined (my $default=$placeholder->cppoptions->default)) {
	 if ($placeholder->cppoptions->deduced==1) {
	    croak( "template parameter ", $placeholder->name,
		   " may not have default value as it can always be deduced from the function arguments" );
	 } else {
	    my $simple= $default !~ /^\s*\{/;
	    if ($simple) {
	       # default value is not dynamic: check whether it depends on other parameters
	       translate_type(my $tt=$default);
	       $tt="package $type_eval_pkg; $tt";
	       my $proto=defined($ctx) ? $ctx->eval_expr->($tt) : eval($tt)  or report_type_error($default);
	       if ($proto->abstract) {
		  if (defined($proto->context_pkg)) {
		     croak("abstract default value ", $proto->full_name, " does not match the function declaration" );
		  }
	       } else {
		  $placeholder->cppoptions->default=$proto;
		  next;
	       }
	    }
	    if ($simple) {
	       translate_type($default);
	       $default="{ $default }";
	    } else {
	       translate_type_expr($default);
	    }
	    foreach (@$extra_template_params) {
	       my ($name, $index)=($_->name, $_->cppoptions->index);
	       if ($default =~ s{\b(?<!:)$name(?:->type)}{\$_[0]->[$index]}g) {
		  if ($index > $placeholder->cppoptions->index && $_->cppoptions->deduced != 1) {
		     croak( "default value for template parameter ", $placeholder->name,
			    " refers to parameter ", $_->name, " which is to be evaluated later" );
		  }
	       }
	    }
	    $default="package $pkg; sub $default";
	    $placeholder->cppoptions->default=defined($ctx) ? $ctx->eval_expr->($default) : eval($default);
	 }
      }
   }
   Symbol::delete_package($type_eval_pkg);
}

sub deny_builtin {
   croak( "Argument '$_[0]' is not an object and can't be involved in template argument type deduction" );
}

sub get_arg_proto {
   my ($arg, $guess_builtin_type)=@_;
   if (is_object($arg)) {
      my $proto=UNIVERSAL::can($arg,"prototype")
         or croak("package ", ref($arg), " does not belong to any declared property or object type and hence can't be used for template parameter deduction");
      $proto->();
   } else {
      $guess_builtin_type->($arg);
   }
}

sub deduce_extra_template_params {
   my ($extra_template_params, $args_for_deduction, $args, $arg_offset, $explicit_args, $guess_builtin_type)=@_;
   my @result;
   $#result=$#$extra_template_params;

   # first deduce from the regular arguments
   foreach my $descr (@$args_for_deduction) {
      foreach ($descr->[0]->match_type(get_arg_proto($args->[$descr->[1]-$arg_offset], $guess_builtin_type))) {
	 my ($placeholder, $proto)=@$_;
	 my $index=$placeholder->cppoptions->index;
	 if (defined $result[$index]) {
	    if ($result[$index] != $proto) {
	       if (UNIVERSAL::isa($result[$index]->pkg, $proto->pkg)) {
		  $result[$index]=$proto;
	       } elsif (!UNIVERSAL::isa($proto->pkg, $result[$index]->pkg)) {
		  croak( "deduction of template parameter ", $placeholder->name, " leads to incompatible types ",
			 $result[$index]->full_name, " and ", $proto->full_name );
	       }
	    }
	 } else {
	    $result[$index]=$proto;
	 }
      }
   }

   # now process the explicitly given parameters
   my $last_explicit= defined($explicit_args) ? ($explicit_args=eval_type_expr($explicit_args), $#$explicit_args) : -1;

   foreach my $placeholder (@$extra_template_params) {
      my $index=$placeholder->cppoptions->index;
      if ($index<=$last_explicit) {
	 $result[$index]=$explicit_args->[$index]->resolve_abstract;
      } elsif (!defined($result[$index]) && defined (my $default=$placeholder->cppoptions->default)) {
	 if (is_object($default)) {
	    $result[$index]=$default;
	 } else {
	    $result[$index]=$default->(\@result);
	 }
      }
   }

   @result
}

sub localize_template_params {
   my $extra_template_params=shift;
   my $i=0;
   map { my $placeholder=$extra_template_params->[$i++];
	 (@{$placeholder->locals}, \($placeholder->param), $_ || croak( "undefined explicit template parameter ", $placeholder->name ) )
   } @_;
}

sub check_matching_template_params {
   my ($args_for_deduction, $args, $guess_builtin_type)=@_;
   foreach my $descr (@$args_for_deduction) {
      foreach ($descr->[0]->match_type(get_arg_proto($args->[$descr->[1]], $guess_builtin_type))) {
	 my ($abstract, $real)=@$_;
	 my $expected=$abstract->concrete_type($args->[0]);
	 if ($real != $expected && !UNIVERSAL::isa($real->pkg,$expected->pkg)) {
	    croak( "type deduction based on argument $descr->[1] leads to conflict: expected ",
		   $expected->full_name, ", got ", $real->full_name );
	 }
      }
   }
}
####################################################################################
my %dictionary;

sub dict_node {
   my ($pkg, $name, $is_method, $node_type)=@_;
   my $node=$dictionary{$pkg}->{$name};
   if (defined($node)) {
      if (ref($node) ne $node_type) {
	 croak( $is_method ? "method" : "function", " $pkg\::$name ", $node->complain_mismatch );
      }
   } elsif ($is_method) {
      no strict 'refs';
      if (exists &{"$pkg\::$name"}) {
	 croak( "non-overloaded method $pkg\::$name already defined" );
      }
   } elsif (defined (my $sub=namespaces::lookup($pkg,$name))) {
      my $owner_pkg=method_owner($sub);
      if (defined (($node=$dictionary{$owner_pkg}) &&= $node->{$name})) {
	 # expanding the imported overloaded function
	 $dictionary{$pkg}->{$name}=$node;
      } else {
	 croak( "non-overloaded function $pkg\::$name already ",
		$owner_pkg ne $pkg ? "imported from package $owner_pkg" : "defined" );
      }
   }
   $node;
}
####################################################################################
sub add_fallback {
   my ($pkg, $name, $code)=@_;
   ( $dictionary{$pkg}->{$name} || croak( "unknown overloaded function $pkg\::$name" ) )->backtrack=$code;
}
####################################################################################
sub _add {
   my ($caller, $name, $sig, $code, $labels, $opts)=@_;
   my $pkg=$name =~ s/^(.*)::([^:]+)$/$2/ ? $1 : $caller;
   my ($min, $max, $default_args, $repeated, @arg_list) = ref($sig) ? @$sig : parse_signature($sig,$caller,$code,$opts);
   my $kw=$opts->{keywords};
   if (defined $kw) {
      if (is_hash($kw)) {
	 $kw=[ $kw ];
      } elsif (is_ARRAY($kw)) {
	 foreach my $table (@$kw) {
	    if (!is_hash($table)) {
	       croak( "expected a hash with keyword descriptions, got ", ref($table) || "'$table'" );
	    }
	 }
      } else {
	 croak( "expected a hash with keyword descriptions or an array thereof; got ", ref($kw) || "'$kw'" );
      }
   }
   my ($is_method, @is_lvalue)= is_object($code) ? (1) : (is_method($code), is_lvalue($code));
   if ($is_method) {
      ++$min;
      ++$max if defined($max);
      unshift @arg_list, $pkg;
      unshift @$default_args, undef if $default_args;
   }
   if (my $wrapper=$opts->{wrapper}) {
      if (ref($wrapper) eq "ARRAY") {
	 local $opts->{wrapper}=$wrapper->[1];
	 local_shift($repeated);
	 _add($caller, $name,
	      [ $min && $min-1,
		$max && $max-1,
		$default_args && inherit_class([ @$default_args[1..$#$default_args] ], $default_args),
		$repeated,
		@arg_list[1..$#arg_list] ],
	      $code, $labels, $opts);
	 $wrapper=$wrapper->[0];
      }
      if (defined($wrapper)) {
	 set_method($wrapper) if $is_method;
	 declare_lvalue($wrapper, @is_lvalue) if @is_lvalue;
	 $code=$wrapper;
      }
   }
   my $node_type= $labels ? "Polymake::Overload::LabeledNode" : "Polymake::Overload::Node";
   my ($signature, @last_glob, $last_repeated);
   my $arg= $is_method ? 0 : -1;
   if (defined (my $node=dict_node($pkg, $name, $is_method, $node_type))) {
      ($signature)= $node->signature =~ /^([^,]+)/;
   DESCEND: {
	 while (++$arg <= $#arg_list  &&  !ref($arg_list[$arg])  &&  $arg_list[$arg] eq '$') { $signature.=',$' }

	 if ($arg <= $#arg_list) {
	  EXISTING: {
	       my $arg_type=$arg_list[$arg];
	       $arg_type=$arg_type->[0] if ref($arg_type);
	       if (defined (my $next_arg=$node->next_arg)) {
		  if ($next_arg == $arg) {
		     $node->expand($min,$max,undef,$code,$labels);
		     $min=$arg+1 if $min<=$arg;
		     my $next_node_sub=$arg_type->can($signature);
		     if (defined($next_node_sub)  &&  $arg_type eq method_owner($next_node_sub)) {
			# exact match - more than one function have the $arg-prefix, must descend deeper
			$signature .= ",$arg_type";
			$signature =~ s/::/\@/g;
			$node=&$next_node_sub;
			redo DESCEND;
		     }

		  } elsif ($next_arg < $arg) {
		     $node->expand($min,$max,undef,$code,$labels);
		     $min=$next_arg+1 if $min<=$next_arg;
		     if (defined (my $next_node_sub=UNIVERSAL->can($node->signature))) {
			$node=&$next_node_sub;
			redo EXISTING;
		     }

		     my $uni_node=$node_type->new($min,$max,0);
		     if ($min<=$arg) {
			$uni_node->push_code($code,$arg-$min+1,$labels);
			$uni_node->default_args=$default_args;
			$min=$arg+1;
		     }
		     $uni_node->signature=$signature;
		     $uni_node->next_arg=$arg;
		     define_function("UNIVERSAL", $node->signature, sub : method { $uni_node });

		  } else { # $next_arg > $arg
		     my $uni_node=$node->clone;
		     assign_max($uni_node->min_arg, $arg+1);
		     $node->expand($min,$max,undef,$code,$labels);
		     $min=$arg+1 if $min<=$arg;
		     $node->signature=$signature;
		     $node->next_arg=$arg;
		     define_function("UNIVERSAL", $signature, sub : method { $uni_node });
		  }

	       } else {
		  # existing signature is a prefix of the new one
		  my $uni_node= (!defined($node->max_arg) || $node->max_arg > $arg)
		     && $node_type->new(max($node->min_arg, $arg+1), $node->max_arg, 0);
		  if ($uni_node) {
		     $uni_node->signature=$node->signature;
		     $node->signature=$signature;
		     $uni_node->default_args=$node->default_args;
		     push @{$uni_node->code}, splice @{$node->code}, max($arg+1-$node->min_arg, 0);
		     if (!defined($node->max_arg)) {
			$uni_node->ellipsis_code=$node->ellipsis_code;
			$node->ellipsis_code='';
			$uni_node->keywords=$node->keywords;
			$node->keywords=undef;
		     }
		     define_function("UNIVERSAL", $signature, sub : method { $uni_node });
		  }
		  $node->next_arg=$arg;
		  $node->expand($min,$max,undef,$code,$labels);
		  if ($min<=$arg) {
		     $node->default_args=$default_args;
		     $min=$arg+1;
		  }
	       }

	       @last_glob=($arg_list[$arg], $signature);
               $last_repeated=$repeated->[$arg];
	       $signature .= ",$arg_type$last_repeated";
	       $signature =~ s/::/\@/g;
	    }
	 } else {
	    for (;;) {
	       $node->expand($min,$max,$arg,$code,$labels);
	       if (defined (my $next_arg=$node->next_arg)) {
		  if (!defined($max) || $max>$next_arg) {
		     # if the node already had default arguments and/or keywords,
		     # expand would throw an exception due to intersections in the code array
		     $node->default_args=$default_args;
		     if ($kw) {
			$node->keywords=$kw;
			$node->push_ellipsis_code($code, $labels);
		     }
		     if (defined (my $next_node_sub=UNIVERSAL->can($node->signature))) {
			$min=$next_arg+1 if $min<=$next_arg;
			$node=&$next_node_sub;
			next;
		     } else {
			--$arg;
			@last_glob=("UNIVERSAL", $node->signature);
			last;
		     }
		  }
	       }
	       return $code;
	    }
	 }
      }
   } else {
      # first instance of this function
      $signature=".$pkg\@$name";
      $signature =~ s/::/\@/g;
   }

   my $min_reached=$min<=$arg;
   for (;;) {
      while (++$arg <= $#arg_list  &&  !ref($arg_list[$arg]) && $arg_list[$arg] eq '$'  &&  !$last_repeated  &&  ($arg<$min || !$kw)) { $signature.=",\$" }

      my $node=$node_type->new($min, $max, @last_glob && (ref($last_glob[0]) || $last_glob[0] !~ /^UNIVERSAL::/));
      if ($min<=$arg) {
	 $node->default_args=$default_args;
         if ($kw) {
            $node->keywords=$kw;
            $node->push_ellipsis_code($code, $labels);
            if ($min_reached && @last_glob) {
               my $kw_node=$node_type->new($min-1, undef, 0);
               $kw_node->signature="$last_glob[1],\$";
               $kw_node->default_args=$default_args;
               $kw_node->keywords=$kw;
               $kw_node->push_ellipsis_code($code, $labels);
               define_function("Polymake::Overload::keyword", $last_glob[1], sub : method { $kw_node });
            }
         } else {
	    $node->push_code($code,$arg-$min+1,$labels);
         }

	 $min=$arg+1;
         $min_reached=1;
      }
      $node->signature=$signature;

      if (@last_glob) {
	 if (ref($last_glob[0])) {
	    define_function($_, $last_glob[1], sub : method { $node }) for @{$last_glob[0]};
	 } else {
	    define_function(@last_glob, sub : method { $node });
	 }
         if ($last_repeated) {
            my $repeat_node=$node->clone;
            $repeat_node->next_arg=$arg;
            undef $repeat_node->backtrack;
	    foreach (ref($last_glob[0]) ? @{$last_glob[0]} : $last_glob[0]) {
	       define_function($_, $signature, sub : method { ++$_[0]; $repeat_node });
	    }
         }

      } else {
	 $dictionary{$pkg}->{$name}=$node;
	 my $head_code=define_function($pkg, $name,
				       $opts->{typeofs}
				       ? sub { &{ $node->resolve(&process_typeofs) || complain($pkg,$name,$is_method,@_) } }
				       : sub { &{ $node->resolve(\@_) || complain($pkg,$name,$is_method,@_) } });
	 set_method($head_code) if $is_method;
	 declare_lvalue($head_code, @is_lvalue) if @is_lvalue;
	 namespaces::export_sub($pkg,$head_code) if exists $opts->{extra_template_params};
	 if ($is_method  and  !UNIVERSAL::isa($pkg, "Polymake::Overload::can")) {
	    no strict 'refs';
	    push @{"$pkg\::ISA"}, "Polymake::Overload::can";
	 }
      }

      if ($arg > $#arg_list) {
         if ($kw) {
            if ($last_repeated) {
               my $kw_node=$node_type->new($node->min_arg,undef,0);
               $kw_node->signature="$signature,\$";
               $kw_node->default_args=$default_args;
               $kw_node->keywords=$kw;
               $kw_node->push_ellipsis_code($code, $labels);
               define_function("Polymake::Overload::keyword", $signature, sub : method { $kw_node });
               $node->next_arg=$arg;
            }
	 } else {
           $node->push_ellipsis_code($code,$labels) unless defined($max);
         }
	 last;
      }

      $node->next_arg=$arg;
      @last_glob=(ref($arg_list[$arg]) || $arg_list[$arg] ne '$' ? $arg_list[$arg] : "UNIVERSAL", $signature);
      $last_repeated=$repeated->[$arg];
      my $arg_type=$arg_list[$arg];
      $arg_type=$arg_type->[0] if ref($arg_type);
      $signature .= ",$arg_type$last_repeated";
      $signature =~ s/::/\@/g;
   }
   $code;
}
####################################################################################
sub complain {
   my ($pkg, $name, $is_method)=splice @_, 0, 3;
   my @args=map { ref($_) || "\$" } @_;
   if (my $leading_object=$is_method && shift @args) {
      croak( "no matching overloaded instance of $leading_object\->$name(" . join(",", @args) . ")" );
   } else {
      croak( "no matching overloaded instance of $pkg\::$name(" . join(",", @args) . ")" );
   }
}
####################################################################################
# 'name', 'signature', \&code, { options } =>
sub add {
   shift;	# get rid of own package name
   (my ($name, $signature, $code, %opts)=@_) >= 3 or return;
   my $caller=caller;
   my $label=delete $opts{label};
   $label=[ $label ] if is_object($label);
   if (defined $signature) {
      $code=_add($caller, $name, $signature, $code, $label, \%opts);
      if ($label && ref($code) eq "CODE") {
	 $name=~/([^:]+)$/;
	 set_sub_name($code, $1);
	 set_prototype($code, $signature);
      }
   } else {
      # without signature
      croak( "neither labels nor signature specified" ) unless defined($label);
      my $is_method=is_method($code);
      my @is_lvalue=is_lvalue($code);
      my $pkg= $name =~ s/^(.*)::([^:]+)$/$2/ ? $1 : $caller;
      my $node=dict_node($pkg, $name, $is_method, "Polymake::Overload::Labeled");
      if (!defined($node)) {
	 $node=$dictionary{$pkg}->{$name}=new Labeled;
	 my $head_code=define_function($pkg, $name, sub { &{ $node->resolve } });
	 set_method($head_code) if $is_method;
	 declare_lvalue($head_code, @is_lvalue) if @is_lvalue;
      }
      LabeledNode::create_controls($node->control_list, $code, $label);
   }
   if (defined (my $subst=$opts{subst_const_op})) {
      no strict 'refs';
      $name=~/([^:]+)$/;
      namespaces::subst_const_op(is_ARRAY($subst) ? @$subst : ($caller,$subst), \&{"$caller\::$1"});
   }
}
####################################################################################
sub add_global {
   shift;	# get rid of own package name
   my $caller=caller;
   my ($name, $signature, $code, %opts)=@_;
   croak( "cannot declare a non-method '$name' as global" ) unless is_method($code);
   croak( "package $caller tries to declare method '$name' as global although it comes from different package" )
      unless method_owner($code) eq $caller;
   my $label=delete $opts{label};
   $label=[ $label ] if is_object($label);
   $code=_add($caller, $name, $signature, $code, undef, \%opts);
   $name =~ s/^.*::([^:]+)$/$1/;
   if ($label) {
      set_sub_name($code, $name);
      set_prototype($code, $signature);
   }
   _add("Polymake::Overload::Global", $name, $signature, sub { $code }, $label, \%opts);
}
####################################################################################
sub resolve {
   my ($pkg, $function, $args)=@_;
   my $node;
   ($node=$dictionary{$pkg}  and  $node=$node->{$function}) ||
   ($node=&UNIVERSAL::can  and  $node=$dictionary{method_owner($node)}  and  $node=$node->{$function})
   and $node->resolve($args)
}

# for labeled functions returns the entire control list!
sub resolve_labeled {
   my ($pkg, $function, $args)=@_;
   my $node;
   $node=$dictionary{$pkg}  and  $node=$node->{$function}  and  Node::resolve($node,$args);
}
####################################################################################
sub process_typeofs {
   [ map { is_object($_) ? bless \( my $dummy ), $_->pkg : $_ } @_ ]
}
####################################################################################
sub process_keywords {
   my $tables=shift;

   # don't process anything, the time is not ripe yet
   return \@_ if $tables == \@deferred_keywords;

   # process the given arguments
   # for performance reasons we duplicate some code instead of branching in the loop

   my (@unknown, @processed_args, $table, $t);
   if (@$tables>1) {
      my $direct_table=0;
      push @processed_args, {} for 0..$#$tables;
      for (my $i=0; $i<=$#_; ++$i) {
	 my $key=$_[$i];
	 if (is_keyword($_[$i]) && ++$i <= $#_) {
	    my $known;
	    $t=0;
	    foreach $table (@$tables) {
	       if (exists $table->{$key}) {
		  $processed_args[$t]->{$key} = $_[$i];
		  $known=1;
	       }
	       ++$t;
	    }
	    push @unknown, $key unless $known;

	 } elsif (ref($key) eq "HASH") {
	    if ($direct_table > $#$tables) {
	       croak( "too many hash arguments" );
	    }
	    push %{$processed_args[$direct_table++]}, %$key;

	 } else {
	    croak( "KEYWORD => value pairs expected, got ", ref($key) || "'$key'" );
	 }
      }

   } else {
      my $args=$processed_args[0]={ };
      $table=$tables->[0];
      for (my $i=0; $i<=$#_; ++$i) {
	 my $key=$_[$i];
	 if (is_keyword($_[$i]) && ++$i <= $#_) {
	    if (!defined($table) || exists $table->{$key}) {
	       $args->{$key} = $_[$i];
	    } else {
	       push @unknown, $key;
	    }
	 } elsif (ref($key) eq "HASH") {
	    @$args{ keys %$key }=values %$key;
	 } else {
	    croak( "KEYWORD => value pairs expected, got ", ref($key) || "'$key'" );
	 }
      }
   }

   if (@unknown) {
      my %known;
      push %known, %$_ for @$tables;
      delete @known{ keys %$_ } for @processed_args;
      croak( "unknown keyword argument", (@unknown>1 && "s"), ": ", join(", ", @unknown),
	     "\nallowed keywords are: ", join(", ", sort keys %known) );
   }

   # filter the values and fill in the defaults
   $t=0;
   foreach $table (@$tables) {
      my $args=$processed_args[$t++];
      while (my ($key, $descr)=each %$table) {
	 if (exists $args->{$key}) {
	    $descr=$descr->[0] if is_ARRAY($descr);
	    $descr->($args,$key) if is_code($descr);

	 } elsif (is_ARRAY($descr)) {
	    my $default=$descr->[1];
	    if (is_code($default)) {
	       $default->($args, $key);
	    } elsif (defined($default)) {
	       $args->{$key}=$default;
	       $descr=$descr->[0];
	       $descr->($args,$key) if is_code($descr);
	    }
	 } elsif (defined($descr) && !is_code($descr)) {
	    $args->{$key}=$descr;
	 }
      }
   }

   @processed_args
}

sub Polymake::enum {
   my ($default, %enum);
   foreach my $name (@_) {
      if ($name =~ /=default$/) {
	 $default=$`;
	 $enum{$`}=1;
      } else {
	 $enum{$name}=1;
      }
   }
   my $accept=sub {
      my ($self, $key)=@_;
      $enum{$self->{$key}}
         or croak( "unknown option value: $key => $self->{$key}" );
   };
   defined($default) ? [ $accept, $default ] : $accept;
}
####################################################################################

package Polymake::Overload::can;

sub can {
   my $head=&UNIVERSAL::can or return undef;
   my ($method)=splice @_, 1, 1;
   my $node=$dictionary{method_owner($head)};
   ($node &&= $node->{$method}) ? $head=$node->resolve(\@_) : @_==1 ? $head : undef
}

####################################################################################

package Polymake::Overload::Global;
# just to pull it into being

package Polymake::Overload::integer;
@ISA=('Polymake::Overload::float');

1
