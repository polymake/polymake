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
#
#  This file is part of the polymake database interface polyDB.
#
#   @author Silke Horn, Andreas Paffenholz
#   http://solros.de
#   http://www.mathematik.tu-darmstadt.de/~paffenholz
#

use strict;
use JSON;

package PolyDB::DatabaseHelpers;

sub add_properties {
   my ($flat_properties, $initial, $t) = @_;
   foreach (sort keys %$t ) {
      if ( ref($t->{$_}) eq "HASH" ) {
         next if defined($t->{$_}->{'temporary'});
         my @k = keys %{$t->{$_}};
         if ( $k[0] =~ /[A-Z]+/ or  $k[0] eq "attachments" ) {
            my $ini = $_ eq "attachments" ? $initial : $initial.".".$_;
            add_properties($flat_properties, $ini, $t->{$_});
         } else {
            my $props = " [";
            foreach ( sort keys %{$t->{$_}} ) {
               $props .= ", ".$_;
            }
            $props =~ s/, (.*)/$1/;
            $props .= "]";
            push @{$flat_properties}, $initial.".".$_." ".$props;
         }
      } else {
         if ( $t->{$_} != 0 ) {
            push @{$flat_properties}, $initial.".".$_;
         }
      }
   }
}


sub read_json {
   my ($file) = @_;

   my $json_file = do {
      open(my $fh,'<',$file) or die "cannot open file $file\n";
      local $/;
      <$fh>
   };
   my $json = JSON->new;
   return $json->decode($json_file);
}


sub get_keys_for_collection {
   my ($db,$collection,$options) = @_;
   my $local = $options->{local};
   my $client;
   unless(defined($client = $options->{client})) {
      $client   = Client::get_client($local, "", "");
   }
   my $output = $client->get_database($db)->run_command([
       "distinct" => "$PolyDB::default::db_type_information",
       "key"      => "key",
       "query"    => { 'col' => $collection }
    ]);
   return $output->{'values'};
}

1;


# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
