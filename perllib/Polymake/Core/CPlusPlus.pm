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

package Polymake::Core::CPlusPlus;

declare ($private_wrapper_ext, %private_wrappers);

# controls whether and where new cpperl glue definitions are generated:
#   "private"   - in "wrappers" folder in user's settings area
#   "shared"    - in the source code tree
#   "none"      - completely forbidden
declare $code_generation = "private";

# path prefix for new and updated cpperl files
# it should point to a unique per-process location
# if several processes are run in parallel
declare $cpperl_src_root;

my ($BigObject_cpp_options, $BigObjectArray_cpp_options);
my $move_private_instances;

use Polymake::Enum FuncFlag => {
   arg_is_const_ref => 0,
   arg_is_lval_ref => 0x1,
   arg_is_univ_ref => 0x2,
   arg_is_const_or_rval_ref => 0x3,
   arg_is_wary => 0x4,
   arg_has_anchor => 0x8,
   arg_is_numeric => 0x10,
   is_method => 0x20,
   is_static => 0x40,
   returns_list => 0x80,
   returns_void => 0x100,
   returns_lvalue => 0x200,
   args_are_swapped => 0x400
};

#######################################################################################
package Polymake::Core::CPlusPlus::FuncDescr;

use Polymake::Struct (
   '$name',
   '$cpperl_file',
   '$arg_types',    # for regular functions: total number of arguments
   '$cross_apps',
   '$auto_func',
   '$return_type',
   '$extension',
);

