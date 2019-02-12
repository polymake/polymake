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

package Polymake::Core::StoredScript;

use Polymake::Struct (
   [ 'new' => '$$$' ],
   [ '$path' => '#1' ],
   [ '$timestamp' => '#2' ],
   [ '&code' => '#3' ],
);

my %script_repo;

sub new {
   my $self=&_new;
   $script_repo{$self->path}=$self;
}

# PKG, filename =>
sub find {
   my $filename=pop @_;
   if (my ($path, $tm, $app)=locate_file($filename)) {
      if (defined (my $script=$script_repo{$path})) {
         if ($tm==$script->timestamp) {
            return $script->code;
         }
      }
      (undef, $path, $app);
   } else {
      croak( "script file \"$filename\" not found" );
   }
}

sub locate_file {
   my ($filename)=@_;
   my ($in_app, $allow_neutral)= $filename =~ s{^($id_re)::(?=[^/])}{}o
      ? (User::application($1), 0)
      : ($User::application, 1);
   my $full_path;
   if (defined $in_app) {
      foreach my $app ($in_app, values %{$in_app->imported}) {
         if (defined ($full_path=find_file_in_path($filename, $app->scriptpath))) {
            return ($full_path, (stat _)[9], $app);
         }
      }
   }
   if ($allow_neutral) {
      if (-f ($full_path="$InstallTop/scripts/$filename")) {
         return ($full_path, (stat _)[9]);
      }
      foreach my $ext (@Extension::active[$Extension::num_bundled..$#Extension::active]) {
         if (-f ($full_path=$ext->dir."/scripts/$filename")) {
            return ($full_path, (stat _)[9]);
         }
      }
      if (defined ($full_path=find_file_in_path($filename, \@User::lookup_scripts))) {
         return ($full_path, (stat _)[9]);
      }
      if (-f $filename) {
         my $tm=(stat _)[9];
         return (Cwd::abs_path($filename), $tm);
      }
   }
   ();
}


1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
