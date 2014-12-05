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

package Polymake::Core::Application;

declare $configured_at=2;         # enforce renewal of obsolete config status values=1
declare $load_time=time;
declare $plausibility_checks=1;

declare $extension;               # Extension contributing to the application being loaded right now
declare $cross_apps_list;         # REQUIRE_APPLICATION active in the current compilation scope

# flags for `declared' member
use Polymake::Enum qw( namespace_declared=1
                       main_init_closed=2
                       cpp_load_initiated=4
                       credits_shown=8
                       has_failed_config=16
                     );

my (%repository, $user_prefs_help);

#################################################################################
#
#  Constructor:
#
#  new Application('name', [ rules to load ]);
#
use Polymake::Struct (
   [ new => '$' ],
   [ '$name' => '#1' ],
   [ '$pkg' => '"Polymake::" . #1' ],
   [ '$top' => 'undef' ],         # application top directory (beneath apps/)
   [ '$installTop' => 'undef' ],  # installation directory
   '@myINC',                      # directories with perl modules
   '@scriptpath',                 # directories with scripts
   '@object_types',               # ObjectType
   [ '$default_type' => 'undef' ],
   '@rules',                      # Rule
   '@rules_to_finalize',          # Rule - production rules defined in the rulefiles being currently read
   '%rulefiles',                  # 'rule_key' => load status
   [ '$configured' => 'undef' ],  # 'rule_key' => configuration result
   '$configured_at',              # last configuration timestamp (0 for core applications unless started with --reconfigure)
   '%rule_code',                  # 'rule_key' => CODE : the complete rulefile-level subroutine
   '%credits',                    # product name => Rule::Credit
   '%credits_by_rulefile',        # 'rule_key' => Rule::Credit
   '%preamble_end',               # 'rule_key' => last line containing a configuration command (CONFIGURE or REQUIRE)
   [ '$custom' => 'undef' ],      # Customize::perApplication
   [ '$prefs' => 'undef' ],       # Preference::perApplication
   [ '$help' => 'new Help' ],
   '%used',                       # 'name' => Application
   '%imported',                   # 'name' => 1
   '@import_sorted',              # 'name', ... : flattened list in DFS order
   '%EXPORT',                     # names and attributes of user functions and methods
   [ '$compile_scope' => 'undef' ],  # Scope object spanning the load phase of a group of rules
   '$untrusted',                  # TRUE if comes from a writable location, that is, may be under development
   '&eval_expr',                  # eval'uating the given source code in the application-specific lexical context
   [ '$cpp' => 'undef' ],         # CPlusPlus::perAplication
   [ '$declared' => '0' ],        # flags indicating the rule loading progress
   [ '$default_file_suffix' => 'undef' ],
   '@file_suffixes',
   [ '$origin_extension' => 'undef' ],  # Extension where this application has been introduced
   '@extensions',                 # Extension : extensions contributing to this application (without origin_extension)
);

