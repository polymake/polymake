#  Copyright (c) 1997-2020
#  Ewgenij Gawrilow, Michael Joswig, and the polymake team
#  Technische UniversitÃ¤t Berlin, Germany
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

package PolyDB::Schema;

sub rename_ref_keys {
   my ($src, $toMongo) = @_;
   if (is_hash($src)) {
      my %result;
      while (my ($key, $value) = each %$src) {
         my $new_key = $toMongo ? $key =~ s/^[\$]/__/r : $key =~ s/^__/\$/r;
         $result{$new_key} = ref($value) ? rename_ref_keys($value, $toMongo) : $value;
      }\%result
   } else {
      [ map { ref($_) ? rename_ref_keys($_, $toMongo) : $_ } @$src ]
   }
}

# Mongo misinterprets document keys '$ref' as DBrefs
sub schema2document {
   my ($schema) = @_;
   rename_ref_keys($schema, true);
}

sub document2schema {
   my ($schema) = @_;
   rename_ref_keys($schema, false);
}

1;

# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
