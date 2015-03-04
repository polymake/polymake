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
use feature 'state';

package Polymake::Core::CPlusPlus;

my ($debug, $dl_suffix, $custom_handler, $private_wrapper_ext, $forbid_private_wrappers, $BigObject_cpp_options, $BigObjectArray_cpp_options);

#######################################################################################

package Polymake::Core::CPlusPlus::FuncDescr;
use Polymake::Struct (
   '$wrapper',
   '$func_ptr',
   '$name | func_ptr_type',
   '$source_file',
   '$arg_types',
   '$cross_apps',
   '$auto_func',
);

sub source_line { $_[0]->source_file+0 }

sub suspend {
   my ($self, $perApp, $extension)=@_;
   my (@apps, @missing_apps);
   foreach my $app_name (@{$self->cross_apps}) {
      if (defined (my $app=lookup Application($app_name))) {
         push @apps, $app;
      } else {
         push @missing_apps, $app_name;
      }
   }
   if (@missing_apps) {
      push @{Application::SuspendedItems::add($perApp->application, $extension, @missing_apps)->functions}, $self;
      1
   } else {
      $self->cross_apps=\@apps;
      0
   }
}

#######################################################################################

package Polymake::Core::CPlusPlus::AutoFunction;

use Polymake::Struct (
   [ new => '$$$%' ],
   [ '$name' => '#%' ],
   [ '$perl_name' => '#1' ],
   [ '$wrapper_name' => '#1' ],
   [ '$flags' => '#2' ],
   [ '$application' => '#3' ],
   [ '$cross_apps' => '$Application::cross_apps_list' ],
   [ '$extension' => '$Application::extension' ],
   [ '@include' => '#%' ],
   [ '$embedded' => '#%', default => 'undef' ],
   [ '$macro_call' => '#%', default => '$this->flags & $func_is_void ? "Void( " : "( "' ],
   '@all_args',
   '@template_params',
   [ '$explicit_template_params' => '#%', default => '[]' ],
   '@lvalue_args',
   '@arg_flags',
   [ '$via_object' => '#%', default => 'undef' ],
   '%inst_cache',
   [ '&builtin_code' => '#%' ],
   '%seen_in',                  # "sourcefile" => number of instantiations
);

sub prepare {
   my ($self, $arg_types, $arg_attrs, $pkg)=@_;
   my ($min, $max, @arg_types)=@$arg_types;
   my $internal=@_<4  and  $pkg="Polymake";
   my $tpcount_before_explicit=defined($self->via_object)+0;
   my $tpcount=$tpcount_before_explicit;
   if (is_integer($self->explicit_template_params)) {
      $tpcount+=$self->explicit_template_params;
      $self->explicit_template_params=[ 0..$self->explicit_template_params-1 ];
   } else {
      $tpcount+=@{$self->explicit_template_params};
   }
   my $perl_arg_index=0;
   my $cpp_arg_index=0;
   my ($arg_for_lvalue_opt, @anchor_args);

   if ($self->perl_name ne "operator" && $self->perl_name ne "construct") {
      $self->name ||= $self->perl_name;
      if (defined $self->cross_apps) {
         $self->wrapper_name.="_A";
      }
      if ($tpcount > $tpcount_before_explicit) {
         $self->wrapper_name.="_T";
         $self->name .= "<" . join(",", map { "T$_" } $tpcount_before_explicit..$tpcount-1) . ">";
      }
      if ($self->flags & $func_has_prescribed_ret_type) {
         $self->wrapper_name.="_R";
      } elsif ($self->macro_call eq "List( ") {
         $self->wrapper_name.="_L";
      }
   } elsif (@{$self->explicit_template_params}) {
      $self->flags |= $func_has_prescribed_ret_type ;   # the proto object is passed as the first argument
   }
   if ($self->flags & $func_has_prescribed_ret_type) {
      ++$perl_arg_index;
   }

   if ($self->flags & $func_is_method) {
      unless ($internal) {
         my $proto=&{UNIVERSAL::can($pkg, "typeof_gen") || croak( "pure perl package $pkg has no C++ binding" )}(undef);
         if (defined (my $opts=$proto->cppoptions)) {
            if ($opts->builtin) {
               if ($opts == $BigObject_cpp_options) {
                  if ($self->flags & $func_is_static) {
                     croak( "can't declare static methods with C++ binding for `big' objects" );
                  }
                  $self->flags |= $func_is_top_object_method;
                  if ($self->via_object) {
                     push @{$self->all_args}, "self.";
                     $self->wrapper_name.="_O";
                  }
                  goto NOMETHOD;
               }
               if ($opts == $BigObjectArray_cpp_options) {
                  croak( "Can't declare methods with C++ binding for `big' object arrays" );
               }
               unless ($self->flags & $func_is_static) {
                  croak( "can't declare methods for C++ primitive type ", $opts->builtin );
               }
            }
         } else {
            croak( "class ", $proto->full_name, " has no C++ binding" );
         }
      }
      if ($self->flags & $func_is_static) {
         # @note supposing that explicit_template_params is empty here, so that $tpcount==0;
         # revise this code when methods with explicit template parameters are allowed
         my $pkg_qual="";
         if ($self->name !~ /::$id_re$/o) {
            ($pkg_qual=$self->name) =~ s/%1/T0/ or $pkg_qual="T0::";
         }
         push @{$self->all_args}, $pkg_qual;
         push @{$self->explicit_template_params}, $perl_arg_index;
      } else {
         push @{$self->all_args}, "arg0.get<T$tpcount>()".($self->perl_name ne 'operator' && ".");
         push @{$self->template_params}, [ $perl_arg_index, $self->flags & $func_is_non_const ? $func_is_lvalue : $self->flags & ($func_is_lvalue|$func_is_lvalue_opt) ];
         $self->arg_flags->[$perl_arg_index]=$self->flags & $func_is_wary;
         $arg_for_lvalue_opt=$tpcount if $self->flags & ($func_is_lvalue | $func_is_lvalue_opt);
         push @anchor_args, "arg0" if $self->flags & $func_has_anchor;
         ++$cpp_arg_index;
      }
      ++$perl_arg_index;
      ++$tpcount;
    NOMETHOD: ;
   }

   my $sig_index=0;
   foreach my $arg_attr (@$arg_attrs) {
      my $arg_type=$arg_types[$sig_index];
      my ($is_repeated, $as_template_arg, $wrapper_suffix);
      if (ref($arg_type) eq "ARRAY") {
         $is_repeated=$arg_type->[1] eq "+";
         $arg_type=$arg_type->[0];
      }
      my $lval_flag=0;
      my $arg_flag=0;
      my $anchor_flag=0;
      my $cpp_arg="arg$cpp_arg_index";
      my $cast_to_target_type;

      while ($arg_attr =~ /$id_re/go) {
         if ($& eq "wary") {
            $arg_flag=$func_is_wary;
         } elsif ($& eq "int") {
            $arg_flag=$func_is_integral;
         } elsif ($& eq "lvalue") {
            $lval_flag=$func_is_lvalue;
         } elsif ($& eq "lvalue_opt") {
            $lval_flag=$func_is_lvalue_opt;
            unless ($self->flags & ($func_is_lvalue | $func_is_lvalue_opt | $func_is_void)) {
               $arg_for_lvalue_opt=$tpcount;
               $self->flags |= $func_is_lvalue_opt;
            }
         } elsif ($& eq "anchor") {
            $anchor_flag=$func_has_anchor;
         } else {
            croak( "unknown argument attribute '$&'" );
         }
      }
      if ($lval_flag == ($func_is_lvalue | $func_is_lvalue_opt)) {
         croak( "multiple conflicting argument attributes: $attrs" );
      }
      if ($lval_flag && $sig_index>=$min) {
         croak( "an lvalue argument can't be optional" );
      }

      if (is_object($arg_type)) {
         my $opts=$arg_type->cppoptions;
         if ($arg_type->abstract) {
            # except BigObject and BigObjectArray
            if (!defined($opts) || !$opts->builtin) {
               $as_template_arg=1;
               my $type_param_index=$arg_type->type_param_index;
               if (defined($type_param_index)) {
                  $cast_to_target_type=$tpcount_before_explicit+$type_param_index;
                  $wrapper_suffix="C";
               }
            }
         } elsif ($opts) {
            if ($opts->builtin) {
               if ($lval_flag) {
                  if ($opts == $BigObject_cpp_options || $opts == $BigObjectArray_cpp_options) {
                     croak( "superfluous 'lvalue' attribute: objects of type ", $arg_type->full_name, " are always passed by reference" );
                  } else {
                     croak( $arg_type->full_name, " is declared as perl built-in type and can't be passed to C++ code by reference" );
                  }
               }
               if ($opts != $BigObject_cpp_options && $opts != $BigObjectArray_cpp_options) {
                  $wrapper_suffix=$opts->builtin;
                  if ($is_repeated) {
                     $wrapper_suffix.="_P";
                     $as_template_arg=1;
                  } else {
                     $cpp_arg.=".get<$wrapper_suffix>()";
                  }
               }
            } else {
               $as_template_arg=1;
            }
         } else {
            croak( "class ", $arg_type->full_name, " has no C++ binding" );
         }

      } elsif ($arg_type eq '$') {
         $as_template_arg= substr($arg_attr,0,1) eq '*';

      } else {
         croak( "pure perl package '$arg_type' can't be passed to C++ subroutine" );
      }

      if ($as_template_arg || $lval_flag==$func_is_lvalue_opt) {
         push @{$self->all_args},
              "$cpp_arg.get<T$tpcount" . (defined($cast_to_target_type) && ", T$cast_to_target_type") . ">()";
         push @{$self->template_params}, [ $perl_arg_index, $lval_flag ];
         ++$tpcount;
      } else {
         push @{$self->all_args}, $cpp_arg;
         push @{$self->lvalue_args}, $perl_arg_index if $lval_flag;
      }
      $self->arg_flags->[$perl_arg_index]=$arg_flag;
      $self->wrapper_name.="_".($wrapper_suffix || ($as_template_arg ? "X" : "x"));
      $self->wrapper_name.=$lval_flag+$anchor_flag if $lval_flag+$anchor_flag;
      push @anchor_args, $cpp_arg if $anchor_flag;
      ++$perl_arg_index;
      ++$cpp_arg_index;
      ++$sig_index;
   }

   if ($max & $Overload::has_trailing_list) {
      $self->wrapper_name.="_e";
      $self->flags |= $func_is_ellipsis;
      push @{$self->all_args}, "arg$cpp_arg_index";
   } elsif ($max & $Overload::has_keywords) {
      $self->wrapper_name.="_o";
      push @{$self->all_args}, "arg$cpp_arg_index";
   }
   if (!$tpcount) {
      $self->inst_cache=undef;
   }

   if (@anchor_args) {
      $self->macro_call="Anch".$self->macro_call.scalar(@anchor_args).", ".join("", map { "($_)" } @anchor_args).", ";
   }
   if ($self->flags & ($func_is_lvalue|$func_is_lvalue_opt)) {
      $self->macro_call="Lvalue".$self->macro_call."T$arg_for_lvalue_opt, ";
   }
   if (my $fl=($self->flags & ($func_is_method|$func_is_lvalue|$func_is_lvalue_opt|$func_is_void))) {
      $self->wrapper_name .= "_f$fl";
   }
   $self->wrapper_name =~ tr/:/_/;
}

sub prepare_wrapper_code {
   my ($self, $code)=@_;
   my $n_args=@{$self->all_args};
   ++$n_args if $self->flags & $func_has_prescribed_ret_type;
   set_number_of_args($code, $n_args, $self->flags & $func_is_ellipsis, undef);
   declare_lvalue($code, 1) if $self->flags & ($func_is_lvalue|$func_is_lvalue_opt);
}

sub deduce_extra_params {
   my ($self, $args)=@_;
   # @note assuming that methods can't have explicit type parameters yet
   if ($self->flags & $func_is_static) {
      $args->[$self->explicit_template_params->[0]]->type
   } else {
      @{namespaces::fetch_explicit_typelist($args)}[@{$self->explicit_template_params}];
   }
}

sub gen_source_code {
   my ($self, $debug_code)=@_;

   my $t_args= $self->template_params ? @{$self->explicit_template_params} + $#{$self->template_params} : -1;
   ++$t_args if $self->via_object;
   my $typelist=join(",", map { "T$_" } 0..$t_args);
   my $call_name= ($self->name eq "stack" && $self->application->name."::") . $self->name;
   my @args=@{$self->all_args};
   my $arg_off= $self->flags & $func_has_prescribed_ret_type ? 1 : 0;
   my $arg_last=$#args;
   my $call_as_method= ($self->flags & $func_is_method) && (defined($self->via_object) || !($self->flags & $func_is_top_object_method)) && shift(@args);
   if ($self->flags & $func_is_static) {
      --$arg_last;  ++$arg_off;
   }

   ( "   template <" . join(", ", $t_args>=0 ? (map { "typename T$_" } 0..$t_args) : "typename T0=void") . ">\n",
     "   FunctionInterface4perl( " . $self->wrapper_name . ($t_args>=0 && ", $typelist") . " ) {\n",
     $arg_last >= 0 &&
     "      perl::Value " . join(", ", map { "arg$_(stack[".($_+$arg_off)."])" } 0..$arg_last) . ";\n",
     $self->via_object &&
     "      perl::Object arg0o;  arg0 >> arg0o;  const T0 self(arg0o);\n",
     "      WrapperReturn" . $self->macro_call . ($call_as_method or $call_name && "(") . "$call_name(" . join(", ", @args) . ")" . (!$call_as_method && $call_name && ")") . " );\n",
     "   };\n",
     "\n",
   )
}

sub find_instance {
   my ($self, $args, $arg_flags)=@_;
   $arg_flags //= $self->arg_flags;
   my $instance=$self->inst_cache;
   if (defined($self->via_object)) {
      $instance=$instance->{$self->via_object} or return;
   }
   my $builtin_code=$self->builtin_code;
   if (@{$self->explicit_template_params}) {
      foreach my $proto ($self->deduce_extra_params($args)) {
         $instance &&= $instance->{$proto->cpp_type_descr->typeid};
      }
   }
   foreach (@{$self->template_params}) {
      my ($arg, $lval_flag)=($args->[$_->[0]], $_->[1]);
      my $typeid=get_magic_typeid($arg, $lval_flag);
      if (defined $typeid) {
         undef $builtin_code;
         $instance &&= $instance->{$typeid};
         if ($lval_flag) {
            if ($lval_flag == $func_is_lvalue_opt) {
               $instance &&= $instance->[$typeid];
            } elsif ($typeid==1) {
               $self->complain_ro_violation($_->[0]);
            }
         }
      } else {
         my $proto=guess_builtin_type($arg, (my $int_flag=$arg_flags->[$_->[0]] & $func_is_integral));
         if ($lval_flag) {
            croak( "Can't pass a primitive type ", $proto->full_name, " by reference to a C++ function" );
         }
         if ($int_flag && $proto->cppoptions->name ne "long" && @_==3 && @$arg_flags==2) {
            # it is an operator and the numeric argument must be first converted to the type of another argument
            $proto=$args->[1-$_->[0]]->type;
            splice @$args, $_->[0], 1, StdConstr::std_parsing_constructor($proto, $args->[$_->[0]]);
         }
         my $descr=$proto->cppoptions->descr || provide_cpp_type_descr($proto, 1);
         if (is_code($descr)) {
            $descr=$descr->();
         }
         $instance &&= $instance->{$descr->typeid};
      }
   }
   $self->check_lvalue_args($args);
   $builtin_code // $instance;
}

sub is_private { 0 }
sub instance_macro { defined($_[0]->via_object) ? "MethodInstance4perl" : "FunctionInstance4perl" }

sub source_file { "auto-".$_[0]->perl_name.".cc" }

sub complain_ro_violation {
   my ($self, $arg_no)=@_;
   croak( "Attempt to modify a read-only C++ object passed as argument $arg_no to function ", $self->perl_name );
}

sub check_lvalue_args {
   my ($self, $args)=@_;
   foreach my $arg_no (@{$self->lvalue_args}) {
      if (defined (my $typeid=get_magic_typeid($args->[$arg_no], $func_is_lvalue))) {
         if ($typeid==1) {
            complain_ro_violation($self,$arg_no);
         }
      } else {
         croak( "Argument $arg_no in the call to ", $self->perl_name, " can't be passed by reference\n",
                "as it is not an object with C++ binding." );
      }
   }
}

#######################################################################################
package Polymake::Core::CPlusPlus::DuplicateInstance;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$descr' => '#1' ],
   [ '$application' => '#2' ],
   [ '$extension' => '$Application::extension' ],
);

