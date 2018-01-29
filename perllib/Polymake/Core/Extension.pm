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

package Polymake::Core::Extension;

declare @active;                 # All successfully configured and loaded extensions
declare $num_bundled=0;          # Number of loaded bundled extensions.
                                 # The objects representing bundled extensions are always stored in @active[0..$num_bundled-1].

declare (%registered_by_dir,     # "AbsPath" => active or disabled Extension
         %registered_by_URI,     # "URI" => active or disabled Extension, maybe replacement
         %refused,               # "URI" => "ignore", "skip", or "stop" : reaction on encountering an unwanted extension in a data file
         %conflicts,             # "URI" => conflicting Extension
        );

# List of obsoleted bundled extensions:
$refused{'bundled:group'} = 'ignore';

use Polymake::Struct (
   [ new => '$;$$' ],
   [ '$dir' => '#1' ],              # top-level directory
   [ '$URI' => '#2' ],              # unique identifier used in XML files and metadata
   [ '$version' => 'undef' ],       # version as string
   [ '$version_num' => 'undef' ],   # version as comparable v-string
   [ '$credit' => 'undef' ],        # Credit credit note, if present in metadata
   [ '$short_name' => 'undef'],
   [ '$is_bundled' => '#3' ],       # boolean
   [ '$build_dir' => 'undef' ],     # for standalone externsions: where to find architecture-dependent files (configuration, shared modules...)
   [ '$meta_tm' => 'undef' ],       # timestamp of the meta-file
   '$is_active',                    # boolean: configured, included in @active
   '@requires',                     # ( Extension ) direct and indirect prerequisites
   '@requires_opt',                 # ( Extension ) direct and indirect optional prerequisites
   '@replaces',                     # URIs obsoleted by this Extension
   '@conflicts',                    # URIs conflicting with this Extension
   [ '$VCS' => 'undef' ],           # version control system for source files
   '$untrusted',                    # TRUE if comes from a writable location, that is, may be under development
   '$configured_at',                # timestamp of the last configuration for the current architecture
);

sub new {
   my $self=&_new;

   if (-f $self->dir."/polymake.ext") {
      $self->untrusted= -w _ && (!$self->is_bundled || $DeveloperMode);
      $self->meta_tm=(stat _)[9];
      open my $meta, $self->dir."/polymake.ext"
        or die "can't read extension description ", $self->dir, "/polymake.ext: $!\n";
      local $/;
      local $_=<$meta>;
      s/^\s*\#.*$//mg;
      my @sections=split /(?:\A\n*|\n{2,})([A-Z_]+)(?=\s)/m; shift @sections;
      my %sections=@sections;
      if (defined (my $URI=delete $sections{URI})) {
         if ($self->is_bundled) {
            die "Bundled extensions have implicit URIs.\n",
                "URI section is not allowed in the extension description file ", $self->dir, "/polymake.ext\n";
         }
         $URI =~ s/^\s+//s;  $URI =~ s/\s+$//s;
         process_URI($self, $URI);
      } elsif (!$self->is_bundled && $self->URI ne "private:") {
         die "extension description file ", $self->dir, "/polymake.ext lacks mandatory URI section\n";
      }

      if (defined (my $short_name=delete $sections{NAME})) {
         if ($self->is_bundled) {
            die "Bundled extensions have fixed names derived from their top directory.\n",
                "NAME section is not allowed in the extension description file ", $self->dir, "/polymake.ext\n";
         }
         $short_name =~ s/^\s+//s;  $short_name =~ s/\s+$//s;
         $self->short_name=$short_name;
      }

      if (defined (my $requires=delete $sections{REQUIRE})) {
         @{$self->requires} = $requires =~ /(\S+)/g;
         if ($DeveloperMode && $self->is_bundled) {
            map { /^bundled:\w+$/ or die "Bundled extension may only depend on other bundled extensions.\n",
                                         "Unexpected URI $_ encountered in the REQUIRE section of description file ", $self->dir, "/polymake.ext\n"
            } @{$self->requires};
         }
      }
      if (defined (my $requires=delete $sections{REQUIRE_OPT})) {
         $self->is_bundled
            or die "REQUIRE_OPT section is only supported for bundled extensions.\n";
         my @req = $requires =~ /(\S+)/g;
         push @{$self->requires_opt}, @req;
      }

      if (defined (my $credit=delete $sections{CREDIT})) {
         $credit =~ s/\A(?:[ \t]*\n)*//s;
         $credit =~ s/(?<=\n)(?:[ \t]*\n)*\Z//s;
         $credit =~ s/^[ \t]*/  /gm;
         if (defined($self->short_name)) {
            $credit .= "\n  ".$self->URI;
         }
         $self->credit=new Credit($self, $credit);
      }
      if (defined (my $replaces=delete $sections{REPLACE})) {
         @{$self->replaces} = $replaces =~ /(\S+)/g;
      }
      if (defined (my $conflicts=delete $sections{CONFLICT})) {
         @{$self->conflicts} = $conflicts =~ /(\S+)/g;
      }

      if (keys %sections) {
         warn_print( "Extension description file ", $self->dir, "/polymake.ext contains unknown or obsolete section(s): ",
                     join(", ", keys %sections) );
      }

   } elsif ($self->is_bundled) {
      die "Bundled extension ", $self->dir, " lacks description file polymake.ext\n";

   } elsif (-f $self->dir."/URI") {
      open my $U, $self->dir."/URI";
      my $URI=<$U>;
      process_URI($self, $URI);
      if (-w $self->dir) {
         require Polymake::Core::InteractiveCommands;
         write_initial_description($self);
         $self->untrusted=1;
      }

   } elsif ($self->URI eq "private:") {
      $self->untrusted=1;

   } else {
      if (defined($self->URI)) {
         die "The extension ", $self->dir, " does not have any URI while ", $self->URI, " was expected\n";
      } else {
         $self->URI="file://".$self->dir;
         require Polymake::Core::InteractiveCommands;
         write_initial_description($self);
         $self->untrusted=1;
      }
   }

   $self;
}

