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

require Config;

package Polymake::Core::CPlusPlus::CPPerlFile;

my $cpperl_version = 3;

use Polymake::Struct (
   [ new => '$;$$' ],
   [ '$filename' => '#1' ],
   [ '$rebuild_in' => '#2' ],
   [ '$embed' => '#3' ],
   [ '$instances' => 'undef' ],
   '$has_removed_instances',
);

sub codec {
   state $codec = JSON->new->utf8->canonical->relaxed->space_after->indent(0);
}

sub new {
   my $self = &_new;
   if (@_ <= 2) {
      open my $F, "<", $self->filename
        or die "can't read ", self->filename, ": $!\n";
      local $/;
      my $contents = codec()->decode(<$F>);
      $self->embed = $contents->{embed};
      $self->instances = ref($contents->{inst}) eq "ARRAY" ? $contents->{inst} : [];
      if (@{$self->instances} && !defined($self->instances->[-1])) {
         pop @{$self->instances};
      }
   } else {
      $self->instances = [];
   }
   $self
}

sub output_filename {
   my ($self, $path_prefix) = @_;
   my $filename = $self->filename;
   if (length($path_prefix)) {
      $filename =~ s/^\Q$InstallTop\E/$path_prefix/;
   }
   my ($dir) = $filename =~ $directory_re;
   -d $dir or File::Path::make_path($dir);
   $filename
}

sub save {
   my ($self, $app, $path_prefix) = @_;
   my %contents = ( version => $cpperl_version, app => $app->name,
                    inst => ($self->has_removed_instances ? [ grep { defined } @{$self->instances} ] : $self->instances) );
   if ($self->embed) {
      $contents{embed} = $self->embed;
   }

   my $text = codec()->encode(\%contents);
   # place every instance on a separate line with a trailing comma, facilitating merge conflict resolution
   # append a `null' element to preserve well-formedness
   $text =~ s/\s* (?'pre' "inst":\s*\[) (?'inst' $balanced_re) (?'post' \],?) \s*/"\n $+{pre}\n" . indent_instances($+{inst}) . " null $+{post}\n"/xe;

   my $filename = output_filename($self, $path_prefix);
   open my $F, ">", $filename
     or die "can't write to $filename: $!\n";
   print $F $text, "\n";
   close $F;
}

sub indent_instances {
   my ($text) = @_;
   my @instances;
   while ($text =~ /\{$balanced_re\}/g) {
      push @instances, "  $&,\n";
   }
   join("", @instances);
}

#######################################################################################
package Polymake::Core::CPlusPlus::HeaderFile;

use Polymake::Struct (
   [ new => '$' ],
   [ '$filename' => '#1->{filename}' ],
   '$prologue',
   '$declarations',
   '$epilogue',
);

sub new {
   my $self = &_new;
   if (-f $self->filename) {
      load($self, $self->filename);
   } else {
      create($self, $_[0]);
   }
   $self;
}

sub load {
   my ($self, $filename)=@_;
   open my $F, "<", $filename
     or die "can't read $filename: $!\n";
   local $/;
   my $text=<$F>;
   close $F;
   $text =~ /^.*Automatically generated contents follow.*\n/m;
   $self->prologue = $` . $&;
   $text = $';
   $text =~ /^.*Automatically generated contents end here.*\n/m;
   $self->declarations = $`;
   $self->epilogue = $& . $';
}

