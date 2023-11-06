#  Copyright (c) 1997-2022
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
#   @author Andreas Paffenholz
#   (c) 2015 - 2023
#   https://polydb.org
#   https://www.mathematik.tu-darmstadt.de/~paffenholz
#


package Polymake::Core::Shell::Completion;

# completion of collection names
add Param( 'PolyDB::Collection',
   sub {
      my ($text, $ctx) = @_;

      my $db = $ctx->obj;
      my %options = (filter => '^' . ($text =~ s/\./\\./r) . '\\w+$');
      $options{only_sections} = true;
      my @proposals = map { "$_." } @{$db->get_allowed_collection_names(\%options)};
      $options{only_sections} = false;
      $options{only_collections} = true;
      
      push @proposals, @{$db->get_allowed_collection_names(\%options)};

      if ($text =~ /\./) {
         # partial input must be a section name;
         # if there are no sections below this level, try collection names
         if (!@proposals) {
            @proposals = @ { $db->get_allowed_collection_names(\%options,recursive=>1,only_collections=>true) };
         }
      } else {
         # either a top-level section name or an unambiguous collection name
         $options{filter} =~ s/^\^/\\./;
         $options{only_sections} = false;
         $options{only_collections} = false;
         $options{recursive} = 1;

         my @collections = @{$db->get_allowed_collection_names(\%options)};

         my (%in_sections, %in_default_section);
         foreach my $coll_name (@collections) {
            $coll_name =~ /\.(\w+)$/;
            $in_sections{$1}++;
            if (length($PolyDB::default::db_section_name) && index($`, $PolyDB::default::db_section_name) == 0) {
               $in_default_section{$1}++;
            }
         }

         push @proposals, (grep { $in_sections{$_} == 1 } keys %in_sections), (grep { $in_default_section{$_} == 1 } keys %in_default_section);
      }
      if ($proposals[0] =~ /\.$/) {
         # got section names among proposals, can't close the string
         $ctx->shell->completion_append_character = undef;
      }
      @proposals
   },
   flags => Param::Flags::quoted + Param::Flags::object);


# completion of property names in queries
add Param( 'PolyDB::query',
   sub {
      my ($text, $ctx) = @_;
      if ($ctx->keyword) {
         if ($ctx->group_keyword eq "sort_by") {
            grep { /^$text/ } "1", "-1"
         } elsif ($ctx->group_keyword eq "projection") {
            grep { /^$text/ } "1", "0"
         } else {
            # no completion for query filter values
            ()
         }
      } else {
         $ctx->obj->list_property_completions($text);
      }
   },
   flags => Param::Flags::quoted + Param::Flags::object);


1;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
