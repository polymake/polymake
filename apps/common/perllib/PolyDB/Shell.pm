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
#   http://solros.de
#   http://www.mathematik.tu-darmstadt.de/~paffenholz
#


package PolyDB::Shell;

insert_before Core::Shell::Helper('function or method name or keyword argument',

   new Core::Shell::Helper(
      'db column name',

      qr{\b db_\w+ $args_start_re (?: $expression_re ,\s* )*
                   (?: db \s*=>\s* ($anon_quote_re)(?'db' $non_quote_re*)\g{-2} \s*,\s*)
                   (?: $id_re \s*=>\s* $expression_re ,\s* )*
                   (?: collection \s*=>\s* (?: $quote_re (?'prefix' $non_quote_re*))?) $}xo,

      sub {
         my ($shell)=@_;
         my ($db, $quote, $prefix) = @+{qw(db quote prefix)};
         if (defined $quote) {
            $shell->completion_proposals = [ list_col_completions($db, $prefix) ];
            $shell->completion_offset=length($prefix);
            $shell->term->Attribs->{completion_append_character}=$quote;
         } else {
            $shell->completion_proposals = ['"'];
         }
         1
      }
      # no specific context help provided
   ),

   new Core::Shell::Helper(
      'db name',

      qr{\b db_\w+ $args_start_re (?: $expression_re ,\s* )*
                   (?: db \s*=>\s* (?: $quote_re (?'prefix' $non_quote_re*))?) $}xo,

      sub {
         my ($shell)=@_;
         my ($quote, $prefix) = @+{qw(quote prefix)};
         if (defined $quote) {
            $shell->completion_proposals = [ list_db_completions($prefix) ];
            $shell->completion_offset=length($prefix);
            $shell->term->Attribs->{completion_append_character}=$quote;
         } else {
            $shell->completion_proposals = ['"'];
         }
         1
      }
      # no specific context help provided
   ),
);

sub list_db_completions {
   my ($prefix)=@_;
   grep { /^\Q$prefix\E/ } @{db_get_list_db()};
}

sub list_col_completions {
   my ($db, $prefix)=@_;
   grep { /^\Q$prefix\E/ } @{db_get_list_col_for_db(db=>$db)};
}

1;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
