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

#  Application methods and user commands available in interactive mode

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);
use feature 'state';

package Polymake::Core::InteractiveCommands;

use Term::Cap;

my ($bold, $boldoff, $under, $underoff);

sub init_termcap {
   state $termcap = defined($ENV{TERM}) && eval {
      my $tc = Term::Cap->Tgetent;
      $bold = $tc->Tputs('md');
      $boldoff = $tc->Tputs('me');
      $under = $tc->Tputs('us');
      $underoff = $tc->Tputs('ue');
      close Term::Cap::DATA;
      1
   } // (warn("Term::Cap->Tgetent failed for TERM $ENV{TERM}: $@"), 0);
}

sub Tgetent {
   init_termcap && Term::Cap->Tgetent
}

sub underline {
   init_termcap;
   "$under$_[0]$underoff"
}

sub bold {
   init_termcap;
   "$bold$_[0]$boldoff"
}

sub clean_text {
   init_termcap;
   $_[0] =~ s/\[\[(?:.*?\|)?(.*?)\]\]/$under$1$underoff/g;
   $_[0] =~ s/''(.*?)''/$1/g;
   $_[0] =~ s{(//|__)(.*?)\1}{$under$2$underoff}g;
   $_[0] =~ s/\*\*(.*?)\*\*/$bold$1$boldoff/g;
   $_[0] .= "\n" unless substr($_[0],-1) eq "\n";
}

###############################################################################################
package Polymake::Core::Application;

has_interactive_commands(1);

###############################################################################################
my %rules_to_wake;

# private
sub store_rule_to_wake {
   my ($self, $filename, $ext, $rule_key, $on_rule) = splice @_, 0, 5;
   for (;;) {
      my $list;
      if ($on_rule) {
         my ($prereq_app, $prereq_rule_key) = splice @_, 0, 2  or  last;
         $list = ($rules_to_wake{$prereq_app}->{$prereq_rule_key} //= [ ]);
      } else {
         my $prereq_ext = shift @_  or  last;
         $list=($rules_to_wake{$prereq_ext} //= [ ]);
      }
      if (@$list && $list->[-2] == $self) {
         push @{$list->[-1]}, $filename, $ext, $rule_key;
      } else {
         push @$list, $self, [ $filename, $ext, $rule_key ];
      }
   }
}
###############################################################################################
# private
sub do_reconfigure {
   my ($self, $list, $filename, $ext, $rule_key);
   my @woken = @_;
   while (($self, $list) = splice @woken, 0, 2) {
      my $new_to_wake = @woken;

      local if (!defined($self->compile_scope)) {
         local unshift @INC, $self;
         local scalar $self->compile_scope = new Scope();
         local ref $self->custom->tied_vars = [];
         $self->compile_scope->cleanup->{$self->custom} = undef;
      }
      while (($filename, $ext, $rule_key) = splice @$list, 0, 3) {
         my $rc;
         local $Extension::loading = $ext;
         if (defined (my $code = $self->rule_code->{$rule_key})) {
            my $reconf = new Reconf($self, false);
            no strict 'refs';
            local ref *{$self->pkg."::self"} = sub { $reconf };
            if (is_object (my $credit = $self->credits_by_rulefile->{$rule_key})) {
               $credit->shown &= ~Rule::Credit::hide;
            }
            &$code;
            $rc = $self->rulefiles->{$rule_key};
         } else {
            $rc = parse_rulefile($self, $rule_key, $filename);
         }
         if ($rc && defined (my $to_wake=$rules_to_wake{$self})) {
            if (defined ($to_wake=delete $to_wake->{$rule_key})) {
               push @woken, @$to_wake;
            }
         }
      }

      $_->finalize for @{$self->rules_to_finalize};
      @{$self->rules_to_finalize} = ();

      for (; $new_to_wake < $#woken; $new_to_wake += 2) {
         ($self, $list) = @woken[$new_to_wake, $new_to_wake+1];
         for (my $i = 0; $i < @$list; $i += 3) {
            ($filename, $ext, $rule_key) = @$list[$i..$i+2];
            delete $self->configured->{$rule_key};
         }
      }
   }
}
###############################################################################################
sub lookup_rulefile_or_key {
   my ($self, $rulefile) = @_;
   if ($rulefile =~ s/\@(.*)$//) {
      # specified by the key including extension URI
      if (local $Extension::loading = $Extension::registered_by_URI{$1}) {
         lookup_rulefile($self, $rulefile, 1);
      } else {
         die "unknown extension URI $1\n";
      }
   } else {
      &lookup_rulefile;
   }
}
###############################################################################################
sub reconfigure {
   my ($self, $rulefile) = @_;
   if ($rulefile =~ m{^($id_re)::(.*)}o) {
      reconfigure($self->used->{$1} || die("application $1 is not declared as USE'd or IMPORT'ed in the current application\n"), "$2");

   } elsif (my ($filename, $ext, $rule_key, $old_status)=&lookup_rulefile_or_key) {
      if (defined (my $conf_status = $self->configured->{$rule_key})) {
         if (is_string($conf_status) and my ($rules, $what) = $conf_status =~ /^0\#(?:(rule)|ext):(.*)/) {
            my @prereq = split /\|/, $what;
            die( "rulefile $rulefile requires ",
                  $rules ? ( @prereq > 1 ? "one of the rulefiles ".join(", ", @prereq) : "another rulefile $what" )
                         : ( @prereq > 1 ? "one of extensions ".join(", ", @prereq) : "the extension $what" ),
                  " which must be reconfigured first;\n",
                  "in the case of success the desired rulefile might be enabled automatically.\n" );
         }
         delete $self->configured->{$rule_key};
      } elsif (defined $old_status) {
         die "rulefile $filename does not contain any configuration clauses\n";
      } else {
         # nothing is known yet about this rulefile
         process_included_rule(@_, $filename, $ext, $rule_key);
         return;
      }
      require Polymake::Configure;
      do_reconfigure($self, [ $filename, $ext, $rule_key ]);

   } else {
      die "rulefile $rulefile does not exist in application ", $self->name, "\n";
   }
}
###############################################################################################
sub unconfigure {
   my ($self, $rulefile) = @_;
   if ($rulefile =~ m{^($id_re)::(.*)}o) {
      unconfigure($self->used->{$1} || die("application $1 is not declared as USE'd or IMPORT'ed in the current application"), "$2");

   } elsif (my ($filename, $ext, $rule_key, $old_status) = &lookup_rulefile_or_key) {
      my $config_state = $self->configured->{$rule_key};
      if ($config_state > 0) {
         if (defined (my $code = $self->rule_code->{$rule_key})) {
            my $unconf = new Unconf($self);
            no strict 'refs';
            local ref *{$self->pkg."::self"} = sub { $unconf };
            local $Extension::loading = $ext;
            &$code;
         } else {
            $self->configured->{$rule_key} = -$load_time;
            $self->custom->set_changed;
         }
      } elsif (defined $config_state) {
         die "rulefile $rulefile is already unconfigured\n";
      } else {
         die "rulefile $filename does not contain any configuration clauses\n";
      }
   } else {
      die "rulefile $rulefile does not exist in application ", $self->name, "\n";
   }
}
###############################################################################################
# private:
sub valid_configured_entry {
   my ($self, $rule_key)=@_;
   exists $self->rulefiles->{$rule_key} or do {
      if ($rule_key =~ m{^/}) {
         -f $rule_key or do {
            delete $self->configured->{$rule_key};
            $self->custom->set_changed;
            false
         }
      } else {
         my ($rulefile, $ext) = split /\@/, $rule_key;
         if ($ext
             ? (($ext = $Extension::registered_by_URI{$ext}) &&
                -f $ext->app_dir($self) . "/rules/$rulefile")
             : -f $self->top . "/rules/$rulefile") {
            !$ext || $ext->is_active;
         } else {
            delete $self->configured->{$rule_key};
            $self->custom->set_changed;
            false
         }
      }
   }
}
###############################################################################################
sub list_configured {
   my ($self, $state)=@_;
   grep {
      valid_configured_entry($self, $_) and
      ($state < 0
       ? $self->configured->{$_} < 0 || $self->configured->{$_} eq "0" || $self->configured->{$_} =~ /\#$/ :
       $state > 0
       ? $self->configured->{$_} > 0
       : $self->configured->{$_} !~ /^0\#/)
   } keys %{$self->configured}
}
###############################################################################################
sub first_credit_in {
   my ($self, $rule_key) = @_;
   LoadPreamble::include_rule($self, $rule_key);
   if (defined (my $credit = $self->credits_by_rulefile->{$rule_key})) {
      if (is_object($credit)) {
         $credit
      } else {
         $self->credits_by_rulefile->{$rule_key} = lookup_credit($self, $credit)
      }
   } elsif ($rule_key =~ /\@(bundled:\w+)$/) {
      $Extension::registered_by_URI{$1}->credit
   } else {
      undef
   }
}
###############################################################################################
sub extend_in {
   my ($self, $ext) = @_;
   if ($self->installTop eq $ext->dir || list_index($self->extensions, $ext) >=0 ) {
      die "Extension ", $ext->URI, " already contributes to application ", $self->name, "\n";
   }
   if ($self->installTop ne $InstallTop) {
      my $foundation_ext = $Extension::registered_by_dir{$self->installTop};
      if (list_index($foundation_ext->requires, $ext) >= 0) {
         die "Application ", $self->name, " is founded in extension ", $foundation_ext->URI, " installed at ", $self->installTop, "\n",
             "which has the specified extension ", $ext->URI, " among its direct or indirect prerequisites.\n",
             "Adding here a contribution to ", $app->name, " would introduce a forbidden cyclic dependency between both extensions.\n",
             "You might consider moving the foundation of ", $app->name, " into extension ", $ext->URI, "\n",
             "or into a third extension, independent on these ones.\n";
      }
      if (list_index($ext->requires, $foundation_ext) < 0) {
         $ext->add_prerequisites($foundation_ext);
      }
   }

   my $source_tree = $ext->get_source_tree;

   my $app_dir = $ext->app_dir($self);
   $source_tree->make_dir(map { "$app_dir/$_" } qw(src include perllib rules scripts testsuite));
   if ($ext->is_bundled) {
      open my $noexport, ">", "$app_dir/.noexport";
      print $noexport "testsuite test\n";
      close $noexport;
   }

   # create include symlink
   my $link_dir = $ext->dir."/include/apps/polymake";
   -d $link_dir or $source_tree->make_dir($link_dir);
   my $app_name = $self->name;
   my $link = "$link_dir/$app_name";
   if (-l $link) {
      unlink $link
        or die "Can't remove obsolete link $link: $!\n";
   } elsif (-e _) {
      File::Path::remove_tree($link);
   }
   symlink "../../../apps/$app_name/include", $link
      or die "Can't create a link $link: $!\n";
}
###############################################################################################
sub found {
   require Polymake::SourceTree;
   (undef, my ($name, $ext)) = @_;
   my $src_tree = $ext ? $ext->get_source_tree : new SourceTree($InstallTop);
   my $app_dir = ($ext ? $ext->dir : $InstallTop)."/apps/$name";
   $src_tree->make_dir(map { "$app_dir/$_" } qw(src include perllib rules scripts testsuite));
   unless ($ext) {
      open my $noexport, ">", "$app_dir/.noexport";
      print $noexport "testsuite test\n";
      close $noexport;
   }
}
###############################################################################################
package Polymake::Core::Application::LoadPreamble;

use Polymake::Struct (
   [ '@ISA' => 'Application' ],
);

# pretend the last configuration step in the preamble to fail
sub configure {
   my ($self, $code, $rule_key, $line) = @_;
   $line < $self->preamble_end->{$rule_key};
}

*include_required = \&configure;
*require_ext = \&configure;

sub include_rule_block {}

sub add_credit {
   my $self = shift;
   my $credit = $self->SUPER::add_credit(@_, 1);
   $credit->shown = Rule::Credit::hide;
   $credit
}

sub include_rule {
   my ($self, $rule_key) = @_;
   if (my $code = $self->rule_code->{$rule_key}) {
      my $credit = $self->credits_by_rulefile->{$rule_key};
      if (defined($credit) && !is_object($credit)) {
         my $reread = new Reconf($self, true);
         no strict 'refs';
         local ref *{$self->pkg."::self"} = sub { $reread };
         &$code;
      }
   } else {
      local bless $self;
      my ($rulefile, $ext_URI) = split /\@/, $rule_key;
      local $Extension::loading = $Extension::registered_by_URI{$ext_URI} if $ext_URI;
      my $conf = $self->configured->{$rule_key};
      if ($conf > 0 || !defined($conf)) {
         local $Shell = new NoShell();
         include_rules($self, $rulefile);
      } else {
         my $save_buf;
         delete local $self->rulefiles->{$rule_key};
         tied(%{$self->configured})->hide($rule_key, $save_buf);
         include_rules($self, $rulefile);
      }
   }
}
###############################################################################################
package Polymake::Core::Application::CppDummy;

sub new {
   state $dummy=bless [ ], $_[0];
}

sub start_loading {}

###############################################################################################
package Polymake::Core::Application::Reconf;

use Polymake::Struct (
   [ new => '$$' ],
   [ '$application' => '#1' ],
   [ '$simulate_failure' => '#2' ],
);

sub start_preamble { shift }
sub preamble_end { $_[0]->application->preamble_end }

sub cpp { new CppDummy; }

sub add_credit {
   my $self = shift;
   my $credit = $self->application->add_credit(@_, 1);
   if ($self->simulate_failure && $_[1] =~ /\S/) {
      $credit->shown = Rule::Credit::hide;
   }
   $credit
}

sub add_custom {
   my $self = shift;
   if (!$self->simulate_failure  and  defined(my $var = $self->application->custom->re_tie(@_))) {
      $var->extension = $Extension::loading;
   }
}

sub close_preamble {
   my ($app, $rule_key, $line, $success) = @_;
   if ($line >= $app->preamble_end->{$rule_key}) {
      ($app->rulefiles->{$rule_key} &&= return 0) = $success;   # avoid repeated execution of the rulefile body
   }
   $success;
}

sub configure {
   my ($self, $code, $rule_key, $line, $optional) = @_;
   if ($self->simulate_failure) {
      $line < $self->preamble_end->{$rule_key};
   } else {
      close_preamble($self->application, $rule_key, $line, $self->application->configure($code, $rule_key, $line, $optional));
   }
}

sub include_required {
   my ($self, $block, $rule_key, $line) = @_;
   if ($self->simulate_failure) {
      $line < $self->preamble_end->{$rule_key};
   } else {
      close_preamble($self->application, $rule_key, $line, $self->application->include_required($block, $rule_key, $line));
   }
}

sub require_ext {
   my ($self, $block, $rule_key, $line) = @_;
   if ($self->simulate_failure) {
      $line < $self->preamble_end->{$rule_key};
   } else {
      close_preamble($self->application, $rule_key, $line, $self->application->require_ext($block, $rule_key, $line));
   }
}

sub AUTOLOAD {
   my $self = shift;
   my ($method) = $AUTOLOAD =~ /::(\w+)$/;
   $self->application->$method(@_);
}

sub DESTROY { }

###############################################################################################
package Polymake::Core::Application::Unconf;

use Polymake::Struct (
   [ new => '$' ],
   [ '$application' => '#1' ],
);

sub start_preamble { shift }
sub preamble_end { $_[0]->application->preamble_end }

sub cpp { new CppDummy; }

sub add_credit {
   my ($self, $product, $ext_URI, $text) = @_;
   my $credit = $self->application->lookup_credit($product);
   if ($text =~ /\S/) {
      $credit->shown = Rule::Credit::hide;
   }
   $credit;
}

sub add_custom {
   my ($self, $varname, $help_text, $state, $pkg) = @_;
   $self->application->custom->unset($varname, $pkg) if $state;
}

# pretend the last configuration step in the preamble to fail
sub configure {
   my ($self, $code, $rule_key, $line) = @_;
   $line < $self->application->preamble_end->{$rule_key} or do {
      $self->application->configured->{$rule_key} = -$load_time;
      $self->application->custom->set_changed;
      0
   }
}

*include_required = \&configure;
*require_ext = \&configure;

sub include_rule_block {}

###############################################################################################
package Polymake::Core::Extension;

sub locate_unknown {
   my ($pkg, $URI, $mandatory)=@_;

   my $ignore=InteractiveCommands::underline("ignore");
   my $skip  =InteractiveCommands::underline("skip");
   my $stop  =InteractiveCommands::underline("stop");

   print <<".";
In order to proceed, you should import a polymake extension with URI
  $URI
Please download and unpack it, or check it out if it is kept in some repository.
Then enter the top directory (where the description file polymake.ext resides).

In the case this is an old URI of an extension you already have,
please specify its current top directory.  To avoid repeating this for each
data file, consider storing the outdated URI in the REPLACE section of that
extension.

If you know that this URI belongs to an obsolete extension which has been
integrated into the core system, please enter $ignore.  Then the data will be
processed as if there were no extension attributes in the file.

Finally, if you don't know where to get the extension or don't want to bother
importing it, please enter $stop to stop loading the data file
or $skip to temporarily move the data items into "undecoded" attachments.
.
   my $dir=$Shell->enter_filename("",
     { prompt => "extension dir",
       check => sub {
          if ($_[0] =~ s/^(ignore|stop|skip)$//i) {
             $refused{$URI}=lc($1);
             return;
          }
          if (-d $_[0] && -x _) {
             return;
          }
          ($_[0] && "The directory $_[0] does not exist or is inaccessible.\n") .
          "Please enter a correct location, `ignore', `stop', or `skip'.\n"
       },
     });

   if (length($dir)) {
      return provide($pkg, "file://".Cwd::abs_path($dir), $mandatory);
   }
   undef
}
######################################################################################
sub obliterate {
   my ($self)=@_;
   my @drop_apps;
   if ($self->is_active) {
      foreach my $app (list_loaded Application) {
         if ($app->installTop eq $self->dir) {
            $Prefs->obliterate_application($app->prefs);
            $Custom->obliterate_application($app->custom);
            $app->cpp->obliterate_extension($self, 1);
            $app->delete($app->name);
            push @drop_apps, $app;
         } elsif (delete_from_list($app->extensions, $self)) {
            $app->custom->obliterate_extension($self);
            $app->cpp->obliterate_extension($self);
            my $had_configured;
            while (my ($rule_key) = each %{$app->configured}) {
               if (index($rule_key, '@'.$self->URI)==0) {
                  delete $app->configured->{$rule_key};
                  $had_configured = true;
               }
            }
            if ($had_configured) {
               $app->custom->set('%configured');
            }
         }
      }
      $Prefs->custom->obliterate_extension($self);
      delete_from_list(\@active, $self);
   }
   delete $registered_by_dir{$self->dir};
   delete @registered_by_URI{$self->URI, @{$self->replaces}};
   @drop_apps;
}
###############################################################################################
sub may_obliterate {
   my ($self)=@_;
   if ($self->is_bundled) {
      my ($ext_name)= $self->dir =~ $filename_re;
      die "Extension $ext_name is bundled with polymake core and may not be obliterated.\n",
          "The only legal way to get rid of it is to disable it during the initial configuration:\n",
          "  configure --without-$ext_name\n";
   }

   foreach my $ext (values %registered_by_dir) {
      if ($ext != $self && contains($ext->requires, $self)) {
         die "Extension ", $self->URI, " is a prerequisite for another active extension ", $ext->URI, " installed at ", $ext->dir, "\n",
             "If you want to get rid of both, please `obliterate' the latter first.\n";
      }
   }
}
###############################################################################################
sub new_bundled {
   require Polymake::SourceTree;
   my ($pkg, $name)=@_;
   my $ext_dir="$InstallTop/bundled/$name";
   if (-d $ext_dir) {
      die "bundled extension $ext_dir already exists\n";
   }
   my $src_tree=new SourceTree($InstallTop);
   $src_tree->make_dir("$ext_dir/apps", "$ext_dir/include", "$ext_dir/support");
   open my $META, ">", "$ext_dir/polymake.ext"
     or die "can't create description file $ext_dir/polymake.ext: $!\n";
   print $META <<'.';
# This is a bundled extension.
# Please describe it briefly here.
# Uncommment and fill the sections below on demand.
# URIs of bundled extensions are written as bundled:NAME .

# CREDIT
# No credits defined so far.

# REQUIRE
# URIs of other bundled extensions which are prerequisite for this one

# REQUIRE_OPT
# URIs of other bundled extensions being optional prerequisites for this one

# CONFLICT
# URIs of other bundled extensions which are incompatible with this one.
.
   close $META;
   $src_tree->copy_file("$InstallTop/support/configure.pl.template", "$ext_dir/support/configure.pl.template");
   my $ext=new($pkg, $ext_dir, "bundled:$name", 1);
   $registered_by_URI{$ext->URI}=$ext;
   $registered_by_dir{$ext_dir}=$ext;
   # at the beginning of its life, the extension does not depend on anything else nor requires any configuration
   unshift @BundledExts, $name;
   require Polymake::Configure;
   Configure::rewrite_config_files($InstallTop, "BundledExts", "@BundledExts");
}
###############################################################################################
sub new_standalone {
   my ($pkg, $ext_dir)=@_;
   if ($ext_dir =~ m{^\Q${InstallTop}\E(?:/|$)}) {
      die <<"---";
A standalone extension can't be created within polymake source tree.
If you have had a bundled extension in mind, please specify its location as "bundled:NAME",
otherwise choose a location outside $InstallTop
---
   }
   if (-d $ext_dir) {
      if (-f "$ext_dir/polymake.ext" || -f "$ext_dir/URI" and -d "$ext_dir/apps") {
         if (exists $registered_by_dir{$ext_dir}) {
            die "Extension located at $ext_dir is already registered in your settings.\n";
         } else {
            die "Location $ext_dir seems to contain an already populated polymake extension.\n",
            "If you want to use and/or further develop it, please import it with a polymake shell command\n",
            "  import_extension(\"$ext_dir\");\n";
         }
      }
      if (my @dangerous=(glob("$ext_dir/{apps/*,include/*}"), grep { -e "$ext_dir/$_" } qw(support/configure.pl polymake.ext))) {
         die "Location $ext_dir contains files and/or subdirectories which will clash with those automatically created by polymake:\n",
             (map { s|^$ext_dir/||;$_."\n" } @dangerous);
      }
      unless (-w $ext_dir) {
         die "You don't have permission to create new files at the specified location $ext_dir\n";
      }
   } else {
      File::Path::make_path($ext_dir);
   }
   my $ext=new($pkg, $ext_dir);
   File::Path::make_path("$ext_dir/apps", "$ext_dir/scripts", "$ext_dir/include", "$ext_dir/support");
   $registered_by_URI{$ext->URI}=$ext;
   $registered_by_dir{$ext_dir}=$ext;
   foreach my $file (qw(configure.pl install.pl generate_ninja_targets.pl)) {
      File::Copy::copy("$InstallTop/support/$file.template", "$ext_dir/support");
   }
   unless (-f "$ext_dir/.gitignore") {
      File::Copy::copy("$InstallTop/support/gitignore.template", "$ext_dir/.gitignore");
   }
   # create the skeleton of the build directory
   require Polymake::Configure;
   $ext->set_build_dir;
   Configure::configure_extension($ext);
}
######################################################################################
# directory, config options => new Extension
sub add {
   my ($pkg, $ext_dir, @config_options)=@_;
   my $self=new($pkg, $ext_dir);
   if (@config_options == 1 && $config_options[0] eq "--help") {
      # this will merely print the usage message
      $self->configure(@config_options);

   } else {
      # perform the same checks as in init() but with different reactions on detected problems

      if (defined (my $other=$registered_by_URI{$self->URI})) {
	 if ($other->URI eq $self->URI) {
	    die "Extension provides the same URI ", $self->URI, " as an already registered extension installed at ", $other->dir, "\n";
	 } else {
	    die "Extension ", $self->URI, " is declared as obsoleted by an already registered extension ", $other->URI, " installed at ", $other->dir, "\n";
	 }
      }

      my $conflicting=$conflicts{$self->URI};
      unless ($conflicting) {
	 foreach (@{$self->conflicts}) {
	    $conflicting=$registered_by_URI{$_} and last;
	 }
      }
      if ($conflicting) {
	 die "Extension ", $self->URI, " is declared as conflicting with an already registered extension ", $conflicting->URI,
	     " installed at ", $conflicting->dir, "\n";
      }

      my (@prereqs, $disable);

      for (@{$self->requires}) {
	 my $prereq_version= s/\#([\d.]+)$// && $1;
	 if (defined (my $prereq=$registered_by_URI{$_})) {
	    if (length($prereq_version)) {
	       if ($prereq->URI ne $_) {
		  die "Extension ", $self->URI,
		      " has as one of its prerequisites a versioned extension URI $_#$prereq_version\n",
		      "which has been replaced by another extension ", $prereq->URI, " installed at ", $prereq->dir, "\n",
		      "Revise the REQUIRE section in the extension description file or look for a newer version of this extension.\n";

	       } elsif (!defined($prereq->version_num)
			  or
			$prereq_version ne $prereq->version && eval("v$prereq_version") gt $prereq->version_num) {

		  die "Extension ", $self->URI,
		      " has as one of its prerequisites a versioned URI $_#$prereq_version\n",
		      "However, the extension installed at ", $prereq->dir,
		      defined($prereq->version_num) ? "is of an older version ".$prereq->version : "does not specify any version", ".\n";
               }
	    }
            push @prereqs, $prereq, @{$prereq->requires};
            unless ($prereq->is_active) {
               $disable=1;
               if ($prereq->is_bundled) {
                  warn_print( "Prerequisite bundled extension ", $prereq->URI, "\n",
                              "is not configured for the current architecture $Arch.\n",
                              "To activate it, you must reconfigure and rebuild the polymake installation.\n",
                              "Then enable the new extension with a polymake shell command\n",
                              "  reconfigure_extension(\"$self->dir\");\n",
                            );
               } else {
                  warn_print( "Prerequisite extension ", $prereq->URI, " installed at ", $prereq->dir, "\n",
                              "is not configured for the current architecture $Arch\n",
                              "It can be activated with a polymake shell command\n",
                              "  reconfigure_extension(\"", $prereq->URI, "\", options...);\n",
                              "Then enable the new extension:\n",
                              "  reconfigure_extension(\"$self->dir\");\n",
                            );
               }
            }

	 } else {
            die "Extension ", $self->URI, "\n",
                "requires an unknown extension $_, therefore can't be imported now.\n",
                "If you need it, please first investigate where to get the prerequisite, download, and import it.\n";
	 }
      }

      my $postpone;
      if (@{$self->replaces}) {
         my $ext;
         if (my @replaced=grep { defined($ext=$registered_by_URI{$_}) && $ext->URI eq $_ } @{$self->replaces}) {
            warn_print( "Extension ", $self->URI, " claims to replace other registered extension", @active_replaced>1 && "s", ":\n",
                        (map { "  ".$_->URI." installed at ".$_->dir."\n" } @active_replaced),
                        "Activation of the new extension is therefore postponed until the next polymake session,\n",
                        "when the extension", @active_replaced>1 && "s", " listed above can safely be obliterated automatically.\n",
                        "You are strongly advised to restart polymake right now to make this change happen as soon as possible." );
            $postpone=1;
         }
      }

      $registered_by_dir{$ext_dir}=$self;
      $registered_by_URI{$_}=$self for $self->URI, @{$self->replaces};
      $conflicts{$_}=$self for @{$self->conflicts};
      @{$self->requires}=uniq(@prereqs) if @prereqs;
      push @User::extensions, $ext_dir;
      $Prefs->custom->set('@extensions');

      if ($disable || !$self->configure(@config_options)) {
	 warn_print( "Extension ", $self->URI, " is disabled on the current architecture $Arch due to problems depicted in the log above.\n",
		     "In order to activate it, investigate the reasons, install third-party software if instructed to do so,\n",
		     "then execute a polymake shell command:\n",
		     "  reconfigure_extension(\"$ext_dir\", options...);" );
	 $Prefs->custom->set('%disabled_extensions', $ext_dir, 1);
         undef
      } elsif ($postpone) {
         undef
      } else {
         activate($self);
         $self
      }
   }
}
###############################################################################################
sub reconfigure {
   my ($self, @config_options)=@_;
   my $ext_dir=$self->dir;
   if ($self->is_bundled) {
      die "Bundled extension $ext_dir can only be reconfigured together with the core system;\n",
          "Please re-run the main configure script.\n";
   } elsif ($ext_dir =~ m{^\Q${InstallTop}\E/ext/}o) {
      die "Extension $ext_dir is already installed and can't be reconfigured any more.\n";
   } elsif (@config_options && !-f "$ext_dir/support/configure.pl" && !-f "$ext_dir/configure.pl") {
      die "Extension $ext_dir does not accept any configuration options.\n";
   } elsif (!-w $ext_dir) {
      die "You don't have write permission in directory $ext_dir.\n";
   }

   foreach my $prereq (@{$self->requires}) {
      unless ($prereq->is_active) {
         if ($prereq->is_bundled) {
            die "Extension ", $self->URI, " requires a bundled extension ", $prereq->URI, " which is not configured for the current architecture $Arch.\n",
                "Please re-configure and re-build polymake enabling that bundled extension;\n",
                "if successful, come back to this one.\n";
         } else {
            die "Extension ", $self->URI, " requires another extension ", $prereq->URI, " installed at ", $prereq->dir, "\n",
                "which is currently disabled due to its own configuration problems.\n",
                "Please try to reconfigure that extension first; if successful, come back to this one.\n";
         }
      }
   }

   require Polymake::Configure;
   $self->set_build_dir;
   my $err=Configure::configure_extension($self, "--force", @config_options);
   if ($err ne "silent\n") {
      if ($err) {
         print "Configuration script failed with following error(s):\n$err\n";
         if ($User::disabled_extensions{$self->dir}) {
            print "Extension ", $self->dir,
                  "\nstays disabled until you successfully retry to configure it.\n";
         } else {
            print "Extension ", $self->dir,
                  "\nwill be disabled from the next polymake session on\n",
                  "unless you successfully retry the configuration now\n";
            $Prefs->custom->set('%disabled_extensions', $self->dir, 1);
         }
      } else {
         print "Configuration succeeded without errors.\n";
         if (delete $User::disabled_extensions{$self->dir}) {
            print "Extension ", $self->URI, " is now enabled for current architecture $Arch.\n";
            activate($self);
            if (defined (my $to_wake=delete $rules_to_wake{$self})) {
               Application::do_reconfigure(@$to_wake);
            }

            # try to enable dependent extensions
            foreach my $ext_dir (@User::extensions) {
               if ($User::disabled_extensions{$ext_dir} && defined (my $ext=$registered_by_dir{$ext_dir})) {
                  unless (grep { !$_->is_active } @{$ext->requires}) {
                     if (-f "$ext_dir/support/configure.pl" || -f "$ext_dir/configure.pl") {
                        print "Now you can try to reconfigure the dependent extension ", $ext->URI, " as well.\n";
                     } elsif ($ext->configure) {
                        delete $User::disabled_extensions{$ext_dir};
                        print "Extension ", $ext->URI, " is now enabled for current architecture $Arch.\n";
                        activate($ext);
                        if (defined (my $to_wake=delete $rules_to_wake{$ext})) {
                           Application::do_reconfigure(@$to_wake);
                        }
                     }
                  }
               }
            }

            $Prefs->custom->set('%disabled_extensions');

         } else {
            print <<".";

All C++ components in the extension will be rebuilt at the start of the next polymake session.
.
         }
      }
   }
}
###############################################################################################
# private:
sub write_initial_description {
   my ($self)=@_;
   my $src_tree=$self->get_source_tree;

   open my $descr, ">", $self->dir."/polymake.ext"
      or die "can't create extension description ", $self->dir, "/polymake.ext: $!\n";
   print $descr <<'.';
# This is a description file of a polymake extension rooted in this directory.
# Please refer to the documentation under
#   https://polymake.org/doku.php/user_guide/extend/extensions
# for detailed explanations of its contents.

# URI is a unique identifier assigned to this extension.
# This is the only mandatory entry in this file.
.
   print $descr "URI ", $self->versioned_URI, <<'.';


# Below you can put a short name for your extension.

# NAME
# short name of the extension


# Below you can extol the features implemented in this extension,
# put your copyright notice, or just refer to your website
# unless its address is already contained in the URI.

# CREDIT
#  short description
#  copyright your name, address, etc.


# If this extension relies on functionality of further extensions,
# please list their URIs below, separated by blanks and/or newlines.
# Each URI can be versioned if certain minimal version is required.

# REQUIRE
#   URI URI#version ...


# If this extension was formerly distributed under different URI
# or was merged with other extensions,
# please list the deprecated URIs below.

# REPLACE
#   URI ...


# If this extension is known to be incompatible with other extensions,
# please list their URIs below.

# CONFLICT
#   URI ...


.
   close $descr;

   if (-f (my $URIfile=$self->dir."/URI")) {
      unlink $URIfile or die "can't remove $URIfile: $!\n";
   }

   unless (glob($self->dir."/support/configure.pl*")) {
      foreach my $filename (qw(configure generate_ninja_targets install)) {
         $src_tree->copy_file("$InstallTop/support/$filename.pl.template", "support/$filename.pl.template");
      }
   }
}

sub add_prerequisites {
   my $self=shift;
   @{$self->requires}=uniq(@{$self->requires}, map { ($_, @{$_->requires}) } @_);
   my $appendURIs=join("", map { "  ".$_->URI."\n" } @_);
   open my $META, $self->dir."/polymake.ext"
     or die "can't open extension description ", $self->dir, "/polymake.ext: $!\n";
   local $/;
   local $_=<$META>;
   close $META;

   s{^ (REQUIRE \b .*? \n)\n }{$1$appendURIs\n}xms
     or
   s{^ \s*\#\s* REQUIRE\b .*\n (?: ^ \s*\#.* \n )* }{REQUIRE\n$appendURIs\n}xm
     or
   $_ .= "\nREQUIRE\n$appendURIs\n";
   {
      my ($META_new, $META_k)=new OverwriteFile($self->dir."/polymake.ext");
      print $META_new $_;
      close $META_new;
   }

   require Polymake::Configure;
   Configure::rewrite_config_files($self->dir, RequireExtensions => $self->list_prerequisite_extensions);
}
###############################################################################################

package Polymake::User;

# TODO: extract commands unrelated to reconfiguration in a separate file,
# for the sake of better legibility

###############################################################################################
sub help {
   my ($subject) = @_;
   if (length($subject) == 0) {
      print $Core::Help::core->display_text;
      return;
   }
   my $help_delim = $help_delimit ? "-------------------\n" : '';
   my $top_help;
   if ($subject =~ m{^($id_re)(?:$|::|/)}o && defined(my $app = lookup Core::Application($1))) {
      $subject = $';
      $top_help = $app->help;
   } elsif ($subject =~ m{^core/}) {
      $subject = $';
      $top_help = $Core::Help::core;
   } else {
      $top_help = $application->help;
   }
   if (my @topics = uniq( $top_help->get_topics($subject) )) {
      my (@subcats, @subtopics, $need_delim);
      foreach my $topic (@topics) {
         my $text = $topic->display_text;
         if (length($text)) {
            print $help_delim if $need_delim++;
            print $text, "\n";
         }
         foreach (@{$topic->toc}) {
            if ($topic->topics->{$_}->category & 1) {
               push @subcats, $_;
            } elsif ($_ ne "any") {
               push @subtopics, $_;
            }
         }
      }
      if (@subcats || @subtopics) {
         print $help_delim if $need_delim;
      }
      my $fp = $topics[0]->full_path =~ s{^/}{}r || $topics[0]->name;
      if (@subcats) {
         print "Categories of $fp:\n", (join ", ", sort @subcats), "\n";
      }
      if (@subtopics) {
         print "Subtopics of $fp:\n", (join ", ", sort @subtopics), "\n";
      }
   } elsif (index($subject, "/") < 0
              and
            @topics = $top_help->find($subject)) {

      if (@topics == 1) {
         print substr($topics[0]->full_path,1), ":\n", $topics[0]->display_text;
      } else {
         print "There are ", scalar(@topics), " help topics matching '$subject':\n";
         my $n = 0;
         if (@topics < 5) {
            foreach (@topics) {
               print $help_delim, ++$n, ": ", substr($_->full_path,1), ":\n",
                     $_->display_text, "\n";
            }
         } else {
            $Shell->fill_history({ temporary => 1 },
               map {
                  $subject = substr($_->full_path, 1);
                  print ++$n, ": $subject\n";
                  "help '$subject';"
               } @topics);
            print $help_delim, "Please choose those interesting you via history navigation (ArrowUp/ArrowDown):\n";
         }
      }
   } elsif ($subject eq "credits") {
      print "Application ", $application->name, " does not use any third-party software directly\n";
   } else {
      err_print( "unknown help topic '$subject'" );
   }
}
###############################################################################################
sub apropos {
   my $expr = shift;
   $expr = qr/$expr/i;
   if (my @list = uniq( map { $_->list_matching_leaves(sub { length($_[0]->text) && $_[0]->name =~ $expr }) }
                        map { $_->help, @{$_->help->related } } $application, values %{$application->used} )) {
      print map { $_->full_path."\n" } @list;
   } else {
      print "No matching help items found\n";
   }
}
###############################################################################################
sub show_preferences {
   my $default_seen;
   foreach ($application->prefs->list_active) {
      print "$_\n";
      $default_seen ||= m/\(\#\)$/;
   }
   if ($default_seen) {
      print "\nPreferences marked with (#) are in effect by default, as specified in the rule files\n";
   }
}
###############################################################################################
sub show_credits {
   my $brief = shift;
   my ($header_shown, $hidden) = (0, 0);
   foreach my $credit (sorted_uniq(sort { ($b->product =~ /^_/) - ($a->product =~ /^_/) || $a->product cmp $b->product }
                                   map { values(%{$_->credits}), grep { defined } map { $_->credit } @{$_->extensions} } $application, values %{$application->used} )) {

      my $internal = $credit->product =~ /^_/;

      if ($credit->shown == Core::Rule::Credit::hide) {
         $hidden = true;
      } else {
          if (!$header_shown && !$internal) {
             print "\nApplication ", $application->name, " currently uses following contributed and third-party software packages:\n";
          }
          if ($brief) {
              if (!$internal) {
                 print ", " if $header_shown;
                 print $credit->product;
                 $header_shown = true;
              }
          } else {
              my $text = $credit->text;
              Core::InteractiveCommands::clean_text($text);
              print "\n", substr($credit->product, $internal), "\n", $text;
              $header_shown = !$internal;
          }
      }
   }
   if ($brief) {
      if ($header_shown) {
         print "\nFor more details:  show_credits;\n";
      }
   } elsif (!$header_shown) {
      if ($hidden) {
         print "\nApplication ", $application->name, " currently does not use any third-party software packages;\n",
               "For more details:  show_unconfigured;\n";
      } else {
         print "\nApplication ", $application->name, " does not offer any interfaces to third-party software.\n";
      }
   } elsif ($hidden || grep { !$_ } map { values %{$_->configured} } $application, values %{$application->used}) {
      print "\n", Core::InteractiveCommands::bold("Note:"), " there are also some other interfaces which are currently disabled;\n",
            "For more details:  show_unconfigured;\n";
   }
}
###############################################################################################
sub show_extensions {
   if (@Core::Extension::active > $Core::Extension::num_bundled) {
      print "Following extensions are loaded:\n",
            map { "  " . $_->URI . "\n    installed at " . $_->dir . "\n\n" .
                  ($_->credit && do { my $text=$_->credit->text; Core::InteractiveCommands::clean_text($text); $text."\n\n" })
            } @Core::Extension::active[$Core::Extension::num_bundled .. $#Core::Extension::active];
   }
   if (keys %disabled_extensions) {
      print "Following extensions are registered but not configured for current architecture:\n",
            (map { "  $_\n" } sort keys %disabled_extensions), "\n";
   }
   if ($Core::Extension::num_bundled) {
      print "Following bundled extensions are configured and used:\n  ",
            join(", ", map { $_->URI =~ /^bundled:(.*)/ } @Core::Extension::active[0..$Core::Extension::num_bundled-1]),
            "\n\n";
   }
   if (!@Core::Extension::active) {
      print "No extensions are currently used\n\n";
   }
   if (keys %disabled_extensions) {
      print <<'.';
To activate an unconfigured extension:  help "reconfigure_extension";
.
   }
   print <<'.';
To import an extension:  help "import_extension";
To start an own extension:  help "found_extension";
.
}

###############################################################################################
sub found_extension {
   my $ext_dir=shift;
   my $ext;
   if ($ext_dir =~ /^bundled:($id_re)$/o) {
      $DeveloperMode or die "A new bundled extension can only be founded from within a git working copy of polymake source tree.\n";
      new_bundled Core::Extension($1);
   } else {
      replace_special_paths($ext_dir);
      $ext_dir = Cwd::abs_path($ext_dir);
      new_standalone Core::Extension($ext_dir);
      push @extensions, $ext_dir;
      $Core::Prefs->custom->set('@extensions');
   }
}
###############################################################################################
sub extend_application {
   my $where = shift;
   my $app = application(@_);
   my $ext = lookup Core::Extension($where);
   if ($ext->is_bundled) {
      $DeveloperMode or die "A bundled extension can only be expanded from within a git working copy of polymake source tree.\n";
   }
   -w $ext->dir or die "You don't have permission to create new files in ", $ext->dir, "\n";
   $app->extend_in($ext);
}
###############################################################################################
sub found_application {
   my ($where, $app_name)=@_;
   my $app = lookup Core::Application($app_name);
   my $exists_in;
   if ($app) {
      $exists_in = $app->installTop;
   } else {
      foreach ($InstallTop, @extensions) {
         if (-d "$_/apps/$app_name") {
            $exists_in=$_; last;
         }
      }
   }
   if ($where eq "core") {
      $DeveloperMode or die "A new core application can only be founded from within a git working copy of polymake source tree.\n";
      if ($exists_in eq $InstallTop) {
         die "Application $app_name already exists in the polymake core\n";
      } else {
         warn_print( "Application $app_name already exists in the extension $exists_in\n",
                     "Be sure to move the basic definitions, rule files, etc. from there into the core.\n" );
      }
      found Core::Application($app_name);

   } else {
      if ($exists_in) {
         die "Application $app_name already exists ", $exists_in eq $InstallTop ? "in the polymake core" : "in extension $exists_in", "\n";
      }
      my $ext = lookup Core::Extension($where);
      if ($ext->is_bundled) {
         die "A new application can't be founded in a bundled extension; please specify \"core\" as location instead.";
      } else {
         -w $ext->dir or die "You don't have permission to create new files in ", $ext->dir, "\n";
      }
      found Core::Application($app_name, $ext);
   }
}
###############################################################################################
sub import_extension {
   my $ext_dir = shift;
   replace_special_paths($ext_dir);
   -d $ext_dir or die "Directory $ext_dir does not exist.\n";
   $ext_dir = Cwd::abs_path($ext_dir);
   if (defined (my $ext = $Core::Extension::registered_by_dir{$ext_dir})) {
      if ($ext->is_active) {
         die "Extension $ext_dir is already loaded.\n";
      } else {
         die <<".";
Extension $ext_dir has already been registered,
but could not be configured for the current architecture $Arch.

Please retry the configuration using the command
  reconfigure_extension("$ext_dir");
and watch for possible error messages.

To get additional help about configuration options:
  reconfigure_extension("$ext_dir", "--help");
.
      }
   } else {
      add Core::Extension($ext_dir, @_);
   }
}
###############################################################################################
sub reconfigure_extension {
   my $what = shift;
   lookup Core::Extension($what)->reconfigure(@_);
}
###############################################################################################
sub obliterate_extension {
   my $what = shift;
   my $ext = lookup Core::Extension($what);
   $ext->may_obliterate;
   $Core::Prefs->custom->delete_from_private_list('@extensions', string_list_index(\@extensions, $ext->dir))
     or die "Extension ", $ext->URI, " can't be obliterated: it is included by global configuration\n";

   foreach my $app ($ext->obliterate) {
      if ($app==$default_application) {
         $Core::Prefs->custom->reset('$default_application');
      }
      if ($app==$application) {
         application($default_application);
      }
      delete_string_from_list(\@start_applications, $app->name);
   }
   $Core::Prefs->custom->set('@start_applications');
   if (delete $disabled_extensions{$ext->dir}) {
      $Core::Prefs->custom->set('%disabled_extensions');
   }
}
###############################################################################################
sub reconfigure {
   @_ or die "usage: reconfigure 'rulefile' ...;\n";
   $application->reconfigure($_) for @_;
}

sub unconfigure {
   @_ or die "usage: unconfigure 'rulefile' ...;\n";
   $application->unconfigure($_) for @_;
}
###############################################################################################
sub show_unconfigured {
   my (%shown_credits, $credit, $shown);
   foreach my $app ($application, values %{$application->used}) {
      foreach my $rule_key ($app->list_configured(-1)) {
         print $app != $application ? $app->name."::" : "", $rule_key, "\n",
               ($credit = $app->first_credit_in($rule_key) and !($shown_credits{$credit->product}++))
               ? ($credit->text, "\n") : ();
         $shown = true;
      }
   }
   if ($shown) {
      print "\nTo enable an interface:  reconfigure(\"", Core::InteractiveCommands::underline("RULEFILE"), "\");\n";
   }
}
###############################################################################################
sub export_configured {
   my $filename = shift;
   my $opts = @_==1 && ref($_[0]) eq "HASH" ? shift : { @_ };
   replace_special_paths($filename);
   $Core::Custom->export($filename, $opts, $Core::Prefs->custom);
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
