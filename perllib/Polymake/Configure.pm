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

#  Utilities needed only in rulefile CONFIGURE clauses and standalone extension configure scripts.

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);

# load the non-interactive basis routines first
use Polymake::ConfigureStandalone;

# the values of these variables are growing when extension configure script assigns something to them
my %augmented_vars=( CFLAGS=>1, CXXFLAGS=>1, CflagsSuppressWarnings=>1, LDFLAGS=>1, LIBS=>1 );

package Polymake::Configure;

if ($ENV{POLYMAKE_HOST_AGENT}) {
   *try_host_agent=sub {
      require "$Polymake::InstallTop/resources/host-agent/client.pl";
      HostAgent::call(@_);
   }
}

###############################################################################################
#  This is executed when polymake is called with --reconfigure
#  before any applications are loaded
sub prepare_reconfigure {
   $Core::Application::configured_at=time;
}

###############################################################################################
# $variable, "program", ..., { prompt => "Prompt", check => \&SUB }
# overwrites a simplified version loaded before

sub find_program {
   my $opts= ref($_[-1]) eq "HASH" ? pop : { };
   my @prognames=splice @_, 1;
   my $prompt=($opts->{prompt} ||= @prognames>1 ? "one of the programs (".join(",",@prognames).")" : "the program `@prognames'");

   if ($_[0] =~ m{^(?:env\s+)? /}x) {
      # absolute path
      if (verify_executable($_[0])) {
         if ($Shell->interactive) {
            print "Please confirm the path to $prompt or enter an alternative location:\n";
            $_[0]=enter_program($_[0]);
         }
         return $_[0];
      }
      undef $_[0];
   }

   my ($first_shot, $full_path, $error)=find_program_in_path(defined($_[0]) ? $_[0] : (), @prognames,
                                                             ref($opts->{check}) eq "CODE" ? $opts->{check} : ());

   if (defined $first_shot) {
      if ($Shell->interactive) {
         print "Please confirm the path to $prompt or enter an alternative location:\n";
         $_[0]=enter_program($full_path);
         return $_[0] if $_[0] ne $full_path;
      }
      if ($first_shot) {
         $full_path =~ $directory_of_cmd_re;
         $_[0]=substr($full_path, $+[1]+1);
      } else {
         $_[0]=$full_path;
      }
      $full_path;
   } elsif ($Shell->interactive) {
      if (defined $error) {
         my ($location)= $full_path =~ $directory_of_cmd_re;
         print "$prompt found at location $location, but did not meet the requirements: $error\n",
               "Please enter an alternative location or an empty string to abort.\n";
      } else {
         print "Could not find $prompt anywhere along your PATH.\n",
               "Please enter the full path or an empty string to abort.\n";
      }
      $_[0]=enter_program("");
   } else {
      undef $_[0];
   }
}

###############################################################################################
sub program_completion : method {
   my ($Shell)=@_;
   my $line=substr($Shell->term->Attribs->{line_buffer}, 0, $Shell->term->Attribs->{point});
   if ($line =~ m{^[/~]}) {
      $Shell->try_filename_completion("", $line);
      if ($#{$Shell->completion_proposals} < 0 && substr($line,0,1) eq "/") {
         # no local matches, try the host file system
         $Shell->completion_proposals=[ try_host_agent("complete $line") =~ /(\S+)/g ];
      }
   } elsif ($line =~ /\S/) {
      $Shell->completion_proposals=[ sorted_uniq(sort( map { /$filename_re/ }
        (grep { -x && -f _ } map { glob "$_/$line*" } split /:/, $ENV{PATH}),
        (try_host_agent("complete $line") =~ /(\S+)/g) )) ];
   }
   $Shell->completion_offset=length($line);
}
###############################################################################################
sub enter_program {
   my ($text)=@_;
   my $host_agent= $text =~ s/^(env\s+)// && $1;
   my %opts=(completion => \&program_completion,
             prompt => "program path",
             check =>
      sub {
         if (length($_[0]) == 0) {
            return;
         }
         if ($_[0] eq $text) {
            substr($_[0], 0, 0) .= $host_agent;
            return;
         }
         my ($executable)= $_[0] =~ m/^(\S+)/;
         if ($_[0] =~ m{^[/~]}) {
            replace_special_paths($_[0]);
            substr($_[0], 0, 0) .= $host_agent;
            if (verify_executable($_[0])) {
               return;
            }
            "File $executable does not exist or is not an executable program\nPlease enter a correct location"
         } else {
            my ($first_shot, $full_path)=find_program_in_path($_[0]);
            if (defined $first_shot) {
               if (!$first_shot) {
                  $_[0]=$full_path;
               }
               return;
            }
            "Program $executable not found anywhere along your PATH.\nPlease enter the full path"
         }
      },
   );
   $Shell->enter_string($text, \%opts);
}