sub new {
   my $self=&_new;
   if (-d (my $top="$InstallTop/apps/".$self->name)) {
      if (-f "$top/rules/main.rules") {
         $self->installTop=$InstallTop;
         $self->top=$top;
         $self->configured_at=$configured_at;
         $self->untrusted=$DeveloperMode && -w _;
      } else {
         croak( "Corrupt application ", $self->name, ": missing main.rules file in $top" );
      }
   } else {
      foreach $extension (@Extension::active[$Extension::num_bundled..$#Extension::active]) {
         if (-d ($top=$extension->app_dir($self))) {
            if (-f "$top/rules/main.rules") {
               $self->installTop=$extension->dir;
               $self->top=$top;
               $self->configured_at=$extension->configured_at;
               $self->untrusted=-w _;
               $self->origin_extension=$extension;
               last;
            } else {
               croak( "Extension ", $extension->dir, " contributes to application ", $self->name,
                      " but does not provide main.rules: missing dependencies and/or wrong order of extensions?" );
            }
         }
      }
      if (!defined($self->top)) {
         croak( "Unknown application ", $self->name );
      }
   }
   define_function(     # for the rule parser
      $self->pkg, "self",
      sub {
         if (my $what=shift) {
            if ($what<0) {
               if ($what==-1) {
                  # providing help path for scopes without own prototype object
                  my $pkg=caller;
                  $self->help->find("objects", $pkg) || $self->help->add(["objects", $pkg]);
               } else {
                  $self;
               }
            } else {
               croak( "This declaration is only allowed in the object type scope" );
            }
         } else {
            $self;
         }
      },
      1);

   my $dir;
   if (-d ($dir=$self->top."/perllib")) {
      push @{$self->myINC}, $dir;
   }
   if (-d ($dir=$self->top."/scripts")) {
      push @{$self->scriptpath}, $dir;
   }

   local $plausibility_checks=$self->untrusted;
   local $Shell=new NoShell();         # disable interactive configuration

   $self->configured=namespaces::declare_var($self->pkg, "%configured");
   $self->custom=$Custom->app_handler($self->pkg);
   $self->custom->add('%configured', <<'.', $Customize::state_config | $Customize::state_hidden | $Customize::state_accumulating, $self->pkg);
Rulefiles with autoconfiguration sections and their exit codes.
Value 0 denotes configuration failure, which disables the corresponding rulefile.
.
   $self->prefs=$Prefs->app_handler($self);
   $self->cpp=new CPlusPlus::perApplication($self);
   include_rules($self);

   foreach $extension (@Extension::active) {
      if ($extension->dir ne $self->installTop && -d ($dir=$extension->app_dir($self))) {
         load_extension($self, $dir);
      }
   }

   $self->cpp->load_private_wrapper;

   $self->prefs->end_loading;
   if (length(my $cmds=$self->prefs->user_commands)) {
      local $User::application=$self;
      local_unshift(\@INC, $self);
      $self->eval_expr->("package Polymake::User; $cmds");
      die $@ if $@;
   }

   if ($Help::gather) {
      $self->custom->create_help_topics($self->help);
      unless ($user_prefs_help) {
         $user_prefs_help=new Help;
         $Prefs->custom->create_help_topics($user_prefs_help);
      }
      push @{$self->help->related}, $user_prefs_help;
   }

   $self;
}
#################################################################################
# private:
sub load_extension {
   my ($self, $app_dir)=@_;
   my $dir;
   if (-d ($dir="$app_dir/perllib")) {
      push @{$self->myINC}, $dir;
   }
   if (-d ($dir="$app_dir/scripts")) {
      push @{$self->scriptpath}, $dir;
   }

   local $plausibility_checks=$extension->untrusted;
   include_rules($self);
   push @{$self->extensions}, $extension;
}
#################################################################################
sub add {
   $repository{$_[1]} // do {
      # must access the repository hash twice in order to create the entry before loading the rules:
      # this helps to detect cyclic dependencies
      $repository{$_[1]} //= (my $self=&new);
      SuspendedItems::harvest($self->name);
      $self
   };
}
sub lookup {
   $repository{$_[1]}
}
sub list {
   values %repository;
}
sub known {
   keys %repository;
}
sub delete {
   my ($self, $name)=@_;
   delete $repository{$name};
}
#################################################################################
# "rulefile" => ( full_path, extension, rule_key, cached_result_code )
# 'rule_key' is a string used as a key in various maps like rulefiles, configured, or rule_code
sub lookup_rulefile {
   my ($self, $rulename, $only_here)=@_;
   my ($filename, $ext, $rule_key, $rc, @accumulated);
   if (defined $extension) {
      # initial loading an extension: first look in the same extension, then in the application root, then in required extensions
      $rule_key=$rulename.'@'.$extension->URI;
      $filename=$extension->app_dir($self)."/rules/$rulename";
      if (defined ($rc=$self->rulefiles->{$rule_key}) or -f $filename) {
         if ($only_here<0) {
            push @accumulated, [ $filename, $extension, $rule_key, $rc ];
         } else {
            return ($filename, $extension, $rule_key, $rc);
         }
      }
      unless ($only_here) {
         $filename=$self->top."/rules/$rulename";
         if (defined ($rc=$self->rulefiles->{$rulename}) or -f $filename) {
            return ($filename, undef, $rulename, $rc);
         }
         foreach $ext (@{$extension->requires}) {
            $rule_key=$rulename.'@'.$ext->URI;
            $filename=$ext->app_dir($self)."/rules/$rulename";
            if (defined ($rc=$self->rulefiles->{$rule_key}) or -f $filename) {
               return ($filename, $ext, $rule_key, $rc);
            }
         }
      }

   } elsif ($rulename =~ m{^/}) {
      if (-f $rulename) {
         # catch rules specified by absolute path
         $filename=$rulename;
         ($rulename) = $filename =~ $filename_re;
         if (index($filename, $self->top."/rules/")==0) {
            return ($filename, undef, $rulename, $self->rulefiles->{$rulename});
         }
         foreach $ext (@{$self->extensions}) {
            if (index($filename, $ext->app_dir($self)."/rules/")==0) {
               $rule_key=$rulename.'@'.$ext->URI;
               return ($filename, $ext, $rule_key, $self->rulefiles->{$rule_key});
            }
         }
         # stray rulefile without registered extension, presumably user's private experiments
         return ($filename, undef, $filename, $self->rulefiles->{$filename});
      }

   } else {
      # loading on demand: first look in the application root, then in all extensions
      $filename=$self->top."/rules/$rulename";
      if (defined ($rc=$self->rulefiles->{$rulename}) or -f $filename) {
         if ($only_here<0) {
            push @accumulated, [ $filename, undef, $rulename, $rc ];
         } else {
            return ($filename, undef, $rulename, $rc);
         }
      }
      foreach $ext (@{$self->extensions}) {
         $filename=$ext->app_dir($self)."/rules/$rulename";
         $rule_key=$rulename.'@'.$ext->URI;
         if (defined ($rc=$self->rulefiles->{$rule_key}) or -f $filename) {
            if ($only_here<0) {
               push @accumulated, [ $filename, $ext, $rule_key, $rc ];
            } else {
               return ($filename, $ext, $rule_key, $rc);
            }
         }
      }
   }

   @accumulated
}
#################################################################################
sub exclude_rule {
   my ($self, $rulename)=@_;
   if ($rulename =~ /^($id_re)::(.*)/o) {
      exclude_rule($self->used->{$1} || croak( "application $1 is not declared as USE'd or IMPORT'ed - may not load its rules here" ), "$2");
   } else {
      if (my ($filename, $ext, $rule_key)=lookup_rulefile($self, $rulename)) {
         # don't exclude if already loaded from elsewhere
         if (!exists $self->rulefiles->{$rule_key}) {
            $self->rulefiles->{$rule_key}=0;
            if (defined (delete $self->configured->{$rule_key})) {
               $self->custom->handler->need_save=1;
            }
         }
      } else {
         croak( "rule file ", $self->name, "::$rulename does not exist" );
      }
   }
}
#################################################################################
# private
sub include_rule {
   my ($self, $rulefile, $only_here)=@_;
   if ($rulefile =~ m{^($id_re)::(.*)}o) {
      include_rules($self->used->{$1} || croak( "application $1 is not declared as USE'd or IMPORT'ed - may not load its rules here" ), "$2");
   } elsif ($rulefile =~ s{^\*/(.*)}{$1}) {
      my $rc_all=0;
      foreach (lookup_rulefile($self, $rulefile, -1)) {
         $rc_all += process_included_rule($self, $rulefile, @$_);
      }
      $rc_all;
   } elsif (my @found=&lookup_rulefile) {
      process_included_rule($self, $rulefile, @found);
   } else {
      $only_here || croak( "rule file ", ($rulefile =~ m{^/} ? $rulefile : $self->name."::$rulefile"), " does not exist" );
   }
}
#################################################################################
sub include_rules {
   my $self=shift;
   is_code($INC[0]) ? $User::application==$self ? undef : local_unshift(\@INC, $self) :
   is_object($INC[0]) ? $INC[0]==$self || (local $INC[0]=$self) : local_unshift(\@INC, $self);
   local_array($self->custom->tied_vars, []);
   local_scalar($self->compile_scope, new Scope());
   $self->compile_scope->cleanup->{$self->custom}=undef;
   my $rc_all=0;
   eval {
      if (@_) {
         # application is already loaded, adding some optional rulefiles
         foreach my $rulename (@_) {
            $rc_all += include_rule($self, $rulename);
         }
      } else {
         # initializing the application or its main part in an extension
         $self->declared &= ~$cpp_load_initiated;
         if ($rc_all=include_rule($self, "main.rules", 1)) {
            unless ($self->declared & $cpp_load_initiated) {
               # no rulefiles at all; nevertheless, there might be clients
               $self->declared |= $cpp_load_initiated;
               $self->cpp->start_loading($extension);
            }
            $self->cpp->end_loading($extension);
         }
      }
      $_->finalize for @{$self->rules_to_finalize};
   };
   if ($@) {
      die beautify_error();
   }
   @{$self->rules_to_finalize}=();
   $rc_all;
}
#################################################################################
sub include_rule_block {
   my ($self, $mandatory, $block)=@_;
   while ($block =~ /\G \s* (\S+)/gx) {
      my @failed;
      my $rulefile=$1;
      my $success=include_rule($self, $rulefile);
      if ($mandatory) {
         push @failed, $rulefile if !$success;
      } else {
         while ($block =~ /\G \s+\+\s+ (\S+)/gxc) {
            $rulefile=$1;
            $success += include_rule($self, $rulefile);
         }
      }
      while ($block =~ /\G \s+\|\s+ (\S+)/gxc) {
         $rulefile=$1;
         if ($success) {
            exclude_rule($self, $rulefile) if !$mandatory;
         } else {
            $success=include_rule($self, $rulefile);
            push @failed, $rulefile if $success < $mandatory;
         }
      }
      return @failed if $success < $mandatory;
   }
   ()
}
#################################################################################
sub eval_type {
   my ($self, $expr, $allow_generic)=@_;
   $self->eval_expr->($allow_generic && $expr !~ /[<>]/ ? "typeof_gen $expr" : "typeof $expr")
}

