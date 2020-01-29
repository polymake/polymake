#  Copyright (c) 1997-2019
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
#
#  This file is part of the polymake database interface polyDB.
#
#   @author Silke Horn, Andreas Paffenholz
#   http://www.mathematik.tu-darmstadt.de/~paffenholz
#

package Polymake::User;

sub load_json {
   my ($filename) = @_;
   replace_special_paths($filename);

   my $json_file = do {
      open(my $fh,'<:encoding(UTF-8)',glob($filename)) or die "cannot open file $filename\n";
      local $/;
      <$fh>
   };
   my $json = JSON->new;
   return $json->decode($json_file);
}

sub save_json {
   my ($data, $file) = @_;

   open my $fh, ">", $file or die "cannot open file $file\n";
   Polymake::write_json($fh, $data);
}

1;

# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