###############################################################################################
# Read all variables configured for the current platform
# and make them available as scalars in packages Polymake::Configure and Polymake::Bundled::$BUNDLED.
sub load_config_vars {
   return if defined $Arch;
   return unless -f (my $conf_file="$Polymake::InstallArch/config.ninja");
   open my $CF, $conf_file
     or die "can't read $conf_file: $!\n";
   local $_;
   while (<$CF>) {
      if (/^\s* (?!root|extroot)(\w+) \s*=\s* (.*?) \s*$/x) {
         no strict 'refs';
         ${$1}=$2;
      } elsif (/^\s* bundled\.(\w+)\.(\w+) \s*=\s* (.*?) \s*$/x) {
         no strict 'refs';
         ${"Polymake::Bundled::${1}::${2}"}=$3;
      }
   }
}

###############################################################################################
# Read the configuration variables for the current extension and its prerequisites,
# return them by reference in a hash map.
sub load_extension_config_vars {
   return unless defined $Core::Application::extension;

   my %result;
   foreach my $ext ($Core::Application::extension, @{$Core::Application::extension->requires}) {
      my ($prefix, $conf_file);
      if ($ext->is_bundled) {
         $prefix="bundled\\.".$ext->short_name."\\.";
         $conf_file="$Polymake::InstallArch/config.ninja";
      } else {
         $prefix="(?!root|extroot|RequireExtensions|BuildModes)";
         $conf_file=$ext->build_dir."/config.ninja";
      }
      if (open my $CF, $conf_file) {
         local $_;
         while (<$CF>) {
            if (my ($name, $value)= /^\s* $prefix (\w+) \s*=\s* (.*?) \s*$/x) {
               if ($value !~ /\$\{super\.\w+\}/) {
                  $result{$name}=$value;
               }
            }
         }
      }
   }
   \%result;
}

###############################################################################################
sub create_extension_build_trees {
   my ($ext, $ext_build_root)=@_;
   local $BuildModes="$BuildModes $ENV{POLYMAKE_BUILD_MODE}" if index($BuildModes, $ENV{POLYMAKE_BUILD_MODE}) < 0;
   create_build_trees($Polymake::InstallArch, $ext_build_root,
                      include => [ "$Polymake::InstallArch/config.ninja",
                                   map { $_->is_bundled ? () : $_->build_dir."/config.ninja" } @{$ext->requires} ],
                      $Polymake::InstallArch =~ m{/build(\.[^/]+)?/\w+$}
                      ? (addvars => "PERL=$Config::Config{perlpath}\n") : ());
}

###############################################################################################
my $eval_pkg="Polymake::StandaloneExt";

# Extension => error message (if any occurred)
sub configure_extension {
   eval { &do_configure_extension };
   my $err=$@;
   wipe_package(\%Polymake::StandaloneExt::);
   $err
}

