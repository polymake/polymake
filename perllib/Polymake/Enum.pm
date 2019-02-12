use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);

require "constant.pm";
require "Polymake/regex.pl";

package Polymake::Enum;

sub import {
   shift;
   my $pkg = caller;
   my $subpkg = shift;
   $pkg .= "::$subpkg" if $subpkg ne "_";
   $pkg =~ /^$qual_id_re$/o or croak( "Polymake::Enum - invalid package name $pkg" );

   my $consts;
   unless (@_ == 1 && is_hash($consts = $_[0])) {
      my $cnt = 0;
      $consts = { map { ($_ => $cnt++) } @_ };
      $cnt > 0 or croak( "usage:\nuse Polymake::Enum CLASSNAME => { ID => VALUE };\n  or\nuse Polymake::Enum CLASSNAME => qw( ID ... )" );
   }

   my $symtab = get_symtab($pkg, true);
   foreach my $id (keys %$consts) {
      namespaces::declare_const_sub($symtab, $id);
   }

   local caller $symtab;
   constant->import($consts);
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