sub register {
   my ($self, $auto_func, $sub)=@_;
   my $last=$#{$self->arg_types};
   if ($last>=0) {
      my $inst_cache=$auto_func->inst_cache;
      my $extra=$auto_func->explicit_template_params;
      for (my $tp=0; $tp<$last; ++$tp) {
         if ($tp==$extra && defined($auto_func->class)) {
            # skip the "void" typeid
            next;
         } elsif ($tp>=$extra  &&  $auto_func->arg_descrs->[$tp-$extra]->flags & FuncFlag::arg_is_univ_ref) {
            $inst_cache=($inst_cache->{$self->arg_types->[$tp]}->[$self->arg_types->[$tp]] //= { });
         } else {
            $inst_cache=($inst_cache->{$self->arg_types->[$tp]} //= { });
         }
      }
      ((($last>=$extra  &&  $last-$extra <= $#{$auto_func->arg_descrs}  &&
        $auto_func->arg_descrs->[$last-$extra]->flags & FuncFlag::arg_is_univ_ref)
       ? $inst_cache->{$self->arg_types->[$last]}->[$self->arg_types->[$last]]
       : $inst_cache->{$self->arg_types->[$last]}) &&= return)=$sub;
   } else {
      ($auto_func->inst_cache &&= return)=$sub;
   }
}

sub suspend {
   my ($self, $perApp)=@_;
   my (@apps, @missing_apps);
   foreach my $app_name (@{$self->cross_apps}) {
      if (defined (my $app=lookup Application($app_name))) {
         push @apps, $app;
      } else {
         push @missing_apps, $app_name;
      }
   }
   if (@missing_apps) {
      push @{Application::SuspendedItems::add($perApp->application, $self->extension, @missing_apps)->functions}, $self;
      1
   } else {
      $self->cross_apps=\@apps;
      0
   }
}

#######################################################################################
package Polymake::Core::CPlusPlus::AutoFunction::ArgDescr;

use Polymake::Struct (
   [ new => '$$;$$' ],
   [ '$arg_index' => '#1' ],       # index of function argument (as passed in @_) to derive the type from
   [ '$flags' => '#2' ],           # arg_is_lval_ref, arg_is_univ_ref, arg_is_wary, arg_is_numeric, is_static
   [ '$pattern' => '#3' ],         # fixed type name or a pattern with '%1' substitution term
   [ '$conv_to' => '#4' ],         # index of another (explicitly given) type to be coerced to
);

#######################################################################################
package Polymake::Core::CPlusPlus::AutoFunction;

use Polymake::Struct (
   [ new => '$$$%' ],
   [ '$name' => '#%' ],
   [ '$perl_name' => '#1' ],
   [ '$caller_kind' => 'undef' ],
   [ '$wrapper_name' => '#1' ],
   [ '$flags' => '#2' ],
   [ '$application' => '#3' ],
   [ '$cross_apps' => '$Application::cross_apps_list' ],
   [ '$extension' => '$Extension::loading' ],
   [ '@include' => '#%' ],
   [ '$in_cc_file' => 'undef' ],
   [ '$returns' => '#%', default => 'undef' ],
   [ '$explicit_template_params' => '#%', default => '0' ],
   [ '$class' => '#%', default => 'undef' ],
   '@arg_descrs',
   [ '$total_args' => '0' ],
   '%inst_cache',
);

sub complain_ro_violation {
   my ($self, $arg_no)=@_;
   croak( "Attempt to modify a read-only C++ object passed as argument $arg_no to function ", $self->perl_name );
}

sub twin_key {
   my ($self)=@_;
   defined($self->in_cc_file) ? $self->application->name.":wrap-".(split /\./, $self->in_cc_file, 2)[0] : $self->application->name
}

#######################################################################################
sub parse_arg_attrs {
   my ($attrs, $for_what)=@_;
   my $arg_flags=0;
   while ($attrs =~ /\G : \s* ($id_re) \s*/xgc) {
      if ($1 eq "wary") {
         $arg_flags |= FuncFlag::arg_is_wary;
      } elsif ($1 eq "anchor") {
         $arg_flags |= FuncFlag::arg_has_anchor;
      } elsif ($1 eq "num") {
         if ($for_what eq 'this') {
            croak( "num attribute is not applicable to `this' argument" );
         }
         $arg_flags |= FuncFlag::arg_is_numeric;
      } elsif ($1 eq "static") {
         if ($for_what eq 'operator') {
            croak( "operator arguments can't be declared `static'" );
         }
         $arg_flags |= FuncFlag::is_static;
      } else {
         croak( "unknown argument attribute: $1" );
      }
   }
   if (pos($attrs) < length($attrs)) {
      if ($attrs =~ /\G & ( & (\s*const)?)? $/xg) {
         if ($1) {
            if ($arg_flags & FuncFlag::arg_has_anchor) {
               croak( "superfluous argument attribute 'anchor': implied by universal reference" );
            }
            $arg_flags |= $2 ? FuncFlag::arg_is_const_or_rval_ref | FuncFlag::arg_has_anchor
                             : FuncFlag::arg_is_univ_ref          | FuncFlag::arg_has_anchor;
         } else {
            $arg_flags |= FuncFlag::arg_is_lval_ref;
         }
      } else {
         croak( "invalid argument attribute: '$attrs'" );
      }
   }
   $arg_flags
}
#######################################################################################
sub prepare_arg_list {
   my ($self, $arg_types, $arg_attrs) = @_;
   my ($min, $max, @arg_types) = @$arg_types;
   my $perl_arg_index = $self->total_args;
   my $static_seen = $enable_plausibility_checks && $perl_arg_index == 1 && ($self->arg_descrs->[0]->flags & FuncFlag::is_static);

   my $sig_index = 0;
   foreach my $arg_attr (@$arg_attrs) {
      my $arg_type = $arg_types[$sig_index];
      my ($is_repeated, $wrapper_suffix, $cast_to_target_type);
      if (ref($arg_type) eq "ARRAY") {
         $is_repeated = $arg_type->[1] eq "+";
         $arg_type = $arg_type->[0];
      }
      my $create_descr = $arg_attr =~ s/^\*//;
      my $arg_flags = parse_arg_attrs($arg_attr);

      if ($enable_plausibility_checks) {
         if ($arg_flags & FuncFlag::is_static) {
            if ($static_seen++) {
               croak( "multiple arguments with `static' attribute" );
            }
            if ($sig_index) {
               croak( "temporary implementation restriction: argument with `static' attribute must be the first one" );
            }
            if ($arg_flags != FuncFlag::is_static) {
               croak( "`static' attribute is mutually exclusive with any other argument attributes" );
            }
            if ($arg_type eq '$' && !$create_descr) {
               croak( "static attribute must be assigned to a typed argument or a type wildcard" );
            }
         }
         if ($arg_flags & FuncFlag::arg_is_const_or_rval_ref and $sig_index >= $min) {
            croak( "a reference argument can't be optional" );
         }
      }

      if (is_object($arg_type)) {
         my $opts = $arg_type->cppoptions;
         if ($arg_type->abstract) {
            if (defined($opts) && $opts->builtin) {
               #  BigObject or BigObjectArray
               $wrapper_suffix = $opts->builtin;
            } else {
               my $type_param_index = $arg_type->type_param_index;
               if (defined($type_param_index)) {
                  $cast_to_target_type = $type_param_index;
                  $wrapper_suffix = "C$type_param_index";
               }
               $create_descr = true;
               undef $arg_type;
            }
         } elsif ($opts) {
            if ($opts->builtin) {
               if ($enable_plausibility_checks and $arg_flags & FuncFlag::arg_is_const_or_rval_ref) {
                  if ($opts->builtin eq "B") {
                     croak( "superfluous argument attribute: objects of type ", $arg_type->full_name, " are always passed by reference" );
                  } else {
                     croak( $arg_type->full_name, " is declared as perl built-in type and can't be passed to C++ code by reference" );
                  }
               }
               $wrapper_suffix = $opts->builtin;
               if ($opts->builtin ne "B") {
                  if ($is_repeated) {
                     $wrapper_suffix .= "_P";
                     undef $arg_type;
                  } else {
                     $arg_type = $opts->builtin;
                  }
                  $create_descr = true;
               }
            } else {
               undef $arg_type;
               $create_descr = true;
            }
         } else {
            croak( "class ", $arg_type->full_name, " has no C++ binding" );
         }

      } elsif ($arg_type eq '$') {
         undef $arg_type;
      } else {
         croak( "pure perl package '$arg_type' can't be passed to C++ subroutine" );
      }

      if ($create_descr) {
         push @{$self->arg_descrs}, new ArgDescr($perl_arg_index, $arg_flags, $arg_type, $cast_to_target_type);
         $self->wrapper_name .=  "." . ($wrapper_suffix || "X");
      } else {
         $self->wrapper_name .= "." . ($wrapper_suffix || "x");
      }
      $self->wrapper_name .= $arg_flags if $arg_flags;
      ++$perl_arg_index;
      ++$sig_index;
   }

   if ($max & Overload::SignatureFlags::has_keywords) {
      $self->wrapper_name .= ".o";
      ++$perl_arg_index;
   }
   $self->total_args = $perl_arg_index;
}
#######################################################################################
sub prepare {
   my ($self, $label, $arg_types, $arg_attrs, $pkg, $internal) = @_;
   $self->name ||= $self->perl_name;
   if (defined($label)) {
      $self->wrapper_name .= "#" . $label->full_name;
   }
   if ($self->explicit_template_params) {
      $self->wrapper_name .= ":T" . $self->explicit_template_params;
   }
   if ($self->flags & FuncFlag::returns_list) {
      $self->wrapper_name .= ":L";
   } elsif ($self->flags & FuncFlag::returns_void) {
      $self->wrapper_name .= ":V";
   } elsif ($self->flags & FuncFlag::returns_lvalue) {
      $self->wrapper_name .= ":F";
   } elsif (defined $self->returns) {
      $self->wrapper_name .= ":R_" . $self->returns;
   }

   if ($self->flags & FuncFlag::is_method) {
      my $this_attrs = shift @$arg_attrs;
      my $arg_flags = parse_arg_attrs($this_attrs, 'this');
      $self->flags |= $arg_flags & FuncFlag::is_static;

      if ($enable_plausibility_checks && !$internal) {
         my $proto = &{UNIVERSAL::can($pkg, "typeof_gen") || croak( "pure perl package $pkg has no C++ binding" )}(undef);
         if (defined(my $opts = $proto->cppoptions)) {
            if ($opts->builtin) {
               if ($opts->builtin eq "B") {
                  croak( "Can't declare methods with C++ binding for `big' objects or arrays thereof" );
               } elsif (not $arg_flags & FuncFlag::is_static) {
                  croak( "can't declare methods for C++ primitive type ", $opts->builtin );
               }
            }
         } else {
            croak( "class ", $proto->full_name, " has no C++ binding" );
         }
         if ($arg_flags & FuncFlag::is_static and $arg_flags != FuncFlag::is_static) {
            croak( "`static' attribute is mutually exclusive with any other argument attributes" );
         }
      }
      push @{$self->arg_descrs}, new ArgDescr(0, $arg_flags);
      $self->wrapper_name .= ":M" . ($arg_flags || "");
      $self->total_args = 1;

   } elsif (defined $self->class) {
      # free function on perl side implemented as a static method of a C++ class
      $self->flags |= FuncFlag::is_static;
   }

   prepare_arg_list($self, $arg_types, $arg_attrs);

   if ($self->flags & FuncFlag::is_static and defined($self->application) and $pkg ne $self->application->pkg) {
      my $app = $self->application->pkg;
      $pkg =~ s/^$app\:://;
      substr($self->wrapper_name, 0, 0) = $pkg . "::";
   }
   if ($self->explicit_template_params + @{$self->arg_descrs} == 0) {
      # no parameter-dependent instances at all
      undef $self->inst_cache;
   }

   # these names must correspond to macro names in perl/macros.h
   $self->caller_kind = $self->flags & FuncFlag::is_static ? "stat" : $self->flags & FuncFlag::is_method ? "meth" : "free";
}
#######################################################################################
sub prescribed_return_types {
   my ($self) = @_;
   if ($self->returns =~ /^($id_re)\s*<\s*($id_re(?:\s*,\s*$id_re)*)\s*>$/o) {
      my ($return_type, $value_types) = ($1, $2);
      [ $return_type, $value_types =~ m/($id_re)/go ]
   } else {
      $self->returns
   }
}
#######################################################################################
sub deduce_extra_params {
   my ($self, $args) = @_;
   @{namespaces::fetch_explicit_typelist($args)};
}

sub find_instance {
   my ($self, $args) = @_;
   my $instance = $self->inst_cache;
   if ($self->explicit_template_params) {
      foreach my $proto ($self->deduce_extra_params($args)) {
         $instance &&= $instance->{$proto->cpp_type_descr->typeid};
      }
   }
   foreach my $arg_descr (@{$self->arg_descrs}) {
      my $arg_no = $arg_descr->arg_index;
      my $lval_flag = $arg_descr->flags & FuncFlag::arg_is_const_or_rval_ref;
      my $typeid = get_magic_typeid($args->[$arg_no], $lval_flag);
      if (defined($typeid)) {
         $instance &&= $instance->{$typeid};
         if ($lval_flag) {
            if ($lval_flag >= FuncFlag::arg_is_univ_ref) {
               $instance &&= $instance->[$typeid];
            } elsif ($typeid != $lval_flag) {
               $self->complain_ro_violation($arg_no);
            }
         }
         next;
      }
      my $proto;
      if ($arg_descr->flags & FuncFlag::is_static) {
         $proto = $args->[$arg_no]->type;
      } else {
         $proto = guess_builtin_type($args->[$arg_no], $arg_descr->flags & FuncFlag::arg_is_numeric);
         if ($lval_flag == FuncFlag::arg_is_lval_ref) {
            croak( "Can't pass a primitive type ", $proto->full_name, " by reference to a C++ function" );
         }
      }
      my $descr = $proto->cppoptions->descr || provide_cpp_type_descr($proto);
      if (is_code($descr)) {
         $descr->();
         $descr = $proto->cppoptions->descr;
      }
      $instance &&= $instance->{$descr->typeid};
      if ($lval_flag >= FuncFlag::arg_is_univ_ref) {
         $instance &&= $instance->[$descr->typeid];
      }
   }
   $instance;
}
#######################################################################################
sub unimplemented_instance {
   croak( "internal error: unimplemented function instance registered for ", $_[0]->name );
}
#######################################################################################
package Polymake::Core::CPlusPlus::Constructor;

use Polymake::Struct (
   [ '@ISA' => 'AutoFunction' ],
   [ new => '$$' ],
   [ '$name' => '"operator new"' ],
   [ '$perl_name' => '"construct"' ],
   [ '$wrapper_name' => '"new"' ],
   [ '$flags' => '0' ],
   [ '$application' => 'undef' ],
   [ '$explicit_template_params' => '0' ],
   [ '$total_args' => '1' ],
);

sub new {
   my $self=&_new;
   # the proto object is passed as the first argument
   push @{$self->arg_descrs}, new ArgDescr(0, FuncFlag::is_static);
   prepare_arg_list($self, @_);
   $self
}

#######################################################################################
package Polymake::Core::CPlusPlus::SpecialOperator;

use Polymake::Struct (
   [ '@ISA' => 'AutoFunction' ],
   [ new => '$' ],
   [ '$name' => '"operator " . #1' ],
   [ '$wrapper_name' => '#1 . ":O"' ],
   [ '$flags' => 'FuncFlag::returns_void' ],
   [ '$extension' => 'undef' ],
   [ '$explicit_template_params' => '0' ],
   [ '$total_args' => '2' ],
);

sub new {
   my $self=&_new;
   push @{$self->arg_descrs}, new ArgDescr(0, FuncFlag::is_static), new ArgDescr(1, 0);
   $self;
}

sub unimplemented_instance {
   sub { croak( "invalid conversion from ", $_[1]->type->full_name, " to ", $_[0]->full_name ) }
}

#######################################################################################
package Polymake::Core::CPlusPlus::Operator;

use Polymake::Struct (
   [ '@ISA' => 'AutoFunction' ],
   [ new => '$$$$$;$' ],
   [ '$wrapper_name' => '#3' ],
   [ '$name' => '"operator " . #4' ],
   [ '$application' => 'undef' ],
   [ '$extension' => 'undef' ],
   '&code',
);

use Polymake::Struct (
   [ 'alt.constructor' => 'new_call_op' ],
   [ new => '$$$%' ],
   [ '$name' => '"operator ()"' ],
   [ '$perl_name' => '"cal"' ],
   [ '$wrapper_name' => '"cal:O"' ],
   [ '$explicit_template_params' => '0' ],
);

sub new {
   my $self=&_new;
   @{$self->arg_descrs} = map { new ArgDescr($self->total_args++, $_) } splice @_, 4;
   if ($self->total_args == 2) {
      if ($self->flags & FuncFlag::returns_lvalue) {
         # binary operation with assignment, operands swapping can't occur
         use namespaces::AnonLvalue;
         $self->code = sub { pop @_; &{ resolve_auto_function($self, \@_) } };

      } else {
         # binary operation, operands may be swapped
         my $swapped_arg_descrs=[ map { new ArgDescr($_, $self->arg_descrs->[1-$_]->flags | FuncFlag::args_are_swapped) } 0..1 ];
         $self->code = sub {
            local if (pop @_) {
               local swap @_, 0, 1;
               local ref $self->arg_descrs = $swapped_arg_descrs;
            }
            &{ resolve_auto_function($self, \@_) }
         };
      }
   } else {
      # unary operation
      no warnings 'void';
      use namespaces::AnonLvalue '$is_lvalue';
      my $is_lvalue = ($self->flags & FuncFlag::returns_lvalue) != 0;
      $self->code=sub { $is_lvalue; splice @_, -2; &{ resolve_auto_function($self, \@_) } };
   }

   $self;
}

sub make_wrapper_name {
   my $name=shift;
   "$name:O" . join("", map { ".X" . ($_ || "") } @_);
}

sub complain_ro_violation {
   my ($self)=@_;
   croak("Attempt to modify a read-only C++ object passed to ", $self->name);
}

#######################################################################################
package Polymake::Core::CPlusPlus::TypeDescr;

use Polymake::Struct (
   '$pkg',                         # reference to the stash, not the package name!
   '$vtbl',                        # vtbl should only be accessed in XS code
   '$cpperl_file',                 #
   '$typeid',                      # std::type_info name
   [ '$generated_by' => 'undef' ], # FuncDescr of a function created such an object if it does not correspond to any declared property type
                                   #     or typeid of a container/composite which this class belongs as element to
   [ '$source_name' => 'undef' ],
   [ '$include' => 'undef' ],
   [ '$cpperl_define_with' => 'undef' ],
   [ '$application' => 'undef' ],
   [ '$cross_apps' => 'undef' ],
   [ '$extension' => 'undef' ],
);

# Sometimes a TypeDescr object is being passed instead of PropertyType 0-th argument to a converting constructor.
# The following methods make it compatible to PropertyType to the extent sufficient for
# resolve_auto_function and new LackingFunctionInstance.

sub type { &{$_[0]->pkg->{type}}() }
sub cpp_type_descr { $_[0] }
sub name { &type->name }

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
   [ '$variadic' => '#%', default => '0' ],
   [ '$fields' => '#%', default => 'undef' ],
   [ '$finalize_with' => 'undef' ],
   [ '$descr' => '#%', default => 'undef' ],     # cached for builtins and persistent types
   [ '$cpperl_define_with' => 'undef' ],
   [ '$application' => '#1' ],
   [ '$cross_apps' => 'undef' ],
   [ '$extension' => '#2' ],
);

sub clone {
   my ($self, $proto) = @_;
   my $clone = inherit_class([ @$self ], $self);
   weak($clone->finalize_with = $proto);
   $clone
}

sub define_field_accessors {
   my ($self, $pkg) = @_;
   if (defined($self->fields)) {
      my $i = 0;
      foreach my $field_name (@{$self->fields}) {
         define_function($pkg, $field_name, Struct::create_accessor($i++, \&composite_access));
      }
   }
}
#######################################################################################
# bind all BigObject types involved in template arg deduction to the C++ Object class

package Polymake::Core::CPlusPlus::BigObjectOptions;
use Polymake::Struct (
   [ '@ISA' => 'Options' ],
);

sub finalize {
   croak("Can't embed a BigObject into a C++ data structure: use SCALAR instead");
}

sub descr : lvalue {
   croak("Can't pass a BigObject to a C++ function expecting a native C++ data type");
   $BigObject_cpp_options
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
package Polymake::Core::CPlusPlus::SharedModule;

use Polymake::Struct (
   [ new => '$;$' ],
   '$so_name',
   [ '$so_timestamp' => 'undef' ],
);

sub new {
   my $self = &_new;
   my ($perApp, $extension) = @_;
   my $app = $perApp->application;
   my $app_name = $app->name;
   my $host_extension = $extension // $app->origin_extension;
   my $build_dir = $host_extension ? $host_extension->build_dir : $InstallArch;
   $self->so_name = "$build_dir/lib/$app_name.$DynaLoader::dl_dlext";

   my $mod_size;
   if (-e $self->so_name) {
      ($self->so_timestamp, $mod_size) = (stat _)[9,7];

   } elsif (is_mutable_location($build_dir)) {
      compile($build_dir);
      ($self->so_timestamp, $mod_size) = (stat $self->so_name)[9,7];

   } else {
      die "Corrupt or incomplete installation: shared module ", $self->so_name, " missing\n";
   }

   # empty shared module is created when the application does not contain any client code
   if ($mod_size) {
      load_shared_module($self->so_name);
   }

   $self
}

#######################################################################################
package Polymake::Core::CPlusPlus::perApplication;

use Polymake::Struct (
   [ 'new' => '$' ],
   [ '$application' => 'weak(#1)' ],
   [ '$embedded_rules' => 'undef' ],  # transformed source lines whilst loading rulefiles
   [ '$loading_embedded_rules' => '0' ],
   '%shared_modules',           # "top_dir" => SharedModule
   [ '$generator' => 'undef' ],
);
#######################################################################################
sub start_loading {
   my ($self, $extension) = @_;
   my $embedded_items_key = $self->application->name;
   my $dummy_name;
   if ($extension && $extension->is_bundled) {
      $embedded_items_key .= ":" . $extension->short_name;
      $dummy_name=$embedded_items_key;
   } elsif (defined(my $shared_mod = new SharedModule($self, $extension))) {
      $self->shared_modules->{$extension ? $extension->dir : $self->application->installTop} = $shared_mod;
      $dummy_name = $shared_mod->so_name;
   }
   my $embedded_rules = $root->embedded_rules->{$embedded_items_key};
   if (defined($embedded_rules) && @$embedded_rules) {
      $self->embedded_rules = $embedded_rules;
      do "c++:1:$dummy_name";
      $#$embedded_rules = -1;
      if ($@) {
         undef $self->embedded_rules;
         die "Error in rules embedded in a C++ client:\n$@";
      }
   }
}
#######################################################################################
sub raw_embedded_rules {
   my ($self, $extension) = @_;
   my @buffer;
   my $cur_file = "";
   my $abs_path = (defined($extension) ? $extension->app_dir($self->application) : $self->application->top) . "/src/";
   foreach (@{$self->embedded_rules}) {
      if (my ($linecmd, $file) = /^(\#line\s+\d+)\s+"(.*)"/) {
         if ($file eq $cur_file) {
            # perl seems confused when it encounters the same source file in the consecutive lines
            push @buffer, "$linecmd\n";
         } else {
            if ($file =~ /\.hh?$/i) {
               die "embedded rules defined in a header file $abs_path$file\n";
            }
            $linecmd .= qq{ "$abs_path$file"\n};
            if (@buffer) {
               # previous source file finished
               push @buffer, "} {\n", $linecmd;
            } else {
               # first source file started
               push @buffer, "{\n", $linecmd;
            }
            $cur_file=$file;
         }
      } else {
         do {
            s{^.*(?:\n|\Z)}{}m;
            push @buffer, $&;
         } while (length($_));
      }
   }
   # close the last source file
   if (@buffer) {
      push @buffer, "}\n";
   }
   undef $self->embedded_rules;
   \@buffer
}
#######################################################################################
sub end_loading {
   my ($self, $extension) = @_;
   if (defined($self->embedded_rules)) {
      if ($Verbose::rules > 1) {
         dbg_print( "reading rules embedded in C++ clients from ",
                    $self->shared_modules->{$extension && !$extension->is_bundled ? $extension->dir : $self->application->installTop}->so_name );
      }
      local scalar $self->loading_embedded_rules = 1;
      do "c++:2:";
      undef $self->embedded_rules;
      if ($@) {
         die "Error in rules embedded in a C++ client:\n$@";
      }
   }

   my $embedded_items_key = $self->application->name;
   if ($extension && $extension->is_bundled) {
      $embedded_items_key .= ":" . $extension->short_name;
   }
   bind_functions($self, $extension, $root->functions->{$embedded_items_key});

   my $duplicates = $root->duplicate_class_instances->{$embedded_items_key};
   if (defined($duplicates) && @$duplicates) {
      push @{$self->cpperl_generator->instances_to_remove},
           map { $_->extension = $extension; $_ } splice @$duplicates;
   }
}
#######################################################################################
sub load_suspended {
   my ($self, $suspended)=@_;
   if (@{$suspended->embedded_rules}) {
      if ($Verbose::rules>1) {
         dbg_print( "reading cross-application rules embedded in C++ clients from ",
                    $self->shared_modules->{$suspended->extension && !$suspended->extension->is_bundled ? $suspended->extension->dir : $self->application->installTop}->so_name );
      }
      $self->embedded_rules = $suspended->embedded_rules;
      local scalar $self->loading_embedded_rules = 1;
      do "c++:3:";
      undef $self->embedded_rules;
      if ($@) {
         die "Error in rules embedded in a C++ client:\n$@";
      }
   }
   bind_functions($self, $suspended->extension, $suspended->functions);
}
#######################################################################################
sub load_private_wrapper {
   my ($self) = @_;
   if (defined($private_wrapper_ext) &&
       -d $private_wrapper_ext->dir."/apps/".$self->application->name) {
      local if ($code_generation eq "shared") {
         local scalar $move_private_instances = 1;
         local scalar $root->type_descr = $root->private_classes;
      }
      if (defined(my $shared_mod = new SharedModule($self, $private_wrapper_ext))) {
         $self->shared_modules->{$private_wrapper_ext->dir} = $shared_mod;
         local $Extension::loading = $private_wrapper_ext;
         $self->end_loading($private_wrapper_ext);
      }
   }
}
#######################################################################################
my %builtin2proxy=( Int => 'NumProxy', double => 'NumProxy', 'std::string' => 'StringProxy', bool => 'BoolProxy' );

sub add_type {
   my ($self, $proto) = splice @_, 0, 2, $_[0]->application, $Extension::loading;
   my $opts = $proto->cppoptions = new Options(@_);
   if ($opts->special) {
      $opts->name ||= $opts->special || $proto->name;
      provide_cpp_type_descr($proto);

   } elsif ($opts->builtin) {
      if ($opts->builtin eq "enum") {
         $opts->name ||= $proto->name;
         my $app_symtab = get_symtab($self->application->pkg);
         while (my ($name, $glob) = each %{get_symtab($proto->pkg)}) {
            if (is_constant_sub($glob)) {
               my $val = $glob->();
               namespaces::declare_const_sub($app_symtab, $name);
               local caller $app_symtab;
               constant->import($name, bless \$val, $proto->pkg);
            }
         }
      } else {
         $opts->name ||= $opts->builtin;
         $root->builtins->{$opts->builtin} = $proto;
         if (defined(my $proxy_class = $builtin2proxy{$opts->builtin})) {
            no strict 'refs';
            push @{$proto->pkg."::ISA"}, "Polymake::Core::CPlusPlus::$proxy_class";
            if ($opts->builtin eq "Int") {
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
   my ($self, $generic_proto) = splice @_, 0, 2, $_[0]->application, $Extension::loading;
   my $opts = new Options(@_);
   croak( "parameterized type can't be declared as a C++ built-in type" )
     if $opts->builtin;

   my $super_pkg = $generic_proto->pkg;
   $generic_proto->cppoptions = $opts;
   if (!is_code($opts->name)) {
      if ($opts->name =~ /^(?: [\w:]+:: )?$/x) {
         $opts->name .= ($super_pkg =~ /([^:]+)$/)[0];
      }
   }
   if ($opts->template_params eq "*") {
      $opts->default_constructor = "";
   } elsif (!exists $root->templates->{$super_pkg}) {
      require Polymake::Core::CPlusPlusGenerator;
      generate_lacking_type($generic_proto);
   }
   if ($super_pkg =~ /::(array|map|pair)$/i) {
      $root->builtins->{lc($1)} ||= $super_pkg;
      if (lc($1) eq "array") {
         bless $opts, "Polymake::Core::CPlusPlus::OptionsForArray";
      }
   }
   define_constructors($generic_proto);
   $opts->define_field_accessors($super_pkg);
   $generic_proto;
}
#######################################################################################
sub add_template_instance {
   my ($self, $proto, $generic_proto, $nested_instantiation)=@_;
   $proto->cppoptions=$generic_proto->cppoptions->clone($proto);
   if (!$proto->abstract && !$proto->cppoptions->builtin && $proto->cppoptions->template_params ne "*" &&
       $proto->cppoptions->name ne "typeid") {
      if ($nested_instantiation && !exists $root->classes->{$proto->pkg}) {
         $proto->cppoptions->descr = sub { create_methods($self, $proto, $generic_proto->cppoptions) };
         $proto->construct = sub : method {
            my $proto = shift;
            if (is_code($proto->cppoptions->descr)) {
               $proto->cppoptions->descr->();
            }
            $proto->construct = \&PropertyType::construct_object;
            $proto->construct->(@_);
         };
         $proto->serialize = sub : method {
            my $proto = shift;
            if (is_code($proto->cppoptions->descr)) {
               $proto->cppoptions->descr->();
            } else {
               croak( "internal error: ", $proto->full_name, "::serialize was not set" );
            }
            $proto->serialize->(@_);
         };
      } else {
         create_methods($self, $proto, $generic_proto->cppoptions);
      }
   }
   $proto
}
#######################################################################################
sub add_auto_function {
   my ($self, $name, $label, $ext_code, $arg_types, $arg_attrs, $func_attrs, $options)=@_;
   my ($pkg, $srcfile)=caller;
   my $application=$self->application;
   my $flags=0;
   if (delete $func_attrs->{method}) {
      if (exists $options->{explicit_template_params}) {
         croak( "Methods with explicit template parameters are not supported yet" );
      }
      $flags |= FuncFlag::is_method;
   }
   my $invalidate_on_change;
   if (defined (my $returns=delete $func_attrs->{returns})) {
      if ($returns eq '@' || $returns eq 'list') {
         if (exists $options->{regular}) {
            croak( "list return can't be enforced for a non-template C++ function; use ListReturn as the return value instead" );
         }
         $flags |= FuncFlag::returns_list;
      } elsif ($returns eq 'void') {
         $flags |= FuncFlag::returns_void;
      } elsif ($returns eq '&' || $returns eq 'lvalue') {
         $flags |= FuncFlag::returns_lvalue;
      } elsif ($returns eq 'cached') {
         $invalidate_on_change=defined($label);
      } else {
         $options->{returns}=$returns;
      }
   }

   my $operator = delete $func_attrs->{operator};
   if (keys %$func_attrs) {
      croak("Unknown attribute", keys(%$func_attrs)>1 && "s", " for a C++ function: ", join(", ", keys %$func_attrs));
   }

   my ($auto_func, $regular_index);
   if ($flags & FuncFlag::is_method and $options->{name} eq "()") {
      if ($options->{explicit_template_params}) {
         croak( "C++ function call operator () can't have explicit type parameters" );
      }
      delete $options->{name};
      $auto_func = new_call_op Operator($name, $flags, $application, $options);
   } else {
      $regular_index = delete $options->{regular};
      $auto_func = new AutoFunction($name, $flags, $application, $options);
   }

   if ($self->loading_embedded_rules) {
      ($auto_func->in_cc_file)= $srcfile =~ $filename_re;
   }

   my ($code, @overload_options);
   if (defined $ext_code) {
      namespaces::fall_off_to_nextstate($ext_code);
   }

   if (defined $regular_index) {
      $auto_func->name ||= $name;
      my $descr=$root->regular_functions->[$regular_index];
      $auto_func->arg_descrs=0;
      $auto_func->total_args=$descr->arg_types;
      my $wrapper=create_function_wrapper($descr, get_symtab($application->pkg), $auto_func->total_args, $auto_func->prescribed_return_types);
      $descr->auto_func=$auto_func;
      $auto_func->inst_cache=$code= defined($ext_code) ? sub { return &$ext_code; &$wrapper; } : $wrapper;

   } else {
      croak( "C++ function template without signature" ) unless defined $arg_types;
      $auto_func->prepare($label, $arg_types, $arg_attrs, $pkg);
      check_twins($auto_func);
      {
         no warnings 'void';
         use namespaces::AnonLvalue '$is_lvalue';
         my $is_lvalue= ($flags & FuncFlag::returns_lvalue) != 0;
         $code= defined($ext_code)
                ? sub { $is_lvalue;
                        return &$ext_code;
                        &{resolve_auto_function($auto_func, \@_)}
                  }
                : sub { $is_lvalue;
                        &{resolve_auto_function($auto_func, \@_)}
                  };
      }
      if ($invalidate_on_change) {
         @overload_options=(invalidate_on_change => 1);
      }
      if ($flags & FuncFlag::is_method) {
         set_method($code);
      }
   }

   if (defined $operator) {
      add_as_operator($self, $pkg->self(), $auto_func, $code, $operator);
   }
   if (defined $arg_types) {
      ( $name, $label, $code, $arg_types, @overload_options )
   } else {
      define_function($pkg, $name, $code);
      ()
   }
}
#######################################################################################
sub add_constructor {
   my ($self, $name, $label, $ext_code, $arg_types, $arg_attrs, $func_attrs)=@_;
   my $flags = FuncFlag::is_method;
   my $const_creation=delete $func_attrs->{const_creation};
   if (keys %$func_attrs) {
      croak("Unknown attribute", keys(%$func_attrs)>1 && "s", " for a C++ constructor: ", join(", ", keys %$func_attrs));
   }
   my $auto_func=new Constructor($arg_types, $arg_attrs);
   check_twins($auto_func);

   my $code;
   if (defined $ext_code) {
      namespaces::fall_off_to_nextstate($ext_code);
      $code = sub : method {
                 return &$ext_code;
                 &{resolve_auto_function($auto_func, \@_)}
              };
   } else {
      $code = sub : method { &{resolve_auto_function($auto_func, \@_)} };
   }

   if (defined $const_creation) {
      my $pkg=caller;
      $const_creation =~ s/\s+$//;
      namespaces::intercept_operation($self->application->pkg, $const_creation, $code, $pkg->type);
   }
   ( $name, $label, $code, $arg_types )
}
#######################################################################################
my %op_groups=( arith => [ qw( + - * / += -= *= /= ) ],
                sets => [ qw( + += - -= * *= ^ ^= ) ],
                bits => [ qw( ~ & | ^ &= |= ^= ) ],
                compare => [ qw( == != < <= > >= ) ],
                eq => [ qw( == != ) ],
                string => [ ],          # just to enable the default string ops in add_operators
              );

sub add_operators {
   my ($self, $proto, $op_signs, $arg_types, $arg_attrs)=@_;
   my $signature;
   if (defined($arg_types)) {
      if (is_object($arg_types->[0])) {
         $signature=[ 3, 3, $arg_types->[0], $proto, '$' ];
         $proto=$arg_types->[0];
      } else {
         $signature=[ 3, 3, $proto, $arg_types->[1], '$' ];
      }
   }
   my @attr_flags = defined($arg_attrs) ? map { AutoFunction::parse_arg_attrs($_, 'operator') } @$arg_attrs : ();
   my @overloads = map { create_operator($self, $proto, $_,  $signature, @attr_flags) }
                   map { /^\@($id_re)$/o ? @{$op_groups{$1} // croak( "unknown operator group $1" )} : $_ }
                   @$op_signs;
   overload::OVERLOAD($proto->pkg, add_standard_operators($proto), @overloads);
}

#######################################################################################
# operator names must match the macro instances in wrappers.h

my %op_descr=( '+' => [ 'add', 0, 2 ], '-' => [ 'sub', 0, 2 ], '*' => [ 'mul', 0, 2 ], '/' => [ 'div', 0, 2 ], '%' => [ 'mod', 0, 2 ],
               '<<' => [ 'lsh', 0, 2 ], '>>' => [ 'rsh', 0, 2 ], '&' => [ 'and', 0, 2 ], '|' => [ '_or', 0, 2 ], '^' => [ 'xor', 0, 2 ],
               '+=' => [ 'Add', FuncFlag::returns_lvalue, 2 ], '-=' => [ 'Sub', FuncFlag::returns_lvalue, 2 ], '*=' => [ 'Mul', FuncFlag::returns_lvalue, 2 ],
               '/=' => [ 'Div', FuncFlag::returns_lvalue, 2 ], '%=' => [ 'Mod', FuncFlag::returns_lvalue, 2 ], '<<=' => [ 'Lsh', FuncFlag::returns_lvalue, 2 ],
               '>>=' => [ 'Rsh', FuncFlag::returns_lvalue, 2 ], '&=' => [ 'And', FuncFlag::returns_lvalue, 2 ], '|=' => [ '_Or', FuncFlag::returns_lvalue, 2 ],
               '^=' => [ 'Xor', FuncFlag::returns_lvalue, 2 ],
               'neg' => [ 'neg', 0, 1 ], '!' => [ 'not', 0, 1 ], '~' => [ 'com', 0, 1 ], 'bool' => [ 'boo', 0, 1 ],
               '++' => [ 'inc', FuncFlag::returns_lvalue, 1 ], '--' => [ 'dec', FuncFlag::returns_lvalue, 1 ],
               '==' => [ '_eq', 0, 2 ], '!=' => [ '_ne', 0, 2 ], '<' => [ '_lt', 0, 2 ],
               '<=' => [ '_le', 0, 2 ], '>' => [ '_gt', 0, 2 ], '>=' => [ '_ge', 0, 2 ],
             );

my %op_as_func=( '<=>' => [ 'cmp', 0, 2 ], '**' => [ 'pow', 0, 2 ], 'abs' => [ 'abs', 0, 1 ],
               );

sub create_operator {
   my ($self, $proto, $op_sign, $signature, @arg_flags)=@_;
   my @overloads;
   my $descr = $op_descr{$op_sign} //
               croak( exists $op_as_func{$op_sign} ? "operator $op_sign must be mapped to a C++ function or method"
                                                   : "operator $op_sign is unknown or can't have C++ binding" );
   my ($name, $func_flags, $num_args)=@$descr;
   if (@arg_flags) {
      if (@arg_flags != $num_args) {
         croak( "the number of arguments in the signature does not match operator $op_sign" );
      }
   } else {
      @arg_flags = (0) x $num_args;
   }
   if ($func_flags & FuncFlag::returns_lvalue) {
      # assignment version
      $_ &= ~(FuncFlag::arg_is_const_or_rval_ref | FuncFlag::arg_has_anchor) for @arg_flags;
      $arg_flags[0] |= FuncFlag::arg_is_lval_ref;

      ($proto->operators //= { })->{'='} //= do {
         push @overloads, ('=' => \&overload_clone_op);
         1
      };
   }
   my $wrapper_name = Operator::make_wrapper_name($name, @arg_flags);
   my $op=($root->auto_functions->{$wrapper_name} //= new Operator($name, $func_flags, $wrapper_name, $op_sign, @arg_flags));

   my $existing_op=($proto->operators //= { })->{$op_sign};
   if (defined $signature) {
      my $ov_node;
      my $create_root= !defined($existing_op) || instanceof Operator($existing_op);
      if ($create_root) {
         $ov_node=$proto->operators->{$op_sign}=new_root Overload::Node;
         if (defined $existing_op) {
            $ov_node->add_fallback($existing_op->code);
         }
      } else {
         $ov_node=$existing_op;
      }
      Overload::add_instance(__PACKAGE__, ".op:$op_sign", undef, $op->code, $signature, undef, $ov_node);
      if ($create_root) {
         no warnings 'void';
         use namespaces::AnonLvalue '$is_lvalue';
         my $is_lvalue= ($func_flags & FuncFlag::returns_lvalue) != 0;
         push @overloads, ($op_sign => sub {
                 $is_lvalue;
                 # when arguments are swapped, the first one is a non-object, look for the fallback straight away
                 &{ $ov_node->resolve($_[2] ? [ ] : \@_) // do { pop(@_); Overload::complain("__OPERATOR__", $op_sign, 0, \@_) } }
              });
      }

   } elsif (defined $existing_op) {
      if (instanceof Operator($existing_op)) {
         croak( "multiple overloading of operator $op_sign for type ", $proto->full_name );
      } else {
         # it must be an Overload::Node
         $existing_op->add_fallback($op->code);
      }

   } else {
      $proto->operators->{$op_sign}=$op;
      push @overloads, ($op_sign => $op->code)
   }
   @overloads
}
#######################################################################################
# private:

my @string_ops=( '""' => \&convert_to_string,
                 map { $_ => eval <<"." } qw( . cmp eq ne lt le gt ge ));
   sub { \$_[2] ? "\$_[1]" $_ "\$_[0]" : "\$_[0]" $_ "\$_[1]" }
.

sub add_standard_operators {
   my ($proto)=@_;
   my @overloads;
   ($proto->operators //= { })->{'""'} //= do {
      push @overloads, fallback => 0, nomethod => \&missing_op, @string_ops;
      1
   };
   @overloads
}
#######################################################################################
# private:
sub add_as_operator {
   my ($self, $proto, $auto_func, $code, $op_sign)=@_;
   my $descr=$op_descr{$op_sign} // $op_as_func{$op_sign} // croak( "operator $op_sign is unknown or can't have C++ binding" );
   if ($auto_func->total_args != $descr->[2]) {
      croak( "function ", $auto_func->name, " has a different number of arguments than operator $op_sign" );
   }
   my $op_code= $op_sign eq '<=>'
                ? sub { pop @_ ? -&$code : &$code } :
                $descr->[2] == 2
                ? sub { pop @_ ? missing_op(@_, 1, $op_sign) : &$code }
                : sub { splice @_,-2; &$code };

   overload::OVERLOAD($proto->pkg, add_standard_operators($proto), $op_sign => $op_code);
}
#######################################################################################
# private:
sub create_methods {
   my ($self, $proto, $generic_opts) = @_;
   my $descr = provide_cpp_type_descr($proto);
   # this is only called for types declared in perl (i.e. "persistent types"),
   # the relation between PropertyType and C++ class descriptors is unambigous
   $proto->dimension = $descr->dimension;
   my $opts = $proto->cppoptions;
   $opts->descr = $descr;

   if ($descr->is_container) {
      Serializer::prime($proto);
      no strict 'refs';
      push @{$proto->pkg."::ISA"}, $descr->is_assoc_container ? "Polymake::Core::CPlusPlus::TiedHash" : "Polymake::Core::CPlusPlus::TiedArray";

   } elsif ($descr->is_composite) {
      Serializer::prime($proto);
      if ($proto->cppoptions->fields) {
         if ($descr->num_members != scalar(@{$proto->cppoptions->fields})) {
            croak( "number of field names does not match the C++ description of class ", $proto->full_name );
         }
      } elsif (defined($proto->cppoptions->fields = $descr->member_names)) {
         $proto->cppoptions->define_field_accessors($proto->pkg);
         add_operators($self, $proto, [ '@eq' ]);
      }
      no strict 'refs';
      push @{$proto->pkg."::ISA"}, "Polymake::Core::CPlusPlus::TiedCompositeArray";

   } else {
      if (!defined($proto->operators)) {
         add_operators($self, $proto, []);
      }
      if ($descr->is_scalar) {
         if (my $int_type = $root->builtins->{Int}) {
            $int_type->add_constructor("construct", undef, \&convert_to_Int, [1, 1, $proto]);
         }
         if (my $float_type = $root->builtins->{double}) {
            $float_type->add_constructor("construct", undef, \&convert_to_Float, [1, 1, $proto]);
         }
         if (!$proto->cppoptions->builtin) {
            $proto->serialize = \&convert_to_string;
         }
      }
      if ($descr->is_serializable) {
         Serializer::prime($proto);
      }
   }
   $proto->toString = \&convert_to_string;

   if (!defined($generic_opts)) {
      define_constructors($proto);
      $proto->cppoptions->define_field_accessors($proto->pkg);
   }
}
#######################################################################################
# private:
sub bind_functions {
   my ($self, $extension, $func_list, $is_temp) = @_;
   unless (defined $func_list) {
      if ($is_temp) {
         croak( "temporary interface file does not contain function definitions" );
      }
      return;
   }
   my $app_stash = get_symtab($self->application->pkg);

   foreach my $descr (splice @$func_list) {
      $descr->extension //= $extension;
      next if defined($descr->cross_apps) && $descr->suspend($self);

      my $auto_func = $root->auto_functions->{$descr->name};
      my $twins;
      if (ref($auto_func) eq "HASH") {
         $twins = $auto_func;
         my $twin_key = $self->application->name . ":" . $descr->cpperl_file;
         $auto_func = $twins->{$twin_key} // $twins->{$self->application->name};
      }

      unless (defined $auto_func) {
         if ($is_temp) {
            croak( "temporary interface file contains definition of an unknown function" );
         }
         # even if the installed version looks suspicious, we are not entitled to fix it
         if (($extension // $self->application)->untrusted) {
            push @{$self->cpperl_generator->instances_to_remove}, $descr;
            dbg_print( "C++ function interface ", $descr->name, " defined in ", $descr->cpperl_file,
                       " has no binding declared in the rules, treating as obsolete" ) if $Verbose::cpp;
         }
         next;
      }

      my $sub;
      if (defined($sub = create_function_wrapper($descr, $app_stash, $auto_func->total_args,
                    $auto_func->flags & FuncFlag::returns_lvalue || $auto_func->prescribed_return_types))) {

         if ($move_private_instances) {
            no warnings 'void';
            use namespaces::AnonLvalue '$is_lvalue';
            my $inner_sub=$sub;
            my $is_lvalue= ($auto_func->flags & FuncFlag::returns_lvalue) != 0;
            $sub=sub {
               $is_lvalue;
               $descr &&= ($self->cpperl_generator->move_function_from_private($descr, \@_), undef);
               &$inner_sub;
            };
         }
         $descr->auto_func=$auto_func;                  # help guessing the right set of headers for non-persistent return types

      } else {
         $sub=$auto_func->unimplemented_instance;
      }

      if ($descr->register($auto_func, $sub)) {
         push @{$root->bound_functions}, $descr;   # preserve from destroying
      } elsif (!$is_temp) {
         push @{$self->cpperl_generator->instances_to_remove}, $descr;
      }
   }
}
#######################################################################################
sub cpperl_generator {
   my ($self)=@_;
   $self->generator //= do {
      require Polymake::Core::CPlusPlusGenerator;
      new Generator($self)
   }
}
#######################################################################################
sub obliterate_extension {
   my ($self, $ext, $entire_app)=@_;
   if (defined (my $shared_mod=$self->shared_modules->{$ext->dir})) {
      undef $shared_mod->so_name;
   }
   if ($entire_app) {
      if ($self->will_update_cpperls) {
         $self->will_update_cpperls=0;
         forget AtEnd($self->application->name.":C++");
      }
   }
}
#######################################################################################
package Polymake::Core::CPlusPlus;

declare $root;

use Polymake::Struct (
   # The first three members are known to C++ class RegistratorQueue.
   # Should a new registration queue become necesary, it should be added there as well.
   # The hash key is 'APPNAME' or 'APPNAME.BUNDLED' for items defined in bundled extensions.
   '%functions',
   '%embedded_rules',
   '%duplicate_class_instances',
   '%classes',
   '%private_classes',
   [ '$type_descr' => '$this->classes' ],
   '%builtins',
   '%templates',
   '%typeids',
   '%auto_functions',
   '@regular_functions',
   '@bound_functions',
   '$auto_default_constructor',
   '$auto_convert_constructor',
   '$auto_assignment',
   '$auto_conversion',
   '@auto_assoc_methods',
   '@auto_set_methods',
);

use Polymake::Enum Assoc => qw( helem find exists delete_void delete_ret );

use Polymake::Ext;

add_settings_callback sub {
   my ($settings) = @_;
   $settings->add_item('_C++::private_wrappers', \%private_wrappers,
                       "Autogenerated C++/perl wrappers",
                       UserSettings::Item::Flags::hidden | UserSettings::Item::Flags::no_export);
};

sub compile {
   my ($build_dir)=@_;
   warn_print( "Recompiling in $build_dir, please be patient..." );
   if ($Verbose::cpp) {
      if (Interrupts::system("ninja -C $build_dir -v")) {
         die "Compilation in $build_dir failed\n";
      }
   } else {
      my $errfile=new Tempfile();
      if (Interrupts::system("ninja -C $build_dir >$errfile.err 2>&1")) {
         die "Compilation failed; see the error log below\n\n" . `cat $errfile.err`;
      }
   }
}

sub recompile_extension {
   my ($ext, $core_last_built)=@_;
   my $last_built=-f $ext->build_dir."/.apps.built" && (stat _)[9];
   if ($last_built < $core_last_built || grep { !$_->is_bundled && $last_built < $_->configured_at } @{$ext->requires}) {
      unlink $ext->build_dir."/.apps.built" if $last_built>0;   # let recreate it with a fresh timestamp
      compile($ext->build_dir);
   }
}

# build_dir => boolean
sub is_mutable_location {
   $_[0] =~ m{/build(?:\.[^/]+)?/\w+$}
}

my $void_signature=[[0, 0], []];
my $unary_op_signature=[[1, 1, '$'], ['*']];
my $binary_op_signature=[[2, 2, qw($ $)], [qw(* *)]];

sub init {
   $root=_new(__PACKAGE__);

   if ($ENV{POLYMAKE_DEBUG_CLIENTS} || $ENV{POLYMAKE_CLIENT_SUFFIX}) {
      die <<".";
Use of environment variables POLYMAKE_DEBUG_CLIENTS and POLYMAKE_CLIENT_SUFFIX is discontinued.
Please use POLYMAKE_BUILD_MODE=Debug to enforce loading client modules built in debug mode.
Note that debug output in some clients only appears at high debug levels (-dd and higher).
.
   }

   if ($DebugLevel) {
      assign_max($Verbose::cpp, 2);
   }

   # create the standard constructors right now
   my $op=new Constructor(@$void_signature);
   $root->auto_default_constructor=$root->auto_functions->{$op->wrapper_name}=$op;

   $op=$root->auto_convert_constructor=new Constructor(@$unary_op_signature);
   $root->auto_convert_constructor=$root->auto_functions->{$op->wrapper_name}=$op;

   $op=new SpecialOperator("assign");
   $root->auto_assignment=$root->auto_functions->{$op->wrapper_name}=$op;
   $op=new SpecialOperator("convert");
   $root->auto_conversion=$root->auto_functions->{$op->wrapper_name}=$op;
   create_assoc_methods();

   $BigObject_cpp_options=new BigObjectOptions(name=>"BigObject", builtin=>"B");
   $BigObjectArray_cpp_options=new BigObjectOptions(name=>"Array<BigObject>", builtin=>"B");

   # check for missing or outdated flags for successful builds
   # this should not replace the full ninja functionality, in particular it does not check for updated C++ sources,
   # but just for the most recent reconfiguration or wrapper generation
   my $core_last_built=do {
      if (is_mutable_location($InstallArch)) {
         if (-f "$InstallArch/.apps.built") {
            (stat _)[9];
         } else {
            compile($InstallArch);
            (stat "$InstallArch/.apps.built")[9];
         }
      } else {
         (stat "$InstallArch/lib")[9];
      }
   };

   foreach my $ext (@Extension::active[$Extension::num_bundled..$#Extension::active]) {
      if (is_mutable_location($ext->build_dir)) {
         recompile_extension($ext, $core_last_built);
      }
   }

   if ($PrivateDir) {
      require Polymake::Core::CPlusPlusPrivateWrappers;
      if (defined ($private_wrapper_ext=PrivateWrappers->init)) {
         recompile_extension($private_wrapper_ext, $core_last_built);
      }
   }
}

sub Polymake::Core::BigObjectType::cppoptions { $BigObject_cpp_options }

#################################################################################
sub provide_cpp_type_descr {
   my ($proto) = @_;
   $root->classes->{$proto->pkg}
     or
   # when bind_functions is executed for the private wrapper extensions in "shared" generation mode,
   # return type registration may require instantiation of property types which should not be considered as "shared use";
   # the return type might be later moved together with a function instance
   $move_private_instances && $root->private_classes->{$proto->pkg}
     or
   do {
      require Polymake::Core::CPlusPlusGenerator;
      &generate_lacking_type;
   }
}

sub get_canned_cpp_class {
   require Polymake::Core::CPlusPlusGenerator;
   my ($x)=@_;
   my $typeid=get_magic_typeid($x, 0);
   $typeid && do {
      if (defined (my $descr=$root->typeids->{$typeid})) {
         scalar($descr->get_cpp_representation);
      } else {
         demangle($typeid);
      }
   }
}
#################################################################################
sub Polymake::Core::PropertyType::cpp_type_descr {
   my ($self)=@_;
   $root->classes->{$self->pkg} || croak( "type ", $self->full_name, " has no C++ binding" );
}

sub Polymake::Core::PropertyType::get_array_element_type {
   my ($proto)=@_;
   if ($proto->cppoptions && !$proto->cppoptions->builtin) {
      my $descr=$proto->cpp_type_descr;
      if ($descr->is_container) {
         $descr->value_type
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
      if ($i>=0 && defined (my $member_protos=$proto->cpp_type_descr->member_types)) {
         return $member_protos->[$i];
      }
   }
   undef;
}

sub Polymake::Core::PropertyType::guess_array_element_type {
   my ($data)=@_;
   if (is_object($data) && UNIVERSAL::can($data, "type") &&
       defined(my $type = $data->type->get_array_element_type)) {
      return $type;
   }
   my $first=$data->[0];
   if (is_object($first) && UNIVERSAL::can($first, "type")) {
      $first->type;
   } else {
      match_builtin_type($first);
   }
}
#################################################################################
sub is_proxy_for {
   my ($x, $type_name)=@_;
   ref($x) && UNIVERSAL::isa($x, "Polymake::Core::CPlusPlus::$builtin2proxy{$type_name}");
}

# the indices correspond to the return values of classify_scalar
my @builtin_type_names=qw( std::string double Int bool );

sub match_builtin_type {
   if (defined(my $type = &classify_scalar)) {
      $root->builtins->{$builtin_type_names[$type]}
   } else {
      undef
   }
}

sub guess_builtin_type {
   if (my $class = ref($_[0])) {
      if ($class eq "ARRAY") {
         # deliberately interpret an empty array as an empty set of indices
         if (defined(my $elem_proto = @{$_[0]} ? guess_builtin_type($_[0]->[0]) : $root->builtins->{Int})) {
            return $root->builtins->{array}->typeof($elem_proto);
         } else {
            croak( "don't know how to match [ '$_[0]->[0]' ] to a C++ type" );
         }
      }
      ### FIXME: mapping for HASH
      if (is_object($_[0])) {
         if (defined(my $proto = UNIVERSAL::can($_[0], ".type"))) {
            $proto = $proto->();
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
#######################################################################################
sub resolve_auto_function {
   &AutoFunction::find_instance // do {
      require Polymake::Core::CPlusPlusGenerator;
      &generate_lacking_function
   }
}
#######################################################################################
sub try_merge_auto_functions {
   my ($f1, $f2) = @_;
   if (defined($f1->in_cc_file) || defined($f2->in_cc_file)) {
      return 0 if $f1->in_cc_file ne $f2->in_cc_file || $f1->application != $f2->application;
   }
   return 1 if $f1->application==$f2->application;

   if (not($f1->flags & FuncFlag::is_method) &&
       defined(my $common_app = $f1->application->common($f2->application))) {
      $f2->application = $common_app;
      return 1;
   }
   0
}
#######################################################################################
sub check_twins {
   my ($auto_func) = @_;
   if (defined(my $twin = $root->auto_functions->{$auto_func->wrapper_name})) {
      if (ref($twin) eq "HASH") {
         foreach my $other (values %$twin) {
            if (try_merge_auto_functions($auto_func, $other)) {
               push @{$other->include}, @{$auto_func->include};
               $_[0] = $other;
               return;
            }
         }
         $twin->{$auto_func->twin_key} = $auto_func;
      } elsif (try_merge_auto_functions($auto_func, $twin)) {
         push @{$twin->include}, @{$auto_func->include};
         $_[0] = $twin;
      } else {
         $root->auto_functions->{$auto_func->wrapper_name}={ map { ($_->twin_key => $_) } $auto_func, $twin };
      }
   } else {
      $root->auto_functions->{$auto_func->wrapper_name} = $auto_func;
   }
}

#######################################################################################

my @assoc_descr=(
   [ Assoc::helem,       "brk",           "[]",                                           FuncFlag::returns_lvalue,
     FuncFlag::arg_is_univ_ref, 0 ],
   [ Assoc::find,        "assoc_find",    [$binary_op_signature->[0], [qw(*:anchor *)]],  0,
     name => "pm::perl::find_element", include => [ "polymake/perl/assoc.h" ] ],
   [ Assoc::exists,      "exists",        [$unary_op_signature->[0],  ["", "*"]],         FuncFlag::is_method ],
   [ Assoc::delete_void, "erase",         [$unary_op_signature->[0],  [qw(& *)]],         FuncFlag::is_method ],
   [ Assoc::delete_ret,  "assoc_delete",  [$binary_op_signature->[0], [qw(*& *)]],        0,
     name => "pm::perl::delayed_erase", include => [ "polymake/perl/assoc.h" ], ],
);

sub create_assoc_methods {
   $#{$root->auto_assoc_methods} = $#{$root->auto_set_methods}=$#assoc_descr;
   foreach my $descr (@assoc_descr) {
      my ($i, $name, $signature, $flags, @options) = @$descr;
      if ($name eq "brk") {
         my $op = new Operator($name, $flags, Operator::make_wrapper_name($name, @options), $signature, @options);
         $root->auto_functions->{$op->wrapper_name} = $op;
         use namespaces::AnonLvalue;
         $root->auto_assoc_methods->[$i] = $op->code = sub { &{ resolve_auto_function($op, \@_) } };
      } else {
         my $auto_func = new AutoFunction($name, $flags, undef, @options);
         $auto_func->prepare(undef, @$signature, "Polymake", 1);
         $auto_func->wrapper_name =~ s/:\K/CORE./;
         $root->auto_functions->{$auto_func->wrapper_name} = $auto_func;
         my $code = $root->auto_assoc_methods->[$i] = sub { &{resolve_auto_function($auto_func, \@_)} };
         set_method($code) if $flags & FuncFlag::is_method;
      }
   }
   $root->auto_set_methods->[Assoc::helem] = \&no_set_method;
   $root->auto_set_methods->[Assoc::find] = \&no_set_method;
   $root->auto_set_methods->[Assoc::exists] = $root->auto_assoc_methods->[Assoc::exists];
   $root->auto_set_methods->[Assoc::delete_void] = $root->auto_assoc_methods->[Assoc::delete_void];
   $root->auto_set_methods->[Assoc::delete_ret] = \&no_set_method;
}

sub no_set_method { croak("This operation is not defined on sets") }

#######################################################################################
sub construct_parsed : method {
   my $proto = shift;
   if ($_[0] =~ /\S/) {
      eval { assign_to_cpp_object($proto->construct->(), $_[0], $PropertyType::trusted_value) }
      // do {
         if ($@ =~ /^(\d+)\t/) {
            local $_ = $_[0];
            pos($_) = $1;
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

sub std_default_constructor : method {
   &{resolve_auto_function($root->auto_default_constructor, \@_)}
}

sub std_parsing_constructor {
   assign_to_cpp_object(std_default_constructor($_[0]), $_[1], $PropertyType::trusted_value);
}

sub std_convert_constructor : method {
   if (is_array($_[1])) {
      &std_parsing_constructor;
   } elsif (is_hash($_[1])) {
      my $descr = $_[0]->cppoptions->descr;
      if (defined(my $member_names = $descr->member_names)) {
         construct_from_members(@_, $member_names);
      } else {
         &std_parsing_constructor;
      }
   } elsif (instanceof Serializer::Sparse($_[1])) {
      &std_parsing_constructor;
   } else {
      &{resolve_auto_function($root->auto_convert_constructor, \@_)};
   }
}

sub construct_from_members {
   my ($proto, $hash, $member_names) = @_;
   if (!$PropertyType::trusted_value and keys %$hash != @$member_names || grep { !exists $hash->{$_} } @$member_names) {
      croak( "can't construct ", $_[0]->full_name, " - wrong member names: expected [", join("; ", @$member_names),
             "] but got [", join("; ", keys %$hash), "]" );
   }
   assign_to_cpp_object($proto->construct->(), [ @$hash{@$member_names} ], $PropertyType::trusted_value);
}

package Polymake::Core::CPlusPlus::StdConstr;

sub set_construct_node {
   my (undef, $proto) = @_;
   state $construct_node = do {
      my $node = new_root Overload::Node;
      Overload::add_instance(__PACKAGE__, ".construct", undef, \&std_default_constructor, $void_signature->[0], undef, $node);
      Overload::add_instance(__PACKAGE__, ".construct", undef, \&std_convert_constructor, $unary_op_signature->[0], undef, $node);
      $node->add_fallback(sub { my $proto = shift; std_parsing_constructor($proto, \@_) });
      $node
   };
   $proto->construct_node = $construct_node;
   $proto->parse = \&construct_parsed;
}

package Polymake::Core::CPlusPlus::Deserializing;

sub set_construct_node {
   state $construct_node = do {
      my $node=new_root Overload::Node;
      Overload::add_instance(__PACKAGE__, ".construct", undef, \&std_convert_constructor, $unary_op_signature->[0], undef, $node);
      $node
   };
   my (undef, $proto) = @_;
   $proto->construct_node = $construct_node;
}

package Polymake::Core::CPlusPlus;

sub define_constructors {
   my ($proto) = @_;
   if (!defined($proto->construct_node) && length($proto->cppoptions->default_constructor)) {
      my $pkg = namespaces::lookup_class(__PACKAGE__, $proto->cppoptions->default_constructor, $proto->application->pkg)
        or croak( "class ", $proto->full_name, " tries to inherit constructors from an unknown package ", $proto->cppoptions->default_constructor );
      $pkg->set_construct_node($proto);
      $proto->create_method_new();
      no strict 'refs';
      push @{$proto->pkg."::ISA"}, $pkg;
   }
}

#######################################################################################
sub dump_arg {
   if (is_object($_[0])) {
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
   unless (DynaLoader::dl_load_file($so_name, 0x01)) {
      my $error=DynaLoader::dl_error();
      $error =~ s/\x00$//;  # sometimes there is a stray NUL character at the end
      $error =~ s/${so_name}:\s+//;
      $error =~ s/undefined symbol: \K(\S+)/demangle($1)/e;
      die "Can't load shared module $so_name: $error\n";
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