# called from C++ clients
sub eval_type_throw {
   &eval_type // croak( $@ =~ /^invalid type expression/ ? "$& $_[1]" : "Error processing type expression $_[1]: $@" );
}
#################################################################################
sub set_file_suffix {
   my ($self, $suffix)=@_;
   ($self->default_file_suffix &&= croak("multiple definition of default file suffix")) ||= $suffix;
   push @{$self->file_suffixes}, $suffix;
}
#################################################################################
sub add_production_rule {
   my $self=shift;
   my $rule=new Rule(@_);
   push @{$self->rules}, $rule;
   push @{$self->rules_to_finalize}, $rule->needs_finalization;
}

sub add_default_value_rule {
   my $self=shift;
   my $rule=new Rule(@_);
   $rule->append_weight(0,0);
   $rule->flags=$Rule::is_default_value;
   push @{$self->rules}, $rule;
   push @{$self->rules_to_finalize}, $rule;
}

sub append_rule_precondition {
   my ($self, $header, $code, $proto, $checks_definedness)=@_;
   $self->rules->[-1]->append_precondition(special Rule($header, $code, $proto), $checks_definedness);
}

sub append_rule_existence_check {
   my $self=shift;
   $self->rules->[-1]->append_existence_check(@_);
}

sub append_rule_weight {
   my ($self, $major, $minor, $header, $code, $proto)=@_;
   $self->rules->[-1]->append_weight($major, $minor, defined($header) && special Rule($header, $code, $proto));
}

