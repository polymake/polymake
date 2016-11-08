#  Copyright (c) 1997-2016
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

@make_vars=qw( );


sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
}

# no options currently
sub usage {
}


sub proceed {
   my ($options)=@_;
   foreach (qw(MongoDB)) {
      my $pkg=$_;
      my $warn;
      local $SIG{__WARN__}=sub { $warn=shift };
      eval "require $pkg;";
      if ($@) {
         if ($@ =~ /\ACan't locate /) {
            die <<".";
ERROR:   perl module $pkg required for bundled/polydb not found on your machine.
         If you have installed it in a non-standard location
         please add the path to the environment variable PERL5LIB.
.
         } else {
            $@ =~ / at .*? line \d+/m;
            die <<".";
ERROR:   perl module $pkg required for bundled/polydb seems to be unusable.
         An attempt to load it has failed because of the following:
         $`
.
         }
      }
   }

   return;
}
