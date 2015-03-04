#  Copyright (c) 1997-2015
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

package Polymake;

# additional directories to try => first writable directory for tempfiles
sub find_writable_dir {
   foreach ($ENV{TMPDIR}, "/tmp", "/var/tmp", @_, ".") {
      return $_ if defined($_) && -d $_ && -w _ && -x _;
   }
   die <<'.';
Cannot find a writable directory for temporary files.
Please set TMPDIR environment variable to a suitable path.
.
}

# "filename" (relative or absolute), "dir", ... => full_path || undef
sub find_file {
   my $filename=shift;
   if (-f $filename) {
      Cwd::abs_path($filename)
   } else {
      my $full_path;
      foreach my $dir (@_) {
         -f ($full_path="$dir/$filename") and return $full_path;
      }
      undef
   }
}

# "filename" (relative), [ "dir", ... ] => full_path || undef
sub find_file_in_path {
   my ($filename, $dirs)=@_;
   my $full_path;
   foreach my $dir (@$dirs) {
      -f ($full_path="$dir/$filename") and return $full_path;
   }
   undef
}

# replace ~ with $HOME and remove trailing slashes
sub replace_special_paths {
   foreach (@_) {
      s|^~(?=/)|$ENV{HOME}|;
      s|(?<!^)/$||;
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
