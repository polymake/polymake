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

package Polymake::Core::UserSettings;

use Polymake::Struct (
   [ new => '$@' ],
   [ '$mode' => '#1' ],
   [ '@sources' => '@' ],
   '%items',
   '$changed',
);

sub new {
   my $self = &_new;
   $self->changed ||= $self->mode eq "private" && $self->sources->[-1]->{version} ne $Version;
   $self
}

sub load_source_file {
   my ($filename, $is_legacy, $init_script) = @_;
   open my $F, $filename or die "can't read $filename: $!\n";
   local $/;
   local $_ = <$F>;
   my $data = !$is_legacy && eval { JSON->new->utf8->with_comments->decode($_) };
   unless (is_hash($data)) {
      if ($is_legacy || $@ and /\A (?: ^\s* (?:\#.*)? \n)* \s* (?: [\$\@\%] | ARCH | package | application)\b/xm) {
         require Polymake::Core::CustomizetoJSON;
         $data = eval { CustomizetoJSON::convert_file($_, $init_script) };
      }
      unless (is_hash($data)) {
         die $@ ? "corrupted settings file $filename: $@\n"
                : "invalid settings file $filename: level is not a JSON object\n";
      }
   }
   $data
}

########################################################################
package Polymake::Core::UserSettings::Item;

use Polymake::Struct (
   [ new => '$$$%' ],
   [ '$ref' => '#1' ],              # reference to the application value
   [ '$comment' => '#2' ],          # from the rules
   [ '$flags' => '#3' ],            # Flags
   [ '$default' => 'undef' ],       # value from the rules
   [ '$reset_to' => 'undef' ],      # from imported configuration or =default

   [ '$extension' => '#%', default => 'undef' ],
   [ '$exporter' => '#%', default => 'undef' ],
);

use Polymake::Enum Flags => {
   by_arch => 1,       # has architecture-dependent values
   accumulating => 2,  # concatenate values from several settings files instead of overwriting
   hidden => 4,        # do not support 'reset', nor expose through interactive help
   is_custom => 8,     # value is set by user or auto-configuration
   has_imported => 16, # reset_to is taken from imported settings
   is_changed => 32,   # value changed after loading of settings files
   no_export => 64     # don't export in shared settings files
};

sub find_value {
   my ($self, $key, $sources, $is_private) = @_;
   $key .= "#$Arch" if $self->flags & Flags::by_arch;
   my $where = $is_private;
   for my $source (@$sources) {
      if (exists $source->{$key}) {
         if ($where == @$sources) {
            $self->flags |= Flags::is_custom;
            return $source->{$key};
         }
         $self->flags |= Flags::has_imported;
         $self->reset_to = $source->{$key};
      }
      ++$where;
   }
   $is_private ? &find_arch_changed : ()
}

# did a refactoring happen that changed the by-arch flag?
# only look at private settings, if any
sub find_arch_changed {
   my ($self, $key, $sources) = @_;
   if ($self->flags & Flags::by_arch) {
      if (exists $sources->[-1]->{$key}) {
         $self->flags |= Flags::is_custom | Flags::is_changed;
         return delete $sources->[-1]->{$key};
      }
   } else {
      if (exists $sources->[-1]->{"$key#$Arch"}) {
         $self->flags |= Flags::is_custom | Flags::is_changed;
         return delete $sources->[-1]->{"$key#$Arch"};
      }
   }
   ()
}

sub split_comments {
   my ($text) = @_;
   $text =~ /\S/ ?  [ map { "# $_" } $text =~ /^\s*(.*)/mg ] : undef
}

sub get_comment {
   my ($self) = @_;
   if (is_object($self->comment)) {
      split_comments($self->comment->text)
   } else {
      sanitize_help(my $text = $self->comment);
      split_comments($text)
   }
}

########################################################################
package Polymake::Core::UserSettings::Item::Scalar;

use Polymake::Struct (
   [ '@ISA' => 'Item' ],
);

sub init_value {
   my ($self) = @_;
   $self->default = ${$self->ref};
   if (my ($value) = &find_value) {
      ${$self->ref} = deserialize($self, $value);
   } elsif ($self->flags & Flags::has_imported) {
      ${$self->ref} = deserialize($self, $self->reset_to);
   }
   if (is_object($self->default) && UNIVERSAL::can($self->default, ".type")) {
      readonly_deref($self->default);
   }
}

sub set_value {
   my ($self, $value) = @_;
   if (is_object($self->default) && defined($value) &&
       !(is_object($value) && UNIVERSAL::isa($value, ref($self->default)))) {
      croak("invalid value type for a non-primitive custom scalar variable");
   }
   ${$self->ref} = $value;
}

sub set_local_value {
   my ($self, $scope, $value) = @_;
   if (is_object($self->default) && defined($value) &&
       !(is_object($value) && UNIVERSAL::isa($value, ref($self->default)))) {
      croak("invalid value type for a non-primitive custom scalar variable");
   }
   local with($scope->locals) {
      local scalar ${$self->ref} = $value;
   }
}

sub reset_value {
   my ($self) = @_;
   ${$self->ref} = $self->flags & Flags::has_imported ? deserialize($self, $self->reset_to) : $self->default;
}

sub deserialize {
   my ($self, $value) = @_;
   if (is_object($self->default)) {
      if (my $type = UNIVERSAL::can($self->default, ".type")) {
         if (is_string($value)) {
            readonly_deref((&$type)->parse->($value))
         } else {
            readonly_deref((&$type)->deserialize->($value))
         }
      } elsif (my $deserialize = UNIVERSAL::can($self->default, "deserialize")) {
         &$deserialize($value)
      } else {
         croak("don't know how to deserialize a settings item of type ", ref($self->default));
      }
   } else {
      $value
   }
}

sub serialize {
   my ($self) = @_;
   my $value = ${$self->ref};
   if (is_object($value)) {
      if (defined(my $type = UNIVERSAL::can($value, ".type"))) {
         (&$type)->serialize->($value, { names => false })
      } elsif (defined(my $to_json = UNIVERSAL::can($value, "TO_JSON"))) {
         $to_json->($value)
      } else {
         err_print("don't know how to serialize settings item of type ", ref($value), ", replacing with 'null'");
         undef
      }
   } else {
      $value
   }
}

########################################################################
package Polymake::Core::UserSettings::Item::Array;

use Polymake::Struct (
   [ '@ISA' => 'Item' ],
);

sub init_value {
   my ($self, $key, $sources, $is_private) = @_;
   if ($self->flags & Flags::accumulating) {
      $self->reset_to = scalar @{$self->ref};
      $key .= "#$Arch" if $self->flags & Flags::by_arch;
      my $where = $is_private;
      for my $source (@$sources) {
         my $value = $source->{$key};
         if (is_array($value) && @$value > 0) {
            push @{$self->ref}, @$value;
            if ($where == @$sources) {
               $self->flags |= Flags::is_custom;
            } else {
               $self->flags |= Flags::has_imported;
               $self->reset_to = scalar @{$self->ref};
            }
         }
         ++$where;
      }
      if ($is_private and !($self->flags & Flags::is_custom) and my ($value) = &find_arch_changed) {
         push @{$self->ref}, @$value;
      }
   } else {
      if (my ($value) = &find_value) {
         $value = deep_copy_list($value);
         swap_deref($self->ref, $value);
         unless ($self->flags & Flags::hidden) {
            $self->reset_to //= $value;
         }
      } elsif ($self->flags & Flags::has_imported) {
         if ($self->flags & Flags::hidden) {
            swap_deref($self->ref, $self->reset_to);
         } else {
            swap_deref($self->ref, deep_copy_list($self->reset_to));
         }
      } elsif (not $self->flags & Flags::hidden) {
         $self->reset_to = deep_copy_list($self->ref);
      }
   }
}

sub set_value {
   my ($self, $value) = @_;
   if (is_array($value)) {
      if ($self->flags & Flags::accumulating) {
         splice @{$self->ref}, $self->reset_to, @{$self->ref} - $self->reset_to, @$value;
      } else {
         @{$self->ref} = @$value;
      }
   } else {
      croak("invalid value for a custom array variable");
   }
}

sub set_local_value {
   my ($self, $scope, $value) = @_;
   local with($scope->locals) {
      local ref $self->ref = $value;
   }
}

sub reset_value {
   my ($self) = @_;
   if ($self->flags & Flags::accumulating) {
      $#{$self->ref} = $self->reset_to - 1;
   } elsif (is_array($self->reset_to)) {
      swap_deref($self->ref, deep_copy_list($self->reset_to));
   } else {
      @{$self->ref} = ();
   }
}

sub serialize {
   my ($self) = @_;
   if ($self->flags & Flags::accumulating and $self->reset_to > 0) {
      [ @{$self->ref}[$self->reset_to..$#{$self->ref}] ]
   } else {
      $self->ref
   }
}

########################################################################
package Polymake::Core::UserSettings::Item::Hash;

use Polymake::Struct (
   [ '@ISA' => 'Item' ],
   [ '$importer' => '#%', default => 'undef' ],
);

sub init_value {
   my ($self, $key, $sources, $is_private) = @_;
   $key .= "#$Arch" if $self->flags & Flags::by_arch;
   unless ($self->flags & Flags::hidden) {
      $self->default = $self->reset_to = deep_copy_hash($self->ref);
   }
   my $where = $is_private;
   for my $source (@$sources) {
      my $value = $source->{$key};
      if (is_hash($value)) {
         if ($where == @$sources) {
            if (($self->flags & (Flags::has_imported | Flags::hidden)) == Flags::has_imported) {
               $self->reset_to = deep_copy_hash($self->ref);
            }
            $self->flags |= Flags::is_custom;
         } else {
            $self->flags |= Flags::has_imported;
         }
         if (defined($self->importer)) {
            $self->importer->($value, $where < @$sources);
         } else {
            push %{$self->ref}, %$value;
         }
      }
      ++$where;
   }
   if ($is_private and ($self->flags & (Flags::is_custom | Flags::has_imported | Flags::hidden)) == Flags::has_imported) {
      $self->reset_to = deep_copy_hash($self->ref);
      if (my ($value) = &find_arch_changed) {
         if (defined($self->importer)) {
            $self->importer->($value, false);
         } else {
            push %{$self->ref}, %$value;
         }
      }
   }
}

sub set_value {
   if (@_ == 3) {
      my ($self, $key, $value) = @_;
      if (defined($value)) {
         $self->ref->{$key} = $value;
      } else {
         delete $self->ref->{$key};
      }
   } else {
      my ($self, $value) = @_;
      if (is_hash($value)) {
         %{$self->ref} = %$value;
      } else {
         croak("invalid value for a custom hash variable");
      }
   }
}

sub set_local_value {
   if (@_ == 4) {
      my ($self, $scope, $key, $value) = @_;
      local with ($scope->locals) {
         if (defined($value)) {
            local $self->ref->{$key} = $value;
         } else {
            delete local $self->ref->{$key};
         }
      }
   } else {
      my ($self, $scope, $value) = @_;
      local with ($scope->locals) {
         local ref $self->ref = $value;
      }
   }
}

sub reset_value {
   my $self = shift;
   if (@_) {
      for my $key (@_) {
         if (is_hash($self->reset_to) && exists $self->reset_to->{$key}) {
            $self->ref->{$key} = $self->reset_to->{$key};
         } else {
            delete $self->ref->{$key};
         }
      }
   } elsif (is_hash($self->reset_to)) {
      swap_deref($self->ref, deep_copy_hash($self->reset_to));
   } else {
      %{$self->ref} = ();
   }
}

sub serialize {
   my ($self) = @_;
   &prepare_comments;
   if (is_hash($self->reset_to) && keys %{$self->reset_to}) {
      my %export;
      while (my ($key, $value) = each %{$self->ref}) {
         unless (exists $self->reset_to->{$key} && equal_nested_elements($value, $self->reset_to->{$key})) {
            $export{$key} = $value;
            if (defined(my $key_comment = split_comments($self->comment->{$key}))) {
               JSON::XS::attach_comments($export{$key}, $key_comment);
            }
         }
      }
      keys %export ? \%export : ()
   } else {
      $self->ref;
   }
}

sub get_comment {
   &prepare_comments;
   split_comments($_[0]->comment->{""})
}

sub prepare_comments {
   my ($self) = @_;
   unless (is_hash($self->comment)) {
      my $topic = $self->comment;
      $self->comment = { };
      if (is_object($topic)) {
         $self->comment->{""} = $topic->text;
         if (defined(my $key_topics = $topic->annex->{keys})) {
            $self->comment->{$_->name} = $_->text for @$key_topics;
         }
      } else {
         sanitize_help($topic);
         ($self->comment->{""}, my @key_parts) = split /^\s*\@key/m, $topic;
         foreach (@key_parts) {
            if (/^\s+ (?'name' (?!\d)[\w-]+) \s+ (?(?=\[) $balanced_re \s* )? $type_re \s+ /xo) {
               $self->comment->{$+{name}} = $';
            }
         }
      }
   }
}

########################################################################
package Polymake::Core::UserSettings;

use Polymake::Ext;

sub add_item {
   my ($self, $key, $ref) = splice @_, 0, 3;
   my $item;
   if (is_scalar_ref($ref)) {
      $item = new Item::Scalar($ref, @_);
   } elsif (is_array($ref)) {
      $item = new Item::Array($ref, @_);
   } elsif (is_hash($ref)) {
      $item = new Item::Hash($ref, @_);
   } else {
      croak("settings item $key must refer to a scalar, array or hash");
   }

   ($self->items->{$key} &&= croak("multiple settings items with equal keys $key")) ||= $item;
   $item->init_value($key, $self->sources, $self->mode eq "private");
   $self->changed ||= $item->flags & Item::Flags::is_changed;
   add_change_monitor($self, $item, $ref);
}

########################################################################
my $private_preamble = <<'.';
#  Your personal polymake configuration is persisted here.
#
#  This file is not intended for manual editing.  Please use instead appropriate polymake commands:
#
#   * set_custom / reset_custom  for manipulating customizable variables
#   * reconfigure / unconfigure  for configuring locations and flags for third-party software used in rules
#   * prefer / reset_preference  for choosing preferred rules and user functions for certain tasks
#
#  In case of disaster recovery, please make sure that no polymake session is running when you edit this file.
#  The contents must be valid JSON with the sole relaxation that comment lines are accepted, like this preamble.

.

sub save {
   my ($self, $private_file) = @_;
   my $out = $self->sources->[-1];
   my $version_changed = $out->{version} ne $Version;
   if ($version_changed) {
      # TODO: remove obsolete entries
      # but: how to distinguish when not all applications or extensions loaded in this session?
      $out->{version} = $Version;
   }

   while (my ($key, $item) = each %{$self->items}) {
      if ($item->flags & Item::Flags::is_changed) {
         $key .= "#$Arch" if $item->flags & Item::Flags::by_arch;
         if ($item->flags & Item::Flags::is_custom and
             my ($value) = $item->serialize) {
            $out->{$key} = $value;
            if (defined(my $comment = $item->get_comment)) {
               JSON::XS::attach_comments($out->{$key}, $comment);
            }
         } else {
            delete $out->{$key};
         }
      }
   }

   my ($F, $F_k) = new OverwriteFile($private_file);
   print $F $private_preamble;
   JSON->new->utf8->canonical->indent->space_after->with_comments->write($out, $F);
   close $F;
   $self->changed = false;
}

########################################################################
my $excerpt_preamble = <<'.';
#  This file contains an excerpt of polymake configuration.
#  It can be used as a standalone read-only configuration or merged with users'
#  private settings using one of the following mechanisms:
#
#   * Storing its absolute path in the environment variable
#     export POLYMAKE_CONFIG_PATH="/absolute/path;user"
#   * Specify its absolute path on the command line
#     polymake --config-path "/absolute/path;user"
#

.

sub save_excerpt {
   my ($self, $file, $include_imported, $suppress) = @_;
   my %out = (version => $Version);

   while (my ($key, $item) = each %{$self->items}) {
      if ($item->flags & ($include_imported ? Item::Flags::is_custom | Item::Flags::has_imported : Item::Flags::is_custom)
          and
          not($item->flags & Item::Flags::no_export)
          and
          not(defined($suppress) && $key =~ $suppress)) {

         local if ($include_imported) {
            if ($item->flags & Item::Flags::accumulating) {
               local scalar $item->reset_to = 0;
            } elsif (is_hash($item->default) && $item->default != $item->reset_to) {
               local ref $item->reset_to = $item->default;
            }
         }

         if (my ($value) = defined($item->exporter)
                           ? $item->exporter->($include_imported, $item->reset_to)
                           : $item->serialize) {
            $key .= "#$Arch" if $item->flags & Item::Flags::by_arch;
            $out{$key} = $value;
            if (defined(my $comment = $item->get_comment)) {
               JSON::XS::attach_comments($out{$key}, $comment);
            }
         }
      }
   }

   my ($F, $F_k) = new OverwriteFile($file);
   print $F $excerpt_preamble;
   JSON->new->utf8->canonical->indent->space_after->with_comments->write(\%out, $F);
   close $F;
}

########################################################################
#
# Analyze the configuration mode and load the files if necessary.
#

sub init {
   my ($mode) = @_;
   my ($private_file, @sources);
   if ($mode ne "ignore" && $mode ne "none") {
      foreach (split /;/, $mode) {
         if ($_ eq "user") {
            if ($PrivateDir) {
               die "conflicting entries in configuration path: \"user\" follows $PrivateDir also designated as user's private directory.\n";
            }
            $PrivateDir = $ENV{POLYMAKE_USER_DIR} || "$ENV{HOME}/.polymake";
            next;
         }
         # this is a suspicious legacy magic word, probably not used anywhere
         if ($_ eq '@interactive') {
            require Polymake::Core::Help::Topic;
            next;
         }
         my $user = s/^user=//;
         my $location;
         if (/^\$/) {
            # looks like an environment var
            $location = $ENV{$'} or next;
         } else {
            $location = $_;
         }
         $location =~ s{^~/}{$ENV{HOME}/};

         if ($user) {
            ($PrivateDir &&= die "conflicting entries in configuration path: \"user=$_\" follows $PrivateDir also designated as user's private directory.\n")
              = $location;
         } elsif (-e $location) {
            if ($PrivateDir) {
               die "conflicting entries in configuration path: global location $location follows private directory $PrivateDir.\n";
            }
            if (-d _) {
               # legacy global files
               my @legacy = grep { -f $_ && -r _ } "$location/customize.pl", "$location/prefer.pl";
               if (@legacy) {
                  warn_print(<<".");
importing settings from legacy file(s) @legacy
Please consider their replacement with JSON settings using `export_configured` command.
.
                  push @sources, map { load_source_file($_, true) } @legacy;
               } else {
                  warn_print("Legacy configuration path entry $location without effect: neither customize.pl nor prefer.pl found there");
               }
            } elsif (-f _ && -r _) {
               push @sources, load_source_file($location);
            } else {
               die -f _ ? "insufficient access rights to import global configuration from $location\n"
                        : "invalid configuration path entry $location: must be a regular file or a directory\n";
            }
         } else {
            die "non-existing configuration path entry $location\n";
         }
      }

      if ($PrivateDir) {
         if (-e $PrivateDir) {
            unless (-d _) {
               die "$PrivateDir designated as a location for user's private configuration is not a directory\n";
            }
            unless (-w _ && -x _) {
               die "Insufficient access rights for private directory $PrivateDir;\n",
                   $mode =~ /(?:^|;)user(?:$|;)/ &&
                     "Please correct with `chmod' or set POLYMAKE_USER_DIR environment variable to a suitable location.\n";
            }
         } else {
            File::Path::mkpath($PrivateDir, 0, 0700);
            warn_print( "created private directory $PrivateDir" );
         }
         $private_file = "$PrivateDir/settings";
         if (-f $private_file) {
            push @sources, load_source_file($private_file);
         } else {
            my $private_data;
            my $legacy_custom_file = "$PrivateDir/customize.pl";
            if (-f $legacy_custom_file) {
               $private_data = load_source_file($legacy_custom_file, true);
            } else {
               $private_data = { };
            }
            my $legacy_prefs_file = "$PrivateDir/prefer.pl";
            if (-f $legacy_prefs_file) {
               my $private_prefs = load_source_file($legacy_prefs_file, true, "$PrivateDir/init.pl");
               push %$private_data, %$private_prefs;
            }
            push @sources, $private_data;
            if (keys %$private_data) {
               print STDERR <<".";

***** ATTENTION *****

Legacy configuration files
  $legacy_custom_file and $legacy_prefs_file
have been converted to JSON settings;
They will no longer be used and can safely be archived or deleted.

*********************

.
               if (-f "$PrivateDir/init.pl") {
                  $private_data->{"Polymake::User::init_script"} = "init.pl";
               }
            }
         }
         $mode = "private";
      } else {
         $mode = "imported";
      }
   }

   my $settings = new(__PACKAGE__, $mode, @sources);
   if ($mode eq "private") {
      add AtEnd("Settings", sub { save($settings, $private_file) if $settings->changed });
   }
   return $settings;
}

########################################################################
#
#  Drop all items defined in an extension
#
sub obliterate_extension {
   my ($self, $extension) = @_;
   while (my ($key, $item) = each %{$self->items}) {
      if ($item->extension == $extension) {
         delete $self->items->{$key};
         drop_change_monitor($item->ref);
         $self->changed = true;
      }
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
