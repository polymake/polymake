#  Copyright (c) 1997-2020
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

package Polymake::Core::Application;

declare $configured_at = 2;       # enforce renewal of obsolete config status values=1
declare $load_time = time;

declare $extension;               # Extension contributing to the application being loaded right now
declare $cross_apps_list;         # REQUIRE_APPLICATION active in the current compilation scope

# flags for `load_state' member
use Polymake::Enum LoadState => {
   start => 0,
   namespace_declared => 1,
   main_init_closed => 2,
   cpp_load_initiated => 4,
   credits_shown => 8,
   has_failed_config => 16
};

my %repository;

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
   '@myINC',                      # [ directory with perl modules, optional preamble lines ]
   '@scriptpath',                 # directories with scripts
   '@object_types',               # BigObjectType
   '@rules',                      # Rule
   '@rules_to_finalize',          # Rule - production rules defined in the rulefiles being currently read
   '%rulefiles',                  # 'rule_key' => load status
   [ '$configured' => 'undef' ],  # 'rule_key' => configuration result
   '$configured_at',              # last configuration timestamp (2 for core applications unless started with --reconfigure)
   '%rule_code',                  # 'rule_key' => CODE : the complete rulefile-level subroutine
   '%credits',                    # product name => Rule::Credit
   '%credits_by_rulefile',        # 'rule_key' => Rule::Credit
   '%preamble_end',               # 'rule_key' => last line containing a configuration command (CONFIGURE or REQUIRE)
   [ '$custom' => 'undef' ],      # Customize::perApplication
   [ '$prefs' => 'undef' ],       # Preference::perApplication
   [ '$help' => 'new Help(undef, #1)' ],
   '%used',                       # 'name' => Application
   '%imported',                   # 'name' => true
   '@linear_imported',            # [ Application ] flattened list in C3 order
   [ '$compile_scope' => 'undef' ],  # Scope object spanning the load phase of a group of rules
   '$untrusted',                  # TRUE if comes from a writable location, that is, may be under development
   '&eval_expr',                  # eval'uating the given source code in the application-specific lexical context
   [ '$cpp' => 'undef' ],         # CPlusPlus::perAplication
   [ '$load_state' => 'LoadState::start' ],   # flags indicating the rule loading progress
   [ '$default_file_suffix' => 'undef' ],
   '@file_suffixes',
   [ '$origin_extension' => 'undef' ],  # Extension where this application has been introduced
   '@extensions',                 # Extension : extensions contributing to this application (without origin_extension)
);

