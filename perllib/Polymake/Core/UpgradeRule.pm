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
use feature 'state';

package Polymake::Core::UpgradeRule;

use Polymake::Struct (
   [ new => '$$$' ],
   [ '$type' => '#1' ],
   [ '$paths' => '#2' ],
   [ '$body' => '#3' ],
);

sub new {
   my $self = &_new;
   $self->paths &&= [ map { [ split /\./ ] } split /\s*\|\s*/, $self->paths ];

   unless (is_code($self->body)) {
      defined($self->paths)
        or croak("upgrade rule for an entire object must have a body");

      if ($self->body =~ /^delete$/) {
         $self->body = sub {
            my ($obj, $prop) = @_;
            delete $obj->{$prop};
            true
         };
      } elsif (my ($new_name) = $self->body =~ /^rename\s+($prop_name_re)$/) {
         $self->body = sub { Polymake::Upgrades::rename_property(@_, $new_name) };
      } else {
         croak("invalid upgrade rule shortcut " . $self->body);
      }
   }
   $self
}

# can be used in upgrade rules too
sub Polymake::Upgrades::rename_property {
   my ($obj, $old_prop_name, $new_prop_name) = @_;
   $obj->{$new_prop_name} = delete $obj->{$old_prop_name};
   if (my $attrs = $obj->{_attrs}) {
      if (defined(my $attr = delete $attrs->{$old_prop_name})) {
         $attrs->{$new_prop_name} = $attr;
      }
   }
   true
}

sub Polymake::Upgrades::move_property {
   my ($old_obj, $old_prop_name, $new_obj, $new_prop_name) = @_;
   $new_prop_name //= $old_prop_name;
   $new_obj->{$new_prop_name} = delete $old_obj->{$old_prop_name};
   if (my $attrs = $old_obj->{_attrs}) {
      if (defined(my $attr = delete $attrs->{$old_prop_name})) {
         $new_obj->{_attrs}->{$new_prop_name} = $attr;
      }
   }
   true
}

sub apply {
   my ($self, $obj, $attrs) = @_;
   if (defined($self->paths)) {
      my $cnt = 0;
      foreach my $path (@{$self->paths}) {
         my $prop = local pop @$path;
         my @subobj = ($obj);
         foreach my $subprop (@$path) {
            @subobj = map { is_array($_) ? @$_ : $_ } grep { defined } map { $_->{$subprop} } @subobj
              or last;
         }
         foreach my $subobj (@subobj) {
            if (exists($subobj->{$prop})) {
               my $rc = $self->body->($subobj, $prop);
               is_integer($rc) or die "upgrade rule ".$self->header." did not return an integer or boolean change indicator\n";
               $cnt += $rc;
            }
         }
      }
      $cnt
   } else {
      my $rc = $self->body->($obj, $attrs);
      is_integer($rc) or die "upgrade rule ".$self->header." did not return an integer or boolean change indicator\n";
      $rc
   }
}

sub header {
   my ($self) = @_;
   my $header = $self->type;
   if (defined($self->paths)) {
      $header .= "." . join(" | ", map { join(".", @$_) } @{$self->paths});
   }
   $header
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