sub embedded { (shift)->descr->auto_func->embedded }
sub is_instance_of { (shift)->descr->auto_func }

#######################################################################################
package Polymake::Core::CPlusPlus::LackingInstance;

use Polymake::Struct (
   [ new => '$$;$' ],
   [ '$is_instance_of' => '#1' ],
   '@args',     # 'C++ type expression', ...
   '@headers',  # [ 'header', ... ], ...
   [ '$embedded' => 'undef' ],
   [ '$application' => 'undef' ],
   [ '$cross_apps' => 'undef' ],
   [ '$extension' => 'undef' ],
   [ '$used_in_extension' => 'undef' ],
   '$source_file',
   '$is_private',
);

sub new {
   my $self=&_new;
   my ($auto_func, $args, $arg_flags)=@_;
   $arg_flags ||= $auto_func->arg_flags;
   set_application($self, $auto_func->embedded, $auto_func->application, $auto_func->cross_apps);
   set_extension($self->extension, $auto_func->extension // ($auto_func->application && $auto_func->application->origin_extension));

   if ($auto_func->via_object) {
      push @{$self->args}, $auto_func->via_object;
   }

   if (@{$auto_func->explicit_template_params}) {
      foreach ($auto_func->deduce_extra_params($args)) {
         my ($arg_type, $includes, $embedded, $app, $cross_apps, $extension)=$_->get_cpp_representation;
         push @{$self->args}, $arg_type;
         push @{$self->headers}, @$includes;
         set_application($self, $embedded, $app, $cross_apps);
         set_extension($self->extension, $extension);
      }
   }

   foreach (@{$auto_func->template_params}) {
      my ($arg_no, $lval_flag)=@$_;
      my ($arg_type, $includes, $embedded, $app, $cross_apps, $extension);
      if (defined (my $typeid=get_magic_typeid($args->[$arg_no], $lval_flag))) {
         my $const= $lval_flag && $typeid==0 ? "" : "const ";
         ($arg_type, $includes, $embedded, $app, $cross_apps, $extension)=($root->typeids->{$typeid} || die "unregistered C++ typeid $typeid\n")->get_cpp_representation;
         $arg_type="Wary< $arg_type >" if $arg_flags->[$arg_no] & $func_is_wary;
         $arg_type="perl::Canned< $const$arg_type >";
      } else {
         my $proto=guess_builtin_type($args->[$arg_no], $arg_flags->[$arg_no] & $func_is_integral);
         my $opts=$proto->cppoptions->finalize;
         if (ref($opts->builtin) eq "Polymake::Enum") {
            $arg_type="perl::Enum<" . $opts->name . ">";
         } elsif ($opts->template_params) {
            $arg_type=$opts->name;
            $arg_type="Wary< $arg_type >" if $arg_flags->[$arg_no] & $func_is_wary;
            $arg_type="perl::Canned< const $arg_type >";
         } else {
            $arg_type=$opts->name;
         }
         $includes=$opts->include;
         $app=$opts->application;
         $cross_apps=$opts->cross_apps;
      }
      push @{$self->args}, $arg_type;
      push @{$self->headers}, @$includes;
      set_application($self, $embedded, $app, $cross_apps);
      set_extension($self->extension, $extension);
   }

   remove_origin_extension($self);
   analyze_provenience($self);
   unless (defined($self->embedded)) {
      $self->source_file=$auto_func->source_file($args);
   }
   $self;
}

sub complain_source_conflict {
   "Don't know where to place the instance of ", $_[0]->is_instance_of->perl_name
}

sub gen_source_code {
   my ($self)=@_;
   my $func=$self->is_instance_of;
   my $macro_name=$func->instance_macro;
   if (defined $self->cross_apps) {
      $macro_name =~ s/(?=Instance4perl)/CrossApp/;
      my $app_list="(" . join(", ", scalar(@{$self->cross_apps}), map { '"'.$_->name.'"' } @{$self->cross_apps}) . ")";
      "   $macro_name(" . join(", ", $func->wrapper_name, $app_list, @{$self->args}) . ");\n"
   } else {
      "   $macro_name(" . join(", ", $func->wrapper_name, @{$self->args}) . ");\n"
   }
}

sub gen_temp_code {
   my ($self)=@_;
   ( $self->is_instance_of->gen_source_code($debug), $self->gen_source_code );
}

sub include {
   my ($self)=@_;
   (@{$self->is_instance_of->include}, @{$self->headers})
}

sub temp_include {
   my ($self)=@_;
   if (defined($self->embedded)) {
      (TempWrapperFor => $self->embedded, @{$self->headers})
   } else {
      (@{$self->is_instance_of->include}, @{$self->headers})
   }
}

my $skip_core_functions_re=qr{^\(eval\s+\d+\)|^\Q${InstallTop}\E/(?:perllib/Polymake|scripts)/|/XML/}o;
my $called_from_app_tree=qr{^(?'top' .*? (?: /bundled/ (?'bundled' $id_re))?) /apps/$id_re/ (?'where' rules|perllib|scripts|src|testsuite)}ox;

sub analyze_provenience {
   my ($self)=@_;
   my $file;
   my $depth=1;
   my $is_saving;
   while (defined ($file=(caller(++$depth))[1])) {
      if ($file =~ m{Core/XMLfile\.pm$}) {
         $is_saving=1 if (caller($depth))[3] =~ /::save(?:_data)?$/;
      }
      if ($file !~ $skip_core_functions_re) {
         if ($file =~ $called_from_app_tree) {
            return if $+{top} eq $InstallTop ||
                      defined ($self->used_in_extension= $+{bundled} ? $Extension::registered_by_URI{"bundled:$+{bundled}"} : $Extension::registered_by_dir{$+{top}});
         }
         last;
      }
   }
   $self->is_private=!$is_saving;
}
#######################################################################################
package Polymake::Core::CPlusPlus::LackingRegular;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$is_instance_of' => '#1' ],
   '$func_ptr_type',
   [ '$embedded | source_file' => '#1 ->embedded' ],
   '$is_private',
);