# private:
sub process_URI {
   my ($self, $URI)=@_;
   chomp $URI;
   if ($URI =~ s/\#([\d.]+)$//) {
      $self->version=$1;
      $self->version_num=eval "v$1";
   }
   $self->URI=$URI;
   delete $refused{$URI};
}
######################################################################################
# load all configured bundled and imported extensions
sub init {
   # create objects for bundled extensions
 LOAD:
   {
      # extensions are ordered by inter-dependencies, prerequisites coming first
      foreach my $name (@BundledExts) {
         my $ext=new(__PACKAGE__, "$InstallTop/bundled/$name", "bundled:$name", 1);
         $ext->short_name=$name;
         if ($DeveloperMode && $ext->meta_tm > $ConfigTime) {
            warn_print("meta-file of bundled extension $name has been changed since last configuration;\nPerforming automatic reconfiguration, please be patient...\n");
            my $build_opt= $ENV{POLYMAKE_BUILD_ROOT} && "--build $ENV{POLYMAKE_BUILD_ROOT}";
            my $config_log=`cd $InstallTop; $^X support/configure.pl $build_opt 2>&1`;
            if ($?) {
               die "Automatic reconfiguration failed, the complete log including the error diagnostics is shown below.\n",
                  "Please investigate the reasons and rerun the configure script manually, possibly with different options.\n\n",
                  $config_log;
            }
            my %ConfigFlags=load_config_file("$InstallArch/config.ninja", $InstallTop);
            $ConfigTime=(stat "$InstallArch/config.ninja")[9];
            @BundledExts=$ConfigFlags{BundledExts} =~ /(\S+)/g;
            @active=();
            redo LOAD;
         }
         $registered_by_URI{$_}=$ext for $ext->URI, @{$ext->replaces};
         $registered_by_dir{$ext->dir}=$ext;
         $ext->VCS=$CoreVCS;
         $ext->configured_at=max($ConfigTime, $Application::configured_at);
         my @bad;
         my @prereqs=map { $registered_by_URI{$_} // do { push @bad, $_; () } } @{$ext->requires};
         if (@bad) {
            die "Corrupted configuration of bundled extensions: prerequisites @bad appear after the dependent extension $name or are disabled.\n",
                "Please investigate and re-run the configure script.\n";
         }
         push @prereqs, grep { defined } map { $registered_by_URI{$_} } @{$ext->requires_opt};
         @{$ext->requires}=uniq(map { ($_, @{$_->requires}) } @prereqs) if @prereqs;
         push @active, $ext;
         $ext->is_active=1;
      }
   }

   # register the inactive bundled extensions in order to recognize them as prerequisites of other extensions and rulefiles
   foreach my $bundled_dir (glob("$InstallTop/bundled/*")) {
      unless (exists $registered_by_dir{$bundled_dir}) {
         my ($name)=$bundled_dir =~ $filename_re;
         my $ext=new(__PACKAGE__, $bundled_dir, "bundled:$name", 1);
         $registered_by_URI{$ext->URI}=$ext;
      }
   }

   $num_bundled=@active;

   # perform dependency checks on standalone extensions
   my @pending=map { new(__PACKAGE__, $_) } @User::extensions;
   my ($list_updated, @survived, %failed);

   my $rounds=0;
   while (@pending) {
      my @next_round;
      foreach my $ext (@pending) {
         if (defined (my $other=$registered_by_URI{$ext->URI})) {
            if ($other->URI eq $ext->URI) {
               warn_print( "Extensions ", $other->dir, " and ", $ext->dir, " have identical URI: ", $ext->URI, <<'.' );

The second one is removed from your settings.
If you need it, `forget' the first one and re-import the second one.
If this is in error, revise the extension description files polymake.ext
and assign distinct URIs.
.
            } else {
               warn_print( "Extension ", $ext->URI, " installed at ", $ext->dir, "\n",
                           "is declared as replaced by another extension ", $other->URI, " installed at ", $other->dir, "\n",
                           "The first one is therefore removed from your settings.\n",
                           "If this is in error, revise the REPLACE sections in the extension description files" );
            }
            $list_updated=1;
            next;
         }

         my $conflicting=$conflicts{$ext->URI};
         unless ($conflicting) {
            foreach (@{$ext->conflicts}) {
               $conflicting=$registered_by_URI{$_} and last;
            }
         }
         if ($conflicting) {
            warn_print( "Extension ", $ext->URI, " installed at ", $ext->dir, "\n",
                        "is declared as conflicting with another extension ", $conflicting->URI, " installed at ", $conflicting->dir, "\n",
                        "The first one is therefore removed from your settings.\n",
                        "If this is in error, revise the CONFLICT sections in the extension description files" );

            $failed{$_}="conflicts with other extensions" for $ext->URI, @{$ext->replaces};
            $list_updated=1;
            next;
         }

         my @prereqs;
         my $satisfied=1;
         for (@{$ext->requires}) {
            my $prereq_version= s/\#([\d.]+)$// && $1;
            if (defined (my $prereq=$registered_by_URI{$_})) {
               if (length($prereq_version)) {
                  if ($prereq->is_bundled) {
                     warn_print( "Extension ", $ext->URI, " installed at ", $ext->dir, "\n",
                                 "imposes a forbidden version requirement on a bundled extension ", $prereq->dir, "\n",
                                 "Please revise the REQUIRE section in the description file ", $ext->dir."/polymake.ext\n",
                                 "and re-import the extension.\n" );
                     $failed{$_}="wrong dependencies" for $ext->URI, @{$ext->replaces};
                     $list_updated=1;
                     $satisfied=0;  last;

                  } elsif ($prereq->URI ne $_) {
                     warn_print( "Extension ", $ext->URI, " installed at ", $ext->dir, "\n",
                                 "has as one of its prerequisites a versioned extension URI $_#$prereq_version\n",
                                 "which has been replaced by another extension ", $prereq->URI, " installed at ", $prereq->dir, "\n",
                                 "Version check is therefore skipped.\n",
                                 "To get rid of this warning, revise the REQUIRE section in the extension description file\n",
                                 "or consider upgrading this extension to a newer version if available." );

                  } elsif (!defined($prereq->version_num)
                             or
                           $prereq_version ne $prereq->version && eval("v$prereq_version") gt $prereq->version_num) {

                     warn_print( "Extension ", $ext->URI, " installed at ", $ext->dir, "\n",
                                 "has as one of its prerequisites a versioned URI $_#$prereq_version\n",
                                 "However, the extension installed at ", $prereq->dir,
                                 defined($prereq->version_num) ? "is of an older version ".$prereq->version : "does not specify any version", ".\n",
                                 "The first one is therefore removed from your settings.\n",
                                 "If this is in error, revise the REQUIRE section in the extension description file\n",
                                 "or try to upgrade the second one to a newer version if available and then re-import the first one" );

                     $failed{$_}="unsatisfied versioned dependencies" for $ext->URI, @{$ext->replaces};
                     $list_updated=1;
                     $satisfied=0;  last;
                  }
               }
               push @prereqs, $prereq, @{$prereq->requires};

            } elsif (defined (my $reason=$failed{$_})) {
               warn_print( "Extension ", $ext->URI, " installed at ", $ext->dir, "\n",
                           "depends on another extension $_ which could not be loaded due to $reason.\n",
                           "More details have been reported in the log above.\n",
                           "Re-import this extension when you have fixed the problem in the first place" );

               $failed{$_}="unsatisfied dependencies" for $ext->URI, @{$ext->replaces};
               $list_updated=1;
               $satisfied=0;  last;

            } else {
               push @next_round, $ext;
               $satisfied=0;  last;
            }
         }
         next unless $satisfied;

         $registered_by_dir{$ext->dir}=$ext;
         $registered_by_URI{$ext->URI}=$ext;
         my @old_replaces;
         foreach (@{$ext->replaces}) {
            if (defined (my $other=$registered_by_URI{$_})) {
               if ($other->URI eq $_) {
                  warn_print( "Extension $_ installed at ", $other->dir, "\n",
                              "is declared as replaced by extension ", $ext->URI, " installed at ", $ext->dir, "\n",
                              "and therefore removed from your settings.\n",
                              "If this is in error, revise the REPLACE section in the extension configuration file,\n",
                              "then re-import the removed extension." );

                  push @old_replaces, @{$other->replaces};
                  delete @registered_by_URI{@{$other->replaces}};
                  delete_from_list(\@survived, $other);
                  $list_updated=1;
               }
            }
            $registered_by_URI{$_}=$ext;
         }

         if (my @forgotten_replaces=grep { !exists $registered_by_URI{$_} } @old_replaces) {
            warn_print( "Extension(s) phased out as announced above provided additional compatibility URIs\n",
                        "which do not occur any more in the current extension ", $ext->dir, "\n",
                        (map { "  $_\n" } @forgotten_replaces),
                        "This may influence other extensions depending on them.\n",
                        "Should you get any messages about missing prerequisites having one of the URIs listed above,\n",
                        "either add them to the REPLACE section of the description file ", $ext->dir."/polymake.ext\n",
                        "or consider upgrading dependent extensions to newer versions, if available." );
         }

         $conflicts{$_}=$ext for @{$ext->conflicts};
         @{$ext->requires}=uniq(@prereqs) if @prereqs;
         push @survived, $ext;
      }
      last if @next_round == @pending;
      if (++$rounds > 1 && $PrivateDir) {
         warn_print( "Extension order in your settings \@User::extensions is incompatible with the inter-dependencies\n",
                     "between the extensions, they will be automatically reordered.\n",
                     "Please revise the results in $PrivateDir/prefer.pl after finishing the running polymake session." );
         $list_updated=1;
      }
      @pending=@next_round;
   }

   if (@pending) {
      my %would_provide=map { ($_ => 1) } map { $_->URI, @{$_->replaces} } @pending;
      foreach my $ext (@pending) {
         if (my @unknown=grep { !$would_provide{$_} } @{$ext->requires}) {
            warn_print( "Extension ", $ext->URI, " installed at ", $ext->dir, "\n",
                        "has unresolved prerequisite(s)\n",
                        ( map { "  $_\n" } @unknown ),
                        "Therefore it is removed from your settings.\n",
                        "If you need it, install the missing extensions and then re-import this one,\n",
                        "or revise the REQUIRE section in the extension description file ", $ext->dir."/polymake.ext" );
         } else {
            warn_print( "Extension ", $ext->URI, " installed at ", $ext->dir, "\n",
                        "has unresolved prerequisite(s)\n",
                        ( map { "  $_\n" } @{$ext->requires} ),
                        "The extension(s) providing them could not be loaded because of problems reported elsewhere in this log\n",
                        "or due to wrong cyclic dependencies between these extensions.\n",
                        "In the latter case, please revise the REQUIRE sections in the extension description files polymake.ext\n",
                        "and re-import the needed extensions in proper order induced by justified dependencies.\n" );
         }
      }
      $list_updated=1;
   }

   if ($list_updated) {
      @User::extensions=grep { exists $registered_by_dir{$_} } @User::extensions;
      $Prefs->custom->set('@extensions');
   }

   # filter out extensions disabled for this architecture

   foreach my $ext (@survived) {
      if ($ext->is_active = !$User::disabled_extensions{$ext->dir}) {
         foreach my $prereq (@{$ext->requires}) {
            unless ($prereq->is_active) {
               if ($prereq->is_bundled) {
                  warn_print( "Extension ", $ext->URI, " installed at ", $ext->dir, "\n",
                              "requires the bundled extension ", $prereq->URI, "\n",
                              "which is not configured for the current architecture $Arch.\n" );
               } else {
                  warn_print( "Extension ", $ext->URI, " installed at ", $ext->dir, "\n",
                              "depends on another extension ", $prereq->URI, " installed at ", $prereq->dir, "\n",
                              "which has been disabled for the current architecture $Arch\n",
                              "because of failed build configuration or further dependencies.\n" );
               }
               $ext->is_active=0;  last;
            }
         }
         unless ($ext->is_active &&= $ext->configure) {
            warn_print( "Extension ", $ext->URI, " disabled due to build configuration problems depicted above.\n",
                        "If you need it, investigate the reasons, install missing third-party software if instructed to do so,\n",
                        "then execute the polymake shell command\n",
                        "  reconfigure_extension(\"", $ext->dir, "\");" );
         }
         unless ($ext->is_active) {
            $Prefs->custom->set('%disabled_extensions', $ext->dir, 1);
         }
      }
      if ($ext->is_active) {
         push @active, $ext;
      }
   }

   if (my @obsolete=grep { !exists $registered_by_dir{$_} } keys %User::disabled_extensions) {
      delete @User::disabled_extensions{@obsolete};
      $Prefs->custom->set('%disabled_extensions');
   }
}
#######################################################################################
sub configure {
   my ($self, @options)=@_;
   my $ext_dir=$self->dir;
   my $ext_build_dir=$ext_dir;

   if ($ext_build_dir =~ s{^\Q${InstallTop}\E(?=/ext/)}{$InstallArch}o) {
      $self->build_dir=$ext_build_dir;
      if (@options == 1 && $options[0] eq "--help") {
         warn_print( "Extension $ext_dir is already built and installed, it does not need any further configuration." );
      } elsif (-f "$ext_build_dir/config.ninja") {
         $self->configured_at=max((stat _)[9], $Application::configured_at);
         @options and warn_print( "Extension $ext_dir is already built and installed, ignoring configuration options @options." );
         1
      } else {
         warn_print( <<"." );
Installed extension $ext_dir seems to be not prepared for your current architecture $Arch.
Please import its source tree, configure, build, and re-install it,
or ask the person who has installed it in the first place for assistance.
For the meanwhile, the extension will stay disabled for architecture $Arch.
.
         0
      }
   } else {
      set_build_dir($self);
      unless (@options) {
         if (-f (my $build_file=$self->build_dir."/build.ninja")) {
            my $conf_tm=(stat _)[9];
            $self->configured_at=max($conf_tm, $Application::configured_at);
            if ($conf_tm >= $self->meta_tm and open my $bf, $build_file) {
               local $_;
               while (<$bf>) {
                  # the core system configuration file is included first:
                  # does it match the current system?
                  if (m{^\s*include\s+(\S+)/config.ninja\s*$}) {
                     return 1 if $1 eq $InstallArch;
                     last;
                  }
               }
            }
         }
      }
      require Polymake::Configure;
      my $err=Configure::configure_extension(@_);
      if (length($err)) {
         if ($err ne "silent\n") {
            warn_print( "Build configuration of extension $ext_dir failed with following error(s):\n$err\n" );
         }
         0
      } else {
         1
      }
   }
}

sub set_build_dir {
   my ($self)=@_;
   # workspace polymake and workspace extension should use the same build directory names
   if ($InstallArch =~ m{/build(?:\.[^/]+)?/\w+$}) {
      $self->build_dir=$self->dir.$&;
   } else {
      $self->build_dir=$self->dir."/build/".($ENV{POLYMAKE_BUILD_MODE} || "Opt");
   }
}
########################################################################################
sub lookup {
   (undef, my $what)=@_;
   my $by_dir;
   unless ($what =~ s{^($id_re)$}{bundled:$1}o) {
      replace_special_paths($what);
      $by_dir=-d $what and $what=Cwd::abs_path($what);
   }
   if ($by_dir) {
      $registered_by_dir{$what} || croak( "Extension at location $what is not registered" )
   } else {
      $registered_by_URI{$what} || croak( "Unknown extension URI $what" )
   }
}

sub app_dir { $_[0]->dir."/apps/".$_[1]->name }

sub versioned_URI {
   my $self=shift;
   $self->URI . (defined($self->version) && "#".$self->version)
}
#####################################################################################

# Looks for an extension specified by its URL, imports it if necessary.
# @return the Extension object in the successful case
# @return undef if not found or couldn't be loaded
# @return 0 if the user chose to ignore the request
# Throws an exception if $mandatory and no Extension loaded.

sub provide {
   my ($pkg, $URI, $mandatory)=@_;
   if (defined (my $ext=$registered_by_URI{$URI})) {
      if ($ext->is_active) {
         $ext;
      } elsif ($mandatory) {
         die "Extension $URI is not configured for the current architecture $Arch.\n",
             "Activate it by polymake shell command\n",
             "  reconfigure_extension(\"$URI\");\n",
             "Then try to reload your data.\n";
      } else {
         warn_print( "Extension $URI is not configured for the current architecture $Arch.\n",
                     "The properties defined in this extension will be missing in your object.\n",
                     "If you need them, activate the extension by polymake shell command\n",
                     "  reconfigure_extension(\"$URI\");\n",
                     "then reload the object from the file backup copy." );
         undef;
      }

   } elsif ($URI =~ m|^file://| and -d (my $ext_dir=$')) {
      require Polymake::Core::InteractiveCommands;
      eval { add($pkg, $ext_dir) } or do {
         if ($mandatory) {
            die "Error importing extension from $ext_dir:\n$@";
         } else {
            warn_print( "Extension at $ext_dir could not be loaded:\n$@",
                        "The properties defined in this extension will be missing in your object.\n",
                        "If you need them, investigate the reasons of the failure,\n",
                        "import the extension with polymake shell commmand\n",
                        "  import_extension(\"$ext_dir\");\n",
                        "then reload the object from the file backup copy." );
            undef;
         }
      }

   } elsif (! $Shell->interactive) {
      if ($mandatory) {
         die "Extension $URI is required for loading the data.\n";
      } else {
         warn_print( "Extension $URI is not known.\n",
                     "The properties defined in this extension will be missing in your object.\n" );
         undef;
      }

   } elsif (!exists $refused{$URI} && defined ($ext=&locate_unknown)) {
      $ext;

   } elsif ($refused{$URI} eq "ignore") {
      0;

   } elsif ($refused{$URI} eq "stop" || $mandatory) {
      die "Extension $URI is required for loading the data.\n";

   } else {
      undef;
   }
}
#####################################################################################
# private:
sub activate {
   my ($self)=@_;
   local $Application::extension=$self;
   $self->is_active=1;
   foreach my $app_dir (glob($self->dir."/apps/*")) {
      $app_dir =~ $filename_re;
      if (defined (my $app=lookup Application($1))) {
         $app->load_extension($app_dir);
      }
   }
   push @active, $self;
}
#####################################################################################
sub get_source_VCS {
   my $self=shift;
   $self->VCS ||= do {
      if ($self->is_bundled || index($self->dir, "$InstallTop/ext/")==0) {
         die "Can't modify an installed bundled extension\n";
      } elsif (!-w $self->dir) {
         die "You don't have permission to create or change files in ", $self->dir, "\n";
      } else {
         require Polymake::SourceVersionControl;
         new SourceVersionControl($self->dir);
      }
   }
}
#####################################################################################
package _::Credit;

use Polymake::Struct (
   [ '@ISA' => 'Rule::Credit' ],
   [ '$product' => '""' ],
);

sub new {
   my $self=&_new;
   if ($_[0]->is_bundled) {
      ($self->product)= $_[0]->URI =~ /:($id_re)$/o;
   } else {
      $self->product=defined($_[0]->short_name)?$_[0]->short_name : $_[0]->URI;
   }
   bless $self, "Polymake::Core::Rule::Credit";
}

sub display {
   my $self=shift;
   dbg_print( "used extension ", $self->product, "\n", $self->text, "\n" );
   $self->shown=1;
}

sub toFileString {
   my $self=shift;
   $self->file_string ||= do {
      my ($copyright)= $self->text =~ /(copyright\b.*)/im;
      (defined($copyright) ? "\n$copyright\n" : $self->text) . $self->product . "\n"
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
