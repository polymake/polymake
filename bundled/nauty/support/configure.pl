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

@conf_vars=qw( NautySrc );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( nauty-src ) }=();
}


sub usage {
   print STDERR "  --with-nauty-src=PATH  Source directory of nauty.\n",
                "                         By default polymake will use the bundled nauty source files,\n",
                "                         for the minimal tarball this option allows using a custom\n",
                "                         directory containing the nauty sources.\n";
}

sub check_nauty {
   my $dir = @_ > 0 ? $_[0] : "bundled/nauty/external/nauty";
   -e "$dir/nauty-h.in"
}

sub proceed {
   my ($options)=@_;
   my $nautysrc;
   if (defined ($nautysrc=$options->{"nauty-src"})) {
      check_nauty($nautysrc) or
         die "Specified nauty source directory invalid, could not find 'nauty-h.in'.";
      $NautySrc = $nautysrc;
   } elsif (!check_nauty()) {
      die "Bundled nauty directory seems to be missing, to use the nauty interface\n",
          "with the minimal tarball please specify a nauty source directory\n",
          "via --with-nauty-src=PATH.";
   }

   return $NautySrc ? "$NautySrc" : "bundled";
}