sub append_rule_permutation {
   my ($self, $perm_name, $proto)=@_;
   my $perm=$proto->property($perm_name);
   unless ($perm->flags & $Property::is_permutation) {
      croak( "$perm_name is not declared as a permutation" );
   }
   $self->rules->[-1]->append_permutation($perm);
}

sub append_overridden_rule {
   my ($self, $proto, $super_proto, $label)=@_;
   if ($plausibility_checks && is_object($super_proto)) {
      $proto->isa($super_proto) or croak( "Invalid override: ", $proto->full_name, " is not derived from ", $super_proto->full_name );
   }
   $label=$self->prefs->find_label($label)
          || croak( "Unknown label $label" );
   push @{$self->rules->[-1]->overridden_in ||= [ ]}, [ $super_proto, $label, $plausibility_checks ? (caller)[1,2] : () ];
}
#################################################################################
sub lookup_credit {
   my ($self, $product)=@_;
   $self->credits->{$product} ||= do {
      my @full_credits=grep { defined } map { $_->credits->{$product} } values %{$self->used};
      $full_credits[0];
   }
}
#################################################################################
sub add_custom {
   my $self=shift;
   $self->custom->add(@_)->extension=$extension;
}
#################################################################################
sub use_apps {
   my ($self, $import)=splice @_,0,2;
   my ($i, $app);

   my $in_ext=$extension;
   local $extension;

   my @apps=map {
      if (my ($appname, $start_rules)= m/^\s* ($id_re) (?: \s*\(\s* (.*?) \s*\) )? \s*$/xo) {
         if (exists $repository{$appname} && !defined $repository{$appname}) {
            # requested application is itself being loaded
            croak( "Cyclic dependence between applications ", $self->name, " and $appname" );
         }
         $app=add Application($appname);
         if (exists $app->used->{$self->name}) {
            if ($in_ext) {
               croak( "Extension ", $in_ext->URI,
                      " attempts to introduce a cyclic dependence between applications ", $self->name, " and $appname" );
            } else {
               croak( "Cyclic dependence between applications ", $self->name, " and $appname" );
            }
         }
         if (defined($start_rules) &&
             scalar(include_rule_block($app, 1, $start_rules))) {
            croak( "could not load mandatory rulefile(s) $start_rules due to configuration issues" );
         }
         $app;
      } else {
         croak( "invalid application name '$_'" );
      }
   } @_;

   if ($import) {
      my $ord=-1;
      my %order=map { $_ => ++$ord } @{$self->import_sorted};

    APPS:
      for ($i=0; $i<=$#apps; ++$i) {
         $app=$apps[$i];
         if (exists $self->imported->{$app->name}) {
            splice @apps, $i--, 1;  next;
         }
         for (my $j=$i+1; $j<=$#apps; ++$j) {
            if (exists $apps[$j]->imported->{$app->name}) {
               splice @apps, $i--, 1; next APPS;
            }
         }

         push @{$self->myINC}, @{$app->myINC};
         push %{$self->EXPORT}, %{$app->EXPORT};
         push %{$self->credits}, %{$app->credits};

         namespaces::using($self->pkg, $app->pkg);
         namespaces::using($self->pkg."::objects", $app->pkg."::objects");
         namespaces::using($self->pkg."::props", $app->pkg."::props");

         $self->imported->{$app->name}=$app;
         $order{$app->name}=++$ord;

         # IMPORT relation is transitive
         while (my ($other_app_name, $other_app)=each %{$app->imported}) {
            $self->imported->{$other_app_name}=$other_app;
            assign_min($order{$other_app_name},$ord);
         }
      }

      my @import_sorted=sort {
         exists $a->imported->{$b->name} ? -1 :
         exists $b->imported->{$a->name} ? 1 :
         $order{$a->name} <=> $order{$b->name}
      } values %{$self->imported};

      @{$self->import_sorted}=map { $_->name } @import_sorted;
      @{$self->prefs->imported}=map { $_->prefs } @import_sorted;
      if ($Help::gather) {
         push @{$self->help->related}, map { $_->help } @import_sorted;
      }
   }

   foreach $app (@apps) {
      $self->used->{$app->name}=$app;
      # flatten the %used
      push %{$self->used}, %{$app->used};
   }
}

