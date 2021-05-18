#  Copyright (c) 1997-2021
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

package Polymake::Core::CPlusPlus::PrivateWrappers;

use Polymake::Struct (
   [ '@ISA' => 'Extension' ],
   [ new => '$' ],
   [ '$URI' => '"private:"' ],
   '$update_me',
);

# increase this every time we want to enforce automatic deletion of private wrappers
my $generation=1;

sub init {
   my $self;
   my $id = id();
   if (defined(my $dir = $private_wrappers{$id})) {
      if (-d ($dir = "$PrivateDir/$dir")) {
         $self = new(@_, $dir);
         if ($self->version < $generation) {
            warn_print( "Removing outdated private wrappers in $dir" );
            File::Path::remove_tree($dir);
            delete $private_wrappers{$id};
            return;
         }
         $self->set_build_dir;

         my ($ext, @prereq);
         foreach my $prereq (@{$self->requires}) {
            if (defined ($ext=$registered_by_URI{$prereq})
                and $ext->is_active) {
               push @prereq, $ext;
            } else {
               ensure_update($self);
            }
         }

         @{$self->requires}=uniq( map { ($_, @{$_->requires}) } @prereq );
         if ($self->update_me) {
            create_build_trees($self);
         }
      } else {
         delete $private_wrappers{$id};
      }
   }
   $self;
}
#######################################################################################
sub create {
   my ($self, $dir);
   if ($PrivateDir) {
      if (-d "$PrivateDir/wrappers") {
         # remove pre-2.12.4 wrappers
         File::Path::remove_tree("$PrivateDir/wrappers");
         foreach (my ($key, $dir)=each %private_wrappers) {
            -d "$PrivateDir/$dir" || delete $private_wrappers{$key};
         }
      }
      my $seq;
      for ($seq=0; -d ($dir="$PrivateDir/wrappers.$seq"); ++$seq) {
         if (string_list_index([ values %private_wrappers ], "wrappers.$seq") < 0) {
            # stray orphaned directory
            File::Path::remove_tree($dir);
            last;
         }
      }
      $private_wrappers{id()} = "wrappers.$seq";
      File::Path::make_path($dir);
   } else {
      $dir = new Tempdir("till_exit");
   }
   File::Path::make_path("$dir/apps");
   $self=new(@_, $dir);
   $self->set_build_dir;
   create_build_trees($self);
   ensure_update($self);
   $self
}
#######################################################################################
sub ensure_prerequisites {
   my ($self, $extensions)=@_;
   my $try_list=$self;
   PropertyParamedType::set_extension($try_list, $extensions);
   if ($try_list != $self) {
      # $try_list->[0] == $self, cf. PropertyParamedType::set_extension.
      @{$self->requires}=uniq( @{$self->requires}, map { ($_, @{$_->requires}) } splice(@$try_list, 1) );
      ensure_update($self);
   }
}
#######################################################################################
sub strip_build_dir {
   $_[0] =~ s{/build(?:\.[^/]+)? \K /\w+$}{}xr;
}
sub id {
   strip_build_dir($InstallArch);
}
#######################################################################################
sub create_build_trees {
   my ($self)=@_;
   my $build_root= strip_build_dir($self->build_dir);
   -d $build_root or File::Path::make_path($build_root);
   require Polymake::Configure;
   my $conf_file="$build_root/config.ninja";
   my ($F, $F_k)=new OverwriteFile($conf_file);
   print $F "root=$InstallTop\n",
            "extroot=", $self->dir, "\n",
            "RequireExtensions=", $self->list_prerequisite_extensions, "\n",
            "CPPERLextraFlags= --ext-config \${config.file}\n";
   close $F;
   undef $F_k;
   Configure::load_config_vars();
   Configure::create_extension_build_trees($self, $build_root);
}
#######################################################################################
sub update_metafile {
   my ($self)=@_;
   my ($F, $F_k)=new OverwriteFile($self->dir."/polymake.ext");
   print $F <<".";
# This pseudo-extension collects automatically generated C++/perl glue code
# used for computing complex expressions occurred in interactive sessions and user scripts.
# This extension is architecture-specific, it is linked to polymake clients installed at
#   $InstallArch
# It can safely be deleted at any time except during running polymake session.

URI private:#$generation
.
   if (@{$self->requires}) {
      print $F "\nREQUIRE\n", map { ($_->URI, "\n") } @{$self->requires};
   }
   close $F;
}
#######################################################################################
sub ensure_update {
   my ($self)=@_;
   $self->update_me ||= do {
      if ($PrivateDir) {
         add AtEnd("Private:C++", sub { update_metafile($self); create_build_trees($self); });
      }
      1
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