sub do_configure_extension {
   no strict 'refs';

   my $ext=shift;
   my $ext_dir=$ext->dir;
   my (%options, %allowed_options, %allowed_with);

   check_extension_source_conflicts($ext_dir, $Polymake::InstallTop, map { $_->dir } @{$ext->requires});

   if (-f "$ext_dir/configure.pl") {
      File::Path::make_path("$ext_dir/support");
      die "Extension ", $ext->URI, " still has an outdated configuration script $ext_dir/configure.pl\n",
          "Please revise the script, paying special attention to renamed variables,\n",
          "like CXXFLAGS and LIBS instead of CXXflags and Libs.\n",
          "Then move it into the `support' subdirectory and execute the polymake shell command\n",
          "  reconfigure_extension(\"", $ext->URI, "\");\n";
   }
   load_config_vars();
   my $wipe_old_stuff;
   my ($ext_build_root)= $ext->build_dir =~ m{^(.*)/[^/]+$};
   # extensions used with a mutable workspace polymake inherit all configured build modes;
   # when using an installed polymake built in a non-standard mode, that mode is enforced as well.
   local $BuildModes="Opt Debug" if !$DeveloperMode && $BuildModes eq "Opt";

   if (!-d $ext->build_dir) {
      # migrate the build directory from pre-ninja era, if possible
      if ($Polymake::InstallArch =~ m{/build/\w+$} || $Polymake::InstallArch !~ m{/build\.[^/]+/\w+$}) {
         # single-architecture workspace or installed polymake
         if (-d "$ext_build_root.$Arch") {
            # should fit the current architecture
            rename "$ext_build_root.$Arch", $ext_build_root
              and $wipe_old_stuff=1;
         } elsif ((my @others=glob("$ext_build_root.*")) == 1) {
            # try this one
            rename $others[0], $ext_build_root
              and $wipe_old_stuff=1;
         }
      }
      if ($wipe_old_stuff) {
         File::Path::remove_tree(grep { -d $_ } "$ext_build_root/lib", "$ext_build_root/apps");
      } else {
         File::Path::make_path($ext_build_root);
      }
   }

   shift if $_[0] eq "--force";   # a dummy option inserted by the user command reconfigure_extension()

   if (-f "$ext_dir/support/configure.pl") {
      my $load_error=eval "package $eval_pkg; do '$ext_dir/support/configure.pl'; \$\@ ";
      if ($load_error) {
         die "corrupt configuration script:\n$load_error\n";
      }
      &{"$eval_pkg\::allowed_options"}(\%allowed_options, \%allowed_with);

      if (!@_) {
         if (-f (my $conf_file="$ext_build_root/config.ninja")) {
            eval { retrieve_config_command_line($conf_file, \@_) };
         } elsif ($wipe_old_stuff && -f ($conf_file="$ext_build_root/conf.make")) {
            eval { retrieve_config_command_line($conf_file, \@_) };
            unlink $conf_file;
         }
      } elsif (@_==1 && $_[0] eq "--defaults") {
         pop @_;
      }
   }

   while (defined (my $arg=shift)) {
      # trim and allow empty arguments
      $arg =~ s/^\s+//;
      $arg =~ s/\s+$//;
      length($arg) or next;
      if ($arg eq "--help") {
         if (exists &{"$eval_pkg\::usage"}) {
            print "Extension ", $ext->URI, " can be configured using following options:\n\n";
            &{"$eval_pkg\::usage"}();
         } else {
            print "Extension ", $ext->URI, " does not have any configuration options.\n";
         }
         die "silent\n";

      } elsif (my ($with, $out, $name, $value)= $arg =~ /^--(with(out)?-)?([-\w]+)(?:=(.*))?$/) {
         if ($with ? exists $allowed_with{$name}
                   : exists $allowed_options{$name}
               and
             defined($out) || defined($value) || @_) {
            if (defined($out)) {
               $value=".none.";
            } else {
               $value=shift unless defined($value);
               $value =~ s{^~/}{$ENV{HOME}/};
               $value =~ s{/+$}{};
            }
            $options{$name}=$value;
            next;
         }
      } elsif ($arg =~ /^--build-modes=([,\w]+)$/) {
         if ($DeveloperMode) {
            print "build modes configured for the core polymake workspace apply to all extensions and can't be changed\n";
         } else {
            add_build_modes($1);
            next;
         }
      }
      if (exists &{"$eval_pkg\::usage"}) {
         print "Unrecognized option $arg;\nFollowing options are allowed:\n\n";
         &{"$eval_pkg\::usage"}();
      } else {
         print "Unrecognized option $arg;\nExtension ", $ext->URI, " does not have any configuration options.\n";
      }
      die "silent\n";
   }

   if (exists &{"$eval_pkg\::proceed"}) {
      &{"$eval_pkg\::proceed"}(\%options);
   }

   my $conf_file="$ext_build_root/config.ninja";
   open my $conf, ">", $conf_file
      or die "can't create $conf_file: $!\n";
   print $conf "# last configured with:\n", "configure.command=";
   write_config_command_line($conf, \%options, \%allowed_with);

   my $prereq_list=$ext->list_prerequisite_extensions;
   print $conf <<"---";

root=$Polymake::InstallTop
extroot=$ext_dir
RequireExtensions=$prereq_list
super.app.includes=\${app.includes}
app.includes=-I\${extroot}/include/app-wrappers -I\${extroot}/include/apps \${super.app.includes}
---
   if (!$DeveloperMode) {
      print $conf "BuildModes=$BuildModes\n";
   }
   if (@{"$eval_pkg\::conf_vars"}) {
      print $conf "\n";
      write_config_vars($eval_pkg, "", $conf, \%augmented_vars);
   }
   close $conf;

   create_extension_build_trees($ext, $ext_build_root);
   $ext->configured_at=time;
}

##############################################################################################
sub rewrite_config_files {
   my ($top_dir, $varname, $value)=@_;
   local $/;
   foreach my $conf_file (glob("$top_dir/build*/config.ninja")) {
      open my $conf, $conf_file
        or die "can't open configuration file $conf_file: $!\n";
      local $_=<$conf>;
      close $conf;

      s{^ [ \t]* \Q$varname\E [ \t]*=[ \t]* \K .*}{$value}xm
        or
      $_ .= "$varname=$value\n";

      my ($conf_new, $conf_k)=new OverwriteFile($conf_file);
      print $conf_new $_;
      close $conf_new;
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