sub common {
   my ($self, $other)=@_;
   if (!defined($other) || $self==$other || exists $self->used->{$other->name}) {
      $self;
   } elsif (exists $other->used->{$self->name}) {
      $other;
   } else {
      undef;
   }
}

#################################################################################
sub find_custom_var {
   my ($self, $name, $alt)=@_;
   my $var;
   if (defined ($self->custom) && !defined ($var=$self->custom->find($name))) {
      foreach my $app (values %{$self->used}) {
         $var=$app->custom->find($name) and last;
      }
   }
   if (defined($var)) {
      ($self->custom, $var)
   } elsif (defined($alt) && defined ($var=$alt->find($name))) {
      ($alt, $var)
   } else {
      croak( "unknown custom variable $name" );
   }
}

# for clients and callable library:
# 'prefixed varname', [key] => value
sub get_custom_var {
   my $var=find_custom_var($User::application, shift, $Prefs->custom);
   no strict 'refs';
   if ($var->prefix eq '%') {
      if (@_) {
         ${$var->name}{$_[0]}
      } else {
         *{$var->name}{HASH}
      }
   } elsif ($var->prefix eq '@') {
      *{$var->name}{ARRAY}
   } else {
      ${$var->name}
   }
}

sub _set_custom {
   (my ($self, $alt, $name, @tail)=@_)>1 or croak( "custom variable, array, hash, or hash element assignment expected" );
   my ($bunch, $var)=find_custom_var($self,$name,$alt);
   $var->set(@tail);
   $bunch->handler->need_save=1;
}

sub set_custom {
   _set_custom($_[0], undef, name_of_custom_var(1));
}

sub _reset_custom {
   (my ($self, $alt, $name, @tail)=@_)>1 or croak( "custom variable, array, hash, or hash element expected" );
   my ($bunch, $var)=find_custom_var($self,$name,$alt);
   $var->reset(@tail);
   $bunch->handler->need_save=1;
}

sub reset_custom {
   _reset_custom($_[0], undef, name_of_custom_var(0));
}
#################################################################################
sub add_top_label {
   my ($self, $name)=@_;
   my $label=( $self->prefs->labels->{$name} &&=
               croak( "multiple definition of label $name" ) )=new Preference::Label($name);
   $label->application=$self;
   $label->extension=$extension;
   $label
}