sub create {
   my ($self, $vars)=@_;
   load($self, "$InstallTop/lib/core/skel/bindings.h");

   # don't parse the copyright notice at the file top
   $self->prologue =~ /^-{2,}\n.*\n/m;
   $self->prologue = $` . $& . subst_vars($', $vars);
   $self->epilogue = subst_vars($self->epilogue, $vars);
}

sub subst_vars {
   my ($text, $vars)=@_;
   my $result=eval qq{<<".#.#.#.";\n$text.#.#.#.\n};
   if ($@) {
      die "Error processing the header file template: $@";
   }
   $result
}

sub save {
   my ($self) = @_;
   my $filename = &CPPerlFile::output_filename;
   open my $F, ">", $filename
     or die "can't write to $filename: $!\n";
   my $contents = $self->prologue . $self->declarations . $self->epilogue;
   # kill superfluous empty lines
   $contents =~ s/^\n{2,}/\n/mg;
   print $F $contents;
   close $F;
}

#######################################################################################
package Polymake::Core::CPlusPlus::LackingFunctionInstance;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$is_instance_of' => '#1' ],
   '@arg_types',     # 'C++ type expression', ...
   '@include',       # [ 'header', ... ]
   [ '$application' => 'undef' ],
   [ '$cross_apps' => 'undef' ],
   [ '$extension' => 'undef' ],
   [ '$cpperl_define_with' => 'undef' ],
   [ '$is_private' => '$code_generation eq "private"' ],
);

sub new {
   my $self = &_new;
   my ($auto_func, $args) = @_;
   my @includes = @{$self->is_instance_of->include};
   set_application($self, $auto_func->cpperl_define_with($args), $auto_func->application, $auto_func->cross_apps);
   set_extension($self->extension, $auto_func->extension // ($auto_func->application && $auto_func->application->origin_extension));

   if ($auto_func->explicit_template_params) {
      foreach ($auto_func->deduce_extra_params($args)) {
         my ($arg_type, $includes, $define_with, $app, $cross_apps, $extension) = $_->get_cpp_representation;
         push @{$self->arg_types}, $arg_type;
         push @includes, @$includes;
         set_application($self, $define_with, $app, $cross_apps);
         set_extension($self->extension, $extension);
      }
   }
   my $arg_offset = $auto_func->explicit_template_params;
   if (defined($auto_func->class)
       and !grep { $_->flags & FuncFlag::is_static } @{$auto_func->arg_descrs}) {
      push @{$self->arg_types}, $auto_func->class."()";
      ++$arg_offset;
   }
   push @{$self->arg_types}, ("void") x $auto_func->total_args;

   my (@need_coerce, $first_obj_descr);

   foreach my $arg_descr (@{$auto_func->arg_descrs}) {
      my $arg_no = $arg_descr->arg_index;
      my ($arg_type, $includes, $define_with, $app, $cross_apps, $extension);
      my $lval_flag = $arg_descr->flags & FuncFlag::arg_is_const_or_rval_ref;
      my $num_flag_seen = 0;
      if (defined(my $typeid = get_magic_typeid($args->[$arg_no], $lval_flag))) {
         my $descr = $root->typeids->{$typeid} || die "unregistered C++ typeid $typeid\n";
         $first_obj_descr = $descr if $arg_no == ($arg_descr->flags & FuncFlag::args_are_swapped ? 1 : 0);
         ($arg_type, $includes, $define_with, $app, $cross_apps, $extension) = $descr->get_cpp_representation;
         unless ($arg_descr->flags & FuncFlag::is_static) {
            $arg_type = "Wary<$arg_type>" if $arg_descr->flags & FuncFlag::arg_is_wary;
            if ($typeid == FuncFlag::arg_is_const_ref) {
               $arg_type = "const $arg_type&";
            } elsif ($typeid == FuncFlag::arg_is_lval_ref) {
               $arg_type = "$arg_type&";
            }
            $arg_type = "perl::Canned<$arg_type>";
            $arg_type = "perl::AnchorArg<$arg_type>" if $arg_descr->flags & FuncFlag::arg_has_anchor;
         }
      } else {
         my $proto = $arg_descr->flags & FuncFlag::is_static
                     ? $args->[$arg_no]->type
                     : guess_builtin_type($args->[$arg_no], my $num_flag = $arg_descr->flags & FuncFlag::arg_is_numeric);
         my $opts = $proto->cppoptions->finalize;
         if ($opts->builtin eq "enum") {
            $arg_type="perl::Enum<" . $opts->name . ">";
         } elsif ($opts->template_params) {
            $arg_type = $opts->name;
            $arg_type = "Wary<$arg_type>" if $arg_descr->flags & FuncFlag::arg_is_wary;
            $arg_type = "perl::TryCanned<const $arg_type>" unless $arg_descr->flags & FuncFlag::is_static;
         } else {
            push @need_coerce, $arg_no+$arg_offset if $num_flag;
            $arg_type = $opts->name;
         }
         $includes = $opts->include;
         $app = $opts->application;
         $cross_apps = $opts->cross_apps;
         $extension = $opts->extension;
      }
      push @includes, @$includes;
      set_application($self, $define_with, $app, $cross_apps);
      set_extension($self->extension, $extension);
      if (defined(my $pattern = $arg_descr->pattern)) {
         $pattern =~ s/%1/$arg_type/g;
         $arg_type = "$pattern($arg_type)";
      } elsif ($arg_descr->flags & FuncFlag::is_static and defined($auto_func->class)) {
         $arg_type = $auto_func->class . "($arg_type)";
      }
      if (defined $arg_descr->conv_to) {
         $arg_type = $self->arg_types->[$arg_descr->conv_to] . "($arg_type)";
      }
      $self->arg_types->[$arg_no+$arg_offset] = $arg_type;
   }

   if (@need_coerce) {
      my ($elem_type, $elem_descr) = $first_obj_descr->is_container ? ($first_obj_descr->element_type, $first_obj_descr->element_descr)
                                                                    : ($first_obj_descr->type, $first_obj_descr);
      my $expects_double = $elem_type->cppoptions->name eq "double";
      foreach my $arg_no (@need_coerce) {
         if (($self->arg_types->[$arg_no] eq "double") != $expects_double) {
            # floating-point numbers must be explicitly coerced to integral types and vice versa
            $self->arg_types->[$arg_no] = $elem_descr->get_cpp_representation . "(" . $self->arg_types->[$arg_no] . ")";
         }
      }
   }

   remove_origin_extension($self);
   @{$self->include} = sorted_uniq(sort(@includes));
   $self;
}

sub complain_source_conflict {
   "Don't know where to place the instance of ", $_[0]->is_instance_of->perl_name
}

sub cpperl_definition {
   my ($self) = @_;
   my $func = $self->is_instance_of;
   my $def = $func->cpperl_instance_definition;
   $def->{sig} = $func->wrapper_name;
   if ($func->flags & FuncFlag::returns_list) {
      $def->{ret} = "list";
   } elsif ($func->flags & FuncFlag::returns_void) {
      $def->{ret} = "empty";
   } elsif ($func->flags & FuncFlag::returns_lvalue) {
      $def->{ret} = "lvalue";
   }
   $def->{args} = $self->arg_types;
   if (defined($self->cross_apps)) {
      $def->{apps} = [ map { $_->name } @{$self->cross_apps} ];
   }
   if (@{$self->include}) {
      $def->{include} = $self->include;
   }
   $def
}

sub override_extension { }
sub needs_recognizer { false }

#######################################################################################
package Polymake::Core::CPlusPlus::FuncDescr;

sub match_cpperl_instance {
   my ($self, $inst)=@_;
   if (defined $self->auto_func) {
      my $expected=$self->auto_func->cpperl_instance_definition;
      $expected->{sig}=$self->auto_func->wrapper_name;
      if ($inst->{sig} =~ /^&/) {
         $expected->{kind}="ptr";
         substr($expected->{sig}, 0, 0) .= "&";
      }
      while (my ($key, $val)=each %$expected) {
         $val eq $inst->{$key}
           or
         return 0;
      }
   }
   1
}

sub is_private {
   defined($private_wrapper_ext) && $_[0]->extension == $private_wrapper_ext
}

#######################################################################################
package Polymake::Core::CPlusPlus::LackingTypeInstance;

use Polymake::Struct (
   [ new => '$' ],
   [ '$is_instance_of' => '#1' ],
   [ '$extension' => '#1->cppoptions->extension' ],
   [ '$is_private' => '$code_generation eq "private"' ],
);

sub complain_source_conflict {
   "Can't create C++ binding for ", $_[0]->is_instance_of->full_name;
}

sub needs_recognizer {
   my ($self) = @_;
   my $proto = $self->is_instance_of;
   my $opts = $proto->cppoptions;
   not($opts->builtin || $opts->special || defined($proto->generic) || defined($opts->cpperl_define_with))
}

sub h_file_vars {
   my ($self) = @_;
   my $proto = $self->is_instance_of;
   my $opts = $proto->cppoptions;
   my $main_header = $opts->include->[-1];
   my ($guard_name, $wrapper_dir) =
      $DeveloperMode && $main_header =~ m{polymake/[^/]+$}
      ? ("CORE_WRAPPERS_", "$InstallTop/include/core-wrappers")
      : ("APP_WRAPPERS_",  ($opts->extension ? $opts->extension->dir : $opts->application->installTop)."/include/app-wrappers");
   my $short_name = $main_header =~ s/\.h$//r;
   $short_name =~ s{^polymake/}{};
   $short_name =~ s{[./: ]}{_}g;
   return { guard_name => $guard_name.$short_name, include_file => $main_header, filename => "$wrapper_dir/$main_header" };
}

sub recognizing_type {
   my ($self) = @_;
   my $proto = $self->is_instance_of;
   my $opts = $proto->cppoptions;
   my $typenames = join(", ", "typename T", map { "typename T$_" } 0 .. $opts->template_params-1);
   my $Ts = join(",", map { "T$_" } 0 .. $opts->template_params-1);
   my $declared = $opts->name;
   if ($declared =~ /%\d/) {
      $declared =~ s/%(\d+)/"T".($1-1)/ge;
   } elsif ($Ts) {
      $declared .= "<$Ts>";
   }
   my $pkg = $proto->pkg;
   my $app_name = $proto->application->name;
   if ($opts->name !~ /::/ && $opts->include->[-1] =~ m{^polymake/$app_name/}) {
      $declared = "polymake::$app_name\::$declared";
   }
   return <<".";
   template <$typenames>
   RecognizeType4perl("$pkg", ($Ts), $declared)

.
}

sub cpperl_filename {
   my ($self) = @_;
   my $proto = $self->is_instance_of;
   $proto->cppoptions->builtin || $proto->cppoptions->special ? "builtins" : $proto->name
}

sub cpperl_define_with {
   my ($self) = @_;
   $self->is_instance_of->cppoptions->cpperl_define_with // $self
}

sub in_cc_file {
   my ($self) = @_;
   my $define_with = $self->is_instance_of->cppoptions->cpperl_define_with;
   $define_with && $define_with->in_cc_file;
}

sub include {
   my $proto = $_[0]->is_instance_of;
   ($proto->generic // $proto)->cppoptions->include
}

sub cpperl_definition {
   my ($self) = @_;
   my $proto = $self->is_instance_of;
   my $opts = $proto->cppoptions;
   my %def = ( pkg => $proto->pkg );
   unless ($proto->abstract) {
      $def{$opts->builtin || $opts->special ? "builtin" : "class"} = $opts->name;
   }
   if (@{$opts->include}) {
      $def{include} = $opts->include;
   }
   if (&needs_recognizer) {
      my $vars = &h_file_vars;
      $def{guard_name} = $vars->{guard_name};
      $def{wrapper_file} = $vars->{filename} =~ s{^\Q$InstallTop/\E}{}r;
   }
   \%def
}

sub override_extension {
   my ($self) = @_;
   $self->is_instance_of->cppoptions->extension = $self->extension;
}

#######################################################################################
package Polymake::Core::CPlusPlus::TypeDescr;

sub get_cpp_representation {
   my ($self, $proto)=@_;
   $self->source_name //= defined($self->generated_by)
                          ? demangle($self->typeid)
                          : (($proto //= &type)->cppoptions->finalize->name //= demangle($self->typeid));

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
               # it is a descriptor of a function
               my $auto_func=$gen->auto_func;
               my @includes=@{$auto_func->include};
               $self->cpperl_define_with=$auto_func;
               $self->application=$auto_func->application;
               $self->cross_apps=$gen->cross_apps;
               $self->extension=$gen->extension;
               if (ref($gen->arg_types)) {
                  foreach my $arg_type (@{$gen->arg_types}) {
                     if (defined (my $arg_type_descr=$root->typeids->{$arg_type})) {
                        (undef, my ($includes, $define_with, $app, $cross_apps, $extension)) = $arg_type_descr->get_cpp_representation;
                        push @includes, @$includes;
                        set_application($self, $define_with, $app, $cross_apps);
                        set_extension($self->extension, $extension);
                     }
                  }
               }
               \@includes
            } else {
               # it is a typeid of a container class
               (undef, my $includes, $self->cpperl_define_with, $self->application, $self->cross_apps, $self->extension)=$parent->get_cpp_representation;
               $includes
            }
         } else {
            my $opts=($proto //= &type)->cppoptions->finalize;
            $self->cpperl_define_with=new LackingTypeInstance($proto);
            $self->application=$opts->application;
            $self->cross_apps=$opts->cross_apps;
            $self->extension=$opts->extension;
            $opts->include
         }
      };
      ($self->source_name, $self->include, $self->cpperl_define_with, $self->application, $self->cross_apps, $self->extension);
   } else {
      $self->source_name;
   }
}

sub Polymake::Core::PropertyType::get_cpp_representation {
   get_cpp_representation(&PropertyType::cpp_type_descr, @_);
}

sub complain_source_conflict {
   "Don't know where to place the instantiation of ", $_[0]->source_name
}

sub match_cpperl_instance {
   my ($self, $inst) = @_;
   my $pkg = $inst->{pkg};
   exists($inst->{class}) || exists($inst->{builtin}) and defined($pkg) && get_symtab($pkg) == $self->pkg
}

*is_private=\&FuncDescr::is_private;

#######################################################################################
package Polymake::Core::CPlusPlus::AutoFunction;

sub is_private { 0 }

sub cpperl_define_with {
   my ($self, $args)=@_;
   if (defined $self->application) {
      $self
   } else {
      my $proto=$args->[0]->type;
      $proto->cppoptions->finalize;
      new LackingTypeInstance($proto);
   }
}

sub cpperl_filename {
   my ($self)=@_;
   if (defined $self->in_cc_file) {
      "wrap-".(split /\./, $self->in_cc_file, 2)[0]
   } else {
      "auto-".$self->name =~ s/:+/_/gr;
   }
}

sub cpperl_instance_definition {
   my ($self)=@_;
   my %def=( func => $self->name );
   if (defined($self->caller_kind) && $self->caller_kind ne "free") {
      $def{kind}=$self->caller_kind;
   }
   if ($self->explicit_template_params) {
      $def{tp}=$self->explicit_template_params;
   }
   \%def
}

sub match_cpperl_instance {
   my ($self, $inst)=@_;
   $self->name eq $inst->{func}
}

#######################################################################################
package Polymake::Core::CPlusPlus::Constructor;

sub cpperl_instance_definition { { op => "new" } }

sub cpperl_define_with {
   my $proto=$_[1]->[0]->type;
   $proto->cppoptions->finalize;
   new LackingTypeInstance($proto);
}

#######################################################################################
package Polymake::Core::CPlusPlus::SpecialOperator;

*cpperl_define_with=\&Constructor::cpperl_define_with;

sub cpperl_instance_definition { { op => $_[0]->perl_name } }

#######################################################################################
package Polymake::Core::CPlusPlus::Operator;

sub cpperl_define_with {
   my ($self, $args)=@_;
   my $proto=$args->[$self->arg_descrs->[0]->flags & FuncFlag::args_are_swapped ? 1 : 0]->type;
   $proto->cppoptions->finalize;
   new LackingTypeInstance($proto);
}

*cpperl_instance_definition=\&SpecialOperator::cpperl_instance_definition;

#######################################################################################
package Polymake::Core::CPlusPlus::Options;

sub finalize {
   my ($self) = @_;
   if (defined(my $proto = $self->finalize_with)) {
      undef $self->finalize_with;
      $self->extension //= $self->application->origin_extension;
      my (@includes, @t_params);

      foreach (@{$proto->params}) {
         unless ($_->cppoptions) {
            die "Can't create C++ binding for ", $proto->full_name, ": non-C++ parameter ", $_->name, "\n";
         }
         my $p_opts = $_->cppoptions->finalize;
         push @t_params, $p_opts->name;
         push @includes, @{$p_opts->include};
         set_application($self, $p_opts->cpperl_define_with, $p_opts->application, $p_opts->cross_apps);
         set_extension($self->extension, $p_opts->extension);
      }

      if (is_code($self->name)) {
         $self->name = is_method($self->name) ? $self->name->($proto, @t_params) : $self->name->(@t_params);
      } elsif ($self->template_params eq "*" || $self->name eq "typeid") {
         undef $self->name;
      } elsif ($self->name =~ /%\d/) {
         unshift @t_params, undef;      # shift the indexing
         $self->name =~ s/%(\d+)/$t_params[$1]/g;
      } else {
         $self->name .= "<" . join(", ", @t_params) . ">";
      }
      if (@includes) {
         $self->include = [ sorted_uniq(sort(@includes, @{$self->include})) ];
      }
      remove_origin_extension($self);
   }
   $self;
}

#######################################################################################
package Polymake::Core::CPlusPlus;

sub set_application {
   my ($self, $new_define_with, $new_app, $new_cross_apps)=@_;
   if (defined($new_define_with) && defined($new_define_with->in_cc_file)) {
      if (defined($self->cpperl_define_with) && defined($self->cpperl_define_with->in_cc_file)) {
         $self->cpperl_define_with->in_cc_file eq $new_define_with->in_cc_file
           or croak( $self->complain_source_conflict, ": conflicting files ",
                     $new_define_with->in_cc_file, " and ", $self->cpperl_define_with->in_cc_file );
      } else {
         $self->cpperl_define_with=$new_define_with;
      }
      set_extension($self->extension, $new_define_with->extension);
   } else {
      $self->cpperl_define_with //= $new_define_with;
   }

   if (defined (my $app=$self->application)) {
      if ($app != $new_app) {
         if (defined($new_define_with) && defined($new_define_with->in_cc_file)) {
            $self->application=$new_app;
            if ($new_app->common($app) != $new_app) {
               add_cross_app($self, $app);
            }
            $app=$new_app;
         } elsif ((my $common_app=$app->common($new_app)) != $app) {
            if ((defined($self->cpperl_define_with) && defined($self->cpperl_define_with->in_cc_file))
                || !defined($common_app)) {
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

sub remove_origin_extension {
   my ($self)=@_;
   if (defined($self->extension) && $self->extension==$self->application->origin_extension) {
      undef $self->extension;
   }
}

*set_extension=\&PropertyParamedType::set_extension;

#######################################################################################
sub generate_lacking_type {
   my ($proto) = @_;
   $proto->cppoptions->finalize;
   my $lacking = new LackingTypeInstance($proto);
   my $generator = $proto->cppoptions->application->cpp->cpperl_generator;
   if ($code_generation eq "shared" && defined(my $descr = delete $root->private_classes->{$proto->pkg})) {
      $generator->move_type_from_private($descr, $proto, $lacking);
   } else {
      $generator->generate_lacking_type($lacking);
      if ($proto->abstract || defined($descr = $root->classes->{$proto->pkg})) {
         $generator->lacking_types->{$proto->pkg} = $lacking;
         $descr
      } else {
         die " C++ binding of type ", $proto->full_name, " was not created as expected\n";
      }
   }
}

sub generate_lacking_function {
   my $lacking = new LackingFunctionInstance(@_);
   my $generator = $lacking->application->cpp->cpperl_generator;
   $generator->generate_lacking_function($lacking);
   if (defined(my $inst = &AutoFunction::find_instance)) {
      push @{$generator->lacking_functions}, $lacking;
      $inst
   } else {
      my $auto_func = shift;
      die "an instance of C++ ",
          defined($auto_func->caller_kind) ? ("function ", $auto_func->perl_name) : $auto_func->name,
          " with wrapper name ", $auto_func->wrapper_name, " was not created as expected\n";
   }
}

#######################################################################################
package Polymake::Core::CPlusPlus::Generator;

use Polymake::Struct (
   [ new => '$' ],
   [ '$perApp' => 'weak(#1)' ],
   '%lacking_types',                  # pkg => LackingTypeInstance for types with declared C++ binding but lacking generated wrappers
   '@lacking_functions',              # LackingFunctionInstance
   '@instances_to_remove',            # LackingXXXInstance describing duplicates or obsolete instances
);

my %enforce_rebuild;

sub new {
   my $self = &_new;
   if ($PrivateDir or $code_generation eq "shared" && $cpperl_src_root ne "") {
      state $finalize = $cpperl_src_root eq "" && add AtEnd("Finalize:C++", \&enforce_rebuild, after => "Private:C++");
      add AtEnd($self->application->name.":C++", sub { generate_cpperl_files($self) },
                before => [ "Customize", "Private:C++", "Finalize:C++", map { "$_:C++" } keys(%{$self->application->used}) ],
                after => "BigObject");
   }
   $self;
}

sub application { $_[0]->perApp->application }
sub shared_modules { $_[0]->perApp->shared_modules }

sub generate_lacking_type {
   my ($self, $lacking) = @_;
   load_shared_module(build_temp_shared_module($self, $lacking));
}

sub generate_lacking_function {
   my ($self, $lacking) = @_;
   my $so_name = build_temp_shared_module($self, $lacking);
   my $embedded_items_key = $self->application->name;
   if (is_object($lacking->extension) && $lacking->extension->is_bundled) {
      $embedded_items_key .= ":" . $lacking->extension->short_name;
   }

   load_shared_module($so_name);
   $self->perApp->bind_functions($lacking->extension, $root->functions->{$embedded_items_key}, 1);

   if (defined(my $embedded_rules = $root->embedded_rules->{$embedded_items_key})) {
      $#$embedded_rules = -1;
   }
   if (defined(my $duplicates = $root->duplicate_class_instances->{$embedded_items_key})) {
      $#$duplicates = -1;
   }
}

sub move_function_from_private {
   my ($self, $descr, $args)=@_;
   my $inst=new LackingFunctionInstance($descr->auto_func, $args);
   decide_about_private_extension($self, $inst);
   if (!$inst->is_private) {
      push @{$self->lacking_functions}, $inst;
      push @{$self->instances_to_remove}, inherit_class([ @$descr ], $descr);
      $descr->extension=$inst->extension;
      if (defined (my $return_type=$descr->return_type)) {
         if (defined (my $return_descr=delete $root->private_classes->{$return_type->pkg})) {
            move_type_from_private($self, $return_descr, $return_type, new LackingTypeInstance($return_type));
         }
      }
   }
}

sub move_type_from_private {
   my ($self, $descr, $proto, $inst)=@_;
   decide_about_private_extension($self, $inst);
   if (!$inst->is_private) {
      $self->lacking_types->{$proto->pkg}=$inst;
      my $descr_clone=inherit_class([ @$descr ], $descr);
      $descr_clone->extension=$private_wrapper_ext;
      push @{$self->instances_to_remove}, $descr_clone;
   }
   $root->classes->{$proto->pkg}=$descr;
}
#######################################################################################
sub decide_about_private_extension {
   my ($self, $lacking)=@_;
   if (!$lacking->is_private) {
      if (ref($lacking->extension) eq "ARRAY") {
         # If several independent extensions are mixtured, the wrapper must be banned into the private area.
         # But if exactly one of the extensions is stand-alone and writable, and all others are bundled,
         # we can remedy it by adding the dependencies between extensions.
         my @standalone=grep { !$_->is_bundled } @{$lacking->extension};
         if (@standalone==1) {
            my $ext=pop @standalone;
            unless ($lacking->is_private=!is_mutable_location($ext->build_dir)) {
               require Polymake::Core::InteractiveCommands;
               delete_from_list($lacking->extension, $ext);
               $ext->add_prerequisites(@{$lacking->extension});
               $lacking->extension=$ext;
               $lacking->override_extension;
            }
         } else {
            $lacking->is_private=1;
         }
      } else {
         my $host_extension= $lacking->extension // $self->application->origin_extension;
         if ($lacking->is_private= !is_mutable_location($host_extension && !$host_extension->is_bundled ? $host_extension->build_dir : $InstallArch)) {
            # The lacking function could be persistently instantiated in the public shared module if it were allowed to extend;
            # maybe the extension using this function is mutable?
            my $used_in_extension=used_in_extension();
            if (defined($used_in_extension) && $used_in_extension != $host_extension &&
                is_mutable_location($used_in_extension->build_dir)) {
               my $preserve_ext=$lacking->extension;
               set_extension($lacking->extension, $used_in_extension);
               if ($lacking->extension == $used_in_extension) {
                  # the extension which uses the function is indeed dependent on the defining extension (or it was just a core function)
                  $lacking->override_extension;
                  $lacking->is_private=0;
               } else {
                  # The using extension is not dependent on the defining one or is immutable.
                  # Restore the state and instantiate the wrapper in the private area
                  $lacking->extension=$preserve_ext;
               }
            }
         }
      }
   }
}
##############################################################################################
sub used_in_extension {
   my $used_in_extension;
   my $depth = 4;
   while (my ($file, $sub) = (caller(++$depth))[1, 3]) {
      if ($file =~ m{^(?'top' .*? (?: /bundled/ (?'bundled' $id_re))?) /apps/$id_re/ (?'where' rules|perllib|scripts|src|testsuite)}ox) {
         my ($top, $bundled, $where) = @+{qw(top bundled where)};
         unless ($where eq "rules" && $sub =~ /^Polymake::Core::(?:PropertyParamedType|PropertyTypeInstance)::new$/
                 || $top eq $InstallTop || $bundled) {
            $used_in_extension = $Extension::registered_by_dir{$top}
              and last;
         }
      }
   }
   $used_in_extension;
}
#######################################################################################
sub build_temp_shared_module {
   my ($self, $lacking) = @_;

   if ($code_generation eq "none") {
      croak( "cpperl interface generation is forbidden!\nMissing interface is:\n",
             JSON->new->canonical->pretty->encode($lacking->cpperl_definition), "\n " );
   }

   my $define_with = $lacking->cpperl_define_with;
   my $src_name = $define_with->in_cc_file;
   my $def_src_dir;
   if (defined($src_name)) {
      if (defined(my $ext = $define_with->extension)) {
         $def_src_dir = $ext->app_dir($self->application) . "/src";
      } else {
         $def_src_dir = $self->application->top."/src";
      }
      unless (-f "$def_src_dir/$src_name") {
         croak( "internal error: missing source file $def_src_dir/$src_name where the function ",
                $lacking->is_instance_of->name, " was initially defined" );
      }
   }

   &decide_about_private_extension;

   my $dir = new Tempdir();
   my $so_file = new Tempfile();
   my $so_name = $so_file->rename.".$DynaLoader::dl_dlext";
   my $cpperl_filename = $define_with->cpperl_filename;
   my $cpperl_file = new CPPerlFile("$dir/$cpperl_filename.cpperl", undef, $src_name);
   push @{$cpperl_file->instances}, $lacking->cpperl_definition;
   $cpperl_file->save($self->application);

   if ($lacking->needs_recognizer) {
      my $h = $lacking->h_file_vars;
      if ($Verbose::cpp) {
         dbg_print( -e $h->{filename} ? "Updating" : "Creating", " header file $h->{filename}" );
      }
      my $h_file = new HeaderFile($h);
      $h_file ->declarations .= $lacking->recognizing_type . "\n";
      $h_file->save($cpperl_src_root);
   }

   write_temp_build_ninja_file("$dir/build.ninja", $self->application, $lacking->extension,
                               $src_name, $def_src_dir, $cpperl_filename, $so_name);

   warn_print( "Compiling temporary shared module, please be patient..." ) if $Verbose::cpp;

   if (system(($Verbose::cpp>1 && "cat $dir/$cpperl_filename.cpperl >&2; ") .
              "ninja -C $dir $so_name ".($Verbose::cpp ? ">&2" : ">/dev/null")." 2>$dir/err.log")) {
      if ($Verbose::cpp) {
         die "C++/perl Interface module compilation failed; see the generated code and the error log below.\n\n",
             `cat $dir/$cpperl_filename.cc; echo ===========================================; cat $dir/err.log`;
      } else {
         die <<'.';
C++/perl Interface module compilation failed; most likely due to a type mismatch.
Set the variable $Polymake::User::Verbose::cpp to a positive value and repeat for more details.
.
      }
   }

   $so_name
}
##############################################################################################
sub write_temp_build_ninja_file {
   my ($file, $app, $extension, $src_name, $def_src_dir, $stem, $so_name) = @_;
   open my $conf, ">", $file
     or die "can't create $file: $!\n";
   print $conf <<"---";
config.file=$InstallArch/config.ninja
include \${config.file}
PERL=$Config::Config{perlpath}
---
   my $cxxflags = "-DPOLYMAKE_APPNAME=".$app->name;
   my $cxxincludes = '${app.includes} ${core.includes}';
   my $extra_ldflags = "";
   my $extra_libs = "";
   my $needs_bundled = false;
   if ($extension) {
      foreach my $ext (is_object($extension) ? ($extension, @{$extension->requires}) :
                       uniq(map { ($_, @{$_->requires}) } @{$extension})) {
         if (!$ext->is_bundled) {
            print $conf "include ", $ext->build_dir."/config.ninja\n";
         } else {
            $needs_bundled = true;
            my $bundled = $ext->short_name;
            $cxxflags .= " \${bundled.$bundled.CXXFLAGS}";
            $extra_ldflags .= " \${bundled.$bundled.LDFLAGS}";
            $extra_libs .= " \${bundled.$bundled.LIBS}";
            $cxxincludes = "\${bundled.$bundled.includes} $cxxincludes";
         }
      }
   }

   my $mode = $ENV{POLYMAKE_BUILD_MODE} || "Opt";
   print $conf <<"---";
include \${root}/support/rules.ninja
CmodeFLAGS=\${C${mode}FLAGS}
CexternModeFLAGS=\${Cextern${mode}FLAGS}
LDmodeFLAGS=\${LD${mode}FLAGS}
---
   if (is_object($extension) && $extension->is_bundled) {
      my $bundled = $extension->short_name;
      $cxxflags .= " -DPOLYMAKE_BUNDLED_EXT=$bundled";
   }
   if ($needs_bundled and -f "$InstallArch/../bundled_flags.ninja") {
      print $conf <<"---";
include $InstallArch/../bundled_flags.ninja
---
   }

   if (defined($src_name)) {
      $cxxflags .= qq{ -DPOLYMAKE_DEFINITION_SOURCE_DIR="$def_src_dir" -DPOLYMAKE_NO_EMBEDDED_RULES};
      my $build_flags_file = "$def_src_dir/build_flags.pl";
      if (-f $build_flags_file) {
         my %flags = do $build_flags_file;
         if (my @custom_flags = grep { defined } @flags{'CXXFLAGS', $src_name}) {
            $cxxflags .= " @custom_flags";
         }
      }
   }

   print $conf "build $stem.cc: gen_cpperl_mod $stem.cpperl\n",
               "  CPPERLextraFlags=--temp-module $so_name\n",
               "build $stem.o: cxxcompile $stem.cc\n",
               "  CXXextraFLAGS=$cxxflags\n",
               "  CXXincludes=$cxxincludes\n\n",
               "build $so_name: sharedmod $stem.o\n",
               "  LDextraFLAGS=$extra_ldflags\n",
               "  LIBSextra=$extra_libs\n";

   close $conf;
}
#######################################################################################
sub generate_cpperl_files {
   my ($self) = @_;
   my %cpperl_files;

   foreach my $inst (values %{$self->lacking_types}) {
      process_lacking_instance($self, \%cpperl_files, $inst);
   }

   foreach my $inst (@{$self->lacking_functions}) {
      process_lacking_instance($self, \%cpperl_files, $inst);
   }

   foreach my $inst (@{$self->instances_to_remove}) {
      remove_instance($self, \%cpperl_files, $inst);
   }

   %{$self->lacking_types} = ();
   @{$self->lacking_functions} = ();
   @{$self->instances_to_remove} = ();

   while (my ($filename, $file) = each %cpperl_files) {
      next unless defined($file);

      if ($Verbose::cpp) {
         dbg_print( -e $filename ? "Updating" : "Creating", " interface definition file $filename" );
      }
      $file->save($self->application, $cpperl_src_root);

      $file->rebuild_in =~ $directory_re;
      $enforce_rebuild{$1} = 1;
   }
}
#######################################################################################
sub enforce_rebuild {
   # remove the build success markers in all modes, not only in the current one
   unlink(map { glob("$_/*/.apps.built") } keys %enforce_rebuild);
}
#######################################################################################
sub process_lacking_instance {
   my ($self, $files, $inst) = @_;
   my $document = $inst->cpperl_definition;
   if ($inst->is_private) {
      $PrivateDir or return;
      $private_wrapper_ext //= PrivateWrappers->create;
      if (defined $inst->extension) {
         $private_wrapper_ext->ensure_prerequisites($inst->extension);
         $document->{ext} = [ map { $_->is_bundled ? $_->short_name : $_->dir }
                                  is_object($inst->extension) ? $inst->extension : @{$inst->extension} ];
      }
   }
   if (my $cpperl_file = load_cpperl_file($self, $files, $inst->cpperl_define_with->cpperl_filename, $inst, 1)) {
      push @{$cpperl_file->instances}, $document;
   }
}
#######################################################################################
sub remove_instance {
   my ($self, $files, $inst) = @_;
   my $cpperl_file = load_cpperl_file($self, $files, $inst->cpperl_file, $inst, 0)
     or return;
   my $inst_num = $inst->cpperl_file+0;
   if ($inst_num < @{$cpperl_file->instances} &&
       $inst->match_cpperl_instance($cpperl_file->instances->[$inst_num])) {
      $cpperl_file->has_removed_instances = true;
      delete $cpperl_file->instances->[$inst_num];
   } else {
      warn_print( "contents of interface definition file ", $cpperl_file->filename, " must have been manipulated\n",
                  "can't locate the duplicate instance at original position $inst_num" );
   }
}
#######################################################################################
sub load_cpperl_file {
   my ($self, $files, $cpperl_filename, $inst, $create_if_missing) = @_;
   my $in_private = $inst->is_private;
   my $src_top = $in_private
                 ? $private_wrapper_ext->dir :
                 defined($inst->extension)
                 ? $inst->extension->dir
                 : $self->application->installTop;

   my $shared_mod = $self->shared_modules->{!$in_private && $inst->extension && $inst->extension->is_bundled ? $InstallTop : $src_top};
   # if the extension has been obliterated, this file won't be registered in %files and thus avoids modification.
   return if defined($shared_mod) && !defined($shared_mod->so_name);

   my $dir = "$src_top/apps/". $self->application->name . "/cpperl";
   my $filename = "$dir/$cpperl_filename.cpperl";

   $files->{$filename} //= do {

      my $rebuild_in = $in_private ? $private_wrapper_ext : $inst->extension // $self->application->origin_extension;
      if (defined($rebuild_in) && !$rebuild_in->is_bundled) {
         $rebuild_in = $rebuild_in->build_dir;
      } else {
         $rebuild_in = $InstallArch;
      }

      if (-r $filename and -w _ || $cpperl_src_root ne "") {
         my $file_mtime = (stat _)[9];
         if (defined(my $last_built = (stat "$rebuild_in/.apps.built")[9])) {
            if ($file_mtime > $last_built) {
               warn_print( <<"." );
Automatic update of the interface definition file $filename refused:
The compiled subtree $rebuild_in is out-of-date.
It will be rebuilt automatically at the begin of the next polymake session.
.
               $rebuild_in =~ $directory_re;
               $enforce_rebuild{$1} = true;
               return;
            }
         } else {
            warn_print( <<"." );
Automatic update of the interface definition file $filename refused:
Timestamp file $rebuild_in/.apps.built is missing unxpectedly.
It will be rebuilt automatically at the begin of the next polymake session.
.
            return;
         }

         my $file = new CPPerlFile($filename, $rebuild_in);
         if ($create_if_missing && defined(my $embed = $inst->cpperl_define_with->in_cc_file)) {
            if ($embed ne $file->embed) {
               warn_print( <<"." );
The interface definition file $filename is corrupted.
It was expected to contain an attribute "embed": "$embed".
.
               return;
            }
         }
         $file

      } elsif (-e _) {
         warn_print( <<"." );
The interface definition file $filename exists but can't be updated due to lacking permissions.
Until this file is writable for you, you can't maintain persistent C++ bindings!
.
         return;

      } elsif ($create_if_missing) {
         if (-d $dir) {
            unless (-r _ && -w _ or $cpperl_src_root ne "") {
               warn_print( <<"." );
The interface definition file $filename can't be created due to lacking permissions.
Until the directory $dir is writable for you, you can't maintain persistent C++ bindings!
.
               return;
            }
         }
         new CPPerlFile($filename, $rebuild_in, $inst->cpperl_define_with->in_cc_file)

      } else {
         warn_print( <<"." );
The interface definition file $filename which must have contained
a duplicate definition of a function or class does not exist.
Should this message appear repeatedly after every session, try a clean build
of the component or extension where this file was supposed to reside.
.
         return;
      }
   };
}

1;


# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
