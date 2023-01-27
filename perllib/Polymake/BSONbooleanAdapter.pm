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

package Polymake::BSONbooleanAdapter;

use Polymake::Ext;

if (exists &MongoDB::BSON::_encode_bson) {
   prepare_XS(\&MongoDB::BSON::_encode_bson, \&MongoDB::BSON::_decode_bson,
              $MongoDB::BSON::_boolean_true, $MongoDB::BSON::_boolean_false);
} elsif (exists &BSON::XS::_encode_bson) {
   prepare_XS(\&BSON::XS::_encode_bson, \&BSON::XS::_decode_bson,
              $BSON::XS::_boolean_true, $BSON::XS::_boolean_false);
} elsif (exists &BSON::PP::_encode_bson) {
   prepare_PP(\&BSON::PP::_encode_bson);
   my $decode = \&BSON::PP::_decode_bson;
   *BSON::PP::_decode_bson = sub {
      local ref *boolean::boolean = sub { $_[0] != 0 };
      &$decode;
   };
} else {
   die "unknown BSON codec\n";
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