sub new {
   my $self = &_new;
   if (-d (my $top = "$InstallTop/apps/".$self->name)) {
      if (-f "$top/rules/main.rules") {
         $self->installTop = $InstallTop;
         $self->top = $top;
         $self->configured_at = $configured_at;
         $self->untrusted = $DeveloperMode && -w _;
      } else {
         croak( "Corrupt application ", $self->name, ": missing main.rules file in $top" );
      }
   } else {
      foreach my $ext (@Extension::active[$Extension::num_bundled..$#Extension::active]) {
         if (-d ($top = $ext->app_dir($self))) {
            if (-f "$top/rules/main.rules") {
               $self->installTop = $ext->dir;
               $self->top = $top;
               $self->configured_at = $ext->configured_at;
               $self->untrusted = -w _;
               $self->origin_extension = $ext;
               last;
            } else {
               croak( "Extension ", $ext->dir, " contributes to application ", $self->name,
                      " but does not provide main.rules: missing dependencies and/or wrong order of extensions?" );
            }
         }
      }
      if (!defined($self->top)) {
         croak( "Unknown application ", $self->name );
      }
   }
   RuleFilter::create_self_method_for_application($self);
   {
      no strict 'refs';

      # for quick retrieval in C++ library
      readonly(${$self->pkg."::.APPL"} = $self);

      # prepare an artificial package for proper ordering
      mro::set_mro($self->pkg."::.IMPORTS", "c3");
   }

   my $dir;
   if (-d ($dir = $self->top."/perllib")) {
      push @{$self->myINC}, [ $dir ];
   }
   if (-d ($dir = $self->top."/scripts")) {
      push @{$self->scriptpath}, $dir;
   }

   local $enable_plausibility_checks = $self->untrusted;
   local $Shell = new NoShell();         # disable interactive configuration

   $self->configured = namespaces::declare_var($self->pkg, "%configured");
   $self->custom = $Custom->app_handler($self->pkg);
   $self->custom->add('%configured', <<'.', Customize::State::config | Customize::State::hidden | Customize::State::accumulating, $self->pkg);
Rulefiles with autoconfiguration sections and their exit codes.
Value 0 denotes configuration failure, which disables the corresponding rulefile.
.
   $self->prefs = $Prefs->app_handler($self);
   $self->cpp = new CPlusPlus::perApplication($self);
   include_rules($self);

   foreach my $ext (@Extension::active) {
      if ($ext->dir ne $self->installTop && -d ($dir = $ext->app_dir($self))) {
         load_extension($self, $ext, $dir);
      }
   }

   $self->cpp->load_private_wrapper;
   $self->custom->end_loading;
   $self->prefs->end_loading;
   if (length(my $cmds = $self->prefs->user_commands)) {
      local $User::application = $self;
      local unshift @INC, $self;
      $self->eval_expr->("package Polymake::User; $cmds");
      die $@ if $@;
   }

   if ($Help::gather) {
      push @{$self->help->related}, $Help::core;
      $self->custom->create_help_topics($self->help);
      state $user_prefs_help = $Prefs->custom->create_help_topics($Help::core), 1;
   }

   $self;
}
#################################################################################
# private:
sub load_extension {
   my ($self, $ext, $app_dir)=@_;
   my $dir;
   if (-d ($dir = "$app_dir/perllib")) {
      push @{$self->myINC}, [ $dir ];
   }
   if (-d ($dir = "$app_dir/scripts")) {
      push @{$self->scriptpath}, $dir;
   }
   local $Extension::loading = $ext;
   local $enable_plausibility_checks = $ext->untrusted;
   include_rules($self);
   push @{$self->extensions}, $ext;
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
sub list_loaded {
   values %repository;
}
sub delete {
   delete $repository{$_[1]};
}

sub used_by {
   my ($self) = @_;
   grep { exists($_->used->{$self->name}) } values %repository;
}
#################################################################################
# try to load the application APP
# when an expression APP::FUNCTION(...) is encountered in the user input
sub try_auto_load {
   my ($app_name) = @_;
   my $found;
   namespaces::temp_disable(0);
   if ($app_name =~ /^$id_re$/o && !exists $repository{$app_name}) {
      foreach my $dir ($InstallTop, map { $_->dir } @Extension::active[$Extension::num_bundled .. $#Extension::active]) {
         if (-d "$dir/apps/$app_name") {
            $found = defined( eval { add(__PACKAGE__, $app_name) } );
            last;
         }
      }
   }
   $found
}

#################################################################################
# "rulefile" => ( full_path, extension, rule_key, cached_result_code )
# 'rule_key' is a string used as a key in various maps like rulefiles, configured, or rule_code
sub lookup_rulefile {
   my ($self, $rulename, $only_here) = @_;
   my ($filename, $ext, $rule_key, $rc, @accumulated);
   if (defined($Extension::loading)) {
      # initial loading an extension: first look in the same extension, then in the application root, then in required extensions
      $rule_key = $rulename . '@' . $Extension::loading->URI;
      $filename = $Extension::loading->app_dir($self) . "/rules/$rulename";
      if (defined($rc = $self->rulefiles->{$rule_key}) or -f $filename) {
         if ($only_here < 0) {
            push @accumulated, [ $filename, $Extension::loading, $rule_key, $rc ];
         } else {
            return ($filename, $Extension::loading, $rule_key, $rc);
         }
      }
      unless ($only_here) {
         $filename = $self->top."/rules/$rulename";
         if (defined($rc = $self->rulefiles->{$rulename}) or -f $filename) {
            return ($filename, undef, $rulename, $rc);
         }
         foreach $ext (@{$Extension::loading->requires}) {
            $rule_key = $rulename.'@'.$ext->URI;
            $filename = $ext->app_dir($self) . "/rules/$rulename";
            if (defined($rc = $self->rulefiles->{$rule_key}) or -f $filename) {
               return ($filename, $ext, $rule_key, $rc);
            }
         }
      }

   } elsif ($rulename =~ m{^/}) {
      if (-f $rulename) {
         # catch rules specified by absolute path
         $filename = $rulename;
         ($rulename) = $filename =~ $filename_re;
         if (index($filename, $self->top."/rules/") == 0) {
            return ($filename, undef, $rulename, $self->rulefiles->{$rulename});
         }
         foreach $ext (@{$self->extensions}) {
            if (index($filename, $ext->app_dir($self)."/rules/") == 0) {
               $rule_key = $rulename.'@'.$ext->URI;
               return ($filename, $ext, $rule_key, $self->rulefiles->{$rule_key});
            }
         }
         # stray rulefile without registered extension, presumably user's private experiments
         return ($filename, undef, $filename, $self->rulefiles->{$filename});
      }

   } else {
      # loading on demand: first look in the application root, then in all extensions
      $filename = $self->top."/rules/$rulename";
      if (defined($rc = $self->rulefiles->{$rulename}) or -f $filename) {
         if ($only_here < 0) {
            push @accumulated, [ $filename, undef, $rulename, $rc ];
         } else {
            return ($filename, undef, $rulename, $rc);
         }
      }
      foreach $ext (@{$self->extensions}) {
         $filename = $ext->app_dir($self)."/rules/$rulename";
         $rule_key = $rulename.'@'.$ext->URI;
         if (defined($rc = $self->rulefiles->{$rule_key}) or -f $filename) {
            if ($only_here < 0) {
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
      if (my ($filename, $ext, $rule_key) = lookup_rulefile($self, $rulename)) {
         # don't exclude if already loaded from elsewhere
         if (!exists $self->rulefiles->{$rule_key}) {
            $self->rulefiles->{$rule_key} = 0;
            if (defined (delete $self->configured->{$rule_key})) {
               $self->custom->set_changed;
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
   my ($self, $rulefile, $only_here) = @_;
   if ($rulefile =~ m{^($id_re)::(.*)}o) {
      include_rules($self->used->{$1} || croak( "application $1 is not declared as USE'd or IMPORT'ed - may not load its rules here" ), "$2");
   } elsif ($rulefile =~ s{^\*/(.*)}{$1}) {
      my $rc_all = 0;
      foreach my $app (reverse(@{$self->linear_imported}),
                       (grep { !$self->imported->{$_->name} } values %{$self->used}), $self) {
         if (my @found = lookup_rulefile($app, $rulefile, -1)) {
            if ($app == $self) {
               foreach (@found) {
                  $rc_all += process_included_rule($app, $rulefile, @$_);
               }
            } else {
               $rc_all += include_rules($app, $rulefile, @found);
            }
         }
      }
      $rc_all;
   } elsif (my @found = &lookup_rulefile) {
      process_included_rule($self, $rulefile, @found);
   } else {
      $only_here || croak( "rule file ", ($rulefile =~ m{^/} ? $rulefile : $self->name."::$rulefile"), " does not exist" );
   }
}
#################################################################################
sub include_rules {
   my $self = shift;
   local if (is_code($INC[0])) {
      if ($User::application != $self) {
         local unshift @INC, $self;
      }
   } elsif (is_object($INC[0])) {
      if ($INC[0] != $self) {
         local $INC[0] = $self;
      }
   } else {
      local unshift @INC, $self;
   }
   local ref $self->custom->tied_vars = [];
   local scalar $self->compile_scope = new Scope();
   $self->compile_scope->cleanup->{$self->custom} = undef;
   my $rc_all=0;
   eval {
      if (@_) {
         # application is already loaded, adding some optional rulefiles
         if (is_array($_[-1])) {
            # recursive call from include_rule with a wildcard application
            my $rulefile = shift;
            foreach my $found (@_) {
               $rc_all += process_included_rule($self, $rulefile, @$found);
            }
         } else {
            foreach my $rulefile (@_) {
               $rc_all += include_rule($self, $rulefile);
            }
         }
      } else {
         # initializing the application or its main part in an extension
         local $CPlusPlus::code_generation = "" if $CPlusPlus::code_generation eq "private";
         $self->load_state &= ~LoadState::cpp_load_initiated;
         if ($rc_all = include_rule($self, "main.rules", 1)) {
            unless ($self->load_state & LoadState::cpp_load_initiated) {
               # no rulefiles at all; nevertheless, there might be clients
               $self->load_state |= LoadState::cpp_load_initiated;
               $self->cpp->start_loading($Extension::loading);
            }
            $self->cpp->end_loading($Extension::loading);
         }
      }
      $_->finalize for @{$self->rules_to_finalize};
   };
   if ($@) {
      die beautify_error();
   }
   @{$self->rules_to_finalize} = ();
   $rc_all;
}
#################################################################################
sub include_rule_block {
   my ($self, $mandatory, $block)=@_;
   while ($block =~ /\G \s* (\S+)/gx) {
      my @failed;
      my $rulefile = $1;
      my $success = include_rule($self, $rulefile);
      if ($mandatory) {
         push @failed, $rulefile if !$success;
      } else {
         while ($block =~ /\G \s+\+\s+ (\S+)/gxc) {
            $rulefile = $1;
            $success += include_rule($self, $rulefile);
         }
      }
      while ($block =~ /\G \s+\|\s+ (\S+)/gxc) {
         $rulefile = $1;
         if ($success) {
            exclude_rule($self, $rulefile) if !$mandatory;
         } else {
            $success = include_rule($self, $rulefile);
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

# called from C++ library
sub construct_type {
   my ($self, $typename) = splice @_, 0, 2;
   local @ARGV = @_ and my $params='(@ARGV)';
   $self->eval_expr->("typeof $typename$params")
     // croak( $@ =~ /^invalid type expression/ ? "$& $typename" : "Error processing type expression $typename: $@" );
}

# called from C++ library
sub construct_explicit_typelist {
   my $self = shift;
   $self->eval_expr->("bless [" . join(",", map { "typeof $_" } @_) . "], 'namespaces::ExplicitTypelist'")
     // croak( $@ =~ /^invalid type expression/ ? "$& @_" : "Error processing type expressions @_: $@" );
}
#################################################################################
sub set_file_suffix {
   my ($self, $suffix) = @_;
   ($self->default_file_suffix &&= croak("multiple definition of default file suffix")) ||= $suffix;
   push @{$self->file_suffixes}, $suffix;
}
#################################################################################
sub lookup_credit {
   my ($self, $product) = @_;
   $self->credits->{$product} //= do {
      my $credit;
      foreach my $app (values %{$self->used}) {
         if (defined($credit = $app->credits->{$product})) {
            keys %{$self->used};
            last;
         }
      }
      $credit
   };
}
#################################################################################
sub add_custom {
   my $self = shift;
   $self->custom->add(@_)->extension = $Extension::loading;
}
#################################################################################
sub use_apps {
   my ($self, $import) = splice @_, 0, 2;
   my ($i, $app);

   my $in_ext = $Extension::loading;
   local $Extension::loading;

   my @apps = map {
      if (my ($appname, $start_rules) = m/^\s* ($id_re) (?: \s*\(\s* (.*?) \s*\) )? \s*$/xo) {
         if (exists $repository{$appname} && !defined $repository{$appname}) {
            # requested application is itself being loaded
            croak( "Cyclic dependence between applications ", $self->name, " and $appname" );
         }
         $app = add Application($appname);
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
         $app
      } else {
         croak( "invalid application name '$_'" );
      }
   } @_;

   if ($import) {
      {  no strict 'refs';
         push @{$self->pkg."::.IMPORTS::ISA"}, map { $_->pkg."::.IMPORTS" } @apps;
         my $linear_isa = mro::get_linear_isa($self->pkg."::.IMPORTS");
         @{$self->linear_imported} = map { ${s/IMPORTS$/APPL/r} } @$linear_isa[1..$#$linear_isa];
      }
      foreach my $app (@{$self->linear_imported}) {
         push @{$self->myINC}, @{$app->myINC};
         push %{$self->credits}, %{$app->credits};
         namespaces::using($self->pkg, $app->pkg);
         $self->imported->{$app->name} = true;
         push @{$self->prefs->imported}, $app->prefs;
         push @{$self->help->related}, $app->help if $Help::gather;
      }
   }

   foreach my $app (@apps) {
      $self->used->{$_->name} = $_ for $app, values %{$app->used};
   }
}

sub common {
   my ($self, $other)=@_;
   if (!defined($other) || $self == $other || exists $self->used->{$other->name}) {
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
   if (defined($self->custom) && !defined($var = $self->custom->find($name))) {
      foreach my $app (values %{$self->used}) {
         if (defined($var = $app->custom->find($name))) {
            keys %{$self->used};
            last;
         }
      }
   }
   if (defined($var)) {
      ($self->custom, $var)
   } elsif (defined($alt) && defined($var = $alt->find($name))) {
      ($alt, $var)
   } else {
      croak( "unknown custom variable $name" );
   }
}

# for clients and callable library:
# 'prefixed varname', [key] => value
sub get_custom_var {
   my $var_name=shift;
   my $var;
   if ($var_name =~ /^.($id_re)::/o && defined(my $app=$repository{$1})) {
      $var_name=substr($var_name,0,1).$';
      $var=find_custom_var($app, $var_name);
   } else {
      $var=find_custom_var($User::application, $var_name, $Prefs->custom);
   }
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
   (my ($self, $alt, $name, @tail) = @_) > 1
     or croak( "custom variable, array, hash, or hash element assignment expected" );
   my ($bunch, $var) = find_custom_var($self, $name, $alt);
   $var->set(@tail);
   $bunch->set_changed;
}

sub set_custom {
   _set_custom($_[0], undef, name_of_custom_var(1));
}

sub _reset_custom {
   (my ($self, $alt, $name, @tail) = @_) > 1
     or croak( "custom variable, array, hash, or hash element expected" );
   my ($bunch, $var) = find_custom_var($self, $name, $alt);
   $var->reset(@tail);
   $bunch->set_changed;
}

sub reset_custom {
   _reset_custom($_[0], undef, name_of_custom_var(0));
}
#################################################################################
sub add_top_label {
   my ($self, $name) = @_;
   my $label = ( $self->prefs->labels->{$name} &&=
                 croak( "multiple definition of label $name" ) ) = new Preference::Label($name);
   $label->application = $self;
   $label->extension = $Extension::loading;
   $label
}

sub add_label {
   my ($self, $name) = @_;
   my $label = $self->prefs->find_label($name, 1) or do {
      $name =~ /^($id_re)/o;
      croak( "unknown label '$1'" );
   };
   $label->set_application($self, $Extension::loading);
   $label
}
#################################################################################
sub prefer {
   my ($self, $expr) = @_;
   $self->prefs->add_preference($expr, Preference::Mode::strict);
}

sub prefer_now {
   my ($self, $expr) = @_;
   $self->prefs->set_temp_preference($Scope, $expr);
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
# This is a placeholder with minimal functionality required for RuleFilter to load
# scripts and data upgrade rules in an early stage, when no real application
# is loaded yet.
sub load_dummy {
   $User::application = _new(__PACKAGE__, "");
   $User::application->configured = { };
   push @INC, $User::application;
   readonly($User::application);
}
#################################################################################
package Polymake::Core::Application::SuspendedItems;

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
   my ($application, $extension, $missing_app_name, @further_missing_apps) = @_;
   my $list = ($suspended{$missing_app_name} //= [ ]);
   my $self;
   # Only look at the end of the list and stop by first application or extension mismatch,
   # because all suspended items are created at the same time, when the owning application is loaded.
   for (my $i = $#$list; $i >= 0; --$i) {
      $self = $list->[$i];
      if ($self->application == $application && $self->extension == $extension) {
         if (equal_lists($self->further_missing_apps, \@further_missing_apps)) {
            return $self;
         }
      } else {
         last;
      }
   }
   $self = new(__PACKAGE__, $application, $extension, @further_missing_apps);
   push @$list, $self;
   $self
}

sub harvest {
   my ($app_name) = @_;

   if (defined(my $list = delete $suspended{$app_name})) {
      foreach my $self (@$list) {
         if (my @missing_apps = grep { !exists $repository{$_} } @{$self->further_missing_apps}) {
            # Associate the suspended items with another missing application.
            my $missing_app_name = shift @missing_apps;
            my $next_list = ($suspended{$missing_app_name} //= [ ]);
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
            my $app = $self->application;
            local $Extension::loading = $self->extension;
            local unshift @INC, $app;
            local scalar $app->compile_scope = new Scope();
            eval {
               local $CPlusPlus::code_generation = "" if $CPlusPlus::code_generation eq "private";
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
