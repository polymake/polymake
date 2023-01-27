#  Copyright (c) 1997-2023
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

package Polymake::User;

declare $application;

sub application {
   if (@_>1) {
      die "usage: application [ \"name\" ]\n";
   } elsif (my ($new_app)=@_) {

      # This magic provides automatic loading of applications when they are first mentioned
      # as a prefix of a user function in the shell input, in a script, documentation example, or tutorial.
      state $register_autoload=namespaces::set_autolookup(\&Core::Application::try_auto_load);

      if (defined wantarray) {
         if (ref($new_app)) {
            warn_print( "application() call without effect as the application ", $new_app->name, " already loaded" );
            $new_app;
         } else {
            add Core::Application($new_app);
         }
      } else {
         $new_app = add Core::Application($new_app) unless ref($new_app);
         return if $application == $new_app;
         readonly_off($application);
         $application = $new_app;
         readonly($application);
      }
   } else {
      # tell the current application
      $application;
   }
}

#################################################################################
sub include {
   my $rc = $application->include_rules(@_);
   if ($rc != @_) {
      foreach (@_) {
         my ($app, $rulefile)= /^($id_re)::/o ? ($application->used->{$1}, $') : ($application, $_);
         my ($filename, $ext, $rule_key, $rc)=$app->lookup_rulefile($rulefile);
         if (!$rc) {
            if ($app->configured->{$rule_key} < 0) {
               warn_print( "rulefile $_ is disabled by auto-configuration.\n",
                           "Try  reconfigure \"$_\";  if you really need it." );
            } elsif ($app->configured->{$rule_key} =~ /^0\#(?=.)/) {
               warn_print( "rulefile $_ can't be loaded because of unsatisfied dependency on $'\n",
                           "Try to reconfigure the prerequisites if you really need it." );
            }
         }
      }
   }
}
#################################################################################
sub load {
   my ($name) = @_;
   my $filename = $name;
   replace_special_paths($filename);
   unless (-f $filename
           or
           defined($application->default_file_suffix)  &&  $filename !~ /\.\w+$/  &&
           -f ($filename .= "." . $application->default_file_suffix)) {
      croak( "no such file: $name" );
   }
   load Core::Datafile($filename);
}
#################################################################################
sub save {
   my ($obj, $filename, %options) = @_;
   if (instanceof Core::BigObject($obj)) {
      my ($schema, $datafile);
      if (defined($filename)) {
         $schema = delete $options{schema};
         replace_special_paths($filename);
         if ($filename !~ /\.\w+$/) {
            $filename .= "." . $obj->default_file_suffix;
         }
         $datafile = new Core::Datafile($filename, \%options);
         $obj->persistent = $datafile unless defined($schema);

      } elsif (defined($datafile = $obj->persistent)) {
         unless ($obj->changed) {
            warn_print( "no changes need to be saved" );
            return;
         }

      } else {
         if (!length($obj->name)) {
            $obj->name = Core::name_of_arg_var(0);
         }
         if (length($obj->name)) {
            $filename = $obj->name . "." . $obj->default_file_suffix;
            if (-f $filename) {
               if ($Shell->interactive) {
                  print "The file $filename already exists.\n",
                        "Please specify another file name or confirm it to be overwritten:\n";
                  $filename = $Shell->enter_filename($filename, { prompt => "data file" }) or return;
               } else {
                  croak( "Can't save an object '", $obj->name, "' since the file $filename already exists.\n",
                         "Please specify the explicit file name as the second argument to save() or delete the existing file (unlink \"$filename\")" );
               }
            }
         } else {
            if ($Shell->interactive) {
               print "Please specify the file name for the anonymous ", $obj->type->full_name, " object:\n";
               $filename = $Shell->enter_filename("", { prompt => "data file" }) or return;
            } else {
               my $i = 1;
               while (-f ($filename = $obj->type->name . "_$i" . "." . $obj->default_file_suffix)) { ++$i }
               warn_print( "saving object as $filename" );
            }
         }
         $datafile = $obj->persistent = new Core::Datafile($filename, \%options);
      }
      $datafile->save($obj, schema => $schema);

   } else {
      &save_data;
   }
}
#################################################################################
sub load_data {
   my ($filename) = @_;
   replace_special_paths($filename);
   load Core::Datafile($filename);
}

sub save_data {
   my ($data, $filename, %options) = @_;
   defined($filename) or croak( "filename missing" );
   replace_special_paths($filename);
   my $df = new Core::Datafile($filename, \%options);
   $df->save($data);
}
#################################################################################
use subs qw(rename unlink mkdir chdir rmdir);

sub rename { my ($from, $to)=@_; replace_special_paths($from, $to); CORE::rename($from, $to) or die "rename failed: $!\n"; }
sub unlink { my @list=@_; replace_special_paths(@list); CORE::unlink(@list) or die "unlink failed: $!\n"; }
sub mkdir { my ($path, $mask)=@_; replace_special_paths($path); CORE::mkdir($path, $mask || 0755) or die "mkdir failed: $!\n"; }
sub rmdir { my ($path)=@_; replace_special_paths($path); CORE::rmdir($path) or die "rmdir failed: $!\n"; }

sub chdir {
   my $path;
   if (my ($path)=@_) {
      replace_special_paths($path) if is_string($path);
      CORE::chdir($path) or die "chdir failed: $!\n";
   } else {
      CORE::chdir;
   }
}

sub pwd { print Cwd::cwd }

#################################################################################
sub prefer {
   if ($_[0] =~ /^($id_re)::/o) {
      shift;  application($1)->prefer($', @_);
   } else {
      $application->prefer(@_);
   }
}

sub prefer_now {
   if ($_[0] =~ /^($id_re)::/o) {
      shift;  application($1)->prefer_now($', @_);
   } else {
      $application->prefer_now(@_);
   }
}

# an alias, for the sake of symmetry
*set_preference=\&prefer;

sub reset_preference {
   if ($_[0] =~ /^($id_re)::/o) {
      shift;  application($1)->reset_preference($', @_);
   } else {
      $application->reset_preference(@_);
   }
}
#################################################################################
sub disable_rules {
   $application->disable_rules(@_);
}
#################################################################################
sub script {
   my $name = shift;
   replace_special_paths($name);
   my ($code, $full_path, $in_app) = find Core::StoredScript($name);
   if (defined $code) {
      local *ARGV = \@_;
      &$code;
   } else {
      local @ARGV = @_;
      local if (defined($in_app)) {
         if ($in_app != $application) {
            local $application = $in_app;
            if (ref($INC[0])) {
               local $INC[0] = $in_app;
            } else {
               local unshift @INC, $in_app;
            }
         }
      } elsif (!ref($INC[0])) {
         local unshift @INC, $application;
      }
      $name = "script" . (defined($in_app) ? ":" : "=") . $full_path;
      if (wantarray) {
         my @ret = do $name;
         die $@ if $@;
         @ret
      } elsif (defined wantarray) {
         my $ret = do $name;
         die $@ if $@;
         $ret
      } else {
         do $name;
         die $@ if $@;
      }
   }
}
#################################################################################
# print boolean values in legible form: true and false instead of 1 and empty string
# enforce creation of a unique lexical scope with this operation inherited by all nested packages
use namespaces 'Polymake::User';
namespaces::memorize_lexical_scope;
namespaces::intercept_operation(undef, "print", "bool");

#################################################################################
# prepare custom variables

package Polymake::User::Verbose;
*Polymake::Verbose::=get_symtab(__PACKAGE__);

Core::add_settings_callback sub {
   my ($settings) = @_;
   my %docs;

   declare $credits = 1;
   $settings->add_item("Polymake::User::Verbose::credits", \$credits,
                       $docs{'$Polymake::User::Verbose::credits'} = <<'.', 0);
# Display the copyright notices and URLs of third-party software:
# 0 - never, 1 - at the first use in the current session, 2 - always
.
   declare $rules = 1;
   $settings->add_item("Polymake::User::Verbose::rules", \$rules,
                       $docs{'$Polymake::User::Verbose::rules'} = <<'.', 0);
# Display the information about the rules:
# 0 - nothing, 1 - significant failures, 2 - summary and all failed preconditions, 3 - executed rule executed
.
   declare $scheduler = 0;
   $settings->add_item("Polymake::User::Verbose::scheduler", \$scheduler,
                       $docs{'$Polymake::User::Verbose::scheduler'} = <<'.', 0);
# Reveal the internals of the rule scheduler:
# 0 - nothing, 1 - summary and statistics, 2 - initial rule selection,
# 3 - shortest path search (overwhelming amount of data)
.
   declare $cpp = 0;
   $settings->add_item("Polymake::User::Verbose::cpp", \$cpp,
                       $docs{'$Polymake::User::Verbose::cpp'} = <<'.', 0);
# Tell about the actions of the perl/C++ interface generator:
# 0 - nothing, 1 - compiler calls and source file updates, 2 - source code generated
.
   declare $files = 1;
   $settings->add_item("Polymake::User::Verbose::files", \$files,
                       $docs{'$Polymake::User::Verbose::files'} = <<'.', 0);
# Notify about nontrivial actions during data file processing
.
   declare $external = 0;
   $settings->add_item("Polymake::User::Verbose::external", \$external,
                       $docs{'$Polymake::User::Verbose::external'} = <<'.', 0);
# Notify about external programs starting in the background
# (not to be mixed up with credits!)
.

   package Polymake::User;

   declare @start_applications;
   $settings->add_item("Polymake::User::start_applications", \@start_applications,
                       $docs{'@start_applications'} = <<'.', 0);
# Applications to be loaded at the beginning of each interactive or batch session
.
   declare $default_application;
   $settings->add_item("Polymake::User::default_application", \$default_application,
                       $docs{'$default_application'} = <<'.', 0);
# Application to start with as the current one
.
   declare @extensions;
   $settings->add_item("Polymake::User::extensions", \@extensions,
                       $docs{'@extensions'} = <<'.', Core::UserSettings::Item::Flags::accumulating);
# A list of directories containing imported and/or locally created extensions
.
   declare @lookup_scripts;
   $settings->add_item("Polymake::User::lookup_scripts", \@lookup_scripts,
                       $docs{'@lookup_scripts'} = <<'.', Core::UserSettings::Item::Flags::accumulating);
# A list of directories where to look for scripts
.
   declare $init_script;
   $settings->add_item("Polymake::User::init_script", \$init_script,
                       $docs{'$init_script'} = <<'.', 0);
# Script executed at the beginning of every session.
# If specified without absolute path, looked up in the private settings directory (~/.polymake).
# The script can perform custom initialization actions, e.g. define simple shortcut commands,
# amend package lookup paths, or load additional rule files.
.
   declare $history_size = 200;
   $settings->add_item("Polymake::User::history_size", \$history_size,
                       $docs{'$history_size'} = <<'.', 0);
# Maximal number of commands stored in the interactive shell's history.
# If set to undef, history list grows unlimited.
.
   declare $history_editor = $ENV{VISUAL} || $ENV{EDITOR} || "vi";
   $settings->add_item("Polymake::User::history_editor", \$history_editor,
                       $docs{'$history_editor'} = <<'.', 0);
# Editor for the ''history'' command.
# Must be a complete shell command. If the temporary file name is expected somewhere in the middle
# of the arguments, please use the placeholder %f.
.
   declare $help_key = "_k1";
   $settings->add_item("Polymake::User::help_key", \$help_key,
                       $docs{'$help_key'} = <<'.', 0);
# Key to press for interactive help in the shell.  Defaults to F1.
.
   declare $help_delimit = 1;
   $settings->add_item("Polymake::User::help_delimit", \$help_delimit,
                       $docs{'$help_delimit'} = <<'.', 0);
# Add delimiters for better readability in help text.
.

   # treat relative paths as starting at $HOME
   s{^(?:~/|(?!/))}{$ENV{HOME}/} for @extensions, @lookup_scripts;

   if (!@start_applications) {
      @start_applications = map { m{apps/([^/]+)/rules} } glob("$InstallTop/apps/*/rules/main.rules");
   }
   if (!$default_application) {
      $default_application = string_list_index(\@start_applications, "polytope") >= 0 ? "polytope" : $start_applications[0];
   }

   Core::Help::add_activation_callback sub {
      my ($help) = @_;
      while (my ($varname, $text) = each %docs) {
         $help->add(['custom', $varname], $text);
      }
   }
};

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