sub add_label {
   my ($self, $name)=@_;
   my $label=$self->prefs->find_label($name, 1) or do {
      $name =~ /^($id_re)/o;
      croak( "unknown label '$1'" );
   };
   $label->set_application($self, $extension);
   $label
}

sub add_labels {
   my ($self, $labels)=@_;
   map { add_label($self, $_) } split /\s*,\s*/, $labels
}
#################################################################################
sub prefer {
   my $self=shift;
   $self->prefs->add_preference(@_);
}

sub prefer_now {
   my $self=shift;
   $self->prefs->set_temp_preference($Scope, @_);
}

# an alias, for the sake of symmetry
*set_preference=\&prefer;

sub reset_preference {
   my $self=shift;
   if ($_[0] eq "all" || $_[0] eq "*") {
      $self->prefs->handler->reset_all($self);
   } else {
      $self->prefs->handler->reset($self, @_);
   }
}
#################################################################################
sub disable_rules {
   my ($self, $pattern)=@_;
   if ($pattern =~ /^ $hier_id_re $/xo) {
      # specified by label
      my $label=$self->prefs->find_label($pattern)
        or die "unknown label \"$pattern\"\n";
      my @rules=$label->list_all_rules
        or die "no matching rules found\n";
      Scheduler::temp_disable_rules(@rules);

   } elsif ($pattern =~ /:/) {
      # specified by header:
      Rule::header_search_pattern($pattern);
      my @rules=grep { $_->header =~ $pattern } @{$self->rules}
        or die "no matching rules found\n";
      Scheduler::temp_disable_rules(@rules);

   } else {
      die "usage: disable_rules(\"label\" || \"OUTPUT : INPUT\")\n";
   }
}

#################################################################################
package _::SuspendedItems;

use Polymake::Struct (
   [ new => '$$@' ],
   [ '$application' => '#1' ],
   [ '$extension' => '#2' ],
   [ '@further_missing_apps' => '@' ],
   '@rulefiles',
   '@rule_keys',
   '@embedded_rules',
   '@functions',
);

my %suspended;

sub add {
   my ($application, $extension, $missing_app_name, @further_missing_apps)=@_;
   my $list=($suspended{$missing_app_name} //= [ ]);
   my $self;
   # Only look at the end of the list and stop by first application or extension mismatch,
   # because all suspended items are created at the same time, when the owning application is loaded.
   for (my $i=$#$list; $i>=0; --$i) {
      $self=$list->[$i];
      if ($self->application == $application && $self->extension == $extension) {
         if (equal_lists($self->further_missing_apps, \@further_missing_apps)) {
            return $self;
         }
      } else {
         last;
      }
   }
   $self=new(__PACKAGE__, $application, $extension, @further_missing_apps);
   push @$list, $self;
   $self
}

sub harvest {
   my ($app_name)=@_;

   if (defined (my $list=delete $suspended{$app_name})) {
      foreach my $self (@$list) {
         if (my @missing_apps=grep { !exists $repository{$_} } @{$self->further_missing_apps}) {
            # Associate the suspended items with another missing application.
            my $missing_app_name=shift @missing_apps;
            my $next_list=($suspended{$missing_app_name} //= [ ]);
            # Look though the entire list, because anything might have happened since the initial load of the owning application.
            foreach my $next_suspended (@$next_list) {
               if ($next_suspended->application == $self->application &&
                   $next_suspended->extension == $self->extension &&
                   equal_lists($next_suspended->further_missing_apps, \@missing_apps)) {

                  push @{$next_suspended->rulefiles}, @{$self->rulefiles};
                  push @{$next_suspended->rule_keys}, @{$self->rule_keys};
                  push @{$next_suspended->embedded_rules}, @{$self->embedded_rules};
                  push @{$next_suspended->functions}, @{$self->functions};
                  undef $self;
                  last;
               }
            }
            if ($self) {
               @{$self->further_missing_apps}=@missing_apps;
               push @$next_list, $self;
            }
         } else {
            # ripe to be loaded
            my $app=$self->application;
            local $extension=$self->extension;
            local_unshift(\@INC, $app);
            local_scalar($app->compile_scope, new Scope());
            eval {
               if (@{$self->rulefiles}) {
                  delete @{$app->rulefiles}{@{$self->rule_keys}};
                  delete @INC{ map { "rules:$_" } @{$self->rulefiles} };
                  foreach my $rulefile (@{$self->rulefiles}) {
                     include_rule($app, $rulefile);
                  }
               }
               $self->application->cpp->load_suspended($self);
            };
            if ($@) {
               die beautify_error();
            }
         }
      }
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