sub new {
   my $self=&_new;
   my $func_descr=pop;
   my $func=$self->is_instance_of;
   $self->func_ptr_type = demangle($func_descr->func_ptr_type);
   $self->func_ptr_type =~ s/pm::type2type<\s* (?'ret' $balanced_re) \(\) \s* (?=\()(?'args' $confined_re) >/$+{ret}$+{args}/xo
      or $self->func_ptr_type =~ s/pm::type2type<\s* ($balanced_re) >/$1/xo;
   $self->func_ptr_type =~ s/(?<![\w:])pm::(perl::(?:Object(?:Type)?|OptionSet|Scalar|Array|Hash))(?![\w:])/$1/g;
   my $i=0;
   @{$func->all_args}=map {
      my $arg="arg$i";
      if (defined (my $type_descr=$root->typeids->{$_})) {
         my $proto=$type_descr->type_proto;
         if (!$proto->cppoptions->builtin) {
            my $const= !contains($self->is_instance_of->lvalue_args, $i) && "const ";
            $arg.=".get< perl::TryCanned< $const".$type_descr->get_cpp_representation($proto)." > >()";
         }
      }
      ++$i; $arg
   } @{$func_descr->arg_types};

   if ($func->flags & $func_is_void) {
      $func->macro_call="Void( ";
   }
   $func->seen_in=1;
   $self;
}

sub gen_source_code {
   my ($self, $debug_code)=@_;
   my $func=$self->is_instance_of;
   my $args=$func->all_args;
   ( "   FunctionWrapper4perl( ".$self->func_ptr_type." ) {\n",
     @$args>0 &&
     "      perl::Value " . join(", ", map { "arg$_(stack[$_])" } 0..$#$args) . ";\n",
     "      IndirectWrapperReturn" . $func->macro_call . join(", ", @$args) . " );\n",
     "   }\n",
     "   FunctionWrapperInstance4perl( ".$self->func_ptr_type." );\n\n",
   )
}

sub gen_temp_code { gen_source_code(shift, $debug) }
sub temp_include { (TempWrapperFor => $_[0]->embedded) }

sub include { () }
sub extension { $_[0]->is_instance_of->extension }
sub used_in_extension { undef }

#######################################################################################
package Polymake::Core::CPlusPlus::Constructor;

use Polymake::Struct (
   [ '@ISA' => 'AutoFunction' ],
   [ new => '' ],
   [ '$perl_name' => '"construct"' ],
   [ '$wrapper_name' => '"new"' ],
   [ '$macro_call' => '"New(T0, "' ],
   [ '$explicit_template_params' => '[ 0 ]' ],
);

sub deduce_extra_params {
   # the only template type argument is the type to be constructed,
   # already represented by a PropertyType object sitting at the front of the argument list
   $_[1]->[0]
}

sub source_file { $_[1]->[0]->name.".cc" }

#######################################################################################
package Polymake::Core::CPlusPlus::SpecialOperator;

use Polymake::Struct (
   [ '@ISA' => 'AutoFunction' ],
   [ new => '$' ],
   [ '$perl_name' => '"operator"' ],
   [ '$wrapper_name' => '#1' ],
   [ '$seen_in' => '1' ],
   [ '@all_args' => '(undef)' ],
   [ '@template_params' => '([1,0])' ],
   [ '$explicit_template_params' => '[ 0 ]' ],
);

*deduce_extra_params=\&Constructor::deduce_extra_params;
*source_file=\&Constructor::source_file;

sub gen_source_code { "" }

sub instance_macro { "OperatorInstance4perl" }

#######################################################################################
package Polymake::Core::CPlusPlus::IndirectWrapper;

use Polymake::Struct (
   [ '$wrapper_name' => '".wrp"' ],
   '%inst_cache',
   [ '$seen_in' => '1' ],
   [ '$template_params' => '0' ],
   [ '$via_object' => 'undef' ],
);

#######################################################################################
package Polymake::Core::CPlusPlus::Operator;

# the following must match the declarations in perl/wrappers.h

my @op_wrapper_class=qw( Unary_ Binary_ UnaryAssign_ BinaryAssign_ - Binary_ );

use Polymake::Struct (
   [ '@ISA' => 'AutoFunction' ],
   [ new => '$$$' ],
   [ '$seen_in' => '1' ],
   [ '$application' => 'undef' ],
   [ '$sign' => '#3' ],
   '%subs',
);

sub new {
   my $self=&_new;
   substr($self->wrapper_name,0,1)=$op_wrapper_class[$self->flags];
   if ($self->flags & 1) {
      # binary operation
      @{$self->template_params}=([0, $self->flags & ($func_is_lvalue|$func_is_lvalue_opt)], [1,0]);
      $#{$self->all_args}=1;
   } else {
      # unary operation
      @{$self->template_params}=([0, $self->flags & ($func_is_lvalue|$func_is_lvalue_opt)]);
      $#{$self->all_args}=0;
   }
   $self;
}

sub code {
   my ($self, $attrs)=@_;
   my $arg_flags=[$attrs & $func_is_wary, $attrs & $func_is_integral];
   my $code;
   if ($self->flags & 1) {
      if ($self->flags & $func_is_lvalue) {
         # binary operation with assignment, operands swapping can't occur
         $code=sub { pop; &{ resolve_auto_function($self, \@_, $arg_flags) } };
         declare_lvalue($code, 1);

      } elsif ($self->flags & $func_is_lvalue_opt) {
         # internal function mapped to an operator, no additional arguments
         $code=sub { &{ resolve_auto_function($self, \@_, $arg_flags) } };
         declare_lvalue($code, 1);

      } else {
         # binary operation, operands may be swapped
         my $arg_flags_swapped=[ reverse @$arg_flags ];
         $code=sub {
            if (pop) {
               local_scalar($self->flags, $func_is_void);
               swap_array_elems(\@_,0,1);
               &{ resolve_auto_function($self, \@_, $arg_flags_swapped) }
            } else {
               &{ resolve_auto_function($self, \@_, $arg_flags) }
            }
         };
      }
   } else {
      # unary operation
      $code=sub { splice @_,-2; &{ resolve_auto_function($self, \@_, $arg_flags) } };
      if ($self->flags & ($func_is_lvalue|$func_is_lvalue_opt)) {
         declare_lvalue($code, 1);
      }
   }
   $code;
}

sub gen_source_code { "" }

sub source_file {
   my ($self, $args)=@_;
   $args->[ ($self->flags & $func_is_void) ? 1 : 0 ]->type->name . ".cc";
}

sub complain_ro_violation {
   my ($self)=@_;
   croak("Attempt to modify a read-only C++ object passed to operator ", $self->sign);
}

sub instance_macro { "OperatorInstance4perl" }

#######################################################################################
package Polymake::Core::CPlusPlus::TypeDescr;

use Polymake::Struct (
   '$pkg',
   '$vtbl | source_file',          # source_file is filled by duplicates only
   '$typeid',
   '$kind',                        # Enum class_is_*
   [ '$generated_by' => 'undef' ], # FuncDescr of a function created such an object if it does not correspond to any declared property type
                                   #     or typeid of a container/composite which this class belongs as element to
   [ '$source_name' => 'undef' ],
   [ '$include' => 'undef' ],
   [ '$embedded' => 'undef' ],
   [ '$application' => 'undef' ],
   [ '$cross_apps' => 'undef' ],
   [ '$extension' => 'undef' ],
);

sub source_line { $_[0]->source_file+0 }

sub type_proto { &{$_[0]->pkg->{".type"}}() }

sub get_cpp_representation {
   my ($self, $proto)=@_;
   $self->source_name //= defined($self->generated_by)
                          ? demangle($self->typeid)
                          : (($proto //= &type_proto)->cppoptions->finalize->name //= demangle($self->typeid));

   if (wantarray) {
      $self->include //= do {
         if (defined (my $gen=$self->generated_by)) {
            my $parent;
            unless (ref($gen)) {
               do {
                  $parent=$root->typeids->{$gen} ||
                     croak( ($parent || $self)->source_name, " pretends to be a dependent type of an unregistered C++ class ", demangle($gen) );
                  $gen=$parent->generated_by;
               } while (defined($gen) && !ref($gen));
            }
            if (ref($gen)) {
               if (defined $gen->arg_types) {
                  # it is a descriptor of an auto-function
                  my $auto_func=$gen->auto_func;
                  my @includes=@{$auto_func->include};
                  $self->embedded=$auto_func->embedded;
                  $self->application=$auto_func->application;
                  $self->cross_apps=$gen->cross_apps;
                  $self->extension=$auto_func->extension;
                  foreach my $arg_type (@{$gen->arg_types}) {
                     if (defined (my $arg_type_descr=$root->typeids->{$arg_type})) {
                        (undef, my ($includes, $embedded, $app, $cross_apps, $extension))=$arg_type_descr->get_cpp_representation;
                        push @includes, @$includes;
                        set_application($self, $embedded, $app, $cross_apps);
                        set_extension($self->extension, $extension);
                     }
                  }
                  \@includes
               } else {
                  # it is an embedded property type
                  $self->embedded=$gen->source_file;
                  $self->extension=$proto->extension;
                  $self->application=$proto->application;
                  [ ]
               }
            } else {
               # it is a typeid of a container class
               (undef, my $includes, $self->embedded, $self->application, $self->cross_apps, $self->extension)=$parent->get_cpp_representation;
               $includes
            }
         } else {
            my $opts=($proto //= &type_proto)->cppoptions->finalize;
            $self->embedded=$opts->embedded;
            $self->application=$opts->application;
            $self->cross_apps=$opts->cross_apps;
            $self->extension=$opts->extension;
            $opts->include
         }
      };
      ($self->source_name, $self->include, $self->embedded, $self->application, $self->cross_apps, $self->extension);
   } else {
      $self->source_name;
   }
}

sub complain_source_conflict {
   "Don't know where to place the instantiation of ", $_[0]->source_name
}

# Sometimes a TypeDescr object is being passed instead of PropertyType 0-th argument to a converting constructor.
# The following methods make it compatible to PropertyType to the extent sufficient for
# resolve_auto_function and new LackingInstance.

sub cpp_type_descr { shift }
sub cpp_persistent_type_descr { shift }
sub name { &{(shift)->pkg->{type}}->name }

#######################################################################################
package Polymake::Core::CPlusPlus::Options;

use Polymake::Struct (
   [ new => '$$%' ],
   [ '$name' => '#%' ],
   [ '@include' => '#%' ],
   [ '$builtin' => '#%', default => 'undef' ],
   [ '$special' => '#%', default => 'undef' ],
   [ '$default_constructor' => '#%', default => '"StdConstr"' ],
   [ '$template_params' => '#%' ],
   [ '$operators' => '#%' ],
   [ '$fields' => '#%', default => 'undef' ],
   [ '$finalize_with' => 'undef' ],
   [ '$descr' => '#%', default => 'undef' ],     # cached for builtins and persistent types
   [ '$embedded' => 'undef' ],
   [ '$application' => '#1' ],
   [ '$cross_apps' => 'undef' ],
   [ '$extension' => '#2' ],
   [ '$used_in_extension' => 'undef' ],
   '$is_private',
);

sub finalize {
   my ($self)=@_;
   if (defined (my $proto=$self->finalize_with)) {
      undef $self->finalize_with;
      $self->extension //= $self->application->origin_extension;
      my @incs=@{$self->include};
      my @t_params;

      foreach (@{$proto->params}) {
         unless ($_->cppoptions) {
            die "Can't create C++ binding for ", $proto->full_name, ": non-C++ parameter ", $_->name, "\n";
         }
         my $p_opts=$_->cppoptions->finalize;
         push @t_params, $p_opts->name;
         push @incs, @{$p_opts->include};
         set_application($self, $p_opts->embedded, $p_opts->application, $p_opts->cross_apps);
         set_extension($self->extension, $p_opts->extension);
      }

      if (is_code($self->name)) {
         $self->name= is_method($self->name) ? $self->name->($proto,@t_params) : $self->name->(@t_params);
      } elsif ($self->template_params eq "*") {
         $self->name="";
      } elsif ($self->name =~ /%\d/) {
         unshift @t_params, undef;      # shift the indexing
         $self->name =~ s/%(\d+)/ $t_params[$1] /g;
      } else {
         $self->name.="< " . join(", ", @t_params) . " >";
      }
      if (@incs>@{$self->include}) {
         $self->include=[ uniq(@incs) ];
      }
      remove_origin_extension($self);
   }
   $self;
}

sub clone {
   my ($self, $proto)=@_;
   my $clone=inherit_class([ @$self ], $self);
   weak($clone->finalize_with=$proto);
   $clone
}

sub complain_source_conflict {
   "Can't create C++ binding for ", $_[0]->name;
}

sub temp_include {
   my ($self)=@_;
   if (defined($self->embedded)) {
      (TempWrapperFor => $self->embedded, @{$self->include})
   } else {
      @{$self->include}
   }
}

sub namespace_prefix {
   my ($self, $app_name)=@_;
   $self->include->[-1] =~ m{^polymake/$app_name/} and "polymake::$app_name\::"
}

sub recognizing_template {
   my ($self, $app, $pkg)=@_;
   my $typenames=join(", ", map { "typename T$_" } 0..$self->template_params-1);
   my $Ts=join(",", map { "T$_" } 0..$self->template_params-1);
   my $declared=$self->name;
   if ($declared =~ /%\d/) {
      $declared =~ s/%(\d+)/"T".($1-1)/ge;
   } else {
      $declared .= "<$Ts>";
   }
   my $qual= $self->name !~ /::/ && namespace_prefix($self, $app->name);
   return <<".";
   template <typename T, $typenames>
   RecognizeType4perl("$pkg", ($Ts), $qual$declared)

.
}

sub recognizing_class {
   my ($self, $app, $pkg)=@_;
   my $declared= $self->name !~ /::/ && namespace_prefix($self, $app->name) . $self->name;
   return <<".";
   template <typename T>
   RecognizeType4perl("$pkg", (), $declared)

.
}

sub register_template {
   my ($self, $pkg)=@_;
   return <<".";
   ClassTemplate4perl("$pkg");
.
}

sub gen_source_code {
   my ($self, $proto)=@_;
   my $pkg=$proto->pkg;
   "   ".($self->builtin || $self->special ? "Builtin" : "Class")."4perl(\"$pkg\", ".$self->name.");\n"
}
*gen_temp_code=\&gen_source_code;

sub analyze_provenience {
   my ($self)=@_;
   my $depth=1;
   while ((my ($file, $sub)=(caller(++$depth))[1,3])) {
      if ($sub eq "(eval)") {
         # clients may trigger creation of new types too
         if ($file =~ m{/perllib/Polymake/Core/CPlusPlus\.pm}) {
            if (ref(my $descr=get_cur_func_descr())) {
               $file=$descr->source_file;
            } else {
               if (defined($descr) && defined ($descr=$root->typeids->{$descr})) {
                  set_extension($self->extension, $descr->extension);
               }
               next;
            }
         } else {
            next;
         }
      } elsif ($file =~ $skip_core_functions_re) {
         next;
      }
      if ($file =~ $called_from_app_tree) {
         my ($top, $bundled, $where)=@+{qw(top bundled where)};
         next if $where eq "rules" && $sub =~ /::add_template_instance$/;
         return if $top eq $InstallTop ||
                   defined ($self->used_in_extension= $+{bundled} ? $Extension::registered_by_URI{"bundled:$+{bundled}"} : $Extension::registered_by_dir{$+{top}});
      }
      last;
   }
   $self->is_private=1;
}
#######################################################################################
# bind all Object types involved in template arg deduction to the C++ Object class

package Polymake::Core::CPlusPlus::ObjectOptions;
use Polymake::Struct (
   [ '@ISA' => 'Options' ],
);

sub finalize {
   croak("Can't embed a `big' Object into a C++ data structure: use SCALAR instead");
}

sub descr : lvalue {
   croak("Can't pass a `big' Object to a C++ function expecting a native C++ data type");
   $debug
}

package Polymake::Core::CPlusPlus::OptionsForArray;
use Polymake::Struct (
   [ '@ISA' => 'Options' ],
);

sub clone {
   my ($self, $proto)=@_;
   if ($proto->params->[0]->cppoptions == $BigObject_cpp_options) {
      # intercept BigObjectArray
      $BigObjectArray_cpp_options;
   } else {
      &Options::clone;
   }
}
#######################################################################################
package Polymake::Core::CPlusPlus::EmbeddedRules;

use Polymake::Struct (
   [ new => '@' ],
   '$lastfile',
   [ '$preproc' => '0' ],
   [ '@lines' => '@' ],
);

*TIEHANDLE=\&new;

sub READLINE {
   my ($self)=@_;
   if ($self->preproc) {
      --$self->preproc;
      shift @{$self->lines};

   } elsif (!@{$self->lines}) {
      if ($self->lastfile) {
         # close the last source file
         undef $self->lastfile;
         "}\n"
      } else {
         undef
      }

   } elsif (my ($linecmd, $line, $file)= $self->lines->[0] =~ /^(\#line\s+(\d+))\s+"(.*)"/) {
      $.=$line-1;
      my $result;
      if ($file eq $self->lastfile) {
         # perl seems confused when it encounters the same source file in the consecutive lines
         shift @{$self->lines};
         $result="$linecmd\n";
      } else {
         if ($self->lastfile !~ /\.hh?$/) {
            $self->preproc += 1;
            if ($self->lastfile) {
               # previous source file finished
               $result="} {\n";
            } else {
               # first source file started
               $result="{\n";
            }
         } else {
            $result=shift @{$self->lines};
         }
         $self->lastfile=$file;
      }
      $result

   } else {
      ++$.;
      $self->lines->[0] =~ s{^.*\n}{}m;
      if (!length($self->lines->[0])) { shift @{$self->lines}; }
      $&;
   }
}

sub CLOSE { !@{$_[0]->lines} }

#######################################################################################
package Polymake::Core::CPlusPlus::SharedModule;

use Polymake::Struct (
   [ new => '$;$' ],
   [ '$build_dir' => 'undef' ],         # directory where to execute `make'
   '$so_name',
   [ '$so_timestamp' => 'undef' ],
   '$is_mutable',                       # boolean: source code for new wrappers can be added directly
);

sub new {
   my $self=&_new;
   my ($perApp, $extension)=@_;
   my $app=$perApp->application;
   my $app_name=$app->name;
   my $build_top;
   if (defined($extension) || $app->installTop ne $InstallTop) {
      # either an extension of a known application, or an application defined in an extension
      if (defined($extension) && $extension->is_bundled) {
         ($build_top=$extension->URI) =~ tr|:|/|;
         substr($build_top,0,0) .= "$InstallArch/";
         $self->is_mutable=$DeveloperMode;
      } elsif (($build_top= defined($extension) ? $extension->dir : $app->installTop) =~ s{^\Q${InstallTop}\E(?=/ext/)}{$InstallArch}o) {
         # An installed extension.
         $self->is_mutable=0;
      } else {
         # Uninstalled private extension
         $build_top.="/build.$Arch";
         $self->is_mutable=1;
      }
   } else {
      # core application
      $build_top=$InstallArch;
      $self->is_mutable= $build_top =~ m{/build.$Arch}o;
   }
   $self->build_dir="$build_top/apps/$app_name";

   my $modsize;
   if (-e ($self->so_name="$build_top/lib/$app_name$dl_suffix")) {

      # should the application clients be recompiled?
      if (# they belong to the user
          $self->is_mutable &&= -w _
            and
          # new wrappers have been generated
          ($self->so_timestamp=(stat _)[9]) < $wrapper_updated{defined($extension) ? $extension->dir."/$app_name" : $app_name}
            ||
          # a core application or one of the prerequisite extensions has been renewed (e.g. installed a new version)
          (defined($extension) && seems_out_of_date($self->so_timestamp, $extension, $app, $perApp))) {
         $modsize=compile($self, $app, $extension);
      } else {
         $modsize=(stat _)[7];
      }

   } elsif ($self->is_mutable) {
      # this application/extension has not yet been built for the current architecture
      require Polymake::Configure;

      my $ext_dir;
      if (defined($extension)) {
         $ext_dir=$extension->dir;
         if ($extension->is_bundled) {
            $ext_dir =~ s{^\Q${InstallTop}\E}{\${ProjectTop}}o;
         }
      }
      Configure::create_build_dir($self->build_dir,
                                  (defined($extension) ? !$extension->is_bundled : $app->installTop ne $InstallTop) && $InstallTop,   # ProjectTop
                                  $ext_dir);
      $modsize=compile($self, $app, $extension);

   } else {
      die "Corrupt or incomplete installation: shared module ", $self->so_name, " missing\n";
   }

   $perApp->functions_begin=@{$root->functions};
   $perApp->embedded_rules_begin=@{$root->embedded_rules};
   $perApp->duplicate_class_instances_begin=@{$root->duplicate_class_instances};
   # empty shared module is created when the application does not contain any client code
   if ($modsize) {
      $self->so_timestamp=(stat _)[9];
      load_shared_module($self->so_name);
   }
   $perApp->functions_end=$#{$root->functions};
   $perApp->embedded_rules_cnt=@{$root->embedded_rules}-$perApp->embedded_rules_begin;
   $perApp->duplicate_class_instances_end=$#{$root->duplicate_class_instances};

   $self
}

sub seems_out_of_date {
   my ($timestamp, $extension, $app, $perApp)=@_;

   # We check for is_mutable in order to avoid endless and fruitless recompilations in developer mode,
   # where the core application may often change independently of the extensions.
   # !is_mutable normally means that the core application or a prerequisite extension is taken from a write-protected installed location.
   # When the installed shared module has in fact changed since the last compilation of the own extension, then it is quite certain
   # that also the entire installed source code has been updated, thus the recompilation of the own extension won't be in vain.
   # However, the private wrappers should be recompiled even in developer mode, because the core library headers can be changed with every sync.

   my $mod=$perApp->shared_modules->{$app->installTop};
   my $out_of_date= defined($mod) && $timestamp < $mod->so_timestamp && ($extension==$private_wrapper_ext || !$mod->is_mutable);
   unless ($out_of_date) {
      foreach my $prereq (@{$extension->requires}) {
         if (defined($mod=$perApp->shared_modules->{$prereq->dir}) && $timestamp < $mod->so_timestamp && ($extension==$private_wrapper_ext || !$mod->is_mutable)) {
            $out_of_date=1;
            last;
         }
      }
   }
   # @todo perform the reconfiguration?
   # if ($out_of_date) {
   # }
   $out_of_date
}

sub compile {
   my ($self, $app, $ext)=@_;
   unless ($MAKE) {
      require Polymake::Core::CPlusPlus_config;
      configure_make($custom_handler);
   }
   my $errfile=new Tempfile();
   my $debug_flag= $debug && "Debug=y";
   warn_print( "Recompiling application ", $app->name, $ext && " in extension ".$ext->dir, ", please be patient..." );
   system("$MAKE -C " . $self->build_dir . " all $debug_flag $MAKEFLAGS" . (!$Verbose::cpp && ">/dev/null") . " 2>$errfile.err")
     and
   die "shared module compilation failed; see the error log below\n\n", `cat $errfile.err`;

   my $old_timestamp=$self->so_timestamp;
   ($self->so_timestamp, my $size)=(stat $self->so_name)[9,7];
   if ($old_timestamp==$self->so_timestamp) {
      # touch the unchanged shared object to avoid further unnecessary make calls
      utime(undef, undef, $self->so_name);
   }
   $size
}

sub build_top {
   my $path=(shift)->so_name;
   $path =~ s{/lib/[^/]+$}{};
   $path
}
#######################################################################################
package Polymake::Core::CPlusPlus::perApplication;

use Polymake::Struct (
   [ 'new' => '$' ],
   [ '$application' => 'weak( #1 )' ],
   [ '$functions_begin' => '0' ],
   [ '$functions_end' => '-1' ],
   '$embedded_rules_begin',
   [ '$embedded_rules_cnt' => '-1' ],
   '@embedded_rules',           # transformed source lines whilst loading rulefiles
   '%lacking_types',            # PropertyType => 1 for types with declared C++ binding but lacking generated wrappers
   '%lacking_templates',
   '@lacking_auto_functions',
   '@obsolete_auto_functions',
   '@duplicate_function_instances',
   [ '$duplicate_class_instances_begin' => '0' ],
   [ '$duplicate_class_instances_end' => '-1' ],
   '@duplicate_class_instances',
   '$will_update_sources',
   '%shared_modules',           # "top_dir" => SharedModule
);
#######################################################################################
sub start_loading {
   my ($self, $extension)=@_;
   if (defined (my $shared_mod=new SharedModule($self, $extension))) {
      pick_embedded_rules($self, $shared_mod);
      $self->shared_modules->{$extension ? $extension->dir : $self->application->installTop}=$shared_mod;
   }
}
#######################################################################################
sub pick_embedded_rules {
   my ($self, $shared_mod)=@_;
   if ($self->embedded_rules_cnt > 0) {
      do "c++:1:".$shared_mod->so_name;
      if ($@) {
         $#{$self->embedded_rules}=-1;
         $self->embedded_rules_cnt=-1;
         die "Error in rules embedded in a C++ client:\n$@";
      }
   }
}
#######################################################################################
sub end_loading {
   my ($self, $extension)=@_;
   if (@{$self->embedded_rules}) {
      if ($Verbose::rules>1) {
         dbg_print( "reading rules embedded in C++ clients from ",
                    $self->shared_modules->{$extension ? $extension->dir : $self->application->installTop}->so_name );
      }
      $self->embedded_rules_cnt=1;   # flag for add_auto_function
      do "c++:2:";
      $self->embedded_rules_cnt=-1;
      if ($@) {
         $#{$self->embedded_rules}=-1;
         die "Error in rules embedded in a C++ client:\n$@";
      }
   }
   bind_functions($self, $extension, $root->functions, $self->functions_begin, $self->functions_end);
   $self->functions_begin=0;
   $self->functions_end=-1;
   $self->duplicate_class_instances_begin=0;
   $self->duplicate_class_instances_end=-1;
}
#######################################################################################
sub load_suspended {
   my ($self, $suspended)=@_;
   if (@{$suspended->embedded_rules}) {
      if ($Verbose::rules>1) {
         dbg_print( "reading cross-application rules embedded in C++ clients from ",
                    $self->shared_modules->{$suspended->extension ? $suspended->extension->dir : $self->application->installTop}->so_name );
      }
      $self->embedded_rules=$suspended->embedded_rules;
      $self->embedded_rules_cnt=1;   # flag for add_auto_function
      do "c++:3:";
      $self->embedded_rules_cnt=-1;
      if ($@) {
         $#{$self->embedded_rules}=-1;
         die "Error in rules embedded in a C++ client:\n$@";
      }
   }
   bind_functions($self, $suspended->extension, $suspended->functions);
}
#######################################################################################
sub load_private_wrapper {
   my ($self)=@_;
   if (defined (my $shared_mod=$private_wrapper_ext && new SharedModule($self, $private_wrapper_ext))) {
      $self->shared_modules->{$private_wrapper_ext->dir}=$shared_mod;
      local $Application::extension=$private_wrapper_ext;
      $self->end_loading;
   }
}
#######################################################################################
# pseudo-file handle passed to RuleFilter transforming embedded rulefiles
sub embedded_rules_handle {
   my ($self)=@_;
   my $lines=$self->embedded_rules_cnt;
   $self->embedded_rules_cnt=0;
   my $handle=Symbol::gensym;
   select(select $handle);
   tie *$handle, "Polymake::Core::CPlusPlus::EmbeddedRules", (splice @{$root->embedded_rules}, $self->embedded_rules_begin, $lines);
   $handle
}

# INC subroutine retrieving transformed embedded rulefile lines
sub get_transformed_embedded {
   my ($maxlen, $self)=@_;
   print STDERR "+>> ", $self->embedded_rules->[0] if $DebugLevel>3;
   $_ .= shift @{$self->embedded_rules};
   return length;
}
#######################################################################################
my %builtin2proxy=( int => 'NumProxy', double => 'NumProxy', 'std::string' => 'StringProxy', bool => 'BoolProxy' );

sub add_type {
   my ($self, $proto)=splice @_, 0, 2, $_[0]->application, $Application::extension;
   my $opts=$proto->cppoptions=new Options(@_);
   if ($opts->special) {
      $opts->name ||= $opts->special || $proto->name;
      provide_cpp_type_descr($proto);

   } elsif ($opts->builtin) {
      if (ref($opts->builtin)) {
         $opts->name ||= $proto->name;
         if (ref($opts->builtin) eq "Polymake::Enum") {
            while (my ($key, $val)=each %{$opts->builtin}) {
               my $ref=readonly_deep(bless \$val, $proto->pkg);
               namespaces::export_sub($proto->application->pkg, define_function($proto->application->pkg, $key, sub { $ref }));
            }
         }
      } else {
         $opts->name ||= $opts->builtin;
         $root->builtins->{$opts->builtin}=$proto;
         if (defined (my $proxy_class=$builtin2proxy{$opts->builtin})) {
            no strict 'refs';
            push @{$proto->pkg."::ISA"}, "Polymake::Core::CPlusPlus::$proxy_class";
            if ($opts->builtin eq "int") {
               Overload::set_integer_type($proto);
            } elsif ($opts->builtin eq "double") {
               Overload::set_float_type($proto);
            } elsif ($opts->builtin eq "std::string") {
               Overload::set_string_type($proto);
            }
         }
      }
      provide_cpp_type_descr($proto);

   } else {
      if (!is_code($opts->name)) {
         if ($opts->name =~ /^(?: [\w:]+:: )?$/x) {
            $opts->name .= $proto->name;
         }
      }
      create_methods($self, $proto);
   }
}
#######################################################################################
sub add_type_template {
   my ($self, $generic_proto)=splice @_, 0, 2, $_[0]->application, $Application::extension;
   my $opts=new Options(@_);
   my $super_pkg=$generic_proto->pkg;
   if (!is_code($opts->name)) {
      if ($opts->name =~ /^(?: [\w:]+:: )?$/x) {
         $opts->name .= ($super_pkg =~ /([^:]+)$/)[0];
      }
   }
   if ($opts->template_params eq "*") {
      $opts->default_constructor="";
   } elsif (!exists $root->templates->{$super_pkg}) {
      $self->lacking_templates->{$super_pkg}=$opts;
      ensure_update_sources($self);
   }
   if ($opts->builtin) {
      croak( "parameterized type can't be declared as a C++ built-in type" );
   } else {
      if ($super_pkg =~ /::(array|map|pair)$/i) {
         $root->builtins->{lc($1)} ||= $super_pkg;
         if (lc($1) eq "array") {
            bless $opts, "Polymake::Core::CPlusPlus::OptionsForArray";
         }
      }
      $generic_proto->cppoptions=$opts;
      define_constructors($generic_proto);
      define_operators($self, $opts, $super_pkg);
   }
   $generic_proto;
}
#######################################################################################
sub add_template_instance {
   my ($self, $proto, $generic_proto, $defer)=@_;
   $proto->cppoptions=$generic_proto->cppoptions->clone($proto);
   if (!$proto->abstract && !$proto->cppoptions->builtin && $proto->cppoptions->template_params ne "*") {
      if ($defer && !exists $root->classes->{$proto->pkg}) {
         $proto->cppoptions->descr=sub { create_methods($self, $proto, $generic_proto->cppoptions) };
         $proto->construct=sub : method {
            my $proto=shift;  $proto->cppoptions->descr->();
            $proto->construct=\&PropertyType::construct_object;
            $proto->construct->(@_);
         };
      } else {
         create_methods($self, $proto, $generic_proto->cppoptions);
      }
   }
   $proto
}
#######################################################################################
sub add_auto_function {
   my ($self, $name, $ext_code, $arg_types, $arg_attrs, $func_attrs, $options)=@_;
   my ($pkg, $srcfile)=caller;
   my $application=$self->application;
   my $returns;
   my $flags=0;
   if (delete $func_attrs->{method}) {
      if (exists $options->{explicit_template_params}) {
         croak( "Methods with explicit template parameters are not supported yet" );
      }
      $flags |= $func_is_method;
   }
   if (delete $func_attrs->{void}) {
      $flags |= $func_is_void;
   }
   if (delete $func_attrs->{lvalue}) {
      if ($flags & $func_is_void) {
         croak( "Attributes 'lvalue' and 'void' are mutually exclusive" );
      }
      $flags |= $func_is_lvalue;
      if ($flags & $func_is_method) {
         $flags |= $func_has_anchor;
      }
   }
   if (delete $func_attrs->{lvalue_opt}) {
      if (!($flags & $func_is_method)) {
         croak( "Attribute 'lvalue_opt' is only applicable to methods" );
      }
      if ($flags & ($func_is_void | $func_is_lvalue)) {
         croak( "Attributes 'lvalue_opt', 'lvalue', and 'void' are mutually exclusive" );
      }
      $flags |= $func_is_lvalue_opt | $func_has_anchor;
   }
   if (delete $func_attrs->{non_const}) {
      if (!($flags & $func_is_method)) {
         croak( "Attribute 'non_const' is only applicable to methods" );
      }
      if ($flags & ($func_is_lvalue | $func_is_lvalue_opt)) {
         croak( "Attributes 'non_const', 'lvalue', and 'lvalue_opt' are mutually exclusive" );
      }
      $flags |= $func_is_non_const;
   }
   if (delete $func_attrs->{anchor}) {
      if (!($flags & $func_is_method)) {
         croak( "Attribute 'anchor' is only applicable to methods" );
      }
      if ($flags & ($func_is_lvalue | $func_is_lvalue_opt)) {
         croak( "Attribute 'anchor' is implied by 'lvalue' and 'lvalue_opt'" );
      }
      $flags |= $func_has_anchor;
   }
   if (delete $func_attrs->{wary}) {
      if (!($flags & $func_is_method)) {
         croak( "Attribute 'wary' is only applicable to methods" );
      }
      $flags |= $func_is_wary;
   }
   if (delete $func_attrs->{static}) {
      if (($flags & ($func_is_method | $func_is_non_const | $func_is_lvalue | $func_is_lvalue_opt | $func_is_wary | $func_has_anchor)) != $func_is_method) {
         if ($flags & $func_is_method) {
            croak( "Attribute 'static' is mutually exclusive with 'non_const', 'lvalue', 'wary', and 'anchor'" );
         } else {
            croak( "Attribute 'static' is only applicable to methods" );
         }
      }
      $flags |= $func_is_static;
   }
   if (defined ($returns=delete $func_attrs->{returns})) {
      if ($flags & $func_is_void) {
         croak( "Attributes 'returns' and 'void' are mutually exclusive" );
      }
      if ($returns eq '@' || $returns eq 'list') {
         $options->{macro_call}="List( ";
         undef $returns;
      } else {
         $options->{macro_call}="Pkg( ";
         $flags |= $func_has_prescribed_ret_type;
      }
   }
   my $const_creation=delete $func_attrs->{const_creation};
   if (delete $func_attrs->{builtin_sub}) {
      $options->{builtin_code}=$ext_code
         or croak("builtin_sub attribute without corresponding perl code");
      undef $ext_code;
   }
   if (keys %$func_attrs) {
      croak("Unknown attribute", keys(%$func_attrs)>1 && "s", " for a C++ function: ", join(", ", keys %$func_attrs));
   }

   my $auto_func= ($flags & $func_is_method and $name eq "construct")
                  ? new Constructor()
                  : new AutoFunction($name, $flags, $application, $options);
   my $code;
   if (defined $ext_code) {
      namespaces::fall_off_to_nextstate($ext_code);
   }

   if (defined($auto_func->embedded)) {
      $auto_func->name ||= $name;
      if (defined $arg_types) {
         $auto_func->flags |= $func_is_ellipsis if $arg_types->[1] & $Overload::has_trailing_list;
      }
      $auto_func->template_params=0;
      my $descr=$root->regular_functions->[$auto_func->embedded];
      $auto_func->embedded=$srcfile;
      $descr->auto_func=$auto_func;
      undef $auto_func->inst_cache;
      $code= defined($ext_code)
             ? (defined($returns)
                ? sub { return &$ext_code;
                        unshift @_, $returns;
                        &{resolve_regular_function($auto_func, $descr, \@_) }
                      }
                : sub { return &$ext_code;
                        &{resolve_regular_function($auto_func, $descr, \@_)}
                      })
             : (defined($returns)
                ? sub { unshift @_, $returns;
                        &{resolve_regular_function($auto_func, $descr, \@_) }
                      }
                : sub { &{resolve_regular_function($auto_func, $descr, \@_)} });

   } else {
      croak( "C++ function template without signature" ) unless defined $arg_types;
      if ($self->embedded_rules_cnt > 0) {
         $auto_func->embedded=$srcfile;
      }
      $auto_func->prepare($arg_types, $arg_attrs, $pkg);

      if (($flags & ($func_is_method | $func_is_static)) != $func_is_method && $pkg ne $application->pkg) {
         my $app=$application->pkg;
         (my $pkg_prefix=$pkg)=~s/^$app\:://;
         $pkg_prefix=~tr/:/_/;
         substr($auto_func->wrapper_name,0,0)=$pkg_prefix."__";
      }
      check_twins($auto_func);

      if ($flags & $func_is_method and $name eq 'operator') {
         my $obj;
         my $closure= defined($ext_code)
                      ? (defined($returns)
                         ? sub { return &$ext_code;
                                 unshift @_, $returns, $obj;
                                 &{resolve_auto_function($auto_func, \@_)}
                               }
                         : sub { return &$ext_code;
                                 unshift @_, $obj;
                                 &{resolve_auto_function($auto_func, \@_)}
                               })
                      : (defined($returns)
                         ? sub { unshift @_, $returns, $obj;
                                 &{resolve_auto_function($auto_func, \@_)}
                               }
                         : sub { unshift @_, $obj;
                                 &{resolve_auto_function($auto_func, \@_)}
                               });
         $code=sub { $obj=shift; $closure };
         declare_lvalue($closure, 1) if $flags & ($func_is_lvalue|$func_is_lvalue_opt);
      } else {
         $code= defined($ext_code)
                ? (defined($returns)
                   ? sub { return &$ext_code;
                           unshift @_, $returns;
                           &{resolve_auto_function($auto_func, \@_)}
                         }
                   : sub { return &$ext_code;
                           &{resolve_auto_function($auto_func, \@_)}
                         })
                : (defined($returns)
                         ? sub { unshift @_, $returns;
                                 &{resolve_auto_function($auto_func, \@_)}
                               }
                         : sub { &{resolve_auto_function($auto_func, \@_)} });
      }
      set_method($code) if $flags & $func_is_method;
      declare_lvalue($code, 1) if $auto_func->flags & ($func_is_lvalue|$func_is_lvalue_opt);
   }

   if (defined $const_creation) {
      namespaces::intercept_const_creation($application->pkg, $const_creation, $code, $name eq "construct" ? ($pkg->type) : ());
   }

   if (defined $arg_types) {
      if ($name eq "operator") {
         overload::OVERLOAD($pkg, '&{}' => $code);
         ()
      } else {
         ( $name, $code, $arg_types )
      }
   } else {
      define_function($pkg, $name, $code);
      ()
   }
}
#######################################################################################
# private:
sub bind_functions {
   my ($self, $extension, $func_list, $begin, $end, $subst_for_temp_src)=@_;

   foreach my $descr (defined($begin) ? @{$func_list}[$begin..$end] : @$func_list) {
      next if defined($descr->cross_apps) && $descr->suspend($self, $extension);

      my $auto_func=$root->auto_functions->{$descr->name};
      my ($src_file, $bunch);
      if (defined $subst_for_temp_src) {
         $src_file=$subst_for_temp_src->{readwrite($descr->source_file)};
         undef $descr->source_file;
         $descr->source_file=$src_file;
      } else {
         $src_file=$descr->source_file;
      }
      if (ref($auto_func) eq "HASH") {
         $bunch=$auto_func;
         $auto_func= $bunch->{auto_func_twin_key($src_file)} || $bunch->{$self->application->name};
      }

      unless (defined $auto_func) {
         if (defined $subst_for_temp_src) {
            croak( "temporary file ", keys(%$subst_for_temp_src), " contains definition of an unknown function" );
         }
         # even if the installed version looks suspicious, we are not entitled to fix it
         if (($extension // $self->application)->untrusted) {
            my $delete_func=(defined($bunch) ? $bunch->{auto_func_twin_key($src_file)} : $root->auto_functions->{$descr->name})=
              new AutoFunction($descr->name, 0, $self->application);
            $delete_func->template_params=-1;
            $delete_func->seen_in->{$descr->source_file}=1;
            push @{$self->obsolete_auto_functions}, $delete_func;
            ensure_update_sources($self);
            dbg_print( "C++ function ", $descr->name, " defined in ", $descr->source_file,
                       " has no binding declared in the rules, treating as obsolete" ) if $Verbose::cpp;
         }
         next;
      }

      if (ref($auto_func->template_params)) {
         $descr->auto_func=$auto_func;                  # help guessing the right set of headers for non-persistent return types

         my $sub=create_function_wrapper($descr, $self->application->pkg);
         $auto_func->prepare_wrapper_code($sub);

         my $last=$#{$descr->arg_types};
         my ($inst_cache, $extra);
         if ($last>=0) {
            $inst_cache=$auto_func->inst_cache;
            $extra=@{$auto_func->explicit_template_params};
            for (my $tp=0; $tp<$last; ++$tp) {
               if ($tp>=$extra  &&  $auto_func->template_params->[$tp-$extra]->[1] & $func_is_lvalue_opt) {
                  $inst_cache=($inst_cache->{$descr->arg_types->[$tp]}->[$descr->arg_types->[$tp]] ||= { });
               } else {
                  $inst_cache=($inst_cache->{$descr->arg_types->[$tp]} ||= { });
               }
            }
         }
         (( $last>=0
            ? $last>=$extra  &&  $last-$extra <= $#{$auto_func->template_params}  &&
              $auto_func->template_params->[$last-$extra]->[1] & $func_is_lvalue_opt
              ? $inst_cache->{$descr->arg_types->[$last]}->[$descr->arg_types->[$last]]
              : $inst_cache->{$descr->arg_types->[$last]}
            : $auto_func->inst_cache )
          &&= do {
                 unless (defined $subst_for_temp_src) {
                    push @{$self->duplicate_function_instances}, new DuplicateInstance($descr,$self->application);
                    $auto_func->seen_in->{$descr->source_file}+=0;   # pull into existence
                    ensure_update_sources($self);
                 }
                 next;
              }
         ) =$sub;
         if (!defined($subst_for_temp_src) && ref($auto_func->seen_in)) {
            $auto_func->seen_in->{$descr->source_file}++;
         }

      } elsif ($auto_func->template_params==0) {
         ($auto_func->inst_cache->{$descr->arg_types} &&= next) =$descr;        # repetition can occur when mixing several applications together
      } elsif (defined $subst_for_temp_src) {
         croak( "temporary file ", keys(%$subst_for_temp_src), " contains definition of an unknown function" );
      } elsif (!$auto_func->seen_in->{$descr->source_file}++) {
         dbg_print( "C++ function ", $descr->name, " defined in ", $descr->source_file,
                    " has no binding declared in the rules, treating as obsolete" ) if $Verbose::cpp;
      }
   }

   if (defined $subst_for_temp_src) {
      splice @{$root->duplicate_class_instances}, $self->duplicate_class_instances_begin;
      $self->duplicate_class_instances_end=$self->duplicate_class_instances_begin-1;

   } elsif ($self->duplicate_class_instances_end >= $self->duplicate_class_instances_begin) {
      push @{$self->duplicate_class_instances},
           map {
              $_->application=$self->application;
              $_->extension=$extension;
              $_
           } @{$root->duplicate_class_instances}[$self->duplicate_class_instances_begin .. $self->duplicate_class_instances_end];
      ensure_update_sources($self);
   }
}
#######################################################################################
sub ensure_update_sources {
   my ($self)=@_;
   if ($PrivateDir) {
      $self->will_update_sources ||= do {
         add AtEnd($self->application->name.":C++", sub { generate_sources($self) },
                   before=>[ "Customize", "Private:C++", map { "$_:C++" } keys(%{$self->application->used}) ], after => 'Object');
         1
      }
   }
}

sub generate_sources {
   my ($self)=@_;
   my %files;

   foreach my $proto (keys %{$self->lacking_types}) {
      my $opts=$proto->cppoptions;
      my $pkg= $proto->context_pkg || ($opts->builtin || $opts->special ? "builtins" : $proto->pkg);
      $pkg =~ /($id_re)$/o;
      my $ccfile=prepare_source_file($self, \%files, $opts, 0,  "$1.cc", "bindings.cc");
      push @{$ccfile->{includes}}, @{$opts->include};
      push @{$ccfile->{instances}}, $opts->gen_source_code($proto);
      if (defined($opts->extension) && defined (my $exts=$ccfile->{extensions})) {
         @{$exts}{ map { $_->URI } is_object($opts->extension) ? $opts->extension : @{$opts->extension} }=();
      }

      if (!$opts->template_params && $pkg ne "builtins" && !defined($opts->embedded)) {
         push @{ prepare_h_file($self, \%files, $opts)->{declarations} },
              $opts->recognizing_class($self->application, $proto->pkg);
      }
   }

   while (my ($pkg, $generic_opts)=each %{$self->lacking_templates}) {
      push @{ prepare_h_file($self, \%files, $generic_opts)->{declarations} },
           $generic_opts->recognizing_template($self->application, $pkg);
      $pkg =~ /($id_re)$/o;
      my $ccfile=prepare_source_file($self, \%files, $generic_opts, 0, "$1.cc", "bindings.cc");
      push @{$ccfile->{includes}}, @{$generic_opts->include};
      push @{$ccfile->{instances}}, $generic_opts->register_template($pkg);
   }

   foreach my $inst (@{$self->duplicate_class_instances}) {
      prepare_source_file($self, \%files, $inst, 1, $inst->source_file, "bindings.cc")->{dup_class}->{$inst->pkg}=$inst;
   }

   foreach my $inst (@{$self->lacking_auto_functions}) {
      my $ccfile=prepare_source_file($self, \%files, $inst, 0, $inst->source_file, "bindings.cc");
      push @{$ccfile->{includes}}, $inst->include;
      if (ref (my $seen_in=$inst->is_instance_of->seen_in)) {
         unless (exists $seen_in->{$ccfile->{filename}}) {
            push @{$ccfile->{declarations}}, $inst->is_instance_of->gen_source_code;
         }
         $seen_in->{$ccfile->{filename}}++;
      }
      push @{$ccfile->{instances}}, $inst->gen_source_code;
      if (defined($inst->extension) && defined (my $ext=$ccfile->{extensions})) {
         @{$ext}{ map { $_->URI } is_object($inst->extension) ? $inst->extension : @{$inst->extension} }=();
      }
   }

   foreach my $auto_func (@{$self->obsolete_auto_functions}) {
      foreach (keys %{$auto_func->seen_in}) {
         my $wrapper_name=$auto_func->wrapper_name;
         if ($wrapper_name =~ /^:/) {
            $wrapper_name="Binary_$'";
         } elsif ($wrapper_name =~ /^\./) {
            $wrapper_name="Unary_$'";
         } elsif ($wrapper_name =~ /^=/) {
            $wrapper_name="BinaryAssign_$'";
         }
         prepare_source_file($self, \%files, $auto_func, 1, $_, "bindings.cc")->{obsolete}->{$wrapper_name}=$auto_func;
      }
   }

   foreach my $inst (@{$self->duplicate_function_instances}) {
      my $ccfile=prepare_source_file($self, \%files, $inst, 1, $inst->descr->source_file, "bindings.cc");
      my $seen_in=$inst->is_instance_of->seen_in;
      if (ref($seen_in) && !$seen_in->{$ccfile->{filename}}) {
         $ccfile->{obsolete}->{$inst->is_instance_of->wrapper_name}=$inst->is_instance_of;
      } else {
         push @{$ccfile->{dup_func}->{$inst->is_instance_of->wrapper_name}}, $inst;
      }
   }

   %{$self->lacking_types}=();
   %{$self->lacking_templates}=();
   @{$self->lacking_auto_functions}=();
   @{$self->obsolete_auto_functions}=();
   @{$self->duplicate_function_instances}=();

   my $t=time+1;
   foreach (values %files) {
      modify_source_file($_);
      $wrapper_updated{$_->{wrapper_tag}}=$t;
   }
   $custom_handler->set('%wrapper_updated');
}
#######################################################################################
sub prepare_h_file {
   my ($self, $files, $opts)=@_;
   my $main_header=$opts->include->[-1];
   my ($guard_name, $wrapper_dir)=
      $DeveloperMode && $main_header =~ m{polymake/[^/]+$}
      ? ("CORE_WRAPPERS_", "$InstallTop/include/core-wrappers")
      : ("APP_WRAPPERS_",  ($opts->extension ? $opts->extension->dir : $self->application->installTop)."/include/app-wrappers");
   (my $short_name=$main_header) =~ s/\.h$//;
   $short_name =~ s{^polymake/}{};
   $short_name =~ s{[./: ]}{_}g;
   prepare_source_file($self, $files, $opts, -1, "$wrapper_dir/$main_header", "bindings.h",
                       guard_name=>$guard_name.$short_name, include_file=>$main_header);
}
#######################################################################################
sub prepare_source_file {
   my ($self, $files, $lacking, $true_filename, $file, @template)=@_;

   my $in_private=$true_filename>0
                  ? defined($private_wrapper_ext) &&
                    index($file, $private_wrapper_ext->app_dir($self->application))==0
                  : $lacking->is_private;

   my $src_top=$in_private
               ? $private_wrapper_ext->dir :
               defined($lacking->extension)
               ? $lacking->extension->dir
               : $self->application->installTop;

   my $shared_mod=$self->shared_modules->{$src_top}       # if the shared module has disappeared (e.g. after obliterate_extension),
      or $in_private or return { };                       # this source file won't be registered in %files and thus avoids modification.

   if (!$true_filename) {
      if (defined($lacking->embedded)) {
         $lacking->embedded =~ $filename_re;
         $file="wrap-$1";
      }
      substr($file,0,0) .= "$src_top/apps/" . $self->application->name . "/src/perl/";
      if ($in_private) {
         # update the list of envolved extensions
         my $ext=$private_wrapper_ext;
         set_extension($ext, $lacking->extension);
         if ($ext != $private_wrapper_ext) {
            # $ext->[0] == $private_wrapper_ext, cf. PropertyType::set_extension.
            push @{$private_wrapper_ext->requires}, splice(@$ext, 1);
            ensure_update_private_wrapper();
         }
      }
   }

   $files->{$file} ||= do {
      my $wrapper_tag=($src_top ne $self->application->installTop && "$src_top/").$self->application->name;
      my $vcs= $in_private ? $private_wrapper_ext->get_source_VCS : $lacking->extension ? $lacking->extension->get_source_VCS : $CoreVCS;
      if (-r $file && -w _) {
         if (defined($shared_mod) && defined($shared_mod->so_timestamp) && (stat _)[9] > $shared_mod->so_timestamp) {
            $wrapper_updated{$wrapper_tag}=time+1;
            $custom_handler->set('%wrapper_updated');
            my $so_name=$shared_mod->so_name;
            die <<".";
Automatical update of the C++ source file $file refused:
The shared module $so_name is out-of-date!
It will be recompiled at the very beginning of the next polymake session.
Alternatively, you may run `make' manually right now.
.
         }
         my $status=$vcs->check_status($file);
         if ($status eq "conflict") {
            die <<".";
The C++ source file $file is in conflicting state w.r.t. the checked-in version.
Please resolve the conflict and run `make' manually before you start polymake the next time.
.
         } elsif ($status eq "outdated") {
            die <<".";
The C++ source file $file is not up-to-date w.r.t. the repository.
Automatical update now would lead to future conflicts.
Please update your working copy and run `make' manually before you start polymake the next time.
.
         } elsif ($status) {
            warn_print( "Can't check the status of file $file: $status\n",
                        "Automatical updates are made to your own risk." );
         }

         dbg_print( "Updating C++ source file $file" ) if $Verbose::cpp;

      } elsif (-e _) {
         die <<"."
The C++ source file $file exists but can't be updated due to lacking permission.
Until this file is writable for you, you can't maintain persistent C++ bindings!
.
      } else {
         create_source_file($file, $vcs, @template, app_name=>$self->application->name);
      }

      { filename => $file, wrapper_tag => $wrapper_tag, vcs => $vcs,
        includes => [ $file =~ m{/src/perl/wrap-|\.h$} ? () : ("polymake/client.h") ],
        $in_private ? (extensions => { }) : (),
      }
   };
}
#######################################################################################
sub compile_load_temp_shared_module {
   my ($self, $lacking, $type_proto)=@_;
   local $_;

   my ($src_top, $shared_mod);
   my @includes=$lacking->temp_include;
   my $TempWrapperFor;
   if ($includes[0] eq "TempWrapperFor") {
      # The path to the source file harbouring the definition is firmly compiled in, thus referring to the original source tree.
      # Here we must guess where the source has eventually landed.
      ($TempWrapperFor, my $embedded)=splice @includes, 0, 2;
      (my $srcfile=$embedded) =~ s{^(.*?(/bundled/$id_re)?)/apps/$id_re/src/}{}o;
      my ($orig, $bundled)=($1, $2);
      my $qual_srcfile=substr($embedded, length($orig));
      if ($bundled && defined (my $ext=$Extension::registered_by_dir{$InstallTop.$bundled})) {
         # source file belongs to the bundled extension
         set_extension($lacking->extension, $ext);
         $src_top=$ext->dir;
      } elsif (-f $embedded && defined (my $ext=$Extension::registered_by_dir{$orig})) {
         # source file belongs to a private extension
         $src_top=$orig;
      } elsif (-f $self->application->installTop.$qual_srcfile) {
         # source file belongs to the application root
         $src_top=$self->application->installTop;
      } else {
         # source file belongs to an installed standalone extension
         foreach my $ext (@{$self->application->extensions}) {
            if (!$ext->is_bundled && -f $ext->dir.$qual_srcfile) {
               set_extension($lacking->extension, $ext);
               $src_top=$ext->dir;
               last;
            }
         }
      }
      $src_top || croak( "can't spot the current location of source file harbouring the definition of function ",
                         $lacking->is_instance_of->name, "; initially located at $embedded" );
      $TempWrapperFor.="=$srcfile";
   } else {
      unshift @includes, "polymake/client.h";
   }

   if (!$lacking->is_private && ref($lacking->extension) eq "ARRAY") {
      # If several independent extensions are mixtured, the wrapper must be banned into the private area.
      # But if exactly one of the extensions is stand-alone and writable, and all others are bundled,
      # we can remedy it by adding the dependencies between extensions.
      my @standalone=grep { !$_->is_bundled } @{$lacking->extension};
      if (@standalone==1) {
         my $ext=pop @standalone;
         $src_top=$ext->dir;
         $shared_mod=($self->shared_modules->{$src_top} ||= new SharedModule($self, $ext));
         unless ($lacking->is_private = !$shared_mod->is_mutable) {
            require Polymake::Core::InteractiveCommands;
            require Polymake::Configure;
            delete_from_list($lacking->extension, $ext);
            $ext->add_prerequisites(@{$lacking->extension});
            Configure::update_extension_build_dir($ext);
            $lacking->extension=$ext;
         }
      } else {
         $lacking->is_private=1;
      }
   }

   if (!$lacking->is_private) {
      $src_top //= defined($lacking->extension) ? $lacking->extension->dir : $self->application->installTop;
      $shared_mod=($self->shared_modules->{$src_top} ||= new SharedModule($self, $lacking->extension));
      if ($lacking->is_private = !$shared_mod->is_mutable) {
         # The lacking function could be persistently instantiated in the public shared module if it were allowed to extend;
         # maybe the extension using this function is mutable?
         if (defined($lacking->used_in_extension)) {
            my $preserve_ext=$lacking->extension;
            set_extension($lacking->extension, $lacking->used_in_extension);
            if ($lacking->extension == $lacking->used_in_extension) {
               # the extension which uses the function is indeed dependent on the defining extension (or it was just a core function)
               $src_top=$lacking->extension->dir;
               $shared_mod=($self->shared_modules->{$src_top} ||= new SharedModule($self, $lacking->extension));
               $lacking->is_private=!$shared_mod->is_mutable;
            }
            if ($lacking->is_private) {
               # The using extension is not dependent on the defining one or is immutable.
               # Restore the state and instantiate the wrapper in the private area
               $lacking->extension=$preserve_ext;
            }
         }
      }
   }

   unless ($MAKE) {
      require Polymake::Core::CPlusPlus_config;
      configure_make($custom_handler);
   }
   my $file=new Tempfile();
   my $so_name=$file->rename.$dl_suffix;

   open my $cc, ">$file.cc";
   print $cc <<".";
#include <unistd.h>
namespace { void delete_temp_file() __attribute__((destructor));
            void delete_temp_file() { unlink("$so_name"); } }
.
   print $cc map { "#include \"$_\"\n" } uniq(@includes);
   print $cc "namespace polymake { namespace ".$self->application->name." { namespace {\n",
             $lacking->gen_temp_code($type_proto),
             "} } }\n";
   close $cc;

   warn_print( "Compiling temporary shared module, please be patient..." ) if $Verbose::cpp;

   if ($lacking->is_private) {
      unless ($private_wrapper_ext) {
         if ($forbid_private_wrappers) {
            croak( "Private wrapper extension is forbidden, can't compile this code:\n", $lacking->gen_temp_code($type_proto), "\n " );
         }
         create_private_wrapper();
      }
      $shared_mod=($self->shared_modules->{$private_wrapper_ext->dir} ||= new SharedModule($self, $private_wrapper_ext));
   }

   my $debug_flag= $debug && "Debug=y";
   system(($Verbose::cpp>1 && "cat $file.cc >&2; ") .
          "$MAKE -C " . $shared_mod->build_dir . " $so_name $debug_flag SharedModules=$file OwnShared=$so_name WrappersOnly= $TempWrapperFor ProcessDep=none " .
          ($lacking->is_private && defined($lacking->extension) && "RequireExtensions='" . join(" ", map { $_->dir } map { ($_, @{$_->requires}) } is_object($lacking->extension) ? ($lacking->extension) : @{$lacking->extension}) . "'") .
          ($Verbose::cpp ? ">&2 " : ">/dev/null") . " 2>$file.err")
     and
   die "Shared module compilation failed; see the error log below\n\n" . `cat $file.err`;

   if (defined $type_proto) {
      load_shared_module($so_name);
   } else {
      my $functions_begin=@{$root->functions};
      load_shared_module($so_name);
      bind_functions($self, undef, $root->functions, $functions_begin, $#{$root->functions},
                     { "$file.cc" => ($lacking->is_private ? "$file.cc" : $lacking->is_instance_of->embedded || $lacking->source_file) });
   }
}
#######################################################################################
sub obliterate_extension {
   my ($self, $ext, $entire_app)=@_;
   if (defined (my $shared_mod=$self->shared_modules->{$ext->dir})) {
      delete $self->shared_modules->{$ext->dir};
      if (defined (delete $wrapper_updated{$ext->dir."/".$self->application->name})) {
         $custom_handler->set('%wrapper_updated');
      }
   }
   if ($entire_app) {
      if ($self->will_update_sources) {
         $self->will_update_sources=0;
         forget AtEnd($self->application->name.":C++");
      }
      if (defined (delete $wrapper_updated{$self->application->name})) {
         $custom_handler->set('%wrapper_updated');
      }
   }
}
#######################################################################################
package Polymake::Core::CPlusPlus;

declare $root;
declare $typeinfo_re=qr{^typeinfo for };
declare $wrapped_typeinfo_re=qr{^typeinfo for PolymakeTestWrap<(.*)>};
declare $constructor_re=qr{::$id_re (?:\[.*?\])? \(\)$}xo;
declare $wrapped_constructor_re=qr{^PolymakeTestWrap<(.*)>::PolymakeTestWrap (?:\[.*?\])? \(\)$}xo;

use Polymake::Struct (
   '@functions',
   '@regular_functions',
   '@embedded_rules',
   '@duplicate_class_instances',
   '%classes',
   '%builtins',
   '%templates',
   '%typeids',
   '%auto_functions',
   '$auto_default_constructor',
   '$auto_convert_constructor',
   '$auto_assignment',
   '$auto_conversion',
   '$indirect_wrapper',
   '@auto_assoc_methods',
   '@auto_set_methods',
   '$update_private_wrapper',
);

# auto-function flags:
# The values in the first row contribute to auto-generated wrapper names.
use Polymake::Enum qw( func: is_method=0x1 is_lvalue=0x2 is_lvalue_opt=0x4 has_anchor=0x8 is_void=0x10
                             is_wary=0x20 is_ellipsis=0x40 is_integral=0x80 is_non_const=0x100
                             is_top_object_method=0x200 has_prescribed_ret_type=0x400 is_static=0x800 );

# class description kind:    (keep in sync with lib/core/include/perl/constants.h)
use Polymake::Enum qw( class_is: scalar container composite opaque kind_mask=0xf
                       assoc_container=0x100 sparse_container=0x200 set=0x400 serializable=0x800 );

use Polymake::Enum qw( assoc: helem find exists delete_void delete_ret );

use Polymake::Ext;

push @UserSettings::add_custom_vars,
sub {
   $custom_handler=$Custom->app_handler(__PACKAGE__);

   declare $MAKE;
   $custom_handler->add('$MAKE', <<'.', $Customize::state_config);
# GNU make utility
.
   declare $MAKEFLAGS;
   $custom_handler->add('$MAKEFLAGS', <<'.', $Customize::state_config);
# Options for make utility (especially useful: -jN for parallel builds on multi-core machines)
.
   declare %private_wrappers;
   $custom_handler->add('%private_wrappers', <<'.', $Customize::state_hidden | $Customize::state_noexport);
# Locations of the automatically generated source code of C++/perl wrappers for private use.
.
   declare %wrapper_updated;
   $custom_handler->add('%wrapper_updated', <<'.', $Customize::state_hidden | $Customize::state_noexport);
# Timestamps of last changes made to the automatically generated source code of C++/perl wrappers
.

   $custom_handler->cleanup;
};

my $void_signature=[[0, 0], []];
my $unary_op_signature=[[1, 1, '$'], ['*']];
my $binary_op_signature=[[2, 2, qw($ $)], [qw(* *)]];

sub init {
   $root=&_new;
   my $standalone=shift;

   if ($ENV{POLYMAKE_DEBUG_CLIENTS}) {
      die <<".";
Use of environment variable POLYMAKE_DEBUG_CLIENTS is deprecated.
Please use POLYMAKE_CLIENT_SUFFIX=$ENV{POLYMAKE_DEBUG_CLIENTS} instead to enforce loading non-standard client modules.
Note that debug output in some clients only appears at high debug levels (-dd and higher).
.
   }

   my $suffix=$ENV{POLYMAKE_CLIENT_SUFFIX};
   $debug= $suffix eq "-d";
   $dl_suffix="$suffix.$DynaLoader::dl_dlext";

   load_shared_module("$InstallArch/lib/core$dl_suffix") if $standalone;

   # create the standard constructors right now
   my $dc=new Constructor;
   $dc->prepare(@$void_signature);
   $dc->name="";
   $root->auto_default_constructor=$root->auto_functions->{$dc->wrapper_name}=$dc;

   my $cc=$root->auto_convert_constructor=new Constructor;
   $cc->prepare(@$unary_op_signature);
   $cc->name="";
   $root->auto_convert_constructor=$root->auto_functions->{$cc->wrapper_name}=$cc;

   $root->auto_assignment=$root->auto_functions->{"=ass"}=new SpecialOperator("assign");
   $root->auto_conversion=$root->auto_functions->{".cnv"}=new SpecialOperator("convert");
   $root->indirect_wrapper=$root->auto_functions->{".wrp"}=new IndirectWrapper();
   create_assoc_methods();

   $BigObject_cpp_options=new ObjectOptions(name=>"perl::Object", builtin=>1);
   $BigObjectArray_cpp_options=new ObjectOptions(name=>"Array<perl::Object>", builtin=>1);
}

sub Polymake::Core::ObjectType::cppoptions { $BigObject_cpp_options }

#################################################################################
# for test driver script
sub forbid_code_generation {
   my ($scope, $any)=@_;
   if ($any) {
      $scope->begin_locals;
      local_sub(\&perApplication::compile_load_temp_shared_module,
                sub {
                   my (undef, $lacking, $type_proto)=@_;
                   croak( "C++ glue code generation is forbidden!\nMissing wrapper is:\n", $lacking->gen_temp_code($type_proto), "\n " );
                });
      $scope->end_locals;
   } else {
      $scope->begin_locals;
      local_scalar($private_wrapper_ext, undef);
      local_incr($forbid_private_wrappers);
      $scope->end_locals;
   }
}
#################################################################################
sub provide_cpp_type_descr {
   my ($proto, $maybe_private)=@_;
   $root->classes->{$proto->pkg}
   or do {
      $proto->cppoptions->finalize;
      $proto->cppoptions->analyze_provenience if $maybe_private;
      my $perApp=$proto->cppoptions->application->cpp;
      $perApp->compile_load_temp_shared_module($proto->cppoptions, $proto);
      if (defined (my $descr=$root->classes->{$proto->pkg})) {
         $perApp->lacking_types->{$proto}=1;
         $perApp->ensure_update_sources;
         $descr
      } else {
         die " C++ binding of type ", $proto->full_name, " was not created as expected\n";
      }
   }
}

sub Polymake::Core::PropertyType::cpp_type_descr {
   my ($self)=@_;
   $root->classes->{$self->pkg} || croak( "type ", $self->full_name, " has no C++ binding" );
}

sub Polymake::Core::PropertyType::cpp_persistent_type_descr {
   my ($self)=@_;
   defined($self->cppoptions) ? ($self->cppoptions->descr //= &Polymake::Core::PropertyType::cpp_type_descr)
                              : &Polymake::Core::PropertyType::cpp_type_descr;  # let's it croak
}

sub Polymake::Core::PropertyType::get_cpp_representation {
   &Polymake::Core::PropertyType::cpp_type_descr->get_cpp_representation(@_);
}

sub get_magic_cpp_class {
   my $typeid=get_magic_typeid(shift, 1);
   $typeid && do {
      if (defined (my $descr=$root->typeids->{$typeid})) {
         scalar($descr->get_cpp_representation);
      } else {
         demangle($typeid);
      }
   }
}

sub Polymake::Core::PropertyType::get_element_type {
   my ($proto)=@_;
   if ($proto->cppoptions && !$proto->cppoptions->builtin) {
      my $descr=$proto->cpp_type_descr;
      if (($descr->kind & $class_is_kind_mask) == $class_is_container) {
         get_type_proto($descr->vtbl, 1)
      } else {
         undef
      }
   } elsif ($proto->dimension==1) {
      $proto->params->[0];
   } else {
      undef
   }
}

sub Polymake::Core::PropertyType::get_field_type {
   my ($proto, $field_name)=@_;
   if ($proto->cppoptions && $proto->cppoptions->fields) {
      my $i=string_list_index($proto->cppoptions->fields, $field_name);
      if ($i>=0 && defined (my $elem_protos=get_type_proto($proto->cpp_type_descr->vtbl, 2))) {
         return $elem_protos->[$i];
      }
   }
   undef;
}

sub Polymake::Core::PropertyType::guess_element_type {
   my ($data)=@_;
   if (is_object($data) && UNIVERSAL::can($data, "type") &&
       defined (my $type=$data->type->get_element_type)) {
      return $type;
   }
   my $first=$data->[0];
   if (is_object($first) && UNIVERSAL::can($first, "type")) {
      $first->type;
   } else {
      match_builtin_type($first);
   }
}

# the indices correspond to the return values of classify_scalar
my @builtin_type_names=qw( std::string double long int bool );

sub match_builtin_type {
   if (defined (my $type=&classify_scalar)) {
      $root->builtins->{$builtin_type_names[$type]}
   } else {
      undef
   }
}

sub guess_builtin_type {
   if (my $class=ref($_[0])) {
      if ($class eq "ARRAY") {
         # deliberately interpret an empty array as an empty set of indices
         if (defined (my $elem_proto= @{$_[0]} ? guess_builtin_type($_[0]->[0]) : $root->builtins->{int})) {
            return $root->builtins->{array}->typeof($elem_proto);
         } else {
            croak( "don't know how to match [ '$_[0]->[0]' ] to a C++ type" );
         }
      }
      ### FIXME: mapping for HASH
      if (is_object($_[0])) {
         if (defined (my $proto=UNIVERSAL::can($_[0], ".type"))) {
            $proto=$proto->();
            if ($proto->cppoptions) {
               return $proto;
            }
         }
      }
      croak( "don't know how to match $class to a C++ type" );
   } else {
      &match_builtin_type // croak( "can't map ", defined($_[0]) ? "'$_[0]'" : "UNDEF", " to a C++ type" )
   }
}

# private:
sub add_cross_app {
   my ($self, $new_app)=@_;
   foreach (@{$self->cross_apps //= [ ]}) {
      if (defined (my $common_app=$_->common($new_app))) {
         $common_app==$_ or $_=$common_app;
         return;
      }
   }
   push @{$self->cross_apps}, $new_app;
}

# private:
sub set_application {
   my ($self, $new_embedded, $new_app, $new_cross_apps)=@_;
   if (defined $new_embedded) {
      if (defined $self->embedded) {
         $self->embedded eq $new_embedded
           or croak( $self->complain_source_conflict, ": conflicting files $new_embedded and ", $self->embedded );
      } else {
         $self->embedded=$new_embedded;
      }
   }
   if (defined (my $app=$self->application)) {
      if ($app != $new_app) {
         if (defined $new_embedded) {
            $self->application=$new_app;
            if ($new_app->common($app) != $new_app) {
               add_cross_app($self, $app);
            }
            $app=$new_app;
         } elsif ((my $common_app=$app->common($new_app)) != $app) {
            if (defined($self->embedded) || !defined($common_app)) {
               add_cross_app($self, $new_app);
            } else {
               $self->application=$app=$common_app;
            }
         }
      }
      if (defined $new_cross_apps) {
         foreach $new_app (@$new_cross_apps) {
            if ($app->common($new_app) != $app) {
               add_cross_app($self, $new_app);
            }
         }
      }
   } else {
      $self->application=$new_app;
      if (defined $new_cross_apps) {
         $self->cross_apps=[ @$new_cross_apps ];
      }
   }
}

*set_extension=\&PropertyParamedType::set_extension;

# private:
sub remove_origin_extension {
   my ($self)=@_;
   if (defined($self->extension) && $self->extension==$self->application->origin_extension) {
      undef $self->extension;
   }
}

#######################################################################################
sub resolve_auto_function {
   &AutoFunction::find_instance // do {
      my $lacking=new LackingInstance(@_);
      my $perApp=$lacking->application->cpp;
      $perApp->compile_load_temp_shared_module($lacking);
      if (defined (my $inst=&AutoFunction::find_instance)) {
         push @{$perApp->lacking_auto_functions}, $lacking;
         $perApp->ensure_update_sources;
         $inst
      } else {
         my $auto_func=shift;
         die "an instance of C++ ",
             instanceof Operator($auto_func) ? ("operator ", $auto_func->sign)
                                             : ("function ", $auto_func->perl_name),
             " was not created as expected\n";
      }
   }
}
#######################################################################################
sub resolve_regular_function {
   my ($auto_func, $descr, $args)=@_;
   my $indirect_wrapper=
   $auto_func->inst_cache ||= do {
      my $code=create_function_wrapper($descr, $auto_func->application->pkg);
      (my $void_ret, @{$auto_func->lvalue_args})=@{$code->()};
      $auto_func->flags |= $func_is_void if $void_ret;
      my $indirect_descr=$root->indirect_wrapper->inst_cache->{$descr->arg_types} || do {
         my $lacking=new LackingRegular($auto_func,$descr);
         my $perApp=$auto_func->application->cpp;
         $perApp->compile_load_temp_shared_module($lacking);
         if (defined (my $inst=$root->indirect_wrapper->inst_cache->{$descr->arg_types})) {
            push @{$perApp->lacking_auto_functions}, $lacking;
            $perApp->ensure_update_sources;
            $inst
         } else {
            die "the wrapper for C++ function ", $auto_func->perl_name, " was not created as expected\n";
         }
      };
      my $n_args=@{$descr->arg_types};
      ++$n_args if $auto_func->flags & $func_has_prescribed_ret_type;
      set_number_of_args($code, $n_args, $auto_func->flags & $func_is_ellipsis, $indirect_descr->wrapper);
      $code
   };
   $auto_func->check_lvalue_args($args);
   $indirect_wrapper
}
#######################################################################################
sub try_merge_auto_functions {
   my ($f1, $f2)=@_;
   return 0 if $f1->embedded ne $f2->embedded;
   return 1 if $f1->application==$f2->application;
   if (not($f1->flags & $func_is_method) && defined (my $common_app=$f1->application->common($f2->application))) {
      $f2->application=$common_app;
      return 1;
   }
   0
}
#######################################################################################
sub auto_func_twin_key {
   my ($src_file)=@_;
   $src_file =~ s{^.*?/(apps/$id_re/src/)(?:perl/wrap-)?}{$1}o;
   $src_file;
}

sub check_twins {
   my ($auto_func)=@_;
   if (defined (my $twin=$root->auto_functions->{$auto_func->wrapper_name})) {
      if (ref($twin) eq "HASH") {
         foreach my $other (values %$twin) {
            if (try_merge_auto_functions($auto_func, $other)) {
               push @{$other->include}, @{$auto_func->include};
               $_[0]=$other;
               return;
            }
         }
         $twin->{$auto_func->embedded ? auto_func_twin_key($auto_func->embedded) : $auto_func->application->name}=$auto_func;
      } elsif (try_merge_auto_functions($auto_func, $twin)) {
         push @{$twin->include}, @{$auto_func->include};
         $_[0]=$twin;
      } else {
         $root->auto_functions->{$auto_func->wrapper_name}=
            { map { ($_->embedded ? auto_func_twin_key($_->embedded) : $_->application->name, $_) } $auto_func, $twin };
      }
   } else {
      $root->auto_functions->{$auto_func->wrapper_name}=$auto_func;
   }
}

#######################################################################################

my %op_descr=( '+' => [ ':add', 1 ], '-' => [ ':sub', 1 ], '*' => [ ':mul', 1 ], '/' => [ ':div', 1 ], '%' => [ ':mod', 1 ],
               '<<' => [ ':lsh', 1 ], '>>' => [ ':rsh', 1 ], '&' => [ ':and', 1 ], '|' => [ ':_or', 1 ], '^' => [ ':xor', 1 ],
               '+=' => [ '=add', 1+$func_is_lvalue ], '-=' => [ '=sub', 1+$func_is_lvalue ], '*=' => [ '=mul', 1+$func_is_lvalue ],
               '/=' => [ '=div', 1+$func_is_lvalue ], '%=' => [ '=mod', 1+$func_is_lvalue ], '<<=' => [ '=lsh', 1+$func_is_lvalue ],
               '>>=' => [ '=rsh', 1+$func_is_lvalue ], '&=' => [ '=and', 1+$func_is_lvalue ], '|=' => [ '=_or', 1+$func_is_lvalue ],
               '^=' => [ '=xor', 1+$func_is_lvalue ],
               'neg' => [ '.neg', 0 ], '!' => [ '.not', 0 ], '~' => [ '.com', 0 ], 'bool' => [ '.boo', 0 ],
               '++' => [ '.inc', $func_is_lvalue ], '--' => [ '.dec', $func_is_lvalue ],
               '==' => [ ':_eq', 1 ], '!=' => [ ':_ne', 1 ], '<' => [ ':_lt', 1 ],
               '<=' => [ ':_le', 1 ], '>' => [ ':_gt', 1 ], '>=' => [ ':_ge', 1 ],
               '<=>' => [ 'cmp', 1 ], '**' => [ 'pow', 1 ], 'abs' => [ 'abs', 0 ],
             );

my %op_groups=( arith => [ qw( neg + - * / += -= *= /= ) ],
                arith_nodiv => [ qw( neg + - * += -= *= ) ],
                sets => [ qw( ~ + += - -= * *= ^ ^= ) ],
                bits => [ qw( ~ & | ^ &= |= ^= ) ],
                compare => [ qw( == != < <= > >= ) ],
                eq => [ qw( == != ) ],
                string => [ ],                                  # just to enable the default string ops in define_operators
              );

my @assoc_descr=(
   [ $assoc_helem,       ":brk",          "[]",                                 1+$func_is_lvalue_opt ],
   [ $assoc_find,        "assoc_find",    $binary_op_signature,                 0,
     name => "pm::perl::find_element", include => [ "polymake/perl/assoc.h" ] ],
   [ $assoc_exists,      "exists",        $unary_op_signature,                  $func_is_method ],
   [ $assoc_delete_void, "erase",         $unary_op_signature,                  $func_is_method+$func_is_non_const+$func_is_void ],
   [ $assoc_delete_ret,  "assoc_delete",  [[2, 2, qw($ $)], [qw(*:lvalue *)]],  0,
     name => "pm::perl::delayed_erase", include => [ "polymake/perl/assoc.h" ], macro_call => "Tmp( " ],
);

sub create_assoc_methods {
   $#{$root->auto_assoc_methods}=$#{$root->auto_set_methods}=$#assoc_descr;
   foreach my $descr (@assoc_descr) {
      my ($i, $name, $signature, $flags, @options)=@$descr;
      if ($name =~ /^\w/) {
         my $auto_func=new AutoFunction($name, $flags, undef, @options);
         $auto_func->prepare(@$signature);
         $root->auto_functions->{$auto_func->wrapper_name}=$auto_func;
         my $code=$root->auto_assoc_methods->[$i]=sub { &{resolve_auto_function($auto_func, \@_)} };
         set_method($code) if $flags & $func_is_method;
         declare_lvalue($code, 1) if $flags & ($func_is_lvalue|$func_is_lvalue_opt);
      } else {
         my $op=new Operator($name, $flags, $signature);
         $root->auto_functions->{$name}=$op;
         $root->auto_assoc_methods->[$i]=$op->code;
      }
   }
   $root->auto_set_methods->[$assoc_helem]=\&no_set_method;
   $root->auto_set_methods->[$assoc_find]=\&no_set_method;
   $root->auto_set_methods->[$assoc_exists]=$root->auto_assoc_methods->[$assoc_exists];
   $root->auto_set_methods->[$assoc_delete_void]=$root->auto_assoc_methods->[$assoc_delete_void];
   $root->auto_set_methods->[$assoc_delete_ret]=\&no_set_method;
}

sub no_set_method { croak("This operation is not defined on sets") }

sub add_operator {
   my ($self, $sign, $pkg)=@_;
   my ($op, $code);

   if ($sign =~ s/^ ([^\s(]++) \s*\(\s* (?'cppname' $qual_id_re) (?: \s*\( (?'sig' $balanced_re) \) )? (?'attrs' (?: \s*:\s* $id_re)* ) \s*\)\s*$/$1/xo) {
      my ($cppname, $signature, $attrs)=@+{qw(cppname sig attrs)};
      my $descr=$op_descr{$sign} or croak( "operator $sign can't have C++ binding" );
      my $flags=0;
      while ($attrs =~ /$id_re/go) {
         if ($& eq "method") {
            $flags |= $func_is_method;
         } elsif ($& eq "wary") {
            $flags |= $func_is_wary;
         } elsif ($& eq "lvalue") {
            $flags |= $func_is_lvalue | $func_has_anchor;
         } elsif ($& eq "lvalue_opt") {
            $flags |= $func_is_lvalue_opt | $func_has_anchor;
         } elsif ($& eq "anchor") {
            $flags |= $func_has_anchor;
         } else {
            croak( "unknown function attribute '$1'" );
         }
      }
      if ($flags and !($flags & $func_is_method)) {
         croak( "attributes $attrs are only applicable to methods" );
      }
      if ($signature) {
         $signature =~ s/^\s+//;  $signature =~ s/\s+$//;
         my @arg_attrs=split /\s*,\s*/, $signature;
         my $num_args=@arg_attrs;
         if ($num_args + (($flags & $func_is_method) != 0) != ($descr->[1] & 1)+1) {
            croak( "signature '$signature' does not match the arity of operator $descr->[0]" );
         }
         $signature=[[$num_args, $num_args, ('$') x $num_args], \@arg_attrs];
      } else {
         $signature= $descr->[1] ? $binary_op_signature : $unary_op_signature;
      }
      $op=new AutoFunction($descr->[0], $flags, $self->application);
      $op->prepare(@$signature, $pkg);
      check_twins($op);

      if ($sign eq '<=>') {
         $code=sub {
            if (pop) {
               - &{ resolve_auto_function($op, \@_) }
            } else {
               &{ resolve_auto_function($op, \@_) }
            }
         }
      } elsif ($sign eq '**') {
         $code=sub {
            if (pop) {
               missing_op(@_,1,'**');
            } else {
               &{ resolve_auto_function($op, \@_) }
            }
         }
      } else {
         $code=sub { splice @_,-2; &{ resolve_auto_function($op, \@_) } }
      }

      $op->name=$cppname;
   } else {
      my $arg_flag=0;
      my $anchor_flag="";
      while ($sign =~ s/:($id_re)$//o) {
         if ($1 eq "wary") {
            $arg_flag |= $func_is_wary;
         } elsif ($1 eq "int") {
            $arg_flag |= $func_is_integral;
         } elsif ($1 eq "anchor") {
            $anchor_flag = "a";
         } else {
            croak( "unknown attribute '$1'" );
         }
      }
      my $descr=$op_descr{$sign} or croak( "operator $sign can't have C++ binding" );
      if ($descr->[0] !~ /^[.:=]/) {
         croak( "operator $sign must be matched to a C++ function or method" );
      }
      my $wrapper_name=$descr->[0].$anchor_flag;
      $code=(($op=($root->auto_functions->{$wrapper_name} ||= new Operator($wrapper_name, $descr->[1], $sign)))->subs->{$arg_flag} ||= $op->code($arg_flag));
   }

   ($op->flags & $func_is_lvalue) && !$_[3]++ ? ($sign => $code, '=' => \&overload_clone_op) : ($sign => $code);
}

sub define_operators {
   my ($self, $opts, $pkg)=@_;
   if (length($opts->operators)) {
      my $clone_added;
      my @ops;
      while ($opts->operators =~ /[^\s\(]++ (?:\s*\( $balanced_re \))?+/gxo) {
         my $op=$&;
         push @ops, map { add_operator($self, $_, $pkg, $clone_added) }
                    ( $op =~ /^\@($id_re) (:$id_re)?/xo
                      ? ( map { $_.$2 } @{$op_groups{$1} or croak( "unknown operator group $1" )} )
                      : $op );
      }
      overload::OVERLOAD(
         $pkg, fallback => 0, nomethod => \&missing_op,
         '""' => \&convert_to_string,
         @PropertyType::string_ops,
         @ops,
      );
   }
   if (defined $opts->fields) {
      my $i=0;
      foreach my $field_name (@{$opts->fields}) {
         define_function($pkg, $field_name, Struct::create_accessor($i++, \&composite_access));
      }
   }
}
#######################################################################################
sub assign_any {
   if ($#_==1 && (ref($_[1]) eq "ARRAY" || UNIVERSAL::isa($_[1], ref($_[0])))) {
      assign_to_cpp_object(@_, $PropertyType::trusted_value);
   } else {
      assign_array_to_cpp_object(@_, $PropertyType::trusted_value);
   }
}

sub construct_any {
   assign_any((shift)->construct->(), @_);
}

sub construct_parsed : method {
   my $proto=shift;
   if ($_[0] =~ /\S/) {
      eval { assign_to_cpp_object($proto->construct->(), $_[0], $PropertyType::trusted_value) }
      // do {
         if ($@ =~ /^(\d+)\t/) {
            local $_=$_[0];
            pos($_)=$1;
            $proto->parse_error;
         } else {
            die $@;
         }
      }
   } else {
      $proto->construct->();
   }
}

Struct::pass_original_object(\&construct_parsed);

sub construct_composite_cpp_value {
   my ($proto)=@_;
   if (($proto->cppoptions->descr->kind & $class_is_kind_mask) == $class_is_composite &&
       @_>2 && !($#_%2)) {
      my ($i,$last)=(1,$#_-1);
      for (; $i<=$last; $i+=2) {
         if (!is_keyword($_[$i])) {
            return &construct_any;
         }
      }
      my $x=$proto->construct->();
      for ($i=1; $i<=$last; $i+=2) {
         my $field=$_[$i];
         $x->$field=$_[$i+1];
      }
      $x;
   } else {
      &construct_any;
   }
}

sub std_default_constructor : method {
   &{resolve_auto_function($root->auto_default_constructor, \@_)}
}

sub std_parsing_constructor {
   assign_to_cpp_object($_[0]->construct->(), $_[1], $PropertyType::trusted_value);
}

sub std_convert_constructor : method {
   if (ref($_[1]) eq "ARRAY") {
      &std_parsing_constructor;
   } else {
      &{resolve_auto_function($root->auto_convert_constructor, \@_)};
   }
}

sub std_deserializing_constructor : method {
   if (ref($_[1]) eq "ARRAY" && get_array_flags($_[1])<0) {
      my $src=pop(@_);
      assign_to_cpp_object(&std_default_constructor, $src, $PropertyType::trusted_value);
   } else {
      &{resolve_auto_function($root->auto_convert_constructor, \@_)};
   }
}

package _::StdConstr;

sub set_construct_node {
   state $construct_node = do {
      my $node=new Overload::Node(undef, undef, 0);
      Overload::add_instance(__PACKAGE__, ".construct", \&std_default_constructor, undef, $void_signature->[0], undef, $node);
      Overload::add_instance(__PACKAGE__, ".construct", \&std_convert_constructor, undef, $unary_op_signature->[0], undef, $node);
      Overload::add_fallback_to_node($node, \&construct_composite_cpp_value);
      $node
   };
   my (undef, $proto)=@_;
   $proto->construct_node=$construct_node;
   $proto->parse=\&construct_parsed;
}

package __::Deserializing;

sub set_construct_node {
   state $construct_node = do {
      my $node=new Overload::Node(undef, undef, 0);
      Overload::add_instance(__PACKAGE__, ".construct", \&std_deserializing_constructor, undef, $unary_op_signature->[0], undef, $node);
      $node
   };
   my (undef, $proto)=@_;
   $proto->construct_node=$construct_node;
}

package __;

sub define_constructors {
   my ($proto)=@_;
   if (!defined($proto->construct_node) && length($proto->cppoptions->default_constructor)) {
      my $pkg=namespaces::lookup_class(__PACKAGE__, $proto->cppoptions->default_constructor, $proto->application->pkg)
        or croak( "class ", $proto->full_name, " tries to inherit constructors from an unknown package ", $proto->cppoptions->default_constructor );
      $pkg->set_construct_node($proto);
      $proto->create_method_new();
      no strict 'refs';
      push @{$proto->pkg."::ISA"}, $pkg;
   }
}

sub create_methods {
   my ($self, $proto, $generic_opts)=@_;
   my $descr=provide_cpp_type_descr($proto, defined($generic_opts));
   # this is only called for types declared in perl (i.e. "persistent types"),
   # the relation between PropertyType and C++ class descriptors is unambigous
   $proto->dimension=obj_dimension($descr->vtbl);
   my $opts=$proto->cppoptions;
   $opts->descr=$descr;

   my $symtab=get_pkg($proto->pkg);
   my $kind=$descr->kind & $class_is_kind_mask;
   my $bespoke_toXML=$proto->toXML;

   if ($kind==$class_is_container) {
      if ($descr->kind & $class_is_assoc_container) {
         $proto->toXML=\&assoc_container_toXML;
         no strict 'refs';
         push @{$proto->pkg."::ISA"}, "Polymake::Core::CPlusPlus::TiedHash";
         $proto->toString=\&convert_to_string;
      } else {
         $proto->toXML=\&container_toXML;
         no strict 'refs';
         push @{$proto->pkg."::ISA"}, "Polymake::Core::CPlusPlus::TiedArray";
         if ($descr->kind & $class_is_set) {
            $proto->toString=\&convert_to_string;
         } else {
            $proto->toString=sub {
               if ($PropertyType::nesting_level) {
                  my $text=&convert_to_string;
                  substr($text,-1,1) eq "\n" ? "<$text>\n" : substr($text,0,1) eq "{" ? $text : "<$text>"
               } else { &convert_to_string }
            };
         }
      }

   } elsif ($kind==$class_is_composite) {
      if ($proto->cppoptions->fields) {
         if (get_type_proto($descr->vtbl, 3) != scalar(@{$proto->cppoptions->fields})) {
            croak( "number of field names does not match the C++ description of class ", $proto->full_name );
         }
      } elsif ($proto->cppoptions->fields=get_type_proto($descr->vtbl, 5)) {
         $proto->cppoptions->operators ||= '@eq';
         define_operators($self, $proto->cppoptions, $proto->pkg);
      }
      $proto->toXML=\&composite_toXML;
      no strict 'refs';
      push @{$proto->pkg."::ISA"}, "Polymake::Core::CPlusPlus::TiedCompositeArray";
      $proto->toString=sub { $PropertyType::nesting_level ? "(".&convert_to_string.")" : &convert_to_string };

   } else {
      if (!length($proto->cppoptions->operators)) {
         overload::OVERLOAD($proto->pkg,
                            fallback => 0,
                            nomethod => \&missing_op,
                            '""' => \&convert_to_string,
                            @PropertyType::string_ops,
                           );
      }
      $proto->toString=\&convert_to_string;
      if ($kind==$class_is_scalar) {
         if (my $int_type=$root->builtins->{int}) {
            $int_type->add_constructor("construct", \&convert_to_int, [1, 1, $proto]);
         }
         if (my $float_type=$root->builtins->{double}) {
            $float_type->add_constructor("construct", \&convert_to_float, [1, 1, $proto]);
         }
      }
   }
   if ($descr->kind & $class_is_serializable and !defined($bespoke_toXML)) {
      $proto->toXML=\&serialized_toXML;
   }

   if (!defined($generic_opts)) {
      define_constructors($proto);
      define_operators($self, $proto->cppoptions, $proto->pkg);
   }
}

# defer the choice of the right method until the first real usage,
# when all prototypes are created
sub container_toXML : method {
   my $proto=shift;
   my $descr=$proto->cppoptions->descr;
   $proto->toXML= do {
      if (defined (my $val_proto=get_type_proto($descr->vtbl, 1))) {
         if ($descr->kind & $class_is_sparse_container) {
            if (defined (my $elem_proto=get_type_proto($descr->vtbl, 0))) {
               my $own_dim=$proto->dimension-$elem_proto->dimension;
               if ($own_dim==1) {
                  sub { PropertyType::sparseArray_toXML($elem_proto, @_) };
               } elsif ($own_dim==2) {
                  sub { PropertyType::sparseMatrix_toXML($val_proto, @_) };
               } else {
                  undef;
               }
            } else {
               undef;
            }
         } else {
            $val_proto->toXML
            ? sub { PropertyType::nontrivialArray_toXML($val_proto, @_) }
            : sub { PropertyType::trivialArray_toXML($val_proto, @_) };
         }
      } else {
         undef;
      }
   };
   $proto->toXML->(@_);
}

sub assoc_container_toXML : method {
   my $proto=shift;
   my $descr=$proto->cppoptions->descr;
   $proto->toXML= do {
      my @kv_proto=map { get_type_proto($descr->vtbl, $_) } 0,1;
      if (defined($kv_proto[0]) && defined($kv_proto[1])) {
         my $pair_proto=$root->builtins->{pair}->typeof(@kv_proto);
         sub { PropertyType::assocContainer_toXML($pair_proto, @_) }
      } else {
         undef
      }
   };
   $proto->toXML->(@_);
}

sub composite_toXML : method {
   my $proto=shift;
   my $descr=$proto->cppoptions->descr;
   $proto->toXML= do {
      my $elem_protos=get_type_proto($descr->vtbl, 2);
      my $trivial=1;
      foreach my $elem_proto (@$elem_protos) {
         if (defined $elem_proto) {
            if ($elem_proto->toXML) {
               $trivial=0;
            }
         } else {
            undef $trivial; last;
         }
      }
      if (defined $trivial) {
         $trivial
         ? \&PropertyType::trivialComposite_toXML
         : sub { PropertyType::nontrivialComposite_toXML($elem_protos, @_) };
      } else {
         undef;
      }
   };
   $proto->toXML->(@_);
}

sub serialized_toXML : method {
   my $proto=shift;
   my $descr=$proto->cppoptions->descr;
   my $serialized_proto=get_type_proto($descr->vtbl, 4);
   $proto->toXML= sub {
      my $serialized=convert_to_serialized(shift);
      $serialized_proto->toXML->($serialized, @_)
   };
   $proto->toXML->(@_);
}

sub serialized_with_id_toXML : method {
   my $proto=shift;
   my $descr=$proto->cppoptions->descr;
   my $serialized_proto=get_type_proto($descr->vtbl, 4);
   $proto->toXML= sub {
      my ($obj, $writer, @attrs)=@_;
      my $new_id;
      my $id=(($writer->{':ids'}->{$proto} //= { })->{$obj->id} //= ($new_id=++$writer->{':global_id'}));
      if (defined $new_id) {
         $writer->{':last_id'}=$id;
         my $serialized=convert_to_serialized($obj);
         $serialized_proto->toXML->($serialized, $writer, @attrs, id=>$id);
      } else {
         if ($id != $writer->{':last_id'}) {
            push @attrs, id => $id;
            $writer->{':last_id'}=$id;
         }
         $writer->emptyTag("r", @attrs);
      }
   };
   $proto->toXML->(@_);
}

#######################################################################################
sub dump_arg {
   if (&is_object) {
      if (my $proto=UNIVERSAL::can($_[0], ".type")) {
         $proto->()->full_name;
      } else {
         ref($_[0]);
      }
   } else {
      ref($_[0]) || "'$_[0]'"
   }
}

sub missing_op {
   defined($_[1])
   ? $_[2]
     ? croak( "undefined operator ", dump_arg($_[1]), " $_[3] ", dump_arg($_[0]) )
     : croak( "undefined operator ", dump_arg($_[0]), " $_[3] ", dump_arg($_[1]) )
   : croak( "undefined operator $_[3] ", dump_arg($_[0]) );
}
#######################################################################################
sub load_shared_module {
   my ($so_name)=@_;
   DynaLoader::dl_load_file($so_name, 0x01)
      or die "Can't load shared module $so_name: ", DynaLoader::dl_error(), "\n";
}
#######################################################################################
sub init_private_wrapper {
   if ($PrivateDir && defined (my $dir=$private_wrappers{$InstallArch})) {
      if (-d ($dir="$PrivateDir/$dir")) {
         $private_wrapper_ext=new Extension($dir, "private:");
         my $ext;
         foreach my $prereq (@{$private_wrapper_ext->requires}) {
            if (defined ($ext=$Extension::registered_by_URI{$prereq})
                and $ext->is_active) {
               $prereq=$ext;
            } else {
               filter_out_extension($dir, $prereq);
               ensure_update_private_wrapper();
               undef $prereq;
            }
         }
         if ($root->update_private_wrapper) {
            @{$private_wrapper_ext->requires}=grep { defined($_) } @{$private_wrapper_ext->requires};
            # update the make.conf file right now because it will be used for recompilation
            write_private_wrapper_conf_make();
         }
      } else {
         delete $private_wrappers{$InstallArch};
         $custom_handler->set('%private_wrappers');
      }
   }
}

sub create_private_wrapper {
   require Polymake::SourceVersionControl;

   my $dir;
   if ($PrivateDir) {
      if (-d "$PrivateDir/wrappers") {
         # remove pre-2.12.4 wrappers
         File::Path::rmtree("$PrivateDir/wrappers");
         foreach (my ($key, $dir)=each %private_wrappers) {
            -d "$PrivateDir/$dir" || delete $private_wrappers{$key};
         }
      }
      my $seq;
      for ($seq=0; -d ($dir="$PrivateDir/wrappers.$seq"); ++$seq) {
         if (string_list_index([ values %private_wrappers ], "wrappers.$seq") < 0) {
            # stray orphaned directory
            File::Path::rmtree($dir);
            last;
         }
      }
      $private_wrappers{$InstallArch}="wrappers.$seq";
      $custom_handler->set('%private_wrappers');
   } else {
      $dir=Tempfile->new_dir;
   }
   File::Path::mkpath($dir);
   $private_wrapper_ext=new Extension($dir, "private:");
   $private_wrapper_ext->VCS=new SourceVersionControl::None($dir);
   File::Path::mkpath("$dir/build.$Arch/lib");
   write_private_wrapper_conf_make();
   ensure_update_private_wrapper();
}

sub ensure_update_private_wrapper {
   if ($PrivateDir && !$root->update_private_wrapper) {
      $root->update_private_wrapper=1;
      add AtEnd("Private:C++", sub { write_private_wrapper_meta();  write_private_wrapper_conf_make(1)});
   }
}

sub write_private_wrapper_meta {
   my ($F, $F_k)=new OverwriteFile($private_wrapper_ext->dir."/polymake.ext");
   print $F <<".";
# This pseudo-extension collects automatically generated C++/perl glue code
# used for computing complex expressions occurred in interactive sessions and user scripts.
# This extension is architecture-specific, it is linked to polymake clients installed at
#   $InstallArch
# It can be safely deleted at any time except during running polymake session.
.
   if (@{$private_wrapper_ext->requires}) {
      print $F "REQUIRE\n", map { ($_->URI, "\n") } @{$private_wrapper_ext->requires};
   }
   close $F;
}

# preserve_time_stamp =>
sub write_private_wrapper_conf_make {
   my ($preserve_time_stamp)=@_;
   my $filename=$private_wrapper_ext->dir."/build.$Arch/conf.make";
   $preserve_time_stamp &&= -f $filename && (stat _)[9];
   my ($F, $F_k)=new OverwriteFile($filename);
   print $F "include $InstallArch/conf.make\n",
            "RequireExtensions := ", join(" ", map { $_->dir } uniq(map { ($_, @{$_->requires}) } @{$private_wrapper_ext->requires})), "\n";
   close $F;
   undef $F_k;
   if ($preserve_time_stamp) {
      utime(undef, $preserve_time_stamp, $filename);
   }
}
#######################################################################################
sub mark_extension_for_rebuild {
   my ($ext_dir)=@_;
   my $t=time+1;
   foreach (glob("$ext_dir/build.$Arch/apps/*")) {
      s{/build.$Arch/apps/}{/};
      $wrapper_updated{$_}=$t;
   }
   $custom_handler->set('%wrapper_updated');
}
#######################################################################################
sub create_source_file {
   my ($file, $vcs, $template, %vars)=@_;
   $template="$InstallTop/lib/core/skel/$template";
   local $/;
   open my $tf, $template
     or die "Can't read the source file template $template: $!\n",
            "Unable to maintain persistent C++ bindings!\n";
   my $content=<$tf>;  close $tf;

   $file =~ $directory_re;
   -d $1 or $vcs->make_dir("$1");

   # don't parse the copyright notice at the file top
   $content =~ /^-{2,}\n.*\n/m;
   $content = $` . $& . eval "<<\".#.#.#.\";\n$'.#.#.#.\n";
   if ($@) {
      die "Error processing the source file template $template: $@";
   }
   # kill superfluous empty lines
   $content =~ s/^\n{2,}/\n/m;

   if (open $tf, ">", $file) {
      dbg_print( "Creating C++ source file $file" ) if $Verbose::cpp;
      print $tf $content;
      close $tf;
   } else {
      die "Can't create the C++ source file $file: $!\n",
          "Unable to maintain persistent C++ bindings!\n";
   }

   $vcs->add_file($file) if $vcs;
}
#######################################################################################

my $cpp_include_re=qr{^\s*\#\s*include\s+[<"](.*?)[">]};
my $cpp_start_namespace_re=qr{^\s*namespace +polymake\b};

sub modify_source_file {
   my ($what)=@_;
   my $filename=$what->{filename};
   open my $F, $filename;
   my ($Fnew, $F_k)=new OverwriteFile($filename);
   local $_;
   my $inst_count=0;
   my $inst_max;

   # copy the preamble till the first #include or namespace opening
   # add extension directories if needed
   my $extensions=$what->{extensions};
   while (defined ($_=<$F>) && ! m{(?:$cpp_include_re|$cpp_start_namespace_re)}o) {
      if (m{^///.*max\.instances=(\d+)}) {
         $inst_max=$1;
      } elsif ($extensions && m{^//\s*ext:\s*(\S+)\s*$}) {
         delete $extensions->{$1};
      }
      print $Fnew $_;
   }
   if ($extensions) {
      print $Fnew map { "// ext: $_\n" } keys %$extensions;
   }

   if (my @includes=@{$what->{includes}}) {
      # merge the include directives avoiding duplicates
      while ($_ =~ $cpp_include_re) {
         push @includes, $1;
         $_=<$F>;
      }
      print $Fnew "#include \"$_\"\n" for (sorted_uniq(sort @includes));
      print $Fnew "\n" if $_ ne "\n";
   }

   do {
      print $Fnew $_;
   } while (! /Automatically generated contents follow/ && defined ($_=<$F>));

   # insert new template declarations
   if (defined (my $declarations=$what->{declarations})) {
      print $Fnew @$declarations;
   }

   # delete obsolete template declarations and duplicates
   my ($obsolete, $dup_func, $dup_class)=@$what{qw(obsolete dup_func dup_class)};
   if ($obsolete // $dup_func // $dup_class) {
      my %seen;
      $obsolete //= { };
      $dup_func //= { };
      $dup_class //= { };
      while (<$F>) {
         my $theader;
         if (/^\s*template\s+</) {
            $theader=$_;
            $_=<$F>;
         }
         if (/^\s*FunctionInterface4perl\s*\(\s* ($id_re) /xo) {
            if (defined (my $auto_func=$obsolete->{$1})) {
               $seen{$auto_func}|=1;
               do { $_=<$F> } while (/\S/);
               next;
            }

         } elsif (!defined($theader) and
                  my ($op, $wrapper_name)=/^\s* (?:(?:Function|(Operator)|Method) (?:CrossApp)? Instance4perl) \s*\(\s* ($id_re) /xo) {

            if (defined (my $auto_func=$obsolete->{$wrapper_name})) {
               $seen{$auto_func} |= 2+defined($op);
               next;
            } elsif (defined (my $inst_list=$dup_func->{$wrapper_name})) {
               my $i=0;
               foreach my $inst (@$inst_list) {
                  last if $. == $inst->descr->source_line;
                  ++$i;
               }
               if ($i <= $#$inst_list) {
                  splice @$inst_list, $i, 1;
                  delete $dup_func->{$wrapper_name} if @$inst_list==0;
                  next;
               }
            }
            ++$inst_count;

         } elsif (!defined($theader) and
                  /^\s*Class4perl \s*\(\s* "($qual_id_re)"/xo) {

            if (defined (my $inst=$dup_class->{$1})) {
               if ($. == $inst->source_line) {
                  delete $dup_class->{$1};
                  next;
               }
            }
            ++$inst_count;

         } elsif (/Automatically generated contents end here/) {
            last;
         }
         print $Fnew $theader, $_;
      }

      if (my @missed=grep { $seen{$_}!=3 } values %$obsolete  or  keys %$dup_func  or  keys %$dup_class) {
         warn_print( "Wrapper declarations and/or instantiations to be deleted from the source file $filename have not been found there:\n",
                     (map { ("  ", $_->wrapper_name, "\n") } @missed),
                     (map { ("  ", $_->is_instance_of->wrapper_name, "\n") } map { @$_ } values %$dup_func),
                     (map { ("  $_\n") } keys %$dup_class),
                     "Please edit the source code manually\n" );
      }
   } else {
      while (defined ($_=<$F>) && ! /Automatically generated contents end here/) {
         ++$inst_count if $inst_max && /^\s* (?: (?:Function|Operator|Method) (?:CrossApp)? Instance4perl | Class4perl)/x;
         print $Fnew $_;
      }
   }

   # insert new instances
   if (defined (my $instances=$what->{instances})) {
      $inst_count+=@$instances;
      print $Fnew @$instances;
   }

   # copy the trailer
   do { print $Fnew $_ } while (<$F>);
   close $F; close $Fnew;

   if ($inst_max && $inst_count >= $inst_max) {
      # maximal length reached

      # compose a new unique name
      my ($dir, $stem, $suffix)= $filename =~ m{^ (.*?) / ([^/]+) (\. [^/.]+) $ }x;
      # don't split a split-off, however!
      unless ($stem =~ /-\d+$/ && -f "$dir/$`$suffix") {
         my $ord=1;
         my $new_name;
         ++$ord while -e ($new_name="$dir/$stem-$ord$suffix");

         $what->{vcs}->copy_file($filename, $new_name);

         # store the current data under the new name
         $F_k->dst=$new_name;
         undef $F_k;

         # filter out all automatic contents from the original file; preserve the #defines and settings
         open $F, $new_name;
         ($Fnew, $F_k)=new OverwriteFile($filename);

         while (defined ($_=<$F>) && ! /(?:$cpp_include_re|$cpp_start_namespace_re)/o) {
            print $Fnew $_;
         }

         while ($_ =~ $cpp_include_re) {
            print $Fnew $_ if $1 eq "polymake/client.h";
            $_=<$F>;
         }

         do {
            print $Fnew $_;
         } while (! /Automatically generated contents follow/ && defined ($_=<$F>));

         while (<$F>) {
            last if /Automatically generated contents end here/;
         }

         do { print $Fnew $_ } while (<$F>);
         close $F; close $Fnew;
      }
   }
}
#######################################################################################
# detect and remove wrappers entangled with a obliterated extension
sub filter_out_extension {
   my ($ext_dir, $obliterated)=@_;
   foreach my $app_dir (glob("$ext_dir/apps/*")) {
      my ($app_name)= $app_dir =~ $filename_re;
      my ($removed, $survived);
      opendir my $SRCDIR, "$app_dir/src/perl";
      while (my $entry=readdir $SRCDIR) {
         if (-f (my $src="$app_dir/src/perl/$entry")) {
            my $kill;
            open my $SRC, $src;
            local $_;
            while (<$SRC>) {
               if (m{^//\s*ext:\s*$obliterated\s*$}) {
                  $kill=1; last;
               } elsif (m{(?:$cpp_include_re|$cpp_start_namespace_re)}o) {
                  last;
               }
            }
            close $SRC;
            if ($kill) {
               unlink $src or die "can't remove obsolete wrappers $src: $!\n";
               $removed=1;
            } else {
               $survived=1;
            }
         }
      }
      closedir $SRCDIR;
      if ($removed) {
         if ($survived) {
            $wrapper_updated{"$ext_dir/$app_name"}=time+1;
         } else {
            delete $wrapper_updated{"$ext_dir/$app_name"};
            File::Path::rmtree("$ext_dir/build.$Arch/apps/$app_name");
            File::Path::rmtree("$ext_dir/apps/$app_name");
         }
         unlink glob("$dir/build.$Arch/lib/${app_name}{,-*}.$DynaLoader::dl_dlext");
         $custom_handler->set('%wrapper_updated');
      }
   }
}
#######################################################################################

sub gen_proxy_op {
   eval <<".";
sub {
   if (\$_[2]) {
      \$_[1] $_ convert_to_$_[0](\$_[0])
   } else {
      convert_to_$_[0](\$_[0]) $_ \$_[1]
   }
}
.
}

#######################################################################################

package Polymake::Core::CPlusPlus::Iterator;

use overload '++' => \&incr, 'bool' => \&not_at_end,
             '${}' => \&deref_to_scalar, '@{}' => \&deref, '%{}' => \&deref,
             '=' => \&overload_clone_op,
             fallback => 0, nomethod => \&missing_op, '""' => sub { &not_at_end ? "ITERATOR" : "VOID ITERATOR" };

#######################################################################################

package Polymake::Core::CPlusPlus::NumProxy;

use overload '0+' => \&convert_to_serialized,
             neg => sub { - convert_to_serialized($_[0]) },
             abs => sub { abs( convert_to_serialized($_[0]) ) },
             (map { $_ => gen_proxy_op("serialized") } qw(+ - * / % ** <=>));

#######################################################################################

package Polymake::Core::CPlusPlus::BoolProxy;

use overload 'bool' => \&convert_to_serialized,
             (map { $_ => gen_proxy_op("serialized") } qw(& | ^));

#######################################################################################

package Polymake::Core::CPlusPlus::StringProxy;

use overload '""' => \&convert_to_string,
             (map { $_ => gen_proxy_op("string") } qw(x . cmp));

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
